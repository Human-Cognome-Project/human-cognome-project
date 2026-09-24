// Standalone check harness for the UPDATE core (update/update_core.h),
// same tiny check-macro style as controller/controller_test.cpp and
// declare/declare_core_test.cpp.
//
// Runs directly against the real, disposable hcp3_core database: resets it
// (DROP SCHEMA public CASCADE; CREATE SCHEMA public;) and reapplies
// ../schema/schema.sql, exactly as the other *_test.cpp harnesses do, then
// exercises update::move_record / update::add_connection /
// update::delete_record / update::delete_connection over a live
// dbk::Controller.
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
#include "update_core.h"

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
using command::AddConnection;
using command::AddressSegment;
using command::AddressSpan;
using command::ConstituentList;
using command::DeclareRecord;
using command::DeleteConnection;
using command::DeleteRecord;
using command::MoveRecord;
using command::MoveSource;
using command::Reference;

// An address from its dot-joined token_id string (via the codec), same
// helper every other *_test.cpp in this codebase uses.
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

std::size_t count_of(const std::vector<Address> &v, const Address &a) {
  return static_cast<std::size_t>(std::count(v.begin(), v.end(), a));
}

bool contains_reason(const std::string &reason, const std::string &needle) {
  return reason.find(needle) != std::string::npos;
}

Reference RefTo(const Address &a) { return Reference::ToAddress(a); }

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

// --------------------------------------------------------------------
// MOVE_RECORD
// --------------------------------------------------------------------

void check_move_record(dbk::Controller &ctl) {
  // --- Explicit MOVE: rekey + cascade, edges repointed, no orphans. ---
  {
    const Address p1 = A("AA");
    const Address p2 = A("AB");
    ctl.mint(p1, "p1", {});
    ctl.mint(p2, "p2", {});
    const Address c1 = A("BA");
    ctl.mint(c1, "c1", {dbk::Constituent{p1, std::nullopt}, dbk::Constituent{p2, std::nullopt}});
    const Address grp = A("CA");
    ctl.mint(grp, "grp", {});
    ctl.add_membership(p1, grp);

    MoveRecord op;
    op.sources = {MoveSource::Explicit(p1)};
    const Address dest = A("DA");
    op.destination = AddressSpan{AddressSegment::Direct(dest)};

    update::MoveResult result = update::move_record(ctl, op);
    check(result.outcomes.size() == 1, "explicit MOVE: one outcome");
    check(result.outcomes.size() == 1 && result.outcomes[0].moved,
          "explicit MOVE: reported moved");
    check(result.outcomes.size() == 1 && result.outcomes[0].from == p1 &&
              result.outcomes[0].to == dest,
          "explicit MOVE: outcome reports from/to");

    check(!ctl.token_exists(p1), "explicit MOVE: old address no longer exists");
    check(ctl.token_exists(dest), "explicit MOVE: new address exists");

    // Structure cascade: c1's constituent FK repointed.
    const auto c1_parents = ctl.parents_of(c1);
    check(c1_parents.size() == 2 && c1_parents[0].parent == dest,
          "explicit MOVE: structure cascade -- c1's parent[0] repointed to new address");
    check(contains(ctl.children_of(dest), c1),
          "explicit MOVE: structure cascade -- new address's children_of includes c1");
    check(!contains(ctl.children_of(p1), c1),
          "explicit MOVE: no orphan -- old address's children_of is gone (address doesn't exist)");

    // Membership cascade: both directions repointed.
    check(contains(ctl.member_of(dest), grp),
          "explicit MOVE: membership cascade -- new address's member_of includes grp");
    check(contains(ctl.members_of(grp), dest),
          "explicit MOVE: membership cascade -- grp's members_of includes new address");
  }

  // --- Wildcard/range MOVE via gather, with cover-N. ---
  {
    const Address ea = A("EA"), eb = A("EB"), ec = A("EC");
    ctl.mint(ea, "ea", {});
    ctl.mint(eb, "eb", {});
    ctl.mint(ec, "ec", {});

    MoveRecord op;
    op.sources = {MoveSource::Prefix(A("E*"))};
    // Elastic FROM, no TO: fills exactly the resolved N sequentially.
    op.destination = AddressSpan{AddressSegment::From(A("FA"))};

    update::MoveResult result = update::move_record(ctl, op);
    check(result.outcomes.size() == 3, "wildcard MOVE: gather resolved N=3 outcomes");
    bool all_moved = result.outcomes.size() == 3;
    for (const auto &o : result.outcomes) all_moved = all_moved && o.moved;
    check(all_moved, "wildcard MOVE: every resolved source moved");

    check(!ctl.token_exists(ea) && !ctl.token_exists(eb) && !ctl.token_exists(ec),
          "wildcard MOVE: old trunk is empty afterward");
    check(ctl.token_exists(A("FA")) && ctl.token_exists(A("FB")) && ctl.token_exists(A("FC")),
          "wildcard MOVE: destination sequentially filled FA/FB/FC");
    // Gather's PK order (EA,EB,EC) maps onto the destination's fill order
    // (FA,FB,FC) -- see README.md "Source ordering".
    check(result.outcomes.size() == 3 && result.outcomes[0].from == ea &&
              result.outcomes[0].to == A("FA") && result.outcomes[1].from == eb &&
              result.outcomes[1].to == A("FB") && result.outcomes[2].from == ec &&
              result.outcomes[2].to == A("FC"),
          "wildcard MOVE: gather order paired with destination fill order");
  }

  // --- Cover-N mismatch (wildcard source) is rejected. ---
  {
    const Address ga = A("GA"), gb = A("GB"), gc = A("GC");
    ctl.mint(ga, "ga", {});
    ctl.mint(gb, "gb", {});
    ctl.mint(gc, "gc", {});

    MoveRecord op;
    op.sources = {MoveSource::Prefix(A("G*"))};  // resolves to N=3.
    op.destination = AddressSpan{AddressSegment::Direct(A("HA")),
                                  AddressSegment::Direct(A("HB"))};  // covers only 2.

    update::MoveResult result = update::move_record(ctl, op);
    check(result.outcomes.size() == 1 && !result.outcomes[0].moved,
          "cover-N mismatch: whole MOVE rejected as one outcome");
    check(result.outcomes.size() == 1 &&
              contains_reason(result.outcomes[0].reason, "does not cover"),
          "cover-N mismatch: reason names the cover-N failure");
    check(ctl.token_exists(ga) && ctl.token_exists(gb) && ctl.token_exists(gc),
          "cover-N mismatch: nothing moved, sources untouched");
    check(!ctl.token_exists(A("HA")) && !ctl.token_exists(A("HB")),
          "cover-N mismatch: destination untouched");
  }

  // --- MOVE destination with a mint-bearing segment is rejected. ---
  {
    const Address src = A("IA");
    ctl.mint(src, "ia", {});

    // Nested-declare destination segment: structurally valid IR (a plain
    // 2-constituent structure node), but a relocation mints nothing.
    {
      auto nested = std::make_shared<DeclareRecord>();
      nested->parents =
          std::vector<ConstituentList>{ConstituentList{RefTo(A("QA")), RefTo(A("QB"))}};
      nested->address = AddressSpan{AddressSegment::Direct(A("QC"))};

      MoveRecord op;
      op.sources = {MoveSource::Explicit(src)};
      op.destination = AddressSpan{AddressSegment::Nested(nested)};

      update::MoveResult result = update::move_record(ctl, op);
      check(result.outcomes.size() == 1 && !result.outcomes[0].moved,
            "MOVE destination nested-declare: rejected as one outcome");
      check(result.outcomes.size() == 1 &&
                contains_reason(result.outcomes[0].reason, "nested declare"),
            "MOVE destination nested-declare: reason names the nested-declare segment");
      check(!ctl.token_exists(A("QC")), "MOVE destination nested-declare: nothing minted");
      check(ctl.token_exists(src), "MOVE destination nested-declare: source untouched");
    }

    // Undeclared-hook destination segment.
    {
      MoveRecord op;
      op.sources = {MoveSource::Explicit(src)};
      op.destination = AddressSpan{AddressSegment::Undeclared()};

      update::MoveResult result = update::move_record(ctl, op);
      check(result.outcomes.size() == 1 && !result.outcomes[0].moved,
            "MOVE destination undeclared hook: rejected as one outcome");
      check(result.outcomes.size() == 1 &&
                contains_reason(result.outcomes[0].reason, "undeclared hook"),
            "MOVE destination undeclared hook: reason names the undeclared-hook segment");
      check(ctl.token_exists(src), "MOVE destination undeclared hook: source untouched");
    }
  }

  // --- Destination collision (rekey's own guard) is a clean per-source
  //     rejection, not a thrown exception through this module. ---
  {
    const Address src = A("JA");
    const Address occupied = A("JB");
    ctl.mint(src, "ja", {});
    ctl.mint(occupied, "jb", {});

    MoveRecord op;
    op.sources = {MoveSource::Explicit(src)};
    op.destination = AddressSpan{AddressSegment::Direct(occupied)};

    update::MoveResult result = update::move_record(ctl, op);
    check(result.outcomes.size() == 1 && !result.outcomes[0].moved,
          "MOVE destination collision: rejected");
    check(ctl.token_exists(src) && ctl.token_exists(occupied),
          "MOVE destination collision: both tokens untouched");
  }
}

// --------------------------------------------------------------------
// ADD_CONNECTION
// --------------------------------------------------------------------

void check_add_connection(dbk::Controller &ctl) {
  // --- Explicit group + explicit element, idempotent re-add. ---
  {
    const Address m1 = A("MA");
    const Address g1 = A("MB");
    ctl.mint(m1, "m1", {});
    ctl.mint(g1, "g1", {});

    AddConnection op;
    op.group = g1;
    op.elements = {m1};

    update::AddConnectionResult r1 = update::add_connection(ctl, op);
    check(r1.outcomes.size() == 1 && r1.outcomes[0].added,
          "ADD_CONNECTION explicit: pair added");
    update::AddConnectionResult r2 = update::add_connection(ctl, op);  // re-add.
    check(r2.outcomes.size() == 1 && r2.outcomes[0].added,
          "ADD_CONNECTION explicit: re-add is a clean success (idempotent)");
    check(count_of(ctl.member_of(m1), g1) == 1,
          "ADD_CONNECTION explicit: no duplicate edge (member_of)");
    check(count_of(ctl.members_of(g1), m1) == 1,
          "ADD_CONNECTION explicit: no duplicate edge (members_of, reciprocal)");
  }

  // --- Wildcard elements -> one group. ---
  {
    const Address ia = A("NA"), ib = A("NB"), ic = A("NC");
    ctl.mint(ia, "ia", {});
    ctl.mint(ib, "ib", {});
    ctl.mint(ic, "ic", {});
    const Address g2 = A("PA");
    ctl.mint(g2, "g2", {});

    AddConnection op;
    op.group = g2;
    op.elements = {A("N*")};

    update::AddConnectionResult result = update::add_connection(ctl, op);
    check(result.outcomes.size() == 3, "ADD_CONNECTION wildcard elements: 3 pairs enumerated");
    bool all_added = result.outcomes.size() == 3;
    for (const auto &o : result.outcomes) all_added = all_added && o.added;
    check(all_added, "ADD_CONNECTION wildcard elements: every pair added");
    const auto members = ctl.members_of(g2);
    check(contains(members, ia) && contains(members, ib) && contains(members, ic),
          "ADD_CONNECTION wildcard elements: group's members_of includes all three");
  }

  // --- Member -> wildcard groups. ---
  {
    const Address m3 = A("PB");
    ctl.mint(m3, "m3", {});
    const Address j1 = A("RA"), j2 = A("RB");
    ctl.mint(j1, "j1", {});
    ctl.mint(j2, "j2", {});

    AddConnection op;
    op.group = A("R*");
    op.elements = {m3};

    update::AddConnectionResult result = update::add_connection(ctl, op);
    check(result.outcomes.size() == 2, "ADD_CONNECTION wildcard group: 2 pairs enumerated");
    bool all_added = result.outcomes.size() == 2;
    for (const auto &o : result.outcomes) all_added = all_added && o.added;
    check(all_added, "ADD_CONNECTION wildcard group: every pair added");
    const auto groups = ctl.member_of(m3);
    check(contains(groups, j1) && contains(groups, j2),
          "ADD_CONNECTION wildcard group: member's member_of includes both groups");
  }

  // --- Both sides wildcard: the full M*N cross-product, BLESSED
  //     (Patrick, 2026-09-18; NOTES.md "Build-phase rulings" ->
  //     ADD_CONNECTION) as a legitimate bulk op -- pinned here as
  //     INTENDED behaviour, not merely tolerated: 2 groups * 3 members
  //     must yield exactly 6 edges, every member joining every group. ---
  {
    const Address wa = A("WA"), wb = A("WB"), wc = A("WC");  // 3 members.
    ctl.mint(wa, "wa", {});
    ctl.mint(wb, "wb", {});
    ctl.mint(wc, "wc", {});
    const Address xa = A("XA"), xb = A("XB");  // 2 groups.
    ctl.mint(xa, "xa", {});
    ctl.mint(xb, "xb", {});

    AddConnection op;
    op.group = A("X*");
    op.elements = {A("W*")};

    update::AddConnectionResult result = update::add_connection(ctl, op);
    check(result.outcomes.size() == 6,
          "ADD_CONNECTION both-wildcard cross-product: 2 groups x 3 members = 6 edges");
    bool all_added = result.outcomes.size() == 6;
    for (const auto &o : result.outcomes) all_added = all_added && o.added;
    check(all_added, "ADD_CONNECTION both-wildcard cross-product: every edge added");
    for (const Address &member : {wa, wb, wc}) {
      const auto member_groups = ctl.member_of(member);
      check(contains(member_groups, xa) && contains(member_groups, xb),
            "ADD_CONNECTION both-wildcard cross-product: every member joins every group");
    }
    for (const Address &group : {xa, xb}) {
      const auto group_members = ctl.members_of(group);
      check(contains(group_members, wa) && contains(group_members, wb) &&
                contains(group_members, wc),
            "ADD_CONNECTION both-wildcard cross-product: every group contains every member");
    }
  }

  // --- Missing endpoints: clean per-pair / whole-statement rejection. ---
  {
    const Address g1 = A("MB");  // existing, from the first sub-test.
    const Address missing_element = A("SA");
    AddConnection op1;
    op1.group = g1;
    op1.elements = {missing_element};
    update::AddConnectionResult r1 = update::add_connection(ctl, op1);
    check(r1.outcomes.size() == 1 && !r1.outcomes[0].added,
          "ADD_CONNECTION missing element: rejected, not thrown");
    check(!contains(ctl.member_of(missing_element), g1),
          "ADD_CONNECTION missing element: nothing written");

    const Address missing_group = A("SB");
    const Address m1 = A("MA");  // existing, from the first sub-test.
    AddConnection op2;
    op2.group = missing_group;
    op2.elements = {m1};
    update::AddConnectionResult r2 = update::add_connection(ctl, op2);
    check(r2.outcomes.size() == 1 && !r2.outcomes[0].added,
          "ADD_CONNECTION missing group: whole statement rejected, not thrown");
    check(!contains(ctl.member_of(m1), missing_group),
          "ADD_CONNECTION missing group: nothing written");
  }
}

// --------------------------------------------------------------------
// DELETE_RECORD / DELETE_CONNECTION
// --------------------------------------------------------------------

void check_delete_record(dbk::Controller &ctl) {
  const Address l1 = A("TA");
  const Address l2 = A("TB");
  ctl.mint(l1, "l1", {});
  ctl.mint(l2, "l2", {});

  DeleteRecord op;
  op.token = l1;

  // Mismatched confirmation: rejected, nothing touched.
  {
    DeleteRecord bad_confirm;
    bad_confirm.token = l2;
    update::DeleteResult result = update::delete_record(ctl, op, bad_confirm);
    check(!result.deleted, "DELETE_RECORD mismatched confirm: rejected");
    check(contains_reason(result.reason, "confirmation does not match"),
          "DELETE_RECORD mismatched confirm: reason names the mismatch");
    check(ctl.token_exists(l1), "DELETE_RECORD mismatched confirm: target untouched");
  }

  // Correct confirmation: executes.
  {
    DeleteRecord confirm;
    confirm.token = l1;
    update::DeleteResult result = update::delete_record(ctl, op, confirm);
    check(result.deleted, "DELETE_RECORD correct confirm: executed");
    check(!ctl.token_exists(l1), "DELETE_RECORD correct confirm: token gone");
  }

  // Reject-on-referenced (G10), probe-driven: a token still referenced on
  // ANY of the four axes cannot be deleted, no cascade invented, and
  // Controller::delete_token is never even attempted -- checked for each
  // axis independently, each verified via read-back that nothing was
  // deleted.
  {
    const Address n1 = A("TC");
    const Address n2 = A("TD");
    ctl.mint(n1, "n1", {});
    ctl.mint(n2, "n2", {});
    const Address n3 = A("UA");
    ctl.mint(n3, "n3", {dbk::Constituent{n1, std::nullopt}, dbk::Constituent{n2, std::nullopt}});

    // Axis: children_of(n1) non-empty -- n1 is used as n3's constituent.
    DeleteRecord referenced_op;
    referenced_op.token = n1;
    update::DeleteResult result = update::delete_record(ctl, referenced_op, referenced_op);
    check(!result.deleted, "DELETE_RECORD reject-on-referenced (children_of): rejected, not thrown");
    check(contains_reason(result.reason, "G10"),
          "DELETE_RECORD reject-on-referenced (children_of): reason names G10");
    check(ctl.token_exists(n1),
          "DELETE_RECORD reject-on-referenced (children_of): token still present");

    // Axis: parents_of(n3) non-empty -- n3 itself carries its own PARENTS
    // rows; a composite can't be deleted out from under them either.
    DeleteRecord composite_op;
    composite_op.token = n3;
    update::DeleteResult composite_result =
        update::delete_record(ctl, composite_op, composite_op);
    check(!composite_result.deleted,
          "DELETE_RECORD reject-on-referenced (parents_of): rejected, not thrown");
    check(contains_reason(composite_result.reason, "G10"),
          "DELETE_RECORD reject-on-referenced (parents_of): reason names G10");
    check(ctl.token_exists(n3),
          "DELETE_RECORD reject-on-referenced (parents_of): token still present");

    // Axis: members_of(group) non-empty -- group has a member.
    const Address group = A("UB");
    const Address member = A("UC");
    ctl.mint(group, "group", {});
    ctl.mint(member, "member", {});
    ctl.add_membership(member, group);

    DeleteRecord group_op;
    group_op.token = group;
    update::DeleteResult group_result = update::delete_record(ctl, group_op, group_op);
    check(!group_result.deleted,
          "DELETE_RECORD reject-on-referenced (members_of): rejected, not thrown");
    check(contains_reason(group_result.reason, "G10"),
          "DELETE_RECORD reject-on-referenced (members_of): reason names G10");
    check(ctl.token_exists(group),
          "DELETE_RECORD reject-on-referenced (members_of): token still present");

    // Axis: member_of(member) non-empty -- member belongs to that group.
    DeleteRecord member_op;
    member_op.token = member;
    update::DeleteResult member_result = update::delete_record(ctl, member_op, member_op);
    check(!member_result.deleted,
          "DELETE_RECORD reject-on-referenced (member_of): rejected, not thrown");
    check(contains_reason(member_result.reason, "G10"),
          "DELETE_RECORD reject-on-referenced (member_of): reason names G10");
    check(ctl.token_exists(member),
          "DELETE_RECORD reject-on-referenced (member_of): token still present");
  }

  // Nonexistent target: clean rejection.
  {
    DeleteRecord missing_op;
    missing_op.token = A("UZ");
    update::DeleteResult result = update::delete_record(ctl, missing_op, missing_op);
    check(!result.deleted, "DELETE_RECORD nonexistent target: rejected, not thrown");
  }
}

void check_delete_connection(dbk::Controller &ctl) {
  const Address pm = A("VA");
  const Address pg = A("VB");
  ctl.mint(pm, "pm", {});
  ctl.mint(pg, "pg", {});
  ctl.add_membership(pm, pg);

  DeleteConnection op;
  op.a = pm;
  op.b = pg;

  // Mismatched confirmation: rejected, edge untouched.
  {
    DeleteConnection bad_confirm;
    bad_confirm.a = pm;
    bad_confirm.b = A("VC");  // some other, unrelated address.
    ctl.mint(bad_confirm.b, "unrelated", {});
    update::DeleteResult result = update::delete_connection(ctl, op, bad_confirm);
    check(!result.deleted, "DELETE_CONNECTION mismatched confirm: rejected");
    check(contains(ctl.member_of(pm), pg),
          "DELETE_CONNECTION mismatched confirm: edge untouched");
  }

  // Correct confirmation: executes, reciprocal removed too.
  {
    update::DeleteResult result = update::delete_connection(ctl, op, op);
    check(result.deleted, "DELETE_CONNECTION correct confirm: executed");
    check(!contains(ctl.member_of(pm), pg),
          "DELETE_CONNECTION correct confirm: member_of edge gone");
    check(!contains(ctl.members_of(pg), pm),
          "DELETE_CONNECTION correct confirm: members_of reciprocal gone too");
  }

  // No such edge: clean rejection, not a silent no-op success.
  {
    const Address unrelated_a = A("VD");
    const Address unrelated_b = A("VE");
    ctl.mint(unrelated_a, "ua", {});
    ctl.mint(unrelated_b, "ub", {});
    DeleteConnection no_edge_op;
    no_edge_op.a = unrelated_a;
    no_edge_op.b = unrelated_b;
    update::DeleteResult result = update::delete_connection(ctl, no_edge_op, no_edge_op);
    check(!result.deleted, "DELETE_CONNECTION no such edge: rejected");
    check(contains_reason(result.reason, "no such membership edge"),
          "DELETE_CONNECTION no such edge: reason names the missing edge");
  }
}

}  // namespace

int main(int argc, char **argv) {
  const std::string schema_path = (argc > 1) ? argv[1] : "../schema/schema.sql";
  const std::string conninfo = "dbname=hcp3_core";

  {
    PGconn *maint = PQconnectdb("dbname=postgres");
    if (maint == nullptr || PQstatus(maint) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL update_core_test: no local Postgres reachable: %s\n",
                   maint ? PQerrorMessage(maint) : "null connection");
      if (maint) PQfinish(maint);
      return 1;
    }
    PGresult *r = PQexec(maint, "SELECT 1 FROM pg_database WHERE datname = 'hcp3_core'");
    const bool exists = PQresultStatus(r) == PGRES_TUPLES_OK && PQntuples(r) > 0;
    PQclear(r);
    if (!exists && !exec_bare(maint, "CREATE DATABASE hcp3_core")) {
      std::fprintf(stderr, "FAIL update_core_test: could not create hcp3_core\n");
      PQfinish(maint);
      return 1;
    }
    PQfinish(maint);
  }

  {
    PGconn *db = PQconnectdb(conninfo.c_str());
    if (db == nullptr || PQstatus(db) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL update_core_test: cannot connect to hcp3_core: %s\n",
                   db ? PQerrorMessage(db) : "null connection");
      if (db) PQfinish(db);
      return 1;
    }
    const bool reset = exec_bare(db, "DROP SCHEMA public CASCADE; CREATE SCHEMA public;");
    const bool applied = reset && exec_bare(db, read_file(schema_path));
    PQfinish(db);
    if (!applied) {
      std::fprintf(stderr, "FAIL update_core_test: schema reset/apply failed\n");
      return 1;
    }
  }

  try {
    dbk::Controller ctl(conninfo);
    check_move_record(ctl);
    check_add_connection(ctl);
    check_delete_record(ctl);
    check_delete_connection(ctl);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "FAIL update_core_test: unexpected exception: %s\n", e.what());
    ++g_failures;
  }

  if (g_failures == 0) {
    std::printf("PASS update_core_test\n");
    return 0;
  }
  std::printf("FAIL update_core_test (%d failed)\n", g_failures);
  return 1;
}
