#include "engine/runtime.h"

#include "taichi/program/compile_config.h"

#include <stdexcept>
#include <string>

namespace engine {

using namespace taichi::lang;

Runtime::Runtime(const InstanceSettings &settings) : settings_(settings) {
#if !defined(TI_WITH_CUDA)
  if (settings_.cuda) {
    throw std::runtime_error(
        "CUDA requested, but the matched Taichi build was compiled without CUDA");
  }
#endif
  // A Program copies the global default configuration when it is constructed,
  // so any setting has to be in place before that call. The global is saved
  // and restored around it, so one instance settings do not leak into the
  // next one in the same process.
  const CompileConfig saved = default_compile_config;
  if (settings_.read_environment) {
    // Only the C API calls this for itself; a native caller has to.
    default_compile_config.apply_llvm_runtime_environment();
  }
  if (settings_.snode_capacity) {
    default_compile_config.llvm_snode_capacity = *settings_.snode_capacity;
  }
  if (settings_.snode_tree_capacity) {
    default_compile_config.llvm_snode_tree_capacity =
        *settings_.snode_tree_capacity;
  }
  if (settings_.device_memory_gb) {
    default_compile_config.device_memory_GB = *settings_.device_memory_gb;
  }
  if (settings_.cpu_threads) {
    default_compile_config.cpu_max_num_threads = *settings_.cpu_threads;
  }
  try {
    program_ = std::make_unique<Program>(settings_.cuda ? taichi::Arch::cuda
                                                        : taichi::Arch::x64);
  } catch (const std::string &error) {
    // The fork reports fatal errors (for example a CUDA driver that refuses
    // to initialize because no device is visible) by throwing a bare string.
    default_compile_config = saved;
    throw std::runtime_error("engine initialization failed: " + error);
  } catch (...) {
    default_compile_config = saved;
    throw;
  }
  default_compile_config = saved;
  program_->materialize_runtime();
}

Runtime::~Runtime() {
  if (program_) {
    program_->finalize();
  }
}

void Runtime::synchronize() {
  program_->synchronize();
}

int Runtime::cpu_threads() const {
  return program_->compile_config().cpu_max_num_threads;
}

int Runtime::issued_snode_ids() const {
  return SNode::counter;
}

CompiledKernel::CompiledKernel(Runtime &runtime, const std::string &name,
                               std::unique_ptr<Block> body,
                               const std::vector<Param> &params, MathMode math)
    : runtime_(&runtime) {
  Program &program = runtime.program();
  kernel_ = std::make_unique<Kernel>(program, std::move(body), name);
  for (const Param &param : params) {
    if (param.is_array) {
      kernel_->insert_ndarray_param(param.dt, param.total_dim);
    } else {
      kernel_->insert_scalar_param(param.dt);
    }
  }
  kernel_->finalize_params();
  kernel_->finalize_rets();
  auto config = program.compile_config();
  if (math == MathMode::Strict) config.fast_math = false;
  compiled_ =
      &program.compile_kernel(config, program.get_device_caps(), *kernel_);
}

LaunchContextBuilder CompiledKernel::make_context() {
  return kernel_->make_launch_context();
}

void CompiledKernel::launch(LaunchContextBuilder &ctx) {
  runtime_->program().launch_kernel(*compiled_, ctx);
}

}  // namespace engine
