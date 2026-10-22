#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "engine/record_map.h"
#include "field/prepared_capacity.h"
#include "field/prepared_update.h"

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
      const BudgetDemand &limits);
  static std::size_t workspace_bytes(const BudgetDemand &limits);
  static std::size_t binding_reserve_bytes(bool cuda);
  static std::size_t required_scratch(const BudgetDemand &limits, bool cuda);

  PreparedStorage(engine::Runtime &runtime, const PreparedCapacity &capacity);
  BudgetFailure check(const PreparedCounts &counts, std::size_t batch) const;
  const PreparedCapacity capacity;
  engine::PreparedRecordMap particles, fields;
  PreparedCounts live;
  bool populated{false};
  std::uint64_t generation{0};
  std::size_t binding_bytes{0};
  std::vector<StorageExtent> manifest;
  std::vector<engine::RecordHandle> particle_headers, field_headers,
      prospective_particles, prospective_fields;
  std::vector<RelationshipHeader> edges, contacts, prospective_edges,
      prospective_contacts;
  std::vector<engine::RecordHandle> focal;
  std::vector<UpdateCommand> commands;
  std::vector<engine::RecordResult> results;
  std::vector<engine::RecordHandle> retired, resolved;
  std::vector<int> prospective_csr, counts, cursors, validation;
  // Scratch indexes are candidate row names, never a replacement allocator.
  std::vector<int> destinations, claim_addresses, claim_kinds, command_indexes;
  std::vector<int> candidate_edges;
  std::vector<float> candidate_shares, new_rows;
  std::array<int, 1> candidate_status{};
  std::unique_ptr<taichi::lang::Ndarray> validation_array, derived_array,
      destinations_array, candidate_edges_array, candidate_shares_array,
      new_rows_array, candidate_status_array, candidate_offsets_array,
      candidate_order_array;
  // Complete activity/history readback, reserved before readiness.
  std::vector<int> inspection_state;
  std::vector<float> inspection_motion;
  std::array<float, 11> inspection_focus{};
  std::uint64_t inspected_generation{};
  // Only the private test adapter can request injected failures.
  int failure_point{};
  std::vector<int> inspection_flags;
  std::array<std::int64_t, 4> inspection_counts{};
  std::unique_ptr<taichi::lang::Ndarray> inspection_flags_array,
      inspection_counts_array;
};
}  // namespace field::detail
