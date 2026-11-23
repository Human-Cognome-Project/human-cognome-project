#pragma once

#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "taichi/program/callable.h"
#include "taichi/program/registration_attempts.h"
#include "llvm/ExecutionEngine/Orc/Core.h"

// Test-only identity observer and compiled layout models. Production presence
// queries now use the real native owner API; no test-side query model remains.
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
