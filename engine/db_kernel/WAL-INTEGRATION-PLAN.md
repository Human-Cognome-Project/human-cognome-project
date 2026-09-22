# WAL-manager activation integration — plan (draft, for adversary vetting)

**Status: BUILT + VERIFIED (2026-09-22).** Built against the built `endpoint/` substrate
(commit `b97034a`) and the built `wal/` bookkeeper, as `wal/wal_kernel.{h,cpp}` +
`wal_kernel_test.cpp`. Drafted 2026-09-22 MDT and reconciled the same day against a
fresh-adversary plan review (SHOULD-FIX 1–4 + CONSIDER 5–8 all folded in:
exception/failure decision, originator reconciliation, inline-DB runtime property,
owned-shape + priority pinned, tests #6/#7 added). A fresh build-adversary returned
SAFE-TO-COMMIT (ASan/UBSan clean, tests value-based not proxies); verified green
independently — `PASS wal_kernel_test`, 30 checks, exit 0. Governed by
`ENDPOINT-ACTIVATION-NOTES.md` (resolved decisions) and `WAL-INTEGRATION-HANDOFF.md`
(the mission). Canadian English. Prose/draft record; code + `API.md`/`NOTES.md` remain
the source of truth for what is *built*.

## Mission (one line)

Wire the built WAL bookkeeper (`wal/`) onto the built endpoint substrate
(`endpoint/`) so the WAL manager runs as a **monitored-endpoint kernel**: it reads
WAL reports off its in-box, books obligations (existing logic, unchanged), and
pushes the owed reciprocal work into its out-box — **fixture-fed**, tested,
standalone-buildable.

## Governing reframe (Patrick, 2026-09-22) — narrows the handoff

Two corrections to `WAL-INTEGRATION-HANDOFF.md`, both **simplifying**:

1. **Every kernel is source-blind = LOCATION-blind, not identity-blind, and couples
   only through in/out boxes (Patrick, 2026-09-22).** The kernel does **not** know or
   care *where* a counterpart physically sits (local vs remote — the separate API-pair
   shuttle makes remote transparent). It **is** aware of *who* it receives from and
   *who* it assigns work to. **Every outbox is a specific counterpart's inbox** — the
   box *is* the addressing; choosing the outbox chooses the recipient (directed
   point-to-point edges). **Each kernel has multiple in/out boxes** — an interactive
   internal-state mail system.
   - The WAL manager is special only in that its **primary stream is
     internally-generated reports** driving **internal** work → the **cache manager**;
     **external** work is fed by/to the **swarm manager** (Pair 2, deferred). So the WAL
     manager's out-boxes are counterpart-specific: cache-manager reciprocals (this
     pass), and later swarm-manager deltas.
   - **What this narrows in the handoff's "GENUINE OPEN POINT":** the WAL manager DOES
     assign reciprocal work to a known counterpart — it is not identity-blind. In this
     single-local-instance pass there is exactly **one** cache-manager counterpart, so
     one reciprocal out-box and **no selection is exercised yet**. Selection-by-*who*
     (which of several cache managers, or cache-vs-swarm) becomes real with multiple
     counterparts — deferred. What is genuinely deferred to a later pass is **reload
     repopulation**: on restart/crash each thread (the WAL manager included)
     repopulates the work it holds for its counterparts from durable state (Patrick,
     2026-09-22). The kernel never needs a counterpart's *location* — only *who* — and
     "who" is carried by which out-box.

2. **Serialization / "API" is a separate matched-pair bridge, built separately.**
   Where the system runs **split** (across a process/machine boundary), an **API
   pair connects the two boxes directly — a shuttle service** moving serialized
   content between a local out-box and a remote in-box. A kernel cannot tell whether
   a box's counterpart is native-local or bridged-remote. **None of that bridge is
   this pass**, and this pass builds no serialization, no transport, no thread split.

**What this pass does NOT touch (settled by the reframe):** the WAL manager's own
durable store (`WalBook`/`wal_manager` Postgres) is the kernel's **internal state**,
not an "API pair" — it stays exactly as built. The "API aspect" Patrick carves out
is the remote box-shuttle, which is separate.

## Scope — in

A single new kernel-set part in `wal/` (the activation wiring), plus a driver and a
test. The **bookkeeping algorithm does not change** — `wal_recognize`, `wal_book`,
`wal_ingest`, `wal_monitor` are reused as-is.

### 1. The WAL manager as a monitored-endpoint kernel

New files `wal/wal_kernel.{h,cpp}` (name TBD-confirm in review): the reaction body
registered as a `scheduler::Handler` on the WAL manager's in-box(es).

- **In-box(es): a per-source SET of standing boxes.** One box per WAL source (core /
  a language shard / the personality DB). Per-source `lsn` order is the **box FIFO**
  (structural); cross-source independence is just the boxes being separate. **The
  kernel never reads `Report.source` to route or branch** — `source` is used only as
  report-carried *ordering data* by the reused `WalMonitor` high-water guard (an
  integrity check that a source's own stream arrived in order), never as a routing
  or behaviour decision. Source-blindness is preserved: the coupling does not depend
  on which source fed which box.
- **Out-box: one standing reciprocal-work box = the cache-manager counterpart's
  inbox.** The kernel pushes one owed-work item per obligation `owed(report)` returns
  into this out-box, which **is** the cache manager's inbox (every outbox is a
  counterpart's inbox). This pass has exactly one counterpart (one cache manager), so
  there is one reciprocal out-box and no destination *selection* to make yet — but the
  out-box IS the counterpart addressing, not a location-agnostic drop. With multiple
  counterparts (several cache managers, or the swarm manager for external work),
  the kernel would select the counterpart's out-box by *who* the work is for; that
  selection is deferred with the multi-counterpart / reload passes. The counterpart's
  physical location (local, or remote via the API-pair shuttle) is never the kernel's
  concern.
  - **Reconciliation with `ENDPOINT-ACTIVATION-NOTES.md`'s Pair-1 "originator"
    wording (Patrick, 2026-09-22).** The note (2026-09-21) says the outbox "emits them
    to *that originator's inbox* (which cache manager falls out of the initiating
    entry's own origin)" — which could read as *selecting among cache managers by
    report data*. Patrick's clarification: **"originator" was system-wide RELOAD
    thinking, not steady-state routing.** Every thread — the WAL manager included —
    repopulates the outbound work it holds for others from its own durable state on
    reload. The WAL manager is itself an originator: on reload it re-derives the
    reciprocal work it owes cache managers **from its own durable open-obligation
    relation** (`list_open`) and re-emits it to the out-box. So:
    - **Steady state (this pass): one known counterpart.** The kernel fills the
      cache-manager out-box (= that counterpart's inbox). Source-blind means
      location-blind: the counterpart is known (who), only its location is abstracted.
      With one cache manager there is no selection to make; with several, the kernel
      selects the counterpart's out-box by *who*.
    - **Reload (separate pass, deferred): the WAL manager repopulates.** This is how
      "a lost out-box entry is re-driven on reload" actually happens — the WAL manager
      re-emits from `list_open`, not an external actor. This design **does not
      foreclose it**: repopulation is the *same* `Sender::send`-to-out-box path,
      seeded from `list_open()` instead of an incoming report — an only-follow bounded
      PK read, no new surface. It is **not built here** (Patrick: reload is addressed
      separately) but is noted as the intended reload hook so the build stays
      compatible with it.
    **Action on commit:** flag `ENDPOINT-ACTIVATION-NOTES.md`'s Pair-1 section as
    narrowed by the 2026-09-22 source-blind reframe, the same treatment
    `wal/USAGE.md` etc. already carry.
- **The reaction body** (per report drawn from the in-box):
  1. Resolve the in-box `Message`'s **arena handle** → the `wal::Report` (see §3).
  2. `owed_set = wal::owed(report)` — the pure recogniser (no DB). This is what the
     kernel will push.
  3. Book it: `WalMonitor::feed(report)` (which calls `wal::ingest` → `open` each
     owed → `close` the settled identity → `record_seen` **last**, and advances the
     per-source high-water mark). **Unchanged built logic.** (`ingest` recomputes
     `owed()` internally — `wal_ingest.cpp` — so step 2's `owed()` is a deliberate,
     accepted redundancy: `owed()` is pure, so it is cheaper and safer to call it
     twice than to change the don't-touch `ingest`/`monitor` logic to hand the set
     back. The coder must NOT "optimize" this by editing the reused logic.)
  4. For each obligation in `owed_set`, push an owed-work item onto the out-box via
     `Sender::send` (the sole enqueue path) — placing a handle into the out-arena
     (§3). A settling report (`kReturn*`/`kMassFill`) has an **empty** `owed_set`, so
     it pushes nothing — correct: a return settles, it owes no new reciprocal.

  **Order + failure decision (owed → feed → push).** The body calls `owed()` first,
  `feed()` second, push last. `WalMonitor::feed()` throws `std::invalid_argument` on
  an out-of-order **same-source** report (`wal_monitor.cpp`). That throw is **not
  caught** — it propagates out of the handler, and since `Scheduler::step()`
  (`scheduler.cpp`) wraps the handler in **no** try/catch, it aborts the entire
  `run_until_idle()` run. This is **deliberate and correct for this pass**: an
  out-of-order same-source feed is a **feeder/programming bug, not runtime data**, and
  the substrate's fail-loud stance is the right one — the WAL kernel does **not**
  swallow it, does **not** invent an error box (that would be imported machinery), and
  does **not** drop-and-continue. Because `feed()` runs before the push, a thrown
  report contributes **nothing** to the out-box. Two things this obliges:
  (a) the plan records that **a throwing handler aborts the whole scheduler run** — a
  *substrate-level* behaviour (the `endpoint/` tests never exercise a throwing
  handler, so this build is the first to rely on it), acceptable for a standalone
  fixture test but noted as a property, not hidden; (b) a test proves the out-box
  received nothing for the offending report (test #6).

### 2. Owed-work item shape (out-box payload)

Each owed-work item = the **`wal::Obligation` identity** `(kind, addr_a, addr_b)` —
exactly what the consumer needs to know which reciprocal to write (a `token_child`
for a structure debt, a `member_of` for a membership debt, a mass fill for a mass
debt). No new type; reuse `wal::Obligation`. Held in the out-arena; the box `Message`
carries the handle.

### 3. The arena/handle mechanism (no serialization)

`box::Message.payload` is an **opaque `std::string`** and `box::Box` never inspects
it. Per `ENDPOINT-ACTIVATION-NOTES.md` ("a box may hold a **reference** into a shared
arena rather than the payload") and the handoff, we do **not** serialize a `Report`
or an `Obligation` — serialization is the deferred bridge seam.

- **In-arena:** a `std::vector<wal::Report>` the test/driver owns and seeds with
  `wal::fixture::` reports (the same builders every `wal/` test uses).
- **Out-arena:** a `std::vector<wal::Obligation>` the kernel appends owed-work items
  to.
- **Handle in the payload:** the arena index, rendered as a decimal string
  (`std::to_string(index)`), parsed back on the receiving side. This is a **handle,
  not a wire encoding** — none of the struct's fields are serialized; the index is
  just how a handle rides the built `std::string` payload slot. (Reviewers: confirm
  this is within the sanctioned "reference into a shared arena" and not smuggled
  serialization. Alternative considered: a dedicated non-owning handle registry — but
  the arena-index string is the minimal fit to the built `Message` and is what the
  handoff prescribes.)
- The arenas are **in-process** and shared by driver + kernel; they are NOT a
  durable store and NOT the obligation relation. They model, for this fixture-fed
  cut, what a real per-source feed and a real consumer would each hold their side of.

### 4. Ownership shape, priority, and the driver

**Ownership (pinned so the coder does not have to guess).** The reaction body is a
**class** — `wal::WalKernel` — holding non-owning references to: the in-arena
(`const std::vector<Report>&`), the out-arena (`std::vector<Obligation>&`), the
`WalMonitor&` (which holds the `WalBook&`), and the out-box `EndpointId`. It exposes
a method `scheduler::Handler make_handler()` (or `handle(const Message&, Sender&)`
bound via a lambda) so the same object's state is shared across every in-box the
handler is registered on. The driver/test owns the arenas, `WalBook`, `WalMonitor`,
`Registry`, `Scheduler`, and every `Box`, and guarantees all of them outlive the
`run_until_idle()` call — matching `register_box`'s "caller-owned, must outlive the
scheduler" contract (`scheduler.h`) and `WalMonitor`'s non-owning `WalBook&`.

**Priority.** One priority level suffices for this cut (`num_levels = 1`): per-source
order comes from the box FIFO, not from priority, and RECONCILE's pinned high-priority
box is out of scope here. All in-boxes register at level 0; the out-box is an
**unregistered** standing box (no handler) drained directly by the test stand-in,
exactly as `dataflow_test.cpp` drains an ephemeral return box.

**Driver.** `wal/wal_kernel_main.cpp` (or fold into the test if a separate binary is
premature — confirm in review): wires `Registry` + `Scheduler` + the in-box set + the
out-box + a `WalBook` (against the disposable `wal_manager` DB) + a `WalMonitor` +
`WalKernel`, registers the handler on each in-box, seeds the in-arena from fixtures,
and `run_until_idle()`. Single-threaded cooperative scheduler, as built (no MPSC, no
wake-on-arrival — the substrate's named deferred seams).

**Runtime property to state plainly (not a defect, a consequence).** `WalBook`'s
libpq calls are **synchronous**, and they run **inside** the run-to-completion
handler on the **single-threaded** scheduler — so a DB round-trip blocks the entire
scheduler until it returns; no other box (including, in a later pass, a pinned
higher-priority RECONCILE box) is serviced meanwhile. This is **acceptable for this
fixture-fed local pass against a disposable DB** and is called out so it is a known
property, not a surprise. Whether the durable store later moves to its own thread is
Patrick's call in a later pass (see out-of-scope) — this pass does not design it.

## Tests — `wal/wal_kernel_test.cpp` (ships with the code; project rule)

Fixture-fed, DB-backed (run from `wal/`, disposable `wal_manager` DB), same
`ok`/`FAIL` + `PASS <name>` convention. The **out-box consumer is a test stand-in**
(the cache manager is not built): the test drains the out-box and reads owed-work out
of the out-arena via the handle.

Cases (at least):
1. **Per-source ingest opens the right obligations.** Seed a DECLARE + a membership
   write on two different sources into two in-boxes; after `run_until_idle`, the
   `WalBook` shows exactly the expected open obligations (`is_open`/`list_open`).
2. **Reciprocal work is emitted, one item per owed obligation, to the out-box.** The
   out-box holds exactly the `owed()` set for the fed reports; each handle resolves to
   the expected `Obligation` identity. A structure DECLARE with two parents emits two
   structure items + one mass item; a settling report emits **nothing**.
3. **The settling return closes the obligation (self-accounting, unchanged).** Feed a
   forward write then its reciprocal return (same or later batch); the obligation
   opens then closes; the return emits no new owed work.
4. **Per-source order is structural; cross-source interleave is independent.** Two
   sources interleaved across their boxes reconcile to the same open set regardless of
   interleave; an out-of-order **same-source** feed throws (the reused `WalMonitor`
   guard), proving the box carries per-source order.
5. **Recycled-slot / only-follow discipline preserved.** The out-box uses the
   scheduler's sole enqueue path (no raw `Box::push`); (if return endpoints are
   exercised) a stale-generation ack is dropped, per the substrate contract.
6. **A thrown (out-of-order same-source) report contributes nothing to the out-box.**
   Feed a same-source report whose `lsn` is not strictly increasing; assert `feed()`
   throws (propagating out of the run, per the failure decision) **and** the out-box
   holds nothing attributable to that report — proving the `owed → feed → push` order
   means a rejected report emits no reciprocal work.
7. **Invariant: out-box handles == freshly-opened relation rows (single tie).** For a
   given fed report, the set of `Obligation` identities the out-box handles resolve to
   is **exactly** the set of rows `list_open` shows as freshly opened by that report —
   one assertion tying the emitted work to the booked obligations, stronger than
   asserting each side independently.

## Out of scope — do NOT build, invent, or stub with placeholder machinery

- **The serialization / API-pair bridge and the split-mode shuttle** (Patrick,
  2026-09-22) — separate kernel, separate pass. No serialization, transport, or
  thread boundary built here.
- **WAL-report → in-box translation / live feed** (Postgres logical-decoding → inbox
  messages) — its own focused discussion, tied to the wire-form/G6 gate. Stay
  fixture-fed; build no live slot.
- **The cache-manager consumer** (its input box, its runtime, the extend
  cache-mgr→swarm-mgr request path) — cache-manager layer, forward. The out-box
  consumer here is a test stand-in only.
- **Pair 2 (swarm manager) and the extra swarm-work out-box** — swarm-side facet,
  gated on the WAL-data-shape exam.
- **The analyst `verify` gate** — analyst layer, forward. The WAL core books and
  routes; it holds no confirmation logic.
- **Moving the durable store off-thread / behind a box** — NOT this pass. The
  `WalBook` DB path stays inline as the kernel's internal state (see reframe). If the
  store is ever reclassified as an API function on its own thread, that is Patrick's
  call in a later pass; do not design it here.
- **NEEDS-PATRICK / design-ahead (do not resolve):** F4; connection mass-recompute
  signal; cross-source global-order basis; MOVE/rekey of open obligations;
  async-removal obligation for DELETE. Ingest-atomicity stays **off** the WAL manager
  (no transaction surface added to `WalBook`) — owned by the cache-manager runtime.

## Design decisions to honour (from `ENDPOINT-ACTIVATION-NOTES.md`)

- **Push, not pull.** The kernel emits owed reciprocal work; `list_open` stays a read
  surface but is not the delivery path here.
- **Volatile transport, durable relation.** The out-box is volatile notification; the
  durable obligation stays in the open-obligation relation; a lost out-box entry is
  re-driven on reload by **the WAL manager itself re-emitting from `list_open`** (it
  is an originator; reload repopulation is a separate, deferred pass); close is still
  by observing the followup write. Pending work is **not** relocated into a memory
  box — the relation stays its durable home, which is exactly what makes reload
  repopulation possible.
- **Source-blind; only-follow / never-search; no imported machinery; one shared data
  space.** Every WAL-DB access stays a bounded PK follow (as built). The scheduler is
  the sole enqueue path.
- **The WAL manager originates no swarm coverage requests and holds no confirmation
  logic** — receive, book, route.

## Don't-touch

The built record tier (`codec/`…`dispatch/`) and the WAL bookkeeping core's
recognise/book/close logic (`wal_recognize`, `wal_book`, `wal_ingest`,
`wal_monitor`). This pass **adds** the box coupling + push out-box + driver + test.
Nothing in the bookkeeping algorithm changes; the `wal/` module edits are limited to
new files (and, at most, a README line pointing at the new part).

## Process (the discipline — coder + adversary, via Sonnet agents)

1. **This plan** → vet with a **fresh adversary** (complete review, no summarization,
   interrogate completeness, refuse partial). Reconcile findings before building.
2. **Build** (coder) to the reconciled plan; standalone-buildable, tests green
   (`g++ -std=c++17 -O2 -Wall -Wextra` [+ libpq/codec includes], prints `PASS`).
   DB-backed parts use the disposable `wal_manager` DB, never `hcp3_core`.
3. **Vet the build** with a fresh adversary; reconcile.
4. **Verify green**, then **commit** to `dbkernel-design-checkpoint` and push;
   update the 2026-09-21 forward flags in the touched docs to reflect what is built.

## Open points (resolved in review; noted for the record)

- **File shape — RESOLVED.** `wal/wal_kernel.{h,cpp}` (the reusable `WalKernel`
  reaction body) + `wal/wal_kernel_test.cpp`. A separate `wal_kernel_main.cpp` smoke
  binary only if it earns its place; otherwise the test *is* the driver for this cut.
- **Arena-handle-as-decimal-string — RESOLVED (adversary-confirmed).** It is a
  reference into a shared arena (sanctioned), not serialization: no struct field is
  encoded, only an index. Arena-by-**index** (not pointer) is deliberately safe —
  `std::vector` reallocation invalidates pointers but never previously-returned
  indices, and the in-arena is fully seeded before the run.

## For Patrick (surface, do not block the build)

- **"Originator" — RESOLVED (Patrick, 2026-09-22):** it was system-wide reload
  thinking, not steady-state routing. Steady state is source-blind; reload
  repopulation (the WAL manager re-emitting owed work from `list_open`) is a separate
  deferred pass this build stays compatible with. Recorded in § *Out-box* + the
  durability bullet.
- On commit, `ENDPOINT-ACTIVATION-NOTES.md`'s Pair-1 section gets a "narrowed by the
  2026-09-22 source-blind reframe" flag, matching the flags already on `wal/USAGE.md`.
