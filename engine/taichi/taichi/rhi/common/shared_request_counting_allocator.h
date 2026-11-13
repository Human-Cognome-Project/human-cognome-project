#pragma once
#include "taichi/rhi/common/request_counting_allocator.h"

namespace taichi::lang {
struct SharedRequestCounter {
  std::size_t live_bytes{0}, control_block_bytes{0};
};

// The shared_ptr constructor keeps the counter alive during allocation. Its
// deleter destroys that counter before control-block deallocation, so deallocate
// must NOT touch the observation pointer. One control-block request is recorded.
template <typename T>
class CounterControlBlockAllocator {
 public:
  using value_type = T;
  std::size_t *request;
  explicit CounterControlBlockAllocator(std::size_t &bytes) noexcept : request(&bytes) {}
  template <typename U>
  CounterControlBlockAllocator(const CounterControlBlockAllocator<U> &other) noexcept
      : request(other.request) {}
  T *allocate(std::size_t n) {
    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
      throw std::bad_alloc();
    auto *p = std::allocator<T>{}.allocate(n);
    *request = n * sizeof(T);
    return p;
  }
  void deallocate(T *p, std::size_t n) noexcept {
    std::allocator<T>{}.deallocate(p, n);
  }
  template <typename U>
  bool operator==(const CounterControlBlockAllocator<U> &other) const noexcept {
    return request == other.request;
  }
  template <typename U>
  bool operator!=(const CounterControlBlockAllocator<U> &other) const noexcept {
    return !(*this == other);
  }
};

// Stable shared accounting domain for movable containers. Rebind/copy allocator
// objects share the domain; copying a container selects a fresh domain. Callers
// synchronize all containers sharing a domain, including moved-from containers.
template <typename T>
class SharedRequestCountingAllocator {
 public:
  using value_type = T;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_swap = std::true_type;
  SharedRequestCountingAllocator() {
    auto *counter = new SharedRequestCounter{};
    state_ = std::shared_ptr<SharedRequestCounter>(counter,
        std::default_delete<SharedRequestCounter>{},
        CounterControlBlockAllocator<SharedRequestCounter>(counter->control_block_bytes));
  }
  // Copy even on allocator move so a moved-from container retains a valid domain.
  SharedRequestCountingAllocator(const SharedRequestCountingAllocator &) noexcept = default;
  SharedRequestCountingAllocator &operator=(const SharedRequestCountingAllocator &) noexcept = default;
  template <typename U>
  SharedRequestCountingAllocator(const SharedRequestCountingAllocator<U> &other) noexcept
      : state_(other.state_) {}
  SharedRequestCountingAllocator select_on_container_copy_construction() const {
    return SharedRequestCountingAllocator{};
  }
  T *allocate(std::size_t n) {
    return RequestCountingAllocator<T>(state_->live_bytes).allocate(n);
  }
  void deallocate(T *p, std::size_t n) noexcept {
    RequestCountingAllocator<T>(state_->live_bytes).deallocate(p, n);
  }
  const SharedRequestCounter *domain() const noexcept { return state_.get(); }
  template <typename U>
  bool operator==(const SharedRequestCountingAllocator<U> &other) const noexcept {
    return state_ == other.state_;
  }
  template <typename U>
  bool operator!=(const SharedRequestCountingAllocator<U> &other) const noexcept {
    return !(*this == other);
  }
 private:
  template <typename> friend class SharedRequestCountingAllocator;
  std::shared_ptr<SharedRequestCounter> state_;
};
}  // namespace taichi::lang
