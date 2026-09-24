# dbmanager — the db/cache manager's tier-2 analyst reaction body

The analyst-command surface of the db/cache manager, realigned onto the
monitored-endpoint substrate (`TIER2-PLAN.md`, adversary-vetted before
build; design pinned in `NOTES.md` "db/cache-manager kernel — box &
priority structure" → "Tier 2 — analyst reaction body"). Same discipline as
`kernels/wal/`: reuses the built record-tier cores unmodified, adds only the box
coupling around them.

## Charter

`DbManagerKernel` is the **tier-2 rehome**, not a new core:

- It routes a request to `dispatch_one` / `dispatch_stream`
  (`dispatch/dispatch.h`) exactly as `dispatch_test.cpp` calls them
  directly — `dispatch/` is consumed verbatim, unmodified, and this module
  adds no verb logic, no validation, and no new rejection path.
- Each request carries its own return endpoint (`box::Message::reply_to`)
  — the box IS the correlation, per `network/ENDPOINT-ACTIVATION-NOTES.md`
  "Request→return correlation — RESOLVED". There is no correlation token,
  no matching table, and (unlike `wal::WalKernel`'s single fixed out-box)
  no shared destination: every request answers at its own caller-supplied
  endpoint.
- The analyst is a **peer kernel composing well-formed requests by
  construction** (`TIER2-PLAN.md`'s reconciliation banner): this module
  does not validate peer-kernel requests. A malformed envelope (a
  non-decimal arena handle, an absent `reply_to`) is a programming/wiring
  bug, surfaced fail-loud the same way `wal::WalKernel` lets a bad handle
  throw — an absent `reply_to` is caught by an explicit guard and throws
  `std::invalid_argument` (NOT undefined behaviour from an unchecked
  `std::optional` dereference, NOT a silent no-op) — not an analyst-facing
  rejection path built here.
- **Core-non-destructive, no WAL coupling**: no report emission, no
  WAL-manager interaction. READ mutates nothing and opens no obligation.
- **Owns nothing.** The driver/test owns the request arena, the response
  arena, and the shared `dbk::Controller`, and must keep all three alive
  across every `run_until_idle()` call this kernel's handler runs under —
  exactly `wal::WalKernel`'s ownership shape.

## Files

- `db_manager_kernel.h` / `db_manager_kernel.cpp` — `DbManagerKernel`: the
  `Request` (`std::variant<dispatch::Command,
  std::vector<dispatch::AdditiveCommand>>`) and `Response`
  (`std::vector<dispatch::Result>`) arena types, `make_handler()`, and the
  reaction body (`handle`): parse the in-box handle → resolve the request
  → `dispatch_one`/`dispatch_stream` over the shared `Controller` →
  append the `Response` → send its own handle to the request's
  `reply_to`.
- `db_manager_kernel_test.cpp` — DB-backed, scheduler-driven fixture test
  (see "Tests" below).

## What this module does NOT build (named seams, TIER2-PLAN.md)

- The analyst-side converter / live feed (how an external analyst's
  request becomes an arena-seeded in-box message) — deferred, seam G6.
- Endpoint advertising / cross-connection — a separate, not-yet-built
  system-configuration routine. This kernel is **advertising-agnostic**:
  it sends to the `reply_to` a message carries and never resolves,
  discovers, or allocates an endpoint itself. The fixture-fed test
  supplies return endpoints directly, standing in for that routine.
- Tiers 1/3/4 and the full setup-runner priority assembly — this pass
  builds tier 2 in isolation (one `Scheduler`, `num_levels=4`, analyst
  boxes at level 1; the other three levels stay empty here).
- WAL emission / any WAL-manager coupling; component-originated
  sub-requests (READ→sim→DECLARE); concurrent-append/MPSC (single-threaded
  first cut).

## Tests

DB-backed against the same disposable `hcp3_core` database
`dispatch_test.cpp` uses (reset the same way: `DROP SCHEMA public CASCADE;
CREATE SCHEMA public;`, then `../schema/schema.sql` reapplied). Drives
fixture `dispatch::Command` / `dispatch::AdditiveCommand` values through
the endpoint/scheduler substrate — via `Scheduler::submit` +
`run_until_idle`, never by calling `dispatch_one`/`dispatch_stream`
directly — which is exactly what this module adds over `dispatch/`.

Covers (`TIER2-PLAN.md` "Tests"):

- Each verb as a single `Command` (DECLARE, READ, MOVE_RECORD,
  ADD_CONNECTION, DELETE_RECORD, DELETE_CONNECTION) routes its `Result` to
  `reply_to`.
- An ordered stream (DECLARE then a MOVE of what it just placed) preserves
  dependent ordering through the box.
- A partial-rejection stream (an ADD_CONNECTION naming a nonexistent
  group — a `Result` with its success flag false, NOT a throw) still runs
  every subsequent item and returns the full N-length `Response`.
- Multi-analyst correlation: two analyst boxes, two distinct
  driver-supplied return endpoints; each request's `Response` lands at its
  own `reply_to`, never crossed.
- READ mutates nothing, checked by a `Controller` relationship/mass
  snapshot diff before/after (no WAL manager in this harness to observe an
  obligation on).
- A malformed arena handle throws `std::invalid_argument`, propagated
  uncaught out of `run_until_idle()`, store untouched.
- A well-formed handle with an absent `reply_to` throws
  `std::invalid_argument`, propagated uncaught out of `run_until_idle()`,
  store untouched, nothing sent to any return box.
- A numeric but out-of-range arena handle throws `std::out_of_range` from
  `req_arena_.at(index)`, propagated uncaught out of `run_until_idle()`.

## Build & run

Requires libpq and a local Postgres reachable as the current OS user (peer
auth, no password) — same harness convention as `dispatch/dispatch_test.cpp`.

```sh
# from kernels/database/dbmanager/
g++ -std=c++17 -O2 -Wall -Wextra \
    -I. -I../codec -I../command -I../controller -I../declare -I../read \
    -I../update -I../dispatch -I../../../network/endpoint -I"$(pg_config --includedir)" \
    db_manager_kernel.cpp db_manager_kernel_test.cpp \
    ../dispatch/dispatch.cpp \
    ../codec/codec.cpp ../command/command_ir.cpp ../command/span_planner.cpp \
    ../controller/controller.cpp \
    ../declare/declare_core.cpp ../read/read_core.cpp ../update/update_core.cpp \
    ../../../network/endpoint/endpoint.cpp ../../../network/endpoint/scheduler.cpp \
    -L"$(pg_config --libdir)" -lpq \
    -o db_manager_kernel_test

./db_manager_kernel_test                       # uses ../schema/schema.sql
./db_manager_kernel_test /path/to/schema.sql   # optional override
```

Output is one `ok`/`FAIL` line per check, a `PASS`/`FAIL
db_manager_kernel_test` summary, and a non-zero exit on any failure. If no
local Postgres is reachable it prints a clear message and exits non-zero
rather than pretending to pass.

### Disposable `hcp3_core` DB

Same convention as `dispatch_test.cpp` / `read/read_core_test.cpp`: the
test connects to `dbname=hcp3_core`, creating it if absent and resetting
it (`DROP SCHEMA public CASCADE; CREATE SCHEMA public;`, then
`schema.sql` reapplied) at the start of every run. Don't point it at an
`hcp3_core` database holding anything you want kept.
