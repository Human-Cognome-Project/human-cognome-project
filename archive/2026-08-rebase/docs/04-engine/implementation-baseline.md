# Implementation Baseline

What the engine **actually is today** — the verified, running substrate behind the architecture
pages. This is the honest done-state; planned optimizations and deferred rework are flagged.

Sources: claims 201 (engine baseline), 239 (O3DE is the compiler/manager), 204 (document-storage
milestone). Live engine details cross-checked against the repo (`hcp-engine/`) and the daemon API.

---

## The engine, concretely

> *The NAPIER inference engine is an AZSL/host-resident compute engine — host C++ plus AZSL
> compute kernels, PhysX fully removed.* — claim 201

- **Headless daemon** — `HCPEngine.HeadlessServerLauncher` listens on **TCP port 9720** with a JSON
  socket API. Verbs: `health`, `ingest`, `retrieve`, `list`, `tokenize`, `phys_ingest`.
- **byte→codepoint→word resolution (end-to-end):**
  - **Byte-floor** — raw bytes → Unicode codepoints, a lossless round-trip (positional map with a
    paint-all fallback). This floor feeds the resolution chambers.
  - **Chambers** — char → word via persistent **VocabBeds** (`BedManager` + host workspaces). The
    settle is a host `SettleKernel` (CPU reference plus an AZSL compute kernel, GPU-equivalence
    verified on a GTX 750 Ti); a differential-contact-floor fix makes the chambers settle and emit
    canonical ids, with inflection-strip and variant-normalize. Verified via an `HCP_RESOLVE_FILE`
    hook on the daemon: "the cat sat on the mat" → 6/6 canonical ids.

This is the concrete substrate behind physics-is-gui-on-pivot-tables (claim 42),
engine-two-sides (claim 57, see [overview.md](overview.md)), and the
[resolution chamber](resolution-chamber.md) (claim 85).

---

## O3DE is the compiler/manager — physics-engine-agnostic

> *O3DE is the chosen code compiler/manager and orchestration base.* — claim 239

The rationale matters for reading the engine:

1. **O3DE is the most powerful game/physics orchestration base available, proven beyond games** —
   used heavily in **robotics** (real-world physical-system orchestration), which fits HCP's
   physical-grounding / world-model needs (claims 18–20), not mere rendering.
2. **Its native C++ means the project works at near-db-primitives** — close enough for what HCP
   needs, which *is* the substrate-first requirement (the db-functions keystone, claim 192):
   operating at the primitive layer where db functions live, not on top of an abstraction stack.
3. **C++-native structure + kernel-escalation routines save substantial low-level work.**

All of this held **independent of whether PhysX functions were used** — **PhysX was always one option
within O3DE, not a lock-in** (claim 239). That option has now been dropped: PhysX is fully removed and
the settle is an AZSL/host-resident compute kernel. The current ingestion pipeline is implemented as
O3DE Gems with host C++ and AZSL compute.

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
| AZSL/host-resident engine, headless daemon (port 9720), byte-floor → chamber pipeline | **built** | claim 201 |
| bytes→words resolution end-to-end (6/6 canonical ids via `HCP_RESOLVE_FILE`) | **built** | claim 201 |
| Compact-ID packer slice (`Pack/`, 13/13 ctest) | **landed, not yet wired** | claim 201 |
| Document storage + >98% reconstruction | **built, paused** | claim 204 |
| Resolution chamber (Tier 1–3, single-word) | **built** (C++ ingestion) | claims 169, 175–181 |
| Multi-word / higher-order construct detection | **planned** | claim 183 |
| Two-LMDB split + ring-buffer reconciliation + loop optimization | **planned** | claims 119/121/282/283 |
| Current GEM internals | **deferred — pending rework** | claims 201/239 |

> **Honesty flag (claims 201/239):** the **current GEM internals are deferred pending rework.** The
> engine described here is real and running, but its internal structure is expected to change in the
> review phase — and substrate facts are current-state, not locked. The PhysX→AZSL migration this
> page now reflects is exactly that kind of change: the earlier PhysX-PBD substrate has been fully
> removed in favour of a host/AZSL settle. Do not treat engine internals as a frozen spec. Deep
> engine documentation is itself deferred (claim 292, see [overview.md](overview.md)). Tracked on
> [../06-status/deferred-and-open.md](../06-status/deferred-and-open.md).

---

## See also

- [overview.md](overview.md) — the three-part meta-structure this engine realizes.
- [../07-operations/build-and-run.md](../07-operations/build-and-run.md) — how to build and run the
  daemon.
