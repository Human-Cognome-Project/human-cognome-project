#pragma once
#include <cstdint>

namespace taichi::lang {
enum class PoolBackingStatus { Unavailable, Unknown, Absent, Live };
struct PoolBackingToken {
  std::uint64_t pool_instance{0}, allocation{0};
  std::uintptr_t base{0};
  bool operator==(const PoolBackingToken &other) const {
    return pool_instance == other.pool_instance && allocation == other.allocation &&
           base == other.base;
  }
};
struct PoolBackingRecord {
  PoolBackingStatus status{PoolBackingStatus::Unavailable};
  PoolBackingToken token;
  std::uint64_t raw_requested_bytes{0};
  bool rounded_extent_available{false};
  std::uint64_t rounded_extent_bytes{0};
};
struct PoolBackingStorage {
  bool available{false}, metadata_complete{false};
  std::uint64_t pool_instance{0}, pool_object_bytes{0}, record_payload_bytes{0};
  // Subset of record_payload_bytes, not an additional charge.
  std::uint64_t incremental_record_payload_bytes{0};
  // Record payload above is a subset of actual map allocation requests below.
  bool record_storage_available{false}, allocator_storage_available{false};
  std::uint64_t record_allocation_bytes{0};
  std::uint64_t allocator_object_bytes{0}, allocator_chunk_capacity_bytes{0};
};
}  // namespace taichi::lang
