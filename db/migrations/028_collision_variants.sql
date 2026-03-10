-- Migration 028: Wire uppercase collision entries as variants of lowercase canonicals
--
-- Per Patrick's rule: when an uppercase entry (e.g. "Apaches") has a lowercase
-- counterpart ("apaches"), the uppercase form is a variant of the lowercase canonical:
--   - uppercase entry: canonical_id → lowercase token_id, proper_common = 'proper'
--   - lowercase entry: proper_common = 'proper' (it IS a proper noun)
--
-- This migration handles only the UNAMBIGUOUS case: uppercase entries with
-- exactly one lowercase counterpart, where that lowercase entry is itself unique.
-- Pre-computed via CTEs (no correlated subqueries — set-based for performance).
--
-- Deferred (438 cases): uppercase entries with 2+ lowercase counterparts.
-- Deferred (45,444 exact name duplicate groups): multi-role vocabulary entries
-- (same surface form, different layer/subcategory). These coexist legitimately —
-- e.g. "Ah" as affix/prefix AND as word/noun. Need Patrick's guidance on whether
-- a primary canonical should be designated or all roles coexist independently.
--
-- Also updates the Label tier envelope query to add `AND canonical_id IS NULL`
-- to prevent variant entries from loading wrong token_ids into env_vocab.

\connect hcp_english

-- Pre-compute: uppercase entries that have exactly one lowercase counterpart,
-- where the lowercase entry is itself unique (no lowercase name duplicates)
CREATE TEMP TABLE _collision_pairs AS
WITH uc_lc AS (
    -- All uppercase → lowercase pairs
    SELECT
        t1.token_id AS upper_id,
        t2.token_id AS lower_id,
        t2.name     AS lower_name
    FROM tokens t1
    JOIN tokens t2
      ON t2.name = lower(t1.name)
     AND t2.token_id != t1.token_id
    WHERE t1.name != lower(t1.name)
      AND t1.canonical_id IS NULL
),
uc_counts AS (
    -- Only uppercase entries with exactly one lowercase match
    SELECT upper_id, min(lower_id) AS lower_id
    FROM uc_lc
    GROUP BY upper_id
    HAVING count(*) = 1
),
lc_unique AS (
    -- Only lowercase entries that are themselves unique (no lowercase name dup)
    SELECT token_id
    FROM tokens
    WHERE name = lower(name)
    GROUP BY token_id
    HAVING count(*) = 1  -- always 1 (token_id is PK), so rewrite:
),
lc_name_unique AS (
    -- Lowercase names that appear exactly once
    SELECT name
    FROM tokens
    WHERE name = lower(name)
    GROUP BY name
    HAVING count(*) = 1
)
SELECT uc.upper_id, uc.lower_id
FROM uc_counts uc
JOIN tokens lc ON lc.token_id = uc.lower_id
JOIN lc_name_unique lnu ON lnu.name = lc.name;

-- How many unambiguous pairs found?
SELECT count(*) AS unambiguous_pairs FROM _collision_pairs;

-- Step 1: Tag lowercase canonicals as proper_common = 'proper'
UPDATE tokens
SET proper_common = 'proper'
WHERE token_id IN (SELECT lower_id FROM _collision_pairs)
  AND proper_common IS NULL;

-- Step 2: Wire uppercase entries as variants
UPDATE tokens
SET canonical_id  = cp.lower_id,
    proper_common = 'proper'
FROM _collision_pairs cp
WHERE tokens.token_id = cp.upper_id;

DROP TABLE _collision_pairs;

-- Verify
SELECT
    count(*) FILTER (WHERE canonical_id IS NOT NULL AND name != lower(name))
        AS uppercase_variants_wired,
    count(*) FILTER (WHERE proper_common = 'proper' AND canonical_id IS NULL)
        AS canonical_proper_nouns,
    count(*) FILTER (WHERE proper_common = 'proper' AND canonical_id IS NOT NULL)
        AS variant_proper_nouns
FROM tokens;

-- ============================================================

\connect hcp_core

-- Update Label tier envelope query: add `AND canonical_id IS NULL`
-- Variants resolve to canonical at lookup (engine lowercases and finds canonical
-- directly); loading a variant's token_id into env_vocab maps the word to the
-- wrong token_id.
UPDATE envelope_queries
SET query_text =
    'SELECT lower(name) AS name, token_id '
    'FROM tokens '
    'WHERE proper_common = ''proper'' '
    '  AND canonical_id IS NULL '
    '  AND length(name) BETWEEN 2 AND 16 '
    'ORDER BY freq_rank ASC NULLS LAST'
WHERE envelope_id = (
    SELECT id FROM envelope_definitions WHERE name = 'english_common_10k'
)
AND priority = 1;

-- Verify envelope
SELECT priority, lmdb_subdb, left(query_text, 180) AS query_preview
FROM envelope_queries
WHERE envelope_id = (SELECT id FROM envelope_definitions WHERE name = 'english_common_10k')
ORDER BY priority;
