-- HCP Migration 002: hcp_english — Token ID decomposition
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
-- Prerequisite: 000_helpers.sql installed in hcp_english

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
    name         TEXT NOT NULL,
    layer        TEXT,
    subcategory  TEXT,
    atomization  JSONB DEFAULT '[]',
    metadata     JSONB DEFAULT '{}',

    -- NSM fields (present for tokens that are also NSM primitives)
    nsm_canonical_order INTEGER,
    nsm_lesson_number   VARCHAR(10),
    nsm_is_universal    BOOLEAN DEFAULT false,

    PRIMARY KEY (token_id)
);

-- B-tree compound index: massive prefix compression for AB.AB.* tokens.
-- ~1.16M English tokens share ns='AB', p2='AB'; the B-tree stores this
-- prefix once in upper tree nodes rather than per-leaf.
CREATE INDEX idx_tokens_prefix    ON tokens(ns, p2, p3, p4, p5);
CREATE INDEX idx_tokens_ns        ON tokens(ns);
CREATE INDEX idx_tokens_ns_p2     ON tokens(ns, p2);
CREATE INDEX idx_tokens_ns_p2_p3  ON tokens(ns, p2, p3);

-- Data lookups
CREATE INDEX idx_tokens_name  ON tokens(name);
CREATE INDEX idx_tokens_layer ON tokens(layer);

-- ================================================================
-- MIGRATE TOKEN DATA
-- ================================================================

INSERT INTO tokens (
    ns, p2, p3, p4, p5,
    name, layer, subcategory, atomization, metadata,
    nsm_canonical_order, nsm_lesson_number, nsm_is_universal
)
SELECT
    s.ns, s.p2, s.p3, s.p4, s.p5,
    t.name, t.layer, t.subcategory, t.atomization, t.metadata,
    t.nsm_canonical_order, t.nsm_lesson_number, t.nsm_is_universal
FROM tokens_backup t
CROSS JOIN LATERAL split_token_id(t.token_id) s;

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
        RAISE EXCEPTION 'hcp_english: token count mismatch (% old → % new)', old_count, new_count;
    END IF;

    SELECT COUNT(*) INTO match_count
    FROM tokens t
    JOIN tokens_backup b ON t.token_id = b.token_id;

    IF match_count != new_count THEN
        RAISE EXCEPTION 'hcp_english: token_id reconstruction mismatch (% of % matched)', match_count, new_count;
    END IF;

    RAISE NOTICE 'hcp_english: % tokens migrated, all token_ids verified', new_count;
    RAISE NOTICE 'hcp_english: AB.AB namespace: % tokens', (
        SELECT COUNT(*) FROM tokens WHERE ns = 'AB' AND p2 = 'AB'
    );
END $$;

COMMIT;

-- Post-commit optimization
ANALYZE tokens;
