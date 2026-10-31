#pragma once

#include <cfloat>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>
#include "llvm/ADT/APInt.h"

// Independent small-system reference: enumerate stationary points on sphere
// faces, pair intersection circles and triple intersections. Long double is
// used only here, never by the engine. Degenerate concentric spheres are
// covered by face proposals; coplanar triples reduce to their pair circles.
namespace placement_reference {
using Point = std::array<long double, 3>;
struct Sphere {
  std::array<double, 3> centre;
  double radius;
};
inline Point add(Point a, Point b) {
  for (int k = 0; k < 3; ++k)
    a[k] += b[k];
  return a;
}
inline Point sub(Point a, Point b) {
  for (int k = 0; k < 3; ++k)
    a[k] -= b[k];
  return a;
}
inline Point mul(Point a, long double x) {
  for (auto &v : a)
    v *= x;
  return a;
}
inline long double dot(Point a, Point b) {
  long double s = 0;
  for (int k = 0; k < 3; ++k)
    s += a[k] * b[k];
  return s;
}
inline long double norm(Point a) {
  return std::sqrt(dot(a, a));
}
inline Point cross(Point a, Point b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
          a[0] * b[1] - a[1] * b[0]};
}
inline Point point(std::array<double, 3> a) {
  return {a[0], a[1], a[2]};
}
inline long double error(long double scale) {
  return 512 * LDBL_EPSILON * std::max(1.L, scale);
}
inline long double minimum(const std::vector<Sphere> &obstacles,
                           double radius,
                           std::array<double, 3> target) {
  const auto f = point(target);
  long double best = std::numeric_limits<long double>::infinity();
  const auto accept = [&](Point p) {
    for (const auto &s : obstacles) {
      const auto d = sub(p, point(s.centre));
      const long double r = static_cast<long double>(radius) + s.radius;
      if (dot(d, d) < r * r - error(r * r))
        return;
    }
    best = std::min(best, norm(sub(p, f)));
  };
  accept(f);
  for (const auto &s : obstacles) {
    const auto c = point(s.centre);
    const long double r = static_cast<long double>(radius) + s.radius;
    const auto d = sub(f, c);
    const auto length = norm(d);
    if (length > 0)
      accept(add(c, mul(d, r / length)));
    else
      for (int axis = 0; axis < 3; ++axis)
        for (int sign : {-1, 1}) {
          auto p = c;
          p[axis] += sign * r;
          accept(p);
        }
  }
  for (std::size_t i = 0; i < obstacles.size(); ++i)
    for (std::size_t j = 0; j < i; ++j) {
      const auto a = point(obstacles[i].centre), b = point(obstacles[j].centre);
      const long double ra = static_cast<long double>(radius) +
                             obstacles[i].radius,
                        rb = static_cast<long double>(radius) +
                             obstacles[j].radius;
      const auto delta = sub(b, a);
      const auto d = norm(delta);
      if (d == 0 || d > ra + rb || d < std::abs(ra - rb))
        continue;
      const auto axis = mul(delta, 1 / d);
      const auto x = (ra * ra - rb * rb + d * d) / (2 * d);
      const auto h2 = ra * ra - x * x;
      if (h2 < -error(ra * ra))
        continue;
      const auto h = std::sqrt(std::max(0.L, h2));
      const auto c = add(a, mul(axis, x));
      auto projected = sub(f, c);
      projected = sub(projected, mul(axis, dot(projected, axis)));
      if (norm(projected) > error(norm(sub(f, c)))) {
        const auto radial = mul(projected, h / norm(projected));
        accept(add(c, radial));
        accept(sub(c, radial));
      } else {
        int k = 0;
        if (std::abs(axis[1]) < std::abs(axis[k]))
          k = 1;
        if (std::abs(axis[2]) < std::abs(axis[k]))
          k = 2;
        Point unit{};
        unit[k] = 1;
        auto v = cross(axis, unit);
        v = mul(v, h / norm(v));
        accept(add(c, v));
        accept(sub(c, v));
      }
      for (std::size_t k = 0; k < j; ++k) {
        const auto third = point(obstacles[k].centre), v = sub(third, a);
        const long double rc =
            static_cast<long double>(radius) + obstacles[k].radius;
        const auto along = dot(axis, v);
        const auto perpendicular = sub(v, mul(axis, along));
        const auto ylen = norm(perpendicular);
        if (ylen == 0)
          continue;
        const auto ey = mul(perpendicular, 1 / ylen), ez = cross(axis, ey);
        const auto y =
            (ra * ra - rc * rc + dot(v, v) - 2 * along * x) / (2 * ylen);
        const auto z2 = ra * ra - x * x - y * y;
        if (z2 < -error(ra * ra))
          continue;
        const auto base = add(c, mul(ey, y)),
                   offset = mul(ez, std::sqrt(std::max(0.L, z2)));
        accept(add(base, offset));
        accept(sub(base, offset));
      }
    }
  if (!std::isfinite(best))
    throw std::runtime_error(
        "independent sphere reference found no feasible boundary");
  return best;
}
// Exact separation and objective checks use integers in units of 2^-1074,
// independently of the production TwoSum/Dekker implementation.
inline llvm::APInt exact(double value) {
  std::uint64_t bits;
  std::memcpy(&bits, &value, 8);
  const auto exponent = (bits >> 52) & 2047;
  if (exponent == 2047)
    throw std::runtime_error("nonfinite placement certificate");
  llvm::APInt n(8192,
                (bits & 0xfffffffffffffULL) | (exponent ? 1ULL << 52 : 0));
  if (exponent)
    n <<= exponent - 1;
  return bits >> 63 ? -n : n;
}
inline llvm::APInt square_distance(std::array<double, 3> a,
                                   std::array<double, 3> b) {
  llvm::APInt sum(8192, 0);
  for (int k = 0; k < 3; ++k) {
    const auto d = exact(a[k]) - exact(b[k]);
    sum += d * d;
  }
  return sum;
}
inline void certificate(const field::PlacementCertificate &c,
                        const std::vector<Sphere> &obstacles,
                        std::array<double, 3> f,
                        int bits) {
  for (const auto &s : obstacles) {
    const auto r = exact(c.intrinsic.radius) + exact(s.radius);
    if (square_distance(c.position, s.centre).slt(r * r))
      throw std::runtime_error("stored placement overlaps an exact exclusion");
  }
  if (square_distance(c.position, f)
          .sgt(exact(c.upper_distance) * exact(c.upper_distance)))
    throw std::runtime_error(
        "upper certificate understates exact stored distance");
  for (auto x : c.position) {
    std::uint64_t word;
    std::memcpy(&word, &x, 8);
    if (bits < 52 && (word & ((1ULL << (52 - bits)) - 1)))
      throw std::runtime_error("placement bypassed actual publication mask");
  }
  const auto optimum = minimum(obstacles, c.intrinsic.radius, f);
  const auto slack =
      error(std::max(optimum, static_cast<long double>(c.upper_distance)));
  if (c.lower_distance > optimum + slack ||
      optimum > c.upper_distance + slack ||
      static_cast<long double>(c.upper_distance) - c.lower_distance >
          c.tolerance ||
      c.lower_distance < 0)
    throw std::runtime_error(
        "global placement certificate disagrees with independent boundary "
        "reference");
}
}  // namespace placement_reference
