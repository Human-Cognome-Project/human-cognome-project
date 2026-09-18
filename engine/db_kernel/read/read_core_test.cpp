// Standalone check harness for the READ core (raw radial). Same style as
// controller/controller_test.cpp: a tiny check macro, one line per check,
// PASS/FAIL summary, non-zero exit on any failure.
//
// Runs directly against the real hcp3_core database, disposable and
// overwritten on every run (reset via ../schema/schema.sql, same as
// controller_test.cpp -- this harness does not invent a second schema
// path). If no local Postgres is reachable it prints a clear message and
// exits non-zero rather than claiming a pass.
//
// Fixture (built once via the controller, mirroring the READ core's own
// only-follow view of it):
//
//   atoms   p1=AA, p2=AB, p3=AC          (no parents)
//   c1=BA   parents [p1, p2]             (structure)
//   c2=CB   parents [p1, p3]             (structure; SHARES p1 with c1)
//   top=DA  parents [c1, c2]             (structure, one level up)
//   grp=EA  members [c1, c2]             (membership, via add_membership)
//
// This gives: a structure-down chain (top -> c1/c2 -> atoms), a
// structure-up reverse edge (c1/c2 -> top), a membership pair
// (grp <-> c1, c2), and a genuinely shared node (p1, reachable from top
// through BOTH c1 and c2) to pin the no-dedup return.
#include <libpq-fe.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "codec.h"
#include "command_ir.h"
#include "controller.h"
#include "read_core.h"

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
using dbread::ReachedVia;
using dbread::ReadNode;
using dbread::ReadResult;
using dbread::ReadStatus;

Address A(const std::string &token_id) {
  const auto a = codec::decode_token_id(token_id);
  if (!a.has_value()) {
    std::fprintf(stderr, "test bug: undecodable token_id '%s'\n", token_id.c_str());
    std::exit(2);
  }
  return *a;
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

// Builds a ReadRecord with just anchor/direction/depth/exclusions set --
// cache_shaped is irrelevant to this core (raw-radial regardless).
command::ReadRecord make_read(const Address &anchor,
                               std::optional<command::ReadAxis> axis, bool reverse,
                               unsigned depth,
                               std::vector<Address> exclusions = {}) {
  command::ReadRecord op;
  op.anchor = anchor;
  op.direction.axis = axis;
  op.direction.reverse = reverse;
  op.depth = depth;
  op.exclusions = std::move(exclusions);
  return op;
}

// Exact-sequence comparison: order, depth and via all matter (no dedup,
// no reordering) -- this is the whole point of what is under test.
bool same_sequence(const std::vector<ReadNode> &actual,
                    const std::vector<std::pair<Address, unsigned>> &expected_addr_depth,
                    const std::vector<ReachedVia> &expected_via) {
  if (actual.size() != expected_addr_depth.size() ||
      actual.size() != expected_via.size()) {
    return false;
  }
  for (std::size_t i = 0; i < actual.size(); ++i) {
    if (actual[i].address != expected_addr_depth[i].first) return false;
    if (actual[i].depth != expected_addr_depth[i].second) return false;
    if (actual[i].via != expected_via[i]) return false;
  }
  return true;
}

std::size_t count_address(const std::vector<ReadNode> &nodes, const Address &addr) {
  std::size_t n = 0;
  for (const ReadNode &node : nodes) {
    if (node.address == addr) ++n;
  }
  return n;
}

void run_read_core_checks(const std::string &conninfo) {
  dbk::Controller ctl(conninfo);

  const Address p1 = A("AA");
  const Address p2 = A("AB");
  const Address p3 = A("AC");
  const Address c1 = A("BA");
  const Address c2 = A("CB");
  const Address top = A("DA");
  const Address grp = A("EA");

  ctl.mint(p1, "p1", {});
  ctl.mint(p2, "p2", {});
  ctl.mint(p3, "p3", {});
  ctl.mint(c1, "c1", {dbk::Constituent{p1, std::nullopt}, dbk::Constituent{p2, std::nullopt}});
  ctl.mint(c2, "c2", {dbk::Constituent{p1, std::nullopt}, dbk::Constituent{p3, std::nullopt}});
  ctl.mint(top, "top", {dbk::Constituent{c1, std::nullopt}, dbk::Constituent{c2, std::nullopt}});
  ctl.mint(grp, "grp", {});
  ctl.add_membership(c1, grp);
  ctl.add_membership(c2, grp);

  // --- Depth 0: the rolled-up node only, regardless of direction. ---
  {
    const ReadResult r = dbread::read(ctl, make_read(c1, std::nullopt, false, 0));
    check(r.status == ReadStatus::kOk, "depth0: status ok");
    const bool exact = r.nodes.size() == 1 && r.nodes[0].address == c1 &&
                        r.nodes[0].depth == 0 && r.nodes[0].via == ReachedVia::kAnchor;
    check(exact, "depth0: rolled-up node only, nothing expanded (no members/parents in scope)");
  }

  // --- Radial (default direction), depth 1 from c1: all four follows. ---
  // parents_of(c1) = [p1, p2]; children_of(c1) = [top]; members_of(c1) = [];
  // member_of(c1) = [grp]. Fixed follow order: parent, child, members,
  // member_of.
  {
    const ReadResult r = dbread::read(ctl, make_read(c1, std::nullopt, false, 1));
    check(r.status == ReadStatus::kOk, "radial depth1: status ok");
    const bool exact = same_sequence(
        r.nodes,
        {{c1, 0}, {p1, 1}, {p2, 1}, {top, 1}, {grp, 1}},
        {ReachedVia::kAnchor, ReachedVia::kParent, ReachedVia::kParent,
         ReachedVia::kChild, ReachedVia::kMemberOf});
    check(exact, "radial depth1: anchor + structure-down (p1,p2) + structure-up "
                 "(top) + membership-up (grp), in fixed follow order");
  }

  // --- Structure-axis-only, forward: top's constituents (c1, c2). ---
  {
    const ReadResult r =
        dbread::read(ctl, make_read(top, command::ReadAxis::kParents, false, 1));
    check(r.status == ReadStatus::kOk, "structure-only forward: status ok");
    const bool exact =
        same_sequence(r.nodes, {{top, 0}, {c1, 1}, {c2, 1}},
                      {ReachedVia::kAnchor, ReachedVia::kParent, ReachedVia::kParent});
    check(exact, "structure-only forward: parents_of(top) = [c1, c2], no "
                 "membership axis touched");
  }

  // --- Membership-axis-only, forward: grp's members (c1, c2). ---
  {
    const ReadResult r =
        dbread::read(ctl, make_read(grp, command::ReadAxis::kMembers, false, 1));
    check(r.status == ReadStatus::kOk, "membership-only forward: status ok");
    const bool exact =
        same_sequence(r.nodes, {{grp, 0}, {c1, 1}, {c2, 1}},
                      {ReachedVia::kAnchor, ReachedVia::kMembers, ReachedVia::kMembers});
    check(exact, "membership-only forward: members_of(grp) = [c1, c2], no "
                 "structure axis touched");
  }

  // --- Reverse orientation on the structure axis: c1's children (top). ---
  {
    const ReadResult r =
        dbread::read(ctl, make_read(c1, command::ReadAxis::kParents, true, 1));
    check(r.status == ReadStatus::kOk, "structure reverse: status ok");
    const bool exact = same_sequence(r.nodes, {{c1, 0}, {top, 1}},
                                      {ReachedVia::kAnchor, ReachedVia::kChild});
    check(exact, "structure reverse (children_of): c1 -> top only, not "
                 "parents_of(c1)");
  }

  // --- Reverse orientation on the membership axis: c1's groups (grp). ---
  {
    const ReadResult r =
        dbread::read(ctl, make_read(c1, command::ReadAxis::kMembers, true, 1));
    check(r.status == ReadStatus::kOk, "membership reverse: status ok");
    const bool exact = same_sequence(r.nodes, {{c1, 0}, {grp, 1}},
                                      {ReachedVia::kAnchor, ReachedVia::kMemberOf});
    check(exact, "membership reverse (member_of): c1 -> grp only, not "
                 "members_of(c1)");
  }

  // --- Depth (LoD): structure-only from top, depth 1 vs depth 2. ---
  {
    const ReadResult shallow =
        dbread::read(ctl, make_read(top, command::ReadAxis::kParents, false, 1));
    check(shallow.nodes.size() == 3, "depth1 from top: 3 nodes (top, c1, c2)");

    const ReadResult deep =
        dbread::read(ctl, make_read(top, command::ReadAxis::kParents, false, 2));
    // top(d0), c1(d1), p1(d2 via c1), p2(d2 via c1), c2(d1), p1(d2 via c2),
    // p3(d2 via c2) -- p1 appears TWICE, once per path.
    const bool exact = same_sequence(
        deep.nodes,
        {{top, 0}, {c1, 1}, {p1, 2}, {p2, 2}, {c2, 1}, {p1, 2}, {p3, 2}},
        {ReachedVia::kAnchor, ReachedVia::kParent, ReachedVia::kParent,
         ReachedVia::kParent, ReachedVia::kParent, ReachedVia::kParent,
         ReachedVia::kParent});
    check(exact, "depth2 from top: expands one more level per branch, LoD "
                 "dial widens what is in scope");
  }

  // --- The no-dedup / once-per-path assertion, isolated: p1 is shared by
  //     both c1 and c2, so a depth-2 structure read from top must return
  //     it TWICE, at its two distinct positions, never collapsed. ---
  {
    const ReadResult r =
        dbread::read(ctl, make_read(top, command::ReadAxis::kParents, false, 2));
    check(count_address(r.nodes, p1) == 2,
          "no-dedup: shared node p1 (reachable via c1 AND c2) appears twice, "
          "not collapsed to one");
    // Both occurrences are legitimately at depth 2, reached via the
    // structure axis, at different positions in the array -- repetition
    // AND position both carry structural information, per spec.
    int seen = 0;
    for (std::size_t i = 0; i < r.nodes.size(); ++i) {
      if (r.nodes[i].address == p1) {
        check(r.nodes[i].depth == 2 && r.nodes[i].via == ReachedVia::kParent,
              "no-dedup: each p1 occurrence individually correct (depth 2, "
              "via structure)");
        ++seen;
      }
    }
    check(seen == 2, "no-dedup: exactly two p1 occurrences located");
  }

  // --- Exclusions: a specific token_id prunes its whole branch. ---
  {
    const ReadResult r = dbread::read(
        ctl, make_read(top, command::ReadAxis::kParents, false, 2, {c2}));
    const bool exact =
        same_sequence(r.nodes, {{top, 0}, {c1, 1}, {p1, 2}, {p2, 2}},
                      {ReachedVia::kAnchor, ReachedVia::kParent, ReachedVia::kParent,
                       ReachedVia::kParent});
    check(exact, "exclusions: naming c2 prunes c2 AND everything beneath it "
                 "(p1-via-c2, p3) -- p1 now appears only once, via c1");
  }

  // --- Exclusions: a terminal-wildcard tree region. c2's address (CB)
  //     is the only fixture token whose leading couplet character is
  //     'C', so a "C*" wildcard exclusion prunes exactly c2's branch --
  //     the same result as the specific-id exclusion above. ---
  {
    const Address wildcard_c = A("C*");
    const ReadResult r = dbread::read(
        ctl, make_read(top, command::ReadAxis::kParents, false, 2, {wildcard_c}));
    const bool exact =
        same_sequence(r.nodes, {{top, 0}, {c1, 1}, {p1, 2}, {p2, 2}},
                      {ReachedVia::kAnchor, ReachedVia::kParent, ReachedVia::kParent,
                       ReachedVia::kParent});
    check(exact, "exclusions: a terminal-wildcard tree region (\"C*\") prunes "
                 "the same branch as the specific-id exclusion, via pure "
                 "prefix comparison (no store scan)");
  }

  // --- A nonexistent anchor: well-formed request, nothing to read. ---
  {
    const ReadResult r = dbread::read(ctl, make_read(A("ZZ"), std::nullopt, false, 3));
    check(r.status == ReadStatus::kOk && r.nodes.empty(),
          "nonexistent anchor: ok status, empty result");
  }

  // --- A wildcard anchor: resolved via gather() to the existing tokens
  //     under the prefix, each then read radially at depth 1, in gather's
  //     PK/address order. "A*" gathers the three atoms p1=AA, p2=AB,
  //     p3=AC (AA < AB < AC in COLLATE "C" order). Radially at depth 1,
  //     each atom's only active follow is children_of (its reverse
  //     structure edge): children_of(p1) = [c1, c2] (both composites use
  //     p1); children_of(p2) = [c1]; children_of(p3) = [c2]. c1 therefore
  //     recurs -- once under p1's own read, again under p2's -- a shared
  //     node reached via two DIFFERENT gathered anchors, not collapsed,
  //     exactly as a shared node via two paths under one anchor is not
  //     collapsed elsewhere in this file. ---
  {
    const ReadResult r = dbread::read(ctl, make_read(A("A*"), std::nullopt, false, 1));
    check(r.status == ReadStatus::kOk, "wildcard anchor: status ok, not deferred");
    const bool exact = same_sequence(
        r.nodes,
        {{p1, 0}, {c1, 1}, {c2, 1}, {p2, 0}, {c1, 1}, {p3, 0}, {c2, 1}},
        {ReachedVia::kAnchor, ReachedVia::kChild, ReachedVia::kChild,
         ReachedVia::kAnchor, ReachedVia::kChild, ReachedVia::kAnchor,
         ReachedVia::kChild});
    check(exact, "wildcard anchor: gathers the A* trunk (p1, p2, p3) and "
                 "concatenates each member's own radial read in gather order; "
                 "c1 recurs across p1's and p2's reads, not deduplicated");
  }

  // --- A wildcard anchor over an empty region: well-formed, no nodes --
  //     NOT the removed deferred status. No fixture token starts with
  //     'Z'. ---
  {
    const ReadResult r = dbread::read(ctl, make_read(A("Z*"), std::nullopt, false, 2));
    check(r.status == ReadStatus::kOk,
          "wildcard anchor, empty region: status ok (not a deferred/error status)");
    check(r.nodes.empty(), "wildcard anchor, empty region: no nodes produced");
  }
}

}  // namespace

int main(int argc, char **argv) {
  const std::string schema_path = (argc > 1) ? argv[1] : "../schema/schema.sql";
  const std::string conninfo = "dbname=hcp3_core";

  {
    PGconn *maint = PQconnectdb("dbname=postgres");
    if (maint == nullptr || PQstatus(maint) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL read_core_test: no local Postgres reachable: %s\n",
                   maint ? PQerrorMessage(maint) : "null connection");
      if (maint) PQfinish(maint);
      return 1;
    }
    PGresult *r = PQexec(maint, "SELECT 1 FROM pg_database WHERE datname = 'hcp3_core'");
    const bool exists = PQresultStatus(r) == PGRES_TUPLES_OK && PQntuples(r) > 0;
    PQclear(r);
    if (!exists && !exec_bare(maint, "CREATE DATABASE hcp3_core")) {
      std::fprintf(stderr, "FAIL read_core_test: could not create hcp3_core\n");
      PQfinish(maint);
      return 1;
    }
    PQfinish(maint);
  }

  {
    PGconn *db = PQconnectdb(conninfo.c_str());
    if (db == nullptr || PQstatus(db) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL read_core_test: cannot connect to hcp3_core: %s\n",
                   db ? PQerrorMessage(db) : "null connection");
      if (db) PQfinish(db);
      return 1;
    }
    const bool reset = exec_bare(db, "DROP SCHEMA public CASCADE; CREATE SCHEMA public;");
    const bool applied = reset && exec_bare(db, read_file(schema_path));
    PQfinish(db);
    if (!applied) {
      std::fprintf(stderr, "FAIL read_core_test: schema reset/apply failed\n");
      return 1;
    }
  }

  try {
    run_read_core_checks(conninfo);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "FAIL read_core_test: unexpected exception: %s\n", e.what());
    ++g_failures;
  }

  if (g_failures == 0) {
    std::printf("PASS read_core_test\n");
    return 0;
  }
  std::printf("FAIL read_core_test (%d failed)\n", g_failures);
  return 1;
}
