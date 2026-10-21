#pragma once

#include <cstddef>
#include <cstdint>

namespace taichi::lang {

// Read-only accounting for the native pools, not a process heap/driver trace.
// Cumulative calls/bytes survive reset; reserved_bytes describes live raw
// reservations, including pool slack. Allocation calls include cache hits.
struct MemoryPoolStatistics {
  std::uint64_t allocation_calls{0};
  std::uint64_t requested_bytes{0};
  std::uint64_t raw_allocation_calls{0};
  std::size_t reserved_bytes{0};
};

}  // namespace taichi::lang
