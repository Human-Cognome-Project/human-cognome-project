#pragma once
#include "engine/runtime.h"

namespace field::detail {
struct CentroidCoordinate {
  taichi::lang::Stmt *value, *residue;
};
// Shared strict reduction/finalization arithmetic for ticks and topology
// repair.
inline CentroidCoordinate centroid_coordinate(taichi::lang::IRBuilder &b,
                                              taichi::lang::Stmt *before,
                                              taichi::lang::Stmt *offset,
                                              taichi::lang::Stmt *mass) {
  using namespace taichi::lang;
  const auto wide = PrimitiveType::f64;
  auto *denom =
      b.create_add(mass, b.create_sub(b.get_float32(1), b.create_sgn(mass)));
  auto *anchor = b.create_cast(before, wide);
  auto *weighted = b.create_cast(offset, wide);
  auto *next = b.create_select(
      b.create_cmp_gt(mass, b.get_float32(0)),
      b.create_cast(
          b.create_add(anchor,
                       b.create_div(weighted, b.create_cast(denom, wide))),
          PrimitiveType::f32),
      b.get_float32(0));
  auto *pending = b.create_cast(
      b.create_sub(weighted,
                   b.create_mul(b.create_sub(b.create_cast(next, wide), anchor),
                                b.create_cast(mass, wide))),
      PrimitiveType::f32);
  return {next, pending};
}
}  // namespace field::detail
