# Project Status

Current state of the Human Cognome Project, sourced from the orchestrator claim-graph and verified
against the live databases and engine. For the explicit deferrals and planned-but-unbuilt items,
read [deferred-and-open.md](deferred-and-open.md) alongside this.

**As of 2026-06-01.**

---

## One-paragraph summary

The linguistic engine is real, running software: an O3DE/PhysX 5 C++ Gem that ingests text, resolves
it through a physics-based resolution chamber, stores documents in an inference-conducive format, and
reproduces them at >98% accuracy. The English vocabulary substrate is ~1.494M entries on NAS. In
2026-04 the project **pivoted** from document storage (done-enough) to **NSM concept modeling**, and
the current active work is **defining the primitive db functions** — the elemental operations that
let every word resolve to db operations that translate to explication statements. The deeper
deeming/weighting math and the bit-class specifics are explicitly deferred to a current-state review.

---

## Done / stable

- **Linguistic engine baseline** (claim 201) — O3DE 25.10.2 C++ Gem, PhysX 5 GPU PBD, headless
  daemon on port 9720, two-phase resolution pipeline, ~21,300 LOC / ~35 modules. See
  [../04-engine/implementation-baseline.md](../04-engine/implementation-baseline.md).
- **English vocabulary substrate** (claim 203) — `hcp_english` ≈ **1,494,216 entries** (verified
  live), full Kaikki re-ingestion. 10 data shards on NAS HAVEN. See
  [../05-data-layer/shards-and-schema.md](../05-data-layer/shards-and-schema.md).
- **Schema decomposition** (claim 207) — token addresses decomposed to `ns/p2/p3/p4/p5`; reference
  arrays as native ARRAY columns (decision 005's junction-table approach abandoned).
- **Resolution chamber, single-word** (claims 169, 175–181) — three-tier resolution implemented in
  the C++ ingestion engine.
- **Data pipeline** — `source_wiktionary → source_english` (delta-dedup, 100% drained) →
  `hcp_english`. See [../05-data-layer/kaikki-pipeline.md](../05-data-layer/kaikki-pipeline.md).

---

## Done, then paused (the 2026-04-16 pivot)

- **Document storage + reconstruction** (claim 204) — >98% reproduction accuracy; ~99% of words
  stored at the highest-level word-construct token; 9-document Gutenberg stress test clean (var rates
  0.15–0.59%). **Paused** per the pivot.
- **Paused alongside it** (claim 99): phrase / idiom / Proper-Entity resolution chambers; higher-order
  tokenization; possessive handling; Label propagation. These constructs *exist* in the shards but
  are not yet assigned to resolution chambers; they are not revisited until story-level constructs are
  evaluated in concept space.

> The pivot is **not** a reframe of the roadmap — NSM concept modeling is the next sub-step *within*
> Phase 1 linguistics (claim 206, see [../../ROADMAP.md](../../ROADMAP.md)).

---

## Active work

> *The area currently being actively worked is the **definition of the primitive functions** — the
> core db elemental operations and how they combine.* — claim 291

Concretely (claims 257–261, 291): extracting the **language-independent elemental base out of
English** (English is the worked example mined for the universal substrate, *not* the thing being
rebuilt — claim 257), such that every word resolves to db operations that translate to **explication
statements** (claim 258). This spans:

- the **construction-blueprint** ops (grammar/PoS assembly, content-blind),
- the **usage/explication** ops (the *why* — what a structure is for),
- the **punctuation/non-verbal** functions NSM assumes but does not specify (claim 260).

See [../03-concept-substrate/explication.md](../03-concept-substrate/explication.md) and
[../03-concept-substrate/punctuation-nonverbal.md](../03-concept-substrate/punctuation-nonverbal.md).

---

## Deferred (see the honesty page)

Summarized here; detailed on [deferred-and-open.md](deferred-and-open.md):

- the **deeming/weighting math** (the math gap, claim 286),
- the **bit-class specifics** (under current-state review, claim 237),
- the **inner envelope-deeming composition** (claims 39/40/41 not fully specified),
- the **current GEM internals** (pending rework, claims 201/239),
- the **linguistic-vs-conceptual done-state** (unclarified by design, claim 236).

---

## Planned (designed, not yet built)

- two-LMDB input/output split + ring-buffer reconciliation + RAM-bus loop optimization (claims
  119/121/282/283),
- higher-order multi-word construct detection in the chamber (claim 183).

---

## Infrastructure note

All authoritative databases live on **NAS HAVEN, `192.168.68.60:5435`** (claim 203). See
[../07-operations/database-access.md](../07-operations/database-access.md) for connection details.

---

## See also

- [../../ROADMAP.md](../../ROADMAP.md) — the 4-phase macro arc.
- [deferred-and-open.md](deferred-and-open.md) — the full honesty page.
- [validation.md](validation.md) — adjacent-worker and cross-model validation signals.
