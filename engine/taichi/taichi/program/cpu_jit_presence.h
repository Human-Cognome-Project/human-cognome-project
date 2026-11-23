#pragma once

#include <cstddef>

#include "taichi/program/compiler_jit_counts.h"

namespace taichi::lang {
// Read through the current live, serialized/quiescent owner. Addresses are opaque
// observations, not lifetime/epoch guards. This never establishes cleanup or B.
struct CpuJitPresence {
  OwnershipCountStatus status{OwnershipCountStatus::Unavailable};
  bool complete{false};
  const void *owner{};  // Existing native JIT session, not the CPU launcher.
  int native_id{-1};
  OwnershipCountStatus id_status{OwnershipCountStatus::Unavailable};
  OwnershipCountStatus dylib_status{OwnershipCountStatus::Unavailable};
  const void *dylib{};
  std::size_t issued_ids{}, charged_entries{};
  // Present + id Absent means this attempt received no ID. An issued ID with
  // dylib Absent is distinct. Short work keeps the authoritative bound but
  // reports no dylib/charge. complete remains false for all backend resources.
};
}  // namespace taichi::lang
