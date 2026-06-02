# Bit-Classes: the Operator Stack

> ## ⚠ Under active review — current-state snapshot, not settled
>
> The bit-class layouts on this page are a **working snapshot** as of late May 2026. Per
> Patrick-direct claim **237**, the bit classes and how they work are **explicitly slated for
> review** as part of the upcoming current-state review (the planned next phase). Specific
> dimensional questions — for example whether *count/mass* and *proper/common noun* are
> first-class substantive bits or handled elsewhere — are **deferred to that review, not
> resolved now.** Bit counts and cell populations below are expected to change. Treat this as
> "where the thinking is," not "the spec." See
> [../06-status/deferred-and-open.md](../06-status/deferred-and-open.md).

Sources: claims 60 (architecture hub), 107 (6 primary types), 7/36/61–64 (verb-class),
9/66–69 (substantive), 70–72 (quantifier), 109/110/112 (modifier), 259 (bits as explication
categorization).

---

## Why bits exist

The bit-layout is the clean **content-type-plus-structure categorization** of explication chains
that NSM leaves muddy (claim 259, see [explication.md](explication.md)). A word's bit-layout
*encodes its explication-as-db-operations*. This is why **the bit-layout is the level-of-detail**
(claim 60): the per-type bit tree is a nested pivot whose conditional bits *are* the LoD at which
a concept is resolved.

The canonical frame (claim 60): **all DB operations at the heart**; the top level is a strict
**class map** of db-instruction role; below each class is a **per-type bit-layout tree** (nested
pivot, conditional bits). `NOT` is a multi-manifest universal operator that sits *alongside* the
class map, native at every level — not a class member.

> Implementation note: `HCPPrimePhases.h` in the engine is still a **flat old snapshot**; the
> canonical structure is the per-type trees described here, which the engine has not yet caught
> up to (claim 60). Do not read the header as authoritative for the current class model.

---

## The 6 primary types (current canonical)

> *6 primary types: SUBSTANTIVE, ACTION, TEMPORAL, OPERATOR, MODIFIER, TERMINATOR.* — claim 107

- **MODIFIER** subsumes the former *QUANTIFIER* primary class as one of its two axes
  (QUANTIFIER + QUALIFIER). Both axes modify a host but along different dimensions.
- **SPATIAL** was collapsed (2026-05-24): it was physical-only and is handled by the verb-class
  physical bit + x/y/z primitives + molecule-layer vocabulary.
- **`NOT`** remains a multi-manifest universal operator alongside the class map.

---

## SUBSTANTIVE

**4 primary bits** (as of 2026-05-24, claim 9): `number`, `pointer/literal`, `has_intent`,
`possessive`.

- **`has_intent`** (claim 66) — does the entity have intent needing ToM-modeling?
  `has_intent=1` (people, animals, modeled minds) needs the Theory-of-Mind framework;
  `has_intent=0` (objects, abstractions) has fixed characteristics. It is a **matched pair** with
  verb-class `requires_intent` for slot-constraint checking and mismatch detection
  (figurative/fiction/parse-error).
- **`possessive`** (claim 67) — does the element participate in possession? A broadphase-relevant
  flag; the owner-vs-possessed relationship is resolved at the bonding layer.
- **`pointer/literal`** — the **pointer cells (pronouns) are the deictic exception zone**
  (claim 68): the literal column is ~500K+ regular entries, while the pointer column is ~100
  exception-dense entries holding all pronoun complexity (I-involvement, direct/indirect person,
  alignment basis, agent/patient forms).
- **`number`** — note that *plural is not a leaf under the number axis* (claim 69): homogeneous
  plurals have interchangeable members (cats); heterogeneous plurals track distinct individuals
  or kinds (we, furniture). An **agentic heterogeneous plural = N distinct ToM constructs running
  concurrently**, not a single aggregate.

---

## ACTION (verb-class)

Action primes/molecules are **state modifiers — programmatically sub-routine calls** (claim 59):
the action prime is the function name, substantive arguments are the particles passed, temporal
anchors (NOW/BEFORE/AFTER) are the *when*, force-dynamic markers are the *how*.

Verb-class refines ACTION via a **2×2** (claims 7/61):

- **Mental / Physical** — engine routing (cognitive vs physics);
- **Declaration / Transformation** — data-op vs math-op.

**THINK** and **DO** are substrate primitives; both operate in both op-modes. Sensory and
expression verbs are *molecule-layer compositions*, not class splits (claim 62): SEE =
read-visual-channel THEN THINK; SAY = THINK THEN write-audio-channel.

**9 primary bits** (as of 2026-05-25, claim 36): transitive/intransitive, past tense, future
tense (both-NULL = NOW), active/passive voice (also covers causative), mood-bit-1, mood-bit-2
(indicative/imperative/interrogative/subjunctive), mental/physical, declaration/transformation,
`requires_intent`. → 512 verb-instance buckets.

Related behaviors: **ambitransitive verbs** carry both interpretations as superposed candidates,
resolved by lowest-energy context (claim 63); **tense bits live in bonding structures**, not
strictly on the verb, and propagate through bonds (claim 64).

---

## MODIFIER (QUANTIFIER + QUALIFIER axes)

**Class-defining invariant** (claim 110): a modifier **requires a bond to a host to be
relevant.** Standalone, it has no substrate referent — it is always partially-applied, awaiting
a bond. At parse time an unbound modifier is a *pending-bond high-tension state* that releases on
binding. Host-type taxonomy: modifier-of-substantive (adjective), modifier-of-action (adverb),
modifier-of-modifier (recursive: "very red", "almost three").

**Axes** (claim 112): three universal + one class-specific.

1. **QUANTIFIER vs QUALIFIER** — quantifier = count/scalar/degree; qualifier = kind-tree.
2. **Substantive-host vs Action-host** — adjective case vs adverb case.
3. **Count vs State** — discrete units vs continuous magnitude.
4. **(Quantifier-only) Absolute vs Scalar** — fires only inside quantifier.

Cell counts: quantifier = 8 (2×2×2), qualifier = 4 (2×2). Sample populations:
`Q×Subst×Count×Abs = {one, two, three}`; `Q×Subst×Count×Scal = {some, many, few}`;
`Qual×Subst×State = {red, smooth, round, fragrant}`; `Qual×Action×State = {quickly, loudly,
suddenly}`. The architecture **permits empty cells** without forcing fills (Qualifier×Count cells
are likely empty in English; may populate from other languages).

- **QUANTIFIER split** (claim 70): **Absolute** (computationally useful directly — only 1 and 0
  are true absolutes) vs **Scalar** (a selection criterion over a collection; every scalar maps to
  a db `SELECT` criterion that determines the array size in play). **Zero** is included as a
  *theoretical* absolute (claim 71) — load-bearing for mathematics (place-value, subtraction);
  it co-exists with the +0 / −0 distinction at the substrate level.
- **QUALIFIER axis is structured as kind-trees** (claim 109), not bespoke per-domain layouts:
  color, texture, shape are domain roots, each a hierarchical kind-tree using the same kind/part
  graph machinery as substantive kinds ("crimson is-a red is-a warm-hue"). The list grows (smell,
  sound, taste, weight, value-judgments…); each new root reuses the same machinery.

---

## OPERATOR / TEMPORAL / TERMINATOR

- **TEMPORAL** — temporal-anchor primes (NOW, BEFORE, AFTER) supply the *when* of an action's
  state change (claim 59); they are a distinct primary class.
- **TERMINATOR** — scope boundaries (the db-function-API role of sentence terminators, claim 5).
- **Cross-class direction bit** (claim 72): a 2-bit direction
  (positive/negative/neither/reserved) is **cross-class** with TERMINATOR math operators because
  the scalar-operator nature crosses class boundaries. Equivalence operators (`=`, SAME) are
  direction-neutral and act as conversion gates between unit structures.

> The punctuation/non-verbal channel (commas, periods, etc.) is treated separately as the
> paralinguistic channel NSM omits — see [punctuation-nonverbal.md](punctuation-nonverbal.md).

---

## Scope note

Per the proof-point (claim 23), the bit design validates against **bounded reading
comprehension**; comprehensive cell-population is downstream and not a current deliverable. Read
this whole page through the review banner at the top.
