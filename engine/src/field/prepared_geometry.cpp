#include "field/field.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "engine/transfer.h"
#include "field/activity.h"
#include "field/contact_math.h"
#include "field/prepared_storage.h"

namespace field {
namespace {
using namespace taichi::lang;
enum Counter {
  Present,
  Awake,
  Sources,
  Pairs,
  Derived,
  Invalid,
  Focus,
  ExplicitPairs,
  PresenceEntries,
  CompactionSlots
};
ArgLoadStmt *array(IRBuilder &b, int id, DataType t, int rank = 1) {
  return b.create_ndarray_arg_load({id}, t, rank, 0);
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
           Stmt *v) {
  b.create_global_store(b.create_external_ptr(a, index), v);
}
Stmt *get(IRBuilder &b, ArgLoadStmt *a, int i) {
  return read(b, a, {b.get_int32(i)});
}
void put(IRBuilder &b, ArgLoadStmt *a, int i, Stmt *v) {
  write(b, a, {b.get_int32(i)}, v);
}
void count(IRBuilder &b, ArgLoadStmt *a, int i) {
  b.create_atomic_add(b.create_external_ptr(a, {b.get_int32(i)}),
                      b.get_int64(1));
}
void invalid(IRBuilder &b, ArgLoadStmt *stats, Stmt *condition) {
  auto guard = b.get_if_guard(b.create_if(condition), true);
  b.create_atomic_or(b.create_external_ptr(stats, {b.get_int32(Invalid)}),
                     b.get_int64(1));
}
void finite(IRBuilder &b, ArgLoadStmt *stats, Stmt *v, bool wide) {
  invalid(b, stats,
          b.create_logical_not(
              wide ? detail::supported_real(b, v)
                   : b.create_cmp_le(
                         b.create_abs(v),
                         detail::real_constant(
                             b, wide,
                             wide ? std::numeric_limits<double>::max()
                                  : std::numeric_limits<float>::max()))));
}
Stmt *key(IRBuilder &b, ArgLoadStmt *pairs, Stmt *row) {
  auto *a = read(b, pairs, at(b, 0, row)), *c = read(b, pairs, at(b, 1, row));
  return b.create_add(
      b.create_mul(b.create_cast(b.create_min(a, c), PrimitiveType::i64),
                   b.get_int64(std::int64_t{1} << 32)),
      b.create_cast(b.create_max(a, c), PrimitiveType::i64));
}
}  // namespace

void Harness::prepare_geometry_execution() {
  auto &s = *prepared_;
  if (!s.capacity.limits()[Budget::GeometryWork])
    return;
  auto kernels = build_geometry_kernels(activity_->particle_activity);
  geometry_collect_ = std::move(kernels[0]);
  geometry_source_order_ = std::move(kernels[1]);
  geometry_discover_ = std::move(kernels[2]);
  geometry_finalize_ = std::move(kernels[3]);
}

std::array<engine::CompiledKernel, 4> Harness::build_geometry_kernels(
    detail::NativeSet particle_selection) {
  auto &s = *prepared_;
  std::array<engine::CompiledKernel, 4> kernels;
  const auto f = real_type(), i = PrimitiveType::i32, wide = PrimitiveType::i64;
  const auto fa = engine::Param::array(f, 2), fv = engine::Param::array(f),
             ia = engine::Param::array(i, 2), iv = engine::Param::array(i),
             cv = engine::Param::array(wide),
             integer = engine::Param::scalar(i);
  const int threads = runtime_->cpu_threads();
  const auto suffix =
      std::to_string(s.particles.bindings().presence->id) +
      (particle_selection.cells == activity_->particle_activity.cells
           ? ""
           : "_selection_" + std::to_string(particle_selection.cells->id));
  {
    IRBuilder b;
    auto *particles = array(b, 0, f, 2), *groups = array(b, 1, f, 2),
         *source = array(b, 2, i, 2), *rows = array(b, 3, i),
         *stats = array(b, 4, wide), *focal = array(b, 5, f),
         *old_focus = array(b, 6, f), *selected = array(b, 7, i);
    auto *fcount = b.create_arg_load({8}, i, false, 0),
         *scount = b.create_arg_load({9}, i, false, 0);
    for (int j = 0; j < 16; ++j)
      put(b, stats, j, b.get_int64(0));
    for (int j = 0; j < 4; ++j) put(b, focal, j, real(b, 0));
    put(b, stats, Sources, b.create_cast(scount, wide));
    put(b, stats, CompactionSlots, b.get_int64(particles_));
    {
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(particles_),
                                      false, threads);
      auto guard = b.get_loop_guard(loop);
      write(b, rows, {b.get_loop_index(loop)}, b.get_int32(0));
    }
    {
      auto *loop =
          b.create_struct_for(s.particles.bindings().presence, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      write(b, rows, {row}, b.get_int32(1));
      count(b, stats, PresenceEntries);
      auto awake =
          b.get_if_guard(b.create_if(particle_selection.active(b, row)), true);
      count(b, stats, Awake);
    }
    {
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(particles_),
                                      false, 1, 0, true);
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      auto present = b.get_if_guard(b.create_if(read(b, rows, {row})), true);
      auto *next = b.create_cast(get(b, stats, Present), i);
      // Writes are at or behind row, so unvisited presence flags stay intact.
      write(b, rows, {next}, row);
      count(b, stats, Present);
      for (int field : {kPosX, kPosY, kPosZ, kMass, kRadius})
        finite(b, stats, read(b, particles, at(b, field, row)),
               profile_.wide());
      invalid(b, stats,
              b.create_logical_or(
                  b.create_cmp_le(read(b, particles, at(b, kMass, row)),
                                  real(b, 0)),
                  b.create_cmp_lt(read(b, particles, at(b, kRadius, row)),
                                  real(b, 0))));
    }
    {
      auto *loop = b.create_range_for(b.get_int32(0), scount, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      auto *a = read(b, source, at(b, 0, row)),
           *c = read(b, source, at(b, 1, row));
      auto *within = b.create_logical_and(
          b.create_logical_and(b.create_cmp_ge(a, b.get_int32(0)),
                               b.create_cmp_lt(a, b.get_int32(particles_))),
          b.create_logical_and(b.create_cmp_ge(c, b.get_int32(0)),
                               b.create_cmp_lt(c, b.get_int32(particles_))));
      auto *bounds = b.create_if(within);
      {
        auto valid = b.get_if_guard(bounds, true);
        invalid(
            b, stats,
            b.create_logical_or(
                b.create_cmp_eq(a, c),
                b.create_logical_not(b.create_logical_and(
                    detail::NativeSet{s.particles.bindings().presence}.active(
                        b, a),
                    detail::NativeSet{s.particles.bindings().presence}.active(
                        b, c)))));
      }
      {
        auto bad = b.get_if_guard(bounds, false);
        invalid(b, stats, b.get_int32(1));
      }
    }
    // Same ordered relative f32 sum as Activity::begin; no live focus or wake
    // state is changed. Published group centres are current even after motion.
    put(b, stats, Focus,
        b.create_select(
            b.create_cmp_gt(fcount, b.get_int32(0)),
            b.get_int64(int(GeometryFocus::Valid)),
            b.create_select(
                b.create_cmp_eq(get(b, stats, Present), b.get_int64(0)),
                b.get_int64(int(GeometryFocus::Empty)),
                b.get_int64(int(GeometryFocus::MissingSelection)))));
    std::array<AllocaStmt *, 4> sum;
    for (auto &v : sum) {
      v = b.create_local_var(f);
      b.create_local_store(v, real(b, 0));
    }
    {
      auto *loop =
          b.create_range_for(b.get_int32(0), fcount, false, 1, 0, true);
      auto guard = b.get_loop_guard(loop);
      auto *row = read(b, selected, {b.get_loop_index(loop)});
      auto *bounds = b.create_if(
          b.create_logical_and(b.create_cmp_ge(row, b.get_int32(0)),
                               b.create_cmp_lt(row, b.get_int32(groups_))));
      {
        auto valid = b.get_if_guard(bounds, true);
        auto *live = b.create_if(
            detail::NativeSet{s.fields.bindings().presence}.active(b, row));
        {
          auto present = b.get_if_guard(live, true);
          auto *mass = read(b, groups, at(b, kCenM, row));
          finite(b, stats, mass, profile_.wide());
          auto *positive = b.create_if(b.create_cmp_gt(mass, real(b, 0)));
          {
            auto good = b.get_if_guard(positive, true);
            b.create_local_store(
                sum[3], b.create_add(b.create_local_load(sum[3]), mass));
            for (int axis = 0; axis < 3; ++axis) {
              auto *centre = read(b, groups, at(b, kCenX + axis, row));
              finite(b, stats, centre, profile_.wide());
              b.create_local_store(
                  sum[axis],
                  b.create_add(
                      b.create_local_load(sum[axis]),
                      b.create_mul(mass, b.create_sub(centre, get(b, old_focus,
                                                                  axis)))));
            }
          }
          {
            auto bad = b.get_if_guard(positive, false);
            put(b, stats, Focus, b.get_int64(int(GeometryFocus::Invalid)));
          }
        }
        {
          auto bad = b.get_if_guard(live, false);
          put(b, stats, Focus, b.get_int64(int(GeometryFocus::Invalid)));
        }
      }
      {
        auto bad = b.get_if_guard(bounds, false);
        put(b, stats, Focus, b.get_int64(int(GeometryFocus::Invalid)));
      }
    }
    auto *mass = b.create_local_load(sum[3]);
    auto *denom =
        b.create_add(mass, b.create_sub(real(b, 1), b.create_sgn(mass)));
    finite(b, stats, mass, profile_.wide());
    put(b, focal, 3, mass);
    for (int axis = 0; axis < 3; ++axis) {
      auto *value = b.create_select(
          b.create_cmp_gt(mass, real(b, 0)),
          b.create_add(get(b, old_focus, axis),
                       b.create_div(b.create_local_load(sum[axis]), denom)),
          real(b, 0));
      finite(b, stats, value, profile_.wide());
      put(b, focal, axis, value);
    }
    kernels[0] = engine::CompiledKernel(
        *runtime_, "geometry_collect_" + suffix, b.extract_ir(),
        {fa, fa, ia, iv, cv, fv, fv, iv, integer, integer},
        engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    auto *source = array(b, 0, i, 2), *order = array(b, 1, i),
         *end = array(b, 2, i);
    auto *count = b.create_arg_load({3}, i, false, 0);
    auto *loop = b.create_range_for(b.get_int32(0), count, false, threads);
    auto guard = b.get_loop_guard(loop);
    auto *row = b.get_loop_index(loop);
    auto *wanted = key(b, source, row);
    auto *rank = b.create_local_var(i), *equal = b.create_local_var(i),
         *before = b.create_local_var(i);
    for (auto *v : {rank, equal, before})
      b.create_local_store(v, b.get_int32(0));
    {
      auto *others =
          b.create_range_for(b.get_int32(0), count, false, 1, 0, true);
      auto other_guard = b.get_loop_guard(others);
      auto *other = b.get_loop_index(others);
      auto *candidate = key(b, source, other);
      auto *same = b.create_cmp_eq(candidate, wanted);
      auto *prior = b.create_logical_and(same, b.create_cmp_lt(other, row));
      b.create_local_store(
          rank, b.create_add(b.create_local_load(rank),
                             b.create_cast(
                                 b.create_logical_or(
                                     b.create_cmp_lt(candidate, wanted), prior),
                                 i)));
      b.create_local_store(equal, b.create_add(b.create_local_load(equal),
                                               b.create_cast(same, i)));
      b.create_local_store(before, b.create_add(b.create_local_load(before),
                                                b.create_cast(prior, i)));
    }
    auto *r = b.create_local_load(rank);
    write(b, order, {r}, row);
    write(b, end, {r},
          b.create_add(b.create_sub(r, b.create_local_load(before)),
                       b.create_local_load(equal)));
    kernels[1] = engine::CompiledKernel(
        *runtime_, "geometry_source_order_" + suffix, b.extract_ir(),
        {ia, iv, iv, integer}, engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    auto *particles = array(b, 0, f, 2), *source = array(b, 1, i, 2),
         *rows = array(b, 2, i), *order = array(b, 3, i), *end = array(b, 4, i),
         *out = array(b, 5, i, 2), *mapping = array(b, 6, i),
         *stats = array(b, 7, wide);
    auto *scount = b.create_arg_load({8}, i, false, 0);
    auto *cursor = b.create_local_var(i);
    b.create_local_store(cursor, b.get_int32(0));
    auto *n = b.create_cast(get(b, stats, Present), i);
    auto *loop = b.create_range_for(b.get_int32(0), n, false, 1, 0, true);
    {
      auto guard = b.get_loop_guard(loop);
      auto *left = b.get_loop_index(loop);
      auto *others = b.create_range_for(b.create_add(left, b.get_int32(1)), n,
                                        false, 1, 0, true);
      auto other_guard = b.get_loop_guard(others);
      auto *a = read(b, rows, {left}),
           *c = read(b, rows, {b.get_loop_index(others)});
      std::array<Stmt *, 3> delta;
      for (int axis = 0; axis < 3; ++axis)
        delta[axis] = b.create_sub(read(b, particles, at(b, kPosX + axis, c)),
                                   read(b, particles, at(b, kPosX + axis, a)));
      const auto geometry = detail::contact_geometry(
          b, delta, read(b, particles, at(b, kRadius, a)),
          read(b, particles, at(b, kRadius, c)), profile_.wide());
      finite(b, stats, geometry.squared, profile_.wide());
      finite(b, stats, geometry.radii, profile_.wide());
      finite(b, stats, geometry.distance, profile_.wide());
      auto *derived = b.create_logical_or(
          b.create_cmp_gt(geometry.penetration, real(b, 0)),
          b.create_logical_and(b.create_cmp_eq(geometry.squared, real(b, 0)),
                               b.create_cmp_gt(geometry.radii, real(b, 0))));
      auto *explicit_pair = b.create_local_var(i),
           *first_source = b.create_local_var(i),
           *group_end = b.create_local_var(i);
      b.create_local_store(explicit_pair, b.get_int32(0));
      b.create_local_store(first_source, b.get_int32(-1));
      b.create_local_store(group_end, b.create_local_load(cursor));
      {
        auto remaining = b.get_if_guard(
            b.create_if(b.create_cmp_lt(b.create_local_load(cursor), scount)),
            true);
        auto *src = read(b, order, {b.create_local_load(cursor)});
        auto *pair_key =
            b.create_add(b.create_mul(b.create_cast(a, wide),
                                      b.get_int64(std::int64_t{1} << 32)),
                         b.create_cast(c, wide));
        auto matches = b.get_if_guard(
            b.create_if(b.create_cmp_eq(key(b, source, src), pair_key)), true);
        b.create_local_store(explicit_pair, b.get_int32(1));
        b.create_local_store(first_source, src);
        b.create_local_store(group_end,
                             read(b, end, {b.create_local_load(cursor)}));
      }
      auto *emit =
          b.create_logical_or(derived, b.create_local_load(explicit_pair));
      {
        auto included = b.get_if_guard(b.create_if(emit), true);
        auto *demand = get(b, stats, Pairs);
        {
          auto fits = b.get_if_guard(
              b.create_if(b.create_cmp_lt(demand, b.get_int64(contacts_))),
              true);
          auto *index = b.create_cast(demand, i);
          auto *has_source = b.create_if(b.create_local_load(explicit_pair));
          {
            auto sourced = b.get_if_guard(has_source, true);
            for (int side = 0; side < 2; ++side)
              write(b, out, at(b, side, index),
                    read(b, source,
                         at(b, side, b.create_local_load(first_source))));
          }
          {
            auto generated = b.get_if_guard(has_source, false);
            write(b, out, at(b, 0, index), a);
            write(b, out, at(b, 1, index), c);
          }
          write(b, out, at(b, 2, index),
                b.create_add(b.create_local_load(explicit_pair),
                             b.create_mul(b.create_cast(derived, i),
                                          b.get_int32(DerivedContact))));
          write(b, out, at(b, 3, index), b.create_local_load(first_source));
          auto *dup = b.create_range_for(b.create_local_load(cursor),
                                         b.create_local_load(group_end), false,
                                         1, 0, true);
          auto dup_guard = b.get_loop_guard(dup);
          write(b, mapping, {read(b, order, {b.get_loop_index(dup)})}, index);
        }
        count(b, stats, Pairs);
        {
          auto generated = b.get_if_guard(b.create_if(derived), true);
          count(b, stats, Derived);
        }
        {
          auto sourced = b.get_if_guard(
              b.create_if(b.create_local_load(explicit_pair)), true);
          count(b, stats, ExplicitPairs);
        }
      }
      b.create_local_store(cursor, b.create_local_load(group_end));
    }
    invalid(b, stats, b.create_cmp_ne(b.create_local_load(cursor), scount));
    kernels[2] = engine::CompiledKernel(
        *runtime_, "geometry_discover_" + suffix, b.extract_ir(),
        {fa, ia, iv, iv, iv, ia, iv, cv, integer}, engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    auto *temporary = array(b, 0, i, 2), *pairs = array(b, 1, i, 2),
         *flags = array(b, 2, i), *permutation = array(b, 3, i),
         *mapping = array(b, 4, i), *offsets = array(b, 5, i, 2),
         *cursors = array(b, 6, i, 2), *orders = array(b, 7, i, 2),
         *stats = array(b, 8, wide);
    auto *scount = b.create_arg_load({9}, i, false, 0);
    auto *n = b.create_cast(get(b, stats, Pairs), i);
    const auto order_key = [&](Stmt *row) {
      auto *first = read(b, temporary, at(b, 3, row));
      return b.create_select(
          b.create_cmp_ge(first, b.get_int32(0)), b.create_cast(first, wide),
          b.create_add(b.create_cast(scount, wide), b.create_cast(row, wide)));
    };
    {
      auto *loop = b.create_range_for(b.get_int32(0), n, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      auto *wanted = order_key(row);
      auto *rank = b.create_local_var(i);
      b.create_local_store(rank, b.get_int32(0));
      {
        auto *others = b.create_range_for(b.get_int32(0), n, false, 1, 0, true);
        auto other_guard = b.get_loop_guard(others);
        b.create_local_store(
            rank,
            b.create_add(
                b.create_local_load(rank),
                b.create_cast(b.create_cmp_lt(
                                  order_key(b.get_loop_index(others)), wanted),
                              i)));
      }
      auto *dest = b.create_local_load(rank);
      write(b, permutation, {row}, dest);
      for (int side = 0; side < 2; ++side)
        write(b, pairs, at(b, side, dest),
              read(b, temporary, at(b, side, row)));
      write(b, flags, {dest}, read(b, temporary, at(b, 2, row)));
    }
    {
      auto *loop = b.create_range_for(b.get_int32(0), scount, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      write(b, mapping, {row}, read(b, permutation, {read(b, mapping, {row})}));
    }
    {
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(particles_),
                                      false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      for (int field = 0; field < 4; ++field)
        write(b, offsets, at(b, field, row), b.get_int32(0));
    }
    {
      auto *loop = b.create_range_for(b.get_int32(0), n, false, 1, 0, true);
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      for (int side = 0; side < 2; ++side) {
        auto *p = read(b, pairs, at(b, side, row));
        write(b, offsets, at(b, 2 * side + 1, p),
              b.create_add(read(b, offsets, at(b, 2 * side + 1, p)),
                           b.get_int32(1)));
      }
    }
    for (int side = 0; side < 2; ++side) {
      auto *next = b.create_local_var(i);
      b.create_local_store(next, b.get_int32(0));
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(particles_),
                                      false, 1, 0, true);
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      auto *count = read(b, offsets, at(b, 2 * side + 1, row));
      write(b, offsets, at(b, 2 * side, row), b.create_local_load(next));
      write(b, cursors, at(b, side, row), b.create_local_load(next));
      b.create_local_store(next,
                           b.create_add(b.create_local_load(next), count));
      write(b, offsets, at(b, 2 * side + 1, row), b.create_local_load(next));
    }
    {
      auto *loop = b.create_range_for(b.get_int32(0), n, false, 1, 0, true);
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      for (int side = 0; side < 2; ++side) {
        auto *p = read(b, pairs, at(b, side, row));
        auto *cursor = read(b, cursors, at(b, side, p));
        write(b, orders, at(b, side, cursor), row);
        write(b, cursors, at(b, side, p), b.create_add(cursor, b.get_int32(1)));
      }
    }
    kernels[3] = engine::CompiledKernel(
        *runtime_, "geometry_finalize_" + suffix, b.extract_ir(),
        {ia, ia, iv, iv, iv, ia, ia, ia, cv, integer},
        engine::MathMode::Strict);
  }
  return kernels;
}

void Harness::launch_geometry_collect() {
  auto &s = *prepared_;
  auto call = geometry_collect_.invocation(true);
  auto &ctx = call.get();
  if (!ctx.prepared) {
    bind(ctx, 0, particle_array_.get(), particle_values_,
         {kParticleFieldCount, std::max(particles_, 1)});
    bind(ctx, 1, group_array_.get(), group_values_,
         {kGroupFieldCount, std::max(groups_, 1)});
    bind(ctx, 2, contact_int_array_.get(), contact_int_data,
         {kContactIntFieldCount, std::max(contacts_, 1)});
    ctx.set_arg_ndarray({3}, *s.geometry_rows);
    ctx.set_arg_ndarray({4}, *s.geometry_counters);
    ctx.set_arg_ndarray({5}, *s.geometry_focus);
    ctx.set_arg_ndarray({6}, *activity_->focus);
    ctx.set_arg_ndarray({7}, activity_->selected_slots());
  }
  engine::argument_int(ctx, 8, s.live.focus);
  engine::argument_int(ctx, 9, s.live.contacts);
  geometry_collect_.launch(ctx);
}
void Harness::launch_geometry_source_order() {
  auto &s = *prepared_;
  auto call = geometry_source_order_.invocation(true);
  auto &ctx = call.get();
  if (!ctx.prepared) {
    bind(ctx, 0, contact_int_array_.get(), contact_int_data,
         {kContactIntFieldCount, std::max(contacts_, 1)});
    ctx.set_arg_ndarray({1}, *s.geometry_source_order);
    ctx.set_arg_ndarray({2}, *s.geometry_group_end);
  }
  engine::argument_int(ctx, 3, s.live.contacts);
  geometry_source_order_.launch(ctx);
}
void Harness::launch_geometry_discover() {
  auto &s = *prepared_;
  auto call = geometry_discover_.invocation(true);
  auto &ctx = call.get();
  if (!ctx.prepared) {
    bind(ctx, 0, particle_array_.get(), particle_values_,
         {kParticleFieldCount, std::max(particles_, 1)});
    bind(ctx, 1, contact_int_array_.get(), contact_int_data,
         {kContactIntFieldCount, std::max(contacts_, 1)});
    ctx.set_arg_ndarray({2}, *s.geometry_rows);
    ctx.set_arg_ndarray({3}, *s.geometry_source_order);
    ctx.set_arg_ndarray({4}, *s.geometry_group_end);
    ctx.set_arg_ndarray({5}, *s.geometry_temporary);
    ctx.set_arg_ndarray({6}, *s.geometry_source_map);
    ctx.set_arg_ndarray({7}, *s.geometry_counters);
  }
  engine::argument_int(ctx, 8, s.live.contacts);
  geometry_discover_.launch(ctx);
}
void Harness::launch_geometry_finalize() {
  auto &s = *prepared_;
  auto call = geometry_finalize_.invocation(true);
  auto &ctx = call.get();
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({0}, *s.geometry_temporary);
    ctx.set_arg_ndarray({1}, *s.geometry_pairs);
    ctx.set_arg_ndarray({2}, *s.geometry_flags);
    ctx.set_arg_ndarray({3}, *s.geometry_permutation);
    ctx.set_arg_ndarray({4}, *s.geometry_source_map);
    ctx.set_arg_ndarray({5}, *s.geometry_offsets);
    ctx.set_arg_ndarray({6}, *s.geometry_cursors);
    ctx.set_arg_ndarray({7}, *s.geometry_orders);
    ctx.set_arg_ndarray({8}, *s.geometry_counters);
  }
  engine::argument_int(ctx, 9, s.live.contacts);
  geometry_finalize_.launch(ctx);
}

GeometryResult Harness::query_prepared_geometry(std::uint64_t expected,
                                                std::uint64_t work) {
  GeometryResult answer;
  answer.numerical_profile = profile_;
  if (!prepared_ || !ready_)
    return answer;
  auto &s = *prepared_;
  const auto &limits = s.capacity.limits();
  s.geometry_valid = false;
  answer.context = s.particles.context();
  answer.generation = s.generation;
  answer.completed_ticks = s.completed_ticks;
  answer.frame_anchor = frame_anchor_;
  const auto finish = [&](GeometryStatus status) {
    answer.status = status;
    s.geometry_last = answer;
    return answer;
  };
  if (!limits[Budget::GeometryWork])
    return finish(GeometryStatus::Unsupported);
  if (expected != s.generation)
    return finish(GeometryStatus::Stale);
  if (s.geometry_query == std::numeric_limits<std::uint64_t>::max())
    return finish(GeometryStatus::Unsupported);
  answer.query = ++s.geometry_query;
  if (work == std::numeric_limits<std::uint64_t>::max())
    work = limits[Budget::GeometryWork];
  const auto exhaust = [&](Budget budget, std::uint64_t required,
                           std::uint64_t capacity) {
    answer.failure = {budget, required, capacity};
    return finish(GeometryStatus::CapacityExhausted);
  };
  if (work > limits[Budget::GeometryWork])
    return exhaust(Budget::GeometryWork, work, limits[Budget::GeometryWork]);
  try {
    validate_prepared_bindings();
    launch_geometry_collect();
    runtime_->synchronize();
    ++answer.synchronizations;
    engine::readback(*s.geometry_counters, s.geometry_counts.data(),
                     sizeof(s.geometry_counts));
    engine::readback(*s.geometry_focus,
                     profile_.wide()
                         ? static_cast<void *>(s.geometry_focal64.data())
                         : s.geometry_focal.data(),
                     4 * profile_.bytes());
    answer.readback_bytes += sizeof(s.geometry_counts) + 4 * profile_.bytes();
    answer.present = s.geometry_counts[Present];
    answer.awake = s.geometry_counts[Awake];
    answer.source_contacts = s.geometry_counts[Sources];
    answer.presence_entries = s.geometry_counts[PresenceEntries];
    answer.compaction_slots = s.geometry_counts[CompactionSlots];
    answer.focus = GeometryFocus(s.geometry_counts[Focus]);
    if (profile_.wide()) {
      answer.focal_mass = s.geometry_focal64[3];
      std::copy_n(s.geometry_focal64.begin(), 3, answer.focal_point.begin());
    } else {
      answer.focal_mass = s.geometry_focal[3];
      std::copy_n(s.geometry_focal.begin(), 3, answer.focal_point.begin());
    }
    if (s.geometry_counts[Invalid] || answer.present != s.live.particles)
      return finish(GeometryStatus::InvalidNumerics);
    const auto pair_tests =
        answer.present * (answer.present ? answer.present - 1 : 0) / 2;
    const auto source_tests = answer.source_contacts * answer.source_contacts;
    answer.required_work = pair_tests + source_tests;
    if (answer.required_work > work)
      return exhaust(Budget::GeometryWork, answer.required_work, work);
    launch_geometry_source_order();
    launch_geometry_discover();
    runtime_->synchronize();
    ++answer.synchronizations;
    engine::readback(*s.geometry_counters, s.geometry_counts.data(),
                     sizeof(s.geometry_counts));
    answer.readback_bytes += sizeof(s.geometry_counts);
    answer.pair_tests = pair_tests;
    answer.source_order_tests = source_tests;
    answer.contacts = s.geometry_counts[Pairs];
    answer.derived = s.geometry_counts[Derived];
    if (s.geometry_counts[Invalid])
      return finish(GeometryStatus::InvalidNumerics);
    if (answer.contacts > limits[Budget::Contacts])
      return exhaust(Budget::Contacts, answer.contacts,
                     limits[Budget::Contacts]);
    const auto references = s.live.particles + s.live.fields +
                            2 * s.live.edges + 2 * answer.contacts +
                            s.live.focus;
    if (references > limits[Budget::References])
      return exhaust(Budget::References, references,
                     limits[Budget::References]);
    answer.required_work += answer.contacts * answer.contacts;
    if (answer.required_work > work)
      return exhaust(Budget::GeometryWork, answer.required_work, work);
    launch_geometry_finalize();
    runtime_->synchronize();
    ++answer.synchronizations;
    answer.union_order_tests = answer.contacts * answer.contacts;
    s.geometry_valid = true;
    return finish(GeometryStatus::Ok);
  } catch (...) {
    ready_ = false;
    s.geometry_valid = false;
    throw;
  }
}

const GeometryInspection &Harness::inspect_prepared_geometry(
    const GeometryResult &q) {
  if (!prepared_ || !ready_)
    throw std::logic_error("unready geometry inspection");
  auto &s = *prepared_;
  if (!s.geometry_valid || q.status != GeometryStatus::Ok ||
      q.query != s.geometry_last.query || q.context != s.particles.context() ||
      q.generation != s.generation || q.completed_ticks != s.completed_ticks)
    throw std::logic_error("stale geometry observation");
  try {
    validate_prepared_bindings();
    const auto copy = [](auto &array, auto &values) {
      engine::readback(*array, values.data(), values.size() * sizeof(int));
    };
    copy(s.geometry_rows, s.geometry_inspection.occupied_rows);
    copy(s.geometry_pairs, s.geometry_inspection.pairs);
    copy(s.geometry_flags, s.geometry_inspection.pair_flags);
    copy(s.geometry_source_map, s.geometry_inspection.source_to_pair);
    copy(s.geometry_offsets, s.geometry_inspection.offsets);
    copy(s.geometry_orders, s.geometry_inspection.orders);
    return s.geometry_inspection;
  } catch (...) {
    ready_ = false;
    s.geometry_valid = false;
    throw;
  }
}
}  // namespace field
