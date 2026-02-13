# English Force Patterns

**Purpose:** Define the physics constants that govern structural assembly and disassembly of English sentences. These are the force rules the inference engine (NAPIER) needs to interpret input structurally and construct output thoughtfully.

**Role:** This document defines the **translation layer** between NAPIER's texture engine (surface text) and mesh engine (conceptual shape). The same forces drive both directions — decomposing surface text into conceptual mesh on input, and wrapping conceptual mesh into well-formed surface text on output.

**Scope:** English-specific constants only. Other languages use different constants with the same force types. The force TYPES are universal (hcp_core); the force VALUES here are English-specific (hcp_english). See [force-pattern-db-requirements.md](force-pattern-db-requirements.md) for the core/shard split.

**Source:** Derived from *Analysing Sentences* (4th ed., Burton-Roberts). See [sentence-analysis-framework.md](sentence-analysis-framework.md) for the full textbook digest.

---

## Critical: Forces Are LoD-Relative

Forces are **not** monolithic or global. They are relative to their specific LoD level and aggregate upward.

```
WORD LEVEL:    Forces resolve individual tokens into phrase-level structures.
               Sub-cat patterns, category compatibility, word-level ordering.

PHRASE LEVEL:  Internal word-level forces collapse into aggregate shapes.
               "the tall old man" → NP[definite, human, male, modified]
               Phrase is now a rigid body. Internal detail invisible to higher levels.

CLAUSE LEVEL:  Phrase-level shapes interact via clause-level forces.
               NP[agent] + VP[action] → Subject–Predicate bonding.
               VP's internal structure (sub-cat filled, complements present) is aggregate.

SENTENCE+:     Clause-level shapes interact. Embedding, coordination, discourse.
```

At each level, lower-level detail is consumed. You don't need to know the internal binding of a noun phrase to understand its role as a subject — you only need its aggregate conceptual shape. **Force values at each LoD level are contextual, not absolute.**

For MVP, the right STRUCTURE for LoD stacking and aggregation matters more than perfect force values. Rough-approximated constants with the correct architecture will outperform precise constants in a flat structure.

---

## Force Type Inventory

The architecture defines two conceptual forces: **gravitational** (attraction/clustering) and **albedo** (reflectivity/representativeness). For language, these manifest as specific, enumerable force patterns operating at specific LoD levels:

| Force Type | What It Does | Primary LoD Level | Physics Analogy |
|---|---|---|---|
| Attraction | Pulls tokens toward each other for assembly | All levels | Gravitational force |
| Binding energy | Holds assembled tokens together; cost to disassemble | All levels | Bond energy |
| Ordering constraint | Enforces positional sequence | Level-specific | Directional force field |
| Category compatibility | Determines what CAN bond with what | All levels | Charge compatibility |
| Valency | Determines how MANY bonds a token requires/permits | WORD→PHRASE | Electron shells |
| Movement force | Pulls tokens from deep-structure to surface position | CLAUSE/SENTENCE | Electromagnetic attraction |
| Structural repair | Inserts tokens to satisfy unmet constraints | CLAUSE | Virtual particle creation |

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

## 10. The Bridge: Grammatical Forces ↔ Concept Mesh

The force patterns defined above are the **texture-side** half of the translation layer. The other half is the concept mesh — ~2,784 concept tokens in AA namespace that form the internal vocabulary of the engine:

| Layer | Address | Count | Content |
|---|---|---|---|
| NSM Primes | AA.AA.AA.AB.* | 65 | Atomic concepts: I, YOU, THINK, KNOW, SEE, DO, GOOD, BAD... |
| nsm_aa | AA.AC.AA.AA.* | 65 | First molecular layer: operational forms of primes + connectives |
| nsm_ab | AA.AC.AA.AB.* | 302 | Second molecular layer: easy, control, shape, colour, animal... |
| nsm_ac | AA.AC.AA.AC.* | 2,352 | Third molecular layer: full conceptual vocabulary |

**None of these tokens have surface expressions.** The name labels are developer convenience. These ARE the mesh — the conceptual shapes that the force patterns translate surface language to and from.

### 10.1 How the Bridge Works

Each concept token in AA.AC has an `eng_refs` metadata field listing the English surface tokens (AB namespace) that can express it. The force patterns determine which bridge to cross:

```
SURFACE (AB)                    FORCES                     MESH (AA.AC)
─────────────                   ──────                     ────────────
"She read the book"      →  grammatical analysis    →  concept assembly
                            (sub-cat, functions)       (concept tokens + roles)

"She"                    →  Subject of S (agent)    →  [person] + [specification]
"read"                   →  V_TRANS head            →  [see] + [word] frame
"the book"               →  NP complement (dO)      →  [thing] + [word]

Conceptual mesh result:  [person:agent] →DO→ [see+word:action] →_TO→ [thing+word:patient]
```

### 10.2 NSM Definition Frames = Mesh-Side Force Templates

The nsm_aa concept definitions encode structural frames using markers that map directly to grammatical functions:

**"do" (AA.AC.AA.AA.Aj):**
```
"Lisa {does} something."                               → agent + action (intransitive)
"Lisa {does} something {_to} Tony."                     → agent + action + patient
"Lisa {does} something {_with} this thing."             → agent + action + instrument
"Lisa {does} something {_to} Tony {_with} this thing."  → agent + action + patient + instrument
```

**"say" (AA.AC.AA.AA.Ad):**
```
"Tony {says} something."                                → agent + speech_act
"Tony {says} something {_to} Lisa."                     → agent + speech_act + addressee
"Tony {says} something {_about} this living thing."     → agent + speech_act + topic
```

**"know" (AA.AC.AA.AA.An):**
```
"Tony {knows} something {_about} Lisa."                 → experiencer + knowledge + topic
```

**"move" (AA.AC.AA.AA.Ay):**
```
"Lisa sees something {move}."                           → entity + motion (intransitive)
"Lisa {moves} near_to this thing."                      → agent + motion + goal
```

### 10.3 Grammatical Function → Conceptual Role Mapping

The force patterns (texture side) determine grammatical functions. Those functions map to conceptual roles (mesh side):

| Grammatical Function | Typical Conceptual Role | NSM Frame Marker |
|---|---|---|
| Subject of active S | Agent (doer) | First NP in definition: "Lisa {does}..." |
| Direct object (dO) | Patient (affected) | `{_to}` marker: "{does} something {_to} Tony" |
| Indirect object (iO) | Recipient/Beneficiary | `{_to}` (ditransitive): "gives something {_to} Lisa" |
| PP[with] complement | Instrument | `{_with}` marker: "{does} something {_with} this thing" |
| PP[about] complement | Topic | `{_about}` marker: "{says} something {_about} Lisa" |
| Subject of intensive V | Theme (described) | "{is} something" / "{seems} something" |
| Subject-predicative (sP) | Property/State | The complement of intensive V |
| Subject of passive S | Patient (affected) | Was patient in deep structure |
| Adverbial (manner) | Manner | How the action is done |
| Adverbial (place) | Location | Where the action occurs |
| Adverbial (time) | Temporal | When the action occurs |

### 10.4 Sub-categorization → Frame Selection

The verb's sub-cat pattern selects which NSM definition frame applies:

| Verb Sub-cat | NSM Frame Pattern | Conceptual Structure |
|---|---|---|
| V_INTRANS | "X {does} something." | agent→action |
| V_TRANS | "X {does} something {_to} Y." | agent→action→patient |
| V_DITRANS | "X {does} something {_to} Y {_with} Z." | agent→action→recipient+theme |
| V_INTENS | "X {is} something." | theme→property |
| V_PREP | "X {does} something {_P} Y." | agent→action→role(determined by P) |
| V_THAT | "X {knows/says} [clause]." | experiencer/agent→proposition |

The sub-cat pattern is a texture-side force constant. The NSM frame is the mesh-side template it activates. The mapping between them is the core of the translation layer.

### 10.5 The eng_refs Bridge Points

Each concept token stores `eng_refs` — an array of AB-namespace token IDs that can express that concept. Example:

```
AA.AC.AA.AA.AA ("see") → eng_refs: [AB.AB.CA.Ez.RQ, AB.AB.CA.Ez.RP, ...]
```

These references are the bridge points:
- **On input (parsing):** Surface token "see" (AB.AB.CA.Ez.RQ) → looked up in concept eng_refs → maps to concept `see` (AA.AC.AA.AA.AA)
- **On output (generation):** Concept `see` → eng_refs consulted → surface token "see" selected based on context

The force patterns control the CONTEXT of selection:
- Which frame slot is being filled? (agent, patient, instrument)
- What grammatical function does this position require? (subject, dO, PP[with])
- What ordering constraints apply? (head-initial, complement before adjunct)
- What category compatibility rules constrain the choice? (transitive V needs NP complement)

### 10.6 LoD Aggregation in the Bridge

The bridge operates at different LoD levels with different granularity:

**Word → Concept (finest grain):**
Individual surface tokens map to individual concept tokens via eng_refs. "read" → `see` + `word`.

**Phrase → Concept Assembly:**
The phrase's internal structure (determined by word-level forces) aggregates into a conceptual frame. VP "read the book" → [action:see+word, patient:thing+word]. The phrase presents to the clause level as: [directed-action-on-word-thing].

**Clause → Proposition:**
The clause's grammatical structure (subject-predicate, determined by clause-level forces) maps to a propositional concept. "She read the book" → [person:agent DO:see+word TO:thing+word]. This is a complete conceptual unit — a proposition.

**Sentence → Discourse Contribution:**
Multi-clause sentences aggregate propositions into discourse-level structures. Subordination, coordination, and discourse connectives organize propositions.

At each level, the lower-level detail collapses. The discourse level doesn't need to know that "the book" was DET+NOM with an empty modifier stack. It just needs: [entity:word-thing, definite].

---

## Resolved Design Questions

*These questions were originally open. Resolved with Project Lead's architectural framing (2026-02-12).*

1. **Numeric scale:** → Float 0.0–1.0 per force per LoD level. 1.0 = absolute constraint (violation = ungrammatical). Below 1.0 = preference (violation = higher energy). For MVP, hand-assigned ordinal values (high=0.9, moderate=0.6, low=0.3) are sufficient. Architecture matters more than exact numbers.

2. **Sub-cat storage:** → Sub-cat is a force constant dimension on the token. Pattern definitions in a lookup table in the language shard. Words map to patterns (junction table with frequency weights for multi-pattern words). See [force-pattern-db-requirements.md](force-pattern-db-requirements.md) Section 4.

3. **Absolute vs. preference:** → Yes, distinguish formally. ABSOLUTE (1.0) = inviolable, physics engine rejects parse. PREFERENCE (<1.0) = increases energy cost, doesn't invalidate. Both types exist in grammar and DB.

4. **Cross-linguistic universals:** → Force CATEGORIES (that attraction exists, that valency is a thing) → hcp_core. Force CONSTANTS (English is SVO, English auxiliary chain) → language shards. Same engine, different language file.

5. **Clausal sub-cat granularity:** → Enumerate PATTERN TYPES (~15 for verbs, ~4 for nouns, ~5 for adjectives, ~3 for prepositions). Each word classifies into one or more patterns. Patterns are the constants; word-to-pattern mapping is vocabulary. Multiple-pattern words get frequency weights.
