#include "taichi/rhi/common/host_memory_pool.h"

#include <memory>
#include <atomic>
#include <limits>
#include <stdexcept>

#if defined(TI_PLATFORM_UNIX)
#include <sys/mman.h>
#include <unistd.h>
#else
#include "taichi/platform/windows/windows.h"
#endif

namespace taichi::lang {

namespace {
std::size_t system_page_size() {
#if defined(TI_PLATFORM_UNIX)
  const auto size = sysconf(_SC_PAGESIZE);
  if (size <= 0) throw std::runtime_error("host page size unavailable");
  return static_cast<std::size_t>(size);
#else
  SYSTEM_INFO info{};
  GetSystemInfo(&info);
  if (!info.dwPageSize) throw std::runtime_error("host page size unavailable");
  return info.dwPageSize;
#endif
}
std::uint64_t next_pool_identity() {
  static std::atomic<std::uint64_t> next{1};
  auto value = next.load(std::memory_order_relaxed);
  while (value != std::numeric_limits<std::uint64_t>::max()) {
    if (next.compare_exchange_weak(value, value + 1, std::memory_order_relaxed))
      return value;
  }
  return 0;  // Disable identity coverage instead of wrapping.
}
void release_raw_mapping(void *ptr, std::size_t bytes) {
#if defined(TI_PLATFORM_UNIX)
  const bool failed = munmap(ptr, bytes) != 0;
#else
  const bool failed = !VirtualFree(ptr, 0, MEM_RELEASE);
#endif
  TI_ERROR_IF(failed, "Failed to free virtual memory ({} B)", bytes);
}
}

HostMemoryPool::HostMemoryPool()
    : raw_memory_chunks_(std::less<std::uintptr_t>{}, RawAllocator(record_allocation_bytes_)),
      pool_instance_(next_pool_identity()), os_page_size_(system_page_size()) {
  allocator_ = std::unique_ptr<UnifiedAllocator>(new UnifiedAllocator());

  TI_TRACE("Memory pool created. Default buffer size per allocator = {} MB",
           UnifiedAllocator::default_allocator_size / 1024 / 1024);
}

void *HostMemoryPool::allocate(std::size_t size,
                               std::size_t alignment,
                               bool exclusive) {
  std::lock_guard<std::mutex> _(mut_allocation_);

  if (!allocator_) {
    TI_ERROR("Memory pool is already destroyed");
  }
  void *ret = allocator_->allocate(size, alignment, exclusive);
  ++statistics_.allocation_calls;
  statistics_.requested_bytes += size;
  return ret;
}

HostMemoryPool::Statistics HostMemoryPool::statistics() {
  std::lock_guard<std::mutex> _(mut_allocation_);
  Statistics result;
  static_cast<MemoryPoolStatistics &>(result) = statistics_;
  for (const auto &entry : raw_memory_chunks_) {
    result.reserved_bytes += entry.second.bytes;
  }
  if (allocator_) {
    for (const auto &chunk : allocator_->chunks_) {
      result.cursor_bytes += static_cast<const char *>(chunk.head) -
                             static_cast<const char *>(chunk.data);
    }
  }
  return result;
}

PoolBackingRecord HostMemoryPool::capture_backing_record(const void *address,
                                                        std::size_t bytes) {
  std::lock_guard<std::mutex> lock(mut_allocation_);
  PoolBackingRecord result;
  if (!pool_instance_) return result;
  result.status = PoolBackingStatus::Unknown;
  if (!address || !bytes) return result;
  const auto value = reinterpret_cast<std::uintptr_t>(address);
  auto found = raw_memory_chunks_.upper_bound(value);
  if (found == raw_memory_chunks_.begin()) return result;
  --found;
  const auto offset = value - found->first;
  if (offset >= found->second.bytes || bytes > found->second.bytes - offset)
    return result;
  if (!found->second.identity) return {};
  result.status = PoolBackingStatus::Live;
  result.token = {pool_instance_, found->second.identity, found->first};
  result.raw_requested_bytes = found->second.bytes;
  result.rounded_extent_available = true;
  result.rounded_extent_bytes = found->second.rounded_bytes;
  return result;
}

PoolBackingRecord HostMemoryPool::backing_record(PoolBackingToken token) {
  std::lock_guard<std::mutex> lock(mut_allocation_);
  PoolBackingRecord result;
  if (!pool_instance_) return result;
  result.status = PoolBackingStatus::Unknown;
  if (token.pool_instance != pool_instance_ || !token.allocation) return result;
  const auto found = raw_memory_chunks_.find(token.base);
  if (found == raw_memory_chunks_.end() || found->second.identity != token.allocation)
    return result;
  result.status = PoolBackingStatus::Live;
  result.token = token;
  result.raw_requested_bytes = found->second.bytes;
  result.rounded_extent_available = true;
  result.rounded_extent_bytes = found->second.rounded_bytes;
  return result;
}

PoolBackingStorage HostMemoryPool::backing_storage() {
  std::lock_guard<std::mutex> lock(mut_allocation_);
  using OldPayload = std::pair<const std::uintptr_t, std::size_t>;
  using Payload = decltype(raw_memory_chunks_)::value_type;
  return {bool(pool_instance_), false, pool_instance_, sizeof(*this),
          raw_memory_chunks_.size() * sizeof(Payload),
          raw_memory_chunks_.size() * (sizeof(Payload) - sizeof(OldPayload)),
          true, bool(allocator_), record_allocation_bytes_,
          allocator_ ? sizeof(UnifiedAllocator) : 0,
          allocator_ ? allocator_->chunks_.capacity() * sizeof(UnifiedAllocator::MemoryChunk) : 0};
}

void HostMemoryPool::release(std::size_t size, void *ptr) {
  std::lock_guard<std::mutex> _(mut_allocation_);

  if (!allocator_) {
    TI_ERROR("Memory pool is already destroyed");
  }

  if (allocator_->release(size, ptr)) {
    if (dynamic_cast<UnifiedAllocator *>(allocator_.get())) {
      deallocate_raw_memory(ptr);  // release raw memory as well
    }
  }
}

void *HostMemoryPool::allocate_raw_memory(std::size_t size) {
  /*
    Be aware that this methods is not protected by the mutex.

    allocate_raw_memory() is designed to be a private method, and
    should only be called by its Allocators friends.

    The caller ensures that no other thread is accessing the memory pool
    when calling this method.
  */

  if (!size) throw std::invalid_argument("zero raw host mapping request");
  const auto remainder = size % os_page_size_;
  const auto padding = remainder ? os_page_size_ - remainder : 0;
  if (size > std::numeric_limits<std::size_t>::max() - padding)
    throw std::overflow_error("host mapping extent overflow");
  const auto rounded = size + padding;
  void *ptr = nullptr;
#if defined(TI_PLATFORM_UNIX)
  ptr = mmap(nullptr, rounded, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
             -1, 0);
  TI_ERROR_IF(ptr == MAP_FAILED, "Virtual memory allocation ({} B) failed.",
              size);
#else
  MEMORYSTATUSEX stat;
  stat.dwLength = sizeof(stat);
  GlobalMemoryStatusEx(&stat);
  if (stat.ullAvailVirtual < rounded) {
    TI_P(stat.ullAvailVirtual);
    TI_P(size);
    TI_ERROR("Insufficient virtual memory space");
  }
  ptr = VirtualAlloc(nullptr, rounded, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  TI_ERROR_IF(ptr == nullptr, "Virtual memory allocation ({} B) failed.", size);
#endif
  TI_ERROR_IF(((uint64_t)ptr) % page_size != 0,
              "Allocated address ({:}) is not aligned by page size {}", ptr,
              page_size);

  const auto base = reinterpret_cast<std::uintptr_t>(ptr);
  if (raw_memory_chunks_.count(base)) {
    TI_ERROR("Memory address ({:}) is already allocated", ptr);
  }

  try {
    raw_memory_chunks_.emplace(base, RawChunk{size, next_pool_identity(), rounded});
  } catch (...) {
    release_raw_mapping(ptr, rounded);
    throw;
  }
  ++statistics_.raw_allocation_calls;
  return ptr;
}

void HostMemoryPool::deallocate_raw_memory(void *ptr) {
  /*
    Be aware that this methods is not protected by the mutex.

    deallocate_raw_memory() is designed to be a private method, and
    should only be called by its Allocators friends.

    The caller ensures that no other thread is accessing the memory pool
    when calling this method.
  */
  const auto base = reinterpret_cast<std::uintptr_t>(ptr);
  if (!raw_memory_chunks_.count(base)) {
    TI_ERROR("Memory address ({:}) is not allocated", ptr);
  }

  std::size_t size = raw_memory_chunks_.at(base).rounded_bytes;
  release_raw_mapping(ptr, size);
  raw_memory_chunks_.erase(base);
}

void HostMemoryPool::reset() {
  std::lock_guard<std::mutex> _(mut_allocation_);
  allocator_ = std::unique_ptr<UnifiedAllocator>(new UnifiedAllocator());

  const auto ptr_map_copied = raw_memory_chunks_;
  for (auto &ptr : ptr_map_copied) {
    deallocate_raw_memory(reinterpret_cast<void *>(ptr.first));
  }
}

HostMemoryPool::~HostMemoryPool() {
  reset();
}

const size_t HostMemoryPool::page_size{1 << 12};  // 4 KB page size by default

HostMemoryPool &HostMemoryPool::get_instance() {
  static HostMemoryPool *memory_pool = new HostMemoryPool();
  return *memory_pool;
}

}  // namespace taichi::lang
