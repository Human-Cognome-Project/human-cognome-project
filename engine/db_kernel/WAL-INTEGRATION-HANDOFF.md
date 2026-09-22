# Handoff — WAL-manager activation integration (draft + execute)

> **✅ EXECUTED (2026-09-22).** Pair-1 built as `wal/wal_kernel.{h,cpp}` +
> `wal_kernel_test.cpp` under the coder+adversary discipline (plan
> `WAL-INTEGRATION-PLAN.md` adversary-vetted; build adversary-vetted SAFE-TO-COMMIT;
> verified green — `PASS wal_kernel_test`, ASan/UBSan clean). **Two Patrick corrections
> narrowed this handoff:** (1) every kernel is **source-blind** — so the "GENUINE OPEN
> POINT" (originator routing) below dissolves: the kernel fills one out-box, topology
> routes; "originator" was reload thinking (each thread repopulates its own owed work
> from durable state — a deferred pass). (2) Serialization / the **"API pair"** is a
> **separate bridge** (a split-mode shuttle between remote boxes), built separately —
> not this pass. Everything else below stood.

**For: a clean context.** You have no prior history with this work; this doc is
self-contained. Your mission: **draft a plan, get it vetted, build it under review,
and commit** — the same coder+adversary discipline the endpoint primitives were built
under (see commit `b97034a` and `endpoint/README.md` for the pattern).

## Mission (one line)

Wire the **built WAL bookkeeper** (`wal/`) onto the **built endpoint substrate**
(`endpoint/`) so the WAL manager runs as a **monitored-endpoint component** that
ingests WAL reports from per-source inboxes, books obligations, and **pushes the owed
reciprocal work to the originating cache manager's outbox** — **fixture-fed**, with
tests, standalone-buildable. This is the "Pair 1" wiring from the design note.

## Read first (governing design + built state)

- **`ENDPOINT-ACTIVATION-NOTES.md`** — the governing design record; its **resolved
  decisions govern**. Read in full, especially:
  - *"WAL-manager comms wiring + swarm indexing"* — Pair 1 (your scope), Pair 2 +
    extra box (NOT your scope), the fan, RECONCILE.
  - *"What the rebase keeps and replaces"* — the **durability doctrine** (boxes are
    volatile; the durable obligation stays in the WAL open-obligation relation;
    completion = ack; at-least-once = ack + reload + idempotent redo).
  - *"Endpoint identity & the dupe-address rule"* — endpoint identity = token_id-space
    address + local slot.
  - The **RECONCILE** section (staging into a pinned box) — context; not the first build.
- **`endpoint/`** (the substrate you build ON — committed `b97034a`): `README.md`,
  `box.h`, `endpoint.h`, `scheduler.h`. Note the ownership rules: **the scheduler is
  the sole enqueue path** (`submit` / `Sender::send`); box is a dumb FIFO; a component
  is a **handler** registered on a box; recycled-slot generation safety.
- **`wal/`** (the bookkeeper you wire IN): `README.md` (charter, file map, build/run,
  the disposable `wal_manager` DB convention) and `USAGE.md` (the door: `open`/`close`/
  `is_open`/`list_open`/`record_seen`; recognition via `wal_recognize`; the monitor
  loop). **The bookkeeping logic stays as-is** — you are wiring it onto boxes and adding
  a push outbox + originator routing + a driver, NOT rewriting recognize/book/close.
- **`wal/USAGE.md`, `WAL-PLAN.md`, `NOTES.md`, `HANDOFF.md`, `API.md`** each carry a
  **2026-09-21 forward flag**: the activation rebase inverts the built PULL/observer
  model to **PUSH**. You are implementing that push. Close semantics are unchanged
  (still self-accounting; durable obligation stays in the relation).

## In scope — buildable now (draft + execute)

1. **WAL manager as a monitored-endpoint component.** Per-source **feed inboxes** (one
   box per WAL source — core / a language shard / the personality DB; per-source FIFO
   ordering falls out of the box, no global order); a **reciprocal-work push outbox** (the
   WAL manager's outbox; its messages land in the **originating cache manager's inbox**).
   The WAL manager's handler: on a report → `record_seen` → `owed()` → `open` each
   obligation (the existing `wal_ingest` logic — **verify its return type**; if it does not
   hand back the owed set, call the pure `owed(report)` yourself to know what to push),
   **and emit that owed reciprocal work to the originating cache manager's inbox.**
   *(Plan decision: with one box per source, keep `wal_monitor`'s single interleaved
   per-source-lsn loop, or drive `wal_ingest` per box.)*
2. **Originator routing.** Route reciprocal work to the cache manager that created the
   initiating entry. Ground truth in the built structs: **`wal::Report` carries
   `Source source`** (an opaque WAL-feed label — core / a shard / the personality DB —
   present at ingest); **`Obligation` is PK-only `(kind, addr_a, addr_b)` and CANNOT and
   MUST NOT carry a source** (the design forbids a source column — per-source order only,
   no source filter). So the routing **key is `Report.source`, resolved at ingest time to
   a standing outbox `EndpointId`** (setup-runner standing wiring: source-label → the
   originating cache manager's inbox endpoint); `Report.source` is an opaque label,
   distinct from the note's endpoint-identity (address + slot), and the resolution is that
   bridge — a direct lookup, never a scan.
   - **GENUINE OPEN POINT — do NOT silently resolve; flag for Patrick.** The push's
     originator identity lives only on the **volatile `Report`**. Re-drive-on-reload
     re-establishes pending work from the **durable open-obligation relation, which has no
     source**, so the originator is not recoverable per open obligation without walking
     History (a scan — forbidden). The ingest-time fixture happy path (route by
     `Report.source`) is buildable now; "re-drive the push to the right originator after a
     reload" is an **unresolved design point**, not a settled guarantee. Build the happy
     path; **raise the reload-originator question** rather than inventing a scan or a
     source column on the obligation.
3. **Runnable process / driver.** A `main`/driver wiring `WalBook` (+ the `wal_manager`
   DB) + the ingest step + the box coupling into a process. **Fixture-fed**, and note the
   **payload decision you hit first:** the built box carries an **opaque `std::string`
   payload** (`box.h`), so a `wal::Report` cannot be dropped in natively. Do NOT serialize
   it — serialization IS the deferred WAL-report→inbox translation seam (out of scope, and
   imported machinery). Instead the inbox/outbox `Message` carries an **in-process handle /
   index into a side arena** of fixture `wal::Report`s (and, on the outbox, the owed-work
   items) — the note sanctions this ("a box may hold a reference into a shared arena rather
   than the payload"). So you reuse the same `wal::fixture::` builders the `wal/` tests use,
   but **injected through a box handle** (today those tests hand native `Report`s straight
   to `wal_ingest`; here they enter via a box). No live feed (see out-of-scope). The
   reciprocal-work outbox consumer is a **test stand-in** (the cache manager is not built):
   the test reads the pushed work out of the arena via the outbox handle and asserts the
   right work lands for the right originator.

**Ships with tests** (project rule): per-source ingest → correct obligations opened;
the settling return closes them (self-accounting, unchanged); reciprocal work is emitted
to the **correct originator's** outbox; recycled-slot / only-follow discipline preserved.

## Out of scope — do NOT build, do NOT invent, do NOT stub with placeholder machinery

- **WAL-report → inbox translation / live feed.** How raw WAL records (Postgres
  logical-decoding output) become inbox messages is its own **focused discussion**
  (tied to the wire-form / G6 gate). Stay **fixture-fed**; do not build a live slot.
- **The cache-manager consumer boxes** (including the extend cache-mgr→swarm-mgr request
  path) — cache-manager layer, forward.
- **Pair 2 (swarm manager: unpack inbound / compose outbound delta packets) and the
  extra box** — the swarm-side facet, deferred/gated.
- **The analyst `verify` function** (the model-consistency integrity gate) — analyst
  layer, forward. The WAL core holds **no confirmation logic** — it only books and routes.
- **NEEDS-PATRICK / design-ahead** (do not resolve): F4 (self-accounting vs the
  cache-manager pending-list — the note argues affirmative, confirm at cache-manager
  design); connection mass-recompute signal; cross-source global-order basis;
  MOVE/rekey of open obligations; async-removal obligation for DELETE. Ingest-atomicity
  (`close()`+`record_seen()` two-write gap) was **ruled off the WAL manager** and
  reassigned to the cache-manager runtime — do not add a transaction surface to `WalBook`.

## Design decisions to honour (from the note)

- **Push, not pull.** The WAL manager emits owed reciprocal work; it does not wait to be
  polled. `list_open` stays as a read surface but is not the delivery path here.
- **Volatile transport, durable relation.** The outbox is volatile notification; the
  durable obligation stays in the WAL open-obligation relation; a lost outbox entry is
  re-driven on reload; close is still by observing the followup write. Do NOT relocate
  durable pending work into a memory box.
- **Only-follow / never-search; no imported machinery / no invented complication; do not
  split or replicate the one shared data space.** Every WAL-DB access stays a bounded PK
  follow (as built).
- **The WAL manager does not originate swarm coverage requests and holds no confirmation
  logic** — receive, book, route.

## Process (the discipline)

1. **Draft a plan** (`WAL-INTEGRATION-PLAN.md` or similar): scope, module/wiring shape,
   the originator-identity decision, tests, named seams. Trace every element to a resolved
   decision in the note; flag where the note is silent rather than inventing.
2. **Vet the plan** with a fresh adversary (complete review, no summarization, interrogate
   completeness, refuse partial). Reconcile findings before building.
3. **Build** (coder) to the reconciled plan; each part standalone-buildable, tests green
   (`g++ -std=c++17 -O2 -Wall -Wextra`, prints `PASS <name>`). DB-backed parts use the
   disposable `wal_manager` DB (see `wal/README.md`), never `hcp3_core`.
4. **Vet the build** with a fresh adversary; reconcile.
5. **Verify green yourself**, then **commit** to `dbkernel-design-checkpoint` (attribution
   trailer per the session's convention) and push.

## Don't-touch

The built record tier (`codec/`…`dispatch/`) and the WAL bookkeeping core's
recognize/book/close logic. You wire the core onto boxes and add the push outbox +
originator routing + a driver — nothing in the bookkeeping algorithm changes.
