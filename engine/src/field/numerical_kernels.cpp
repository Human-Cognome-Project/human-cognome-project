#include "field/field.h"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "field/activity.h"
#include "field/centroid_math.h"
#include "field/contact_math.h"
#include "field/prepared_storage.h"

namespace field {
namespace {
using namespace taichi::lang;

// Element f of item i, addressed directly in the 2-axis {field, item} dense
// layout -- row-major, so this is exactly the old packed f*count+i byte
// offset; only the in-kernel index expression changes, not the layout.
std::vector<Stmt *> at(IRBuilder &b, int f, Stmt *i) {
  return {b.get_int32(f), i};
}

Stmt *load(IRBuilder &b, ArgLoadStmt *array, const std::vector<Stmt *> &index) {
  return b.create_global_load(b.create_external_ptr(array, index));
}

void store(IRBuilder &b,
           ArgLoadStmt *array,
           const std::vector<Stmt *> &index,
           Stmt *value) {
  b.create_global_store(b.create_external_ptr(array, index), value);
}

void accumulate(IRBuilder &b,
                ArgLoadStmt *array,
                const std::vector<Stmt *> &index,
                Stmt *value) {
  b.create_atomic_add(b.create_external_ptr(array, index), value);
}

void count_work(IRBuilder &b, ArgLoadStmt *counters, int which) {
  accumulate(b, counters, {b.get_int32(which)}, b.get_int64(1));
}

// total_dim is 2 for {field, item} dense arrays and 1 for universal/determined
// (single-axis, indexed directly).
ArgLoadStmt *array_param(IRBuilder &b, int id, DataType dt, int total_dim) {
  return b.create_ndarray_arg_load({id}, dt, total_dim, /*arg_depth=*/0);
}

Stmt *scalar_param(IRBuilder &b, int id, DataType dt) {
  return b.create_arg_load({id}, dt, /*is_ptr=*/false, /*arg_depth=*/0);
}

}  // namespace

std::array<engine::CompiledKernel, 7> Harness::build_numerical_kernels(
    detail::NativeSet particle_selection,
    detail::NativeSet centroid_selection) {
  auto &runtime = *runtime_;
  std::array<engine::CompiledKernel, 7> kernels;
  const int threads = runtime.cpu_threads();

  // Clear: particle accumulators, then the group side the next centroid pass
  // will build into, then the universal mass cell. The universal mass is the
  // total mass of the construct -- each particle's mass counted exactly
  // once -- so it is rebuilt here from the particle array directly, not from
  // the group centroids (which double-count a particle that sits in more
  // than one group). Three offloaded tasks, one compilation.
  {
    IRBuilder b;
    auto *particles = array_param(b, 0, real_type(), /*total_dim=*/2);
    auto *groups = array_param(b, 1, real_type(), /*total_dim=*/2);
    auto *universal = array_param(b, 4, real_type(), /*total_dim=*/1);
    auto *seed = scalar_param(b, 5, PrimitiveType::i32);
    auto *mass_count = scalar_param(b, 7, PrimitiveType::i32);
    auto *zero = real(b, 0.0f);
    {
      auto *loop =
          b.create_struct_for(particle_selection.cells, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      for (int f : {kForceX, kForceY, kForceZ}) {
        store(b, particles, at(b, f, i), zero);
      }
    }
    {
      auto *loop =
          b.create_struct_for(centroid_selection.cells, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      for (int f : {kAccX, kAccY, kAccZ}) {
        store(b, groups, at(b, f, i), zero);
      }
      auto reset = b.get_if_guard(b.create_if(seed), true);
      store(b, groups, at(b, kAccM, i), zero);
      for (int f : {kCenX, kCenY, kCenZ, kCenM}) {
        store(b, groups, at(b, f, i), zero);
      }
    }
    {
      auto *loop =
          b.create_range_for(b.get_int32(0), b.get_int32(1), false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      auto reset = b.get_if_guard(b.create_if(seed), true);
      store(b, universal, {i}, zero);
    }
    {
      auto *loop =
          b.create_range_for(b.get_int32(0), mass_count, false, 1, 0, true);
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      std::optional<IRBuilder::IfGuard> presence_guard;
      if (prepared_)
        presence_guard.emplace(b.get_if_guard(
            b.create_if(
                detail::NativeSet{prepared_->particles.bindings().presence}
                    .active(b, i)),
            true));
      if (profile_.wide())
        for (int axis = 0; axis < 3; ++axis)
          store(b, particles, at(b, kPosX + axis, i),
                detail::publish_position(
                    b, load(b, particles, at(b, kPosX + axis, i)),
                    profile_.body_fraction_bits));
      store(b, universal, {b.get_int32(0)},
            b.create_add(load(b, universal, {b.get_int32(0)}),
                         load(b, particles, at(b, kMass, i))));
    }
    kernels[0] = engine::CompiledKernel(
        runtime, "field_clear_" + std::to_string(particle_selection.cells->id),
        b.extract_ir(),
        {engine::Param::array(real_type(), /*total_dim=*/2),
         engine::Param::array(real_type(), /*total_dim=*/2),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::array(real_type()),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::array(PrimitiveType::i32, 2),
         engine::Param::scalar(PrimitiveType::i32)},
        engine::MathMode::Strict);
  }

  // Each candidate visits every connected field in stable source order.
  {
    IRBuilder b;
    auto *ratios =
        profile_.wide() ? array_param(b, 12, PrimitiveType::u64, 1) : nullptr;
    auto *edge_i = array_param(b, 0, PrimitiveType::i32, /*total_dim=*/2);
    auto *edge_f = array_param(b, 1, PrimitiveType::f32, /*total_dim=*/2);
    auto *particles = array_param(b, 2, real_type(), /*total_dim=*/2);
    auto *groups = array_param(b, 3, real_type(), /*total_dim=*/2);
    auto *n = scalar_param(b, 4, PrimitiveType::i32);
    auto *g = scalar_param(b, 5, PrimitiveType::i32);
    auto *e = scalar_param(b, 6, PrimitiveType::i32);
    auto *gravity = scalar_param(b, 7, real_type());
    auto *counters = array_param(b, 9, PrimitiveType::i64, 1);
    auto *offsets = array_param(b, 10, PrimitiveType::i32, 2);
    auto *order = array_param(b, 11, PrimitiveType::i32, 1);
    (void)n;
    (void)g;
    (void)e;

    auto *loop = b.create_struct_for(particle_selection.cells, false, threads);
    {
      auto guard = b.get_loop_guard(loop);
      auto *p = b.get_loop_index(loop);
      auto *members =
          b.create_range_for(load(b, offsets, at(b, 0, p)),
                             load(b, offsets, at(b, 1, p)), false, 1, 0, true);
      auto member_guard = b.get_loop_guard(members);
      auto *k = load(b, order, {b.get_loop_index(members)});
      auto *grp = load(b, edge_i, at(b, kEdgeGroup, k));
      auto *share = profile_.wide() ? detail::source_ratio(b, ratios, k,
                                                           std::max(edges_, 1))
                                    : load(b, edge_f, at(b, kShare, k));
      count_work(b, counters, detail::ForceRequests);
      if (profile_.wide()) {
        auto corrupt =
            b.get_if_guard(b.create_if(b.create_cmp_ne(
                               b.create_cast(share, PrimitiveType::f32),
                               load(b, edge_f, at(b, kShare, k)))),
                           true);
        count_work(b, counters, detail::TrialErrors);
      }
      auto *m1 = b.create_mul(load(b, particles, at(b, kMass, p)), share);
      auto *gm = load(b, groups, at(b, kCenM, grp));
      // Evaluate the exact zero-response cases before potentially overflowing
      // intermediates, rather than multiplying infinity by a zero gate.
      auto response = b.get_if_guard(
          b.create_if(b.create_logical_and(
              profile_.wide()
                  ? b.create_cmp_ne(
                        load(b, ratios,
                             {b.create_add(
                                 grp, b.get_int32(2 * std::max(edges_, 1)))}),
                        b.get_uint64(0))
                  : b.create_cmp_ne(gm, m1),
              b.create_cmp_ne(gravity, real(b, 0)))),
          true);
      count_work(b, counters, detail::Forces);

      auto *cx = load(b, particles, at(b, kPosX, p));
      auto *cy = load(b, particles, at(b, kPosY, p));
      auto *cz = load(b, particles, at(b, kPosZ, p));

      auto *gx = load(b, groups, at(b, kCenX, grp));
      auto *gy = load(b, groups, at(b, kCenY, grp));
      auto *gz = load(b, groups, at(b, kCenZ, grp));

      auto *dx = b.create_sub(gx, cx);
      auto *dy = b.create_sub(gy, cy);
      auto *dz = b.create_sub(gz, cz);

      // Exact squared separation: the one law is m1*m2/d^2, with no epsilon
      // added to any interaction. The singularity at d==0 is closed by an
      // exact branchless gate below, never by softening the law.
      auto *d2 =
          b.create_add(b.create_add(b.create_mul(dx, dx), b.create_mul(dy, dy)),
                       b.create_mul(dz, dz));
      // live == 1.0 when d2 > 0, exactly 0.0 when d2 == 0 (d2 is always >= 0).
      auto *live = b.create_sgn(d2);
      // Guard the rsqrt input against d2==0 without touching any d2>0 value:
      // safe == d2 where d2>0, == 1 where d2==0, so rsqrt is always finite.
      auto *safe = b.create_add(d2, b.create_sub(real(b, 1.0f), live));
      auto *inv = b.create_rsqrt(safe);
      auto *inv3 = b.create_mul(inv, b.create_mul(inv, inv));

      // Preserve the existing algebra after the exact zero-response branch.
      // A group whose whole mass is this participation has nothing else in it.
      auto *engaged = b.create_sgn(b.create_abs(b.create_sub(gm, m1)));
      // The exact d==0 gate (live) drives the interaction to exactly zero at
      // coincidence -- zero force, not infinity.
      auto *scale = b.create_mul(
          b.create_mul(b.create_mul(gravity, m1), b.create_mul(gm, inv3)),
          b.create_mul(engaged, live));

      auto *fx = b.create_mul(scale, dx);
      auto *fy = b.create_mul(scale, dy);
      auto *fz = b.create_mul(scale, dz);
      if (profile_.wide())
        for (auto *value :
             std::array<Stmt *, 9>{m1, gm, d2, inv, inv3, scale, fx, fy, fz})
          detail::mark_nonfinite(b, counters, value, detail::TrialErrors);
      store(b, particles, at(b, kForceX, p),
            b.create_add(load(b, particles, at(b, kForceX, p)), fx));
      store(b, particles, at(b, kForceY, p),
            b.create_add(load(b, particles, at(b, kForceY, p)), fy));
      store(b, particles, at(b, kForceZ, p),
            b.create_add(load(b, particles, at(b, kForceZ, p)), fz));
    }
    kernels[1] = engine::CompiledKernel(
        runtime, "field_force_" + std::to_string(particle_selection.cells->id),
        b.extract_ir(),
        detail::ratio_parameters(
            {engine::Param::array(PrimitiveType::i32, /*total_dim=*/2),
             engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
             engine::Param::array(real_type(), /*total_dim=*/2),
             engine::Param::array(real_type(), /*total_dim=*/2),
             engine::Param::scalar(PrimitiveType::i32),
             engine::Param::scalar(PrimitiveType::i32),
             engine::Param::scalar(PrimitiveType::i32),
             engine::Param::scalar(real_type()),
             engine::Param::array(PrimitiveType::i32, 2),
             engine::Param::array(PrimitiveType::i64),
             engine::Param::array(PrimitiveType::i32, 2),
             engine::Param::array(PrimitiveType::i32)},
            profile_.wide()),
        engine::MathMode::Strict);
  }

  // One writer per particle, with canonical A-side then B-side contact order.
  // This preserves the exact body-contact law without float atomic ordering.
  {
    IRBuilder b;
    auto *pairs = array_param(b, 0, PrimitiveType::i32, 2);
    auto *particles = array_param(b, 1, real_type(), 2);
    auto *count = scalar_param(b, 2, PrimitiveType::i32);
    auto *stiffness = scalar_param(b, 3, real_type());
    auto *counters = array_param(b, 5, PrimitiveType::i64, 1);
    auto *a_offsets = array_param(b, 6, PrimitiveType::i32, 2);
    auto *a_order = array_param(b, 7, PrimitiveType::i32, 1);
    auto *b_offsets = array_param(b, 8, PrimitiveType::i32, 2);
    auto *b_order = array_param(b, 9, PrimitiveType::i32, 1);
    (void)count;
    auto *loop = b.create_struct_for(particle_selection.cells, false, threads);
    {
      auto guard = b.get_loop_guard(loop);
      auto *p = b.get_loop_index(loop);
      for (int side = 0; side < 2; ++side) {
        auto *offsets = side == 0 ? a_offsets : b_offsets;
        auto *order = side == 0 ? a_order : b_order;
        auto *members = b.create_range_for(load(b, offsets, at(b, 0, p)),
                                           load(b, offsets, at(b, 1, p)), false,
                                           1, 0, true);
        auto member_guard = b.get_loop_guard(members);
        auto *k = load(b, order, {b.get_loop_index(members)});
        auto *a = load(b, pairs, at(b, kContactA, k));
        auto *bp = load(b, pairs, at(b, kContactB, k));
        count_work(b, counters, detail::Contacts);
        auto *dx = b.create_sub(load(b, particles, at(b, kPosX, bp)),
                                load(b, particles, at(b, kPosX, a)));
        auto *dy = b.create_sub(load(b, particles, at(b, kPosY, bp)),
                                load(b, particles, at(b, kPosY, a)));
        auto *dz = b.create_sub(load(b, particles, at(b, kPosZ, bp)),
                                load(b, particles, at(b, kPosZ, a)));
        const auto geometry = detail::contact_geometry(
            b, {dx, dy, dz}, load(b, particles, at(b, kRadius, a)),
            load(b, particles, at(b, kRadius, bp)), profile_.wide());
        auto *live = geometry.live, *inv = geometry.inverse,
             *penetration = geometry.penetration;
        if (profile_.wide())
          for (auto *value :
               std::array<Stmt *, 5>{geometry.squared, inv, geometry.distance,
                                     geometry.radii, penetration})
            detail::mark_nonfinite(b, counters, value, detail::TrialErrors);
        auto *scale = b.create_neg(b.create_mul(
            b.create_mul(stiffness, penetration), b.create_mul(live, inv)));
        for (int axis = 0; axis < 3; ++axis) {
          auto *delta = axis == 0 ? dx : (axis == 1 ? dy : dz);
          Stmt *force = b.create_mul(scale, delta);
          if (side == 1)
            force = b.create_neg(force);
          store(b, particles, at(b, kForceX + axis, p),
                b.create_add(load(b, particles, at(b, kForceX + axis, p)),
                             force));
        }
      }
    }
    kernels[2] = engine::CompiledKernel(
        runtime,
        "field_contact_" + std::to_string(particle_selection.cells->id),
        b.extract_ir(),
        {engine::Param::array(PrimitiveType::i32, 2),
         engine::Param::array(real_type(), 2),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::scalar(real_type()),
         engine::Param::array(PrimitiveType::i32, 2),
         engine::Param::array(PrimitiveType::i64),
         engine::Param::array(PrimitiveType::i32, 2),
         engine::Param::array(PrimitiveType::i32),
         engine::Param::array(PrimitiveType::i32, 2),
         engine::Param::array(PrimitiveType::i32),
         engine::Param::scalar(PrimitiveType::i32)},
        engine::MathMode::Strict);
  }

  // Integrate: force over total mass, then the brake, then the radial
  // velocity dampening.
  {
    IRBuilder b;
    auto *particles = array_param(b, 0, real_type(), /*total_dim=*/2);
    auto *dt = scalar_param(b, 2, real_type());
    auto *universal = array_param(b, 3, real_type(), /*total_dim=*/1);
    auto *reach_strength = scalar_param(b, 4, real_type());
    auto *ps = array_param(b, 5, PrimitiveType::i32, 2);
    auto *pd = array_param(b, 6, real_type(), 2);
    auto *origin = array_param(b, 7, real_type(), 1);
    auto *counters = array_param(b, 8, PrimitiveType::i64, 1);
    auto *edges = array_param(b, 9, PrimitiveType::i32, 2);
    auto *offsets = array_param(b, 10, PrimitiveType::i32, 2);
    auto *order = array_param(b, 11, PrimitiveType::i32, 1);
    auto *final_speed =
        profile_.wide() ? array_param(b, 12, real_type(), 1) : nullptr;
    const bool live_capacity =
        prepared_ && prepared_->capacity.limits()[Budget::LiveResponses];
    auto *leaf_fields =
        live_capacity ? array_param(b, 13, PrimitiveType::i32, 1) : nullptr;
    auto *live_installed =
        live_capacity ? scalar_param(b, 14, PrimitiveType::i32) : nullptr;

    auto *loop = b.create_struct_for(particle_selection.cells, false, threads);
    {
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);

      count_work(b, counters, detail::Evaluated);
      std::array<Stmt *, 3> old_position, old_velocity;
      for (int axis = 0; axis < 3; ++axis) {
        old_position[axis] = load(b, particles, at(b, kPosX + axis, i));
        old_velocity[axis] = load(b, particles, at(b, kVelX + axis, i));
      }

      auto *mass = load(b, particles, at(b, kMass, i));

      auto *fx = load(b, particles, at(b, kForceX, i));
      auto *fy = load(b, particles, at(b, kForceY, i));
      auto *fz = load(b, particles, at(b, kForceZ, i));

      // The whole mass absorbs the force the share raised.
      auto *mass_live = b.create_sgn(mass);
      auto *safe_mass =
          b.create_add(mass, b.create_sub(real(b, 1.0f), mass_live));
      auto *step_over_mass =
          b.create_div(b.create_mul(dt, mass_live), safe_mass);
      auto *vx =
          b.create_add(old_velocity[0], b.create_mul(fx, step_over_mass));
      auto *vy =
          b.create_add(old_velocity[1], b.create_mul(fy, step_over_mass));
      auto *vz =
          b.create_add(old_velocity[2], b.create_mul(fz, step_over_mass));

      // The brake. Distance to the resultant force pulling this particle --
      // the one accumulation the model defines -- against the distance this
      // step would travel.
      auto *reach = b.create_sqrt(
          b.create_add(b.create_add(b.create_mul(fx, fx), b.create_mul(fy, fy)),
                       b.create_mul(fz, fz)));

      auto *speed = b.create_sqrt(
          b.create_add(b.create_add(b.create_mul(vx, vx), b.create_mul(vy, vy)),
                       b.create_mul(vz, vz)));
      if (profile_.wide())
        for (auto *value : std::array<Stmt *, 5>{fx, fy, fz, reach, speed})
          detail::mark_nonfinite(b, counters, value, detail::TrialErrors);
      auto *travel = b.create_mul(speed, dt);

      // live == 1.0 when reach > 0, exactly 0.0 when reach == 0 (reach is
      // always >= 0) -- the same exact gate the force kernel uses for its
      // d==0 case. safe == reach where reach>0, == 1 where reach==0, so the
      // division is always finite; multiplying the exponent by live sends
      // the brake to exactly exp(0) == 1 when there is no pull to measure
      // against, rather than attenuating carried momentum or dividing by
      // zero.
      auto *live = b.create_sgn(reach);
      auto *safe = b.create_add(reach, b.create_sub(real(b, 1.0f), live));
      auto *ratio = b.create_div(travel, safe);
      auto *ratio2 = b.create_mul(ratio, ratio);
      // Negligible while the step is a real margin short of the reach,
      // aggressive as it closes on or crosses it.
      auto *brake = b.create_exp(
          b.create_neg(b.create_mul(b.create_mul(ratio2, ratio2), live)));

      vx = b.create_mul(vx, brake);
      vy = b.create_mul(vy, brake);
      vz = b.create_mul(vz, brake);

      // Radial velocity dampening: a first-cut draft, to be calibrated and
      // reviewed. It bounds outward runaway (a math artifact) while leaving
      // the settled bulk untouched -- the inverse of the (now suspended)
      // universal pull's job. The reach is where the total-mass field
      // strength M/d^2 falls to reach_strength; inside the reach the
      // dampening is inert.
      auto *px =
          b.create_sub(old_position[0], load(b, origin, {b.get_int32(0)}));
      auto *py =
          b.create_sub(old_position[1], load(b, origin, {b.get_int32(1)}));
      auto *pz =
          b.create_sub(old_position[2], load(b, origin, {b.get_int32(2)}));
      auto *pd2 =
          b.create_add(b.create_add(b.create_mul(px, px), b.create_mul(py, py)),
                       b.create_mul(pz, pz));
      // The same exact d==0 gate used elsewhere: no radial direction, no
      // dampening, at the origin.
      auto *plive = b.create_sgn(pd2);
      auto *psafe = b.create_add(pd2, b.create_sub(real(b, 1.0f), plive));
      auto *pinv = b.create_rsqrt(psafe);
      auto *rux = b.create_mul(px, pinv);
      auto *ruy = b.create_mul(py, pinv);
      auto *ruz = b.create_mul(pz, pinv);

      auto *m_universal = load(b, universal, {b.get_int32(0)});
      auto *total_live = b.create_sgn(m_universal);
      auto *safe_total =
          b.create_add(m_universal, b.create_sub(real(b, 1.0f), total_live));
      // reach_ratio == reach_strength / (M/d2) == threshold / field-strength.
      auto *reach_ratio =
          b.create_div(b.create_mul(reach_strength, pd2), safe_total);
      if (profile_.wide())
        for (auto *value : std::array<Stmt *, 2>{pd2, reach_ratio})
          detail::mark_nonfinite(b, counters, value, detail::TrialErrors);
      auto *excess =
          b.create_max(b.create_sub(reach_ratio, real(b, 1.0f)), real(b, 0.0f));
      // D == 1 inside the reach (bulk untouched), D -> 0 far outside
      // (escape velocity bled off).
      auto *dampen = b.create_exp(b.create_neg(excess));

      auto *vr = b.create_add(
          b.create_add(b.create_mul(vx, rux), b.create_mul(vy, ruy)),
          b.create_mul(vz, ruz));
      // Only the outward radial component is dampened -- inward motion is
      // untouched.
      auto *outward = b.create_max(vr, real(b, 0.0f));
      auto *factor = b.create_mul(
          b.create_mul(outward, b.create_sub(real(b, 1.0f), dampen)), plive);

      vx = b.create_sub(vx, b.create_mul(factor, rux));
      vy = b.create_sub(vy, b.create_mul(factor, ruy));
      vz = b.create_sub(vz, b.create_mul(factor, ruz));

      if (!profile_.wide()) {
        store(b, particles, at(b, kVelX, i), vx);
        store(b, particles, at(b, kVelY, i), vy);
        store(b, particles, at(b, kVelZ, i), vz);
      }
      std::array<Stmt *, 3> retained_velocity{};
      auto *positive_dt = profile_.wide()
                              ? b.create_cmp_gt(dt, real(b, 0)) : nullptr;
      auto *safe_dt = profile_.wide()
                          ? b.create_select(positive_dt, dt, real(b, 1)) : nullptr;

      Stmt *moved = b.get_int32(0), *continued = b.get_int32(0),
           *attempted = b.get_int32(0);
      for (int pair : {0, 1, 2}) {
        const int pos_field = kPosX + pair;
        Stmt *v = pair == 0 ? vx : (pair == 1 ? vy : vz);
        auto *trial = b.create_add(old_position[pair], b.create_mul(v, dt));
        attempted = b.create_logical_or(
            attempted, b.create_cmp_ne(trial, old_position[pair]));
        if (profile_.wide()) {
          detail::mark_nonfinite(b, counters, trial, detail::TrialErrors);
          detail::mark_nonfinite_motion(b, counters, v, detail::TrialErrors);
        }
        auto *next = profile_.wide()
                         ? detail::publish_position(b, trial,
                                                    profile_.body_fraction_bits)
                         : trial;
        auto *accepted = b.create_sub(next, old_position[pair]);
        store(b, pd, at(b, pair, i), accepted);
        if (profile_.wide()) {
          // One publication precision governs position and retained motion.
          // A zero-duration visit preserves incoming motion; no division by
          // zero or artificial velocity reset is introduced by observation.
          retained_velocity[pair] = b.create_select(
              positive_dt, b.create_div(accepted, safe_dt), old_velocity[pair]);
          detail::mark_nonfinite(b, counters, retained_velocity[pair],
                                detail::TrialErrors);
          store(b, particles, at(b, kVelX + pair, i), retained_velocity[pair]);
        }

        moved = b.create_logical_or(moved,
                                    b.create_cmp_ne(next, old_position[pair]));
        if (!profile_.wide())
          continued = b.create_logical_or(
              continued, b.create_cmp_ne(v, old_velocity[pair]));
        store(b, particles, at(b, pos_field, i), next);
      }
      if (profile_.wide()) {
        auto *speed2 = real(b, 0);
        for (auto *v : retained_velocity)
          speed2 = b.create_add(speed2, b.create_mul(v, v));
        auto *value = b.create_sqrt(speed2);
        detail::mark_nonfinite(b, counters, value, detail::TrialErrors);
        store(b, final_speed, {i}, value);
      }
      {
        // Trial movement activity follows the complete published change.

        // Attempts and velocity remain physical/diagnostic state, not another
        // reason to poll. No same-type notifier overlaps this phase; later
        // contact/centroid notifications remain pending after consumption.
        auto fixed = b.get_if_guard(
            b.create_if(b.create_logical_not(
                profile_.wide() ? moved
                                : b.create_logical_or(continued, moved))),
            true);
        particle_selection.deactivate(b, i);
      }
      if (profile_.wide()) {
        auto attempted_guard = b.get_if_guard(b.create_if(attempted), true);
        count_work(b, counters, detail::TrialAttempts);
      }
      auto *motion = b.create_if(moved);
      {
        auto moving = b.get_if_guard(motion, true);
        store(b, ps, at(b, detail::Moving, i), b.get_int32(1));
        count_work(b, counters, detail::Moved);
        // Repeated member touches collapse in Taichi's native centroid mask.
        {
          auto *members = b.create_range_for(load(b, offsets, at(b, 0, i)),
                                             load(b, offsets, at(b, 1, i)),
                                             false, 1, 0, true);
          auto member_guard = b.get_loop_guard(members);
          auto *edge = load(b, order, {b.get_loop_index(members)});
          auto *field = load(b, edges, at(b, kEdgeGroup, edge));
          std::optional<IRBuilder::IfGuard> selected;
          if (live_capacity)
            selected.emplace(
                b.get_if_guard(b.create_if(b.create_logical_or(
                                   b.create_logical_not(live_installed),
                                   load(b, leaf_fields, {field}))),
                               true));
          centroid_selection.activate(b, field);
          count_work(b, counters, detail::AdjacencyVisits);
        }
        auto newly = b.get_if_guard(
            b.create_if(b.create_cmp_eq(
                load(b, ps, at(b, detail::PriorMoving, i)), b.get_int32(0))),
            true);
        count_work(b, counters, detail::Woken);
      }
      {
        auto still = b.get_if_guard(motion, false);
        count_work(b, counters, detail::Rejected);
      }
    }
    std::vector<engine::Param> params{
        engine::Param::array(real_type(), 2),
        engine::Param::scalar(PrimitiveType::i32),
        engine::Param::scalar(real_type()),
        engine::Param::array(real_type()),
        engine::Param::scalar(real_type()),
        engine::Param::array(PrimitiveType::i32, 2),
        engine::Param::array(real_type(), 2),
        engine::Param::array(real_type()),
        engine::Param::array(PrimitiveType::i64),
        engine::Param::array(PrimitiveType::i32, 2),
        engine::Param::array(PrimitiveType::i32, 2),
        engine::Param::array(PrimitiveType::i32)};
    if (profile_.wide())
      params.push_back(engine::Param::array(real_type()));
    if (live_capacity) {
      params.push_back(engine::Param::array(PrimitiveType::i32));
      params.push_back(engine::Param::scalar(PrimitiveType::i32));
    }
    kernels[3] = engine::CompiledKernel(
        runtime,
        "field_integrate_" + std::to_string(particle_selection.cells->id),
        b.extract_ir(), params, engine::MathMode::Strict);
  }

  // Determiners: the secondary set, running on the products of the primary
  // calculation. Fixed-state particles have just consumed activity; their
  // cached speed is still correct. Continuing particles update it here.
  {
    IRBuilder b;
    auto *error_counts =
        profile_.wide() ? array_param(b, 3, PrimitiveType::i64, 1) : nullptr;
    auto *particles = array_param(b, 0, real_type(), /*total_dim=*/2);
    auto *out = array_param(b, 1, real_type(), /*total_dim=*/1);
    auto *loop = b.create_struct_for(particle_selection.cells, false, threads);
    {
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      auto *vx = load(b, particles, at(b, kVelX, i));
      auto *vy = load(b, particles, at(b, kVelY, i));
      auto *vz = load(b, particles, at(b, kVelZ, i));
      auto *speed = b.create_sqrt(
          b.create_add(b.create_add(b.create_mul(vx, vx), b.create_mul(vy, vy)),
                       b.create_mul(vz, vz)));
      if (profile_.wide())
        detail::mark_nonfinite(b, error_counts, speed, detail::TrialErrors);
      store(b, out, {i}, speed);
    }
    std::vector<engine::Param> params{
        engine::Param::array(real_type(), 2), engine::Param::array(real_type()),
        engine::Param::scalar(PrimitiveType::i32)};
    if (profile_.wide())
      params.push_back(engine::Param::array(PrimitiveType::i64));
    kernels[4] = engine::CompiledKernel(
        runtime,
        "field_determine_" + std::to_string(particle_selection.cells->id),
        b.extract_ir(), params, engine::MathMode::Strict);
  }

  // Centroid: the consolidation. A reduction over the same edge list the
  // force pass streamed, into per-group accumulators.
  {
    IRBuilder b;
    auto *ratios =
        profile_.wide() ? array_param(b, 12, PrimitiveType::u64, 1) : nullptr;
    auto *edge_i = array_param(b, 0, PrimitiveType::i32, /*total_dim=*/2);
    auto *edge_f = array_param(b, 1, PrimitiveType::f32, /*total_dim=*/2);
    auto *particles = array_param(b, 2, real_type(), /*total_dim=*/2);
    auto *groups = array_param(b, 3, real_type(), /*total_dim=*/2);
    auto *n = scalar_param(b, 4, PrimitiveType::i32);
    auto *g = scalar_param(b, 5, PrimitiveType::i32);
    auto *e = scalar_param(b, 6, PrimitiveType::i32);
    auto *counters = array_param(b, 8, PrimitiveType::i64, 1);
    auto *seed = scalar_param(b, 9, PrimitiveType::i32);
    auto *offsets = array_param(b, 10, PrimitiveType::i32, 2);
    auto *order = array_param(b, 11, PrimitiveType::i32, 1);
    auto *anchor_work =
        profile_.wide() ? array_param(b, 7, real_type(), 2) : nullptr;
    (void)n;
    (void)g;
    (void)e;

    auto *loop = b.create_struct_for(centroid_selection.cells, false, threads);
    {
      auto guard = b.get_loop_guard(loop);
      auto *grp = b.get_loop_index(loop);
      // Native centroid entries are activated during integration, after clear.
      for (int axis = 0; axis < 3; ++axis)
        store(b, groups, at(b, kAccX + axis, grp), real(b, 0));
      if (profile_.wide()) {
        store(b, groups, at(b, kAccM, grp), real(b, 0));
        for (int axis = 0; axis < 3; ++axis)
          store(b, anchor_work, at(b, axis, grp), real(b, 0));
      }
      // Seed around a member position rather than coordinate zero, preserving
      // the initial rounding residue that later small motions can combine with.
      {
        auto *begin = load(b, offsets, at(b, 0, grp));
        auto *end = load(b, offsets, at(b, 1, grp));
        auto initial =
            b.get_if_guard(b.create_if(b.create_logical_and(
                               profile_.wide() ? b.get_int32(1) : seed,
                               b.create_cmp_lt(begin, end))),
                           true);
        auto *first = load(b, order, {begin});
        auto *anchor = load(b, edge_i, at(b, kEdgeParticle, first));
        for (int axis = 0; axis < 3; ++axis)
          store(b, profile_.wide() ? anchor_work : groups,
                at(b, profile_.wide() ? axis : kCenX + axis, grp),
                load(b, particles, at(b, kPosX + axis, anchor)));
      }
      auto *members = b.create_range_for(load(b, offsets, at(b, 0, grp)),
                                         load(b, offsets, at(b, 1, grp)), false,
                                         1, 0, true);
      auto member_guard = b.get_loop_guard(members);
      auto *k = load(b, order, {b.get_loop_index(members)});
      auto *p = load(b, edge_i, at(b, kEdgeParticle, k));
      auto *share = profile_.wide() ? detail::source_ratio(b, ratios, k,
                                                           std::max(edges_, 1))
                                    : load(b, edge_f, at(b, kShare, k));
      count_work(b, counters, detail::ReducedEdges);
      auto *m = b.create_mul(load(b, particles, at(b, kMass, p)), share);
      if (profile_.wide()) {
        auto bad =
            b.get_if_guard(b.create_if(b.create_cmp_le(m, real(b, 0))), true);
        count_work(b, counters, detail::TrialErrors);
      }
      for (int pair : {0, 1, 2}) {
        auto *pos = b.create_sub(
            load(b, particles, at(b, kPosX + pair, p)),
            load(b, profile_.wide() ? anchor_work : groups,
                 at(b, profile_.wide() ? pair : kCenX + pair, grp)));
        store(b, groups, at(b, kAccX + pair, grp),
              b.create_add(load(b, groups, at(b, kAccX + pair, grp)),
                           b.create_mul(m, pos)));
      }
      {
        auto initial = b.get_if_guard(
            b.create_if(profile_.wide() ? b.get_int32(1) : seed), true);
        store(b, groups, at(b, kAccM, grp),
              b.create_add(load(b, groups, at(b, kAccM, grp)), m));
      }
    }
    kernels[5] = engine::CompiledKernel(
        runtime,
        "field_centroid_" + std::to_string(particle_selection.cells->id),
        b.extract_ir(),
        detail::ratio_parameters(
            {engine::Param::array(PrimitiveType::i32, /*total_dim=*/2),
             engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
             engine::Param::array(real_type(), /*total_dim=*/2),
             engine::Param::array(real_type(), /*total_dim=*/2),
             engine::Param::scalar(PrimitiveType::i32),
             engine::Param::scalar(PrimitiveType::i32),
             engine::Param::scalar(PrimitiveType::i32),
             engine::Param::array(
                 profile_.wide() ? real_type() : PrimitiveType::i32, 2),
             engine::Param::array(PrimitiveType::i64),
             engine::Param::scalar(PrimitiveType::i32),
             engine::Param::array(PrimitiveType::i32, 2),
             engine::Param::array(PrimitiveType::i32)},
            profile_.wide()),
        engine::MathMode::Strict);
  }

  // Publish: divide the accumulators through and hand them to the next tick.
  {
    IRBuilder b;
    auto *groups = array_param(b, 0, real_type(), /*total_dim=*/2);
    auto *gs = array_param(b, 2, PrimitiveType::i32, 2);
    auto *gd = array_param(b, 3, real_type(), 2);
    auto *counters = array_param(b, 4, PrimitiveType::i64, 1);
    {
      auto *loop =
          b.create_struct_for(centroid_selection.cells, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      store(b, gs, at(b, detail::Changed, i), b.get_int32(0));
      count_work(b, counters, detail::Recomputed);
      auto *m = load(b, groups, at(b, kAccM, i));
      // Ordinary weighted mean: preserve every positive mass, substitute one
      // only for the exact empty-mass case.
      Stmt *changed = b.get_int32(0);
      for (int pair : {0, 1, 2}) {
        auto *before = load(b, groups, at(b, kCenX + pair, i));
        auto *offset = load(b, groups, at(b, kAccX + pair, i));
        // Widen the final origin addition so a distant seed anchor cannot
        // introduce an extra f32 rounding before publishing the f32 centre.
        const auto coordinate = detail::centroid_coordinate(
            b, profile_.wide() ? load(b, gd, at(b, pair, i)) : before, offset,
            m, profile_);
        if (profile_.wide()) {
          detail::mark_nonfinite(b, counters, coordinate.trial,
                                 detail::TrialErrors);
          detail::mark_nonfinite(b, counters, m, detail::TrialErrors);
        }
        auto *next = coordinate.value;
        auto *pending = coordinate.residue;
        store(b, gd, at(b, 3 + pair, i), before);
        store(b, gd, at(b, pair, i), pending);
        changed = b.create_logical_or(changed, b.create_cmp_ne(next, before));
        store(b, groups, at(b, kCenX + pair, i), next);
      }
      {
        auto moved = b.get_if_guard(b.create_if(changed), true);
        store(b, gs, at(b, detail::Changed, i), b.get_int32(1));
      }
      store(b, groups, at(b, kCenM, i), m);
    }
    kernels[6] = engine::CompiledKernel(
        runtime,
        "field_publish_centroids_" +
            std::to_string(particle_selection.cells->id),
        b.extract_ir(),
        {engine::Param::array(real_type(), /*total_dim=*/2),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::array(PrimitiveType::i32, 2),
         engine::Param::array(real_type(), 2),
         engine::Param::array(PrimitiveType::i64)},
        engine::MathMode::Strict);
  }
  return kernels;
}

}  // namespace field
