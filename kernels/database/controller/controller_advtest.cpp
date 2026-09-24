// Adversarial validation harness for the hcp3_core write/mint controller.
//
// Complements controller_test.cpp. It targets the paths the base harness does
// not exercise, all against the real, disposable hcp3_core (local Postgres,
// peer auth). Like the base harness it resets hcp3_core (DROP SCHEMA public
// CASCADE + reapply ../schema/schema.sql) so it starts from a clean schema,
// then probes:
//
//   1. Multi-constituent FK rollback: a mint whose SECOND constituent is
//      missing must roll back the token row AND the first constituent's link,
//      leaving no orphan token_parent / token_child rows.
//   2. Connection recovery: after that failed mint, the same controller must
//      still be usable for a subsequent valid mint and reads.
//   3. token_text derivation: token.token_text is exactly the codec's
//      dot-joined rendering of token_id (verified by reading the column back
//      directly and comparing to codec::encode_token_id), never the caller's
//      notation.
//   4. Notation injection safety: a notation carrying SQL metacharacters is
//      stored verbatim as data; the tables survive intact (parameterized).
//   5. DISTINCT-reverse count: a composite with a parent repeated across
//      ordinals produces exactly one token_child row for that parent.
//   6. Idempotent re-mint preserves the original notation/token_text (the
//      re-mint's differing notation is ignored, nothing is rewritten).
//   7. add_membership: a write against a missing group rolls back BOTH
//      directions (no orphan members/member_of row); a re-add of an
//      existing pair is a clean idempotent no-op (ON CONFLICT DO NOTHING,
//      no throw, no duplicate row); and a one-sided/inconsistent pair
//      (only member_of OR only members pre-seeded) is healed on its
//      missing side without disturbing the side already present.
//   8. rekey is FK-safe end to end: the old token_id is fully gone from all
//      four relationship stores (direct row counts), the new token_id's
//      token_text is the codec dot-join (not carried over verbatim), and a
//      rekey that fails (new_id already exists) leaves everything untouched.
//   9. delete_pair leaves the non-reciprocal direction of an unrelated
//      membership edge alone, and delete_token is blocked by a live FK
//      reference (open seam G10: the database's default behaviour, not an
//      invented cascade).
//
// One ok/FAIL line per check; PASS/FAIL summary; non-zero exit on any failure.
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

// Render an address as the schema's text[] literal, mirroring the controller's
// private helper, so verification queries can key on token_id directly.
std::string pg_array(const Address &addr) {
  std::string out = "{";
  for (std::size_t i = 0; i < addr.size(); ++i) {
    if (i > 0) out.push_back(',');
    out.push_back('"');
    out.push_back(codec::kAlphabet[addr[i].first]);
    out.push_back(addr[i].partial ? codec::kPartialMarker
                                   : codec::kAlphabet[addr[i].second]);
    out.push_back('"');
  }
  out.push_back('}');
  return out;
}

// Single-value text query on a raw verification connection. Returns nullopt on
// zero rows / SQL NULL, otherwise the value as a string.
std::optional<std::string> scalar(PGconn *v, const std::string &sql) {
  PGresult *res = PQexec(v, sql.c_str());
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    std::fprintf(stderr, "verify query failed: %s\n  sql: %s\n",
                 PQresultErrorMessage(res), sql.c_str());
    PQclear(res);
    std::exit(2);
  }
  std::optional<std::string> out;
  if (PQntuples(res) > 0 && !PQgetisnull(res, 0, 0)) {
    out = std::string(PQgetvalue(res, 0, 0));
  }
  PQclear(res);
  return out;
}

void run_adv_checks(const std::string &conninfo) {
  dbk::Controller ctl(conninfo);
  // Independent read-only connection for direct table verification (columns
  // the controller exposes no getter for, e.g. token_text; and raw row counts).
  PGconn *v = PQconnectdb(conninfo.c_str());
  if (v == nullptr || PQstatus(v) != CONNECTION_OK) {
    std::fprintf(stderr, "FAIL controller_advtest: verify connection failed\n");
    std::exit(2);
  }

  const Address p1 = A("AA");
  const Address p2 = A("AB");
  ctl.mint(p1, "particle-1", {});
  ctl.mint(p2, "particle-2", {});

  // --- 1. Multi-constituent FK rollback: first constituent valid, second
  //        missing. The whole mint must roll back (no orphan link/child). ---
  const Address x = A("BA");
  const Address missing = A("DZ");  // never minted
  {
    bool threw = false;
    try {
      ctl.mint(x, "half-linked",
               {dbk::Constituent{p1, 1}, dbk::Constituent{missing, 1}});
    } catch (const std::exception &) {
      threw = true;
    }
    check(threw, "rollback: mint with a later missing constituent throws");
    check(!ctl.token_exists(x), "rollback: composite token row not created");
    check(ctl.parents_of(x).empty(), "rollback: no orphan token_parent rows");
    check(!contains(ctl.children_of(p1), x),
          "rollback: first (valid) constituent got no reverse edge");
    // Direct count: token_parent must hold zero rows for x.
    const auto n = scalar(
        v, "SELECT count(*) FROM token_parent WHERE token_id = '" + pg_array(x) +
               "'::text[]");
    check(n.has_value() && *n == "0", "rollback: token_parent count for composite is 0");
  }

  // --- 2. Connection recovery: the same controller works after the failure. ---
  {
    const Address y = A("BB");
    const bool existed =
        ctl.mint(y, "recovered", {dbk::Constituent{p1, 1}, dbk::Constituent{p2, 2}});
    check(!existed, "recovery: a valid mint after a failed mint succeeds");
    check(ctl.token_exists(y), "recovery: recovered token is present");
    check(contains(ctl.children_of(p1), y), "recovery: reverse edge wired after recovery");
  }

  // --- 3. token_text derivation == codec dot-join of token_id, not notation. ---
  {
    const Address c = A("BC");
    ctl.mint(c, "notation-not-text", {dbk::Constituent{p1, 1}});
    const auto text = scalar(v, "SELECT token_text FROM token WHERE token_id = '" +
                                    pg_array(c) + "'::text[]");
    const auto expected = codec::encode_token_id(c);
    check(text.has_value() && expected.has_value() && *text == *expected,
          "token_text equals codec dot-join of token_id");
    check(text.has_value() && *text == "BC", "token_text derived (BC), not the notation");
    const auto note = scalar(v, "SELECT notation FROM token WHERE token_id = '" +
                                    pg_array(c) + "'::text[]");
    check(note.has_value() && *note == "notation-not-text",
          "notation stored verbatim, distinct from token_text");
  }

  // --- 4. Notation injection safety: SQL metacharacters are inert data. ---
  {
    const Address inj = A("BD");
    const std::string evil = "'); DROP TABLE token; -- \\ \" weird";
    ctl.mint(inj, evil, {dbk::Constituent{p1, 1}});
    check(ctl.token_exists(inj), "injection: token minted with hostile notation");
    const auto still = scalar(v, "SELECT count(*) FROM token");
    check(still.has_value() && std::stoi(*still) >= 5,
          "injection: token table intact (not dropped)");
    const auto stored = scalar(v, "SELECT notation FROM token WHERE token_id = '" +
                                      pg_array(inj) + "'::text[]");
    check(stored.has_value() && *stored == evil,
          "injection: hostile notation round-trips verbatim");
  }

  // --- 5. DISTINCT-reverse: parent repeated across ordinals -> one child row. ---
  {
    const Address rep = A("BE");
    ctl.mint(rep, "repeat-parent",
             {dbk::Constituent{p1, 1}, dbk::Constituent{p1, 2},
              dbk::Constituent{p1, 3}});
    const auto parents = ctl.parents_of(rep);
    check(parents.size() == 3, "distinct-reverse: three token_parent rows kept");
    const auto childrows =
        scalar(v, "SELECT count(*) FROM token_child WHERE child_token_id = '" +
                      pg_array(rep) + "'::text[] AND token_id = '" + pg_array(p1) +
                      "'::text[]");
    check(childrows.has_value() && *childrows == "1",
          "distinct-reverse: exactly one token_child row for the repeated parent");
  }

  // --- 6. Idempotent re-mint does not rewrite notation/token_text. ---
  {
    const Address c = A("BC");  // minted in step 3 with "notation-not-text"
    const bool existed = ctl.mint(c, "DIFFERENT-notation", {dbk::Constituent{p2, 9}});
    check(existed, "idempotent: re-mint of existing token reports existed=true");
    const auto note = scalar(v, "SELECT notation FROM token WHERE token_id = '" +
                                    pg_array(c) + "'::text[]");
    check(note.has_value() && *note == "notation-not-text",
          "idempotent: re-mint left the original notation untouched");
    const auto pn = scalar(v, "SELECT count(*) FROM token_parent WHERE token_id = '" +
                                  pg_array(c) + "'::text[]");
    check(pn.has_value() && *pn == "1",
          "idempotent: re-mint added no token_parent rows");
  }

  // --- 7. add_membership: missing group -> no orphan row on either side;
  //        idempotent re-add is a clean no-op; a one-sided/inconsistent
  //        pair is healed on its missing side only. ---
  {
    const Address member = A("BF");
    const Address missing_group = A("DY");  // never minted
    ctl.mint(member, "member-only", {});
    bool threw = false;
    try {
      ctl.add_membership(member, missing_group);
    } catch (const std::exception &) {
      threw = true;
    }
    check(threw, "add_membership rollback: missing group throws (FK violation)");
    const auto mo = scalar(v, "SELECT count(*) FROM member_of WHERE token_id = '" +
                                   pg_array(member) + "'::text[]");
    check(mo.has_value() && *mo == "0",
          "add_membership rollback: no orphan member_of row");
    const auto mm = scalar(v, "SELECT count(*) FROM members WHERE member_token_id = '" +
                                   pg_array(member) + "'::text[]");
    check(mm.has_value() && *mm == "0",
          "add_membership rollback: no orphan members row (both sides rolled back)");

    // Idempotent re-add (ON CONFLICT DO NOTHING, SEE-style): calling
    // add_membership twice for the same pair must not throw and must not
    // leave a duplicate row on either side.
    const Address member2 = A("BN");
    const Address group2 = A("BP");
    ctl.mint(member2, "member-2", {});
    ctl.mint(group2, "group-2", {});
    ctl.add_membership(member2, group2);
    bool rethrew = false;
    try {
      ctl.add_membership(member2, group2);  // re-add, same pair
    } catch (const std::exception &) {
      rethrew = true;
    }
    check(!rethrew, "add_membership idempotent: re-adding an existing pair does not throw");
    const auto mo2 = scalar(v, "SELECT count(*) FROM member_of WHERE token_id = '" +
                                    pg_array(member2) + "'::text[] AND group_token_id = '" +
                                    pg_array(group2) + "'::text[]");
    check(mo2.has_value() && *mo2 == "1",
          "add_membership idempotent: exactly one member_of row after re-add");
    const auto mm2 = scalar(v, "SELECT count(*) FROM members WHERE token_id = '" +
                                    pg_array(group2) + "'::text[] AND member_token_id = '" +
                                    pg_array(member2) + "'::text[]");
    check(mm2.has_value() && *mm2 == "1",
          "add_membership idempotent: exactly one members row after re-add");

    // One-sided healing: pre-seed only member_of directly (bypassing the
    // controller), leaving its members reciprocal absent. add_membership
    // must fill in exactly the missing side, in one call, without
    // duplicating (or erroring on) the side already present.
    const Address member3 = A("BQ");
    const Address group3 = A("BR");
    ctl.mint(member3, "member-3", {});
    ctl.mint(group3, "group-3", {});
    const bool preseeded = exec_bare(
        v, "INSERT INTO member_of (token_id, group_token_id) VALUES ('" +
               pg_array(member3) + "'::text[], '" + pg_array(group3) + "'::text[])");
    check(preseeded, "add_membership heal setup: pre-seeded a one-sided member_of row");
    bool heal_threw = false;
    try {
      ctl.add_membership(member3, group3);
    } catch (const std::exception &) {
      heal_threw = true;
    }
    check(!heal_threw, "add_membership heal: does not throw on a pre-existing one-sided pair");
    check(contains(ctl.members_of(group3), member3),
          "add_membership heal: fills in the missing members reciprocal");
    const auto mo3 = scalar(v, "SELECT count(*) FROM member_of WHERE token_id = '" +
                                    pg_array(member3) + "'::text[] AND group_token_id = '" +
                                    pg_array(group3) + "'::text[]");
    check(mo3.has_value() && *mo3 == "1",
          "add_membership heal: the pre-existing member_of side stays at exactly one row");
  }

  // --- 8. rekey end to end: old_id fully gone from every store; new_id's
  //        token_text is codec-derived, not carried over; a failed rekey
  //        (new_id collision) leaves both tokens and all rows untouched. ---
  {
    const Address base = A("BG");
    const Address dep = A("BH");
    ctl.mint(base, "rekey-base", {});
    ctl.mint(dep, "rekey-dep", {dbk::Constituent{base, 3}});
    const Address grp = A("BI");
    ctl.mint(grp, "rekey-group", {});
    ctl.add_membership(base, grp);

    const Address renamed = A("BJ");
    ctl.rekey(base, renamed);

    // Old token_id: zero rows anywhere, direct count (not just controller
    // reads, which only prove reachability from the new id).
    const auto old_token =
        scalar(v, "SELECT count(*) FROM token WHERE token_id = '" + pg_array(base) + "'::text[]");
    check(old_token.has_value() && *old_token == "0", "rekey: old token row gone");
    const auto old_parent = scalar(
        v, "SELECT count(*) FROM token_parent WHERE token_id = '" + pg_array(base) +
               "'::text[] OR parent_token_id = '" + pg_array(base) + "'::text[]");
    check(old_parent.has_value() && *old_parent == "0",
          "rekey: old_id gone from token_parent (both columns)");
    const auto old_child = scalar(
        v, "SELECT count(*) FROM token_child WHERE token_id = '" + pg_array(base) +
               "'::text[] OR child_token_id = '" + pg_array(base) + "'::text[]");
    check(old_child.has_value() && *old_child == "0",
          "rekey: old_id gone from token_child (both columns)");
    const auto old_members = scalar(
        v, "SELECT count(*) FROM members WHERE token_id = '" + pg_array(base) +
               "'::text[] OR member_token_id = '" + pg_array(base) + "'::text[]");
    check(old_members.has_value() && *old_members == "0",
          "rekey: old_id gone from members (both columns)");
    const auto old_member_of = scalar(
        v, "SELECT count(*) FROM member_of WHERE token_id = '" + pg_array(base) +
               "'::text[] OR group_token_id = '" + pg_array(base) + "'::text[]");
    check(old_member_of.has_value() && *old_member_of == "0",
          "rekey: old_id gone from member_of (both columns)");

    // New token_text is the codec dot-join of new_id, not a copy of the old
    // token_text.
    const auto new_text = scalar(v, "SELECT token_text FROM token WHERE token_id = '" +
                                         pg_array(renamed) + "'::text[]");
    const auto expected_text = codec::encode_token_id(renamed);
    check(new_text.has_value() && expected_text.has_value() && *new_text == *expected_text,
          "rekey: new token_text is the codec dot-join of new_id");
    check(new_text.has_value() && *new_text != "BG",
          "rekey: new token_text is not the old address's rendering");

    // dep's link survives, repointed to renamed.
    check(contains(ctl.children_of(renamed), dep),
          "rekey: dependent composite's reverse edge repointed to renamed");

    // A rekey onto an existing token_id (dep) throws and touches nothing:
    // renamed and dep both survive unchanged, no partial repoint leaks.
    bool collide_threw = false;
    try {
      ctl.rekey(renamed, dep);
    } catch (const std::exception &) {
      collide_threw = true;
    }
    check(collide_threw, "rekey collision: rekeying onto an existing new_id throws");
    check(ctl.token_exists(renamed) && ctl.token_exists(dep),
          "rekey collision: both tokens survive untouched");
    check(contains(ctl.children_of(renamed), dep),
          "rekey collision: dep's structural link to renamed is undisturbed");
  }

  // --- 9. delete_pair / delete_token: specific-pair only; FK-blocked
  //        delete leaves the DB's default (no-cascade) behaviour standing,
  //        no invented cascade (open seam G10). ---
  {
    const Address g = A("BK");
    const Address m1 = A("BL");
    const Address m2 = A("BM");
    ctl.mint(g, "delete-group", {});
    ctl.mint(m1, "delete-member-1", {});
    ctl.mint(m2, "delete-member-2", {});
    ctl.add_membership(m1, g);
    ctl.add_membership(m2, g);

    ctl.delete_pair(m1, g);
    check(!contains(ctl.members_of(g), m1) && contains(ctl.members_of(g), m2),
          "delete_pair: removes exactly the named pair, leaves the other member");
    check(ctl.member_of(m1).empty(), "delete_pair: m1's reciprocal side is gone too");

    // delete_token on a token still referenced (m2 -> g via members/member_of,
    // and g itself is a live FK target) must throw, not silently cascade.
    bool blocked = false;
    try {
      ctl.delete_token(g);
    } catch (const std::exception &) {
      blocked = true;
    }
    check(blocked, "delete_token: FK-referenced token is blocked, not force-deleted");
    check(ctl.token_exists(g), "delete_token: blocked delete leaves the token intact");

    // Once the last reference is removed, delete_token succeeds.
    ctl.delete_pair(m2, g);
    ctl.delete_token(g);
    check(!ctl.token_exists(g), "delete_token: succeeds once no row references the token");
  }

  PQfinish(v);
}

}  // namespace

int main(int argc, char **argv) {
  const std::string schema_path = (argc > 1) ? argv[1] : "../schema/schema.sql";
  const std::string conninfo = "dbname=hcp3_core";

  {
    PGconn *maint = PQconnectdb("dbname=postgres");
    if (maint == nullptr || PQstatus(maint) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL controller_advtest: no local Postgres: %s\n",
                   maint ? PQerrorMessage(maint) : "null connection");
      if (maint) PQfinish(maint);
      return 1;
    }
    PGresult *r = PQexec(maint, "SELECT 1 FROM pg_database WHERE datname = 'hcp3_core'");
    const bool exists = PQresultStatus(r) == PGRES_TUPLES_OK && PQntuples(r) > 0;
    PQclear(r);
    if (!exists && !exec_bare(maint, "CREATE DATABASE hcp3_core")) {
      std::fprintf(stderr, "FAIL controller_advtest: could not create hcp3_core\n");
      PQfinish(maint);
      return 1;
    }
    PQfinish(maint);
  }

  {
    PGconn *db = PQconnectdb(conninfo.c_str());
    if (db == nullptr || PQstatus(db) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL controller_advtest: cannot connect hcp3_core: %s\n",
                   db ? PQerrorMessage(db) : "null connection");
      if (db) PQfinish(db);
      return 1;
    }
    const bool reset = exec_bare(db, "DROP SCHEMA public CASCADE; CREATE SCHEMA public;");
    const bool applied = reset && exec_bare(db, read_file(schema_path));
    PQfinish(db);
    if (!applied) {
      std::fprintf(stderr, "FAIL controller_advtest: schema reset/apply failed\n");
      return 1;
    }
  }

  try {
    run_adv_checks(conninfo);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "FAIL controller_advtest: unexpected exception: %s\n", e.what());
    ++g_failures;
  }

  if (g_failures == 0) {
    std::printf("PASS controller_advtest\n");
    return 0;
  }
  std::printf("FAIL controller_advtest (%d failed)\n", g_failures);
  return 1;
}
