-- HCP Migration 003: hcp_names — Token ID decomposition
--
-- Changes:
--   1. tokens: monolithic TEXT PK → decomposed CHAR(2) columns + generated TEXT PK
--
-- Preserved unchanged:
--   - entries (word_token, pos_token, etymology_tokens reference token_id TEXT)
--   - forms (form_token, tag_tokens, form_tokens reference token_id TEXT)
--   - relations (relation_token, target_token, tag_tokens reference token_id TEXT)
--   - senses (gloss_tokens, tag_tokens reference token_id TEXT)
--   All remain compatible since PK stays TEXT.
--
-- Note: Name component tokens are 3-pair (yA.XX.XX), not 5-pair.
-- The decomposed schema handles this naturally: p4 and p5 will be NULL.
--
-- Prerequisite: 000_helpers.sql installed in hcp_names

BEGIN;

-- ================================================================
-- BACKUP
-- ================================================================

CREATE TEMP TABLE tokens_backup AS SELECT * FROM tokens;

-- ================================================================
-- RECREATE TOKENS WITH DECOMPOSED STRUCTURE
-- ================================================================

DROP TABLE IF EXISTS tokens CASCADE;

CREATE TABLE tokens (
    -- Decomposed token address (base-50 pairs, always exactly 2 chars)
    ns   TEXT NOT NULL,
    p2   TEXT,
    p3   TEXT,
    p4   TEXT,
    p5   TEXT,

    -- Generated PK: single-column for clean FKs, JOINs, LMDB export
    -- Uses || and COALESCE (IMMUTABLE) instead of concat_ws (STABLE)
    token_id TEXT GENERATED ALWAYS AS (
        ns ||
        COALESCE('.' || p2, '') ||
        COALESCE('.' || p3, '') ||
        COALESCE('.' || p4, '') ||
        COALESCE('.' || p5, '')
    ) STORED NOT NULL,

    -- Token data
    name        TEXT NOT NULL,
    atomization JSONB DEFAULT '[]',
    metadata    JSONB DEFAULT '{}',

    PRIMARY KEY (token_id)
);

-- B-tree compound index: prefix compression for yA.* tokens.
-- All ~150K name components share ns='yA'; B-tree compresses this.
CREATE INDEX idx_tokens_prefix   ON tokens(ns, p2, p3, p4, p5);
CREATE INDEX idx_tokens_ns       ON tokens(ns);
CREATE INDEX idx_tokens_ns_p2    ON tokens(ns, p2);

-- Data lookups
CREATE INDEX idx_tokens_name ON tokens(name);

-- ================================================================
-- MIGRATE TOKEN DATA
-- ================================================================

INSERT INTO tokens (ns, p2, p3, p4, p5, name, atomization, metadata)
SELECT
    s.ns, s.p2, s.p3, s.p4, s.p5,
    t.name, t.atomization, t.metadata
FROM tokens_backup t
CROSS JOIN LATERAL split_token_id(t.token_id) s;

-- ================================================================
-- RESTORE CASCADED FOREIGN KEYS
-- ================================================================
-- DROP TABLE tokens CASCADE removes entries.word_token FK.
-- Restore it now that the new tokens table has the same TEXT PK.

ALTER TABLE entries
    ADD CONSTRAINT entries_word_token_fkey
    FOREIGN KEY (word_token) REFERENCES tokens(token_id);

-- ================================================================
-- VERIFY
-- ================================================================

DO $$
DECLARE
    old_count   INTEGER;
    new_count   INTEGER;
    match_count INTEGER;
BEGIN
    SELECT COUNT(*) INTO old_count FROM tokens_backup;
    SELECT COUNT(*) INTO new_count FROM tokens;

    IF old_count != new_count THEN
        RAISE EXCEPTION 'hcp_names: token count mismatch (% old → % new)', old_count, new_count;
    END IF;

    SELECT COUNT(*) INTO match_count
    FROM tokens t
    JOIN tokens_backup b ON t.token_id = b.token_id;

    IF match_count != new_count THEN
        RAISE EXCEPTION 'hcp_names: token_id reconstruction mismatch (% of % matched)', match_count, new_count;
    END IF;

    RAISE NOTICE 'hcp_names: % tokens migrated, all token_ids verified', new_count;
    RAISE NOTICE 'hcp_names: yA namespace: % tokens', (
        SELECT COUNT(*) FROM tokens WHERE ns = 'yA'
    );
END $$;

COMMIT;

-- Post-commit optimization
ANALYZE tokens;
