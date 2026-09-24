#pragma once

#include <string>
#include <variant>
#include <vector>

#include "command_ir.h"
#include "controller.h"
#include "declare_core.h"
#include "read_core.h"
#include "update_core.h"

// The verb dispatcher (PLAN.md II.8) and the arraying executor (PLAN.md
// II.7): routes a record-tier command IR to its core over the controller
// door, and executes an ordered stream of additive commands.
//
// This is dispatch OVER THE IN-PROCESS IR -- construct a command:: struct
// (or an AdditiveCommand stream) in C++, call dispatch_one/dispatch_stream,
// read the result back. There is no text/wire parser here: the external
// request encoding is a separate, later, deferred layer (PLAN.md I.H, seam
// G6; NOTES.md "Request transport -- firmed 2026-09-18"). This module
// CONSUMES command/, declare/, read/, update/ unmodified; it adds no new
// semantics of its own beyond routing and ordering.
//
// See README.md "What the arraying executor adds" for why II.7 is, beyond
// the ordered-stream piece below, already subsumed by the cores' own
// intra-command array handling (DeclareRecord's PARENTS set, MoveRecord's
// multi-selector sources, AddConnection's element set).
namespace dispatch {

// Which verb a Command/Result belongs to (PLAN.md I.A -- the full
// record-tier + named cache-tier vocabulary).
enum class Verb {
  kDeclare,
  kRead,
  kMove,
  kAddConnection,
  kDeleteRecord,
  kDeleteConnection,
  kUpdateCache,
  kRebaseCache,
};

// DELETE_RECORD / DELETE_CONNECTION pair their op with the caller's
// required confirmation (update_core.h: "the caller must resupply the
// exact target as `confirm`" -- the op-layer gate). Bundled into one
// struct so a single Command value carries everything dispatch_one needs;
// `op` and `confirm` are ordinarily identical (the caller re-states the
// target), and a mismatch is reported by the update core, not rejected
// here.
struct DeleteRecordRequest {
  command::DeleteRecord op;
  command::DeleteRecord confirm;
};
struct DeleteConnectionRequest {
  command::DeleteConnection op;
  command::DeleteConnection confirm;
};

// Cache-tier verbs (PLAN.md I.A, II.8): named face entries only, no data
// -- UPDATE_CACHE / REBASE_CACHE carry no fields because nothing behind
// them is built yet (the cache tier is out of scope; see PLAN.md Part I
// "Out of scope"). RECONCILE is no longer a db/cache-manager verb: it is
// an analyst -> WAL-manager message (the WAL manager promotes the relevant
// pending queue into a priority in-box); see ENDPOINT-ACTIVATION-NOTES.md
// "RECONCILE".
struct UpdateCache {};
struct RebaseCache {};

// The additive record-tier ops -- exactly the ops PLAN.md I.F / NOTES.md
// "Arraying is universal" name as arrayable: DECLARE, READ, MOVE_RECORD,
// ADD_CONNECTION. This IS how "the destructive ops are not arrayable" is
// enforced for the stream surface below: DeleteRecordRequest /
// DeleteConnectionRequest and the cache stubs are not alternatives of
// this variant, so a std::vector<AdditiveCommand> cannot represent a
// DELETE, structurally, not by a runtime check.
using AdditiveCommand = std::variant<command::DeclareRecord, command::ReadRecord,
                                      command::MoveRecord, command::AddConnection>;

// The full verb surface (PLAN.md I.A), dispatched one at a time by
// dispatch_one.
using Command =
    std::variant<command::DeclareRecord, command::ReadRecord, command::MoveRecord,
                 command::AddConnection, DeleteRecordRequest, DeleteConnectionRequest,
                 UpdateCache, RebaseCache>;

// One dispatch outcome: which verb ran, and that verb's own result type
// (PLAN.md I.G "Return contract" -- UNCHANGED per-verb shape; dispatch
// adds no new return semantics). The cache-tier stubs carry a plain
// "<VERB>: not yet implemented" string instead of a core result, since no
// core exists behind them yet.
struct Result {
  Verb verb;
  std::variant<declare::Result, dbread::ReadResult, update::MoveResult,
               update::AddConnectionResult, update::DeleteResult, std::string>
      value;
};

// Dispatches one command to its core:
//   DeclareRecord          -> declare::execute
//   ReadRecord             -> dbread::read
//   MoveRecord             -> update::move_record
//   AddConnection          -> update::add_connection
//   DeleteRecordRequest    -> update::delete_record
//   DeleteConnectionRequest-> update::delete_connection
//   UpdateCache/RebaseCache -> a non-fatal stub result; `ctl` is not
//     touched.
// Every core re-validates its own IR defensively (same discipline as
// declare_core.cpp/update_core.cpp calling their command:: validators);
// this function performs no validation of its own and adds no new
// rejection paths.
Result dispatch_one(dbk::Controller &ctl, const Command &cmd);

// The arraying executor (PLAN.md II.7): a serial, ORDERED stream of
// additive commands, executed one at a time, in the given order, against
// the same controller -- exactly "ADDITIVE ops only ... executed IN
// ORDER" (team-lead brief). Each item's own array/broadcast/co-index/SEE
// handling is entirely its owning core's existing job (see README.md);
// what this function adds, and the only thing it adds, is the
// cross-command piece none of the single-op cores provide: N separate
// verb invocations, run in stream order, one Result per item, in the
// same order as the input.
std::vector<Result> dispatch_stream(dbk::Controller &ctl,
                                     const std::vector<AdditiveCommand> &stream);

}  // namespace dispatch
