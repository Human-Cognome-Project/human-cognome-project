# TIER2-PLAN — the analyst-facing current-work body on the activation substrate

> **Terminology.** This is a database/cache-manager reaction handler, not analyst cognition or an implementation of analyst functions. It is the analyst-facing current-work endpoint of the DB/cache kernel.

**Status: BUILT + adversary-vetted (2026-09-23).** Realigns the
db/cache manager's **tier-2 analyst-command surface** onto the monitored-endpoint
substrate. Built as `dbmanager/` (`db_manager_kernel.{h,cpp}` + test + README);
plan adversary-vetted, build adversary-vetted (one UB blocker found on an unchecked
`reply_to` deref and fixed to an explicit fail-loud throw + regression test), verified
green — `PASS db_manager_kernel_test`, 38/38, ASan/UBSan clean. Design pinned in `NOTES.md` "db/cache-manager kernel — box & priority
structure" → "Tier 2 — analyst-facing current-work body". Canadian English.

Discipline: draft plan → fresh adversary vets → reconcile → **Sonnet** builds →
fresh adversary vets build → verify green → commit. This file is the plan half.

> **Reconciliation (2026-09-23, adversary round 1 + Patrick reframe).** Adversary
> verdict: PLAN NOT READY, 3 blockers + 4 should-fixes.
>
> **Patrick reframe (2026-09-23) — governs the reconciliation.** The analyst is **not
> a human at a command line; it is a peer kernel composing requests per its own
> ruleset.** Requests are therefore **well-formed by construction** — the return
> endpoint and the arena handle are part of what the composing kernel emits. We do
> **not** design request-validation / malformation-rejection between trusted peer
> kernels. A malformed envelope (absent `reply_to`, bad handle) is a **programming/
> wiring bug**, surfaced fail-loud by the substrate's existing discipline (the same
> way `WalKernel` lets a bad arena handle throw), NOT an analyst-facing rejection
> path to build.
>
> **Consequences:** adversary **blockers 1 (reply_to-check ordering) and 3 (`kDropped`
> handling) DISSOLVE** — there is no missing/stale-`reply_to` runtime case to order or
> handle; `parse_handle` runs before dispatch as the natural bug-surface, nothing more
> designed. Should-fix 7 (an ordering test for the dissolved blocker 1) is dropped.
>
> **Blocker 2 reframed by Patrick (2026-09-23) — the endpoint ADVERTISING system is not
> built yet.** How a kernel obtains / allocates / discovers another's endpoint (incl. a
> return endpoint) is a **separate, not-yet-constructed system** — a named deferred seam,
> NOT tier 2's to design. So the manager is **advertising-agnostic**: it sends to the
> `reply_to` the message carries and never resolves, discovers, or allocates an endpoint
> itself. In the fixture-fed test the **driver supplies the return endpoint** (a box it
> owns and reads). The substrate's recycled-slot / stale-generation safety already lives
> in `network/endpoint/` + its tests; tier 2 neither re-designs it nor claims a return-endpoint
> lifecycle. (Earlier draft's "ephemeral per-request, composing kernel allocates/recycles"
> is withdrawn — it presumed the unbuilt advertising system.)
>
> **Kept and reconciled below:** should-fix 4 (`num_levels`/level, §In-scope 3);
> should-fix 5 (READ test → `Controller` DB-diff); should-fix 6 (partial-rejection
> stream, where "rejection" is the CORE's own semantic `Result`, not envelope malformation).

## Mission (one line)

Wire the **built record-tier cores** (`declare/`, `read/`, `update/`, routed by
`dispatch/`) onto the **built endpoint substrate** (`network/endpoint/`) so the db/cache
manager serves analyst current work as a **monitored-endpoint kernel**: a request
arrives at an analyst in-box, its verb runs as a reaction body over the shared
`Controller`, and the `Result` returns to the request's **caller-supplied return
endpoint** — fixture-fed, with tests, standalone-buildable. This is the tier-2
"rehome": **no core logic and no `dispatch/` logic changes.**

## Governing decisions (traced; do not re-litigate)

- **One kernel, one handler, N analyst boxes, one `Controller`.** `NOTES.md` tier-2
  pin; mirrors `WalKernel` on N per-source in-boxes sharing one `WalMonitor`.
- **Message = `{ payload = decimal arena handle, reply_to = return endpoint }`.**
  Handle-in-payload, never serialization — `kernels/wal/WAL-INTEGRATION-PLAN.md` §3 /
  `wal_kernel.cpp::parse_handle`. Serialization is the deferred G6 wire seam.
- **`reply_to` is the caller-supplied return endpoint; the box IS the correlation.**
  `network/ENDPOINT-ACTIVATION-NOTES.md` "Request→return correlation — RESOLVED": the
  request names its own return endpoint; no correlation token, no matching table.
- **Both request forms kept:** a single `dispatch::Command` (any verb, incl. the
  non-arrayable DELETEs) and an ordered `std::vector<dispatch::AdditiveCommand>`
  stream drained as one unit. `dispatch/README.md`; NOTES "Arraying is universal".
- **Core-non-destructive: no WAL report emission in tier 2.** Reports reach the WAL
  manager from the Postgres logical-decoding feed (deferred seam), not this handler.
  READ mutates nothing, emits nothing, opens no obligation. `NOTES.md` tier-2 pin;
  activation-notes "READ … emits no WAL report".
- **Single-threaded cooperative first cut**, handlers run to completion; no blocking
  sub-requests (the analyst's READ→sim→DECLARE loop is the analyst kernel's business,
  modelled as the external driver). `network/endpoint/README.md` deferred seams.

## In scope — buildable now

New module `dbmanager/` inside `kernels/database/`; WAL is now the peer family `kernels/wal/`:

1. **`DbManagerKernel`** (`dbmanager/db_manager_kernel.{h,cpp}`). Owns nothing —
   holds references to the request arena, the response arena, and the `Controller`
   (the driver/test owns all three and keeps them alive across each
   `run_until_idle()`), exactly the `WalKernel` ownership shape.
   - **Request arena type:** `using Request = std::variant<dispatch::Command,
     std::vector<dispatch::AdditiveCommand>>;` — a side arena
     `const std::vector<Request>&`, seeded by the driver before the run.
   - **Response arena type:** `using Response = std::vector<dispatch::Result>;` —
     a single-`Command` request yields a one-element `Response`; a stream yields one
     `Result` per item, in order. `std::vector<Response>&`, appended by the handler.
     Uniformly a vector (single = size 1) so it is **one reply message per request**
     regardless of form — the return endpoint correlates one answer to one request.
   - Note: the `Request`'s `dispatch::Command` arm still admits the `UpdateCache` /
     `RebaseCache` stubs; they remain **reachable-but-inert** ("not yet implemented"
     `Result`) through this handler, to be revisited when tier 4 / the cache tier is
     built. Sanctioned by NOTES' Tier-2 pin ("any verb incl. DELETE").
   - `make_handler()` → `scheduler::Handler`; `handle(const box::Message&,
     scheduler::Sender&)` is the reaction body.
2. **The reaction body (`handle`)** — order and failure decision mirror
   `wal_kernel.cpp`:
   - `parse_handle(payload)` → index (fail-loud on malformed, same `stoull` +
     full-consume check; a malformed handle is a feeder bug, not swallowed).
   - `const Request& req = req_arena_.at(index);`
   - `std::visit`: a `dispatch::Command` → `Response{ dispatch_one(ctl_, cmd) }`;
     a stream → `dispatch_stream(ctl_, stream)` (already returns the ordered
     `vector<Result>`). **`dispatch/` is called verbatim — unchanged.**
   - Append the `Response` to `resp_arena_`; `sender.send(*message.reply_to,
     box::Message{ std::to_string(new_index), std::nullopt })`.
   - `reply_to` is **present by construction** — the composing analyst kernel emits it
     per its ruleset (we do not validate peer-kernel requests; see the reframe note).
     A malformed envelope is a programming/wiring bug, surfaced fail-loud by the same
     discipline as a bad handle, not an analyst-facing rejection path.
3. **A driver/test** (standing in for the deferred configuration routine) wiring
   `Registry` + `Scheduler` + N analyst boxes + the shared `Controller` + the two
   arenas, seeding fixture `Request`s and asserting the right `Response` lands at the
   right return endpoint. DB-backed (disposable `hcp3_core`, the `dispatch_test`
   harness shape).
   - **Priority levels (pinned, per `kernels/wal/WAL-INTEGRATION-PLAN.md` §4's precedent of
     pinning them for the isolated cut):** `Scheduler` constructed with `num_levels = 4`,
     reflecting the pinned tier map (0 = reconcile, **1 = analyst**, 2 = pending,
     3 = maintenance). Analyst boxes `register_box` at **level 1**. Only level 1 is
     exercised in this isolated cut; the other levels stay empty (the multi-tier
     assembly is out of scope).
   - **Return endpoints are driver-supplied** (the driver owns each return box and
     reads the `Response` out of it), standing in for the config routine — the manager
     never allocates or advertises one. Endpoint advertising/cross-connection is the
     deferred config-routine seam.

## Failure model (mirror WalKernel)

- **Normal record-tier rejection** (validation failure, reject-on-referenced, etc.)
  is a `dispatch::Result` value → returned to `reply_to` like any answer. Not a throw.
  This is the only "rejection" tier 2 has — it comes from the core, on a well-formed
  request.
- **Programming / infrastructure bugs** (malformed arena handle, a DB failure
  surfacing from a core) **throw** and abort the `run_until_idle()` run —
  `Scheduler::step()` wraps no handler in try/catch, by design. This is the substrate's
  existing bug-surface (same as `WalKernel` on a bad handle), NOT a designed
  peer-request rejection path. `parse_handle` + the arena resolve run before any
  `dispatch_*` call, so a bad-handle bug touches the DB not at all.

## Out of scope — do NOT build

- **The analyst-side converter / live feed** — how an external analyst's request
  becomes an arena-seeded in-box message (the G6 wire / split-mode API-pair shuttle).
  Fixture-fed only.
- **Endpoint advertising / cross-connection** — how kernels advertise their endpoints
  and get cross-connected is part of the (not-yet-built) **system configuration
  routine** (the setup runner extended to establish + cross-connect advertised
  endpoints — `network/ENDPOINT-ACTIVATION-NOTES.md` "The setup runner IS the command
  structure"). The manager is **advertising-agnostic** at runtime; the **fixture-fed
  test driver stands in for the configuration routine**, wiring the boxes and supplying
  return endpoints directly, exactly as that routine will.
- **Tiers 1, 3, 4 and the full setup-runner tier wiring** — reconcile box, the
  WAL-fed pending-work consumer (the file-now/wire-later split), standing maintenance.
  This pass builds tier 2 in isolation; the multi-tier priority assembly is a later
  integration. Note the intended mapping (level 0 reconcile … analyst … pending …
  maintenance) but do not build the other tiers.
- **WAL emission / any WAL-manager coupling** — none in tier 2.
- **Component-originated sub-requests / re-activation** (READ→sim→DECLARE) — the
  substrate's deferred seam; the requester is the external driver here.
- **Concurrent-append / MPSC** — single-threaded first cut (`box.h` named seam).

## Tests (project rule — ships with tests)

`g++ -std=c++17 -O2 -Wall -Wextra`, prints `PASS <name>`, DB-backed against a
disposable `hcp3_core` (skips-clean with a clear non-zero message if no local
Postgres), same harness as `dispatch_test.cpp`:

- Each verb as a single `Command` → correct `Result` lands at `reply_to` (DECLARE,
  READ, MOVE_RECORD, ADD_CONNECTION, DELETE_RECORD, DELETE_CONNECTION).
- An ordered stream (DECLARE then a MOVE of what it just placed) → ordered
  `Response`, dependent ordering preserved (the `dispatch_stream` guarantee, now
  through a box).
- **Partial-rejection stream:** a stream whose earlier item semantically rejects (a
  `Result` with its success flag false — NOT a throw) still runs every subsequent item
  to completion and returns the full N-length `Response` to the one `reply_to`
  (`dispatch_stream`'s unconditional loop, now through a box).
- **Multi-analyst correlation:** two analyst boxes, two distinct driver-supplied return
  endpoints; each request's `Response` routes to its own `reply_to`, never crossed —
  the box is the correlation.
- **READ** returns its result with **no mutation**, checked by a `Controller` DB-state
  diff (no new `token_child`/`members`/`member_of` rows, no mass change) before/after —
  there is no WAL manager in this harness to observe an obligation on.
- Malformed arena handle → throws (the substrate's bug-surface), DB untouched.

## Named seams (not built, not stubbed with placeholder machinery)

Analyst-side converter/live feed (G6); **endpoint advertising / cross-connection (the
system configuration routine)**; tiers 1/3/4 + setup-runner tier assembly; sub-request
re-activation; MPSC concurrency; WAL coupling. Each a later, separate pass.
