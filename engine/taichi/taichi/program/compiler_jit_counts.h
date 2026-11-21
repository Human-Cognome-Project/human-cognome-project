#pragma once

#include <cstddef>

namespace taichi::lang {
enum class OwnershipCountStatus { Unavailable, Absent, Present, Incomplete };

// Value observations under live, quiescent ownership. Entry/module budgets
// bound inspection work only. No allocation, factory, loading or registration.
// complete remains false: these counts exclude borrowers, arenas, executable
// allocations and other owned resources; they are not a byte census or B.
// On Incomplete, container sizes remain known but object/role counts are not
// reported. Callers must check status before treating those zeros as counts.
struct CompilerOwnershipCounts {
  OwnershipCountStatus status{OwnershipCountStatus::Unavailable};
  bool complete{false};
  const void *owner{nullptr};
  std::size_t pending_entries{}, pending_objects{};
  std::size_t disk_metadata_entries{}, loaded_objects{}, visited_entries{};
};
struct JitOwnershipCounts {
  OwnershipCountStatus status{OwnershipCountStatus::Unavailable};
  bool complete{false};
  const void *owner{nullptr};
  std::size_t modules{}, runtime_modules{}, kernel_modules{}, visited_modules{};
};
}  // namespace taichi::lang
