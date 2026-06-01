# Deferred and Open Items

> **This is the honesty page.** Several parts of the architecture are explicitly **deferred,
> in-flux, or not yet specified.** This page collects them in one place so nothing in-flux is
> read as settled. Each item is also flagged inline where it appears. If you are evaluating what
> is *built and stable* vs *intended*, start here.

The items below are sourced from Patrick-direct claims that explicitly mark them open. They are
**not** gaps to hide — they are the current edges of the work.

---

## 1. The deeming / weighting math — the math gap

> *The deeper deeming mechanics — what gets evaluated, with what weight, in what circumstances —
> are deferred to later. This is squarely the math layer.* — claim 286

This is the **acknowledged math gap.** The mechanism and a pragmatic human-set policy are in place
**now** (load-balancing + need-driven frequency, claims 276/277, see
[../04-engine/cognitive-cycle.md](../04-engine/cognitive-cycle.md#who-decides-what-gets-a-tick-the-deeming-policy));
the **principled weighting math** is the deferred deeper layer. It is the determination-engine
optimization/weighting math (confidence-as-softmax-over-curated-candidates, claim 79).

The open **envelope-deeming seam *is* this math layer.** It is the target for the math collaborators
later — see [validation.md](validation.md) (John Bridges, optimization physicist, claim 264). Do not
present a deeming/weighting formula as settled; there isn't one yet.

---

## 2. The bit-class specifics — under current-state review

> *The bit classes (substantive / verb / quantifier / modifier / etc. bit-layouts) and how they
> work will be reviewed as part of the upcoming current-state review.* — claim 237

The bit-class layouts on
[../03-concept-substrate/bit-classes.md](../03-concept-substrate/bit-classes.md) are a **working
snapshot**, not a spec. Specific open dimensional questions — e.g. whether **count/mass** and
**proper/common noun** are first-class substantive bits or handled elsewhere (claims 9, 66–69) — are
**deferred to that review, not resolved now.** Bit counts and cell populations are expected to
change. The review is the planned next phase after the claim-graph distillation.

---

## 3. Inner envelope-deeming composition — not yet fully specified

The envelope cluster has a solid base: an envelope is a db query+filter set shaping a workspace
(claim 273), envelopes are fluid Venn structures / stored query-sets (claims 39/41), and they form
the warm working set (claim 50). **But the inner composition mechanics of how an envelope is
*deemed* / composed are not yet fully specified** (claims 39/40/41 are not fully worked out). What
NAPIER works on each moment is an envelope, self-selected and continuous — *how* that selection is
weighted is part of the deferred math (item 1 above). Document the envelope as a query+filter
workspace; do not invent the deeming-composition detail.

---

## 4. Current GEM internals — deferred pending rework

> *The current ingestion pipeline is implemented as O3DE Gems, much of which will need review.* — claim 239

The engine is real and running (claim 201, see
[../04-engine/implementation-baseline.md](../04-engine/implementation-baseline.md)), but the
**current GEM internals are deferred pending rework.** Internal engine structure is expected to
change in the review phase. Two consequences:

- PhysX-specific substrate facts (velocity.w channel, phase-group predicate, `onAdvance()` callback;
  see [../04-engine/resolution-furnace.md](../04-engine/resolution-furnace.md)) are **current-state,
  not locked.** PhysX is one option within O3DE, not a lock-in (claim 239).
- `HCPPrimePhases.h` is a **flat old snapshot**; the canonical class structure is the per-type bit
  trees, which the engine has not yet caught up to (claim 60).

Do not treat engine internals as a frozen spec.

---

## 5. Linguistic vs conceptual done-state — unclarified by design

> *It has not yet been clarified what is and is not done in the linguistic vs the conceptual realms
> — and clarifying that boundary is part of why this distillation was called for.* — claim 236

A currency consequence to carry: the older (Feb) grammar force-constants and the May-11 prime/force
work are **different aspects of the same thing, not one superseding the other.** Their done/not-done
status is an **open review item**, not a settled supersession. Specifically:

- The **force notation is a rough English expression skin** (claim 241), not a refined force model —
  see [../03-concept-substrate/forces-and-pbd.md](../03-concept-substrate/forces-and-pbd.md). Do not
  present specific bond-strength constants as the spec, and do not mark the Feb force-constant
  material "superseded."
- The aspect-neutral principle that **forces are LoD-relative** (claim 227) stands regardless of how
  the review resolves.

---

## 6. The translation bridge is vision-level

The cross-species / cross-interface translation bridge (claim 247, see
[../02-architecture/interface-and-tom.md](../02-architecture/interface-and-tom.md)) is a
**vision-level destination**, not a shipped capability. What is built today is the single-language
(English text) case; the generality is documented as architectural *intent*. The bridge is only as
honest as its weaker-modeled side.

---

## Planned (not deferred) — engine optimizations not yet implemented

Distinct from the deferrals above, these are **designed but not yet built** (so the engine pages
describe them as planned):

- **Two-LMDB input/output split + ring-buffer reconciliation + RAM-bus loop optimization** (claims
  119/121/282/283) — "planned, not yet implemented as of 2026-05-28." See
  [../04-engine/reconciliation-loop.md](../04-engine/reconciliation-loop.md).
- **Higher-order multi-word construct detection** in the chamber (claim 183) — planned offshoot of
  existing chamber machinery. See
  [../04-engine/resolution-chamber.md](../04-engine/resolution-chamber.md#planned-higher-order-multi-word-constructs).

---

## Known technical debt (tracked, not blocking)

- **Token-reference arrays stored as dotted strings** (claims 31/212) — ~5× bloat, lost B-tree/BVH
  compression. The proper form is `text[]` with diff-only encoding. This is an *intelligence*
  problem, not just storage (it cripples traversal, claim 255). See
  [../05-data-layer/shards-and-schema.md](../05-data-layer/shards-and-schema.md#the-decomposition-pattern-decisions-001--005-reconciled).
- **Migration `048_position_normalization` is superseded by `049_position_arrays`** (claim 205) and
  would wrongly re-run on a from-scratch replay — see
  [../../db/migrations/README_position_history.md](../../db/migrations/README_position_history.md).

---

## How to use this page

When writing or reviewing any HCP doc: if a topic appears here, it **must** be presented as
deferred / in-flux / planned, never as settled. The inline flags in the architecture, concept, and
engine pages point back here. This is a standing requirement, not a one-time note.
