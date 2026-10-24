#include "field/centroid_cascade.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "engine/transfer.h"
#include "field/activity.h"
#include "field/field.h"
#include "field/prepared_storage.h"

namespace field::detail {
namespace {
using namespace taichi::lang;
enum NodeColumn {
  RefBegin,
  RefCount,
  ChildBegin,
  ChildCount,
  Field,
  Level,
  Publish,
  Bits,
  Columns
};
enum Counter {
  Error,
  References,
  Fields,
  Reductions,
  Nodes,
  AwakeReferences,
  BadNodes,
  Reserved
};
constexpr int source_error = 1, numerical_error = 2;
bool same(const engine::RecordHandle &a, const engine::RecordHandle &b) {
  return a.context == b.context && a.incarnation == b.incarnation &&
         a.address == b.address && a.record == b.record;
}
std::size_t physical(std::uint64_t n) {
  return std::max<std::uint64_t>(n, 1);
}
bool span(std::uint64_t first, std::uint64_t n, std::size_t total) {
  return first <= total && n <= total - first;
}
template <class T>
bool aliases(const T *input, std::size_t n, const std::vector<T> &owned) {
  if (!input || !n || owned.empty())
    return false;
  const auto a = reinterpret_cast<std::uintptr_t>(input),
             b = reinterpret_cast<std::uintptr_t>(owned.data());
  return a <= b ? b - a < n * sizeof(T) : a - b < owned.size() * sizeof(T);
}
ArgLoadStmt *array(IRBuilder &b, int id, DataType type, int rank = 1) {
  return b.create_ndarray_arg_load({id}, type, rank, 0);
}
Stmt *read(IRBuilder &b, ArgLoadStmt *a, std::vector<Stmt *> index) {
  return b.create_global_load(b.create_external_ptr(a, index));
}
void write(IRBuilder &b, ArgLoadStmt *a, std::vector<Stmt *> index, Stmt *v) {
  b.create_global_store(b.create_external_ptr(a, index), v);
}
std::vector<Stmt *> cell(IRBuilder &b, int f, Stmt *row) {
  return {b.get_int32(f), row};
}
void count(IRBuilder &b, ArgLoadStmt *stats, int c) {
  b.create_atomic_add(b.create_external_ptr(stats, {b.get_int32(c)}),
                      b.get_int64(1));
}
void error(IRBuilder &b, ArgLoadStmt *stats, Stmt *bad, int flag) {
  auto guard = b.get_if_guard(b.create_if(bad), true);
  b.create_atomic_or(b.create_external_ptr(stats, {b.get_int32(Error)}),
                     b.get_int64(flag));
}
Stmt *finite(IRBuilder &b, Stmt *x) {
  return b.create_cmp_le(b.create_abs(x),
                         b.get_float64(std::numeric_limits<double>::max()));
}
}  // namespace

void CentroidCascade::append_layout(const BudgetDemand &l,
                                    std::vector<StorageExtent> &out) {
  if (!l[Budget::CascadeNodes])
    return;
  const auto n = physical(l[Budget::CascadeNodes]),
             r = physical(l[Budget::CascadeReferences]),
             h = physical(l[Budget::CascadeLinks]),
             levels = l[Budget::CascadeLevels] + 1;
  const auto add = [&](const char *name, std::size_t count, std::size_t bytes,
                       std::size_t copies) {
    if (count > std::numeric_limits<std::size_t>::max() / bytes / copies)
      throw std::overflow_error("cascade workspace overflow");
    out.push_back({name, count, bytes, copies, count * bytes * copies});
  };
  add("cascade-node-headers", n, sizeof(CascadeNode), 1);
  add("cascade-cohort-handles", r, sizeof(engine::RecordHandle), 1);
  add("cascade-node-controls", Columns * n, sizeof(int), 2);
  add("cascade-reference-controls", 3 * r, sizeof(int), 2);
  add("cascade-children", h, sizeof(int), 2);
  add("cascade-order", n, sizeof(int), 2);
  add("cascade-fraction-masks", n, sizeof(std::uint64_t), 2);
  add("cascade-parent-rank", n, sizeof(int), 2);
  add("cascade-level-offset-cursor", levels, sizeof(int), 2);
  add("cascade-values", 7 * n, sizeof(double), 2);
  add("cascade-valid", n, sizeof(int), 2);
  add("cascade-counters", 8, sizeof(std::int64_t), 2);
  add("cascade-root-summary", 7, sizeof(double), 2);
}

struct CentroidCascade::Impl {
  Harness &h;
  const BudgetDemand &limits;
  const int nstride, rstride;
  CascadeInspection inspection;
  std::vector<int> controls, refs, order, parents, ranks, offsets, cursors;
  std::vector<std::uint64_t> masks;
  std::array<std::int64_t, 8> counts{};
  std::array<double, 7> summary{};
  std::unique_ptr<Ndarray> node_array, ref_array, child_array, order_array,
      mask_array, values_array, valid_array, stats_array, summary_array;
  bool bound{}, valid{};
  std::uint64_t generation{}, binding{}, query_id{}, node_count{}, ref_count{},
      link_count{}, rounds{}, leaf_refs{}, work{};
  CascadeResult last;
  // Prepared contexts finish before their borrowed source/candidate arrays.
  engine::CompiledKernel validate_kernel, reduce_kernel;
  explicit Impl(Harness &owner)
      : h(owner),
        limits(owner.prepared_->capacity.limits()),
        nstride(int(physical(limits[Budget::CascadeNodes]))),
        rstride(int(physical(limits[Budget::CascadeReferences]))),
        controls(Columns * nstride),
        refs(3 * rstride),
        order(nstride),
        parents(nstride),
        ranks(nstride),
        offsets(limits[Budget::CascadeLevels] + 1),
        cursors(offsets.size()),
        masks(nstride) {
    engine::numeric::Precision(engine::numeric::Real::F64,
                               engine::numeric::Real::F64, 52)
        .require_backend(*h.runtime_);
    inspection.children.resize(physical(limits[Budget::CascadeLinks]));
    inspection.nodes.resize(nstride);
    inspection.cohorts.resize(rstride);
    inspection.values.resize(7 * nstride);
    inspection.valid.resize(nstride);
    auto make = [&](DataType dt, std::vector<int> shape) {
      auto a = std::make_unique<Ndarray>(&h.runtime_->program(), dt, shape);
      h.runtime_->program().fill_ndarray_fast_u32(a.get(), 0);
      return a;
    };
    node_array = make(PrimitiveType::i32, {Columns, nstride});
    ref_array = make(PrimitiveType::i32, {3, rstride});
    child_array = make(PrimitiveType::i32, {int(inspection.children.size())});
    order_array = make(PrimitiveType::i32, {nstride});
    mask_array = make(PrimitiveType::u64, {nstride});
    values_array = make(PrimitiveType::f64, {7, nstride});
    valid_array = make(PrimitiveType::i32, {nstride});
    stats_array = make(PrimitiveType::i64, {8});
    summary_array = make(PrimitiveType::f64, {7});
    compile();
    launch_validate(0, 0);
    launch_reduce(0, 0, -1);
    h.runtime_->synchronize();
  }
  void compile();
  void launch_validate(int n, int r);
  void launch_reduce(int first, int end, int root);
  void read_counts() {
    engine::readback(*stats_array, counts.data(), sizeof(counts));
  }
  int &control(int f, int row) {
    return controls[f * nstride + row];
  }
  int &reference(int f, int row) {
    return refs[f * rstride + row];
  }
  bool particle(const engine::RecordHandle &v) const {
    const auto &s = *h.prepared_;
    return v.record >= 0 && std::size_t(v.record) < s.particle_headers.size() &&
           same(s.particle_headers[v.record], v);
  }
  bool field(const engine::RecordHandle &v) const {
    const auto &s = *h.prepared_;
    return v.record >= 0 && std::size_t(v.record) < s.field_headers.size() &&
           same(s.field_headers[v.record], v);
  }
};

void CentroidCascade::Impl::compile() {
  auto &s = *h.prepared_;
  const auto f = PrimitiveType::f32, d = PrimitiveType::f64,
             i = PrimitiveType::i32, w = PrimitiveType::i64;
  const auto fa = engine::Param::array(f, 2), ia = engine::Param::array(i, 2),
             iv = engine::Param::array(i), cv = engine::Param::array(w),
             integer = engine::Param::scalar(i);
  const int threads = h.runtime_->cpu_threads();
  const auto name = std::to_string(s.particles.bindings().presence->id);
  {
    IRBuilder b;
    auto *p = array(b, 0, f, 2), *edges = array(b, 1, i, 2),
         *shares = array(b, 2, f, 2), *nodes = array(b, 3, i, 2),
         *references = array(b, 4, i, 2), *flags = array(b, 5, i),
         *stats = array(b, 6, w);
    auto *nc = b.create_arg_load({7}, i, false, 0),
         *rc = b.create_arg_load({8}, i, false, 0);
    for (int c = 0; c < 8; ++c)
      write(b, stats, {b.get_int32(c)}, b.get_int64(0));
    {
      auto *loop = b.create_range_for(b.get_int32(0), nc, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      write(b, flags, {row}, b.get_int32(0));
      count(b, stats, Fields);
      auto *field = read(b, nodes, cell(b, Field, row));
      error(b, stats,
            b.create_logical_not(
                NativeSet{s.fields.bindings().presence}.active(b, field)),
            source_error);
    }
    {
      auto *loop = b.create_range_for(b.get_int32(0), rc, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      auto *body = read(b, references, cell(b, 0, row)),
           *field = read(b, references, cell(b, 1, row)),
           *edge = read(b, references, cell(b, 2, row));
      count(b, stats, References);
      error(b, stats,
            b.create_logical_not(
                NativeSet{s.particles.bindings().presence}.active(b, body)),
            source_error);
      error(b, stats,
            b.create_logical_or(
                b.create_cmp_ne(read(b, edges, cell(b, kEdgeParticle, edge)),
                                body),
                b.create_logical_or(
                    b.create_cmp_ne(read(b, edges, cell(b, kEdgeGroup, edge)),
                                    field),
                    b.create_cmp_ne(read(b, shares, cell(b, kShare, edge)),
                                    b.get_float32(1)))),
            source_error);
      auto *mass = b.create_cast(read(b, p, cell(b, kMass, body)), d);
      error(b, stats,
            b.create_logical_or(b.create_logical_not(finite(b, mass)),
                                b.create_cmp_le(mass, b.get_float64(0))),
            numerical_error);
      for (int axis = 0; axis < 3; ++axis)
        error(
            b, stats,
            b.create_logical_not(finite(
                b, b.create_cast(read(b, p, cell(b, kPosX + axis, body)), d))),
            numerical_error);
      auto awake = b.get_if_guard(
          b.create_if(h.activity_->particle_activity.active(b, body)), true);
      count(b, stats, AwakeReferences);
    }
    validate_kernel = engine::CompiledKernel(
        *h.runtime_, "cascade_validate_" + name, b.extract_ir(),
        {fa, ia, fa, ia, ia, iv, cv, integer, integer},
        engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    auto *p = array(b, 0, f, 2), *nodes = array(b, 1, i, 2),
         *references = array(b, 2, i, 2), *children = array(b, 3, i),
         *order = array(b, 4, i), *masks = array(b, 5, PrimitiveType::u64),
         *values = array(b, 6, d, 2), *flags = array(b, 7, i),
         *stats = array(b, 8, w), *summary = array(b, 9, d);
    auto *first = b.create_arg_load({10}, i, false, 0),
         *end = b.create_arg_load({11}, i, false, 0),
         *root = b.create_arg_load({12}, i, false, 0);
    auto *loop = b.create_range_for(first, end, false, threads);
    {
      auto guard = b.get_loop_guard(loop);
      auto *row = read(b, order, {b.get_loop_index(loop)});
      auto *m = b.create_local_var(d), *bad = b.create_local_var(i);
      b.create_local_store(m, b.get_float64(0));
      b.create_local_store(bad, b.get_int32(0));
      std::array<AllocaStmt *, 3> anchor, weighted;
      for (int axis = 0; axis < 3; ++axis) {
        anchor[axis] = b.create_local_var(d);
        weighted[axis] = b.create_local_var(d);
        b.create_local_store(anchor[axis], b.get_float64(0));
        b.create_local_store(weighted[axis], b.get_float64(0));
      }
      auto *has_children = b.create_if(b.create_cmp_gt(
          read(b, nodes, cell(b, ChildCount, row)), b.get_int32(0)));
      const auto reduce = [&](bool child) {
        auto guard = b.get_if_guard(has_children, child);
        auto *begin =
            read(b, nodes, cell(b, child ? ChildBegin : RefBegin, row));
        auto *length =
            read(b, nodes, cell(b, child ? ChildCount : RefCount, row));
        auto *members =
            b.create_range_for(b.get_int32(0), length, false, 1, 0, true);
        auto member_guard = b.get_loop_guard(members);
        auto *index = b.get_loop_index(members);
        auto *ref =
            child ? read(b, children, {b.create_add(begin, index)})
                  : read(b, references, cell(b, 0, b.create_add(begin, index)));
        auto *mass = child ? read(b, values, cell(b, 6, ref))
                           : b.create_cast(read(b, p, cell(b, kMass, ref)), d);
        if (child)
          b.create_local_store(
              bad, b.create_logical_or(
                       b.create_local_load(bad),
                       b.create_cmp_eq(read(b, flags, {ref}), b.get_int32(0))));
        b.create_local_store(m, b.create_add(b.create_local_load(m), mass));
        count(b, stats, Reductions);
        for (int axis = 0; axis < 3; ++axis) {
          auto *x =
              child ? read(b, values, cell(b, 3 + axis, ref))
                    : b.create_cast(read(b, p, cell(b, kPosX + axis, ref)), d);
          {
            auto initial = b.get_if_guard(
                b.create_if(b.create_cmp_eq(index, b.get_int32(0))), true);
            b.create_local_store(anchor[axis], x);
          }
          b.create_local_store(
              weighted[axis],
              b.create_add(
                  b.create_local_load(weighted[axis]),
                  b.create_mul(mass, b.create_sub(x, b.create_local_load(
                                                         anchor[axis])))));
        }
      };
      reduce(true);
      reduce(false);
      auto *mass = b.create_local_load(m);
      auto *positive = b.create_cmp_gt(mass, b.get_float64(0));
      b.create_local_store(
          bad, b.create_logical_or(b.create_local_load(bad),
                                   b.create_logical_not(b.create_logical_and(
                                       positive, finite(b, mass)))));
      auto *denom = b.create_select(positive, mass, b.get_float64(1));
      write(b, values, cell(b, 6, row), mass);
      for (int axis = 0; axis < 3; ++axis) {
        auto *trial = b.create_add(
            b.create_local_load(anchor[axis]),
            b.create_div(b.create_local_load(weighted[axis]), denom));
        b.create_local_store(
            bad, b.create_logical_or(b.create_local_load(bad),
                                     b.create_logical_not(finite(b, trial))));
        // Same validated Precision fraction mask as the standalone Q_b.
        // Internal grouping has the identity mask; no float narrowing or
        // carried residue.
        auto *published = b.create_bit_cast(
            b.create_and(b.create_bit_cast(trial, PrimitiveType::u64),
                         read(b, masks, {row})),
            d);
        write(b, values, cell(b, axis, row), trial);
        write(b, values, cell(b, 3 + axis, row), published);
      }
      write(b, flags, {row}, b.create_logical_not(b.create_local_load(bad)));
      count(b, stats, Nodes);
      error(b, stats, b.create_local_load(bad), numerical_error);
      {
        auto invalid =
            b.get_if_guard(b.create_if(b.create_local_load(bad)), true);
        count(b, stats, BadNodes);
      }
      {
        auto final =
            b.get_if_guard(b.create_if(b.create_cmp_eq(row, root)), true);
        for (int f = 0; f < 7; ++f)
          write(b, summary, {b.get_int32(f)}, read(b, values, cell(b, f, row)));
      }
    }
    reduce_kernel = engine::CompiledKernel(
        *h.runtime_, "cascade_reduce_" + name, b.extract_ir(),
        {fa, ia, ia, iv, iv, engine::Param::array(PrimitiveType::u64),
         engine::Param::array(d, 2), iv, cv, engine::Param::array(d), integer,
         integer, integer},
        engine::MathMode::Strict);
  }
}

void CentroidCascade::Impl::launch_validate(int n, int r) {
  auto call = validate_kernel.invocation(true);
  auto &ctx = call.get();
  if (!ctx.prepared) {
    h.bind(ctx, 0, h.particle_array_.get(), h.particle_data,
           {kParticleFieldCount, std::max(h.particles_, 1)});
    h.bind(ctx, 1, h.edge_int_array_.get(), h.edge_int_data,
           {kEdgeIntFieldCount, std::max(h.edges_, 1)});
    h.bind(ctx, 2, h.edge_float_array_.get(), h.edge_float_data,
           {kEdgeFloatFieldCount, std::max(h.edges_, 1)});
    ctx.set_arg_ndarray({3}, *node_array);
    ctx.set_arg_ndarray({4}, *ref_array);
    ctx.set_arg_ndarray({5}, *valid_array);
    ctx.set_arg_ndarray({6}, *stats_array);
  }
  engine::argument_int(ctx, 7, n);
  engine::argument_int(ctx, 8, r);
  validate_kernel.launch(ctx);
}
void CentroidCascade::Impl::launch_reduce(int first, int end, int root) {
  auto call = reduce_kernel.invocation(true);
  auto &ctx = call.get();
  if (!ctx.prepared) {
    h.bind(ctx, 0, h.particle_array_.get(), h.particle_data,
           {kParticleFieldCount, std::max(h.particles_, 1)});
    ctx.set_arg_ndarray({1}, *node_array);
    ctx.set_arg_ndarray({2}, *ref_array);
    ctx.set_arg_ndarray({3}, *child_array);
    ctx.set_arg_ndarray({4}, *order_array);
    ctx.set_arg_ndarray({5}, *mask_array);
    ctx.set_arg_ndarray({6}, *values_array);
    ctx.set_arg_ndarray({7}, *valid_array);
    ctx.set_arg_ndarray({8}, *stats_array);
    ctx.set_arg_ndarray({9}, *summary_array);
  }
  engine::argument_int(ctx, 10, first);
  engine::argument_int(ctx, 11, end);
  engine::argument_int(ctx, 12, root);
  reduce_kernel.launch(ctx);
}

CentroidCascade::CentroidCascade(Harness &h)
    : impl_(std::make_unique<Impl>(h)) {
}
CentroidCascade::~CentroidCascade() = default;
std::size_t CentroidCascade::binding_bytes() const {
  return impl_->validate_kernel.prepared_bytes() +
         impl_->reduce_kernel.prepared_bytes();
}

CascadeBinding CentroidCascade::bind(std::uint64_t expected,
                                     const CascadeSpec &spec,
                                     std::uint64_t budget) {
  auto &a = *impl_;
  auto &s = *a.h.prepared_;
  CascadeBinding answer;
  answer.generation = s.generation;
  a.bound = false;
  a.valid = false;
  const auto finish = [&](CascadeStatus status) {
    answer.status = status;
    return answer;
  };
  const auto exhaust = [&](Budget b, std::uint64_t need, std::uint64_t cap) {
    answer.failure = {b, need, cap};
    return finish(CascadeStatus::CapacityExhausted);
  };
  if (expected != s.generation)
    return finish(CascadeStatus::Stale);
  if (a.binding == UINT64_MAX)
    return finish(CascadeStatus::Unsupported);
  answer.binding = ++a.binding;
  if (budget == UINT64_MAX)
    budget = a.limits[Budget::CascadeWork];
  if (budget > a.limits[Budget::CascadeWork])
    return exhaust(Budget::CascadeWork, budget, a.limits[Budget::CascadeWork]);
  for (const auto &item :
       {std::pair<Budget, std::uint64_t>{Budget::CascadeNodes, spec.node_count},
        {Budget::CascadeLinks, spec.child_count},
        {Budget::CascadeReferences, spec.cohort_count}})
    if (item.second > a.limits[item.first])
      return exhaust(item.first, item.second, a.limits[item.first]);
  if ((spec.node_count && !spec.nodes) ||
      (spec.child_count && !spec.children) ||
      (spec.cohort_count && !spec.cohorts))
    return finish(CascadeStatus::InvalidHierarchy);
  // The caller owns stable input for the duration of bind. Diagnostic borrows
  // expire at this boundary and cannot alias its mutable normalization tables.
  if (aliases(spec.nodes, spec.node_count, a.inspection.nodes) ||
      aliases(spec.children, spec.child_count, a.inspection.children) ||
      aliases(spec.cohorts, spec.cohort_count, a.inspection.cohorts))
    return finish(CascadeStatus::InvalidHierarchy);
  if (!spec.node_count) {
    if (s.live.particles || spec.child_count || spec.cohort_count)
      return finish(CascadeStatus::InvalidHierarchy);
    a.node_count = a.ref_count = a.link_count = a.rounds = a.leaf_refs =
        a.work = 0;
    a.generation = s.generation;
    a.bound = true;
    return finish(CascadeStatus::Empty);
  }
  const auto charge = [&]() {
    if (answer.preparation_visits == budget)
      return false;
    ++answer.preparation_visits;
    return true;
  };
  const auto work_failure = [&]() {
    return exhaust(Budget::CascadeWork, answer.preparation_visits + 1, budget);
  };
  // Charge logical-prefix initialization before touching reserved tables.
  for (std::size_t i = 0; i < spec.node_count; ++i) {
    if (!charge())
      return work_failure();
    a.parents[i] = 0;
    if (!charge())
      return work_failure();
    a.ranks[i] = 0;
  }
  std::uint64_t nr = 0, nh = 0, leaf = 0, rounds = 0;
  for (std::size_t row = 0; row < spec.node_count; ++row) {
    if (!charge())
      return work_failure();
    auto node = spec.nodes[row];
    if (!a.field(node.field))
      return finish(CascadeStatus::InvalidReference);
    if (!node.comparison || node.level < 0 || !node.cohort_count ||
        !span(node.cohort_begin, node.cohort_count, spec.cohort_count) ||
        !span(node.child_begin, node.child_count, spec.child_count))
      return finish(CascadeStatus::InvalidHierarchy);
    if (node.calculation != engine::numeric::Real::F64 ||
        node.storage != engine::numeric::Real::F64 ||
        node.retained_fraction_bits < 0 || node.retained_fraction_bits > 52)
      return finish(CascadeStatus::UnsupportedProfile);
    if (!node.publish && node.retained_fraction_bits != 52)
      return finish(CascadeStatus::InvalidHierarchy);
    for (std::size_t j = 0; j < row; ++j) {
      if (!charge())
        return work_failure();
      const auto &other = a.inspection.nodes[j];
      if (other.comparison == node.comparison &&
          (!same(other.field, node.field) || other.level != node.level ||
           (other.publish && node.publish)))
        return finish(CascadeStatus::InvalidHierarchy);
    }
    const auto rb = nr;
    int last_edge = -1;
    for (std::uint64_t k = 0; k < node.cohort_count; ++k) {
      if (!charge())
        return work_failure();
      const auto ref = spec.cohorts[node.cohort_begin + k];
      if (!a.particle(ref))
        return finish(CascadeStatus::InvalidReference);
      bool duplicate = false;
      for (std::uint64_t j = rb; j < nr; ++j) {
        if (!charge())
          return work_failure();
        if (same(a.inspection.cohorts[j], ref)) {
          duplicate = true;
          break;
        }
      }
      if (duplicate) {
        ++answer.aliases;
        continue;
      }
      if (nr == a.limits[Budget::CascadeReferences])
        return exhaust(Budget::CascadeReferences, nr + 1,
                       a.limits[Budget::CascadeReferences]);
      int match = -1, matches = 0;
      bool whole = true;
      for (std::size_t e = 0; e < s.live.edges; ++e) {
        if (!charge())
          return work_failure();
        if (same(s.edges[e].first, ref) &&
            same(s.edges[e].second, node.field)) {
          match = int(e);
          ++matches;
          whole = whole && s.edges[e].share == 1;
        }
      }
      if (!matches)
        return finish(CascadeStatus::InvalidReference);
      if (matches != 1 || !whole)
        return finish(CascadeStatus::UnsupportedParticipation);
      if (match <= last_edge)
        return finish(CascadeStatus::InvalidHierarchy);
      last_edge = match;
      a.inspection.cohorts[nr] = ref;
      a.reference(0, nr) = ref.record;
      a.reference(1, nr) = node.field.record;
      a.reference(2, nr) = match;
      ++nr;
    }
    const auto cb = nh;
    int last_child = -1;
    for (std::uint64_t k = 0; k < node.child_count; ++k) {
      if (!charge())
        return work_failure();
      const int child = spec.children[node.child_begin + k];
      if (child < 0 || std::size_t(child) >= row)
        return finish(CascadeStatus::InvalidHierarchy);
      bool duplicate = false;
      for (std::uint64_t j = cb; j < nh; ++j) {
        if (!charge())
          return work_failure();
        if (a.inspection.children[j] == child) {
          duplicate = true;
          break;
        }
      }
      if (duplicate) {
        ++answer.aliases;
        continue;
      }
      if (child <= last_child)
        return finish(CascadeStatus::InvalidHierarchy);
      last_child = child;
      if (nh == a.limits[Budget::CascadeLinks])
        return exhaust(Budget::CascadeLinks, nh + 1,
                       a.limits[Budget::CascadeLinks]);
      const auto &part = a.inspection.nodes[child];
      if (part.publish) {
        if (part.level >= node.level || part.comparison == node.comparison)
          return finish(CascadeStatus::InvalidHierarchy);
      } else if (part.level != node.level ||
                 part.comparison != node.comparison ||
                 !same(part.field, node.field))
        return finish(CascadeStatus::InvalidHierarchy);
      if (++a.parents[child] != 1)
        return finish(CascadeStatus::InvalidHierarchy);
      a.inspection.children[nh++] = child;
      a.ranks[row] = std::max(a.ranks[row], a.ranks[child] + 1);
    }
    node.cohort_begin = rb;
    node.cohort_count = nr - rb;
    node.child_begin = cb;
    node.child_count = nh - cb;
    if (!node.child_count)
      leaf += node.cohort_count;
    else {
      std::uint64_t total = 0;
      for (std::uint64_t k = cb; k < nh; ++k) {
        const auto &part = a.inspection.nodes[a.inspection.children[k]];
        total += part.cohort_count;
        for (std::uint64_t j = part.cohort_begin;
             j < part.cohort_begin + part.cohort_count; ++j) {
          bool found = false;
          for (std::uint64_t v = rb; v < nr; ++v) {
            if (!charge())
              return work_failure();
            if (same(a.inspection.cohorts[v], a.inspection.cohorts[j])) {
              found = true;
              break;
            }
          }
          if (!found)
            return finish(CascadeStatus::InvalidHierarchy);
          for (std::uint64_t earlier = cb; earlier < k; ++earlier) {
            const auto &other =
                a.inspection.nodes[a.inspection.children[earlier]];
            for (std::uint64_t v = other.cohort_begin;
                 v < other.cohort_begin + other.cohort_count; ++v) {
              if (!charge())
                return work_failure();
              if (same(a.inspection.cohorts[v], a.inspection.cohorts[j]))
                return finish(CascadeStatus::InvalidHierarchy);
            }
          }
        }
      }
      if (total != node.cohort_count)
        return finish(CascadeStatus::InvalidHierarchy);
    }
    a.inspection.nodes[row] = node;
    a.control(RefBegin, row) = int(rb);
    a.control(RefCount, row) = int(node.cohort_count);
    a.control(ChildBegin, row) = int(cb);
    a.control(ChildCount, row) = int(node.child_count);
    a.control(Field, row) = node.field.record;
    a.control(Level, row) = node.level;
    a.control(Publish, row) = node.publish;
    a.control(Bits, row) = node.retained_fraction_bits;
    a.masks[row] = engine::numeric::Precision(node.calculation, node.storage,
                                              node.retained_fraction_bits)
                       .fraction_mask();
    rounds = std::max(rounds, std::uint64_t(a.ranks[row]) + 1);
  }
  if (!a.inspection.nodes[spec.node_count - 1].publish)
    return finish(CascadeStatus::InvalidHierarchy);
  for (std::size_t i = 0; i < spec.node_count; ++i) {
    if (!charge())
      return work_failure();
    if (a.parents[i] != (i + 1 == spec.node_count ? 0 : 1))
      return finish(CascadeStatus::InvalidHierarchy);
  }
  if (rounds > a.limits[Budget::CascadeLevels])
    return exhaust(Budget::CascadeLevels, rounds,
                   a.limits[Budget::CascadeLevels]);
  const auto references = s.live.references() + spec.node_count + nr + nh;
  if (references > a.limits[Budget::References])
    return exhaust(Budget::References, references,
                   a.limits[Budget::References]);
  for (std::size_t i = 0; i <= rounds; ++i) {
    if (!charge())
      return work_failure();
    a.offsets[i] = 0;
  }
  for (std::size_t i = 0; i < spec.node_count; ++i) {
    if (!charge())
      return work_failure();
    ++a.offsets[a.ranks[i] + 1];
  }
  for (std::size_t i = 1; i <= rounds; ++i) {
    if (!charge())
      return work_failure();
    a.offsets[i] += a.offsets[i - 1];
  }
  for (std::size_t i = 0; i <= rounds; ++i) {
    if (!charge())
      return work_failure();
    a.cursors[i] = a.offsets[i];
  }
  for (std::size_t i = 0; i < spec.node_count; ++i) {
    if (!charge())
      return work_failure();
    a.order[a.cursors[a.ranks[i]]++] = int(i);
  }
  for (std::uint64_t i = 0; i < spec.node_count + nr; ++i)
    if (!charge())
      return work_failure();
  answer.nodes = spec.node_count;
  answer.references = nr;
  answer.links = nh;
  answer.levels = rounds;
  try {
    a.h.validate_prepared_bindings();
    const auto upload = [&](auto &array, const auto &data) {
      const auto bytes = data.size() * sizeof(data[0]);
      engine::upload(*array, data.data(), bytes);
      answer.uploaded_bytes += bytes;
    };
    upload(a.node_array, a.controls);
    upload(a.ref_array, a.refs);
    upload(a.child_array, a.inspection.children);
    upload(a.order_array, a.order);
    upload(a.mask_array, a.masks);
    a.launch_validate(int(spec.node_count), int(nr));
    a.h.runtime_->synchronize();
    ++answer.synchronizations;
    a.read_counts();
    answer.readback_bytes += sizeof(a.counts);
    if (a.counts[Fields] != std::int64_t(spec.node_count) ||
        a.counts[References] != std::int64_t(nr))
      throw std::logic_error("incomplete cascade binding validation");
    if (a.counts[Error] & source_error)
      throw std::logic_error("cascade source binding invariant failed");
    if (a.counts[Error] & numerical_error)
      return finish(CascadeStatus::InvalidNumerics);
    a.node_count = spec.node_count;
    a.ref_count = nr;
    a.link_count = nh;
    a.rounds = rounds;
    a.leaf_refs = leaf;
    a.work = 2 * spec.node_count + nr + leaf + nh;
    a.generation = s.generation;
    a.bound = true;
    return finish(CascadeStatus::Ok);
  } catch (...) {
    a.h.ready_ = false;
    throw;
  }
}

CascadeResult CentroidCascade::query(std::uint64_t expected,
                                     std::uint64_t budget) {
  auto &a = *impl_;
  auto &s = *a.h.prepared_;
  CascadeResult answer;
  a.valid = false;
  answer.context = s.particles.context();
  answer.generation = s.generation;
  answer.completed_ticks = s.completed_ticks;
  answer.binding = a.binding;
  answer.frame_anchor = a.h.frame_anchor_;
  const auto finish = [&](CascadeStatus status) {
    answer.status = status;
    a.last = answer;
    return answer;
  };
  const auto exhaust = [&](std::uint64_t need, std::uint64_t cap) {
    answer.failure = {Budget::CascadeWork, need, cap};
    return finish(CascadeStatus::CapacityExhausted);
  };
  if (expected != s.generation)
    return finish(CascadeStatus::Stale);
  if (a.query_id == UINT64_MAX)
    return finish(CascadeStatus::Unsupported);
  answer.query = ++a.query_id;
  if (budget == UINT64_MAX)
    budget = a.limits[Budget::CascadeWork];
  if (budget > a.limits[Budget::CascadeWork])
    return exhaust(budget, a.limits[Budget::CascadeWork]);
  if (!s.live.particles) {
    a.valid = true;
    return finish(CascadeStatus::Empty);
  }
  if (!a.bound || a.generation != s.generation)
    return finish(CascadeStatus::Stale);
  answer.nodes = a.node_count;
  answer.links = a.link_count;
  answer.references = a.ref_count;
  answer.levels = a.rounds;
  answer.required_work = a.work;
  answer.root = int(a.node_count) - 1;
  if (a.work > budget)
    return exhaust(a.work, budget);
  try {
    a.h.validate_prepared_bindings();
    a.launch_validate(int(a.node_count), int(a.ref_count));
    a.h.runtime_->synchronize();
    ++answer.synchronizations;
    a.read_counts();
    answer.readback_bytes += sizeof(a.counts);
    if (a.counts[Fields] != std::int64_t(a.node_count) ||
        a.counts[References] != std::int64_t(a.ref_count))
      throw std::logic_error("incomplete cascade source validation");
    answer.reference_visits = a.counts[References];
    answer.field_checks = a.counts[Fields];
    answer.awake_reference_visits = a.counts[AwakeReferences];
    if (a.counts[Error] & source_error)
      throw std::logic_error("cascade source invariant failed");
    if (a.counts[Error] & numerical_error)
      return finish(CascadeStatus::InvalidNumerics);
    for (std::size_t level = 0; level < a.rounds; ++level)
      a.launch_reduce(a.offsets[level], a.offsets[level + 1], answer.root);
    a.h.runtime_->synchronize();
    ++answer.synchronizations;
    a.read_counts();
    engine::readback(*a.summary_array, a.summary.data(), sizeof(a.summary));
    answer.readback_bytes += sizeof(a.counts) + sizeof(a.summary);
    answer.reduction_visits = a.counts[Reductions];
    answer.node_reductions = a.counts[Nodes];
    if (a.counts[Nodes] != std::int64_t(a.node_count) ||
        a.counts[Reductions] != std::int64_t(a.leaf_refs + a.link_count))
      throw std::logic_error("incomplete cascade reduction");
    if (a.counts[Error])
      return finish(CascadeStatus::InvalidNumerics);
    std::copy_n(a.summary.begin(), 3, answer.trial.begin());
    std::copy_n(a.summary.begin() + 3, 3, answer.centre.begin());
    answer.mass = a.summary[6];
    a.valid = true;
    return finish(CascadeStatus::Ok);
  } catch (...) {
    a.h.ready_ = false;
    throw;
  }
}

const CascadeInspection &CentroidCascade::inspect(const CascadeResult &q) {
  auto &a = *impl_;
  auto &s = *a.h.prepared_;
  if (!a.valid || !a.h.ready_ ||
      (q.status != CascadeStatus::Ok && q.status != CascadeStatus::Empty) ||
      q.query != a.last.query || q.binding != a.binding ||
      q.context != s.particles.context() || q.generation != s.generation ||
      q.completed_ticks != s.completed_ticks)
    throw std::logic_error("stale centroid cascade observation");
  try {
    a.h.validate_prepared_bindings();
    engine::readback(*a.values_array, a.inspection.values.data(),
                     a.inspection.values.size() * sizeof(double));
    engine::readback(*a.valid_array, a.inspection.valid.data(),
                     a.inspection.valid.size() * sizeof(int));
    return a.inspection;
  } catch (...) {
    a.h.ready_ = false;
    throw;
  }
}
}  // namespace field::detail

namespace field {
CascadeBinding Harness::bind_centroid_cascade(std::uint64_t generation,
                                              const CascadeSpec &spec,
                                              std::uint64_t work) {
  if (!prepared_ || !ready_)
    return {};
  if (!centroid_cascade_) {
    CascadeBinding r;
    r.status = CascadeStatus::Unsupported;
    return r;
  }
  return centroid_cascade_->bind(generation, spec, work);
}
CascadeResult Harness::query_centroid_cascade(std::uint64_t generation,
                                              std::uint64_t work) {
  if (!prepared_ || !ready_)
    return {};
  if (!centroid_cascade_) {
    CascadeResult r;
    r.status = CascadeStatus::Unsupported;
    return r;
  }
  return centroid_cascade_->query(generation, work);
}
const CascadeInspection &Harness::inspect_centroid_cascade(
    const CascadeResult &q) {
  if (!prepared_ || !ready_ || !centroid_cascade_)
    throw std::logic_error("unready centroid cascade inspection");
  return centroid_cascade_->inspect(q);
}
}  // namespace field
