#pragma once
#include <limits>
#include <memory>
#include <new>

namespace taichi::lang {
// Tracks successful live C++ allocation requests, not malloc usable size or
// physical residency. Owner synchronizes access and outlives every rebound copy.
template <typename T>
class RequestCountingAllocator {
 public:
  using value_type = T;
  std::size_t *live_bytes;
  explicit RequestCountingAllocator(std::size_t &bytes) noexcept : live_bytes(&bytes) {}
  template <typename U>
  RequestCountingAllocator(const RequestCountingAllocator<U> &other) noexcept
      : live_bytes(other.live_bytes) {}
  T *allocate(std::size_t n) {
    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
      throw std::bad_alloc();
    const auto bytes = n * sizeof(T);
    if (*live_bytes > std::numeric_limits<std::size_t>::max() - bytes)
      throw std::bad_alloc();
    auto *result = std::allocator<T>{}.allocate(n);
    *live_bytes += bytes;
    return result;
  }
  void deallocate(T *ptr, std::size_t n) noexcept {
    std::allocator<T>{}.deallocate(ptr,n);
    *live_bytes -= n * sizeof(T);
  }
  template <typename U>
  bool operator==(const RequestCountingAllocator<U> &other) const noexcept {
    return live_bytes == other.live_bytes;
  }
  template <typename U>
  bool operator!=(const RequestCountingAllocator<U> &other) const noexcept {
    return !(*this == other);
  }
};
}  // namespace taichi::lang
