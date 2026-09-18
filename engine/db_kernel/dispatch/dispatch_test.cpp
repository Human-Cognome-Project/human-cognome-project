// Standalone check harness for the verb dispatcher + arraying executor
// (dispatch/dispatch.h), same tiny check-macro style as
// declare/declare_core_test.cpp and friends.
//
// Runs directly against the real, disposable hcp3_core database: resets it
// (DROP SCHEMA public CASCADE; CREATE SCHEMA public;) and reapplies
// ../schema/schema.sql, then drives every record-tier verb through
// dispatch_one/dispatch_stream over a live dbk::Controller. This is the
// "minimal testable surface" PLAN.md II.8 asks for: everything below
// constructs command:: IR directly in C++ and dispatches it -- there is
// no text/wire parser to drive here (that layer is deferred, G6).
//
// If no local Postgres is reachable it prints a clear message and exits
// non-zero rather than faking anything.
#include <libpq-fe.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "codec.h"
#include "command_ir.h"
#include "controller.h"
#include "dispatch.h"

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

Address A(const std::string &token_id) {
  const auto a = codec::decode_token_id(token_id);
  if (!a.has_value()) {
    std::fprintf(stderr, "test bug: undecodable token_id '%s'\n", token_id.c_str());
    std::exit(2);
  }
  return *a;
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
void run_dispatch_checks(const std::string &conninfo) {
  dbk::Controller ctl(conninfo);

  // --- Seed fixtures: mass-1 atoms, no constituents, minted directly via
  //     the controller (the seed floor is a bootstrap channel, not a
  //     dispatched verb -- PLAN.md II.3 "Mass"). ---
  const Address p1 = A("AA");
  const Address p2 = A("AB");
  const Address p3 = A("AC");
  const Address grp = A("AD");
  const Address del_ok = A("AE");    // an isolated token: safe to delete.
  const Address del_bad = A("AF");   // an isolated token: confirm-mismatch case.
  const Address grp2 = A("AG");      // a second group, for the 4-verb chained stream below.
  for (const auto &addr : {p1, p2, p3, grp, del_ok, del_bad, grp2}) {
    ctl.mint(addr, "seed-atom", {}, 1);
  }
  check(ctl.token_exists(p1) && ctl.token_exists(del_bad),
        "seed fixtures: atoms minted directly via the controller");

  // ---------------------------------------------------------------
  // DECLARE, then READ it back -- both through dispatch_one.
  // ---------------------------------------------------------------
  {
    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{
        ConstituentList{RefTo(p1), RefTo(p2)}};
    node.address = AddressSpan{AddressSegment::Direct(A("BA"))};
    node.notation = {std::string("composite-1")};

    dispatch::Result declared = dispatch::dispatch_one(ctl, dispatch::Command(node));
    check(declared.verb == dispatch::Verb::kDeclare, "DECLARE dispatches to Verb::kDeclare");
    const auto &dresult = std::get<declare::Result>(declared.value);
    check(dresult.outcomes.size() == 1 && dresult.outcomes[0].ingested &&
              dresult.outcomes[0].address == A("BA"),
          "DECLARE via dispatch: minted at its ADDRESS");

    command::ReadRecord read_op;
    read_op.anchor = A("BA");
    dispatch::Result read_back = dispatch::dispatch_one(ctl, dispatch::Command(read_op));
    check(read_back.verb == dispatch::Verb::kRead, "READ dispatches to Verb::kRead");
    const auto &rresult = std::get<dbread::ReadResult>(read_back.value);
    check(!rresult.nodes.empty() && rresult.nodes.front().address == A("BA") &&
              rresult.nodes.front().depth == 0,
          "READ via dispatch: anchor node comes back at depth 0");
  }

  // ---------------------------------------------------------------
  // MOVE_RECORD via dispatch_one.
  // ---------------------------------------------------------------
  {
    command::MoveRecord move_op;
    move_op.sources = {command::MoveSource::Explicit(A("BA"))};
    move_op.destination = AddressSpan{AddressSegment::Direct(A("BB"))};

    dispatch::Result moved = dispatch::dispatch_one(ctl, dispatch::Command(move_op));
    check(moved.verb == dispatch::Verb::kMove, "MOVE_RECORD dispatches to Verb::kMove");
    const auto &mresult = std::get<update::MoveResult>(moved.value);
    check(mresult.outcomes.size() == 1 && mresult.outcomes[0].moved &&
              mresult.outcomes[0].from == A("BA") && mresult.outcomes[0].to == A("BB"),
          "MOVE_RECORD via dispatch: BA rekeyed to BB");
    check(!ctl.token_exists(A("BA")) && ctl.token_exists(A("BB")),
          "MOVE_RECORD via dispatch: store reflects the rekey");
  }

  // ---------------------------------------------------------------
  // ADD_CONNECTION via dispatch_one.
  // ---------------------------------------------------------------
  {
    command::AddConnection add_op;
    add_op.group = grp;
    add_op.elements = {p3};

    dispatch::Result added = dispatch::dispatch_one(ctl, dispatch::Command(add_op));
    check(added.verb == dispatch::Verb::kAddConnection,
          "ADD_CONNECTION dispatches to Verb::kAddConnection");
    const auto &aresult = std::get<update::AddConnectionResult>(added.value);
    check(aresult.outcomes.size() == 1 && aresult.outcomes[0].added,
          "ADD_CONNECTION via dispatch: pair added");
    const auto groups_of_p3 = ctl.member_of(p3);
    check(std::find(groups_of_p3.begin(), groups_of_p3.end(), grp) != groups_of_p3.end(),
          "ADD_CONNECTION via dispatch: member_of(p3) contains the group");
  }

  // ---------------------------------------------------------------
  // Gated DELETE_RECORD / DELETE_CONNECTION via dispatch_one.
  // ---------------------------------------------------------------
  {
    dispatch::DeleteRecordRequest req;
    req.op.token = del_ok;
    req.confirm.token = del_ok;
    dispatch::Result deleted = dispatch::dispatch_one(ctl, dispatch::Command(req));
    check(deleted.verb == dispatch::Verb::kDeleteRecord,
          "DELETE_RECORD dispatches to Verb::kDeleteRecord");
    const auto &dresult = std::get<update::DeleteResult>(deleted.value);
    check(dresult.deleted, "DELETE_RECORD via dispatch: matching confirm deletes");
    check(!ctl.token_exists(del_ok), "DELETE_RECORD via dispatch: token gone from the store");

    dispatch::DeleteRecordRequest bad_req;
    bad_req.op.token = del_bad;
    bad_req.confirm.token = p1;  // mismatched -- not a re-statement of the target.
    dispatch::Result rejected = dispatch::dispatch_one(ctl, dispatch::Command(bad_req));
    const auto &rresult = std::get<update::DeleteResult>(rejected.value);
    check(!rresult.deleted, "DELETE_RECORD via dispatch: mismatched confirm rejects");
    check(ctl.token_exists(del_bad),
          "DELETE_RECORD via dispatch: rejected delete leaves the token untouched");
  }
  {
    ctl.add_membership(del_bad, grp);  // give it one edge to delete.
    dispatch::DeleteConnectionRequest req;
    req.op = command::DeleteConnection{del_bad, grp};
    req.confirm = req.op;
    dispatch::Result deleted = dispatch::dispatch_one(ctl, dispatch::Command(req));
    check(deleted.verb == dispatch::Verb::kDeleteConnection,
          "DELETE_CONNECTION dispatches to Verb::kDeleteConnection");
    const auto &dresult = std::get<update::DeleteResult>(deleted.value);
    check(dresult.deleted, "DELETE_CONNECTION via dispatch: matching confirm deletes the pair");
    const auto groups_of_del_bad = ctl.member_of(del_bad);
    check(std::find(groups_of_del_bad.begin(), groups_of_del_bad.end(), grp) ==
              groups_of_del_bad.end(),
          "DELETE_CONNECTION via dispatch: reciprocal edge gone");
  }

  // ---------------------------------------------------------------
  // Cache-tier verbs: named stubs, ctl untouched.
  // ---------------------------------------------------------------
  {
    dispatch::Result r = dispatch::dispatch_one(ctl, dispatch::Command(dispatch::Reconcile{}));
    check(r.verb == dispatch::Verb::kReconcile &&
              std::get<std::string>(r.value) == "RECONCILE: not yet implemented",
          "RECONCILE dispatches to its non-fatal stub");
  }
  {
    dispatch::Result r =
        dispatch::dispatch_one(ctl, dispatch::Command(dispatch::UpdateCache{}));
    check(r.verb == dispatch::Verb::kUpdateCache &&
              std::get<std::string>(r.value) == "UPDATE_CACHE: not yet implemented",
          "UPDATE_CACHE dispatches to its non-fatal stub");
  }
  {
    dispatch::Result r =
        dispatch::dispatch_one(ctl, dispatch::Command(dispatch::RebaseCache{}));
    check(r.verb == dispatch::Verb::kRebaseCache &&
              std::get<std::string>(r.value) == "REBASE_CACHE: not yet implemented",
          "REBASE_CACHE dispatches to its non-fatal stub");
  }

  // ---------------------------------------------------------------
  // The arraying executor: an ordered, additive stream. A DECLARE
  // followed by a MOVE targeting what that DECLARE just placed --
  // this only succeeds if the stream executes strictly in order
  // (PLAN.md I.F / II.7; NOTES.md "Arraying is universal": "ordering
  // is semantic ... execute in order").
  // ---------------------------------------------------------------
  {
    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{
        ConstituentList{RefTo(p1), RefTo(p2)}};
    node.address = AddressSpan{AddressSegment::Direct(A("CA"))};

    command::MoveRecord move_op;
    move_op.sources = {command::MoveSource::Explicit(A("CA"))};
    move_op.destination = AddressSpan{AddressSegment::Direct(A("CB"))};

    std::vector<dispatch::AdditiveCommand> stream = {
        dispatch::AdditiveCommand(node), dispatch::AdditiveCommand(move_op)};
    std::vector<dispatch::Result> results = dispatch::dispatch_stream(ctl, stream);

    check(results.size() == 2, "arrayed stream: one Result per stream item, in order");
    check(results[0].verb == dispatch::Verb::kDeclare &&
              std::get<declare::Result>(results[0].value).outcomes[0].ingested,
          "arrayed stream: item 0 (DECLARE) ran and ingested");
    const bool move_saw_the_declare =
        results[1].verb == dispatch::Verb::kMove &&
        std::get<update::MoveResult>(results[1].value).outcomes[0].moved;
    check(move_saw_the_declare,
          "arrayed stream: item 1 (MOVE) succeeded against what item 0 just "
          "declared -- proves in-order, synchronous execution");
    check(!ctl.token_exists(A("CA")) && ctl.token_exists(A("CB")),
          "arrayed stream: final store state reflects both ops in sequence");
  }

  // ---------------------------------------------------------------
  // The arraying executor, all four additive verbs in ONE ordered
  // stream, each depending on the one before it: DECLARE places a
  // token; READ (same stream) reads back what was JUST declared; MOVE
  // relocates it; ADD_CONNECTION attaches the token at its POST-MOVE
  // address to a group. None of this is a single core's own
  // intra-command array (that would be one DeclareRecord/MoveRecord/
  // AddConnection with an N>1 field) -- these are four DISTINCT verb
  // invocations, of four DIFFERENT verbs, chained through the executor.
  // Per Patrick's ruling: DECLARE, READ, MOVE_RECORD and ADD_CONNECTION
  // are ALL batchable this way -- DELETE_RECORD/DELETE_CONNECTION are
  // the sole non-batchable (destructive) carve-outs, which is exactly
  // why AdditiveCommand has no alternative for either of them.
  // ---------------------------------------------------------------
  {
    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{
        ConstituentList{RefTo(p1), RefTo(p2)}};
    node.address = AddressSpan{AddressSegment::Direct(A("DA"))};

    command::ReadRecord read_op;
    read_op.anchor = A("DA");  // exists only because item 0 just ran.

    command::MoveRecord move_op;
    move_op.sources = {command::MoveSource::Explicit(A("DA"))};
    move_op.destination = AddressSpan{AddressSegment::Direct(A("DB"))};

    command::AddConnection add_op;
    add_op.group = grp2;
    add_op.elements = {A("DB")};  // exists only because item 2 just moved it there.

    std::vector<dispatch::AdditiveCommand> stream = {
        dispatch::AdditiveCommand(node), dispatch::AdditiveCommand(read_op),
        dispatch::AdditiveCommand(move_op), dispatch::AdditiveCommand(add_op)};
    std::vector<dispatch::Result> results = dispatch::dispatch_stream(ctl, stream);

    check(results.size() == 4,
          "4-verb chained stream: one Result per item, DECLARE+READ+MOVE_RECORD+ADD_CONNECTION");
    check(results[0].verb == dispatch::Verb::kDeclare &&
              std::get<declare::Result>(results[0].value).outcomes[0].ingested,
          "4-verb chained stream: item 0 DECLARE ingested at DA");
    check(results[1].verb == dispatch::Verb::kRead &&
              !std::get<dbread::ReadResult>(results[1].value).nodes.empty(),
          "4-verb chained stream: item 1 READ found DA (proves it ran after item 0)");
    check(results[2].verb == dispatch::Verb::kMove &&
              std::get<update::MoveResult>(results[2].value).outcomes[0].moved,
          "4-verb chained stream: item 2 MOVE_RECORD relocated DA to DB");
    check(results[3].verb == dispatch::Verb::kAddConnection &&
              std::get<update::AddConnectionResult>(results[3].value).outcomes[0].added,
          "4-verb chained stream: item 3 ADD_CONNECTION attached DB (proves it ran after "
          "the move placed it there)");
    const auto groups_of_db = ctl.member_of(A("DB"));
    check(std::find(groups_of_db.begin(), groups_of_db.end(), grp2) != groups_of_db.end(),
          "4-verb chained stream: final store state reflects all four ops in sequence");
  }
}

}  // namespace

int main(int argc, char **argv) {
  const std::string schema_path = (argc > 1) ? argv[1] : "../schema/schema.sql";
  const std::string conninfo = "dbname=hcp3_core";

  {
    PGconn *maint = PQconnectdb("dbname=postgres");
    if (maint == nullptr || PQstatus(maint) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL dispatch_test: no local Postgres reachable: %s\n",
                   maint ? PQerrorMessage(maint) : "null connection");
      if (maint) PQfinish(maint);
      return 1;
    }
    PGresult *r = PQexec(maint, "SELECT 1 FROM pg_database WHERE datname = 'hcp3_core'");
    const bool exists = PQresultStatus(r) == PGRES_TUPLES_OK && PQntuples(r) > 0;
    PQclear(r);
    if (!exists && !exec_bare(maint, "CREATE DATABASE hcp3_core")) {
      std::fprintf(stderr, "FAIL dispatch_test: could not create hcp3_core\n");
      PQfinish(maint);
      return 1;
    }
    PQfinish(maint);
  }

  {
    PGconn *db = PQconnectdb(conninfo.c_str());
    if (db == nullptr || PQstatus(db) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL dispatch_test: cannot connect to hcp3_core: %s\n",
                   db ? PQerrorMessage(db) : "null connection");
      if (db) PQfinish(db);
      return 1;
    }
    const bool reset = exec_bare(db, "DROP SCHEMA public CASCADE; CREATE SCHEMA public;");
    const bool applied = reset && exec_bare(db, read_file(schema_path));
    PQfinish(db);
    if (!applied) {
      std::fprintf(stderr, "FAIL dispatch_test: schema reset/apply failed\n");
      return 1;
    }
  }

  try {
    run_dispatch_checks(conninfo);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "FAIL dispatch_test: unexpected exception: %s\n", e.what());
    ++g_failures;
  }

  if (g_failures == 0) {
    std::printf("PASS dispatch_test\n");
    return 0;
  }
  std::printf("FAIL dispatch_test (%d failed)\n", g_failures);
  return 1;
}
