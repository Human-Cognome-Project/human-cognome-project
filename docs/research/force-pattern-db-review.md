# Force Pattern DB Review

**From:** hcp_db (DB Specialist)
**Reviewing:** [force-pattern-db-requirements.md](force-pattern-db-requirements.md) by hcp_ling
**Date:** 2026-02-12

---

## Overall Assessment

The linguistics analysis is thorough and the architectural reasoning is sound. The separation of universal force types (hcp_core) from language-specific constants (hcp_english) is exactly right. The sub-cat pattern analysis — patterns as a small lookup table, word-to-pattern as vocabulary — is an elegant decomposition that maps cleanly to DB structures.

There are **three structural issues** that need to be resolved before implementation, all stemming from the same root cause: the proposal designs around TEXT shortcode primary keys, which conflicts with the decomposed token ID pattern that now governs the entire database.

---

## 1. The Token ID Consistency Problem

### What the proposal does

Every registry table uses human-readable TEXT shortcodes as PKs:

```
force_type_id = 'ATTR'
rel_type_id   = 'HEAD_COMP'
lod_level_id  = 'WORD'
pattern_id    = 'V_TRANS'
cat_id        = 'N'
```

### Why this is a problem

After Decisions 001 and 005, **every token reference in both databases** uses the same pattern: decomposed (ns, p2, p3, p4, p5) columns with a generated token_id and compound B-tree index. This uniformity is the whole point — it drives B-tree prefix compression, enables consistent LMDB export, and means application code has exactly one pattern for token reference.

TEXT shortcode PKs create a parallel addressing system. Code that processes force patterns would need different lookup logic than code that processes word tokens, etymology chains, or sense glosses. Cross-referencing becomes string matching instead of prefix scanning. The B-tree compression story falls apart.

### What should happen instead

Force types, relationship types, LoD levels, and structural categories are all **universal concepts** — they belong in the AA namespace as proper tokens in hcp_core. Specifically:

- **Force types** → AA.AC namespace (structural tokens). 7 tokens: attraction, binding_energy, ordering, compatibility, valency, movement, structural_repair.
- **Relationship types** → AA.AC namespace. 7 tokens: head_complement, head_adjunct, subject_predicate, determiner_nominal, coordination, movement_trace, coreference.
- **LoD levels** → AA.AC namespace. 8 tokens: byte, character, morpheme, word, phrase, clause, sentence, discourse.
- **Universal category types** → AA.AC namespace. The concept "phrasal categories are determined by head categories" is an AA structural token.

These are small additions (~25 tokens) to the existing 2,719 structural tokens in AA.AC.

**Language-specific categories** (N, V, NP, VP, S, etc.) and **sub-cat patterns** (V_TRANS, V_DITRANS, etc.) are English-specific — they go in hcp_english as AB namespace tokens.

Then every rule table, slot definition, and mapping table references these concepts via decomposed (ns, p2, p3, p4, p5) columns. One pattern everywhere.

### What the human-readable labels become

The shortcodes ('ATTR', 'V_TRANS', 'WORD') become the `name` column on the token — exactly how every other concept is already stored. `token_id = 'AA.AC.AA.CK.xx'`, `name = 'attraction'`. Queries by name still work via the existing `idx_tokens_name` index. The addressing system provides structure; the name provides readability.

---

## 2. JSONB Slot Definitions — Use Junction Table Instead

### What the proposal does

Two options are offered for sub-cat slot definitions:
- Option A: JSONB `slot_defs` column on the pattern table
- Option B: Separate `sub_cat_slots` table

### Recommendation: Option B (junction table with decomposed references)

The proposal correctly notes Option B as an alternative. Given Decision 005 — where we converted all 6 array columns to junction tables — this is the consistent choice.

```sql
CREATE TABLE sub_cat_slots (
    -- Pattern reference (decomposed)
    pat_ns TEXT NOT NULL, pat_p2 TEXT, pat_p3 TEXT, pat_p4 TEXT, pat_p5 TEXT,
    pat_token TEXT GENERATED ALWAYS AS (...) STORED NOT NULL,

    slot_num SMALLINT NOT NULL,   -- 1, 2, 3

    -- Required category (decomposed reference to category token)
    cat_ns TEXT NOT NULL, cat_p2 TEXT, cat_p3 TEXT, cat_p4 TEXT, cat_p5 TEXT,
    cat_token TEXT GENERATED ALWAYS AS (...) STORED NOT NULL,

    -- Grammatical function (decomposed reference to function token)
    func_ns TEXT, func_p2 TEXT, func_p3 TEXT, func_p4 TEXT, func_p5 TEXT,
    func_token TEXT GENERATED ALWAYS AS (...) STORED,

    -- For prepositional verbs: required preposition (decomposed)
    prep_ns TEXT, prep_p2 TEXT, prep_p3 TEXT, prep_p4 TEXT, prep_p5 TEXT,
    prep_token TEXT GENERATED ALWAYS AS (...) STORED,

    PRIMARY KEY (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num)
);
```

This is ~60-80 rows total (25-30 patterns × 1-3 slots each). Tiny table, but consistent with the decomposition pattern. The B-tree on the pattern prefix columns gives instant lookup of all slots for a given pattern.

JSONB would be opaque to the B-tree indexing system and create an inconsistency where some token references are decomposed pairs and others are embedded in JSON blobs. In a system this small, the overhead of a junction table is negligible; the consistency benefit is large.

---

## 3. TEXT[] Array for Word-to-Pattern Mapping — Must Be Junction Table

### What the proposal does

Suggests `sub_cat TEXT[]` on the tokens table as one option.

### This is now ruled out

Decision 005 eliminated all TEXT[] arrays from the entire database and replaced them with junction tables. We cannot re-introduce arrays. The junction table approach the proposal also mentions is correct:

```sql
CREATE TABLE token_sub_cat (
    -- Token reference (decomposed)
    tok_ns TEXT NOT NULL, tok_p2 TEXT, tok_p3 TEXT, tok_p4 TEXT, tok_p5 TEXT,
    tok_token TEXT GENERATED ALWAYS AS (...) STORED NOT NULL,

    -- Pattern reference (decomposed)
    pat_ns TEXT NOT NULL, pat_p2 TEXT, pat_p3 TEXT, pat_p4 TEXT, pat_p5 TEXT,
    pat_token TEXT GENERATED ALWAYS AS (...) STORED NOT NULL,

    frequency REAL DEFAULT 0.5,  -- disambiguation weight

    PRIMARY KEY (tok_ns, tok_p2, tok_p3, tok_p4, tok_p5,
                 pat_ns, pat_p2, pat_p3, pat_p4, pat_p5)
);
CREATE INDEX idx_token_sub_cat_pat ON token_sub_cat(
    pat_ns, pat_p2, pat_p3, pat_p4, pat_p5
);
```

The `frequency` column is a good idea — it provides initial energy values for the inference engine's disambiguation. The compound PK ensures each word-pattern pair is unique. The reverse index on pattern columns enables "find all words with this pattern" queries.

---

## 4. Specific Table-by-Table Assessment

### hcp_core registries (items 1-4)

**Force types, relationship types, LoD levels:** Approve with token ID modification above. These become ~25 new tokens in AA.AC. No new tables needed — they're rows in the existing `tokens` table with appropriate metadata.

The `aggregates_from` / `resolves_to` columns proposed on the LoD registry become metadata or a small linking table with decomposed token references. Given there are only 8 LoD levels, a `lod_transitions` table or JSONB metadata on the tokens themselves is fine. I'd lean toward a simple 2-column table:

```sql
CREATE TABLE lod_transitions (
    -- Lower LoD level
    from_ns TEXT NOT NULL, from_p2 TEXT, from_p3 TEXT, from_p4 TEXT, from_p5 TEXT,
    from_token TEXT GENERATED ALWAYS AS (...) STORED NOT NULL,
    -- Higher LoD level
    to_ns TEXT NOT NULL, to_p2 TEXT, to_p3 TEXT, to_p4 TEXT, to_p5 TEXT,
    to_token TEXT GENERATED ALWAYS AS (...) STORED NOT NULL,
    PRIMARY KEY (from_ns, from_p2, from_p3, from_p4, from_p5)
);
```

7 rows. Tiny but explicit.

**Universal category type registry (item 4):** The rule "phrasal category = head category" is a structural principle. It becomes a token in AA.AC with a descriptive name. The actual mapping (N→NP, V→VP) lives in hcp_english as language-specific data.

### hcp_english category tables (item 5)

Lexical categories (N, V, A, etc.), phrasal categories (NP, VP, etc.), and clause categories (S, S', S'') become **tokens in hcp_english** — likely in a new subcategory under AB or in the existing structural area. They're referenced by decomposed token ID from the rule tables.

~17 tokens total. Could also be metadata on existing PoS tokens that already exist in hcp_english (we already have "noun", "verb", "adj" etc. as tokens used for pos_token references). The structural category labels (N, V, NP, VP) could be alternate names or form variants of the existing PoS tokens, linking linguistic category theory to the existing data.

This is a design question for the Project Lead: are structural categories the same entities as the existing PoS tokens, or distinct concepts with a mapping between them?

### Sub-cat pattern table (item 6)

Approve. ~25-30 tokens in hcp_english with the slot table described above.

### Ordering rules (item 9)

Small table (~8-10 rows). Each rule has:
- Rule token (decomposed)
- LoD level (decomposed reference to AA.AC LoD token)
- Constraint type (ABSOLUTE or PREFERENCE — could be a boolean or enum)
- Strength (REAL, 0.0-1.0)
- Rule definition (TEXT description for human readability)

```sql
CREATE TABLE ordering_rules (
    -- Rule identifier (decomposed token reference)
    rule_ns TEXT NOT NULL, rule_p2 TEXT, rule_p3 TEXT, rule_p4 TEXT, rule_p5 TEXT,
    rule_token TEXT GENERATED ALWAYS AS (...) STORED NOT NULL,

    -- LoD level (decomposed reference to AA.AC lod token)
    lod_ns TEXT NOT NULL, lod_p2 TEXT, lod_p3 TEXT, lod_p4 TEXT, lod_p5 TEXT,
    lod_token TEXT GENERATED ALWAYS AS (...) STORED NOT NULL,

    is_absolute BOOLEAN NOT NULL DEFAULT false,
    strength REAL NOT NULL DEFAULT 0.5,
    rule_def TEXT,

    PRIMARY KEY (rule_ns, rule_p2, rule_p3, rule_p4, rule_p5)
);
```

### Movement rules (item 10) and Repair rules (item 11)

Same pattern as ordering rules. Small tables (~5 and ~4 rows respectively). Each rule is a token with decomposed references to trigger conditions and LoD levels.

### Token dimension additions (item 8)

Adding `count_mass`, `proper_common`, `aux_type` to the tokens table is fine — these are single-value properties on noun/auxiliary tokens. They can be simple TEXT or BOOLEAN columns. Small, fixed vocabulary of values.

However, `sub_cat` must be the junction table (token_sub_cat) described above, not a column.

---

## 5. Implementation Approach

### Recommended order

1. **AA.AC tokens first** — Add ~25 universal tokens (force types, LoD levels, relationship types) to hcp_core. These are the vocabulary that everything else references.

2. **Category tokens** — Add structural category tokens (N, V, NP, VP, etc.) to hcp_english. Resolve the PoS mapping question with Project Lead.

3. **Sub-cat pattern tokens + slot table** — Create the ~25-30 pattern tokens and the sub_cat_slots junction table in hcp_english.

4. **token_sub_cat junction table** — The word-to-pattern mapping. This will be the large one — every verb, noun, adjective, and preposition needs classification. But initially it can be empty, populated as ingestion proceeds.

5. **Rule tables** — ordering_rules, movement_rules, repair_rules. Small, hand-populated tables in hcp_english.

6. **Token dimensions** — Add count_mass, proper_common, aux_type columns to hcp_english tokens table via ALTER TABLE.

### What can wait

The frequency values on token_sub_cat and the exact strength values on rule tables are calibration data. The schema should be built first with placeholder values. Calibration happens when PBM corpus data is available.

---

## 6. Questions for Project Lead

1. **PoS ↔ structural category mapping:** Are the linguistic structural categories (N, V, A, etc.) the same entities as the existing PoS tokens (noun, verb, adj)? Or are they distinct concepts that reference each other? This determines whether we add new tokens or annotate existing ones.

2. **Force profile on relationship types:** The proposal shows `force_profile` as a complex multi-valued field (e.g., "ATTR:strong, BIND:high, ORD:language-specific"). Should this be another junction table (relationship × force_type → value), or is it documentation-only for now?

3. **Namespace allocation:** Should force types and LoD levels get their own sub-range within AA.AC, or just be added to the existing structural token pool? A designated range (e.g., AA.AC.AB for force infrastructure) would keep them organized.

---

## Summary

| Aspect | Assessment |
|--------|-----------|
| Overall architecture | Approved — universal/language-specific split is correct |
| Force type registries | Approve as AA.AC tokens, not TEXT shortcode tables |
| Sub-cat pattern analysis | Excellent — small pattern set with word-to-pattern mapping |
| JSONB slot definitions | Replace with decomposed junction table |
| TEXT[] on tokens | Replace with junction table (Decision 005 precedent) |
| Ordering/movement/repair rules | Approve as small rule tables with decomposed references |
| Token dimensions (count_mass etc.) | Approve as simple columns |
| Sub-cat MVP priority | Agree — this is the highest-priority DB work |
| LoD-tagging of constants | Correct and important for the inference engine |
| Frequency weights | Good idea for disambiguation — include from the start |

**Bottom line:** The linguistics analysis is solid. The DB implementation needs to route everything through the decomposed token ID system — no parallel addressing, no JSONB for structured token data, no TEXT[] arrays. With those adjustments, the proposed schema maps cleanly to our existing patterns and should be straightforward to implement.
