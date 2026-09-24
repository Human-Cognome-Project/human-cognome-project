// Unit tests for recognition (wal_recognize.h/.cpp): a pure decoded-report
// -> owed-obligations function. Standalone: links only codec.cpp and
// wal_recognize.cpp, no DB, own tiny check harness (same convention as
// codec/codec_test.cpp and wal/wal_report_test.cpp).
#include <algorithm>
#include <cstdio>
#include <string>

#include "codec.h"
#include "wal_recognize.h"
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

// One couplet per element, same convention as wal_report_test.cpp.
codec::Address addr(uint16_t couplet_code) {
  return codec::Address{codec::make_full_element(couplet_code)};
}

bool contains(const std::vector<wal::Obligation> &obligations,
              const wal::Obligation &target) {
  return std::find(obligations.begin(), obligations.end(), target) !=
         obligations.end();
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
// kDeclare -- structure per parent, mass only when NULL.
// ---------------------------------------------------------------------

void test_declare_structure_per_parent() {
  const wal::Report r = wal::fixture::declare(
      "core", 100, wal::Scope::kGlobal, kChild, {kParent1, kParent2});
  const std::vector<wal::Obligation> obligations = wal::owed(r);

  check(contains(obligations, wal::Obligation{wal::ObligationKind::kStructure,
                                               kParent1, kChild}),
        "declare: structure obligation (parent1, child)");
  check(contains(obligations, wal::Obligation{wal::ObligationKind::kStructure,
                                               kParent2, kChild}),
        "declare: structure obligation (parent2, child)");

  int structure_count = 0;
  for (const auto &o : obligations) {
    if (o.kind == wal::ObligationKind::kStructure) {
      ++structure_count;
    }
  }
  check(structure_count == 2, "declare: exactly one structure obligation per parent");
  // Total size, not just presence: 2 structure + 1 mass, nothing extra.
  check(obligations.size() == 3,
        "declare (2 parents, NULL mass): exactly 3 obligations total");
}

void test_declare_opens_mass_when_null() {
  const wal::Report r = wal::fixture::declare("core", 101, wal::Scope::kGlobal,
                                               kChild, {kParent1});
  const std::vector<wal::Obligation> obligations = wal::owed(r);

  check(contains(obligations, wal::Obligation{wal::ObligationKind::kMass,
                                               kChild, codec::Address{}}),
        "declare: mass obligation (token_id, <empty>) opens when token.mass is NULL");
  // Total size: 1 structure + 1 mass, nothing extra.
  check(obligations.size() == 2,
        "declare (1 parent, NULL mass): exactly 2 obligations total");
}

void test_declare_with_mass_opens_no_mass_obligation() {
  // The seed-floor exception: token.mass is given, not NULL.
  const wal::Report r = wal::fixture::declare_with_mass(
      "core", 102, wal::Scope::kGlobal, kChild, {kParent1}, 0);
  const std::vector<wal::Obligation> obligations = wal::owed(r);

  bool has_mass = false;
  for (const auto &o : obligations) {
    if (o.kind == wal::ObligationKind::kMass) {
      has_mass = true;
    }
  }
  check(!has_mass,
        "declare_with_mass (seed floor): opens no mass obligation");
  check(contains(obligations, wal::Obligation{wal::ObligationKind::kStructure,
                                               kParent1, kChild}),
        "declare_with_mass: still opens its structure obligation");
}

// ---------------------------------------------------------------------
// kAddConnection -- structure return owed, mass never opened.
// ---------------------------------------------------------------------

void test_add_connection_structure_no_mass() {
  const wal::Report r = wal::fixture::add_connection(
      "core", 103, wal::Scope::kGlobal, kExistingToken, kNewParent);
  const std::vector<wal::Obligation> obligations = wal::owed(r);

  check(obligations.size() == 1,
        "add_connection: exactly one obligation (its structure return)");
  check(contains(obligations, wal::Obligation{wal::ObligationKind::kStructure,
                                               kNewParent, kExistingToken}),
        "add_connection: structure obligation (new_parent, existing_token)");

  bool has_mass = false;
  for (const auto &o : obligations) {
    if (o.kind == wal::ObligationKind::kMass) {
      has_mass = true;
    }
  }
  check(!has_mass, "add_connection: never opens a mass obligation");
}

// ---------------------------------------------------------------------
// kMembershipWrite -- membership per group.
// ---------------------------------------------------------------------

void test_membership_write_per_group() {
  const wal::Report r = wal::fixture::membership_write(
      "lang-1", 5, wal::Scope::kLocal, kMember, {kGroup1, kGroup2});
  const std::vector<wal::Obligation> obligations = wal::owed(r);

  check(contains(obligations, wal::Obligation{wal::ObligationKind::kMembership,
                                               kMember, kGroup1}),
        "membership_write: membership obligation (member, group1)");
  check(contains(obligations, wal::Obligation{wal::ObligationKind::kMembership,
                                               kMember, kGroup2}),
        "membership_write: membership obligation (member, group2)");
  check(obligations.size() == 2,
        "membership_write: exactly one membership obligation per group");
}

// ---------------------------------------------------------------------
// kDelete -- no obligation at all, for either delete shape.
// ---------------------------------------------------------------------

void test_delete_token_yields_empty() {
  const wal::Report r =
      wal::fixture::delete_token("core", 7, wal::Scope::kGlobal, kRemoved);
  check(wal::owed(r).empty(), "delete_token: empty obligation set");
}

void test_delete_pair_yields_empty() {
  const wal::Report r = wal::fixture::delete_pair(
      "core", 8, wal::Scope::kGlobal, kPairMember, kPairGroup);
  check(wal::owed(r).empty(), "delete_pair: empty obligation set");
}

// ---------------------------------------------------------------------
// Settling reports (return-write / mass-fill) owe nothing -- they SETTLE
// (W-5), they don't open.
// ---------------------------------------------------------------------

void test_return_token_child_yields_empty() {
  const wal::Report r = wal::fixture::return_token_child(
      "core", 200, wal::Scope::kGlobal, kReturnParent, kReturnChild);
  check(wal::owed(r).empty(), "return_token_child: empty obligation set");
}

void test_return_member_of_yields_empty() {
  const wal::Report r = wal::fixture::return_member_of(
      "core", 201, wal::Scope::kGlobal, kReturnMember, kReturnGroup);
  check(wal::owed(r).empty(), "return_member_of: empty obligation set");
}

void test_mass_fill_yields_empty() {
  const wal::Report r = wal::fixture::mass_fill(
      "core", 300, wal::Scope::kGlobal, kMassToken, 42);
  check(wal::owed(r).empty(), "mass_fill: empty obligation set");
}

}  // namespace

int main() {
  test_declare_structure_per_parent();
  test_declare_opens_mass_when_null();
  test_declare_with_mass_opens_no_mass_obligation();
  test_add_connection_structure_no_mass();
  test_membership_write_per_group();
  test_delete_token_yields_empty();
  test_delete_pair_yields_empty();
  test_return_token_child_yields_empty();
  test_return_member_of_yields_empty();
  test_mass_fill_yields_empty();

  if (g_failures == 0) {
    std::printf("PASS wal_recognize_test\n");
    return 0;
  }
  std::printf("FAIL wal_recognize_test (%d failing check%s)\n", g_failures,
              g_failures == 1 ? "" : "s");
  return 1;
}
