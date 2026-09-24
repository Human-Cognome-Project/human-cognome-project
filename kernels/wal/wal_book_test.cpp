// Standalone check harness for the WAL bookkeeping door (WAL-IMPL-PLAN.md
// W-4). Same style as controller/controller_test.cpp and
// read/read_core_test.cpp: a tiny check macro, one line per check,
// PASS/FAIL summary, non-zero exit on any failure.
//
// Runs directly against a real, DISPOSABLE `wal_manager` database --
// created if absent, reset (DROP SCHEMA public CASCADE; CREATE SCHEMA
// public;) and reapplied from wal_schema.sql on every run, exactly as
// read_core_test.cpp does for `hcp3_core`. This is the WAL manager's OWN
// database -- it is never `hcp3_core`, and this harness never touches
// `hcp3_core`. If no local Postgres is reachable it prints a clear message
// and exits non-zero rather than claiming a pass.
#include <libpq-fe.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "codec.h"
#include "wal_book.h"
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
// record_seen() wrote to `history` -- WalBook exposes no history reader
// (not part of the W-4 door's spec), so the test reads it directly.
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

// The EXPLAIN output, one line per plan row, concatenated -- used to prove
// (or disprove) index-vs-scan access shape.
std::string explain_plan(PGconn *conn, const std::string &sql) {
  PGresult *res = PQexec(conn, ("EXPLAIN " + sql).c_str());
  std::string out;
  if (PQresultStatus(res) == PGRES_TUPLES_OK) {
    const int n = PQntuples(res);
    for (int i = 0; i < n; ++i) {
      out += PQgetvalue(res, i, 0);
      out += '\n';
    }
  } else {
    std::fprintf(stderr, "EXPLAIN failed: %s\n  sql: %s\n", PQresultErrorMessage(res),
                 sql.c_str());
  }
  PQclear(res);
  return out;
}

bool contains(const std::string &haystack, const std::string &needle) {
  return haystack.find(needle) != std::string::npos;
}

void run_wal_book_checks(const std::string &conninfo, PGconn *raw) {
  WalBook book(conninfo);

  // --- open -> close round trip leaves zero rows for that identity. ---
  {
    const Obligation ob{ObligationKind::kStructure, A("AA"), A("BA")};
    check(!book.is_open(ob), "sanity: fresh identity is not open before open()");
    book.open(ob, "core", 1);
    check(book.is_open(ob), "open: identity is open after open()");
    book.close(ob);
    check(!book.is_open(ob), "close: identity no longer open after close()");
    const auto listing = book.list_open(ObligationKind::kStructure, A("AA"));
    check(listing.empty(),
          "close: zero rows remain under (kind, addr_a) for that identity");
  }

  // --- double-open is idempotent: no duplicate-key error, one row. ---
  {
    const Obligation ob{ObligationKind::kStructure, A("AB"), A("BB")};
    book.open(ob, "core", 2);
    book.open(ob, "core", 2);  // must not throw
    check(book.is_open(ob), "double-open: identity is open after two opens");
    const auto listing = book.list_open(ObligationKind::kStructure, A("AB"));
    check(listing.size() == 1, "double-open: exactly one row, not two");
    book.close(ob);
    check(!book.is_open(ob), "double-open: cleans up via a single close()");
  }

  // --- mass obligation: addr_b is the empty-address sentinel. ---
  {
    const Obligation mass_ob{ObligationKind::kMass, A("CC"), codec::Address{}};
    book.open(mass_ob, "core", 3);
    check(book.is_open(mass_ob), "mass: obligation with empty addr_b sentinel is open");
    const auto listing = book.list_open(ObligationKind::kMass, A("CC"));
    check(listing.size() == 1 && listing[0].addr_b.empty(),
          "mass: round-trips the empty addr_b sentinel through list_open");
    book.close(mass_ob);
    check(!book.is_open(mass_ob), "mass: closes cleanly like any other kind");
  }

  // --- close of an absent identity is a no-op, AND the report that named
  //     it is still recorded in History by record_seen. ---
  {
    const Obligation absent{ObligationKind::kMembership, A("CB"), A("DA")};
    check(!book.is_open(absent), "sanity: identity was never opened");
    book.close(absent);  // must not throw
    check(!book.is_open(absent), "close-of-absent: still not open (clean no-op)");

    const Report r = wal::fixture::return_member_of("core", 42, Scope::kLocal,
                                                     A("CB"), A("DA"));
    book.record_seen(r, absent);
    const long long n = scalar_count(
        raw, "SELECT count(*) FROM history WHERE source = 'core' AND lsn = 42 "
             "AND settled_kind = 'membership'");
    check(n == 1,
          "close-of-absent: record_seen still History-records the report "
          "and its named (but never-open) settled identity");
  }

  // --- record_seen with no settlement: settled_* all NULL together. ---
  {
    const Report r = wal::fixture::declare("core", 50, Scope::kGlobal, A("EA"), {A("AA")});
    book.record_seen(r);  // settled omitted -> opened work, settled nothing
    const long long n = scalar_count(
        raw, "SELECT count(*) FROM history WHERE source = 'core' AND lsn = 50 "
             "AND settled_kind IS NULL AND settled_addr_a IS NULL "
             "AND settled_addr_b IS NULL AND scope = 'global'");
    check(n == 1, "record_seen: no settlement records all-NULL settled_* together");
  }

  // --- History append-only: (source, lsn) is unique; a second append on
  //     the same key is rejected, not merged. ---
  {
    const Report r = wal::fixture::mass_fill("core", 99, Scope::kLocal, A("FA"), 7);
    book.record_seen(r, std::nullopt);
    bool threw = false;
    try {
      book.record_seen(r, std::nullopt);
    } catch (const std::exception &) {
      threw = true;
    }
    check(threw,
          "history append-only: a second record_seen on the same (source, lsn) "
          "is rejected");
    const long long n =
        scalar_count(raw, "SELECT count(*) FROM history WHERE source = 'core' AND lsn = 99");
    check(n == 1, "history append-only: exactly one row survives, not two");
  }

  // --- list_open: unfiltered reads the whole relation. ---
  {
    const Obligation ob1{ObligationKind::kStructure, A("GA"), A("GB")};
    const Obligation ob2{ObligationKind::kMembership, A("HA"), A("HB")};
    book.open(ob1, "core", 4);
    book.open(ob2, "core", 5);
    const auto all = book.list_open();
    bool has1 = false;
    bool has2 = false;
    for (const Obligation &o : all) {
      if (o == ob1) has1 = true;
      if (o == ob2) has2 = true;
    }
    check(has1 && has2, "list_open: unfiltered read sees obligations of every kind");
    book.close(ob1);
    book.close(ob2);
  }

  // --- list_open: addr_a without kind is a caller error (not a scan). ---
  {
    bool threw = false;
    try {
      book.list_open(std::nullopt, A("AA"));
    } catch (const std::invalid_argument &) {
      threw = true;
    }
    check(threw,
          "list_open: addr_a without kind is refused (not a PK-leading prefix)");
  }

  // --- Adversarial: EXPLAIN shows PK / PK-prefix access for is_open /
  //     list_open-shaped queries, and a source-filtered query -- which
  //     this door never offers -- is a seq scan (proves no such path
  //     exists to offer). Bulk-load filler rows first so the planner's
  //     cost model, which prefers a seq scan on a tiny table regardless of
  //     available indexes, has a real reason to choose the index. ---
  {
    check(exec_bare(raw,
                    "INSERT INTO obligation (kind, addr_a, addr_b, source, lsn) "
                    "SELECT 'structure', "
                    "       ARRAY[chr(65 + (i % 26)) || chr(65 + ((i / 26) % 26))], "
                    "       ARRAY['Z','Z'], 'filler', i "
                    "FROM generate_series(1, 3000) AS i "
                    "ON CONFLICT DO NOTHING"),
          "adversarial setup: bulk filler rows loaded");
    check(exec_bare(raw, "ANALYZE obligation"), "adversarial setup: ANALYZE run");

    const std::string is_open_plan =
        explain_plan(raw,
                     "SELECT 1 FROM obligation WHERE kind = 'structure' "
                     "AND addr_a = ARRAY['AB'] AND addr_b = ARRAY['Z','Z']");
    check(contains(is_open_plan, "Index") && !contains(is_open_plan, "Seq Scan"),
          "adversarial: is_open's full-PK-equality query plans as an index "
          "access, not a seq scan");

    const std::string list_open_prefix_plan =
        explain_plan(raw,
                     "SELECT kind FROM obligation WHERE kind = 'structure' "
                     "AND addr_a = ARRAY['AB']");
    check(contains(list_open_prefix_plan, "Index") &&
              !contains(list_open_prefix_plan, "Seq Scan"),
          "adversarial: list_open's kind+addr_a prefix query plans as an "
          "index access, not a seq scan");

    const std::string source_filter_plan =
        explain_plan(raw, "SELECT 1 FROM obligation WHERE source = 'filler'");
    check(contains(source_filter_plan, "Seq Scan"),
          "adversarial: a source-filtered query -- which this door offers no "
          "method for -- is a seq scan, proving no index backs that access");

    check(exec_bare(raw, "DELETE FROM obligation WHERE source = 'filler'"),
          "adversarial cleanup: filler rows removed");
  }
}

}  // namespace

int main(int argc, char **argv) {
  const std::string schema_path = (argc > 1) ? argv[1] : "wal_schema.sql";
  const std::string conninfo = "dbname=wal_manager";

  {
    PGconn *maint = PQconnectdb("dbname=postgres");
    if (maint == nullptr || PQstatus(maint) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL wal_book_test: no local Postgres reachable: %s\n",
                   maint ? PQerrorMessage(maint) : "null connection");
      if (maint) PQfinish(maint);
      return 1;
    }
    PGresult *r = PQexec(maint, "SELECT 1 FROM pg_database WHERE datname = 'wal_manager'");
    const bool exists = PQresultStatus(r) == PGRES_TUPLES_OK && PQntuples(r) > 0;
    PQclear(r);
    if (!exists && !exec_bare(maint, "CREATE DATABASE wal_manager")) {
      std::fprintf(stderr, "FAIL wal_book_test: could not create wal_manager\n");
      PQfinish(maint);
      return 1;
    }
    PQfinish(maint);
  }

  PGconn *raw = PQconnectdb(conninfo.c_str());
  if (raw == nullptr || PQstatus(raw) != CONNECTION_OK) {
    std::fprintf(stderr, "FAIL wal_book_test: cannot connect to wal_manager: %s\n",
                 raw ? PQerrorMessage(raw) : "null connection");
    if (raw) PQfinish(raw);
    return 1;
  }
  {
    const bool reset = exec_bare(raw, "DROP SCHEMA public CASCADE; CREATE SCHEMA public;");
    const bool applied = reset && exec_bare(raw, read_file(schema_path));
    if (!applied) {
      std::fprintf(stderr, "FAIL wal_book_test: schema reset/apply failed\n");
      PQfinish(raw);
      return 1;
    }
  }

  try {
    run_wal_book_checks(conninfo, raw);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "FAIL wal_book_test: unexpected exception: %s\n", e.what());
    ++g_failures;
  }

  PQfinish(raw);

  if (g_failures == 0) {
    std::printf("PASS wal_book_test\n");
    return 0;
  }
  std::printf("FAIL wal_book_test (%d failed)\n", g_failures);
  return 1;
}
