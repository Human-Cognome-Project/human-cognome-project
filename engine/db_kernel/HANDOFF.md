# HANDOFF — reload pointer for the next session

> **⚠ Reload pointer (2026-09-22).** The core-data-flows discussion HAPPENED and
> produced the **new messaging system**: a monitored-endpoint activation substrate
> (`engine/db_kernel/endpoint/`, commit `b97034a`) plus the WAL manager wired onto it as
> a **monitored-endpoint kernel** (`wal/wal_kernel.{h,cpp}`, commits `3aca2ac`/`cf7e0c6`).
> **Next entry point (Patrick, 2026-09-22): realign the main db_kernel / cache-manager
> design with this new messaging system** — see "Incoming direction" below. Branch
> `dbkernel-design-checkpoint`.
>
> **Progress since (2026-09-23):** the four-tier box-priority structure is pinned and
> **tier 2 — the analyst reaction body — is BUILT** (`dbmanager/`, commit `8614b43`);
> RECONCILE was removed from the dispatch surface (analyst → WAL manager); the cache
> structure is captured **forming** in `NOTES.md`. Remaining: tiers 1/3/4, the
> config/advertising routine, the cache structure build-out. See "Current state" and the
> "Progress" note under the Cache-manager work stream below.

**For:** the next db_kernel session. **Incoming direction (Patrick, 2026-09-22):
realign the cache-manager design with the new messaging system** — the core-data-flows
discussion that was pending is done and gave us the activation substrate + the WAL
Pair-1 kernel; the cache-manager design now has to be brought onto that basis. See
"Incoming direction" below. **Branch:** `dbkernel-design-checkpoint`, through commit
`cf7e0c6`.
One read of this file should be the whole reload — pull individual docs by
name below only as the work touches them.

Verify any status/commit claim here against the live repo before building on
it (`git log --oneline`, `git show -s --format=%B <hash>`) — this file is a
snapshot, not a live query.

---

## Incoming direction — realign the cache-manager design with the messaging system

**(Patrick, 2026-09-22 — this is the entry point.)** The pending core-data-flows /
command-structure discussion (Patrick, 2026-09-19) HAPPENED and settled into the
**monitored-endpoint activation model** — the "new messaging system." The next context
**realigns the main db_kernel / cache-manager design with it.** Read the messaging
model first, then rework the cache-manager design (mostly still design; **tier 2 —
the analyst reaction body — is now built**, `dbmanager/`, 2026-09-23) so it
sits natively on boxes rather than on the old active-instruction / polling shape.

**The messaging model to realign onto (read these first):**
- `ENDPOINT-ACTIVATION-NOTES.md` — the governing model. Kernels couple **only through
  in/out boxes**; **data arriving at a box is the activation** (no instruction, no
  poll). **Every outbox is a specific counterpart's inbox**; **each kernel has multiple
  in/out boxes** (an interactive internal-state mail system). **Source-blind =
  LOCATION-blind, not identity-blind:** a kernel knows *who* it talks to, never *where*
  they physically sit (the API-pair shuttle abstracts local vs remote in split mode).
  Priority is box-granular ("do-next, not preempt"); a request names its own return
  endpoint; RECONCILE is a pinned normally-empty top-priority box the WAL manager stages
  into.
- `endpoint/README.md` + headers — the built substrate (`box`, `endpoint` registry,
  `scheduler` = sole enqueue path). Single-threaded cooperative first cut.
- `WAL-INTEGRATION-PLAN.md` + `wal/wal_kernel.{h,cpp}` — the worked example: a real
  kernel (the WAL manager) wired onto the substrate. Use it as the pattern for how the
  cache manager becomes a kernel.

**What realigning the cache-manager design has to work through (the open items below
under "Cache manager" all still stand, now re-framed onto boxes):**
- The cache manager is a **kernel with multiple in/out boxes**: its **inbox** is fed by
  the WAL manager's reciprocal-work outbox (built) + (later) the unpacked-swarm-work box;
  it does the owed work against its own store; its own writes become the next WAL
  reports (closing obligations by observation — unchanged). Reconcile is a pinned box it
  drains first. Extend (pull new granularity) is a cache-mgr→swarm-mgr **request** box
  (forward).
- **F4** — whether self-accounting replaces the cache manager's own drainable
  pending-list. The activation model points **F4-affirmative** (the denied list is the
  cache manager's own; the durable obligation stays in the WAL relation) — confirm it
  as part of this realignment. `NOTES.md` "Process runtime"; `ENDPOINT-ACTIVATION-NOTES.md`
  "What the rebase keeps and replaces".
- **Ingest-atomicity** — the observe→act cross-kernel-cycle atomicity was ruled OFF the
  WAL manager and reassigned here; the realignment owns where that transaction boundary
  sits on the box model.
- **Reload repopulation** — each kernel (cache manager included) re-derives the work it
  holds for its counterparts from durable state on restart; design where the cache
  manager's durable pending state lives and how it repopulates its outboxes.
- **Scope of "system overall"** — the 2026-09-19 note left open whether the rebase
  touched just the record-tier command surface or wider. The activation note settled the
  **cross-kernel** command layer (the setup runner IS the command structure; no separate
  command protocol survives). Confirm with Patrick whether the analyst-facing layer / the
  wider HCP vocabulary is in this realignment's scope.

Current record-tier command baseline (unchanged, the reaction bodies a kernel runs):
- **Verbs:** `DECLARE_RECORD`, `READ_RECORD`, `MOVE_RECORD`, `ADD_CONNECTION`,
  `DELETE_RECORD`, `DELETE_CONNECTION` — a nested/arrayed **in-process IR**, run
  via `dispatch/`; external wire deferred (G6). Full surface: `API.md` §2; IR in
  `command/command_ir.h`. On the activation model these survive as **reaction bodies**,
  not dispatched commands.

---

## Current state

- **Record tier — COMPLETE.** `codec/`, `schema/`, `controller/`, `command/`,
  `declare/`, `read/`, `update/`, `dispatch/`, `seed/` — all built, tested
  green against a live disposable `hcp3_core`. Full reference: `API.md`.
  `ingestion/` is retired (superseded, breadcrumb only).
- **WAL manager — COMPLETE as a tested library, AND wired onto the messaging
  system (Pair-1 kernel, 2026-09-22).** `wal/` kernel set (W-1…W-6) built, every
  test PASSes, package-vetted primary↔adversary. A pure bookkeeper/observer over
  WAL reports — maintains the **active deferred-work topology** (the live
  open-obligation relation + append-only History) in its own `wal_manager` Postgres
  DB, never `hcp3_core`. See `wal/README.md` (charter, file map, build/run) and
  `wal/USAGE.md` (consumer contract). **Now also built as a monitored-endpoint
  kernel** — `wal/wal_kernel.{h,cpp}` + `wal_kernel_test.cpp` (commits
  `3aca2ac`/`cf7e0c6`; 30 checks PASS, ASan/UBSan clean; coder+adversary discipline;
  plan `WAL-INTEGRATION-PLAN.md`): reads a fixture `Report` off a per-source in-box →
  books via the unchanged path → pushes owed reciprocal work to the cache-manager
  out-box. **Still fixture-fed** (no live report feed; `main()` outside the tests
  only in the kernel test's driver). Deferred: reload repopulation, the API-pair
  transport bridge, the live feed, Pair-2/swarm coupling, and the cache-manager
  consumer. This is the pattern the cache-manager realignment follows.
- **Tier 2 of the db/cache manager IS now built (2026-09-23).** `dbmanager/`
  (`db_manager_kernel.{h,cpp}` + test + README) — the analyst reaction body: the
  record-tier verbs run as reaction bodies over the shared `Controller`, reusing
  `dispatch/` verbatim, each `Result` returned to the request's caller-supplied
  `reply_to`. Built to `TIER2-PLAN.md` under the coder+adversary discipline;
  `PASS db_manager_kernel_test`, 38/38, ASan/UBSan clean; commit `8614b43`. So the
  db/cache-manager runtime is **no longer zero-code** — tier 2 exists.
- **The rest of the db/cache manager runtime and the swarm/p2p layer are still
  design notes or less.** Tiers 1 (reconcile box), 3 (pending-work consumer /
  file-now-wire-later), 4 (standing maintenance), the config/advertising routine,
  and the cache structure itself (view-composer design captured **forming** in
  `NOTES.md`, not built) remain unbuilt. The cache tier (`UPDATE_CACHE`/`REBASE_CACHE`)
  is a partial exception worth being precise about: `dispatch/`
  has named face entries and dispatch stubs for the **two** (empty IR structs,
  a branch that returns "not yet implemented") — that much *is* code, but
  it's scaffolding with **no mechanism designed**, exactly as `API.md` §9
  states it. **`RECONCILE` was removed from the dispatch surface (2026-09-22,
  adversary-vetted CLEAN, `dispatch_test` 31/31 green):** it is no longer a
  db/cache-manager verb — the analyst messages the WAL manager directly and
  the WAL manager promotes the relevant pending queue into a priority in-box
  (`ENDPOINT-ACTIVATION-NOTES.md` "RECONCILE"). Don't assume otherwise from how firm the prose elsewhere reads;
  `API.md` §9 is the ground truth for built-vs-deferred.

## How to consume the WAL manager

Read `wal/USAGE.md` in full before writing cache-manager code that touches
it; `wal/README.md` if you also need to know how the WAL manager's own
build/tests work. The one-line boundary, repeated here because everything
else follows from it:

**The cache manager writes the primary change and builds its own return
paths; the WAL manager books what returns are owed and watches them land.**

**On the messaging system this is now a PUSH, not a pull (2026-09-22).** The WAL
kernel (`wal/wal_kernel.{h,cpp}`) emits owed reciprocal work to the cache-manager
**out-box** (= the cache manager's in-box) as reports arrive — the cache manager no
longer polls `list_open` to discover work in steady state. `is_open` / `list_open`
(`list_open(kind)` / `list_open(kind, addr_a)` — PK/PK-prefix reads only, never
filtered by `source`) stay as the **read** surface (used for reload re-emit, and for
any reader that needs the current relation), not the delivery path. The cache manager
does the owed work against its own store; that write becomes the next WAL report; the
WAL manager observes it and closes the matching obligation by identity equality — not
told to, it just notices. **A consumer never calls `open` / `close` / `record_seen`,
and never issues raw SQL against the WAL manager's DB** — those are the WAL manager's
own writes. The realignment (below) reworks the cache manager as a kernel whose in-box
this out-box feeds.

## Available work streams

### Cache manager — THE ACTIVE STREAM: realign the design onto the messaging system

> **2026-09-22:** this is the entry point (see "Incoming direction"). The core-data-flows
> discussion settled into the activation model, so the cache-manager design is **reworked
> onto boxes** before it is built — it becomes a kernel with multiple in/out boxes (fed by
> the WAL manager's reciprocal-work out-box), not a poller of `list_open`. The open items
> below all still stand; they are now worked through *on the box model*. This is a
> **design** step first (realign), then build under the coder+adversary discipline.
>
> **Progress (2026-09-23):** the four-tier box-priority structure is pinned (reconcile ▸
> analyst ▸ pending ▸ standing-maintenance), and **tier 2 (the analyst reaction body) is
> BUILT** as `dbmanager/` (commit `8614b43`; `NOTES.md` "Tier 2 — analyst reaction body").
> RECONCILE was removed from the dispatch surface (analyst → WAL manager). The cache
> **structure** itself is captured **forming** in `NOTES.md` ("Cache manager — view
> composer") — not built. Remaining: tiers 1/3/4, the config/advertising routine, the
> cache structure build-out.

Scope (per `NOTES.md` "Process runtime — the cache manager
is a complete runtime"): a complete, standalone runtime process that owns
the DB, serves analyst input/output, and carries the **warm working
cache** fed from cold storage — with cross-processing (wiring, mass) as a
pending workstream behind primary I/O. It navigates the WAL manager's
topology (above) to do that deferred work.

It **owns** these open items — none were resolved by building the WAL
manager, and each needs a decision or design as part of building this
stream, not before it:

- **Runnable binary — READY-NOW, small, a reasonable place to start.** The
  WAL manager is a complete, tested *library*, not yet an assembled process:
  `main()` exists only in `wal/*_test.cpp`; `wal_book` / `wal_ingest` /
  `wal_recognize` / `wal_monitor` have no entry point of their own. Standing
  one up means writing a driver/`main` that wires `WalBook` (against the
  `wal_manager` DB) + `WalMonitor` + `wal_ingest` together. This is buildable
  **today**, but only as a **fixture-fed** demo/smoke binary — it would feed
  itself `wal::fixture::` reports (`wal_report.h`), the same as every
  existing test does, because there is no live report source yet. A
  **real** (non-fixture) runnable process is gated on the next item.
- **Live report feed (bucket B, design-ahead; gates a real runnable
  process)** — Postgres logical-decoding ingest, or whatever the **G6**
  external wire form ends up being (`API.md` §9's G6 row; `WAL-PLAN.md` §1
  bucket B). Until one of these exists, a standalone WAL-manager binary has
  no real input — every report is an in-process fixture. This is the actual
  gate on a live `wal_manager` process, not the driver/`main` above, which
  is separable and buildable now.
- **F4** — whether the WAL manager's self-accounting (no drain, close on
  observing the followup write) *replaces* this runtime's own
  pending-list / file-now-wire-later runtime, or only governs the WAL
  manager's own view of it. `NOTES.md` "Process runtime", the "⚠ Revisit"
  bullet.
- **RECONCILE** — the analyst-raised "this deferred cross-work is priority
  now" flag. **No longer a db/cache-manager verb (removed 2026-09-22):** the
  analyst messages the **WAL manager directly**, which navigates its own
  open-obligation topology (only-follow) to **move** the relevant already-known
  deferred work into a pinned priority box; the cache manager only *drains* it.
  A momentary priority boost on existing work, not new work. The WAL manager's
  open→close obligation ledger is unaffected, but it does take the action of
  selecting/placing the reconcile work. `ENDPOINT-ACTIVATION-NOTES.md`
  "RECONCILE"; `wal/USAGE.md`'s RECONCILE section.
- **Ingest-atomicity ownership** — the transaction boundary around the
  observe (WAL report) → book (act on it) cycle. Raised during WAL-manager
  package review, traced, and **ruled dropped from the WAL manager**
  (Patrick, 2026-09-19: no transaction surface added to `WalBook`) —
  **explicitly reassigned to this stream**, because cross-kernel-cycle
  atomicity is the orchestrating runtime's job, not the bookkeeping door's.
  The gap is real, not illusory: a crash between the WAL manager's
  `close()` and `record_seen()` can leave a settlement silently
  mis-recorded in History. Full trace: `WAL-PLAN.md`'s status blockquote;
  also noted in `NOTES.md`'s F4 bullet and `wal/USAGE.md`.
- **Attach / granularity** — what the cache manager attaches to, and at
  what granularity, in the local storage swarm (which DBs/regions it
  pulls into the warm tier). Named in `SWARM-NOTES.md`'s division-of-labour
  note ("the cache manager decides what to attach / at what granularity")
  but not designed — it's this stream's decision, made before the swarm
  side can build against it.
- **Mass aggregation model + connection mass-recompute signal** — how mass
  is actually computed (sum vs. centroid, nested aggregation — deferred,
  `NOTES.md` "Cache tier" / `API.md` §9), and how a dirtied *existing*
  token's mass gets marked for recompute (an `ADD_CONNECTION` writes no
  `token.mass` and the WAL manager's NULL-signal only covers a *new-token*
  DECLARE — `WAL-PLAN.md` §5/§8). Both are this runtime's to design; the
  WAL manager only ever monitors the resulting `token.mass` write land.

**Future WAL-manager extension (not owned outright today — contingent on
bucket-B groundwork this stream may lay):** `WAL-PLAN.md` §7 names two more
WAL-manager-adjacent items, "named, not designed," each blocked on something
that doesn't exist yet rather than ready to build now — **MOVE_RECORD/rekey
of open obligations** (a MOVE changes the `token_id`s an open obligation
keys on; today MOVE is atomic so nothing is owed, but re-keying a live
obligation under a moved `token_id` is unresolved) and an **async removal
obligation for DELETE** (would need an INSERT/DELETE polarity bit the bare
axis identity doesn't carry, since a `member_of` add and removal of the same
pair compute the same key — blocked on the async reciprocal existing at
all). Neither is this stream's to build today; noted here so they aren't
lost, and to revisit once the async reciprocal groundwork above makes them
concrete.

### Swarm / p2p layer — PRELIMINARY, GATED

Direction-only notes, not designed, not built, not adversary-firmed — see
`SWARM-NOTES.md` in full. Explicitly **gated on examining the WAL data
shape first** (Patrick), which in practice means gated on the cache manager
stream existing enough to show what that shape actually is in use. Open,
per `SWARM-NOTES.md`'s "To pin" / "Parked" sections:

- The **delta-span vs. state-range** chunking fork (sync/update unit vs.
  bulk-coverage unit) and the **hash-key granularity hinge** (per WAL
  entry? per piece? per shard?) — the concrete decision the whole
  torrent-style mapping turns on.
- The symmetric **tracker/server/client** topology (every node is all
  three, reusing the core distillation / content-addressing / pull-on-
  request structures already in place) — direction is written down, not
  built.
- The **stdin/stdout-with-monitors** coupling mechanism (preliminary,
  lean-coupling idea, not implemented).
- **Conflict resolution** — parked outright; detection falls out of
  content-addressing (divergent hashes at the same identity), but the
  resolution policy is undesigned and Patrick holds ideas not yet given.

### Deferred record-tier seams — Patrick's call / as-needed

Not part of either stream above; pick up only if/when they block something
or Patrick prioritizes one. Full detail: `API.md` §9, `NOTES.md` "Open
decisions".

- **G4** — next-slot mechanism (analyst-supplied start vs. per-trunk
  cursor); also gates manager-placed mint.
- **G5** — block boundaries past hex couplets (trunk map extent).
- **G6** — external wire/transport format (verbs currently dispatch over
  the in-process IR only).
- **G10** — `DELETE_RECORD` FK-dependent policy on a still-referenced
  token (currently held as reject-on-referenced; not fully resolved).
- Deferred models: mass aggregation (folds into the cache-manager stream
  above), notation derivation, the prose→token_id swap, extrapolation /
  relative-placement rules.

## Doc pointers

| Doc | What's in it |
| --- | --- |
| `ENDPOINT-ACTIVATION-NOTES.md` | **The messaging model** — monitored-endpoint activation, boxes as the sole coupling, source-blind=location-blind, the mail-system, priority, RECONCILE, endpoint identity. Read FIRST for the realignment; its resolved decisions govern. |
| `endpoint/README.md` (+ `box.h`/`endpoint.h`/`scheduler.h`) | The **built** local activation substrate (commit `b97034a`): dumb-FIFO box, endpoint registry, scheduler as sole enqueue path. Pure C++17, no DB. |
| `WAL-INTEGRATION-PLAN.md` | The **worked example** — the adversary-vetted plan for wiring the WAL manager onto the substrate (BUILT). The pattern the cache-manager realignment follows. |
| `WAL-INTEGRATION-HANDOFF.md` | The (EXECUTED) mission record for the WAL kernel; carries the source-blind + API-pair clarifications. |
| `NOTES.md` | The working design record — governing principles, firmed rulings, build state, open decisions. Source of truth for *intent*; code is source of truth for *behaviour*. |
| `PLAN.md` | The record-tier API-face implementation plan (executed; historical staging record). |
| `API.md` | The record-tier + WAL-manager API reference, and §9's built-vs-deferred table — the fastest way to check what's actually built. |
| `WAL-PLAN.md` | The WAL manager's design (rev. 6) — IMPLEMENTED; top status carries the ingest-atomicity trace + ownership ruling and the Pair-1-push-built flag. §8/§10 are its own open-items lists. |
| `WAL-IMPL-PLAN.md` | The WAL manager's build-task breakdown (W-1…W-6) — IMPLEMENTED. |
| `wal/README.md` | Working *on* the WAL manager: charter, file map, build/run, disposable-DB convention (now includes `wal_kernel`). |
| `wal/USAGE.md` | Working *with* the WAL manager (consumer contract) — read before writing cache-manager code against it; carries the push-model-built flag. |
| `SWARM-NOTES.md` | Preliminary swarm/p2p direction notes — gated, not designed. |
| `HANDOFF.md` | This file. |
