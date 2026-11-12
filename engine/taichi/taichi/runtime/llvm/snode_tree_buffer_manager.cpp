#include "snode_tree_buffer_manager.h"
#include "taichi/runtime/llvm/llvm_runtime_executor.h"
#include <stdexcept>

namespace taichi::lang {

SNodeTreeBufferManager::SNodeTreeBufferManager(
    LlvmRuntimeExecutor *runtime_exec)
    : runtime_exec_(runtime_exec) {
  TI_TRACE("SNode tree buffer manager created.");
}

Ptr SNodeTreeBufferManager::allocate(std::size_t size,
                                     const int snode_tree_id,
                                     uint64 *result_buffer) {
  if (snode_tree_id_to_device_alloc_.count(snode_tree_id))
    throw std::invalid_argument("SNode root already allocated");
  auto devalloc = runtime_exec_->allocate_memory_on_device(size, result_buffer);
  try {
    auto record = devalloc.device->capture_allocation_record(devalloc);
    snode_tree_id_to_device_alloc_.emplace(snode_tree_id, RootBacking{devalloc, record});
  } catch (...) {
    runtime_exec_->deallocate_memory_on_device(devalloc);
    throw;
  }
  return (Ptr)runtime_exec_->get_device_alloc_info_ptr(devalloc);
}

void SNodeTreeBufferManager::destroy(SNodeTree *snode_tree) {
  auto found = snode_tree_id_to_device_alloc_.find(snode_tree->id());
  if (found == snode_tree_id_to_device_alloc_.end())
    return;
  runtime_exec_->deallocate_memory_on_device(found->second.allocation);
  snode_tree_id_to_device_alloc_.erase(found);
}

AllocationRecord SNodeTreeBufferManager::root_backing_record(int tree_id) const {
  const auto found = snode_tree_id_to_device_alloc_.find(tree_id);
  if (found == snode_tree_id_to_device_alloc_.end()) {
    AllocationRecord unknown;
    unknown.status = AllocationRecordStatus::Unknown;
    return unknown;
  }
  const auto &record = found->second.record;
  if (record.status == AllocationRecordStatus::Unavailable)
    return {};
  return runtime_exec_->get_compute_device()->allocation_record(record.token);
}

}  // namespace taichi::lang
