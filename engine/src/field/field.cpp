#include "field/field.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <set>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

#include "engine/packed_view.h"
#include "engine/transfer.h"
#include "field/activity.h"

namespace field {
namespace {

using Clock = std::chrono::steady_clock;
double elapsed(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start)
      .count();
}

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

void store(IRBuilder &b, ArgLoadStmt *array, const std::vector<Stmt *> &index,
           Stmt *value) {
  b.create_global_store(b.create_external_ptr(array, index), value);
}

void accumulate(IRBuilder &b, ArgLoadStmt *array,
                const std::vector<Stmt *> &index, Stmt *value) {
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

void validate_parameters(const Parameters &p) {
  for (float value : {p.gravity, p.dt, p.contact_stiffness, p.reach_strength}) {
    if (!std::isfinite(value) || value < 0.0f) {
      throw std::invalid_argument(
          "field: parameters must be finite and non-negative");
    }
  }
}

std::size_t storage_size(int fields, int count) {
  return std::size_t(fields) * std::size_t(std::max(count, 1));
}

}  // namespace

Harness::Harness(engine::Runtime &runtime, int particle_count, int group_count,
                 int edge_count, int contact_count, HostPath path)
    : runtime_(&runtime),
      particles_(particle_count),
      groups_(group_count),
      edges_(edge_count),
      contacts_(contact_count),
      path_(path) {
  const auto allocation_start = Clock::now();
  if (path_ == HostPath::CpuDirect && runtime.on_cuda())
    throw std::invalid_argument("direct host binding requires CPU backend");
  if (particle_count < 0 || group_count < 0 || edge_count < 0 ||
      contact_count < 0) {
    throw std::invalid_argument("field::Harness: counts must be non-negative");
  }
  // Empty logical arrays still need nonzero allocation/transfer sizes in
  // the RHI. Their real loop counts and host accessor strides stay zero.
  particle_data.assign(storage_size(kParticleFieldCount, particles_), 0.0f);
  group_data.assign(storage_size(kGroupFieldCount, groups_), 0.0f);
  edge_float_data.assign(storage_size(kEdgeFloatFieldCount, edges_), 0.0f);
  edge_int_data.assign(storage_size(kEdgeIntFieldCount, edges_), 0);
  contact_int_data.assign(storage_size(kContactIntFieldCount, contacts_), 0);
  determined_.assign(particles_, 0.0f);
  universal_data.assign(1, 0.0f);

  // Each dense array below is 2-axis {num_fields, num_items} rather than a
  // single flat {num_fields * num_items} axis: a 1-D Ndarray shape is a
  // single i32 axis and so cannot even express more than 2^31 elements,
  // while two axes each far under 2^31 let the (i64-widened) codegen
  // flatten the product beyond that cap. Row-major, so element (f, i) still
  // lives at byte offset f*num_items + i -- exactly the host SoA layout
  // below; only the in-kernel index expression changes.
  Program &program = runtime.program();
  if (path_ == HostPath::BulkCopy) {
    particle_array_ = std::make_unique<Ndarray>(
        &program, PrimitiveType::f32,
        std::vector<int>{kParticleFieldCount, std::max(particles_, 1)});
    group_array_ = std::make_unique<Ndarray>(
        &program, PrimitiveType::f32,
        std::vector<int>{kGroupFieldCount, std::max(groups_, 1)});
    edge_float_array_ = std::make_unique<Ndarray>(
        &program, PrimitiveType::f32,
        std::vector<int>{kEdgeFloatFieldCount, std::max(edges_, 1)});
    edge_int_array_ = std::make_unique<Ndarray>(
        &program, PrimitiveType::i32,
        std::vector<int>{kEdgeIntFieldCount, std::max(edges_, 1)});
    contact_int_array_ = std::make_unique<Ndarray>(
        &program, PrimitiveType::i32,
        std::vector<int>{kContactIntFieldCount, std::max(contacts_, 1)});
    determined_array_ =
        std::make_unique<Ndarray>(&program, PrimitiveType::f32,
                                  std::vector<int>{std::max(particles_, 1)});
    // The universal mass: one cell, the total mass of the construct with each
    // particle counted exactly once, recomputed by the clear pass each tick.
    universal_array_ = std::make_unique<Ndarray>(&program, PrimitiveType::f32,
                                                 std::vector<int>{1});
  }
  activity_ = std::make_unique<detail::Activity>(runtime, particles_, groups_, edges_, contacts_);
  timings_.allocation_ms = elapsed(allocation_start) - activity_->compilation_ms;
  const auto compilation_start = Clock::now();

  const int threads = runtime.cpu_threads();

  // Clear: particle accumulators, then the group side the next centroid pass
  // will build into, then the universal mass cell. The universal mass is the
  // total mass of the construct -- each particle's mass counted exactly
  // once -- so it is rebuilt here from the particle array directly, not from
  // the group centroids (which double-count a particle that sits in more
  // than one group). Three offloaded tasks, one compilation.
  {
    IRBuilder b;
    auto *particles = array_param(b, 0, PrimitiveType::f32, /*total_dim=*/2);
    auto *groups = array_param(b, 1, PrimitiveType::f32, /*total_dim=*/2);
    auto *n = scalar_param(b, 2, PrimitiveType::i32);
    auto *g = scalar_param(b, 3, PrimitiveType::i32);
    auto *universal = array_param(b, 4, PrimitiveType::f32, /*total_dim=*/1);
    auto *seed = scalar_param(b, 5, PrimitiveType::i32);
    auto *ps = array_param(b, 6, PrimitiveType::i32, 2);
    auto *mass_count = scalar_param(b, 7, PrimitiveType::i32);
    auto *zero = b.get_float32(0.0f);
    {
      auto *loop = b.create_range_for(b.get_int32(0), n, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      auto active =
          b.get_if_guard(b.create_if(b.create_logical_or(
                             seed, load(b, ps, at(b, detail::Candidate, i)))),
                         true);
      for (int f : {kForceX, kForceY, kForceZ}) {
        store(b, particles, at(b, f, i), zero);
      }
    }
    {
      auto *loop = b.create_range_for(b.get_int32(0), g, false, threads);
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
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(1), false,
                                      threads);
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
      store(b, universal, {b.get_int32(0)},
            b.create_add(load(b, universal, {b.get_int32(0)}),
                         load(b, particles, at(b, kMass, i))));
    }
    clear_ = engine::CompiledKernel(
        runtime, "field_clear", b.extract_ir(),
        {engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::array(PrimitiveType::f32),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::array(PrimitiveType::i32, 2),
         engine::Param::scalar(PrimitiveType::i32)},
        engine::MathMode::Strict);
  }

  // Each candidate visits every connected field in stable source order.
  {
    IRBuilder b;
    auto *edge_i = array_param(b, 0, PrimitiveType::i32, /*total_dim=*/2);
    auto *edge_f = array_param(b, 1, PrimitiveType::f32, /*total_dim=*/2);
    auto *particles = array_param(b, 2, PrimitiveType::f32, /*total_dim=*/2);
    auto *groups = array_param(b, 3, PrimitiveType::f32, /*total_dim=*/2);
    auto *n = scalar_param(b, 4, PrimitiveType::i32);
    auto *g = scalar_param(b, 5, PrimitiveType::i32);
    auto *e = scalar_param(b, 6, PrimitiveType::i32);
    auto *gravity = scalar_param(b, 7, PrimitiveType::f32);
    auto *ps = array_param(b, 8, PrimitiveType::i32, 2);
    auto *counters = array_param(b, 9, PrimitiveType::i64, 1);
    auto *offsets = array_param(b, 10, PrimitiveType::i32, 2);
    auto *order = array_param(b, 11, PrimitiveType::i32, 1);
    (void)n;
    (void)g;
    (void)e;

    auto *loop = b.create_range_for(b.get_int32(0), n, false, threads);
    {
      auto guard = b.get_loop_guard(loop);
      auto *p = b.get_loop_index(loop);
      auto active = b.get_if_guard(
          b.create_if(load(b, ps, at(b, detail::Candidate, p))), true);
      auto *members =
          b.create_range_for(load(b, offsets, at(b, 0, p)),
                             load(b, offsets, at(b, 1, p)), false, 1, 0, true);
      auto member_guard = b.get_loop_guard(members);
      auto *k = load(b, order, {b.get_loop_index(members)});
      auto *grp = load(b, edge_i, at(b, kEdgeGroup, k));
      auto *share = load(b, edge_f, at(b, kShare, k));
      count_work(b, counters, detail::ForceRequests);
      auto *m1 = b.create_mul(load(b, particles, at(b, kMass, p)), share);
      auto *gm = load(b, groups, at(b, kCenM, grp));
      // Evaluate the exact zero-response cases before potentially overflowing
      // intermediates, rather than multiplying infinity by a zero gate.
      auto response =
          b.get_if_guard(b.create_if(b.create_logical_and(
                             b.create_cmp_ne(gm, m1),
                             b.create_cmp_ne(gravity, b.get_float32(0)))),
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
      auto *d2 = b.create_add(
          b.create_add(b.create_mul(dx, dx), b.create_mul(dy, dy)),
          b.create_mul(dz, dz));
      // live == 1.0 when d2 > 0, exactly 0.0 when d2 == 0 (d2 is always >= 0).
      auto *live = b.create_sgn(d2);
      // Guard the rsqrt input against d2==0 without touching any d2>0 value:
      // safe == d2 where d2>0, == 1 where d2==0, so rsqrt is always finite.
      auto *safe =
          b.create_add(d2, b.create_sub(b.get_float32(1.0f), live));
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
      store(b, particles, at(b, kForceX, p),
            b.create_add(load(b, particles, at(b, kForceX, p)), fx));
      store(b, particles, at(b, kForceY, p),
            b.create_add(load(b, particles, at(b, kForceY, p)), fy));
      store(b, particles, at(b, kForceZ, p),
            b.create_add(load(b, particles, at(b, kForceZ, p)), fz));
    }
    force_ = engine::CompiledKernel(
        runtime, "field_force", b.extract_ir(),
        {engine::Param::array(PrimitiveType::i32, /*total_dim=*/2),
         engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::scalar(PrimitiveType::f32),
         engine::Param::array(PrimitiveType::i32, 2),
         engine::Param::array(PrimitiveType::i64),
         engine::Param::array(PrimitiveType::i32, 2),
         engine::Param::array(PrimitiveType::i32)},
        engine::MathMode::Strict);
  }

  // One writer per particle, with canonical A-side then B-side contact order.
  // This preserves the exact body-contact law without float atomic ordering.
  {
    IRBuilder b;
    auto *pairs = array_param(b, 0, PrimitiveType::i32, 2);
    auto *particles = array_param(b, 1, PrimitiveType::f32, 2);
    auto *count = scalar_param(b, 2, PrimitiveType::i32);
    auto *stiffness = scalar_param(b, 3, PrimitiveType::f32);
    auto *ps = array_param(b, 4, PrimitiveType::i32, 2);
    auto *counters = array_param(b, 5, PrimitiveType::i64, 1);
    auto *a_offsets = array_param(b, 6, PrimitiveType::i32, 2);
    auto *a_order = array_param(b, 7, PrimitiveType::i32, 1);
    auto *b_offsets = array_param(b, 8, PrimitiveType::i32, 2);
    auto *b_order = array_param(b, 9, PrimitiveType::i32, 1);
    auto *n = scalar_param(b, 10, PrimitiveType::i32);
    (void)count;
    auto *loop = b.create_range_for(b.get_int32(0), n, false, threads);
    {
      auto guard = b.get_loop_guard(loop);
      auto *p = b.get_loop_index(loop);
      auto active = b.get_if_guard(
          b.create_if(load(b, ps, at(b, detail::Candidate, p))), true);
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
        auto *d2 = b.create_add(
            b.create_add(b.create_mul(dx, dx), b.create_mul(dy, dy)),
            b.create_mul(dz, dz));
        auto *live = b.create_sgn(d2);
        auto *safe = b.create_add(d2, b.create_sub(b.get_float32(1), live));
        auto *inv = b.create_rsqrt(safe);
        auto *distance = b.create_mul(safe, inv);
        auto *radii = b.create_add(load(b, particles, at(b, kRadius, a)),
                                   load(b, particles, at(b, kRadius, bp)));
        auto *penetration =
            b.create_max(b.create_sub(radii, distance), b.get_float32(0));
        auto *scale = b.create_neg(b.create_mul(
            b.create_mul(stiffness, penetration), b.create_mul(live, inv)));
        for (int axis = 0; axis < 3; ++axis) {
          auto *delta = axis == 0 ? dx : (axis == 1 ? dy : dz);
          Stmt *force = b.create_mul(scale, delta);
          if (side == 1) force = b.create_neg(force);
          store(b, particles, at(b, kForceX + axis, p),
                b.create_add(load(b, particles, at(b, kForceX + axis, p)),
                             force));
        }
      }
    }
    contact_ =
        engine::CompiledKernel(runtime, "field_contact", b.extract_ir(),
                               {engine::Param::array(PrimitiveType::i32, 2),
                                engine::Param::array(PrimitiveType::f32, 2),
                                engine::Param::scalar(PrimitiveType::i32),
                                engine::Param::scalar(PrimitiveType::f32),
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
    auto *particles = array_param(b, 0, PrimitiveType::f32, /*total_dim=*/2);
    auto *n = scalar_param(b, 1, PrimitiveType::i32);
    auto *dt = scalar_param(b, 2, PrimitiveType::f32);
    auto *universal = array_param(b, 3, PrimitiveType::f32, /*total_dim=*/1);
    auto *reach_strength = scalar_param(b, 4, PrimitiveType::f32);
    auto *ps = array_param(b, 5, PrimitiveType::i32, 2);
    auto *pd = array_param(b, 6, PrimitiveType::f32, 2);
    auto *origin = array_param(b, 7, PrimitiveType::f32, 1);
    auto *counters = array_param(b, 8, PrimitiveType::i64, 1);

    auto *loop = b.create_range_for(b.get_int32(0), n, false, threads);
    {
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);

      auto active = b.get_if_guard(
          b.create_if(load(b, ps, at(b, detail::Candidate, i))), true);
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
      auto *safe_mass = b.create_add(
          mass, b.create_sub(b.get_float32(1.0f), mass_live));
      auto *step_over_mass = b.create_div(b.create_mul(dt, mass_live), safe_mass);
      auto *vx =
          b.create_add(old_velocity[0], b.create_mul(fx, step_over_mass));
      auto *vy =
          b.create_add(old_velocity[1], b.create_mul(fy, step_over_mass));
      auto *vz =
          b.create_add(old_velocity[2], b.create_mul(fz, step_over_mass));

      // The brake. Distance to the resultant force pulling this particle --
      // the one accumulation the model defines -- against the distance this
      // step would travel.
      auto *reach = b.create_sqrt(b.create_add(
          b.create_add(b.create_mul(fx, fx), b.create_mul(fy, fy)),
          b.create_mul(fz, fz)));

      auto *speed = b.create_sqrt(b.create_add(
          b.create_add(b.create_mul(vx, vx), b.create_mul(vy, vy)),
          b.create_mul(vz, vz)));
      auto *travel = b.create_mul(speed, dt);

      // live == 1.0 when reach > 0, exactly 0.0 when reach == 0 (reach is
      // always >= 0) -- the same exact gate the force kernel uses for its
      // d==0 case. safe == reach where reach>0, == 1 where reach==0, so the
      // division is always finite; multiplying the exponent by live sends
      // the brake to exactly exp(0) == 1 when there is no pull to measure
      // against, rather than attenuating carried momentum or dividing by
      // zero.
      auto *live = b.create_sgn(reach);
      auto *safe =
          b.create_add(reach, b.create_sub(b.get_float32(1.0f), live));
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
      auto *pd2 = b.create_add(
          b.create_add(b.create_mul(px, px), b.create_mul(py, py)),
          b.create_mul(pz, pz));
      // The same exact d==0 gate used elsewhere: no radial direction, no
      // dampening, at the origin.
      auto *plive = b.create_sgn(pd2);
      auto *psafe =
          b.create_add(pd2, b.create_sub(b.get_float32(1.0f), plive));
      auto *pinv = b.create_rsqrt(psafe);
      auto *rux = b.create_mul(px, pinv);
      auto *ruy = b.create_mul(py, pinv);
      auto *ruz = b.create_mul(pz, pinv);

      auto *m_universal = load(b, universal, {b.get_int32(0)});
      auto *total_live = b.create_sgn(m_universal);
      auto *safe_total = b.create_add(
          m_universal, b.create_sub(b.get_float32(1.0f), total_live));
      // reach_ratio == reach_strength / (M/d2) == threshold / field-strength.
      auto *reach_ratio =
          b.create_div(b.create_mul(reach_strength, pd2), safe_total);
      auto *excess =
          b.create_max(b.create_sub(reach_ratio, b.get_float32(1.0f)),
                       b.get_float32(0.0f));
      // D == 1 inside the reach (bulk untouched), D -> 0 far outside
      // (escape velocity bled off).
      auto *dampen = b.create_exp(b.create_neg(excess));

      auto *vr = b.create_add(
          b.create_add(b.create_mul(vx, rux), b.create_mul(vy, ruy)),
          b.create_mul(vz, ruz));
      // Only the outward radial component is dampened -- inward motion is
      // untouched.
      auto *outward = b.create_max(vr, b.get_float32(0.0f));
      auto *factor = b.create_mul(
          b.create_mul(outward, b.create_sub(b.get_float32(1.0f), dampen)),
          plive);

      vx = b.create_sub(vx, b.create_mul(factor, rux));
      vy = b.create_sub(vy, b.create_mul(factor, ruy));
      vz = b.create_sub(vz, b.create_mul(factor, ruz));

      store(b, particles, at(b, kVelX, i), vx);
      store(b, particles, at(b, kVelY, i), vy);
      store(b, particles, at(b, kVelZ, i), vz);

      Stmt *moved = b.get_int32(0), *continued = b.get_int32(0);
      for (int pair : {0, 1, 2}) {
        const int pos_field = kPosX + pair;
        Stmt *v = pair == 0 ? vx : (pair == 1 ? vy : vz);
        auto *next = b.create_add(old_position[pair], b.create_mul(v, dt));
        store(b, pd, at(b, pair, i), b.create_sub(next, old_position[pair]));
        moved = b.create_logical_or(moved,
                                    b.create_cmp_ne(next, old_position[pair]));
        continued = b.create_logical_or(continued,
                                        b.create_cmp_ne(v, old_velocity[pair]));
        store(b, particles, at(b, pos_field, i), next);
      }
      store(b, ps, at(b, detail::Continued, i), b.get_int32(0));
      {
        auto changed = b.get_if_guard(
            b.create_if(b.create_logical_or(continued, moved)), true);
        store(b, ps, at(b, detail::Continued, i), b.get_int32(1));
      }
      auto *motion = b.create_if(moved);
      {
        auto moving = b.get_if_guard(motion, true);
        store(b, ps, at(b, detail::Moving, i), b.get_int32(1));
        count_work(b, counters, detail::Moved);
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
    integrate_ = engine::CompiledKernel(
        runtime, "field_integrate", b.extract_ir(),
        {engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::scalar(PrimitiveType::f32),
         engine::Param::array(PrimitiveType::f32),
         engine::Param::scalar(PrimitiveType::f32),
         engine::Param::array(PrimitiveType::i32, 2),
         engine::Param::array(PrimitiveType::f32, 2),
         engine::Param::array(PrimitiveType::f32),
         engine::Param::array(PrimitiveType::i64)},
        engine::MathMode::Strict);
  }

  // Determiners: the secondary set, running on the products of the primary
  // calculation. Speed for now; the stage exists to hold the shape.
  {
    IRBuilder b;
    auto *particles = array_param(b, 0, PrimitiveType::f32, /*total_dim=*/2);
    auto *out = array_param(b, 1, PrimitiveType::f32, /*total_dim=*/1);
    auto *n = scalar_param(b, 2, PrimitiveType::i32);
    auto *loop = b.create_range_for(b.get_int32(0), n, false, threads);
    {
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      auto *vx = load(b, particles, at(b, kVelX, i));
      auto *vy = load(b, particles, at(b, kVelY, i));
      auto *vz = load(b, particles, at(b, kVelZ, i));
      store(b, out, {i},
            b.create_sqrt(b.create_add(
                b.create_add(b.create_mul(vx, vx), b.create_mul(vy, vy)),
                b.create_mul(vz, vz))));
    }
    determine_ = engine::CompiledKernel(
        runtime, "field_determine", b.extract_ir(),
        {engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::array(PrimitiveType::f32),
         engine::Param::scalar(PrimitiveType::i32)},
        engine::MathMode::Strict);
  }

  // Centroid: the consolidation. A reduction over the same edge list the
  // force pass streamed, into per-group accumulators.
  {
    IRBuilder b;
    auto *edge_i = array_param(b, 0, PrimitiveType::i32, /*total_dim=*/2);
    auto *edge_f = array_param(b, 1, PrimitiveType::f32, /*total_dim=*/2);
    auto *particles = array_param(b, 2, PrimitiveType::f32, /*total_dim=*/2);
    auto *groups = array_param(b, 3, PrimitiveType::f32, /*total_dim=*/2);
    auto *n = scalar_param(b, 4, PrimitiveType::i32);
    auto *g = scalar_param(b, 5, PrimitiveType::i32);
    auto *e = scalar_param(b, 6, PrimitiveType::i32);
    auto *gs = array_param(b, 7, PrimitiveType::i32, 2);
    auto *counters = array_param(b, 8, PrimitiveType::i64, 1);
    auto *seed = scalar_param(b, 9, PrimitiveType::i32);
    auto *offsets = array_param(b, 10, PrimitiveType::i32, 2);
    auto *order = array_param(b, 11, PrimitiveType::i32, 1);
    (void)n;
    (void)g;
    (void)e;

    auto *loop = b.create_range_for(b.get_int32(0), g, false, threads);
    {
      auto guard = b.get_loop_guard(loop);
      auto *grp = b.get_loop_index(loop);
      auto active = b.get_if_guard(
          b.create_if(load(b, gs, at(b, detail::Flagged, grp))), true);
      // Seed around a member position rather than coordinate zero, preserving
      // the initial rounding residue that later small motions can combine with.
      {
        auto *begin = load(b, offsets, at(b, 0, grp));
        auto *end = load(b, offsets, at(b, 1, grp));
        auto initial = b.get_if_guard(b.create_if(b.create_logical_and(
                                          seed, b.create_cmp_lt(begin, end))),
                                      true);
        auto *first = load(b, order, {begin});
        auto *anchor = load(b, edge_i, at(b, kEdgeParticle, first));
        for (int axis = 0; axis < 3; ++axis)
          store(b, groups, at(b, kCenX + axis, grp),
                load(b, particles, at(b, kPosX + axis, anchor)));
      }
      auto *members = b.create_range_for(load(b, offsets, at(b, 0, grp)),
                                         load(b, offsets, at(b, 1, grp)), false,
                                         1, 0, true);
      auto member_guard = b.get_loop_guard(members);
      auto *k = load(b, order, {b.get_loop_index(members)});
      auto *p = load(b, edge_i, at(b, kEdgeParticle, k));
      auto *share = load(b, edge_f, at(b, kShare, k));
      count_work(b, counters, detail::ReducedEdges);
      auto *m = b.create_mul(load(b, particles, at(b, kMass, p)), share);
      for (int pair : {0, 1, 2}) {
        auto *pos = b.create_sub(load(b, particles, at(b, kPosX + pair, p)),
                                 load(b, groups, at(b, kCenX + pair, grp)));
        store(b, groups, at(b, kAccX + pair, grp),
              b.create_add(load(b, groups, at(b, kAccX + pair, grp)),
                           b.create_mul(m, pos)));
      }
      {
        auto initial = b.get_if_guard(b.create_if(seed), true);
        store(b, groups, at(b, kAccM, grp),
              b.create_add(load(b, groups, at(b, kAccM, grp)), m));
      }
    }
    centroid_ = engine::CompiledKernel(
        runtime, "field_centroid", b.extract_ir(),
        {engine::Param::array(PrimitiveType::i32, /*total_dim=*/2),
         engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::array(PrimitiveType::i32, 2),
         engine::Param::array(PrimitiveType::i64),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::array(PrimitiveType::i32, 2),
         engine::Param::array(PrimitiveType::i32)},
        engine::MathMode::Strict);
  }

  // Publish: divide the accumulators through and hand them to the next tick.
  {
    IRBuilder b;
    auto *groups = array_param(b, 0, PrimitiveType::f32, /*total_dim=*/2);
    auto *g = scalar_param(b, 1, PrimitiveType::i32);
    auto *gs = array_param(b, 2, PrimitiveType::i32, 2);
    auto *gd = array_param(b, 3, PrimitiveType::f32, 2);
    auto *counters = array_param(b, 4, PrimitiveType::i64, 1);
    {
      auto *loop = b.create_range_for(b.get_int32(0), g, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      store(b, gs, at(b, detail::Changed, i), b.get_int32(0));
      auto active = b.get_if_guard(
          b.create_if(load(b, gs, at(b, detail::Flagged, i))), true);
      count_work(b, counters, detail::Recomputed);
      auto *m = load(b, groups, at(b, kAccM, i));
      // Ordinary weighted mean: preserve every positive mass, substitute one
      // only for the exact empty-mass case.
      auto *denom =
          b.create_add(m, b.create_sub(b.get_float32(1), b.create_sgn(m)));
      Stmt *changed = b.get_int32(0);
      for (int pair : {0, 1, 2}) {
        auto *before = load(b, groups, at(b, kCenX + pair, i));
        auto *offset = load(b, groups, at(b, kAccX + pair, i));
        // Widen the final origin addition so a distant seed anchor cannot
        // introduce an extra f32 rounding before publishing the f32 centre.
        const auto wide = PrimitiveType::f64;
        auto *before_wide = b.create_cast(before, wide);
        auto *offset_wide = b.create_cast(offset, wide);
        auto *next = b.create_select(
            b.create_cmp_gt(m, b.get_float32(0)),
            b.create_cast(
                b.create_add(
                    before_wide,
                    b.create_div(offset_wide, b.create_cast(denom, wide))),
                PrimitiveType::f32),
            b.get_float32(0));
        store(b, gd, at(b, 3 + pair, i), before);
        auto *pending = b.create_cast(
            b.create_sub(offset_wide,
                         b.create_mul(b.create_sub(b.create_cast(next, wide),
                                                   before_wide),
                                      b.create_cast(m, wide))),
            PrimitiveType::f32);
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
    finalize_ = engine::CompiledKernel(
        runtime, "field_publish_centroids", b.extract_ir(),
        {engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::array(PrimitiveType::i32, 2),
         engine::Param::array(PrimitiveType::f32, 2),
         engine::Param::array(PrimitiveType::i64)},
        engine::MathMode::Strict);
  }
  timings_.compilation_ms = activity_->compilation_ms + elapsed(compilation_start);
}

template <class T>
void Harness::bind(LaunchContextBuilder &ctx, int argument,
                   const Ndarray *array, std::vector<T> &values,
                   std::vector<int> shape) {
  engine::PackedView<T> view(values, std::move(shape), generation_);
  if (path_ == HostPath::BulkCopy) {
    ctx.set_arg_ndarray({argument}, *array);
  } else {
    std::vector<taichi::int64> dimensions(view.shape().begin(),
                                          view.shape().end());
    ctx.set_arg_external_array_with_shape(
        {argument}, reinterpret_cast<std::uintptr_t>(view.data()), view.bytes(),
        dimensions);
  }
}

Harness::~Harness() = default;

// Activity launches share the exact same checked payload binding on both host
// paths; keep the template bodies private to this implementation.
template void Harness::bind<float>(LaunchContextBuilder &, int, const Ndarray *,
                                   std::vector<float> &, std::vector<int>);
template void Harness::bind<int>(LaunchContextBuilder &, int, const Ndarray *,
                                 std::vector<int> &, std::vector<int>);

void Harness::set_tick_mode(TickMode mode) {
  runtime_->synchronize();
  mode_ = mode;
  invalidated_ = true;
}

void Harness::set_focus(const std::vector<int> &groups) {
  if (!ready_) throw std::logic_error("focus requires seeded field geometry");
  std::set<int> distinct;
  for (int g : groups)
    if (g < 0 || g >= groups_ || !distinct.insert(g).second)
      throw std::invalid_argument(
          "focus requires distinct prepared field slots");
  if (!groups.empty()) {
    download();
    for (int g : groups)
      if (!(group(kCenM, g) > 0))
        throw std::invalid_argument(
            "focal fields require exposed participating mass");
    int anchor_slot = -1;
    for (int e = 0; e < edges_; ++e)
      if (edge_int(kEdgeGroup, e) == groups.front()) {
        anchor_slot = edge_int(kEdgeParticle, e);
        break;
      }
    if (anchor_slot < 0) throw std::invalid_argument("focal field has no member");
    FrameAnchor shift{}, next_anchor = frame_anchor_;
    for (int axis = 0; axis < 3; ++axis) {
      shift[axis] = particle(kPosX + axis, anchor_slot);
      next_anchor[axis] += shift[axis];
    }
    if (shift != FrameAnchor{}) {
      auto local = particle_data;
      for (int slot = 0; slot < particles_; ++slot) {
        if (particle(kMass, slot) == 0) continue;
        for (int axis = 0; axis < 3; ++axis) {
          const double value = double(particle(kPosX + axis, slot)) - shift[axis];
          if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<float>::max())
            throw std::invalid_argument("focus-local extent outside f32 range");
          local[offset(kPosX + axis, particles_, slot)] = static_cast<float>(value);
        }
      }
      activity_->set_focus(groups);
      std::copy(local.begin(), local.end(), particle_data.begin());
      publish(previous_parameters_, next_anchor);
      return;
    }
  }
  activity_->set_focus(groups);
  invalidated_ = true;
}

void Harness::validate_staging() const {
  if (particle_data.size() != storage_size(kParticleFieldCount, particles_) ||
      group_data.size() != storage_size(kGroupFieldCount, groups_) ||
      edge_float_data.size() != storage_size(kEdgeFloatFieldCount, edges_) ||
      edge_int_data.size() != storage_size(kEdgeIntFieldCount, edges_) ||
      contact_int_data.size() !=
          storage_size(kContactIntFieldCount, contacts_)) {
    throw std::invalid_argument(
        "field::Harness::upload: staging size mismatch");
  }
  for (const auto *data : {&particle_data, &group_data, &edge_float_data}) {
    for (float value : *data) {
      if (!std::isfinite(value)) {
        throw std::invalid_argument(
            "field::Harness::upload: non-finite staging");
      }
    }
  }
  for (int i = 0; i < particles_; ++i) {
    const float mass = particle_data[offset(kMass, particles_, i)];
    const float radius = particle_data[offset(kRadius, particles_, i)];
    if (mass < 0.0f || radius < 0.0f) {
      throw std::invalid_argument(
          "field::Harness::upload: negative mass or radius");
    }
    if (mass == 0.0f) {
      for (int f = 0; f < kParticleFieldCount; ++f) {
        if (particle_data[offset(f, particles_, i)] != 0.0f) {
          throw std::invalid_argument(
              "field::Harness::upload: free slot must be zero");
        }
      }
    }
  }
  // The kernels address device memory directly with these indices; an
  // out-of-range one would read or write past the array, not fail.
  for (int e = 0; e < edges_; ++e) {
    const int p = edge_int_data[offset(kEdgeParticle, edges_, e)];
    const int g = edge_int_data[offset(kEdgeGroup, edges_, e)];
    if (p < 0 || p >= particles_ || g < 0 || g >= groups_) {
      throw std::out_of_range("field::Harness::upload: edge " +
                              std::to_string(e) +
                              " names a particle or group out of range");
    }
    const float share = edge_float_data[offset(kShare, edges_, e)];
    if (particle_data[offset(kMass, particles_, p)] == 0.0f || share <= 0.0f ||
        share > 1.0f) {
      throw std::invalid_argument(
          "field::Harness::upload: invalid participation");
    }
  }
  for (int k = 0; k < contacts_; ++k) {
    const int a = contact_int_data[offset(kContactA, contacts_, k)];
    const int b = contact_int_data[offset(kContactB, contacts_, k)];
    if (a < 0 || a >= particles_ || b < 0 || b >= particles_) {
      throw std::out_of_range("field::Harness::upload: contact " +
                              std::to_string(k) +
                              " names a particle out of range");
    }
    if (a == b || particle_data[offset(kMass, particles_, a)] == 0.0f ||
        particle_data[offset(kMass, particles_, b)] == 0.0f) {
      throw std::invalid_argument(
          "field::Harness::upload: invalid contact endpoints");
    }
  }
}

void Harness::invalidate() {
  ready_ = false;
  uploaded_ = false;
  runtime_->synchronize();
  invalidated_ = true;
  demand_ = {};
}

void Harness::publish(const Parameters &p) {
  publish(p, frame_anchor_);
}

void Harness::publish(const Parameters &p, const FrameAnchor &anchor) {
  invalidate();
  validate_parameters(p);
  for (double coordinate : anchor)
    if (!std::isfinite(coordinate) || std::abs(coordinate) > std::numeric_limits<float>::max())
      throw std::invalid_argument("invalid frame reference");
  frame_anchor_ = anchor;
  upload();
  seed_centroids(p);
}

void Harness::upload() {
  invalidate();
  validate_staging();
  activity_->index(edge_int_data, contact_int_data);
  exposed_ = 0;
  for (int p = 0; p < particles_; ++p)
    if (particle(kMass, p) > 0) ++exposed_;
  ++generation_;
  const auto start = Clock::now();
  // Validate each complete typed shape before any copy; synchronous transfer
  // completion and exclusive harness ownership keep borrowed vectors stable.
  engine::PackedView<float> particles(
      particle_data, {kParticleFieldCount, std::max(particles_, 1)},
      generation_);
  engine::PackedView<float> groups(
      group_data, {kGroupFieldCount, std::max(groups_, 1)}, generation_);
  engine::PackedView<float> edge_f(edge_float_data,
                                   {kEdgeFloatFieldCount, std::max(edges_, 1)},
                                   generation_);
  engine::PackedView<int> edge_i(
      edge_int_data, {kEdgeIntFieldCount, std::max(edges_, 1)}, generation_);
  engine::PackedView<int> contacts(
      contact_int_data, {kContactIntFieldCount, std::max(contacts_, 1)},
      generation_);
  if (path_ == HostPath::BulkCopy) {
    engine::upload(*particle_array_, particle_data.data(),
                   particle_data.size() * sizeof(float));
    engine::upload(*group_array_, group_data.data(),
                   group_data.size() * sizeof(float));
    engine::upload(*edge_float_array_, edge_float_data.data(),
                   edge_float_data.size() * sizeof(float));
    engine::upload(*edge_int_array_, edge_int_data.data(),
                   edge_int_data.size() * sizeof(int));
    engine::upload(*contact_int_array_, contact_int_data.data(),
                   contact_int_data.size() * sizeof(int));
    runtime_->synchronize();
    timings_.upload_bytes += particles.bytes() + groups.bytes() +
                             edge_f.bytes() + edge_i.bytes() + contacts.bytes();
  }
  timings_.upload_ms += elapsed(start);
  uploaded_ = true;
}

void Harness::download() {
  const auto start = Clock::now();
  runtime_->synchronize();
  if (path_ == HostPath::BulkCopy) {
    engine::readback(*particle_array_, particle_data.data(),
                     particle_data.size() * sizeof(float));
    engine::readback(*group_array_, group_data.data(),
                     group_data.size() * sizeof(float));
    if (particles_ > 0) {
      engine::readback(*determined_array_, determined_.data(),
                       determined_.size() * sizeof(float));
    }
    timings_.download_bytes += (particle_data.size() + group_data.size() +
                                (particles_ > 0 ? determined_.size() : 0)) *
                               sizeof(float);
  }
  timings_.report_ms += elapsed(start);
}

bool Harness::uploaded_payload_matches() {
  if (!uploaded_ || ready_)
    throw std::logic_error("payload diagnostic requires unseeded upload");
  runtime_->synchronize();
  if (path_ == HostPath::CpuDirect)
    return true;  // Bound storage is the source.
  const auto matches = [](Ndarray &array, const auto &source) {
    using T = typename std::decay_t<decltype(source)>::value_type;
    std::vector<T> received(source.size());
    engine::readback(array, received.data(), received.size() * sizeof(T));
    return !std::memcmp(source.data(), received.data(),
                        received.size() * sizeof(T));
  };
  return matches(*particle_array_, particle_data) &&
         matches(*group_array_, group_data) &&
         matches(*edge_float_array_, edge_float_data) &&
         matches(*edge_int_array_, edge_int_data) &&
         matches(*contact_int_array_, contact_int_data);
}

void Harness::launch_clear(bool seed) {
  auto ctx = clear_.make_context();
  bind(ctx, 0, particle_array_.get(), particle_data,
       {kParticleFieldCount, std::max(particles_, 1)});
  bind(ctx, 1, group_array_.get(), group_data,
       {kGroupFieldCount, std::max(groups_, 1)});
  ctx.set_arg_int({2}, particles_);
  ctx.set_arg_int({3}, groups_);
  bind(ctx, 4, universal_array_.get(), universal_data, {1});
  ctx.set_arg_int({5}, seed);
  ctx.set_arg_ndarray({6}, *activity_->particle_flags);
  ctx.set_arg_int({7}, seed ? particles_ : 0);
  clear_.launch(ctx);
}

void Harness::launch_force(const Parameters &p) {
  auto ctx = force_.make_context();
  bind(ctx, 0, edge_int_array_.get(), edge_int_data,
       {kEdgeIntFieldCount, std::max(edges_, 1)});
  bind(ctx, 1, edge_float_array_.get(), edge_float_data,
       {kEdgeFloatFieldCount, std::max(edges_, 1)});
  bind(ctx, 2, particle_array_.get(), particle_data,
       {kParticleFieldCount, std::max(particles_, 1)});
  bind(ctx, 3, group_array_.get(), group_data,
       {kGroupFieldCount, std::max(groups_, 1)});
  ctx.set_arg_int({4}, particles_);
  ctx.set_arg_int({5}, groups_);
  ctx.set_arg_int({6}, edges_);
  ctx.set_arg_float({7}, p.gravity);
  ctx.set_arg_ndarray({8}, *activity_->particle_flags);
  ctx.set_arg_ndarray({9}, *activity_->counters);
  ctx.set_arg_ndarray({10}, *activity_->particle_offsets);
  ctx.set_arg_ndarray({11}, *activity_->particle_edges);
  force_.launch(ctx);
}

void Harness::launch_contact(const Parameters &p) {
  auto ctx = contact_.make_context();
  bind(ctx, 0, contact_int_array_.get(), contact_int_data,
       {kContactIntFieldCount, std::max(contacts_, 1)});
  bind(ctx, 1, particle_array_.get(), particle_data,
       {kParticleFieldCount, std::max(particles_, 1)});
  ctx.set_arg_int({2}, contacts_);
  ctx.set_arg_float({3}, p.contact_stiffness);
  ctx.set_arg_ndarray({4}, *activity_->particle_flags);
  ctx.set_arg_ndarray({5}, *activity_->counters);
  ctx.set_arg_ndarray({6}, *activity_->contact_a_offsets);
  ctx.set_arg_ndarray({7}, *activity_->contact_a_edges);
  ctx.set_arg_ndarray({8}, *activity_->contact_b_offsets);
  ctx.set_arg_ndarray({9}, *activity_->contact_b_edges);
  ctx.set_arg_int({10}, particles_);
  contact_.launch(ctx);
}

void Harness::launch_integrate(const Parameters &p) {
  auto ctx = integrate_.make_context();
  bind(ctx, 0, particle_array_.get(), particle_data,
       {kParticleFieldCount, std::max(particles_, 1)});
  ctx.set_arg_int({1}, particles_);
  ctx.set_arg_float({2}, p.dt);
  bind(ctx, 3, universal_array_.get(), universal_data, {1});
  ctx.set_arg_float({4}, p.reach_strength);
  ctx.set_arg_ndarray({5}, *activity_->particle_flags);
  ctx.set_arg_ndarray({6}, *activity_->particle_delta);
  ctx.set_arg_ndarray({7}, *activity_->focus);
  ctx.set_arg_ndarray({8}, *activity_->counters);
  integrate_.launch(ctx);
}

void Harness::launch_determine() {
  auto ctx = determine_.make_context();
  bind(ctx, 0, particle_array_.get(), particle_data,
       {kParticleFieldCount, std::max(particles_, 1)});
  bind(ctx, 1, determined_array_.get(), particles_ ? determined_ : empty_determined_,
       {std::max(particles_, 1)});
  ctx.set_arg_int({2}, particles_);
  determine_.launch(ctx);
}

void Harness::launch_centroid(const Parameters &, bool seed) {
  {
    auto ctx = centroid_.make_context();
    bind(ctx, 0, edge_int_array_.get(), edge_int_data,
         {kEdgeIntFieldCount, std::max(edges_, 1)});
    bind(ctx, 1, edge_float_array_.get(), edge_float_data,
         {kEdgeFloatFieldCount, std::max(edges_, 1)});
    bind(ctx, 2, particle_array_.get(), particle_data,
         {kParticleFieldCount, std::max(particles_, 1)});
    bind(ctx, 3, group_array_.get(), group_data,
         {kGroupFieldCount, std::max(groups_, 1)});
    ctx.set_arg_int({4}, particles_);
    ctx.set_arg_int({5}, groups_);
    ctx.set_arg_int({6}, edges_);
    ctx.set_arg_ndarray({7}, *activity_->group_flags);
    ctx.set_arg_ndarray({8}, *activity_->counters);
    ctx.set_arg_int({9}, seed);
    ctx.set_arg_ndarray({10}, *activity_->group_offsets);
    ctx.set_arg_ndarray({11}, *activity_->group_edges);
    centroid_.launch(ctx);
  }
  {
    auto ctx = finalize_.make_context();
    bind(ctx, 0, group_array_.get(), group_data,
         {kGroupFieldCount, std::max(groups_, 1)});
    ctx.set_arg_int({1}, groups_);
    ctx.set_arg_ndarray({2}, *activity_->group_flags);
    ctx.set_arg_ndarray({3}, *activity_->group_delta);
    ctx.set_arg_ndarray({4}, *activity_->counters);
    finalize_.launch(ctx);
  }
}

void Harness::seed_centroids(const Parameters &p) {
  const auto start = Clock::now();
  ready_ = false;
  if (!uploaded_) {
    throw std::logic_error(
        "field::Harness: upload required before centroid seeding");
  }
  validate_parameters(p);
  activity_->set_frame_anchor(frame_anchor_);
  activity_->reset();
  launch_clear(true);
  launch_centroid(p, true);
  launch_determine();
  runtime_->synchronize();
  ready_ = true;
  previous_parameters_ = p;
  invalidated_ = true;
  timings_.seed_ms += elapsed(start);
}

void Harness::tick(const Parameters &p) {
  if (!ready_) {
    throw std::logic_error(
        "field::Harness: successful publication required before tick");
  }
  validate_parameters(p);
  const auto start = std::chrono::steady_clock::now();
  const bool parameters_changed =
      p.gravity != previous_parameters_.gravity ||
      p.dt != previous_parameters_.dt ||
      p.contact_stiffness != previous_parameters_.contact_stiffness ||
      p.reach_strength != previous_parameters_.reach_strength;
  activity_->begin(*this, mode_ == TickMode::AllLoaded, invalidated_ || parameters_changed);
  activity_->touch(*this);
  launch_clear(false);
  launch_force(p);
  launch_contact(p);
  launch_integrate(p);
  launch_determine();
  activity_->mark(*this);
  launch_centroid(p, false);
  runtime_->synchronize();
  demand_ = activity_->report(exposed_);
  demand_.frame_anchor = frame_anchor_;
  demand_.tick_ms = std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - start)
                        .count();
  previous_parameters_ = p;
  invalidated_ = false;
  timings_.kernel_ms += elapsed(start);
}

}  // namespace field
