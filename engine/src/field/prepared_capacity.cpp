#include "field/prepared_capacity.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "field/activity.h"

namespace field {
namespace {
std::size_t product(std::size_t a, std::size_t b) {
  if (a && b > std::numeric_limits<std::size_t>::max() / a)
    throw std::overflow_error("prepared numerical byte product overflow");
  return a * b;
}
DenseShape shape(const char *name,
                 std::size_t rows,
                 std::uint64_t count,
                 std::size_t element_bytes = sizeof(float)) {
  const auto stride = std::max<std::size_t>(count, 1);
  return {name, rows, stride, element_bytes,
          product(product(rows, stride), element_bytes)};
}
}  // namespace
PreparedCapacity::PreparedCapacity(BudgetDemand limits) : limits_(limits) {
  for (std::size_t i = 0; i < static_cast<std::size_t>(Budget::ScratchBytes);
       ++i) {
    if (limits.values[i] > (std::uint64_t{1} << 30))
      throw std::invalid_argument(
          "prepared logical dimension exceeds padded i32 domain");
  }
  if (!limits[Budget::Potential] ||
      limits[Budget::Particles] > limits[Budget::Potential] ||
      limits[Budget::Focus] > limits[Budget::Fields])
    throw std::invalid_argument("inconsistent prepared logical limits");
  const auto base = numerical_bytes();
  if (limits[Budget::ScratchBytes] >
      std::numeric_limits<std::size_t>::max() - base)
    throw std::overflow_error(
        "prepared numerical plus scratch byte sum overflow");
}
BudgetFailure PreparedCapacity::check(const BudgetDemand &demand) const {
  for (std::size_t i = 0; i < demand.values.size(); ++i)
    if (demand.values[i] > limits_.values[i])
      return {static_cast<Budget>(i), demand.values[i], limits_.values[i]};
  return {};
}
std::array<DenseShape, 22> PreparedCapacity::numerical_shapes() const {
  const auto p = limits_[Budget::Particles], g = limits_[Budget::Fields],
             e = limits_[Budget::Participation], c = limits_[Budget::Contacts];
  return {{shape("particles", kParticleFieldCount, p),
           shape("groups", kGroupFieldCount, g),
           shape("edge-floats", kEdgeFloatFieldCount, e),
           shape("edge-integers", kEdgeIntFieldCount, e, sizeof(int)),
           shape("contacts", kContactIntFieldCount, c, sizeof(int)),
           shape("determiners", 1, p),
           shape("particle-flags", detail::ParticleStateCount, p, sizeof(int)),
           shape("particle-delta", 3, p),
           shape("group-flags", detail::GroupStateCount, g, sizeof(int)),
           shape("group-delta", 6, g),
           shape("particle-offsets", 2, p, sizeof(int)),
           shape("group-offsets", 2, g, sizeof(int)),
           shape("contact-a-offsets", 2, p, sizeof(int)),
           shape("contact-b-offsets", 2, p, sizeof(int)),
           shape("particle-edge-order", 1, e, sizeof(int)),
           shape("group-edge-order", 1, e, sizeof(int)),
           shape("contact-a-order", 1, c, sizeof(int)),
           shape("contact-b-order", 1, c, sizeof(int)),
           shape("focus", 11, 1),
           shape("selected-focus", 1, limits_[Budget::Focus], sizeof(int)),
           shape("counters", 1, detail::CounterCount, sizeof(std::int64_t)),
           shape("universal-mass", 1, 1)}};
}
std::size_t PreparedCapacity::numerical_bytes() const {
  std::size_t total = 0;
  for (const auto &entry : numerical_shapes()) {
    if (entry.bytes > std::numeric_limits<std::size_t>::max() - total)
      throw std::overflow_error("prepared numerical byte sum overflow");
    total += entry.bytes;
  }
  return total;
}
const char *budget_name(Budget budget) {
  static const char *names[]{
      "potential",  "particles", "fields", "participation", "contacts",
      "references", "focus",     "batch",  "scratch-bytes", "none"};
  const auto i = static_cast<std::size_t>(budget);
  return names[i <= static_cast<std::size_t>(Budget::Count)
                   ? i
                   : static_cast<std::size_t>(Budget::Count)];
}
}  // namespace field
