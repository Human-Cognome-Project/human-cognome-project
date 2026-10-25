#pragma once
#include "engine/runtime.h"
#include "field/numeric_ir.h"

namespace field::detail {
struct CentroidCoordinate {
  taichi::lang::Stmt *value, *residue, *trial;
};
// Shared strict reduction/finalization arithmetic for ticks and topology
// repair.
inline CentroidCoordinate centroid_coordinate(taichi::lang::IRBuilder &b,
                                              taichi::lang::Stmt *before,
                                              taichi::lang::Stmt *offset,
                                              taichi::lang::Stmt *mass,
                                              NumericalProfile profile = {}) {
  using namespace taichi::lang;
  if (profile.wide()) {
    auto *positive = b.create_cmp_gt(mass, b.get_float64(0));
    auto *denom = b.create_select(positive, mass, b.get_float64(1));
    auto *trial = b.create_select(
        positive, b.create_add(before, b.create_div(offset, denom)),
        b.get_float64(0));
    return {publish_position(b, trial, profile.field_fraction_bits),
            b.get_float64(0), trial};
  }
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
  return {next, pending, next};
}
}  // namespace field::detail
