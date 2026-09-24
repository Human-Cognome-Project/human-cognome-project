-- HCP Migration 001: hcp_core — Token ID decomposition
--
-- Changes:
--   1. tokens: monolithic TEXT PK → decomposed CHAR(2) columns + generated TEXT PK
--   2. Remove pbm_entries (core stores atomizations, not PBMs)
--   3. Remove scopes (associated with pbm_entries)
--   4. Add shard_registry (canonical namespace → database routing)
--
-- Preserved unchanged:
--   - namespace_allocations (uses pattern TEXT, not token_ids)
--   - metadata (references token_id TEXT — still compatible)
--
-- Prerequisite: 000_helpers.sql installed in hcp_core

BEGIN;

-- ================================================================
-- BACKUP
-- ================================================================

CREATE TEMP TABLE tokens_backup AS SELECT * FROM tokens;

-- ================================================================
-- REMOVE PBM TABLES
-- ================================================================
-- Core shard holds universal translation elements (byte codes, NSM
-- primitives, structural tokens) and their atomizations. PBMs for
-- these elements will be added later from properly sourced data.

DROP TABLE IF EXISTS pbm_entries CASCADE;
DROP TABLE IF EXISTS scopes CASCADE;

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
    category    TEXT,
    subcategory TEXT,
    metadata    JSONB DEFAULT '{}',

    PRIMARY KEY (token_id)
);

-- B-tree compound index: prefix compression on shared namespaces.
-- Millions of AA.AA.* tokens share the prefix; B-tree internal nodes
-- compress this hierarchy automatically.
CREATE INDEX idx_tokens_prefix ON tokens(ns, p2, p3, p4, p5);

-- Single- and two-level indexes for common query patterns
CREATE INDEX idx_tokens_ns     ON tokens(ns);
CREATE INDEX idx_tokens_ns_p2  ON tokens(ns, p2);

-- Data lookups
CREATE INDEX idx_tokens_name     ON tokens(name);
CREATE INDEX idx_tokens_category ON tokens(category);

-- ================================================================
-- MIGRATE TOKEN DATA
-- ================================================================

INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory, metadata)
SELECT
    s.ns, s.p2, s.p3, s.p4, s.p5,
    t.name, t.category, t.subcategory, t.metadata
FROM tokens_backup t
CROSS JOIN LATERAL split_token_id(t.token_id) s;

-- ================================================================
-- SHARD REGISTRY
-- ================================================================
-- Canonical source of truth for namespace → database routing.
-- Application code and DB tooling both read from here.

CREATE TABLE IF NOT EXISTS shard_registry (
    ns_prefix   TEXT PRIMARY KEY,
    shard_db    TEXT NOT NULL,
    description TEXT,
    active      BOOLEAN DEFAULT true,
    created_at  TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

COMMENT ON TABLE shard_registry IS
    'Canonical namespace-to-database routing. Read by application layer and DB tooling.';

INSERT INTO shard_registry (ns_prefix, shard_db, description, active) VALUES
    ('AA', 'hcp_core',    'Universal/computational: byte codes, NSM primitives, structural tokens', true),
    ('AB', 'hcp_english', 'Text mode: English language family', true),
    ('yA', 'hcp_names',   'Name components (cross-linguistic)', true),
    ('zA', 'hcp_en_pbm',  'Source PBMs and documents (experimental)', true)
ON CONFLICT (ns_prefix) DO UPDATE SET
    shard_db    = EXCLUDED.shard_db,
    description = EXCLUDED.description,
    active      = EXCLUDED.active;

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
        RAISE EXCEPTION 'hcp_core: token count mismatch (% old → % new)', old_count, new_count;
    END IF;

    -- Verify every generated token_id matches its original
    SELECT COUNT(*) INTO match_count
    FROM tokens t
    JOIN tokens_backup b ON t.token_id = b.token_id;

    IF match_count != new_count THEN
        RAISE EXCEPTION 'hcp_core: token_id reconstruction mismatch (% of % matched)', match_count, new_count;
    END IF;

    RAISE NOTICE 'hcp_core: % tokens migrated, all token_ids verified', new_count;
END $$;

COMMIT;

-- Post-commit optimization
ANALYZE tokens;
ANALYZE shard_registry;
