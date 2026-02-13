# Concept Mesh Decomposition Analysis

**Author:** hcp_ling (Linguistics Specialist)
**Date:** 2026-02-12
**Status:** Research findings — exploratory investigation

## 1. Overview

The hcp_core database contains the full **conceptual mesh**: 2,744 concept tokens organized in a strict decomposition hierarchy. This document maps how concepts at each layer decompose through lower layers down to irreducible primitives, and identifies the structural patterns that inform PBM design.

**Key numbers:**
- 65 primes (atomic, irreducible)
- 302 basic vocabulary (first-order combinations, no stored definitions)
- 2,352 dictionary entries (fully decomposed using controlled vocabulary)
- 25 structural framework tokens (HCP-specific, outside NSM hierarchy)

All concepts live in the AA namespace under p2=AC ("conceptual"), p3=AA ("NSM"), with p4 differentiating layers.

## 2. Layer Architecture

### Layer 0: NSM Primes — 65 tokens (p4=AA)

Irreducible conceptual atoms. These cannot be decomposed further — they ARE the decomposition endpoints.

**Token ID pattern:** `AA.AC.AA.AA.*`

**Metadata:** `definition` (49/65), `eng_refs` (65/65), `is_universal` (65/65)

**Definition format:** Tony/Lisa illustrative scenarios. These don't *define* the concept — they *demonstrate* its structural behavior using frame markers:

```
do:   Lisa {does} something. | Lisa {does} something {_with} this thing.
      | Lisa {does} something {_to} Tony.
say:  Tony {says} something. | Tony {says} something {_to} Lisa.
      | Tony {says} something {_about} this living thing.
know: Tony {knows} Lisa is inside this thing, because Tony sees Lisa inside.
      | Tony {knows} something {_about} Lisa.
```

**Frame markers** encode valency — the structural slots a concept can fill:
- `{_to}` — directional/recipient complement
- `{_with}` — instrumental complement
- `{_about}` — topic complement
- `{_as}` — equivalence complement

These map directly to the sub-categorization patterns documented in english-sub-cat-patterns.md.

**Categories** (from nsm_category metadata on related AA.AA.AB tokens):
- substantive (6): thing, person, body, I, you, someone
- mental_predicate (7): think, know, want, feel, see, hear
- speech (3): say, word, true
- action (4): do, happen, move, touch
- descriptor (2): good, bad
- determiner (3): this, same, other
- quantifier (5): one, two, some, all, many, more
- evaluator (2): big, small
- space (8): place, inside, above, below, side, near, far, here
- time (7): time, before, after, now, moment, a_long_time, a_short_time
- logical (5): not, if, because, can, maybe
- life_death (2): live, die
- existence (1): there_is
- possession (1): have
- intensifier (2): very, like
- similarity (2): kind, part
- specification (2): for_some_time, that (standard, non-universal)

**16 primes have no stored definition** — these are either self-evident (I, you, here, now, someone) or defined purely by prime status (after, for_some_time, moment, body, die, maybe, below, that, and, or, it).

### Layer 1: Basic Vocabulary — 302 tokens (p4=AB)

First-order concept combinations. These are concepts that can be understood through direct demonstration or simple combination of primes.

**Token ID pattern:** `AA.AC.AA.AB.*`

**Metadata:** `eng_refs` (301/302), `is_universal` (302/302) — but **NO definitions**

**Distribution:** 70 universal, 232 standard

**Examples:** animal, building, car, cause, change, child, colour, control, country, cut, damage, distance, eat, family, food, give, go, group, help, house, machine, make, name, number, plant, read, shape, sound, tool, water, write

These represent the "basic English vocabulary" — words that NSM methodology considers definable but treats as building blocks for higher definitions. Their absence of stored definitions is significant: it means Layer 1 concepts are **implicitly defined** by their usage patterns in Layer 2 definitions.

**Critical relationship:** All 302 Layer 1 names appear as "(See)" entries in Layer 2 (100% overlap). This means every Layer 1 concept has a surface-level entry in the dictionary that says "this concept is self-evident."

### Layer 2: Full Dictionary — 2,352 tokens (p4=AC)

Complete concept definitions using controlled vocabulary decomposed toward primes.

**Token ID pattern:** `AA.AC.AA.AC.*`

**Metadata:** `definition` (2,352/2,352), `eng_refs` (2,316/2,352), `variants` (2,352/2,352), `is_universal` (2,352/2,352)

**Distribution:** 152 universal, 2,200 standard

**Three definition types:**

| Type | Count | Description |
|------|-------|-------------|
| "(See)." only | 210 | Purely self-evident — no additional definition needed |
| "(See)." + extensions | 191 | Self-evident base meaning + polysemic extensions |
| Fully decomposed | 1,951 | Complete reductive definition using lower-layer vocabulary |

**The 401 "(See)" entries break down as:**
- 302 match Layer 1 names (basic vocabulary echoed in dictionary)
- ~49 match prime names (primes with surface-word polysemy)
- ~48 are grammatical forms, pronouns, demonstratives (function words)

**"(See)" + extensions pattern** — these encode polysemy:
```
back:  (See). Before. Move_to the place where you were before.
       Become like you were before. Part_of your body...
can:   (See). Metal container that people make...
good:  (See). This helps someone. Able to_do something. Much.
have:  (See). Part_of. What someone can know and say_about this...
place: (See). Where you expect to_see something. Time. Put.
```

Each "(See)" references the concept's prime/basic meaning; extensions after it are additional senses accessible to the surface word but distinct from the core concept.

### Structural Framework — 25 tokens (p3=AB)

HCP-specific tokens encoding the linguistic force framework. These are NOT part of the NSM hierarchy — they're meta-concepts about how language works.

**Token ID pattern:** `AA.AC.AB.*`

**All universal. Organized by p4:**
- p4=AA: Force types — attraction, binding_energy, compatibility, movement, ordering, structural_repair, valency (7)
- p4=AB: Relationship types — coordination, coreference, determiner_nominal, head_adjunct, head_complement, movement_trace, subject_predicate (7)
- p4=AC: Linguistic units — byte, character, clause, discourse, morpheme, phrase, sentence, word (8)
- p4=AD: Structural principles — binary_branching, endocentricity, phrasal_category_from_head (3)

## 3. Decomposition Mechanism

### 3.1 Vocabulary Control

Layer 2 definitions use approximately **1,019 distinct lexical items** (words and compound forms). Classification:

| Source | Count | Examples |
|--------|-------|----------|
| Primes (Layer 0) | 61 | thing, person, see, know, do, good, bad, this, other |
| Basic vocab (Layer 1) | 255 | animal, cause, change, help, make, name, water |
| Self-referential (See) | 29 | am, be, is, was, something, these |
| Compound forms | ~200 | to_do, the_same_as, good_for, in_a_place, more_than |
| Morphological inflections | ~300 | does, doing, things, happens, caused, becoming |
| Frame markers | 5 | _to, _with, _about, _as, _be |
| Intra-layer references | ~100 | achieve, admire, attract, calculate, confuse |

**Key finding:** The definition vocabulary is overwhelmingly drawn from Layers 0+1, extended only by:
1. Standard morphological inflection (predictable, rule-based)
2. Compound expressions built from lower-layer atoms (joined with underscores)
3. A small set of intra-layer derivational references

This confirms the hierarchy is genuinely reductive — definitions decompose toward primitives, not circularly.

### 3.2 Cross-Reference Patterns

Four distinct reference mechanisms exist within the definitions:

**Pattern 1: (See)** — Lower-layer reference
```
"(See)."  →  "This concept's base meaning is a prime or basic vocabulary item"
```
Appears in 401 entries. Establishes that the surface word's core sense maps to a concept already defined at a lower layer.

**Pattern 2: (See [compound] lesson/ref)** — Compound concept reference
```
"(See [in_a_place] 1G/1-25)"  →  "See the compound concept 'in_a_place' defined in lesson 1G"
"(See [distance_between] 6B/6-05)"  →  "See 'distance_between' in lesson 6B"
```
These reference multi-word concept frames that have specific lesson numbers. The lesson references (e.g., 1G/1-25) correspond to the NSM pedagogical ordering.

**Pattern 3: [word]** — Intra-layer morphological reference
```
"achievement" → "Something you [achieve]."
"admiration"  → "When you [admire] someone or something."
"advertisement" → "What you tell people when you [advertise]."
```
192 concepts reference 223 unique targets. These are almost exclusively morphological families — derived forms pointing to their base. Most references are 1:1 (max fan-in: 2).

**Pattern 4: Underscored compounds** — Inline complex concepts
```
"to_do", "to_be", "to_cause", "to_happen"
"the_same_as", "not_the_same_as"
"good_for", "bad_for"
"in_a_place", "in_front_of", "on_top_of"
"kind_of", "part_of", "amount_of", "number_of"
```
These are multi-word concepts treated as single lexical units within definitions. They're built compositionally from primes and basic vocabulary but function as atomic units in definition syntax.

### 3.3 The Decomposition Chain (Worked Example)

**"accident"** (nsm_ac, fully decomposed):
> Someone causes something bad to_happen, but not because they tried to_make it happen; It happens when people do_not expect it to_happen, and because they do_not expect it, they do_not do things that can prevent it.

Vocabulary breakdown:
- **Primes used:** someone, thing, bad, happen, because, not, do, can, people, time (implicit)
- **Basic vocab used:** cause, expect, prevent, try, make
- **Compounds:** to_happen, to_make, do_not
- **Inflections:** causes, happens, tried

Every non-inflectional, non-compound word traces to Layer 0 or Layer 1.

**"acid"** (nsm_ac, fully decomposed):
> This is a kind_of chemical like the chemical that makes fruit taste sour; This chemical can make holes_in things it touches; Mixing this chemical and another chemical can make salt.

Vocabulary breakdown:
- **Primes used:** this, kind, like, thing, can, another
- **Basic vocab used:** chemical, fruit, taste, sour, salt, make, mix, hole
- **Compounds:** kind_of, holes_in

## 4. The Bridge: eng_refs

Every layer has `eng_refs` — JSON arrays of token IDs in the AB namespace (hcp_english) that link concepts to their English surface expressions.

| Layer | Total | Has eng_refs | Coverage |
|-------|-------|-------------|----------|
| Primes (Layer 0) | 65 | 65 | 100% |
| Basic vocab (Layer 1) | 302 | 301 | 99.7% |
| Dictionary (Layer 2) | 2,352 | 2,316 | 98.5% |

**eng_refs format:** `["AB.AB.CA.AB.Cp"]` — token IDs pointing to hcp_english tokens.

The p3 component of referenced tokens encodes part of speech:
- CA = noun, CB = verb, CC = adjective, CD = adverb, CE = preposition, etc.

This means each concept knows which surface forms can express it, and those surface forms carry grammatical category information. The translation layer (force patterns) mediates between these.

## 5. Key Structural Insights

### 5.1 Layer 1 Is a "Trust Boundary"

Layer 1 (302 basic vocabulary items) has no stored definitions. This is not an omission — it's architecturally significant:

- **Layer 0 (primes):** Defined by demonstration (scenarios)
- **Layer 1 (basic):** Defined by *shared understanding* — no decomposition provided
- **Layer 2 (dictionary):** Defined by decomposition into Layers 0+1

Layer 1 functions as a **trust boundary**: the system assumes these concepts are grounded (either through demonstration or cultural common ground). Everything above Layer 1 decomposes. Nothing below Layer 1 needs to.

### 5.2 Polysemy Is Explicitly Encoded

The "(See) + extensions" pattern explicitly separates a word's core conceptual meaning from its polysemic extensions:

```
"back": Core = (See) [basic concept]. Extensions = Before | Move_to return | Body part
"can":  Core = (See) [ability prime]. Extensions = Metal container
"fall": Core = (See) [basic concept]. Extensions = Become bad | Become less | Autumn season
```

Each extension is a *separate sense* — a different conceptual pathway through the same surface form. This is exactly what PBMs need to encode: one surface token, multiple concept bonds.

### 5.3 The Compound Expression System

Underscored compounds (`to_do`, `the_same_as`, `in_a_place`) function as a **controlled vocabulary extension mechanism**. They're compositional (built from atoms) but conventionalized (used as units).

This creates an implicit intermediate layer between basic vocabulary and full definitions — a set of ~200 recurring multi-word frames that definitions use as building blocks. These are candidates for their own concept tokens or PBM templates.

### 5.4 Morphological Derivation Is Shallow

The [bracket] cross-reference pattern shows that morphological derivation within nsm_ac is almost exclusively one step deep:
- achievement → [achieve] → (fully decomposed)
- admiration → [admire] → (fully decomposed)
- advertisement → [advertise] → (fully decomposed)

No chains deeper than 2 were found. This means PBMs encoding morphological relationships need only single-hop links.

## 6. Implications for PBM Design

### 6.1 Bond Topology

The decomposition hierarchy suggests a natural PBM bond structure:

- **Atomic bonds:** Prime-to-prime relationships (encoded in Layer 0 scenarios)
- **Compositional bonds:** Layer 1 concepts bonded to their constituent primes (currently implicit)
- **Definitional bonds:** Layer 2 concepts bonded to their definition vocabulary
- **Polysemic bonds:** "(See)" entries with extensions — one concept, multiple bond configurations
- **Derivational bonds:** [bracket] references — morphological family links

### 6.2 What's Missing (and Needed)

1. **Layer 1 definitions** — The 302 basic vocabulary items need decompositions (or explicit "grounded" markers) for the mesh to be fully traversable
2. **Compound concept tokens** — The ~200 underscored compounds are used as units but don't have their own concept tokens
3. **Inflectional rules** — Morphological inflections (plurals, tenses, etc.) are used freely in definitions but have no concept-level encoding
4. **Frequency/salience weights** — No indication of which senses are primary vs. secondary for polysemic entries

### 6.3 Suggested PBM Structure

```
Concept Token → [
    PBM_prime_bonds:     [{prime_id, role, frame_marker}]
    PBM_def_bonds:       [{target_concept, relationship_type}]
    PBM_polysemy:        [{sense_index, definition, eng_refs_subset}]
    PBM_derivation:      [{base_concept, derivation_type}]
    PBM_surface_bridges: [{eng_token_id, pos_category}]
]
```

## 7. Complete hcp_core Namespace Map

For reference, the full token structure of hcp_core:

| p2 | p3 | p4 | Count | Content |
|----|----|----|-------|---------|
| AA | AA | AA | 256 | ASCII/byte code characters |
| AA | AA | AB | 65 | NSM prime LABELS (uppercase, byte-code namespace) |
| AB | AA | — | 2,162 | Unicode character definitions |
| **AC** | **AA** | **AA** | **65** | **NSM primes (concepts)** |
| **AC** | **AA** | **AB** | **302** | **Basic vocabulary (no definitions)** |
| **AC** | **AA** | **AC** | **2,352** | **Full dictionary (decomposed)** |
| **AC** | **AB** | — | **25** | **Structural framework** |
| AD | AA | — | 4 | Format markers |
| AD | AB | — | 28 | Code page definitions |
| | | | **5,259** | **Total** |

The conceptual mesh (bold rows) contains 2,744 tokens. The remaining 2,515 are byte-code infrastructure.
