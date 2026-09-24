#include "field/field.h"

#include <algorithm>

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

// total_dim is 2 for the six {field, item} dense arrays (particle, group,
// edge_float, edge_int, bond_float, bond_int), and 1 for universal/determined
// (single-axis, indexed directly).
ArgLoadStmt *array_param(IRBuilder &b, int id, DataType dt, int total_dim) {
  return b.create_ndarray_arg_load({id}, dt, total_dim, /*arg_depth=*/0);
}

Stmt *scalar_param(IRBuilder &b, int id, DataType dt) {
  return b.create_arg_load({id}, dt, /*is_ptr=*/false, /*arg_depth=*/0);
}

// Denominator guard applied to every division, never to some of them.
constexpr float kWeightFloor = 1e-20f;

}  // namespace

Harness::Harness(engine::Runtime &runtime,
                 int particle_count,
                 int group_count,
                 int edge_count,
                 int bond_count)
    : runtime_(&runtime),
      particles_(particle_count),
      groups_(group_count),
      edges_(edge_count),
      bonds_(bond_count) {
  particle_data.assign(std::size_t(particles_) * kParticleFieldCount, 0.0f);
  group_data.assign(std::size_t(groups_) * kGroupFieldCount, 0.0f);
  edge_float_data.assign(std::size_t(edges_) * kEdgeFloatFieldCount, 0.0f);
  edge_int_data.assign(std::size_t(edges_) * kEdgeIntFieldCount, 0);
  // Sized the same as the size-safe device arrays below (max(bonds_, 1)):
  // a bonds_==0 upload of a zero-length host vector to that one-element
  // device array is a 0-byte transfer, which the RHI layer rejects. The
  // stride used by bond_float()/bond_int() is still the real bonds_, so a
  // padded element is simply never addressed.
  bond_float_data.assign(std::size_t(std::max(bonds_, 1)) * kBondFloatFieldCount,
                         0.0f);
  bond_int_data.assign(std::size_t(std::max(bonds_, 1)) * kBondIntFieldCount, 0);
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
      std::vector<int>{kParticleFieldCount, particles_});
  group_array_ = std::make_unique<Ndarray>(
      &program, PrimitiveType::f32,
      std::vector<int>{kGroupFieldCount, groups_});
  edge_float_array_ = std::make_unique<Ndarray>(
      &program, PrimitiveType::f32,
      std::vector<int>{kEdgeFloatFieldCount, edges_});
  edge_int_array_ = std::make_unique<Ndarray>(
      &program, PrimitiveType::i32,
      std::vector<int>{kEdgeIntFieldCount, edges_});
  // Guard bonds_==0 with a size-safe allocation of at least one element; the
  // kernel loop over zero bonds is a no-op either way.
  bond_float_array_ = std::make_unique<Ndarray>(
      &program, PrimitiveType::f32,
      std::vector<int>{kBondFloatFieldCount, std::max(bonds_, 1)});
  bond_int_array_ = std::make_unique<Ndarray>(
      &program, PrimitiveType::i32,
      std::vector<int>{kBondIntFieldCount, std::max(bonds_, 1)});
  determined_array_ = std::make_unique<Ndarray>(&program, PrimitiveType::f32,
                                                std::vector<int>{particles_});
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
      auto *off_x = load(b, edge_f, at(b, kOffX, k));
      auto *off_y = load(b, edge_f, at(b, kOffY, k));
      auto *off_z = load(b, edge_f, at(b, kOffZ, k));

      auto *cx = load(b, particles, at(b, kPosX, p));
      auto *cy = load(b, particles, at(b, kPosY, p));
      auto *cz = load(b, particles, at(b, kPosZ, p));

      // The share acts where it sits, not at the particle's centre.
      auto *sx = b.create_add(cx, off_x);
      auto *sy = b.create_add(cy, off_y);
      auto *sz = b.create_add(cz, off_z);

      auto *gx = load(b, groups, at(b, kCenX, grp));
      auto *gy = load(b, groups, at(b, kCenY, grp));
      auto *gz = load(b, groups, at(b, kCenZ, grp));
      auto *gm = load(b, groups, at(b, kCenM, grp));

      auto *dx = b.create_sub(gx, sx);
      auto *dy = b.create_sub(gy, sy);
      auto *dz = b.create_sub(gz, sz);

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

  // Contact: one thread per bond edge. This is the corona SEPARATOR, not a
  // second attraction -- the field kernel above already supplies the
  // like-attraction, and superposition does the rest. Here the corona is
  // the regime-gated standoff a bonded pair is held at: aligned or defined
  // bonds stand off flush (d0 == 1.0, unit spheres in flush contact);
  // opposed bonds stand off at the corona distance (d0 == 1.0 + 2*corona,
  // coronas abutting rather than overlapping). The separator resists the
  // pair closing inside d0 -- a finite push apart, breakable by stronger
  // surrounding forces, never a hard constraint. A defined bond additionally
  // carries the invariant hold: resist the pair being pulled apart past d0,
  // a pull back together, which is what makes a defined bond shear-
  // resistant. An opposed or aligned bond has no such hold and can be
  // pulled free. No attraction is added here.
  {
    IRBuilder b;
    auto *bond_i = array_param(b, 0, PrimitiveType::i32, /*total_dim=*/2);
    auto *bond_f = array_param(b, 1, PrimitiveType::f32, /*total_dim=*/2);
    auto *particles = array_param(b, 2, PrimitiveType::f32, /*total_dim=*/2);
    auto *n = scalar_param(b, 3, PrimitiveType::i32);
    auto *bcount = scalar_param(b, 4, PrimitiveType::i32);
    auto *corona = scalar_param(b, 5, PrimitiveType::f32);
    auto *sep_stiffness = scalar_param(b, 6, PrimitiveType::f32);
    auto *hold_stiffness = scalar_param(b, 7, PrimitiveType::f32);
    (void)n;

    auto *loop = b.create_range_for(b.get_int32(0), bcount, false, threads);
    {
      auto guard = b.get_loop_guard(loop);
      auto *k = b.get_loop_index(loop);

      auto *a = load(b, bond_i, at(b, kBondA, k));
      auto *bp = load(b, bond_i, at(b, kBondB, k));
      auto *defined = load(b, bond_i, at(b, kBondDefined, k));
      auto *align = load(b, bond_f, at(b, kBondAlign, k));

      auto *ax = load(b, particles, at(b, kPosX, a));
      auto *ay = load(b, particles, at(b, kPosY, a));
      auto *az = load(b, particles, at(b, kPosZ, a));
      auto *bx = load(b, particles, at(b, kPosX, bp));
      auto *by = load(b, particles, at(b, kPosY, bp));
      auto *bz = load(b, particles, at(b, kPosZ, bp));

      auto *dx = b.create_sub(bx, ax);
      auto *dy = b.create_sub(by, ay);
      auto *dz = b.create_sub(bz, az);

      // Exact squared separation, the same exact d==0 gate the force kernel
      // uses: zero contact force at coincidence, never a softened law.
      auto *d2 = b.create_add(
          b.create_add(b.create_mul(dx, dx), b.create_mul(dy, dy)),
          b.create_mul(dz, dz));
      auto *live = b.create_sgn(d2);
      auto *safe =
          b.create_add(d2, b.create_sub(b.get_float32(1.0f), live));
      auto *inv = b.create_rsqrt(safe);
      // Distance: safe * inv == sqrt(d2) for d2>0 (and == 1 at d2==0, where
      // live gates every downstream term to zero anyway).
      auto *d = b.create_mul(safe, inv);

      // Unit direction from a to b.
      auto *ux = b.create_mul(dx, inv);
      auto *uy = b.create_mul(dy, inv);
      auto *uz = b.create_mul(dz, inv);

      // Regime, branchless: abut (not defined, not aligned -- the opposed
      // case) is the only regime whose standoff is not flush.
      auto *defined_f = b.create_cast(defined, PrimitiveType::f32);
      auto *aligned = b.create_max(b.create_sgn(align), b.get_float32(0.0f));
      auto *abut = b.create_mul(b.create_sub(b.get_float32(1.0f), defined_f),
                                b.create_sub(b.get_float32(1.0f), aligned));
      auto *d0 = b.create_add(
          b.get_float32(1.0f),
          b.create_mul(b.create_mul(b.get_float32(2.0f), corona), abut));

      // Separator: a finite push apart while the pair is closer than d0.
      auto *pen = b.create_max(b.create_sub(d0, d), b.get_float32(0.0f));
      auto *sep = b.create_mul(b.create_mul(sep_stiffness, pen), live);

      // Invariant hold: defined bonds only, a pull back together while the
      // pair has stretched past d0.
      auto *stretch = b.create_max(b.create_sub(d, d0), b.get_float32(0.0f));
      auto *hold = b.create_mul(
          b.create_mul(b.create_mul(hold_stiffness, stretch), defined_f),
          live);

      // Net radial magnitude toward b: hold pulls together, sep pushes
      // apart.
      auto *mag = b.create_sub(hold, sep);
      auto *fx = b.create_mul(ux, mag);
      auto *fy = b.create_mul(uy, mag);
      auto *fz = b.create_mul(uz, mag);
      // Equal and opposite onto the two endpoints, so this composes by
      // superposition with the field force already accumulated there. A
      // particle may sit in more than one bond, so this is an atomic
      // accumulation, not a plain store.
      accumulate(b, particles, at(b, kForceX, a), fx);
      accumulate(b, particles, at(b, kForceY, a), fy);
      accumulate(b, particles, at(b, kForceZ, a), fz);
      accumulate(b, particles, at(b, kForceX, bp), b.create_neg(fx));
      accumulate(b, particles, at(b, kForceY, bp), b.create_neg(fy));
      accumulate(b, particles, at(b, kForceZ, bp), b.create_neg(fz));
    }
    contact_ = engine::CompiledKernel(
        runtime, "field_contact", b.extract_ir(),
        {engine::Param::array(PrimitiveType::i32, /*total_dim=*/2),
         engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::scalar(PrimitiveType::i32),
         engine::Param::scalar(PrimitiveType::f32),
         engine::Param::scalar(PrimitiveType::f32),
         engine::Param::scalar(PrimitiveType::f32)});
  }

  // Universal: a virtual particle pegged at the origin by definition -- its
  // position is never computed -- carrying the aggregation of the centroid
  // masses already published (kCenM summed into the single universal cell).
  // Every particle is pulled toward it by the same one law, m1*m2/d^2, with
  // the origin as target: it is one more valid centroid in the particle's
  // calculation, just one whose position needs no lookup. One thread per
  // particle.
  //
  // SUSPENDED, not removed: tick() no longer calls launch_universal, so this
  // inward pull does not run. It is kept intact -- kernel, launcher, and the
  // universal-mass sum in field_clear that feeds it -- because it may be
  // re-activated later. Runaway kinetics are now bounded instead by the
  // radial velocity dampening in integrate, which reads the same universal
  // mass cell for its reach scale.
  {
    IRBuilder b;
    auto *particles = array_param(b, 0, PrimitiveType::f32, /*total_dim=*/2);
    auto *universal = array_param(b, 1, PrimitiveType::f32, /*total_dim=*/1);
    auto *n = scalar_param(b, 2, PrimitiveType::i32);
    auto *gravity = scalar_param(b, 3, PrimitiveType::f32);

    auto *loop = b.create_range_for(b.get_int32(0), n, false, threads);
    {
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);

      auto *px = load(b, particles, at(b, kPosX, i));
      auto *py = load(b, particles, at(b, kPosY, i));
      auto *pz = load(b, particles, at(b, kPosZ, i));
      auto *mass = load(b, particles, at(b, kMass, i));

      // Exact squared distance to the origin, the same exact d==0 gate the
      // force kernel uses: zero pull at the origin, never a softened law.
      auto *d2 = b.create_add(
          b.create_add(b.create_mul(px, px), b.create_mul(py, py)),
          b.create_mul(pz, pz));
      auto *live = b.create_sgn(d2);
      auto *safe =
          b.create_add(d2, b.create_sub(b.get_float32(1.0f), live));
      auto *inv = b.create_rsqrt(safe);
      auto *inv3 = b.create_mul(inv, b.create_mul(inv, inv));

      auto *m_universal = load(b, universal, {b.get_int32(0)});

      auto *scale = b.create_mul(
          b.create_mul(b.create_mul(gravity, mass), m_universal),
          b.create_mul(inv3, live));

      // Pull toward the origin, i.e. along (0 - pos): composes by
      // superposition with the field and contact forces already
      // accumulated this tick.
      accumulate(b, particles, at(b, kForceX, i),
                b.create_neg(b.create_mul(scale, px)));
      accumulate(b, particles, at(b, kForceY, i),
                b.create_neg(b.create_mul(scale, py)));
      accumulate(b, particles, at(b, kForceZ, i),
                b.create_neg(b.create_mul(scale, pz)));
    }
    universal_ = engine::CompiledKernel(
        runtime, "field_universal", b.extract_ir(),
        {engine::Param::array(PrimitiveType::f32, /*total_dim=*/2),
         engine::Param::array(PrimitiveType::f32),
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
      auto *step_over_mass = b.create_div(dt, mass);
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
      // reach_ratio == reach_strength / (M/d2) == threshold / field-strength.
      auto *reach_ratio =
          b.create_div(b.create_mul(reach_strength, pd2), m_universal);
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
        auto *pos = b.create_add(
            load(b, particles, at(b, kPosX + pair, p)),
            load(b, edge_f, at(b, kOffX + pair, k)));
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

void Harness::upload() {
  engine::upload(*particle_array_, particle_data.data(),
                 particle_data.size() * sizeof(float));
  engine::upload(*group_array_, group_data.data(),
                 group_data.size() * sizeof(float));
  engine::upload(*edge_float_array_, edge_float_data.data(),
                 edge_float_data.size() * sizeof(float));
  engine::upload(*edge_int_array_, edge_int_data.data(),
                 edge_int_data.size() * sizeof(int));
  engine::upload(*bond_float_array_, bond_float_data.data(),
                 bond_float_data.size() * sizeof(float));
  engine::upload(*bond_int_array_, bond_int_data.data(),
                 bond_int_data.size() * sizeof(int));
}

void Harness::download() {
  runtime_->synchronize();
  engine::readback(*particle_array_, particle_data.data(),
                   particle_data.size() * sizeof(float));
  engine::readback(*group_array_, group_data.data(),
                   group_data.size() * sizeof(float));
  engine::readback(*determined_array_, determined_.data(),
                   determined_.size() * sizeof(float));
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
  ctx.set_arg_ndarray({0}, *bond_int_array_);
  ctx.set_arg_ndarray({1}, *bond_float_array_);
  ctx.set_arg_ndarray({2}, *particle_array_);
  ctx.set_arg_int({3}, particles_);
  ctx.set_arg_int({4}, bonds_);
  ctx.set_arg_float({5}, p.corona);
  ctx.set_arg_float({6}, p.sep_stiffness);
  ctx.set_arg_float({7}, p.hold_stiffness);
  contact_.launch(ctx);
}

void Harness::launch_universal(const Parameters &p) {
  auto ctx = universal_.make_context();
  ctx.set_arg_ndarray({0}, *particle_array_);
  ctx.set_arg_ndarray({1}, *universal_array_);
  ctx.set_arg_int({2}, particles_);
  ctx.set_arg_float({3}, p.gravity);
  universal_.launch(ctx);
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
  launch_clear();
  launch_centroid(p);
  runtime_->synchronize();
}

void Harness::tick(const Parameters &p) {
  launch_clear();
  launch_force(p);
  launch_contact(p);
  // universal inward pull suspended (not removed) -- replaced by the radial
  // velocity dampening in integrate; may be re-activated later.
  launch_integrate(p);
  launch_determine();
  launch_centroid(p);
}

}  // namespace field
