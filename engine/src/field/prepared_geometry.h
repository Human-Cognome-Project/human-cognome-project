#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <vector>

#include "field/prepared_capacity.h"

namespace field {
enum class GeometryStatus {
  Ok,
  Unsupported,
  Stale,
  Unready,
  InvalidNumerics,
  CapacityExhausted
};
enum class GeometryFocus { Empty, MissingSelection, Valid, Invalid };
enum GeometryPairFlags { ExplicitContact = 1, DerivedContact = 2 };
struct GeometryResult {
  GeometryStatus status{GeometryStatus::Unready};
  BudgetFailure failure{};
  // Query tokens describe observations only; they never schedule calculation.
  std::uint64_t context{}, generation{}, completed_ticks{}, query{};
  std::uint64_t present{}, awake{}, source_contacts{}, contacts{}, derived{},
      presence_entries{}, compaction_slots{}, pair_tests{},
      source_order_tests{}, union_order_tests{}, required_work{},
      readback_bytes{}, synchronizations{};
  GeometryFocus focus{GeometryFocus::Empty};
  std::array<float, 3> focal_point{};
  float focal_mass{};
  std::array<double, 3> frame_anchor{};
};
// Explicit inspection copies of candidate metadata only. Numerical payload
// stays backend-owned. Arrays retain fixed prepared strides, with logical
// counts in the successful query result. Never use a failed/stale candidate for
// admission.
struct GeometryInspection {
  std::vector<int> occupied_rows, pairs, pair_flags, source_to_pair, offsets,
      orders;
};
}  // namespace field
