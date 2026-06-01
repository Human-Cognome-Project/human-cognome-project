# The Reconciliation Loop

How the CPU (orchestration) and the GPU (furnace) exchange work without contention. This is the
**I/O reconciliation shell** of the three-part meta-structure (claim 281).

> **Status:** the optimized loop described here is **planned** (claim 121, "not yet implemented as
> of 2026-05-28"). The two-LMDB split and source-locking are the design; the current engine uses a
> simpler path. See [implementation-baseline.md](implementation-baseline.md).

Sources: claims 119 (input/output LMDB split), 121 (LMDB-GPU I/O cycle), 282 (loop optimization),
283 (ring buffer), 284 (stale-work check), 120 (three-layer storage), 114 (single-writer).

---

## Two LMDB stores, two single writers

> *LMDB is structured as **two separate** database structures — input LMDB and output LMDB — not
> halves of a single LMDB. Both memory-mapped.* — claim 119

Writer assignment:

- **Postgres** (the warm-cache manager) is the **single writer to the input LMDB.**
- **The GPU engine** is the **single writer to the output LMDB.**

Distinct stores with distinct writers = **zero contention, zero coordination overhead.**
Source-locking (claim 114 — source-locked single writers perform fastest) is the *performance
gate*; the two-store separation is what *enables* it.

The flow:

```
Postgres composes input ──▶ INPUT LMDB ──▶ GPU reads, resolves ──▶ OUTPUT LMDB ──▶ CPU/Postgres reads back
   (warm cache manager)    (mem-mapped,        (the furnace)        (mem-mapped,      (consumes results)
    single writer)          Postgres-locked)                         GPU-locked)
```

Because both sides are **memory-mapped**, this eliminates per-cycle CPU↔GPU transfer overhead:
reconciliation happens at **RAM-bus speed.** The Taichi physics/imagination path is **separate** —
no LMDB; concept meshes instantiate from the cold shard directly into GPU memory (claim 120).

---

## The output LMDB is a ring buffer

> *GPU outflow goes to a separate LMDB **owned by the GPU** (single writer), which **evicts oldest
> first** — a bounded FIFO / ring buffer that never grows unbounded. The CPU reads its
> reconciliations from that LMDB as fast as they appear — no waiting, no write contention.* — claim 283

Single-producer (GPU) / single-consumer (CPU), source-locked, memory-mapped, zero-contention. The
key property: **evict-oldest = reconciliation, not archival.** An un-consumed old reconciliation is
simply dropped, and the engine **never blocks on the reader** — consistent with the
snapshots-for-continuous-impression principle (claim 275): you need *current state*, not every
frame.

---

## Missed reconciliations are recovered, not errors

> *Because the output LMDB evicts oldest-first, a reconciliation can be dropped before the CPU
> reads it (if the CPU fell behind). The CPU tracks outstanding/stale work and **on occasion**
> sweeps it to check whether it actually got processed but the reconciliation was missed.* — claim 284

This is the eventual-consistency safety net for the lossy ring buffer. The low-frequency stale-work
check guarantees **no silent loss** while the fast lossy buffer stays correct and never blocks the
hot path. A miss is **not an error** (no-error-category, claim 195): it is deferred resolution —
the stale work sits in an unknown/−0 state (null-vs-zero, claim 17) until the occasional check
resolves whether it completed.

> Human-cognition parallel (claim 285): *"give me a moment — I lost my train of thought"* is the
> human version of this stale-work check. Structural equivalence, offered as a validation signal,
> not decoration.

---

## Loop optimization: feed the furnace in its preferred shape

> *The engine loop will be restructured for (1) RAM-bus-speed reconciliation with the CPU and
> (2) data repacking for GPU efficiency.* — claim 282

Two parts:

1. **RAM-bus-speed reconciliation** — the CPU↔furnace exchange runs at memory-bus speed via the
   memory-mapped LMDB interface (claims 119/121); no slower interconnect tax on the sync.
2. **Data repacking for GPU efficiency** — data laid out coalesced / aligned / padded
   (**bed-packing**; engine-shaped, not Postgres-symmetric, claim 91) so the furnace resolves at
   full throughput instead of stalling on bad memory-access patterns.

Together: feed the furnace in its preferred shape + reconcile at memory speed, keeping the working
set in the fast path (bounded-n, claim 271). **The furnace itself does not change** (still just a
furnace, claim 281); the loop *around* it gets faster.

---

## See also

- [overview.md](overview.md) — where this shell sits in the three-part structure.
- [implementation-baseline.md](implementation-baseline.md) — what the *current* (pre-optimization)
  engine does.
- [../05-data-layer/shards-and-schema.md](../05-data-layer/shards-and-schema.md) — the warm/cold
  Postgres side the input LMDB is composed from.
