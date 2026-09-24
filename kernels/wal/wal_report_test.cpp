// Unit tests for the decoded WAL report seam (wal_report.h). Standalone:
// links only codec.cpp, no DB, no other wal/ part, own tiny check harness
// (same convention as codec/codec_test.cpp).
#include <cstdio>
#include <string>

#include "codec.h"
#include "wal_report.h"

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

// One couplet per element, built the same way codec_test.cpp does, so
// each fixture address is trivially distinct and easy to read back.
codec::Address addr(uint16_t couplet_code) {
  return codec::Address{codec::make_full_element(couplet_code)};
}

// Round-trips one address through codec's string form and confirms it
// comes back unchanged -- the "footprint round-trips through codec"
// acceptance check, applied per address in a report's footprint.
bool round_trips(const codec::Address &a) {
  const std::optional<std::string> token_id = codec::encode_token_id(a);
  if (!token_id.has_value()) {
    return false;
  }
  const std::optional<codec::Address> back = codec::decode_token_id(*token_id);
  return back.has_value() && *back == a;
}

void check_footprint_round_trips(const wal::Report &r, const std::string &label) {
  check(round_trips(r.primary), label + ": primary round-trips through codec");
  for (std::size_t i = 0; i < r.counterparts.size(); ++i) {
    check(round_trips(r.counterparts[i]),
          label + ": counterpart round-trips through codec");
  }
}

// ---------------------------------------------------------------------
// Fixture addresses -- one couplet each, all distinct.
// ---------------------------------------------------------------------

const codec::Address kChild = addr(1);
const codec::Address kParent1 = addr(2);
const codec::Address kParent2 = addr(3);
const codec::Address kExistingToken = addr(4);
const codec::Address kNewParent = addr(5);
const codec::Address kMember = addr(6);
const codec::Address kGroup1 = addr(7);
const codec::Address kGroup2 = addr(8);
const codec::Address kRemoved = addr(9);
const codec::Address kPairMember = addr(10);
const codec::Address kPairGroup = addr(11);
const codec::Address kReturnParent = addr(12);
const codec::Address kReturnChild = addr(13);
const codec::Address kReturnMember = addr(14);
const codec::Address kReturnGroup = addr(15);
const codec::Address kMassToken = addr(16);

// ---------------------------------------------------------------------
// Per-op fixture checks.
// ---------------------------------------------------------------------

void test_declare() {
  const wal::Report r = wal::fixture::declare(
      "core", 100, wal::Scope::kGlobal, kChild, {kParent1, kParent2});

  check(r.source == "core", "declare: source carried");
  check(r.lsn == 100, "declare: lsn carried");
  check(r.scope == wal::Scope::kGlobal, "declare: scope carried");
  check(r.op == wal::OpKind::kDeclare, "declare: op is kDeclare");
  check(r.primary == kChild, "declare: primary is the new token");
  check(r.counterparts.size() == 2 && r.counterparts[0] == kParent1 &&
            r.counterparts[1] == kParent2,
        "declare: counterparts are the parent_id(s), in order");
  check(!r.mass.has_value(), "declare: token.mass is NULL (not yet computed)");
  check_footprint_round_trips(r, "declare");
}

void test_declare_with_mass() {
  const wal::Report r = wal::fixture::declare_with_mass(
      "core", 101, wal::Scope::kGlobal, kChild, {kParent1}, 0);

  check(r.op == wal::OpKind::kDeclare, "declare_with_mass: op is kDeclare");
  check(r.mass.has_value() && *r.mass == 0,
        "declare_with_mass: token.mass is the given seed-floor value, not NULL");
  check_footprint_round_trips(r, "declare_with_mass");
}

void test_add_connection() {
  const wal::Report r = wal::fixture::add_connection(
      "core", 102, wal::Scope::kGlobal, kExistingToken, kNewParent);

  check(r.op == wal::OpKind::kAddConnection, "add_connection: op is kAddConnection");
  check(r.primary == kExistingToken, "add_connection: primary is the existing token");
  check(r.counterparts.size() == 1 && r.counterparts[0] == kNewParent,
        "add_connection: counterpart is the new parent");
  check(!r.mass.has_value(), "add_connection: emits no mass obligation");
  check_footprint_round_trips(r, "add_connection");
}

void test_membership_write() {
  const wal::Report r = wal::fixture::membership_write(
      "lang-1", 5, wal::Scope::kLocal, kMember, {kGroup1, kGroup2});

  check(r.op == wal::OpKind::kMembershipWrite,
        "membership_write: op is kMembershipWrite");
  check(r.scope == wal::Scope::kLocal, "membership_write: scope carried");
  check(r.primary == kMember, "membership_write: primary is the member");
  check(r.counterparts.size() == 2 && r.counterparts[0] == kGroup1 &&
            r.counterparts[1] == kGroup2,
        "membership_write: counterparts are the group(s) joined");
  check(!r.mass.has_value(), "membership_write: never touches token.mass");
  check_footprint_round_trips(r, "membership_write");
}

void test_delete_token() {
  const wal::Report r =
      wal::fixture::delete_token("core", 7, wal::Scope::kGlobal, kRemoved);

  check(r.op == wal::OpKind::kDelete, "delete_token: op is kDelete");
  check(r.primary == kRemoved, "delete_token: primary is the removed token");
  check(r.counterparts.empty(), "delete_token: no counterpart");
  check_footprint_round_trips(r, "delete_token");
}

void test_delete_pair() {
  const wal::Report r = wal::fixture::delete_pair(
      "core", 8, wal::Scope::kGlobal, kPairMember, kPairGroup);

  check(r.op == wal::OpKind::kDelete, "delete_pair: op is kDelete");
  check(r.primary == kPairMember, "delete_pair: primary is the member side");
  check(r.counterparts.size() == 1 && r.counterparts[0] == kPairGroup,
        "delete_pair: counterpart is the removed group");
  check_footprint_round_trips(r, "delete_pair");
}

void test_return_token_child() {
  const wal::Report r = wal::fixture::return_token_child(
      "core", 200, wal::Scope::kGlobal, kReturnParent, kReturnChild);

  check(r.op == wal::OpKind::kReturnTokenChild,
        "return_token_child: op is kReturnTokenChild");
  check(r.primary == kReturnChild, "return_token_child: primary is the child");
  check(r.counterparts.size() == 1 && r.counterparts[0] == kReturnParent,
        "return_token_child: counterpart is the parent -- settles (parent, child)");
  check(!r.mass.has_value(), "return_token_child: never touches token.mass");
  check_footprint_round_trips(r, "return_token_child");
}

void test_return_member_of() {
  const wal::Report r = wal::fixture::return_member_of(
      "core", 201, wal::Scope::kGlobal, kReturnMember, kReturnGroup);

  check(r.op == wal::OpKind::kReturnMemberOf,
        "return_member_of: op is kReturnMemberOf");
  check(r.primary == kReturnMember, "return_member_of: primary is the member");
  check(r.counterparts.size() == 1 && r.counterparts[0] == kReturnGroup,
        "return_member_of: counterpart is the group -- settles (member, group)");
  check_footprint_round_trips(r, "return_member_of");
}

void test_mass_fill() {
  const wal::Report r = wal::fixture::mass_fill(
      "core", 300, wal::Scope::kGlobal, kMassToken, 42);

  check(r.op == wal::OpKind::kMassFill, "mass_fill: op is kMassFill");
  check(r.primary == kMassToken, "mass_fill: primary is the existing token_id");
  check(r.counterparts.empty(), "mass_fill: no counterpart");
  check(r.mass.has_value() && *r.mass == 42,
        "mass_fill: token.mass is the non-NULL fill, for an existing token_id");
  check_footprint_round_trips(r, "mass_fill");
}

// The mass rule fires only for a new-token DECLARE (never a connection):
// a direct check that add_connection and mass_fill are the only two ops
// that can carry a mass, and that they mean different things (NULL vs.
// the fill), matching WAL-IMPL-PLAN.md W-2/W-3.
void test_mass_is_declare_or_fill_only() {
  const wal::Report declared =
      wal::fixture::declare("core", 1, wal::Scope::kGlobal, kChild, {kParent1});
  const wal::Report connected = wal::fixture::add_connection(
      "core", 2, wal::Scope::kGlobal, kExistingToken, kNewParent);
  const wal::Report filled =
      wal::fixture::mass_fill("core", 3, wal::Scope::kGlobal, kMassToken, 7);

  check(!declared.mass.has_value(),
        "mass rule: an ordinary DECLARE opens the mass obligation (NULL)");
  check(!connected.mass.has_value(),
        "mass rule: an ADD_CONNECTION carries no mass at all");
  check(filled.mass.has_value(),
        "mass rule: only a mass-fill carries the non-NULL close");
}

}  // namespace

int main() {
  test_declare();
  test_declare_with_mass();
  test_add_connection();
  test_membership_write();
  test_delete_token();
  test_delete_pair();
  test_return_token_child();
  test_return_member_of();
  test_mass_fill();
  test_mass_is_declare_or_fill_only();

  if (g_failures == 0) {
    std::printf("PASS wal_report_test\n");
    return 0;
  }
  std::printf("FAIL wal_report_test (%d failing check%s)\n", g_failures,
              g_failures == 1 ? "" : "s");
  return 1;
}
