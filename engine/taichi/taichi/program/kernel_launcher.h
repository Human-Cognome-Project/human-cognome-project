#pragma once

#include "taichi/codegen/compiled_kernel_data.h"
#include "taichi/program/launch_context_builder.h"
#include "taichi/program/launch_registry_storage.h"
#include "taichi/program/registration_attempts.h"

namespace taichi::lang {

class KernelLauncher {
 public:
  using Handle = KernelLaunchHandle;
  // Synchronous observations under live quiescent launcher/runtime ownership.
  virtual LaunchRegistryStorage registry_storage() const { return {}; }
  virtual LaunchContextStorage context_storage(std::size_t index,
                                               std::size_t parameter_budget) const { return {}; }
  virtual CpuRegistrationAttemptCounts cpu_registration_attempt_counts(std::size_t entry_budget) const noexcept { return {}; }
  virtual CpuRegistrationAttempt cpu_registration_attempt(std::size_t index) const noexcept { return {}; }

  virtual void launch_kernel(const CompiledKernelData &compiled_kernel_data,
                             LaunchContextBuilder &ctx) = 0;

  virtual ~KernelLauncher() = default;
};

}  // namespace taichi::lang
