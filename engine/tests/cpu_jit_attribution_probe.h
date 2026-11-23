#pragma once

#include <charconv>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "taichi/program/callable.h"
#include "taichi/program/registration_attempts.h"
#include "llvm/ExecutionEngine/Orc/Core.h"

// Test-only proposal models. These are neither native APIs nor an operational
// association registry. The live owner supplies the authoritative issuance bound
// in the proposed design; the fixture witnesses it at intercepted native calls.
namespace cpu_jit_attribution_inspection {
using namespace taichi::lang;

struct Token {
  // Session addresses are live-owner scope, not generation/lifetime guards.
  // Only record metadata may be inspected after the original owner dies.
  const void *session{};
  std::size_t cpu_slot{};
  int native_id{-1};
  CompiledKernelIdentity record;
};
struct Presence {
  OwnershipCountStatus status{OwnershipCountStatus::Unavailable};
  bool complete{false};
  OwnershipCountStatus dylib_status{OwnershipCountStatus::Unavailable};
  const void *session{}, *dylib{};
  std::size_t charged_entries{};
};

// Native name lookup scans the session's dylib vector. Under the current CPU
// owner's exclusive unique-name creation, issued IDs bound its length, including
// holes. This is a conservative preflight model, not a measured scan count.
inline Presence probe(const Token &token, const void *live_session,
                      llvm::orc::ExecutionSession &backend,
                      std::size_t issued_bound, std::size_t entry_budget) {
  Presence result;
  result.session = live_session;
  if (!live_session || token.session != live_session || token.native_id < 0 || backend.getPlatform())
    return result;
  if (std::size_t(token.native_id) >= issued_bound) {
    result.status = OwnershipCountStatus::Absent; // Not an issued ID.
    return result;
  }
  result.status = OwnershipCountStatus::Incomplete;
  if (entry_budget < issued_bound) return result;
  char name[std::numeric_limits<int>::digits10 + 3];
  const auto formatted = std::to_chars(name, name + sizeof(name), token.native_id);
  if (formatted.ec != std::errc{}) return result;
  result.charged_entries = issued_bound;
  result.dylib = backend.getJITDylibByName(llvm::StringRef(name, formatted.ptr - name));
  result.dylib_status = result.dylib ? OwnershipCountStatus::Present : OwnershipCountStatus::Absent;
  result.status = OwnershipCountStatus::Present;
  return result;
}

// Layout candidates use the source-pinned native member types. The fixture
// reconciles CurrentSlot with actual one-slot native capacity first. sizeof
// predicts replacement storage; it does not observe a production association.
struct CurrentRegistration {
  CompiledKernelIdentity record;
  const void *module{};
  OwnershipCountStatus module_status{OwnershipCountStatus::Absent};
  CpuRegistrationStage stage{CpuRegistrationStage::Unobserved};
  bool failed{};
};
struct IdRegistration {
  CompiledKernelIdentity record;
  const void *module{};
  OwnershipCountStatus module_status{OwnershipCountStatus::Absent};
  CpuRegistrationStage stage{CpuRegistrationStage::Unobserved};
  bool failed{};
  int native_id{-1};
};
struct AppendedLinkRegistration {
  CompiledKernelIdentity record;
  const void *module{};
  OwnershipCountStatus module_status{OwnershipCountStatus::Absent};
  CpuRegistrationStage stage{CpuRegistrationStage::Unobserved};
  bool failed{};
  int native_id{-1};
  const void *dylib{};
  OwnershipCountStatus dylib_status{OwnershipCountStatus::Unavailable};
};
struct PackedLinkRegistration {
  CompiledKernelIdentity record;
  const void *module{}, *dylib{};
  int native_id{-1};
  OwnershipCountStatus module_status{OwnershipCountStatus::Absent};
  OwnershipCountStatus dylib_status{OwnershipCountStatus::Unavailable};
  CpuRegistrationStage stage{CpuRegistrationStage::Unobserved};
  bool failed{};
};
template <typename Registration> struct Slot {
  std::vector<std::int32_t (*)(void *)> tasks;
  std::vector<std::pair<std::vector<int>, Callable::Parameter>> parameters;
  Registration registration;
};
using CurrentSlot = Slot<CurrentRegistration>;
using IdSlot = Slot<IdRegistration>;
using AppendedLinkSlot = Slot<AppendedLinkRegistration>;
using PackedLinkSlot = Slot<PackedLinkRegistration>;
}  // namespace cpu_jit_attribution_inspection
