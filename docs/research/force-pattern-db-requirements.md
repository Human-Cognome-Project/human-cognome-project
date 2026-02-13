# Force Pattern DB Requirements

**From:** hcp_ling (Linguistics Specialist)
**For:** hcp_db (DB Specialist) and Project Lead
**Date:** 2026-02-12
**Context:** Defines what force pattern data needs to be stored, where it lives, and what structure it needs. Based on complete analysis of *Analysing Sentences* (Burton-Roberts, 4th ed.) and Project Lead's architectural framing.

**Reference documents:**
- [english-force-patterns.md](english-force-patterns.md) — Full force type inventory and English-specific rules
- [sentence-analysis-framework.md](sentence-analysis-framework.md) — Complete textbook digest (11 chapters)
- [architecture.md](../spec/architecture.md) — Two-engine model, conceptual forces
- [napier-inference-engine.md](../spec/napier-inference-engine.md) — Inference engine spec

---

## The Big Picture

The linguistics force patterns are the **translation layer** between NAPIER's two engines:

```
TEXTURE (surface text)  ←→  FORCE PATTERNS  ←→  MESH (conceptual shape)
```

On **input** (interpretation): surface text is decomposed through force rules into a conceptual mesh. "She read the book" → [agent] [action:directed] [entity:affected].

On **output** (construction): a conceptual mesh is wrapped into well-formed surface text through the same force rules. [agent] [action:directed] [entity:affected] → "She read the book" (English) or "Elle a lu le livre" (French).

The same force definitions drive both directions. The physics is symmetric.

---

## 1. Universal Force Categories → hcp_core

These are the force TYPES — the fact that these forces exist, their general behavior, and the rules for how they interact. They belong in hcp_core because they apply to every language.

Think of these as the engine's force type registry.

### Force Type Definitions

| Force Type | Universal Behavior | What Varies by Language |
|---|---|---|
| **Attraction** | Heads attract complements more strongly than adjuncts. Structural relationships have attraction strength. | Which categories attract which; strength gradients |
| **Binding Energy** | Complements bind more tightly than adjuncts. Obligatory elements have high binding energy. | Specific binding values per relationship type |
| **Ordering** | Every language has positional constraints on structural elements. | The actual ordering rules (SVO, SOV, VSO, etc.) |
| **Category Compatibility** | Coordination requires same category (universal). Heads select complement categories. | Which specific categories exist; selection patterns |
| **Valency** | Lexical items have complement slot counts. Slots are typed and obligatory. | Number and types of slots per lexical item |
| **Movement** | Surface position can differ from structural position. Traces link moved elements to origin. | Which movements exist; triggers and targets |
| **Structural Repair** | When constraints aren't met, the system can insert structural elements. | What gets inserted and when |

### What hcp_core Needs to Store

**Force type registry table** — a small, stable enumeration:

```
force_type_id  | label              | description
---------------|--------------------|------------------------------------------
ATTR           | Attraction         | Pull between structurally related tokens
BIND           | Binding Energy     | Cost to disassemble a structural bond
ORD            | Ordering           | Positional constraint on bonded tokens
COMPAT         | Compatibility      | Category constraint on what can bond
VAL            | Valency            | Bond slot count and types on a token
MOV            | Movement           | Surface vs. deep position displacement
REPAIR         | Structural Repair  | Constraint-satisfaction token insertion
```

**Relationship type registry** — the universal structural relationships:

```
rel_type_id    | label              | force_profile
---------------|--------------------|-----------------------------------------
HEAD_COMP      | Head–Complement    | ATTR:strong, BIND:high, ORD:language-specific
HEAD_ADJ       | Head–Adjunct       | ATTR:weak, BIND:low, ORD:language-specific
SUBJ_PRED      | Subject–Predicate  | ATTR:strong, BIND:high, ORD:language-specific
DET_NOM        | Determiner–Nominal | ATTR:strong, BIND:varies, ORD:language-specific
COORD          | Coordination       | COMPAT:same-category(universal), BIND:moderate
MOV_TRACE      | Movement Trace     | MOV:links surface to deep position
COREF          | Co-reference       | Links covert element to controller
```

**LoD level registry** — the structural levels at which forces operate:

```
lod_level_id   | label              | aggregates_from    | resolves_to
---------------|--------------------|--------------------|-------------------
BYTE           | Byte code          | —                  | CHARACTER
CHAR           | Character          | BYTE               | MORPHEME
MORPH          | Morpheme           | CHAR               | WORD
WORD           | Word               | MORPH              | PHRASE
PHRASE         | Phrase             | WORD               | CLAUSE
CLAUSE         | Clause             | PHRASE              | SENTENCE
SENT           | Sentence           | CLAUSE              | DISCOURSE
DISC           | Discourse          | SENT                | —
```

These registries are small (< 20 rows each), stable, and universal. They define the engine's vocabulary for force operations.

---

## 2. Language-Specific Constants → hcp_english

These are the actual force VALUES and RULES for English. A different language shard (hcp_french, hcp_mandarin) would have different constants but reference the same universal types.

### 2a. Structural Category Set

English has specific lexical and phrasal categories. These need to be enumerable as token dimensions.

**Lexical categories (word-level):**

| cat_id | label | description |
|---|---|---|
| N | Noun | Takes determiners, pluralizes |
| V | Verb | Conjugates for tense |
| A | Adjective | Gradable (very X, more X) |
| Adv | Adverb | Modifies V, A, or Adv |
| P | Preposition | Takes NP complement |
| DET | Determiner | ART/DEM/Q/POSS |
| C | Complementiser | that, whether, wh-words in C position |
| AUX | Auxiliary | MOD/PERF/PROG/PASS |

**Phrasal categories (phrase-level):**

| cat_id | label | head_cat | description |
|---|---|---|---|
| NP | Noun Phrase | N | DET + NOM structure |
| VP | Verb Phrase | V | V + complements + adjuncts |
| AP | Adjective Phrase | A | A + modifiers/complements |
| AdvP | Adverb Phrase | Adv | Adv + modifiers |
| PP | Prepositional Phrase | P | P + NP complement |
| NOM | Nominal | N | N + modifiers (sub-NP level) |

**Clause categories (clause-level):**

| cat_id | label | structure | description |
|---|---|---|---|
| S | Sentence | NP + VP | Core clause |
| S' | S-bar | C1 + S | Clause with complementiser layer |
| S'' | S-double-bar | C2 + S' | Clause with wh-movement layer |

### 2b. Ordering Constants

These are the English-specific positional rules. Each is a constraint with a strength value.

**Proposed encoding:**

```
rule_id        | lod_level | constraint_type | rule_definition                        | strength
---------------|-----------|-----------------|----------------------------------------|---------
EN_O1          | PHRASE    | ABSOLUTE        | AUX order: MOD>PERF>PROG>PASS>V       | 1.0
EN_O2          | PHRASE    | PREFERENCE      | NP pre-mod: PRE-DET>DET>QA>AP>N(c)>N  | 0.9
EN_O3          | PHRASE    | PREFERENCE      | NP post-mod: N>PP>RelClause            | 0.85
EN_O4          | CLAUSE    | ABSOLUTE        | Clause: C2>C1>S                        | 1.0
EN_O5          | PHRASE    | PREFERENCE      | VP: V>Complement>Adjunct               | 0.9
EN_O6          | CLAUSE    | ABSOLUTE        | Tense on first V only                  | 1.0
EN_HD          | PHRASE    | ABSOLUTE        | Head-initial (V before complement)     | 1.0
EN_SV          | CLAUSE    | ABSOLUTE        | Subject before predicate               | 1.0
```

**Strength values:**
- `1.0` = ABSOLUTE constraint. Violation = ungrammatical. No exceptions.
- `0.85-0.99` = STRONG PREFERENCE. Default position, but can be overridden by specific movement rules or stylistic choice.
- `0.5-0.84` = WEAK PREFERENCE. Multiple orderings acceptable; this is the default.

For MVP, the distinction between ABSOLUTE (1.0) and PREFERENCE (<1.0) is what matters most. The exact preference values can be calibrated later.

### 2c. Sub-categorization Patterns

This is the most complex and most critical constant set. Sub-categorization defines the **bond template** for each lexical item — what it attracts, how many, and what types.

**Key insight for DB design:** Sub-cat patterns are not unique per word. There are a relatively small number of PATTERN TYPES, and each word is classified into one (or sometimes more than one) pattern. The patterns are the constants; the word-to-pattern mapping is the vocabulary.

#### Verb Sub-cat Patterns

| pattern_id | label | slot_count | slot_1 | slot_2 | notes |
|---|---|---|---|---|---|
| V_INTRANS | Intransitive | 0 | — | — | laugh, arrive, exist |
| V_TRANS | Transitive | 1 | NP:dO | — | read, hit, see |
| V_DITRANS | Ditransitive | 2 | NP:iO | NP:dO | give, send, tell |
| V_INTENS | Intensive | 1 | AP/NP/PP:sP | — | seem, appear, become |
| V_COMPLEX | Complex transitive | 2 | NP:dO | AP/NP/PP:oP | make, consider, elect |
| V_PREP | Prepositional | 1 | PP[specific_P]:PC | — | rely-on, depend-on |
| V_TRANS_INF_I | Trans + to-inf (Type I) | 1 | NP+to-VP:lower_subj | — | believe, expect |
| V_TRANS_INF_II | Trans + to-inf (Type II) | 2 | NP:dO | to-VP:comp | persuade, tell |
| V_INF | To-infinitive comp | 1 | to-VP:comp | — | want, try, hope |
| V_ING | -ing complement | 1 | -ing-VP:comp | — | enjoy, avoid, finish |
| V_BARE | Bare infinitive comp | 1 | NP+bare-VP | — | see, hear, let |
| V_THAT | That-clause comp | 1 | S':comp | — | know, believe, say |
| V_WH | Wh-clause comp | 1 | S'':comp | — | wonder, ask |
| V_DITRANS_ALT | Ditrans with PP alt | 2 | NP:dO | PP[to]:iO | give (alt form) |
| V_PHRASAL | Phrasal verb | 1 | particle + NP:dO | — | look-up, put-off |

This gives ~15 core verb patterns. Individual verbs can participate in multiple patterns (e.g., "give" is both V_DITRANS and V_DITRANS_ALT). Some verbs participate in many patterns ("know" can be V_TRANS, V_THAT, V_WH).

#### Noun Sub-cat Patterns

| pattern_id | label | complement | notes |
|---|---|---|---|
| N_BARE | No complement | — | Most nouns: dog, idea, house |
| N_PP | PP complement | PP[of/about/for]:comp | student-of, destruction-of |
| N_THAT | That-clause complement | S':comp | claim-that, fact-that |
| N_INF | To-infinitive complement | to-VP:comp | desire-to, attempt-to |

#### Adjective Sub-cat Patterns

| pattern_id | label | complement | control_type | notes |
|---|---|---|---|---|
| A_BARE | No complement | — | — | tall, red, big |
| A_PP | PP complement | PP[of/at/about]:comp | — | fond-of, angry-at |
| A_THAT | That-clause comp | S':comp | — | aware-that, glad-that |
| A_INF_A | To-inf (Type A) | to-VP:comp | subj→lower_subj | reluctant, eager |
| A_INF_B | To-inf (Type B) | to-VP:comp | subj→lower_obj | impossible, easy |

#### Preposition Sub-cat Patterns

| pattern_id | label | complement | notes |
|---|---|---|---|
| P_NP | NP complement | NP | Most prepositions: in, on, with |
| P_S | S complement | S | Temporal: after, before, until, since |
| P_ING | -ing clause comp | -ing-VP | without, by |

### 2d. Noun Feature Constants

| feature | values | affects |
|---|---|---|
| count_mass | count / mass | DET selection, pluralization |
| proper_common | proper / common | DET (proper = ∅), NP status |

### 2e. Movement Rule Constants

| rule_id | trigger | lod_level | what_moves | from | to | trace | strength |
|---|---|---|---|---|---|---|---|
| EN_M1 | PASS aux present | CLAUSE | dO (NP) | VP-internal | Subject of S | gap | 1.0 |
| EN_M2 | wh-marked constituent | CLAUSE | wh-phrase | any S-internal | C2 of S'' | gap | 1.0 |
| EN_M3 | direct question | CLAUSE | tensed AUX | pre-VP | C1 of S' | empty | 1.0 |
| EN_M4 | clausal subject (heavy) | SENTENCE | clause | subject | right-peripheral | expletive 'it' | 0.8 |
| EN_M5 | phrasal V + NP obj | PHRASE | particle | post-V | post-NP | — | 0.7 |

### 2f. Repair Rule Constants

| rule_id | trigger | lod_level | action | inserted_element |
|---|---|---|---|---|
| EN_R1 | negation/question, no AUX | CLAUSE | insert dummy tense carrier | 'do' |
| EN_R2 | extraposition leaves empty subject | CLAUSE | insert placeholder | 'it' (expletive) |
| EN_R3 | mass/plural N needs DET slot | PHRASE | null fill | ∅ (empty DET) |
| EN_R4 | non-finite clause needs subject | CLAUSE | covert NP | PRO (controlled or free) |

---

## 3. LoD Aggregation and Data Structure Implications

This is the most architecturally significant point. Forces are **not global** — they are relative to their LoD level and aggregate upward.

### How Aggregation Works

```
WORD LEVEL:    [the] [tall] [old] [man]
               DET   AP     AP    N
               Forces: DET→NOM(strong), AP→NOM(moderate), AP→NOM(moderate), N=head

PHRASE LEVEL:  [the tall old man]  =  NP
               Internal forces collapsed.
               Aggregate shape: NP, definite, human, male, modified(tall+old)
               The phrase is now a rigid body.

CLAUSE LEVEL:  [The tall old man] [read the book]  =  S
               NP = subject,  VP = predicate
               Force: SUBJ↔PRED (strong mutual attraction)
               The NP's internal structure is invisible here.
               Only its aggregate shape matters: [agent, definite, human]
```

### What This Means for Data Structure

**The inference engine needs to be able to:**

1. **Look up word-level forces** — sub-cat patterns, category, valency — when processing at word/phrase boundary
2. **Compute aggregate shapes** at each LoD level — the result of resolving all forces at one level becomes the input to the next level
3. **Look up LoD-appropriate forces** — clause-level processing shouldn't need to query word-level sub-cat; it should see the already-resolved phrase-level shapes

**Implication for the DB:** Force constants are queried at specific LoD levels. The sub-cat pattern for "read" (V_TRANS, takes NP) is a word-level constant. Once the VP "read the book" is assembled, clause-level processing sees it as [VP: action-directed-at-entity], not as individual force constants.

**This means force constant storage is LoD-tagged.** Every constant has a level at which it operates:
- Sub-cat patterns → WORD level (queried during phrase assembly)
- Ordering rules → level-specific (auxiliary ordering at PHRASE level, subject-predicate order at CLAUSE level)
- Movement rules → CLAUSE/SENTENCE level
- Category compatibility → all levels (but the categories themselves change by level)

### Suggested Approach

Force constants don't need their own separate tables — they are **dimensions on tokens** and **rule tables in the language shard**. Specifically:

**On tokens (dimensions/columns):**
- `sub_cat_pattern` — foreign key to the sub-cat pattern table (e.g., V_TRANS)
- `count_mass` — for nouns
- `proper_common` — for nouns
- `aux_type` — for auxiliaries (MOD/PERF/PROG/PASS)
- These are looked up when the token is encountered during processing

**In rule tables (per language shard):**
- Ordering rule table
- Movement rule table
- Repair rule table
- Sub-cat pattern definitions (the pattern table itself — what V_TRANS means in terms of slots)
- These are loaded once when the language shard is activated

**In hcp_core (universal):**
- Force type registry
- Relationship type registry
- LoD level registry
- These define the vocabulary; they rarely change

---

## 4. Sub-Categorization Deep Dive

Patrick flagged this for specific attention. Here's my analysis of how sub-cat should work as force constants that aggregate.

### The Aggregation Chain

```
WORD LEVEL:
  "gave" → V_DITRANS pattern → {slot_1: NP(iO), slot_2: NP(dO)}
  This is a force constant: the verb has 2 open bond slots, each typed.

PHRASE LEVEL (VP assembly):
  "gave him a book" → VP
  The V_DITRANS slots are FILLED:
    slot_1 filled by "him" (NP, pronoun, human, animate)
    slot_2 filled by "a book" (NP, indef, physical object)
  Aggregate VP shape: [transfer-action, filled, recipient=human, theme=object]
  Sub-cat pattern has been CONSUMED — no open slots remain.

CLAUSE LEVEL:
  "She gave him a book" → S
  VP presents as: [predicate: transfer, saturated]
  NP presents as: [subject: agent, definite, female, human]
  Clause-level forces: SUBJ↔PRED attraction, both present → grammatical.
  Sub-cat is invisible at this level. It was resolved one level down.
```

### What This Means

Sub-cat is a **word-level** force constant. It creates bond slots that must be filled at the **word-to-phrase transition**. Once filled, the sub-cat pattern is consumed — the phrase presents an aggregate shape to the next level up.

But the sub-cat pattern leaves a TRACE in the aggregate shape. A VP built from V_TRANS has a different aggregate than one built from V_INTRANS:
- V_TRANS VP: [action + affected-entity]
- V_INTRANS VP: [action, self-contained]
- V_DITRANS VP: [transfer + recipient + theme]
- V_INTENS VP: [state + property-of-subject]

These aggregate shapes are what the mesh engine works with. The sub-cat pattern determines which aggregate shape results.

### Sub-Cat Pattern Encoding

For the DB, I recommend:

**A sub-cat pattern table** in each language shard:

```
pattern_id   TEXT PRIMARY KEY   -- e.g., 'V_TRANS'
cat          TEXT               -- e.g., 'V' (what category this pattern applies to)
slot_count   SMALLINT           -- number of complement slots (0-3)
slot_defs    JSONB              -- slot definitions: [{cat: 'NP', func: 'dO'}, ...]
mesh_shape   TEXT               -- aggregate conceptual shape label
notes        TEXT
```

**Why JSONB for slot_defs:** Slot definitions are structured but variable — different patterns have different numbers of slots with different types. A rigid columnar layout would either waste space or limit expressiveness. JSONB allows `[{cat: "NP", func: "dO"}]` for transitive and `[{cat: "NP", func: "iO"}, {cat: "NP", func: "dO"}]` for ditransitive, without needing separate columns.

**Alternative if JSONB is undesirable:** A separate `sub_cat_slots` table:

```
pattern_id   TEXT REFERENCES sub_cat_patterns(pattern_id)
slot_num     SMALLINT          -- 1, 2, 3
slot_cat      TEXT              -- required category: NP, PP, AP, S', to-VP, etc.
slot_func    TEXT              -- grammatical function: dO, iO, sP, oP, PC, comp
specific_p   TEXT              -- for prepositional verbs: the required preposition
```

**Word-to-pattern mapping** is then a dimension on the token:

```
-- In tokens table
sub_cat      TEXT[]            -- array because words can have multiple patterns
                               -- e.g., "know": ['V_TRANS', 'V_THAT', 'V_WH']
```

Or if arrays are undesirable, a junction table:

```
token_id     TEXT REFERENCES tokens(token_id)
pattern_id   TEXT REFERENCES sub_cat_patterns(pattern_id)
frequency    REAL             -- how common this pattern is for this word (for ambiguity resolution)
```

The `frequency` column on the junction table is interesting — it provides the initial energy value for disambiguation when a verb has multiple possible patterns. "Know" as V_THAT is more common than "know" as V_WH, which affects which parse the physics engine prefers.

---

## 5. Number Ranges and Encoding

### Force Strength Scale

For MVP, I recommend a simple two-tier system:

**Tier 1: Constraint Type (boolean)**
- `ABSOLUTE` — violation is ungrammatical. Energy cost = infinity (parse rejected).
- `PREFERENCE` — violation is non-default. Energy cost = finite (parse disfavored).

**Tier 2: Preference Weight (float, 0.0–1.0)**
- `1.0` = absolute constraint (redundant with Tier 1, but useful as single-field encoding)
- `0.8–0.99` = strong preference (default ordering, normal position)
- `0.5–0.79` = moderate preference (common alternative)
- `0.1–0.49` = weak preference (unusual but acceptable)
- `0.0` = no preference (any option equally valid)

**Why this works for MVP:** The linguistics distinguishes ~4 ordinal levels (HIGH/MODERATE/LOW/NONE). The float scale lets us express these with room for refinement. The engine can treat anything at 1.0 as absolute and everything below as ranked preferences.

**Calibration approach:** Start with hand-assigned values from the grammar (I've provided initial values in english-force-patterns.md). Refine using PBM corpus statistics once available. The structure stays the same; only the numbers change.

### Binding Energy Encoding

Same 0.0–1.0 scale:
- Complement bonds: 0.9–1.0 (removal = ungrammatical)
- Restrictive modifiers: 0.5–0.7 (removal changes meaning)
- Adjuncts: 0.1–0.4 (removal simplifies but doesn't break)
- Parenthetical: 0.0–0.1 (freely removable)

---

## 6. Resolving My Open Questions

These were originally posed as questions for the Project Lead. Patrick's architectural framing resolves them.

### Q1: Numeric Scale
**Resolved.** LoD-relative forces mean the scale is always relative within a level, not globally absolute. A float 0.0–1.0 per force per LoD level, with 1.0 reserved for absolute constraints. For MVP, hand-assigned ordinal values (high=0.9, moderate=0.6, low=0.3) are sufficient. The architecture matters more than the exact numbers.

### Q2: Sub-cat Storage
**Resolved.** Sub-cat is a force constant on the token — it's a dimension. The inference engine looks up the token's sub-cat pattern during phrase assembly (word→phrase LoD transition). Pattern definitions live in the language shard as a lookup table. Word-to-pattern mapping is a dimension or junction table on the token. See Section 4 above for specifics.

### Q3: Absolute vs. Preference Distinction
**Resolved.** Yes, distinguish formally. ABSOLUTE constraints (strength=1.0) are inviolable — the physics engine rejects any parse that violates them. PREFERENCES (strength<1.0) increase energy cost but don't invalidate. Both the grammar and the DB need this distinction. See Section 5 above.

### Q4: Cross-Linguistic Universals
**Resolved.** Patrick was explicit: universals → hcp_core, language-specific → language shards. Force CATEGORIES (that attraction exists, that valency is a thing) are in hcp_core. Force CONSTANTS (English is SVO, English auxiliary chain, English sub-cat patterns) are in hcp_english. Same engine, different language file. See Section 1 vs. Section 2.

### Q5: Clausal Sub-Cat Granularity
**Resolved.** Enumerate the PATTERN TYPES (there are ~15 for verbs, ~4 for nouns, ~5 for adjectives, ~3 for prepositions — a manageable set). Each word is classified into one or more patterns. The patterns are the constants; the word-to-pattern mapping is vocabulary. Multiple-pattern words (like "know") get multiple entries with frequency weights. See Section 4.

---

## 7. Summary of DB Actions Needed

### In hcp_core (one-time setup, small tables)

1. **Force type registry** — ~7 rows. Defines ATTR, BIND, ORD, COMPAT, VAL, MOV, REPAIR.
2. **Relationship type registry** — ~7 rows. Defines HEAD_COMP, HEAD_ADJ, SUBJ_PRED, etc.
3. **LoD level registry** — ~8 rows. Defines BYTE through DISCOURSE.
4. **Universal category type registry** — maps the concept "phrasal category is determined by head category" as a rule, not English-specific data.

### In hcp_english (language-specific, some large)

5. **Category tables** — lexical categories (~8), phrasal categories (~6), clause categories (~3). Small, stable.
6. **Sub-cat pattern table** — ~25-30 patterns across V/N/A/P. Small, stable.
7. **Sub-cat slot definitions** — either JSONB on the pattern table or a separate slot table. Small.
8. **Token dimension additions** — `sub_cat` (array or junction table), `count_mass`, `proper_common`, `aux_type` on relevant tokens. This is the large one — every verb, noun, adjective, and preposition needs sub-cat classification.
9. **Ordering rule table** — ~8-10 English-specific ordering constants with strength values.
10. **Movement rule table** — ~5 English-specific movement rules with triggers and targets.
11. **Repair rule table** — ~4 English-specific repair rules.

### Priority for MVP

The sub-cat pattern infrastructure (items 6-8) is the highest-priority DB work for the force pattern system. The inference engine's ability to assemble phrases depends on knowing each verb's bond template. Everything else is smaller and more stable.

The ordering/movement/repair rule tables (items 9-11) are small and can be simple — they're essentially configuration tables.

The universal registries (items 1-4) should be set up early since language-specific tables will reference them.
