#pragma once

#include <array>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include "engine/transfer.h"

#ifdef HCP_TRANSFER_AUDIT
// Linux test executable only. Fixed storage is allocated before observing the
// nonallocating boundary. A full journal fails closed rather than losing calls.
namespace transfer_audit {
struct Event {
  taichi::lang::Ndarray *array;
  const void *host;
  std::size_t bytes, offset;
  bool upload;
};
struct Journal {
  std::array<Event, 256> events{};
  std::array<unsigned char, 2 * 1024 * 1024> payload{};
  std::size_t count{}, used{};
  void record(taichi::lang::Ndarray &array, const void *host,
              std::size_t bytes, bool upload) {
    if (count == events.size() || bytes > payload.size() - used)
      std::abort();
    events[count++] = {&array, host, bytes, used, upload};
    std::memcpy(payload.data() + used, host, bytes);
    used += bytes;
  }
};
inline thread_local Journal *active{};
struct Scope {
  explicit Scope(Journal &journal) {
    if (active) std::abort();
    active = &journal;
  }
  ~Scope() { active = nullptr; }
  Scope(const Scope &) = delete;
  Scope &operator=(const Scope &) = delete;
};
}  // namespace transfer_audit

extern "C" void hcp_real_upload(taichi::lang::Ndarray &, const void *, std::size_t)
    __asm__("__real__ZN6engine6uploadERN6taichi4lang7NdarrayEPKvm");
extern "C" void hcp_wrap_upload(taichi::lang::Ndarray &, const void *, std::size_t)
    __asm__("__wrap__ZN6engine6uploadERN6taichi4lang7NdarrayEPKvm");
extern "C" void hcp_wrap_upload(taichi::lang::Ndarray &a, const void *p, std::size_t n) {
  hcp_real_upload(a, p, n);
  if (transfer_audit::active) transfer_audit::active->record(a, p, n, true);
}
extern "C" void hcp_real_readback(taichi::lang::Ndarray &, void *, std::size_t)
    __asm__("__real__ZN6engine8readbackERN6taichi4lang7NdarrayEPvm");
extern "C" void hcp_wrap_readback(taichi::lang::Ndarray &, void *, std::size_t)
    __asm__("__wrap__ZN6engine8readbackERN6taichi4lang7NdarrayEPvm");
extern "C" void hcp_wrap_readback(taichi::lang::Ndarray &a, void *p, std::size_t n) {
  hcp_real_readback(a, p, n);
  if (transfer_audit::active) transfer_audit::active->record(a, p, n, false);
}
#endif
