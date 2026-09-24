-- HCP Migration 016 companion: Lowercase entity DB token names
-- Target: hcp_fic_entities AND hcp_nf_entities (run on both)
-- Depends on: 016_lowercase_normalization (conceptually)
--
-- Entity DB tokens.name fields have mixed formatting from the librarian's
-- inconsistent entries (snake_case early, display strings later).
-- Lowercase all for consistency.
--
-- Entity_names label token references (cross-shard to hcp_english) are
-- unaffected — 0 collisions found in analysis. Token_ids don't change.
--
-- Also adds particle_key generated column to entity tokens for consistency.

BEGIN;

-- Add particle_key (same definition as hcp_english)
ALTER TABLE tokens ADD COLUMN IF NOT EXISTS particle_key TEXT
    GENERATED ALWAYS AS (left(name, 1) || length(name)::text) STORED;

-- Lowercase all token names
UPDATE tokens SET name = lower(name) WHERE name != lower(name);

-- Verify
DO $$
DECLARE
    upper_count INTEGER;
BEGIN
    SELECT count(*) INTO upper_count
    FROM tokens WHERE name ~ '[A-Z]';

    IF upper_count > 0 THEN
        RAISE EXCEPTION '016-entity: % tokens still have uppercase', upper_count;
    END IF;
    RAISE NOTICE '016-entity: All token names lowercased — OK';
END $$;

COMMIT;

ANALYZE tokens;
