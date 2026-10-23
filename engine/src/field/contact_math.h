#pragma once
#include <array>
#include "engine/runtime.h"

namespace field::detail {
struct ContactGeometry {
  taichi::lang::Stmt *squared, *live, *inverse, *distance, *radii, *penetration;
};
// The accepted f32 contact geometry, shared by the force law and coverage
// preflight. No tolerance/clearance or alternative force law is introduced.
inline ContactGeometry contact_geometry(
    taichi::lang::IRBuilder &b,
    const std::array<taichi::lang::Stmt *, 3> &delta,
    taichi::lang::Stmt *radius_a,
    taichi::lang::Stmt *radius_b) {
  auto *d2 = b.create_add(b.create_add(b.create_mul(delta[0], delta[0]),
                                       b.create_mul(delta[1], delta[1])),
                          b.create_mul(delta[2], delta[2]));
  auto *live = b.create_sgn(d2);
  auto *safe = b.create_add(d2, b.create_sub(b.get_float32(1), live));
  auto *inverse = b.create_rsqrt(safe);
  auto *distance = b.create_mul(safe, inverse);
  auto *radii = b.create_add(radius_a, radius_b);
  auto *penetration =
      b.create_max(b.create_sub(radii, distance), b.get_float32(0));
  return {d2, live, inverse, distance, radii, penetration};
}
}  // namespace field::detail
