# Variant Forms — Wiktionary Audit (2026-03-04)

Migration 023 applied. This document records the initial inventory findings and data quality notes for the linguist specialist.

## What was done

Added `category TEXT` and `canonical_id TEXT REFERENCES tokens(token_id)` to `hcp_english.tokens`. Auto-populated from Wiktionary `form_tags` data (unambiguous pairs only). Updated `compile_vocab_lmdb.py` to exclude `canonical_id IS NOT NULL` rows from `vbed_*` compilation.

## Final counts (post-migration, post-fix)

| Category | Token count | Wiktionary source tags |
|----------|-------------|------------------------|
| archaic  | 5,180 | archaic, obsolete, dated |
| dialect  | 249 | dialectal |
| casual   | 151 | colloquial, informal, slang |
| literary | 21 | poetic |
| **Total** | **5,601** | |

## Data quality issues found

### Circular chains — 7 pairs cleared (14 tokens)

Wiktionary cross-tagged these forms as variants of each other. Both canonical_id and category have been cleared on both sides. **Requires linguist review to determine correct annotation.**

| Pair | Notes |
|------|-------|
| betel ↔ beetle | DIFFERENT words (plant vs insect) — Wiktionary error |
| travel ↔ travail | DIFFERENT modern words — Wiktionary error |
| shave ↔ shove | DIFFERENT modern words — Wiktionary error |
| prophecy ↔ prophesy | Noun vs verb — related but neither is archaic of the other |
| haught ↔ haut | Both should map to "haughty" — fix when confirmed |
| rencontre ↔ rencounter | Both should map to "encounter" — fix when confirmed |
| prejudical ↔ prejudicial | "prejudical" is a misspelling → "prejudicial" is correct |

Recommended fixes:
- `prejudical` → `prejudicial` (clear case)
- `haught`, `haut` → `haughty`
- `rencontre`, `rencounter` → `encounter`
- betel, beetle, travel, travail, shave, shove, prophecy, prophesy: leave as-is (not variants of each other)

### Cascading chains — 62 tokens flattened

These pointed to an intermediate variant that itself pointed to a canonical. All flattened to point directly to the terminal canonical. No manual review needed.

### Ambiguous forms — 71 forms skipped

Surface forms with >1 canonical candidate from Wiktionary. Not auto-populated. **Requires linguist review.**

To inspect:
```sql
WITH all_tagged AS (
    SELECT DISTINCT tag.name AS tag_name, f.form_text AS surface, lower(e.word) AS canonical_word
    FROM form_tags ft
    JOIN tokens tag ON tag.token_id = ft.token_id
      AND tag.name IN ('archaic','obsolete','dated','dialectal','poetic','informal','colloquial','slang')
    JOIN forms f ON f.id = ft.form_id
    JOIN entries e ON e.id = f.entry_id
    WHERE f.form_text IS NOT NULL AND e.word IS NOT NULL
      AND lower(f.form_text) != lower(e.word)
)
SELECT surface, array_agg(DISTINCT canonical_word ORDER BY canonical_word) AS candidates,
       array_agg(DISTINCT tag_name ORDER BY tag_name) AS tags
FROM all_tagged GROUP BY surface
HAVING count(DISTINCT canonical_word) > 1
ORDER BY surface;
```

Notable ambiguous cases: `couldst` → {can, could} (→ should be "could"), `et` → {ate, eat} (→ should be "eat"), `abolisht` → {abolish, abolished} (→ should be "abolish"), `curst` → {curse, cursed} (→ should be "curse").

### Questionable Wiktionary mappings (not structural bugs)

Some auto-populated pairs may be wrong despite being unambiguous in Wiktionary. Flag to check:
- `bang` → `bhang` (bang = hit, bhang = hemp drug — probably wrong)
- `yew` → `you` (yew = tree, you = pronoun — probably wrong)
- `ew` → `yew` (then cascaded to `you` — definitely wrong)

Query to check suspicious archaic pairs:
```sql
SELECT t.name AS surface, tc.name AS canonical, t.category
FROM tokens t
JOIN tokens tc ON tc.token_id = t.canonical_id
WHERE t.category IS NOT NULL
  AND length(t.name) <= 4
ORDER BY t.name;
```

## Multi-token forms — deferred

"gonna" → "going to", "wanna" → "want to", etc. require multi-token decomposition. Not in scope for migration 023. These will go to `var` (category='lingo') until the decomposition design is built. Recommendation: use `atomization` JSONB field or a new `decomposition` junction table when ready.

## LMDB compiler change

`scripts/compile_vocab_lmdb.py`: `fetch_entries()` now filters `AND canonical_id IS NULL`. Variants are excluded from `vbed_*` compilation. Load via envelope queries instead:

```sql
SELECT name AS word, canonical_id AS token_id
FROM tokens
WHERE category = 'archaic'   -- or 'dialect', 'casual', 'literary'
  AND canonical_id IS NOT NULL
  AND length(name) = :wlen
ORDER BY freq_rank ASC NULLS LAST
```

## Remaining annotation work

1. Linguist: review 71 ambiguous forms, fix 7 circular pairs (especially prejudical→prejudicial)
2. Linguist: verify quality of `bang`→`bhang`, `yew`→`you` and similar suspicious pairs
3. Linguist: define additional variant rules for common patterns not in Wiktionary (e.g., `-in'` → `-ing` dialect truncation: darlin', runnin', somethin')
4. Engine specialist: wire env_* fallback in BedManager (or separate resolution layer) to use envelope-loaded variants at scan time; set morph bits 12-14 (VARIANT/VARIANT_ARCHAIC/VARIANT_DIALECT/VARIANT_CASUAL) on resolution from variant env_* entries
