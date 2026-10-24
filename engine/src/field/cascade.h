#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>
#include "engine/precision.h"
#include "engine/record_map.h"
#include "field/prepared_capacity.h"

namespace field {
enum class CascadeStatus {
  Ok,
  Empty,
  Unsupported,
  Unready,
  Stale,
  InvalidReference,
  InvalidHierarchy,
  UnsupportedParticipation,
  UnsupportedProfile,
  InvalidNumerics,
  CapacityExhausted
};
// Explicit local comparison identity; not an address or semantic matcher.
// Higher levels mean coarser comparisons. Children precede parents, with the
// unique expressed root last. Internal grouping keeps its parent's context.
struct CascadeNode {
  engine::RecordHandle field;
  std::uint64_t comparison{};
  int level{};
  bool publish{true};
  int retained_fraction_bits{52};
  engine::numeric::Real calculation{engine::numeric::Real::F64};
  engine::numeric::Real storage{engine::numeric::Real::F64};
  std::uint64_t cohort_begin{}, cohort_count{}, child_begin{}, child_count{};
};
struct CascadeSpec {
  const CascadeNode *nodes{};
  std::size_t node_count{};
  const int *children{};
  std::size_t child_count{};
  const engine::RecordHandle *cohorts{};
  std::size_t cohort_count{};
};
struct CascadeBinding {
  CascadeStatus status{CascadeStatus::Unready};
  BudgetFailure failure{};
  std::uint64_t generation{}, binding{}, nodes{}, links{}, references{},
      levels{}, aliases{}, preparation_visits{}, uploaded_bytes{},
      readback_bytes{}, synchronizations{};
};
struct CascadeResult {
  CascadeStatus status{CascadeStatus::Unready};
  BudgetFailure failure{};
  // Observation provenance only, never activity scheduling.
  std::uint64_t context{}, generation{}, completed_ticks{}, binding{}, query{},
      nodes{}, links{}, references{}, levels{}, required_work{},
      reference_visits{}, field_checks{}, reduction_visits{}, node_reductions{},
      awake_reference_visits{}, readback_bytes{}, synchronizations{};
  int root{-1};
  std::array<double, 3> trial{}, centre{}, frame_anchor{};
  double mass{};
};
// Borrowed until the next synchronized operation or destruction. Use only the
// logical prefixes in the successful result; copying the reference is not a
// lifetime extension. Bind input must be caller-owned, not these mutable
// tables. Fixed capacity strides: trial XYZ, published XYZ, untrimmed mass.
// Internal grouping rows are temporary arithmetic, not physical publication.
struct CascadeInspection {
  std::vector<CascadeNode> nodes;
  std::vector<engine::RecordHandle> cohorts;
  std::vector<double> values;
  std::vector<int> children, valid;
};
}  // namespace field
