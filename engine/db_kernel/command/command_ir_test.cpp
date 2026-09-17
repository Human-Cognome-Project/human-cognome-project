// Standalone check harness for the Command IR's structural validation.
// Same style as codec/codec_test.cpp: a tiny check macro, one line per
// check, PASS/FAIL summary, non-zero exit on any failure. Standard
// library + codec + command_ir + span_planner only.
#include <cstdio>
#include <memory>
#include <string>

#include "codec.h"
#include "command_ir.h"
#include "span_planner.h"

namespace {

int g_failures = 0;

void check(bool ok, const std::string &what) {
  if (ok) {
    std::printf("ok   %s\n", what.c_str());
  } else {
    std::printf("FAIL %s\n", what.c_str());
    ++g_failures;
  }
}

using command::AddConnection;
using command::AddressSegment;
using command::AddressSpan;
using command::DeclareRecord;
using command::DeleteConnection;
using command::DeleteRecord;
using command::MoveRecord;
using command::MoveSource;
using command::ReadRecord;
using command::Reference;
using command::ValidationStatus;

codec::Address addr(const std::string &token_id) {
  auto a = codec::decode_token_id(token_id);
  if (!a.has_value()) {
    std::fprintf(stderr, "test bug: %s is not a decodable token_id\n",
                 token_id.c_str());
    std::abort();
  }
  return *a;
}

AddressSpan direct_span(const std::vector<std::string> &token_ids) {
  AddressSpan span;
  for (const auto &t : token_ids) {
    span.push_back(AddressSegment::Direct(addr(t)));
  }
  return span;
}

// A minimal, otherwise-valid two-constituent structure declare (N=1):
// PARENTS [[AA, AB]], ADDRESS = one direct slot. Building block for the
// tests below, each of which perturbs exactly one thing.
DeclareRecord minimal_structure_declare() {
  DeclareRecord node;
  node.parents = std::vector<command::ConstituentList>{
      {Reference::ToAddress(addr("AA")), Reference::ToAddress(addr("AB"))}};
  node.address = direct_span({"BA"});
  return node;
}

// ---------------------------------------------------------------------
// DECLARE_RECORD structural validation
// ---------------------------------------------------------------------

void test_missing_required_downward_field() {
  DeclareRecord node;  // neither PARENTS nor MEMBERS.
  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kInvalid,
        "a node with neither PARENTS nor MEMBERS is rejected");
}

void test_structure_floor_below_two_rejected() {
  DeclareRecord node = minimal_structure_declare();
  node.parents = std::vector<command::ConstituentList>{{Reference::ToAddress(addr("AA"))}};
  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kInvalid,
        "a PARENTS member with fewer than 2 constituents is rejected "
        "(anti-alias floor)");
}

void test_structure_floor_of_two_accepted() {
  auto node = minimal_structure_declare();
  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kValid,
        "a minimal well-formed structure declare (PARENTS>=2, matching "
        "ADDRESS) is accepted");
}

void test_grouping_empty_rejected() {
  DeclareRecord node;
  node.members = std::vector<Reference>{};  // present but empty.
  node.notation = {std::string("a label")};
  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kInvalid,
        "an empty MEMBERS list is rejected (grouping floor >= 1)");
}

void test_grouping_single_member_accepted() {
  DeclareRecord node;
  node.members = std::vector<Reference>{Reference::ToAddress(addr("AA"))};
  node.notation = {std::string("single hex codes")};
  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kValid,
        "a single-member MEMBERS list is accepted (grouping floor is >= 1, "
        "not >= 2)");
}

void test_grouping_only_node_rejects_own_address() {
  DeclareRecord node;
  node.members = std::vector<Reference>{Reference::ToAddress(addr("AA"))};
  node.notation = {std::string("single hex codes")};
  node.address = direct_span({"BA"});  // a grouping-only node may not place itself.
  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kInvalid,
        "a grouping-only node (MEMBERS present, PARENTS absent) that also "
        "carries an own-address placement is rejected");
}

void test_grouping_only_node_requires_naming_prose() {
  DeclareRecord node;
  node.members = std::vector<Reference>{Reference::ToAddress(addr("AA"))};
  // NOTATION absent: no PARENTS to derive a surface from, and no naming
  // prose either -- the label would be unaddressable and unnamed.
  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kInvalid,
        "a grouping-only node with no NOTATION prose is rejected -- it has "
        "no PARENTS to derive a surface from and no naming literal");
}

void test_mixed_structure_and_grouping_node_accepted() {
  // A node may carry structure and/or grouping at once (PLAN.md I.B).
  auto node = minimal_structure_declare();
  node.members = std::vector<Reference>{Reference::ToAddress(addr("AC"))};
  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kValid,
        "a node carrying both PARENTS and MEMBERS validates both floors "
        "and is accepted");
}

void test_notation_blanks_accepted() {
  auto node = minimal_structure_declare();
  node.notation = {std::nullopt};  // one slot, legal blank ("successive commas").
  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kValid,
        "a blank NOTATION slot is legal and does not fail validation");
}

void test_notation_length_mismatch_rejected() {
  auto node = minimal_structure_declare();  // N=1
  node.notation = {std::string("a"), std::string("b")};  // length 2 != N=1.
  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kInvalid,
        "NOTATION length must co-index with N -- a mismatch is rejected "
        "as a co-index violation");
}

void test_member_of_section_two_sparse_accepted() {
  auto node = minimal_structure_declare();  // N=1, valid index is 0.
  command::DeclareRecord::MemberOf member_of;
  member_of.shared = {Reference::ToAddress(addr("CA"))};
  member_of.per_member[0] = {Reference::ToAddress(addr("CB"))};
  node.member_of = member_of;
  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kValid,
        "MEMBER_OF section 2 as a sparse (<=N) per-member map is accepted, "
        "not rejected as a co-index violation");
}

void test_member_of_section_two_out_of_range_rejected() {
  auto node = minimal_structure_declare();  // N=1: only index 0 is valid.
  command::DeclareRecord::MemberOf member_of;
  member_of.per_member[1] = {Reference::ToAddress(addr("CB"))};
  node.member_of = member_of;
  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kInvalid,
        "a MEMBER_OF section 2 index >= N is out of range and rejected");
}

void test_span_must_cover_n() {
  auto node = minimal_structure_declare();
  node.address = direct_span({"BA", "BB"});  // 2 slots, N=1.
  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kInvalid,
        "an ADDRESS span that does not cover N is rejected");
}

void test_nested_declare_recurses_and_is_validated() {
  auto bad_nested = std::make_shared<DeclareRecord>();  // neither field present.
  auto node = minimal_structure_declare();
  node.parents = std::vector<command::ConstituentList>{
      {Reference::ToAddress(addr("AA")), Reference::ToNested(bad_nested)}};
  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kInvalid,
        "a nested declare inside PARENTS recurses into full structural "
        "validation, and an invalid nested node fails the whole statement");

  auto good_nested = std::make_shared<DeclareRecord>(minimal_structure_declare());
  auto node2 = minimal_structure_declare();
  node2.parents = std::vector<command::ConstituentList>{
      {Reference::ToAddress(addr("AA")), Reference::ToNested(good_nested)}};
  auto result2 = command::validate_declare(node2);
  check(result2.status == ValidationStatus::kValid,
        "a valid nested declare inside PARENTS passes recursion");
}

void test_nested_declare_in_address_span_recurses() {
  auto bad_nested = std::make_shared<DeclareRecord>();  // invalid: no fields.
  auto node = minimal_structure_declare();
  node.address = AddressSpan{AddressSegment::Nested(bad_nested)};
  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kInvalid,
        "a nested declare occupying an ADDRESS slot is itself structurally "
        "validated");
}

void test_malformed_reference_rejected() {
  auto node = minimal_structure_declare();
  Reference empty_ref;  // neither address nor nested set.
  node.parents = std::vector<command::ConstituentList>{
      {Reference::ToAddress(addr("AA")), empty_ref}};
  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kInvalid,
        "a reference that is neither a plain address nor a nested declare "
        "is rejected");
}

void test_declare_with_open_after_tail_is_accepted_pending_seam() {
  // N=2: a direct slot plus an open (un-TO'd) AFTER tail as the last
  // segment. AFTER is structurally legal here (same elastic shape as an
  // open FROM); only its concrete origin is G5-pending -- that must NOT
  // fail DECLARE validation.
  DeclareRecord node;
  node.parents = std::vector<command::ConstituentList>{
      {Reference::ToAddress(addr("AA")), Reference::ToAddress(addr("AB"))},
      {Reference::ToAddress(addr("AC")), Reference::ToAddress(addr("AD"))}};
  node.address =
      AddressSpan{AddressSegment::Direct(addr("BA")), AddressSegment::After(addr("CZ"))};

  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kValid,
        "a DECLARE whose ADDRESS ends in an open AFTER tail validates -- "
        "AFTER is structurally legal, only its concrete origin is G5-pending");

  // Assert the actual pending-seam status and per-slot markers the design
  // promises, not just that validation didn't fail.
  auto plan_result = command::plan(*node.address, 2);
  check(plan_result.status == command::PlanStatus::kPendingSeam,
        "the underlying plan() call for that ADDRESS reports kPendingSeam");
  check(plan_result.slots.size() == 2, "both slots are present in the pending plan");
  check(plan_result.slots[0].address.has_value() && *plan_result.slots[0].address == addr("BA"),
        "the resolvable (direct) slot carries its concrete address");
  check(!plan_result.slots[1].address.has_value() && !plan_result.slots[1].is_undeclared_hook,
        "the AFTER slot's address is nullopt (G5-pending), not an undeclared hook");
}

void test_declare_with_closed_after_to_is_accepted_pending_seam() {
  // N=2: a direct slot plus a closed AFTER..TO run. Its slot count can't
  // be verified without the trunk map, so cover-N cannot be checked past
  // it -- that must be reported as pending, not as a rejection.
  DeclareRecord node;
  node.parents = std::vector<command::ConstituentList>{
      {Reference::ToAddress(addr("AA")), Reference::ToAddress(addr("AB"))},
      {Reference::ToAddress(addr("AC")), Reference::ToAddress(addr("AD"))}};
  node.address = AddressSpan{AddressSegment::Direct(addr("BA")),
                              AddressSegment::After(addr("CZ"), addr("DA"))};

  auto result = command::validate_declare(node);
  check(result.status == ValidationStatus::kValid,
        "a DECLARE whose ADDRESS contains a closed AFTER..TO run validates "
        "-- its slot count cannot be verified without the trunk map, so it "
        "is not rejected as a cover-N failure");

  auto plan_result = command::plan(*node.address, 2);
  check(plan_result.status == command::PlanStatus::kPendingSeam,
        "the underlying plan() call reports kPendingSeam as soon as it "
        "reaches the AFTER..TO segment");
  check(plan_result.slots.size() == 1 && plan_result.slots[0].address == addr("BA"),
        "only the prefix planned before the pending segment is returned");
  check(!plan_result.reason.empty(),
        "the pending-seam result carries a reason naming the G5 dependency");
}

// ---------------------------------------------------------------------
// The other record-tier verbs -- lighter coverage: presence/shape only,
// per PLAN.md I.D / I.C (no TYPE gating applies to them either).
// ---------------------------------------------------------------------

void test_read_record_validation() {
  ReadRecord ok;
  ok.anchor = addr("AA");
  check(command::validate_read(ok).status == ValidationStatus::kValid,
        "a READ_RECORD with a valid anchor is accepted");

  ReadRecord bad;
  bad.anchor = {codec::make_partial_element(0)};
  bad.anchor[0].first = 200;  // out of alphabet range -> invalid element.
  check(command::validate_read(bad).status == ValidationStatus::kInvalid,
        "a READ_RECORD with an invalid anchor address is rejected");
}

// F-1, superseded (Patrick's later ruling): MOVE's distinguishing feature
// is that its SOURCE may be a range or a prefix/context wildcard -- every
// other record-tier op stays explicit-ids-only. A wildcard/range
// source's resolved unit count depends on the live store, so this module
// does NOT compute or check "does the destination cover N" for MOVE at
// all; that cover-N check is deferred to the UPDATE core. Only per-field
// well-formedness is validated here.
// The known-N (all-explicit-sources) path: cover-N enforced exactly like
// DECLARE's ADDRESS.
void test_move_record_validation() {
  MoveRecord explicit_ok;
  explicit_ok.sources = {MoveSource::Explicit(addr("AA"))};
  explicit_ok.destination = direct_span({"BA"});
  check(command::validate_move_record(explicit_ok).status == ValidationStatus::kValid,
        "a MOVE_RECORD with a well-formed explicit source and a matching "
        "destination is accepted");

  MoveRecord empty;
  check(command::validate_move_record(empty).status == ValidationStatus::kInvalid,
        "a MOVE_RECORD with no source selectors is rejected");

  MoveRecord malformed_destination;
  malformed_destination.sources = {MoveSource::Explicit(addr("AA"))};
  malformed_destination.destination =
      AddressSpan{AddressSegment::From(addr("BA")), AddressSegment::Direct(addr("CA"))};
  check(command::validate_move_record(malformed_destination).status == ValidationStatus::kInvalid,
        "a MOVE_RECORD destination with an open FROM in a non-last position "
        "is rejected");

  // All sources explicit => N is statically known (= sources.size()) and
  // IS enforced -- this is the degenerate, known-N case Patrick asked to
  // keep as-is.
  MoveRecord under_covered;
  under_covered.sources = {MoveSource::Explicit(addr("AA")), MoveSource::Explicit(addr("AB"))};
  under_covered.destination = direct_span({"BA"});  // covers 1, needs 2.
  check(command::validate_move_record(under_covered).status == ValidationStatus::kInvalid,
        "with all-explicit sources, a destination that under-covers N is "
        "rejected -- static cover-N still applies when N is known");

  MoveRecord over_covered;
  over_covered.sources = {MoveSource::Explicit(addr("AA")), MoveSource::Explicit(addr("AB"))};
  over_covered.destination = direct_span({"BA", "BB", "BC"});  // covers 3, needs 2.
  check(command::validate_move_record(over_covered).status == ValidationStatus::kInvalid,
        "with all-explicit sources, a destination that over-covers N is "
        "also rejected");
}

// The unknown-N (any wildcard/range source present) path: MOVE's
// characteristic bulk-correction mechanism -- relocate a whole
// misclassified branch/region at once. Cover-N is deferred to the
// UPDATE core since the resolved unit count needs the live store.
void test_move_record_wildcard_source_forms() {
  MoveRecord range_move;
  range_move.sources = {MoveSource::Range(addr("AA"), addr("AD"))};
  range_move.destination = direct_span({"BA"});
  check(command::validate_move_record(range_move).status == ValidationStatus::kValid,
        "a well-formed FROM..TO range source is accepted for MOVE");

  MoveRecord backward_range;
  backward_range.sources = {MoveSource::Range(addr("AD"), addr("AA"))};
  backward_range.destination = direct_span({"BA"});
  check(command::validate_move_record(backward_range).status == ValidationStatus::kInvalid,
        "a range source that does not walk forward is rejected");

  // kPrefix: a partial (wildcard-tail) address selecting a whole block.
  MoveRecord prefix_move;
  codec::Address wildcard = {codec::make_partial_element(0)};  // "A*"
  prefix_move.sources = {MoveSource::Prefix(wildcard)};
  prefix_move.destination = direct_span({"BA"});
  check(command::validate_move_record(prefix_move).status == ValidationStatus::kValid,
        "a well-formed prefix/context wildcard source is accepted for MOVE");

  MoveRecord non_wildcard_prefix;
  non_wildcard_prefix.sources = {MoveSource::Prefix(addr("AA"))};  // concrete, not partial.
  non_wildcard_prefix.destination = direct_span({"BA"});
  check(command::validate_move_record(non_wildcard_prefix).status == ValidationStatus::kInvalid,
        "a kPrefix source that is not actually a wildcard (partial) "
        "address is rejected");

  // No cover-N check at all once a wildcard/range source is present: an
  // arbitrary-count destination is accepted regardless of the (unknown)
  // resolved source unit count.
  MoveRecord uncounted;
  uncounted.sources = {MoveSource::Prefix(wildcard)};
  uncounted.destination = direct_span({"BA", "BB", "BC"});
  check(command::validate_move_record(uncounted).status == ValidationStatus::kValid,
        "a wildcard source's destination is never checked against a "
        "static cover-N -- deferred to the UPDATE core against the live "
        "store");

  // Coexistence: an explicit source and a range source in the SAME MOVE
  // -- any non-explicit source in the list defers cover-N for the whole
  // statement (total N cannot be known without resolving the range).
  MoveRecord mixed;
  mixed.sources = {MoveSource::Explicit(addr("AA")), MoveSource::Range(addr("BA"), addr("BC"))};
  mixed.destination = direct_span({"CA"});  // covers 1 -- would fail a naive N=2 check.
  check(command::validate_move_record(mixed).status == ValidationStatus::kValid,
        "an explicit source coexisting with a range source in one MOVE "
        "defers cover-N entirely, rather than checking against the "
        "explicit sources' partial count");

  // No MOVE-specific grammar restriction on the destination: the full
  // DECLARE span alphabet (nested declare, undeclared hook, open tail)
  // is accepted there too.
  MoveRecord full_grammar_destination;
  full_grammar_destination.sources = {MoveSource::Prefix(wildcard)};
  full_grammar_destination.destination = AddressSpan{
      AddressSegment::Direct(addr("BA")),
      AddressSegment::Nested(std::make_shared<DeclareRecord>(minimal_structure_declare())),
      AddressSegment::Undeclared(), AddressSegment::From(addr("CA"))};
  check(command::validate_move_record(full_grammar_destination).status == ValidationStatus::kValid,
        "MOVE's destination accepts the full DECLARE span alphabet "
        "(nested declare, undeclared hook, open elastic tail) -- no "
        "grammar restriction");
}

// DELETE carries the one STATED no-wildcard rule (NOTES.md "Build-phase
// rulings -- firmed 2026-09-17": "specific ids/pairs only ... explicit
// only, no wildcards"). It is enforced; nothing else is rejected on
// wildcard grounds.
void test_delete_ops_reject_wildcards() {
  codec::Address wildcard = {codec::make_partial_element(0)};  // "A*"

  DeleteRecord delete_record_wildcard{wildcard};
  check(command::validate_delete_record(delete_record_wildcard).status == ValidationStatus::kInvalid,
        "DELETE_RECORD rejects a wildcard address -- specific IDs only, "
        "no wildcards");

  DeleteConnection delete_connection_wildcard{wildcard, addr("AA")};
  check(command::validate_delete_connection(delete_connection_wildcard).status ==
            ValidationStatus::kInvalid,
        "DELETE_CONNECTION rejects a wildcard address in either slot -- "
        "specific IDs only, no wildcards");
}

// NOTES.md "Build-phase rulings -- firmed 2026-09-17": READ's anchor and
// exclusions, and ADD_CONNECTION's group and elements, MAY be a terminal
// wildcard -- a nominal, tree-constrained region read/connect, distinct
// from a forbidden arbitrary property-predicate scan. (This firmed
// ruling permits what was previously just "not specially rejected" --
// same accept outcome, now a stated allowance rather than an absence of
// a rule.) The bidirectional wildcard EXPANSION for ADD_CONNECTION
// (resolving the wildcard to its address range and enumerating the
// pairs) is an execution-time concern for the UPDATE core -- this
// module only validates that the address itself is well-formed.
void test_read_and_add_connection_permit_terminal_wildcards() {
  codec::Address wildcard = {codec::make_partial_element(0)};  // "A*"

  ReadRecord read_anchor_wildcard;
  read_anchor_wildcard.anchor = wildcard;
  check(command::validate_read(read_anchor_wildcard).status == ValidationStatus::kValid,
        "READ_RECORD's anchor permits a terminal wildcard (nominal "
        "tree-constrained read)");

  ReadRecord read_exclusion_wildcard;
  read_exclusion_wildcard.anchor = addr("AA");
  read_exclusion_wildcard.exclusions = {wildcard};
  check(command::validate_read(read_exclusion_wildcard).status == ValidationStatus::kValid,
        "READ_RECORD's exclusions permit a terminal-wildcard tree region, "
        "not just a specific token_id");

  AddConnection add_group_wildcard;
  add_group_wildcard.group = wildcard;
  add_group_wildcard.elements = {addr("AA")};
  check(command::validate_add_connection(add_group_wildcard).status == ValidationStatus::kValid,
        "ADD_CONNECTION's group permits a terminal wildcard (\"add this "
        "member to all wildcard-selected groups\")");

  AddConnection add_element_wildcard;
  add_element_wildcard.group = addr("AA");
  add_element_wildcard.elements = {wildcard};
  check(command::validate_add_connection(add_element_wildcard).status == ValidationStatus::kValid,
        "ADD_CONNECTION's elements permit a terminal wildcard (\"add all "
        "wildcard-selected members to this group\")");
}

// NOTES.md "Build-phase rulings -- firmed 2026-09-17": "Wildcards are
// TERMINAL ONLY -- no inline wildcards." A wildcard may only be an
// address's LAST element; codec::is_valid_address already enforces this
// (is_valid_address rejects a partial element that isn't the last one),
// so every validator built on it inherits the rule for free. These tests
// pin that explicitly, per Patrick's request, rather than leaving it as
// an implicit codec-level fact.
void test_wildcards_are_terminal_only() {
  // A terminal wildcard may follow any number of full elements -- not
  // just be the sole element.
  codec::Address terminal_wildcard = {codec::make_full_element(0), codec::make_partial_element(1)};
  check(codec::is_valid_address(terminal_wildcard),
        "a multi-element address ending in a wildcard (e.g. \"AA.B*\") is "
        "itself a valid address (sanity check for the cases below)");

  // An INLINE (non-terminal) wildcard: partial, then a further element.
  codec::Address inline_wildcard = {codec::make_partial_element(0), codec::make_full_element(1)};
  check(!codec::is_valid_address(inline_wildcard),
        "an inline (non-terminal) wildcard is not even a valid codec "
        "address");

  ReadRecord read_inline;
  read_inline.anchor = inline_wildcard;
  check(command::validate_read(read_inline).status == ValidationStatus::kInvalid,
        "READ_RECORD's anchor rejects an inline wildcard even though "
        "terminal wildcards are permitted");

  ReadRecord read_inline_exclusion;
  read_inline_exclusion.anchor = addr("AA");
  read_inline_exclusion.exclusions = {inline_wildcard};
  check(command::validate_read(read_inline_exclusion).status == ValidationStatus::kInvalid,
        "READ_RECORD's exclusions reject an inline wildcard");

  AddConnection add_inline_group;
  add_inline_group.group = inline_wildcard;
  add_inline_group.elements = {addr("AA")};
  check(command::validate_add_connection(add_inline_group).status == ValidationStatus::kInvalid,
        "ADD_CONNECTION's group rejects an inline wildcard");

  AddConnection add_inline_element;
  add_inline_element.group = addr("AA");
  add_inline_element.elements = {inline_wildcard};
  check(command::validate_add_connection(add_inline_element).status == ValidationStatus::kInvalid,
        "ADD_CONNECTION's elements reject an inline wildcard");

  MoveRecord move_inline_prefix;
  move_inline_prefix.sources = {MoveSource::Prefix(inline_wildcard)};
  move_inline_prefix.destination = direct_span({"BA"});
  check(command::validate_move_record(move_inline_prefix).status == ValidationStatus::kInvalid,
        "MOVE_RECORD's kPrefix source rejects an inline wildcard -- "
        "terminal-only applies to MOVE's wildcard form too");

  MoveRecord move_terminal_prefix;
  move_terminal_prefix.sources = {MoveSource::Prefix(terminal_wildcard)};
  move_terminal_prefix.destination = direct_span({"BA"});
  check(command::validate_move_record(move_terminal_prefix).status == ValidationStatus::kValid,
        "MOVE_RECORD's kPrefix source accepts a multi-element terminal "
        "wildcard, not just a single-element one");
}

void test_add_connection_validation() {
  AddConnection ok;
  ok.group = addr("AA");
  ok.elements = {addr("AB")};
  check(command::validate_add_connection(ok).status == ValidationStatus::kValid,
        "an ADD_CONNECTION with >= 1 element is accepted");

  AddConnection empty;
  empty.group = addr("AA");
  check(command::validate_add_connection(empty).status == ValidationStatus::kInvalid,
        "an ADD_CONNECTION with zero elements is rejected");
}

void test_delete_ops_validation() {
  DeleteRecord ok_record{addr("AA")};
  check(command::validate_delete_record(ok_record).status == ValidationStatus::kValid,
        "a DELETE_RECORD naming a valid address is accepted");

  DeleteConnection ok_conn{addr("AA"), addr("AB")};
  check(command::validate_delete_connection(ok_conn).status == ValidationStatus::kValid,
        "a DELETE_CONNECTION naming a valid pair is accepted");
}

}  // namespace

int main() {
  test_missing_required_downward_field();
  test_structure_floor_below_two_rejected();
  test_structure_floor_of_two_accepted();
  test_grouping_empty_rejected();
  test_grouping_single_member_accepted();
  test_grouping_only_node_rejects_own_address();
  test_grouping_only_node_requires_naming_prose();
  test_mixed_structure_and_grouping_node_accepted();
  test_notation_blanks_accepted();
  test_notation_length_mismatch_rejected();
  test_member_of_section_two_sparse_accepted();
  test_member_of_section_two_out_of_range_rejected();
  test_span_must_cover_n();
  test_nested_declare_recurses_and_is_validated();
  test_nested_declare_in_address_span_recurses();
  test_malformed_reference_rejected();
  test_declare_with_open_after_tail_is_accepted_pending_seam();
  test_declare_with_closed_after_to_is_accepted_pending_seam();
  test_read_record_validation();
  test_move_record_validation();
  test_move_record_wildcard_source_forms();
  test_delete_ops_reject_wildcards();
  test_read_and_add_connection_permit_terminal_wildcards();
  test_wildcards_are_terminal_only();
  test_add_connection_validation();
  test_delete_ops_validation();

  if (g_failures == 0) {
    std::printf("PASS command_ir_test\n");
    return 0;
  }
  std::printf("FAIL command_ir_test (%d check(s) failed)\n", g_failures);
  return 1;
}
