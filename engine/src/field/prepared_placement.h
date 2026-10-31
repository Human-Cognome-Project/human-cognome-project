#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include "field/prepared_update.h"

namespace field {
enum class PlacementStatus {
  Ok = 0,
  Unsupported = 1,
  InvalidInput = 2,
  InvalidNumerics = 3,
  CapacityExhausted = 4,
  Stale = 5,
  Unready = 6
};
// Source adapters bind these opaque identifiers to a complete intrinsic
// definition. The engine never looks up a database to infer missing geometry.
struct PlacementGeometry {
  std::uint64_t definition{}, revision{};
  bool complete{};
};
struct PlacementIntrinsic {
  std::uint64_t command{}, order{};
  double mass{}, radius{};
  PlacementGeometry geometry;
};
struct PlacementCertificate {
  PlacementIntrinsic intrinsic;
  std::array<double, 3> position{};
  double lower_distance{}, upper_distance{}, tolerance{}, coordinate_bound{};
};
struct PlacementResult {
  NumericalProfile numerical_profile{};
  PlacementStatus status{PlacementStatus::Unready};
  BudgetFailure failure{};
  std::uint64_t context{}, generation{}, completed_ticks{}, query{};
  std::uint64_t placements{}, present{}, retained{}, work{},
      source_work_bound{}, cell_peak{}, cell_visits{}, candidate_tests{},
      heap_moves{}, proposals{}, synchronizations{}, upload_bytes{},
      readback_bytes{};
  std::array<double, 3> target{}, frame_anchor{};
};
// Candidate diagnostics only. No handle is claimed and no occurrence is
// initialized by this query. B3 must coordinate source, hierarchy and geometry.
struct PlacementInspection {
  std::vector<UpdateCommand> proposal;
  std::vector<PlacementCertificate> certificates;
};
}  // namespace field
