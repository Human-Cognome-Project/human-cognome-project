# PBM & Entity DB Schema Review

**From:** hcp_db (DB Specialist)
**Reviewing:** pbm-format-spec.md (792d662), entity-db-design.md (4a15e64)
**Date:** 2026-02-13

---

## Overall Assessment

Both specifications are solid and follow project conventions. Decomposed references throughout, no TEXT[] arrays, junction tables where needed. The fiction/non-fiction split is architecturally sound. Three issues and two recommendations below.

---

## 1. Schema Sanity Check

### PBM Format Spec — Approved with one fix

**Follows conventions:**
- Decomposed (ns, p2, p3, p4, p5) references on all tables ✓
- Generated token_id columns using || and COALESCE ✓
- No TEXT[] arrays anywhere ✓
- Row-per-token content stream (junction table pattern) ✓
- Position-anchored metadata as key-value rows, not JSONB ✓
- Structural markers as 4-pair tokens (AA.AE.AA.AA) — valid, p5 NULL is supported ✓

**Issue: Redundant index on pbm_content.**

```sql
-- This index duplicates the primary key:
CREATE INDEX idx_pbm_content_doc ON pbm_content (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5, position);
-- PK already covers: PRIMARY KEY (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5, position)
```

The PK's underlying B-tree index already serves all queries that this index would serve. Remove it. The other two indexes (idx_pbm_content_token, idx_pbm_content_doc_id) are correct and necessary.

### Entity DB Design — Approved with one concern

**Follows conventions:**
- All 7 tables use decomposed references ✓
- Generated token_id columns ✓
- No TEXT[] arrays ✓
- entity_names uses junction table pattern with position column ✓
- entity_properties uses simple key-value (EAV), acceptable for bootstrap ✓

**Concern: document_rights table duplicates document_provenance fields.**

The PBM spec's `document_provenance` already has: rights_status, copyright_holder, copyright_year, license_type, reproduction_rights, ip_notes.

The entity spec proposes a separate `document_rights` table with: rights_status, rights_holder, license_type, plus jurisdiction, expiry_year, determination_date, determination_source, source_catalog, catalog_id.

**Recommendation:** Don't create a separate table. Add the missing fields (jurisdiction, expiry_year, determination_date, determination_source, source_catalog, catalog_id) to `document_provenance`. One table for all per-document metadata including rights. This avoids:
- Two tables covering the same concern
- Potential inconsistency between rights_status in provenance vs. document_rights
- Extra JOINs for rights queries

If the entity spec's document_rights table is dropped, the merged provenance table covers everything.

### AA.AF Namespace for Entity Sub-Types — Approved

Clean extension of the AA namespace:

| p2 | Domain |
|----|--------|
| AA.AA | Encoding |
| AA.AB | Text Encodings |
| AA.AC | Structural Tokens (+ force infrastructure at AC.AB) |
| AA.AD | Abbreviation Classes |
| AA.AE | PBM Structural Markers (91 tokens, proposed) |
| AA.AF | Entity Classification (~17 sub-types + relationship types, proposed) |

Each p2 gets a logical domain. Entity sub-types and relationship types at AA.AF are distinct from linguistic relationship types at AA.AC.AB.AB — different domains, no conflict.

---

## 2. Namespace Allocation — The 8-Namespace Split

### Assessment: Sound

| Side | PBMs | People | Places | Things |
|------|------|--------|--------|--------|
| Non-fiction | z* | y* | x* | w* |
| Fiction | v* | u* | t* | s* |

**What works:**
- Complete fiction/non-fiction separation eliminates entity collision
- Each entity type gets its own prefix for independent scaling
- shard_registry routes by ns_prefix (2-char, e.g., "uA" → hcp_fic_entities)
- Cross-side references are supported (real places appearing in fiction → entity_appearances links x* entity to v* PBM)

**Bootstrap approach (2 combined entity DBs) is correct:**
- hcp_fic_entities holds u*, t*, s* — split when any type exceeds ~500MB
- hcp_nf_entities holds y*, x*, w* — same split policy
- shard_registry entries at 2-char level (uA, tA, sA, yA, xA, wA) enable transparent splitting

**yA re-allocation is clean.** Decision 002 retired yA (name components) and the hcp_names database was dropped. The letter is available for re-allocation to non-fiction people entities. The old namespace_allocations entry for y* needs to be UPDATED (not just supplemented).

### Required namespace_allocations changes

```sql
-- Update old y* entry (was "Proper Nouns & Abbreviations")
UPDATE namespace_allocations
SET name = 'NF People',
    description = 'Real person entities — historical and contemporary'
WHERE pattern = 'y*';

-- Insert new allocations (per both specs)
-- [see implementation below]
```

---

## 3. PBM Source Field

**Already covered.** The `document_provenance` table has:

| Field | Type | Description |
|-------|------|-------------|
| source_type | TEXT | 'file', 'url', 'api', 'manual' |
| source_path | TEXT | "Original file path or URL" |

For Gutenberg: `source_type = 'url'`, `source_path = 'https://www.gutenberg.org/files/84/84-0.txt'`. For local files: `source_type = 'file'`, `source_path = '/data/books/analysing-sentences.pdf'`.

**No changes needed.** The field exists, is TEXT (as requested for pre-web-addressing), and handles both URLs and file paths.

If the Project Lead wants the Gutenberg book number specifically, that's covered by the `source_catalog`/`catalog_id` fields I recommend merging from document_rights into provenance (see §1 above). With the merge: `source_catalog = 'gutenberg'`, `catalog_id = '84'`.

---

## 4. Feedback and Concerns

### Minor items (not blocking)

1. **entity_relationships has no duplicate protection.** SERIAL PK means identical relationships can be inserted. Consider adding a unique constraint on `(source_ns..source_p5, target_ns..target_p5, relationship_type, temporal_note)` or at minimum document that duplicates should be prevented at application level.

2. **entity_descriptions SERIAL PK allows multiple descriptions of same type.** Intentional per source_note field (different sources may describe the same aspect differently). Acceptable — just confirm this is desired behavior.

3. **PBM structural markers use 4-pair addressing (p5 = NULL).** This is correct and intentional — 2,500 values at p4 level is more than enough for each marker category. Room for growth if needed.

4. **Cross-DB referential integrity.** Entity appearances reference PBM token IDs in z*/v*. No FK constraints possible across databases. This is expected — the engine resolves cross-shard references via shard_registry at read time. Just document that cross-shard references are validated at application level, not DB level.

5. **The tokens table in entity DBs doesn't have a metadata JSONB column.** The PBM spec's tokens table has one, hcp_core has one, hcp_english has one (as `metadata` and/or `atomization`). The entity DB spec omits it. Should include `metadata JSONB DEFAULT '{}'::jsonb` for consistency, even if rarely used.

### Not blocking, but worth noting

The entity spec's relationship type vocabulary (§3.5.1) is extensive (~40 relationship types). These map to AA.AF.AD tokens per Appendix A. The force infrastructure relationship types (AA.AC.AB.AB — head_complement, subject_predicate, etc.) are linguistic structure relationships. The entity relationship types (AA.AF.AD — parent_of, killed_by, etc.) are semantic/narrative relationships. These are orthogonal — no conflict, no overlap.

---

## 5. Summary

| Aspect | Assessment |
|--------|-----------|
| PBM format spec | **Approved** — remove redundant index |
| Entity DB design | **Approved** — merge document_rights into provenance |
| 8-namespace split | **Approved** — clean fiction/non-fiction separation |
| Bootstrap 2-DB approach | **Approved** — shard_registry handles split transparently |
| PBM source field | **Already covered** by document_provenance.source_path |
| AA.AF entity sub-types | **Approved** — clean namespace extension |
| AA.AE PBM markers | **Approved** — 91 tokens, 4-pair addressing |

---

## 6. Phase 1 Implementation Plan

Once these specs are approved:

1. **Update namespace_allocations** — new y*/x*/w*/v*/u*/t*/s* entries, update old y* entry, add AA.AE + AA.AF categories
2. **Insert 91 PBM structural markers** into hcp_core.tokens (AA.AE namespace)
3. **Insert ~17 entity sub-type tokens** into hcp_core.tokens (AA.AF namespace)
4. **Create hcp_en_pbm database** — 8 tables per PBM spec §6 (with document_rights fields merged into provenance)
5. **Create hcp_fic_entities database** — 7 tables per entity spec §3
6. **Create hcp_nf_entities database** — identical schema
7. **Register new shard entries** — uA, tA, sA, yA, xA, wA in shard_registry
8. **Update spec docs** — namespace-reference.md and token-addressing.md to reflect new 8-namespace scheme
