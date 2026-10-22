#include "taichi/runtime/llvm/kernel_launcher.h"
#include "taichi/program/program.h"
#include "taichi/runtime/program_impls/llvm/llvm_program.h"
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
  const auto *origin = ctx.originating_kernel();
  if (!origin || !origin->program)
    throw std::invalid_argument(
        "prepared context requires an originating Kernel");
  auto *program = origin->program->get_program_impl();
  auto *llvm_program = dynamic_cast<LlvmProgramImpl *>(program);
  if (!llvm_program ||
      llvm_program->get_runtime_executor() != get_runtime_executor() ||
      compiled.arch() != origin->program->compile_config().arch)
    throw std::invalid_argument("prepared context belongs to another runtime");
  if (!program->owns_compiled_kernel(*origin, data))
    throw std::invalid_argument(
        "prepared context belongs to another compiled kernel");
  // Validate the actual target ABI too: a cached key or launch ID alone cannot
  // certify the source signature (including a colliding/stale cache key).
  const auto &abi = compiled.get_internal_data();
  if (abi.args_type != origin->args_type || abi.args_type != ctx.args_type ||
      abi.args_size != origin->args_size ||
      abi.args_size != ctx.arg_buffer_size ||
      abi.ret_type != origin->ret_type || abi.ret_type != ctx.result_type() ||
      abi.ret_size != origin->ret_size ||
      abi.ret_size != ctx.result_buffer_size ||
      abi.args.size() != origin->nested_parameters.size() ||
      abi.rets.size() != origin->rets.size())
    throw std::invalid_argument("prepared context differs from target ABI");
  for (const auto &entry : abi.args) {
    const auto source = origin->nested_parameters.find(entry.first);
    if (source == origin->nested_parameters.end() ||
        !(entry.second == source->second))
      throw std::invalid_argument("prepared parameter differs from target ABI");
  }
  for (std::size_t i = 0; i < abi.rets.size(); ++i)
    if (abi.rets[i].dt != origin->rets[i].dt)
      throw std::invalid_argument("prepared return differs from target ABI");
  // All origin/runtime/ABI checks precede registration, pointer mutation and
  // backend persistent-buffer acquisition. Rejection preserves the context.
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
