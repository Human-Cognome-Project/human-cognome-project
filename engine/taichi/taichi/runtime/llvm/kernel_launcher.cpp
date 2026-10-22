#include "taichi/runtime/llvm/kernel_launcher.h"
#include <stdexcept>

namespace taichi::lang {
namespace LLVM {

KernelLauncher::KernelLauncher(Config config) : config_(std::move(config)) {
}

void KernelLauncher::launch_kernel(
    const lang::CompiledKernelData &compiled_kernel_data,
    LaunchContextBuilder &ctx) {
  TI_ASSERT(arch_uses_llvm(compiled_kernel_data.arch()));
  const auto &llvm_ckd =
      dynamic_cast<const LLVM::CompiledKernelData &>(compiled_kernel_data);
  auto handle = register_llvm_kernel(llvm_ckd);
  if (ctx.prepared &&
      (!ctx.prepared->usable || ctx.prepared->owner != get_runtime_executor() ||
       ctx.prepared->launch_id != handle.get_launch_id()))
    throw std::invalid_argument(
        "prepared context belongs to another kernel/runtime");
  try {
    launch_llvm_kernel(handle, ctx);
  } catch (...) {
    if (ctx.prepared)
      ctx.prepared->usable = false;
    throw;
  }
}

void KernelLauncher::prepare_kernel(const lang::CompiledKernelData &data,
                                    LaunchContextBuilder &ctx) {
  if (ctx.prepared)
    throw std::logic_error("context is already prepared");
  const auto &compiled = dynamic_cast<const LLVM::CompiledKernelData &>(data);
  auto handle = register_llvm_kernel(compiled);
  auto state = prepare_llvm_kernel(handle, ctx);
  state->owner = get_runtime_executor();
  state->launch_id = handle.get_launch_id();
  ctx.prepared = std::move(state);
}

std::unique_ptr<LaunchContextBuilder::Prepared>
KernelLauncher::prepare_llvm_kernel(Handle, LaunchContextBuilder &) {
  throw std::invalid_argument("prepared launch unsupported on this backend");
}

}  // namespace LLVM
}  // namespace taichi::lang
