#pragma once

#include <cstddef>
#include <cstdint>

#include "taichi/program/compiled_kernel_borrow.h"

namespace taichi::lang {
enum class CpuRegistrationStage : std::uint8_t {
  Unobserved, RowAllocated, IrCloned, CreatingModule, ModuleReturned, Published
};

// Queries require live, serialized/quiescent launcher/session ownership.
// The record identity alone may outlive finalization/Runtime: it retains only
// invalidatable metadata. owner/module are opaque, non-owning live-owner IDs.
// Present describes known association, never eligibility or complete resources.
struct CpuRegistrationAttempt {
  OwnershipCountStatus status{OwnershipCountStatus::Unavailable};
  bool complete{false};
  const void *owner{};
  std::size_t id{};  // Existing append-only launch-slot ID, scoped to owner.
  CompiledKernelIdentity record;
  CpuRegistrationStage stage{CpuRegistrationStage::Unobserved};
  bool failed{false};
  // Absent before the factory; Incomplete before entering it and on an error
  // without a returned wrapper; Present identifies only a confirmed wrapper.
  OwnershipCountStatus module_status{OwnershipCountStatus::Unavailable};
  const void *module{};
  int native_id{-1}; // Session-scoped; -1 means no ID was issued to this row.
};

struct CpuRegistrationAttemptCounts {
  OwnershipCountStatus status{OwnershipCountStatus::Unavailable};
  bool complete{false};
  const void *owner{};
  std::size_t issued_attempts{}, stored_slots{}, visited_attempts{};
  std::size_t recorded_attempts{}, missing_metadata{}, unknown_records{};
  std::size_t failed_attempts{}, published_attempts{}, known_modules{}, uncertain_modules{};
  // Short work reports issued/stored sizes but no visited/role counts. After a
  // full scan, Incomplete retains known counts but flags missing identity/history
  // or uncertain JIT side effects. complete remains false for backend resources.
};
}  // namespace taichi::lang
