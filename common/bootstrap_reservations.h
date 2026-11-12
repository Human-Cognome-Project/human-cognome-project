#pragma once
#include "reservation_allocator.h"
#include <utility>

namespace hcp::resources {
// Exclusive/synchronous, like ReservationDomain. The outer tracker charges its
// own single allocation before construction. Intrusive handles avoid creating
// another unaccounted shared control block to track the first control block.
class BootstrapReservations {
  struct State {
    ReservationDomain requests;
    std::size_t references{1};
    explicit State(const ReservationDomain &d) noexcept : requests(d) {}
  };
  State *state_{};
  explicit BootstrapReservations(State *s) noexcept : state_(s) {}
 public:
  BootstrapReservations() = default;
  BootstrapReservations(const BootstrapReservations &other) noexcept : state_(other.state_) {
    if (state_) ++state_->references;
  }
  BootstrapReservations(BootstrapReservations &&other) noexcept
      : state_(std::exchange(other.state_,nullptr)) {}
  BootstrapReservations &operator=(BootstrapReservations other) noexcept {
    std::swap(state_,other.state_); return *this;
  }
  ~BootstrapReservations() {
    if (state_ && !--state_->references) {
      state_->requests.released(sizeof(State));
      state_->~State();
      std::allocator<State>{}.deallocate(state_,1);
    }
  }
  explicit operator bool() const noexcept { return state_ != nullptr; }
  bool operator==(const BootstrapReservations &other) const noexcept { return state_ == other.state_; }
  static BootstrapReservations create(ReservationBudget budget, ReservationResult &result) {
    ReservationDomain requests(budget);
    try {
      requests.before(1,sizeof(State));
      auto *p = std::allocator<State>{}.allocate(1);
      requests.acquired(sizeof(State));
      ::new (static_cast<void *>(p)) State(requests);
      result = requests.result();
      return BootstrapReservations(p);
    } catch (...) {
      result = requests.result();
      throw;
    }
  }
  // Snapshot only; no caller-owned result reference is retained by allocators.
  ReservationResult result() const noexcept { return state_ ? state_->requests.result() : ReservationResult{}; }
  template<class T> friend struct BootstrapAllocator;
};

template<class T> struct BootstrapAllocator {
  using value_type = T;
  BootstrapReservations tracker;
  explicit BootstrapAllocator(BootstrapReservations t) noexcept : tracker(std::move(t)) {}
  template<class U> BootstrapAllocator(const BootstrapAllocator<U> &a) noexcept : tracker(a.tracker) {}
  T *allocate(std::size_t n) {
    if (!tracker.state_) throw ReservationStop{};
    auto &requests = tracker.state_->requests;
    requests.before(n,sizeof(T));
    auto *p = std::allocator<T>{}.allocate(n);
    requests.acquired(n*sizeof(T));
    return p;
  }
  void deallocate(T *p, std::size_t n) noexcept {
    std::allocator<T>{}.deallocate(p,n);
    tracker.state_->requests.released(n*sizeof(T));
  }
  template<class U> bool operator==(const BootstrapAllocator<U> &a) const noexcept { return tracker == a.tracker; }
  template<class U> bool operator!=(const BootstrapAllocator<U> &a) const noexcept { return !(*this == a); }
};

// Rebinding discovers the implementation's actual domain/control-block request.
// The allocator retains the outer tracker through the last weak release; neither
// tracker nor domain owns the other via a reverse strong reference.
inline std::shared_ptr<ReservationDomain> reserve_domain(
    const BootstrapReservations &tracker, ReservationBudget selected) {
  return std::allocate_shared<ReservationDomain>(BootstrapAllocator<ReservationDomain>(tracker),selected);
}
}
