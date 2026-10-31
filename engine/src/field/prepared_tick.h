#pragma once

#include <cstdint>
#include <limits>
#include "field/prepared_geometry.h"

namespace field {
enum class TickStatus {
  Ok,
  Unsupported,
  Stale,
  Unready,
  InvalidInput,
  InvalidNumerics,
  CapacityExhausted
};
// Attempt diagnostics are separate from Harness::demand(), which always
// describes the last successfully published tick.
struct TickResult {
  NumericalProfile numerical_profile{};
  TickStatus status{TickStatus::Unready};
  BudgetFailure failure{};
  std::uint64_t context{}, generation{}, completed_ticks{};
  std::uint64_t required_work{}, copy_values{}, numerical_work_bound{},
      selection_work_bound{}, geometry_work{}, synchronizations{},
      readback_bytes{};
  bool coverage_changed{};
  // Attempt diagnostics only: query remains zero and is not an inspection
  // token.
  GeometryResult current, candidate;
};
}  // namespace field
