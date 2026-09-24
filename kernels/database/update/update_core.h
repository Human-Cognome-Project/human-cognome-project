#pragma once

#include <optional>
#include <string>
#include <vector>

#include "codec.h"
#include "command_ir.h"
#include "controller.h"

// The UPDATE core (PLAN.md II.5): executes the four UPDATE_RECORD sub-ops
// -- MOVE_RECORD, ADD_CONNECTION, DELETE_RECORD, DELETE_CONNECTION --
// against the controller door, each already structurally validated by its
// command:: validator (re-validated here defensively, same discipline as
// declare/declare_core.cpp).
//
// Scope: ONE UPDATE statement per call. Cross-COMMAND arraying (the
// serial-stream executor for MOVE_RECORD/ADD_CONNECTION, PLAN.md II.7) is
// a later module -- out of scope here; this module handles MOVE_RECORD's
// and ADD_CONNECTION's own INTRINSIC range/wildcard forms (their
// resolution via the controller's gather() primitive), which is a
// different thing from cross-statement arraying.
//
// Spec anchors: PLAN.md I.D, I.F, II.0, II.5; NOTES.md "UPDATE --
// record-tier ops", "Build-phase rulings -- firmed 2026-09-17/18" (MOVE
// from->to relationship, wildcards terminal-only, gather primitive,
// DELETE gates). See README.md in this directory for the semantic
// decisions made where the spec was silent.
namespace update {

// ---------------------------------------------------------------------
// MOVE_RECORD -- relocation; topology-invariant (re-key only).
// ---------------------------------------------------------------------

// One resolved source token's relocation result. `from` is populated once
// the source token is known (even on failure, so a rejection can still be
// reported against a specific token); `to` is populated once a
// destination slot was planned for it. A whole-STATEMENT rejection (an
// invalid op, a mint-bearing destination, or a cover-N failure) is
// reported as a single MoveOutcome with both `from` and `to` empty --
// mirroring declare::Result's use of one Outcome for a whole-node
// rejection (declare/declare_core.cpp).
struct MoveOutcome {
  bool moved = false;
  std::optional<codec::Address> from;
  std::optional<codec::Address> to;
  std::string reason;  // set iff !moved.
};

struct MoveResult {
  std::vector<MoveOutcome> outcomes;
};

// Resolves every source selector (kExplicit / kRange / kPrefix, the
// latter two via Controller::gather) into an ordered, concatenated token
// list (source-selector order preserved; each selector's own tokens in
// gather's PK order -- see README.md "Source ordering"), plans the
// destination span against that resolved count, then rekeys each source
// token to its planned slot in order.
MoveResult move_record(dbk::Controller &ctl, const command::MoveRecord &op);

// ---------------------------------------------------------------------
// ADD_CONNECTION -- assert membership, pairwise.
// ---------------------------------------------------------------------

// One attempted (or failed-to-resolve) member/group pair. `member` and/or
// `group` are populated once known, even on failure, so a rejection can
// be reported against the specific address that failed to resolve.
struct ConnectionOutcome {
  bool added = false;
  std::optional<codec::Address> member;
  std::optional<codec::Address> group;
  std::string reason;  // set iff !added.
};

struct AddConnectionResult {
  std::vector<ConnectionOutcome> outcomes;
};

// Resolves `op.group` and each of `op.elements` (either may be a terminal
// wildcard, resolved via Controller::gather to its existing address
// range; an explicit address is used as-is after an existence check) and
// enumerates every (member, group) pair across the resolved sets, adding
// each via Controller::add_membership (idempotent -- a re-add is a clean
// success, not an error).
AddConnectionResult add_connection(dbk::Controller &ctl, const command::AddConnection &op);

// ---------------------------------------------------------------------
// DELETE_RECORD / DELETE_CONNECTION -- the destructive pair. Single,
// specific-target only, gated behind a FULL specific-target confirmation
// (NOTES.md "DELETE gates -- firmed 2026-09-18"): the caller must
// resupply the exact target as `confirm`, not a blanket confirm=true
// flag -- re-stating the target IS the confirmation mechanism. A
// mismatched confirm rejects cleanly without touching the store.
// ---------------------------------------------------------------------

struct DeleteResult {
  bool deleted = false;
  std::string reason;  // set iff !deleted.
};

// `confirm` must equal `op` exactly (confirm.token == op.token). On a
// match, probes parents_of/children_of/members_of/member_of (only-follow,
// no FK-error sniffing): if the token is still referenced anywhere, it is
// reported as a clean reject-on-referenced (PLAN.md G10) and
// Controller::delete_token is never called; otherwise it is called
// locally. A genuine store/connection error still throws.
DeleteResult delete_record(dbk::Controller &ctl, const command::DeleteRecord &op,
                            const command::DeleteRecord &confirm);

// `confirm` must equal `op` exactly (confirm.a == op.a && confirm.b ==
// op.b). Executes Controller::delete_pair locally on a match, after
// confirming the named edge actually exists (membership axis only; see
// README.md for the a/b -> member/group mapping this module uses).
DeleteResult delete_connection(dbk::Controller &ctl, const command::DeleteConnection &op,
                                const command::DeleteConnection &confirm);

}  // namespace update
