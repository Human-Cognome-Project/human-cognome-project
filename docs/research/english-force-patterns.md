# English Force Patterns

**Purpose:** Define the physics constants that govern structural assembly and disassembly of English sentences. These are the force rules the inference engine (NAPIER) needs to interpret input and construct output structurally.

**Scope:** English only. Other languages will have different constants but use the same force types.

**Source:** Derived from *Analysing Sentences* (4th ed., Burton-Roberts). See [sentence-analysis-framework.md](sentence-analysis-framework.md) for the full textbook digest.

---

## Force Type Inventory

The architecture defines two conceptual forces: **gravitational** (attraction/clustering) and **albedo** (reflectivity/representativeness). For language, these manifest as specific, enumerable force patterns:

| Force Type | What It Does | Physics Analogy |
|---|---|---|
| Attraction | Pulls tokens toward each other for assembly | Gravitational force |
| Binding energy | Holds assembled tokens together; cost to disassemble | Bond energy |
| Ordering constraint | Enforces positional sequence | Directional force field |
| Category compatibility | Determines what CAN bond with what | Charge compatibility |
| Valency | Determines how MANY bonds a token requires/permits | Electron shells |
| Movement force | Pulls tokens from deep-structure to surface position | Electromagnetic attraction |
| Structural repair | Inserts tokens to satisfy unmet constraints | Virtual particle creation |

---

## 1. Attraction Forces

Attraction determines which tokens want to be near each other. Different structural relationships have different attraction strengths.

### Attraction Strength Scale

| Relationship | Strength | Obligatoriness | Direction |
|---|---|---|---|
| Head ↔ Complement | **STRONG** | Both obligatory | Mutual |
| Subject ↔ Predicate | **STRONG** | Both obligatory | Mutual |
| Determiner → Nominal | **STRONG** | DET obligatory for singular count N | One-way |
| Head ← Modifier | **MODERATE** | Head obligatory, modifier optional | One-way (toward head) |
| Coordination bond | **MODERATE** | Both conjuncts obligatory once initiated | Mutual |
| Adjunct → Host | **WEAK** | Host obligatory, adjunct optional | One-way (toward host) |
| Non-restrictive relative → NP | **WEAK** | Parenthetical, freely detachable | One-way |

### Attraction Rules

**Rule A1: Head-Complement Attraction**
A head token attracts complements of the categories specified by its sub-categorization features. The head cannot be satisfied without its complement(s), and the complement(s) cannot appear without their head.

- Transitive V attracts exactly 1 NP
- Ditransitive V attracts exactly 2 NPs (or NP + PP)
- Prepositional V attracts exactly 1 PP
- Preposition attracts exactly 1 NP (or, for temporal P: 1 S)
- Intensive V attracts exactly 1 predicative (AP, NP, or PP)

**Rule A2: Subject-Predicate Attraction**
At sentence scope, NP (subject) and VP (predicate) attract each other with strong mutual force. Neither can exist without the other at S level. This is the sentence-level bonding rule.

**Rule A3: Modifier-Head Proximity**
Modifiers are attracted toward their head. When a modifier has its own internal complexity (complements), proximity pressure may push it to post-position to keep the modifier's own head close to the modified head. This is the **Friendly Head Principle** — it's an energy-minimization constraint, not a hard rule.

**Rule A4: Determiner-Nominal Attraction**
Singular count nouns require a determiner. Mass nouns and plural count nouns can satisfy this with an empty (null) determiner. The DET position is always structurally present.

---

## 2. Binding Energies

Binding energy is the cost of removing a constituent from its assembly. High binding energy = hard to remove = obligatory. Low binding energy = easy to remove = optional.

### Binding Energy Scale

| Bond Type | Binding Energy | Removal Result |
|---|---|---|
| Head–complement | **HIGH** | Ungrammatical (structure collapses) |
| Subject–predicate | **HIGH** | Ungrammatical (not a sentence) |
| DET–NOM (singular count N) | **HIGH** | Ungrammatical (*"dog barked") |
| Head–adjunct | **LOW** | Grammatical (structure simplified) |
| S-adverbial–S | **LOW** | Grammatical (loses speaker comment or discourse link) |
| Non-restrictive relative–NP | **LOW** | Grammatical (loses parenthetical info) |
| Restrictive relative–NOM | **MODERATE** | Grammatical but meaning changes (referent becomes ambiguous) |

### Binding Energy Rules

**Rule B1: Complement Binding**
Complements have high binding energy to their head. Removing a complement from a transitive V produces an ungrammatical result: *"She read ∅" is incomplete. The constituency test for complements is that they FAIL the omission test.

**Rule B2: Adjunct Binding**
Adjuncts have low binding energy. Removing an adjunct preserves grammaticality: "She read the book quickly" → "She read the book" — perfectly fine. Adjuncts PASS the omission test.

**Rule B3: Restrictive vs. Non-Restrictive**
Restrictive relatives modify meaning (moderate binding — removal changes reference): "The man **who wore the hat** left" → "The man left" (which man?). Non-restrictive relatives add dispensable information (low binding): "John, **who wore a hat**, left" → "John left" (still clear who).

---

## 3. Ordering Constraints

English has specific positional rules. These are directional force fields — they don't just determine WHETHER tokens bond, but WHERE they must be relative to each other.

### Head-Direction Parameter
**English is predominantly head-initial** (within phrases):
- V precedes its complements: "read **the book**"
- P precedes its complement: "in **the garden**"
- Exception: subject precedes predicate (subject-initial)

### Specific Ordering Rules

**Rule O1: Auxiliary Ordering (ABSOLUTE)**
```
MOD > PERF(have) > PROG(be) > PASS(be) > Lexical V
```
This ordering is inviolable. No grammatical English sentence reverses any pair in this sequence. Each auxiliary takes the following VP as its complement.

Violation examples (all ungrammatical):
- *"She has will leave" (PERF before MOD)
- *"She is having left" (PROG before PERF)
- *"She is being have left" (PASS before PROG)

**Rule O2: NP Pre-Modifier Ordering**
```
PRE-DET > DET > QA > AP > N(compound) > N(head)
```
- "**All** **the** **three** **tall** **garden** **chairs**"
- Not absolute (AP order has some flexibility), but QA always precedes AP, and compound N immediately precedes head N.

**Rule O3: NP Post-Modifier Ordering**
```
N(head) > PP > Relative Clause
```
- "the man **in the hat** **who I met**"
- PPs are closer to the head noun; relative clauses are further out.

**Rule O4: Clause Structure**
```
C2 > C1 > S(NP + VP)
```
- C2 holds fronted wh-phrases (outermost)
- C1 holds complementisers or fronted auxiliaries
- S holds the core subject + predicate

**Rule O5: Complement Before Adjunct (within VP)**
```
V > Complement(s) > Adjunct(s)
```
- "She put **the book** **on the shelf** *carefully*" — complements first, adjuncts after
- Adjuncts can be more mobile (fronting, etc.) but the default position is after complements

**Rule O6: Tense Position**
Only the first verb in the auxiliary chain carries tense. Each subsequent verb takes the morphological form dictated by the preceding auxiliary:
- MOD → bare stem
- PERF → past participle (-en/-ed)
- PROG → present participle (-ing)
- PASS → past participle (-en/-ed)

---

## 4. Category Compatibility

These are the "charge rules" — what categories of tokens can bond with what.

### Compatibility Matrix

**Rule C1: Coordination Constraint**
Only constituents of the **same category** can be coordinated with "and"/"or":
- NP and NP: "the dog **and** the cat"
- VP and VP: "ran **and** jumped"
- AP and AP: "tall **and** strong"
- *NP and VP: *"the dog **and** jumped" — UNGRAMMATICAL

This is an absolute constraint — no exceptions.

**Rule C2: Verb Sub-categorization Compatibility**

| Verb Type | Complement Category | Example |
|---|---|---|
| Transitive | NP | "read **the book**" |
| Intransitive | ∅ | "laughed" |
| Ditransitive | NP + NP (or NP + PP) | "gave **him** **a book**" |
| Intensive | AP / NP / PP | "seemed **happy**" |
| Complex transitive | NP + (AP / NP / PP) | "made **him** **happy**" |
| Prepositional | PP (specific P) | "relied **on him**" |

Note: prepositional verbs select not just PP but a *specific preposition*: "rely **on**" not *"rely **at**". The preposition is part of the verb's sub-cat specification.

**Rule C3: Clausal Complement Compatibility**
Verbs also sub-categorize for clause types:
- "want" takes to-infinitive: "wants **to leave**"
- "enjoy" takes -ing participle: "enjoys **swimming**"
- "see" takes bare infinitive: "saw him **leave**"
- "believe" takes NP + to-infinitive (Type I): "believes **him to be guilty**"
- "persuade" takes NP + to-infinitive (Type II): "persuaded **him to leave**"

These are not interchangeable: *"wants **leaving**", *"enjoys **to swim**" — UNGRAMMATICAL.

**Rule C4: Determiner-Noun Compatibility**
- Count nouns take: a/the/this/every/each + singular; the/these/those/some/many + plural
- Mass nouns take: the/this/some/much + (no plural)
- Proper nouns take: ∅ (no determiner) — they are already full NPs

**Rule C5: Adjective Complement Compatibility**
Adjectives sub-categorize for complement type:
- Type A (reluctant-type): takes to-infinitive, subject controls lower subject
- Type B (impossible-type): takes to-infinitive, subject controls lower object
- Some take PP: "fond **of cheese**"
- Some take S': "aware **that she left**"

**Rule C6: Preposition Complement Compatibility**
- Most prepositions take NP: "in **the garden**"
- Temporal prepositions (after, before, until, since) also take S: "after **she left**"
- Some take -ing clause: "without **leaving**"

---

## 5. Valency

Valency is the number and type of bond slots a token has available. It's the most direct input to the physics engine — it determines how many particles need to be assembled around a given token.

### Verb Valency Table

| Sub-category | Bond Slots | Slot Types | Obligatory? |
|---|---|---|---|
| Intransitive | 0 | — | — |
| Transitive | 1 | NP(dO) | Yes |
| Ditransitive | 2 | NP(iO) + NP(dO) | Yes |
| Intensive | 1 | AP/NP/PP (sP) | Yes |
| Complex transitive | 2 | NP(dO) + AP/NP/PP (oP) | Yes |
| Prepositional | 1 | PP (PC) | Yes |

All complement slots are obligatory. Adjunct slots are unlimited and optional.

### Noun Valency
- Some nouns take PP complements: "student **of physics**", "destruction **of the city**"
- Some nouns take S' complements: "the claim **that she left**"
- Most nouns have 0 complement slots (adjuncts only)

### Adjective Valency
- Some adjectives take PP complements: "fond **of cheese**", "angry **at him**"
- Some take S' or infinitival complements: "glad **that she left**", "eager **to please**"
- Many have 0 complement slots (modified only by degree adverbs)

### Preposition Valency
- Always exactly 1 slot (NP or S or -ing clause)
- Prepositions are never intransitive in English

### Sentence-Level Valency
- S always requires exactly 2 constituents: NP(subject) + VP(predicate)
- Both obligatory, both at S level

---

## 6. Movement Forces

Some tokens appear in a different surface position than their structural (deep) position. Movement is driven by specific forces.

### Movement Rules

**Rule M1: Passive Movement**
- **Trigger:** Passive voice (PASS auxiliary + past participle)
- **What moves:** Direct object NP
- **From:** Complement position within VP
- **To:** Subject position (specifier of S)
- **Trace:** Gap (●) left in original position
- **Force:** Empty subject position attracts nearest eligible NP

**Rule M2: Wh-Fronting**
- **Trigger:** Interrogative or relative clause formation
- **What moves:** Wh-phrase (who, what, which NP, where, when, etc.)
- **From:** Any position within S (subject, object, complement of P, adverbial)
- **To:** C2 position (daughter of S'')
- **Trace:** Gap (●) left in original position
- **Force:** C2 position exerts strong attraction on wh-marked constituents

**Rule M3: Auxiliary Fronting (Subject-Auxiliary Inversion)**
- **Trigger:** Direct question formation
- **What moves:** The tensed (first) auxiliary
- **From:** Pre-VP position within S
- **To:** C1 position (daughter of S')
- **Trace:** Empty position in original location
- **Force:** C1 attracts tensed auxiliary in interrogative context
- **Note:** Does NOT apply in subordinate questions: "I wonder what she bought" (no inversion)

**Rule M4: Extraposition**
- **Trigger:** Clausal subject (heavy constituent in subject position)
- **What moves:** The clause
- **From:** Subject position
- **To:** End of sentence (right-peripheral)
- **Placeholder:** Expletive "it" fills vacated subject position
- **Force:** Heavy constituents are repelled from subject position (weight-based displacement)

**Rule M5: Particle Movement**
- **Trigger:** Phrasal verb with NP object
- **What moves:** Particle
- **From:** Immediately after V
- **To:** After the NP object
- **Constraint:** Only with non-pronominal NPs. Pronouns force particle movement: "looked **it** up" not *"looked up **it**"
- **Force:** Light pronoun objects resist separation from V; heavy NP objects can intervene

### Movement Hierarchy
Movement forces have a priority ordering:
1. Wh-fronting (strongest — crosses clause boundaries)
2. Auxiliary fronting (clause-internal, to C1)
3. Passive movement (VP-internal to S-level)
4. Extraposition (weight-driven, language-specific preference)
5. Particle movement (phrasal-verb-internal, optional)

---

## 7. Structural Repair Forces

When structural constraints aren't met, the system generates tokens to satisfy them. These are analogous to virtual particles in physics — created by the field itself to maintain consistency.

### Repair Rules

**Rule R1: Do-Support**
- **Trigger:** Negation or question requires tensed auxiliary, but no auxiliary is present
- **Action:** Insert `do` as semantically empty tense carrier
- **Example:** "She left" → "She **did** not leave" / "**Did** she leave?"
- **Constraint:** Only activates when no other auxiliary is available

**Rule R2: Expletive Insertion**
- **Trigger:** Extraposition moves clause out of subject position, leaving it empty
- **Action:** Insert expletive "it" as structural placeholder
- **Example:** "That she left surprised me" → "**It** surprised me that she left"
- **Constraint:** Only fills subject position; only triggered by clausal movement

**Rule R3: Empty Determiner**
- **Trigger:** Mass noun or plural count noun in NP that requires DET structurally
- **Action:** DET position exists but is filled by ∅ (null element)
- **Example:** "∅ Water flows", "∅ Dogs bark"
- **Note:** The structural position is real (DET + NOM), only the surface realization is empty

**Rule R4: Covert Subject (PRO)**
- **Trigger:** Non-finite clause requires subject but cannot have overt NP
- **Action:** Covert NP (PRO) fills subject position
- **Controlled PRO:** Co-indexed with an NP in the higher clause — "She_i wants PRO_i to leave"
- **Free PRO:** General/indefinite reference — "PRO_arb to err is human"
- **Force:** Higher-clause controller attracts co-reference from lower clause's empty subject

---

## 8. Structural Ambiguity Resolution

When the same surface sequence is compatible with multiple structural analyses, the system is in a **soft-body state**. Resolution requires determining which parse has the lowest energy.

### Ambiguity Types

**Type SA1: Attachment Ambiguity**
- "She saw the man with the telescope"
  - Parse A: PP "with the telescope" modifies VP (she used a telescope to see)
  - Parse B: PP "with the telescope" modifies NP "the man" (the man had a telescope)
- **Resolution input:** Context, relative bond strengths, frequency of attachment patterns

**Type SA2: Category Ambiguity**
- "The old men and women"
  - Parse A: AP "old" modifies only "men" → [old men] and [women]
  - Parse B: AP "old" modifies the coordinated NP → old [men and women]
- **Resolution input:** Prosody, context, scope of modifier

**Type SA3: Sub-categorization Ambiguity**
- "She believes him to be guilty" vs. "She persuaded him to leave"
  - Same surface: V + NP + to-infinitive
  - Different structure: Type I (NP = lower subject only) vs. Type II (NP = upper object + lower subject)
- **Resolution input:** Verb's sub-cat features (the verb itself disambiguates — this is a lexical lookup, not soft-body)

**Type SA4: Complement vs. Adjunct Ambiguity**
- "the student of physics in the library"
  - "of physics" = complement of N "student" (selected by the noun)
  - "in the library" = adjunct of NOM (freely added)
- **Resolution input:** Noun's sub-cat features, PP semantics

### Resolution Principle
The physics engine explores multiple parse states simultaneously (parallel particles per NAPIER). Each parse has an energy cost. The lowest-energy parse wins. Energy inputs include:
- Bond strength from known patterns (PBM statistics)
- Sub-categorization feature satisfaction (does the verb expect this complement type?)
- Ordering constraint satisfaction (are things in their expected positions?)
- Contextual fit (does this parse make sense given prior discourse?)

---

## 9. Force Constants Summary

These are the values that would be encoded in the English language database as a specialized number set.

### Constant Categories

**Category 1: Verb Sub-categorization Table**
Every verb entry specifies: complement count, complement categories, complement functions, clausal complement types accepted. This is the most complex force constant set — there are hundreds of distinct sub-cat patterns.

**Category 2: Ordering Rules**
A finite set of positional constraints (O1-O6 above). These are boolean/absolute in most cases (auxiliary ordering is inviolable), with some having preference rankings (modifier ordering within NP has some flexibility).

**Category 3: Category Compatibility Matrix**
A lookup table: for each head category, what complement categories are possible? For each modifier, what heads can it modify? For coordination, same-category check.

**Category 4: Binding Energy Values**
Relative binding strengths for each relationship type. These determine how easily a constituent can be removed or moved. Not absolute numbers yet — the scale needs to be calibrated against actual PBM data.

**Category 5: Movement Rule Set**
A finite set of movement transformations (M1-M5 above). Each specifies: trigger condition, what moves, from where, to where, what trace is left.

**Category 6: Repair Rule Set**
A finite set of structural repair operations (R1-R4 above). Each specifies: trigger condition, what is inserted, where, constraints.

**Category 7: Ambiguity Resolution Weights**
Preference rankings for common ambiguity types. These will be partially derived from PBM statistics (frequency of each parse) and partially from structural principles (e.g., minimal attachment preference).

---

## Open Questions for Project Lead

1. **Numeric scale:** What range should binding energies use? A simple ordinal (1-5) or a continuous scale (0.0-1.0)? The physics engine likely needs continuous values, but the linguistics only distinguishes ~4 levels (high/moderate/low/none).

2. **Sub-cat storage:** Verb sub-categorization is the largest and most complex constant set. Should each verb have its sub-cat pattern as a dimension on the token, or as a separate lookup table linked to the token? (I know storage isn't my domain, but this affects how the inference engine accesses the force constants.)

3. **Calibration from data:** Some of these force constants are absolute rules (auxiliary ordering, coordination constraint) and some are preferences (modifier ordering, ambiguity resolution). The absolute rules are known from the grammar. The preferences need calibration from corpus data. Should we distinguish these two types formally?

4. **Cross-linguistic universals:** Some of these forces are English-specific (head-initial ordering, specific auxiliary order). Some are universal (head-complement attraction is strong in every language, coordination requires same category universally). Should the universal forces be separated from the language-specific constants?

5. **Granularity of sub-cat for clausal complements:** The believe-vs-persuade distinction (Type I vs Type II) is a critical force constant — same surface, different structure, disambiguated by the verb's entry. How fine-grained do we go? Every verb in the language needs its clausal sub-cat pattern cataloged.
