# wal — the WAL manager (bookkeeper)

The WAL manager's own standalone kernel set (`WAL-PLAN.md`, `WAL-IMPL-PLAN.md`
— rev. 6 / 7-point ruling set, Patrick 2026-09-18). Same discipline as the
record-tier modules (`kernels/database/codec/`, `kernels/database/declare/`, `kernels/database/read/`): each part builds, runs,
and tests **on its own**.

This file is for working **on** this kernel set (charter, file map,
build/run). If you're working on something that **uses** it instead —
chiefly the cache manager — see `USAGE.md` for the consumer contract.
For the intended live ingress, deferred-work flow, and local-data direction,
see [REPORT-TO-WORK.md](REPORT-TO-WORK.md).

## Charter

The WAL manager is a **bookkeeper/observer over WAL reports** — never a
writer of the primary change:

- It **never touches `hcp3_core`** and **never reads the command string**;
  recognition works from a report's own initial data only (`wal_recognize`).
- It **never designs or drives the cache manager** — the async
  file-now/wire-later runtime this monitors for is out of scope; this set
  only makes the mechanism tolerant of that gap, driven from fixtures
  (`wal_report.h`).
- What it *does* write is its **own** bookkeeping DB (`wal_manager` — this
  is not a core-store write): an obligation **opens** as a row insert when a
  change is seen, and **closes** as a PK-delete when that change's own
  reciprocal return is observed. **Open = membership of the relation** —
  no status column, no scan, no done-mark, no drain.
- **only-follow, no reverse index**: every access is a bounded PK follow.
  Matching a followup to the obligation it settles is direct equality on
  the shared axis-appropriate identity (`(parent, child)` for structure,
  `(member, group)` for membership), never a reverse-index lookup or a
  predicate scan on a non-key column.
- Mass is booked **with** a change's other followup obligations (not a
  separate table); the mass **value** lives entirely in the FIXED
  `token.mass` — the WAL manager only monitors the NULL→non-NULL fill, and
  only for a new-token DECLARE.
- **Per-source ordering only.** Each source's own native `lsn` orders that
  source; there is no cross-source global order.

One line: **the cache manager writes the primary change and builds its own
return paths; the WAL manager books what returns are owed and watches them
land.**

## Instance-local databases — discussion note (2026-09-25)

NAPIER's planned private databases hold one instance's personal history and
relationships (provisional names: `personality.db`, `relationships.db`; their
partition is not settled). The intended WAL privacy rule is **directional**:
changes in global factors may generate deferred work *into* these private
databases, while changes originating there do not ordinarily generate
deferred work that writes directly to global databases or another instance.
Private database reports can still be booked by that instance's WAL manager
for its own followup tracking. Any significant derived result shared beyond
the instance needs a separate guarded release path; local reports and WAL
History entries are not automatically network/tracker output.

**Status: design intent, not enforced by this kernel.** `Report::scope` and
`history.scope` record local/global provenance; `Obligation` contains only its
identity, and the fixture-fed `WalKernel` pushes all owed items to one
cache-manager out-box without checking a destination. The future live report
feed and deferred-work routing need to preserve origin and target scope so
this rule can be enforced. Private storage, encryption and the release
guardrails remain deferred. See the ongoing
[`NAPIER system discussion`](../../docs/napier-system-discussion.md) for the
wider local/global relationship.

## Files

| File | What it is |
| --- | --- |
| `wal_schema.sql` | The WAL DB's own Postgres schema — `obligation` (live open-obligation relation, PK identity, no status column) and `history` (append-only, one row per report). |
| `wal_verify.sql` | Structural assertions on `wal_schema.sql` (PK shape, no non-PK index, address-column collation, `history`'s append-only shape). |
| `wal_schema_tests.md` | What each `wal_verify.sql` assertion proves. |
| `wal_report.h` | The decoded-report struct (`Report`, `OpKind`) — the in-process stand-in for logical-decoding output — plus `fixture::` builders for every op. |
| `wal_report_test.cpp` | Round-trips every fixture op's footprint through `codec`; checks the mass rule (NULL for an ordinary DECLARE, absent for a connection, non-NULL only for a mass-fill). |
| `wal_recognize.{h,cpp}` | Pure `owed(report) -> vector<Obligation>`: no DB, no store read, no command string. |
| `wal_recognize_test.cpp` | Each recognition rule (structure, membership, mass); DELETE yields nothing. |
| `wal_book.{h,cpp}` | The libpq bookkeeping door over the WAL DB: `open` / `close` / `is_open` / `list_open` / `record_seen`, every one a bounded PK follow. |
| `wal_book_test.cpp` | Open/close round-trip, double-open idempotence, absent-identity close as a no-op, History append-only, and an adversarial `EXPLAIN` check that every access plans as PK/PK-prefix (never a scan). |
| `wal_ingest.{h,cpp}` | The one-report step: `record_seen` → `owed(report)` → `open` each; if `report` is a settling write, `close` the obligation it settles by identity equality. |
| `wal_ingest_test.cpp` | Same-batch and later-batch forward/return pairs both close; a mass-fill closes a mass obligation; an unmatched settling write is inert. |
| `wal_monitor.{h,cpp}` | The ordered per-source loop: feeds `wal_ingest` one report at a time, tracking one `lsn` high-water mark per source. |
| `wal_monitor_test.cpp` | A scripted multi-source, interleaved stream reconciles to the expected open set; per-source progress is independent; out-of-order same-source delivery throws; a DELETE is History-booked only. |
| `wal_kernel.{h,cpp}` | The WAL manager as a monitored-endpoint kernel (`WAL-INTEGRATION-PLAN.md`): the `scheduler::Handler` reaction body wired onto the `network/endpoint/` substrate — per-source in-box(es), owed-work pushed as arena-handles to one standing out-box. Reuses `wal_recognize`/`wal_book`/`wal_ingest`/`wal_monitor` unchanged. |
| `wal_kernel_test.cpp` | Fixture-fed, DB-backed, scheduler-driven: per-source ingest, owed-work emission, same/later-batch settlement, cross-source interleave independence, recycled-endpoint drop (volatile transport, durable relation), a thrown out-of-order report pushing nothing, and the out-box/freshly-opened-rows single-tie invariant. |

## Build & run

Every part is standalone-buildable, C++17 + libpq, its own tiny in-file
check harness (`ok`/`FAIL` lines, `PASS <part>_test` on success, non-zero
exit on any failure) — same convention as `kernels/database/codec/README.md`. **DB-backed
tests must be run from inside this directory** (`wal/`): they load
`wal_schema.sql` relative to the current working directory (optionally
overridable as `argv[1]`).

```sh
# from kernels/wal/ — pure, no DB:
g++ -std=c++17 -O2 -Wall -Wextra -I. -I../database/codec \
    wal_recognize.cpp wal_recognize_test.cpp ../database/codec/codec.cpp \
    -o /tmp/wal_recognize_test && /tmp/wal_recognize_test

g++ -std=c++17 -O2 -Wall -Wextra -I. -I../database/codec \
    wal_report_test.cpp ../database/codec/codec.cpp \
    -o /tmp/wal_report_test && /tmp/wal_report_test

# from kernels/wal/ — DB-backed (see "Disposable wal_manager DB" below):
g++ -std=c++17 -O2 -Wall -Wextra -I. -I../database/codec -I"$(pg_config --includedir)" \
    wal_book.cpp wal_book_test.cpp ../database/codec/codec.cpp \
    -L"$(pg_config --libdir)" -lpq \
    -o /tmp/wal_book_test && /tmp/wal_book_test

g++ -std=c++17 -O2 -Wall -Wextra -I. -I../database/codec -I"$(pg_config --includedir)" \
    wal_ingest.cpp wal_book.cpp wal_recognize.cpp wal_ingest_test.cpp ../database/codec/codec.cpp \
    -L"$(pg_config --libdir)" -lpq \
    -o /tmp/wal_ingest_test && /tmp/wal_ingest_test

g++ -std=c++17 -O2 -Wall -Wextra -I. -I../database/codec -I"$(pg_config --includedir)" \
    wal_monitor.cpp wal_ingest.cpp wal_book.cpp wal_recognize.cpp wal_monitor_test.cpp ../database/codec/codec.cpp \
    -L"$(pg_config --libdir)" -lpq \
    -o /tmp/wal_monitor_test && /tmp/wal_monitor_test

# from kernels/wal/ — DB-backed AND links the shared endpoint substrate:
g++ -std=c++17 -O2 -Wall -Wextra -I. -I../database/codec -I../../network/endpoint -I"$(pg_config --includedir)" \
    wal_kernel.cpp wal_kernel_test.cpp \
    wal_monitor.cpp wal_ingest.cpp wal_book.cpp wal_recognize.cpp \
    ../../network/endpoint/endpoint.cpp ../../network/endpoint/scheduler.cpp ../database/codec/codec.cpp \
    -L"$(pg_config --libdir)" -lpq \
    -o /tmp/wal_kernel_test && /tmp/wal_kernel_test
```

Schema-only check (no C++ build):

```sh
# from kernels/wal/
createdb wal_manager 2>/dev/null || true
psql -d wal_manager -v ON_ERROR_STOP=1 -f wal_schema.sql
psql -d wal_manager -v ON_ERROR_STOP=1 -f wal_verify.sql
```

### Disposable `wal_manager` DB

The DB-backed tests (`wal_book_test`, `wal_ingest_test`, `wal_monitor_test`)
connect to `dbname=wal_manager` — a **disposable** database, created if
absent and reset (`DROP SCHEMA public CASCADE; CREATE SCHEMA public;`, then
`wal_schema.sql` reapplied) at the start of every run, exactly as
`read/read_core_test.cpp` does for `hcp3_core`. It is **always a separate
database from `hcp3_core`** — nothing here ever connects to the core store.
Because the reset is destructive, don't point these tests at a
`wal_manager` database holding anything you want kept, and don't run two of
these DB-backed tests against it at the same instant (concurrent resets
race — a transient "relation does not exist" is that race, not a code
defect; the test is deterministic when run alone).

## Decisions on record

**Design-ahead (named, not built — WAL-IMPL-PLAN.md §1 bucket B):**
- The async reciprocal split (the cache manager's file-now/wire-later
  runtime) does not exist yet; this set only makes the monitor gap-tolerant
  via fixtures.
- Live Postgres logical-decoding ingest — the wire form is not designed;
  ingest here is from an in-process decoded `Report`, not a live slot.
- The mass-fill write — there is no `token.mass` UPDATE primitive today;
  `wal_report.h`'s `mass_fill` fixture is the only way to exercise the
  mass-close path.
- MOVE_RECORD / rekey of open obligations — MOVE is atomic today (nothing
  owed); re-keying live obligations under a moved `token_id` is
  design-ahead, and is the one case that could demand a reverse index, so
  it stays out.

**NEEDS-PATRICK (do not plan or build — WAL-IMPL-PLAN.md §1 bucket C):**
- The connection mass-**recompute** signal (how a dirtied existing-token
  mass is marked, as opposed to a new-token DECLARE's NULL signal) —
  part of the deferred mass-aggregation model.
- **F4** — whether self-accounting replaces the cache manager's own
  pending-list runtime, or only governs the WAL manager's view.
- The cross-source global-order basis — swarm-side, only if/when the swarm
  side is built.
- The optional canonical NOTES WAL-management section — scope is already
  recorded across NOTES/PLAN/API; consolidating it is Patrick's call, not a
  build item.
- The swarm side entirely (unspooling peers' compressed WAL patterns, the
  cross-network DELETE validation act, drift control) — deferred.
