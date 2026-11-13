#include "taichi/runtime/cpu/kernel_launcher.h"
#include "taichi/rhi/arch.h"
#include <stdexcept>
#include <limits>

namespace taichi::lang {
namespace cpu {

LaunchRegistryStorage KernelLauncher::registry_storage() const {
  LaunchRegistryStorage result;
  result.owner = this;
  result.contexts = contexts_.size();
  if (contexts_.capacity() > std::numeric_limits<std::size_t>::max() / sizeof(Context)) {
    result.status = LaunchMetadataStatus::Overflow;
    return result;
  }
  result.status = LaunchMetadataStatus::Present;
  result.object_bytes = sizeof(KernelLauncher);
  result.context_capacity_bytes = contexts_.capacity() * sizeof(Context);
  return result;
}

LaunchContextStorage KernelLauncher::context_storage(std::size_t index,
                                                     std::size_t parameter_budget) const {
  LaunchContextStorage result;
  result.owner = this;
  if (index >= contexts_.size()) {
    result.status = LaunchMetadataStatus::Invalid;
    return result;
  }
  const auto &ctx = contexts_[index];
  result.required_parameters = ctx.parameters.size();
  if (parameter_budget < result.required_parameters) {
    result.status = LaunchMetadataStatus::InsufficientWork;
    return result;
  }
  const auto add = [](std::size_t &total, std::size_t count, std::size_t width) {
    const auto max = std::numeric_limits<std::size_t>::max();
    if (width && count > max / width) return false;
    const auto bytes = count * width;
    if (bytes > max - total) return false;
    total += bytes;
    return true;
  };
  result.status = LaunchMetadataStatus::Overflow;
  if (!add(result.task_capacity_bytes, ctx.task_funcs.capacity(), sizeof(Context::TaskFunc)) ||
      !add(result.parameter_capacity_bytes, ctx.parameters.capacity(),
           sizeof(decltype(ctx.parameters)::value_type))) return result;
  for (const auto &entry : ctx.parameters) {
    ++result.visited_parameters;
    if (!add(result.path_capacity_bytes, entry.first.capacity(), sizeof(int)) ||
        !add(result.element_shape_capacity_bytes, entry.second.element_shape.capacity(), sizeof(int)) ||
        !add(result.name_capacity_charge_bytes, entry.second.name.capacity(), 1) ||
        !add(result.name_capacity_charge_bytes, 1, 1)) return result;
  }
  result.status = LaunchMetadataStatus::Present;
  return result;
}

std::unique_ptr<LaunchContextBuilder::Prepared>
KernelLauncher::prepare_llvm_kernel(Handle handle, LaunchContextBuilder &ctx) {
  auto state = std::make_unique<LaunchContextBuilder::Prepared>();
  ctx.prepare_scalars(*state);
  for (const auto &entry : contexts_.at(handle.get_launch_id()).parameters) {
    if (!entry.second.is_array)
      continue;
    const auto &key = entry.first;
    auto data = key;
    data.push_back(TypeFactory::DATA_PTR_POS_IN_NDARRAY);
    auto grad = key;
    grad.push_back(TypeFactory::GRAD_PTR_POS_IN_NDARRAY);
    auto type = ctx.device_allocation_type.at(key);
    auto *ptr = ctx.array_ptrs.at(data);
    if (!ptr || ctx.array_runtime_sizes.at(key) == 0 || ctx.array_ptrs.at(grad))
      throw std::invalid_argument("invalid prepared CPU array binding");
    if (type == LaunchContextBuilder::DevAllocType::kNdarray) {
      if (static_cast<DeviceAllocation *>(ptr)->device !=
          get_runtime_executor()->llvm_device())
        throw std::invalid_argument("prepared array belongs to another device");
      ptr = get_runtime_executor()->get_device_alloc_info_ptr(
          *static_cast<DeviceAllocation *>(ptr));
    } else if (type != LaunchContextBuilder::DevAllocType::kNone)
      throw std::invalid_argument("unsupported prepared CPU binding");
    ctx.set_ndarray_ptrs(key, reinterpret_cast<uint64>(ptr), 0);
  }
  state->backing_device_instance = get_runtime_executor()->llvm_device()->allocation_instance();
  state->backing_arg_bytes = ctx.arg_buffer_size;
  state->backing_result_bytes = ctx.result_buffer_size;
  state->backing_buffers_available = true;
  return state;
}

void KernelLauncher::launch_llvm_kernel(Handle handle,
                                        LaunchContextBuilder &ctx) {
  TI_ASSERT(handle.get_launch_id() < contexts_.size());
  const auto &launcher_ctx = contexts_[handle.get_launch_id()];
  auto *executor = get_runtime_executor();

  ctx.get_context().runtime = executor->get_llvm_runtime();
  if (ctx.prepared) {
    for (auto task : launcher_ctx.task_funcs)
      task(&ctx.get_context());
    return;
  }
  // For taichi ndarrays, context.array_ptrs saves pointer to its
  // |DeviceAllocation|, CPU backend actually want to use the raw ptr here.
  const auto &parameters = launcher_ctx.parameters;
  for (int i = 0; i < (int)parameters.size(); i++) {
    const auto &kv = parameters[i];
    const auto &key = kv.first;
    const auto &parameter = kv.second;
    std::vector<int> data_ptr_idx = key;
    data_ptr_idx.push_back(TypeFactory::DATA_PTR_POS_IN_NDARRAY);
    std::vector<int> grad_ptr_idx = key;
    grad_ptr_idx.push_back(TypeFactory::GRAD_PTR_POS_IN_NDARRAY);

    if (parameter.is_array && ctx.device_allocation_type[key] ==
                                  LaunchContextBuilder::DevAllocType::kNone) {
      ctx.set_ndarray_ptrs(key, (uint64)ctx.array_ptrs[data_ptr_idx],
                           (uint64)ctx.array_ptrs[grad_ptr_idx]);
    }
    if (parameter.is_array &&
        ctx.device_allocation_type[key] !=
            LaunchContextBuilder::DevAllocType::kNone &&
        ctx.array_runtime_sizes[key] > 0) {
      DeviceAllocation *ptr =
          static_cast<DeviceAllocation *>(ctx.array_ptrs[data_ptr_idx]);
      uint64 host_ptr = (uint64)executor->get_device_alloc_info_ptr(*ptr);
      // Preserve the binding descriptor for subsequent launches. Only the
      // argument buffer gets the resolved pointer; array_ptrs still holds a
      // DeviceAllocation and must not be relabelled as an external raw pointer.
      auto grad_ptr = ctx.array_ptrs[grad_ptr_idx];
      uint64 host_ptr_grad =
          grad_ptr == nullptr ? 0
                              : (uint64)executor->get_device_alloc_info_ptr(
                                    *static_cast<DeviceAllocation *>(grad_ptr));
      ctx.set_ndarray_ptrs(key, host_ptr, host_ptr_grad);
    }
    if (parameter.is_argpack) {
      data_ptr_idx = key;
      data_ptr_idx.push_back(TypeFactory::DATA_PTR_POS_IN_ARGPACK);
      auto *argpack = ctx.argpack_ptrs[key];
      auto argpack_ptr = argpack->get_device_allocation();
      uint64 host_ptr =
          (uint64)executor->get_device_alloc_info_ptr(argpack_ptr);
      if (key.size() == 1) {
        ctx.set_argpack_ptr(key, host_ptr);
      } else {
        auto key_parent = key;
        key_parent.pop_back();
        auto *argpack_parent = ctx.argpack_ptrs[key_parent];
        argpack_parent->set_arg_nested_argpack_ptr(key.back(), host_ptr);
      }
    }
  }
  for (auto task : launcher_ctx.task_funcs) {
    task(&ctx.get_context());
  }
}

KernelLauncher::Handle KernelLauncher::register_llvm_kernel(
    const LLVM::CompiledKernelData &compiled) {
  TI_ASSERT(arch_is_cpu(compiled.arch()));

  if (!compiled.get_handle()) {
    auto handle = make_handle();
    auto index = handle.get_launch_id();
    contexts_.resize(index + 1);

    auto &ctx = contexts_[index];
    auto *executor = get_runtime_executor();

    auto data = compiled.get_internal_data().compiled_data.clone();
    auto parameters = compiled.get_internal_data().args;
    auto *jit_module = executor->create_jit_module(std::move(data.module));

    // Construct task_funcs
    using TaskFunc = int32 (*)(void *);
    std::vector<TaskFunc> task_funcs;
    task_funcs.reserve(data.tasks.size());
    for (auto &task : data.tasks) {
      auto *func_ptr = jit_module->lookup_function(task.name);
      TI_ASSERT_INFO(func_ptr, "Offloaded datum function {} not found",
                     task.name);
      task_funcs.push_back((TaskFunc)(func_ptr));
    }

    // Populate ctx
    ctx.parameters = std::move(parameters);
    ctx.task_funcs = std::move(task_funcs);

    compiled.set_handle(handle);
  }
  return *compiled.get_handle();
}

}  // namespace cpu
}  // namespace taichi::lang
