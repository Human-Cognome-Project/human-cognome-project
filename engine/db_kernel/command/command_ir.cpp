#include "command_ir.h"

#include <algorithm>

#include "span_planner.h"

namespace command {

namespace {

ValidationResult Invalid(std::string reason) {
  return ValidationResult{ValidationStatus::kInvalid, std::move(reason)};
}

ValidationResult Valid() {
  return ValidationResult{ValidationStatus::kValid, {}};
}

// A wildcard is a prefix/context address -- its last element is partial
// (codec's wildcard-tail marker), selecting a whole block/trunk rather
// than one leaf. Every record-tier op except MOVE_RECORD's source is
// explicit-ids-only (NOTES.md UPDATE ops: DELETE is "specific IDs and
// connections ONLY -- no wildcards"), so this guards those ops' address
// fields. An empty address has no last element and is never a wildcard
// by this definition.
bool is_wildcard(const codec::Address &address) {
  return !address.empty() && address.back().partial;
}

bool is_explicit_address(const codec::Address &address) {
  return codec::is_valid_address(address) && !is_wildcard(address);
}

ValidationResult validate_reference(const Reference &ref) {
  const bool has_address = ref.address.has_value();
  const bool has_nested = (ref.nested != nullptr);
  if (has_address == has_nested) {
    return Invalid(
        "a reference must be either a plain address or a nested declare, "
        "not both or neither");
  }
  if (has_address) {
    if (!codec::is_valid_address(*ref.address)) {
      return Invalid("reference address is not a valid codec address");
    }
    return Valid();
  }
  return validate_declare(*ref.nested);
}

ValidationResult validate_reference_list(const std::vector<Reference> &refs) {
  for (const auto &ref : refs) {
    auto result = validate_reference(ref);
    if (!result.ok()) {
      return result;
    }
  }
  return Valid();
}

// Recurses into any nested declare occupying an ADDRESS span slot
// (PLAN.md I.E: "a nested declare mints one item ... legal anywhere").
// The planner (span_planner.cpp) only accounts for the slot; the nested
// statement's own structural validity is this module's job.
ValidationResult validate_address_span_nested(const AddressSpan &span) {
  for (const auto &segment : span) {
    if (segment.kind == AddressSegment::Kind::kNestedDeclare) {
      if (segment.nested == nullptr) {
        return Invalid("a nested-declare ADDRESS segment carries no statement");
      }
      auto result = validate_declare(*segment.nested);
      if (!result.ok()) {
        return result;
      }
    } else if (segment.origin.has_value() &&
               !codec::is_valid_address(*segment.origin)) {
      return Invalid("ADDRESS segment origin is not a valid codec address");
    } else if (segment.to_bound.has_value() &&
               !codec::is_valid_address(*segment.to_bound)) {
      return Invalid("ADDRESS segment TO bound is not a valid codec address");
    }
  }
  return Valid();
}

}  // namespace

ValidationResult validate_declare(const DeclareRecord &node) {
  const bool has_parents = node.parents.has_value();
  const bool has_members = node.members.has_value();

  // Required downward field: PARENTS => structure, MEMBERS => grouping.
  // No TYPE anywhere -- this presence check IS the validation (NOTES.md
  // "Relationship model & type -- firmed 2026-09-17").
  if (!has_parents && !has_members) {
    return Invalid(
        "declare node carries neither PARENTS (structure) nor MEMBERS "
        "(grouping) -- at least one downward field is required");
  }

  // N: the number of tokens this node itself declares. PARENTS -- when
  // present -- IS the member array and sets N (PLAN.md I.B). A
  // pure-grouping node declares exactly its one naming-literal token
  // (PLAN.md I.B "Grouping-node identity"; NOTES.md's label dual gives no
  // grouping-side analogue of "PARENTS sets N", so N=1 there is this
  // module's bookkeeping choice, not a stated spec value -- flagged in
  // the handoff report).
  const std::size_t n = has_parents ? node.parents->size() : 1;

  if (has_parents) {
    if (node.parents->empty()) {
      return Invalid(
          "PARENTS is present but empty -- it is the member array and "
          "must set N >= 1");
    }
    if (has_members && node.parents->size() != 1) {
      // Ruling #2 (Patrick, build-phase; default reject-until-ruled): a
      // mixed PARENTS+MEMBERS node mints its naming literal as a SINGLE
      // literal (N=1). N>1 here would mean N naming literals sharing one
      // MEMBERS roster -- an unspecified broadcast case -- so this
      // fails fast rather than guessing a semantic for it.
      return Invalid(
          "a mixed PARENTS+MEMBERS node must declare exactly one naming "
          "literal (N=1) -- N=" +
          std::to_string(node.parents->size()) +
          " is unspecified for this combination");
    }
    for (std::size_t i = 0; i < node.parents->size(); ++i) {
      const ConstituentList &list = (*node.parents)[i];
      if (list.size() < 2) {
        return Invalid("PARENTS member " + std::to_string(i) +
                        " has fewer than 2 constituent references "
                        "(anti-alias floor)");
      }
      auto result = validate_reference_list(list);
      if (!result.ok()) {
        return result;
      }
    }
  }

  if (has_members) {
    if (node.members->empty()) {
      return Invalid("MEMBERS is present but empty -- the grouping floor is >= 1");
    }
    auto result = validate_reference_list(*node.members);
    if (!result.ok()) {
      return result;
    }
  }

  // Grouping-node naming literal (ruling #1, Patrick, build-phase --
  // SUPERSEDES the earlier "ADDRESS forbidden on a grouping node" rule
  // and drops any NOTATION-based identity requirement; the naming
  // literal is referenced via an address, never via prose). A grouping
  // node (MEMBERS present) establishes its naming literal one of two
  // structurally distinguishable ways:
  //   mint form:            PARENTS present -- the naming literal is
  //                         minted from that composition; ADDRESS, if
  //                         present, is the mint's placement target,
  //                         else the manager places it (see the
  //                         has_parents-only ADDRESS-required check
  //                         below, which this does NOT apply to once
  //                         MEMBERS is also present).
  //   use-provided-ID form: PARENTS absent, ADDRESS present -- ADDRESS
  //                         references the pre-existing naming literal.
  // Neither PARENTS nor ADDRESS establishes no naming literal at all.
  // Whether a provided ADDRESS actually exists in the store is an
  // EXECUTION-time check for the declare core, not this IR -- only the
  // form is validated here.
  if (has_members && !has_parents && !node.address.has_value()) {
    return Invalid(
        "a grouping node (MEMBERS present) with neither PARENTS nor "
        "ADDRESS establishes no naming literal -- it must either mint one "
        "(PARENTS) or reference an existing one (ADDRESS)");
  }

  // A plain structure node (PARENTS present, MEMBERS absent) still
  // requires its own ADDRESS; the relaxation above applies only once
  // MEMBERS is also present (the mint form allows manager-placement).
  if (has_parents && !has_members && !node.address.has_value()) {
    return Invalid(
        "a structure node (PARENTS present) requires an ADDRESS span "
        "covering its N slots");
  }

  // NOTATION co-index: length must equal N exactly when present. Blank
  // (nullopt/empty) individual entries are legal ("successive commas")
  // and are a content rule, not a length rule -- they do NOT relax this
  // check (PLAN.md II.1).
  if (!node.notation.empty() && node.notation.size() != n) {
    return Invalid("NOTATION length " + std::to_string(node.notation.size()) +
                    " does not match N=" + std::to_string(n) +
                    " (co-index violation)");
  }

  // MEMBER_OF: section 1 (shared) is an ordinary reference list; section
  // 2 (per_member) is SPARSE (<= N) -- only its indices are range-checked
  // against N, its size is never compared to N.
  if (node.member_of.has_value()) {
    auto result = validate_reference_list(node.member_of->shared);
    if (!result.ok()) {
      return result;
    }
    for (const auto &entry : node.member_of->per_member) {
      if (entry.first >= n) {
        return Invalid("MEMBER_OF section 2 index " +
                        std::to_string(entry.first) +
                        " is out of range for N=" + std::to_string(n));
      }
      auto per_result = validate_reference_list(entry.second);
      if (!per_result.ok()) {
        return per_result;
      }
    }
  }

  // ADDRESS span: must cover exactly N slots, and any nested declare
  // occupying a slot recurses into full structural validation.
  if (node.address.has_value()) {
    if (has_parents) {
      // Ruling #3 (Patrick, build-phase; reject): when PARENTS is
      // present, EVERY one of its N slots already has its own
      // PARENTS[i] composition. A nested-declare ADDRESS segment mints
      // an entirely separate token from ITS OWN PARENTS for that slot,
      // so the two can never be reconciled -- PARENTS[i]'s constituents
      // would simply be silently dropped, never consulted. Rejected as
      // malformed regardless of the nested declare's own validity.
      for (std::size_t i = 0; i < node.address->size(); ++i) {
        if ((*node.address)[i].kind == AddressSegment::Kind::kNestedDeclare) {
          return Invalid(
              "ADDRESS segment " + std::to_string(i) +
              " is a nested declare, which collides with this node's own "
              "PARENTS composition for that slot (PARENTS[i] would be "
              "silently dropped) -- not permitted when PARENTS is present");
        }
      }
    }

    auto nested_result = validate_address_span_nested(*node.address);
    if (!nested_result.ok()) {
      return nested_result;
    }
    PlanResult plan_result = plan(*node.address, n);
    if (plan_result.status == PlanStatus::kInvalid) {
      return Invalid("ADDRESS span does not cover N=" + std::to_string(n) +
                      ": " + plan_result.reason);
    }
    // kPendingSeam (an AFTER segment awaiting the deferred trunk map) is
    // NOT a validation failure -- the span is structurally well-formed;
    // it just cannot be fully resolved yet (G5).
  }

  return Valid();
}

ValidationResult validate_read(const ReadRecord &op) {
  // NOTES.md/PLAN.md do not state a no-wildcard rule for READ (unlike
  // DELETE's explicit "specific IDs and connections ONLY -- no
  // wildcards"). Per Patrick: don't invent a restriction OR an allowance
  // here -- a partial (wildcard-tail) address is neither specially
  // rejected nor specially accepted; it is simply a valid codec address
  // like any other.
  if (!codec::is_valid_address(op.anchor)) {
    return Invalid("READ_RECORD anchor is not a valid codec address");
  }
  for (std::size_t i = 0; i < op.exclusions.size(); ++i) {
    if (!codec::is_valid_address(op.exclusions[i])) {
      return Invalid("READ_RECORD exclusion " + std::to_string(i) +
                      " is not a valid codec address");
    }
  }
  return Valid();
}

namespace {

ValidationResult validate_move_source(const MoveSource &source) {
  switch (source.kind) {
    case MoveSource::Kind::kExplicit:
      if (!is_explicit_address(source.from)) {
        return Invalid("MOVE_RECORD explicit source is not a concrete, "
                        "valid codec address");
      }
      return Valid();
    case MoveSource::Kind::kRange: {
      if (!source.to.has_value()) {
        return Invalid("MOVE_RECORD range source has no TO bound");
      }
      if (!is_explicit_address(source.from) || !is_explicit_address(*source.to)) {
        return Invalid("MOVE_RECORD range source bounds must be concrete, "
                        "valid codec addresses");
      }
      if (!span_length(source.from, *source.to).has_value()) {
        return Invalid("MOVE_RECORD range source is not a forward, "
                        "same-depth run");
      }
      return Valid();
    }
    case MoveSource::Kind::kPrefix:
      if (!codec::is_valid_address(source.from) || !is_wildcard(source.from)) {
        return Invalid("MOVE_RECORD prefix source is not a valid partial "
                        "(wildcard-tail) codec address");
      }
      return Valid();
  }
  return Invalid("MOVE_RECORD source has an unrecognized kind");
}

}  // namespace

ValidationResult validate_move_record(const MoveRecord &op) {
  if (op.sources.empty()) {
    return Invalid("MOVE_RECORD carries no source selectors");
  }
  for (std::size_t i = 0; i < op.sources.size(); ++i) {
    auto result = validate_move_source(op.sources[i]);
    if (!result.ok()) {
      return Invalid("MOVE_RECORD source " + std::to_string(i) + ": " + result.reason);
    }
  }

  const bool all_explicit =
      std::all_of(op.sources.begin(), op.sources.end(), [](const MoveSource &s) {
        return s.kind == MoveSource::Kind::kExplicit;
      });

  if (all_explicit) {
    // The degenerate, known-N case: every source is one concrete
    // address, so N = sources.size() is known statically -- enforce
    // cover-N exactly as DECLARE's ADDRESS does (same nested-declare
    // recursion, then plan()).
    auto nested_result = validate_address_span_nested(op.destination);
    if (!nested_result.ok()) {
      return nested_result;
    }
    PlanResult plan_result = plan(op.destination, op.sources.size());
    if (plan_result.status == PlanStatus::kInvalid) {
      return Invalid("MOVE_RECORD destination span does not cover N=" +
                      std::to_string(op.sources.size()) + ": " + plan_result.reason);
    }
    return Valid();
  }

  // At least one source is a range/prefix wildcard (MOVE's bulk-
  // correction mechanism -- relocating a whole misclassified branch/
  // region at once): the resolved unit count depends on which addresses
  // in that region are actually populated in the live store, which this
  // module cannot see. Only the destination's own shape is validated
  // here (same DECLARE span grammar, open/elastic allowed, no MOVE-
  // specific restriction); "does the destination cover the resolved
  // source-N" is an EXECUTION-time check belonging to the UPDATE core
  // (PLAN.md II.5), not this one.
  auto shape_result = validate_span_shape(op.destination);
  if (!shape_result.ok) {
    return Invalid("MOVE_RECORD destination span is malformed: " + shape_result.reason);
  }
  return Valid();
}

ValidationResult validate_add_connection(const AddConnection &op) {
  // No stated no-wildcard rule for ADD_CONNECTION either (unlike
  // DELETE) -- don't invent one; a partial address is just an ordinary
  // valid codec address here.
  if (!codec::is_valid_address(op.group)) {
    return Invalid("ADD_CONNECTION group is not a valid codec address");
  }
  if (op.elements.empty()) {
    return Invalid("ADD_CONNECTION requires >= 1 element");
  }
  for (std::size_t i = 0; i < op.elements.size(); ++i) {
    if (!codec::is_valid_address(op.elements[i])) {
      return Invalid("ADD_CONNECTION element " + std::to_string(i) +
                      " is not a valid codec address");
    }
  }
  return Valid();
}

ValidationResult validate_delete_record(const DeleteRecord &op) {
  if (!is_explicit_address(op.token)) {
    return Invalid("DELETE_RECORD token must be a specific, explicit codec "
                    "address -- no wildcards");
  }
  return Valid();
}

ValidationResult validate_delete_connection(const DeleteConnection &op) {
  if (!is_explicit_address(op.a) || !is_explicit_address(op.b)) {
    return Invalid("DELETE_CONNECTION pair must be specific, explicit codec "
                    "addresses -- no wildcards");
  }
  return Valid();
}

}  // namespace command
