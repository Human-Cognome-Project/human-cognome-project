// Standalone check harness for the WAL ingest step (WAL-IMPL-PLAN.md W-5).
// Same style as wal_book_test.cpp: a tiny check macro, one line per check,
// PASS/FAIL summary, non-zero exit on any failure.
//
// Runs directly against a real, DISPOSABLE `wal_manager` database --
// created if absent, reset (DROP SCHEMA public CASCADE; CREATE SCHEMA
// public;) and reapplied from wal_schema.sql on every run, exactly as
// wal_book_test.cpp does. This is the WAL manager's OWN database -- it is
// never `hcp3_core`, and this harness never touches `hcp3_core`.
#include <libpq-fe.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

#include "codec.h"
#include "wal_book.h"
#include "wal_ingest.h"
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

using codec::Address;
using wal::Obligation;
using wal::ObligationKind;
using wal::Report;
using wal::Scope;
using wal::WalBook;

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

// Raw single-value scalar query, for test-side verification of what
// record_seen() wrote to `history` -- same pattern as wal_book_test.cpp.
long long scalar_count(PGconn *conn, const std::string &sql) {
  PGresult *res = PQexec(conn, sql.c_str());
  if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
    PQclear(res);
    return -1;
  }
  const long long n = std::atoll(PQgetvalue(res, 0, 0));
  PQclear(res);
  return n;
}

void run_wal_ingest_checks(const std::string &conninfo, PGconn *raw) {
  WalBook book(conninfo);

  // --- forward-then-return in the SAME batch closes the obligation. ---
  {
    const Report forward = wal::fixture::declare("core", 1, Scope::kGlobal, A("AB"), {A("AA")});
    wal::ingest(book, forward);

    const Obligation structure_ob{ObligationKind::kStructure, A("AA"), A("AB")};
    const Obligation mass_ob{ObligationKind::kMass, A("AB"), codec::Address{}};
    check(book.is_open(structure_ob), "same-batch: DECLARE opens the structure obligation");
    check(book.is_open(mass_ob), "same-batch: DECLARE opens the mass obligation");

    const Report ret = wal::fixture::return_token_child("core", 2, Scope::kGlobal, A("AA"), A("AB"));
    wal::ingest(book, ret);
    check(!book.is_open(structure_ob),
          "same-batch: matching return_token_child closes the structure obligation");
    check(book.is_open(mass_ob),
          "same-batch: the return closes ONLY the structure obligation, not the mass one");

    const long long n = scalar_count(
        raw, "SELECT count(*) FROM history WHERE source = 'core' AND lsn = 2 "
             "AND settled_kind = 'structure'");
    check(n == 1, "same-batch: history records the return's settlement");

    book.close(mass_ob);  // cleanup for later checks
  }

  // --- forward-then-return in a LATER batch also closes (gap tolerance). ---
  {
    const Report forward = wal::fixture::membership_write("core", 10, Scope::kLocal, A("BA"), {A("BB")});
    wal::ingest(book, forward);
    const Obligation member_ob{ObligationKind::kMembership, A("BA"), A("BB")};
    check(book.is_open(member_ob), "later-batch: membership write opens its obligation");

    // A gap: an unrelated report lands in between, in a later "batch".
    const Report unrelated = wal::fixture::declare("core", 11, Scope::kGlobal, A("CA"), {A("CB")});
    wal::ingest(book, unrelated);
    check(book.is_open(member_ob),
          "later-batch: an intervening unrelated report does not disturb the open obligation");

    const Report ret = wal::fixture::return_member_of("core", 12, Scope::kLocal, A("BA"), A("BB"));
    wal::ingest(book, ret);
    check(!book.is_open(member_ob),
          "later-batch: the return, arriving after a gap, still closes the obligation");

    // cleanup of the unrelated fixture's obligations
    book.close(Obligation{ObligationKind::kStructure, A("CB"), A("CA")});
    book.close(Obligation{ObligationKind::kMass, A("CA"), codec::Address{}});
  }

  // --- a mass-fill closes a mass obligation. ---
  {
    const Report forward = wal::fixture::declare("core", 20, Scope::kGlobal, A("DA"), {A("DB")});
    wal::ingest(book, forward);
    const Obligation mass_ob{ObligationKind::kMass, A("DA"), codec::Address{}};
    check(book.is_open(mass_ob), "mass-fill: DECLARE opens the mass obligation");

    const Report fill = wal::fixture::mass_fill("core", 21, Scope::kGlobal, A("DA"), 7);
    wal::ingest(book, fill);
    check(!book.is_open(mass_ob), "mass-fill: mass_fill closes the mass obligation");

    const long long n = scalar_count(
        raw, "SELECT count(*) FROM history WHERE source = 'core' AND lsn = 21 "
             "AND settled_kind = 'mass'");
    check(n == 1, "mass-fill: history records the mass settlement");

    book.close(Obligation{ObligationKind::kStructure, A("DB"), A("DA")});  // cleanup
  }

  // --- an unmatched settling write is inert: History-recorded, closes
  //     nothing. ---
  {
    const Report ret = wal::fixture::return_token_child("core", 30, Scope::kGlobal, A("EA"), A("EB"));
    wal::ingest(book, ret);
    const Obligation structure_ob{ObligationKind::kStructure, A("EA"), A("EB")};
    check(!book.is_open(structure_ob),
          "unmatched: no obligation was ever open for this identity (sanity)");

    const long long n = scalar_count(
        raw, "SELECT count(*) FROM history WHERE source = 'core' AND lsn = 30 "
             "AND settled_kind IS NULL AND settled_addr_a IS NULL "
             "AND settled_addr_b IS NULL");
    check(n == 1,
          "unmatched: an unmatched settling write is still History-recorded, "
          "with no settlement (all settled_* NULL)");
  }

  // --- a DELETE opens nothing and settles nothing: History-only. ---
  {
    const Report del = wal::fixture::delete_token("core", 40, Scope::kLocal, A("FA"));
    wal::ingest(book, del);

    const auto all = book.list_open();
    for (const Obligation &o : all) {
      check(o.addr_a != A("FA") && o.addr_b != A("FA"),
            "delete: DELETE opened no obligation touching the removed token");
    }

    const long long n = scalar_count(
        raw, "SELECT count(*) FROM history WHERE source = 'core' AND lsn = 40 "
             "AND settled_kind IS NULL AND settled_addr_a IS NULL "
             "AND settled_addr_b IS NULL");
    check(n == 1, "delete: History-recorded with no settlement");
  }
}

}  // namespace

int main(int argc, char **argv) {
  const std::string schema_path = (argc > 1) ? argv[1] : "wal_schema.sql";
  const std::string conninfo = "dbname=wal_manager";

  {
    PGconn *maint = PQconnectdb("dbname=postgres");
    if (maint == nullptr || PQstatus(maint) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL wal_ingest_test: no local Postgres reachable: %s\n",
                   maint ? PQerrorMessage(maint) : "null connection");
      if (maint) PQfinish(maint);
      return 1;
    }
    PGresult *r = PQexec(maint, "SELECT 1 FROM pg_database WHERE datname = 'wal_manager'");
    const bool exists = PQresultStatus(r) == PGRES_TUPLES_OK && PQntuples(r) > 0;
    PQclear(r);
    if (!exists && !exec_bare(maint, "CREATE DATABASE wal_manager")) {
      std::fprintf(stderr, "FAIL wal_ingest_test: could not create wal_manager\n");
      PQfinish(maint);
      return 1;
    }
    PQfinish(maint);
  }

  PGconn *raw = PQconnectdb(conninfo.c_str());
  if (raw == nullptr || PQstatus(raw) != CONNECTION_OK) {
    std::fprintf(stderr, "FAIL wal_ingest_test: cannot connect to wal_manager: %s\n",
                 raw ? PQerrorMessage(raw) : "null connection");
    if (raw) PQfinish(raw);
    return 1;
  }
  {
    const bool reset = exec_bare(raw, "DROP SCHEMA public CASCADE; CREATE SCHEMA public;");
    const bool applied = reset && exec_bare(raw, read_file(schema_path));
    if (!applied) {
      std::fprintf(stderr, "FAIL wal_ingest_test: schema reset/apply failed\n");
      PQfinish(raw);
      return 1;
    }
  }

  try {
    run_wal_ingest_checks(conninfo, raw);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "FAIL wal_ingest_test: unexpected exception: %s\n", e.what());
    ++g_failures;
  }

  PQfinish(raw);

  if (g_failures == 0) {
    std::printf("PASS wal_ingest_test\n");
    return 0;
  }
  std::printf("FAIL wal_ingest_test (%d failed)\n", g_failures);
  return 1;
}
