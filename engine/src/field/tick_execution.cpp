#include "field/tick_execution.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <stdexcept>
#include "engine/transfer.h"
#include "field/activity.h"
#include "field/prepared_storage.h"
#include "taichi/jit/jit_module.h"
#include "taichi/runtime/program_impls/llvm/llvm_program.h"

namespace field::detail {
namespace {
using namespace taichi::lang;
using Kernel = engine::CompiledKernel;
using Param = engine::Param;
enum GeometryArray {
  Rows,
  Temporary,
  Pairs,
  PairFlags,
  SourceOrder,
  GroupEnd,
  SourceMap,
  Permutation,
  Offsets,
  Cursors,
  Orders,
  Stats,
  Focal,
  GeometryArrayCount
};
enum GeometryCount {
  Present,
  Awake,
  Sources,
  PairCount,
  Derived,
  Invalid,
  FocusState,
  ExplicitPairs,
  PresenceEntries,
  CompactionSlots
};
ArgLoadStmt *array(IRBuilder &b, int id, DataType type, int rank = 1) {
  return b.create_ndarray_arg_load({id}, type, rank, 0);
}
std::vector<Stmt *> at(IRBuilder &b, int field, Stmt *row) {
  return {b.get_int32(field), row};
}
Stmt *read(IRBuilder &b, ArgLoadStmt *a, const std::vector<Stmt *> &index) {
  return b.create_global_load(b.create_external_ptr(a, index));
}
void write(IRBuilder &b,
           ArgLoadStmt *a,
           const std::vector<Stmt *> &index,
           Stmt *value) {
  b.create_global_store(b.create_external_ptr(a, index), value);
}
void increment(IRBuilder &b, ArgLoadStmt *a, int field) {
  b.create_atomic_add(b.create_external_ptr(a, {b.get_int32(field)}),
                      b.get_int64(1));
}
std::unique_ptr<Ndarray> allocate(engine::Runtime &r,
                                  DataType t,
                                  int rank,
                                  int rows,
                                  int stride) {
  auto a = std::make_unique<Ndarray>(
      &r.program(), t,
      rank == 1 ? std::vector<int>{stride} : std::vector<int>{rows, stride});
  r.program().fill_ndarray_fast_u32(a.get(), 0);
  return a;
}
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
}  // namespace

struct TickExecution::Geometry {
  TickExecution &owner;
  std::array<std::unique_ptr<Ndarray>, GeometryArrayCount> arrays;
  std::array<Kernel, 4> kernels;
  Kernel reset;
  std::array<std::int64_t, 16> counts{};
  std::array<double, 4> focal{};
  explicit Geometry(TickExecution &o, bool complete) : owner(o) {
    auto &h = o.h_;
    const int p = std::max(h.particles_, 1), c = std::max(h.contacts_, 1);
    const auto add = [&](int id, DataType type, int rank, int rows,
                         int stride) {
      arrays[id] = allocate(*h.runtime_, type, rank, rows, stride);
    };
    add(Pairs, PrimitiveType::i32, 2, 2, c);
    add(PairFlags, PrimitiveType::i32, 1, 1, c);
    add(SourceMap, PrimitiveType::i32, 1, 1, c);
    add(Offsets, PrimitiveType::i32, 2, 4, p);
    add(Orders, PrimitiveType::i32, 2, 2, c);
    add(Stats, PrimitiveType::i64, 1, 1, 16);
    if (!complete)
      return;
    add(Rows, PrimitiveType::i32, 1, 1, p);
    add(Temporary, PrimitiveType::i32, 2, 4, c);
    add(SourceOrder, PrimitiveType::i32, 1, 1, c);
    add(GroupEnd, PrimitiveType::i32, 1, 1, c);
    add(Permutation, PrimitiveType::i32, 1, 1, c);
    add(Cursors, PrimitiveType::i32, 2, 2, p);
    add(Focal, PrimitiveType::f64, 1, 1, 4);
    kernels = h.build_geometry_kernels(o.selection_.particles());
    IRBuilder b;
    std::vector<Param> params;
    for (int id = 0; id < GeometryArrayCount; ++id) {
      const auto &a = *arrays[id];
      const int rank = int(a.shape.size());
      auto *out = array(b, id, a.dtype, rank);
      params.push_back(Param::array(a.dtype, rank));
      for (int field = 0; field < (rank == 1 ? 1 : a.shape[0]); ++field) {
        auto *loop =
            b.create_range_for(b.get_int32(0), b.get_int32(a.shape.back()),
                               false, h.runtime_->cpu_threads());
        auto guard = b.get_loop_guard(loop);
        auto *row = b.get_loop_index(loop);
        auto *zero = a.dtype == PrimitiveType::f64   ? b.get_float64(0)
                     : a.dtype == PrimitiveType::i64 ? b.get_int64(0)
                                                     : b.get_int32(0);
        write(b, out, rank == 1 ? std::vector<Stmt *>{row} : at(b, field, row),
              zero);
      }
    }
    reset = Kernel(*h.runtime_,
                   "tick_geometry_reset_" +
                       std::to_string(o.selection_.particles().cells->id),
                   b.extract_ir(), params, engine::MathMode::Strict);
  }
  void bind(LaunchContextBuilder &ctx, int argument, int id) {
    ctx.set_arg_ndarray({argument}, *arrays[id]);
  }
  void source(LaunchContextBuilder &ctx, int argument) {
    auto &h = owner.h_;
    h.bind(ctx, argument, h.contact_int_array_.get(), h.contact_int_data,
           {kContactIntFieldCount, std::max(h.contacts_, 1)});
  }
  void launch(int phase) {
    auto &h = owner.h_;
    auto call = kernels[phase].invocation(true);
    auto &ctx = call.get();
    if (!ctx.prepared) {
      switch (phase) {
        case 0:
          ctx.set_arg_ndarray({0}, *owner.values_[Bodies]);
          ctx.set_arg_ndarray({1}, *owner.values_[Groups]);
          source(ctx, 2);
          bind(ctx, 3, Rows);
          bind(ctx, 4, Stats);
          bind(ctx, 5, Focal);
          ctx.set_arg_ndarray({6}, *owner.values_[Focus]);
          ctx.set_arg_ndarray({7}, h.activity_->selected_slots());
          break;
        case 1:
          source(ctx, 0);
          bind(ctx, 1, SourceOrder);
          bind(ctx, 2, GroupEnd);
          break;
        case 2:
          ctx.set_arg_ndarray({0}, *owner.values_[Bodies]);
          source(ctx, 1);
          bind(ctx, 2, Rows);
          bind(ctx, 3, SourceOrder);
          bind(ctx, 4, GroupEnd);
          bind(ctx, 5, Temporary);
          bind(ctx, 6, SourceMap);
          bind(ctx, 7, Stats);
          break;
        case 3:
          bind(ctx, 0, Temporary);
          bind(ctx, 1, Pairs);
          bind(ctx, 2, PairFlags);
          bind(ctx, 3, Permutation);
          bind(ctx, 4, SourceMap);
          bind(ctx, 5, Offsets);
          bind(ctx, 6, Cursors);
          bind(ctx, 7, Orders);
          bind(ctx, 8, Stats);
          break;
      }
    }
    if (phase == 0) {
      engine::argument_int(ctx, 8, h.prepared_->live.focus);
      engine::argument_int(ctx, 9, h.prepared_->live.contacts);
    } else
      engine::argument_int(ctx,
                           phase == 1   ? 3
                           : phase == 2 ? 8
                                        : 9,
                           h.prepared_->live.contacts);
    kernels[phase].launch(ctx);
  }
  GeometryResult discover(std::uint64_t remaining) {
    auto &h = owner.h_;
    auto &s = *h.prepared_;
    const auto &limits = s.capacity.limits();
    GeometryResult q;
    q.numerical_profile = h.profile_;
    q.context = s.particles.context();
    q.generation = s.generation;
    q.completed_ticks = s.completed_ticks;
    q.frame_anchor = h.frame_anchor_;
    const auto exhausted = [&](Budget budget, std::uint64_t required,
                               std::uint64_t capacity) {
      q.status = GeometryStatus::CapacityExhausted;
      q.failure = {budget, required, capacity};
      return q;
    };
    {
      auto call = reset.invocation(true);
      auto &ctx = call.get();
      if (!ctx.prepared)
        for (int id = 0; id < GeometryArrayCount; ++id)
          ctx.set_arg_ndarray({id}, *arrays[id]);
      reset.launch(ctx);
    }
    launch(0);
    h.runtime_->synchronize();
    ++q.synchronizations;
    engine::readback(*arrays[Stats], counts.data(), sizeof(counts));
    engine::readback(*arrays[Focal], focal.data(), sizeof(focal));
    q.readback_bytes += sizeof(counts) + sizeof(focal);
    q.present = counts[Present];
    q.awake = counts[Awake];
    q.source_contacts = counts[Sources];
    q.presence_entries = counts[PresenceEntries];
    q.compaction_slots = counts[CompactionSlots];
    q.focus = GeometryFocus(counts[FocusState]);
    q.focal_mass = focal[3];
    std::copy_n(focal.begin(), 3, q.focal_point.begin());
    if (counts[Invalid] || q.present != s.live.particles) {
      q.status = GeometryStatus::InvalidNumerics;
      return q;
    }
    q.required_work = q.present * (q.present ? q.present - 1 : 0) / 2 +
                      q.source_contacts * q.source_contacts;
    if (q.required_work > remaining)
      return exhausted(Budget::GeometryWork, q.required_work, remaining);
    launch(1);
    launch(2);
    h.runtime_->synchronize();
    ++q.synchronizations;
    engine::readback(*arrays[Stats], counts.data(), sizeof(counts));
    q.readback_bytes += sizeof(counts);
    q.pair_tests = q.present * (q.present ? q.present - 1 : 0) / 2;
    q.source_order_tests = q.source_contacts * q.source_contacts;
    q.contacts = counts[PairCount];
    q.derived = counts[Derived];
    if (counts[Invalid]) {
      q.status = GeometryStatus::InvalidNumerics;
      return q;
    }
    if (q.contacts > limits[Budget::Contacts])
      return exhausted(Budget::Contacts, q.contacts, limits[Budget::Contacts]);
    q.required_work += q.contacts * q.contacts;
    if (q.required_work > remaining)
      return exhausted(Budget::GeometryWork, q.required_work, remaining);
    launch(3);
    h.runtime_->synchronize();
    ++q.synchronizations;
    q.union_order_tests = q.contacts * q.contacts;
    q.status = GeometryStatus::Ok;
    return q;
  }
};

void TickExecution::append_layout(const BudgetDemand &l,
                                  std::vector<StorageExtent> &out) {
  if (!l[Budget::TickWork])
    return;
  const auto p = std::max<std::uint64_t>(l[Budget::Particles], 1),
             g = std::max<std::uint64_t>(l[Budget::Fields], 1),
             c = std::max<std::uint64_t>(l[Budget::Contacts], 1),
             n = std::max<std::uint64_t>(l[Budget::CascadeNodes], 1);
  const bool live = l[Budget::LiveResponses] != 0;
  const auto add = [&](const char *name, std::uint64_t count,
                       std::size_t bytes) {
    if (count > SIZE_MAX / bytes)
      throw std::overflow_error("tick workspace overflow");
    out.push_back(
        {name, std::size_t(count), bytes, 1, std::size_t(count) * bytes});
  };
  add("tick-candidate-real",
      (kParticleFieldCount + 1 + 3) * p + (kGroupFieldCount + 6) * g + 12 +
          (live ? 12 * n : 0),
      sizeof(double));
  add("tick-candidate-flags",
      ParticleStateCount * p + GroupStateCount * g + (live ? n : 0),
      sizeof(int));
  add("tick-candidate-counters", TrialCounterCount + (live ? 10 + 2 * n : 0),
      sizeof(std::int64_t));
  add("tick-private-contact-views", 22 * p + 34 * c, sizeof(int));
  add("tick-private-geometry-counters", 48, sizeof(std::int64_t));
  add("tick-private-geometry-focus", 8, sizeof(double));
  add("tick-validation", 2, sizeof(std::int64_t));
  add("tick-fixed-owner-control", 1, sizeof(TickExecution));
  add("tick-geometry-control", 3, sizeof(Geometry));
  add("tick-copy-binding-descriptors", 21, sizeof(CopySlot));
  add("tick-native-array-control", live ? 52 : 47,
      sizeof(Ndarray) + 4 * sizeof(int));
}

void TickExecution::add(Surface id,
                        Ndarray *source,
                        RealBuffer *host,
                        DataType type,
                        int rank,
                        int rows,
                        int stride) {
  values_[id] = allocate(*h_.runtime_, type, rank, rows, stride);
  copies_.push_back(
      {source, values_[id].get(), host, type, rank, rows, stride, true});
  copy_values_ += std::uint64_t(rows) * stride;
}

TickExecution::TickExecution(Harness &h)
    : h_(h),
      native_before_(native_requested(*h.runtime_)),
      selection_(*h.runtime_,
                 h.activity_->particle_activity,
                 h.particles_,
                 h.activity_->centroids,
                 h.groups_) {
  auto &r = *h.runtime_;
  const int p = std::max(h.particles_, 1), g = std::max(h.groups_, 1);
  copies_.reserve(21);
  const auto f = PrimitiveType::f64, i = PrimitiveType::i32;
  add(Bodies, h.particle_array_.get(), &h.particle_values_, f, 2,
      kParticleFieldCount, p);
  add(Groups, h.group_array_.get(), &h.group_values_, f, 2, kGroupFieldCount,
      g);
  add(Universal, h.universal_array_.get(), &h.universal_values_, f, 1, 1, 1);
  add(Speed, h.determined_array_.get(),
      h.particles_ ? &h.determined_values_ : &h.empty_determined_values_, f, 1,
      1, p);
  add(ParticleFlags, h.activity_->particle_flags.get(), nullptr, i, 2, 2, p);
  add(ParticleDelta, h.activity_->particle_delta.get(), nullptr, f, 2, 3, p);
  add(GroupFlags, h.activity_->group_flags.get(), nullptr, i, 2, 2, g);
  add(GroupDelta, h.activity_->group_delta.get(), nullptr, f, 2, 6, g);
  add(Counters, h.activity_->counters.get(), nullptr, PrimitiveType::i64, 1, 1,
      TrialCounterCount);
  add(Focus, h.activity_->focus.get(), nullptr, f, 1, 1, 11);
  numerical_ =
      h.build_numerical_kernels(selection_.particles(), selection_.centroids());
  activity_ = h.activity_->build_kernels(selection_.particles(),
                                         selection_.centroids());
  if (h.live_hierarchy_) {
    auto &tree = *h.live_hierarchy_;
    const int n = std::max<std::uint64_t>(
        h.prepared_->capacity.limits()[Budget::CascadeNodes], 1);
    for (const auto &item :
         std::array<std::array<int, 4>, 5>{{{{Packets, 0, 2, 8}},
                                            {{Selectors, 1, 1, 1}},
                                            {{Previous, 13, 2, 4}},
                                            {{HierarchyCounters, 14, 1, 1}},
                                            {{Visits, 28, 2, 2}}}}) {
      const auto id = Surface(item[0]);
      auto *input = tree.input_array(item[1]);
      const auto type = id == Selectors ? DataType(i)
                        : id == Visits || id == HierarchyCounters
                            ? DataType(PrimitiveType::i64)
                            : DataType(f);
      add(id, input, nullptr, type, item[2], item[3],
          id == HierarchyCounters ? 10 : n);
    }
    hierarchy_ =
        tree.build_kernels(selection_.particles(), selection_.centroids());
  }
  current_ = std::make_unique<Geometry>(*this, true);
  candidate_ = std::make_unique<Geometry>(*this, true);
  installed_ = std::make_unique<Geometry>(*this, false);
  for (int id : {Pairs, PairFlags, SourceMap, Offsets, Orders, Stats}) {
    auto &a = candidate_->arrays[id];
    const auto &shape = a->shape;
    const int rank = int(shape.size());
    copies_.push_back({installed_->arrays[id].get(), a.get(), nullptr, a->dtype,
                       rank, rank == 1 ? 1 : shape[0], shape.back(), false});
  }
  for (int side = 0; side < 4; ++side)
    contact_csr_[side] = allocate(r, i, side < 2 ? 2 : 1, side < 2 ? 2 : 1,
                                  side < 2 ? p : std::max(h.contacts_, 1));
  status_ = allocate(r, PrimitiveType::i64, 1, 1, 2);
  compile_copies();
  compile_coverage();
  // Warm every enabled binding on the empty owner. Publication runs with its
  // writes disabled; neither numerical values nor committed bits change.
  copy(false);
  selection_.stage();
  current_->discover(h.prepared_->capacity.limits()[Budget::GeometryWork]);
  split_contacts();
  run_physics({});
  if (h.live_hierarchy_)
    for (int phase = 0; phase < 3; ++phase)
      run_hierarchy(phase, {});
  candidate_->discover(h.prepared_->capacity.limits()[Budget::GeometryWork]);
  coverage(*current_, *installed_, before_, false);
  coverage(*candidate_, *current_, after_, false);
  coverage(*candidate_, *installed_, compare_, false);
  check_numerics();
  copy(true, false);
  selection_.abandon();
  r.synchronize();
  native_bytes_ = native_requested(r) - native_before_;
}
TickExecution::~TickExecution() = default;

engine::CompiledKernel &TickExecution::kernel(TickKernel kind) {
  const auto n = std::size_t(kind);
  if (n < 7)
    return numerical_[n];
  return activity_[n - 7];
}
void TickExecution::redirect(TickKernel kind, LaunchContextBuilder &ctx) {
  const auto bind = [&](int arg, Surface id) {
    ctx.set_arg_ndarray({arg}, *values_[id]);
  };
  const auto contact = [&](int arg) {
    ctx.set_arg_ndarray({arg}, *current_->arrays[Pairs]);
  };
  const auto csr = [&](int arg, int index) {
    ctx.set_arg_ndarray({arg}, *contact_csr_[index]);
  };
  switch (kind) {
    case TickKernel::Clear:
      bind(0, Bodies);
      bind(1, Groups);
      bind(4, Universal);
      bind(6, ParticleFlags);
      break;
    case TickKernel::Force:
      bind(2, Bodies);
      bind(3, Groups);
      bind(8, ParticleFlags);
      bind(9, Counters);
      break;
    case TickKernel::Contact:
      contact(0);
      bind(1, Bodies);
      bind(4, ParticleFlags);
      bind(5, Counters);
      csr(6, 0);
      csr(7, 2);
      csr(8, 1);
      csr(9, 3);
      break;
    case TickKernel::Integrate:
      bind(0, Bodies);
      bind(3, Universal);
      bind(5, ParticleFlags);
      bind(6, ParticleDelta);
      bind(7, Focus);
      bind(8, Counters);
      bind(12, Speed);
      break;
    case TickKernel::Determine:
      bind(0, Bodies);
      bind(1, Speed);
      bind(3, Counters);
      break;
    case TickKernel::Centroid:
      bind(2, Bodies);
      bind(3, Groups);
      bind(7, GroupDelta);
      bind(8, Counters);
      break;
    case TickKernel::Finalize:
      bind(0, Groups);
      bind(2, GroupFlags);
      bind(3, GroupDelta);
      bind(4, Counters);
      break;
    case TickKernel::Begin:
      bind(0, Bodies);
      bind(1, Groups);
      bind(2, ParticleFlags);
      bind(3, ParticleDelta);
      bind(4, GroupFlags);
      bind(5, Counters);
      bind(6, Focus);
      break;
    case TickKernel::Touch:
      bind(0, Bodies);
      bind(1, Groups);
      bind(4, GroupFlags);
      bind(5, GroupDelta);
      bind(6, Counters);
      break;
    case TickKernel::ContactTouch:
      bind(0, ParticleFlags);
      bind(1, Counters);
      csr(2, 0);
      csr(3, 2);
      csr(4, 1);
      csr(5, 3);
      contact(6);
      break;
    case TickKernel::Mark:
      bind(0, Bodies);
      bind(1, Groups);
      bind(4, ParticleFlags);
      bind(5, ParticleDelta);
      bind(6, GroupFlags);
      bind(7, GroupDelta);
      bind(8, Counters);
      break;
  }
}

void TickExecution::compile_copies() {
  const auto suffix = std::to_string(selection_.particles().cells->id);
  for (int publication = 0; publication < 2; ++publication) {
    IRBuilder b;
    std::vector<Param> params;
    const int enabled_id = 2 * int(copies_.size());
    auto *enabled =
        b.create_arg_load({enabled_id}, PrimitiveType::i32, false, 0);
    for (std::size_t n = 0; n < copies_.size(); ++n) {
      const auto &s = copies_[n];
      auto *from = array(b, 2 * n, s.type, s.rank);
      auto *to = array(b, 2 * n + 1, s.type, s.rank);
      params.push_back(Param::array(s.type, s.rank));
      params.push_back(Param::array(s.type, s.rank));
      if (!publication && !s.staged)
        continue;
      for (int row = 0; row < s.rows; ++row) {
        auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(s.stride),
                                        false, h_.runtime_->cpu_threads());
        auto guard = b.get_loop_guard(loop);
        auto active = b.get_if_guard(b.create_if(enabled), true);
        auto *index = b.get_loop_index(loop);
        const auto cell =
            s.rank == 1 ? std::vector<Stmt *>{index} : at(b, row, index);
        write(b, to, cell, read(b, from, cell));
      }
    }
    params.push_back(Param::scalar(PrimitiveType::i32));
    auto result = Kernel(
        *h_.runtime_,
        std::string(publication ? "tick_publish_" : "tick_stage_") + suffix,
        b.extract_ir(), params, engine::MathMode::Strict);
    (publication ? publish_ : stage_) = std::move(result);
  }
}
void TickExecution::copy(bool publication, bool enabled) {
  auto &k = publication ? publish_ : stage_;
  auto call = k.invocation(true);
  auto &ctx = call.get();
  if (!ctx.prepared)
    for (std::size_t n = 0; n < copies_.size(); ++n) {
      auto &s = copies_[n];
      const int live_arg = 2 * n + int(publication),
                scratch_arg = 2 * n + !publication;
      if (s.host)
        h_.bind(ctx, live_arg, s.source, *s.host,
                s.rank == 1 ? std::vector<int>{s.stride}
                            : std::vector<int>{s.rows, s.stride});
      else
        ctx.set_arg_ndarray({live_arg}, *s.source);
      ctx.set_arg_ndarray({scratch_arg}, *s.destination);
    }
  engine::argument_int(ctx, 2 * copies_.size(), enabled);
  k.launch(ctx);
}

void TickExecution::compile_coverage() {
  auto &r = *h_.runtime_;
  const auto suffix = std::to_string(selection_.particles().cells->id);
  const int c = h_.contacts_, p = std::max(h_.particles_, 1);
  const auto i = PrimitiveType::i32, wide = PrimitiveType::i64;
  for (int mode = 0; mode < 3; ++mode) {
    IRBuilder b;
    auto *next = array(b, 0, i, 2), *flags = array(b, 1, i),
         *mapping = array(b, 2, i), *counts = array(b, 3, wide),
         *old = array(b, 4, i, 2), *old_flags = array(b, 5, i),
         *old_mapping = array(b, 6, i), *old_counts = array(b, 7, wide),
         *status = array(b, 8, wide);
    auto *wake = b.create_arg_load({9}, i, false, 0);
    write(b, status, {b.get_int32(0)}, b.get_int64(0));
    const auto get = [&](ArgLoadStmt *a, int field) {
      return read(b, a, {b.get_int32(field)});
    };
    const auto activate = [&](ArgLoadStmt *pairs, Stmt *row) {
      for (int side = 0; side < 2; ++side) {
        auto *body = read(b, pairs, at(b, side, row));
        auto valid = b.get_if_guard(
            b.create_if(b.create_logical_and(
                b.create_cmp_ge(body, b.get_int32(0)),
                b.create_cmp_lt(body, b.get_int32(h_.particles_)))),
            true);
        auto present = b.get_if_guard(
            b.create_if(
                NativeSet{h_.prepared_->particles.bindings().presence}.active(
                    b, body)),
            true);
        selection_.particles().activate(b, body);
      }
    };
    {
      auto change =
          b.get_if_guard(b.create_if(b.create_cmp_ne(get(counts, Sources),
                                                     get(old_counts, Sources))),
                         true);
      increment(b, status, 0);
    }
    {
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(c), false,
                                      r.cpu_threads());
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      auto *has_new =
          b.create_cmp_lt(b.create_cast(row, wide), get(counts, PairCount));
      auto *has_old =
          b.create_cmp_lt(b.create_cast(row, wide), get(old_counts, PairCount));
      Stmt *different =
          b.create_cmp_ne(read(b, flags, {row}), read(b, old_flags, {row}));
      for (int side = 0; side < 2; ++side)
        different = b.create_logical_or(
            different, b.create_cmp_ne(read(b, next, at(b, side, row)),
                                       read(b, old, at(b, side, row))));
      different = b.create_logical_or(
          b.create_cmp_ne(has_new, has_old),
          b.create_logical_and(b.create_logical_and(has_new, has_old),
                               different));
      auto changed = b.get_if_guard(b.create_if(different), true);
      increment(b, status, 0);
      auto do_wake = b.get_if_guard(b.create_if(wake), true);
      {
        auto active = b.get_if_guard(b.create_if(has_new), true);
        activate(next, row);
      }
      {
        auto active = b.get_if_guard(b.create_if(has_old), true);
        activate(old, row);
      }
    }
    {
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(c), false,
                                      r.cpu_threads());
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      auto *has_new =
          b.create_cmp_lt(b.create_cast(row, wide), get(counts, Sources));
      auto *has_old =
          b.create_cmp_lt(b.create_cast(row, wide), get(old_counts, Sources));
      auto *different = b.create_logical_or(
          b.create_cmp_ne(has_new, has_old),
          b.create_logical_and(b.create_logical_and(has_new, has_old),
                               b.create_cmp_ne(read(b, mapping, {row}),
                                               read(b, old_mapping, {row}))));
      auto changed = b.get_if_guard(b.create_if(different), true);
      increment(b, status, 0);
      auto do_wake = b.get_if_guard(b.create_if(wake), true);
      {
        auto active = b.get_if_guard(b.create_if(has_new), true);
        activate(next, read(b, mapping, {row}));
      }
      {
        auto active = b.get_if_guard(b.create_if(has_old), true);
        activate(old, read(b, old_mapping, {row}));
      }
    }
    auto k = Kernel(r, "tick_coverage_" + suffix + "_" + std::to_string(mode),
                    b.extract_ir(),
                    {Param::array(i, 2), Param::array(i), Param::array(i),
                     Param::array(wide), Param::array(i, 2), Param::array(i),
                     Param::array(i), Param::array(wide), Param::array(wide),
                     Param::scalar(i)},
                    engine::MathMode::Strict);
    (mode == 0 ? before_ : mode == 1 ? after_ : compare_) = std::move(k);
  }
  {
    IRBuilder b;
    auto *offsets = array(b, 0, i, 2), *orders = array(b, 1, i, 2);
    std::array<ArgLoadStmt *, 4> out;
    for (int n = 0; n < 4; ++n)
      out[n] = array(b, 2 + n, i, n < 2 ? 2 : 1);
    for (int side = 0; side < 2; ++side) {
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(p), false,
                                      r.cpu_threads());
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      for (int column = 0; column < 2; ++column)
        write(b, out[side], at(b, column, row),
              read(b, offsets, at(b, 2 * side + column, row)));
    }
    {
      auto *loop = b.create_range_for(
          b.get_int32(0), b.get_int32(std::max(c, 1)), false, r.cpu_threads());
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      for (int side = 0; side < 2; ++side)
        write(b, out[2 + side], {row}, read(b, orders, at(b, side, row)));
    }
    split_ = Kernel(r, "tick_contact_csr_" + suffix, b.extract_ir(),
                    {Param::array(i, 2), Param::array(i, 2), Param::array(i, 2),
                     Param::array(i, 2), Param::array(i), Param::array(i)},
                    engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    std::vector<Param> params;
    int count = 0;
    for (const auto &slot : copies_)
      if (slot.staged && slot.type == PrimitiveType::f64)
        ++count;
    auto *status = array(b, count, wide);
    write(b, status, {b.get_int32(1)}, b.get_int64(0));
    int arg = 0;
    for (const auto &slot : copies_) {
      if (!slot.staged || slot.type != PrimitiveType::f64)
        continue;
      auto *a = array(b, arg++, slot.type, slot.rank);
      params.push_back(Param::array(slot.type, slot.rank));
      for (int field = 0; field < slot.rows; ++field) {
        auto *loop = b.create_range_for(
            b.get_int32(0), b.get_int32(slot.stride), false, r.cpu_threads());
        auto guard = b.get_loop_guard(loop);
        auto *row = b.get_loop_index(loop);
        const auto cell =
            slot.rank == 1 ? std::vector<Stmt *>{row} : at(b, field, row);
        auto invalid = b.get_if_guard(b.create_if(b.create_logical_not(
                                          supported_real(b, read(b, a, cell)))),
                                      true);
        increment(b, status, 1);
      }
    }
    params.push_back(Param::array(wide));
    validate_ = Kernel(r, "tick_numeric_check_" + suffix, b.extract_ir(),
                       params, engine::MathMode::Strict);
  }
}
void TickExecution::coverage(Geometry &next,
                             Geometry &old,
                             Kernel &k,
                             bool wake) {
  auto call = k.invocation(true);
  auto &ctx = call.get();
  if (!ctx.prepared) {
    for (int side = 0; side < 2; ++side) {
      auto &g = side ? old : next;
      int argument = 4 * side;
      for (int id : {Pairs, PairFlags, SourceMap, Stats})
        ctx.set_arg_ndarray({argument++}, *g.arrays[id]);
    }
    ctx.set_arg_ndarray({8}, *status_);
  }
  engine::argument_int(ctx, 9, wake);
  k.launch(ctx);
}
void TickExecution::split_contacts() {
  auto call = split_.invocation(true);
  auto &ctx = call.get();
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({0}, *current_->arrays[Offsets]);
    ctx.set_arg_ndarray({1}, *current_->arrays[Orders]);
    for (int n = 0; n < 4; ++n)
      ctx.set_arg_ndarray({n + 2}, *contact_csr_[n]);
  }
  split_.launch(ctx);
}
void TickExecution::check_numerics() {
  auto call = validate_.invocation(true);
  auto &ctx = call.get();
  if (!ctx.prepared) {
    int arg = 0;
    for (const auto &slot : copies_)
      if (slot.staged && slot.type == PrimitiveType::f64)
        ctx.set_arg_ndarray({arg++}, *slot.destination);
    ctx.set_arg_ndarray({arg}, *status_);
  }
  validate_.launch(ctx);
}
void TickExecution::run_hierarchy(int phase, const Parameters64 &p) {
  auto &tree = *h_.live_hierarchy_;
  auto &k = hierarchy_[phase];
  auto call = k.invocation(true);
  auto &ctx = call.get();
  if (!ctx.prepared) {
    for (int input = 0; input < 24; ++input)
      tree.bind_input(ctx, input, input);
    tree.bind_input(ctx, 28, 28);
    tree.bind_input(ctx, 29, 29);
    for (const auto &entry :
         std::array<std::array<int, 2>, 10>{{{{0, Packets}},
                                             {{1, Selectors}},
                                             {{7, Bodies}},
                                             {{8, Groups}},
                                             {{13, Previous}},
                                             {{14, HierarchyCounters}},
                                             {{15, Counters}},
                                             {{16, GroupDelta}},
                                             {{28, Visits}},
                                             {{29, GroupFlags}}}})
      ctx.set_arg_ndarray({entry[0]}, *values_[entry[1]]);
    ctx.set_arg_ndarray({19}, *current_->arrays[Pairs]);
    ctx.set_arg_ndarray({20}, *contact_csr_[0]);
    ctx.set_arg_ndarray({21}, *contact_csr_[2]);
    ctx.set_arg_ndarray({22}, *contact_csr_[1]);
    ctx.set_arg_ndarray({23}, *contact_csr_[3]);
  }
  engine::argument_int(ctx, 24, tree.node_count());
  engine::argument_float(ctx, 25, p.gravity);
  engine::argument_int(ctx, 26, phase == 2 ? 0 : -1);
  engine::argument_float(ctx, 27, p.contact_stiffness);
  k.launch(ctx);
}
void TickExecution::run_physics(const Parameters64 &p) {
  const auto &old = h_.previous_parameters_;
  const bool changed = p.gravity != old.gravity || p.dt != old.dt ||
                       p.contact_stiffness != old.contact_stiffness ||
                       p.reach_strength != old.reach_strength;
  h_.activity_->begin(h_, h_.mode_ == TickMode::AllLoaded,
                      h_.invalidated_ || changed, p.reach_strength != 0, this);
  h_.launch_clear(false, this);
  if (h_.live_hierarchy_installed()) {
    run_hierarchy(2, p);
    run_hierarchy(0, p);
  } else
    h_.launch_force(p, this);
  h_.launch_contact(p, this);
  h_.launch_integrate(p, this);
  h_.activity_->touch_contacts(h_, p.contact_stiffness, this);
  if (h_.live_hierarchy_installed())
    run_hierarchy(1, p);
  else {
    h_.activity_->mark(h_, h_.mode_ == TickMode::AllLoaded, this);
    h_.launch_centroid(p, false, this);
    h_.activity_->touch(h_, p.gravity, this);
  }
}

std::uint64_t TickExecution::tick_work_bound(TickResult &q) const {
  const auto &l = h_.prepared_->capacity.limits();
  const auto p = l[Budget::Particles], g = l[Budget::Fields],
             e = l[Budget::Participation], c = l[Budget::Contacts],
             n = l[Budget::CascadeNodes];
  q.copy_values = 2 * copy_values_ +
                  2 * (7 * std::max<std::uint64_t>(p, 1) +
                       13 * std::max<std::uint64_t>(c, 1) + 20) +
                  4 * std::max<std::uint64_t>(p, 1) +
                  2 * std::max<std::uint64_t>(c, 1);
  for (const auto &slot : copies_)
    if (!slot.staged)
      q.copy_values += std::uint64_t(slot.rows) * slot.stride;
  // Units are typed scalar-copy slots or bounded source/reference-row
  // evaluations, not FLOPs. C3 recipient checks can revisit each connected
  // response and contact for every covered occurrence; charge that product.
  const auto add = [](std::uint64_t a, std::uint64_t b) {
    return b > UINT64_MAX - a ? UINT64_MAX : a + b;
  };
  const auto mul = [](std::uint64_t a, std::uint64_t b) {
    return a && b > UINT64_MAX / a ? UINT64_MAX : a * b;
  };
  const auto references = add(add(l[Budget::LiveResponses], mul(2, c)), 1);
  q.numerical_work_bound = add(256, mul(32, add(add(p, g), add(e, c))));
  q.numerical_work_bound = add(q.numerical_work_bound, mul(64, n));
  q.numerical_work_bound = add(
      q.numerical_work_bound,
      mul(32, add(add(l[Budget::CascadeLinks], l[Budget::CascadeReferences]),
                  l[Budget::LiveResponses])));
  q.numerical_work_bound =
      add(q.numerical_work_bound,
          mul(64, mul(l[Budget::CascadeCoverage], references)));
  q.selection_work_bound = add(mul(8, add(p, g)), mul(8, c));
  return add(add(q.copy_values, q.numerical_work_bound),
             q.selection_work_bound);
}
TickResult TickExecution::tick(std::uint64_t expected,
                               const Parameters64 &p,
                               std::uint64_t tick_limit,
                               std::uint64_t geometry_limit) {
  auto &s = *h_.prepared_;
  const auto &l = s.capacity.limits();
  TickResult q;
  q.context = s.particles.context();
  q.generation = s.generation;
  q.completed_ticks = s.completed_ticks;
  if (!h_.ready_)
    return q;
  if (expected != s.generation) {
    q.status = TickStatus::Stale;
    return q;
  }
  try {
    h_.validate_tick_parameters(p);
  } catch (const std::invalid_argument &) {
    q.status = TickStatus::InvalidInput;
    return q;
  }
  if (s.completed_ticks == UINT64_MAX) {
    q.status = TickStatus::Unsupported;
    return q;
  }
  if (tick_limit == UINT64_MAX)
    tick_limit = l[Budget::TickWork];
  if (geometry_limit == UINT64_MAX)
    geometry_limit = l[Budget::GeometryWork];
  const auto exhaust = [&](Budget b, std::uint64_t required,
                           std::uint64_t capacity) {
    q.status = TickStatus::CapacityExhausted;
    q.failure = {b, required, capacity};
    return q;
  };
  if (tick_limit > l[Budget::TickWork])
    return exhaust(Budget::TickWork, tick_limit, l[Budget::TickWork]);
  if (geometry_limit > l[Budget::GeometryWork])
    return exhaust(Budget::GeometryWork, geometry_limit,
                   l[Budget::GeometryWork]);
  q.required_work = tick_work_bound(q);
  if (q.required_work > tick_limit)
    return exhaust(Budget::TickWork, q.required_work, tick_limit);
  const auto collect_geometry = [&](Geometry &g, GeometryResult &result) {
    result = g.discover(geometry_limit - q.geometry_work);
    q.geometry_work += result.required_work;
    q.synchronizations += result.synchronizations;
    q.readback_bytes += result.readback_bytes;
    if (result.status == GeometryStatus::Ok)
      return true;
    q.status = result.status == GeometryStatus::CapacityExhausted
                   ? TickStatus::CapacityExhausted
                   : TickStatus::InvalidNumerics;
    q.failure = result.failure;
    if (q.failure.budget == Budget::GeometryWork)
      q.failure = {Budget::GeometryWork, q.geometry_work, geometry_limit};
    return false;
  };
  try {
    h_.validate_prepared_bindings();
    if (h_.live_hierarchy_)
      h_.live_hierarchy_->validate();
    const auto start = std::chrono::steady_clock::now();
    copy(false);
    selection_.stage();
    if (!collect_geometry(*current_, q.current))
      return q;
    coverage(*current_, *installed_, before_, true);
    split_contacts();
    if (s.failure_point == 200)
      throw std::runtime_error("injected candidate failure");
    run_physics(p);
    if (!collect_geometry(*candidate_, q.candidate))
      return q;
    const auto references = s.live.references() +
                            2 * (std::uint64_t(installed_->counts[PairCount]) +
                                 q.current.contacts + q.candidate.contacts);
    if (references > l[Budget::References])
      return exhaust(Budget::References, references, l[Budget::References]);
    coverage(*candidate_, *current_, after_, true);
    coverage(*candidate_, *installed_, compare_, false);
    check_numerics();
    h_.runtime_->synchronize();
    ++q.synchronizations;
    engine::readback(*status_, status_values_.data(), sizeof(status_values_));
    q.readback_bytes += sizeof(status_values_);
    q.coverage_changed = status_values_[0] != 0;
    Demand report =
        h_.activity_->report(h_.exposed_, h_.exposed_fields_,
                             values_[Counters].get(), values_[Focus].get());
    q.readback_bytes +=
        TrialCounterCount * sizeof(std::int64_t) + 8 * sizeof(double);
    std::array<std::int64_t, 10> tree_counts{};
    if (h_.live_hierarchy_) {
      engine::readback(*values_[HierarchyCounters], tree_counts.data(),
                       sizeof(tree_counts));
      q.readback_bytes += sizeof(tree_counts);
      if (h_.live_hierarchy_installed())
        report.force_skipped =
            h_.live_hierarchy_->response_count() - report.force_requests;
    }
    if (status_values_[1] || report.numerical_errors || tree_counts[0]) {
      q.status = TickStatus::InvalidNumerics;
      return q;
    }
    if (q.coverage_changed && s.generation == UINT64_MAX) {
      q.status = TickStatus::Unsupported;
      return q;
    }
    // No healthy rejection is possible beyond this point. Readers remain
    // excluded until numerical, contact and native selection copies finish.
    copy(true);
    if (s.failure_point == 201)
      throw std::runtime_error("injected publication failure");
    selection_.publish();
    h_.runtime_->synchronize();
    ++q.synchronizations;
    installed_->counts = candidate_->counts;
    if (q.coverage_changed) {
      ++s.generation;
      if (h_.live_hierarchy_)
        h_.live_hierarchy_->retag_generation(s.generation);
    }
    ++s.completed_ticks;
    h_.previous_parameters_ = p;
    h_.invalidated_ = false;
    report.frame_anchor = h_.frame_anchor_;
    report.tick_ms = std::chrono::duration<double, std::milli>(
                         std::chrono::steady_clock::now() - start)
                         .count();
    h_.timings_.kernel_ms += report.tick_ms;
    h_.demand_ = report;
    q.generation = s.generation;
    q.completed_ticks = s.completed_ticks;
    q.status = TickStatus::Ok;
    return q;
  } catch (...) {
    h_.ready_ = false;
    throw;
  }
}

std::size_t TickExecution::binding_bytes() const {
  std::size_t bytes = selection_.binding_bytes();
  for (const auto *group : {&numerical_})
    for (const auto &k : *group)
      bytes += k.prepared_bytes();
  for (const auto &k : activity_)
    bytes += k.prepared_bytes();
  if (h_.live_hierarchy_)
    for (const auto &k : hierarchy_)
      bytes += k.prepared_bytes();
  for (const auto *g : {current_.get(), candidate_.get()}) {
    for (const auto &k : g->kernels)
      bytes += k.prepared_bytes();
    bytes += g->reset.prepared_bytes();
  }
  for (const auto *k :
       {&stage_, &publish_, &before_, &after_, &compare_, &validate_, &split_})
    bytes += k->prepared_bytes();
  return bytes;
}
}  // namespace field::detail

namespace field {
TickResult Harness::tick_prepared(std::uint64_t generation,
                                  const Parameters64 &p,
                                  std::uint64_t tick_work,
                                  std::uint64_t geometry_work) {
  if (!tick_execution_) {
    TickResult q;
    q.status = TickStatus::Unsupported;
    return q;
  }
  return tick_execution_->tick(generation, p, tick_work, geometry_work);
}
}  // namespace field
