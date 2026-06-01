# Implementation Baseline

What the engine **actually is today** — the verified, running substrate behind the architecture
pages. This is the honest done-state; planned optimizations and deferred rework are flagged.

Sources: claims 201 (engine baseline), 239 (O3DE is the compiler/manager), 204 (document-storage
milestone). Live engine details cross-checked against the repo (`hcp-engine/`) and the daemon API.

---

## The engine, concretely

> *The NAPIER inference engine is implemented as an O3DE 25.10.2 C++ Gem using PhysX 5
> GPU-accelerated Position Based Dynamics (PBD).* — claim 201

- **Headless daemon** — `HCPEngine.HeadlessServerLauncher` listens on **TCP port 9720** with a JSON
  socket API. Verbs: `health`, `ingest`, `retrieve`, `list`, `tokenize`, `phys_ingest`.
- **Two-phase resolution pipeline:**
  - **Phase 1** — byte → Unicode-codepoint PBD superposition (~16K codepoints/chunk,
    Unicode-aware).
  - **Phase 2** — char → word via persistent **VocabBeds** (`BedManager` + GPU workspaces,
    triple-pipelined cascade) with inflection-strip and variant-normalize.
- **Size** — ~21,300 lines of C++ across ~35 modules as of 2026-04-08, actively evolving since
  (tokens→entries table migration, demand-driven LMDB flow).

This is the concrete substrate behind physics-is-gui-on-pivot-tables (claim 42),
engine-two-sides (claim 57, see [overview.md](overview.md)), and the
[resolution chamber](resolution-chamber.md) (claim 85).

---

## O3DE is the compiler/manager — PhysX-agnostic

> *O3DE is the chosen code compiler/manager and orchestration base.* — claim 239

The rationale matters for reading the engine:

1. **O3DE is the most powerful game/physics orchestration base available, proven beyond games** —
   used heavily in **robotics** (real-world physical-system orchestration), which fits HCP's
   physical-grounding / world-model needs (claims 18–20), not mere rendering.
2. **Its native C++ means the project works at near-db-primitives** — close enough for what HCP
   needs, which *is* the substrate-first requirement (the db-functions keystone, claim 192):
   operating at the primitive layer where db functions live, not on top of an abstraction stack.
3. **C++-native structure + kernel-escalation routines save substantial low-level work.**

All of this holds **independent of whether PhysX functions are used** — **PhysX is one option within
O3DE, not a lock-in** (claim 239). The current ingestion pipeline is implemented as O3DE Gems.

---

## The document-storage milestone (done, then paused)

> *The engine stores documents in an inference-conducive format and reproduces them on demand at
> >98% accuracy.* — claim 204

- ~99% of words are stored at the highest-level word-construct `token_id`.
- A 9-document Gutenberg stress test (2026-04-13) ingested cleanly with var rates 0.15–0.59% (Moby
  Dick high-water 0.59% on whaling vocabulary).
- Residual reconstruction issues are minor spacing/capitalization — **no meaning drift.**

**Status: paused.** Phrase / idiom / Proper-Entity constructs *exist* in the shards but are **not
yet assigned to resolution chambers.** Per the 2026-04-16 pivot (claim 99), further document-storage
and higher-order-tokenization work is paused while focus moves to NSM concept modeling; these
elements are not revisited until story-level constructs are evaluated in concept space. (See
[../06-status/status.md](../06-status/status.md).)

---

## What is built vs planned vs deferred

Read the engine pages with this triage in mind:

| Item | State | Reference |
|------|-------|-----------|
| O3DE/PhysX 5 Gem, headless daemon (port 9720), two-phase pipeline | **built** | claim 201 |
| Document storage + >98% reconstruction | **built, paused** | claim 204 |
| Resolution chamber (Tier 1–3, single-word) | **built** (C++ ingestion) | claims 169, 175–181 |
| Multi-word / higher-order construct detection | **planned** | claim 183 |
| Two-LMDB split + ring-buffer reconciliation + loop optimization | **planned** | claims 119/121/282/283 |
| Current GEM internals | **deferred — pending rework** | claims 201/239 |

> **Honesty flag (claims 201/239):** the **current GEM internals are deferred pending rework.** The
> engine described here is real and running, but its internal structure is expected to change in the
> review phase, and PhysX-specific substrate facts (see
> [resolution-furnace.md](resolution-furnace.md)) are current-state, not locked. Do not treat engine
> internals as a frozen spec. Tracked on
> [../06-status/deferred-and-open.md](../06-status/deferred-and-open.md).

---

## See also

- [overview.md](overview.md) — the three-part meta-structure this engine realizes.
- [../07-operations/build-and-run.md](../07-operations/build-and-run.md) — how to build and run the
  daemon.
