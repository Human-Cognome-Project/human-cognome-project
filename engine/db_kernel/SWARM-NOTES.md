# Swarm / p2p layer — preliminary notes

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

## To pin when we examine the data shape

- **Exactly what becomes a bit-stream hash key, and at what granularity** (per WAL
  entry? per piece? per shard?) — the hinge the whole torrent-mapping turns on.
- The division of labour above (cache manager attach/granularity vs WAL manager
  prep/unpack vs core distillation/indicators) — confirm and detail.
- How this relates to already-named deferred seams: G6 (external wire form),
  cross-source ordering, the swarm-side of DELETE validation, and the
  cache-manager runtime (F4 / RECONCILE).
