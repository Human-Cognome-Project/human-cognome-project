# Kaikki → hcp_english Population Plan
*Linguist specialist — 2026-03-10*

Reference documents:
- `docs/kaikki-analysis.md` — data structure and counts
- `docs/kaikki-tag-mapping.md` — definitive tag→bit mappings
- `docs/hcp-english-schema-design.md` — schema intent
- `db/migrations/029_new_english_schema.sql` — live schema
- `db/migrations/030_inflection_functions.sql` — inflection helpers

Source: `/opt/project/sources/data/kaikki/english.jsonl` (1,454,988 entries, 2.7GB)

---

## Guiding Principles

1. **No single-shot bulk load.** Work in batches by PoS category. Review before committing.
2. **Transactions.** Every batch runs in an explicit BEGIN/COMMIT block. If anything looks wrong, ROLLBACK.
3. **Non-destructive.** Existing token_ids in `tokens` are never modified. We add to new tables, or INSERT new root tokens with fresh IDs.
4. **Existing tokens first.** When a Kaikki word already exists in `tokens`, use the existing token_id for all new table rows. Never create a duplicate.
5. **Delta variants only.** Standard rule forms (regular past, regular plural, etc.) are not stored. Only irregular/archaic/dialect/spelling deltas go into `token_variants`.
6. **Stop and verify at each pass.** Row counts, spot checks, ask Patrick if anything looks wrong.
7. **Labels last.** The 193K N_PROPER entries are a large, careful job. Do them after common roots are clean.

---

## Token ID Allocation (New Entries)

For tokens NOT already in `tokens`:
- `ns` = 'AB' (English namespace)
- `p2` = 'AA' (standard)
- `p3` = starting character of word (a=AA, b=AB, ..., z=AZ)
- `p4` = word length in characters (1=AA, 2=AB, ..., 50=BX) using base-50 encode
- `p5` = sequential 0-indexed within (p3, p4) bucket, 2500 capacity per bucket

**Sequential tracking**: A helper query gets the current max p5 per (p3, p4) bucket before each insert batch. Sequence continues from there.

**Bucket capacity note**: Some buckets will be dense (e.g., s-words length 5-8). Monitor. At 2500 limit, use p2='AB' (overflow bucket) — Patrick confirmed 2026-03-10. The `alloc_p5()` function in pass1_insert_roots.py handles this automatically and logs a WARNING when overflow occurs.

**Existing token check**: Before any INSERT, check `SELECT token_id FROM tokens WHERE name = $word`. If found, use that token_id for token_pos/glosses/variants. Never INSERT a duplicate name in the same namespace.

---

## Pre-Population: Schema Verification

Before starting any pass, verify migration 029 is live:

```sql
\connect hcp_english
SELECT count(*) FROM token_pos;           -- should be 0
SELECT count(*) FROM token_glosses;       -- should be 0
SELECT count(*) FROM token_variants;      -- should be 0
SELECT count(*) FROM inflection_rules;    -- should be 27
SELECT column_name FROM information_schema.columns
  WHERE table_name = 'tokens'
  AND column_name IN ('source_language', 'characteristics');
```

---

## Pass 0 — Schema Verification ✅ DONE (2026-03-10)

**Decision (Patrick 2026-03-10)**: Skip old-column seeding. Use Kaikki as the PoS source for all entries. Existing tokens will get token_pos records when Kaikki passes process them (name-match → existing token_id reused).

Verification ran: token_pos=0, token_glosses=0, token_variants=0, inflection_rules=27, source_language and characteristics columns present. Schema clean.

---

## Pass 0 — [SUPERSEDED] Wire Existing Tokens from Old Columns

~~The existing 1.39M tokens in `tokens` need token_pos records to be usable by the grammar identifier. Before adding any new tokens from Kaikki, we should ensure existing high-frequency tokens have at least a skeleton token_pos row.~~

~~**Scope**: Tokens with `freq_rank IS NOT NULL` — these are the working vocabulary (approximately 176K tokens from frequency data merge). These are the ones the engine uses now.~~

~~**Source for PoS**: The existing `tokens` table has `layer`, `subcategory`, `aux_type`, `proper_common` columns. These are the old PoS signals. Map them to new `pos_tag` enum:~~

| Old columns | New pos_tag |
|------------|-------------|
| `proper_common = 'proper'` | `N_PROPER` |
| `aux_type IS NOT NULL` | `V_AUX` |
| `layer = 'verb'` AND aux_type NULL | `V_MAIN` |
| `layer = 'noun'` | `N_COMMON` |
| `layer = 'adj'` | `ADJ` |
| `layer = 'adv'` | `ADV` |
| `layer = 'prep'` | `PREP` |
| `layer = 'conj'` | `CONJ_COORD` (refine with Kaikki data in Pass 1) |
| `layer = 'det'` | `DET` |
| `layer = 'pron'` | `N_PRONOUN` |
| `layer = 'intj'` | `INTJ` |
| `layer = 'num'` | `NUM` |

This gets the working vocabulary online quickly. Kaikki passes refine and extend.

**SQL pattern**:
```sql
BEGIN;
INSERT INTO token_pos (token_id, pos, is_primary, morpheme_accept)
SELECT
    t.token_id,
    CASE
        WHEN t.proper_common = 'proper' THEN 'N_PROPER'::pos_tag
        WHEN t.aux_type IS NOT NULL     THEN 'V_AUX'::pos_tag
        WHEN t.layer = 'verb'           THEN 'V_MAIN'::pos_tag
        WHEN t.layer = 'noun'           THEN 'N_COMMON'::pos_tag
        WHEN t.layer = 'adj'            THEN 'ADJ'::pos_tag
        WHEN t.layer = 'adv'            THEN 'ADV'::pos_tag
        ELSE NULL
    END AS pos,
    true AS is_primary,
    CASE
        WHEN t.layer = 'verb' THEN 15   -- PLURAL|PAST|PROGRESSIVE|3RD_SING
        WHEN t.layer = 'noun' THEN 1    -- PLURAL
        WHEN t.layer = 'adj'  THEN 48   -- COMPARATIVE|SUPERLATIVE
        ELSE 0
    END AS morpheme_accept
FROM tokens t
WHERE t.freq_rank IS NOT NULL
  AND t.layer IS NOT NULL
  AND NOT EXISTS (
    SELECT 1 FROM token_pos tp WHERE tp.token_id = t.token_id
  );
-- Review count before committing
COMMIT;
```

~~**Verify**: Row count matches expected. Spot check 10-20 entries for correct PoS assignment.~~

---

## Pass 1 — Root Tokens from Kaikki (Non-name, Non-form-of)

**Scope**: All Kaikki entries where:
- `pos` is NOT in {phrase, prep_phrase, proverb, prefix, suffix, symbol, character, contraction}
- Senses do NOT have `form-of` as the ONLY tag (i.e. this is a root, not purely an inflected form)
- Senses do NOT have `alt-of` as the ONLY tag

**What this creates**: Root token rows in `tokens` for words not already present.

**Batching strategy**: Work PoS category by PoS category, alphabetically within each. This keeps transactions small and auditable.

Order:
1. `verb` entries (217K) — highest engine impact, do first
2. `noun` entries (810K) — largest set, do in alpha batches (a-e, f-m, n-s, t-z)
3. `adj` entries (182K) — alpha batches
4. `adv` entries (27K) — one batch
5. `pron`, `prep`, `conj`, `det`, `intj`, `particle`, `num` — one batch each (small sets)
6. `name` entries (193K) — Pass 6 (last, most careful)

**For each entry**:
1. Lowercase the word
2. Check if already in `tokens` — if yes, record existing token_id, skip INSERT
3. If new: mint token_id using (p3, p4, next_p5) scheme
4. INSERT into `tokens` (name, ns, p2, p3, p4, p5, token_id, characteristics=0)
5. Set `source_language` if BORROWING detected from etymology (see Pass 5)

**Gloss first sense**: Kaikki's first sense gloss (when not form-of/alt-of) is used as the primary gloss. Store with status='DRAFT'.

**Characteristics on tokens row**: Only register/temporal/geographic bits that apply at the ROOT level (not per-PoS-sense). If ALL senses of a word are archaic, set ARCHAIC on tokens. If only some senses are archaic, set it on the specific token_pos record instead.

**Batch size**: ~1000 entries per transaction. Verify row counts before commit.

**Script**: `scripts/pass1_insert_roots.py` — handles streaming, filtering, bucket tracking, collision detection, and batched transactions. Logs to `scripts/pass1_progress.log`.

**Multi-word entries**: Filtered out (space in word) — these are Pass 7 (idioms). Hyphen-prefix fragments also filtered.

**Scope expansion (Patrick 2026-03-10)**: Initialisms are included in existing PoS passes (they appear as noun/adj entries with `abbreviation` tag — handled in Pass 2 via `all_cap` flag on token_pos). Idioms (phrase/prep_phrase) are Pass 7.

---

## Pass 2 — PoS Records (token_pos)

After Pass 1, every token exists. Now add token_pos records for Kaikki-sourced entries.

**For each Kaikki entry**:
1. Look up token_id by name
2. Determine pos_tag from kaikki-tag-mapping.md section 2
3. Check if token_pos (token_id, pos) already exists (from Pass 0 seed)
4. If exists: UPDATE to fill in any missing fields
5. If new: INSERT

**morpheme_accept bitmask**:
```
N_COMMON  → bit 0 (PLURAL)                    = 1
V_MAIN    → bits 1,2,3 (PAST, PROGRESSIVE, 3RD_SING) = 14; + bit 0 if gerund → 15
ADJ       → bits 4,5 (COMPARATIVE, SUPERLATIVE) = 48
ADV       → 0 (most adverbs don't inflect)
```

**Special cases**:
- `be`: V_MAIN + V_AUX + V_COPULA — three token_pos records, is_primary on V_COPULA
- `have`, `do`, `will`, `shall`, `may`, `might`, `can`, `could`, `would`, `should`, `must`: V_MAIN + V_AUX
- Pronouns: N_PRONOUN, morpheme_accept=0 (pronouns don't take plural/possessive in standard English)
- Uncountable nouns (Kaikki `uncountable` tag): N_COMMON, morpheme_accept=0 (no plural)
- Plural-only nouns (Kaikki `plural-only` tag): N_COMMON, set note rather than morpheme_accept

**cap_property** (N_PROPER only): `start_cap` for all names. `all_cap` for entries where the word appears in ALL CAPS in Kaikki (e.g., `fbi`, `nato` stored lowercase but all_cap flagged).

---

## Pass 3 — Gloss Records (token_glosses)

**For each (token, PoS) pair**:
1. Find Kaikki senses for this (word, pos) where NOT form-of and NOT alt-of
2. Take the first non-empty gloss from `senses[0].glosses[0]`
3. INSERT into token_glosses (token_id, pos, gloss_text, status='DRAFT')
4. UPDATE token_pos.gloss_id to point to new gloss record

**Multi-sense words**: If a word has 5 senses as a noun, we take the first sense gloss as the primary gloss. The nuance_note field can capture additional sense distinctions later. For now, one gloss per (token, PoS).

**Words with no usable gloss**: If all senses are form-of/alt-of, skip — the root's gloss will be set when we process the root.

**Label glosses** (N_PROPER): Simple gloss text. If word also has an N_COMMON entry, check if conceptually divorced (meaning assembly decision). For now: insert gloss from Kaikki name entry. If Kaikki has no gloss for the name (common for simple given names), gloss_text = 'proper name'.

---

## Pass 4 — Variants (token_variants)

This is the most careful pass. We only store delta forms — see kaikki-tag-mapping.md section 3.

**Sources for variants**:

### 4a. forms[] delta entries
For each entry's `forms[]` array:
1. Apply `classify_form(form.tags)` logic from kaikki-tag-mapping.md
2. If returns (None, 0): skip
3. If returns (morpheme, characteristics): INSERT token_variants

### 4b. alt-of sense entries
Kaikki entries where primary sense is `alt-of`:
- `word` is the variant surface form
- `alt_of[0].word` is the canonical root
- Tags on the sense → characteristics bitmask
- Example: `colour` → alt-of `color` with British tag → SPELLING_UK variant

### 4c. Irregular form-of with delta tags
Kaikki entries where primary sense is `form-of` AND sense has delta tags (archaic, dialectal, etc.):
- `word` is the irregular form
- `form_of[0].word` is the root
- `classify_form(sense.tags)` → morpheme + characteristics
- Example: `spake` → form-of `speak` with archaic tag → ARCHAIC_PAST variant

### 4d. Contraction entries
From kaikki-tag-mapping.md section 6 — handle each contraction type explicitly. Small set, done carefully.

### 4e. Common misspellings
Kaikki entries tagged `misspelling`:
- morpheme = 'MISSPELLING'
- characteristics = 0
- note = "misspelling of '{correct_form}'"
- Filter: only include misspellings where the canonical form is a known common word (freq_rank IS NOT NULL). Rare-word misspellings are not worth storing.

**Verification after Pass 4**:
```sql
SELECT morpheme, count(*) FROM token_variants GROUP BY morpheme ORDER BY count DESC;
```
Expected rough distribution:
- PLURAL: ~5-15K (irregular plurals)
- PAST: ~5-10K (irregular pasts)
- SPELLING_UK / SPELLING_US: ~10-20K each
- ARCHAIC: ~20-40K
- DIALECT: ~5K
- MISSPELLING: ~2-5K (filtered)

---

## Pass 5 — Loanword Source Language

Set `source_language` and BORROWING bit (bit 24) on tokens with non-native etymology.

**Process**:
1. For each entry with `etymology_text` non-null
2. Apply etymology patterns from kaikki-tag-mapping.md section 4
3. If match found (and not Old English = native): UPDATE tokens SET source_language=..., characteristics = characteristics | (1<<24)
4. If no match: no change

**Batch safely**: Run in batches of 10K. This is UPDATE-only, no new rows. Low risk.

**Verify**:
```sql
SELECT source_language, count(*)
FROM tokens
WHERE source_language IS NOT NULL
GROUP BY source_language ORDER BY count DESC;
```
Expected top: la (Latin), fr (French), de (German), el/grc (Greek), nl (Dutch).

---

## Pass 6 — Labels (N_PROPER)

The 193K `name` PoS entries. Most are personal names, place names, titles.

**Done last** because:
- They're standalone — no dependency on verb/noun passes
- They're the most numerous single PoS category after noun
- Many will already exist in `tokens` (entities, proper nouns already ingested)
- They need care: a name that coincidentally matches a common word (grace, mark) must get its own independent token_pos record, not overwrite the common word's

**Process**:
1. Lowercase the name
2. Check if name exists in tokens:
   a. If exists AND already has N_PROPER token_pos: skip
   b. If exists AND has no N_PROPER token_pos: add N_PROPER record to token_pos (new row)
   c. If NOT exists: INSERT new token (new token_id), then add N_PROPER token_pos
3. cap_property = 'start_cap' by default. 'all_cap' for initialisms.
4. Gloss from Kaikki, or 'proper name' if none available

**Do NOT** import all 193K blindly. The name set includes:
- Personal given names (John, Mary, Patrick) ✓
- Place names (London, Thames, Paris) ✓
- But also: very obscure proper names, fictional character names from niche works, etc.

**Filter**: Only import names that appear in Kaikki with at least one clear gloss, OR names that already appear as vars/labels in our existing ingested texts. Review the top 5-10K by any frequency proxy before committing the full set.

Ask Patrick before committing the full name set — he may want to review a sample first.

---

## Verification and Quality Checks (After All Passes)

```sql
-- Summary
SELECT
    (SELECT count(*) FROM tokens)           AS tokens,
    (SELECT count(*) FROM token_pos)        AS token_pos_records,
    (SELECT count(*) FROM token_glosses)    AS gloss_records,
    (SELECT count(*) FROM token_variants)   AS variant_records;

-- PoS distribution
SELECT pos, count(*) FROM token_pos GROUP BY pos ORDER BY count DESC;

-- Gloss coverage
SELECT status, count(*) FROM token_glosses GROUP BY status;

-- Variant morpheme distribution
SELECT morpheme, count(*) FROM token_variants GROUP BY morpheme ORDER BY count DESC;

-- Source language distribution
SELECT source_language, count(*) FROM tokens
WHERE source_language IS NOT NULL
GROUP BY source_language ORDER BY count DESC;

-- Check no orphaned token_pos (pos records pointing to missing tokens)
SELECT count(*) FROM token_pos tp
LEFT JOIN tokens t ON t.token_id = tp.token_id
WHERE t.token_id IS NULL;
```

**Regression test**: After population, run Dracula and Sign of Four through the engine. Var rates should not increase from current baseline (Dracula: 110 vars / 199K tokens). If var rate increases, investigate before proceeding to LMDB compiler update.

---

## Pass 7 — Idioms and Multi-word Expressions

**Patrick confirmed (2026-03-10)**: Include idioms, prep_phrases, and proverbs as distinct elements.

**Scope**: Kaikki entries where pos IN {phrase, prep_phrase, proverb} AND word contains space AND is_root (not purely form-of/alt-of).

**From kaikki-analysis.md**: phrase=5,061 + prep_phrase=2,973 + proverb=1,546 = ~9,580 entries.

**Token_id scheme**: Same as Pass 1. p3 = first letter of first word. p4 = total character length (including spaces). p5 = sequential within bucket. Longer phrases have unusual p4 values but the scheme handles them.

**PoS mapping**: No standard pos_tag for multi-word expressions. Options:
- Store with the gloss only, no token_pos record (lookup by name returns token_id, meaning assembly uses gloss directly)
- OR: Add `PHRASE` to pos_tag enum (requires schema change — consult DB specialist)

Ask Patrick before running. Do AFTER all single-word passes are verified clean.

---

## Checkpoint Questions for Patrick

**Answered (2026-03-10)**:
1. ~~Pass 0 (seeding token_pos from existing tokens)~~ → SKIP. Use Kaikki as PoS source. Existing tokens get token_pos records when Kaikki finds their name.
2. ~~Pass 6 (Labels) — import all 193K or filter?~~ → Proceed. Watch for john/John collisions (common word and proper noun with distinctly different glosses). Same label may need own gloss only when completely divorced from the common word meaning.
3. ~~Bucket overflow~~ → Use p2='AB' for overflow. Handled automatically in script.
4. **Idioms/initialisms** → Include. Initialisms go through normal PoS passes (all_cap flag). Idioms are Pass 7.

**Still open**:
5. Pass 7 (Idioms) — PoS representation: store with no token_pos (gloss only), or add PHRASE to pos_tag enum?

---

## Task Dependencies

This plan feeds:
- Task #16: LMDB compiler update (needs token_pos.pos + characteristics in vbed entries)
- Task #17: Verification run (needs full population complete)
- Grammar identifier kernel (needs token_pos.pos per token at lookup time)
