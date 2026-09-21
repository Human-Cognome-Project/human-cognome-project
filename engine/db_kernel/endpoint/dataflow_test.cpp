// Integration test: proves the whole box / endpoint / scheduler contract
// end-to-end (ENDPOINT-PRIMITIVES-PLAN.md, module 4). Standalone: standard
// library only, no test framework, no DB.
//
// Every scenario seeds work via `submit` (the external-driver entry
// point) and drains via `run_until_idle` -- nothing polls.
#include <cstdio>
#include <optional>
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

// A trivial "component": echoes an ack back to whatever reply_to it was
// given, or does nothing if there is none.
void echo_handler(const box::Message &m, scheduler::Sender &sender) {
  if (m.reply_to.has_value()) {
    sender.send(*m.reply_to, box::Message{"ack:" + m.payload, std::nullopt});
  }
}

// Round-trip: a request with a reply_to is submitted to a component's
// input box; the handler acks to reply_to; the requester's return box
// receives it. Completion is the ack landing -- no poll.
void test_round_trip() {
  endpoint::Registry registry(/*standing_capacity=*/1, /*ephemeral_capacity=*/4);
  scheduler::Scheduler sched(registry, /*num_levels=*/1);

  box::Box component_input;
  endpoint::EndpointId component_id = registry.register_standing(0, &component_input);
  sched.register_box(&component_input, /*priority=*/0, echo_handler);

  endpoint::EndpointId return_id = registry.allocate();
  sched.submit(component_id, box::Message{"request", return_id});
  sched.run_until_idle();

  box::Box *return_box = registry.resolve(return_id);
  check(return_box != nullptr && !return_box->empty(),
        "the requester's return box holds the ack after run_until_idle -- completion is the "
        "ack landing, not a poll");
  box::Message ack = return_box->pop();
  check(ack.payload == "ack:request", "the ack carries the expected result");
}

// Multi-connection / correlation: several requests, each with its own
// reply_to, interleaved into one input box; each ack routes to its own
// return endpoint, no cross-talk. (Honesty: this proves correlation via
// sequential interleave, not concurrent-append safety -- that is the
// deferred MPSC seam.)
void test_multi_connection_correlation() {
  endpoint::Registry registry(1, 8);
  scheduler::Scheduler sched(registry, 1);

  box::Box component_input;
  endpoint::EndpointId component_id = registry.register_standing(0, &component_input);
  sched.register_box(&component_input, 0, echo_handler);

  endpoint::EndpointId r1 = registry.allocate();
  endpoint::EndpointId r2 = registry.allocate();
  endpoint::EndpointId r3 = registry.allocate();

  // Sequential interleave into the shared input box.
  sched.submit(component_id, box::Message{"req-A", r1});
  sched.submit(component_id, box::Message{"req-B", r2});
  sched.submit(component_id, box::Message{"req-C", r3});
  sched.run_until_idle();

  box::Box *b1 = registry.resolve(r1);
  box::Box *b2 = registry.resolve(r2);
  box::Box *b3 = registry.resolve(r3);
  check(b1 && b1->size() == 1 && b2 && b2->size() == 1 && b3 && b3->size() == 1,
        "each of three interleaved requests deposits exactly one ack in its own return box");

  box::Message a1 = b1->pop();
  box::Message a2 = b2->pop();
  box::Message a3 = b3->pop();
  check(a1.payload == "ack:req-A", "request A's ack lands at A's return endpoint");
  check(a2.payload == "ack:req-B", "request B's ack lands at B's return endpoint");
  check(a3.payload == "ack:req-C", "request C's ack lands at C's return endpoint");
  check(b1->empty() && b2->empty() && b3->empty(),
        "no return box received more than its own request's ack -- no cross-talk");
}

// Reconcile pattern: work staged into the top-priority pinned box is
// selected ahead of normal boxes at the next boundary, then the box is
// empty again.
void test_reconcile_pattern() {
  endpoint::Registry registry(2, 0);
  scheduler::Scheduler sched(registry, /*num_levels=*/2);

  box::Box reconcile_box;  // pinned top priority, normally empty
  box::Box normal_box;
  endpoint::EndpointId reconcile_id = registry.register_standing(0, &reconcile_box);
  endpoint::EndpointId normal_id = registry.register_standing(1, &normal_box);

  std::vector<std::string> trace;
  sched.register_box(&reconcile_box, 0, [&trace](const box::Message &m, scheduler::Sender &) {
    trace.push_back("reconcile:" + m.payload);
  });
  sched.register_box(
      &normal_box, 1,
      [&trace, reconcile_id](const box::Message &m, scheduler::Sender &sender) {
        trace.push_back("normal:" + m.payload);
        if (m.payload == "trigger") {
          sender.send(reconcile_id, box::Message{"urgent", std::nullopt});
        }
      });

  sched.submit(normal_id, box::Message{"trigger", std::nullopt});
  sched.submit(normal_id, box::Message{"after", std::nullopt});
  sched.run_until_idle();

  check(trace.size() == 3 && trace[0] == "normal:trigger" && trace[1] == "reconcile:urgent" &&
            trace[2] == "normal:after",
        "work staged into the pinned top box is selected ahead of the normal box at the next "
        "selection boundary");
  check(reconcile_box.empty(), "the reconcile box is empty again once its staged work drains");
}

// Recycled-slot safety: an ack addressed to a since-recycled return
// endpoint (stale generation) returns kDropped and is not misdelivered.
void test_recycled_slot_safety() {
  endpoint::Registry registry(1, 4);
  scheduler::Scheduler sched(registry, 1);

  scheduler::SendStatus observed_ack_status = scheduler::SendStatus::kDelivered;
  box::Box component_input;
  endpoint::EndpointId component_id = registry.register_standing(0, &component_input);
  sched.register_box(&component_input, 0,
                      [&observed_ack_status](const box::Message &m, scheduler::Sender &sender) {
                        if (m.reply_to.has_value()) {
                          observed_ack_status = sender.send(
                              *m.reply_to, box::Message{"ack:" + m.payload, std::nullopt});
                        }
                      });

  endpoint::EndpointId stale_return = registry.allocate();
  registry.recycle(stale_return);  // since-recycled before the ack ever arrives

  sched.submit(component_id, box::Message{"orphaned request", stale_return});
  sched.run_until_idle();

  check(observed_ack_status == scheduler::SendStatus::kDropped,
        "an ack addressed to a since-recycled return endpoint is dropped (kDropped), never "
        "misdelivered into whatever now occupies that slot");
}

}  // namespace

int main() {
  test_round_trip();
  test_multi_connection_correlation();
  test_reconcile_pattern();
  test_recycled_slot_safety();

  if (g_failures == 0) {
    std::printf("PASS dataflow_test\n");
    return 0;
  }
  std::printf("FAIL dataflow_test (%d failed)\n", g_failures);
  return 1;
}
