#pragma once
#include "taichi/common/core.h"
#include "taichi/rhi/common/unified_allocator.h"
#include "taichi/rhi/device.h"
#include "taichi/rhi/common/memory_pool_stats.h"
#include "taichi/rhi/common/pool_backing.h"
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
  // Range containment only: caller independently establishes a live request.
  // Tokens do not extend pool/request lifetime. Lookups lock, but never scan.
  PoolBackingRecord capture_backing_record(const void *address, std::size_t bytes);
  PoolBackingRecord backing_record(PoolBackingToken token);
  PoolBackingStorage backing_storage();
  HostMemoryPool();
  ~HostMemoryPool();

 protected:
  void *allocate_raw_memory(std::size_t size);
  void deallocate_raw_memory(void *ptr);

  // All the raw memory allocated from OS/Driver
  // We need to keep track of them to guarantee that they are freed
  struct RawChunk {
    std::size_t bytes;
    std::uint64_t identity;
    std::size_t rounded_bytes;
  };
  std::map<std::uintptr_t, RawChunk> raw_memory_chunks_;
  const std::uint64_t pool_instance_;
  const std::size_t os_page_size_;

  std::unique_ptr<UnifiedAllocator> allocator_;
  std::mutex mut_allocation_;
  MemoryPoolStatistics statistics_;

  friend class UnifiedAllocator;
};

}  // namespace taichi::lang
