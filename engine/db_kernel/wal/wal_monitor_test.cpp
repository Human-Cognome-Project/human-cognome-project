// Standalone check harness for the WAL monitor loop (WAL-IMPL-PLAN.md W-6).
// Same style as wal_book_test.cpp / wal_ingest_test.cpp: a tiny check
// macro, one line per check, PASS/FAIL summary, non-zero exit on any
// failure.
//
// Runs directly against a real, DISPOSABLE `wal_manager` database --
// created if absent, reset (DROP SCHEMA public CASCADE; CREATE SCHEMA
// public;) and reapplied from wal_schema.sql on every run, exactly as
// wal_book_test.cpp / wal_ingest_test.cpp do. This is the WAL manager's
// OWN database -- it is never `hcp3_core`, and this harness never touches
// `hcp3_core`.
#include <libpq-fe.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "codec.h"
#include "wal_book.h"
#include "wal_ingest.h"
#include "wal_monitor.h"
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
using wal::WalMonitor;

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

// Raw single-value scalar query -- same pattern as wal_ingest_test.cpp's
// scalar_count, generalized to any single bigint/count result (used here
// for both COUNT(*) and MAX(lsn) against `history`, for the
// replay-from-History consistency checks).
long long scalar_ll(PGconn *conn, const std::string &sql) {
  PGresult *res = PQexec(conn, sql.c_str());
  if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0 ||
      PQgetisnull(res, 0, 0)) {
    PQclear(res);
    return -1;
  }
  const long long n = std::atoll(PQgetvalue(res, 0, 0));
  PQclear(res);
  return n;
}

void run_wal_monitor_checks(const std::string &conninfo, PGconn *raw) {
  WalBook book(conninfo);
  WalMonitor mon(book);

  // ------------------------------------------------------------------
  // Two sources, fed INTERLEAVED via individual feed() calls, checking
  // per-source high-water after each step -- proves interleaving does
  // not couple progress, and (by giving both sources overlapping lsn
  // NUMBERS -- both start at 1) that there is no shared/global lsn space:
  // a bug that kept one high-water keyed on lsn alone, ignoring source,
  // would fail this immediately.
  // ------------------------------------------------------------------
  const Obligation structure_GA_GB{ObligationKind::kStructure, A("GA"), A("GB")};
  const Obligation mass_GB{ObligationKind::kMass, A("GB"), codec::Address{}};
  const Obligation membership_HA_HB{ObligationKind::kMembership, A("HA"), A("HB")};
  const Obligation structure_JA_JB{ObligationKind::kStructure, A("JA"), A("JB")};
  const Obligation mass_JB{ObligationKind::kMass, A("JB"), codec::Address{}};

  {
    check(!mon.high_water("core").has_value(), "init: core has no high-water yet");
    check(!mon.high_water("shard1").has_value(), "init: shard1 has no high-water yet");

    mon.feed(wal::fixture::declare("core", 1, Scope::kGlobal, A("GB"), {A("GA")}));
    check(mon.high_water("core") == std::optional<wal::Lsn>(1),
          "interleave: core high-water advances to 1 after its lsn-1 report");
    check(!mon.high_water("shard1").has_value(),
          "interleave: shard1 untouched by core's report");
    check(book.is_open(structure_GA_GB) && book.is_open(mass_GB),
          "interleave: core's DECLARE opened structure + mass obligations");

    mon.feed(wal::fixture::declare("shard1", 1, Scope::kGlobal, A("JB"), {A("JA")}));
    check(mon.high_water("shard1") == std::optional<wal::Lsn>(1),
          "interleave: shard1 high-water advances to 1 (same NUMBER as core's, "
          "different source -- no shared lsn space)");
    check(mon.high_water("core") == std::optional<wal::Lsn>(1),
          "interleave: core's high-water untouched by shard1's report");
    check(book.is_open(structure_JA_JB) && book.is_open(mass_JB),
          "interleave: shard1's DECLARE opened its own structure + mass obligations");

    mon.feed(wal::fixture::return_token_child("core", 2, Scope::kGlobal, A("GA"), A("GB")));
    check(mon.high_water("core") == std::optional<wal::Lsn>(2),
          "interleave: core high-water advances to 2");
    check(mon.high_water("shard1") == std::optional<wal::Lsn>(1),
          "interleave: shard1 high-water STILL 1 -- core's lsn-2 report does not "
          "advance it, and does not invent a cross-source order");
    check(!book.is_open(structure_GA_GB), "interleave: core's return closes its structure obligation");
    check(book.is_open(mass_GB), "interleave: the return closed ONLY structure, not mass");

    mon.feed(wal::fixture::mass_fill("shard1", 2, Scope::kGlobal, A("JB"), 9));
    check(mon.high_water("shard1") == std::optional<wal::Lsn>(2),
          "interleave: shard1 high-water advances to 2");
    check(mon.high_water("core") == std::optional<wal::Lsn>(2),
          "interleave: core high-water untouched by shard1's lsn-2 report");
    check(!book.is_open(mass_JB), "interleave: shard1's mass-fill closes its mass obligation");
    check(book.is_open(structure_JA_JB),
          "interleave: shard1's structure obligation is untouched by the mass-fill");

    mon.feed(wal::fixture::membership_write("core", 3, Scope::kLocal, A("HA"), {A("HB")}));
    check(mon.high_water("core") == std::optional<wal::Lsn>(3), "interleave: core high-water advances to 3");
    check(book.is_open(membership_HA_HB), "interleave: core's membership write opens its obligation");

    // A DELETE report: History-booked only -- no removal obligation opened,
    // no cross-network validation-act obligation (that is swarm-side, not
    // built here -- WAL-PLAN.md S6).
    mon.feed(wal::fixture::delete_token("core", 4, Scope::kLocal, A("IA")));
    check(mon.high_water("core") == std::optional<wal::Lsn>(4), "interleave: core high-water advances to 4");
    {
      const auto all = book.list_open();
      bool touches_deleted = false;
      for (const Obligation &o : all) {
        if (o.addr_a == A("IA") || o.addr_b == A("IA")) touches_deleted = true;
      }
      check(!touches_deleted, "delete: DELETE opened no obligation touching the removed token");
    }
    const long long del_settled = scalar_ll(
        raw, "SELECT count(*) FROM history WHERE source = 'core' AND lsn = 4 "
             "AND settled_kind IS NULL AND settled_addr_a IS NULL AND settled_addr_b IS NULL");
    check(del_settled == 1, "delete: History-recorded with no settlement");
  }

  // ------------------------------------------------------------------
  // Out-of-order same-source delivery is rejected -- a feeder bug, not
  // silently accepted or reordered. Must not disturb book or progress.
  // ------------------------------------------------------------------
  {
    bool threw = false;
    try {
      mon.feed(wal::fixture::return_token_child("core", 2, Scope::kGlobal, A("GA"), A("GB")));
    } catch (const std::invalid_argument &) {
      threw = true;
    }
    check(threw, "out-of-order: re-feeding an already-passed core lsn throws invalid_argument");
    check(mon.high_water("core") == std::optional<wal::Lsn>(4),
          "out-of-order: rejected feed does not move core's high-water");

    bool threw2 = false;
    try {
      mon.feed(wal::fixture::mass_fill("shard1", 2, Scope::kGlobal, A("JB"), 9));
    } catch (const std::invalid_argument &) {
      threw2 = true;
    }
    check(threw2, "out-of-order: re-feeding an already-passed shard1 lsn throws invalid_argument");
    check(mon.high_water("shard1") == std::optional<wal::Lsn>(2),
          "out-of-order: rejected feed does not move shard1's high-water");

    const long long core_hist = scalar_ll(raw, "SELECT count(*) FROM history WHERE source = 'core'");
    check(core_hist == 4, "out-of-order: rejected feed does not add a spurious History row (core)");
    const long long shard1_hist = scalar_ll(raw, "SELECT count(*) FROM history WHERE source = 'shard1'");
    check(shard1_hist == 2, "out-of-order: rejected feed does not add a spurious History row (shard1)");
  }

  // ------------------------------------------------------------------
  // feed_source(): one source's whole ordered stream handed over at once.
  // ------------------------------------------------------------------
  const Obligation structure_KA_KB{ObligationKind::kStructure, A("KA"), A("KB")};
  const Obligation mass_KB{ObligationKind::kMass, A("KB"), codec::Address{}};
  {
    const std::vector<Report> shard2_stream = {
        wal::fixture::declare("shard2", 5, Scope::kGlobal, A("KB"), {A("KA")}),
        wal::fixture::return_token_child("shard2", 6, Scope::kGlobal, A("KA"), A("KB")),
    };
    mon.feed_source(shard2_stream);
    check(mon.high_water("shard2") == std::optional<wal::Lsn>(6),
          "feed_source: high-water lands on the stream's last lsn");
    check(!book.is_open(structure_KA_KB), "feed_source: forward-then-return closes the structure obligation");
    check(book.is_open(mass_KB), "feed_source: the mass obligation is left open (no mass-fill in this stream)");
  }

  // ------------------------------------------------------------------
  // feed_all(): a scripted stream handed over as one vector.
  // ------------------------------------------------------------------
  const Obligation membership_LA_LB{ObligationKind::kMembership, A("LA"), A("LB")};
  {
    const std::vector<Report> shard3_stream = {
        wal::fixture::membership_write("shard3", 1, Scope::kLocal, A("LA"), {A("LB")}),
        wal::fixture::return_member_of("shard3", 2, Scope::kLocal, A("LA"), A("LB")),
    };
    mon.feed_all(shard3_stream);
    check(mon.high_water("shard3") == std::optional<wal::Lsn>(2),
          "feed_all: high-water lands on the stream's last lsn");
    check(!book.is_open(membership_LA_LB), "feed_all: forward-then-return closes the membership obligation");
  }

  // ------------------------------------------------------------------
  // Ends with EXACTLY the still-open obligations expected (opened minus
  // closed), across every source fed above.
  // ------------------------------------------------------------------
  {
    const auto all = book.list_open();
    check(all.size() == 4,
          "final: exactly 4 obligations remain open across all sources (got " +
              std::to_string(all.size()) + ")");
    check(book.is_open(mass_GB), "final: core's mass(GB) is open");
    check(book.is_open(membership_HA_HB), "final: core's membership(HA,HB) is open");
    check(book.is_open(structure_JA_JB), "final: shard1's structure(JA,JB) is open");
    check(book.is_open(mass_KB), "final: shard2's mass(KB) is open");
    check(!book.is_open(structure_GA_GB), "final: core's structure(GA,GB) is closed");
    check(!book.is_open(mass_JB), "final: shard1's mass(JB) is closed");
    check(!book.is_open(structure_KA_KB), "final: shard2's structure(KA,KB) is closed");
    check(!book.is_open(membership_LA_LB), "final: shard3's membership(LA,LB) is closed");
  }

  // ------------------------------------------------------------------
  // Replay/state-from-History consistency. History is the durable,
  // append-only per-source record (wal_schema.sql); it is deliberately
  // NOT re-derivable into obligations here (its footprint column is
  // "audit/debug only -- never compared, ranged, or keyed on", by the
  // schema's own charter), so "consistent" is checked two ways that
  // respect that boundary:
  //   (a) History's own per-source count/max(lsn) agrees with the
  //       monitor's in-memory per-source progress (the durable record
  //       and the live progress map tell the same story), and
  //   (b) every settlement History recorded is still reflected as
  //       closed in the live obligation relation (the append-only log
  //       and the live relation do not disagree about what settled).
  // ------------------------------------------------------------------
  {
    struct SourceExpect {
      std::string source;
      long long count;
      long long max_lsn;
    };
    const std::vector<SourceExpect> expects = {
        {"core", 4, 4}, {"shard1", 2, 2}, {"shard2", 2, 6}, {"shard3", 2, 2},
    };
    for (const auto &e : expects) {
      const long long count =
          scalar_ll(raw, "SELECT count(*) FROM history WHERE source = '" + e.source + "'");
      const long long max_lsn =
          scalar_ll(raw, "SELECT max(lsn) FROM history WHERE source = '" + e.source + "'");
      check(count == e.count, "replay: History row count for " + e.source + " matches reports fed");
      check(max_lsn == e.max_lsn, "replay: History max(lsn) for " + e.source + " matches high-water");
      check(mon.high_water(e.source) == std::optional<wal::Lsn>(static_cast<wal::Lsn>(e.max_lsn)),
            "replay: monitor's own high-water for " + e.source + " agrees with History's max(lsn)");
    }

    const long long settled_rows = scalar_ll(raw, "SELECT count(*) FROM history WHERE settled_kind IS NOT NULL");
    check(settled_rows == 4,
          "replay: exactly 4 History rows recorded a settlement (the 4 forward/return pairs)");
    // Each identity History recorded as settled is confirmed closed in the
    // live obligation relation -- the append-only record and the live
    // relation agree.
    check(!book.is_open(structure_GA_GB), "replay: History's core settlement stays closed live");
    check(!book.is_open(mass_JB), "replay: History's shard1 settlement stays closed live");
    check(!book.is_open(structure_KA_KB), "replay: History's shard2 settlement stays closed live");
    check(!book.is_open(membership_LA_LB), "replay: History's shard3 settlement stays closed live");
  }
}

}  // namespace

int main(int argc, char **argv) {
  const std::string schema_path = (argc > 1) ? argv[1] : "wal_schema.sql";
  const std::string conninfo = "dbname=wal_manager";

  {
    PGconn *maint = PQconnectdb("dbname=postgres");
    if (maint == nullptr || PQstatus(maint) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL wal_monitor_test: no local Postgres reachable: %s\n",
                   maint ? PQerrorMessage(maint) : "null connection");
      if (maint) PQfinish(maint);
      return 1;
    }
    PGresult *r = PQexec(maint, "SELECT 1 FROM pg_database WHERE datname = 'wal_manager'");
    const bool exists = PQresultStatus(r) == PGRES_TUPLES_OK && PQntuples(r) > 0;
    PQclear(r);
    if (!exists && !exec_bare(maint, "CREATE DATABASE wal_manager")) {
      std::fprintf(stderr, "FAIL wal_monitor_test: could not create wal_manager\n");
      PQfinish(maint);
      return 1;
    }
    PQfinish(maint);
  }

  PGconn *raw = PQconnectdb(conninfo.c_str());
  if (raw == nullptr || PQstatus(raw) != CONNECTION_OK) {
    std::fprintf(stderr, "FAIL wal_monitor_test: cannot connect to wal_manager: %s\n",
                 raw ? PQerrorMessage(raw) : "null connection");
    if (raw) PQfinish(raw);
    return 1;
  }
  {
    const bool reset = exec_bare(raw, "DROP SCHEMA public CASCADE; CREATE SCHEMA public;");
    const bool applied = reset && exec_bare(raw, read_file(schema_path));
    if (!applied) {
      std::fprintf(stderr, "FAIL wal_monitor_test: schema reset/apply failed\n");
      PQfinish(raw);
      return 1;
    }
  }

  try {
    run_wal_monitor_checks(conninfo, raw);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "FAIL wal_monitor_test: unexpected exception: %s\n", e.what());
    ++g_failures;
  }

  PQfinish(raw);

  if (g_failures == 0) {
    std::printf("PASS wal_monitor_test\n");
    return 0;
  }
  std::printf("FAIL wal_monitor_test (%d failed)\n", g_failures);
  return 1;
}
