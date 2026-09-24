#include "wal_kernel.h"

#include <optional>
#include <stdexcept>
#include <string>

namespace wal {

namespace {

// Parses the in-box Message's payload -- a decimal arena-index HANDLE,
// never a serialization of the Report/Obligation itself
// (WAL-INTEGRATION-PLAN.md §3, "handle in the payload") -- back into an
// index. A malformed handle is a caller/feeder error, surfaced the same
// fail-loud way the rest of this substrate treats one (not swallowed, not
// defaulted).
std::size_t parse_handle(const std::string &payload) {
  std::size_t consumed = 0;
  const unsigned long long value = std::stoull(payload, &consumed);
  if (consumed != payload.size()) {
    throw std::invalid_argument("wal::WalKernel: malformed arena handle '" + payload + "'");
  }
  return static_cast<std::size_t>(value);
}

}  // namespace

scheduler::Handler WalKernel::make_handler() {
  return [this](const box::Message &message, scheduler::Sender &sender) {
    handle(message, sender);
  };
}

void WalKernel::handle(const box::Message &message, scheduler::Sender &sender) {
  const std::size_t index = parse_handle(message.payload);
  const Report &report = in_arena_.at(index);

  // Order + failure decision (WAL-INTEGRATION-PLAN.md, part 1): owed()
  // first -- pure, no DB, no side effect -- so nothing has been pushed or
  // booked yet if what follows throws.
  const std::vector<Obligation> owed_set = owed(report);

  // Unchanged built logic. NOT caught: WalMonitor::feed() throws
  // std::invalid_argument on an out-of-order same-source report, which is
  // a feeder/programming bug, not runtime data to swallow or reorder
  // around. The throw propagates out of this handler and, since
  // Scheduler::step() wraps no handler in try/catch, aborts the whole
  // run_until_idle() run -- deliberate, per the plan's failure decision.
  // Because this runs before the push loop below, a thrown report
  // contributes nothing to the out-box.
  monitor_.feed(report);

  for (const Obligation &obligation : owed_set) {
    out_arena_.push_back(obligation);
    const std::size_t new_index = out_arena_.size() - 1;
    sender.send(out_box_id_, box::Message{std::to_string(new_index), std::nullopt});
  }
}

}  // namespace wal
