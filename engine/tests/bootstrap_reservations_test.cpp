#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"
#include "../../common/bootstrap_reservations.h"
#include <iostream>
#include <stdexcept>

namespace R = hcp::resources;
namespace A = allocation_lifetime;
constexpr R::ReservationBudget unlimited{UINT64_MAX,UINT64_MAX};
void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
template<class F> void refused(F f) {
  const auto before = A::counts.attempts;
  bool stopped = false;
  try { f(); } catch (const R::ReservationStop &) { stopped = true; }
  require(stopped && A::counts.attempts == before,"refusal attempted allocation");
}
int main() {
  try {
    std::uint64_t outer = 0, combined = 0;
    // All bootstrap requests, including tracker storage, are observed independently.
    A::begin();
    {
      R::ReservationResult initial;
      auto tracker = R::BootstrapReservations::create(unlimited,initial);
      outer = initial.live_bytes;
      require(outer == A::counts.live && A::counts.blocks == 1,"tracker omitted its own request");
      auto domain = R::reserve_domain(tracker,unlimited);
      combined = tracker.result().live_bytes;
      require(combined == A::counts.live && combined > outer && A::counts.blocks == 2 &&
                  tracker.result().charged_work == combined,"domain request differs from observed bytes");
      std::weak_ptr<R::ReservationDomain> weak = domain;
      auto alias = domain;
      domain.reset(); alias.reset();
      require(weak.expired() && tracker.result().live_bytes == combined,
              "weak domain control block released its accounting early");
      weak.reset();
      require(tracker.result().live_bytes == outer && tracker.result().peak_bytes == combined &&
                  tracker.result().charged_work == combined,"domain release refunded work or lost tracker");
    }
    require(!A::end().live,"bootstrap leaked storage");

    // Zero/one-short/exact admission at both real request boundaries.
    for (bool work : {false,true}) {
      for (auto limit : {std::uint64_t{0},outer-1}) {
        A::begin();
        R::ReservationResult result;
        refused([&] { auto t = R::BootstrapReservations::create(
            work ? R::ReservationBudget{UINT64_MAX,limit} : R::ReservationBudget{limit,UINT64_MAX},result); });
        require(result.status == (work ? R::ReservationStatus::WorkExhausted : R::ReservationStatus::ByteExhausted) &&
                    result.required_bytes == outer && result.required_work == outer && !result.live_bytes &&
                    !result.charged_work,"tracker refusal demand incorrect");
        require(!A::end().live,"tracker refusal leaked");
      }
      for (auto limit : {outer,combined-1,combined}) {
        A::begin();
        {
          R::ReservationResult result;
          auto t = R::BootstrapReservations::create(
              work ? R::ReservationBudget{UINT64_MAX,limit} : R::ReservationBudget{limit,UINT64_MAX},result);
          if (limit == combined) {
            auto d = R::reserve_domain(t,unlimited);
            require(t.result().live_bytes == combined,"exact bootstrap retry failed");
          } else {
            refused([&] { auto d = R::reserve_domain(t,unlimited); });
            const auto r = t.result();
            require(r.status == (work ? R::ReservationStatus::WorkExhausted : R::ReservationStatus::ByteExhausted) &&
                        r.required_bytes == combined && r.required_work == combined &&
                        r.live_bytes == outer && r.charged_work == outer,"domain refusal changed accepted charge");
          }
        }
        require(!A::end().live,"budget test leaked");
      }
    }

    // No hidden observer lifetime requirement: selected shared objects can retain
    // the domain after both external handles disappear, including weak references.
    A::begin();
    {
      R::ReservationResult initial;
      auto tracker = R::BootstrapReservations::create(unlimited,initial);
      auto domain = R::reserve_domain(tracker,unlimited);
      auto object = std::allocate_shared<int>(R::SharedReservationAllocator<int>(domain),17);
      const auto selected = domain->result().live_bytes;
      require(A::counts.live == tracker.result().live_bytes+selected,"bootstrap and selected requests overlap");
      std::weak_ptr<int> weak = object;
      auto copy = tracker;
      tracker = {};
      auto &same = copy;
      copy = std::move(same); // Self move preserves the last observation handle.
      domain.reset(); object.reset(); copy = {};
      require(weak.expired() && A::counts.blocks == 3,"weak selected block lost domain/tracker lifetime");
      weak.reset();
      require(!A::counts.live && !A::counts.blocks,"weak release left a lifetime cycle");
    }
    A::end();

    // Both actual allocation failures unwind; prepaid cumulative work remains.
    for (std::size_t fail : {1u,2u}) {
      A::begin(fail);
      R::ReservationResult initial;
      R::BootstrapReservations tracker;
      bool failed = false;
      try {
        tracker = R::BootstrapReservations::create(unlimited,initial);
        auto domain = R::reserve_domain(tracker,unlimited);
      } catch (const std::bad_alloc &) { failed = true; }
      require(failed && A::counts.attempts == fail,"failure injection did not reach request");
      if (fail == 1) require(!initial.live_bytes && initial.charged_work == outer,"outer failure lost prepaid work");
      else require(tracker.result().live_bytes == outer && tracker.result().charged_work == combined,
                   "domain failure leaked bytes or refunded work");
      tracker = {};
      require(!A::end().live,"allocation failure leaked");
    }

    // Rebound aligned requests and constructor failure use the same allocator.
    struct alignas(64) Throwing { explicit Throwing(bool fail) { if (fail) throw 17; } };
    A::begin();
    {
      R::ReservationResult initial;
      auto tracker = R::BootstrapReservations::create(unlimited,initial);
      bool failed = false;
      try { auto p = std::allocate_shared<Throwing>(R::BootstrapAllocator<Throwing>(tracker),true); }
      catch (int e) { failed = e == 17; }
      require(failed && tracker.result().live_bytes == outer && A::counts.live == outer &&
                  tracker.result().charged_work > outer,"constructor unwind lost bootstrap accounting");
      auto p = std::allocate_shared<Throwing>(R::BootstrapAllocator<Throwing>(tracker),false);
      require(reinterpret_cast<std::uintptr_t>(p.get())%alignof(Throwing) == 0 &&
                  tracker.result().live_bytes == A::counts.live,"aligned bootstrap request untracked");
      const auto accepted = tracker.result();
      R::BootstrapAllocator<std::uint64_t> allocator(tracker);
      refused([&] { auto *q = allocator.allocate(SIZE_MAX); allocator.deallocate(q,SIZE_MAX); });
      require(tracker.result().status == R::ReservationStatus::Overflow &&
                  tracker.result().live_bytes == accepted.live_bytes &&
                  tracker.result().charged_work == accepted.charged_work,"overflow changed accepted counters");
      R::BootstrapAllocator<char> bytes(tracker);
      refused([&] { auto *q = bytes.allocate(SIZE_MAX); bytes.deallocate(q,SIZE_MAX); });
      require(tracker.result().status == R::ReservationStatus::Overflow &&
                  tracker.result().live_bytes == accepted.live_bytes &&
                  tracker.result().charged_work == accepted.charged_work,"sum overflow changed accepted counters");
    }
    require(!A::end().live,"aligned/overflow test leaked");
    std::cout << "bootstrap_reservations PASS outer=" << outer << " combined=" << combined << '\n';
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
