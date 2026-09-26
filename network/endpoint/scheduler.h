// scheduler -- the monitor loop: readiness ownership + the sole enqueue
// path (ENDPOINT-PRIMITIVES-PLAN.md, "Ownership decision").
//
// box is a dumb FIFO; endpoint::Registry is a dumb name -> box lookup.
// THIS is where occupancy becomes activation: the scheduler holds a
// priority-indexed ready structure (one ready FIFO per fixed priority
// level, level 0 == highest, checked first) and is the ONLY place work
// gets pushed into a box AND marked ready in the same routine -- raw
// box::Box::push is never the injection point.
//
// Depends on box AND endpoint (F5) -- it resolves ids through the
// registry and pushes into the boxes the registry hands back.
#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <unordered_map>
#include <vector>

#include "box.h"
#include "endpoint.h"

namespace scheduler {

enum class SendStatus { kDelivered, kDropped };

// A small fixed range of priority levels. 0 is highest -- checked first
// at every selection boundary.
using Priority = unsigned;

class Scheduler;

// Passed to every handler so it can ack (or forward work) without ever
// touching a Box directly. Sender::send resolves through the SAME routing
// routine as Scheduler::submit (F6) -- there is exactly one enqueue path,
// whether work originates outside the substrate or from a running
// handler.
class Sender {
 public:
  explicit Sender(Scheduler &scheduler) : scheduler_(scheduler) {}

  SendStatus send(endpoint::EndpointId id, box::Message message);

 private:
  Scheduler &scheduler_;
};

// A handler runs to completion on the head item of an occupied box, then
// may Sender::send zero or more messages onward. No reply_to on the
// inbound Message => the handler simply does not send an ack -- the
// substrate does nothing, by design.
using Handler = std::function<void(const box::Message &, Sender &)>;

class Scheduler {
 public:
  Scheduler(endpoint::Registry &registry, unsigned num_levels);

  // Wire a box into the monitor set at a fixed priority level. `box` is
  // caller-owned (typically the same box already handed to
  // endpoint::Registry::register_standing) and must outlive the
  // scheduler. Throws std::out_of_range if priority >= num_levels.
  //
  // Setup-time only: throws std::logic_error if `box` is currently ready
  // (already queued for step()) -- re-registering it mid-run would leave
  // a stale entry in the ready structure. Registering an idle box that
  // was registered before (not currently ready) is fine.
  void register_box(box::Box *box, Priority priority, Handler handler);

  // External injection entry point -- the harness / an out-of-substrate
  // producer seeds work here. Resolves `id` via the registry; on success
  // pushes into the box and marks it ready; returns kDropped if `id` is
  // unresolvable or stale. A box with no registered handler (e.g. an
  // ephemeral return mailbox) still receives the push -- it is
  // kDelivered -- it is simply never selected by step() since nothing
  // monitors it; the external driver resolves and pops it directly.
  SendStatus submit(endpoint::EndpointId id, box::Message message);

  // Take the highest-priority ready box, run its handler on the head item
  // to completion, pop it, and drop the box from the ready set if it is
  // now empty (re-entering it if a handler left more behind, including
  // anything the handler itself just sent into it -- selection-boundary
  // priority re-evaluation, F8). Returns false if nothing was ready.
  //
  // If the handler throws, the head item stays consumed and the exception
  // propagates to the caller; the box re-enters the ready set if items
  // remain, so the rest of its queue is not stranded.
  bool step();

  // step() until no box is ready.
  void run_until_idle();

 private:
  struct MonitoredEntry {
    Priority priority = 0;
    Handler handler;
    bool ready = false;  // already queued in ready_[priority]? (dedupe)
  };

  void mark_ready(box::Box *box);

  endpoint::Registry &registry_;
  unsigned num_levels_;
  std::unordered_map<box::Box *, MonitoredEntry> monitored_;
  std::vector<std::deque<box::Box *>> ready_;  // one FIFO per priority level
};

}  // namespace scheduler
