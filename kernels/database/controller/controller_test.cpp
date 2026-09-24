// Standalone check harness for the hcp3_core write/mint controller. Same
// style as codec/codec_test.cpp: a tiny check macro, one line per check,
// PASS/FAIL summary, non-zero exit on any failure.
//
// Runs directly against the real hcp3_core database, which is disposable and
// may be overwritten any number of times. It resets hcp3_core by dropping the
// public schema and reapplying ../schema/schema.sql, then exercises the
// controller (see/mint/link/wire, only-follow reads, idempotent re-mint,
// fold-and-wire consistency, membership reads/writes, the mutation
// primitives, FK rollback), and confirms the 2026-09-17 rebase landed
// (`token.type` and `token_sibling_group` are gone).
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
#include "controller.h"

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

// An address from its dot-joined token_id string (via the codec).
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

// Run a bare (unparameterized) statement; returns true on success.
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

// The actual controller exercise. Records via check().
void run_controller_checks(const std::string &conninfo) {
  dbk::Controller ctl(conninfo);

  // --- Schema-shape checks: the 2026-09-17 rebase landed. ---
  {
    PGconn *raw = PQconnectdb(conninfo.c_str());
    if (raw == nullptr || PQstatus(raw) != CONNECTION_OK) {
      std::fprintf(stderr, "schema-shape check: cannot open verify connection\n");
      std::exit(2);
    }
    PGresult *r1 = PQexec(raw,
        "SELECT 1 FROM information_schema.tables WHERE table_schema='public' "
        "AND table_name='token_sibling_group'");
    check(PQresultStatus(r1) == PGRES_TUPLES_OK && PQntuples(r1) == 0,
          "schema: token_sibling_group no longer exists");
    PQclear(r1);
    PGresult *r2 = PQexec(raw,
        "SELECT 1 FROM information_schema.columns WHERE table_schema='public' "
        "AND table_name='token' AND column_name='type'");
    check(PQresultStatus(r2) == PGRES_TUPLES_OK && PQntuples(r2) == 0,
          "schema: token.type column no longer exists");
    PQclear(r2);
    PGresult *r3 = PQexec(raw,
        "SELECT 1 FROM information_schema.tables WHERE table_schema='public' "
        "AND table_name IN ('members','member_of')");
    check(PQresultStatus(r3) == PGRES_TUPLES_OK && PQntuples(r3) == 2,
          "schema: members and member_of both exist");
    PQclear(r3);
    PQfinish(raw);
  }

  // --- Base particles: no constituents. ---
  const Address p1 = A("AA");
  const Address p2 = A("AB");
  const Address p3 = A("AC");
  const Address grp = A("CA");

  check(!ctl.token_exists(p1), "token absent before mint");

  ctl.mint(p1, "particle-1", {});
  ctl.mint(p2, "particle-2", {});
  ctl.mint(p3, "particle-3", {});
  ctl.mint(grp, "group-token", {});

  check(ctl.token_exists(p1), "SEE/MINT: base token present after mint");
  check(ctl.parents_of(p1).empty(), "only-follow: base token has no parents");
  check(ctl.children_of(p1).empty(), "only-follow: base token has no children yet");
  check(ctl.members_of(p1).empty(), "only-follow: base token has no members yet");
  check(ctl.member_of(p1).empty(), "only-follow: base token joins no groups yet");

  // --- Composite c1 = [BA] from p1(mass 1), p2(mass 2). ---
  const Address c1 = A("BA");
  ctl.mint(c1, "composite-1", {dbk::Constituent{p1, 1}, dbk::Constituent{p2, 2}});

  {
    const auto parents = ctl.parents_of(c1);
    check(parents.size() == 2, "LINK: c1 has two parent rows");
    const bool ordered = parents.size() == 2 && parents[0].ordinal == 0 &&
                         parents[0].parent == p1 && parents[0].mass == 1 &&
                         parents[1].ordinal == 1 && parents[1].parent == p2 &&
                         parents[1].mass == 2;
    check(ordered, "LINK: c1 constituents ordered 0-based with per-element mass");
  }
  check(contains(ctl.children_of(p1), c1), "WIRE: p1 points back at c1");
  check(contains(ctl.children_of(p2), c1), "WIRE: p2 points back at c1");

  // --- Composite c2 = [BB] from p1(mass 5), p3(mass 1), p1(mass 2). ---
  // p1 appears twice: two token_parent rows, ONE token_child reverse edge.
  const Address c2 = A("BB");
  ctl.mint(c2, "composite-2",
           {dbk::Constituent{p1, 5}, dbk::Constituent{p3, 1},
            dbk::Constituent{p1, 2}});

  {
    const auto parents = ctl.parents_of(c2);
    check(parents.size() == 3, "LINK: c2 keeps a row per ordinal (repeat parent kept)");
    const bool ok = parents.size() == 3 && parents[0].parent == p1 &&
                    parents[0].mass == 5 && parents[1].parent == p3 &&
                    parents[1].mass == 1 && parents[2].parent == p1 &&
                    parents[2].mass == 2;
    check(ok, "LINK: c2 ordinals/masses recorded in order, duplicate parent allowed");
  }
  {
    // p1 is now a parent of both c1 and c2; the repeat within c2 is deduped.
    const auto kids = ctl.children_of(p1);
    check(kids.size() == 2 && contains(kids, c1) && contains(kids, c2),
          "WIRE: repeated constituent yields a single reverse edge (deduped)");
    const auto p3kids = ctl.children_of(p3);
    check(p3kids.size() == 1 && contains(p3kids, c2), "WIRE: p3 points back at c2 only");
  }

  // --- fold-and-wire consistency: token_child is the exact reverse of the
  //     distinct parents in token_parent, for every composite. ---
  {
    bool consistent = true;
    for (const Address &comp : {c1, c2}) {
      std::vector<Address> distinct;
      for (const auto &pe : ctl.parents_of(comp)) {
        if (!contains(distinct, pe.parent)) distinct.push_back(pe.parent);
      }
      for (const Address &par : distinct) {
        if (!contains(ctl.children_of(par), comp)) consistent = false;
      }
    }
    check(consistent, "fold-and-wire: every distinct parent lists the composite as a child");
  }

  // --- Idempotent re-mint: SEE finds c1, nothing changes. ---
  {
    const auto before = ctl.parents_of(c1);
    const bool existed = ctl.mint(c1, "composite-1-again", {dbk::Constituent{p3, 99}});
    check(existed, "idempotent re-mint reports existed = true");
    const auto after = ctl.parents_of(c1);
    check(after.size() == before.size() && after.size() == 2,
          "idempotent re-mint leaves token_parent unchanged (no duplicate links)");
    check(!contains(ctl.children_of(p3), c1),
          "idempotent re-mint wires nothing (p3 not linked to c1)");
  }

  // --- Membership: add_membership writes both directions; members_of /
  //     member_of follow each side, deterministically ordered. ---
  ctl.add_membership(p1, grp);
  ctl.add_membership(p2, grp);
  {
    const auto g1 = ctl.member_of(p1);
    check(g1.size() == 1 && g1[0] == grp, "member_of: p1 joins grp (upward)");
    const auto g2 = ctl.member_of(p2);
    check(g2.size() == 1 && g2[0] == grp, "member_of: p2 joins grp (upward)");
    const auto mem = ctl.members_of(grp);
    check(mem.size() == 2 && mem[0] == p1 && mem[1] == p2,
          "members_of: grp contains p1, p2 (downward), sorted deterministically");
  }
  {
    // A token with no memberships reads back empty on both axes.
    check(ctl.members_of(p3).empty(), "members_of: token with no members is empty");
    check(ctl.member_of(p3).empty(), "member_of: token joining nothing is empty");
  }

  // --- add_membership is idempotent (SEE-style): re-adding an existing
  //     pair is a clean no-op, no throw, no duplicate rows. ---
  {
    bool threw = false;
    try {
      ctl.add_membership(p1, grp);  // p1/grp already joined above
    } catch (const std::exception &) {
      threw = true;
    }
    check(!threw, "add_membership: re-adding an existing pair does not throw");
    const auto g1_again = ctl.member_of(p1);
    check(g1_again.size() == 1 && g1_again[0] == grp,
          "add_membership: re-add leaves member_of at exactly one row");
    const auto mem_again = ctl.members_of(grp);
    check(mem_again.size() == 2 && mem_again[0] == p1 && mem_again[1] == p2,
          "add_membership: re-add leaves members unchanged (no duplicate)");
  }

  // --- Optional token attributes: notation/mass pass-through, no `type`. ---
  {
    // Base particles (step above) were minted without mass -> NULL.
    const auto a1 = ctl.attributes_of(p1);
    check(a1.has_value() && !a1->mass.has_value(),
          "attributes_of: absent mass round-trips as NULL (nullopt)");

    // A fresh token minted WITH mass=0 reads back verbatim.
    const Address t = A("CB");
    ctl.mint(t, "typed-token", {}, 0);
    const auto at = ctl.attributes_of(t);
    check(at.has_value() && at->mass.has_value() && *at->mass == 0,
          "attributes_of: stored mass=0 read back verbatim (not confused with NULL)");

    // A composite with a nonzero mass — still pure pass-through, nothing computed.
    const Address lbl = A("CC");
    ctl.mint(lbl, "label-token", {}, 42);
    const auto al = ctl.attributes_of(lbl);
    check(al.has_value() && al->mass.has_value() && *al->mass == 42,
          "attributes_of: stored mass=42 read back verbatim");

    // A token that does not exist -> nullopt (distinct from present-but-NULL).
    check(!ctl.attributes_of(A("DZ")).has_value(),
          "attributes_of: absent token returns nullopt");
  }

  // --- Mint writes NULL mass, both whole-token and per-constituent, when
  //     the caller supplies none (never a stand-in 0). ---
  {
    const Address blank = A("CD");
    ctl.mint(blank, "blank-mass", {dbk::Constituent{p1, std::nullopt}});
    const auto attrs = ctl.attributes_of(blank);
    check(attrs.has_value() && !attrs->mass.has_value(),
          "mint: absent whole-token mass stored as NULL, not 0");
    const auto parents = ctl.parents_of(blank);
    check(parents.size() == 1 && !parents[0].mass.has_value(),
          "mint: absent per-constituent mass stored as NULL, not 0");
  }

  // --- rekey: cascade-repoints token_parent, token_child, members,
  //     member_of; re-derives token_text; leaves no dangling references. ---
  {
    // c1 (BA) has parents p1,p2 (structure), and is not yet a member of
    // anything; give it a membership edge too so rekey exercises all four
    // stores.
    ctl.add_membership(c1, grp);
    const Address moved = A("ZZ");
    ctl.rekey(c1, moved);

    check(!ctl.token_exists(c1), "rekey: old_id no longer exists");
    check(ctl.token_exists(moved), "rekey: new_id now exists");

    const auto attrs = ctl.attributes_of(moved);
    check(attrs.has_value(), "rekey: new_id carries the token row forward");

    // Structure: moved's own token_parent row is unaffected in content, and
    // its parents (p1, p2) now list `moved`, not c1, as a child.
    const auto moved_parents = ctl.parents_of(moved);
    check(moved_parents.size() == 2 && moved_parents[0].parent == p1 &&
              moved_parents[1].parent == p2,
          "rekey: moved token's own constituents are unchanged");
    check(contains(ctl.children_of(p1), moved) && !contains(ctl.children_of(p1), c1),
          "rekey: p1's child reference repointed from c1 to moved");
    check(contains(ctl.children_of(p2), moved) && !contains(ctl.children_of(p2), c1),
          "rekey: p2's child reference repointed from c1 to moved");

    // Membership: moved is now the member of grp that c1 used to be, both
    // directions.
    check(contains(ctl.members_of(grp), moved) && !contains(ctl.members_of(grp), c1),
          "rekey: members_of(grp) repointed from c1 to moved");
    const auto moved_groups = ctl.member_of(moved);
    check(contains(moved_groups, grp), "rekey: member_of(moved) includes grp");

    // c2 lists p1 twice as a constituent; rekey p1 and confirm c2's ordered
    // constituent list is fully repointed (both ordinals).
    const Address p1_moved = A("ZY");
    ctl.rekey(p1, p1_moved);
    const auto c2_parents = ctl.parents_of(c2);
    check(c2_parents.size() == 3 && c2_parents[0].parent == p1_moved &&
              c2_parents[2].parent == p1_moved,
          "rekey: repeated constituent repointed at every ordinal");
    check(!ctl.token_exists(p1), "rekey: p1 (old) no longer exists after its own rekey");
  }

  // --- rekey error paths: missing old_id; new_id already occupied. ---
  {
    bool threw = false;
    try {
      ctl.rekey(A("DZ"), A("EA"));  // DZ never minted
    } catch (const std::exception &) {
      threw = true;
    }
    check(threw, "rekey: nonexistent old_id throws");

    threw = false;
    try {
      ctl.rekey(p3, grp);  // grp already exists -> would merge identities
    } catch (const std::exception &) {
      threw = true;
    }
    check(threw, "rekey: rekeying onto an existing new_id throws (no merge)");
    check(ctl.token_exists(p3), "rekey: failed rekey leaves old_id intact");
  }

  // --- delete_pair: removes one membership edge and its reciprocal only. ---
  {
    ctl.add_membership(p3, grp);
    check(contains(ctl.members_of(grp), p3), "delete_pair setup: p3 is a member of grp");
    ctl.delete_pair(p3, grp);
    check(!contains(ctl.members_of(grp), p3),
          "delete_pair: members_of(grp) no longer lists p3");
    check(!contains(ctl.member_of(p3), grp),
          "delete_pair: member_of(p3) no longer lists grp (reciprocal removed too)");
    // The group token itself, and its other members, are untouched.
    check(ctl.token_exists(grp), "delete_pair: the group token itself is untouched");
    check(!ctl.members_of(grp).empty(),
          "delete_pair: grp still has its other members (deletion is specific-pair only)");
  }

  // --- delete_token: removes an unreferenced token outright. ---
  {
    const Address lonely = A("ZX");
    ctl.mint(lonely, "lonely", {});
    check(ctl.token_exists(lonely), "delete_token setup: lonely token minted");
    ctl.delete_token(lonely);
    check(!ctl.token_exists(lonely), "delete_token: token removed");
  }

  // --- gather: the terminal-wildcard / range enumerator (firmed
  //     2026-09-18). Fresh, unused trunk (E) so results are exact and
  //     unaffected by tokens minted earlier in this run. ---
  {
    const Address e_a = A("EA");
    const Address e_z = A("EZ");         // uppercase-second couplet
    const Address e_b = A("Eb");         // lowercase-second couplet
    const Address e_c_deep = A("EC.AA"); // a deeper address nested under trunk E
    const Address f_a = A("FA");         // sibling trunk — must never appear
    ctl.mint(e_a, "e-a", {});
    ctl.mint(e_z, "e-z", {});
    ctl.mint(e_b, "e-b", {});
    ctl.mint(e_c_deep, "e-c-deep", {});
    ctl.mint(f_a, "f-a", {});

    // Prefix form: gather("E*") == every token under trunk E, PK order,
    // including the deeper EC.AA address (a trunk grows deeper — it is not
    // bounded to one couplet), and correctly ordering EZ before Eb (the
    // exact case the pre-collation-fix PK order got wrong: 'Z' < 'b' in
    // byte order but NOT in the DB's prior case-interleaved default).
    const Address e_star = A("E*");
    const auto e_gathered = ctl.gather(e_star);
    const std::vector<Address> e_expected = {e_a, e_c_deep, e_z, e_b};
    check(e_gathered == e_expected,
          "gather(prefix): trunk E returns exactly its 4 members, in byte "
          "order (EA, EC.AA, EZ, Eb — EZ before Eb, deeper address included)");
    check(!contains(e_gathered, f_a),
          "gather(prefix): a sibling trunk (F) is excluded from trunk E's gather");

    // Non-existent prefix: an entirely untouched trunk gathers empty.
    const auto g_gathered = ctl.gather(A("G*"));
    check(g_gathered.empty(), "gather(prefix): a trunk with no members returns empty");

    // Range form: gather(from, to) == the inclusive [from, to] interval,
    // picking up the same deeper address between two direct-couplet bounds.
    const auto range_all = ctl.gather(e_a, e_b);
    check(range_all == e_expected,
          "gather(range): [EA, Eb] inclusive returns the same 4 members in "
          "the same PK order as the prefix form");
    const auto range_narrow = ctl.gather(A("EB"), A("ED"));
    const std::vector<Address> narrow_expected = {e_c_deep};
    check(range_narrow == narrow_expected,
          "gather(range): [EB, ED] picks up only the deeper EC.AA address");
    const auto range_point = ctl.gather(e_a, e_a);
    const std::vector<Address> point_expected = {e_a};
    check(range_point == point_expected,
          "gather(range): a single-point [EA, EA] range is inclusive of EA itself");

    // Empty region: a range over an untouched trunk gathers empty.
    const auto range_empty = ctl.gather(A("GA"), A("GZ"));
    check(range_empty.empty(), "gather(range): an empty region returns empty");

    // Rejection: a non-terminal (inline) wildcard is not a valid codec
    // address at all, so the codec itself refuses to decode it — confirming
    // gather has no path to accept one.
    check(!codec::decode_token_id("E*.AA").has_value(),
          "gather precondition: an inline (non-terminal) wildcard does not "
          "even decode as a valid address (codec-level guarantee)");

    // Rejection: gather(range) refuses a wildcard endpoint.
    {
      bool threw = false;
      try {
        ctl.gather(e_star, e_z);
      } catch (const std::exception &) {
        threw = true;
      }
      check(threw, "gather(range): a partial/wildcard endpoint is rejected");
    }

    // Rejection: gather(prefix) refuses a non-wildcard address.
    {
      bool threw = false;
      try {
        ctl.gather(e_a);
      } catch (const std::exception &) {
        threw = true;
      }
      check(threw, "gather(prefix): a fully-specified (non-wildcard) address is rejected");
    }
  }

  // --- gather(prefix): the CARRY branch. The wildcard's fixed first
  //     character is the alphabet MAXIMUM ('z', index 49), so the
  //     exclusive high bound cannot just bump that position — it must
  //     carry into the fixed leading elements (base-50, second-char-
  //     fastest, same as increment_full_address). Trunk here is "HA.z*":
  //     leading element HA, wildcard element z*. High must come out as
  //     HB (HA's own successor), one element shorter than low. ---
  {
    const Address low_exact = A("HA.zA");     // the trunk's own low bound
    const Address near_top = A("HA.zZ");      // still first-char 'z', within trunk
    const Address deep = A("HA.zA.AB");       // nested deeper under the trunk
    const Address excl_below = A("HA.YA");    // same leading elem, below the z couplet
    const Address excl_above = A("HB.AA");    // next leading elem — past the carry
    ctl.mint(low_exact, "carry-low", {});
    ctl.mint(near_top, "carry-near-top", {});
    ctl.mint(deep, "carry-deep", {});
    ctl.mint(excl_below, "carry-excl-below", {});
    ctl.mint(excl_above, "carry-excl-above", {});

    const Address wildcard = A("HA.z*");
    const auto gathered = ctl.gather(wildcard);
    const std::vector<Address> expected = {low_exact, deep, near_top};
    check(gathered == expected,
          "gather(prefix) carry branch: HA.z* returns exactly {HA.zA, "
          "HA.zA.AB, HA.zZ} in byte order (high bound carried into the "
          "leading element HA -> HB)");
    check(!contains(gathered, excl_below),
          "gather(prefix) carry branch: excludes HA.YA (below the z couplet, "
          "same leading element)");
    check(!contains(gathered, excl_above),
          "gather(prefix) carry branch: excludes HB.AA (past the carried "
          "high bound)");
  }

  // --- gather(prefix): the OPEN-ENDED branch. The wildcard has no leading
  //     elements at all and its first character is already the alphabet
  //     maximum ('z*' alone) — there is no trunk above it, so the high
  //     bound is nullopt and the query is a lower-bound-only scan. Result
  //     correctness is asserted directly (the Index-Cond/planner point for
  //     the bounded branches is already settled; this branch is inherently
  //     half-open on a real address space, not a fallback). ---
  {
    const Address top_low = A("zA");      // the trunk's own low bound
    const Address top_max = A("zz");      // the absolute maximum single-couplet address
    const Address top_deep = A("zC.AB");  // nested deeper under the open-ended trunk
    const Address excl_sibling = A("yz"); // the sibling trunk just below 'z' — excluded
    ctl.mint(top_low, "open-low", {});
    ctl.mint(top_max, "open-max", {});
    ctl.mint(top_deep, "open-deep", {});
    ctl.mint(excl_sibling, "open-excl-sibling", {});

    const Address wildcard = A("z*");
    const auto gathered = ctl.gather(wildcard);
    const std::vector<Address> expected = {top_low, top_deep, top_max};
    check(gathered == expected,
          "gather(prefix) open-ended branch: z* returns exactly {zA, zC.AB, "
          "zz} in byte order, with no upper bound needed");
    check(!contains(gathered, excl_sibling),
          "gather(prefix) open-ended branch: excludes yz (the sibling trunk "
          "just below z, correctly outside the low bound)");
  }

  // --- Error path: mint referencing a missing constituent rolls back. ---
  {
    const Address bad = A("DA");
    const Address missing = A("DZ");  // never minted
    bool threw = false;
    try {
      ctl.mint(bad, "bad-composite", {dbk::Constituent{missing, 1}});
    } catch (const std::exception &) {
      threw = true;
    }
    check(threw, "mint with a missing constituent throws (FK violation)");
    check(!ctl.token_exists(bad), "failed mint rolled back: token row not created");
  }
}

}  // namespace

int main(int argc, char **argv) {
  // Schema path: default relative to controller/, override via argv[1].
  const std::string schema_path = (argc > 1) ? argv[1] : "../schema/schema.sql";
  const std::string conninfo = "dbname=hcp3_core";

  // Ensure hcp3_core exists. It is the real, disposable target store; created
  // once if absent (CREATE DATABASE cannot run over a connection to itself),
  // never dropped. Peer auth over the local unix socket needs no password.
  {
    PGconn *maint = PQconnectdb("dbname=postgres");
    if (maint == nullptr || PQstatus(maint) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL controller_test: no local Postgres reachable: %s\n",
                   maint ? PQerrorMessage(maint) : "null connection");
      if (maint) PQfinish(maint);
      return 1;
    }
    PGresult *r = PQexec(maint, "SELECT 1 FROM pg_database WHERE datname = 'hcp3_core'");
    const bool exists = PQresultStatus(r) == PGRES_TUPLES_OK && PQntuples(r) > 0;
    PQclear(r);
    if (!exists && !exec_bare(maint, "CREATE DATABASE hcp3_core")) {
      std::fprintf(stderr, "FAIL controller_test: could not create hcp3_core\n");
      PQfinish(maint);
      return 1;
    }
    PQfinish(maint);
  }

  // Reset hcp3_core and (re)apply the schema on a direct connection. DDL is
  // the coordinator's job, not a controller graph write.
  {
    PGconn *db = PQconnectdb(conninfo.c_str());
    if (db == nullptr || PQstatus(db) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL controller_test: cannot connect to hcp3_core: %s\n",
                   db ? PQerrorMessage(db) : "null connection");
      if (db) PQfinish(db);
      return 1;
    }
    const bool reset = exec_bare(db, "DROP SCHEMA public CASCADE; CREATE SCHEMA public;");
    const bool applied = reset && exec_bare(db, read_file(schema_path));
    PQfinish(db);
    if (!applied) {
      std::fprintf(stderr, "FAIL controller_test: schema reset/apply failed\n");
      return 1;
    }
  }

  try {
    run_controller_checks(conninfo);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "FAIL controller_test: unexpected exception: %s\n", e.what());
    ++g_failures;
  }

  if (g_failures == 0) {
    std::printf("PASS controller_test\n");
    return 0;
  }
  std::printf("FAIL controller_test (%d failed)\n", g_failures);
  return 1;
}
