# Handoff — WAL-manager activation integration (draft + execute)

> **✅ EXECUTED (2026-09-22).** Pair-1 built as `wal/wal_kernel.{h,cpp}` +
> `wal_kernel_test.cpp` under the coder+adversary discipline (plan
> `WAL-INTEGRATION-PLAN.md` adversary-vetted; build adversary-vetted SAFE-TO-COMMIT;
> verified green — `PASS wal_kernel_test`, ASan/UBSan clean). **Two Patrick corrections
> sharpened this handoff:** (1) **source-blind = LOCATION-blind, not identity-blind** —
> a kernel doesn't know/care WHERE a counterpart sits (local vs remote), but IS aware
> of WHO it receives from and assigns to; every outbox is a specific counterpart's
> inbox, and each kernel has multiple in/out boxes (a mail system). So the "originator"
> wording below is right — the WAL manager assigns reciprocal work to a known
> counterpart (the cache manager) by choosing its out-box. This pass has one
> cache-manager counterpart ⇒ one reciprocal out-box, no selection exercised yet;
> selection-by-who (multiple counterparts) and **reload repopulation** (each thread
> re-derives work-for-counterparts from durable state — WAL re-emits from `list_open`)
> are deferred. (2) Serialization / the **"API pair"** is a **separate bridge** (a
> split-mode shuttle between remote boxes), built separately — not this pass.
> **Everything else below stood EXCEPT the "In scope" §2 "Originator routing," which the
> build superseded** (the kernel never routes on `Report.source`; one fixed
> cache-manager out-box) — §2 carries its own struck-through flag.

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
2. **Originator routing.** ⛔ **SUPERSEDED BY THE BUILD (2026-09-22) — this item was NOT
   built as written; see `WAL-INTEGRATION-PLAN.md`'s *Out-box* section for what was built.**
   The mechanism below (a `Report.source` → outbox-`EndpointId` routing key, resolved
   per report at ingest) is exactly what the source-blind correction rules out: **the
   built kernel never reads `Report.source` to route or branch** (`wal_kernel.h:19-22`,
   `wal_kernel.cpp`) — `source` is only per-source ordering data inside the unchanged
   `WalMonitor`. This pass has **one** cache-manager counterpart, so there is **one**
   fixed reciprocal out-box (that counterpart's inbox), chosen once at construction, with
   no per-report source lookup and no setup-runner source-label→endpoint table. Kept below
   only as the mission's original framing, struck. The paragraph and its "GENUINE OPEN
   POINT" are retained verbatim for provenance:
   > ~~Route reciprocal work to the cache manager that created the
   > initiating entry. … the routing **key is `Report.source`, resolved at ingest time to
   > a standing outbox `EndpointId`** (setup-runner standing wiring: source-label → the
   > originating cache manager's inbox endpoint) … a direct lookup, never a scan.~~
   > - ~~**GENUINE OPEN POINT.** The push's originator identity lives only on the volatile
   >   `Report` … build the happy path; raise the reload-originator question.~~

   **How it actually resolved:** "originator" was reload/system-wide thinking, not
   steady-state routing (Patrick, 2026-09-22). Steady state: the WAL manager knows its
   counterpart (the cache manager) and fills that counterpart's out-box — source-blind =
   location-blind, not identity-blind. Selection-by-*who* only arises with multiple
   counterparts (deferred). The reload question is also deferred and answered the same way
   the note now records it: on reload each kernel **repopulates the work it holds for its
   counterparts from its own durable state** — the WAL manager re-emits owed work from
   `list_open` (a bounded PK read, no History scan, no source column).
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
