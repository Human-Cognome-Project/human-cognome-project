-- 004b: Import name-only tokens from hcp_names into hcp_english
-- Decision 002: Name-only words become regular tokens with PoS = label.
-- Capitalized = PRIMARY spelling, lowercase = misspelling variant.
--
-- Prerequisites:
--   - /tmp/hcp_names_export.csv must exist (exported by 004_names_merge.sh)
--   - Step 1 (004a) must have run already
--
-- Run against: hcp_english

-- ============================================================================
-- Base-50 helper: convert sequential integer to 2-char pair
-- ============================================================================
CREATE OR REPLACE FUNCTION base50_pair(seq INTEGER)
RETURNS TEXT AS $$
DECLARE
    alpha TEXT := 'ABCDEFGHIJKLMNPQRSTUVWXYZabcdefghijklmnpqrstuvwxyz';
BEGIN
    RETURN SUBSTRING(alpha FROM (seq / 50) + 1 FOR 1)
        || SUBSTRING(alpha FROM (seq % 50) + 1 FOR 1);
END;
$$ LANGUAGE plpgsql IMMUTABLE STRICT;

-- ============================================================================
-- Load exported names into staging table (outside transaction for \COPY)
-- ============================================================================
CREATE TEMP TABLE imported_names (name TEXT NOT NULL);
\COPY imported_names FROM '/tmp/hcp_names_export.csv' CSV

-- ============================================================================
-- Main migration in a transaction
-- ============================================================================
BEGIN;

-- Filter to name-only tokens (no lowercase match in hcp_english entries)
CREATE TEMP TABLE name_only AS
SELECT DISTINCT i.name
FROM imported_names i
WHERE NOT EXISTS (
    SELECT 1 FROM entries e
    WHERE e.word = LOWER(i.name)
)
AND i.name IS NOT NULL
AND LENGTH(TRIM(i.name)) > 0;

-- Find next available sequential position in AB.AB.CA
-- Token position = p4_seq * 2500 + p5_seq where each pair decodes from base-50
CREATE TEMP TABLE next_position AS
SELECT
    (
        (POSITION(LEFT(t.p4, 1) IN 'ABCDEFGHIJKLMNPQRSTUVWXYZabcdefghijklmnpqrstuvwxyz') - 1) * 50
        + (POSITION(RIGHT(t.p4, 1) IN 'ABCDEFGHIJKLMNPQRSTUVWXYZabcdefghijklmnpqrstuvwxyz') - 1)
    ) * 2500
    + (POSITION(LEFT(t.p5, 1) IN 'ABCDEFGHIJKLMNPQRSTUVWXYZabcdefghijklmnpqrstuvwxyz') - 1) * 50
    + (POSITION(RIGHT(t.p5, 1) IN 'ABCDEFGHIJKLMNPQRSTUVWXYZabcdefghijklmnpqrstuvwxyz') - 1)
    + 1 AS start_seq
FROM tokens t
WHERE t.ns = 'AB' AND t.p2 = 'AB' AND t.p3 = 'CA'
ORDER BY t.p4 DESC, t.p5 DESC
LIMIT 1;

-- Assign sequential token IDs to name-only tokens
CREATE TEMP TABLE name_tokens AS
SELECT
    n.name,
    (SELECT start_seq FROM next_position) + ROW_NUMBER() OVER (ORDER BY n.name) - 1 AS seq
FROM name_only n;

-- ============================================================================
-- Insert tokens, entries, forms — all inside one DO block for GET DIAGNOSTICS
-- ============================================================================
DO $$
DECLARE
    name_count BIGINT;
    start_s BIGINT;
    tok_cnt BIGINT;
    ent_cnt BIGINT;
    frm_cnt BIGINT;
    t_total BIGINT;
    e_total BIGINT;
    f_total BIGINT;
    l_total BIGINT;
    min_p4 TEXT; min_p5 TEXT; max_p4 TEXT; max_p5 TEXT;
    min_s BIGINT; max_s BIGINT;
BEGIN
    -- Report counts
    SELECT COUNT(*) INTO name_count FROM name_only;
    SELECT start_seq INTO start_s FROM next_position;
    RAISE NOTICE 'Name-only tokens to insert: %', name_count;
    RAISE NOTICE 'Starting at CA position: %', start_s;

    -- Show address range
    SELECT MIN(seq), MAX(seq) INTO min_s, max_s FROM name_tokens;
    IF min_s IS NOT NULL THEN
        min_p4 := base50_pair((min_s / 2500)::INTEGER);
        min_p5 := base50_pair((min_s % 2500)::INTEGER);
        max_p4 := base50_pair((max_s / 2500)::INTEGER);
        max_p5 := base50_pair((max_s % 2500)::INTEGER);
        RAISE NOTICE 'Address range: AB.AB.CA.%.% through AB.AB.CA.%.%',
            min_p4, min_p5, max_p4, max_p5;
    END IF;

    -- Insert tokens
    INSERT INTO tokens (ns, p2, p3, p4, p5, name, layer, subcategory, metadata)
    SELECT
        'AB', 'AB', 'CA',
        base50_pair((nt.seq / 2500)::INTEGER),
        base50_pair((nt.seq % 2500)::INTEGER),
        nt.name,
        'C',
        'label',
        '{"source": "hcp_names_merge"}'::JSONB
    FROM name_tokens nt;
    GET DIAGNOSTICS tok_cnt = ROW_COUNT;
    RAISE NOTICE 'Tokens inserted: %', tok_cnt;

    -- Insert entries (one per name-only token, PoS = label)
    INSERT INTO entries (word_token, pos_token, word)
    SELECT
        'AB.AB.CA.'
            || base50_pair((nt.seq / 2500)::INTEGER) || '.'
            || base50_pair((nt.seq % 2500)::INTEGER),
        'AB.AB.CA.DB.Ek',   -- "label" PoS token
        nt.name
    FROM name_tokens nt;
    GET DIAGNOSTICS ent_cnt = ROW_COUNT;
    RAISE NOTICE 'Entries inserted: %', ent_cnt;

    -- Insert misspelling forms (lowercase variant for uppercase-starting names)
    INSERT INTO forms (entry_id, form_text, tag_tokens)
    SELECT
        e.id,
        LOWER(e.word),
        ARRAY['AB.AB.CA.Da.fI']::TEXT[]  -- "misspelling" tag token
    FROM entries e
    INNER JOIN name_tokens nt
        ON e.word_token = 'AB.AB.CA.'
            || base50_pair((nt.seq / 2500)::INTEGER) || '.'
            || base50_pair((nt.seq % 2500)::INTEGER)
    WHERE LOWER(e.word) <> e.word  -- only if lowering changes it
      AND LENGTH(TRIM(e.word)) > 0;
    GET DIAGNOSTICS frm_cnt = ROW_COUNT;
    RAISE NOTICE 'Misspelling forms inserted: %', frm_cnt;

    -- Final totals
    SELECT COUNT(*) INTO t_total FROM tokens;
    SELECT COUNT(*) INTO e_total FROM entries;
    SELECT COUNT(*) INTO f_total FROM forms;
    SELECT COUNT(*) INTO l_total FROM entries WHERE pos_token = 'AB.AB.CA.DB.Ek';

    RAISE NOTICE '=== Final counts ===';
    RAISE NOTICE 'Tokens:  %', t_total;
    RAISE NOTICE 'Entries: %', e_total;
    RAISE NOTICE 'Forms:   %', f_total;
    RAISE NOTICE 'Label entries: %', l_total;
END;
$$;

COMMIT;

-- Cleanup helper function
DROP FUNCTION IF EXISTS base50_pair(INTEGER);
