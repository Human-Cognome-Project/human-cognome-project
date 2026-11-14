#pragma once
#include "field/numeric_ir.h"
#include <array>

namespace field::detail {
// Strict diagnostic arithmetic shared by the packet probe and live owner.
// No force/activity writes; unsupported arithmetic remains sticky.
struct IntentAccumulator {
  using Stmt = taichi::lang::Stmt;
  using Local = taichi::lang::AllocaStmt;
  taichi::lang::IRBuilder &b;
  Local *bad, *coincident, *weight;
  std::array<Local *, 3> force{};
  explicit IntentAccumulator(taichi::lang::IRBuilder &builder) : b(builder) {
    using namespace taichi::lang;
    bad = variable(PrimitiveType::i32, b.get_int32(0));
    coincident = variable(PrimitiveType::i32, b.get_int32(0));
    weight = variable(PrimitiveType::f64, b.get_float64(0));
    for (auto &v : force) v = variable(PrimitiveType::f64, b.get_float64(0));
  }
  Local *variable(taichi::lang::DataType type, Stmt *initial) {
    auto *v = b.create_local_var(type); b.create_local_store(v, initial); return v;
  }
  void fail(Stmt *condition) {
    b.create_local_store(bad, b.create_logical_or(b.create_local_load(bad), condition));
  }
  Stmt *checked(Stmt *v) {
    auto *ok = supported_real(b, v);
    fail(b.create_logical_not(ok));
    return b.create_select(ok, v, b.get_float64(0));
  }
  Stmt *mul(Stmt *a, Stmt *c) {
    auto *v = b.create_mul(a, c);
    fail(b.create_logical_and(b.create_logical_and(
        b.create_cmp_ne(a, b.get_float64(0)), b.create_cmp_ne(c, b.get_float64(0))),
        b.create_cmp_eq(v, b.get_float64(0))));
    return checked(v);
  }
  Stmt *add(Stmt *a, Stmt *c) {
    auto *v = b.create_add(a, c);
    fail(b.create_logical_and(b.create_cmp_eq(v, b.get_float64(0)),
                              b.create_cmp_ne(a, b.create_neg(c))));
    return checked(v);
  }
  Stmt *divide(Stmt *a, Stmt *c) {
    auto *v = b.create_div(a, c);
    fail(b.create_logical_and(b.create_cmp_ne(a, b.get_float64(0)),
                              b.create_cmp_eq(v, b.get_float64(0))));
    return checked(v);
  }
  void append(std::array<Stmt *, 3> d, Stmt *coupling) {
    for (auto &v : d) v = checked(v);
    coupling = checked(coupling);
    fail(b.create_cmp_lt(coupling, b.get_float64(0)));
    auto active = b.get_if_guard(b.create_if(b.create_cmp_gt(coupling, b.get_float64(0))), true);
    auto *zero = b.create_logical_and(b.create_cmp_eq(d[0], b.get_float64(0)),
        b.create_logical_and(b.create_cmp_eq(d[1], b.get_float64(0)),
                            b.create_cmp_eq(d[2], b.get_float64(0))));
    {
      auto exact = b.get_if_guard(b.create_if(zero), true);
      b.create_local_store(coincident, b.create_add(b.create_local_load(coincident), b.get_int32(1)));
    }
    {
      auto separated = b.get_if_guard(b.create_if(b.create_logical_not(zero)), true);
      Stmt *d2 = b.get_float64(0);
      for (auto *v : d) d2 = add(d2, mul(v, v));
      auto *positive = b.create_cmp_gt(d2, b.get_float64(0));
      fail(b.create_logical_not(positive));
      auto *safe = b.create_select(positive, d2, b.get_float64(1));
      auto *distance = checked(b.create_sqrt(safe));
      auto *cube = mul(safe, distance);
      auto *cube_positive = b.create_cmp_gt(cube, b.get_float64(0));
      fail(b.create_logical_not(cube_positive));
      auto *alpha = divide(coupling, b.create_select(cube_positive, cube, b.get_float64(1)));
      b.create_local_store(weight, add(b.create_local_load(weight), alpha));
      for (int k = 0; k < 3; ++k)
        b.create_local_store(force[k], add(b.create_local_load(force[k]), mul(alpha, d[k])));
    }
  }
  // status, field availability, coefficient sum, coincidence count,
  // field-force XYZ, relative candidate XYZ, operative-frame candidate XYZ.
  std::array<Stmt *, 13> finish(std::array<Stmt *, 3> position) {
    auto *has = b.create_cmp_gt(b.create_local_load(weight), b.get_float64(0));
    std::array<Stmt *, 3> relative{}, common{};
    for (int k = 0; k < 3; ++k) {
      relative[k] = divide(b.create_local_load(force[k]),
          b.create_select(has, b.create_local_load(weight), b.get_float64(1)));
      common[k] = add(checked(position[k]), relative[k]);
    }
    auto *ok = b.create_logical_not(b.create_local_load(bad));
    auto *status = b.create_select(b.create_local_load(bad), b.get_int32(3),
        b.create_select(b.create_cmp_gt(b.create_local_load(coincident), b.get_int32(0)), b.get_int32(2),
                        b.create_select(has, b.get_int32(1), b.get_int32(0))));
    auto *valid = b.create_cmp_eq(status, b.get_int32(1));
    std::array<Stmt *, 13> out{};
    out[0] = status; out[1] = b.create_select(ok, b.get_int32(1), b.get_int32(0));
    out[2] = b.create_select(ok, b.create_local_load(weight), b.get_float64(0));
    out[3] = b.create_local_load(coincident);
    for (int k = 0; k < 3; ++k) {
      out[4+k] = b.create_select(ok, b.create_local_load(force[k]), b.get_float64(0));
      out[7+k] = b.create_select(valid, relative[k], b.get_float64(0));
      out[10+k] = b.create_select(valid, common[k], b.get_float64(0));
    }
    return out;
  }
};
}  // namespace field::detail
