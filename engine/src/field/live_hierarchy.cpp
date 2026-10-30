#include "field/live_hierarchy.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include "engine/transfer.h"
#ifdef HCP_WITH_CUDA_QUALIFICATION
#include "engine/device.h"
#endif
#include "field/activity.h"
#include "field/centroid_cascade.h"
#include "field/cascade_source.h"
#include "field/contact_math.h"
#include "field/field.h"
#include "field/numeric_ir.h"
#include "field/prepared_storage.h"

namespace field::detail {
namespace {
using namespace taichi::lang;
enum Config {
  Parent,
  ChildBegin,
  ChildCount,
  SourceBegin,
  SourceCount,
  Field,
  CoverBegin,
  CoverCount,
  MultipleBodies,
  Complement,
  Columns
};
enum Count {
  Errors,
  Visits,
  Publications,
  SourceReads,
  ChildReads,
  ForceReads,
  RecipientChecks,
  ConservativeWakes,
  WakeInfluenceReads,
  WakeContactReads,
  Counts
};
std::size_t physical(std::uint64_t n) {
  return std::max<std::uint64_t>(n, 1);
}
ArgLoadStmt *array(IRBuilder &b, int i, DataType t, int rank = 1) {
  return b.create_ndarray_arg_load({i}, t, rank, 0);
}
Stmt *read(IRBuilder &b, ArgLoadStmt *a, std::vector<Stmt *> at) {
  return b.create_global_load(b.create_external_ptr(a, at));
}
void write(IRBuilder &b, ArgLoadStmt *a, std::vector<Stmt *> at, Stmt *v) {
  b.create_global_store(b.create_external_ptr(a, at), v);
}
std::vector<Stmt *> cell(IRBuilder &b, int f, Stmt *n) {
  return {b.get_int32(f), n};
}
void count(IRBuilder &b, ArgLoadStmt *a, int c) {
  b.create_atomic_add(b.create_external_ptr(a, {b.get_int32(c)}),
                      b.get_int64(1));
}
struct Args {
  ArgLoadStmt *values, *selector, *cfg, *children, *sources, *ratios, *masks,
      *bodies, *groups, *coverage, *influences, *weights, *offsets, *old,
      *stats, *activity, *group_delta, *leaf, *owners, *pairs, *a_offsets,
      *a_order, *b_offsets, *b_order;
  Stmt *nodes, *gravity, *op, *stiffness;
  ArgLoadStmt *visits, *group_flags;
  explicit Args(IRBuilder &b) {
    const auto d = PrimitiveType::f64, i = PrimitiveType::i32,
               u = PrimitiveType::u64;
    values = array(b, 0, d, 2);
    selector = array(b, 1, i);
    cfg = array(b, 2, i, 2);
    children = array(b, 3, i);
    sources = array(b, 4, i, 2);
    ratios = array(b, 5, u, 2);
    masks = array(b, 6, u);
    bodies = array(b, 7, d, 2);
    groups = array(b, 8, d, 2);
    coverage = array(b, 9, i);
    influences = array(b, 10, i, 2);
    weights = array(b, 11, u, 2);
    offsets = array(b, 12, i, 2);
    old = array(b, 13, d, 2);
    stats = array(b, 14, PrimitiveType::i64);
    activity = array(b, 15, PrimitiveType::i64);
    group_delta = array(b, 16, d, 2);
    leaf = array(b, 17, i);
    owners = array(b, 18, i);
    pairs = array(b, 19, i, 2);
    a_offsets = array(b, 20, i, 2);
    a_order = array(b, 21, i);
    b_offsets = array(b, 22, i, 2);
    b_order = array(b, 23, i);
    nodes = b.create_arg_load({24}, i, false, 0);
    gravity = b.create_arg_load({25}, d, false, 0);
    op = b.create_arg_load({26}, i, false, 0);
    stiffness = b.create_arg_load({27}, d, false, 0);
    visits = array(b, 28, PrimitiveType::i64, 2);
    group_flags = array(b, 29, PrimitiveType::i32, 2);
  }
};
std::vector<engine::Param> signature() {
  using engine::Param;
  const auto d = PrimitiveType::f64, i = PrimitiveType::i32,
             u = PrimitiveType::u64, l = PrimitiveType::i64;
  return {Param::array(d, 2), Param::array(i),    Param::array(i, 2),
          Param::array(i),    Param::array(i, 2), Param::array(u, 2),
          Param::array(u),    Param::array(d, 2), Param::array(d, 2),
          Param::array(i),    Param::array(i, 2), Param::array(u, 2),
          Param::array(i, 2), Param::array(d, 2), Param::array(l),
          Param::array(l),    Param::array(d, 2), Param::array(i),
          Param::array(i),    Param::array(i, 2), Param::array(i, 2),
          Param::array(i),    Param::array(i, 2), Param::array(i),
          Param::scalar(i),   Param::scalar(d),   Param::scalar(i),
          Param::scalar(d),   Param::array(l, 2), Param::array(i, 2)};
}
std::array<Stmt *, 4> packet(IRBuilder &b, const Args &a, Stmt *node) {
  auto *s = b.create_atomic_or(b.create_external_ptr(a.selector, {node}),
                               b.get_int32(0));
  std::array<Stmt *, 4> out{};
  for (int k = 0; k < 4; ++k)
    out[k] = read(
        b, a.values,
        {b.create_add(b.create_mul(s, b.get_int32(4)), b.get_int32(k)), node});
  return out;
}
std::array<Stmt *, 4> substitute(IRBuilder &b,
                                 Stmt *node,
                                 Stmt *changed,
                                 const std::array<Stmt *, 4> &current,
                                 const std::array<Stmt *, 4> &replacement) {
  std::array<Stmt *, 4> out{};
  for (int k = 0; k < 4; ++k)
    out[k] = b.create_select(b.create_cmp_eq(node, changed), replacement[k],
                             current[k]);
  return out;
}
// One influence is evaluated at a time, with explicit force units. No force
// cache or centroid integration exists. All paired checks share acquired
// inputs.
std::array<Stmt *, 3> contribution(IRBuilder &b,
                                   const Args &a,
                                   Stmt *body,
                                   Stmt *ref,
                                   const std::array<Stmt *, 4> &child,
                                   const std::array<Stmt *, 4> &parent,
                                   bool physical_work = false) {
  std::array<AllocaStmt *, 3> result{};
  for (auto &v : result) {
    v = b.create_local_var(PrimitiveType::f64);
    b.create_local_store(v, b.get_float64(0));
  }
  auto *kind = read(b, a.influences, cell(b, 3, ref));
  auto *node = read(b, a.influences, cell(b, 1, ref));
  auto *pnode = read(b, a.influences, cell(b, 2, ref));
  auto *source_node = b.create_select(kind, pnode, node);
  auto *eligible = b.create_logical_and(
      read(b, a.cfg, cell(b, MultipleBodies, source_node)),
      b.create_logical_or(b.create_cmp_eq(kind, b.get_int32(0)),
                          read(b, a.cfg, cell(b, Complement, node))));
  {
    auto nonzero = b.get_if_guard(
        b.create_if(b.create_logical_and(
            eligible, b.create_cmp_ne(a.gravity, b.get_float64(0)))),
        true);
    if (physical_work)
      count(b, a.activity, Forces);
    std::array<Stmt *, 3> delta{};
    Stmt *d2 = b.get_float64(0);
    for (int k = 0; k < 3; ++k) {
      delta[k] = b.create_select(
          kind, b.create_sub(parent[k], child[k]),
          b.create_sub(child[k], read(b, a.bodies, cell(b, kPosX + k, body))));
      d2 = b.create_add(d2, b.create_mul(delta[k], delta[k]));
    }
    auto live = b.get_if_guard(
        b.create_if(b.create_cmp_gt(d2, b.get_float64(0))), true);
    auto *inv = b.create_rsqrt(d2);
    auto *weight = b.create_mul(
        read(b, a.bodies, cell(b, kMass, body)),
        b.create_div(b.create_cast(read(b, a.weights, cell(b, 0, ref)),
                                   PrimitiveType::f64),
                     b.create_cast(read(b, a.weights, cell(b, 1, ref)),
                                   PrimitiveType::f64)));
    auto *mass = b.create_select(kind, parent[3], child[3]);
    auto *scale = b.create_mul(
        b.create_mul(a.gravity, weight),
        b.create_mul(mass, b.create_mul(inv, b.create_mul(inv, inv))));
    for (auto *v : std::array<Stmt *, 5>{d2, inv, weight, mass, scale})
      mark_nonfinite(b, a.stats, v, Errors);
    for (int k = 0; k < 3; ++k) {
      auto *v = b.create_mul(scale, delta[k]);
      mark_nonfinite(b, a.stats, v, Errors);
      b.create_local_store(result[k], v);
    }
  }
  std::array<Stmt *, 3> out{};
  for (int k = 0; k < 3; ++k)
    out[k] = b.create_local_load(result[k]);
  return out;
}
}  // namespace

void LiveHierarchy::append_layout(const BudgetDemand &l,
                                  std::vector<StorageExtent> &out) {
  if (!l[Budget::LiveResponses])
    return;
  const auto h = physical(l[Budget::CascadeNodes]),
             s = physical(l[Budget::CascadeReferences]),
             u = physical(l[Budget::CascadeCoverage]),
             r = physical(l[Budget::LiveResponses]),
             p = physical(l[Budget::Particles]),
             g = physical(l[Budget::Fields]);
  const auto add = [&](const char *name, std::size_t n, std::size_t bytes,
                       std::size_t copies = 2) {
    if (n > SIZE_MAX / bytes / copies)
      throw std::overflow_error("live hierarchy workspace overflow");
    out.push_back({name, n, bytes, copies, n * bytes * copies});
  };
  add("live-publication-slots", 8 * h, 8);
  add("live-publication-selectors", h, 4);
  add("live-previous-publication", 4 * h, 8);
  add("live-node-controls", Columns * h, 4);
  add("live-children", physical(l[Budget::CascadeLinks]), 4);
  add("live-source-records", 2 * s, 4);
  add("live-source-ratios", 2 * s, 8);
  add("live-fraction-masks", h, 8);
  add("live-coverage-bodies", u, 4);
  add("live-response-controls", 4 * r, 4);
  add("live-response-ratios", 2 * r, 8);
  add("live-response-offsets", 2 * p, 4);
  add("live-leaf-fields", g, 4);
  add("live-field-owners", g, 4);
  add("live-field-inspection", h, 4, 1);
  add("live-counters", Counts, 8);
  add("live-node-observations", 2 * h, 8);
  add("live-node-identities", h, sizeof(CascadeNode), 1);
  add("live-source-identities", s, sizeof(CascadeSourceRow), 1);
  add("live-logical-order", h, sizeof(int), 1);
  add("refresh-slots", 8 * h, sizeof(double), 1);
  add("refresh-selectors", h, sizeof(int), 1);
  add("refresh-history", 4 * h, sizeof(double), 1);
  add("refresh-node-controls", Columns * h, sizeof(int));
  add("refresh-children", physical(l[Budget::CascadeLinks]), sizeof(int));
  add("refresh-sources", 2 * s, sizeof(int));
  add("refresh-source-ratios", 2 * s, sizeof(std::uint64_t));
  add("refresh-masks", h, sizeof(std::uint64_t));
  add("refresh-plan", 5 * h, sizeof(int));
  add("refresh-ranks", h, sizeof(int), 1);
  add("refresh-node-identities", 2 * h, sizeof(CascadeNode), 1);
  add("refresh-source-identities", s, sizeof(CascadeSourceRow), 1);
  add("refresh-exact-coverage", u, sizeof(CascadeCoverage), 1);
  add("refresh-coverage-sums", u, sizeof(std::uint64_t), 1);
  add("refresh-coverage-bodies", u, sizeof(int), 1);
  add("refresh-response-controls", 4 * r, sizeof(int), 1);
  add("refresh-response-ratios", 2 * r, sizeof(std::uint64_t), 1);
  add("refresh-response-offsets", 2 * p, sizeof(int), 1);
  add("refresh-leaf-fields", g, sizeof(int), 1);
  add("refresh-owners", g, sizeof(int), 1);
}

struct LiveHierarchy::Impl {
  Harness &h;
  const int H, S, U, R, P, G, L;
  bool bound{};
  // Local qualification only; normal builds continue to refuse CUDA C3.
  bool cuda_qualification_device{};
  std::uint64_t generation{}, nodes{}, responses{};
  LiveHierarchyInspection inspection;
  std::vector<int> cfg, children, sources, coverage, influences, offsets, leaf,
      owners;
  std::vector<std::uint64_t> ratios, masks, weights;
  std::vector<CascadeSourceRow> source_headers;
  std::array<std::unique_ptr<Ndarray>, 20> owned;
  std::array<Ndarray *, 19> arrays{};
  struct Refresh {
    std::vector<int> cfg, children, sources, coverage, influences, offsets,
        leaf, owners, plan, rank;
    std::vector<std::uint64_t> ratios, masks, weights, coverage_sum;
    std::vector<CascadeNode> nodes, proposed;
    std::vector<CascadeSourceRow> source_headers;
    std::vector<CascadeCoverage> exact;
    std::array<std::unique_ptr<Ndarray>, 9> arrays;
    engine::CompiledKernel candidate, commit;
    std::size_t count{}, responses{};
    explicit Refresh(Impl &a)
        : cfg(Columns * a.H),
          children(a.L),
          sources(2 * a.S),
          coverage(a.U),
          influences(4 * a.R),
          offsets(2 * std::max(a.P, 1)),
          leaf(std::max(a.G, 1)),
          owners(std::max(a.G, 1), -1),
          plan(5 * a.H),
          rank(a.H),
          ratios(2 * a.S),
          masks(a.H),
          weights(2 * a.R),
          coverage_sum(a.U),
          nodes(a.H),
          proposed(a.H),
          source_headers(a.S),
          exact(a.U) {
    }
  };
  std::unique_ptr<Refresh> refresh;
  engine::CompiledKernel force_kernel, poll_kernel, command_kernel;
  explicit Impl(Harness &owner)
      : h(owner),
        H(int(physical(h.prepared_->capacity.limits()[Budget::CascadeNodes]))),
        S(int(physical(
            h.prepared_->capacity.limits()[Budget::CascadeReferences]))),
        U(int(
            physical(h.prepared_->capacity.limits()[Budget::CascadeCoverage]))),
        R(int(physical(h.prepared_->capacity.limits()[Budget::LiveResponses]))),
        P(h.particles_),
        G(h.groups_),
        L(int(physical(h.prepared_->capacity.limits()[Budget::CascadeLinks]))),
        cfg(Columns * H),
        children(L),
        sources(2 * S),
        coverage(U),
        influences(4 * R),
        offsets(2 * std::max(P, 1)),
        leaf(std::max(G, 1)),
        owners(std::max(G, 1), -1),
        ratios(2 * S),
        masks(H),
        weights(2 * R),
        source_headers(S) {
#ifdef HCP_WITH_CUDA_QUALIFICATION
    if (h.runtime_->on_cuda()) {
      const auto device = engine::bound_cuda_device();
      cuda_qualification_device =
          device.name == "NVIDIA GeForce GTX 1070" &&
          device.compute_capability_major == 6 &&
          device.compute_capability_minor == 1;
    }
#endif
    inspection.nodes.resize(H);
    inspection.order.resize(H);
    inspection.slots.resize(8 * H);
    inspection.previous.resize(4 * H);
    inspection.selectors.resize(H);
    inspection.fields.resize(H);
    inspection.visits.resize(2 * H);
    const std::array<DataType, 19> types{
        PrimitiveType::f64, PrimitiveType::i32, PrimitiveType::i32,
        PrimitiveType::i32, PrimitiveType::i32, PrimitiveType::u64,
        PrimitiveType::u64, PrimitiveType::f64, PrimitiveType::f64,
        PrimitiveType::i32, PrimitiveType::i32, PrimitiveType::u64,
        PrimitiveType::i32, PrimitiveType::f64, PrimitiveType::i64,
        PrimitiveType::i64, PrimitiveType::f64, PrimitiveType::i32,
        PrimitiveType::i32};
    const std::array<std::vector<int>, 19> shapes{
        {{8, H},
         {H},
         {Columns, H},
         {L},
         {2, S},
         {2, S},
         {H},
         {kParticleFieldCount, std::max(P, 1)},
         {kGroupFieldCount, std::max(G, 1)},
         {U},
         {4, R},
         {2, R},
         {2, std::max(P, 1)},
         {4, H},
         {Counts},
         {TrialCounterCount},
         {6, std::max(G, 1)},
         {std::max(G, 1)},
         {std::max(G, 1)}}};
    for (int i = 0; i < 19; ++i) {
      if (i == 7 || i == 8 || i == 15 || i == 16)
        continue;
      owned[i] = std::make_unique<Ndarray>(&h.runtime_->program(), types[i],
                                           shapes[i]);
      arrays[i] = owned[i].get();
      h.runtime_->program().fill_ndarray_fast_u32(arrays[i], 0);
    }
    owned[19] = std::make_unique<Ndarray>(
        &h.runtime_->program(), PrimitiveType::i64, std::vector<int>{2, H});
    h.runtime_->program().fill_ndarray_fast_u32(owned[19].get(), 0);
    compile();
    prepare_refresh_execution();
    launch(command_kernel, 0, 0);
    launch(force_kernel, 0, -1);
    launch(poll_kernel, 0, -1);
    h.runtime_->synchronize();
  }
  int &config(int c, int n) {
    return cfg[c * H + n];
  }
  void compile();
  void prepare_refresh_execution();
  void launch_refresh(bool commit);
  void launch(engine::CompiledKernel &kernel,
              double gravity,
              int op,
              double stiffness = 0) {
    auto call = kernel.invocation(true);
    auto &c = call.get();
    if (!c.prepared) {
      for (int i = 0; i < 19; ++i)
        if (arrays[i])
          c.set_arg_ndarray({i}, *arrays[i]);
      h.bind(c, 7, h.particle_array_.get(), h.particle_values_,
             {kParticleFieldCount, std::max(P, 1)});
      h.bind(c, 8, h.group_array_.get(), h.group_values_,
             {kGroupFieldCount, std::max(G, 1)});
      c.set_arg_ndarray({15}, *h.activity_->counters);
      c.set_arg_ndarray({16}, *h.activity_->group_delta);
      h.bind(c, 19, h.contact_int_array_.get(), h.contact_int_data,
             {kContactIntFieldCount, std::max(h.contacts_, 1)});
      c.set_arg_ndarray({20}, *h.activity_->contact_a_offsets);
      c.set_arg_ndarray({21}, *h.activity_->contact_a_edges);
      c.set_arg_ndarray({22}, *h.activity_->contact_b_offsets);
      c.set_arg_ndarray({23}, *h.activity_->contact_b_edges);
      c.set_arg_ndarray({28}, *owned[19]);
      c.set_arg_ndarray({29}, *h.activity_->group_flags);
    }
    engine::argument_int(c, 24, nodes);
    engine::argument_float(c, 25, gravity);
    engine::argument_int(c, 26, op);
    engine::argument_float(c, 27, stiffness);
    kernel.launch(c);
  }
};

void LiveHierarchy::Impl::compile() {
  const auto suffix = std::to_string(h.activity_->centroids.cells->id);
  const int threads = h.runtime_->cpu_threads();
  {
    IRBuilder b;
    Args a(b);
    auto *reset = b.create_if(b.create_cmp_eq(a.op, b.get_int32(0)));
    {
      auto guard = b.get_if_guard(reset, true);
      for (int k = 0; k < Counts; ++k)
        write(b, a.stats, {b.get_int32(k)}, b.get_int64(0));
    }
    {
      auto guard = b.get_if_guard(reset, false);
      auto *loop = b.create_range_for(b.get_int32(0), a.nodes, false, threads);
      auto row_scope = b.get_loop_guard(loop);
      auto *node = b.get_loop_index(loop);
      auto *f = read(b, a.cfg, cell(b, Field, node));
      write(b, a.group_flags, cell(b, Flagged, f), b.get_int32(0));
      write(b, a.group_flags, cell(b, Changed, f), b.get_int32(0));
      for (int k = kAccX; k <= kAccM; ++k)
        write(b, a.groups, cell(b, k, f), b.get_float64(0));
      for (int k = 0; k < 4; ++k) {
        auto *previous = read(b, a.groups, cell(b, k, f));
        write(b, a.old, cell(b, k, node), previous);
        write(b, a.groups, cell(b, k, f), read(b, a.values, cell(b, k, node)));
        if (k < 3) {
          write(b, a.group_delta, cell(b, 3 + k, f), previous);
          write(b, a.group_delta, cell(b, k, f), b.get_float64(0));
        }
      }
    }
    command_kernel = engine::CompiledKernel(
        *h.runtime_, "live_hierarchy_control_" + suffix, b.extract_ir(),
        signature(), engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    Args a(b);
    auto *loop = b.create_struct_for(h.activity_->particle_activity.cells,
                                     false, threads);
    auto scope = b.get_loop_guard(loop);
    auto *body = b.get_loop_index(loop);
    auto *refs = b.create_range_for(read(b, a.offsets, cell(b, 0, body)),
                                    read(b, a.offsets, cell(b, 1, body)), false,
                                    1, 0, true);
    auto ref_scope = b.get_loop_guard(refs);
    auto *ref = b.get_loop_index(refs);
    auto *node = read(b, a.influences, cell(b, 1, ref));
    auto child = packet(b, a, node);
    // A local row uses its own node as the harmless second packet. This keeps
    // the reader path uniform and never indexes a negative root parent.
    auto parent = packet(b, a, read(b, a.influences, cell(b, 2, ref)));
    auto f = contribution(b, a, body, ref, child, parent, true);
    count(b, a.stats, ForceReads);
    count(b, a.activity, ForceRequests);
    for (int k = 0; k < 3; ++k)
      write(b, a.bodies, cell(b, kForceX + k, body),
            b.create_add(read(b, a.bodies, cell(b, kForceX + k, body)), f[k]));
    force_kernel = engine::CompiledKernel(
        *h.runtime_, "live_hierarchy_force_" + suffix, b.extract_ir(),
        signature(), engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    Args a(b);
    auto *loop =
        b.create_struct_for(h.activity_->centroids.cells, false, threads, 1);
    auto scope = b.get_loop_guard(loop);
    auto *field = b.get_loop_index(loop);
    auto *node = read(b, a.owners, {field});
    auto target = b.get_if_guard(
        b.create_if(b.create_logical_and(
            b.create_cmp_ge(node, b.get_int32(0)),
            b.create_logical_or(b.create_cmp_lt(a.op, b.get_int32(0)),
                                b.create_cmp_eq(node, a.op)))),
        true);
    h.activity_->centroids.deactivate(b, field);
    write(b, a.group_flags, cell(b, Flagged, field), b.get_int32(1));
    write(b, a.group_flags, cell(b, Changed, field), b.get_int32(0));
    for (int k = kAccX; k <= kAccM; ++k)
      write(b, a.groups, cell(b, k, field), b.get_float64(0));
    b.create_atomic_add(b.create_external_ptr(a.visits, cell(b, 0, node)),
                        b.get_int64(1));
    count(b, a.stats, Visits);
    count(b, a.activity, Recomputed);
    count(b, a.activity, GroupGateEntries);
    count(b, a.activity, Flags);
    auto *selector = b.create_atomic_or(
        b.create_external_ptr(a.selector, {node}), b.get_int32(0));
    std::array<Stmt *, 4> old{};
    for (int k = 0; k < 4; ++k)
      old[k] = read(
          b, a.values,
          {b.create_add(b.create_mul(selector, b.get_int32(4)), b.get_int32(k)),
           node});
    auto *mass = b.create_local_var(PrimitiveType::f64);
    b.create_local_store(mass, b.get_float64(0));
    std::array<AllocaStmt *, 3> anchor{}, sum{};
    for (int k = 0; k < 3; ++k) {
      anchor[k] = b.create_local_var(PrimitiveType::f64);
      sum[k] = b.create_local_var(PrimitiveType::f64);
      b.create_local_store(anchor[k], b.get_float64(0));
      b.create_local_store(sum[k], b.get_float64(0));
    }
    auto *branch = b.create_if(read(b, a.cfg, cell(b, ChildCount, node)));
    for (int parent = 0; parent < 2; ++parent) {
      auto mode = b.get_if_guard(branch, parent == 1);
      auto *begin =
          read(b, a.cfg, cell(b, parent ? ChildBegin : SourceBegin, node));
      auto *length =
          read(b, a.cfg, cell(b, parent ? ChildCount : SourceCount, node));
      auto *members =
          b.create_range_for(b.get_int32(0), length, false, 1, 0, true);
      auto member = b.get_loop_guard(members);
      auto *index = b.get_loop_index(members);
      auto *ref = b.create_add(begin, index);
      std::array<Stmt *, 4> value{};
      if (parent) {
        value = packet(b, a, read(b, a.children, {ref}));
        count(b, a.stats, ChildReads);
        count(b, a.activity, ReducedEdges);
      } else {
        auto *body = read(b, a.sources, cell(b, 0, ref));
        for (int k = 0; k < 3; ++k)
          value[k] = read(b, a.bodies, cell(b, kPosX + k, body));
        value[3] = b.create_mul(
            read(b, a.bodies, cell(b, kMass, body)),
            b.create_div(b.create_cast(read(b, a.ratios, cell(b, 0, ref)),
                                       PrimitiveType::f64),
                         b.create_cast(read(b, a.ratios, cell(b, 1, ref)),
                                       PrimitiveType::f64)));
        count(b, a.stats, SourceReads);
        count(b, a.activity, ReducedEdges);
      }
      for (auto *v : value)
        mark_nonfinite(b, a.stats, v, Errors);
      {
        auto bad = b.get_if_guard(
            b.create_if(b.create_cmp_le(value[3], b.get_float64(0))), true);
        count(b, a.stats, Errors);
      }
      for (int k = 0; k < 3; ++k) {
        {
          auto first = b.get_if_guard(
              b.create_if(b.create_cmp_eq(index, b.get_int32(0))), true);
          b.create_local_store(anchor[k], value[k]);
        }
        b.create_local_store(
            sum[k],
            b.create_add(
                b.create_local_load(sum[k]),
                b.create_mul(
                    value[3],
                    b.create_sub(value[k], b.create_local_load(anchor[k])))));
      }
      b.create_local_store(mass,
                           b.create_add(b.create_local_load(mass), value[3]));
    }
    std::array<Stmt *, 4> next{};
    next[3] = b.create_local_load(mass);
    Stmt *changed = b.create_cmp_ne(next[3], old[3]);
    for (int k = 0; k < 3; ++k) {
      auto *trial =
          b.create_add(b.create_local_load(anchor[k]),
                       b.create_div(b.create_local_load(sum[k]), next[3]));
      next[k] = b.create_bit_cast(
          b.create_and(b.create_bit_cast(trial, PrimitiveType::u64),
                       read(b, a.masks, {node})),
          PrimitiveType::f64);
      mark_nonfinite(b, a.stats, trial, Errors);
      mark_nonfinite(b, a.stats, next[k], Errors);
      changed = b.create_logical_or(changed, b.create_cmp_ne(next[k], old[k]));
    }
    mark_nonfinite(b, a.stats, next[3], Errors);
    auto publish = b.get_if_guard(b.create_if(changed), true);
    write(b, a.group_flags, cell(b, Changed, field), b.get_int32(1));
    auto *slot = b.create_xor(selector, b.get_int32(1));
    for (int k = 0; k < 4; ++k) {
      write(b, a.old, cell(b, k, node), old[k]);
      write(b, a.values,
            {b.create_add(b.create_mul(slot, b.get_int32(4)), b.get_int32(k)),
             node},
            next[k]);
      write(b, a.groups, cell(b, k, field), next[k]);
      if (k < 3)
        write(b, a.group_delta, cell(b, 3 + k, field), old[k]);
    }
    b.create_atomic_xor(b.create_external_ptr(a.selector, {node}),
                        b.get_int32(1));
    b.create_atomic_add(b.create_external_ptr(a.visits, cell(b, 1, node)),
                        b.get_int64(1));
    count(b, a.stats, Publications);
    {
      auto *parent = read(b, a.cfg, cell(b, Parent, node));
      auto has_parent = b.get_if_guard(
          b.create_if(b.create_cmp_ge(parent, b.get_int32(0))), true);
      h.activity_->centroids.activate(b,
                                      read(b, a.cfg, cell(b, Field, parent)));
    }
    auto *begin = read(b, a.cfg, cell(b, CoverBegin, node));
    auto *recipients = b.create_range_for(
        b.get_int32(0), read(b, a.cfg, cell(b, CoverCount, node)), false, 1, 0,
        true);
    auto recipient = b.get_loop_guard(recipients);
    auto *body = read(b, a.coverage,
                      {b.create_add(begin, b.get_loop_index(recipients))});
    count(b, a.stats, RecipientChecks);
    count(b, a.activity, GeometryChecks);
    count(b, a.activity, AdjacencyVisits);
    std::array<AllocaStmt *, 3> before{}, after{};
    for (int k = 0; k < 3; ++k) {
      before[k] = b.create_local_var(PrimitiveType::f64);
      after[k] = b.create_local_var(PrimitiveType::f64);
      b.create_local_store(before[k], b.get_float64(0));
      b.create_local_store(after[k], b.get_float64(0));
    }
    {
      auto *refs = b.create_range_for(read(b, a.offsets, cell(b, 0, body)),
                                      read(b, a.offsets, cell(b, 1, body)),
                                      false, 1, 0, true);
      auto refs_scope = b.get_loop_guard(refs);
      auto *ref = b.get_loop_index(refs);
      count(b, a.stats, WakeInfluenceReads);
      auto *cn = read(b, a.influences, cell(b, 1, ref)),
           *pn = read(b, a.influences, cell(b, 2, ref));
      auto c = packet(b, a, cn), p = packet(b, a, pn);
      auto f0 = contribution(b, a, body, ref, substitute(b, cn, node, c, old),
                             substitute(b, pn, node, p, old));
      auto f1 = contribution(b, a, body, ref, substitute(b, cn, node, c, next),
                             substitute(b, pn, node, p, next));
      for (int k = 0; k < 3; ++k) {
        b.create_local_store(
            before[k], b.create_add(b.create_local_load(before[k]), f0[k]));
        b.create_local_store(
            after[k], b.create_add(b.create_local_load(after[k]), f1[k]));
      }
    }
    Stmt *different = b.create_cmp_ne(next[3], old[3]);
    for (int k = 0; k < 3; ++k)
      different = b.create_logical_or(
          different, b.create_cmp_ne(b.create_local_load(before[k]),
                                     b.create_local_load(after[k])));
    // Pairwise deletion from a mixed publication set cannot by itself certify
    // that the cumulative input equals the body's last evaluated input. Retain
    // the existing kForce baseline and compare a fresh complete combined force,
    // with canonical contacts. No new force cache or activity registry is
    // needed.
    for (int side = 0; side < 2; ++side) {
      auto *off = side ? a.b_offsets : a.a_offsets,
           *order = side ? a.b_order : a.a_order;
      auto *contacts =
          b.create_range_for(read(b, off, cell(b, 0, body)),
                             read(b, off, cell(b, 1, body)), false, 1, 0, true);
      auto contact = b.get_loop_guard(contacts);
      count(b, a.stats, WakeContactReads);
      auto *pair = read(b, order, {b.get_loop_index(contacts)});
      auto *pa = read(b, a.pairs, cell(b, kContactA, pair));
      auto *pb = read(b, a.pairs, cell(b, kContactB, pair));
      std::array<Stmt *, 3> delta{};
      for (int k = 0; k < 3; ++k)
        delta[k] = b.create_sub(read(b, a.bodies, cell(b, kPosX + k, pb)),
                                read(b, a.bodies, cell(b, kPosX + k, pa)));
      const auto geom =
          contact_geometry(b, delta, read(b, a.bodies, cell(b, kRadius, pa)),
                           read(b, a.bodies, cell(b, kRadius, pb)), true);
      auto *scale =
          b.create_neg(b.create_mul(b.create_mul(a.stiffness, geom.penetration),
                                    b.create_mul(geom.live, geom.inverse)));
      for (int k = 0; k < 3; ++k) {
        Stmt *force = b.create_mul(scale, delta[k]);
        if (side)
          force = b.create_neg(force);
        b.create_local_store(
            after[k], b.create_add(b.create_local_load(after[k]), force));
      }
    }
    Stmt *baseline_changed = b.get_int32(0);
    for (int k = 0; k < 3; ++k) {
      auto *current = b.create_local_load(after[k]);
      mark_nonfinite(b, a.stats, current, Errors);
      baseline_changed = b.create_logical_or(
          baseline_changed,
          b.create_cmp_ne(current,
                          read(b, a.bodies, cell(b, kForceX + k, body))));
    }
    {
      auto conservative = b.get_if_guard(
          b.create_if(b.create_logical_and(b.create_logical_not(different),
                                           baseline_changed)),
          true);
      count(b, a.stats, ConservativeWakes);
    }
    auto wake = b.get_if_guard(
        b.create_if(b.create_logical_or(different, baseline_changed)), true);
    h.activity_->particle_activity.activate(b, body);
    poll_kernel = engine::CompiledKernel(
        *h.runtime_, "live_hierarchy_poll_" + suffix, b.extract_ir(),
        signature(), engine::MathMode::Strict);
  }
}

LiveHierarchy::LiveHierarchy(Harness &h) : impl_(std::make_unique<Impl>(h)) {
}
LiveHierarchy::~LiveHierarchy() = default;
bool LiveHierarchy::installed() const {
  return impl_->bound;
}
std::uint64_t LiveHierarchy::response_count() const {
  return impl_->responses;
}
void LiveHierarchy::validate() const {
  if (impl_->bound && impl_->generation != impl_->h.prepared_->generation)
    throw std::logic_error("live hierarchy source generation changed");
}
std::size_t LiveHierarchy::binding_bytes() const {
  return impl_->force_kernel.prepared_bytes() +
         impl_->poll_kernel.prepared_bytes() +
         impl_->command_kernel.prepared_bytes() +
         impl_->refresh->candidate.prepared_bytes() +
         impl_->refresh->commit.prepared_bytes();
}
std::array<const engine::CompiledKernel *, 3> LiveHierarchy::kernels() const {
  return {&impl_->force_kernel, &impl_->poll_kernel, &impl_->command_kernel};
}
std::size_t LiveHierarchy::fixed_object_bytes() const {
  return sizeof(Impl) + sizeof(Impl::Refresh);
}
void LiveHierarchy::poll_target(double gravity, int target, double stiffness) {
  validate();
  impl_->launch(impl_->poll_kernel, gravity, target, stiffness);
}
const Ndarray &LiveHierarchy::leaf_fields() const {
  return *impl_->arrays[17];
}
void LiveHierarchy::force(double gravity) {
  validate();
  impl_->launch(impl_->command_kernel, gravity, 0);
  impl_->launch(impl_->force_kernel, gravity, -1);
}
void LiveHierarchy::poll(double gravity, double stiffness) {
  validate();
  impl_->launch(impl_->poll_kernel, gravity, -1, stiffness);
  engine::readback(*impl_->arrays[14], impl_->inspection.counters.data(),
                   sizeof(impl_->inspection.counters));
  if (impl_->inspection.counters[Errors])
    throw std::runtime_error("live hierarchy numerical invariant failed");
}
const LiveHierarchyInspection &LiveHierarchy::inspect() {
  auto &a = *impl_;
  a.h.runtime_->synchronize();
  engine::readback(*a.arrays[0], a.inspection.slots.data(),
                   a.inspection.slots.size() * 8);
  engine::readback(*a.arrays[1], a.inspection.selectors.data(),
                   a.inspection.selectors.size() * 4);
  engine::readback(*a.arrays[13], a.inspection.previous.data(),
                   a.inspection.previous.size() * 8);
  engine::readback(*a.arrays[14], a.inspection.counters.data(),
                   sizeof(a.inspection.counters));
  engine::readback(*a.owned[19], a.inspection.visits.data(),
                   a.inspection.visits.size() * 8);
  return a.inspection;
}

LiveHierarchyResult LiveHierarchy::install(std::uint64_t expected,
                                           const WeightedCascadeSpec &spec,
                                           std::uint64_t work_limit) {
  auto &a = *impl_;
  auto &h = a.h;
  auto &s = *h.prepared_;
  LiveHierarchyResult out;
  out.generation = s.generation;
  if (!h.ready_) {
    out.status = CascadeStatus::Unready;
    return out;
  }
  if (a.bound || !h.profile_.wide() ||
      (h.runtime_->arch() != taichi::Arch::x64 && !a.cuda_qualification_device))
    return out;
  if (expected != s.generation) {
    out.status = CascadeStatus::Stale;
    return out;
  }
  if (spec.node_count != s.live.fields || !spec.node_count) {
    out.status = CascadeStatus::InvalidHierarchy;
    return out;
  }
  const auto binding =
      h.centroid_cascade_->bind_weighted(expected, spec, work_limit, true);
  out.status = binding.status;
  out.failure = binding.failure;
  out.preparation_visits = binding.preparation_visits;
  out.upload_bytes = binding.uploaded_bytes;
  out.readback_bytes = binding.readback_bytes;
  out.synchronizations = binding.synchronizations;
  if (binding.status != CascadeStatus::Ok)
    return out;
  const auto budget = work_limit == UINT64_MAX
                          ? s.capacity.limits()[Budget::CascadeWork]
                          : work_limit;
  const auto query =
      h.query_centroid_cascade(expected, budget - out.preparation_visits);
  out.reference_work = query.required_work;
  out.readback_bytes += query.readback_bytes;
  out.synchronizations += query.synchronizations;
  if (query.status != CascadeStatus::Ok) {
    out.status = query.status;
    out.failure = query.failure;
    if (out.failure.budget == Budget::CascadeWork) {
      out.failure.required += out.preparation_visits;
      out.failure.capacity = budget;
    }
    return out;
  }
  out.preparation_visits += query.required_work;
  const auto &v = h.inspect_centroid_cascade(query);
  std::fill(a.owners.begin(), a.owners.end(), -1);
  std::fill(a.leaf.begin(), a.leaf.end(), 0);
  std::fill(a.cfg.begin(), a.cfg.end(), 0);
  std::fill_n(a.cfg.begin(), a.H, -1);
  const auto charge = [&]() {
    if (out.preparation_visits >= budget) {
      out.status = CascadeStatus::CapacityExhausted;
      out.failure = {Budget::CascadeWork, out.preparation_visits + 1, budget};
      return false;
    }
    ++out.preparation_visits;
    return true;
  };
  for (std::size_t n = 0; n < spec.node_count; ++n) {
    const auto &node = v.nodes[n];
    int f = node.field.record;
    a.inspection.nodes[n] = node;
    if (!node.publish || a.owners[f] >= 0) {
      out.status = CascadeStatus::Unsupported;
      return out;
    }
    a.owners[f] = int(n);
    a.inspection.fields[n] = f;
    std::size_t full = 0;
    for (std::size_t e = 0; e < s.live.edges; ++e) {
      if (!charge())
        return out;
      if (s.edges[e].second.record == f)
        ++full;
    }
    if (full != node.cohort_count) {
      out.status = CascadeStatus::Unsupported;
      return out;
    }
    a.config(ChildBegin, n) = int(node.child_begin);
    a.config(ChildCount, n) = int(node.child_count);
    a.config(SourceBegin, n) = int(node.cohort_begin);
    a.config(SourceCount, n) = int(node.cohort_count);
    a.config(Field, n) = f;
    a.config(CoverBegin, n) = v.coverage_offsets[n];
    a.config(CoverCount, n) = v.coverage_offsets[n + 1] - v.coverage_offsets[n];
    a.config(MultipleBodies, n) = a.config(CoverCount, n) > 1;
    a.leaf[f] = node.child_count == 0;
    a.masks[n] = engine::numeric::Precision(node.calculation, node.storage,
                                            node.retained_fraction_bits)
                     .fraction_mask();
    for (std::size_t j = 0; j < node.child_count; ++j) {
      if (!charge())
        return out;
      int child = v.children[node.child_begin + j];
      a.children[node.child_begin + j] = child;
      a.config(Parent, child) = int(n);
    }
    for (std::size_t j = 0; j < node.cohort_count; ++j) {
      if (!charge())
        return out;
      auto q = node.cohort_begin + j;
      auto row = v.source_rows[q].row;
      a.source_headers[q] = v.source_rows[q];
      a.sources[q] = v.cohorts[q].record;
      a.sources[a.S + q] = int(row);
      a.ratios[q] = s.source_counts[row];
      a.ratios[a.S + q] = s.source_masses[v.cohorts[q].record];
    }
    for (int j = v.coverage_offsets[n]; j < v.coverage_offsets[n + 1]; ++j)
      a.coverage[j] = v.coverage[j].occurrence.record;
  }
  for (int f = 0; f < h.groups_; ++f)
    if (s.field_headers[f].record >= 0 && a.owners[f] < 0) {
      out.status = CascadeStatus::Unsupported;
      return out;
    }
  for (std::size_t n = 0; n + 1 < spec.node_count; ++n) {
    auto p = a.config(Parent, n);
    // Exact parent coverage is the sum of children; any sibling contribution
    // supplies a complement. A unary aggregation has none, irrespective of Q.
    a.config(Complement, n) = a.config(ChildCount, p) > 1;
  }
  std::size_t nr = 0;
  for (int body = 0; body < h.particles_; ++body) {
    a.offsets[body] = int(nr);
    bool local = false;
    for (std::size_t n = 0; n < spec.node_count; ++n) {
      const auto &node = v.nodes[n];
      const auto add = [&](int kind, int parent, std::uint64_t c,
                           std::uint64_t d) {
        if (nr == std::size_t(a.R)) {
          out.status = CascadeStatus::CapacityExhausted;
          out.failure = {Budget::LiveResponses, nr + 1, std::uint64_t(a.R)};
          return false;
        }
        a.influences[nr] = body;
        a.influences[a.R + nr] = int(n);
        a.influences[2 * a.R + nr] = parent;
        a.influences[3 * a.R + nr] = kind;
        a.weights[nr] = c;
        a.weights[a.R + nr] = d;
        ++nr;
        return true;
      };
      if (!node.child_count)
        for (std::size_t j = node.cohort_begin;
             j < node.cohort_begin + node.cohort_count; ++j) {
          if (!charge())
            return out;
          if (v.cohorts[j].record == body) {
            local = true;
            if (!add(0, int(n), a.ratios[j], a.ratios[a.S + j]))
              return out;
          }
        }
      if (n + 1 < spec.node_count)
        for (int j = v.coverage_offsets[n]; j < v.coverage_offsets[n + 1];
             ++j) {
          if (!charge())
            return out;
          const auto &c = v.coverage[j];
          if (c.occurrence.record == body &&
              !add(1, a.config(Parent, n), c.count, c.denominator))
            return out;
        }
    }
    a.offsets[h.particles_ + body] = int(nr);
    if (s.particle_headers[body].record >= 0 && !local) {
      out.status = CascadeStatus::Unsupported;
      return out;
    }
  }
  // No committed publication is touched until every structural/work check above
  // succeeds. Native/backend failures from here make the execution unready.
  try {
    for (std::size_t n = 0; n < spec.node_count; ++n)
      for (int slot = 0; slot < 2; ++slot)
        for (int k = 0; k < 4; ++k)
          a.inspection.slots[(slot * 4 + k) * a.H + n] =
              v.values[(k == 3 ? 6 : k + 3) * a.H + n];
    std::fill(a.inspection.selectors.begin(), a.inspection.selectors.end(), 0);
    const auto upload = [&](int i, const auto &x) {
      const auto bytes = x.size() * sizeof(x[0]);
      engine::upload(*a.arrays[i], x.data(), bytes);
      out.upload_bytes += bytes;
    };
    upload(0, a.inspection.slots);
    upload(1, a.inspection.selectors);
    upload(2, a.cfg);
    upload(3, a.children);
    upload(4, a.sources);
    upload(5, a.ratios);
    upload(6, a.masks);
    upload(9, a.coverage);
    upload(10, a.influences);
    upload(11, a.weights);
    upload(12, a.offsets);
    upload(17, a.leaf);
    upload(18, a.owners);
    a.inspection.node_count = spec.node_count;
    for (std::size_t i = 0; i < spec.node_count; ++i)
      a.inspection.order[i] = int(i);
    a.nodes = spec.node_count;
    a.responses = nr;
    a.generation = s.generation;
    a.launch(a.command_kernel, 0, 1);
    h.runtime_->synchronize();
    ++out.synchronizations;
    a.bound = true;
    h.invalidated_ = true;
    out.status = CascadeStatus::Ok;
    out.nodes = a.nodes;
    out.responses = nr;
    return out;
  } catch (...) {
    h.ready_ = false;
    throw;
  }
}
#include "live_hierarchy_refresh.inc"
}  // namespace field::detail
