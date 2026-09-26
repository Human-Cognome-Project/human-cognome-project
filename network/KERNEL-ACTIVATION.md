# Kernel activation and runtime composition

**Status (2026-09-26):** Network-layer companion to the
[NAPIER system guide](../docs/napier-system-guide.md). The local C++
[endpoint substrate](endpoint/README.md) is built; runtime balancing,
cross-location bridges, and the swarm are future work. The detailed
[activation record](ENDPOINT-ACTIVATION-NOTES.md) documents the decisions
and their evolution.

## One communication shape

Kernels own their work and exchange messages through directed inbox/outbox
endpoints. A message occupies a box; the scheduler chooses an occupied box
according to its fixed priority and invokes the handler. A handler can
emit to a counterpart's inbox or a caller-supplied return endpoint. The
box carries the addressing and its priority, while order inside a box is
FIFO. Selection of higher-priority work happens between units, without
interrupting a handler already running.

The built `network/endpoint/` implementation contains local in-memory
queues, an endpoint registry with recyclable return slots, and a cooperative
priority-indexed ready scheduler. It processes fixture-fed handlers one
unit at a time. It does **not** implement OS wake-on-arrival, concurrent
multi-producer appends, automatic worker proliferation, or transport between
machines. Current [WAL handler](../kernels/wal/README.md) and
[DB-manager tier-2 handler](../kernels/database/dbmanager/README.md)
consume the local substrate with fixture-driven input.

| Box / tier in the intended cache manager | Sender and effect | Built extent |
|---|---|---|
| Top reconcile, normally empty | WAL manager selects existing relevant obligations after analyst RECONCILE; do next. | Priority substrate exists; reconcile routing does not. |
| Analyst requests | One ordered current-work stream per analyst with its reply endpoint. | Tier-2 fixture handler exists. |
| Pending reciprocal work | WAL manager emits obligations; cache manager performs deferred return writes. | WAL fixture out-box exists; cache consumer does not. |
| Background maintenance | Standing lower-priority cache upkeep, view refresh. | Design only. |

**RECONCILE is an analyst message to the WAL manager**, never a DB verb. It
promotes already tracked work; the durable home stays the WAL manager's
PostgreSQL obligations. See [report to work](../kernels/wal/REPORT-TO-WORK.md).

## Assembly and location

The endpoint monitor is meant to watch multiple boxes and activate less
frequently needed secondary kernel sets when their input arrives. Primary
workers may proliferate with demand. An active runtime balancer can
adjust that assembly as work moves, while respecting data dependencies
such as the field model's end-of-tick centroid publication. Its scaling
signals, scheduling policy, threads and resource limits are **not yet
specified**; the current scheduler's ready-box selection is the limited
built base, not a dynamic balancer.

Kernels need to know **which counterpart** they address, not the
counterpart's physical location. Local boxes can connect in memory;
configuration/topology would set up endpoint paths. At a process or
machine boundary, a future matched converter/transport bridge would
serialize and deliver between equivalent boxes. Serialization belongs
at that seam, rather than in every internal kernel interaction. The
future thread manager owns lower-frequency system-facing activation;
the physics [engine harness](../engine/ARCHITECTURE.md) controls the
model and its data transfers, not these network tasks. The p2p swarm
and tracker are a later, lower-priority application of this structure.

## Three different monitors

- The **network scheduler/monitor** selects occupied mailboxes and runs
  component handlers. Its eventual runtime balancer would manage active
  kernel sets.
- [`wal::WalMonitor`](../kernels/wal/wal_monitor.h) checks a WAL source's
  report ordering and high-water position. It is not the box scheduler.
- A **browser model view** is a human-readable rendering sampled from
  physics results. It does not decide the engine's tick rate or how often
  a future analyst reads raw numerical output.

This distinction matters when adding monitoring or readback: each control
point has a different owner and cadence.
