#pragma once

#include <array>
#include "field/numeric_ir.h"

namespace field::detail::placement {
// Native strict-f64 interval primitives. Inputs are checked before arithmetic;
// the supported operand windows keep every error-free transform normal or zero
// on the fork's FTZ backend. See CERTIFIED-PLACEMENT.md for the derivation.
struct Interval {
  taichi::lang::Stmt *lo{}, *hi{};
};
struct Rounded {
  taichi::lang::Stmt *value{}, *error{};
};
class Math {
 public:
  using V = taichi::lang::Stmt;
  using B = taichi::lang::IRBuilder;
  B &b;
  // An attempt-local i32 status, never a live model flag.
  taichi::lang::AllocaStmt *status;
  explicit Math(B &builder, taichi::lang::AllocaStmt *attempt_status)
      : b(builder), status(attempt_status) {
  }
  V *f(double x) {
    return b.get_float64(x);
  }
  V *i(int x) {
    return b.get_int32(x);
  }
  V *both(V *a, V *c) {
    return b.create_logical_and(a, c);
  }
  V *either(V *a, V *c) {
    return b.create_logical_or(a, c);
  }
  V *healthy() {
    return b.create_cmp_eq(b.create_local_load(status), i(0));
  }
  void invalid(V *condition) {
    b.create_local_store(status,
                         b.create_select(both(healthy(), condition), i(3),
                                         b.create_local_load(status)));
  }
  V *supported(V *x, int minimum, int maximum) {
    auto *m =
        b.create_and(b.create_bit_cast(x, taichi::lang::PrimitiveType::u64),
                     b.get_uint64(0x7fffffffffffffffULL));
    return either(
        b.create_cmp_eq(m, b.get_uint64(0)),
        both(b.create_cmp_ge(m,
                             b.get_uint64(std::uint64_t(minimum + 1023) << 52)),
             b.create_cmp_le(
                 m, b.get_uint64(std::uint64_t(maximum + 1023) << 52))));
  }
  V *checked(V *x, int minimum, int maximum) {
    auto *ok = supported(x, minimum, maximum);
    invalid(b.create_logical_not(ok));
    return b.create_select(ok, x, f(0));
  }
  V *next(V *x, bool up) {
    auto *word = b.create_bit_cast(x, taichi::lang::PrimitiveType::u64);
    auto *one = b.get_uint64(1);
    auto *positive = b.create_cmp_gt(x, f(0));
    V *increase =
        up ? static_cast<V *>(positive) : b.create_logical_not(positive);
    auto *bits = b.create_select(increase, b.create_add(word, one),
                                 b.create_sub(word, one));
    // Zero is only nudged for an outward bound, never into an unsupported
    // subnormal. Exact zero operations below do not nudge their result.
    bits = b.create_select(
        b.create_cmp_eq(x, f(0)),
        b.get_uint64(up ? 0x0010000000000000ULL : 0x8010000000000000ULL), bits);
    return b.create_bit_cast(bits, taichi::lang::PrimitiveType::f64);
  }
  Rounded sum(V *a, V *c) {
    a = checked(a, -768, 768);
    c = checked(c, -768, 768);
    auto *s = b.create_add(a, c);
    auto *v = b.create_sub(s, a);
    auto *e =
        b.create_add(b.create_sub(a, b.create_sub(s, v)), b.create_sub(c, v));
    return {s, e};
  }
  Rounded product(V *a, V *c) {
    a = checked(a, -384, 384);
    c = checked(c, -384, 384);
    const auto split = [&](V *x) {
      auto *t = b.create_mul(f(134217729.0), x);  // 2^27 + 1
      auto *hi = b.create_sub(t, b.create_sub(t, x));
      return std::array<V *, 2>{hi, b.create_sub(x, hi)};
    };
    const auto aa = split(a), cc = split(c);
    auto *p = b.create_mul(a, c);
    auto *e = b.create_add(
        b.create_add(b.create_add(b.create_sub(b.create_mul(aa[0], cc[0]), p),
                                  b.create_mul(aa[0], cc[1])),
                     b.create_mul(aa[1], cc[0])),
        b.create_mul(aa[1], cc[1]));
    return {p, e};
  }
  Interval enclose(Rounded x) {
    return {b.create_select(b.create_cmp_lt(x.error, f(0)),
                            next(x.value, false), x.value),
            b.create_select(b.create_cmp_gt(x.error, f(0)), next(x.value, true),
                            x.value)};
  }
  Interval point(V *x) {
    return {x, x};
  }
  Interval add(Interval a, Interval c) {
    if (a.lo == a.hi && c.lo == c.hi)
      return enclose(sum(a.lo, c.lo));
    return {enclose(sum(a.lo, c.lo)).lo, enclose(sum(a.hi, c.hi)).hi};
  }
  Interval neg(Interval a) {
    if (a.lo == a.hi) {
      auto *x = b.create_neg(a.lo);
      return {x, x};
    }
    return {b.create_neg(a.hi), b.create_neg(a.lo)};
  }
  Interval sub(Interval a, Interval c) {
    return add(a, neg(c));
  }
  Interval mul(Interval a, Interval c) {
    if (a.lo == a.hi && c.lo == c.hi)
      return enclose(product(a.lo, c.lo));
    if (a.lo == a.hi || c.lo == c.hi) {
      if (c.lo == c.hi)
        std::swap(a, c);
      const auto left = enclose(product(a.lo, c.lo)),
                 right = enclose(product(a.lo, c.hi));
      return {b.create_min(left.lo, right.lo), b.create_max(left.hi, right.hi)};
    }
    std::array<Interval, 4> p{
        enclose(product(a.lo, c.lo)), enclose(product(a.lo, c.hi)),
        enclose(product(a.hi, c.lo)), enclose(product(a.hi, c.hi))};
    auto out = p[0];
    for (int k = 1; k < 4; ++k) {
      out.lo = b.create_min(out.lo, p[k].lo);
      out.hi = b.create_max(out.hi, p[k].hi);
    }
    return out;
  }
  Interval square(Interval a) {
    if (a.lo == a.hi)
      return enclose(product(a.lo, a.lo));
    V *small = b.create_min(b.create_abs(a.lo), b.create_abs(a.hi));
    small = b.create_select(
        both(b.create_cmp_le(a.lo, f(0)), b.create_cmp_ge(a.hi, f(0))), f(0),
        small);
    auto *large = b.create_max(b.create_abs(a.lo), b.create_abs(a.hi));
    return {enclose(product(small, small)).lo,
            enclose(product(large, large)).hi};
  }
  Interval root(Interval a) {
    const auto bounds = [&](V *x) -> Interval {
      x = checked(x, -768, 768);
      invalid(b.create_cmp_lt(x, f(0)));
      auto *r = b.create_sqrt(b.create_max(x, f(0)));
      const auto squared = product(r, r);
      auto *less = either(b.create_cmp_lt(squared.value, x),
                          both(b.create_cmp_eq(squared.value, x),
                               b.create_cmp_lt(squared.error, f(0))));
      auto *more = either(b.create_cmp_gt(squared.value, x),
                          both(b.create_cmp_eq(squared.value, x),
                               b.create_cmp_gt(squared.error, f(0))));
      return {b.create_select(more, next(r, false), r),
              b.create_select(less, next(r, true), r)};
    };
    if (a.lo == a.hi)
      return bounds(a.lo);
    return {bounds(a.lo).lo, bounds(a.hi).hi};
  }
  Interval divide_positive(Interval a, Interval c) {
    invalid(either(b.create_cmp_lt(a.lo, f(0)), b.create_cmp_le(c.lo, f(0))));
    const auto bound = [&](V *x, V *y, bool upper) {
      x = checked(x, -768, 768);
      y = checked(y, -768, 768);
      V *q =
          b.create_div(x, b.create_select(b.create_cmp_gt(y, f(0)), y, f(1)));
      auto *ok =
          both(supported(q, -768, 768),
               either(b.create_cmp_eq(x, f(0)), b.create_cmp_ne(q, f(0))));
      invalid(b.create_logical_not(ok));
      q = b.create_select(ok, q, f(0));
      // Always outward for nonzero division. Exact zero is preserved.
      return b.create_select(b.create_cmp_eq(x, f(0)), f(0), next(q, upper));
    };
    return {bound(a.lo, c.hi, false), bound(a.hi, c.lo, true)};
  }
  Interval norm2(const std::array<Interval, 3> &d) {
    return add(add(square(d[0]), square(d[1])), square(d[2]));
  }
  Interval distance2(const std::array<V *, 3> &a, const std::array<V *, 3> &c) {
    std::array<Interval, 3> d;
    for (int k = 0; k < 3; ++k)
      d[k] = sub(point(a[k]), point(c[k]));
    return norm2(d);
  }
  // Largest truncation cell width in [-m,m], for the body's actual publication
  // mask. m is an outward bound on candidate absolute coordinates.
  V *quantum(V *m, int bits) {
    m = checked(m, -256, 256);
    auto *word = b.create_bit_cast(m, taichi::lang::PrimitiveType::u64);
    auto *exponent = b.create_and(word, b.get_uint64(0x7ff0000000000000ULL));
    auto *step = b.create_bit_cast(
        b.create_sub(exponent, b.get_uint64(std::uint64_t(bits) << 52)),
        taichi::lang::PrimitiveType::f64);
    return b.create_select(b.create_cmp_eq(m, f(0)), f(0), step);
  }
  V *tolerance(V *m, V *upper, int bits) {
    auto q = mul(root(point(f(3))), point(quantum(m, bits)));
    // e >= 2u/(1-u), u=2^-53: one adjacent outward endpoint per
    // correctly-rounded operation. The norm enclosure has at most 5 squared
    // factors plus sqrt (3.5 distance factors), bounded by four. Its width is
    // <= ((1+e)^4-(1-e)^4)*D = 8e(1+e^2)*D, with D <= upper.
    auto e = point(f(0x1.0000000000001p-52));
    auto arithmetic = mul(mul(point(f(8)), e), add(point(f(1)), square(e)));
    return add(q, mul(arithmetic, point(upper))).hi;
  }
};
}  // namespace field::detail::placement
