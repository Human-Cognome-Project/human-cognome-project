// box -- the dumb FIFO queue endpoints are built out of.
//
// A box knows nothing about the scheduler, readiness, or priority. It
// holds Message in arrival order and nothing else. Header-only,
// independent of every other module in this directory (ENDPOINT-PRIMITIVES-
// PLAN.md, module 1).
//
// endpoint::EndpointId is physically defined HERE rather than in
// endpoint.h. box::Message needs it structurally (the `reply_to` field),
// and box.h must stay header-only and free of any dependency on the
// registry logic (allocate/resolve/recycle) that gives an EndpointId its
// meaning. endpoint.h includes this header and builds that logic around
// the id defined here; box itself never interprets an EndpointId -- it
// only carries one opaque payload plus an optional reply_to through the
// queue, unexamined. (A plan-silent call -- see endpoint/README.md.)
#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace endpoint {

struct EndpointId {
  uint32_t slot = 0;
  uint32_t generation = 0;

  bool operator==(const EndpointId &other) const {
    return slot == other.slot && generation == other.generation;
  }
  bool operator!=(const EndpointId &other) const { return !(*this == other); }
};

}  // namespace endpoint

namespace box {

// An opaque payload the substrate never interprets, plus an optional
// return endpoint. No reply_to => whatever handler receives this Message
// simply does not send an ack -- the substrate does nothing, by design.
struct Message {
  std::string payload;
  std::optional<endpoint::EndpointId> reply_to;
};

// An in-memory FIFO queue of Message. Volatile. FIFO is the only
// ordering -- no per-item priority (priority is per box, owned by the
// scheduler, not here).
//
// Concurrency boundary (named seam, not built): push() is where MPSC
// safety lives once producers are separate threads. First cut is
// single-threaded.
//
// push() is exposed here for the scheduler to call (it owns every box it
// manages) -- but it is NEVER the substrate's injection point. All work
// enters through the scheduler (submit / Sender::send), which pushes AND
// marks the box ready in the same routine. Calling push() directly from
// outside the scheduler desynchronizes occupancy from readiness and is a
// caller error, not a supported path.
class Box {
 public:
  void push(Message message) { queue_.push_back(std::move(message)); }

  // Caller error surfaced as !ok (throws), never UB.
  Message pop() {
    if (queue_.empty()) {
      throw std::logic_error("box::Box::pop on an empty box");
    }
    Message front = std::move(queue_.front());
    queue_.pop_front();
    return front;
  }

  bool empty() const { return queue_.empty(); }
  std::size_t size() const { return queue_.size(); }

 private:
  std::deque<Message> queue_;
};

}  // namespace box
