#pragma once
#include "taichi/common/core.h"
#include "taichi/rhi/common/unified_allocator.h"
#include "taichi/rhi/device.h"
#include "taichi/rhi/common/memory_pool_stats.h"
#include <mutex>
#include <vector>
#include <memory>
#include <thread>

namespace taichi::lang {

// A memory pool that runs on the host

class TI_DLL_EXPORT HostMemoryPool {
 public:
  struct Statistics : MemoryPoolStatistics {
    // Native bump cursors include alignment gaps and exclusive allocations.
    std::size_t cursor_bytes{0};
  };
  static const size_t page_size;

  static HostMemoryPool &get_instance();

  void *allocate(std::size_t size,
                 std::size_t alignment,
                 bool exclusive = false);
  void release(std::size_t size, void *ptr);
  void reset();
  Statistics statistics();
  HostMemoryPool();
  ~HostMemoryPool();

 protected:
  void *allocate_raw_memory(std::size_t size);
  void deallocate_raw_memory(void *ptr);

  // All the raw memory allocated from OS/Driver
  // We need to keep track of them to guarantee that they are freed
  std::map<void *, std::size_t> raw_memory_chunks_;

  std::unique_ptr<UnifiedAllocator> allocator_;
  std::mutex mut_allocation_;
  MemoryPoolStatistics statistics_;

  friend class UnifiedAllocator;
};

}  // namespace taichi::lang
