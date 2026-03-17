# Grammar Identifier Kernel — Specification
*Linguist specialist — 2026-03-07*

This document specifies the Grammar Identifier kernel for the HCP engine. Its job is to transform a stream of morpheme-resolved word particles into a role-labeled, color+tone-annotated stream ready for concept space entry.

spaCy / Universal Dependencies informed the rule set, but this is kernel code — not a wrapper.

---

## Overview

The Grammar Identifier sits at the boundary between word space and concept space. It does not produce the conceptual mesh — it labels the particles so that the mesh transformation knows the shape to construct.

**Invariant**: Equivalent surface forms must produce identical output annotations. Active/passive, dialect/archaic, fronted/canonical — all equivalent meanings must yield the same role+color+tone assignments. This is the correctness criterion for every rule.

---

## Pipeline Position

```
morpheme-resolved word particles
  (token_id, pos_tag, morph_bits, position)
          ↓
  Grammar Identifier Kernel
          ↓
role-annotated particle stream
  (token_id, pos_tag, morph_bits, position,
   grammatical_role, color, tone,
   bond_target, bond_type, bond_strength,
   physics_properties)
          ↓
  Concept Space Entry (gloss selection → mesh construction)
```

---

## Input Format

Each particle in the input stream carries:

| Field | Type | Source |
|-------|------|--------|
| `token_id` | string (14-byte HCP ID) | Phase 2 resolution |
| `pos_tag` | uint8 | Stored metadata on token |
| `morph_bits` | uint16 | Set during inflection/variant resolve |
| `position` | uint32 | Ordinal position in document |
| `sentence_id` | uint32 | From structural token boundaries |

Sentence boundaries are already marked by structural tokens (`.`, `?`, `!`, newline). The kernel operates sentence-by-sentence.

---

## Output Format

Each particle in the output stream additionally carries:

| Field | Type | Notes |
|-------|------|-------|
| `grammatical_role` | uint8 | See Role Table |
| `color` | uint8 | See Color Table |
| `tone` | uint8 | See Tone Table |
| `bond_target` | uint32 | Position of particle this bonds TO (forward) |
| `bond_type` | uint8 | See Bond Type Table |
| `bond_strength` | float32 | Normalized 0.0–1.0, see per-rule values |
| `field_effect` | uint8 | Non-point effects (negation, modifiers) — see Physics Notes |
| `valence` | uint8 | Number of open bond slots on this particle |
| `mass` | float32 | Physics-adjacent — see Physics Notes |
| `charge` | float32 | Physics-adjacent — see Physics Notes |
| `spin` | float32 | Physics-adjacent — see Physics Notes |

---

## PoS Class Definitions

Stored `pos_tag` values (uint8). These are set at token record level in hcp_english.

| Value | Class | Description |
|-------|-------|-------------|
| 0 | UNKNOWN | Unclassified |
| 1 | N_COMMON | Common noun (cat, table, idea) |
| 2 | N_PROPER | Proper noun / Label (London, Sherlock) |
| 3 | N_PRONOUN | Personal pronoun (he, she, it, they) |
| 4 | V_MAIN | Main verb (run, think, give) |
| 5 | V_AUX | Auxiliary verb (is, has, will, can) |
| 6 | V_COPULA | Copula specifically (be, is, are, was, were) |
| 7 | ADJ | Adjective (red, tall, happy) |
| 8 | ADV | Adverb (quickly, very, never) |
| 9 | DET | Determiner (the, a, an, this, every, all) |
| 10 | PREP | Preposition (on, in, at, by, with, of, to) |
| 11 | CONJ_COORD | Coordinating conjunction (and, or, but, nor) |
| 12 | CONJ_SUB | Subordinating conjunction (because, if, when, that, which) |
| 13 | PART | Particle (to as infinitive marker; up/out in phrasal verbs) |
| 14 | NUM | Numeral (one, two, first, 42) |
| 15 | INTJ | Interjection (oh, ah, well) |

---

## Grammatical Role Table

| Value | Role | Description |
|-------|------|-------------|
| 0 | UNASSIGNED | Not yet classified |
| 1 | PREDICATE | Main verb — the sentence anchor |
| 2 | AUX | Auxiliary supporting the predicate |
| 3 | AGENT | Subject / nominal subject (nsubj) |
| 4 | PATIENT | Direct object (obj) |
| 5 | RECIPIENT | Indirect object (iobj) |
| 6 | PRED_COMP | Predicative complement (after copula) |
| 7 | ATTR_MOD | Attributive modifier (ADJ/ADV pre-modifying N/V) |
| 8 | DET_MOD | Determiner specifying a noun |
| 9 | PP_HEAD | Preposition — head of a prepositional phrase |
| 10 | PP_OBJ | Object of a preposition (N inside PP) |
| 11 | PP_MOD_N | Entire PP modifying a noun |
| 12 | PP_MOD_V | Entire PP modifying a verb |
| 13 | CONJ_LINK | Conjunction linking two constituents |
| 14 | SUB_CLAUSE | Subordinate clause root |
| 15 | NEG | Negation marker (not, never, n't) |
| 16 | EXPLETIVE | Dummy subject (there in "there is", it in "it seems") |

---

## Color + Tone Table

Color = PoS category (intrinsic particle type).
Tone = grammatical variant within that category.

Both are uint8. Color in high nibble (bits 7-4), tone in low nibble (bits 3-0).

### Color assignments

| Color value | PoS class | Display |
|-------------|-----------|---------|
| 1 | Noun (all subtypes) | Blue |
| 2 | Verb (all subtypes) | Red |
| 3 | Adjective | Amber |
| 4 | Adverb | Amber (muted) |
| 5 | Determiner | Grey |
| 6 | Preposition | Green |
| 7 | Conjunction | Purple |
| 8 | Numeral | Cyan |
| 9 | Interjection | White |
| 0 | Unknown/other | Black |

### Tone assignments (per color)

**Blue (Noun) tones:**
| Tone | Role | Display |
|------|------|---------|
| 1 | AGENT (subject) | Bright blue |
| 2 | PATIENT (direct object) | Mid blue |
| 3 | RECIPIENT (indirect object) | Muted blue |
| 4 | PP_OBJ (object of preposition) | Steel blue |
| 5 | PRED_COMP (after copula) | Pale blue |
| 6 | Pronoun | Blue-violet |
| 7 | Proper noun / Label | Deep blue |
| 0 | Unassigned | Flat blue |

**Red (Verb) tones:**
| Tone | Variant | Display |
|------|---------|---------|
| 1 | Transitive (has direct object) | Bright red |
| 2 | Intransitive (no direct object) | Muted red |
| 3 | Ditransitive (recipient + patient) | Deep red |
| 4 | Copula | Rose |
| 5 | Auxiliary | Orange-red |
| 6 | Passive (surface patient is subject) | Crimson |
| 7 | Negated | Dark red |
| 0 | Unclassified | Flat red |

**Amber (Adjective) tones:**
| Tone | Variant | Display |
|------|---------|---------|
| 1 | Attributive (pre-nominal) | Bright amber |
| 2 | Predicative (post-copula) | Muted amber |
| 3 | Comparative (-er / more) | Amber-gold |
| 4 | Superlative (-est / most) | Deep amber |
| 0 | Unclassified | Flat amber |

**Amber-muted (Adverb) tones:**
| Tone | Variant | Display |
|------|---------|---------|
| 1 | Verb modifier | Amber-muted |
| 2 | Adjective modifier (intensifier) | Lighter amber |
| 3 | Sentence modifier (frankly, obviously) | Warm grey |
| 0 | Unclassified | Flat |

**Grey (Determiner) tones:**
| Tone | Variant | Display |
|------|---------|---------|
| 1 | Definite (the) | Mid grey |
| 2 | Indefinite (a, an) | Light grey |
| 3 | Demonstrative (this, that, these, those) | Dark grey |
| 4 | Universal (all, every, each) | Near-white |
| 5 | Existential (some, any) | Warm grey |
| 6 | Possessive determiner (my, his, her) | Blue-grey |
| 0 | Unclassified | Flat grey |

**Green (Preposition) tones:**
| Tone | Relation type | Display |
|------|--------------|---------|
| 1 | Place (on, in, at, above, below, near) | Mid green |
| 2 | Direction (to, into, toward, from) | Bright green |
| 3 | Time (before, after, during, since) | Teal |
| 4 | Cause/reason (because of, due to) | Olive |
| 5 | Instrument/manner (with, by) | Sage |
| 6 | Possession/association (of, for) | Dark green |
| 0 | Unclassified | Flat green |

**Purple (Conjunction) tones:**
| Tone | Variant | Display |
|------|---------|---------|
| 1 | Additive coordination (and) | Mid purple |
| 2 | Adversative coordination (but, yet) | Blue-purple |
| 3 | Disjunctive coordination (or, nor) | Violet |
| 4 | Causal subordination (because, since) | Deep purple |
| 5 | Conditional subordination (if, unless) | Lavender |
| 6 | Concessive subordination (although, though) | Mauve |
| 7 | Relative/complementizer (that, which, who) | Plum |
| 0 | Unclassified | Flat purple |

---

## Bond Type Table

All bonds are FORWARD (from earlier position to later position in stream).

| Value | Bond type | Description |
|-------|-----------|-------------|
| 0 | NONE | No bond |
| 1 | AGENT_BOND | Subject → Predicate |
| 2 | PATIENT_BOND | Predicate → Direct Object |
| 3 | RECIPIENT_BOND | Predicate → Indirect Object |
| 4 | SPECIFIER_BOND | Determiner → Noun |
| 5 | ATTR_MOD_BOND | Adjective → Noun (pre-nominal) |
| 6 | PRED_MOD_BOND | Adverb → Verb |
| 7 | PP_BOND | Preposition → its NP object |
| 8 | PP_ATTACH_N | PP → Noun it modifies |
| 9 | PP_ATTACH_V | PP → Verb it modifies |
| 10 | AUX_BOND | Auxiliary → Main verb |
| 11 | NEG_BOND | Negation → Verb |
| 12 | COORD_BOND | Coordinator → second conjunct |
| 13 | SUB_BOND | Subordinator → clause it introduces |
| 14 | COPULA_COMP_BOND | Copula → Predicative complement |
| 15 | EXPLETIVE_BOND | Expletive → predicate it stands for |

---

## Rule Set

The kernel operates sentence-by-sentence. Within each sentence:

### Phase 1: Identify the Predicate anchor

1. Scan left-to-right for the first `V_MAIN` or `V_COPULA` not marked as auxiliary
2. If auxiliary chain precedes it (`V_AUX ... V_MAIN`), mark all auxiliaries as AUX role
3. The main verb = PREDICATE
4. If no main verb found: mark all particles UNASSIGNED, pass through

**Passive detection**: If morph_bits has `PAST` set AND an auxiliary with BE morph is present before the participle, flag PASSIVE. Remap roles: surface AGENT becomes logical PATIENT, assign tone 6 (crimson) to verb.

**Question detection**: If sentence ends in `?` structural token OR if auxiliary precedes subject (VSO order), flag QUESTION. Role assignments follow the logical (not surface) order.

### Phase 2: Identify Agent (Subject)

Scan left of PREDICATE:
1. Rightmost noun phrase before PREDICATE = AGENT
2. A noun phrase is: optional DET_MOD chain + optional ATTR_MOD chain + N head
3. If EXPLETIVE (`there`, `it`) found in subject position: mark EXPLETIVE, AGENT may be post-verbal (existential construction: "there is a cat")
4. Pronoun in subject position: AGENT, tone 6 (pronoun)

### Phase 3: Identify Patient and Recipient (Objects)

Scan right of PREDICATE:
1. First noun phrase immediately after PREDICATE = PATIENT (if verb is transitive)
2. If two noun phrases follow in sequence before any PP: first = RECIPIENT, second = PATIENT (ditransitive)
3. If `to/for + NP` follows PATIENT: that NP = RECIPIENT (prepositional dative alternation)
4. Copula: no PATIENT. First constituent after copula = PRED_COMP

Transitivity determination:
- `V_COPULA` → always intransitive (no PATIENT)
- `V_MAIN` with NP following → transitive; assign tone 1 (bright red)
- `V_MAIN` with no NP following (PP or end-of-sentence) → intransitive; assign tone 2 (muted red)
- Two NP slots → ditransitive; assign tone 3 (deep red)

### Phase 4: Determiner and Modifier attachment

Left-to-right scan for unassigned particles:
1. DET immediately before N: DET_MOD role, SPECIFIER_BOND to that N, bond_strength = 0.95
2. ADJ immediately before N (or DET+ADJ+N): ATTR_MOD role, ATTR_MOD_BOND to that N, bond_strength = 0.80
3. ADJ after copula: PRED_COMP role (already handled in Phase 3)
4. ADV before or after V_MAIN: PRED_MOD role, PRED_MOD_BOND to that V, bond_strength = 0.65
5. ADV before ADJ (intensifier: "very tall"): ATTR_MOD role modifying ADJ, bond_strength = 0.70

### Phase 5: Prepositional phrase attachment

PP structure: `PREP + NP_object`
1. Mark PREP as PP_HEAD
2. NP immediately following PREP = PP_OBJ (internal to PP)
3. PP attaches to nearest eligible host:
   - If host is N: PP_ATTACH_N bond, bond_strength = 0.50 (ambiguous, proximity-weighted)
   - If host is V: PP_ATTACH_V bond, bond_strength = 0.55
4. PP attachment is inherently ambiguous ("I saw the man with the telescope") — record both candidates if equal proximity, flag `ambiguous_attachment = true`. Downstream force analysis resolves.

### Phase 6: Negation

1. `not` / `n't` (NEG morph bit) immediately associated with verb: NEG role, NEG_BOND to PREDICATE
2. `never`, `nobody`, `nothing`, `nowhere`: NEG role, sentence-scope field effect (see Physics Notes)
3. Negation does not change AGENT/PATIENT role assignments

### Phase 7: Conjunctions and Subordinate clauses

Coordinating conjunctions:
1. CONJ_COORD between two NPs: links them as parallel AGENT or PATIENT fillers
2. CONJ_COORD between two VPs: COORD_BOND, each VP is internally resolved

Subordinating conjunctions:
1. CONJ_SUB introduces a clause: SUB_BOND to the clause root verb
2. Relative clause (which, who, that): SUB_BOND from head noun to relative clause verb
3. Complement clause (think that, know that): SUB_BOND from matrix verb to embedded PREDICATE

---

## Physics-Adjacent Properties

These are noted for the engine specialist to consider. All values are starting estimates — tune by measurement.

### Mass

Mass = tendency to accumulate bonds (be a bond target).

| PoS | Mass | Rationale |
|-----|------|-----------|
| N_COMMON | 1.0 | Nouns are the primary bond-accumulating particles |
| N_PROPER | 1.2 | Labels carry slightly more structural weight |
| N_PRONOUN | 0.7 | Pronouns are lighter — they reference, not anchor |
| V_MAIN | 0.8 | Verbs mediate — they bond but don't accumulate |
| V_COPULA | 0.4 | Copula is nearly massless — it connects, barely exists |
| ADJ | 0.5 | Modifiers have less independent mass |
| ADV | 0.4 | Even lighter modifiers |
| DET | 0.2 | Almost massless — pure specifier |
| PREP | 0.3 | Relational, low independent mass |
| CONJ | 0.1 | Structural connector, minimal mass |

### Charge

Charge = force exerted on neighboring particles (bond-creating force).

| PoS/Role | Charge | Direction |
|----------|--------|-----------|
| V_MAIN (transitive) | +1.0 | Bilateral — pulls AGENT from left, PATIENT from right |
| V_MAIN (intransitive) | +0.7 | Unilateral — pulls AGENT only |
| V_COPULA | +0.5 | Connects subject to complement |
| DET | +0.9 | Strong forward pull toward its noun |
| PREP | +0.8 | Strong pull toward its NP object |
| ADJ (attributive) | +0.6 | Forward pull toward noun |
| N (as AGENT) | −0.4 | Mild backward pull toward DET/ADJ that specify it |

### Spin

Spin = property modification on the target particle (changes the target's conceptual properties without forming a primary bond).

| PoS | Spin effect |
|-----|-------------|
| ADV modifying V | Modifies the verb's force magnitude or direction |
| ADJ modifying N | Modifies the noun's conceptual properties (color, size, state) |
| ADV modifying ADJ (intensifier) | Amplifies or dampens the adjective's spin |
| Comparative/superlative ADJ | Spin has magnitude (more/most = higher spin value) |

### Bond Strength Reference

Higher = tighter coupling between particles.

| Bond type | Strength | Notes |
|-----------|----------|-------|
| SPECIFIER_BOND (DET→N) | 0.95 | Strongest bond in English — determiner+noun is near-unbreakable |
| ATTR_MOD_BOND (ADJ→N) | 0.80 | Strong — attributive modifier tightly coupled to head noun |
| AUX_BOND (AUX→V) | 0.85 | Auxiliary is part of the verb complex |
| NEG_BOND (NEG→V) | 0.80 | Negation is tightly coupled to its verb |
| AGENT_BOND (S→V) | 0.70 | Core grammatical bond — subject to predicate |
| PATIENT_BOND (V→O) | 0.70 | Core grammatical bond — predicate to direct object |
| COPULA_COMP_BOND | 0.65 | Predicative complement — slightly looser |
| RECIPIENT_BOND | 0.65 | Indirect object — slightly looser than direct |
| PRED_MOD_BOND (ADV→V) | 0.65 | Adverbial modification |
| PP_BOND (PREP→NP_obj) | 0.75 | Internal PP bond — tight |
| PP_ATTACH_V | 0.55 | PP→Verb attachment — moderate |
| PP_ATTACH_N | 0.50 | PP→Noun attachment — weakest, most ambiguous |
| COORD_BOND | 0.60 | Coordination — symmetric |
| SUB_BOND | 0.55 | Subordination — clause attachment |

### Field Effects

Some particles do not form point bonds — they modify the force FIELD of the sentence region.

| Particle/role | Field effect |
|---------------|-------------|
| NEG (not, never) | Inverts the polarity of the predicate's patient bond. Downstream: the PATIENT is "the thing NOT acted upon." |
| Sentence-scope NEG (nobody, nothing) | Field inversion over entire predicate region, not just one bond |
| Intensifier ADV (very, quite, rather) | Amplifies spin magnitude of target ADJ/ADV |
| Downtoner ADV (barely, hardly, scarcely) | Near-negation field — dampens without fully inverting |
| Modal AUX (can, could, may, might, must, shall, should, will, would) | Force field modification on the entire predicate — possibility/necessity/volition overlay |
| Conditional CONJ_SUB (if, unless) | Creates a conditional field region — bonds within are hypothetical-weighted |

### Chirality (Passive Voice)

Passive construction ("the cat was chased by the dog") has the same logical bond topology as active ("the dog chased the cat") but with inverted surface assignment.

Model as **bond chirality**: the AGENT_BOND and PATIENT_BOND have the same strength and type, but the chirality flag is set, indicating surface positions are swapped relative to logical roles. Downstream concept space entry uses logical roles (not surface positions) — the chirality flag records that the surface was inverted.

This means: passive and active forms of the same proposition produce identical conceptual mesh topology. The chirality flag is surface metadata, not conceptual structure.

### Valence

Number of open bond slots on a particle.

| PoS/type | Valence |
|----------|---------|
| V_MAIN transitive | 2 (one AGENT slot, one PATIENT slot) |
| V_MAIN intransitive | 1 (AGENT only) |
| V_MAIN ditransitive | 3 (AGENT + RECIPIENT + PATIENT) |
| V_COPULA | 2 (subject + predicative complement) |
| N (as AGENT or PATIENT) | Variable — can accept DET + multiple ADJ + PP attachments |
| DET | 1 (its noun) |
| ADJ (attributive) | 1 (its noun head) |
| PREP | 2 (its NP object + its attachment host) |
| CONJ_COORD | 2 (left conjunct + right conjunct) |

---

## Edge Cases and Disambiguation

### Expletive subjects
"There is a cat on the mat" — `there` = EXPLETIVE, logical subject = "a cat" (post-verbal NP). Real AGENT slot = post-verbal NP. Mark `there` as EXPLETIVE role, zero mass, zero charge.

### Phrasal verbs
"pick up", "look after", "give in" — the PART particle is incorporated into the verb complex. PART bonds to V_MAIN with AUX_BOND (same as auxiliary). The verb + particle together = the PREDICATE unit.

### Fronting / topicalization
"The book, she read." — object moved to sentence-initial position. Detection: NP before subject with no verb between them, followed by a second NP + verb. Role assignment uses logical order (the fronted NP = PATIENT), not surface order.

### Cleft constructions
"It was the cat that sat on the mat." — expletive `it` + copula + focused element + relative clause. Mark `it` as EXPLETIVE. The focused element = PRED_COMP of copula. Relative clause verb = logical PREDICATE of the whole.

### Coordination of unlike categories
"She is clever and kind." — two ADJ in predicative position coordinated. Both = PRED_COMP, COORD_BOND between them.

"He runs and swims." — two V_MAIN coordinated. Both = PREDICATE at their scope level. Shared AGENT bonds to both.

### Ditransitive dative alternation
"She gave him a book" (double object) vs "She gave a book to him" (prepositional dative).
Both = same roles: RECIPIENT (him) + PATIENT (a book).
Surface differs; role output is identical. This is an instance of the invariant requirement.

---

## What This Kernel Does NOT Do

- Does not select glosses — that is the concept space entry step
- Does not construct the NSM molecular structure — that follows from the selected gloss
- Does not resolve PP attachment ambiguity definitively — flags it, downstream resolves
- Does not handle discourse-level structure (coreference, anaphora) — that is above sentence scope
- Does not handle multi-sentence constructions

---

## Implementation Notes for Engine Specialist

- Operates sentence-by-sentence; sentence boundaries from existing structural token stream
- Left-to-right scan with two passes: predicate-finding first, then role assignment outward from predicate
- PP attachment ambiguity: record both candidates, set `ambiguous_attachment` flag, do not block
- All physics values (mass, charge, spin, bond_strength) are float32 starting estimates — expose as tuneable constants, not hardcoded literals
- The `field_effect` field is a bitmask for concurrent field effects (negation + modal simultaneously possible)
- Passive/question detection uses morph_bits already set by prior kernel — no re-parsing needed
- Output stream is same length as input stream (one output record per input particle, no merging)
