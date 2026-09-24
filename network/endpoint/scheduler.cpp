#include "scheduler.h"

#include <stdexcept>
#include <utility>

namespace scheduler {

SendStatus Sender::send(endpoint::EndpointId id, box::Message message) {
  return scheduler_.submit(id, std::move(message));
}

Scheduler::Scheduler(endpoint::Registry &registry, unsigned num_levels)
    : registry_(registry), num_levels_(num_levels), ready_(num_levels) {
  if (num_levels == 0) {
    throw std::invalid_argument("scheduler::Scheduler: num_levels must be >= 1");
  }
}

void Scheduler::register_box(box::Box *box, Priority priority, Handler handler) {
  if (priority >= num_levels_) {
    throw std::out_of_range("scheduler::Scheduler::register_box: priority level out of range");
  }
  auto existing = monitored_.find(box);
  if (existing != monitored_.end() && existing->second.ready) {
    // register_box is setup-time only. Replacing this box's entry while
    // it is sitting in ready_[old_priority] would desync the deque from
    // monitored_: the stale ready_ entry would later be popped and
    // looked up against the NEW MonitoredEntry (same Box* key), whose
    // fresh `ready` flag is false -- silently corrupting readiness
    // bookkeeping rather than failing loudly. Refuse it instead.
    throw std::logic_error(
        "scheduler::Scheduler::register_box: box is currently ready (queued for step()) -- "
        "register_box is setup-time only, before the box has any pending work");
  }
  MonitoredEntry entry;
  entry.priority = priority;
  entry.handler = std::move(handler);
  entry.ready = false;
  monitored_.insert_or_assign(box, std::move(entry));
}

void Scheduler::mark_ready(box::Box *box) {
  auto it = monitored_.find(box);
  if (it == monitored_.end()) {
    // An unmonitored box (e.g. an ephemeral return mailbox) -- nothing
    // polls it; the external driver resolves and pops it directly.
    return;
  }
  MonitoredEntry &entry = it->second;
  if (!entry.ready) {
    entry.ready = true;
    ready_[entry.priority].push_back(box);
  }
}

SendStatus Scheduler::submit(endpoint::EndpointId id, box::Message message) {
  box::Box *b = registry_.resolve(id);
  if (b == nullptr) {
    return SendStatus::kDropped;  // unresolvable or stale generation
  }
  b->push(std::move(message));
  mark_ready(b);
  return SendStatus::kDelivered;
}

bool Scheduler::step() {
  for (unsigned level = 0; level < num_levels_; ++level) {
    if (ready_[level].empty()) {
      continue;
    }
    box::Box *b = ready_[level].front();
    ready_[level].pop_front();

    // A box only ever lands in ready_ via mark_ready(), which only
    // enqueues boxes present in monitored_ -- this lookup cannot fail.
    MonitoredEntry &entry = monitored_.at(b);
    entry.ready = false;

    box::Message message = b->pop();
    Sender sender(*this);
    entry.handler(message, sender);

    if (!b->empty()) {
      // Still occupied -- possibly because the handler just sent into
      // its own box, possibly because more was already queued behind
      // the item just processed. Either way it re-enters the ready set
      // at its fixed priority and is re-evaluated at the NEXT selection
      // boundary against every other ready box (F8) -- it does not get
      // to keep running FIFO in a tight loop ahead of a higher-priority
      // box that just became ready.
      mark_ready(b);
    }
    return true;
  }
  return false;
}

void Scheduler::run_until_idle() {
  while (step()) {
  }
}

}  // namespace scheduler
