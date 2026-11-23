#pragma once

#include "taichi/codegen/llvm/compiled_kernel_data.h"
#include "taichi/runtime/llvm/kernel_launcher.h"

namespace taichi::lang {
namespace cpu {

class KernelLauncher : public LLVM::KernelLauncher {
  using Base = LLVM::KernelLauncher;

  struct Context {
    using TaskFunc = int32 (*)(void *);
    std::vector<TaskFunc> task_funcs;
    std::vector<std::pair<std::vector<int>, Callable::Parameter>> parameters;
    struct Registration {
      CompiledKernelIdentity record;
      const void *module{};
      OwnershipCountStatus module_status{OwnershipCountStatus::Absent};
      CpuRegistrationStage stage{CpuRegistrationStage::Unobserved};
      bool failed{false};
      int native_id{-1};
    } registration;
  };

 public:
  using Base::Base;
  LaunchRegistryStorage registry_storage() const override;
  LaunchContextStorage context_storage(std::size_t index,
                                       std::size_t parameter_budget) const override;
  CpuRegistrationAttemptCounts cpu_registration_attempt_counts(std::size_t entry_budget) const noexcept override;
  CpuRegistrationAttempt cpu_registration_attempt(std::size_t index) const noexcept override;
  CpuJitPresence cpu_registration_jit_presence(std::size_t index,
                                               std::size_t entry_budget) const override;

  void launch_llvm_kernel(Handle handle, LaunchContextBuilder &ctx) override;
  std::unique_ptr<LaunchContextBuilder::Prepared> prepare_llvm_kernel(
      Handle handle,
      LaunchContextBuilder &ctx) override;
  Handle register_llvm_kernel(
      const LLVM::CompiledKernelData &compiled) override;

 private:
  std::vector<Context> contexts_;
  std::size_t issued_attempts_{0};
};

}  // namespace cpu
}  // namespace taichi::lang
