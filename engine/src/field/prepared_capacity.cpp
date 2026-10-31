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
PreparedCapacity::PreparedCapacity(BudgetDemand limits,
                                   NumericalProfile profile)
    : limits_(limits), profile_(profile) {
  profile_.validate();
  if (profile_.wide() &&
      2 * std::max<std::uint64_t>(limits[Budget::Participation], 1) +
              std::max<std::uint64_t>(limits[Budget::Fields], 1) >
          std::uint64_t(INT32_MAX))
    throw std::invalid_argument("trial source descriptor exceeds native axis");
  if (limits[Budget::GeometryWork] > std::uint64_t(INT64_MAX))
    throw std::invalid_argument(
        "geometry work exceeds signed native counter range");
  if (limits[Budget::CascadeWork] > std::uint64_t(INT64_MAX))
    throw std::invalid_argument(
        "cascade work exceeds signed native counter range");
  for (auto kind :
       {Budget::CascadeNodes, Budget::CascadeLinks, Budget::CascadeReferences,
        Budget::CascadeLevels, Budget::CascadeCoverage, Budget::LiveResponses})
    if (limits[kind] > (std::uint64_t{1} << 30))
      throw std::invalid_argument("cascade dimension exceeds native axis");
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
  if (limits[Budget::LiveResponses] &&
      (!profile.wide() || !limits[Budget::CascadeNodes] ||
       !limits[Budget::CascadeCoverage]))
    throw std::invalid_argument(
        "live hierarchy requires weighted f64 capacity");
  if (limits[Budget::TickWork] > std::uint64_t(INT64_MAX))
    throw std::invalid_argument(
        "tick work exceeds signed native counter range");
  if (limits[Budget::TickWork] &&
      (!profile.wide() || !limits[Budget::GeometryWork]))
    throw std::invalid_argument(
        "contact-safe ticks require f64 geometry capacity");
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
  return {
      {shape("particles", kParticleFieldCount, p, profile_.bytes()),
       shape("groups", kGroupFieldCount, g, profile_.bytes()),
       shape("edge-floats", kEdgeFloatFieldCount, e),
       shape("edge-integers", kEdgeIntFieldCount, e, sizeof(int)),
       shape("contacts", kContactIntFieldCount, c, sizeof(int)),
       shape("determiners", 1, p, profile_.bytes()),
       shape("particle-flags", detail::ParticleStateCount, p, sizeof(int)),
       shape("particle-delta", 3, p, profile_.bytes()),
       shape("group-flags", detail::GroupStateCount, g, sizeof(int)),
       shape("group-delta", 6, g, profile_.bytes()),
       shape("particle-offsets", 2, p, sizeof(int)),
       shape("group-offsets", 2, g, sizeof(int)),
       shape("contact-a-offsets", 2, p, sizeof(int)),
       shape("contact-b-offsets", 2, p, sizeof(int)),
       shape("particle-edge-order", 1, e, sizeof(int)),
       shape("group-edge-order", 1, e, sizeof(int)),
       shape("contact-a-order", 1, c, sizeof(int)),
       shape("contact-b-order", 1, c, sizeof(int)),
       shape("focus", 11, 1, profile_.bytes()),
       shape("selected-focus", 1, limits_[Budget::Focus], sizeof(int)),
       shape("counters", 1,
             profile_.wide() ? detail::TrialCounterCount : detail::CounterCount,
             sizeof(std::int64_t)),
       shape("universal-mass", 1, 1, profile_.bytes())}};
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
  static const char *names[]{"potential",
                             "particles",
                             "fields",
                             "participation",
                             "contacts",
                             "references",
                             "focus",
                             "batch",
                             "scratch-bytes",
                             "geometry-work",
                             "cascade-nodes",
                             "cascade-links",
                             "cascade-references",
                             "cascade-levels",
                             "cascade-work",
                             "cascade-coverage",
                             "live-responses",
                             "tick-work",
                             "none"};
  const auto i = static_cast<std::size_t>(budget);
  return names[i <= static_cast<std::size_t>(Budget::Count)
                   ? i
                   : static_cast<std::size_t>(Budget::Count)];
}
}  // namespace field
