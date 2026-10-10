#include "field/field.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

#include "engine/transfer.h"

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

void store(IRBuilder &b, ArgLoadStmt *array, const std::vector<Stmt *> &index,
          Stmt *value) {
  b.create_global_store(b.create_external_ptr(array, index), value);
}

void accumulate(IRBuilder &b, ArgLoadStmt *array, const std::vector<Stmt *> &index,
                Stmt *value) {
  b.create_atomic_add(b.create_external_ptr(array, index), value);
}

// total_dim is 2 for {field, item} dense arrays and 1 for universal/determined
// (single-axis, indexed directly).
ArgLoadStmt *array_param(IRBuilder &b, int id, DataType dt, int total_dim) {
  return b.create_ndarray_arg_load({id}, dt, total_dim, /*arg_depth=*/0);
}

Stmt *scalar_param(IRBuilder &b, int id, DataType dt) {
  return b.create_arg_load({id}, dt, /*is_ptr=*/false, /*arg_depth=*/0);
}

// Empty-group centroid guard; particle/total-mass guards preserve positive
// denominators exactly and substitute one only for a mass-zero free state.
constexpr float kWeightFloor = 1e-20f;

void validate_parameters(const Parameters &p) {
  for (float value : {p.gravity, p.dt, p.contact_stiffness, p.reach_strength}) {
    if (!std::isfinite(value) || value < 0.0f) {
      throw std::invalid_argument("field: parameters must be finite and non-negative");
    }
  }
}

std::size_t storage_size(int fields, int count) {
  return std::size_t(fields) * std::size_t(std::max(count, 1));
}

}  // namespace

Harness::Harness(engine::Runtime &runtime,
                 int particle_count,
                 int group_count,
                 int edge_count,
                 int contact_count)
    : runtime_(&runtime),
      particles_(particle_count),
      groups_(group_count),
      edges_(edge_count),
      contacts_(contact_count) {
  if (particle_count < 0 || group_count < 0 || edge_count < 0 || contact_count < 0) {
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
  determined_array_ = std::make_unique<Ndarray>(&program, PrimitiveType::f32,
                                                std::vector<int>{std::max(particles_, 1)});
  // The universal mass: one cell, the total mass of the construct with each
  // particle counted exactly once, recomputed by the clear pass each tick.
  universal_array_ = std::make_unique<Ndarray>(&program, PrimitiveType::f32,
                                               std::vector<int>{1});

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
    auto *zero = b.get_float32(0.0f);
    {
      auto *loop = b.create_range_for(b.get_int32(0), n, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      for (int f : {kForceX, kForceY, kForceZ}) {
        store(b, particles, at(b, f, i), zero);
      }
    }
    {
      auto *loop = b.create_range_for(b.get_int32(0), g, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      for (int f : {kAccX, kAccY, kAccZ, kAccM}) {
        store(b, groups, at(b, f, i), zero);
      }
    }
    {
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(1), false,
                                      threads);
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      store(b, universal, {i}, zero);
    }
    {
      auto *loop = b.create_range_for(b.get_int32(0), n, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      accumulate(b, universal, {b.get_int32(0)}, load(b, particles, at(b, kMass, i)));
    }
    clear_ = engine::CompiledKernel(
        runtime, "field_clear", b.extract_ir(),
        {engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::array(PrimitiveType::f32)});
  }

  // Force: one thread per membership edge. Identical work in every thread,
  // no loop and no branch.
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
    (void)n;
    (void)g;

    auto *loop = b.create_range_for(b.get_int32(0), e, false, threads);
    {
      auto guard = b.get_loop_guard(loop);
      auto *k = b.get_loop_index(loop);

      auto *p = load(b, edge_i, at(b, kEdgeParticle, k));
      auto *grp = load(b, edge_i, at(b, kEdgeGroup, k));
      auto *share = load(b, edge_f, at(b, kShare, k));

      auto *cx = load(b, particles, at(b, kPosX, p));
      auto *cy = load(b, particles, at(b, kPosY, p));
      auto *cz = load(b, particles, at(b, kPosZ, p));

      auto *gx = load(b, groups, at(b, kCenX, grp));
      auto *gy = load(b, groups, at(b, kCenY, grp));
      auto *gz = load(b, groups, at(b, kCenZ, grp));
      auto *gm = load(b, groups, at(b, kCenM, grp));

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

      auto *m1 = b.create_mul(load(b, particles, at(b, kMass, p)), share);

      // A group whose whole mass is this participation has nothing else in it
      // to pull with. Written as arithmetic rather than as a test: sgn of the
      // absolute difference is exactly zero when the masses match and one
      // otherwise, so every edge runs the same instructions. It is exact for
      // the sole-member case, where both sides are the same product.
      auto *engaged = b.create_sgn(b.create_abs(b.create_sub(gm, m1)));
      // The exact d==0 gate (live) drives the interaction to exactly zero at
      // coincidence -- zero force, not infinity.
      auto *scale = b.create_mul(
          b.create_mul(b.create_mul(gravity, m1), b.create_mul(gm, inv3)),
          b.create_mul(engaged, live));

      auto *fx = b.create_mul(scale, dx);
      auto *fy = b.create_mul(scale, dy);
      auto *fz = b.create_mul(scale, dz);
      accumulate(b, particles, at(b, kForceX, p), fx);
      accumulate(b, particles, at(b, kForceY, p), fy);
      accumulate(b, particles, at(b, kForceZ, p), fz);
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
         engine::Param::scalar(PrimitiveType::f32)});
  }

  // Finite ordinary contact, one thread per explicit pair. There is no
  // attraction above r_a+r_b and no directional choice at coincidence.
  {
    IRBuilder b;
    auto *pairs = array_param(b, 0, PrimitiveType::i32, /*total_dim=*/2);
    auto *particles = array_param(b, 1, PrimitiveType::f32, /*total_dim=*/2);
    auto *count = scalar_param(b, 2, PrimitiveType::i32);
    auto *stiffness = scalar_param(b, 3, PrimitiveType::f32);
    auto *loop = b.create_range_for(b.get_int32(0), count, false, threads);
    {
      auto guard = b.get_loop_guard(loop);
      auto *k = b.get_loop_index(loop);
      auto *a = load(b, pairs, at(b, kContactA, k));
      auto *bp = load(b, pairs, at(b, kContactB, k));
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
      auto *safe = b.create_add(d2, b.create_sub(b.get_float32(1.0f), live));
      auto *inv = b.create_rsqrt(safe);
      auto *d = b.create_mul(safe, inv);
      auto *sum_radii = b.create_add(load(b, particles, at(b, kRadius, a)),
                                     load(b, particles, at(b, kRadius, bp)));
      auto *penetration = b.create_max(b.create_sub(sum_radii, d),
                                       b.get_float32(0.0f));
      // Negative magnitude along A->B pushes A away and B oppositely.
      auto *scale = b.create_neg(b.create_mul(
          b.create_mul(stiffness, penetration), b.create_mul(live, inv)));
      for (int axis = 0; axis < 3; ++axis) {
        auto *delta = axis == 0 ? dx : (axis == 1 ? dy : dz);
        auto *force = b.create_mul(scale, delta);
        accumulate(b, particles, at(b, kForceX + axis, a), force);
        accumulate(b, particles, at(b, kForceX + axis, bp), b.create_neg(force));
      }
    }
    contact_ = engine::CompiledKernel(
        runtime, "field_contact", b.extract_ir(),
        {engine::Param::array(PrimitiveType::i32, /*total_dim=*/2),
         engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::scalar(PrimitiveType::f32)});
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

    auto *loop = b.create_range_for(b.get_int32(0), n, false, threads);
    {
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);

      auto *mass = load(b, particles, at(b, kMass, i));

      auto *fx = load(b, particles, at(b, kForceX, i));
      auto *fy = load(b, particles, at(b, kForceY, i));
      auto *fz = load(b, particles, at(b, kForceZ, i));

      // The whole mass absorbs the force the share raised.
      auto *mass_live = b.create_sgn(mass);
      auto *safe_mass = b.create_add(
          mass, b.create_sub(b.get_float32(1.0f), mass_live));
      auto *step_over_mass = b.create_div(b.create_mul(dt, mass_live), safe_mass);
      auto *vx = b.create_add(load(b, particles, at(b, kVelX, i)),
                              b.create_mul(fx, step_over_mass));
      auto *vy = b.create_add(load(b, particles, at(b, kVelY, i)),
                              b.create_mul(fy, step_over_mass));
      auto *vz = b.create_add(load(b, particles, at(b, kVelZ, i)),
                              b.create_mul(fz, step_over_mass));

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
      auto *px = load(b, particles, at(b, kPosX, i));
      auto *py = load(b, particles, at(b, kPosY, i));
      auto *pz = load(b, particles, at(b, kPosZ, i));
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

      for (int pair : {0, 1, 2}) {
        const int pos_field = kPosX + pair;
        Stmt *v = pair == 0 ? vx : (pair == 1 ? vy : vz);
        store(b, particles, at(b, pos_field, i),
              b.create_add(load(b, particles, at(b, pos_field, i)),
                           b.create_mul(v, dt)));
      }
    }
    integrate_ = engine::CompiledKernel(
        runtime, "field_integrate", b.extract_ir(),
        {engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::scalar(PrimitiveType::f32),
         engine::Param::array(PrimitiveType::f32),
         engine::Param::scalar(PrimitiveType::f32)});
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
         engine::Param::scalar(PrimitiveType::i32)});
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
    (void)n;
    (void)g;

    auto *loop = b.create_range_for(b.get_int32(0), e, false, threads);
    {
      auto guard = b.get_loop_guard(loop);
      auto *k = b.get_loop_index(loop);
      auto *p = load(b, edge_i, at(b, kEdgeParticle, k));
      auto *grp = load(b, edge_i, at(b, kEdgeGroup, k));
      auto *share = load(b, edge_f, at(b, kShare, k));
      auto *m = b.create_mul(load(b, particles, at(b, kMass, p)), share);
      for (int pair : {0, 1, 2}) {
        auto *pos = load(b, particles, at(b, kPosX + pair, p));
        accumulate(b, groups, at(b, kAccX + pair, grp),
                   b.create_mul(m, pos));
      }
      accumulate(b, groups, at(b, kAccM, grp), m);
    }
    centroid_ = engine::CompiledKernel(
        runtime, "field_centroid", b.extract_ir(),
        {engine::Param::array(PrimitiveType::i32, /*total_dim=*/2),
         engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::scalar(PrimitiveType::i32)});
  }

  // Publish: divide the accumulators through and hand them to the next tick.
  {
    IRBuilder b;
    auto *groups = array_param(b, 0, PrimitiveType::f32, /*total_dim=*/2);
    auto *g = scalar_param(b, 1, PrimitiveType::i32);
    auto *floor = b.get_float32(kWeightFloor);
    {
      auto *loop = b.create_range_for(b.get_int32(0), g, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      auto *m = load(b, groups, at(b, kAccM, i));
      auto *denom = b.create_add(m, floor);
      for (int pair : {0, 1, 2}) {
        store(b, groups, at(b, kCenX + pair, i),
              b.create_div(load(b, groups, at(b, kAccX + pair, i)), denom));
      }
      store(b, groups, at(b, kCenM, i), m);
    }
    finalize_ = engine::CompiledKernel(
        runtime, "field_publish_centroids", b.extract_ir(),
        {engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::scalar(PrimitiveType::i32)});
  }
}

void Harness::validate_staging() const {
  if (particle_data.size() != storage_size(kParticleFieldCount, particles_) ||
      group_data.size() != storage_size(kGroupFieldCount, groups_) ||
      edge_float_data.size() != storage_size(kEdgeFloatFieldCount, edges_) ||
      edge_int_data.size() != storage_size(kEdgeIntFieldCount, edges_) ||
      contact_int_data.size() != storage_size(kContactIntFieldCount, contacts_)) {
    throw std::invalid_argument("field::Harness::upload: staging size mismatch");
  }
  for (const auto *data : {&particle_data, &group_data, &edge_float_data}) {
    for (float value : *data) {
      if (!std::isfinite(value)) {
        throw std::invalid_argument("field::Harness::upload: non-finite staging");
      }
    }
  }
  for (int i = 0; i < particles_; ++i) {
    const float mass = particle_data[offset(kMass, particles_, i)];
    const float radius = particle_data[offset(kRadius, particles_, i)];
    if (mass < 0.0f || radius < 0.0f) {
      throw std::invalid_argument("field::Harness::upload: negative mass or radius");
    }
    if (mass == 0.0f) {
      for (int f = 0; f < kParticleFieldCount; ++f) {
        if (particle_data[offset(f, particles_, i)] != 0.0f) {
          throw std::invalid_argument("field::Harness::upload: free slot must be zero");
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
      throw std::out_of_range("field::Harness::upload: edge " + std::to_string(e) +
                              " names a particle or group out of range");
    }
    const float share = edge_float_data[offset(kShare, edges_, e)];
    if (particle_data[offset(kMass, particles_, p)] == 0.0f ||
        share <= 0.0f || share > 1.0f) {
      throw std::invalid_argument("field::Harness::upload: invalid participation");
    }
  }
  for (int k = 0; k < contacts_; ++k) {
    const int a = contact_int_data[offset(kContactA, contacts_, k)];
    const int b = contact_int_data[offset(kContactB, contacts_, k)];
    if (a < 0 || a >= particles_ || b < 0 || b >= particles_) {
      throw std::out_of_range("field::Harness::upload: contact " + std::to_string(k) +
                              " names a particle out of range");
    }
    if (a == b || particle_data[offset(kMass, particles_, a)] == 0.0f ||
        particle_data[offset(kMass, particles_, b)] == 0.0f) {
      throw std::invalid_argument("field::Harness::upload: invalid contact endpoints");
    }
  }
}

void Harness::invalidate() {
  ready_ = false;
  uploaded_ = false;
  runtime_->synchronize();
}

void Harness::publish(const Parameters &p) {
  invalidate();
  validate_parameters(p);
  upload();
  seed_centroids(p);
}

void Harness::upload() {
  invalidate();
  validate_staging();
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
  uploaded_ = true;
}

void Harness::download() {
  runtime_->synchronize();
  engine::readback(*particle_array_, particle_data.data(),
                   particle_data.size() * sizeof(float));
  engine::readback(*group_array_, group_data.data(),
                   group_data.size() * sizeof(float));
  if (particles_ > 0) {
    engine::readback(*determined_array_, determined_.data(),
                     determined_.size() * sizeof(float));
  }
}

void Harness::launch_clear() {
  auto ctx = clear_.make_context();
  ctx.set_arg_ndarray({0}, *particle_array_);
  ctx.set_arg_ndarray({1}, *group_array_);
  ctx.set_arg_int({2}, particles_);
  ctx.set_arg_int({3}, groups_);
  ctx.set_arg_ndarray({4}, *universal_array_);
  clear_.launch(ctx);
}

void Harness::launch_force(const Parameters &p) {
  auto ctx = force_.make_context();
  ctx.set_arg_ndarray({0}, *edge_int_array_);
  ctx.set_arg_ndarray({1}, *edge_float_array_);
  ctx.set_arg_ndarray({2}, *particle_array_);
  ctx.set_arg_ndarray({3}, *group_array_);
  ctx.set_arg_int({4}, particles_);
  ctx.set_arg_int({5}, groups_);
  ctx.set_arg_int({6}, edges_);
  ctx.set_arg_float({7}, p.gravity);
  force_.launch(ctx);
}

void Harness::launch_contact(const Parameters &p) {
  auto ctx = contact_.make_context();
  ctx.set_arg_ndarray({0}, *contact_int_array_);
  ctx.set_arg_ndarray({1}, *particle_array_);
  ctx.set_arg_int({2}, contacts_);
  ctx.set_arg_float({3}, p.contact_stiffness);
  contact_.launch(ctx);
}

void Harness::launch_integrate(const Parameters &p) {
  auto ctx = integrate_.make_context();
  ctx.set_arg_ndarray({0}, *particle_array_);
  ctx.set_arg_int({1}, particles_);
  ctx.set_arg_float({2}, p.dt);
  ctx.set_arg_ndarray({3}, *universal_array_);
  ctx.set_arg_float({4}, p.reach_strength);
  integrate_.launch(ctx);
}

void Harness::launch_determine() {
  auto ctx = determine_.make_context();
  ctx.set_arg_ndarray({0}, *particle_array_);
  ctx.set_arg_ndarray({1}, *determined_array_);
  ctx.set_arg_int({2}, particles_);
  determine_.launch(ctx);
}

void Harness::launch_centroid(const Parameters &) {
  {
    auto ctx = centroid_.make_context();
    ctx.set_arg_ndarray({0}, *edge_int_array_);
    ctx.set_arg_ndarray({1}, *edge_float_array_);
    ctx.set_arg_ndarray({2}, *particle_array_);
    ctx.set_arg_ndarray({3}, *group_array_);
    ctx.set_arg_int({4}, particles_);
    ctx.set_arg_int({5}, groups_);
    ctx.set_arg_int({6}, edges_);
    centroid_.launch(ctx);
  }
  {
    auto ctx = finalize_.make_context();
    ctx.set_arg_ndarray({0}, *group_array_);
    ctx.set_arg_int({1}, groups_);
    finalize_.launch(ctx);
  }
}

void Harness::seed_centroids(const Parameters &p) {
  ready_ = false;
  if (!uploaded_) {
    throw std::logic_error("field::Harness: upload required before centroid seeding");
  }
  validate_parameters(p);
  launch_clear();
  launch_centroid(p);
  launch_determine();
  runtime_->synchronize();
  ready_ = true;
}

void Harness::tick(const Parameters &p) {
  if (!ready_) {
    throw std::logic_error("field::Harness: successful publication required before tick");
  }
  validate_parameters(p);
  launch_clear();
  launch_force(p);
  launch_contact(p);
  launch_integrate(p);
  launch_determine();
  launch_centroid(p);
}

}  // namespace field
