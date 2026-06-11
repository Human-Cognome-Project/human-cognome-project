# Concept Explication Format (CEF) — v1.0 draft

2026-06-10 — the authoring format for registry concept explications. Produced by running the
test suite in [explication-format-test-report-2026-06-10.md](explication-format-test-report-2026-06-10.md)
against a v0.1 candidate; every rule below either survived a test or was forced by a failure.
Non-canonical until Patrick confirms; doctrinal grounding: 500/502 (collapse key = the
explication), 507 (address segments), 513 (forces/axes), 524 (generative rules only),
527 (shader layer), 528 (shape & force library).

---

## 1. What an explication is

A concept explication is a **normalized structure over ISA ops** with six sections:

```
explication := {
  CLASS    registry class slice + per-class bits     (→ address segments, 507)
  SHAPE    static structure: participant slots with co-class requirements,
           edges (KIND/PART/LIKE/SYM), axes & positions, ToM envelopes
  FORCE    dynamics: constraints/preconditions, Δs, settle chains
  READOUT  channel writes (affect, expression, motor, …), if any
  SKINS    surface exponents per language — seam attachment, convenience only
  META     provenance, status, dependencies (by ADDRESS, never surface form)
}
```

Any section may be empty **except that SHAPE and FORCE may not both be empty**
(push: force-heavy/readout-empty; contain: shape-only — both legal).

The **collapse key** is computed over CLASS+SHAPE+FORCE+READOUT in normal form (§4). SKINS and
META are never key material.

## 2. The rules

- **R1 — ISA-closed.** Every op in SHAPE/FORCE/READOUT resolves to the floor+convenience set in
  prime-db-functions.md. No per-concept ops, ever (527). Mechanical check: extract op tokens,
  diff against the ISA list.
- **R2 — Key = generative minimum.** Anything the walkers can derive (spawned forces, magnitudes,
  default inferences) is EXCLUDED from the key and computed at runtime. Putting derivables in the
  key lets two correct authors diverge (proven: hope/fear mirror, T4). Derivables may be cached
  as edges/attrs; they are never identity.
- **R3 — Δ is parameterized:** `Δ(sign, ref, prospect, magnitude)` where `ref ∈ {WANT-graph,
  THINK-graph}` (desire vs expectation). Valence is the WANT-component's sign and is absent, not
  null-forced, for pure THINK-graph readouts (surprise). Multi-Δ concepts conjoin
  (disappointment = Δ(−,WANT) ⊕ Δ(−,THINK)).
- **R4 — Harvested axes only** (513): axes enter the format by recurring in authored data, start
  life as EDGES, and are promoted to CLASS bits only once stable across a domain (a plain 503
  migration). Current affect axis set: valence, arousal(attr), orientation
  {prospective, present, retrospective}, aspect {punctual, sustained, standing}, counter-force
  {available, blocked-now, blocked-always}, attribution locus {self, other, world, none},
  evaluator {own-standard, others-ToM}.
- **R5 — Configs compose.** Across concepts by address (laugh contains surprise's config);
  within a domain by override (`ashamed = sorry ⊕ {evaluator: others-ToM}`). **Keying always
  happens on the EXPANDED form**, so derived and direct authoring collapse identically.
- **R6 — Bit/segment order = harvest-stability order** (most stable axis first). This is what
  makes LoD-as-prefix-truncation read *coarser* rather than *false* (grief → "sad-like").
- **R7 — Magnitudes are attrs, never bits.** terrified = fear + magnitude. If two concepts seem
  to differ only by intensity, look for the real axis first (grief/sad actually differ on
  counter-force horizon, not magnitude).
- **R8 — Boundary discipline:** stimulus-property readouts (beautiful, disgusting) are
  DESCRIPTORS grounding into affect space (467) — edge into the subtree, not an address in it.

## 3. Authoring procedure

1. Write the NSM-style script (the method substrate — unchanged).
2. Factor it into the six sections; slots take role names from the fixed inventory (§4.1).
3. Identify each Δ's reference graph, prospect, and the axis values; check the axis inventory —
   a concept that two existing axes cannot discriminate is the signal to HARVEST a new axis
   (fear/worry → aspect), not to special-case.
4. Strip derivables out of the key (R2): if a clause follows from (valence × prospect ×
   counter), it's walker output, not identity.
5. Expand any composition references (R5), normalize (§4), compute the key.
6. Collision against the registry: same key = same concept (attach the new skin to the existing
   node); near-miss = either a synonym you've authored slightly wrong or a real distinction the
   axis set is missing — resolve before minting.
7. Mint: address per CLASS bits (507), edges for graded structure, skins to the seam,
   provenance to META.

## 4. Normal form (collapse-key normalization)

Specified to the depth the tests exercised; the arbitrary-size procedure is OPEN (§5).

### 4.1 Slots
Fixed role inventory, in binding order: `x` experiencer/agent · `X` target/stimulus ·
`Y` secondary participant · `E` event/Δ carrier · `P` place · ToM envelopes named by
perspective primes (I/YOU/SOMEONE/PEOPLE). No author-invented names.

### 4.2 Ordering
Section order fixed: CLASS, SHAPE, FORCE, READOUT. Within SHAPE: slots, then edges sorted
(rel-type, then target address). Within FORCE: constraints → Δs → settle chains; conjuncts
sorted by (op, args) after slot canonicalization. Axes in harvest-stability order (R6).

### 4.3 References
All concept references by ADDRESS. A surface form anywhere in key material is a format error
(it would make the key skin-dependent).

### 4.4 Absence semantics
- axis **absent** = unspecified (LoD truncation reads through it)
- axis **∅-marked** = tested-neutral (surprise's valence)
These key differently. (T5.)

## 5. Open

- **Canonical ordering at scale** (the canonical-SMILES problem, 511-adjacent): §4.2 is proven
  only on configs ≤ ~6 clauses. Next hard case before trusting it: *promise* (multi-agent,
  temporal, normative).
- Whether `arousal` is an axis at all or always derivable magnitude (R2 may eat it).
- The per-class required/optional axis table beyond affect — to be harvested domain by domain,
  same procedure (513).
