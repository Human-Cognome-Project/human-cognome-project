# HANDOFF — reload pointer for the next session

**For:** the next db_kernel session. **Incoming direction (Patrick, 2026-09-19):
a design *discussion* of the core data flows, and possibly redefining the command
structure of the system overall to a cleaner baseline — see "Incoming direction"
below. Patrick drives it; it is NOT a build, and it supersedes "start on the cache
manager" as the entry point.** **Branch:** `dbkernel-design-checkpoint`, through
commit `90f37ff`.
One read of this file should be the whole reload — pull individual docs by
name below only as the work touches them.

Verify any status/commit claim here against the live repo before building on
it (`git log --oneline`, `git show -s --format=%B <hash>`) — this file is a
snapshot, not a live query.

---

## Incoming direction — core data flows / command-structure baseline

**(Patrick, 2026-09-19 — this is the entry point; it supersedes "start on the
cache manager.")** The next context is a **design discussion, not a build**:
examine the **core data flows**, and possibly **redefine the command structure of
the system overall to a cleaner baseline.** Patrick drives it. It may reshape the
record-tier command surface (the verbs + IR below), so it **precedes** committing
further to the cache-manager build — the work streams further down remain
available but are **downstream of / informed by** this discussion; do not start
building them until the baseline settles.

Current command-structure baseline to reconsider *from*:
- **Verbs:** `DECLARE_RECORD`, `READ_RECORD`, `MOVE_RECORD`, `ADD_CONNECTION`,
  `DELETE_RECORD`, `DELETE_CONNECTION` — a nested/arrayed **in-process IR**, run
  via `dispatch/` (the arraying executor); external wire deferred (G6). Full
  surface: `API.md` §2; IR in `command/command_ir.h`.
- **Core data flow today:** analyst → command IR → `dispatch` → verb cores →
  `hcp3_core` (synchronous record tier); the deferred cross-work (reciprocals,
  mass) surfaces as the WAL manager's obligation topology → cache manager. Read is
  only-follow; identity is the address.
- **Scope of "system overall" is TBD with Patrick** — the db_kernel record-tier
  command surface only, or wider (analyst-facing layer, cross-kernel commands, the
  field engine, the whole HCP command vocabulary). Confirm scope first.

---

## Current state

- **Record tier — COMPLETE.** `codec/`, `schema/`, `controller/`, `command/`,
  `declare/`, `read/`, `update/`, `dispatch/`, `seed/` — all built, tested
  green against a live disposable `hcp3_core`. Full reference: `API.md`.
  `ingestion/` is retired (superseded, breadcrumb only).
- **WAL manager — COMPLETE as a tested library; NOT yet an assembled
  runnable binary.** `wal/` kernel set (W-1…W-6) built, every test PASSes,
  package-vetted primary↔adversary, docs current. A pure bookkeeper/observer
  over WAL reports — maintains the **active deferred-work topology** (the
  live open-obligation relation + append-only History) in its own
  `wal_manager` Postgres DB, never `hcp3_core`. See `wal/README.md` (charter,
  file map, build/run) and `wal/USAGE.md` (the consumer contract — this is
  what you read to use it). **Verified (team-lead, this handoff):** `main()`
  exists only in the five `*_test.cpp` harnesses (`grep -n "^int main" wal/*.cpp`)
  — `wal_book`/`wal_ingest`/`wal_recognize`/`wal_monitor` have no entry point
  of their own, so there is no `wal_manager` process/daemon today, only a
  library the cache manager links against. See "Available work streams" below
  for what standing one up needs.
- **Nothing else in `db_kernel` is a built mechanism.** The cache manager
  (as a runtime) and the swarm/p2p layer are design notes or less — zero
  code exists for either. The cache tier (`RECONCILE`/`UPDATE_CACHE`/
  `REBASE_CACHE`) is a partial exception worth being precise about: `dispatch/`
  has named face entries and dispatch stubs for all three (empty IR structs,
  a branch that returns "not yet implemented") — that much *is* code, but
  it's scaffolding with **no mechanism designed**, exactly as `API.md` §9
  states it. Don't assume otherwise from how firm the prose elsewhere reads;
  `API.md` §9 is the ground truth for built-vs-deferred.

## How to consume the WAL manager

Read `wal/USAGE.md` in full before writing cache-manager code that touches
it; `wal/README.md` if you also need to know how the WAL manager's own
build/tests work. The one-line boundary, repeated here because everything
else follows from it:

**The cache manager writes the primary change and builds its own return
paths; the WAL manager books what returns are owed and watches them land.**

Concretely: the cache manager navigates the WAL manager's open-obligation
relation (`is_open` — PK lookup; `list_open(kind)` / `list_open(kind,
addr_a)` — PK/PK-prefix reads only, never filtered by `source`) to find
what deferred cross-work is still owed, then does that work itself against
its own store. That write becomes the next WAL report; the WAL manager
observes it and closes the matching obligation by identity equality — it is
not told to, it just notices. **A consumer never calls `open` / `close` /
`record_seen`, and never issues raw SQL against the WAL manager's DB** —
those are the WAL manager's own writes, not surface for anything else to use.

## Available work streams

### Cache manager — available (but see "Incoming direction" first)

> **2026-09-19:** the incoming context reconsiders the core data flows /
> command structure *first*; this stream stays available but may be reshaped by
> that discussion — don't start building it until the baseline settles.

Once unblocked: the record tier and the WAL manager are both in place under it. Scope (per `NOTES.md` "Process runtime — the cache manager
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
  now" flag. A momentary priority boost on already-known deferred work
  (found by navigating the WAL manager's topology), not new work; the WAL
  manager's own bookkeeping is unaffected by it. `NOTES.md` "Cache tier",
  `wal/USAGE.md`'s RECONCILE section.
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
| `NOTES.md` | The working design record — governing principles, firmed rulings, build state, open decisions. Source of truth for *intent*; code is source of truth for *behaviour*. |
| `PLAN.md` | The record-tier API-face implementation plan (executed; historical staging record). |
| `API.md` | The record-tier + WAL-manager API reference, and §9's built-vs-deferred table — the fastest way to check what's actually built. |
| `WAL-PLAN.md` | The WAL manager's design (rev. 6) — now marked IMPLEMENTED; its top status blockquote carries the ingest-atomicity trace and ownership ruling. §8/§10 are its own open-items lists. |
| `WAL-IMPL-PLAN.md` | The WAL manager's build-task breakdown (W-1…W-6) — now marked IMPLEMENTED. |
| `wal/README.md` | Working *on* the WAL manager: charter, file map, build/run, disposable-DB convention. |
| `wal/USAGE.md` | Working *with* the WAL manager (consumer contract) — read this before writing cache-manager code against it. |
| `SWARM-NOTES.md` | Preliminary swarm/p2p direction notes — gated, not designed. |
| `HANDOFF.md` | This file. |
