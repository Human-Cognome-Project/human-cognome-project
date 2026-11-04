#include "field/placement.h"

#include <algorithm>
#include <cfenv>
#include <cmath>
#include <stdexcept>
#include "engine/transfer.h"
#include "field/activity.h"
#include "field/admission_contacts.h"
#include "field/placement_search.h"
#include "field/prepared_storage.h"
#include "taichi/jit/jit_module.h"
#include "taichi/runtime/program_impls/llvm/llvm_program.h"

namespace field::detail {
namespace {
using namespace taichi::lang;
using Param = engine::Param;
using PlacementCounters = placement::Search;
std::uint64_t native_requested(engine::Runtime &r) {
  r.synchronize();
  auto *impl = static_cast<LlvmProgramImpl *>(r.program().get_program_impl());
  auto *executor = impl->get_runtime_executor();
  executor->get_runtime_jit_module()->call<void *>(
      "runtime_LLVMRuntime_get_total_requested_memory",
      executor->get_llvm_runtime(), executor->get_llvm_runtime());
  return impl->fetch_result<std::uint64_t>(
      taichi_result_buffer_runtime_query_id, r.program().result_buffer);
}
std::unique_ptr<Ndarray> allocate(engine::Runtime &r,
                                  DataType type,
                                  std::vector<int> shape) {
  auto out = std::make_unique<Ndarray>(&r.program(), type, shape);
  r.program().fill_ndarray_fast_u32(out.get(), 0);
  return out;
}
PlacementStatus translate(UpdateStatus s) {
  switch (s) {
    case UpdateStatus::Ok:
      return PlacementStatus::Ok;
    case UpdateStatus::Stale:
      return PlacementStatus::Stale;
    case UpdateStatus::CapacityExhausted:
      return PlacementStatus::CapacityExhausted;
    case UpdateStatus::InvalidNumerics:
      return PlacementStatus::InvalidNumerics;
    case UpdateStatus::Unsupported:
      return PlacementStatus::Unsupported;
    case UpdateStatus::Unready:
      return PlacementStatus::Unready;
    default:
      return PlacementStatus::InvalidInput;
  }
}
bool coordinate(double x) {
  return NumericalProfile::supported_real(x) &&
         (x == 0 || (std::abs(x) >= 0x1p-256 && std::abs(x) <= 0x1p128));
}
bool radius_supported(double x) {
  return x == 0 || (x >= 0x1p-128 && x <= 0x1p128);
}
}  // namespace

void Placement::append_layout(const BudgetDemand &l,
                              std::vector<StorageExtent> &out) {
  if (!l[Budget::PlacementWork])
    return;
  const auto p = std::max<std::uint64_t>(l[Budget::Particles], 1),
             b = std::max({l[Budget::Batch], l[Budget::BootstrapCommands],
                           std::uint64_t{1}}),
             c = l[Budget::PlacementCells];
  const auto add = [&](const char *name, std::uint64_t count,
                       std::size_t bytes) {
    if (count > SIZE_MAX / bytes)
      throw std::overflow_error("placement workspace overflow");
    out.push_back(
        {name, std::size_t(count), bytes, 1, std::size_t(count) * bytes});
  };
  add("placement-source-rows-and-filter", 3 * p,
      sizeof(int));  // native rows/filter + host filter
  add("placement-geometry-counters", 32,
      sizeof(std::int64_t));                                // native + host
  add("placement-focal", 8, sizeof(double));                // native + host
  add("placement-intrinsic-input", 6 * b, sizeof(double));  // native + host
  add("placement-occupancy", 4 * (p + b), sizeof(double));
  add("placement-interval-heap", 7 * c, sizeof(double));
  add("placement-candidate-output", 21 * b,
      sizeof(double));  // attempt, published, inspection
  add("placement-status-and-work", 32, sizeof(std::uint64_t));
  add("placement-intrinsic-provenance", 2 * b, sizeof(PlacementIntrinsic));
  add("placement-proposal-provenance", 2 * b, sizeof(UpdateCommand));
  add("placement-inspection-certificates", b, sizeof(PlacementCertificate));
  add("placement-array-control", 10, sizeof(Ndarray) + 4 * sizeof(int));
  add("placement-fixed-owner-control", 1, sizeof(Placement));
  add("exchange-state", 8, sizeof(double));
  add("exchange-state-control", 1, sizeof(Ndarray) + 4 * sizeof(int));
  if (l[Budget::BootstrapCommands]) {
    add("bootstrap-focal-contributions", 6 * std::max<std::uint64_t>(
        l[Budget::Participation], 1), sizeof(std::uint64_t));
    add("bootstrap-focal-control", 1, sizeof(Ndarray) + 4 * sizeof(int));
  }
  AdmissionContacts::append_layout(l, out);
}
Placement::Placement(Harness &h) : h_(h) {
  if (!h.prepared_ || !h.activity_ || !h.profile_.wide() ||
      h.runtime_->arch() != taichi::Arch::x64 ||
      &h.activity_->runtime_ != h.runtime_ || h.activity_->p_ != h.particles_ ||
      h.activity_->g_ != h.groups_ || std::fegetround() != FE_TONEAREST)
    throw std::invalid_argument(
        "placement requires an exclusive matching x64/f64 owner and "
        "round-to-nearest");
  h.validate_prepared_bindings();
  auto &r = *h.runtime_;
  const auto before = native_requested(r);
  const int p = std::max(h.particles_, 1),
            b = std::max<std::uint64_t>(
                h.prepared_->capacity.command_capacity(), 1),
            c = h.prepared_->capacity.limits()[Budget::PlacementCells];
  const auto d = PrimitiveType::f64, i = PrimitiveType::i32;
  rows_ = allocate(r, i, {p});
  stats_ = allocate(r, PrimitiveType::i64, {16});
  focal_ = allocate(r, d, {4});
  keep_ = allocate(r, i, {p});
  requests_ = allocate(r, d, {3, b});
  occupancy_ = allocate(r, d, {4, p + b});
  cells_ = allocate(r, d, {7, c});
  attempt_ = allocate(r, d, {7, b});
  published_ = allocate(r, d, {7, b});
  counts_ = allocate(r, PrimitiveType::u64, {16});
  exchange_state_ = allocate(r, d, {8});
  if (h.prepared_->capacity.limits()[Budget::BootstrapCommands]) {
    const int e = std::max(h.edges_, 1);
    bootstrap_contributions_ = allocate(r, PrimitiveType::u64, {3, e});
    bootstrap_values_.resize(3 * e);
  }
  keep_values_.resize(p);
  request_values_.resize(3 * b);
  result_values_.resize(7 * b);
  intrinsics_.resize(b);
  published_intrinsics_.resize(b);
  published_commands_.resize(b);
  inspection_.certificates.reserve(b);
  inspection_.proposal.reserve(b);
  build();
  build_exchange();
  reduce_exchange();
  collect();
  capture();
  solve(0, INT64_MAX, 0, c);
  if (bootstrap_contributions_)
    solve(0, INT64_MAX, 0, c, true);
  publish(false);
  initialize(0);
  if (h.prepared_->capacity.limits()[Budget::LiveResponses])
    contacts_ = std::make_unique<AdmissionContacts>(h);
  r.synchronize();
  native_bytes_ = native_requested(r) - before;
}
Placement::~Placement() = default;
void Placement::build() {
  auto &r = *h_.runtime_;
  using namespace taichi::lang;
  collect_ = h_.build_geometry_collect_kernel(h_.activity_->particle_activity);
  {
    IRBuilder b;
    auto *bodies = b.create_ndarray_arg_load({0}, PrimitiveType::f64, 2, 0),
         *rows = b.create_ndarray_arg_load({1}, PrimitiveType::i32, 1, 0),
         *keep = b.create_ndarray_arg_load({2}, PrimitiveType::i32, 1, 0),
         *stats = b.create_ndarray_arg_load({3}, PrimitiveType::i64, 1, 0),
         *out = b.create_ndarray_arg_load({4}, PrimitiveType::f64, 2, 0),
         *counts = b.create_ndarray_arg_load({5}, PrimitiveType::u64, 1, 0);
    const auto read = [&](ArgLoadStmt *a, std::vector<Stmt *> at) {
      return b.create_global_load(b.create_external_ptr(a, at));
    };
    const auto write = [&](ArgLoadStmt *a, std::vector<Stmt *> at, Stmt *v) {
      b.create_global_store(b.create_external_ptr(a, at), v);
    };
    auto *status = b.create_local_var(PrimitiveType::i32);
    b.create_local_store(status, b.get_int32(0));
    placement::Math m(b, status);
    for (int k = 0; k < 16; ++k)
      write(counts, {b.get_int32(k)}, b.get_uint64(0));
    auto *n = b.create_local_var(PrimitiveType::i32);
    b.create_local_store(n, b.get_int32(0));
    auto *loop = b.create_range_for(
        b.get_int32(0),
        b.create_cast(read(stats, {b.get_int32(0)}), PrimitiveType::i32), false,
        1, 0, true);
    {
      auto guard = b.get_loop_guard(loop);
      auto *record = read(rows, {b.get_loop_index(loop)});
      auto retained = b.get_if_guard(
          b.create_if(b.create_cmp_gt(read(keep, {record}), b.get_int32(0))),
          true);
      const auto value = [&](int k) {
        return read(bodies, {b.get_int32(k), record});
      };
      for (int k = 0; k < 3; ++k) {
        auto *x = value(kPosX + k);
        m.invalid(b.create_logical_not(m.supported(x, -256, 128)));
        write(out, {b.get_int32(k), b.create_local_load(n)}, x);
      }
      auto *radius = value(kRadius);
      m.invalid(b.create_logical_not(m.both(b.create_cmp_ge(radius, m.f(0)),
                                            m.supported(radius, -128, 128))));
      write(out, {b.get_int32(3), b.create_local_load(n)}, radius);
      b.create_local_store(
          n, b.create_add(b.create_local_load(n), b.get_int32(1)));
    }
    write(counts, {b.get_int32(PlacementCounters::Status)},
          b.create_cast(b.create_local_load(status), PrimitiveType::u64));
    write(counts, {b.get_int32(PlacementCounters::Occupants)},
          b.create_cast(b.create_local_load(n), PrimitiveType::u64));
    capture_ = engine::CompiledKernel(
        r, "placement_capture", b.extract_ir(),
        {Param::array(PrimitiveType::f64, 2), Param::array(PrimitiveType::i32),
         Param::array(PrimitiveType::i32), Param::array(PrimitiveType::i64),
         Param::array(PrimitiveType::f64, 2), Param::array(PrimitiveType::u64)},
        engine::MathMode::Strict);
  }
  for (bool bootstrap : {false, true}) {
    if (bootstrap && !bootstrap_contributions_)
      continue;
    IRBuilder b;
    auto *occupants = b.create_ndarray_arg_load({0}, PrimitiveType::f64, 2, 0),
         *input = b.create_ndarray_arg_load({1}, PrimitiveType::f64, 2, 0),
         *focus = b.create_ndarray_arg_load({2}, PrimitiveType::f64, 1, 0),
         *cells = b.create_ndarray_arg_load({3}, PrimitiveType::f64, 2, 0),
         *out = b.create_ndarray_arg_load({4}, PrimitiveType::f64, 2, 0),
         *counts = b.create_ndarray_arg_load({5}, PrimitiveType::u64, 1, 0);
    auto *number = b.create_arg_load({6}, PrimitiveType::i32, false, 0),
         *current = b.create_arg_load({7}, PrimitiveType::i32, false, 0),
         *capacity = b.create_arg_load({10}, PrimitiveType::i32, false, 0);
    auto *limit =
             b.create_cast(b.create_arg_load({8}, PrimitiveType::i64, false, 0),
                           PrimitiveType::u64),
         *prefix =
             b.create_cast(b.create_arg_load({9}, PrimitiveType::i64, false, 0),
                           PrimitiveType::u64);
    placement::Search search(b, occupants, input, focus, cells, out, counts,
                             number, current, limit, prefix, capacity,
                             h_.profile_.body_fraction_bits,
                             bootstrap ? b.create_ndarray_arg_load(
                                 {12}, PrimitiveType::u64, 2, 0) : nullptr,
                             bootstrap ? b.create_arg_load(
                                 {13}, PrimitiveType::i32, false, 0) : nullptr);
    search.emit();
    std::vector<Param> parameters{
         Param::array(PrimitiveType::f64, 2),
         Param::array(PrimitiveType::f64, 2), Param::array(PrimitiveType::f64),
         Param::array(PrimitiveType::f64, 2),
         Param::array(PrimitiveType::f64, 2), Param::array(PrimitiveType::u64),
         Param::scalar(PrimitiveType::i32), Param::scalar(PrimitiveType::i32),
         Param::scalar(PrimitiveType::i64), Param::scalar(PrimitiveType::i64),
         Param::scalar(PrimitiveType::i32), Param::scalar(PrimitiveType::i32)};
    if (bootstrap) {
      parameters.push_back(Param::array(PrimitiveType::u64, 2));
      parameters.push_back(Param::scalar(PrimitiveType::i32));
    }
    (bootstrap ? bootstrap_solve_ : solve_) = engine::CompiledKernel(
        r, std::string(bootstrap ? "placement_bootstrap_solve_b" : "placement_solve_b") +
               std::to_string(h_.profile_.body_fraction_bits),
        b.extract_ir(), parameters,
        engine::MathMode::Strict, engine::Optimization::Local);
  }
  {
    IRBuilder b;
    auto *source = b.create_ndarray_arg_load({0}, PrimitiveType::f64, 2, 0),
         *destination =
             b.create_ndarray_arg_load({1}, PrimitiveType::f64, 2, 0);
    const int count = std::max<std::uint64_t>(
        h_.prepared_->capacity.command_capacity(), 1);
    for (int field = 0; field < 7; ++field) {
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(count), false,
                                      h_.runtime_->cpu_threads());
      auto guard = b.get_loop_guard(loop);
      auto *enabled = b.create_arg_load({2}, PrimitiveType::i32, false, 0);
      auto copy = b.get_if_guard(b.create_if(enabled), true);
      auto *row = b.get_loop_index(loop);
      b.create_global_store(
          b.create_external_ptr(destination, {b.get_int32(field), row}),
          b.create_global_load(
              b.create_external_ptr(source, {b.get_int32(field), row})));
    }
    publish_ = engine::CompiledKernel(
        r, "placement_publish_b" + std::to_string(count), b.extract_ir(),
        {Param::array(PrimitiveType::f64, 2),
         Param::array(PrimitiveType::f64, 2),
         Param::scalar(PrimitiveType::i32)},
        engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    auto *positions = b.create_ndarray_arg_load({0}, PrimitiveType::f64, 2, 0),
         *inputs = b.create_ndarray_arg_load({1}, PrimitiveType::f64, 2, 0),
         *starts = b.create_ndarray_arg_load({2}, PrimitiveType::f64, 2, 0);
    auto *number = b.create_arg_load({3}, PrimitiveType::i32, false, 0);
    const int capacity = std::max<std::uint64_t>(
        h_.prepared_->capacity.command_capacity(), 1);
    // One record per iteration: zero the whole prospective row, then scatter
    // certified coordinates by command index (not canonical placement rank).
    // These are transient rows; no live body, activity or history is written.
    for (int field = 0; field < 12; ++field) {
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(capacity),
                                     false, r.cpu_threads());
      auto guard = b.get_loop_guard(loop);
      b.create_global_store(b.create_external_ptr(
          starts, {b.get_int32(field), b.get_loop_index(loop)}), b.get_float64(0));
    }
    auto *loop = b.create_range_for(b.get_int32(0), number, false,
                                   r.cpu_threads());
    {
      auto guard = b.get_loop_guard(loop);
      auto *rank = b.get_loop_index(loop);
      auto *command = b.create_cast(b.create_global_load(
          b.create_external_ptr(inputs, {b.get_int32(2), rank})),
          PrimitiveType::i32);
      for (int k = 0; k < 3; ++k)
        b.create_global_store(
            b.create_external_ptr(starts, {b.get_int32(kPosX + k), command}),
            b.create_global_load(b.create_external_ptr(positions,
                {b.get_int32(k), rank})));
      for (const auto &mapping : {std::pair<int, int>{kMass, 0},
                                  std::pair<int, int>{kRadius, 1}})
        b.create_global_store(
            b.create_external_ptr(starts, {b.get_int32(mapping.first), command}),
            b.create_global_load(b.create_external_ptr(inputs,
                {b.get_int32(mapping.second), rank})));
    }
    initialize_ = engine::CompiledKernel(
        r, "placement_initialize_b" + std::to_string(capacity), b.extract_ir(),
        {Param::array(PrimitiveType::f64, 2), Param::array(PrimitiveType::f64, 2),
         Param::array(PrimitiveType::f64, 2), Param::scalar(PrimitiveType::i32)},
        engine::MathMode::Strict);
  }
}
void Placement::initialize(int bodies) {
  auto call = initialize_.invocation(true);
  auto &c = call.get();
  if (!c.prepared) {
    c.set_arg_ndarray({0}, *attempt_);
    c.set_arg_ndarray({1}, *requests_);
    c.set_arg_ndarray({2}, *h_.prepared_->new_rows_array);
  }
  engine::argument_int(c, 3, bodies);
  initialize_.launch(c);
}
void Placement::collect() {
  auto call = collect_.invocation(true);
  auto &c = call.get();
  auto &h = h_;
  if (!c.prepared) {
    h.bind(c, 0, h.particle_array_.get(), h.particle_values_,
           {kParticleFieldCount, std::max(h.particles_, 1)});
    h.bind(c, 1, h.group_array_.get(), h.group_values_,
           {kGroupFieldCount, std::max(h.groups_, 1)});
    h.bind(c, 2, h.contact_int_array_.get(), h.contact_int_data,
           {kContactIntFieldCount, std::max(h.contacts_, 1)});
    c.set_arg_ndarray({3}, *rows_);
    c.set_arg_ndarray({4}, *stats_);
    c.set_arg_ndarray({5}, *focal_);
    c.set_arg_ndarray({6}, *h.activity_->focus);
    c.set_arg_ndarray({7}, h.activity_->selected_slots());
  }
  engine::argument_int(c, 8, h.prepared_->live.focus);
  engine::argument_int(c, 9, h.prepared_->live.contacts);
  collect_.launch(c);
}
void Placement::capture() {
  auto call = capture_.invocation(true);
  auto &c = call.get();
  if (!c.prepared) {
    h_.bind(c, 0, h_.particle_array_.get(), h_.particle_values_,
            {kParticleFieldCount, std::max(h_.particles_, 1)});
    c.set_arg_ndarray({1}, *rows_);
    c.set_arg_ndarray({2}, *keep_);
    c.set_arg_ndarray({3}, *stats_);
    c.set_arg_ndarray({4}, *occupancy_);
    c.set_arg_ndarray({5}, *counts_);
  }
  capture_.launch(c);
}
void Placement::solve(int number,
                      std::uint64_t limit,
                      std::uint64_t prefix,
                      int cells, bool bootstrap) {
  auto &kernel = bootstrap ? bootstrap_solve_ : solve_;
  auto call = kernel.invocation(true);
  auto &c = call.get();
  if (!c.prepared) {
    c.set_arg_ndarray({0}, *occupancy_);
    c.set_arg_ndarray({1}, *requests_);
    c.set_arg_ndarray({2}, *focal_);
    c.set_arg_ndarray({3}, *cells_);
    c.set_arg_ndarray({4}, *attempt_);
    c.set_arg_ndarray({5}, *counts_);
    if (bootstrap)
      c.set_arg_ndarray({12}, *bootstrap_contributions_);
  }
  engine::argument_int(c, 6, number);
  engine::argument_int(c, 7, count_values_[PlacementCounters::Occupants]);
  engine::argument_int(c, 8, limit);
  engine::argument_int(c, 9, prefix);
  engine::argument_int(c, 10, cells);
  engine::argument_int(c, 11, intrinsics_.size());
  if (bootstrap)
    engine::argument_int(c, 13, bootstrap_edges_);
  kernel.launch(c);
}
void Placement::publish(bool enabled) {
  auto call = publish_.invocation(true);
  auto &c = call.get();
  if (!c.prepared) {
    c.set_arg_ndarray({0}, *attempt_);
    c.set_arg_ndarray({1}, *published_);
  }
  engine::argument_int(c, 2, enabled);
  publish_.launch(c);
}
std::size_t Placement::binding_bytes() const {
  return collect_.prepared_bytes() + capture_.prepared_bytes() +
         solve_.prepared_bytes() + publish_.prepared_bytes() +
         initialize_.prepared_bytes() + exchange_reduce_.prepared_bytes() +
         (bootstrap_contributions_ ? bootstrap_solve_.prepared_bytes() : 0) +
         (contacts_ ? contacts_->binding_bytes() : 0);
}

PlacementResult Placement::query(std::uint64_t expected,
                                 const UpdateCommand *commands,
                                 std::size_t command_count,
                                 const PlacementIntrinsic *inputs,
                                 std::size_t body_count,
                                 std::uint64_t work,
                                 std::uint64_t cells) {
  return attempt(expected, commands, command_count, inputs, body_count, work,
                 cells, true);
}
PlacementResult Placement::stage(std::uint64_t expected,
                                 const UpdateCommand *commands,
                                 std::size_t command_count,
                                 const PlacementIntrinsic *inputs,
                                 std::size_t body_count,
                                 std::uint64_t work,
                                 std::uint64_t cells, bool bootstrap,
                                 const UpdateRef *focus, std::size_t focus_count) {
  return attempt(expected, commands, command_count, inputs, body_count, work,
                 cells, false, bootstrap, focus, focus_count);
}
PlacementResult Placement::attempt(std::uint64_t expected,
                                   const UpdateCommand *commands,
                                   std::size_t command_count,
                                   const PlacementIntrinsic *inputs,
                                   std::size_t body_count,
                                   std::uint64_t work,
                                   std::uint64_t cells,
                                   bool publication, bool bootstrap,
                                   const UpdateRef *focus, std::size_t focus_count) {
  auto &h = h_;
  auto &s = *h.prepared_;
  const auto &l = s.capacity.limits();
  PlacementResult q;
  q.numerical_profile = h.profile_;
  q.context = s.particles.context();
  q.generation = s.generation;
  q.completed_ticks = s.completed_ticks;
  q.frame_anchor = h.frame_anchor_;
  const auto fail = [&](Budget b, std::uint64_t needed,
                        std::uint64_t capacity) {
    q.status = PlacementStatus::CapacityExhausted;
    q.failure = {b, needed, capacity};
    return q;
  };
  if (!h.ready_)
    return q;
  if (expected != s.generation) {
    q.status = PlacementStatus::Stale;
    return q;
  }
  if (std::fegetround() != FE_TONEAREST || serial_ == UINT64_MAX) {
    q.status = PlacementStatus::Unsupported;
    return q;
  }
  if (work == UINT64_MAX)
    work = l[Budget::PlacementWork];
  if (cells == UINT64_MAX)
    cells = l[Budget::PlacementCells];
  if (work > l[Budget::PlacementWork])
    return fail(Budget::PlacementWork, work, l[Budget::PlacementWork]);
  if (cells > l[Budget::PlacementCells])
    return fail(Budget::PlacementCells, cells, l[Budget::PlacementCells]);
  const auto command_budget = bootstrap ? Budget::BootstrapCommands : Budget::Batch;
  if (command_count > l[command_budget])
    return fail(command_budget, command_count, l[command_budget]);
  if (body_count > l[command_budget])
    return fail(command_budget, body_count, l[command_budget]);
  if (s.live.particles + body_count > l[Budget::Particles])
    return fail(Budget::Particles, s.live.particles + body_count,
                l[Budget::Particles]);
  q.status = PlacementStatus::InvalidInput;
  if (!body_count || !commands || !inputs || body_count > command_count)
    return q;
  if (bootstrap && focus_count > l[Budget::Focus])
    return fail(Budget::Focus, focus_count, l[Budget::Focus]);
  if (bootstrap && (!bootstrap_contributions_ || publication ||
                    !focus || !focus_count))
    return q;
  const auto sat_add = [](std::uint64_t a, std::uint64_t b) {
    return b > UINT64_MAX - a ? UINT64_MAX : a + b;
  };
  const auto sat_mul = [](std::uint64_t a, std::uint64_t b) {
    return a && b > UINT64_MAX / a ? UINT64_MAX : a * b;
  };
  std::uint64_t logarithm = 1;
  for (auto n = l[Budget::Participation]; n > 1; n >>= 1)
    ++logarithm;
  const auto p = l[Budget::Particles], g = l[Budget::Fields],
             e = l[Budget::Participation], c = l[Budget::Contacts],
             f = l[Budget::Focus], b = s.capacity.command_capacity();
  std::uint64_t prefix = 0;
  for (auto term : {sat_mul(16, p + g + e + c + b + f + 1),
                    sat_mul(4 * command_count, p + g + f + 12),
                    sat_mul(4 * (e + c), command_count + 1), sat_mul(4 * g, e),
                    sat_mul(64 * e, logarithm), 8 * (p + b + f) + 7 * b + 64})
    prefix = sat_add(prefix, term);
  q.source_work_bound = prefix;
  if (bootstrap) {
    prefix = sat_add(prefix, sat_mul(64, sat_mul(e + 1, body_count + f + 1)));
    prefix = sat_add(prefix, sat_mul(focus_count, focus_count));
    q.source_work_bound = prefix;
  }
  if (!publication) {
    prefix = sat_add(prefix, sat_add(sat_mul(12, b), sat_mul(8, body_count)));
    q.source_work_bound = prefix;
  }
  if (prefix > work)
    return fail(Budget::PlacementWork, prefix, work);
  std::size_t definitions = 0;
  for (std::size_t i = 0; i < command_count; ++i)
    if (commands[i].operation == UpdateOp::DefineParticle)
      ++definitions;
  if (definitions != body_count)
    return q;
  for (std::size_t i = 0; i < body_count; ++i) {
    const auto &in = inputs[i];
    if (in.command >= command_count ||
        commands[in.command].operation != UpdateOp::DefineParticle ||
        !in.geometry.complete || !in.geometry.definition ||
        !in.geometry.revision || !NumericalProfile::supported_real(in.mass) ||
        !(in.mass > 0) || !radius_supported(in.radius))
      return q;
    for (std::size_t j = 0; j < i; ++j)
      if (inputs[j].command == in.command || inputs[j].order == in.order)
        return q;
    intrinsics_[i] = in;
  }
  std::sort(intrinsics_.begin(), intrinsics_.begin() + body_count,
            [](const auto &a, const auto &b) { return a.order < b.order; });
  try {
    h.validate_prepared_bindings();
    std::fill(s.new_row_values.doubles().begin(),
              s.new_row_values.doubles().end(), 0);
    const auto stride = std::max<std::uint64_t>(b, 1);
    for (std::size_t i = 0; i < body_count; ++i) {
      const auto &in = intrinsics_[i];
      s.new_row_values[kMass * stride + in.command] = in.mass;
      s.new_row_values[kRadius * stride + in.command] = in.radius;
    }
    SourceProjection projection;
    const auto source = h.update_prepared_impl(
        expected, commands, command_count, nullptr, UINT64_MAX, &projection,
        bootstrap ? Harness::PreparedUpdateMode::BootstrapProjection
                  : Harness::PreparedUpdateMode::FixtureCommit,
        focus, focus_count);
    if (source.status != UpdateStatus::Ok) {
      q.status = translate(source.status);
      q.failure = source.failure;
      return q;
    }
    const auto references =
        s.live.particles + projection.particle_claims + s.live.fields +
        projection.field_claims +
        2 * std::max(s.live.edges, projection.counts.edges) +
        2 * std::max(s.live.contacts, projection.counts.contacts) +
        projection.counts.focus +
        (h.live_hierarchy_ ? h.live_hierarchy_->reference_count() : 0);
    if (references > l[Budget::References])
      return fail(Budget::References, references, l[Budget::References]);
    if (bootstrap) {
      // First exposed participant of the first selected field, in canonical
      // occurrence order. All other occurrences retain that canonical order.
      std::size_t anchor = body_count;
      for (std::size_t rank = 0; rank < body_count && anchor == body_count; ++rank)
        for (std::size_t edge = 0; edge < projection.counts.edges; ++edge)
          if (s.prospective_edges[edge].second.record == s.candidate_focus[0] &&
              s.prospective_edges[edge].first.record ==
                  s.resolved[intrinsics_[rank].command].record) {
            anchor = rank;
            break;
          }
      if (anchor == body_count)
        return q;
      std::rotate(intrinsics_.begin(), intrinsics_.begin() + anchor,
                  intrinsics_.begin() + anchor + 1);
      const auto es = std::max(h.edges_, 1);
      std::fill(bootstrap_values_.begin(), bootstrap_values_.end(), 0);
      bootstrap_edges_ = 0;
      for (std::size_t edge = 0; edge < projection.counts.edges; ++edge) {
        const auto &source = s.prospective_edges[edge];
        if (std::find(s.candidate_focus.begin(),
                      s.candidate_focus.begin() + focus_count,
                      source.second.record) == s.candidate_focus.begin() + focus_count)
          continue;
        std::size_t rank = 0;
        while (rank < body_count && s.resolved[intrinsics_[rank].command].record !=
                                       source.first.record)
          ++rank;
        if (rank == body_count)
          return q;
        bootstrap_values_[bootstrap_edges_] = rank;
        bootstrap_values_[es + bootstrap_edges_] = s.candidate_ratios[edge];
        bootstrap_values_[2 * es + bootstrap_edges_] = s.candidate_ratios[es + edge];
        ++bootstrap_edges_;
      }
    }
    std::fill(keep_values_.begin(), keep_values_.end(), 0);
    for (int row = 0; row < h.particles_; ++row)
      keep_values_[row] =
          s.particle_headers[row].record >= 0 && s.validation[row] > 0;
    std::fill(request_values_.begin(), request_values_.end(), 0);
    for (std::size_t i = 0; i < body_count; ++i) {
      request_values_[i] = intrinsics_[i].mass;
      request_values_[stride + i] = intrinsics_[i].radius;
      request_values_[2 * stride + i] = double(intrinsics_[i].command);
    }
    engine::upload(*keep_, keep_values_.data(),
                   keep_values_.size() * sizeof(int));
    engine::upload(*requests_, request_values_.data(),
                   request_values_.size() * sizeof(double));
    q.upload_bytes = keep_values_.size() * sizeof(int) +
                     request_values_.size() * sizeof(double);
    if (bootstrap) {
      engine::upload(*bootstrap_contributions_, bootstrap_values_.data(),
                     bootstrap_values_.size() * sizeof(std::uint64_t));
      q.upload_bytes += bootstrap_values_.size() * sizeof(std::uint64_t);
    }
    collect();
    h.runtime_->synchronize();
    ++q.synchronizations;
    engine::readback(*stats_, geometry_counts_.data(),
                     sizeof(geometry_counts_));
    engine::readback(*focal_, focal_values_.data(), sizeof(focal_values_));
    q.readback_bytes += sizeof(geometry_counts_) + sizeof(focal_values_);
    q.present = geometry_counts_[0];
    std::copy_n(focal_values_.begin(), 3, q.target.begin());
    if (geometry_counts_[5] || q.present != s.live.particles ||
        geometry_counts_[6] == int(GeometryFocus::Invalid)) {
      q.status = PlacementStatus::InvalidNumerics;
      return q;
    }
    if (geometry_counts_[6] == int(GeometryFocus::MissingSelection)) {
      q.status = PlacementStatus::InvalidInput;
      return q;
    }
    for (double x : q.target)
      if (!coordinate(x)) {
        q.status = PlacementStatus::InvalidNumerics;
        return q;
      }
    capture();
    h.runtime_->synchronize();
    ++q.synchronizations;
    engine::readback(*counts_, count_values_.data(), sizeof(count_values_));
    q.readback_bytes += sizeof(count_values_);
    if (count_values_[PlacementCounters::Status]) {
      q.status = PlacementStatus::InvalidNumerics;
      return q;
    }
    q.retained = count_values_[PlacementCounters::Occupants];
    solve(body_count, work, prefix, int(cells), bootstrap);
    h.runtime_->synchronize();
    ++q.synchronizations;
    engine::readback(*counts_, count_values_.data(), sizeof(count_values_));
    q.readback_bytes += sizeof(count_values_);
    q.work = count_values_[PlacementCounters::Work];
    q.cell_peak = count_values_[PlacementCounters::Peak];
    q.cell_visits = count_values_[PlacementCounters::Visits];
    q.candidate_tests = count_values_[PlacementCounters::CandidateTests];
    q.heap_moves = count_values_[PlacementCounters::Moves];
    q.proposals = count_values_[PlacementCounters::Proposals];
    q.status = PlacementStatus(count_values_[PlacementCounters::Status]);
    if (q.status == PlacementStatus::CapacityExhausted) {
      if (count_values_[PlacementCounters::RequiredWork])
        q.failure = {Budget::PlacementWork,
                     count_values_[PlacementCounters::RequiredWork], work};
      else
        q.failure = {Budget::PlacementCells,
                     count_values_[PlacementCounters::RequiredCells], cells};
      return q;
    }
    if (q.status != PlacementStatus::Ok)
      return q;
    if (count_values_[PlacementCounters::Completed] != body_count)
      throw std::runtime_error("incomplete certified placement result");
    if (s.failure_point == 300)
      throw std::runtime_error("injected placement candidate failure");
    if (!publication) {
      initialize(int(body_count));
      h.runtime_->synchronize();
      ++q.synchronizations;
      q.placements = body_count;
      return q;
    }
    publish(true);
    if (s.failure_point == 301)
      throw std::runtime_error("injected placement result publication failure");
    h.runtime_->synchronize();
    ++q.synchronizations;
    if (s.generation != q.generation || s.completed_ticks != q.completed_ticks)
      throw std::runtime_error("concurrent placement source mutation");
    std::copy_n(intrinsics_.begin(), body_count, published_intrinsics_.begin());
    std::copy_n(s.commands.begin(), command_count, published_commands_.begin());
    published_count_ = body_count;
    published_commands_count_ = command_count;
    q.placements = body_count;
    q.query = ++serial_;
    last_ = q;
    return q;
  } catch (...) {
    h.ready_ = false;
    throw;
  }
}
const PlacementInspection &Placement::inspect(const PlacementResult &q) {
  const auto &s = *h_.prepared_;
  if (!h_.ready_ || q.status != PlacementStatus::Ok || !q.query ||
      q.query != last_.query || q.context != last_.context ||
      q.generation != last_.generation ||
      q.completed_ticks != last_.completed_ticks ||
      last_.generation != s.generation ||
      last_.completed_ticks != s.completed_ticks)
    throw std::logic_error("stale placement candidate observation");
  try {
    engine::readback(*published_, result_values_.data(),
                     result_values_.size() * sizeof(double));
    inspection_.proposal.assign(
        published_commands_.begin(),
        published_commands_.begin() + published_commands_count_);
    inspection_.certificates.resize(published_count_);
    const auto stride = intrinsics_.size();
    for (std::size_t i = 0; i < published_count_; ++i) {
      auto &c = inspection_.certificates[i];
      c.intrinsic = published_intrinsics_[i];
      for (int k = 0; k < 3; ++k)
        c.position[k] = result_values_[k * stride + i];
      c.lower_distance = result_values_[3 * stride + i];
      c.upper_distance = result_values_[4 * stride + i];
      c.tolerance = result_values_[5 * stride + i];
      c.coordinate_bound = result_values_[6 * stride + i];
    }
    return inspection_;
  } catch (...) {
    h_.ready_ = false;
    throw;
  }
}
#include "field/placement_exchange.inc"
}  // namespace field::detail

namespace field {
PlacementResult Harness::query_prepared_placement(
    std::uint64_t generation,
    const UpdateCommand *commands,
    std::size_t count,
    const PlacementIntrinsic *inputs,
    std::size_t bodies,
    std::uint64_t work,
    std::uint64_t cells) {
  if (!placement_) {
    PlacementResult q;
    q.numerical_profile = profile_;
    q.status = PlacementStatus::Unsupported;
    return q;
  }
  return placement_->query(generation, commands, count, inputs, bodies, work,
                           cells);
}
const PlacementInspection &Harness::inspect_prepared_placement(
    const PlacementResult &q) {
  if (!placement_)
    throw std::logic_error("placement capability disabled");
  return placement_->inspect(q);
}
}  // namespace field
