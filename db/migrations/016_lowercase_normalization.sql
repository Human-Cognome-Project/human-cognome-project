-- HCP Migration 016: Lowercase normalization + particle key
-- Target: hcp_english (primary), hcp_fic_entities, hcp_nf_entities (name cleanup)
-- Depends on: 001 (tokens schema)
--
-- The engine resolves all input to lowercase canonical forms. Capitalization
-- is positional (sentence-initial) or Label-based (proper nouns) — both
-- reconstructable without storing case variants.
--
-- hcp_english has 297,666 tokens with uppercase characters. Of those:
--   - 278,549 are true Labels (no lowercase collision partner) — just rename
--   - 59,756 collision groups where uppercase + lowercase variants coexist
--     - 12,624 groups already have a lowercase entry (delete uppercase dupes)
--     - 47,132 groups need a survivor renamed to lowercase (delete rest)
--   - 0 entity-referenced tokens have collisions (safe)
--
-- Also adds particle_key: a generated column concatenating first letter +
-- character count. Pre-calculated so the engine skips per-lookup computation
-- during PBD assembly. Single field, fewer buckets for analysis.
--
-- AC.AA namespace (40 structural tokens) excluded — not words.

BEGIN;

-- ============================================================================
-- Step 1: Add particle_key generated column
-- ============================================================================
-- First letter + character count of the surface form, concatenated.
-- Examples: "hello" → 'h5', "the" → 't3', "a" → 'a1'
-- Engine uses this for fast candidate narrowing during PBD assembly.

ALTER TABLE tokens ADD COLUMN particle_key TEXT
    GENERATED ALWAYS AS (left(name, 1) || length(name)::text) STORED;

CREATE INDEX idx_tokens_particle_key ON tokens (particle_key);

-- ============================================================================
-- Step 2: Build dedup mapping for collision groups
-- ============================================================================
-- A collision group is a set of tokens in AB.* where multiple entries exist
-- for the same word in different capitalizations.
--
-- For each group, pick a survivor:
--   - If a lowercase entry already exists → that token_id survives
--   - If no lowercase entry → lowest serial token_id survives, gets renamed
--
-- We use a temp table so the dedup logic runs once, then Steps 3-4 reference it.

CREATE TEMP TABLE _dedup_map AS
WITH collision_groups AS (
    -- Find all lowercase names that have multiple tokens (mixed case)
    SELECT lower(name) AS lname
    FROM tokens
    WHERE ns LIKE 'AB%'
    GROUP BY lower(name)
    HAVING count(*) > 1
),
-- For each collision group, find if a lowercase entry exists
group_survivors AS (
    SELECT
        cg.lname,
        -- Survivor: prefer existing lowercase entry, else lowest token_id
        COALESCE(
            (SELECT token_id FROM tokens
             WHERE ns LIKE 'AB%' AND name = cg.lname
             ORDER BY token_id LIMIT 1),
            (SELECT token_id FROM tokens
             WHERE ns LIKE 'AB%' AND lower(name) = cg.lname
             ORDER BY token_id LIMIT 1)
        ) AS survivor_id,
        -- Does a proper lowercase entry already exist?
        EXISTS (
            SELECT 1 FROM tokens
            WHERE ns LIKE 'AB%' AND name = cg.lname
        ) AS has_lowercase
    FROM collision_groups cg
)
SELECT * FROM group_survivors;

-- Index for fast lookups in subsequent steps
CREATE INDEX _idx_dedup_lname ON _dedup_map (lname);
CREATE INDEX _idx_dedup_survivor ON _dedup_map (survivor_id);

-- ============================================================================
-- Step 3: Delete retired duplicates
-- ============================================================================
-- For each collision group: delete every token that isn't the survivor.

DELETE FROM tokens
WHERE ns LIKE 'AB%'
  AND token_id IN (
    SELECT t.token_id
    FROM tokens t
    JOIN _dedup_map d ON lower(t.name) = d.lname
    WHERE t.token_id != d.survivor_id
      AND t.ns LIKE 'AB%'
  );

-- ============================================================================
-- Step 4: Rename survivors that need lowercasing
-- ============================================================================
-- Survivors from groups without an existing lowercase entry still have
-- their original mixed-case name. Rename them.

UPDATE tokens
SET name = lower(name)
WHERE ns LIKE 'AB%'
  AND token_id IN (
    SELECT survivor_id FROM _dedup_map WHERE NOT has_lowercase
  );

-- ============================================================================
-- Step 5: Lowercase all remaining AB.* names
-- ============================================================================
-- The 278,549 true Labels (no collision partner) plus any stragglers.
-- Safe because collision groups are already resolved.

UPDATE tokens
SET name = lower(name)
WHERE ns LIKE 'AB%'
  AND name != lower(name);

-- ============================================================================
-- Step 6: Lowercase entity DB token names
-- ============================================================================
-- Entity DB tokens tables have mixed formatting from the librarian's
-- inconsistent entries. Lowercase for consistency. Entity_names label
-- token references are unaffected (0 collisions found in analysis).
--
-- NOTE: This migration runs on hcp_english. Steps 6a and 6b are
-- separate statements to run on the entity databases.
-- Uncomment and run on the appropriate database, or run separately:
--
-- === Run on hcp_fic_entities ===
-- UPDATE tokens SET name = lower(name) WHERE name != lower(name);
--
-- === Run on hcp_nf_entities ===
-- UPDATE tokens SET name = lower(name) WHERE name != lower(name);

-- ============================================================================
-- Step 7: Verify
-- ============================================================================

-- No uppercase in AB.* namespace
DO $$
DECLARE
    upper_count INTEGER;
BEGIN
    SELECT count(*) INTO upper_count
    FROM tokens
    WHERE ns LIKE 'AB%' AND name ~ '[A-Z]';

    IF upper_count > 0 THEN
        RAISE EXCEPTION '016: % AB tokens still have uppercase characters', upper_count;
    END IF;
    RAISE NOTICE '016: No uppercase AB tokens remaining — OK';
END $$;

-- No duplicate lowercase names in AB.*
DO $$
DECLARE
    dupe_count INTEGER;
BEGIN
    SELECT count(*) INTO dupe_count
    FROM (
        SELECT name
        FROM tokens
        WHERE ns LIKE 'AB%'
        GROUP BY name
        HAVING count(*) > 1
    ) dupes;

    IF dupe_count > 0 THEN
        RAISE EXCEPTION '016: % duplicate lowercase names found in AB', dupe_count;
    END IF;
    RAISE NOTICE '016: No duplicate names in AB — OK';
END $$;

-- particle_key populated on all AB rows
DO $$
DECLARE
    null_count INTEGER;
BEGIN
    SELECT count(*) INTO null_count
    FROM tokens
    WHERE ns LIKE 'AB%' AND particle_key IS NULL;

    IF null_count > 0 THEN
        RAISE EXCEPTION '016: % AB tokens have NULL particle_key', null_count;
    END IF;
    RAISE NOTICE '016: All AB tokens have particle_key — OK';
END $$;

-- Summary stats
DO $$
DECLARE
    total_ab INTEGER;
    distinct_keys INTEGER;
BEGIN
    SELECT count(*), count(DISTINCT particle_key)
    INTO total_ab, distinct_keys
    FROM tokens WHERE ns LIKE 'AB%';

    RAISE NOTICE '016: % AB tokens, % distinct particle_keys', total_ab, distinct_keys;
END $$;

-- Clean up
DROP TABLE _dedup_map;

COMMIT;

-- Post-commit optimization
ANALYZE tokens;
