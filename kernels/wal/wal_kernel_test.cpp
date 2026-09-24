// Standalone check harness for the WAL manager as a monitored-endpoint
// kernel (WAL-INTEGRATION-PLAN.md). Same style as wal_book_test.cpp /
// wal_monitor_test.cpp: a tiny check macro, one line per check, PASS/FAIL
// summary, non-zero exit on any failure. DB-backed against the same
// disposable `wal_manager` database, reset the same way.
//
// Also exercises the box/endpoint/scheduler substrate (dataflow_test.cpp's
// pattern): work is seeded via `Scheduler::submit`, drained via
// `run_until_idle`, and the out-box is an UNREGISTERED standing box this
// test drains directly, standing in for the not-yet-built cache manager
// (WAL-INTEGRATION-PLAN.md "Ownership shape, priority, and the driver").
#include <libpq-fe.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "codec.h"
#include "endpoint.h"
#include "scheduler.h"
#include "wal_book.h"
#include "wal_kernel.h"
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
using wal::WalKernel;
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

// Seeds `report` into the in-arena and submits its handle (the arena
// index, decimal-string-rendered -- WAL-INTEGRATION-PLAN.md §3) to
// `box_id` via the scheduler's sole enqueue path. Does NOT run the
// scheduler -- callers batch as many seed_and_submit calls as they want
// processed together before calling run_until_idle().
void seed_and_submit(scheduler::Scheduler &sched, std::vector<Report> &in_arena,
                      endpoint::EndpointId box_id, Report report) {
  in_arena.push_back(std::move(report));
  const std::size_t idx = in_arena.size() - 1;
  sched.submit(box_id, box::Message{std::to_string(idx), std::nullopt});
}

// Drains every currently-queued message out of `out_box`, resolving each
// handle against `out_arena`, in FIFO (pop) order. Consumes the box.
std::vector<Obligation> drain_out_box(box::Box &out_box, const std::vector<Obligation> &out_arena) {
  std::vector<Obligation> drained;
  while (!out_box.empty()) {
    const box::Message m = out_box.pop();
    std::size_t consumed = 0;
    const unsigned long long idx_ull = std::stoull(m.payload, &consumed);
    if (consumed != m.payload.size()) {
      std::fprintf(stderr, "test bug: malformed out-box handle '%s'\n", m.payload.c_str());
      std::exit(2);
    }
    drained.push_back(out_arena.at(static_cast<std::size_t>(idx_ull)));
  }
  return drained;
}

// Multiset equality by value (Obligation has == but no <, so this is an
// O(n^2) remove-as-matched comparison -- fine at these test sizes).
bool same_multiset(std::vector<Obligation> a, std::vector<Obligation> b) {
  if (a.size() != b.size()) return false;
  for (const Obligation &item : a) {
    const auto it = std::find(b.begin(), b.end(), item);
    if (it == b.end()) return false;
    b.erase(it);
  }
  return true;
}

// Elements of `after` not present (by ==) in `before` -- used to isolate
// exactly what one report's ingest freshly opened in the live relation,
// tying it back to what the same report's ingest pushed to the out-box
// (test #7's single-tie invariant).
std::vector<Obligation> newly_opened(const std::vector<Obligation> &before,
                                      const std::vector<Obligation> &after) {
  std::vector<Obligation> added;
  for (const Obligation &o : after) {
    if (std::find(before.begin(), before.end(), o) == before.end()) {
      added.push_back(o);
    }
  }
  return added;
}

int count_kind(const std::vector<Obligation> &obligations, ObligationKind kind) {
  int n = 0;
  for (const Obligation &o : obligations) {
    if (o.kind == kind) ++n;
  }
  return n;
}

void run_wal_kernel_checks(const std::string &conninfo) {
  WalBook book(conninfo);
  WalMonitor monitor(book);

  // -- Shared driver: one Registry/Scheduler, one in-box per source, one
  // standing (unregistered) out-box, one WalKernel instance whose handler
  // is registered on every in-box -- exactly the plan's "Ownership shape"
  // (a single object's state shared across every in-box it is bound to).
  endpoint::Registry registry(/*standing_capacity=*/3, /*ephemeral_capacity=*/0);
  scheduler::Scheduler sched(registry, /*num_levels=*/1);

  box::Box core_box;
  box::Box shard1_box;
  box::Box out_box;
  const endpoint::EndpointId core_id = registry.register_standing(0, &core_box);
  const endpoint::EndpointId shard1_id = registry.register_standing(1, &shard1_box);
  const endpoint::EndpointId out_id = registry.register_standing(2, &out_box);
  // out_box is deliberately NOT registered with the scheduler (no
  // handler) -- it is the cache-manager stand-in, drained directly by
  // this test, per the plan's "Priority" section.

  std::vector<Report> in_arena;
  std::vector<Obligation> out_arena;
  WalKernel kernel(in_arena, out_arena, monitor, out_id);
  sched.register_box(&core_box, 0, kernel.make_handler());
  sched.register_box(&shard1_box, 0, kernel.make_handler());

  // ==================================================================
  // Test 1 -- per-source ingest opens the right obligations. A DECLARE
  // on "core" and a membership write on "shard1", fed through their own
  // in-boxes; after run_until_idle the WalBook shows exactly the
  // expected open obligations.
  // ==================================================================
  const Obligation structure_AA_BA{ObligationKind::kStructure, A("AA"), A("BA")};
  const Obligation structure_AB_BA{ObligationKind::kStructure, A("AB"), A("BA")};
  const Obligation mass_BA{ObligationKind::kMass, A("BA"), codec::Address{}};
  const Obligation membership_CA_CB{ObligationKind::kMembership, A("CA"), A("CB")};

  seed_and_submit(sched, in_arena, core_id,
                   wal::fixture::declare("core", 1, Scope::kGlobal, A("BA"), {A("AA"), A("AB")}));
  seed_and_submit(sched, in_arena, shard1_id,
                   wal::fixture::membership_write("shard1", 1, Scope::kLocal, A("CA"), {A("CB")}));
  sched.run_until_idle();

  check(book.is_open(structure_AA_BA) && book.is_open(structure_AB_BA) && book.is_open(mass_BA),
        "test1: core's DECLARE (two parents) opened both structure obligations and the mass "
        "obligation");
  check(book.is_open(membership_CA_CB),
        "test1: shard1's membership write opened its own obligation, on its own in-box");
  check(book.list_open().size() == 4,
        "test1: exactly the four obligations owed by the two fed reports are open, nothing "
        "extra");

  // ==================================================================
  // Test 2 -- reciprocal work is emitted, one item per owed obligation,
  // to the out-box; a settling report emits nothing.
  // ==================================================================
  {
    const std::vector<Obligation> drained = drain_out_box(out_box, out_arena);
    check(drained.size() == 4,
          "test2: out-box holds exactly one handle per owed obligation across both fed reports "
          "(2 structure + 1 mass + 1 membership)");

    const std::vector<Obligation> core_owed(drained.begin(), drained.begin() + 3);
    const std::vector<Obligation> shard1_owed(drained.begin() + 3, drained.end());
    check(count_kind(core_owed, ObligationKind::kStructure) == 2 &&
              count_kind(core_owed, ObligationKind::kMass) == 1,
          "test2: the two-parent structure DECLARE emitted two structure items + one mass item");
    check(same_multiset(core_owed, {structure_AA_BA, structure_AB_BA, mass_BA}),
          "test2: core's three out-box handles resolve to exactly its owed() set");
    check(same_multiset(shard1_owed, {membership_CA_CB}),
          "test2: shard1's one out-box handle resolves to its owed() set");
  }
  {
    // A settling report (return_token_child) -- owed() is empty for it,
    // so it must push nothing.
    seed_and_submit(sched, in_arena, core_id,
                     wal::fixture::return_token_child("core", 2, Scope::kGlobal, A("AA"), A("BA")));
    sched.run_until_idle();
    const std::vector<Obligation> drained = drain_out_box(out_box, out_arena);
    check(drained.empty(), "test2: a settling report (return_token_child) emits no owed work");
  }

  // ==================================================================
  // Test 3 -- the settling return closes the obligation (self-accounting,
  // unchanged), for a later-batch return (the one just fed above) AND a
  // same-batch forward/return pair.
  // ==================================================================
  check(!book.is_open(structure_AA_BA),
        "test3 (later batch): core's return_token_child(AA,BA) closed the structure obligation "
        "it settles");
  check(book.is_open(structure_AB_BA) && book.is_open(mass_BA),
        "test3: the return closed ONLY its own identity -- the sibling structure obligation and "
        "the mass obligation are untouched");

  const Obligation structure_AC_BB{ObligationKind::kStructure, A("AC"), A("BB")};
  const Obligation mass_BB{ObligationKind::kMass, A("BB"), codec::Address{}};
  {
    seed_and_submit(sched, in_arena, core_id,
                     wal::fixture::declare("core", 3, Scope::kGlobal, A("BB"), {A("AC")}));
    seed_and_submit(sched, in_arena, core_id,
                     wal::fixture::return_token_child("core", 4, Scope::kGlobal, A("AC"), A("BB")));
    sched.run_until_idle();

    check(!book.is_open(structure_AC_BB),
          "test3 (same batch): a forward DECLARE and its reciprocal return, fed in the same "
          "run_until_idle batch, leave the structure obligation closed");
    check(book.is_open(mass_BB),
          "test3 (same batch): the mass obligation the DECLARE opened is untouched by the "
          "return (a return_token_child settles structure only)");

    const std::vector<Obligation> drained = drain_out_box(out_box, out_arena);
    check(same_multiset(drained, {structure_AC_BB, mass_BB}),
          "test3: the same-batch pair emits owed work for the forward DECLARE only -- the "
          "return that follows it emits nothing");
  }

  // ==================================================================
  // Test 4 -- per-source order is structural (separate in-boxes); cross-
  // source interleave is independent of submission order.
  // ==================================================================
  const Obligation membership_DA_DB{ObligationKind::kMembership, A("DA"), A("DB")};
  const Obligation structure_EB_EA{ObligationKind::kStructure, A("EB"), A("EA")};
  const Obligation mass_EA{ObligationKind::kMass, A("EA"), codec::Address{}};
  {
    // Submitted shard1-before-core this time (the reverse of every
    // earlier section) -- the final result must not depend on that.
    seed_and_submit(sched, in_arena, shard1_id,
                     wal::fixture::declare("shard1", 2, Scope::kGlobal, A("EA"), {A("EB")}));
    seed_and_submit(sched, in_arena, core_id,
                     wal::fixture::membership_write("core", 5, Scope::kLocal, A("DA"), {A("DB")}));
    sched.run_until_idle();

    check(book.is_open(membership_DA_DB) && book.is_open(structure_EB_EA) &&
              book.is_open(mass_EA),
          "test4: both sources' obligations open correctly regardless of which source's report "
          "was submitted first");
    check(monitor.high_water("core") == std::optional<wal::Lsn>(5) &&
              monitor.high_water("shard1") == std::optional<wal::Lsn>(2),
          "test4: each source's own high-water advanced independently -- no cross-source "
          "coupling");

    const std::vector<Obligation> drained = drain_out_box(out_box, out_arena);
    check(same_multiset(drained, {membership_DA_DB, structure_EB_EA, mass_EA}),
          "test4: the out-box holds exactly the union of both sources' owed sets, regardless of "
          "interleave order");
  }

  // ==================================================================
  // Test 5 -- recycled-slot / only-follow discipline preserved. The
  // out-box uses the scheduler's sole enqueue path (Sender::send ->
  // Scheduler::submit, the same routine external submit() uses --
  // wal_kernel.cpp never touches box::Box::push directly); a send to a
  // since-recycled (stale-generation) endpoint is dropped, never
  // misdelivered, and the obligation it would have announced is still
  // durably booked -- "volatile transport, durable relation."
  // ==================================================================
  {
    endpoint::Registry probe_registry(/*standing_capacity=*/1, /*ephemeral_capacity=*/4);
    scheduler::Scheduler probe_sched(probe_registry, /*num_levels=*/1);
    box::Box probe_in_box;
    const endpoint::EndpointId probe_in_id = probe_registry.register_standing(0, &probe_in_box);

    const endpoint::EndpointId stale_out_id = probe_registry.allocate();
    probe_registry.recycle(stale_out_id);  // stale before it is ever sent to

    check(probe_registry.resolve(stale_out_id) == nullptr,
          "test5 setup: the recycled endpoint no longer resolves");

    std::vector<Report> probe_in_arena;
    std::vector<Obligation> probe_out_arena;
    // Reuses the SAME book/monitor as the shared driver above -- a
    // distinct source ("probe5") keeps its high-water independent of
    // "core"/"shard1", exactly as test4 proved sources are.
    WalKernel probe_kernel(probe_in_arena, probe_out_arena, monitor, stale_out_id);
    probe_sched.register_box(&probe_in_box, 0, probe_kernel.make_handler());

    const Obligation membership_probe{ObligationKind::kMembership, A("DC"), A("DD")};
    check(!book.is_open(membership_probe), "test5 setup: sanity, the probe identity is not open yet");

    // Confirms the underlying mechanism Sender::send relies on (it
    // delegates verbatim to Scheduler::submit, scheduler.cpp) drops a
    // stale-generation send exactly as dataflow_test.cpp's
    // test_recycled_slot_safety demonstrates.
    const scheduler::SendStatus direct_status =
        probe_sched.submit(stale_out_id, box::Message{"0", std::nullopt});
    check(direct_status == scheduler::SendStatus::kDropped,
          "test5: submit() to the recycled endpoint id is kDropped, the same path "
          "WalKernel's Sender::send uses for its out-box push");
    check(probe_in_box.empty(),
          "test5 setup: the direct submit() targeted the stale id, not probe_in_box -- nothing "
          "landed there");

    seed_and_submit(probe_sched, probe_in_arena, probe_in_id,
                     wal::fixture::membership_write("probe5", 1, Scope::kLocal, A("DC"), {A("DD")}));
    probe_sched.run_until_idle();

    check(probe_out_arena.size() == 1,
          "test5: the handler still appends the owed obligation to the out-arena even though "
          "its out-box send is dropped -- the push and the send are separate steps");
    check(book.is_open(membership_probe),
          "test5: the obligation is durably booked regardless of whether its out-box "
          "notification was delivered -- volatile transport, durable relation");
  }

  // ==================================================================
  // Test 6 -- a thrown (out-of-order same-source) report contributes
  // nothing to the out-box. feed()'s check runs before ingest() and
  // before this kernel's push loop, so nothing is booked and nothing is
  // pushed.
  // ==================================================================
  {
    const std::size_t pre_out_arena_size = out_arena.size();
    check(out_box.empty(), "test6 setup: out-box is drained clean before the throwing report");

    // core's high-water is 5 (test4); lsn=2 is already behind it.
    seed_and_submit(sched, in_arena, core_id,
                     wal::fixture::return_token_child("core", 2, Scope::kGlobal, A("AA"), A("BA")));

    bool threw_invalid_argument = false;
    try {
      sched.run_until_idle();
    } catch (const std::invalid_argument &) {
      threw_invalid_argument = true;
    } catch (...) {
      check(false, "test6: the propagated exception was not std::invalid_argument");
    }
    check(threw_invalid_argument,
          "test6: an out-of-order same-source report's std::invalid_argument propagates out of "
          "run_until_idle(), uncaught (the substrate's fail-loud stance, per the plan's failure "
          "decision)");
    check(out_arena.size() == pre_out_arena_size,
          "test6: the out-arena gained nothing from the offending report -- owed() ran but "
          "feed() threw before anything was pushed");
    check(out_box.empty(),
          "test6: the out-box received nothing attributable to the offending report");
    check(monitor.high_water("core") == std::optional<wal::Lsn>(5),
          "test6: the rejected report did not move core's high-water");
  }

  // ==================================================================
  // Test 7 -- invariant: for a freshly-fed report, the set of Obligation
  // identities the out-box handles resolve to is EXACTLY the set of
  // relation rows freshly opened by that report (single tie, stronger
  // than checking each side independently).
  // ==================================================================
  {
    const std::vector<Obligation> open_before = book.list_open();
    const std::size_t pre_out_idx = out_arena.size();

    // core's high-water is 5 after test4 (test6's throw did not move it).
    seed_and_submit(sched, in_arena, core_id,
                     wal::fixture::add_connection("core", 6, Scope::kGlobal, A("FA"), A("FB")));
    sched.run_until_idle();

    const std::vector<Obligation> open_after = book.list_open();
    const std::vector<Obligation> opened = newly_opened(open_before, open_after);
    const std::vector<Obligation> pushed(out_arena.begin() + static_cast<long>(pre_out_idx),
                                          out_arena.end());
    const std::vector<Obligation> drained = drain_out_box(out_box, out_arena);

    const Obligation structure_FB_FA{ObligationKind::kStructure, A("FB"), A("FA")};
    check(opened.size() == 1 && opened[0] == structure_FB_FA,
          "test7: the ADD_CONNECTION report freshly opened exactly one relation row, the "
          "expected structure obligation");
    check(same_multiset(pushed, drained),
          "test7 sanity: what landed in out_arena at this report's push time is exactly what "
          "the out-box handles resolved to");
    check(same_multiset(opened, pushed),
          "test7: the out-box handles resolve to EXACTLY the set of relation rows freshly "
          "opened by this report -- single tie between emitted work and booked obligations");
  }
}

}  // namespace

int main(int argc, char **argv) {
  const std::string schema_path = (argc > 1) ? argv[1] : "wal_schema.sql";
  const std::string conninfo = "dbname=wal_manager";

  {
    PGconn *maint = PQconnectdb("dbname=postgres");
    if (maint == nullptr || PQstatus(maint) != CONNECTION_OK) {
      std::fprintf(stderr, "FAIL wal_kernel_test: no local Postgres reachable: %s\n",
                   maint ? PQerrorMessage(maint) : "null connection");
      if (maint) PQfinish(maint);
      return 1;
    }
    PGresult *r = PQexec(maint, "SELECT 1 FROM pg_database WHERE datname = 'wal_manager'");
    const bool exists = PQresultStatus(r) == PGRES_TUPLES_OK && PQntuples(r) > 0;
    PQclear(r);
    if (!exists && !exec_bare(maint, "CREATE DATABASE wal_manager")) {
      std::fprintf(stderr, "FAIL wal_kernel_test: could not create wal_manager\n");
      PQfinish(maint);
      return 1;
    }
    PQfinish(maint);
  }

  PGconn *raw = PQconnectdb(conninfo.c_str());
  if (raw == nullptr || PQstatus(raw) != CONNECTION_OK) {
    std::fprintf(stderr, "FAIL wal_kernel_test: cannot connect to wal_manager: %s\n",
                 raw ? PQerrorMessage(raw) : "null connection");
    if (raw) PQfinish(raw);
    return 1;
  }
  {
    const bool reset = exec_bare(raw, "DROP SCHEMA public CASCADE; CREATE SCHEMA public;");
    const bool applied = reset && exec_bare(raw, read_file(schema_path));
    if (!applied) {
      std::fprintf(stderr, "FAIL wal_kernel_test: schema reset/apply failed\n");
      PQfinish(raw);
      return 1;
    }
  }
  PQfinish(raw);

  try {
    run_wal_kernel_checks(conninfo);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "FAIL wal_kernel_test: unexpected exception: %s\n", e.what());
    ++g_failures;
  }

  if (g_failures == 0) {
    std::printf("PASS wal_kernel_test\n");
    return 0;
  }
  std::printf("FAIL wal_kernel_test (%d failed)\n", g_failures);
  return 1;
}
