# Engine Overview

> ## ⚠ Light overview — design state, current *and* future, subject to change
>
> Engine documentation is intentionally kept **light** (Patrick-direct, claim 292). Two reasons:
> the **Gem internals are in active rework** (claims 201/239), and **much of the engine mechanics
> below are forward-looking design** (claims 265–288), **not yet built.** Documenting it deeply now
> would be documenting a moving target. So this page is a high-level map that marks current-vs-future
> state; the deep per-subsystem pages were intentionally **deferred**. For what is *actually built
> and verified*, see [implementation-baseline.md](implementation-baseline.md); for the open/deferred
> items, see [../06-status/deferred-and-open.md](../06-status/deferred-and-open.md).

The NAPIER engine has a clean three-part meta-structure. This page frames it at a high level.

> **Front-of-mind framing (claim 281):** The GPU is a **resolution furnace, nothing more.**
> Cognition does **not** live in the GPU. Intelligence lives in the connections and the traversal
> over them (claims 140/255), not in the furnace.

---

## The engine has two sides

> *The physics engine has two operations: **resolution** (surfaces deltas — what's different from
> steady state) and **inference** (walks them — what those deltas imply).* — claim 57

- **Resolution** is broadphase-style: burn through parallel physics/PBD work to *surface* what
  diverges. This is the GPU furnace's job.
- **Inference** is the determination walk: take the surfaced deltas and walk what they *imply*. This
  is CPU/warm-orchestrated (the determination engine, see
  [../02-architecture/intelligence.md](../02-architecture/intelligence.md)).

Before designing any engine behavior, name which side you are on — confusing the two is the most
common modeling error.

---

## The three-part meta-structure (claim 281)

1. **The furnace — GPU resolution.** Parallel PBD: broadphase, settling, structural matching. Burns
   through resolution throughput at speed. Its only job is resolution; cognition lives elsewhere.
   *(Current substrate: PhysX 5 PBD — but PhysX is one option within O3DE, not a lock-in, and the
   substrate specifics are current-state, not locked — claims 216/239.)*

2. **The reconciliation shell — the ~11 ms / ~90 Hz beat (claim 265).** This is an **I/O /
   reconciliation signal, not a work signal** (claim 274): it exists for the *outside world* (input
   management, conversational fluidity), not to pace cognition. *Design intent:* inputs map into the
   cycle at adjustable per-modality frequencies; the optimized reconciliation runs over a two-LMDB
   (input/output) memory-mapped, source-locked ring buffer (claims 119/121/283). **Much of this is
   forward-looking design — "planned, not yet implemented."**

3. **Continuous self-deemed cognition.** NAPIER works on whatever **it deems appropriate**,
   continuously, at native speed *between* reconciliation beats. The workspace it works on each moment
   is an **envelope** — a db query+filter set (claim 273). What it deems relevant is self-selected and
   continuous, **not** one-envelope-per-tick. *(The deeper deeming/weighting math is the deferred
   math gap — claim 286.)*

---

## Storage tiers feed the engine (claim 120)

| Tier | Store | Holds |
|------|-------|-------|
| **COLD** | NAS Postgres shards | possibility space + always-loaded structures (`hcp_core`, personality DB) |
| **WARM** | Postgres `hcp_var` | the temporal triad — reference DBs / var DB / WAL log |
| **HOT** | LMDB (two source-locked stores, input + output) | cognitive-resolution working set |

The two-LMDB split (single-writer each, zero-contention) is **design intent**, not yet implemented
(claim 121). Engine writebacks flow through Postgres for durability (WAL).

---

## What is built vs forward-looking

This is the honest split — read it before going deeper anywhere in this section:

| Item | State |
|------|-------|
| O3DE/PhysX 5 Gem, headless daemon (port 9720), two-phase pipeline | **built** (claim 201) |
| Document storage + >98% reconstruction | **built, paused** (claim 204) |
| Resolution chamber, single-word (Tier 1–3) | **built** (claims 169, 175–181) |
| The 11 ms cycle / modality streams / deeming policy | **design** (claims 265–288) |
| Two-LMDB split + ring-buffer reconciliation + loop optimization | **planned** (claims 119/121/282/283) |
| Higher-order multi-word construct detection | **planned** (claim 183) |
| Current GEM internals | **deferred — pending rework** (claims 201/239) |

---

## Where to go from here

- [implementation-baseline.md](implementation-baseline.md) — **what's actually built and verified**
  (the non-moving-target page).
- [resolution-chamber.md](resolution-chamber.md) — how input text becomes identified tokens
  (the built ingestion path; current-state).
- [../06-status/deferred-and-open.md](../06-status/deferred-and-open.md) — the deferred / in-flux /
  planned items, collected.

> **Deferred by design:** the deep per-subsystem engine pages (cognitive-cycle internals,
> resolution-furnace substrate, reconciliation-loop mechanics) are intentionally **not** written out
> here, per claim 292 — they are a moving target. The forward-looking mechanics are summarized above
> and tracked in the claim-graph (claims 265–288); they will be documented in depth once the engine
> rework settles.
