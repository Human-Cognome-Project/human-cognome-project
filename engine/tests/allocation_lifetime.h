#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>

// Opt-in, current-thread C++ allocation witness. Fixed storage avoids observing
// the observer. No malloc/driver/RSS/stack measurement is implied.
namespace allocation_lifetime {
struct Entry { void *pointer{}; std::size_t bytes{}; bool used{}; };
struct Counts { std::size_t attempts{}, acquired{}, released{}, live{}, peak{}, blocks{}; };
inline thread_local Entry table[65536];
inline thread_local Counts counts;
inline thread_local bool active = false;
inline thread_local std::size_t fail_at = 0;
inline std::size_t bucket(void *p) {
  return (reinterpret_cast<std::uintptr_t>(p) >> 4) % 65536;
}
inline void begin(std::size_t fail = 0) {
  if (active || counts.blocks) std::abort();
  for (auto &e : table) e = {};
  counts = {}; fail_at = fail; active = true;
}
inline void before() {
  if (active && ++counts.attempts == fail_at) throw std::bad_alloc();
}
inline void acquired(void *p, std::size_t n) {
  if (!active) return;
  for (std::size_t j = 0, i = bucket(p); j < 65536; ++j, i = (i+1)%65536) {
    auto &e = table[i];
    if (!e.pointer) {
      if (n > SIZE_MAX-counts.live) std::abort();
      e = {p,n,true}; ++counts.acquired; ++counts.blocks; counts.live += n;
      if (counts.live > counts.peak) counts.peak = counts.live;
      return;
    }
  }
  std::abort(); // Never silently undercount when fixed observer capacity is full.
}
inline void released(void *p) noexcept {
  if (!active || !p) return;
  for (std::size_t j = 0, i = bucket(p); j < 65536; ++j, i = (i+1)%65536) {
    auto &e = table[i];
    if (!e.used) return; // A pre-existing allocation is outside this scope.
    if (e.pointer == p) {
      counts.live -= e.bytes; --counts.blocks; ++counts.released;
      e.pointer = nullptr; // Keep a tombstone so collision chains remain valid.
      return;
    }
  }
}
inline Counts end() {
  if (!active) std::abort();
  active = false;
  return counts;
}
}
