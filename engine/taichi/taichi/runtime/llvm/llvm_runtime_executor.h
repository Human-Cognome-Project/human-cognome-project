#pragma once

#include <cstddef>
#include <memory>
#include <optional>

#ifdef TI_WITH_LLVM

#include "taichi/rhi/llvm/llvm_device.h"
#include "taichi/runtime/llvm/llvm_offline_cache.h"
#include "taichi/runtime/llvm/snode_tree_buffer_manager.h"
#include "taichi/runtime/llvm/llvm_context.h"
#include "taichi/struct/snode_tree.h"
#include "taichi/program/compile_config.h"
#include "taichi/program/reservation_request.h"

#include "taichi/system/threading.h"

#define TI_RUNTIME_HOST
#include "taichi/program/context.h"
#undef TI_RUNTIME_HOST

namespace taichi::lang {

namespace cuda {
class CudaDevice;
}  // namespace cuda

namespace amdgpu {
class AmdgpuDevice;
}  // namespace amdgpu

namespace cpu {
class CpuDevice;
}  // namespace cpu

class LlvmRuntimeExecutor {
 public:
  LlvmRuntimeExecutor(CompileConfig &config, KernelProfilerBase *profiler);
  virtual ~LlvmRuntimeExecutor();
  /**
   * Initializes the runtime system for LLVM based backends.
   */
  void materialize_runtime(KernelProfilerBase *profiler,
                           uint64 **result_buffer_ptr);

  // SNodeTree Allocation
  void initialize_llvm_runtime_snodes(
      const LlvmOfflineCache::FieldCacheData &field_cache_data,
      uint64 *result_buffer);

  // Ndarray and ArgPack Allocation
  DeviceAllocation allocate_memory_on_device(std::size_t alloc_size,
                                             uint64 *result_buffer);

  void deallocate_memory_on_device(DeviceAllocation handle);
  ReservationLimits reservation_limits() const;
  PoolBackingRecord reservation_pool_backing(ReservationKind kind, int node,
      ReservationList list, uint64_t index) const;
  ReservationRequest reservation_request(ReservationKind kind, int node,
      ReservationList list, uint64_t index) const;

  AllocationRecord snode_root_backing_record(int tree_id) const {
    if (!snode_tree_buffer_manager_)
      return {};
    return snode_tree_buffer_manager_->root_backing_record(tree_id);
  }

  std::optional<std::size_t> snode_root_record_storage_bytes() const {
    if (!snode_tree_buffer_manager_)
      return std::nullopt;
    return snode_tree_buffer_manager_->root_record_storage_bytes();
  }
  RuntimeBookkeeping runtime_bookkeeping() const {
    RuntimeBookkeeping result;
    result.available = true;
    result.executor_identity = this;
    result.executor_base_object_bytes = sizeof(LlvmRuntimeExecutor);
    result.thread_pool.available = true;
    if (thread_pool_) {
      auto &storage = result.thread_pool;
      storage.present = true;
      storage.owner = thread_pool_.get();
      storage.object_bytes = sizeof(ThreadPool);
      storage.thread_handles = thread_pool_->threads.size();
      if (thread_pool_->threads.capacity() > std::numeric_limits<std::size_t>::max() / sizeof(std::thread))
        storage.overflow = true;
      else
        storage.handle_capacity_bytes = thread_pool_->threads.capacity() * sizeof(std::thread);
    }
    result.tree_allocations = runtime_map_storage(snode_tree_allocs_);
    result.runtime_allocations = runtime_map_storage(allocated_runtime_memory_allocs_);
    if (snode_tree_buffer_manager_) {
      result.root_manager_identity = snode_tree_buffer_manager_.get();
      result.root_manager_object_bytes = sizeof(SNodeTreeBufferManager);
      result.root_records = snode_tree_buffer_manager_->root_metadata();
    }
    return result;
  }

  void check_runtime_error(uint64 *result_buffer);

  uint64_t *get_device_alloc_info_ptr(const DeviceAllocation &alloc);

  const CompileConfig &get_config() const {
    return config_;
  }

  TaichiLLVMContext *get_llvm_context();

  JITModule *create_jit_module(std::unique_ptr<llvm::Module> module);

  JITModule *get_runtime_jit_module();

  LLVMRuntime *get_llvm_runtime();

  Device *get_compute_device();

  LlvmDevice *llvm_device();

  void synchronize();

  bool use_device_memory_pool() {
    return use_device_memory_pool_;
  }

 private:
  /* ----------------------- */
  /* ------ Allocation ----- */
  /* ----------------------- */
  template <typename T>
  T fetch_result(int i, uint64 *result_buffer) {
    return taichi_union_cast_with_different_sizes<T>(
        fetch_result_uint64(i, result_buffer));
  }

  template <typename T>
  T fetch_result(char *result_buffer, int offset) {
    return *(T *)(result_buffer + offset);
  }

  DevicePtr get_snode_tree_device_ptr(int tree_id);

  void fill_ndarray(const DeviceAllocation &alloc,
                    std::size_t size,
                    uint32_t data);

  void *preallocate_memory(std::size_t prealloc_size,
                           DeviceAllocationUnique &devalloc);
  void preallocate_runtime_memory();

  /* ------------------------- */
  /* ---- Runtime Helpers ---- */
  /* ------------------------- */
  void print_list_manager_info(void *list_manager, uint64 *result_buffer);
  void print_memory_profiler_info(
      std::vector<std::unique_ptr<SNodeTree>> &snode_trees_,
      uint64 *result_buffer);

  template <typename T, typename... Args>
  T runtime_query(const std::string &key,
                  uint64 *result_buffer,
                  Args &&...args) {
    TI_ASSERT(arch_uses_llvm(config_.arch));

    auto runtime = get_runtime_jit_module();
    runtime->call<void *>("runtime_" + key, llvm_runtime_,
                          std::forward<Args>(args)...);
    return taichi_union_cast_with_different_sizes<T>(fetch_result_uint64(
        taichi_result_buffer_runtime_query_id, result_buffer));
  }

  /* -------------------------- */
  /* ------ Member Access ----- */
  /* -------------------------- */
  void finalize();

  uint64 fetch_result_uint64(int i, uint64 *result_buffer);
  void destroy_snode_tree(SNodeTree *snode_tree);
  std::size_t get_snode_num_dynamically_allocated(SNode *snode,
                                                  uint64 *result_buffer);

  void init_runtime_jit_module(std::unique_ptr<llvm::Module> module);

 private:
  CompileConfig &config_;
  const int snode_capacity_;
  const int snode_tree_capacity_;

  std::unique_ptr<TaichiLLVMContext> llvm_context_{nullptr};
  std::unique_ptr<JITSession> jit_session_{nullptr};
  JITModule *runtime_jit_module_{nullptr};
  void *llvm_runtime_{nullptr};
  using ReservationQuery = void (*)(void *, int, int, int, uint64_t, uint64_t *);
  ReservationQuery reservation_query_{nullptr};

  std::unique_ptr<ThreadPool> thread_pool_{nullptr};
  std::shared_ptr<Device> device_{nullptr};

  std::unique_ptr<SNodeTreeBufferManager> snode_tree_buffer_manager_{nullptr};
  template <typename Key>
  using AllocationMap = std::unordered_map<Key, DeviceAllocation, std::hash<Key>,
      std::equal_to<Key>,
      SharedRequestCountingAllocator<std::pair<const Key, DeviceAllocation>>>;
  AllocationMap<int> snode_tree_allocs_;
  DeviceAllocationUnique preallocated_runtime_objects_allocs_ = nullptr;
  DeviceAllocationUnique preallocated_runtime_memory_allocs_ = nullptr;
  AllocationMap<DeviceAllocationId> allocated_runtime_memory_allocs_;

  // good buddy
  friend LlvmProgramImpl;
  friend SNodeTreeBufferManager;

  bool use_device_memory_pool_ = false;
  bool finalized_{false};
  KernelProfilerBase *profiler_ = nullptr;
};

}  // namespace taichi::lang

#endif  // TI_WITH_LLVM
