#pragma once

#include <variant>
#include <vector>

#include "controller.h"
#include "dispatch.h"
#include "endpoint.h"
#include "scheduler.h"

// The db/cache manager's tier-2 analyst reaction body as a monitored-
// endpoint kernel (TIER2-PLAN.md). Mirrors wal::WalKernel's ownership and
// handle-parsing shape exactly (wal/wal_kernel.h): owns nothing, holds
// references to the request arena, the response arena and the shared
// dbk::Controller. The one legitimate difference from WalKernel: this
// kernel sends its answer to the REQUEST'S OWN carried return endpoint
// (message.reply_to), not to a fixed out-box -- each request names its own
// correlation, per ENDPOINT-ACTIVATION-NOTES.md "Request->return
// correlation -- RESOLVED".
//
// dispatch/ is called verbatim and unmodified -- this class adds no verb
// logic, no validation, and no new rejection path of its own. The analyst
// is a peer kernel composing well-formed requests by construction
// (TIER2-PLAN.md reconciliation banner): a malformed envelope (bad handle,
// absent reply_to) is a programming/wiring bug, surfaced fail-loud the same
// way WalKernel lets a bad arena handle throw -- an absent reply_to is
// caught by an explicit check and throws std::invalid_argument (NOT
// undefined behaviour, NOT a silent no-op) -- not an analyst-facing
// rejection path this class builds.
namespace dbmanager {

// Either a single record-tier command or an ordered stream of additive
// commands (TIER2-PLAN.md In-scope 1) -- both request forms the analyst
// surface keeps.
using Request = std::variant<dispatch::Command, std::vector<dispatch::AdditiveCommand>>;

// One reply message per request, regardless of form: a single-Command
// request yields a one-element Response; a stream yields one Result per
// item, in order (dispatch_stream's own ordering guarantee, unchanged).
using Response = std::vector<dispatch::Result>;

// Owns nothing. The driver/test owns the request arena, the response
// arena and the Controller, and must keep every one of them alive across
// each run_until_idle() call this kernel's handler runs under -- exactly
// the WalKernel ownership shape (wal/wal_kernel.h).
class DbManagerKernel {
 public:
  // `req_arena` is seeded (fully, before the run starts) by the
  // driver/test with the fixture Requests a Message's payload handle
  // indexes into. `resp_arena` is where this kernel appends one Response
  // per handled request. `ctl` is the shared dbk::Controller every
  // dispatched verb runs against, unchanged (dispatch/ owns all core
  // logic).
  DbManagerKernel(const std::vector<Request> &req_arena, std::vector<Response> &resp_arena,
                   dbk::Controller &ctl)
      : req_arena_(req_arena), resp_arena_(resp_arena), ctl_(ctl) {}

  DbManagerKernel(const DbManagerKernel &) = delete;
  DbManagerKernel &operator=(const DbManagerKernel &) = delete;

  // Bound to this object's state, so every analyst in-box this is
  // registered on shares the same response arena/controller.
  scheduler::Handler make_handler();

  // The reaction body itself: resolve the in-box Message's arena handle
  // to a Request, run it through dispatch_one/dispatch_stream over the
  // shared Controller, append the Response to the response arena, and
  // send the answer's own handle to the request's carried return
  // endpoint (message.reply_to).
  void handle(const box::Message &message, scheduler::Sender &sender);

 private:
  const std::vector<Request> &req_arena_;
  std::vector<Response> &resp_arena_;
  dbk::Controller &ctl_;
};

}  // namespace dbmanager
