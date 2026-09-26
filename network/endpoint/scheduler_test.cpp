// Unit tests for scheduler::Scheduler. Standalone: standard library only,
// no test framework, no DB. Prints one line per check; the process exits
// non-zero if any check failed.
#include <cstdio>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "endpoint.h"
#include "scheduler.h"

namespace {

int g_failures = 0;

void check(bool ok, const std::string &what) {
  if (ok) {
    std::printf("ok   %s\n", what.c_str());
  } else {
    std::printf("FAIL %s\n", what.c_str());
    ++g_failures;
  }
}

// Priority order: the top-level ready box is drained first, even though
// the lower-priority box became ready earlier.
void test_priority_order() {
  endpoint::Registry registry(2, 0);
  scheduler::Scheduler sched(registry, /*num_levels=*/2);

  box::Box high_box;
  box::Box low_box;
  endpoint::EndpointId high_id = registry.register_standing(0, &high_box);
  endpoint::EndpointId low_id = registry.register_standing(1, &low_box);

  std::vector<std::string> trace;
  sched.register_box(&high_box, /*priority=*/0,
                      [&trace](const box::Message &m, scheduler::Sender &) {
                        trace.push_back("high:" + m.payload);
                      });
  sched.register_box(&low_box, /*priority=*/1,
                      [&trace](const box::Message &m, scheduler::Sender &) {
                        trace.push_back("low:" + m.payload);
                      });

  // low becomes ready first; high becomes ready second.
  sched.submit(low_id, box::Message{"l1", std::nullopt});
  sched.submit(high_id, box::Message{"h1", std::nullopt});

  bool stepped = sched.step();
  check(stepped, "step() finds ready work");
  check(trace.size() == 1 && trace[0] == "high:h1",
        "the higher-priority box is drained first, regardless of which box became ready first");

  sched.run_until_idle();
  check(trace.size() == 2 && trace[1] == "low:l1",
        "the lower-priority box is drained once nothing higher-priority is ready");
}

// FIFO within a single box.
void test_fifo_within_a_box() {
  endpoint::Registry registry(1, 0);
  scheduler::Scheduler sched(registry, 1);

  box::Box b;
  endpoint::EndpointId id = registry.register_standing(0, &b);
  std::vector<std::string> trace;
  sched.register_box(&b, 0, [&trace](const box::Message &m, scheduler::Sender &) {
    trace.push_back(m.payload);
  });

  sched.submit(id, box::Message{"a", std::nullopt});
  sched.submit(id, box::Message{"b", std::nullopt});
  sched.submit(id, box::Message{"c", std::nullopt});
  sched.run_until_idle();

  check(trace.size() == 3 && trace[0] == "a" && trace[1] == "b" && trace[2] == "c",
        "messages within one box are handled in FIFO (push) order");
}

// Selection-boundary priority re-evaluation (F8): a handler that sends
// into the pinned top box causes that box to be picked next, ahead of
// further work already queued in a lower-priority box. Also covers "a
// pinned top box wins when occupied and is inert when empty."
void test_selection_boundary_reevaluation() {
  endpoint::Registry registry(2, 0);
  scheduler::Scheduler sched(registry, /*num_levels=*/2);

  box::Box reconcile_box;  // pinned top priority, normally empty
  box::Box normal_box;
  endpoint::EndpointId reconcile_id = registry.register_standing(0, &reconcile_box);
  endpoint::EndpointId normal_id = registry.register_standing(1, &normal_box);

  std::vector<std::string> trace;
  sched.register_box(&reconcile_box, /*priority=*/0,
                      [&trace](const box::Message &m, scheduler::Sender &) {
                        trace.push_back("reconcile:" + m.payload);
                      });
  sched.register_box(
      &normal_box, /*priority=*/1,
      [&trace, &sched, reconcile_id](const box::Message &m, scheduler::Sender &sender) {
        trace.push_back("normal:" + m.payload);
        if (m.payload == "first") {
          // Escalate into the pinned reconcile box mid-flow.
          sender.send(reconcile_id, box::Message{"escalated", std::nullopt});
        }
      });

  // Two messages queued in the lower-priority box before anything runs;
  // the reconcile box starts empty (inert).
  sched.submit(normal_id, box::Message{"first", std::nullopt});
  sched.submit(normal_id, box::Message{"second", std::nullopt});

  bool stepped_while_empty_elsewhere = true;  // just to anchor a message below
  (void)stepped_while_empty_elsewhere;

  // Step 1: only normal_box is ready -- it wins by elimination, and its
  // handler escalates into the (until now inert) reconcile box.
  sched.step();
  check(trace.size() == 1 && trace[0] == "normal:first",
        "the pinned reconcile box is inert (never selected) while it is empty");

  // Step 2: reconcile_box just became ready; normal_box is STILL ready
  // too (it has "second" left). The re-evaluated selection must pick the
  // now-ready top box next, ahead of the already-queued lower-priority
  // work.
  sched.step();
  check(trace.size() == 2 && trace[1] == "reconcile:escalated",
        "a box made ready mid-handler wins the very next selection boundary ahead of "
        "already-queued lower-priority work (selection-boundary re-evaluation, F8)");

  // Step 3: reconcile_box is empty again; normal_box drains its last item.
  sched.step();
  check(trace.size() == 3 && trace[2] == "normal:second",
        "normal work resumes once the reconcile box has drained back to empty");

  bool more_ready = sched.step();
  check(!more_ready,
        "nothing is ready once both boxes are drained -- the pinned box is inert when empty");
}

// send / submit return kDropped on an unresolvable or stale endpoint.
void test_send_dropped_on_bad_endpoint() {
  endpoint::Registry registry(1, 1);
  scheduler::Scheduler sched(registry, 1);

  // Entirely unresolvable: no standing slot registered at 0, no ephemeral
  // slot at that generation either (slot 0 here happens to be standing
  // range but was never register_standing'd).
  scheduler::SendStatus unresolvable =
      sched.submit(endpoint::EndpointId{0, 0}, box::Message{"nobody home", std::nullopt});
  check(unresolvable == scheduler::SendStatus::kDropped,
        "submit to an unresolvable endpoint returns kDropped");

  endpoint::EndpointId stale = registry.allocate();
  registry.recycle(stale);
  scheduler::SendStatus dropped =
      sched.submit(stale, box::Message{"late ack", std::nullopt});
  check(dropped == scheduler::SendStatus::kDropped,
        "submit to a stale (since-recycled) endpoint returns kDropped");

  endpoint::EndpointId fresh = registry.allocate();
  scheduler::SendStatus delivered =
      sched.submit(fresh, box::Message{"on time", std::nullopt});
  check(delivered == scheduler::SendStatus::kDelivered,
        "submit to a valid, currently-allocated endpoint returns kDelivered");
}

void test_handler_throw_does_not_strand_box() {
  endpoint::Registry registry(1, 0);
  scheduler::Scheduler sched(registry, 1);
  box::Box inbox;
  endpoint::EndpointId id = registry.register_standing(0, &inbox);
  std::vector<std::string> seen;
  sched.register_box(&inbox, 0, [&](const box::Message &m, scheduler::Sender &) {
    seen.push_back(m.payload);
    if (m.payload == "m1") {
      throw std::runtime_error("handler failure");
    }
  });
  sched.submit(id, box::Message{"m1", std::nullopt});
  sched.submit(id, box::Message{"m2", std::nullopt});

  bool threw = false;
  try {
    sched.step();
  } catch (const std::runtime_error &) {
    threw = true;
  }
  check(threw, "a handler exception propagates out of step()");
  check(sched.step(), "after a handler throws, the box is still ready for its remaining items");
  check(seen.size() == 2 && seen[1] == "m2",
        "the item queued behind the failed one is processed; the failed item is not retried");
  check(inbox.empty() && !sched.step(), "the box drains and the scheduler goes idle");
}

}  // namespace

int main() {
  test_priority_order();
  test_fifo_within_a_box();
  test_selection_boundary_reevaluation();
  test_send_dropped_on_bad_endpoint();
  test_handler_throw_does_not_strand_box();

  if (g_failures == 0) {
    std::printf("PASS scheduler_test\n");
    return 0;
  }
  std::printf("FAIL scheduler_test (%d failed)\n", g_failures);
  return 1;
}
