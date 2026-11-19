#pragma once

#include <array>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include "engine/transfer.h"
#include "engine/runtime.h"
#include "field/centroid_cascade.h"
#include "field/live_hierarchy.h"

#ifdef HCP_TRANSFER_AUDIT
// Linux test executable only. Fixed storage is allocated before observing the
// nonallocating boundary. A full journal fails closed rather than losing calls.
namespace transfer_audit {
struct Event {
  taichi::lang::Ndarray *array;
  const void *host;
  std::size_t bytes, offset;
  bool upload;
  bool initialization{}, copy{};
};
struct Journal {
  std::array<Event, 256> events{};
  std::array<unsigned char, 2 * 1024 * 1024> payload{};
  std::size_t count{}, used{};
  enum class Fault { None, BeforeCopy, AfterCopy, FinalSync };
  Fault fault{Fault::None};
  bool legacy{}, copied{};
  std::size_t refresh_commits{};
  std::uint64_t refresh_copy_bytes{};
  field::CascadeResult query{};
  void record(taichi::lang::Ndarray &array, const void *host,
              std::size_t bytes, bool upload) {
    if (count == events.size() || bytes > payload.size() - used)
      std::abort();
    events[count++] = {&array, host, bytes, used, upload};
    std::memcpy(payload.data() + used, host, bytes);
    used += bytes;
  }
  void native(taichi::lang::Ndarray &array, std::size_t bytes, bool initialize) {
    if (count == events.size()) std::abort();
    events[count++] = {&array, nullptr, bytes, used, false, initialize, !initialize};
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
extern "C" void hcp_real_fill(taichi::lang::Program *, taichi::lang::Ndarray *, std::uint32_t)
    __asm__("__real__ZN6taichi4lang7Program21fill_ndarray_fast_u32EPNS0_7NdarrayEj");
extern "C" void hcp_wrap_fill(taichi::lang::Program *, taichi::lang::Ndarray *, std::uint32_t)
    __asm__("__wrap__ZN6taichi4lang7Program21fill_ndarray_fast_u32EPNS0_7NdarrayEj");
extern "C" void hcp_wrap_fill(taichi::lang::Program *p, taichi::lang::Ndarray *a, std::uint32_t v) {
  hcp_real_fill(p, a, v);
  if (transfer_audit::active) {
    if (v) std::abort();
    transfer_audit::active->native(*a, a->get_nelement() * a->get_element_size(), true);
  }
}
extern "C" void hcp_real_copy(field::detail::CentroidCascade *, const field::CascadeResult &,
                               taichi::lang::Ndarray &)
    __asm__("__real__ZN5field6detail15CentroidCascade14copy_publishedERKNS_13CascadeResultERN6taichi4lang7NdarrayE");
extern "C" void hcp_wrap_copy(field::detail::CentroidCascade *, const field::CascadeResult &,
                               taichi::lang::Ndarray &)
    __asm__("__wrap__ZN5field6detail15CentroidCascade14copy_publishedERKNS_13CascadeResultERN6taichi4lang7NdarrayE");
extern "C" void hcp_wrap_copy(field::detail::CentroidCascade *owner, const field::CascadeResult &q,
                               taichi::lang::Ndarray &destination) {
  auto *journal = transfer_audit::active;
  using Fault = transfer_audit::Journal::Fault;
  if (journal && journal->fault == Fault::BeforeCopy)
    throw std::runtime_error("test native seed copy entry failure");
  if (journal && journal->legacy) {
    // Test-only reference substitution: the former host mapping and upload.
    // All storage is fixed before the nonallocating installation boundary.
    const auto &source = owner->inspect(q);
    const int H = destination.shape[1];
    if (H > 70) std::abort();
    std::array<double, 8 * 70> seed{};
    for (int bank = 0; bank < 2; ++bank)
      for (int column = 0; column < 4; ++column)
        for (std::size_t row = 0; row < q.nodes; ++row)
          seed[(bank * 4 + column) * H + row] = source.values[(column + 3) * H + row];
    engine::upload(destination, seed.data(), std::size_t(8 * H) * sizeof(double));
  } else {
    hcp_real_copy(owner, q, destination);
    if (journal) journal->native(destination, q.nodes * 64, false);
  }
  if (journal) {
    journal->query = q;
    journal->copied = true;
    if (journal->fault == Fault::AfterCopy)
      throw std::runtime_error("test native seed copy completion failure");
  }
}
extern "C" void hcp_real_sync(engine::Runtime *)
    __asm__("__real__ZN6engine7Runtime11synchronizeEv");
extern "C" void hcp_wrap_sync(engine::Runtime *)
    __asm__("__wrap__ZN6engine7Runtime11synchronizeEv");
extern "C" void hcp_wrap_sync(engine::Runtime *r) {
  hcp_real_sync(r);
  if (transfer_audit::active && transfer_audit::active->copied &&
      transfer_audit::active->fault == transfer_audit::Journal::Fault::FinalSync)
    throw std::runtime_error("test final installation synchronization failure");
}
extern "C" void hcp_real_refresh_commit(field::detail::LiveHierarchy *, field::UpdateResult &)
    __asm__("__real__ZN5field6detail13LiveHierarchy14commit_refreshERNS_12UpdateResultE");
extern "C" void hcp_wrap_refresh_commit(field::detail::LiveHierarchy *, field::UpdateResult &)
    __asm__("__wrap__ZN5field6detail13LiveHierarchy14commit_refreshERNS_12UpdateResultE");
extern "C" void hcp_wrap_refresh_commit(field::detail::LiveHierarchy *owner, field::UpdateResult &out) {
  auto *journal = transfer_audit::active;
  using Fault = transfer_audit::Journal::Fault;
  if (journal && journal->fault == Fault::BeforeCopy)
    throw std::runtime_error("test refresh commit entry failure after claims");
  const auto before = out.native_copy_bytes;
  hcp_real_refresh_commit(owner, out);
  if (journal) {
    ++journal->refresh_commits;
    journal->refresh_copy_bytes += out.native_copy_bytes - before;
    journal->copied = true;
    if (journal->fault == Fault::AfterCopy)
      throw std::runtime_error("test refresh commit completion failure");
  }
}
#endif
