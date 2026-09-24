#include "declare_core.h"

#include "span_planner.h"

namespace declare {

namespace {

// ---------------------------------------------------------------------
// Reference resolution -- shared by PARENTS constituents, MEMBERS,
// MEMBER_OF (shared and per_member). A Reference is exactly one of a
// plain address (must already exist -- "must pre-exist") or a full
// nested DeclareRecord (recursed via execute(); "a nested mint collapses
// to its address" -- PLAN.md I.E, II.3).
// ---------------------------------------------------------------------

struct RefResolution {
  bool ok = false;
  std::optional<codec::Address> address;
  std::string reason;
};

RefResolution ResolveFailed(std::string reason) {
  return RefResolution{false, std::nullopt, std::move(reason)};
}

RefResolution ResolveOk(codec::Address address) {
  return RefResolution{true, std::move(address), ""};
}

// resolve_reference() and execute() are mutually recursive (a Reference
// may nest a full DeclareRecord).
RefResolution resolve_reference(dbk::Controller &ctl, const command::Reference &ref);

RefResolution resolve_reference(dbk::Controller &ctl, const command::Reference &ref) {
  if (ref.address.has_value()) {
    if (!ctl.token_exists(*ref.address)) {
      return ResolveFailed(
          "referenced address does not exist (must pre-exist or be inline-nested)");
    }
    return ResolveOk(*ref.address);
  }

  Result nested = execute(ctl, *ref.nested);
  if (nested.outcomes.size() != 1) {
    return ResolveFailed(
        "a nested declare used as a reference must collapse to exactly one "
        "address; this one produced " +
        std::to_string(nested.outcomes.size()));
  }
  const Outcome &o = nested.outcomes[0];
  if (!o.ingested) {
    return ResolveFailed("nested declare rejected: " + o.reason);
  }
  return ResolveOk(*o.address);
}

// Resolves every reference in `refs`, in order, all-or-nothing: the first
// failure abandons the rest and reports that failure. Used to resolve a
// member's own required references (its constituents, or a grouping
// node's MEMBERS roster) before anything is written -- see README.md
// "Per-member atomicity".
struct RefListResolution {
  bool ok = true;
  std::vector<codec::Address> addresses;
  std::string reason;
};

RefListResolution resolve_reference_list(dbk::Controller &ctl,
                                          const std::vector<command::Reference> &refs) {
  RefListResolution out;
  out.addresses.reserve(refs.size());
  for (const auto &ref : refs) {
    RefResolution r = resolve_reference(ctl, ref);
    if (!r.ok) {
      out.ok = false;
      out.reason = r.reason;
      return out;
    }
    out.addresses.push_back(*r.address);
  }
  return out;
}

// NOTATION's per-slot surface form. A blank slot (absent from a short
// NOTATION vector, or an explicitly-nullopt entry -- "successive commas")
// stores blank: mint()'s `notation` parameter is a plain std::string with
// no NULL channel of its own, so this module's rendering of "blank" is
// the empty string (NOTES.md "The literal intake formula": "a blank ...
// field ... reads blank now"). PLAN.md II.3.
std::string notation_at(const std::vector<std::optional<std::string>> &notation,
                         std::size_t i) {
  if (i < notation.size() && notation[i].has_value()) {
    return *notation[i];
  }
  return "";
}

// The MEMBER_OF groups that apply to PARENTS member `i`: section 1
// (shared, broadcasts to all N) union section 2's sparse per-member entry
// for index `i`, if any (NOTES.md "The literal intake formula" MEMBER_OF
// paragraph; command_ir.h DeclareRecord::MemberOf).
std::vector<command::Reference> member_of_groups_for(const command::DeclareRecord &node,
                                                      std::size_t i) {
  std::vector<command::Reference> groups;
  if (!node.member_of.has_value()) {
    return groups;
  }
  groups = node.member_of->shared;
  auto it = node.member_of->per_member.find(i);
  if (it != node.member_of->per_member.end()) {
    groups.insert(groups.end(), it->second.begin(), it->second.end());
  }
  return groups;
}

// ---------------------------------------------------------------------
// Placement resolution: one PARENTS member's concrete address, from the
// already-computed span plan (command::plan()).
// ---------------------------------------------------------------------

struct PlacementResolution {
  bool ok = false;
  std::optional<codec::Address> address;
  std::string reason;
};

// A kNestedDeclare slot mints its nested statement and collapses this
// member's placement onto whatever address that mint produced (PLAN.md
// I.E: "a nested declare mints one item -> one address -> one
// self-delimiting slot"). An undeclared-hook slot, or a slot left
// unresolved by a pending AFTER segment (G5, the deferred trunk map),
// resolves to nothing -- reported, never guessed.
PlacementResolution resolve_placement(dbk::Controller &ctl,
                                       const command::PlannedSlot &slot) {
  if (slot.is_undeclared_hook) {
    return PlacementResolution{
        false, std::nullopt,
        "slot is the inert undeclared hook -- no address assigned "
        "(extrapolate_address is deferred)"};
  }
  if (slot.nested != nullptr) {
    Result nested = execute(ctl, *slot.nested);
    if (nested.outcomes.size() != 1 || !nested.outcomes[0].ingested) {
      return PlacementResolution{
          false, std::nullopt,
          "ADDRESS-span nested declare did not collapse to one placed address"};
    }
    return PlacementResolution{true, nested.outcomes[0].address, ""};
  }
  if (slot.address.has_value()) {
    return PlacementResolution{true, slot.address, ""};
  }
  return PlacementResolution{
      false, std::nullopt,
      "slot has no concrete address (an AFTER segment pending the deferred "
      "trunk map, G5)"};
}

// ---------------------------------------------------------------------
// Structure node (PARENTS present). May also carry MEMBERS (a node may
// be structure and grouping at once -- PLAN.md I.B).
// ---------------------------------------------------------------------

Result execute_structure(dbk::Controller &ctl, const command::DeclareRecord &node) {
  Result result;
  const std::size_t n = node.parents->size();

  command::PlanResult plan_result = command::plan(*node.address, n);
  if (plan_result.status == command::PlanStatus::kInvalid) {
    // Not expected for an already-validated node; defensive only.
    result.outcomes.push_back(
        Outcome{false, std::nullopt, "ADDRESS span invalid: " + plan_result.reason});
    return result;
  }

  for (std::size_t i = 0; i < n; ++i) {
    if (i >= plan_result.slots.size()) {
      result.outcomes.push_back(Outcome{
          false, std::nullopt,
          "ADDRESS span pending the deferred trunk map before this slot (G5)"});
      continue;
    }

    PlacementResolution placement = resolve_placement(ctl, plan_result.slots[i]);
    if (!placement.ok) {
      result.outcomes.push_back(Outcome{false, std::nullopt, placement.reason});
      continue;
    }

    // Per-member atomicity (README.md): resolve this member's own
    // constituents AND its MEMBER_OF groups before writing anything -- a
    // rejected reference rejects the whole point, never a partially-
    // minted token (PLAN.md I.G "a rejected point -> un-ingested").
    RefListResolution constituents = resolve_reference_list(ctl, (*node.parents)[i]);
    if (!constituents.ok) {
      result.outcomes.push_back(Outcome{
          false, std::nullopt,
          "member " + std::to_string(i) + " constituent: " + constituents.reason});
      continue;
    }
    RefListResolution groups =
        resolve_reference_list(ctl, member_of_groups_for(node, i));
    if (!groups.ok) {
      result.outcomes.push_back(
          Outcome{false, std::nullopt,
                  "member " + std::to_string(i) + " MEMBER_OF: " + groups.reason});
      continue;
    }

    std::vector<dbk::Constituent> mint_constituents;
    mint_constituents.reserve(constituents.addresses.size());
    for (const auto &addr : constituents.addresses) {
      // References only, no per-position mass (PLAN.md II.3: "no
      // per-constituent mass -- LINK writes SQL NULL").
      mint_constituents.push_back(dbk::Constituent{addr, std::nullopt});
    }

    const codec::Address placed = *placement.address;
    // Mass blank at declare (pending work for the deferred aggregation
    // workstream); the analyst path always passes nullopt. The seed
    // floor is minted directly through the controller, never through
    // this core.
    ctl.mint(placed, notation_at(node.notation, i), mint_constituents, std::nullopt);

    for (const auto &group_addr : groups.addresses) {
      // Door is SEE-idempotent (ON CONFLICT DO NOTHING, both directions) --
      // no caller-side dedup guard needed.
      ctl.add_membership(placed, group_addr);
    }

    result.outcomes.push_back(Outcome{true, placed, ""});
  }

  // Grouping layered on top of the placed structure member -- the
  // "mint form" of the naming literal (NOTES.md "Grouping-node naming
  // literal", ruling #1/#2): a mixed PARENTS+MEMBERS node mints a SINGLE
  // literal (N=1, enforced by command::validate_declare) that is both the
  // structure and the label; its own newly-placed address is the naming
  // literal MEMBERS/MEMBER_OF are authored against. This loop is still
  // written generically over `result.outcomes` (N is always 1 here by
  // validation, but nothing below assumes it). A MEMBERS reference that
  // fails to resolve skips just that edge; it does not retract the
  // already-ingested point.
  if (node.members.has_value()) {
    for (const auto &outcome : result.outcomes) {
      if (!outcome.ingested) {
        continue;
      }
      const codec::Address &group_addr = *outcome.address;
      for (const auto &mref : *node.members) {
        RefResolution mres = resolve_reference(ctl, mref);
        if (mres.ok) {
          ctl.add_membership(*mres.address, group_addr);
        }
      }
    }
  }

  return result;
}

// ---------------------------------------------------------------------
// Grouping node, use-provided-ID form (MEMBERS present, PARENTS absent).
// NOTES.md "Grouping-node naming literal" ruling #1: ADDRESS references a
// PRE-EXISTING naming literal -- no PARENTS composition to mint here.
// (The other form -- PARENTS present, i.e. mint -- is handled by
// execute_structure() above, since minting is exactly what a structure
// node already does; MEMBERS is simply also present there.)
// ---------------------------------------------------------------------

Result execute_grouping_use_provided_id(dbk::Controller &ctl,
                                         const command::DeclareRecord &node) {
  Result result;

  // command::validate_declare guarantees ADDRESS is present for this node
  // shape (MEMBERS present, PARENTS absent -- ruling #1). Resolved via the
  // same span-planning machinery a structure placement uses (handles a
  // plain Direct/Pin/FROM value, and -- since ruling #3's rejection is
  // scoped to PARENTS being present -- also a kNestedDeclare segment,
  // which mints inline and trivially satisfies the existence check
  // below; see README.md "Use-provided-ID via an inline nested mint").
  command::PlanResult plan_result = command::plan(*node.address, 1);
  if (plan_result.status == command::PlanStatus::kInvalid) {
    // Not expected for an already-validated node; defensive only.
    result.outcomes.push_back(
        Outcome{false, std::nullopt, "ADDRESS span invalid: " + plan_result.reason});
    return result;
  }
  if (plan_result.slots.empty()) {
    result.outcomes.push_back(Outcome{
        false, std::nullopt,
        "ADDRESS span pending the deferred trunk map before this slot (G5)"});
    return result;
  }

  PlacementResolution placement = resolve_placement(ctl, plan_result.slots[0]);
  if (!placement.ok) {
    result.outcomes.push_back(Outcome{false, std::nullopt, placement.reason});
    return result;
  }
  const codec::Address naming_literal = *placement.address;
  if (!ctl.token_exists(naming_literal)) {
    result.outcomes.push_back(Outcome{
        false, std::nullopt,
        "grouping node's ADDRESS (use-provided-ID form) does not reference "
        "an existing token"});
    return result;
  }

  // The point's whole content is MEMBERS (plus optional MEMBER_OF);
  // per-member atomicity applies here too (README.md): all required
  // references resolve before anything is written, or the single point
  // rejects outright.
  RefListResolution members = resolve_reference_list(ctl, *node.members);
  if (!members.ok) {
    result.outcomes.push_back(Outcome{false, std::nullopt, "MEMBERS: " + members.reason});
    return result;
  }
  RefListResolution groups = resolve_reference_list(ctl, member_of_groups_for(node, 0));
  if (!groups.ok) {
    result.outcomes.push_back(
        Outcome{false, std::nullopt, "MEMBER_OF: " + groups.reason});
    return result;
  }

  // Door is SEE-idempotent (ON CONFLICT DO NOTHING, both directions) --
  // no caller-side dedup guard needed.
  for (const auto &member_addr : members.addresses) {
    ctl.add_membership(member_addr, naming_literal);
  }
  for (const auto &group_addr : groups.addresses) {
    ctl.add_membership(naming_literal, group_addr);
  }

  result.outcomes.push_back(Outcome{true, naming_literal, ""});
  return result;
}

}  // namespace

Result execute(dbk::Controller &ctl, const command::DeclareRecord &node) {
  command::ValidationResult validation = command::validate_declare(node);
  if (!validation.ok()) {
    Result result;
    result.outcomes.push_back(
        Outcome{false, std::nullopt, "invalid declare: " + validation.reason});
    return result;
  }

  const bool has_parents = node.parents.has_value();
  const bool has_members = node.members.has_value();

  if (has_parents) {
    // Mint form (NOTES.md "Grouping-node naming literal"): PARENTS
    // present mints the naming literal (or a plain structure token, if
    // MEMBERS is absent), placed at ADDRESS if given, else
    // manager-placed. "Manager-placed" (ADDRESS entirely absent) has no
    // implementation here: there is no next-slot/address-assignment
    // mechanism anywhere in this codebase yet (PLAN.md Part V, G4/G5 --
    // open, Patrick's) and inventing one (which trunk, which kind) would
    // be exactly the "no invented complication" this module is bound
    // not to do. Rejected cleanly, not guessed. (A plain structure node,
    // MEMBERS absent, cannot reach this: command::validate_declare
    // already requires ADDRESS whenever MEMBERS is absent.)
    if (has_members && !node.address.has_value()) {
      Result result;
      result.outcomes.push_back(Outcome{
          false, std::nullopt,
          "manager-placed address assignment (mint form, ADDRESS omitted) is "
          "not implemented -- no next-slot/address-assignment mechanism "
          "exists yet (PLAN.md Part V, G4/G5); supply a target ADDRESS to "
          "mint at, or reference an existing one via the use-provided-ID "
          "form instead"});
      return result;
    }
    return execute_structure(ctl, node);
  }

  // !has_parents: command::validate_declare guarantees has_members here
  // (a node with neither PARENTS nor MEMBERS is rejected upstream) and
  // that ADDRESS is present (ruling #1's use-provided-ID form).
  return execute_grouping_use_provided_id(ctl, node);
}

}  // namespace declare
