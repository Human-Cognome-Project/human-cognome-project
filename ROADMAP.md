# Roadmap

The macro structure of the Human Cognome Project is a **4-phase meta-level arc** with a genuine
dependency chain — the phases are not loose labels (claim 206, Patrick-direct, still operative).

For the *current* live status within Phase 1, see
[docs/06-status/status.md](docs/06-status/status.md). For what is explicitly deferred, see
[docs/06-status/deferred-and-open.md](docs/06-status/deferred-and-open.md).

---

## The four phases

### Phase 1 — Linguistic Engine (English first)

Build the substrate that turns text into traversable, inference-conducive structure. **This is the
project's primary external-facing proof of concept** (the "linguistic proof"). The proof-point target
is **grade-school reading comprehension — the lower grade the better** (claim 23): traceable
inference on bounded data, not scale. ("Bob's Pets" kindergarten-primer level is the concrete test.)

Phase 1 includes two sub-steps:

1. the linguistic engine itself (resolution chamber, document storage, vocabulary substrate) —
   **largely built**, see [docs/06-status/status.md](docs/06-status/status.md); and
2. **NSM concept-primitive modeling** — the 2026-04-16 pivot focus (claim 99). This is **not** a
   reframe of the roadmap; it is the next sub-step *within* Phase 1. The current active work —
   defining the primitive db functions (claim 291) — lives here.

### Phase 2 — Identity & Theory of Mind

The personality DB (seed + living layer) and relationship DB. The **seed** is a deterministic
starting condition (baseline bonding patterns, force weightings, abstraction preferences) —
proto-identity, reproducible from a known seed; the **living layer** is the accumulated-experience
trajectory (claim 233). This makes a NAPIER instance reproducible yet free to grow.

### Phase 3 — Full Text Inference

NAPIER as an orchestration layer for full inference. **Phase 3 assumes full ToM modeling (Phase 2)** —
not just LLM-type operations. This dependency is real: full inference is built *on* the identity/ToM
substrate, not beside it.

### Phase 4 — Multi-Modality

The same structural primitives extended to audio / visual / other expression modes. This is where the
[conceptual hub](docs/02-architecture/linguistic-vs-conceptual.md) and the
[interface-agnostic capability](docs/02-architecture/interface-and-tom.md) pay off: every modality is
another interface mapped onto the shared concept layer (claims 246/267/268). Audio is waves of force;
the physics substrate is modality-universal.

---

## The dependency arc

```
Phase 1 (Linguistic Engine, English)
   │   proof-point: grade-school reading comprehension (claim 23)
   │   includes: NSM concept-primitive modeling (the pivot, within Phase 1)
   ▼
Phase 2 (Identity & Theory of Mind)
   │   personality DB seed + living layer; relationship DB
   ▼
Phase 3 (Full Text Inference)   ◀── assumes full ToM modeling from Phase 2
   ▼
Phase 4 (Multi-Modality)        ◀── same primitives, extended to audio/visual/other
```

NAPIER is the inference-model name (claim 202).

---

## What is *not* current

The per-phase "Current Focus" / status sub-lists from the older (2026-03-17) roadmap are **stale on
specifics** — only this macro arc + dependency structure is current (claim 206). For the live state,
always defer to [docs/06-status/status.md](docs/06-status/status.md), which is sourced from the
orchestrator claim-graph.
