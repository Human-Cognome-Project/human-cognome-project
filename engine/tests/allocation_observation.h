#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <new>
#ifdef HCP_ALLOCATION_LIFETIME
#include "allocation_lifetime.h"
#endif

// Counts C++ new/new[] in the linked executable, including worker threads.
// It does not observe arbitrary malloc, physical page faults or CUDA driver
// APIs.
namespace allocation_observation {
std::atomic<bool> enabled{false};
std::atomic<std::uint64_t> calls{0}, bytes{0};
void record(std::size_t n) {
#ifdef HCP_ALLOCATION_LIFETIME
  allocation_lifetime::before();
#endif
  if (enabled.load(std::memory_order_relaxed)) {
    calls.fetch_add(1, std::memory_order_relaxed);
    bytes.fetch_add(n, std::memory_order_relaxed);
  }
}
void *allocate(std::size_t n) {
  record(n);
  if (void *p = std::malloc(std::max<std::size_t>(n, 1))) {
#ifdef HCP_ALLOCATION_LIFETIME
    allocation_lifetime::acquired(p,n);
#endif
    return p;
  }
  throw std::bad_alloc();
}
}  // namespace allocation_observation
static void observed_free(void *p) noexcept {
#ifdef HCP_ALLOCATION_LIFETIME
  allocation_lifetime::released(p);
#endif
  std::free(p);
}
void *operator new(std::size_t n) {
  return allocation_observation::allocate(n);
}
void *operator new[](std::size_t n) {
  return allocation_observation::allocate(n);
}
void operator delete(void *p) noexcept {
  observed_free(p);
}
void operator delete[](void *p) noexcept {
  observed_free(p);
}
void operator delete(void *p, std::size_t) noexcept {
  observed_free(p);
}
void operator delete[](void *p, std::size_t) noexcept {
  observed_free(p);
}
void *operator new(std::size_t n, std::align_val_t a) {
  allocation_observation::record(n);
  void *p = nullptr;
  if (posix_memalign(&p, static_cast<std::size_t>(a),
                     std::max<std::size_t>(n, 1)) == 0) {
#ifdef HCP_ALLOCATION_LIFETIME
    allocation_lifetime::acquired(p,n);
#endif
    return p;
  }
  throw std::bad_alloc();
}
void *operator new[](std::size_t n, std::align_val_t a) {
  return ::operator new(n, a);
}
void operator delete(void *p, std::align_val_t) noexcept {
  observed_free(p);
}
void operator delete[](void *p, std::align_val_t) noexcept {
  observed_free(p);
}
void operator delete(void *p, std::size_t, std::align_val_t) noexcept {
  observed_free(p);
}
void operator delete[](void *p, std::size_t, std::align_val_t) noexcept {
  observed_free(p);
}
