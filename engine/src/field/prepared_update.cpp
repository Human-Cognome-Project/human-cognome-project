#include "field/field.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include "engine/transfer.h"
#include "field/activity.h"
#include "field/centroid_math.h"
#include "field/prepared_storage.h"

namespace field {
namespace {
using namespace taichi::lang;
using detail::RelationshipHeader;
using engine::RecordHandle;
bool same(RecordHandle a, RecordHandle b) {
  return a.context == b.context && a.incarnation == b.incarnation &&
         a.address == b.address && a.record == b.record;
}
bool same(const RelationshipHeader &a, const RelationshipHeader &b) {
  return same(a.first, b.first) && same(a.second, b.second) &&
         a.ordinal == b.ordinal && a.share == b.share;
}
ArgLoadStmt *array(IRBuilder &b, int id, DataType t, int rank = 1) {
  return b.create_ndarray_arg_load({id}, t, rank, 0);
}
Stmt *read(IRBuilder &b, ArgLoadStmt *a, std::vector<Stmt *> at) {
  return b.create_global_load(b.create_external_ptr(a, at));
}
void write(IRBuilder &b, ArgLoadStmt *a, std::vector<Stmt *> at, Stmt *v) {
  b.create_global_store(b.create_external_ptr(a, at), v);
}
std::vector<Stmt *> cell(IRBuilder &b, int f, Stmt *i) {
  return {b.get_int32(f), i};
}
Stmt *get(IRBuilder &b, ArgLoadStmt *a, int n) {
  return read(b, a, {b.get_int32(n)});
}
void put(IRBuilder &b, ArgLoadStmt *a, int n, Stmt *v) {
  write(b, a, {b.get_int32(n)}, v);
}
Stmt *plus(IRBuilder &b, Stmt *i, int n) {
  return b.create_add(i, b.get_int32(n));
}
// Read a current survivor or a new fixture row. The guarded branch prevents
// speculative out-of-bounds reads of either input on CPU and CUDA.
Stmt *body(IRBuilder &b,
           ArgLoadStmt *particles,
           ArgLoadStmt *starts,
           ArgLoadStmt *validation,
           int f,
           Stmt *row) {
  auto *v = b.create_local_var(PrimitiveType::f32);
  auto *state = read(b, validation, {row});
  auto *branch = b.create_if(b.create_cmp_gt(state, b.get_int32(1)));
  {
    auto guard = b.get_if_guard(branch, true);
    b.create_local_store(
        v, read(b, starts, cell(b, f, b.create_sub(state, b.get_int32(2)))));
  }
  {
    auto guard = b.get_if_guard(branch, false);
    b.create_local_store(v, read(b, particles, cell(b, f, row)));
  }
  return b.create_local_load(v);
}
void finite(IRBuilder &b, ArgLoadStmt *status, Stmt *v, int error = 1) {
  auto guard = b.get_if_guard(
      b.create_if(b.create_logical_not(b.create_cmp_le(
          b.create_abs(v), b.get_float32(std::numeric_limits<float>::max())))),
      true);
  b.create_atomic_or(b.create_external_ptr(status, {b.get_int32(0)}),
                     b.get_int32(error));
}
}  // namespace

void Harness::prepare_update_execution() {
  auto &s = *prepared_;
  const auto f = PrimitiveType::f32, i = PrimitiveType::i32;
  const auto fv = engine::Param::array(f), fa = engine::Param::array(f, 2),
             iv = engine::Param::array(i), ia = engine::Param::array(i, 2),
             integer = engine::Param::scalar(i);
  const int p = particles_, g = groups_, gs = std::max(g, 1), n = p + g,
            threads = runtime_->cpu_threads();
  // Validation: [P state | G state | P wake | E edits | C edits].
  // State 0 absent, -1 retired, P=1 survivor/>=2 command+2;
  // G=1 untouched/2 changed/>=3 command+3. Scratch row names are remapped to
  // actual NodeManager results only after numerical preflight succeeds.
  {
    IRBuilder b;
    auto *particles = array(b, 0, f, 2), *groups = array(b, 1, f, 2),
         *edges = array(b, 2, i, 2), *shares = array(b, 3, f),
         *out = array(b, 4, f), *v = array(b, 5, i),
         *starts = array(b, 6, f, 2), *offsets = array(b, 7, i, 2),
         *order = array(b, 8, i), *status = array(b, 9, i),
         *selected = array(b, 10, i), *focus = array(b, 11, f),
         *delta = array(b, 13, f, 2);
    auto *fcount = b.create_arg_load({12}, i, false, 0);
    put(b, status, 0, b.get_int32(0));
    {
      auto *loop =
          b.create_range_for(b.get_int32(0), b.get_int32(g), false, threads);
      auto guard = b.get_loop_guard(loop);
      Stmt *row = b.get_loop_index(loop),
           *state = read(b, v, {plus(b, row, p)});
      auto present = b.get_if_guard(
          b.create_if(b.create_cmp_gt(state, b.get_int32(0))), true);
      auto *repair = b.create_if(b.create_cmp_ge(state, b.get_int32(2)));
      {
        auto changed = b.get_if_guard(repair, true);
        auto *mass = b.create_local_var(f);
        b.create_local_store(mass, b.get_float32(0));
        std::array<AllocaStmt *, 3> anchor, weighted;
        for (int axis = 0; axis < 3; ++axis) {
          anchor[axis] = b.create_local_var(f);
          weighted[axis] = b.create_local_var(f);
          b.create_local_store(anchor[axis], b.get_float32(0));
          b.create_local_store(weighted[axis], b.get_float32(0));
          auto *existing = b.create_if(b.create_cmp_eq(state, b.get_int32(2)));
          {
            auto old = b.get_if_guard(existing, true);
            b.create_local_store(anchor[axis],
                                 read(b, groups, cell(b, kCenX + axis, row)));
          }
          {
            auto fresh = b.get_if_guard(existing, false);
            auto nonempty = b.get_if_guard(
                b.create_if(b.create_cmp_lt(read(b, offsets, cell(b, 0, row)),
                                            read(b, offsets, cell(b, 1, row)))),
                true);
            auto *first = read(b, order, {read(b, offsets, cell(b, 0, row))});
            auto *member = read(b, edges, cell(b, kEdgeParticle, first));
            b.create_local_store(anchor[axis], body(b, particles, starts, v,
                                                    kPosX + axis, member));
          }
        }
        {
          auto *members = b.create_range_for(read(b, offsets, cell(b, 0, row)),
                                             read(b, offsets, cell(b, 1, row)),
                                             false, 1, 0, true);
          auto member_guard = b.get_loop_guard(members);
          auto *edge = read(b, order, {b.get_loop_index(members)});
          auto *member = read(b, edges, cell(b, kEdgeParticle, edge));
          auto *m = b.create_mul(body(b, particles, starts, v, kMass, member),
                                 read(b, shares, {edge}));
          b.create_local_store(mass,
                               b.create_add(b.create_local_load(mass), m));
          for (int axis = 0; axis < 3; ++axis)
            b.create_local_store(
                weighted[axis],
                b.create_add(
                    b.create_local_load(weighted[axis]),
                    b.create_mul(
                        m, b.create_sub(body(b, particles, starts, v,
                                             kPosX + axis, member),
                                        b.create_local_load(anchor[axis])))));
        }
        auto *m = b.create_local_load(mass);
        finite(b, status, m);
        write(b, out, {plus(b, row, kCenM * gs)}, m);
        write(b, out, {plus(b, row, kAccM * gs)}, m);
        for (int axis = 0; axis < 3; ++axis) {
          auto *before = b.create_local_load(anchor[axis]);
          auto *offset = b.create_local_load(weighted[axis]);
          const auto result = detail::centroid_coordinate(b, before, offset, m);
          finite(b, status, result.value);
          finite(b, status, result.residue);
          write(b, out, {plus(b, row, (kCenX + axis) * gs)}, result.value);
          write(b, out, {plus(b, row, (kAccX + axis) * gs)}, offset);
          // A fresh reduction already includes old pending member motion.
          write(b, out, {plus(b, row, (8 + axis) * gs)}, result.residue);
          write(b, out, {plus(b, row, (11 + axis) * gs)}, before);
        }
      }
      {
        auto unchanged = b.get_if_guard(repair, false);
        for (int field = 0; field < 8; ++field)
          write(b, out, {plus(b, row, field * gs)},
                read(b, groups, cell(b, field, row)));
        for (int field = 0; field < 6; ++field)
          write(b, out, {plus(b, row, (8 + field) * gs)},
                read(b, delta, cell(b, field, row)));
      }
    }
    // Focal computation has the same source order and relative arithmetic as
    // Activity::begin. Its handles and coordinate frame cannot change here.
    for (int j = 0; j < 11; ++j)
      put(b, out, 14 * gs + 1 + j, get(b, focus, j));
    for (int j = 3; j < 8; ++j)
      put(b, out, 14 * gs + 1 + j, b.get_float32(0));
    {
      auto *loop =
          b.create_range_for(b.get_int32(0), fcount, false, 1, 0, true);
      auto guard = b.get_loop_guard(loop);
      auto *row = read(b, selected, {b.get_loop_index(loop)});
      auto *mass = read(b, out, {plus(b, row, kCenM * gs)});
      {
        auto bad = b.get_if_guard(
            b.create_if(b.create_cmp_le(mass, b.get_float32(0))), true);
        b.create_atomic_or(b.create_external_ptr(status, {b.get_int32(0)}),
                           b.get_int32(2));
      }
      for (int axis = 0; axis < 3; ++axis)
        put(b, out, 14 * gs + 4 + axis,
            b.create_add(
                get(b, out, 14 * gs + 4 + axis),
                b.create_mul(
                    mass, b.create_sub(
                              read(b, out, {plus(b, row, (kCenX + axis) * gs)}),
                              get(b, focus, axis)))));
      put(b, out, 14 * gs + 7, b.create_add(get(b, out, 14 * gs + 7), mass));
    }
    auto *mass = get(b, out, 14 * gs + 7);
    auto *denom =
        b.create_add(mass, b.create_sub(b.get_float32(1), b.create_sgn(mass)));
    finite(b, status, mass);
    for (int axis = 0; axis < 3; ++axis) {
      auto *next = b.create_select(
          b.create_cmp_gt(mass, b.get_float32(0)),
          b.create_add(get(b, focus, axis),
                       b.create_div(get(b, out, 14 * gs + 4 + axis), denom)),
          get(b, focus, 8 + axis));
      finite(b, status, next);
      put(b, out, 14 * gs + 1 + axis, next);
      auto change = b.get_if_guard(
          b.create_if(b.create_cmp_ne(next, get(b, focus, axis))), true);
      put(b, out, 14 * gs + 8, b.get_float32(1));
    }
    put(b, out, 14 * gs, b.get_float32(0));
    {
      auto *loop =
          b.create_range_for(b.get_int32(0), b.get_int32(p), false, 1, 0, true);
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      auto live = b.get_if_guard(
          b.create_if(b.create_cmp_gt(read(b, v, {row}), b.get_int32(0))),
          true);
      put(b, out, 14 * gs,
          b.create_add(get(b, out, 14 * gs),
                       body(b, particles, starts, v, kMass, row)));
    }
    finite(b, status, get(b, out, 14 * gs));
    update_candidate_ = engine::CompiledKernel(
        *runtime_,
        "prepared_candidate_" +
            std::to_string(s.particles.bindings().presence->id),
        b.extract_ir(),
        {fa, fa, ia, fv, fv, iv, fa, ia, iv, iv, iv, fv, integer, fa},
        engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    auto *particles = array(b, 0, f, 2), *groups = array(b, 1, f, 2),
         *determined = array(b, 2, f), *universal = array(b, 3, f),
         *ps = array(b, 4, i, 2), *pd = array(b, 5, f, 2),
         *flags = array(b, 6, i, 2), *gd = array(b, 7, f, 2),
         *focus = array(b, 8, f), *derived = array(b, 9, f),
         *v = array(b, 10, i), *starts = array(b, 11, f, 2),
         *dest = array(b, 12, i), *status = array(b, 14, i);
    auto *radial = b.create_arg_load({13}, i, false, 0);
    {
      auto *loop =
          b.create_range_for(b.get_int32(0), b.get_int32(p), false, threads);
      auto guard = b.get_loop_guard(loop);
      Stmt *row = b.get_loop_index(loop), *state = read(b, v, {row}),
           *target = read(b, dest, {row});
      {
        auto retired = b.get_if_guard(
            b.create_if(b.create_cmp_eq(state, b.get_int32(-1))), true);
        for (int field = 0; field < kParticleFieldCount; ++field)
          write(b, particles, cell(b, field, row), b.get_float32(0));
        write(b, determined, {row}, b.get_float32(0));
        for (int j = 0; j < 2; ++j)
          write(b, ps, cell(b, j, row), b.get_int32(0));
        for (int j = 0; j < 3; ++j)
          write(b, pd, cell(b, j, row), b.get_float32(0));
        activity_->particle_activity.deactivate(b, row);
      }
      {
        auto fresh = b.get_if_guard(
            b.create_if(b.create_cmp_gt(state, b.get_int32(1))), true);
        auto *command = b.create_sub(state, b.get_int32(2));
        for (int field = 0; field < kParticleFieldCount; ++field)
          write(b, particles, cell(b, field, target),
                read(b, starts, cell(b, field, command)));
        // Test adapter supplies a validated initial determiner, not survivor
        // state.
        write(b, determined, {target}, read(b, starts, cell(b, 11, command)));
        for (int j = 0; j < 2; ++j)
          write(b, ps, cell(b, j, target), b.get_int32(0));
        for (int j = 0; j < 3; ++j)
          write(b, pd, cell(b, j, target), b.get_float32(0));
        activity_->particle_activity.activate(b, target);
      }
    }
    {
      auto *loop =
          b.create_range_for(b.get_int32(0), b.get_int32(g), false, threads);
      auto guard = b.get_loop_guard(loop);
      Stmt *row = b.get_loop_index(loop),
           *state = read(b, v, {plus(b, row, p)}),
           *target = read(b, dest, {plus(b, row, p)});
      {
        auto retired = b.get_if_guard(
            b.create_if(b.create_cmp_eq(state, b.get_int32(-1))), true);
        for (int j = 0; j < 8; ++j)
          write(b, groups, cell(b, j, row), b.get_float32(0));
        for (int j = 0; j < 6; ++j)
          write(b, gd, cell(b, j, row), b.get_float32(0));
        for (int j = 0; j < 2; ++j)
          write(b, flags, cell(b, j, row), b.get_int32(0));
        activity_->centroids.deactivate(b, row);
      }
      {
        auto changed = b.get_if_guard(
            b.create_if(b.create_cmp_ge(state, b.get_int32(2))), true);
        for (int j = 0; j < 8; ++j)
          write(b, groups, cell(b, j, target),
                read(b, derived, {plus(b, row, j * gs)}));
        for (int j = 0; j < 6; ++j)
          write(b, gd, cell(b, j, target),
                read(b, derived, {plus(b, row, (8 + j) * gs)}));
        for (int j = 0; j < 2; ++j)
          write(b, flags, cell(b, j, target), b.get_int32(0));
        activity_->centroids.deactivate(b, target);
      }
    }
    // Rebuild U in actual native record order, after retired rows are zeroed.
    // Keep old U in scratch to decide global radial invalidation.
    put(b, derived, 14 * gs, get(b, universal, 0));
    put(b, universal, 0, b.get_float32(0));
    {
      auto *loop =
          b.create_range_for(b.get_int32(0), b.get_int32(p), false, 1, 0, true);
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      auto present = b.get_if_guard(
          b.create_if(detail::NativeSet{s.particles.bindings().presence}.active(
              b, row)),
          true);
      put(b, universal, 0,
          b.create_add(get(b, universal, 0),
                       read(b, particles, cell(b, kMass, row))));
    }
    finite(b, status, get(b, universal, 0));
    auto *global = b.create_logical_and(
        radial,
        b.create_logical_or(
            b.create_cmp_ne(get(b, universal, 0), get(b, derived, 14 * gs)),
            b.create_cmp_ne(get(b, derived, 14 * gs + 8), b.get_float32(0))));
    {
      auto *loop =
          b.create_range_for(b.get_int32(0), b.get_int32(p), false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      auto live = b.get_if_guard(
          b.create_if(b.create_cmp_gt(read(b, v, {row}), b.get_int32(0))),
          true);
      auto wake = b.get_if_guard(b.create_if(b.create_logical_or(
                                     global, read(b, v, {plus(b, row, n)}))),
                                 true);
      activity_->particle_activity.activate(b, read(b, dest, {row}));
    }
    for (int j = 0; j < 11; ++j)
      put(b, focus, j, get(b, derived, 14 * gs + 1 + j));
    update_commit_ = engine::CompiledKernel(
        *runtime_,
        "prepared_commit_" +
            std::to_string(s.particles.bindings().presence->id),
        b.extract_ir(),
        {fa, fa, fv, fv, ia, fa, ia, fa, fv, fv, iv, fa, iv, integer, iv},
        engine::MathMode::Strict);
  }
  // Empty, but actual bindings and all offloaded families are prepared here.
  launch_update_candidate();
  launch_update_commit();
  runtime_->synchronize();
}

void Harness::launch_update_candidate() {
  auto &s = *prepared_;
  auto call = update_candidate_.invocation(true);
  auto &ctx = call.get();
  if (!ctx.prepared) {
    bind(ctx, 0, particle_array_.get(), particle_data,
         {kParticleFieldCount, std::max(particles_, 1)});
    bind(ctx, 1, group_array_.get(), group_data,
         {kGroupFieldCount, std::max(groups_, 1)});
    ctx.set_arg_ndarray({2}, *s.candidate_edges_array);
    ctx.set_arg_ndarray({3}, *s.candidate_shares_array);
    ctx.set_arg_ndarray({4}, *s.derived_array);
    ctx.set_arg_ndarray({5}, *s.validation_array);
    ctx.set_arg_ndarray({6}, *s.new_rows_array);
    ctx.set_arg_ndarray({7}, *s.candidate_offsets_array);
    ctx.set_arg_ndarray({8}, *s.candidate_order_array);
    ctx.set_arg_ndarray({9}, *s.candidate_status_array);
    ctx.set_arg_ndarray({10}, activity_->selected_slots());
    ctx.set_arg_ndarray({11}, *activity_->focus);
    ctx.set_arg_ndarray({13}, *activity_->group_delta);
  }
  engine::argument_int(ctx, 12, int(s.live.focus));
  update_candidate_.launch(ctx);
}
void Harness::launch_update_commit() {
  auto &s = *prepared_;
  auto call = update_commit_.invocation(true);
  auto &ctx = call.get();
  if (!ctx.prepared) {
    bind(ctx, 0, particle_array_.get(), particle_data,
         {kParticleFieldCount, std::max(particles_, 1)});
    bind(ctx, 1, group_array_.get(), group_data,
         {kGroupFieldCount, std::max(groups_, 1)});
    auto &d = particles_ ? determined_ : empty_determined_;
    bind(ctx, 2, determined_array_.get(), d, {std::max(particles_, 1)});
    bind(ctx, 3, universal_array_.get(), universal_data, {1});
    ctx.set_arg_ndarray({4}, *activity_->particle_flags);
    ctx.set_arg_ndarray({5}, *activity_->particle_delta);
    ctx.set_arg_ndarray({6}, *activity_->group_flags);
    ctx.set_arg_ndarray({7}, *activity_->group_delta);
    ctx.set_arg_ndarray({8}, *activity_->focus);
    ctx.set_arg_ndarray({9}, *s.derived_array);
    ctx.set_arg_ndarray({10}, *s.validation_array);
    ctx.set_arg_ndarray({11}, *s.new_rows_array);
    ctx.set_arg_ndarray({12}, *s.destinations_array);
    ctx.set_arg_ndarray({14}, *s.candidate_status_array);
  }
  engine::argument_int(ctx, 13, previous_parameters_.reach_strength != 0);
  update_commit_.launch(ctx);
}

UpdateResult Harness::update_prepared(std::uint64_t expected,
                                      const UpdateCommand *input,
                                      std::size_t count) {
  UpdateResult answer;
  if (!prepared_ || !ready_) {
    answer.status = UpdateStatus::Unready;
    return answer;
  }
  auto &s = *prepared_;
  answer.generation = s.generation;
  // New starts have a single call lifetime, including rejected batches.
  struct StartsGuard {
    std::vector<float> &rows;
    ~StartsGuard() {
      std::fill(rows.begin(), rows.end(), 0);
    }
  } starts_guard{s.new_rows};
  const auto reject = [&](UpdateStatus status) {
    answer.status = status;
    return answer;
  };
  const auto exhausted = [&](Budget budget, std::size_t required) {
    answer.failure = {budget, required, s.capacity.limits()[budget]};
    return reject(UpdateStatus::CapacityExhausted);
  };
  const auto &limits = s.capacity.limits();
  if (expected != s.generation)
    return reject(UpdateStatus::Stale);
  if (count > limits[Budget::Batch])
    return exhausted(Budget::Batch, count);
  if (!count)
    return reject(UpdateStatus::NoChange);
  if (!input)
    return reject(UpdateStatus::InvalidInput);
  try {
    validate_prepared_bindings();
  } catch (...) {
    ready_ = false;
    throw;
  }
  std::copy_n(input, count, s.commands.begin());
  const int p = particles_, g = groups_, n = p + g, es = std::max(edges_, 1),
            gs = std::max(groups_, 1),
            bs = int(std::max(limits[Budget::Batch], std::uint64_t{1}));
  std::fill(s.validation.begin(), s.validation.end(), 0);
  std::fill(s.resolved.begin(), s.resolved.end(), RecordHandle{});
  std::copy(s.particle_headers.begin(), s.particle_headers.end(),
            s.prospective_particles.begin());
  std::copy(s.field_headers.begin(), s.field_headers.end(),
            s.prospective_fields.begin());
  for (int row = 0; row < p; ++row) {
    s.validation[row] = s.particle_headers[row].record >= 0;
    s.destinations[row] = row;
  }
  for (int row = 0; row < g; ++row) {
    s.validation[p + row] = s.field_headers[row].record >= 0;
    s.destinations[p + row] = row;
  }
  detail::PreparedCounts candidate = s.live;
  std::size_t pclaims = 0, gclaims = 0;
  for (std::size_t c = 0; c < count; ++c) {
    const auto &cmd = s.commands[c];
    switch (cmd.operation) {
      case UpdateOp::DefineParticle:
        ++pclaims;
        ++candidate.particles;
        break;
      case UpdateOp::DefineField:
        ++gclaims;
        ++candidate.fields;
        break;
      case UpdateOp::RetireParticle:
      case UpdateOp::RetireField: {
        const bool particle = cmd.operation == UpdateOp::RetireParticle;
        const auto &headers = particle ? s.particle_headers : s.field_headers;
        const int offset = particle ? 0 : p, end = particle ? p : g;
        const auto h = cmd.first.handle;
        if (cmd.first.command >= 0 || h.record < 0 || h.record >= end ||
            !same(headers[h.record], h) || s.validation[offset + h.record] != 1)
          return reject(UpdateStatus::Stale);
        if (!particle)
          for (std::size_t f = 0; f < s.live.focus; ++f)
            if (same(s.focal[f], h))
              return reject(UpdateStatus::InvalidInput);
        s.validation[offset + h.record] = -1;
        (particle ? s.prospective_particles
                  : s.prospective_fields)[h.record] = {};
        if (particle)
          --candidate.particles;
        else
          --candidate.fields;
        break;
      }
      case UpdateOp::InsertParticipation:
        ++candidate.edges;
        break;
      case UpdateOp::InsertContact:
        ++candidate.contacts;
        break;
      case UpdateOp::ReplaceParticipation:
      case UpdateOp::RemoveParticipation:
      case UpdateOp::RemoveContact: {
        const bool edge = cmd.operation != UpdateOp::RemoveContact;
        const auto live = edge ? s.live.edges : s.live.contacts;
        const int offset = 2 * p + g + (edge ? 0 : edges_);
        if (cmd.generation != s.generation || cmd.row >= live)
          return reject(UpdateStatus::Stale);
        if (s.validation[offset + cmd.row])
          return reject(UpdateStatus::InvalidInput);
        s.validation[offset + cmd.row] = int(c + 1);
        if (cmd.operation == UpdateOp::RemoveParticipation)
          --candidate.edges;
        if (!edge)
          --candidate.contacts;
        break;
      }
      case UpdateOp::ReplaceFocus:
        return reject(UpdateStatus::Unsupported);
      default:
        return reject(UpdateStatus::InvalidInput);
    }
  }
  if (s.live.particles + pclaims > limits[Budget::Particles])
    return exhausted(Budget::Particles, s.live.particles + pclaims);
  if (s.live.fields + gclaims > limits[Budget::Fields])
    return exhausted(Budget::Fields, s.live.fields + gclaims);
  if (const auto failure = s.check(candidate, count); failure.exhausted()) {
    answer.failure = failure;
    return reject(UpdateStatus::CapacityExhausted);
  }
  const auto peak = s.live.particles + pclaims + s.live.fields + gclaims +
                    2 * std::max(s.live.edges, candidate.edges) +
                    2 * std::max(s.live.contacts, candidate.contacts) +
                    s.live.focus;
  if (peak > limits[Budget::References])
    return exhausted(Budget::References, peak);
  // Allocate only scratch names here. Native ownership stays in NodeManager.
  for (std::size_t c = 0; c < count; ++c) {
    const auto &cmd = s.commands[c];
    const bool particle = cmd.operation == UpdateOp::DefineParticle;
    if (!particle && cmd.operation != UpdateOp::DefineField)
      continue;
    const auto &old = particle ? s.particle_headers : s.field_headers;
    auto &headers = particle ? s.prospective_particles : s.prospective_fields;
    const int offset = particle ? 0 : p, end = particle ? p : g;
    const auto potential =
        particle ? limits[Budget::Potential] : limits[Budget::Fields];
    if (cmd.address < 0 || std::uint64_t(cmd.address) >= potential ||
        cmd.kind < 1 || cmd.kind > 255)
      return reject(UpdateStatus::InvalidInput);
    for (int row = 0; row < end; ++row) {
      ++answer.structural_visits;
      if ((old[row].record >= 0 && old[row].address == cmd.address) ||
          (headers[row].record >= 0 && headers[row].address == cmd.address))
        return reject(UpdateStatus::InvalidInput);
    }
    if (particle) {
      for (int field = 0; field < 12; ++field)
        if (!std::isfinite(s.new_rows[field * std::size_t(bs) + c]))
          return reject(UpdateStatus::InvalidNumerics);
      if (!(s.new_rows[kMass * std::size_t(bs) + c] > 0) ||
          s.new_rows[kRadius * std::size_t(bs) + c] < 0)
        return reject(UpdateStatus::InvalidNumerics);
    }
    int row = 0;
    while (row < end && s.validation[offset + row] != 0)
      ++row;
    if (row == end)
      throw std::logic_error("preflight scratch name invariant");
    s.resolved[c] = {0, c + 1, cmd.address, row};
    headers[row] = s.resolved[c];
    s.validation[offset + row] = int(c) + (particle ? 2 : 3);
    if (particle)
      s.validation[n + row] = 1;
  }
  const auto resolve = [&](const UpdateRef &ref, bool particle,
                           RecordHandle &out) {
    const auto &headers =
        particle ? s.prospective_particles : s.prospective_fields;
    if (ref.command >= 0) {
      if (std::size_t(ref.command) >= count ||
          s.commands[ref.command].operation !=
              (particle ? UpdateOp::DefineParticle : UpdateOp::DefineField))
        return false;
      out = s.resolved[ref.command];
      return true;
    }
    out = ref.handle;
    return out.context && out.record >= 0 && out.record < (particle ? p : g) &&
           same(out, headers[out.record]);
  };
  const auto ledger = [&](bool edge) {
    const auto &old = edge ? s.edges : s.contacts;
    auto &next = edge ? s.prospective_edges : s.prospective_contacts;
    const auto live = edge ? s.live.edges : s.live.contacts;
    const auto final = edge ? candidate.edges : candidate.contacts;
    const auto insert =
        edge ? UpdateOp::InsertParticipation : UpdateOp::InsertContact;
    const int offset = 2 * p + g + (edge ? 0 : edges_);
    for (std::size_t c = 0; c < count; ++c)
      if (s.commands[c].operation == insert && s.commands[c].position >= final)
        return false;
    std::size_t source = 0;
    for (std::size_t row = 0; row < final; ++row) {
      const UpdateCommand *command = nullptr;
      for (std::size_t c = 0; c < count; ++c) {
        ++answer.structural_visits;
        if (s.commands[c].operation == insert &&
            s.commands[c].position == row) {
          if (command)
            return false;
          command = &s.commands[c];
        }
      }
      if (!command) {
        while (source < live) {
          const int edit = s.validation[offset + source];
          if (!edit ||
              s.commands[edit - 1].operation == UpdateOp::ReplaceParticipation)
            break;
          ++source;
        }
        if (source == live)
          return false;
        const int edit = s.validation[offset + source];
        if (edit)
          command = &s.commands[edit - 1];
        else
          next[row] = old[source];
        ++source;
      }
      if (command) {
        RelationshipHeader value;
        if (!resolve(command->first, true, value.first) ||
            !resolve(command->second, !edge, value.second))
          return false;
        value.ordinal = command->ordinal;
        value.share = edge ? command->share : 0;
        if ((edge && (!std::isfinite(value.share) || !(value.share > 0) ||
                      value.share > 1)) ||
            (!edge && same(value.first, value.second)))
          return false;
        next[row] = value;
      }
      // Every retained endpoint must still exist, including unedited rows.
      const auto a = next[row].first, b = next[row].second;
      const auto &second_headers =
          edge ? s.prospective_fields : s.prospective_particles;
      if (a.record < 0 || a.record >= p ||
          !same(a, s.prospective_particles[a.record]) || b.record < 0 ||
          b.record >= (edge ? g : p) || !same(b, second_headers[b.record]))
        return false;
    }
    return true;
  };
  if (!ledger(true) || !ledger(false))
    return reject(UpdateStatus::InvalidInput);
  bool changed =
      pclaims || gclaims || candidate.particles != s.live.particles ||
      candidate.fields != s.live.fields || candidate.edges != s.live.edges ||
      candidate.contacts != s.live.contacts;
  for (std::size_t row = 0; row < std::min(candidate.edges, s.live.edges);
       ++row)
    changed |= !same(s.edges[row], s.prospective_edges[row]);
  for (std::size_t row = 0; row < std::min(candidate.contacts, s.live.contacts);
       ++row)
    changed |= !same(s.contacts[row], s.prospective_contacts[row]);
  if (!changed)
    return reject(UpdateStatus::NoChange);
  if (s.generation == std::numeric_limits<std::uint64_t>::max()) {
    ready_ = false;
    return reject(UpdateStatus::Unready);
  }
  // Compare each field's logical contribution sequence, not compact ledger
  // indexes. Metadata-only edits and unrelated compaction do not reseed
  // history.
  for (int row = 0; row < g; ++row) {
    if (s.validation[p + row] != 1)
      continue;
    std::size_t a = 0, b = 0;
    for (;;) {
      while (a < s.live.edges && s.edges[a].second.record != row) {
        ++a;
        ++answer.structural_visits;
      }
      while (b < candidate.edges &&
             s.prospective_edges[b].second.record != row) {
        ++b;
        ++answer.structural_visits;
      }
      if (a == s.live.edges && b == candidate.edges)
        break;
      if (a == s.live.edges || b == candidate.edges ||
          !same(s.edges[a].first, s.prospective_edges[b].first) ||
          s.edges[a].share != s.prospective_edges[b].share) {
        s.validation[p + row] = 2;
        break;
      }
      ++a;
      ++b;
      answer.structural_visits += 2;
    }
  }
  for (const auto &pair :
       {std::make_pair(s.edges.data(), s.live.edges),
        std::make_pair(s.prospective_edges.data(), candidate.edges)})
    for (std::size_t row = 0; row < pair.second; ++row) {
      const auto &edge = pair.first[row];
      const int state = s.validation[p + edge.second.record];
      if (state != 1 && s.validation[edge.first.record] > 0)
        s.validation[n + edge.first.record] = 1;
    }
  for (std::size_t c = 0; c < count; ++c) {
    const auto &cmd = s.commands[c];
    if (cmd.operation == UpdateOp::InsertContact ||
        cmd.operation == UpdateOp::RemoveContact) {
      RecordHandle a, b;
      if (cmd.operation == UpdateOp::RemoveContact) {
        a = s.contacts[cmd.row].first;
        b = s.contacts[cmd.row].second;
      } else {
        resolve(cmd.first, true, a);
        resolve(cmd.second, true, b);
      }
      if (s.validation[a.record] > 0)
        s.validation[n + a.record] = 1;
      if (s.validation[b.record] > 0)
        s.validation[n + b.record] = 1;
    }
  }
  // Candidate group CSR: source order preserved, no numerical-slot sorting.
  auto *offsets = s.prospective_csr.data(), *order = offsets + 2 * gs;
  std::fill_n(offsets, 2 * gs, 0);
  std::fill_n(order, es, 0);
  std::fill(s.counts.begin(), s.counts.end(), 0);
  std::fill(s.candidate_edges.begin(), s.candidate_edges.end(), 0);
  std::fill(s.candidate_shares.begin(), s.candidate_shares.end(), 0);
  for (std::size_t row = 0; row < candidate.edges; ++row) {
    const auto &edge = s.prospective_edges[row];
    s.candidate_edges[row] = edge.first.record;
    s.candidate_edges[es + row] = edge.second.record;
    s.candidate_shares[row] = edge.share;
    ++s.counts[edge.second.record];
  }
  int cursor = 0;
  for (int row = 0; row < g; ++row) {
    offsets[row] = cursor;
    s.cursors[row] = cursor;
    cursor += s.counts[row];
    offsets[gs + row] = cursor;
  }
  for (std::size_t row = 0; row < candidate.edges; ++row)
    order[s.cursors[s.prospective_edges[row].second.record]++] = int(row);
  answer.index_visits += 2 * candidate.edges + g;
  try {
    const auto upload = [&](Ndarray &array, const void *data,
                            std::size_t bytes) {
      engine::upload(array, data, bytes);
      answer.upload_bytes += bytes;
    };
    upload(*s.validation_array, s.validation.data(),
           s.validation.size() * sizeof(int));
    upload(*s.candidate_edges_array, s.candidate_edges.data(),
           s.candidate_edges.size() * sizeof(int));
    upload(*s.candidate_shares_array, s.candidate_shares.data(),
           s.candidate_shares.size() * sizeof(float));
    upload(*s.new_rows_array, s.new_rows.data(),
           s.new_rows.size() * sizeof(float));
    upload(*s.candidate_offsets_array, offsets, 2 * gs * sizeof(int));
    upload(*s.candidate_order_array, order, es * sizeof(int));
    launch_update_candidate();
    runtime_->synchronize();
    ++answer.synchronizations;
    engine::readback(*s.candidate_status_array, s.candidate_status.data(),
                     sizeof(s.candidate_status));
    answer.readback_bytes += sizeof(s.candidate_status);
    if (s.candidate_status[0])
      return reject(UpdateStatus::InvalidNumerics);
    if (s.failure_point == 1)
      return reject(UpdateStatus::InvalidInput);
    // Claims begin mutation, even though no numerical survivor has been
    // written.
    for (bool particle : {true, false}) {
      std::size_t size = 0;
      for (std::size_t c = 0; c < count; ++c)
        if (s.commands[c].operation ==
            (particle ? UpdateOp::DefineParticle : UpdateOp::DefineField)) {
          s.claim_addresses[size] = s.commands[c].address;
          s.claim_kinds[size] = s.commands[c].kind;
          s.command_indexes[size++] = int(c);
        }
      auto &map = particle ? s.particles : s.fields;
      if (map.define_batch(s.claim_addresses.data(), s.claim_kinds.data(), size,
                           s.results.data()) != engine::RecordStatus::Ok)
        throw std::runtime_error("validated cross-family native claim failed");
      if (size) {
        ++answer.synchronizations;
        answer.upload_bytes += (2 + 5 * size) * sizeof(std::int64_t);
        answer.readback_bytes += (1 + 5 * size) * sizeof(std::int64_t);
      }
      for (std::size_t j = 0; j < size; ++j) {
        const int c = s.command_indexes[j];
        const auto provisional = s.resolved[c];
        s.destinations[(particle ? 0 : p) + provisional.record] =
            s.results[j].handle.record;
        s.resolved[c] = s.results[j].handle;
      }
    }
    if (s.failure_point == 2)
      throw std::runtime_error("injected failure after native claims");
    upload(*s.destinations_array, s.destinations.data(),
           s.destinations.size() * sizeof(int));
    launch_update_commit();
    runtime_->synchronize();
    ++answer.synchronizations;
    engine::readback(*s.candidate_status_array, s.candidate_status.data(),
                     sizeof(s.candidate_status));
    answer.readback_bytes += sizeof(s.candidate_status);
    if (s.candidate_status[0])
      throw std::runtime_error("post-claim numerical invariant failed");
    if (s.failure_point == 3)
      throw std::runtime_error("injected failure after numerical writes");
    const auto actual = [&](RecordHandle h) {
      return h.context ? h : s.resolved[h.incarnation - 1];
    };
    for (std::size_t row = 0; row < candidate.edges; ++row) {
      auto &edge = s.prospective_edges[row];
      edge.first = actual(edge.first);
      edge.second = actual(edge.second);
    }
    for (std::size_t row = 0; row < candidate.contacts; ++row) {
      auto &contact = s.prospective_contacts[row];
      contact.first = actual(contact.first);
      contact.second = actual(contact.second);
    }
    // Commit metadata under the same exclusive owner boundary. An exception
    // below cannot leave a healthy partial generation.
    for (std::size_t c = 0; c < count; ++c) {
      const auto &cmd = s.commands[c];
      if (cmd.operation == UpdateOp::RetireParticle)
        s.particle_headers[cmd.first.handle.record] = {};
      if (cmd.operation == UpdateOp::RetireField)
        s.field_headers[cmd.first.handle.record] = {};
      if (cmd.operation == UpdateOp::DefineParticle)
        s.particle_headers[s.resolved[c].record] = s.resolved[c];
      if (cmd.operation == UpdateOp::DefineField)
        s.field_headers[s.resolved[c].record] = s.resolved[c];
    }
    s.edges.swap(s.prospective_edges);
    s.contacts.swap(s.prospective_contacts);
    s.live = candidate;
    std::fill(edge_int_data.begin(), edge_int_data.end(), 0);
    std::fill(edge_float_data.begin(), edge_float_data.end(), 0);
    std::fill(contact_int_data.begin(), contact_int_data.end(), 0);
    for (std::size_t row = 0; row < candidate.edges; ++row) {
      edge_int(kEdgeParticle, int(row)) = s.edges[row].first.record;
      edge_int(kEdgeGroup, int(row)) = s.edges[row].second.record;
      edge_float(kShare, int(row)) = s.edges[row].share;
    }
    for (std::size_t row = 0; row < candidate.contacts; ++row) {
      contact_int(kContactA, int(row)) = s.contacts[row].first.record;
      contact_int(kContactB, int(row)) = s.contacts[row].second.record;
    }
    if (path_ == HostPath::BulkCopy) {
      upload(*edge_int_array_, edge_int_data.data(),
             edge_int_data.size() * sizeof(int));
      upload(*edge_float_array_, edge_float_data.data(),
             edge_float_data.size() * sizeof(float));
      upload(*contact_int_array_, contact_int_data.data(),
             contact_int_data.size() * sizeof(int));
    }
    activity_->index(edge_int_data, contact_int_data);
    answer.index_visits +=
        4 * candidate.edges + 4 * candidate.contacts + 3 * p + g;
    answer.upload_bytes += s.prospective_csr.size() * sizeof(int);
    if (s.failure_point == 4)
      throw std::runtime_error("injected failure after topology writes");
    for (bool particle : {true, false}) {
      std::size_t size = 0;
      for (std::size_t c = 0; c < count; ++c)
        if (s.commands[c].operation ==
            (particle ? UpdateOp::RetireParticle : UpdateOp::RetireField))
          s.retired[size++] = s.commands[c].first.handle;
      auto &map = particle ? s.particles : s.fields;
      if (map.release_batch(s.retired.data(), size, s.results.data()) !=
          engine::RecordStatus::Ok)
        throw std::runtime_error(
            "validated cross-family native retirement failed");
      if (size) {
        ++answer.collections;
        ++answer.synchronizations;
        answer.upload_bytes += (2 + 5 * size) * sizeof(std::int64_t);
        answer.readback_bytes += (1 + 5 * size) * sizeof(std::int64_t);
      }
    }
    runtime_->synchronize();
    ++answer.synchronizations;
    exposed_ = candidate.particles;
    exposed_fields_ = 0;
    std::fill(s.counts.begin(), s.counts.end(), 0);
    for (std::size_t row = 0; row < candidate.edges; ++row)
      s.counts[s.edges[row].second.record] = 1;
    for (int row = 0; row < g; ++row)
      exposed_fields_ += s.counts[row];
    s.populated = true;
    answer.generation = ++s.generation;
    return reject(UpdateStatus::Ok);
  } catch (...) {
    // Backend failure is never advertised as ordinary invalid input. Mutation
    // includes provisional native ownership, not only numerical writes.
    ready_ = false;
    throw;
  }
}
}  // namespace field
