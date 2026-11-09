#pragma once
#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

namespace assembly::detail {
struct ReservationBudget { std::uint64_t bytes, allocation_work; };
enum class ReservationStatus { Ok, ByteExhausted, WorkExhausted, Overflow };
struct ReservationResult {
  ReservationStatus status{ReservationStatus::Ok};
  std::uint64_t live_bytes{}, peak_bytes{}, charged_work{}, required_bytes{}, required_work{};
};
struct ReservationStop {};
// Exclusive, synchronous allocation domain. It must outlive every attached vector.
// Bytes are actual allocator requests, not allocator metadata or physical residency.
class ReservationDomain {
 public:
  explicit ReservationDomain(ReservationBudget budget) : budget_(budget) {}
  const ReservationResult &result() const { return result_; }
  void before(std::uint64_t count, std::uint64_t size) {
    if (count > UINT64_MAX/size) stop(ReservationStatus::Overflow,0,0);
    const auto n = count*size;
    if (n > UINT64_MAX-result_.live_bytes || n > UINT64_MAX-result_.charged_work)
      stop(ReservationStatus::Overflow,0,0);
    const auto bytes = result_.live_bytes+n, work = result_.charged_work+n;
    if (bytes > budget_.bytes) stop(ReservationStatus::ByteExhausted,bytes,work);
    if (work > budget_.allocation_work) stop(ReservationStatus::WorkExhausted,bytes,work);
    result_.charged_work = work; // Prepaid even if the underlying allocator throws.
  }
  void acquired(std::uint64_t n) noexcept {
    result_.live_bytes += n;
    result_.peak_bytes = std::max(result_.peak_bytes,result_.live_bytes);
  }
  void released(std::uint64_t n) noexcept { result_.live_bytes -= n; }
 private:
  [[noreturn]] void stop(ReservationStatus status, std::uint64_t bytes, std::uint64_t work) {
    result_.status = status; result_.required_bytes = bytes; result_.required_work = work;
    throw ReservationStop{};
  }
  ReservationBudget budget_;
  ReservationResult result_;
};

template<class T> struct ReservationAllocator {
  using value_type = T;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_swap = std::true_type;
  ReservationDomain *domain{};
  ReservationAllocator() = default; // Standalone setup remains outside owner admission.
  explicit ReservationAllocator(ReservationDomain *d) noexcept : domain(d) {}
  template<class U> ReservationAllocator(const ReservationAllocator<U> &a) noexcept : domain(a.domain) {}
  T *allocate(std::size_t n) {
    if (domain) domain->before(n,sizeof(T));
    auto *p = std::allocator<T>{}.allocate(n);
    if (domain) domain->acquired(n*sizeof(T));
    return p;
  }
  void deallocate(T *p, std::size_t n) noexcept {
    std::allocator<T>{}.deallocate(p,n);
    if (domain) domain->released(n*sizeof(T));
  }
  template<class U> bool operator==(const ReservationAllocator<U> &a) const noexcept { return domain == a.domain; }
  template<class U> bool operator!=(const ReservationAllocator<U> &a) const noexcept { return !(*this == a); }
};
template<class T> using ReservedVector = std::vector<T,ReservationAllocator<T>>;

// Shared pointees/control blocks may outlive the owner, including through weak
// references after pointee destruction. The rebound allocator keeps the domain
// alive through the final deallocation; the domain does not own these objects.
template<class T> struct SharedReservationAllocator {
  using value_type = T;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_swap = std::true_type;
  std::shared_ptr<ReservationDomain> domain;
  SharedReservationAllocator() = default;
  explicit SharedReservationAllocator(std::shared_ptr<ReservationDomain> d) noexcept : domain(std::move(d)) {}
  template<class U> SharedReservationAllocator(const SharedReservationAllocator<U> &a) noexcept : domain(a.domain) {}
  SharedReservationAllocator select_on_container_copy_construction() const noexcept { return {}; }
  T *allocate(std::size_t n) { return ReservationAllocator<T>(domain.get()).allocate(n); }
  void deallocate(T *p, std::size_t n) noexcept { ReservationAllocator<T>(domain.get()).deallocate(p,n); }
  template<class U> bool operator==(const SharedReservationAllocator<U> &a) const noexcept { return domain == a.domain; }
  template<class U> bool operator!=(const SharedReservationAllocator<U> &a) const noexcept { return !(*this == a); }
};

// Single-object ownership. Keep the domain alive through destruction (which may
// release attached members) and the final object deallocation, even for the owner
// that itself holds the domain. Empty handles need not retain that lifetime.
template<class T> struct ReservationDelete {
  SharedReservationAllocator<T> allocator;
  void operator()(T *p) noexcept {
    p->~T();
    allocator.deallocate(p,1);
    allocator.domain.reset();
  }
};
template<class T> using ReservedUnique = std::unique_ptr<T,ReservationDelete<T>>;

// The callback constructs T in supplied storage, allowing private constructors
// to stay private. Allocation is admitted/acquired before callback evaluation;
// constructor arguments and nested allocations are not implicitly covered.
template<class T, class Construct>
ReservedUnique<T> reserve_unique(const std::shared_ptr<ReservationDomain> &domain, Construct construct) {
  SharedReservationAllocator<T> allocator(domain);
  auto *p = allocator.allocate(1);
  try { construct(p); }
  catch (...) { allocator.deallocate(p,1); throw; }
  return ReservedUnique<T>(p,ReservationDelete<T>{std::move(allocator)});
}
}
