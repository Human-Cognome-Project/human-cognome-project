# dbmanager — the db/cache manager's tier-2 analyst-facing current-work body

> **Boundary:** this is the DB/cache manager's analyst-facing request/reaction surface, not analyst cognition and not the future analyst implementation.

The analyst-command surface of the db/cache manager, realigned onto the
monitored-endpoint substrate (`TIER2-PLAN.md`, adversary-vetted before
build; design pinned in `NOTES.md` "db/cache-manager kernel — box &
priority structure" → "Tier 2 — analyst-facing current-work body"). Same discipline as
`kernels/wal/`: reuses the built record-tier cores unmodified, adds only the box
coupling around them.

## Charter

`DbManagerKernel` is the **tier-2 rehome**, not a new core:

- It routes a request to `dispatch_one` / `dispatch_stream`
  (`dispatch/dispatch.h`) directly — `dispatch/` is consumed verbatim, unmodified, and this module
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

## Verification status

The former DB-resetting integration test has been removed. The tier-2
kernel remains available for a later verification approach that preserves
existing database contents.
