#include "update_core.h"

#include <algorithm>

#include "span_planner.h"

namespace update {

namespace {

// A wildcard is a terminal partial address -- same test every other
// module in this codebase uses (command/command_ir.cpp, read/read_core.cpp).
bool is_wildcard(const codec::Address &address) {
  return !address.empty() && address.back().partial;
}

// ---------------------------------------------------------------------
// MOVE_RECORD
// ---------------------------------------------------------------------

MoveOutcome StatementRejected(std::string reason) {
  return MoveOutcome{false, std::nullopt, std::nullopt, std::move(reason)};
}

// Concatenates every source selector's resolved token list, in selector
// order, each selector's own tokens in the order it produces them
// (kExplicit: the one address as given; kRange/kPrefix: Controller::gather's
// PK/address order). See README.md "Source ordering" -- this is the
// module's own decision for how several coexisting selectors combine into
// one ordered list ("source in PK/byte order <-> destination slots in
// order" per the team-lead brief), not a re-sort of the whole combined
// set into global PK order.
std::vector<codec::Address> resolve_sources(dbk::Controller &ctl,
                                             const command::MoveRecord &op) {
  std::vector<codec::Address> sources;
  for (const command::MoveSource &src : op.sources) {
    switch (src.kind) {
      case command::MoveSource::Kind::kExplicit:
        // Existence is checked per-token during placement, not here --
        // an explicit source still occupies a destination slot (and thus
        // counts toward N) even if it turns out not to exist; see
        // README.md.
        sources.push_back(src.from);
        break;
      case command::MoveSource::Kind::kRange:
        for (codec::Address &a : ctl.gather(src.from, *src.to)) {
          sources.push_back(std::move(a));
        }
        break;
      case command::MoveSource::Kind::kPrefix:
        for (codec::Address &a : ctl.gather(src.from)) {
          sources.push_back(std::move(a));
        }
        break;
    }
  }
  return sources;
}

}  // namespace

MoveResult move_record(dbk::Controller &ctl, const command::MoveRecord &op) {
  MoveResult result;

  command::ValidationResult validation = command::validate_move_record(op);
  if (!validation.ok()) {
    result.outcomes.push_back(StatementRejected("invalid move_record: " + validation.reason));
    return result;
  }

  // F-1, the deferred half (PLAN.md II.5): reject a destination
  // containing a mint-bearing nested-declare or the inert undeclared
  // hook -- a relocation mints nothing. command::validate_move_record
  // (an IR-level, no-DB-access check) permits these shapes structurally;
  // this module is where they are actually rejected, per the team-lead
  // brief.
  for (std::size_t i = 0; i < op.destination.size(); ++i) {
    const command::AddressSegment::Kind kind = op.destination[i].kind;
    if (kind == command::AddressSegment::Kind::kNestedDeclare) {
      result.outcomes.push_back(StatementRejected(
          "MOVE_RECORD destination segment " + std::to_string(i) +
          " is a nested declare -- a relocation mints nothing (deferred half of F-1)"));
      return result;
    }
    if (kind == command::AddressSegment::Kind::kUndeclared) {
      result.outcomes.push_back(StatementRejected(
          "MOVE_RECORD destination segment " + std::to_string(i) +
          " is the undeclared hook -- a relocation mints nothing (deferred half of F-1)"));
      return result;
    }
  }

  const std::vector<codec::Address> sources = resolve_sources(ctl, op);
  const std::size_t n = sources.size();

  // Cover-N: for an all-explicit MoveRecord this was already checked at
  // IR level (validate_move_record), against the same N this resolves
  // to; for a wildcard/range source, N is only knowable here, against
  // the live store -- this is exactly the execution-time cover-N check
  // PLAN.md II.5 assigns to this module.
  command::PlanResult plan_result = command::plan(op.destination, n);
  if (plan_result.status == command::PlanStatus::kInvalid) {
    result.outcomes.push_back(StatementRejected(
        "MOVE_RECORD destination span does not cover the resolved source "
        "count N=" +
        std::to_string(n) + ": " + plan_result.reason));
    return result;
  }

  // Per-source-token processing, in order (README.md "Ordering is
  // semantic"): each source's own success/failure is independent, same
  // discipline as declare::execute_structure's per-member outcomes.
  for (std::size_t i = 0; i < n; ++i) {
    const codec::Address &from_addr = sources[i];

    if (i >= plan_result.slots.size()) {
      // Not reachable given plan()'s contract (slots.size() == n on
      // kValid/kPendingSeam), but guarded rather than indexed blindly.
      result.outcomes.push_back(MoveOutcome{
          false, from_addr, std::nullopt,
          "destination span pending the deferred trunk map before this slot (G5)"});
      continue;
    }
    const command::PlannedSlot &slot = plan_result.slots[i];
    if (!slot.address.has_value()) {
      // The only way to reach an address-less slot here is an
      // AFTER-pending origin (G5): kNestedDeclare/kUndeclared were
      // already rejected above, for the whole statement, before
      // resolving sources or planning at all.
      result.outcomes.push_back(MoveOutcome{
          false, from_addr, std::nullopt,
          "destination slot pending the deferred trunk map (G5)"});
      continue;
    }
    const codec::Address to_addr = *slot.address;

    if (!ctl.token_exists(from_addr)) {
      result.outcomes.push_back(
          MoveOutcome{false, from_addr, to_addr, "source token does not exist"});
      continue;
    }

    if (from_addr == to_addr) {
      // Degenerate no-op: the token is already at its "destination".
      // Not stated in the spec either way; calling Controller::rekey
      // here would spuriously trip its own "new_id already exists"
      // guard (which exists to catch a genuine identity merge, not this
      // case) -- see README.md "Same-address MOVE".
      result.outcomes.push_back(MoveOutcome{true, from_addr, to_addr, ""});
      continue;
    }

    try {
      ctl.rekey(from_addr, to_addr);
      result.outcomes.push_back(MoveOutcome{true, from_addr, to_addr, ""});
    } catch (const std::exception &e) {
      // Covers, in particular, a destination collision ("new_id already
      // exists") -- a real rejection (no aliasing/forwarding), not
      // swallowed as a whole-statement failure: the other sources in
      // this MOVE are still attempted.
      result.outcomes.push_back(MoveOutcome{false, from_addr, to_addr, e.what()});
    }
  }

  return result;
}

// ---------------------------------------------------------------------
// ADD_CONNECTION
// ---------------------------------------------------------------------

namespace {

struct ExpandResult {
  bool ok = true;
  std::vector<codec::Address> addresses;  // resolved, EXISTING tokens.
  std::string reason;                     // set iff !ok.
};

// Resolves one ADD_CONNECTION operand (the group, or one element) to its
// set of existing tokens. A terminal wildcard resolves via gather(),
// which reads straight off `token` -- so everything it returns already
// exists; a plain address is used as-is after an explicit existence
// check (mirrors declare::resolve_reference's "must pre-exist" check).
ExpandResult expand_side(dbk::Controller &ctl, const codec::Address &addr) {
  if (is_wildcard(addr)) {
    return ExpandResult{true, ctl.gather(addr), ""};
  }
  if (!ctl.token_exists(addr)) {
    return ExpandResult{
        false, {}, "endpoint does not exist -- ADD_CONNECTION mints nothing, endpoints must pre-exist"};
  }
  return ExpandResult{true, {addr}, ""};
}

}  // namespace

AddConnectionResult add_connection(dbk::Controller &ctl, const command::AddConnection &op) {
  AddConnectionResult result;

  command::ValidationResult validation = command::validate_add_connection(op);
  if (!validation.ok()) {
    result.outcomes.push_back(ConnectionOutcome{
        false, std::nullopt, std::nullopt, "invalid add_connection: " + validation.reason});
    return result;
  }

  // Group is the scalar frame that must lead (NOTES.md "UPDATE --
  // record-tier ops"): if it fails to resolve to anything, there is
  // nothing to attach elements to at all -- a whole-statement rejection,
  // not a per-pair one.
  ExpandResult groups = expand_side(ctl, op.group);
  if (!groups.ok) {
    result.outcomes.push_back(
        ConnectionOutcome{false, std::nullopt, op.group, groups.reason});
    return result;
  }

  // Each element is resolved and reported independently (README.md
  // "Per-pair granularity"): a raw element entry that fails to resolve
  // rejects just that entry, not the whole statement, and every
  // resolved (member, group) pair -- the cross product of this
  // element's resolution with the group's -- is written and reported on
  // its own.
  for (const codec::Address &element : op.elements) {
    ExpandResult members = expand_side(ctl, element);
    if (!members.ok) {
      result.outcomes.push_back(
          ConnectionOutcome{false, element, std::nullopt, members.reason});
      continue;
    }
    for (const codec::Address &member_addr : members.addresses) {
      for (const codec::Address &group_addr : groups.addresses) {
        // Door is SEE-idempotent (ON CONFLICT DO NOTHING, both
        // directions) -- a re-add is a clean success, never an error.
        ctl.add_membership(member_addr, group_addr);
        result.outcomes.push_back(
            ConnectionOutcome{true, member_addr, group_addr, ""});
      }
    }
  }

  return result;
}

// ---------------------------------------------------------------------
// DELETE_RECORD / DELETE_CONNECTION
// ---------------------------------------------------------------------

namespace {

// True iff `token` is still referenced by any of the four relationship
// stores (an only-follow probe over the door's own reads, per Patrick's
// 2026-09-18 robustness ruling -- see README.md "G10"). This is the
// isolated-tokens-only test G10 is held to: a token with no incoming
// structure or membership edges anywhere is safe to delete outright.
bool still_referenced(dbk::Controller &ctl, const codec::Address &token) {
  return !ctl.parents_of(token).empty() || !ctl.children_of(token).empty() ||
         !ctl.members_of(token).empty() || !ctl.member_of(token).empty();
}

}  // namespace

DeleteResult delete_record(dbk::Controller &ctl, const command::DeleteRecord &op,
                            const command::DeleteRecord &confirm) {
  command::ValidationResult validation = command::validate_delete_record(op);
  if (!validation.ok()) {
    return DeleteResult{false, "invalid delete_record: " + validation.reason};
  }

  // Full specific-target confirmation (NOTES.md "DELETE gates -- firmed
  // 2026-09-18"): the caller must resupply the exact target. No
  // blanket confirm=true flag exists to short-circuit this.
  if (confirm.token != op.token) {
    return DeleteResult{
        false,
        "confirmation does not match the target -- nothing deleted (no "
        "bare/fire-and-forget delete)"};
  }

  if (!ctl.token_exists(op.token)) {
    return DeleteResult{false, "target token does not exist"};
  }

  // G10 (Patrick, robustness swap 2026-09-18): an only-follow reference
  // probe over the door's own reads, in place of sniffing the FK-error
  // string a failed DELETE would raise. If the token is still referenced
  // anywhere (see README.md "G10" for why these four reads cover every
  // FK column that could block the delete), reject cleanly WITHOUT
  // attempting delete_token at all -- no cascade/repoint invented, and no
  // reliance on the controller's error-message wording.
  if (still_referenced(ctl, op.token)) {
    return DeleteResult{
        false, "target token is still referenced by dependents -- "
               "reject-on-referenced (G10), no cascade invented"};
  }

  ctl.delete_token(op.token);
  return DeleteResult{true, ""};
}

DeleteResult delete_connection(dbk::Controller &ctl, const command::DeleteConnection &op,
                                const command::DeleteConnection &confirm) {
  command::ValidationResult validation = command::validate_delete_connection(op);
  if (!validation.ok()) {
    return DeleteResult{false, "invalid delete_connection: " + validation.reason};
  }

  if (confirm.a != op.a || confirm.b != op.b) {
    return DeleteResult{
        false,
        "confirmation does not match the target pair -- nothing deleted (no "
        "bare/fire-and-forget delete)"};
  }

  // op.a is read as the member, op.b as the group -- matching
  // Controller::add_membership(member, group) / Controller::delete_pair
  // (member, group)'s own parameter order (see README.md "The a/b ->
  // member/group mapping"). Confirming the edge actually exists in that
  // direction, before calling delete_pair, both gives an accurate
  // `deleted` result (delete_pair's own DELETEs are silent no-ops on a
  // non-match, since it issues no row-count check) and validates the
  // a=member/b=group reading itself.
  const std::vector<codec::Address> groups_of_a = ctl.member_of(op.a);
  const bool exists =
      std::find(groups_of_a.begin(), groups_of_a.end(), op.b) != groups_of_a.end();
  if (!exists) {
    return DeleteResult{false, "no such membership edge exists between the pair"};
  }

  ctl.delete_pair(op.a, op.b);
  return DeleteResult{true, ""};
}

}  // namespace update
