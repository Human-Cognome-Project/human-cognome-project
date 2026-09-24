# Endpoint-substrate primitives — implementation plan (rev. 2)

> **STATUS: IMPLEMENTED.** The local substrate described here is built under `network/endpoint/`. The later WAL integration is also built under `kernels/wal/`; the remaining out-of-scope seams below stay future work.

**Rev. 2 reconciles the plan-adversary findings (2026-09-21):** readiness ownership +
injection entry point (F1), generation bump-site (F2), slot-space partition (F3),
handler-continuation deferral named (F4), build-order (F5), send semantics (F6),
standing-endpoint generation (F7), test-claim wording (F8), liveness seam (F9). The
core design was found faithful and minimal; these are seam specifications, not redesign.

**Scope: the LOCAL activation substrate only** — memory-box endpoints, the
monitor/scheduler, the endpoint registry, and the request/return/ack contract. This is
the "get some data flows set up" pass. Derived from `ENDPOINT-ACTIVATION-NOTES.md`;
where that note and this plan differ, the note's resolved decisions govern.

**Explicitly OUT of scope (named seams, not built, not stubbed):**
- Converter-pairs / transmission (cross-representation, cross-node, swarm side).
- **Real concurrency** — thread-safe MPSC enqueue and true wake-on-arrival. First cut is
  a single-threaded cooperative scheduler; "activation" here means *the scheduler selects
  occupied boxes*, not block-until-ready. The concurrency boundary is named on `push`, not
  built.
- **Component-originated sub-requests + re-activation (F4).** The note's flagship flow
  (READ → sim → DECLARE) has a component originate a sub-request and be re-activated when
  its return box fills — which means the return endpoint becomes a *standing box + handler*,
  since a run-to-completion handler cannot block awaiting a sub-result. This cut models the
  **requester as the external driver**; component-origination + re-activation is deferred
  and named here so no one assumes await-style continuations (which run-to-completion
  forbids).
- Cross-node endpoint identity (the `node_addr` half). Local slots only here.
- **Durability** — boxes are volatile by design; the durable home is the store, recovery is
  ack + reload + idempotent redo. Not built here; this substrate is coupling only.
- **Liveness / stall detection (F9)** — the note's cost model names a monitor that stops
  draining as the one real source of unbounded growth ("a liveness signal to detect"). Not
  built here; named as a seam.
- WAL-manager integration — deferred to a later discussion once these primitives exist.

**Discipline:** each part standalone-buildable and self-testing (C++17, tiny in-file
`ok`/`FAIL` harness, prints `PASS <name>`, non-zero exit on failure), same convention as
`kernels/database/codec/`, `kernels/wal/`. These primitives are **pure in-memory — no libpq/Postgres**, so tests
build and run with a plain `g++` and no database. Tests ship with every part. No imported
machinery — build the minimum the flows need; name anything beyond it. Canadian English.

---

## What a "data flow" is here

A component is a **monitor over one or more boxes**: when a box it owns is occupied, the
scheduler runs that component's **handler** on the head item to completion, then moves on.
A request carries a **return endpoint**; the handler, when done, sends an **ack** (a result,
or an empty completion signal) to that return endpoint. Correlation is the return endpoint
itself — the box that receives the ack *is* the answer. Nothing polls; occupancy drives it.

## Ownership decision (resolves F1 — the readiness hole)

- **`box` is a dumb FIFO** — it knows nothing about the scheduler or readiness. Header-only,
  independent.
- **The scheduler owns readiness and is the SOLE enqueue path.** Raw `box.push` is never the
  injection point. All work enters through the scheduler (`submit` for external seeding,
  `Sender::send` from a handler) — one internal routine: resolve `EndpointId` → box → push →
  mark that box ready at its priority level.
- **Selection is a follow, not a scan.** The scheduler holds a **priority-indexed ready
  structure** (one ready-list per fixed priority level). "Pick the highest-priority occupied
  box" = take the top non-empty level — O(levels), a small fixed constant — never an
  O(all-boxes) scan. This satisfies the note's cost-model constraint.

## Modules

### 1. `box` (`box.h`, header-only)
An in-memory **FIFO queue** of `Message`. Volatile. Knows nothing of the scheduler.
- `Message` = an opaque payload (byte buffer, e.g. `std::string`/`std::vector<std::byte>`) +
  an optional `reply_to` `EndpointId`. The substrate never interprets the payload.
- Ops: `push(Message)` (append tail), `pop() -> Message` (take head), `empty()`, `size()`.
  `pop()` on empty is a caller error surfaced as `!ok` (assert/throw), never UB.
- FIFO is the only ordering; no per-item priority (priority is per box, §3).
- **Concurrency boundary (named seam):** `push` is where MPSC safety lives once producers are
  separate threads. First cut single-threaded; the boundary is documented, not implemented.
- Tests: FIFO order; occupancy transitions; pop-empty is `!ok`, not UB.

### 2. `endpoint` (`endpoint.h`, `endpoint.cpp`)
Endpoint identity + the local registry (name → box). Only-follow: resolution is a direct map
lookup, never a scan.
- `EndpointId = { uint32_t slot; uint32_t generation; }` — copyable, comparable. (The
  `node_addr` half is a deferred seam; local only.)
- **Slot-space partition (F3):** standing and ephemeral slots occupy **disjoint** ranges (e.g.
  a reserved standing range vs a separate ephemeral/freelist range). `allocate()` never returns
  a slot `register_standing` can claim — a same-generation standing/ephemeral collision is
  structurally impossible, not left to the generation stamp to catch.
- `register_standing(slot, Box*)` — fixed, well-known endpoints (component inputs, the pinned
  reconcile box). Stable for the process lifetime. **Standing generation (F7):** a fixed
  sentinel (`kStandingGeneration`); `resolve` treats a standing slot as always valid (never
  stale), so a `reply_to` naming a component input box always resolves.
- `allocate() -> EndpointId` — an ephemeral return endpoint from the freelist. Returns the
  slot with its **current** generation. **`allocate` does NOT bump (F2).**
- `resolve(EndpointId) -> Box*` (or `!ok`) — direct lookup. For an ephemeral slot, a **stale
  generation** (slot recycled past this id) resolves to `!ok` → a late/duplicate ack is dropped,
  never misdelivered. Standing slots always resolve.
- `recycle(EndpointId)` — return an ephemeral slot to the freelist and **bump its generation.
  Recycle is the ONE and ONLY generation bump-site (F2)** — a late ack arriving any time after
  recycle already fails the check.
- Tests: register/resolve; allocate/recycle round-trip; **stale-generation resolve fails** (the
  one real local correctness point); recycle bumps generation; standing resolves regardless of
  generation; standing and ephemeral slot ranges are disjoint.

### 3. `scheduler` (`scheduler.h`, `scheduler.cpp`) — depends on `box` AND `endpoint` (F5)
The monitor loop: readiness + box-granular priority (see Ownership decision).
- A box is registered with a **priority level** and a **handler**:
  `void handler(const Message&, Sender&)`. `Sender` exposes `send(EndpointId, Message) -> SendStatus`.
- `submit(EndpointId, Message) -> SendStatus` — external injection entry point (the harness /
  an out-of-substrate producer seeds work here). Same routing routine as `send`.
- **`send` / `submit` semantics (F6):** resolve `EndpointId` via the registry; on success push
  into the box and mark it ready; return `SendStatus` = `kDelivered` or `kDropped`
  (unresolvable or stale-generation). A handler with **no `reply_to`** simply does not send —
  the substrate does nothing (no ack), by design.
- **Readiness, not polling:** enqueue marks the box ready; the scheduler services only ready
  boxes (O(occupied)).
- `step()` = take the highest-priority ready box, run its handler on the head item **to
  completion**, pop it; drop it from the ready set if now empty. `run_until_idle()` = `step()`
  until no box is ready.
- **Selection-boundary priority re-evaluation (F8, corrected claim):** priority is re-checked
  at each selection boundary; a handler that `send`s into a higher-priority box (e.g. the
  reconcile box) causes that box to be picked next. In this single-threaded cut a handler is a
  plain call that runs to completion by construction, so **non-preemption is structural, not a
  tested property** — the tested property is *selection ordering*. True non-preemption under
  threads is deferred with the concurrency seam.
- The **reconcile box** is not special code: a box pinned at the top priority level, normally
  empty. Shown in a test, not a mechanism.
- Tests: priority order (top-level ready box drained first); **selection-boundary re-eval** (a
  handler sends into the top/reconcile box; assert it is picked next); FIFO within a box; a
  pinned top box wins when occupied and is inert when empty; `send`/`submit` return
  `kDropped` on a stale/unresolvable endpoint.

### 4. Integration / data-flow test (`dataflow_test.cpp`)
Proves the whole contract end-to-end over the three modules (seed via `submit`, then
`run_until_idle`):
- **Round-trip:** a request Message (with a `reply_to`) is `submit`ted to a component's input
  box; its handler processes and `send`s an ack/result to `reply_to`; the requester's return
  box receives it. Completion is the ack landing — no poll.
- **Multi-connection / correlation:** several requests, each with its **own** `reply_to`,
  interleaved into one input box; each ack routes to its own return endpoint; no cross-talk.
  Correlation is the return endpoint, not position. **(Honesty: this proves correlation via
  sequential interleave; it does NOT test concurrent-append safety — that is the deferred MPSC
  seam.)**
- **Reconcile pattern:** work staged into the top-priority pinned box is selected ahead of
  normal boxes at the next boundary, then the box is empty again.
- **Recycled-slot safety:** an ack addressed to a since-recycled return endpoint (stale
  generation) returns `kDropped` and is not misdelivered.

## Build order
`box` (independent) → `endpoint` (independent) → `scheduler` (depends on **box and endpoint**,
F5) → `dataflow_test` (depends on all three). Each merges only when its own `PASS` is green.

## Deferred seams (named, not built)
Converter-pairs/transmission; real MPSC concurrency + true wake-on-arrival; component-originated
sub-request + re-activation (return endpoint as standing box+handler, F4); liveness/stall
detection (F9); `node_addr` cross-node identity; durability / ack-reload-idempotent recovery;
WAL-manager integration. Each is a later, separate pass — none is filled with placeholder
machinery here.
