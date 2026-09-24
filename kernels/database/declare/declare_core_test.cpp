// Standalone check harness for the DECLARE core (declare/declare_core.h),
// same tiny check-macro style as controller/controller_test.cpp.
//
// Runs directly against the real, disposable hcp3_core database: resets it
// (DROP SCHEMA public CASCADE; CREATE SCHEMA public;) and reapplies
// ../schema/schema.sql, exactly as controller_test.cpp does, then exercises
// declare::execute() over a live dbk::Controller.
//
// If no local Postgres is reachable it prints a clear message and exits
// non-zero rather than faking anything.
#include <libpq-fe.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "codec.h"
#include "command_ir.h"
#include "controller.h"
#include "declare_core.h"

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

using codec::Address;
using command::AddressSegment;
using command::AddressSpan;
using command::ConstituentList;
using command::DeclareRecord;
using command::Reference;

// An address from its dot-joined token_id string (via the codec), same
// helper controller_test.cpp uses.
Address A(const std::string &token_id) {
  const auto a = codec::decode_token_id(token_id);
  if (!a.has_value()) {
    std::fprintf(stderr, "test bug: undecodable token_id '%s'\n", token_id.c_str());
    std::exit(2);
  }
  return *a;
}

bool contains(const std::vector<Address> &v, const Address &a) {
  return std::find(v.begin(), v.end(), a) != v.end();
}

bool contains_reason(const std::string &reason, const std::string &needle) {
  return reason.find(needle) != std::string::npos;
}

bool exec_bare(PGconn *conn, const std::string &sql) {
  PGresult *res = PQexec(conn, sql.c_str());
  const ExecStatusType st = PQresultStatus(res);
  const bool ok = (st == PGRES_COMMAND_OK || st == PGRES_TUPLES_OK);
  if (!ok) {
    std::fprintf(stderr, "exec failed: %s\n  sql: %s\n", PQresultErrorMessage(res),
                 sql.c_str());
  }
  PQclear(res);
  return ok;
}

std::string read_file(const std::string &path) {
  std::ifstream in(path);
  if (!in) {
    std::fprintf(stderr, "cannot open schema file: %s\n", path.c_str());
    std::exit(2);
  }
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

ConstituentList refs(std::vector<Reference> v) { return v; }

Reference RefTo(const Address &a) { return Reference::ToAddress(a); }

// --------------------------------------------------------------------
// The actual declare::execute() exercise. Records via check().
// --------------------------------------------------------------------
void run_declare_checks(const std::string &conninfo) {
  dbk::Controller ctl(conninfo);

  // --- Seed fixtures: mass-1 atoms, no constituents, inserted directly via
  //     the controller (the seed floor is a bootstrap channel, NOT the
  //     DECLARE verb -- PLAN.md II.3 "Mass"). ---
  const Address p1 = A("AA");
  const Address p2 = A("AB");
  const Address p3 = A("AC");
  const Address p4 = A("AD");
  const Address p5 = A("AE");  // dedicated pure-grouping naming-literal atom.
  const Address grp_fixture = A("AF");  // a pre-existing group handle for MEMBER_OF.
  for (const auto &addr : {p1, p2, p3, p4, p5, grp_fixture}) {
    ctl.mint(addr, "seed-atom", {}, 1);
  }
  check(ctl.token_exists(p1) && ctl.token_exists(p5) && ctl.token_exists(grp_fixture),
        "seed fixtures: atoms minted directly via the controller");

  // --- Single literal declare: PARENTS sets N=1, ADDRESS a direct value,
  //     MEMBER_OF authors the up-membership reciprocal. ---
  {
    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{refs({RefTo(p1), RefTo(p2)})};
    node.address = AddressSpan{AddressSegment::Direct(A("BA"))};
    node.notation = {std::string("composite-1")};
    DeclareRecord::MemberOf mo;
    mo.shared = {RefTo(grp_fixture)};
    node.member_of = mo;

    declare::Result result = declare::execute(ctl, node);
    check(result.outcomes.size() == 1, "single declare: returns one outcome");
    check(result.outcomes[0].ingested && result.outcomes[0].address == A("BA"),
          "single declare: returns the actual assigned address");

    const auto parents = ctl.parents_of(A("BA"));
    check(parents.size() == 2 && parents[0].parent == p1 && parents[1].parent == p2,
          "single declare: constituents linked in order, references only");
    check(!parents[0].mass.has_value() && !parents[1].mass.has_value(),
          "single declare: no per-constituent mass (LINK writes SQL NULL)");

    const auto attrs = ctl.attributes_of(A("BA"));
    check(attrs.has_value() && !attrs->mass.has_value(),
          "single declare: blank mass stored as SQL NULL");
    check(attrs.has_value() && attrs->notation.has_value() &&
              *attrs->notation == "composite-1",
          "single declare: NOTATION stored");

    check(contains(ctl.member_of(A("BA")), grp_fixture),
          "single declare: MEMBER_OF authors the up-membership edge");
    check(contains(ctl.members_of(grp_fixture), A("BA")),
          "single declare: MEMBER_OF's members reciprocal is written too");
  }

  // --- Multi-member set: PARENTS sets N=2 via one statement, ADDRESS an
  //     open (elastic) FROM origin filling both slots sequentially,
  //     NOTATION co-indexed over the two members. ---
  {
    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{refs({RefTo(p1), RefTo(p3)}),
                                                 refs({RefTo(p1), RefTo(p4)})};
    node.address = AddressSpan{AddressSegment::From(A("CA"))};
    node.notation = {std::string("multi-0"), std::string("multi-1")};

    declare::Result result = declare::execute(ctl, node);
    check(result.outcomes.size() == 2, "multi-member set: N outcomes for N=2 PARENTS");
    check(result.outcomes[0].ingested && result.outcomes[0].address == A("CA"),
          "multi-member set: member 0 placed at the FROM origin");
    check(result.outcomes[1].ingested && result.outcomes[1].address == A("CB"),
          "multi-member set: member 1 placed at the sequential successor");

    const auto attrs0 = ctl.attributes_of(A("CA"));
    const auto attrs1 = ctl.attributes_of(A("CB"));
    check(attrs0.has_value() && attrs0->notation == "multi-0" && attrs1.has_value() &&
              attrs1->notation == "multi-1",
          "multi-member set: NOTATION co-indexed to the right member");
    check(ctl.parents_of(A("CA")).size() == 2 && ctl.parents_of(A("CB")).size() == 2,
          "multi-member set: each member's own constituents linked");
  }

  // --- Grouping (mixed node): PARENTS present (N=1) AND MEMBERS present --
  //     the placed structure address is the naming literal; MEMBERS is
  //     realized as membership edges, both directions. ---
  {
    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{refs({RefTo(p1), RefTo(p2)})};
    node.address = AddressSpan{AddressSegment::Direct(A("DA"))};
    node.notation = {std::string("group-1")};
    node.members = std::vector<Reference>{RefTo(p3), RefTo(p4)};

    declare::Result result = declare::execute(ctl, node);
    check(result.outcomes.size() == 1 && result.outcomes[0].ingested &&
              result.outcomes[0].address == A("DA"),
          "grouping (mixed node): the structural placement is returned");

    const auto members = ctl.members_of(A("DA"));
    check(members.size() == 2 && contains(members, p3) && contains(members, p4),
          "grouping (mixed node): members written (downward)");
    check(contains(ctl.member_of(p3), A("DA")) && contains(ctl.member_of(p4), A("DA")),
          "grouping (mixed node): member_of reciprocal written (upward), both members");
  }

  // --- Nested declare: a PARENTS constituent is a full nested DECLARE,
  //     building a connected sub-structure in one call. The nested mint
  //     collapses to its address, which is then linked as the outer
  //     member's constituent. ---
  {
    auto sub = std::make_shared<DeclareRecord>();
    sub->parents = std::vector<ConstituentList>{refs({RefTo(p1), RefTo(p3)})};
    sub->address = AddressSpan{AddressSegment::Direct(A("EB"))};
    sub->notation = {std::string("nested-sub")};

    DeclareRecord outer;
    outer.parents =
        std::vector<ConstituentList>{refs({Reference::ToNested(sub), RefTo(p2)})};
    outer.address = AddressSpan{AddressSegment::Direct(A("EA"))};
    outer.notation = {std::string("nested-outer")};

    declare::Result result = declare::execute(ctl, outer);
    check(result.outcomes.size() == 1 && result.outcomes[0].ingested &&
              result.outcomes[0].address == A("EA"),
          "nested declare: outer statement returns its own placed address");
    check(ctl.token_exists(A("EB")), "nested declare: the nested sub-structure was minted");
    const auto outer_parents = ctl.parents_of(A("EA"));
    check(outer_parents.size() == 2 && outer_parents[0].parent == A("EB") &&
              outer_parents[1].parent == p2,
          "nested declare: outer constituent list collapses onto the nested address");
    const auto sub_parents = ctl.parents_of(A("EB"));
    check(sub_parents.size() == 2 && sub_parents[0].parent == p1 &&
              sub_parents[1].parent == p3,
          "nested declare: the whole connected sub-structure was built in one call");
  }

  // --- SEE idempotency: re-declaring an existing statement is a
  //     no-op/link-only -- no duplicate token_parent rows, no duplicate-key
  //     exception on the already-linked MEMBER_OF edge. ---
  {
    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{refs({RefTo(p1), RefTo(p2)})};
    node.address = AddressSpan{AddressSegment::Direct(A("BA"))};
    node.notation = {std::string("composite-1-again")};
    DeclareRecord::MemberOf mo;
    mo.shared = {RefTo(grp_fixture)};  // already linked by the single-declare test above.
    node.member_of = mo;

    bool threw = false;
    declare::Result result;
    try {
      result = declare::execute(ctl, node);
    } catch (const std::exception &) {
      threw = true;
    }
    check(!threw, "SEE idempotency: re-declaring an existing token does not throw");
    check(result.outcomes.size() == 1 && result.outcomes[0].ingested &&
              result.outcomes[0].address == A("BA"),
          "SEE idempotency: re-declare still returns the actual (unchanged) address");
    check(ctl.parents_of(A("BA")).size() == 2,
          "SEE idempotency: re-declare leaves token_parent unchanged (no duplicate links)");
    check(ctl.member_of(A("BA")).size() == 1,
          "SEE idempotency: re-declare leaves the MEMBER_OF edge singular (no duplicate)");
  }

  // --- Blank NOTATION: an absent NOTATION vector stores blank (""), not a
  //     stand-in string. ---
  {
    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{refs({RefTo(p1), RefTo(p2)})};
    node.address = AddressSpan{AddressSegment::Direct(A("FA"))};
    // node.notation left empty -- no NOTATION field supplied at all.

    declare::Result result = declare::execute(ctl, node);
    check(result.outcomes.size() == 1 && result.outcomes[0].ingested,
          "blank NOTATION: declare still succeeds with NOTATION entirely absent");
    const auto attrs = ctl.attributes_of(A("FA"));
    check(attrs.has_value() && attrs->notation.has_value() && attrs->notation->empty(),
          "blank NOTATION: stored blank (empty string), not a placeholder");
    check(attrs.has_value() && !attrs->mass.has_value(),
          "blank NOTATION case also confirms mass stored as SQL NULL");
  }

  // --- Rejection: a missing constituent is reported un-ingested, not
  //     thrown, and nothing is minted. ---
  {
    const Address missing = A("ZZ");  // never minted.
    check(!ctl.token_exists(missing), "rejection setup: the missing constituent is absent");

    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{refs({RefTo(missing), RefTo(p2)})};
    node.address = AddressSpan{AddressSegment::Direct(A("GA"))};
    node.notation = {std::string("should-not-exist")};

    bool threw = false;
    declare::Result result;
    try {
      result = declare::execute(ctl, node);
    } catch (const std::exception &) {
      threw = true;
    }
    check(!threw, "rejection: a missing constituent does not throw");
    check(result.outcomes.size() == 1 && !result.outcomes[0].ingested &&
              !result.outcomes[0].reason.empty(),
          "rejection: the point is reported un-ingested with a reason");
    check(!ctl.token_exists(A("GA")),
          "rejection: nothing was minted at the rejected point's address");
  }

  // --- Grouping, use-provided-ID form (MEMBERS present, PARENTS absent):
  //     ADDRESS references a pre-existing naming literal directly --
  //     NOTATION plays no role in identity at all. ---
  {
    DeclareRecord node;
    node.members = std::vector<Reference>{RefTo(p1), RefTo(p2)};
    node.address = AddressSpan{AddressSegment::Direct(p5)};
    // node.notation left entirely absent -- identity comes from ADDRESS.

    declare::Result result = declare::execute(ctl, node);
    check(result.outcomes.size() == 1 && result.outcomes[0].ingested &&
              result.outcomes[0].address == p5,
          "use-provided-ID: returns the referenced naming-literal token_id, "
          "not a freshly placed address");
    check(contains(ctl.members_of(p5), p1) && contains(ctl.members_of(p5), p2),
          "use-provided-ID: members written (downward)");
    check(contains(ctl.member_of(p1), p5) && contains(ctl.member_of(p2), p5),
          "use-provided-ID: member_of reciprocal written (upward)");
  }

  // --- Grouping, use-provided-ID form, unresolvable (ADDRESS names a
  //     token that was never minted): rejected, not thrown, not
  //     fabricated. ---
  {
    DeclareRecord node;
    node.members = std::vector<Reference>{RefTo(p1)};
    node.address = AddressSpan{AddressSegment::Direct(A("ZZ"))};  // never minted.

    bool threw = false;
    declare::Result result;
    try {
      result = declare::execute(ctl, node);
    } catch (const std::exception &) {
      threw = true;
    }
    check(!threw, "use-provided-ID: an absent referenced token does not throw");
    check(result.outcomes.size() == 1 && !result.outcomes[0].ingested &&
              contains_reason(result.outcomes[0].reason, "existing token"),
          "use-provided-ID: an absent referenced token is rejected, not "
          "fabricated");
  }

  // --- Grouping, use-provided-ID form, via an inline nested mint: the
  //     ADDRESS span's own segment is a nested DECLARE (legal here since
  //     ruling #3's rejection only applies when PARENTS is present on
  //     THIS node); the nested mint's resulting address is both minted
  //     and, trivially, already existing, so it becomes the naming
  //     literal. ---
  {
    auto naming_sub = std::make_shared<DeclareRecord>();
    naming_sub->parents = std::vector<ConstituentList>{refs({RefTo(p1), RefTo(p2)})};
    naming_sub->address = AddressSpan{AddressSegment::Direct(A("HB"))};

    DeclareRecord node;
    node.members = std::vector<Reference>{RefTo(p3)};
    node.address = AddressSpan{AddressSegment::Nested(naming_sub)};

    declare::Result result = declare::execute(ctl, node);
    check(result.outcomes.size() == 1 && result.outcomes[0].ingested &&
              result.outcomes[0].address == A("HB"),
          "use-provided-ID via inline nested mint: the naming literal is the "
          "nested statement's own minted address");
    check(contains(ctl.members_of(A("HB")), p3),
          "use-provided-ID via inline nested mint: MEMBERS written against it");
  }

  // --- Mint form, ADDRESS omitted entirely ("manager-placed"): rejected
  //     cleanly -- no next-slot/address-assignment mechanism exists yet
  //     (G4/G5); not guessed at. ---
  {
    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{refs({RefTo(p1), RefTo(p2)})};
    node.members = std::vector<Reference>{RefTo(p3)};
    // node.address left entirely absent -- valid per command::validate_declare
    // (the mint form may omit ADDRESS), but not executable here.

    bool threw = false;
    declare::Result result;
    try {
      result = declare::execute(ctl, node);
    } catch (const std::exception &) {
      threw = true;
    }
    check(!threw, "manager-placed (ADDRESS omitted): does not throw");
    check(result.outcomes.size() == 1 && !result.outcomes[0].ingested &&
              contains_reason(result.outcomes[0].reason, "manager-placed"),
          "manager-placed (ADDRESS omitted): rejected, not guessed at");
  }

  // --- @-pin: a requested address that this module has no override
  //     mechanism for -- the actual assigned address is exactly the
  //     pinned one. ---
  {
    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{refs({RefTo(p1), RefTo(p2)})};
    node.address = AddressSpan{AddressSegment::Pin(A("IA"))};

    declare::Result result = declare::execute(ctl, node);
    check(result.outcomes.size() == 1 && result.outcomes[0].ingested &&
              result.outcomes[0].address == A("IA"),
          "@-pin: the actual assigned address is the pinned address (no "
          "override mechanism exists yet)");
  }

  // --- Undeclared-hook: one slot of a multi-member set is left
  //     undeclared -- that point rejects un-ingested; its sibling in the
  //     same statement is unaffected. ---
  {
    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{refs({RefTo(p1), RefTo(p2)}),
                                                 refs({RefTo(p1), RefTo(p3)})};
    node.address =
        AddressSpan{AddressSegment::Direct(A("JA")), AddressSegment::Undeclared()};

    declare::Result result = declare::execute(ctl, node);
    check(result.outcomes.size() == 2, "undeclared hook: still N outcomes");
    check(result.outcomes[0].ingested && result.outcomes[0].address == A("JA"),
          "undeclared hook: the sibling with a concrete slot still ingests");
    check(!result.outcomes[1].ingested &&
              contains_reason(result.outcomes[1].reason, "undeclared hook"),
          "undeclared hook: the undeclared slot's point is rejected un-ingested");
  }

  // --- Pending seam: an open (elastic) AFTER origin cannot be resolved
  //     without the deferred trunk map (G5) -- rejected cleanly, not
  //     thrown, not guessed. ---
  {
    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{refs({RefTo(p1), RefTo(p2)})};
    node.address = AddressSpan{AddressSegment::After(A("KA"))};  // open, last segment.

    bool threw = false;
    declare::Result result;
    try {
      result = declare::execute(ctl, node);
    } catch (const std::exception &) {
      threw = true;
    }
    check(!threw, "pending seam: an unresolved AFTER origin does not throw");
    check(result.outcomes.size() == 1 && !result.outcomes[0].ingested &&
              contains_reason(result.outcomes[0].reason, "trunk map"),
          "pending seam: rejected pending the deferred trunk map (G5), not "
          "guessed at");
  }

  // --- Per-member MEMBER_OF: section 2's sparse per-member entry applies
  //     ONLY to its own index; the other member keeps only the section-1
  //     shared group. ---
  {
    const Address shared_group = A("LA");
    const Address sparse_group = A("LB");
    ctl.mint(shared_group, "", {}, 1);
    ctl.mint(sparse_group, "", {}, 1);

    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{refs({RefTo(p1), RefTo(p2)}),
                                                 refs({RefTo(p1), RefTo(p3)})};
    node.address = AddressSpan{AddressSegment::From(A("LC"))};
    DeclareRecord::MemberOf mo;
    mo.shared = {RefTo(shared_group)};
    mo.per_member[1] = {RefTo(sparse_group)};  // only member index 1.
    node.member_of = mo;

    declare::Result result = declare::execute(ctl, node);
    check(result.outcomes.size() == 2 && result.outcomes[0].ingested &&
              result.outcomes[1].ingested,
          "per-member MEMBER_OF: both members ingest");
    const Address member0 = *result.outcomes[0].address;
    const Address member1 = *result.outcomes[1].address;
    check(contains(ctl.member_of(member0), shared_group) &&
              !contains(ctl.member_of(member0), sparse_group),
          "per-member MEMBER_OF: member 0 gets only the shared group");
    check(contains(ctl.member_of(member1), shared_group) &&
              contains(ctl.member_of(member1), sparse_group),
          "per-member MEMBER_OF: member 1 gets the shared group AND its own "
          "sparse entry");
  }

  // --- A nested reference producing more than one outcome cannot
  //     collapse to a single reference slot -- rejected, not guessed
  //     (e.g. picking "the first" arbitrarily). ---
  {
    auto multi_member_sub = std::make_shared<DeclareRecord>();
    multi_member_sub->parents =
        std::vector<ConstituentList>{refs({RefTo(p1), RefTo(p2)}),
                                      refs({RefTo(p1), RefTo(p3)})};  // N=2.
    multi_member_sub->address = AddressSpan{AddressSegment::From(A("MA"))};

    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{
        refs({Reference::ToNested(multi_member_sub), RefTo(p2)})};
    node.address = AddressSpan{AddressSegment::Direct(A("MC"))};

    bool threw = false;
    declare::Result result;
    try {
      result = declare::execute(ctl, node);
    } catch (const std::exception &) {
      threw = true;
    }
    check(!threw, ">1-outcome nested reference: does not throw");
    check(result.outcomes.size() == 1 && !result.outcomes[0].ingested &&
              contains_reason(result.outcomes[0].reason, "exactly one"),
          ">1-outcome nested reference: rejected rather than picking one "
          "address arbitrarily");
    check(ctl.token_exists(A("MA")) && ctl.token_exists(A("MB")),
          "eager nested mint is NOT rolled back when the outer point later "
          "rejects (README.md 'Atomicity boundary') -- the sub-structure "
          "the nested reference already minted stays minted even though "
          "MC itself never ingests");
    check(!ctl.token_exists(A("MC")),
          "the outer point's own address was never minted, only rejected");
  }

  // --- Membership re-add is a door no-op (controller.h: add_membership is
  //     now SEE-idempotent, ON CONFLICT DO NOTHING both directions) -- no
  //     caller-side guard needed, confirmed directly against the door. ---
  {
    ctl.add_membership(p1, grp_fixture);  // already linked earlier in this run.
    bool threw = false;
    try {
      ctl.add_membership(p1, grp_fixture);  // re-add, direct door call.
    } catch (const std::exception &) {
      threw = true;
    }
    check(!threw, "membership re-add: the door itself no-ops, no exception");
    const auto groups = ctl.member_of(p1);
    check(std::count(groups.begin(), groups.end(), grp_fixture) == 1,
          "membership re-add: no duplicate edge recorded");
  }
}

}  // namespace

int main(int argc, char **argv) {
  const std::string schema_path = (argc > 1) ? argv[1] : "../schema/schema.sql";
  const std::string conninfo = "dbname=hcp3_core";

  {
    PGconn *maint = PQconnectdb("dbname=postgres");
    if (maint == nullptr || PQstatus(maint) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL declare_core_test: no local Postgres reachable: %s\n",
                   maint ? PQerrorMessage(maint) : "null connection");
      if (maint) PQfinish(maint);
      return 1;
    }
    PGresult *r = PQexec(maint, "SELECT 1 FROM pg_database WHERE datname = 'hcp3_core'");
    const bool exists = PQresultStatus(r) == PGRES_TUPLES_OK && PQntuples(r) > 0;
    PQclear(r);
    if (!exists && !exec_bare(maint, "CREATE DATABASE hcp3_core")) {
      std::fprintf(stderr, "FAIL declare_core_test: could not create hcp3_core\n");
      PQfinish(maint);
      return 1;
    }
    PQfinish(maint);
  }

  {
    PGconn *db = PQconnectdb(conninfo.c_str());
    if (db == nullptr || PQstatus(db) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL declare_core_test: cannot connect to hcp3_core: %s\n",
                   db ? PQerrorMessage(db) : "null connection");
      if (db) PQfinish(db);
      return 1;
    }
    const bool reset = exec_bare(db, "DROP SCHEMA public CASCADE; CREATE SCHEMA public;");
    const bool applied = reset && exec_bare(db, read_file(schema_path));
    PQfinish(db);
    if (!applied) {
      std::fprintf(stderr, "FAIL declare_core_test: schema reset/apply failed\n");
      return 1;
    }
  }

  try {
    run_declare_checks(conninfo);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "FAIL declare_core_test: unexpected exception: %s\n", e.what());
    ++g_failures;
  }

  if (g_failures == 0) {
    std::printf("PASS declare_core_test\n");
    return 0;
  }
  std::printf("FAIL declare_core_test (%d failed)\n", g_failures);
  return 1;
}
