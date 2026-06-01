# Sentence Analysis Framework

Source: *Analysing Sentences* (4th ed., Noel Burton-Roberts)

## Overview

This document captures the core analytical framework from the textbook for mapping to HCP's structural model. The framework provides a rigorous, hierarchical decomposition of sentence structure into constituents, categories, and functions — directly relevant to HCP's bond-based structural storage.

---

## Chapter 1: Constituents

### Core Principle
Sentences have **structure** — they are not flat sequences of words. Structure means:
1. Divisible into parts (**constituents**)
2. Parts belong to different **categories**
3. Parts are arranged in a specific way
4. Each part has a **function** relative to others

### Hierarchical Organization
- Sentences consist of **phrases**, not directly of words
- Phrases consist of words (or other phrases)
- This creates a hierarchy: S → phrases → words → morphemes → characters
- **Tree diagrams** (phrase markers) visualize this hierarchy

### Key Terminology
- **Node**: any point in the tree (labeled with category)
- **Dominance**: node A dominates node B if A is higher and connected to B
- **Immediate dominance**: A immediately dominates B if there is no intervening node
- **Immediate constituents**: the parts directly inside a larger constituent
- **Sisters**: nodes immediately dominated by the same parent node

### Five Constituency Tests
These tests determine whether a string of words forms a genuine constituent:

1. **Omission test**: can the string be removed and leave a grammatical sentence?
2. **Replacement test**: can the string be replaced by a single word (pronoun, pro-form)?
3. **Question test**: can the string serve as an answer to a wh-question?
4. **Movement test**: can the string be moved to another position in the sentence?
5. **Sense test**: does the proposed grouping form a coherent unit of meaning?

No single test is definitive — converging evidence from multiple tests establishes constituency.

### Structural Ambiguity
The same linear word sequence can have different hierarchical structures, producing different meanings. Example: "old men and women" — are the women old or not? Depends on the tree structure.

### HCP Relevance
- Constituency maps directly to **LoD stacking**: words are atomic at one level, phrases are rigid bodies at the next level up
- The hierarchy is exactly what PBM bond patterns encode — not the linear sequence but the structural relationships
- Constituency tests could inform the **soft-body resolution** mechanism — ambiguous parses are soft bodies that need energy-loss minimization to resolve

---

## Chapter 2: Functions

### Core Principle
Constituents have **functions** — roles they play relative to their sister constituents. Function is always defined in terms of the relationship between sisters (nodes sharing the same parent).

### Three Functional Relationships

#### 1. Subject ~ Predicate (S level)
- **Mutual (two-way) dependency**: both required
- Subject = NP immediately dominated by S
- Predicate = VP immediately dominated by S
- Both obligatory — removing either is ungrammatical
- The sentence-level split: `S → NP + VP`

#### 2. Modifier ~ Head (within phrases)
- **One-way dependency**: head is obligatory, modifier is optional
- The **head** determines the category of the phrase (N heads NP, V heads VP, etc.)
- Modifiers add information but can be removed without destroying grammaticality
- Example: "very tall" — "tall" is head (A), "very" is modifier (Adv)
- Multiple modifiers can stack

#### 3. Head ~ Complement (within phrases)
- **Two-way dependency**: both obligatory
- Head precedes complement (in English)
- Complement is **selected by the head** — the head determines what complement it requires
- Example: "put [the book] [on the shelf]" — V "put" requires both NP and PP complements
- This is **sub-categorization**: the head's lexical entry specifies its complement requirements

### Functions Summary Table

| Relationship | Dependency | Obligatoriness | Level |
|---|---|---|---|
| Subject ~ Predicate | Mutual | Both obligatory | S |
| Modifier ~ Head | One-way (toward head) | Head obligatory, modifier optional | Phrase |
| Head ~ Complement | Mutual | Both obligatory | Phrase |

### HCP Relevance
- Functions map to **bond types** — the relationship between two tokens in a Forward Pair-Bond isn't just adjacency, it's functional
- Subject~Predicate = a specific bond pattern at sentence scope
- Modifier~Head = a bond pattern within phrase scope, with the modifier having lower structural weight
- Head~Complement = a bond pattern within phrase scope, with mutual dependency (strong bond)
- **Conceptual forces** (albedo, gravitational) could encode these functional relationships: heads have high gravitational force (they attract complements), modifiers have lower binding energy

---

## Chapter 3: Categories

### Lexical Categories (word-level)
| Category | Abbreviation | Examples | Test |
|---|---|---|---|
| Noun | N | dog, idea, London | Takes determiners, pluralizes |
| Verb | V | run, seem, put | Conjugates for tense |
| Adjective | A | tall, happy, red | Gradable (very X, more X) |
| Adverb | Adv (or DEG) | quickly, very, often | Modifies V, A, or Adv |
| Preposition | P | in, on, with, to | Takes NP complement |

### Phrasal Categories
| Category | Abbreviation | Head | Example |
|---|---|---|---|
| Noun Phrase | NP | N | "the big dog" |
| Verb Phrase | VP | V | "ran quickly home" |
| Adjective Phrase | AP | A | "very tall" |
| Adverb Phrase | AdvP | Adv | "quite slowly" |
| Prepositional Phrase | PP | P | "in the garden" |

### Key Category Principles
1. **Head determines phrase category**: N → NP, V → VP, A → AP, etc.
2. **Lexical categories have ONE function**: HEAD (of their phrase)
3. **Phrasal categories have MULTIPLE possible functions**: subject, complement, modifier, etc.
4. **Co-ordination test**: only constituents of the SAME category can be co-ordinated with "and"/"or"
5. **Pronouns** stand for full NPs (not just nouns) — "he" replaces "the tall man", not "man"
6. **Proper nouns (names)** are full NPs — "London" is NP, not just N

### Noun Sub-categories
- **Proper vs. Common**: proper nouns name specific entities, common nouns classify
- **Count vs. Mass**: count nouns take plural/numerals, mass nouns don't
- These are sub-categorization features on the lexical entry

### HCP Relevance
- Lexical categories map to **token dimensions** — part-of-speech is a fundamental dimension of every text token
- Phrasal categories are **rigid-body assemblies at the phrase LoD level**
- The head-determines-category principle is crucial for **LoD stacking**: when you zoom out from word-level to phrase-level, the phrase inherits its category from its head token
- Co-ordination as a same-category test maps to a **structural constraint** in the physics engine — only same-type rigid bodies can co-ordinate
- Proper nouns being full NPs directly supports HCP's design: names in v*/w*/x* namespaces atomize to word tokens in AB namespace; the "name" is a construct (bond pattern), not a token property

---

## Chapter 4: The Verb Phrase

### Two Kinds of Verbs
1. **Lexical verbs**: carry meaning, head the VP
2. **Auxiliary verbs**: grammatical function (tense, modality, aspect) — covered in later chapters

### Six Sub-categories of Lexical Verbs

| Sub-category | Feature | Complement(s) | Example |
|---|---|---|---|
| Transitive | [trans] | NP (direct object) | "She read **the book**" |
| Intransitive | [intrans] | ∅ (none) | "She **laughed**" |
| Ditransitive | [ditrans] | NP (iO) + NP (dO) | "She gave **him** **a book**" |
| Intensive | [intens] | AP/NP/PP (subject-predicative) | "She seemed **happy**" |
| Complex trans. | [complex] | NP (dO) + AP/NP/PP (obj-predicative) | "She made **him** **happy**" |
| Prepositional | [prep] | PP (prep. complement) | "She relied **on him**" |

### VP Functions
| Function | Abbreviation | Definition |
|---|---|---|
| Direct object | dO | NP complement of transitive/ditransitive V |
| Indirect object | iO | First NP in ditransitive (recipient/beneficiary) |
| Subject-predicative | sP | Complement of intensive V, predicates about subject |
| Object-predicative | oP | Second complement of complex transitive, predicates about dO |
| Prepositional complement | PC | PP complement of prepositional V |

### Sub-categorization Decision Procedure
To identify verb type, check:
1. How many complements does V take? (0, 1, or 2)
2. What categories are the complements? (NP, AP, PP)
3. What function do the complements serve?

### Ditransitive Alternation
Ditransitives have two forms:
- Double-object: V + NP(iO) + NP(dO) — "gave **him** **a book**"
- Prepositional: V + NP(dO) + PP(iO) — "gave **a book** **to him**"

### HCP Relevance
- Sub-categorization features are **token dimensions** on verb entries — they define what bond patterns the verb participates in
- The verb's sub-category directly determines the **PBM template** for the VP scope: transitive creates V→NP bonds, ditransitive creates V→NP→NP bonds, etc.
- Intensive and complex transitive verbs create **predicative bonds** — a special relationship where one constituent describes another (subject or object). These are structurally different from object bonds.
- The ditransitive alternation shows the same semantic content with different structural encoding — same conceptual mesh, different texture. This is exactly HCP's two-engine separation.

---

## Chapter 5: Adverbials

### The Complement/Adjunct Distinction
The fundamental structural difference within VP:
- **Complements** = sisters of V (within VP1). Obligatory, selected by the verb's sub-categorization.
- **Adjuncts** = sisters of VP (creating VP2, VP3, etc.). Optional, freely added.

This creates **layered VP structure**: VP1 is the basic verb + complements, then each adjunct adverbial wraps another VP layer around it.

### Types of Adverbials

#### VP Adverbials (Adjuncts)
- Manner: "She spoke **quietly**"
- Place: "She spoke **in the hall**"
- Time: "She spoke **yesterday**"
- These modify VP (are sisters of VP within a higher VP)
- Tested by `do so` substitution: "She spoke quietly and he did so too" — `did so` replaces VP1

#### S-Adverbials
- **Disjuncts**: speaker comment on the proposition — "**Frankly**, she's wrong"
- **Conjuncts**: discourse-linking — "**However**, she left"
- These are sisters of S within a higher S (not inside VP at all)

### Phrasal Verbs
- V + Particle (not V + PP): "look **up** the word" / "look the word **up**"
- Particle movement test: particle can move past NP object; true prepositions cannot
- "She looked **up** the chimney" (PP) vs. "She looked **up** the word" (phrasal verb)

### Ellipsis
- Omission of grammatically obligatory elements recoverable from context
- "John can swim and Mary can ∅ too" — VP ellipsis
- Shows that constituency structure persists even when material is phonologically absent

### HCP Relevance
- VP layering maps directly to **LoD stacking within a single phrase**: VP1 is the tight rigid body, each adjunct adds an outer shell at decreasing bond strength
- The complement/adjunct distinction is a **bond strength differential** — complements have strong (obligatory) bonds to the head, adjuncts have weaker (optional) bonds to the assembly
- Phrasal verbs are **multi-token rigid bodies at the lexical level** — the particle is part of the verb's identity, not a separate phrase
- Ellipsis demonstrates that PBM structure can exist without surface realization — the bond pattern is present even when tokens are silent. Critical for the mesh/texture separation.

---

## Chapter 6: The Auxiliary Verb Phrase

### Auxiliary vs. Lexical Verbs
- **Lexical verbs**: carry meaning, head the basic VP
- **Auxiliary verbs**: encode tense, aspect, modality — grammatical scaffolding

### Four Auxiliary Types (Fixed Order)
```
MOD > PERF (have) > PROG (be) > PASS (be) > Lexical V
```

| Type | Form | Following V form | Function |
|---|---|---|---|
| Modal (MOD) | can, will, shall, may, must | bare stem | Modality |
| Perfect (PERF) | have | past participle (-en) | Aspect (completion) |
| Progressive (PROG) | be | present participle (-ing) | Aspect (duration) |
| Passive (PASS) | be | past participle (-en) | Voice |

### Structural Analysis
- Each auxiliary takes a **VP complement** — creating nested VP layers
- "She **might** [have [been [being [examined]]]]" — four auxiliary layers
- Only the **first verb** carries tense ([pres] or [past]); all others are non-finite
- Each auxiliary determines the **morphological form** of the following verb

### S-bar (S') and Complementiser Position
- S' (S-bar) dominates C + S
- C position: daughter of S', sister of S
- In declarative sentences, C is typically empty
- In questions, the tensed auxiliary **fronts to C**: "Can she swim?" — `can` moves to C

### Passive Construction
- Object NP moves to subject position, leaving a **gap** (●)
- "The book was read ●" — gap marks where the object originated
- This is a structural transformation, not just a different word order

### Negation and Do-Support
- `not` attaches after the tensed auxiliary: "She has **not** left"
- When no auxiliary is present, **do-support** inserts `do` to carry tense: "She **does not** leave"
- `do` is a semantically empty auxiliary — pure tense carrier

### Lexical `be`
- Always behaves syntactically like an auxiliary (inverts for questions, takes `not` directly)
- Despite being a lexical verb (intensive: "She **is** happy")

### HCP Relevance
- Auxiliary ordering is a **rigid constraint** — it's not probabilistic, it's structural law. This maps to a physics engine constraint on assembly order, like molecular bond angles.
- The nesting (each aux takes VP complement) creates **LoD shells**: the lexical VP is the core, each auxiliary wraps a structural layer encoding temporal, aspectual, or voice information
- Tense as a feature on V ([pres]/[past]) maps to a **token dimension** — but it's a dimension of the *first verb in the chain*, not each verb independently
- Passive gaps (●) are critical: they're **phantom bonds** — structural positions where a token was but moved. The PBM needs to encode both the surface position and the deep-structure origin.
- Do-support reveals that English requires a tensed element — when meaning doesn't supply one, structure inserts a dummy. This is a **structural repair mechanism** analogous to energy-loss minimization.

---

## Chapter 7: The Noun Phrase

### NP Internal Structure
```
NP → DET + NOM
```

NOM (Nominal) is the key intermediate level between NP and N.

### Determiner Types

| Type | Abbreviation | Examples |
|---|---|---|
| Article | ART | the, a |
| Demonstrative | DEM | this, that, these, those |
| Quantifier | Q | some, any, no, each, every |
| Possessive | POSS | my, John's, the dog's |
| Pre-determiner | PRE-DET | all, both, half |

### NOM: The Recursive Core
NOM is recursive — modifiers create NOM-within-NOM layers:
- "the [very tall] [old] man" → NP[DET + NOM[AP + NOM[AP + NOM[N]]]]
- Each adjective phrase (AP) is a sister of NOM within a higher NOM

### Pre-modification (within NOM, before N)
- **APs**: "the **tall** man"
- **Quantifying Adjectives (QA)**: much, many, few, little, numerals — "**three** tall men"
- **Participle Phrases (PartP)**: "the **running** man", "the **broken** vase"
- **Compound Nouns (N+N)**: "**garden** chair" — N modifying N within NOM

### Post-modification (within NOM, after N)
- **PPs**: "the man **in the hat**"
- **APs with complements**: "a man **fond of cheese**" (AP post-poses when it has a complement)

### The Friendly Head Principle
The head of a modifying phrase wants to be adjacent to the head noun it modifies. This explains:
- Why APs pre-modify (head A is closest to N): "the **tall** man"
- Why APs with complements post-modify (putting AP before N would separate A from N with the complement): "a man **fond of cheese**" not "*a fond of cheese man"

### Empty Determiners
- Plural count nouns: "∅ Dogs bark" (DET = empty)
- Mass nouns: "∅ Water flows" (DET = empty)
- The DET position exists structurally even when phonologically empty

### Complement vs. Adjunct within NOM
- Sister of N = **complement**: "student **of physics**" (selected by N)
- Sister of NOM = **adjunct**: "student **in the library**" (freely added modifier)
- Parallel to VP: sister of V = complement, sister of VP = adjunct

### pro-NOM `one`
- `one` replaces NOM (not N, not NP): "the tall **one**" replaces NOM "tall man"
- Proves the NOM constituent exists as a real structural level

### HCP Relevance
- NOM as a recursive intermediate level maps to **sub-assembly rigid bodies** — each modifier wraps another structural layer, but the whole NOM is a single unit at the phrase LoD
- DET + NOM structure means determiners are **scope markers**, not modifiers — they define the referential scope (definite, indefinite, quantified) of the NOM assembly. This is a dimension on the NP, not a bond to the head N.
- Empty determiners prove structural positions exist without surface tokens — same principle as passive gaps. The PBM encodes the structural slot even when no token fills it.
- The Friendly Head Principle is a **proximity constraint** in the physics engine — bond partners prefer adjacency. When a modifier gets too complex (has its own complements), it must relocate to maintain head-proximity. This is literally energy minimization.
- N+N compounds (garden chair) are **lexicalized rigid bodies** — two N tokens fused into a single NOM-level assembly

---

## Chapter 8: Sentences Within Sentences

### Subordinate Clauses
Complex sentences contain embedded sentences (subordinate clauses). These are full S structures functioning as constituents within a larger S.

### S-bar (S') Structure
```
S' → C + S
```
- C = Complementiser position (daughter of S', sister of S)
- **that** (declarative complementiser): "I know **that** she left"
- **whether** (interrogative complementiser): "I wonder **whether** she left"

### Functions of Subordinate Clauses

| Function | Example |
|---|---|
| Subject | "**That she left** surprised me" |
| Extraposed subject | "It surprised me **that she left**" (with expletive `it`) |
| Complement of V | "I know **that she left**" |
| Complement of A | "I'm glad **that she left**" |
| Complement of N | "the fact **that she left**" |
| Complement of P | "despite **that she left**" |
| Adverbial | "**Although she left**, I stayed" |

### Subordinating Conjunctions
- although, unless, if, because, while, etc.
- Occupy C1 position (same as `that`/`whether`)

### Temporal Prepositions Taking S Complements
- **after, before, until, since** are prepositions (not complementisers) that can take S as complement
- "I left **after** [she arrived]" — PP with S complement
- Evidence: they also take NP complements ("after the party"), which true complementisers cannot

### Abbreviated Clausal Analysis (ACA)
- Notation for labeling clause boundaries and functions without drawing full trees
- Practical for complex multi-clause sentences

### HCP Relevance
- Subordinate clauses are **recursive PBM nesting** — a PBM within a PBM. The embedded S has its own complete bond structure, and that entire structure functions as a single constituent in the parent S.
- Complementisers are **structural junction tokens** — they mark the boundary between structural levels. In physics terms, they're the joint connecting two rigid-body assemblies.
- Extraposition (moving a clause and leaving expletive `it`) is another case of **structural movement with a placeholder** — like passive gaps but at clause level. The PBM must track both the surface position (`it`) and the structural origin.
- The fact that `after/before/until/since` are prepositions taking S complements (not complementisers) shows that **category identity is independent of what categories a word selects as complement**. A preposition is always P, even when its complement is S rather than NP. This supports strict category-based dimensions on tokens.

---

## Chapter 9: Wh-Clauses

### S-double-bar (S'')
A third level of sentence structure, needed for wh-movement:
```
S'' → C2 + S'
S'  → C1 + S
S   → NP + VP
```

### Two C Positions
- **C1** (daughter of S', sister of S): complementisers (`that`, `whether`) and fronted auxiliaries
- **C2** (daughter of S'', sister of S'): fronted wh-phrases

### Wh-Questions
Involve two movements:
1. Wh-phrase fronts to **C2**
2. Tensed auxiliary fronts to **C1**
- "**What** **did** she buy ●?" — `what` in C2, `did` in C1, gap ● in object position

### Subordinate Wh-Interrogatives
Only wh-fronting (no auxiliary inversion):
- "I wonder **what** she bought ●" — `what` in C2, C1 empty, gap ●

### Relative Clauses
Non-interrogative wh-clauses that modify nouns:

#### Three Types of Clauses within NP
| Type | Position | Function | Example |
|---|---|---|---|
| Noun-complement clause | Sister of N (within NOM) | Complement | "the claim **that she left**" |
| Restrictive relative | Sister of NOM (within higher NOM) | Adjunct | "the man **who left**" |
| Non-restrictive relative | Sister of NP (within higher NP) | Peripheral modifier | "John, **who left**, was happy" |

#### Key Distinctions
- Noun-complement: `that` is a complementiser in C1 (cannot be replaced by wh-word)
- Restrictive relative: `who/which/that` in C2 (can be replaced by each other)
- Non-restrictive: always `who/which` in C2 (never `that`), set off by commas

### `that` in Relatives
- Treated as an alternative wh-form occupying C2 (not the complementiser `that` in C1)
- Evidence: in relatives, `that` alternates with `who/which`; in noun-complement clauses, it doesn't

### HCP Relevance
- S'' introduces a **third structural level** for sentence nodes — the PBM hierarchy for a wh-question has three layers before you reach the NP+VP core. This is deeper nesting than simple declaratives.
- Wh-movement creates **long-distance bonds** — the wh-phrase in C2 is semantically connected to its gap position deep within the clause. The PBM must encode this non-local relationship (the phrase is pronounced in one place but interpreted in another).
- The three types of clauses within NP (complement, restrictive, non-restrictive) occupy different structural levels. This maps to **different bond attachment points** on the NP rigid body — complement bonds to N, restrictive bonds to NOM, non-restrictive bonds to NP. Three distinct bond types within one phrase.
- Restrictive relatives constrain reference (adjunct = energy-reducing modifier), non-restrictive relatives add parenthetical information (peripheral = weak outer-shell bond).

---

## Chapter 10: Non-Finite Clauses

### Core Properties
- No tensed verb; always subordinate (cannot stand alone)
- Subject NP is typically **covert** (unpronounced)

### Covert NPs
- **Free** (PRO_arb): general/indefinite reference — "**∅** To err is human"
- **Controlled** (PRO): co-indexed with an NP in the superordinate clause — "She_i wants **∅_i** to leave"

### Four Non-Finite Verb Types

| Type | Form | Example |
|---|---|---|
| Ia: Bare infinitive | stem | "She saw him **leave**" |
| Ib: To-infinitive | to + stem | "She wants **to leave**" |
| IIa: Passive participle | -en/-ed | "the book **written** by her" |
| IIb: -ing participle | -ing | "She enjoys **swimming**" |

### `to` as Auxiliary
- `to` is analyzed as a [-tense] auxiliary, not a preposition
- It takes VP complement, parallel to modal auxiliaries
- Carries the [-tense] feature that makes the clause non-finite

### Functions of Non-Finite Clauses
- Subject: "**To leave** would be rude"
- Extraposed subject: "It would be rude **to leave**"
- Complement of V: "She wants **to leave**"
- Complement of A: "She is eager **to please**"
- Complement of N: "a desire **to leave**"
- Complement of P: "without **leaving**"
- Adverbial: "**Leaving early**, she missed the show"
- Modifier in NP: "the man **to see**" (non-finite relative)

### Verb Sub-categorization for Clausal Complements
Highly intricate — verbs differ in which non-finite clause types they accept:
- **Type I** (believe-type): "I believe **him to be guilty**" — NP between clauses is the lower subject
- **Type II** (persuade-type): "I persuaded **him to leave**" — NP between clauses is the upper object AND lower subject

### Control Patterns for Adjective Complements
- **Type A** (reluctant-type): subject controls lower subject — "She_i is reluctant **∅_i to leave**"
- **Type B** (impossible-type): subject controls lower object — "She_i is impossible **∅ to please ∅_i**"

### HCP Relevance
- Covert NPs are **implicit bond references** — the PBM must encode a bond partner that has no surface token. The controlled PRO is a **co-reference bond** linking the covert position to an overt NP elsewhere in the structure.
- The [-tense] feature on `to` makes it a **structural marker** like complementisers — it exists purely to carry a grammatical feature, not meaning. These are structural tokens (AA namespace candidates) rather than content tokens.
- Verb sub-categorization for clausal complements means verb tokens need **complex bond templates** — not just "takes NP" or "takes PP" but "takes NP + to-infinitive VP where the NP is the lower subject." The verb's dimensions must encode these multi-clause assembly patterns.
- The Type I/Type II distinction (believe vs. persuade) shows that the **same surface pattern** (V + NP + to-VP) can have **different structural analyses**. This is exactly structural ambiguity requiring soft-body resolution — the physics engine must determine the correct bond configuration.
- Non-finite relatives ("the man to see") are **reduced relative clauses** — full clausal structure compressed into a modifier. The PBM stores the full structural decomposition even though the surface form is minimal.

---

## Chapter 11: Languages, Sentences, and Grammars

### Core Theoretical Framework
- A **language** = an infinite set of sentences
- Natural languages are infinite via recursion: co-ordination ("X and X and X..."), relative clauses ("the man who saw the man who saw..."), clause embedding
- A **grammar** = a finite description (set of rules) that generates an infinite language
- **Generative grammar**: rules simultaneously *admit* sentences as grammatical and *assign structural descriptions* to them

### Chomsky's Key Insight
- Language is an internal mental capacity (**competence**), not just observable behavior (performance)
- The grammar is a model of what speakers *know* about their language
- Phrase structure rules, transformations, and structural descriptions model this knowledge

### Rules vs. Descriptions
- **Phrase markers** (tree diagrams) describe individual sentences
- **Rules** (grammar) define the entire language — what is and isn't possible
- A phrase marker is a single output of the rule system; the grammar is the system itself

### HCP Relevance
- This chapter validates HCP's approach: the physics engine IS the grammar — it's a finite set of rules (forces, constraints, assembly patterns) that generates an infinite space of valid structures
- The competence/performance distinction maps to **mesh vs. texture** — competence is the cognitive engine (what structures are possible), performance is the texture engine (what actually gets expressed in a given modality)
- Recursion as the source of infinity is exactly **LoD stacking with self-similarity** — the same structural patterns repeat at nested levels, creating unbounded depth from finite rules
- The insight that rules both *admit* and *describe* structure parallels HCP's dual use of PBMs: they both validate incoming expressions (is this grammatical?) and describe stored expressions (what is the structure of this?)

---

## Complete HCP Mapping

### Structural Concepts → HCP Equivalents

| Textbook Concept | HCP Equivalent | Notes |
|---|---|---|
| Constituent | Token or rigid-body assembly | Depends on LoD level |
| Tree diagram | Bond pattern / PBM | Hierarchical decomposition stored as pair-bonds |
| Phrase category | LoD-level category tag | NP, VP, PP etc. as token dimensions at phrase level |
| Lexical category | Token dimension (PoS) | N, V, A, P, Adv as dimensions on word tokens |
| Head | High-gravitational-force token | Determines phrase category, attracts complements |
| Modifier | Low-binding-energy token | Optional, weaker bond to head |
| Complement | Strong-bond token | Obligatory, selected by head's sub-cat features |
| Subject~Predicate | Sentence-scope bond pattern | NP+VP at S level |
| Sub-categorization | Token dimensions on V | Defines complement requirements = bond templates |
| Constituency test | Soft-body resolution input | Multiple tests = energy-loss minimization |
| Structural ambiguity | Multiple parse states | Soft-body candidates, resolved by energy minimization |
| Co-ordination | Same-category constraint | Physics engine constraint on rigid-body joining |

### Layered Structure → LoD Stacking

| Textbook Concept | HCP Equivalent | Notes |
|---|---|---|
| VP1 (V + complements) | Core rigid body | Tight inner assembly, strong bonds |
| VP2+ (VP + adjuncts) | LoD shell layers | Each adjunct wraps an outer structural layer |
| Complement vs. adjunct | Bond strength differential | Complements = strong/obligatory, adjuncts = weak/optional |
| Auxiliary VP nesting | Temporal/modal LoD shells | MOD>PERF>PROG>PASS>V = ordered structural layers |
| NOM recursion | Sub-assembly rigid bodies | Each modifier wraps NOM layer; whole NOM = phrase-level unit |
| S / S' / S'' | Three-level sentence hierarchy | S = core, S' = complementiser layer, S'' = wh-movement layer |
| Subordinate clauses | Recursive PBM nesting | PBM-within-PBM, inner PBM functions as single constituent |

### Movement & Gaps → Bond Tracking

| Textbook Concept | HCP Equivalent | Notes |
|---|---|---|
| Passive gap (●) | Phantom bond | Structural position where token was; PBM encodes origin |
| Wh-movement | Long-distance bond | Wh-phrase in C2 linked to gap deep in clause |
| Extraposition | Clause-level movement + placeholder | Expletive `it` marks surface position; PBM tracks origin |
| Controlled PRO | Co-reference bond | Covert NP co-indexed with overt NP in superordinate clause |
| Free PRO | Implicit bond reference | Bond partner exists structurally but has no surface token |

### Grammatical Features → Token Dimensions

| Textbook Concept | HCP Equivalent | Notes |
|---|---|---|
| Tense ([pres]/[past]) | Dimension on first V in chain | Only one verb per clause carries tense |
| [-tense] on `to` | Structural marker dimension | AA namespace candidate — pure grammatical function |
| Verb sub-cat features | Complex bond templates | [trans], [intrans], [ditrans], [intens], [complex], [prep] |
| Clausal sub-cat | Multi-clause bond templates | Verb specifies which clause types it accepts |
| DET type | Scope-marking dimension on NP | ART/DEM/Q/POSS define referential scope, not modification |
| Count/mass distinction | Noun sub-category dimension | Affects DET selection and pluralization |
| Proper/common distinction | Noun sub-category dimension | Proper nouns = full NPs (name construct in HCP) |

### Functional Tokens → AA Namespace Candidates

| Token | Role | Notes |
|---|---|---|
| Complementisers (that, whether) | Structural junction markers | Mark clause boundaries, carry clause type |
| `to` (infinitival) | Non-finite clause marker | [-tense] auxiliary, not a preposition |
| `do` (support) | Tense carrier | Semantically empty, exists for structural requirements |
| Empty DET (∅) | Null scope marker | Structural position exists without surface realization |
| Expletive `it` | Placeholder for extraposed clause | Points to real clause in non-canonical position |

### Physics Engine Parallels

| Textbook Concept | Physics Equivalent | Notes |
|---|---|---|
| Friendly Head Principle | Proximity constraint / energy minimization | Bond partners prefer adjacency; complex modifiers relocate |
| Do-support | Structural repair mechanism | System inserts dummy element to satisfy constraint |
| Ellipsis | Structure without surface realization | Bond pattern present, tokens silent — mesh/texture separation |
| Recursion (infinite language from finite rules) | Self-similar LoD stacking | Same patterns nest indefinitely = finite rules, infinite output |
| Generative grammar | Physics engine constraint system | Finite rules generating infinite valid configurations |
| Competence vs. performance | Mesh engine vs. texture engine | What's structurally possible vs. what gets expressed |
| Phrasal verbs | Multi-token lexical rigid body | Particle is part of verb identity, not a separate phrase |
| Ditransitive alternation | Same mesh, different texture | Same semantic bonds, different surface arrangement |

---

## Design Implications for Sentence Analysis Pipeline

### Pipeline Stages

**Stage 1: Tokenization** (byte codes → word tokens)
- Input text → character-level decomposition (AA byte codes)
- Character assembly into word tokens (AB namespace lookup)
- Unrecognized sequences → soft-body candidates for spelling correction / new token creation
- Output: ordered list of word-level tokens with initial PoS dimension candidates

**Stage 2: Phrase Assembly** (word tokens → phrase-level rigid bodies)
- Apply LoD stacking: group words into phrases (NP, VP, AP, AdvP, PP)
- Head identification: each phrase's category determined by its head token
- Complement attachment: verb sub-cat features dictate required complements
- Modifier attachment: optional modifiers create outer LoD shells
- DET + NOM assembly for NPs; NOM recursion for multiple modifiers
- Output: phrase-level rigid bodies with internal bond structure

**Stage 3: Clause Assembly** (phrases → clause-level structure)
- Subject~Predicate bonding: NP + VP at S level
- Auxiliary VP layering: identify MOD>PERF>PROG>PASS>V ordering
- Passive detection: identify moved object, mark gap
- Negation and do-support: identify structural repair tokens
- Output: S-level structures with functional labels

**Stage 4: Complex Sentence Assembly** (clauses → multi-clause structure)
- Subordinate clause embedding: identify complementisers, attach S' to parent
- Wh-movement tracking: identify fronted wh-phrases, link to gaps
- Relative clause attachment: determine type (complement/restrictive/non-restrictive) and attachment level (N/NOM/NP)
- Non-finite clause integration: identify control patterns, co-reference bonds
- Extraposition tracking: link expletive `it` to displaced clause
- Output: complete hierarchical sentence structure

**Stage 5: PBM Generation** (structure → stored bonds)
- Walk the complete tree; generate Forward Pair-Bonds (FPBs) for each structural adjacency
- Encode bond type (complement, adjunct, subject~predicate, movement trace, co-reference)
- Encode bond strength (obligatory bonds stronger than optional)
- Handle gaps: phantom bonds for moved elements
- Handle covert NPs: implicit bonds for controlled PRO
- Calculate FBR (recurrence) within scope
- Output: complete Pair-Bond Map for the sentence

### Open Design Questions

1. **Bond type encoding**: How do we differentiate complement bonds from adjunct bonds from movement-trace bonds in the FPB format? The current `TokenID(0).TokenID(1).FBR` format doesn't have a field for bond type.

2. **Structural tokens**: Should complementisers, infinitival `to`, do-support `do`, and expletive `it` be in AA namespace (structural/computational) or AB namespace (English text)? They're English words but serve purely structural functions.

3. **Gap representation**: How to encode passive gaps and wh-movement traces? Options:
   - Dedicated gap token in AA namespace
   - Bond metadata linking surface position to deep-structure origin
   - Dual-position encoding (token appears in both positions with cross-reference)

4. **Ambiguity resolution**: When structural ambiguity exists (multiple valid parses), at what pipeline stage does soft-body resolution occur? Early (phrase assembly) or late (after full parse attempts)?

5. **NOM vs. N distinction**: Do we store NOM as a distinct LoD level, or collapse it? The textbook treats NOM as real structure (pro-NOM `one` proves it). But it adds a level between N and NP that most NLP systems skip.

6. **Auxiliary ordering constraint**: The MOD>PERF>PROG>PASS>V ordering is absolute in English. Is this encoded as:
   - A constraint in the physics engine (rejecting invalid orderings)?
   - A dimension on each auxiliary type (position in chain)?
   - An assembly rule specific to the English texture engine?
