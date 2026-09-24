# endpoint -- the local activation substrate

The endpoint-substrate primitive kernel set (`ENDPOINT-PRIMITIVES-PLAN.md`
rev. 2, resolving the plan-adversary findings against
`ENDPOINT-ACTIVATION-NOTES.md`). Same discipline as the record-tier modules
(`codec/`, `wal/`): each part builds, runs, and tests **on its own**.

## Scope

The **local** activation substrate only: memory-box endpoints, the
monitor/scheduler, the endpoint registry, and the request/return/ack
contract -- "get some data flows set up." These primitives are **pure
in-memory C++17, no libpq/Postgres**: every test builds and runs with a
plain `g++` and no database.

**Explicitly OUT of scope (named seams, not built, not stubbed):**
converter-pairs/transmission; real MPSC concurrency and true
wake-on-arrival (first cut is a single-threaded cooperative scheduler --
"activation" means *the scheduler selects occupied boxes*, not
block-until-ready); component-originated sub-requests + re-activation (the
note's flagship READ -> sim -> DECLARE flow, deferred because a
run-to-completion handler cannot block awaiting a sub-result -- this cut
models the requester as the external driver instead); `node_addr`
cross-node identity (local slots only); durability (boxes are volatile by
design -- recovery is ack + reload + idempotent redo, owned by the store,
not this substrate); liveness/stall detection; WAL-manager integration.

## The ownership decision

- **`box` is a dumb FIFO.** It knows nothing about the scheduler or
  readiness. Header-only, independent.
- **The scheduler owns readiness and is the SOLE enqueue path.** Raw
  `box::Box::push` is never the injection point. All work enters through
  the scheduler (`submit` for external seeding, `Sender::send` from a
  running handler) -- one internal routine: resolve `EndpointId` -> box ->
  push -> mark that box ready at its priority level.
- **Selection is a follow, not a scan.** The scheduler holds a
  priority-indexed ready structure (one ready FIFO per fixed priority
  level). Picking the highest-priority occupied box is "take the top
  non-empty level" -- O(levels), a small fixed constant -- never an
  O(all-boxes) scan.

## Files

| File | What it is |
| --- | --- |
| `box.h` | Header-only `Message` (opaque payload + optional `reply_to`) and `Box` (an in-memory FIFO queue). `endpoint::EndpointId` is physically defined here too -- see *A plan-silent call*, below. |
| `box_test.cpp` | FIFO order; occupancy transitions; pop-on-empty is `!ok` (throws), never UB; `reply_to` round-trips through the box unexamined. |
| `endpoint.h` / `endpoint.cpp` | `endpoint::Registry`: disjoint standing/ephemeral slot ranges (F3), the standing-generation sentinel (F7), `register_standing`, `allocate` (never bumps, F2), `resolve` (stale-generation drop, standing always resolves), `recycle` (the ONE generation bump-site, F2). |
| `endpoint_test.cpp` | Register/resolve; allocate/recycle round-trip; stale-generation resolve fails; recycle bumps generation; allocate itself never bumps; standing resolves regardless of generation; standing/ephemeral ranges are disjoint. |
| `scheduler.h` / `scheduler.cpp` | `scheduler::Scheduler`: the priority-indexed ready structure, `submit` / `Sender::send` (one routing routine, F6, returning `SendStatus::kDelivered`/`kDropped`), `step()` / `run_until_idle()`, selection-boundary priority re-evaluation (F8). |
| `scheduler_test.cpp` | Priority order (top-level ready box drained first); selection-boundary re-evaluation (a handler sends into the pinned top box mid-flow and it wins the *next* selection ahead of already-queued lower-priority work); FIFO within a box; a pinned top box is inert while empty; `send`/`submit` return `kDropped` on a stale/unresolvable endpoint. |
| `dataflow_test.cpp` | End-to-end integration: round-trip ack; multi-connection correlation via sequential interleave (honestly not concurrent-append safety -- that's the deferred MPSC seam); the reconcile pattern; recycled-slot stale-drop. |

## Build & run the tests

Pure, no DB -- run from `endpoint/`:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -o /tmp/box_test box_test.cpp \
    && /tmp/box_test

g++ -std=c++17 -O2 -Wall -Wextra -o /tmp/endpoint_test endpoint.cpp endpoint_test.cpp \
    && /tmp/endpoint_test

g++ -std=c++17 -O2 -Wall -Wextra -o /tmp/scheduler_test endpoint.cpp scheduler.cpp scheduler_test.cpp \
    && /tmp/scheduler_test

g++ -std=c++17 -O2 -Wall -Wextra -o /tmp/dataflow_test endpoint.cpp scheduler.cpp dataflow_test.cpp \
    && /tmp/dataflow_test
```

Each prints one `ok`/`FAIL` line per check and `PASS <name>` on success,
with a non-zero exit if anything failed -- same convention as
`codec/README.md` and `wal/README.md`.

## Build order

`box` (independent) -> `endpoint` (depends on `box` for `EndpointId`/`Box`,
but not on the scheduler) -> `scheduler` (depends on **box and endpoint**,
F5) -> `dataflow_test` (depends on all three). Each merges only when its
own `PASS` is green.

## A plan-silent call the plan left open

The plan lists `box.h` as header-only/independent and separately says
`Message` carries an optional `reply_to` `EndpointId` -- but `EndpointId`
identity/registry semantics are module 2's (`endpoint.h`). Those two
requirements are in tension: `Message` needs the `EndpointId` *type* to
exist structurally, while `box.h` is meant to know nothing about the
registry that gives an `EndpointId` its meaning.

Resolved by splitting the type from its semantics: `EndpointId` (a bare
`{slot, generation}` POD, no logic) is physically defined in `box.h`,
inside `namespace endpoint`, right where `Message` needs it. `endpoint.h`
includes `box.h` and builds the actual registry (`Registry`:
allocate/resolve/recycle, the disjoint ranges, the sentinel) around that
id. `box::Box` still never inspects a `reply_to` -- it just carries it
through the queue -- so the *independence* the plan is protecting (box
doesn't know about the scheduler or the registry's rules) holds; only the
POD type's physical byte-layout definition had to live somewhere, and
`box.h` was the lower, dependency-free layer to put it in.

A second call, also plan-silent: **who owns the `box::Box` behind an
`EndpointId`.** Standing endpoints (component inputs, the pinned reconcile
box) are wired by the setup/topology code, so `register_standing` takes a
caller-owned `Box*` that must outlive the registry. Ephemeral return
endpoints are anonymous per-request mailboxes with no topology role, so
`Registry` owns their storage outright (a fixed-size `vector<Box>` sized
by `ephemeral_capacity`, indexed by slot) and hands back only the
`EndpointId`; the caller resolves it through the registry rather than
holding a `Box*` directly. This keeps `allocate()`'s signature exactly as
the plan states it (`allocate() -> EndpointId`, no `Box*` parameter) while
still giving every ephemeral slot a real, addressable box from the moment
the registry is constructed.

## Deferred seams (named, not built)

Converter-pairs/transmission; real MPSC concurrency + true
wake-on-arrival; component-originated sub-request + re-activation (return
endpoint as standing box+handler, F4); liveness/stall detection (F9);
`node_addr` cross-node identity; durability / ack-reload-idempotent
recovery; WAL-manager integration. Each is a later, separate pass -- none
is filled with placeholder machinery here.
