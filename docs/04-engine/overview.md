# Engine Overview

The NAPIER engine has a clean three-part meta-structure. This page frames it; the four pages
that follow detail each part.

> **Read-me-first framing (claim 281, kept front-of-mind):** The GPU is a **resolution furnace,
> nothing more.** Cognition does **not** live in the GPU. Intelligence lives in the connections
> and the traversal over them (claims 140/255), not in the furnace.

Sources: claims 57 (two sides), 281 (furnace + meta-structure), 265 (cognitive clock),
274 (tick is reconciliation), 120 (three-layer storage).

---

## The engine has two sides

> *The physics engine has two operations: **resolution** (surfaces deltas — what's different from
> steady state) and **inference** (walks them — what those deltas imply). Always disambiguate
> which side a discussion is on before designing.* — claim 57

- **Resolution** is broadphase-style: burn through parallel physics/PBD work to *surface* what
  diverges. This is the GPU furnace's job.
- **Inference** is the determination walk: take the surfaced deltas and walk what they *imply*.
  This is CPU/warm-orchestrated (the determination engine, see
  [../02-architecture/intelligence.md](../02-architecture/intelligence.md)).

Confusing the two is the most common modeling error. Before designing any engine behavior, name
which side you are on.

---

## The three-part meta-structure

> *Three-part meta-structure: furnace (GPU resolution) + I/O reconciliation shell (the 11 ms
> beat) + continuous self-deemed cognition (envelopes, CPU/warm-orchestrated).* — claim 281

1. **The furnace** — GPU resolution. Parallel PBD: broadphase, settling, structural matching.
   Burns through resolution throughput at speed. → [resolution-furnace.md](resolution-furnace.md)

2. **The reconciliation shell** — the ~11 ms / ~90 Hz beat. **This is an I/O / reconciliation
   signal, not a work signal** (claim 274). It exists for the *outside world* — input management
   and conversational fluidity with other entities — not to pace cognition.
   → [cognitive-cycle.md](cognitive-cycle.md) and [reconciliation-loop.md](reconciliation-loop.md)

3. **Continuous self-deemed cognition** — NAPIER works on whatever **it deems appropriate**,
   continuously, at native speed *between* reconciliation beats. The workspace it works on each
   moment is an **envelope** (a db query+filter set, claim 273). What it deems relevant is
   self-selected and continuous — **not** one-envelope-per-tick.
   → [resolution-chamber.md](resolution-chamber.md) for the input-resolution path.

---

## Three storage layers feed the engine

The engine sits on a cold / warm / hot tiering (claim 120):

| Tier | Store | Holds | Who reads |
|------|-------|-------|-----------|
| **COLD** | NAS Postgres shards | possibility space + always-loaded structures (`hcp_core`, personality DB) | streamed up on demand |
| **WARM** | Postgres `hcp_var` (physics-side) | the **temporal triad** — reference DBs ("what I can do"), var DB ("what I am doing"), WAL log ("what I did") | CPU resolution |
| **HOT** | LMDB — **two** separate stores (input + output) | cognitive-resolution working set | GPU resolution |

The two LMDB stores are **source-locked** (claim 119): Postgres is the single writer to the input
LMDB; the GPU is the single writer to the output LMDB. Distinct stores with distinct writers =
zero contention. The Taichi physics/imagination path does **not** use LMDB — concept meshes
instantiate from the cold shard directly into GPU memory. Engine writebacks flow through Postgres
for durability (WAL). Details in [reconciliation-loop.md](reconciliation-loop.md).

---

## What is actually built today

The current engine is real, running software, but several parts of the meta-structure above are
**planned optimizations**, not shipped. The honest baseline is on
[implementation-baseline.md](implementation-baseline.md) (claim 201), and the deferred items —
including the **current GEM internals pending rework** (claims 201/239) — are tracked on
[../06-status/deferred-and-open.md](../06-status/deferred-and-open.md).

---

## The pages in this section

- [cognitive-cycle.md](cognitive-cycle.md) — the ~11 ms reconciliation beat, modality streams,
  deeming policy.
- [resolution-furnace.md](resolution-furnace.md) — the GPU PBD substrate; what it is and isn't.
- [reconciliation-loop.md](reconciliation-loop.md) — the ring-buffer LMDB cycle and stale-work
  recovery.
- [resolution-chamber.md](resolution-chamber.md) — how input text becomes identified tokens.
- [implementation-baseline.md](implementation-baseline.md) — the verified current engine.
