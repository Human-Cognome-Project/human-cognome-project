// Standalone check harness for the db/cache manager's tier-2 analyst
// reaction body as a monitored-endpoint kernel (TIER2-PLAN.md). Same tiny
// check-macro style as wal/wal_kernel_test.cpp / dispatch/dispatch_test.cpp:
// one ok/FAIL line per check, PASS/FAIL summary, non-zero exit on any
// failure. Runs directly against the real, disposable hcp3_core database
// (reset the same way dispatch_test.cpp resets it), driving fixture
// dispatch::Command / dispatch::AdditiveCommand values through the
// endpoint/scheduler substrate rather than calling dispatch_one/
// dispatch_stream directly -- exactly what this module adds over
// dispatch/.
//
// If no local Postgres is reachable it prints a clear message and exits
// non-zero rather than faking anything.
#include <libpq-fe.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "codec.h"
#include "command_ir.h"
#include "controller.h"
#include "db_manager_kernel.h"
#include "dispatch.h"
#include "endpoint.h"
#include "scheduler.h"

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
using dbmanager::DbManagerKernel;
using dbmanager::Request;
using dbmanager::Response;

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
    std::fprintf(stderr, "exec failed: %s\n  sql: %s\n", PQresultErrorMessage(res), sql.c_str());
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

// Seeds `req` into the request arena and submits its handle (the arena
// index, decimal-string-rendered -- same handle-in-payload discipline as
// wal_kernel_test.cpp's seed_and_submit) to `box_id`, carrying `reply_to`
// as the request's own caller-supplied return endpoint. Does NOT run the
// scheduler.
std::size_t seed_and_submit(scheduler::Scheduler &sched, std::vector<Request> &req_arena,
                             endpoint::EndpointId box_id, endpoint::EndpointId reply_to,
                             Request req) {
  req_arena.push_back(std::move(req));
  const std::size_t idx = req_arena.size() - 1;
  sched.submit(box_id, box::Message{std::to_string(idx), reply_to});
  return idx;
}

// Pops one message off `box`, resolves its handle against `resp_arena`,
// and returns the Response it names. The box must be non-empty.
const Response &pop_response(box::Box &box, const std::vector<Response> &resp_arena) {
  const box::Message m = box.pop();
  std::size_t consumed = 0;
  const unsigned long long idx_ull = std::stoull(m.payload, &consumed);
  if (consumed != m.payload.size()) {
    std::fprintf(stderr, "test bug: malformed return-box handle '%s'\n", m.payload.c_str());
    std::exit(2);
  }
  return resp_arena.at(static_cast<std::size_t>(idx_ull));
}

// A snapshot of everything a READ traversal or a write could touch for a
// handful of tokens -- used for the READ no-mutation check (TIER2-PLAN.md
// "READ returns its result with no mutation, checked by a Controller
// DB-state diff"). Captures the relationship rows on both traversal
// sides plus the token's own mass, for every address in `tokens`.
struct RelationSnapshot {
  std::vector<Address> parents_of;
  std::vector<Address> children_of;
  std::vector<Address> members_of;
  std::vector<Address> member_of;
  std::optional<int> mass;
};

RelationSnapshot snapshot_of(dbk::Controller &ctl, const Address &token) {
  RelationSnapshot s;
  for (const auto &p : ctl.parents_of(token)) s.parents_of.push_back(p.parent);
  s.children_of = ctl.children_of(token);
  s.members_of = ctl.members_of(token);
  s.member_of = ctl.member_of(token);
  const auto attrs = ctl.attributes_of(token);
  s.mass = attrs.has_value() ? attrs->mass : std::nullopt;
  return s;
}

bool same_snapshot(const RelationSnapshot &a, const RelationSnapshot &b) {
  return a.parents_of == b.parents_of && a.children_of == b.children_of &&
         a.members_of == b.members_of && a.member_of == b.member_of && a.mass == b.mass;
}

// --------------------------------------------------------------------
void run_db_manager_kernel_checks(const std::string &conninfo) {
  dbk::Controller ctl(conninfo);

  // --- Seed fixtures: mass-1 atoms, minted directly via the controller
  //     (the seed floor is a bootstrap channel, not a dispatched verb --
  //     same convention as dispatch_test.cpp). ---
  const Address p1 = A("AA");
  const Address p2 = A("AB");
  const Address p3 = A("AC");
  const Address grp = A("AD");
  const Address del_ok = A("AE");
  const Address del_bad = A("AF");
  for (const auto &addr : {p1, p2, p3, grp, del_ok, del_bad}) {
    ctl.mint(addr, "seed-atom", {}, 1);
  }
  check(ctl.token_exists(p1) && ctl.token_exists(del_bad),
        "seed fixtures: atoms minted directly via the controller");

  // --- Wire the substrate: Registry(4 standing, 0 ephemeral) + one
  //     Scheduler(num_levels=4) -- the pinned tier map (0 reconcile, 1
  //     analyst, 2 pending, 3 maintenance), only level 1 exercised here.
  //     Two analyst in-boxes (register_box at level 1); two standing
  //     return boxes, driver-supplied, NOT registered with the scheduler
  //     (drained directly by this test), standing in for the deferred
  //     configuration routine (TIER2-PLAN.md "Return endpoints are
  //     driver-supplied"). ---
  endpoint::Registry registry(/*standing_capacity=*/4, /*ephemeral_capacity=*/0);
  scheduler::Scheduler sched(registry, /*num_levels=*/4);

  box::Box analyst1_box;
  box::Box analyst2_box;
  box::Box return1_box;
  box::Box return2_box;
  const endpoint::EndpointId analyst1_id = registry.register_standing(0, &analyst1_box);
  const endpoint::EndpointId analyst2_id = registry.register_standing(1, &analyst2_box);
  const endpoint::EndpointId return1_id = registry.register_standing(2, &return1_box);
  const endpoint::EndpointId return2_id = registry.register_standing(3, &return2_box);

  std::vector<Request> req_arena;
  std::vector<Response> resp_arena;
  DbManagerKernel kernel(req_arena, resp_arena, ctl);
  sched.register_box(&analyst1_box, /*priority=*/1, kernel.make_handler());
  sched.register_box(&analyst2_box, /*priority=*/1, kernel.make_handler());

  // ==================================================================
  // Test 1 -- each verb as a single Command routes its Result to reply_to.
  // ==================================================================
  {
    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{ConstituentList{RefTo(p1), RefTo(p2)}};
    node.address = AddressSpan{AddressSegment::Direct(A("BA"))};
    node.notation = {std::string("composite-1")};

    seed_and_submit(sched, req_arena, analyst1_id, return1_id,
                     Request(dispatch::Command(node)));
    sched.run_until_idle();

    const Response &resp = pop_response(return1_box, resp_arena);
    check(resp.size() == 1, "DECLARE (single Command): one-element Response");
    check(resp[0].verb == dispatch::Verb::kDeclare,
          "DECLARE (single Command): Result verb is kDeclare");
    const auto &dresult = std::get<declare::Result>(resp[0].value);
    check(dresult.outcomes.size() == 1 && dresult.outcomes[0].ingested &&
              dresult.outcomes[0].address == A("BA"),
          "DECLARE (single Command): routed Result shows the mint at BA");
  }
  {
    command::ReadRecord op;
    op.anchor = A("BA");

    seed_and_submit(sched, req_arena, analyst1_id, return1_id, Request(dispatch::Command(op)));
    sched.run_until_idle();

    const Response &resp = pop_response(return1_box, resp_arena);
    check(resp.size() == 1 && resp[0].verb == dispatch::Verb::kRead,
          "READ (single Command): one-element Response, verb kRead");
    const auto &rresult = std::get<dbread::ReadResult>(resp[0].value);
    check(!rresult.nodes.empty() && rresult.nodes.front().address == A("BA"),
          "READ (single Command): routed Result's anchor node is BA");
  }
  {
    command::MoveRecord op;
    op.sources = {command::MoveSource::Explicit(A("BA"))};
    op.destination = AddressSpan{AddressSegment::Direct(A("BB"))};

    seed_and_submit(sched, req_arena, analyst1_id, return1_id, Request(dispatch::Command(op)));
    sched.run_until_idle();

    const Response &resp = pop_response(return1_box, resp_arena);
    check(resp.size() == 1 && resp[0].verb == dispatch::Verb::kMove,
          "MOVE_RECORD (single Command): one-element Response, verb kMove");
    const auto &mresult = std::get<update::MoveResult>(resp[0].value);
    check(mresult.outcomes.size() == 1 && mresult.outcomes[0].moved &&
              mresult.outcomes[0].to == A("BB"),
          "MOVE_RECORD (single Command): routed Result shows BA rekeyed to BB");
    check(!ctl.token_exists(A("BA")) && ctl.token_exists(A("BB")),
          "MOVE_RECORD (single Command): store reflects the rekey");
  }
  {
    command::AddConnection op;
    op.group = grp;
    op.elements = {p3};

    seed_and_submit(sched, req_arena, analyst1_id, return1_id, Request(dispatch::Command(op)));
    sched.run_until_idle();

    const Response &resp = pop_response(return1_box, resp_arena);
    check(resp.size() == 1 && resp[0].verb == dispatch::Verb::kAddConnection,
          "ADD_CONNECTION (single Command): one-element Response, verb kAddConnection");
    const auto &aresult = std::get<update::AddConnectionResult>(resp[0].value);
    check(aresult.outcomes.size() == 1 && aresult.outcomes[0].added,
          "ADD_CONNECTION (single Command): routed Result shows the pair added");
  }
  {
    dispatch::DeleteRecordRequest req;
    req.op.token = del_ok;
    req.confirm.token = del_ok;

    seed_and_submit(sched, req_arena, analyst1_id, return1_id, Request(dispatch::Command(req)));
    sched.run_until_idle();

    const Response &resp = pop_response(return1_box, resp_arena);
    check(resp.size() == 1 && resp[0].verb == dispatch::Verb::kDeleteRecord,
          "DELETE_RECORD (single Command): one-element Response, verb kDeleteRecord");
    const auto &dresult = std::get<update::DeleteResult>(resp[0].value);
    check(dresult.deleted, "DELETE_RECORD (single Command): matching confirm deletes");
    check(!ctl.token_exists(del_ok), "DELETE_RECORD (single Command): token gone from the store");
  }
  {
    ctl.add_membership(del_bad, grp);  // give it one edge to delete.
    dispatch::DeleteConnectionRequest req;
    req.op = command::DeleteConnection{del_bad, grp};
    req.confirm = req.op;

    seed_and_submit(sched, req_arena, analyst1_id, return1_id, Request(dispatch::Command(req)));
    sched.run_until_idle();

    const Response &resp = pop_response(return1_box, resp_arena);
    check(resp.size() == 1 && resp[0].verb == dispatch::Verb::kDeleteConnection,
          "DELETE_CONNECTION (single Command): one-element Response, verb kDeleteConnection");
    const auto &dresult = std::get<update::DeleteResult>(resp[0].value);
    check(dresult.deleted,
          "DELETE_CONNECTION (single Command): matching confirm deletes the pair");
  }

  // ==================================================================
  // Test 2 -- an ordered stream (DECLARE then a MOVE of what it just
  // placed) preserves dependent ordering through the box, proving the
  // handler calls dispatch_stream (not per-item dispatch_one) for the
  // stream request form.
  // ==================================================================
  {
    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{ConstituentList{RefTo(p1), RefTo(p2)}};
    node.address = AddressSpan{AddressSegment::Direct(A("CA"))};

    command::MoveRecord move_op;
    move_op.sources = {command::MoveSource::Explicit(A("CA"))};
    move_op.destination = AddressSpan{AddressSegment::Direct(A("CB"))};

    std::vector<dispatch::AdditiveCommand> stream = {dispatch::AdditiveCommand(node),
                                                       dispatch::AdditiveCommand(move_op)};
    seed_and_submit(sched, req_arena, analyst1_id, return1_id, Request(stream));
    sched.run_until_idle();

    const Response &resp = pop_response(return1_box, resp_arena);
    check(resp.size() == 2, "ordered stream: two-element Response, one per stream item");
    check(resp[0].verb == dispatch::Verb::kDeclare &&
              std::get<declare::Result>(resp[0].value).outcomes[0].ingested,
          "ordered stream: item 0 (DECLARE) ran and ingested");
    const bool move_saw_the_declare =
        resp[1].verb == dispatch::Verb::kMove &&
        std::get<update::MoveResult>(resp[1].value).outcomes[0].moved;
    check(move_saw_the_declare,
          "ordered stream: item 1 (MOVE_RECORD) succeeded against what item 0 just declared -- "
          "dependent ordering preserved through the box");
    check(!ctl.token_exists(A("CA")) && ctl.token_exists(A("CB")),
          "ordered stream: final store state reflects both ops in sequence");
  }

  // ==================================================================
  // Test 3 -- partial-rejection stream: an earlier item's Result rejects
  // semantically (success flag false, NOT a throw -- an ADD_CONNECTION
  // naming a nonexistent group), and the stream still runs every
  // subsequent item to completion, returning the full N-length Response
  // to the one reply_to (dispatch_stream's unconditional loop, now
  // through a box).
  // ==================================================================
  {
    command::AddConnection bad_add;
    bad_add.group = A("ZZ");  // never minted -- whole-statement rejection.
    bad_add.elements = {p3};

    DeclareRecord node;
    node.parents = std::vector<ConstituentList>{ConstituentList{RefTo(p1), RefTo(p2)}};
    node.address = AddressSpan{AddressSegment::Direct(A("DA"))};

    std::vector<dispatch::AdditiveCommand> stream = {dispatch::AdditiveCommand(bad_add),
                                                       dispatch::AdditiveCommand(node)};
    seed_and_submit(sched, req_arena, analyst1_id, return1_id, Request(stream));
    sched.run_until_idle();

    const Response &resp = pop_response(return1_box, resp_arena);
    check(resp.size() == 2,
          "partial-rejection stream: full N-length Response returned despite item 0's rejection");
    const auto &bad_result = std::get<update::AddConnectionResult>(resp[0].value);
    check(!bad_result.outcomes.empty() && !bad_result.outcomes[0].added,
          "partial-rejection stream: item 0's Result carries success flag false, not a throw");
    check(resp[1].verb == dispatch::Verb::kDeclare &&
              std::get<declare::Result>(resp[1].value).outcomes[0].ingested,
          "partial-rejection stream: item 1 still ran to completion after item 0's rejection");
    check(ctl.token_exists(A("DA")),
          "partial-rejection stream: item 1's write landed in the store");
  }

  // ==================================================================
  // Test 4 -- multi-analyst correlation: two analyst boxes, two distinct
  // driver-supplied return endpoints; each request's Response routes to
  // its OWN reply_to, never crossed -- the box is the correlation.
  // ==================================================================
  {
    command::ReadRecord read_p1;
    read_p1.anchor = p1;
    command::ReadRecord read_p2;
    read_p2.anchor = p2;

    const std::size_t idx1 = seed_and_submit(sched, req_arena, analyst1_id, return1_id,
                                              Request(dispatch::Command(read_p1)));
    const std::size_t idx2 = seed_and_submit(sched, req_arena, analyst2_id, return2_id,
                                              Request(dispatch::Command(read_p2)));
    sched.run_until_idle();

    check(!return1_box.empty() && !return2_box.empty(),
          "multi-analyst correlation: both return boxes received an answer");
    check(return2_box.empty() == false, "multi-analyst correlation: return2 box populated");

    const Response &resp1 = pop_response(return1_box, resp_arena);
    const Response &resp2 = pop_response(return2_box, resp_arena);
    check(std::get<dbread::ReadResult>(resp1[0].value).nodes.front().address == p1,
          "multi-analyst correlation: analyst1's request (anchor p1) answers at return1");
    check(std::get<dbread::ReadResult>(resp2[0].value).nodes.front().address == p2,
          "multi-analyst correlation: analyst2's request (anchor p2) answers at return2");
    check(return1_box.empty() && return2_box.empty(),
          "multi-analyst correlation: each return box drained exactly one answer, no crossover");
    check(idx1 != idx2, "multi-analyst correlation: sanity, the two requests are distinct arena "
                         "entries");
  }

  // ==================================================================
  // Test 5 -- READ mutates nothing: a Controller DB-state diff around a
  // READ (single Command) shows no change to the anchor's relationship
  // rows or mass. There is no WAL manager in this harness to observe an
  // obligation on (TIER2-PLAN.md "no WAL report emission in tier 2").
  // ==================================================================
  {
    const RelationSnapshot before_p1 = snapshot_of(ctl, p1);
    const RelationSnapshot before_bb = snapshot_of(ctl, A("BB"));  // has structure edges (test 1's rekeyed BA)

    command::ReadRecord op;
    op.anchor = A("BB");
    op.depth = 3;

    seed_and_submit(sched, req_arena, analyst1_id, return1_id, Request(dispatch::Command(op)));
    sched.run_until_idle();
    pop_response(return1_box, resp_arena);  // drain; already checked elsewhere.

    const RelationSnapshot after_p1 = snapshot_of(ctl, p1);
    const RelationSnapshot after_bb = snapshot_of(ctl, A("BB"));

    check(same_snapshot(before_p1, after_p1),
          "READ no-mutation: p1's relationship rows and mass unchanged by a READ elsewhere");
    check(same_snapshot(before_bb, after_bb),
          "READ no-mutation: the READ's own anchor (BB) rows and mass unchanged by the READ");
  }

  // ==================================================================
  // Test 6 -- malformed arena handle throws (the substrate's bug-surface,
  // same as WalKernel on a bad handle), DB untouched.
  // ==================================================================
  {
    const bool p1_exists_before = ctl.token_exists(p1);

    sched.submit(analyst1_id, box::Message{"not-a-number", return1_id});

    bool threw_invalid_argument = false;
    try {
      sched.run_until_idle();
    } catch (const std::invalid_argument &) {
      threw_invalid_argument = true;
    } catch (...) {
      check(false, "malformed handle: the propagated exception was not std::invalid_argument");
    }
    check(threw_invalid_argument,
          "malformed handle: a non-decimal arena handle throws std::invalid_argument, propagated "
          "out of run_until_idle() uncaught");
    check(return1_box.empty(),
          "malformed handle: nothing was sent to reply_to -- the throw happens before dispatch");
    check(ctl.token_exists(p1) == p1_exists_before,
          "malformed handle: the store is untouched by the rejected request");
  }

  // ==================================================================
  // Test 7 -- absent reply_to on an otherwise well-formed request (a
  // valid arena handle, but the carried Message has no return endpoint)
  // throws std::invalid_argument, the same fail-loud discipline as a bad
  // arena handle -- NOT the unchecked-`*optional`-dereference UB this
  // regresses against, and NOT a silent no-op. Regression for the
  // adversary-found blocker: `sender.send(*message.reply_to, ...)` used
  // to dereference an empty optional unchecked.
  // ==================================================================
  {
    const bool p1_exists_before = ctl.token_exists(p1);

    command::ReadRecord op;
    op.anchor = p1;
    req_arena.push_back(Request(dispatch::Command(op)));
    const std::size_t idx = req_arena.size() - 1;

    // A well-formed handle (indexes a real, valid request) but no
    // reply_to -- distinct from Test 6's malformed-payload case.
    sched.submit(analyst1_id, box::Message{std::to_string(idx), std::nullopt});

    bool threw_invalid_argument = false;
    try {
      sched.run_until_idle();
    } catch (const std::invalid_argument &) {
      threw_invalid_argument = true;
    } catch (...) {
      check(false, "absent reply_to: the propagated exception was not std::invalid_argument");
    }
    check(threw_invalid_argument,
          "absent reply_to: a well-formed handle with no reply_to throws "
          "std::invalid_argument, propagated out of run_until_idle() uncaught");
    check(return1_box.empty() && return2_box.empty(),
          "absent reply_to: no return box received anything");
    check(ctl.token_exists(p1) == p1_exists_before,
          "absent reply_to: the store is untouched by the rejected request");
  }

  // ==================================================================
  // Test 8 -- a numeric but out-of-range arena handle throws
  // std::out_of_range from req_arena_.at(index) (untested region the
  // adversary noted).
  // ==================================================================
  {
    const std::size_t out_of_range_idx = req_arena.size() + 1000;
    sched.submit(analyst1_id, box::Message{std::to_string(out_of_range_idx), return1_id});

    bool threw_out_of_range = false;
    try {
      sched.run_until_idle();
    } catch (const std::out_of_range &) {
      threw_out_of_range = true;
    } catch (...) {
      check(false, "out-of-range handle: the propagated exception was not std::out_of_range");
    }
    check(threw_out_of_range,
          "out-of-range handle: a numeric but out-of-range arena handle throws "
          "std::out_of_range, propagated out of run_until_idle() uncaught");
    check(return1_box.empty(), "out-of-range handle: nothing was sent to reply_to");
  }
}

}  // namespace

int main(int argc, char **argv) {
  const std::string schema_path = (argc > 1) ? argv[1] : "../schema/schema.sql";
  const std::string conninfo = "dbname=hcp3_core";

  {
    PGconn *maint = PQconnectdb("dbname=postgres");
    if (maint == nullptr || PQstatus(maint) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL db_manager_kernel_test: no local Postgres reachable: %s\n",
                   maint ? PQerrorMessage(maint) : "null connection");
      if (maint) PQfinish(maint);
      return 1;
    }
    PGresult *r = PQexec(maint, "SELECT 1 FROM pg_database WHERE datname = 'hcp3_core'");
    const bool exists = PQresultStatus(r) == PGRES_TUPLES_OK && PQntuples(r) > 0;
    PQclear(r);
    if (!exists && !exec_bare(maint, "CREATE DATABASE hcp3_core")) {
      std::fprintf(stderr, "FAIL db_manager_kernel_test: could not create hcp3_core\n");
      PQfinish(maint);
      return 1;
    }
    PQfinish(maint);
  }

  {
    PGconn *db = PQconnectdb(conninfo.c_str());
    if (db == nullptr || PQstatus(db) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL db_manager_kernel_test: cannot connect to hcp3_core: %s\n",
                   db ? PQerrorMessage(db) : "null connection");
      if (db) PQfinish(db);
      return 1;
    }
    const bool reset = exec_bare(db, "DROP SCHEMA public CASCADE; CREATE SCHEMA public;");
    const bool applied = reset && exec_bare(db, read_file(schema_path));
    PQfinish(db);
    if (!applied) {
      std::fprintf(stderr, "FAIL db_manager_kernel_test: schema reset/apply failed\n");
      return 1;
    }
  }

  try {
    run_db_manager_kernel_checks(conninfo);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "FAIL db_manager_kernel_test: unexpected exception: %s\n", e.what());
    ++g_failures;
  }

  if (g_failures == 0) {
    std::printf("PASS db_manager_kernel_test\n");
    return 0;
  }
  std::printf("FAIL db_manager_kernel_test (%d failed)\n", g_failures);
  return 1;
}
