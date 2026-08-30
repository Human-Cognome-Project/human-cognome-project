# Roadmap

The project was re-founded on the field-balancing physics basis in August 2026 (see
[docs/physics-basis.md](docs/physics-basis.md)). The prior 4-phase linguistic arc is archived with
the paradigm that defined it. The new arc, in dependency order:

## 0. The rewrite (this phase — largely complete)

Review every artifact of the previous era, disposition it, archive honestly, and rewrite the
project so others can understand it before anything is built. Record: [REBASE_REVIEW_PLAN.md](REBASE_REVIEW_PLAN.md)
and [review/](review/). Remaining in this phase: the GitHub issue sweep, and review of the
orchestrator claim-graph (out-of-date with the new paradigm; superseded on its own schedule).

## 1. The clean substrate

Define the new schema from the precept (flat pool, arrayed-pair addresses, one-level composition,
flags, provenance paths) and stand it up empty. Extraction then pulls from the old stores
(read-only, per [docs/data-protocol.md](docs/data-protocol.md)): arrayed addresses from the
decomposed columns, O/o drift corrected with a kept mapping table, sentinels flagged. The old
databases are never modified.

## 2. The landing lattice

Place Kaikki/Wiktionary first — the full lexical inventory at the present face, etymologies as
drafted temporal flows, reconstructed forms flagged model-produced. This gives every subsequent
element somewhere to land.

## 3. Corpus accretion

Documents enter as byte-particle streams (UTF-8 bytes are already two-nibble particles) and accrete
onto the lattice: Gutenberg first (corpus and provenance metadata already in hand), then further
aggregator emissions with their raw faces where recoverable. Creators and works get placed on
temporal coordinates; target flows connect placements; unresolvable differentials are detections of
unread sources.

## 4. The engine

The single balancing operation on Taichi's sparse LoD machinery — `taichi_core` is pure C++;
direct C++ against the core is the probable path, evaluated as we go. Method requirements fixed in
advance: a research corpus before the port (the AZSL-corpus method from the previous era), and
oracle-first validation for every kernel (CPU reference oracle, GPU mirror, hardware equivalence
harness). Tests on everything.

## 5. NAPIER

Inference as flow-solving in the emitted field, on the far side of the compression. The DI's
substrate contract is the compressed field only.

## Standing open problems (physics)

Carried from the packages, in view of anyone who wants them: ladder coefficients (Lamoreaux's
five-percent number is the first target), the surplus/flow crossover, retardation as
density-dependent propagation, the empty lower ledger, and the amount-side re-reading protocol.
See [docs/physics-basis.md](docs/physics-basis.md) § Open problems.
