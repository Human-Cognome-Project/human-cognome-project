#pragma once
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace assembly::detail {
// Shared enclosure validation for original double-valued geometry. No packing,
// coordinate generation, scale conversion or f32 staging occurs here.
template<class Point, class Components>
double intrinsic_enclosing_radius(const Point &centroid, const Components &components) {
  const auto finite = [](const auto &p) {
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
      throw std::invalid_argument("assembly: non-finite coordinates");
  };
  finite(centroid);
  if (components.empty())
    throw std::invalid_argument("assembly: supplied component geometry required");
  double radius = 0;
  for (const auto &component : components) {
    finite(component.centre);
    if (!std::isfinite(component.radius) || component.radius < 0)
      throw std::invalid_argument("assembly: invalid component radius");
    const double distance = std::hypot(component.centre.x - centroid.x,
                                       component.centre.y - centroid.y,
                                       component.centre.z - centroid.z);
    const double extent = distance + component.radius;
    if (!std::isfinite(extent))
      throw std::invalid_argument("assembly: enclosure overflow");
    radius = std::max(radius, extent);
  }
  return radius;
}
}
