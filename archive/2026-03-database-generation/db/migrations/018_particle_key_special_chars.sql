-- Migration 018: Add particle_key with special character bucketing
--
-- particle_key is used by the engine for PBD bed assignment — it determines
-- which persistent vocab bed a word lands in.
--
-- Default: first letter + character count (e.g., "hello" → "h5", "the" → "t3")
--
-- Exception: words containing ' or - get the special character + length instead.
-- This creates small, distinct buckets for apostrophe/hyphen words that need
-- priority processing for decomposition and the two-pass uppercase check.
--
-- Examples:
--   "don't"      → "'5"    (apostrophe + length)
--   "i'tikaf"    → "'7"
--   "well-known"  → "-10"   (hyphen + length)
--   "self-aware"  → "-10"
--   "hello"       → "h5"    (normal: first letter + length)
--
-- AC.AA structural tokens excluded (not words).

BEGIN;

-- Add the column with special-character-aware generation expression
ALTER TABLE tokens ADD COLUMN particle_key TEXT
    GENERATED ALWAYS AS (
        CASE
            WHEN name LIKE '%''%' THEN '''' || length(name)::text
            WHEN name LIKE '%-%'  THEN '-' || length(name)::text
            ELSE left(name, 1) || length(name)::text
        END
    ) STORED;

CREATE INDEX idx_tokens_particle_key ON tokens (particle_key);

-- Verify: all AB tokens have particle_key
DO $$
DECLARE
    null_count INTEGER;
BEGIN
    SELECT count(*) INTO null_count
    FROM tokens
    WHERE ns LIKE 'AB%' AND particle_key IS NULL;

    IF null_count > 0 THEN
        RAISE EXCEPTION '018: % AB tokens have NULL particle_key', null_count;
    END IF;
    RAISE NOTICE '018: All AB tokens have particle_key — OK';
END $$;

-- Stats: special character buckets
DO $$
DECLARE
    total_ab INTEGER;
    distinct_keys INTEGER;
    apos_count INTEGER;
    hyphen_count INTEGER;
BEGIN
    SELECT count(*), count(DISTINCT particle_key)
    INTO total_ab, distinct_keys
    FROM tokens WHERE ns LIKE 'AB%';

    SELECT count(*) INTO apos_count
    FROM tokens WHERE ns LIKE 'AB%' AND particle_key LIKE '''%';

    SELECT count(*) INTO hyphen_count
    FROM tokens WHERE ns LIKE 'AB%' AND particle_key LIKE '-%';

    RAISE NOTICE '018: % AB tokens, % distinct particle_keys', total_ab, distinct_keys;
    RAISE NOTICE '018: % apostrophe-bucketed, % hyphen-bucketed', apos_count, hyphen_count;
END $$;

COMMIT;

ANALYZE tokens;
