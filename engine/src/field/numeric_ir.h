#pragma once
#include <limits>

#include "engine/precision.h"
#include "field/numerical_profile.h"
namespace field::detail {
inline taichi::lang::DataType real_type(bool wide) {
  return wide ? taichi::lang::PrimitiveType::f64
              : taichi::lang::PrimitiveType::f32;
}
inline taichi::lang::Stmt *real_constant(taichi::lang::IRBuilder &b, bool wide,
                                         double value) {
  return wide ? static_cast<taichi::lang::Stmt *>(b.get_float64(value))
              : b.get_float32(float(value));
}
inline taichi::lang::Stmt *source_ratio(taichi::lang::IRBuilder &b,
                                        taichi::lang::ArgLoadStmt *ratios,
                                        taichi::lang::Stmt *row, int stride) {
  const auto d = taichi::lang::PrimitiveType::f64;
  const auto load = [&](int column) {
    return b.create_cast(
        b.create_global_load(b.create_external_ptr(
            ratios, {b.create_add(row, b.get_int32(column * stride))})),
        d);
  };
  return b.create_div(load(0), load(1));
}
inline taichi::lang::Stmt *publish_position(taichi::lang::IRBuilder &b,
                                            taichi::lang::Stmt *x, int bits) {
  return engine::numeric::truncate_position(
             b, x,
             {engine::numeric::Real::F64, engine::numeric::Real::F64, bits})
      .value;
}
inline std::vector<engine::Param> ratio_parameters(
    std::initializer_list<engine::Param> p, bool wide) {
  std::vector<engine::Param> out(p);
  if (wide)
    out.push_back(engine::Param::array(taichi::lang::PrimitiveType::u64));
  return out;
}
inline taichi::lang::Stmt *supported_real(taichi::lang::IRBuilder &b,
                                          taichi::lang::Stmt *x) {
  auto *bits =
      b.create_and(b.create_bit_cast(x, taichi::lang::PrimitiveType::u64),
                   b.get_uint64(0x7fffffffffffffffULL));
  auto *exponent = b.create_and(bits, b.get_uint64(0x7ff0000000000000ULL));
  return b.create_logical_or(
      b.create_cmp_eq(bits, b.get_uint64(0)),
      b.create_logical_and(
          b.create_cmp_ne(exponent, b.get_uint64(0)),
          b.create_cmp_ne(exponent, b.get_uint64(0x7ff0000000000000ULL))));
}
inline void mark_nonfinite(taichi::lang::IRBuilder &b,
                           taichi::lang::ArgLoadStmt *c, taichi::lang::Stmt *x,
                           int index) {
  auto guard = b.get_if_guard(
      b.create_if(b.create_logical_not(supported_real(b, x))), true);
  b.create_atomic_add(b.create_external_ptr(c, {b.get_int32(index)}),
                      b.get_int64(1));
}
// Transient integrated motion may underflow before position publication. NaN
// and infinity remain errors; retained state is checked normal-or-zero below.
inline void mark_nonfinite_motion(taichi::lang::IRBuilder &b,
                                  taichi::lang::ArgLoadStmt *c,
                                  taichi::lang::Stmt *x, int index) {
  auto *exponent = b.create_and(
      b.create_bit_cast(x, taichi::lang::PrimitiveType::u64),
      b.get_uint64(0x7ff0000000000000ULL));
  auto guard = b.get_if_guard(b.create_if(b.create_cmp_eq(
      exponent, b.get_uint64(0x7ff0000000000000ULL))), true);
  b.create_atomic_add(b.create_external_ptr(c, {b.get_int32(index)}),
                      b.get_int64(1));
}
}  // namespace field::detail
