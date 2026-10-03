// Check harness for the structural-mass trigger (schema/structural_mass_trigger.sql).
// Same tiny check-macro style as controller_txn_test.cpp.
//
// argv[1] = libpq conninfo (dbname must start with hcp_test_), argv[2] = path to
// the install migration. REFUSES to run against anything but a throwaway
// database; run_mass_trigger_test.sh creates one from schema.sql. The test
// removes the trigger from that throwaway database, seeds a floor with plain
// SQL, then installs the migration over the populated floor.
#include <libpq-fe.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "codec.h"
#include "controller.h"

namespace {

int g_failures = 0;

void check(bool ok, const std::string &what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) {
    ++g_failures;
  }
}

codec::Address A(const std::string &token_id) {
  const auto a = codec::decode_token_id(token_id);
  if (!a.has_value()) {
    std::fprintf(stderr, "test bug: undecodable token_id '%s'\n", token_id.c_str());
    std::exit(2);
  }
  return *a;
}

void exec(PGconn *conn, const std::string &sql) {
  PGresult *res = PQexec(conn, sql.c_str());
  const auto st = PQresultStatus(res);
  if (st != PGRES_COMMAND_OK && st != PGRES_TUPLES_OK) {
    std::fprintf(stderr, "test setup failed: %s\n%s\n", PQerrorMessage(conn), sql.c_str());
    std::exit(2);
  }
  PQclear(res);
}

// Single-value query as text; "NULL" for SQL NULL, "ERR" on failure.
std::string value(PGconn *conn, const std::string &sql) {
  PGresult *res = PQexec(conn, sql.c_str());
  std::string out = "ERR";
  if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) == 1) {
    out = PQgetvalue(res, 0, 0);
    if (PQgetisnull(res, 0, 0)) {
      out = "NULL";
    }
  }
  PQclear(res);
  return out;
}

std::string mass_of(PGconn *conn, const std::string &couplets) {
  return value(conn, "SELECT mass FROM token WHERE token_id = '{" + couplets + "}'");
}

}  // namespace

int main(int argc, char **argv) {
  if (argc != 3) {
    std::fprintf(stderr, "usage: %s <conninfo with dbname=hcp_test_*> <migration.sql>\n", argv[0]);
    return 2;
  }
  const std::string conninfo = argv[1];
  if (conninfo.find("dbname=hcp_test_") == std::string::npos) {
    std::fprintf(stderr, "refusing: dbname must start with hcp_test_\n");
    return 2;
  }
  std::ifstream in(argv[2]);
  std::stringstream migration;
  migration << in.rdbuf();
  if (migration.str().empty()) {
    std::fprintf(stderr, "cannot read migration %s\n", argv[2]);
    return 2;
  }

  PGconn *raw = PQconnectdb(conninfo.c_str());
  if (PQstatus(raw) != CONNECTION_OK) {
    std::fprintf(stderr, "cannot connect: %s\n", PQerrorMessage(raw));
    return 2;
  }

  // (b) Seed a floor in a database WITHOUT the trigger (schema.sql installs
  // it; drop it here to model an existing database), then install the
  // migration over the populated floor.
  exec(raw, "DROP TRIGGER token_parent_structural_mass ON token_parent");
  exec(raw, "DROP FUNCTION token_parent_structural_mass()");
  exec(raw,
       "INSERT INTO token (token_id, mass) VALUES ('{N0}', 1), ('{N1}', 1), "
       "('{B0}', 2), ('{B1}', 2), ('{B2}', 2)");
  exec(raw,
       "INSERT INTO token_parent (token_id, ordinal, parent_token_id, mass) VALUES "
       "('{B0}', 0, '{N0}', 1), ('{B0}', 1, '{N1}', 1), "
       "('{B1}', 0, '{N1}', 1), ('{B1}', 1, '{N0}', 1), "
       "('{B2}', 0, '{N0}', 1), ('{B2}', 1, '{N0}', 1)");
  exec(raw, migration.str());
  check(mass_of(raw, "N0") == "1" && mass_of(raw, "N1") == "1",
        "install over seeded floor: nibble masses unchanged (1)");
  check(mass_of(raw, "B0") == "2" && mass_of(raw, "B1") == "2" && mass_of(raw, "B2") == "2",
        "install over seeded floor: byte-code masses unchanged (2)");
  check(value(raw, "SELECT count(*) FROM token_parent") == "6",
        "install over seeded floor: token_parent rows untouched");
  check(value(raw, "SELECT count(*) FROM pg_trigger WHERE tgname = 'token_parent_structural_mass'") == "1",
        "install: trigger present");

  dbk::Controller ctl(conninfo);

  // (a) Structural mass over the seeded byte codes (each mass 2).
  ctl.mint(A("C2"), "two-byte", {{A("B0"), std::nullopt}, {A("B1"), std::nullopt}});
  check(mass_of(raw, "C2") == "4", "2-byte character reads mass 4");
  ctl.mint(A("C3"), "three-byte",
           {{A("B0"), std::nullopt}, {A("B1"), std::nullopt}, {A("B2"), std::nullopt}});
  check(mass_of(raw, "C3") == "6", "3-byte character reads mass 6");
  ctl.mint(A("C4"), "four-byte",
           {{A("B0"), std::nullopt}, {A("B1"), std::nullopt}, {A("B2"), std::nullopt},
            {A("B0"), std::nullopt}});
  check(mass_of(raw, "C4") == "8", "4-byte character reads mass 8 (repeated parent counted per ordinal)");
  check(mass_of(raw, "B0") == "2" && mass_of(raw, "N0") == "1",
        "parents unchanged by minting a composite (no cascade)");

  // One level only: a composite over composites sums their stored masses.
  ctl.mint(A("D1"), "over-composites", {{A("C2"), std::nullopt}, {A("C3"), std::nullopt}});
  check(mass_of(raw, "D1") == "10", "composite over composites: sum of stored masses (4+6)");

  // A parentless token never fires the trigger: its supplied mass stands,
  // and a NULL stays NULL.
  ctl.mint(A("E0"), "atom", {}, 7);
  ctl.mint(A("E1"), "label", {});
  check(mass_of(raw, "E0") == "7" && mass_of(raw, "E1") == "NULL",
        "parentless tokens: supplied mass stands, NULL stays NULL");

  // (c) NULL-mass parent: standalone mint raises and leaves nothing.
  bool threw = false;
  try {
    ctl.mint(A("F0"), "over-null", {{A("E1"), std::nullopt}});
  } catch (const std::runtime_error &) {
    threw = true;
  }
  check(threw, "NULL-mass parent: mint raises");
  check(!ctl.token_exists(A("F0")), "NULL-mass parent: standalone mint writes nothing");

  // Inside a command transaction the whole command rolls back.
  const std::string tokens_before = value(raw, "SELECT count(*) FROM token");
  const std::string parents_before = value(raw, "SELECT count(*) FROM token_parent");
  const std::string children_before = value(raw, "SELECT count(*) FROM token_child");
  threw = false;
  try {
    ctl.with_transaction([&] {
      ctl.mint(A("G0"), "fine", {{A("B0"), std::nullopt}});
      ctl.mint(A("G1"), "over-null", {{A("E1"), std::nullopt}});
    });
  } catch (const std::runtime_error &) {
    threw = true;
  }
  check(threw, "command with NULL-mass parent: error propagates");
  check(!ctl.in_transaction(), "command with NULL-mass parent: scope closed");
  check(!ctl.token_exists(A("G0")) && !ctl.token_exists(A("G1")),
        "command with NULL-mass parent: earlier mint rolled back too");
  check(value(raw, "SELECT count(*) FROM token") == tokens_before &&
            value(raw, "SELECT count(*) FROM token_parent") == parents_before &&
            value(raw, "SELECT count(*) FROM token_child") == children_before,
        "command with NULL-mass parent: no rows written");

  PQfinish(raw);
  std::printf("%s controller_mass_trigger_test\n", g_failures == 0 ? "PASS" : "FAIL");
  return g_failures == 0 ? 0 : 1;
}
