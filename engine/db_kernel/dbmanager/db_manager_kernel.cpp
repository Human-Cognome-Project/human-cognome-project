#include "db_manager_kernel.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace dbmanager {

namespace {

// Parses the in-box Message's payload -- a decimal arena-index HANDLE,
// never a serialization of the Request/Response itself -- back into an
// index. Mirrors wal::parse_handle (wal/wal_kernel.cpp) exactly: a
// malformed handle is a caller/feeder bug, surfaced fail-loud (not
// swallowed, not defaulted).
std::size_t parse_handle(const std::string &payload) {
  std::size_t consumed = 0;
  const unsigned long long value = std::stoull(payload, &consumed);
  if (consumed != payload.size()) {
    throw std::invalid_argument("dbmanager::DbManagerKernel: malformed arena handle '" +
                                 payload + "'");
  }
  return static_cast<std::size_t>(value);
}

}  // namespace

scheduler::Handler DbManagerKernel::make_handler() {
  return [this](const box::Message &message, scheduler::Sender &sender) {
    handle(message, sender);
  };
}

void DbManagerKernel::handle(const box::Message &message, scheduler::Sender &sender) {
  const std::size_t index = parse_handle(message.payload);
  const Request &req = req_arena_.at(index);

  // dispatch/ is called verbatim -- unchanged. A single Command yields a
  // one-element Response; a stream yields dispatch_stream's own ordered
  // vector<Result> (dependent ordering preserved, unmodified).
  Response response = std::visit(
      [this](const auto &value) -> Response {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, dispatch::Command>) {
          return Response{dispatch::dispatch_one(ctl_, value)};
        } else {
          return dispatch::dispatch_stream(ctl_, value);
        }
      },
      req);

  resp_arena_.push_back(std::move(response));
  const std::size_t new_index = resp_arena_.size() - 1;

  // reply_to is present by construction -- the composing analyst kernel
  // emits it per its own ruleset (TIER2-PLAN.md reconciliation banner: we
  // do not validate peer-kernel requests). No presence/validity
  // precondition, no kDropped handling: an absent reply_to is a
  // programming/wiring bug, surfaced fail-loud by an explicit throw --
  // the same discipline as a bad arena handle above (parse_handle) --
  // NOT undefined behaviour from an unchecked optional dereference, and
  // NOT a silent no-op.
  if (!message.reply_to) {
    throw std::invalid_argument("dbmanager::DbManagerKernel: request has no reply_to");
  }
  sender.send(*message.reply_to, box::Message{std::to_string(new_index), std::nullopt});
}

}  // namespace dbmanager
