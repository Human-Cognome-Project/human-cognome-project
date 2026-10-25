#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "engine/record_map.h"
#include "field/prepared_capacity.h"
#include "field/prepared_geometry.h"
#include "field/prepared_update.h"
#include "field/real_buffer.h"

namespace field::detail {
struct RelationshipHeader {
  engine::RecordHandle first, second;
  std::uint64_t ordinal{};
  float share{};
};
struct PreparedCounts {
  std::size_t particles{}, fields{}, edges{}, contacts{}, focus{};
  std::size_t references() const {
    return particles + fields + 2 * edges + 2 * contacts + focus;
  }
};
struct StorageExtent {
  const char *name;
  std::size_t count, element_bytes, copies, bytes;
};

// Working-capacity metadata/workspace; never a payload per potential address.
// Numerical arrays and awake state remain in the existing Harness/Activity.
class PreparedStorage {
 public:
  static std::vector<StorageExtent> workspace_layout(
      const BudgetDemand &limits, NumericalProfile profile = {});
  static std::size_t workspace_bytes(const BudgetDemand &limits,
                                     NumericalProfile profile = {});
  static std::size_t binding_reserve_bytes(bool cuda, bool geometry = false,
                                           bool cascade = false,
                                           bool weighted = false,
                                           bool wide = false);
  static std::size_t required_scratch(const BudgetDemand &limits, bool cuda,
                                      NumericalProfile profile = {});

  PreparedStorage(engine::Runtime &runtime, const PreparedCapacity &capacity);
  BudgetFailure check(const PreparedCounts &counts, std::size_t batch) const;
  bool prepare_ratios(bool prospective, std::size_t count,
                      std::uint64_t &visits);
  const PreparedCapacity capacity;
  engine::PreparedRecordMap particles, fields;
  PreparedCounts live;
  bool populated{false};
  std::uint64_t generation{0};
  std::uint64_t completed_ticks{0};  // Observation provenance, never activity.
  std::size_t binding_bytes{0};
  std::vector<StorageExtent> manifest;
  std::vector<engine::RecordHandle> particle_headers, field_headers,
      prospective_particles, prospective_fields;
  // By present-record capacity, not potential address. Structural denominator
  // provenance is independent of the resident numerical physical mass. Zero
  // encodes unavailable here; explicitly supplied zero is rejected on input.
  std::vector<std::uint64_t> source_masses, prospective_source_masses;
  std::vector<RelationshipHeader> edges, contacts, prospective_edges,
      prospective_contacts;
  // Ordered participation rows only; contacts have no count payload.
  std::vector<std::uint64_t> source_counts, prospective_source_counts;
  std::vector<engine::RecordHandle> focal;
  std::vector<UpdateCommand> commands;
  std::vector<engine::RecordResult> results;
  std::vector<engine::RecordHandle> retired, resolved;
  std::vector<int> prospective_csr, counts, cursors, validation;
  // Scratch indexes are candidate row names, never a replacement allocator.
  std::vector<int> destinations, claim_addresses, claim_kinds, command_indexes;
  std::vector<int> candidate_edges;
  std::vector<float> candidate_shares;
  RealBuffer new_row_values, inspection_motion_values;
  std::vector<float> &new_rows{new_row_values.legacy()};
  // Trial-only flat descriptor: E raw counts, E denominators, G structural
  // has-other-occurrence flags. Committed/prospective host and native copies.
  std::vector<std::uint64_t> live_ratios, candidate_ratios;
  std::unique_ptr<taichi::lang::Ndarray> live_ratios_array,
      candidate_ratios_array;
  std::array<int, 1> candidate_status{};
  std::unique_ptr<taichi::lang::Ndarray> validation_array, derived_array,
      destinations_array, candidate_edges_array, candidate_shares_array,
      new_rows_array, candidate_status_array, candidate_offsets_array,
      candidate_order_array;
  // Complete activity/history readback, reserved before readiness.
  std::vector<int> inspection_state;
  std::vector<float> &inspection_motion{inspection_motion_values.legacy()};
  const std::vector<double> &motion64() const {
    return inspection_motion_values.doubles();
  }
  std::array<float, 11> inspection_focus{};
  std::array<double, 11> inspection_focus64{};
  std::uint64_t inspected_generation{};
  // Only the private test adapter can request injected failures.
  int failure_point{};
  std::vector<int> inspection_flags;
  std::array<std::int64_t, 4> inspection_counts{};
  std::unique_ptr<taichi::lang::Ndarray> inspection_flags_array,
      inspection_counts_array;
  std::unique_ptr<taichi::lang::Ndarray> geometry_rows, geometry_temporary,
      geometry_pairs, geometry_flags, geometry_source_order, geometry_group_end,
      geometry_source_map, geometry_permutation, geometry_offsets,
      geometry_cursors, geometry_orders, geometry_counters, geometry_focus;
  std::array<std::int64_t, 16> geometry_counts{};
  std::array<float, 4> geometry_focal{};
  std::array<double, 4> geometry_focal64{};
  GeometryInspection geometry_inspection;
  GeometryResult geometry_last;
  std::uint64_t geometry_query{0};
  bool geometry_valid{false};
};
}  // namespace field::detail
