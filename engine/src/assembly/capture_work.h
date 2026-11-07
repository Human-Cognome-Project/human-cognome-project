#pragma once
#include "assembly/letter_projection.h"

namespace assembly::detail {
enum class CaptureStatus { Ok, WorkExhausted, Overflow };
struct CaptureResult {
  CaptureStatus status{CaptureStatus::Ok};
  std::uint64_t charged{}, next_required{};
};
// Internal unwind token; caught at capture, never exposed to its caller.
struct CaptureStop { CaptureResult result; };
class CaptureWork {
 public:
  explicit CaptureWork(std::uint64_t limit) : limit_(limit) {}
  void charge(std::uint64_t n) {
    if (n > UINT64_MAX-used_) throw CaptureStop{{CaptureStatus::Overflow,used_,0}};
    if (n > limit_-used_) throw CaptureStop{{CaptureStatus::WorkExhausted,used_,used_+n}};
    used_ += n;
  }
  void elements(std::uint64_t count, std::uint64_t size) {
    if (count > UINT64_MAX/size) throw CaptureStop{{CaptureStatus::Overflow,used_,0}};
    charge(count*size);
  }
  template<class T> bool equal(const T &a, const T &b) {
    charge(1); payload(a); payload(b); return a == b;
  }
  CaptureResult result() const { return {CaptureStatus::Ok,used_,0}; }
 private:
  void payload(const warm::Uuid &u) { charge(u.str().size()); }
  void payload(const warm::ObjectKey &o) {
    payload(o.scope); elements(o.address.size(),sizeof(codec::AddressElement));
  }
  void payload(const warm::ToolRef &r) { payload(r.object); payload(r.revision); }
  void payload(const OccurrenceKey &k) {
    payload(k.study); elements(k.source.path.size(),sizeof(std::uint32_t));
  }
  std::uint64_t limit_, used_{};
};
}
