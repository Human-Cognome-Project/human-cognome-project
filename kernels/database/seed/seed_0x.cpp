// Scratch/seed driver -- NOT one of the record-tier cores, and it modifies
// none of their sources. It seeds the SEED FLOOR into the real hcp3_core
// store and reads it back to verify: the 16 hex atoms (mass 1) plus `0x`
// (mass 0/undefined) -- "the ONLY declared masses" (NOTES.md "Mass -- why
// none is declared"; PLAN.md I.B "Mass"). Every other composite mass above
// this floor is manager-built/derived, never declared.
//
// This starts clean: it resets hcp3_core ONCE (DROP SCHEMA public CASCADE;
// CREATE SCHEMA public;) and reapplies ../schema/schema.sql. Subsequent
// base entries will NOT reset -- they accumulate on this base.
//
// All writes go through dbk::Controller::mint -- the ONLY declared-mass
// path (its optional `mass` parameter is a bootstrap channel, not the
// DECLARE verb; see NOTES.md "Mass"). This driver never mints through
// declare::execute() or dispatch:: -- those build on TOP of a store that
// already has a floor to ground constituent references against, which is
// precisely this seed's job. The controller's read API
// (token_exists/parents_of/children_of/members_of/member_of/
// attributes_of) does not surface token_text or table row counts, so for
// those verification-only read-backs this driver opens its own libpq
// connection and issues direct SELECTs. It never WRITES except through the
// controller door.
#include <libpq-fe.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "codec.h"
#include "controller.h"
#include "span_planner.h"

namespace {

int g_failures = 0;

void check(bool ok, const std::string &what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) ++g_failures;
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

// A one-column scalar from a single-row query (verification reads only).
std::string scalar(PGconn *conn, const std::string &sql) {
  PGresult *res = PQexec(conn, sql.c_str());
  if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
    std::fprintf(stderr, "scalar query failed: %s\n  sql: %s\n",
                 PQresultErrorMessage(res), sql.c_str());
    PQclear(res);
    std::exit(2);
  }
  std::string out = PQgetisnull(res, 0, 0) ? std::string("<null>") : PQgetvalue(res, 0, 0);
  PQclear(res);
  return out;
}

long count_rows(PGconn *conn, const std::string &table) {
  return std::stol(scalar(conn, "SELECT count(*) FROM " + table));
}

}  // namespace

int main(int argc, char **argv) {
  const std::string schema_path = (argc > 1) ? argv[1] : "../schema/schema.sql";
  const std::string conninfo = "dbname=hcp3_core";

  // Ensure hcp3_core exists (created once if absent; peer auth, no password).
  {
    PGconn *maint = PQconnectdb("dbname=postgres");
    if (maint == nullptr || PQstatus(maint) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL seed_0x: no local Postgres reachable: %s\n",
                   maint ? PQerrorMessage(maint) : "null connection");
      if (maint) PQfinish(maint);
      return 1;
    }
    PGresult *r = PQexec(maint, "SELECT 1 FROM pg_database WHERE datname = 'hcp3_core'");
    const bool exists = PQresultStatus(r) == PGRES_TUPLES_OK && PQntuples(r) > 0;
    PQclear(r);
    if (!exists && !exec_bare(maint, "CREATE DATABASE hcp3_core")) {
      std::fprintf(stderr, "FAIL seed_0x: could not create hcp3_core\n");
      PQfinish(maint);
      return 1;
    }
    PQfinish(maint);
  }

  // FIRST base entry: reset once so the base begins empty, then apply schema.
  {
    PGconn *db = PQconnectdb(conninfo.c_str());
    if (db == nullptr || PQstatus(db) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL seed_0x: cannot connect to hcp3_core: %s\n",
                   db ? PQerrorMessage(db) : "null connection");
      if (db) PQfinish(db);
      return 1;
    }
    const bool reset = exec_bare(db, "DROP SCHEMA public CASCADE; CREATE SCHEMA public;");
    const bool applied = reset && exec_bare(db, read_file(schema_path));
    PQfinish(db);
    if (!applied) {
      std::fprintf(stderr, "FAIL seed_0x: schema reset/apply failed\n");
      return 1;
    }
  }

  // `0x` lands first, at the A trunk's own first slot (NOTES.md "Per-kind
  // trunk allocation": "single hex codes + 0x -- the A trunk
  // (AA.AA.AA.AA.A*)"). The 16 hex atoms follow it sequentially in the
  // same trunk, via the radix-64 successor helper (command/span_planner.h)
  // -- the analyst-supplied-start sequential fill NOTES.md "Analyst
  // command semantics" describes, applied here by hand for the one
  // bootstrap statement that has no analyst above it yet.
  const codec::Address addr_0x = *codec::decode_token_id("AA.AA.AA.AA.AA");
  std::vector<codec::Address> hex_addrs;
  {
    codec::Address cursor = addr_0x;
    for (int i = 0; i < 16; ++i) {
      const auto next = command::successor(cursor);
      if (!next.has_value()) {
        std::fprintf(stderr, "FAIL seed_0x: address space exhausted seeding hex atom %d\n", i);
        return 1;
      }
      hex_addrs.push_back(*next);
      cursor = *next;
    }
  }
  static const char kHexDigits[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                       '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

  // ---- Seed the floor directly through the controller (the ONLY
  //      declared-mass path) and read it back only-follow. ----
  try {
    dbk::Controller ctl(conninfo);

    // `0x` -- mass 0/undefined (declared, not derived: there is nothing
    // for the manager to compute at the floor, and this is the one
    // explicit exception to "mass none declared" -- PLAN.md I.B).
    const bool zerox_existed = ctl.mint(addr_0x, "0x", {}, 0);
    check(!zerox_existed, "0x freshly minted (fresh schema, not a SEE no-op)");
    check(ctl.token_exists(addr_0x), "token_exists(0x) = true");
    check(ctl.parents_of(addr_0x).empty(), "0x: parents_of empty (no constituents)");
    check(ctl.children_of(addr_0x).empty(), "0x: children_of empty (nothing points at it)");
    check(ctl.members_of(addr_0x).empty(), "0x: members_of empty (not a grouping)");
    check(ctl.member_of(addr_0x).empty(), "0x: member_of empty (no memberships)");
    const auto zerox_attrs = ctl.attributes_of(addr_0x);
    check(zerox_attrs.has_value() && zerox_attrs->mass.has_value() && *zerox_attrs->mass == 0,
          "0x: stored mass == 0 (declared, the seed-floor exception)");

    // The 16 hex atoms -- mass 1 (declared; the only other seed-floor
    // exception). Each is a bare base identity: no parents, no members.
    bool all_hex_minted = true;
    bool all_hex_mass_one = true;
    for (int i = 0; i < 16; ++i) {
      const bool existed = ctl.mint(hex_addrs[i], std::string(1, kHexDigits[i]), {}, 1);
      all_hex_minted = all_hex_minted && !existed && ctl.token_exists(hex_addrs[i]);
      const auto attrs = ctl.attributes_of(hex_addrs[i]);
      all_hex_mass_one =
          all_hex_mass_one && attrs.has_value() && attrs->mass.has_value() && *attrs->mass == 1;
    }
    check(all_hex_minted, "16 hex atoms freshly minted and present");
    check(all_hex_mass_one, "16 hex atoms: stored mass == 1 (declared, the seed-floor mass)");

    std::printf(
        "\nseed floor: 0x=%s mass=0, 16 hex atoms %s..%s mass=1\n",
        codec::encode_token_id(addr_0x)->c_str(), codec::encode_token_id(hex_addrs.front())->c_str(),
        codec::encode_token_id(hex_addrs.back())->c_str());
  } catch (const std::exception &e) {
    std::fprintf(stderr, "FAIL seed_0x: exception during seed/read-back: %s\n", e.what());
    return 1;
  }

  // ---- Verification-only direct reads: token_text, notation, row counts. ----
  {
    PGconn *db = PQconnectdb(conninfo.c_str());
    if (db == nullptr || PQstatus(db) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL seed_0x: cannot reconnect for verification: %s\n",
                   db ? PQerrorMessage(db) : "null connection");
      if (db) PQfinish(db);
      return 1;
    }

    const std::string text =
        scalar(db, "SELECT token_text FROM token WHERE token_id = '{AA,AA,AA,AA,AA}'::text[]");
    const std::string notation =
        scalar(db, "SELECT notation   FROM token WHERE token_id = '{AA,AA,AA,AA,AA}'::text[]");
    check(text == "AA.AA.AA.AA.AA", "token_text == \"AA.AA.AA.AA.AA\" (controller-derived dot-join)");
    check(notation == "0x", "notation == \"0x\"");

    const long n_token = count_rows(db, "token");
    const long n_parent = count_rows(db, "token_parent");
    const long n_child = count_rows(db, "token_child");
    const long n_members = count_rows(db, "members");
    const long n_member_of = count_rows(db, "member_of");

    std::printf(
        "\nrow counts: token=%ld token_parent=%ld token_child=%ld members=%ld member_of=%ld\n",
        n_token, n_parent, n_child, n_members, n_member_of);

    check(n_token == 17, "exactly 17 rows in token (0x + 16 hex atoms)");
    check(n_parent == 0, "0 rows in token_parent (the floor has no constituents)");
    check(n_child == 0, "0 rows in token_child (nothing composes onto the floor yet)");
    check(n_members == 0, "0 rows in members (no groupings seeded)");
    check(n_member_of == 0, "0 rows in member_of (no memberships seeded)");

    PQfinish(db);
  }

  if (g_failures == 0) {
    std::printf("\nPASS seed_0x -- seed floor landed and read back clean\n");
    return 0;
  }
  std::printf("\nFAIL seed_0x (%d failed)\n", g_failures);
  return 1;
}
