# Swarm / p2p layer — preliminary notes

> **⚠ Forward flag (2026-09-21).** The swarm-side facet and tracker indexing are now being
> designed on the activation substrate — `engine/db_kernel/ENDPOINT-ACTIVATION-NOTES.md`
> ("WAL-manager comms wiring + swarm indexing"): Pair-2 boxes (unpack inbound / compose
> outbound delta packets); the **address tree IS the tracker index**; a truncated area = the
> existing **coarse label token** (this doc's distillation / connection indicators) reused via
> the LoD dial. The delta-span vs state-range fork (below) is touched but stays **parked**;
> the layer remains gated on the WAL-data-shape exam.

**Status: PRELIMINARY NOTES (Patrick, 2026-09-19). Direction/draft only — not
designed, not built, not adversary-firmed.** The actual design waits on examining
the **WAL data shape** first. This file just captures Patrick's framing so it
isn't lost; nothing here is a committed mechanism. Distinct from the built WAL
manager (`wal/`), which is the *local* bookkeeper and stays untouched by this.

## Storage tiers

- **Cold storage DB** — the massive primary store; total data size is expected to
  be very large. Feeds the warm tier.
- **Warm working cache** — the cache manager's active working set, fed from cold
  storage.

## Decentralization / democratization of the primary data

The primary data is distributed **p2p / torrent-style** so no single holder has
the whole — the primary data stays decentralized and democratized.

The crux: the **WAL becomes the distribution manifest.** Part of the **WAL_db
manager's** job is to **prepare WAL data for a p2p/torrent-style tracker/client
system, treating the WAL data as bit-stream hash keys, NOT the data footprint.**
The WAL entries are the piece-hashes / index into the swarm, not carriers of the
content itself.

## The WAL_db manager's swarm-side facet (distinct from the built local bookkeeper)

The built WAL manager (`wal/`) is the *local* bookkeeper — per-source deferred-work
topology, no cross-source/swarm surface. Its **swarm-side facet** (deferred; these
notes) does two things:

- **Outbound prep** — prepare WAL data as **bit-stream hash keys** for the
  tracker/client system (the manifest/index into the swarm).
- **Inbound unpack** — unpack incoming data packets of updated information into the
  **DBs and granularities the cache manager has attached to** in the local storage
  swarm. Division of labour: the **cache manager decides what to attach / at what
  granularity**; the **WAL manager prepares (outbound) and unpacks (inbound)**.

## Core DB = distillation + connection indicators

The **core DB** holds a **distillation of primary nodes across swarms** — enough to
serve as **connection indicators** (what exists out there, and how to reach it),
**not** the full data. **Expanded coverage is acquired only on request**
(pull-on-demand, torrent-style: fetch the pieces you ask for).

## Why the flow works — determinism is the enabling property

The torrent property that makes decentralization work — **identical file + identical
settings ⇒ identical piece hashes, so more than one generator produces the same
manifest** — is something the data model **already guarantees**: address IS identity,
dedup by SEE, no aliasing (the record tier is deterministic and content-addressed by
construction). So identical-hash generation across holders is not something to
engineer; it falls out. Consequence: **no minter, no authority** — any holder
independently derives the same piece hash, so pieces are interchangeable,
sourceable from wherever has them, and verifiable without trusting the source.

What in the built data serves this (candidate structure, confirm at the exam):
- **The WAL** — already the ordered, self-interpreting stream; a natural manifest /
  piece list.
- **Trunk / contiguous-address-range layout** (kinds in trunks, sparse boundaries,
  PK order = address order via `COLLATE "C"`) — natural, deterministic chunk
  boundaries; everyone ranges it the same way.
- **Only-follow** — a chunk (a bounded address range + its stored lists) is
  self-contained: servable and verifiable without the whole.

## To pin when we examine the data shape

- **Is a piece a delta-span or a state-range?** A **delta-span** (a run of WAL
  entries) is the natural unit for *sync/updates* ("what changed"); a **state-range**
  (a trunk/address range of cold storage) is the natural unit for *bulk coverage*
  ("a region to pull"). Both are deterministic + content-addressed; they serve
  different jobs. The process likely needs both — *which unit for which path* is the
  concrete decision.

## Symmetric node topology — every node tracker/server/client

Every node is simultaneously **tracker + server + client** (there are lean ways to
do it), so the network updates **asynchronously with minimal interference** — no
privileged/central node. This is the **"kernel split is free" principle at network
scale**: each node has its own ongoing work, coupled by a store/stream (pieces +
manifest), not a synchronous handoff, staged by demand + availability.

It can be lean because the three roles **reuse structures already placed**, not new
infrastructure:
- **Tracker** ← the **core distillation** (primary nodes across swarms = connection
  indicators). A node's tracker knowledge *is* its local distillation — no
  heavyweight tracker service.
- **Server** ← **deterministic content-addressing + only-follow-servable chunks**.
  Any holder serves and verifies a piece from its own store, no authority (the
  identical-hashes property).
- **Client** ← **pull-coverage-on-request**. Request the pieces you want, when you
  want them.

## Lean coupling mechanism — stdin/stdout with monitors (preliminary)

The store/stream coupling can be realized **leanly via well-placed stdin/stdout
monitoring** (Patrick, 2026-09-19). Each kernel/node is a process that reads its
**stdin** (incoming reports/pieces/requests) and writes its **stdout** (emitted
reports/pieces); **monitors are allocated at the stream junctions** (where kernels
or nodes meet) to observe/route/compose — without any kernel needing to know about
another. Pipes give async + backpressure natively: no RPC framework, no service
mesh, no synchronous handoff. It's the Unix-pipe expression of "each part does its
own thing, coupled by a stream," and it composes both the kernel set and the
tracker/server/client roles.

**Guardrail:** this is stdin/stdout as the **inter-process data/control channel**
(the coupling) — distinct from the standing rule that **big result logs go to
files, never dumped to stdout**. The pipe is not the log; don't conflate them.

## Parked (Patrick holds ideas — not designed)

- **Conflict resolution.** Determinism makes conflicts **cheap to detect** —
  identical data ⇒ identical hashes (dedups; no conflict), so a conflict surfaces
  exactly as *divergent hashes at the same identity* (drift, independent authoring,
  or an incoming update racing a local one). Detection falls out; the **resolution
  policy** (which wins / how to merge) is the open part. Patrick has ideas —
  PARKED. Relates to drift control and the tentative→systemic validation path.

- **Exactly what becomes a bit-stream hash key, and at what granularity** (per WAL
  entry? per piece? per shard?) — the hinge the whole torrent-mapping turns on.
- The division of labour above (cache manager attach/granularity vs WAL manager
  prep/unpack vs core distillation/indicators) — confirm and detail.
- How this relates to already-named deferred seams: G6 (external wire form),
  cross-source ordering, the swarm-side of DELETE validation, and the
  cache-manager runtime (F4 / RECONCILE).
