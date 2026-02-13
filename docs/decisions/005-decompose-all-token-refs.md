# Decision 005: Decompose All Token ID References

**Date:** 2026-02-12
**Status:** Implemented
**Shards affected:** hcp_core, hcp_english

## Context

Decision 001 decomposed the tokens table PK into (ns, p2, p3, p4, p5) columns with a generated TEXT token_id. However, all other columns storing token IDs (foreign references, tag arrays, etymology arrays, gloss arrays) still used monolithic TEXT strings or TEXT[] arrays.

## Decision

Apply the same decomposition pattern to **every** column storing a token ID reference:

1. **Single token_id columns** → decomposed (prefix_ns, prefix_p2, prefix_p3, prefix_p4, prefix_p5) + GENERATED prefix_token_id + compound B-tree index
2. **Token ID arrays (TEXT[])** → junction tables with (parent_id, position, ns, p2, p3, p4, p5) + GENERATED token_id + compound B-tree index

## Uniform Pattern

Every token reference in the database now follows one of two patterns:

### Inline reference (single token)
```sql
-- Column naming: prefix_ns, prefix_p2, ... prefix_p5
-- Generated: prefix_token_id (reconstructed from parts)
-- Index: compound B-tree on (prefix_ns, prefix_p2, prefix_p3, prefix_p4, prefix_p5)

word_ns TEXT, word_p2 TEXT, word_p3 TEXT, word_p4 TEXT, word_p5 TEXT,
word_token TEXT GENERATED ALWAYS AS (
    word_ns || COALESCE('.' || word_p2, '') || COALESCE('.' || word_p3, '') ||
    COALESCE('.' || word_p4, '') || COALESCE('.' || word_p5, '')
) STORED
```

### Junction table reference (array of tokens)
```sql
-- Table naming: parent_noun + semantic role (e.g., entry_etymology, form_tags)
-- PK: (parent_id, position) — preserves original array ordering
-- Columns: (ns, p2, p3, p4, p5) + GENERATED token_id
-- Index: compound B-tree on (ns, p2, p3, p4, p5)

CREATE TABLE form_tags (
    form_id INTEGER NOT NULL REFERENCES forms(id),
    position SMALLINT NOT NULL,
    ns TEXT NOT NULL, p2 TEXT, p3 TEXT, p4 TEXT, p5 TEXT,
    token_id TEXT GENERATED ALWAYS AS (...) STORED NOT NULL,
    PRIMARY KEY (form_id, position)
);
```

## Why Junction Tables Over Parallel Arrays

Considered storing `ns_arr TEXT[], p2_arr TEXT[]` etc. Rejected because:
- TEXT[] arrays cannot use B-tree prefix compression
- No FK constraints possible on array elements
- Parallel array indexing is error-prone and unmaintainable
- Junction tables use the same uniform (ns, p2, p3, p4, p5) pattern as everything else

## Migration Results

### Phase 1: Single column decomposition

| Table | Column | Rows Decomposed |
|-------|--------|----------------|
| entries | word_token → (word_ns..word_p5) | 1,406,336 |
| entries | pos_token → (pos_ns..pos_p5) | 1,430,172 |
| forms | form_token → (form_ns..form_p5) | 690,057 |
| relations | relation_token → (rel_ns..rel_p5) | 449,534 |
| relations | target_token → (tgt_ns..tgt_p5) | 409,596 |
| hcp_core metadata | token_id → (ns..p5) | 0 (schema-only) |

### Phase 2+3: Junction tables

| Junction Table | Source Column | Rows |
|---------------|--------------|------|
| relation_tags | relations.tag_tokens | 11,946 |
| form_components | forms.form_tokens | 387,724 |
| sense_tags | senses.tag_tokens | 1,505,304 |
| entry_etymology | entries.etymology_tokens | 3,469,262 |
| form_tags | forms.tag_tokens | 2,346,489 |
| sense_glosses | senses.gloss_tokens | 10,746,878 |

**Total junction table rows: 18,467,603**

### Final schema (hcp_english)

| Table | Rows | Role |
|-------|------|------|
| tokens | 1,396,117 | Token definitions |
| entries | 1,433,117 | Dictionary entries |
| forms | 2,148,493 | Form variants |
| relations | 449,534 | Word relationships |
| senses | 1,490,650 | Sense definitions (id + entry_id only) |
| entry_etymology | 3,469,262 | Etymology token chains |
| form_tags | 2,346,489 | Grammatical tags on forms |
| form_components | 387,724 | Component tokens of forms |
| sense_tags | 1,505,304 | Tags on sense definitions |
| sense_glosses | 10,746,878 | Gloss token sequences |
| relation_tags | 11,946 | Tags on relations |

**Zero TEXT[] array columns remain. Every token reference is decomposed.**

## Column Naming Convention

| Column Prefix | Used On | Meaning |
|--------------|---------|---------|
| (none: ns, p2..p5) | tokens table, junction tables | The token itself |
| word_ | entries | Word being defined |
| pos_ | entries | Part of speech reference |
| form_ | forms | Token for this form variant |
| rel_ | relations | Relation type token |
| tgt_ | relations | Target token of relation |

## Files

- `db/migrations/005a_decompose_single_refs.sql` — Phase 1
- `db/migrations/005b_decompose_core_metadata.sql` — Phase 1 (hcp_core)
- `db/migrations/005c_junction_tables.sql` — Phase 2 (5 junction tables)
- `db/migrations/005d_junction_sense_glosses.sql` — Phase 3 (sense_glosses)
- `db/migrations/005_decompose_refs.sh` — Orchestration script
