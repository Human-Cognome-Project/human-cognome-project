-- Migration 023: Variant form support
-- Target: hcp_english
-- Depends on: 002 (tokens schema), 016 (lowercase normalization)
--
-- Adds two columns to tokens:
--   category    — register/era tag: archaic, dialect, casual, literary, formal
--   canonical_id — FK to the base token_id this surface form maps to
--
-- Design: variant forms (darlin'→darling, betwixt→between, abandoneth→abandon)
-- are rows in tokens with their own token_id (DB-internal identifier) that point
-- to a canonical token_id via canonical_id. At scan time, the engine resolves
-- the variant surface → canonical token_id + variant morph bits. The variant's
-- own token_id never enters the position map.
--
-- This is structurally parallel to inflection stripping: "walking" has a row
-- in tokens but resolves to "walk"'s token_id + PROG bit at scan time.
--
-- LMDB vbed_* compilation excludes canonical_id IS NOT NULL rows — variants
-- are loaded via envelope activation into env_* sub-dbs only (keep vbed lean).
-- Envelope query pattern:
--   SELECT name AS word, canonical_id AS token_id
--   FROM tokens
--   WHERE category = 'archaic' AND canonical_id IS NOT NULL
--   AND length(name) = :wlen
--
-- Morph bits 12-14 (reserved, now assigned):
--   Bit 12: VARIANT         — any registered variant surface form
--   Bit 13: VARIANT_ARCHAIC — archaic/poetic (o'er, betwixt, abandoneth)
--   Bit 14: VARIANT_DIALECT — dialectal/truncation (ag'in, acrost, brang)
--   Bit 15: VARIANT_CASUAL  — casual/informal (gonna → deferred multi-token)
-- Engine sets these bits when resolving from env_* variant sub-dbs.
--
-- Auto-population: unambiguous Wiktionary form_tags (5,897 clean pairs).
-- Ambiguous cases (71 forms with >1 canonical candidate) are logged as NOTICEs
-- for linguist review. Multi-token decompositions (gonna→going to) are deferred.
--
-- Deferred question: particle_key bucketing for apostrophe-containing variants.
-- (Not a blocker — env_* doesn't use particle_key for bed assignment.)

\connect hcp_english

BEGIN;

-- ============================================================
-- Step 1: Add columns
-- ============================================================

ALTER TABLE tokens
    ADD COLUMN IF NOT EXISTS category     TEXT,
    ADD COLUMN IF NOT EXISTS canonical_id TEXT REFERENCES tokens(token_id);

COMMENT ON COLUMN tokens.category IS
    'Register/era tag: archaic, dialect, casual, literary, formal. NULL = standard modern English.
     Used by LMDB compiler (exclude from vbed_*) and envelope queries for context-driven loading.
     Multi-tag forms use priority: dialect > casual > archaic > literary.';

COMMENT ON COLUMN tokens.canonical_id IS
    'Canonical token_id this surface form maps to. NULL = this token IS canonical (or canonical unknown).
     When set: engine resolves to canonical_id + variant morph bits; this row''s token_id never enters
     the position map. LMDB envelope queries write canonical_id as the lookup value.
     Examples: abandoneth -> abandon.token_id (archaic);  ag''in -> again.token_id (dialect).
     Multi-token decompositions (gonna -> going+to) are deferred: canonical_id stays NULL,
     forms go to var until decomposition design is implemented.';

CREATE INDEX IF NOT EXISTS idx_tokens_category     ON tokens (category);
CREATE INDEX IF NOT EXISTS idx_tokens_canonical_id ON tokens (canonical_id);

-- ============================================================
-- Step 2: Build variant pairs from form_tags (Wiktionary data)
-- ============================================================
-- Source: form_tags (usage label tokens) → forms (surface text) → entries (canonical word)
-- Tag → category mapping:
--   dialectal                        → 'dialect'
--   colloquial, informal, slang      → 'casual'
--   archaic, obsolete, dated         → 'archaic'
--   poetic                           → 'literary'
-- Multi-tag forms: highest-priority category wins (dialect > casual > archaic > literary)
-- Exclude: self-references (lower(surface) = canonical_word)
-- Exclude: ambiguous (surface maps to >1 distinct canonical word)

CREATE TEMP TABLE _variant_pairs AS
WITH raw_tagged AS (
    -- Surface form → canonical word, with all applicable tags
    SELECT DISTINCT
        f.form_text                        AS surface,
        lower(e.word)                      AS canonical_word,
        tag.name                           AS tag_name
    FROM form_tags ft
    JOIN tokens tag ON tag.token_id = ft.token_id
      AND tag.name IN ('archaic', 'obsolete', 'dated', 'dialectal', 'poetic',
                       'informal', 'colloquial', 'slang')
    JOIN forms f   ON f.id = ft.form_id
    JOIN entries e ON e.id = f.entry_id
    WHERE f.form_text IS NOT NULL
      AND e.word IS NOT NULL
      AND lower(f.form_text) != lower(e.word)   -- exclude self-references
),
-- Only keep unambiguous surface forms (exactly one canonical word)
unambiguous AS (
    SELECT surface, max(canonical_word) AS canonical_word
    FROM raw_tagged
    GROUP BY surface
    HAVING count(DISTINCT canonical_word) = 1
),
-- Pick category: highest-priority tag per surface form
prioritised AS (
    SELECT
        u.surface,
        u.canonical_word,
        min(CASE tag_name
            WHEN 'dialectal'  THEN 1
            WHEN 'colloquial' THEN 2
            WHEN 'informal'   THEN 2
            WHEN 'slang'      THEN 2
            WHEN 'archaic'    THEN 3
            WHEN 'obsolete'   THEN 3
            WHEN 'dated'      THEN 3
            WHEN 'poetic'     THEN 4
            ELSE 5
        END) AS priority,
        (CASE min(CASE tag_name
            WHEN 'dialectal'  THEN 1
            WHEN 'colloquial' THEN 2
            WHEN 'informal'   THEN 2
            WHEN 'slang'      THEN 2
            WHEN 'archaic'    THEN 3
            WHEN 'obsolete'   THEN 3
            WHEN 'dated'      THEN 3
            WHEN 'poetic'     THEN 4
            ELSE 5
        END)
            WHEN 1 THEN 'dialect'
            WHEN 2 THEN 'casual'
            WHEN 3 THEN 'archaic'
            WHEN 4 THEN 'literary'
        END) AS category
    FROM unambiguous u
    JOIN raw_tagged rt ON rt.surface = u.surface
    GROUP BY u.surface, u.canonical_word
)
-- Join to tokens to get actual token_ids — both surface and canonical must exist
SELECT
    ts.token_id  AS surface_tid,
    tc.token_id  AS canonical_tid,
    p.surface,
    p.canonical_word,
    p.category
FROM prioritised p
JOIN tokens ts ON ts.name = p.surface        AND ts.ns LIKE 'AB%'
JOIN tokens tc ON tc.name = p.canonical_word AND tc.ns LIKE 'AB%'
WHERE ts.token_id != tc.token_id;   -- final self-reference guard

CREATE INDEX _idx_vp_surface_tid ON _variant_pairs (surface_tid);

-- ============================================================
-- Step 3: Apply category and canonical_id
-- ============================================================

UPDATE tokens t
SET
    category     = vp.category,
    canonical_id = vp.canonical_tid
FROM _variant_pairs vp
WHERE t.token_id = vp.surface_tid;

-- ============================================================
-- Step 4: Verify and report
-- ============================================================

DO $$
DECLARE
    r_archaic  INTEGER;
    r_dialect  INTEGER;
    r_casual   INTEGER;
    r_literary INTEGER;
    r_total    INTEGER;
    r_null_can INTEGER;
BEGIN
    SELECT count(*) INTO r_total    FROM tokens WHERE ns LIKE 'AB%' AND category IS NOT NULL;
    SELECT count(*) INTO r_archaic  FROM tokens WHERE ns LIKE 'AB%' AND category = 'archaic';
    SELECT count(*) INTO r_dialect  FROM tokens WHERE ns LIKE 'AB%' AND category = 'dialect';
    SELECT count(*) INTO r_casual   FROM tokens WHERE ns LIKE 'AB%' AND category = 'casual';
    SELECT count(*) INTO r_literary FROM tokens WHERE ns LIKE 'AB%' AND category = 'literary';
    SELECT count(*) INTO r_null_can
        FROM tokens WHERE ns LIKE 'AB%' AND category IS NOT NULL AND canonical_id IS NULL;

    RAISE NOTICE '023: % variant tokens annotated (archaic=%, dialect=%, casual=%, literary=%)',
        r_total, r_archaic, r_dialect, r_casual, r_literary;

    IF r_null_can > 0 THEN
        RAISE NOTICE '023: WARNING — % tokens have category but NULL canonical_id (multi-token deferred cases)',
            r_null_can;
    END IF;

    RAISE NOTICE '023: category+canonical_id populated from Wiktionary form_tags (unambiguous pairs only)';
    RAISE NOTICE '023: Ambiguous cases (71 forms) skipped — require linguist review';
    RAISE NOTICE '023: Multi-token forms (gonna, wanna, kinda) deferred — will go to var until decomposition design';
END $$;

DROP TABLE _variant_pairs;

COMMIT;

-- Post-commit optimization
ANALYZE tokens;

-- ============================================================
-- Post-migration chain fixes (applied after initial run)
-- ============================================================
-- The auto-population created two structural problems that were fixed
-- immediately after migration:
--
-- 1. CIRCULAR CHAINS (7 pairs, 14 tokens): Wiktionary tagged each form
--    as a variant of the other (e.g., betel↔beetle, travel↔travail,
--    shave↔shove). Both sides had canonical_id set to each other.
--    Fix: clear canonical_id and category on both sides. Flagged for
--    linguist review — these are genuinely ambiguous or Wiktionary errors.
--    Circular pairs cleared: betel/beetle, haught/haut, prejudical/prejudicial,
--    prophecy/prophesy, rencontre/rencounter, shave/shove, travel/travail.
--
-- 2. CASCADING CHAINS (62 tokens): variant → intermediate_variant → canonical.
--    The intermediate was itself tagged as a variant pointing elsewhere.
--    Fix: flatten by pointing directly to the terminal canonical (no canonical_id).
--
-- Both fixes applied in a single transaction:
--
-- BEGIN;
-- UPDATE tokens SET canonical_id = NULL, category = NULL
-- WHERE token_id IN (
--     SELECT t.token_id FROM tokens t
--     JOIN tokens tc ON tc.token_id = t.canonical_id
--     WHERE tc.canonical_id = t.token_id
-- );
-- WITH RECURSIVE chain AS (
--     SELECT token_id AS start_id, canonical_id AS next_id, canonical_id AS terminal_id
--     FROM tokens WHERE canonical_id IS NOT NULL AND ns LIKE 'AB%'
--     UNION ALL
--     SELECT c.start_id, t.canonical_id, t.canonical_id
--     FROM chain c JOIN tokens t ON t.token_id = c.next_id
--     WHERE t.canonical_id IS NOT NULL
-- ), terminals AS (
--     SELECT DISTINCT ON (start_id) start_id, terminal_id FROM chain c
--     WHERE NOT EXISTS (SELECT 1 FROM tokens t WHERE t.token_id = c.terminal_id AND t.canonical_id IS NOT NULL)
--     ORDER BY start_id
-- )
-- UPDATE tokens t SET canonical_id = tm.terminal_id
-- FROM terminals tm WHERE t.token_id = tm.start_id AND t.canonical_id != tm.terminal_id;
-- COMMIT;
--
-- Post-fix result: 0 chained variants, 0 circular references.
-- Final counts (after fix): archaic=5182, dialect=249, casual=151, literary=21 (total=5603)

-- ============================================================
-- Ambiguous forms (71) — for linguist review
-- ============================================================
-- These surface forms had >1 canonical candidate from Wiktionary.
-- Run this query manually to inspect and annotate manually:
--
-- WITH all_tagged AS (
--     SELECT DISTINCT tag.name AS tag_name, f.form_text AS surface, lower(e.word) AS canonical_word
--     FROM form_tags ft
--     JOIN tokens tag ON tag.token_id = ft.token_id
--       AND tag.name IN ('archaic','obsolete','dated','dialectal','poetic','informal','colloquial','slang')
--     JOIN forms f ON f.id = ft.form_id
--     JOIN entries e ON e.id = f.entry_id
--     WHERE f.form_text IS NOT NULL AND e.word IS NOT NULL
--       AND lower(f.form_text) != lower(e.word)
-- )
-- SELECT surface, array_agg(DISTINCT canonical_word ORDER BY canonical_word) AS candidates,
--        array_agg(DISTINCT tag_name ORDER BY tag_name) AS tags
-- FROM all_tagged
-- GROUP BY surface
-- HAVING count(DISTINCT canonical_word) > 1
-- ORDER BY surface;
--
-- To manually set a canonical_id after review:
-- UPDATE tokens SET category = 'archaic', canonical_id = (SELECT token_id FROM tokens WHERE name = 'chosen_canonical')
-- WHERE name = 'ambiguous_surface' AND ns LIKE 'AB%';
