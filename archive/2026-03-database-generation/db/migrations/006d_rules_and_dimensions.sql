-- HCP Migration 006d: Rule Tables + Token Dimensions (hcp_english)
--
-- Creates:
--   1. ordering_rules table (~8-10 rows)
--   2. movement_rules table (~5 rows)
--   3. repair_rules table (~4 rows)
--   4. Token dimension columns (count_mass, proper_common, aux_type)
--
-- Rules are small, hand-populated tables encoding English-specific
-- force constants. Values are initial estimates — calibration happens
-- when PBM corpus data is available.
--
-- Prerequisite: 006a (force infrastructure tokens in hcp_core)

BEGIN;

-- ================================================================
-- ORDERING RULES
-- ================================================================
-- English-specific linear precedence constraints.
-- Each rule references an LoD level from AA.AC.AB.AC (hcp_core).

CREATE TABLE ordering_rules (
    -- Rule identifier (decomposed token — these become AB tokens)
    rule_ns TEXT NOT NULL, rule_p2 TEXT, rule_p3 TEXT, rule_p4 TEXT, rule_p5 TEXT,
    rule_token TEXT GENERATED ALWAYS AS (
        rule_ns ||
        COALESCE('.' || rule_p2, '') ||
        COALESCE('.' || rule_p3, '') ||
        COALESCE('.' || rule_p4, '') ||
        COALESCE('.' || rule_p5, '')
    ) STORED NOT NULL,

    -- LoD level (decomposed reference to AA.AC.AB.AC.* token)
    lod_ns TEXT NOT NULL, lod_p2 TEXT, lod_p3 TEXT, lod_p4 TEXT, lod_p5 TEXT,
    lod_token TEXT GENERATED ALWAYS AS (
        lod_ns ||
        COALESCE('.' || lod_p2, '') ||
        COALESCE('.' || lod_p3, '') ||
        COALESCE('.' || lod_p4, '') ||
        COALESCE('.' || lod_p5, '')
    ) STORED NOT NULL,

    is_absolute  BOOLEAN NOT NULL DEFAULT false,  -- true = hard constraint, false = preference
    strength     REAL NOT NULL DEFAULT 0.5,        -- 0.0-1.0, higher = stronger preference
    rule_def     TEXT,                             -- human-readable description

    PRIMARY KEY (rule_ns, rule_p2, rule_p3, rule_p4, rule_p5)
);

CREATE INDEX idx_ordering_rules_lod ON ordering_rules(lod_ns, lod_p2, lod_p3, lod_p4, lod_p5);

-- ================================================================
-- MOVEMENT RULES
-- ================================================================
-- English-specific displacement forces (wh-movement, topicalization, etc.)

CREATE TABLE movement_rules (
    rule_ns TEXT NOT NULL, rule_p2 TEXT, rule_p3 TEXT, rule_p4 TEXT, rule_p5 TEXT,
    rule_token TEXT GENERATED ALWAYS AS (
        rule_ns ||
        COALESCE('.' || rule_p2, '') ||
        COALESCE('.' || rule_p3, '') ||
        COALESCE('.' || rule_p4, '') ||
        COALESCE('.' || rule_p5, '')
    ) STORED NOT NULL,

    lod_ns TEXT NOT NULL, lod_p2 TEXT, lod_p3 TEXT, lod_p4 TEXT, lod_p5 TEXT,
    lod_token TEXT GENERATED ALWAYS AS (
        lod_ns ||
        COALESCE('.' || lod_p2, '') ||
        COALESCE('.' || lod_p3, '') ||
        COALESCE('.' || lod_p4, '') ||
        COALESCE('.' || lod_p5, '')
    ) STORED NOT NULL,

    is_obligatory BOOLEAN NOT NULL DEFAULT false,  -- must move vs. may move
    strength      REAL NOT NULL DEFAULT 0.5,
    rule_def      TEXT,
    trigger_cond  TEXT,                             -- what triggers this movement

    PRIMARY KEY (rule_ns, rule_p2, rule_p3, rule_p4, rule_p5)
);

CREATE INDEX idx_movement_rules_lod ON movement_rules(lod_ns, lod_p2, lod_p3, lod_p4, lod_p5);

-- ================================================================
-- REPAIR RULES
-- ================================================================
-- English-specific error correction and structural recovery.

CREATE TABLE repair_rules (
    rule_ns TEXT NOT NULL, rule_p2 TEXT, rule_p3 TEXT, rule_p4 TEXT, rule_p5 TEXT,
    rule_token TEXT GENERATED ALWAYS AS (
        rule_ns ||
        COALESCE('.' || rule_p2, '') ||
        COALESCE('.' || rule_p3, '') ||
        COALESCE('.' || rule_p4, '') ||
        COALESCE('.' || rule_p5, '')
    ) STORED NOT NULL,

    lod_ns TEXT NOT NULL, lod_p2 TEXT, lod_p3 TEXT, lod_p4 TEXT, lod_p5 TEXT,
    lod_token TEXT GENERATED ALWAYS AS (
        lod_ns ||
        COALESCE('.' || lod_p2, '') ||
        COALESCE('.' || lod_p3, '') ||
        COALESCE('.' || lod_p4, '') ||
        COALESCE('.' || lod_p5, '')
    ) STORED NOT NULL,

    priority      SMALLINT NOT NULL DEFAULT 5,     -- 1=highest priority, 10=lowest
    strength      REAL NOT NULL DEFAULT 0.5,
    rule_def      TEXT,
    trigger_cond  TEXT,

    PRIMARY KEY (rule_ns, rule_p2, rule_p3, rule_p4, rule_p5)
);

CREATE INDEX idx_repair_rules_lod ON repair_rules(lod_ns, lod_p2, lod_p3, lod_p4, lod_p5);

-- ================================================================
-- TOKEN DIMENSION COLUMNS
-- ================================================================
-- Add linguistic dimension columns to the tokens table.
-- These are simple single-value properties on specific token types.

-- count_mass: for nouns — 'count', 'mass', or 'both'
ALTER TABLE tokens ADD COLUMN IF NOT EXISTS count_mass TEXT;

-- proper_common: for nouns — 'proper', 'common', or 'both'
ALTER TABLE tokens ADD COLUMN IF NOT EXISTS proper_common TEXT;

-- aux_type: for verbs — 'modal', 'perf', 'prog', 'pass', 'do_support', or NULL
ALTER TABLE tokens ADD COLUMN IF NOT EXISTS aux_type TEXT;

COMMIT;

-- ================================================================
-- NOTE ON RULE DATA
-- ================================================================
-- The rule tables are created empty. Rule data (ordering constraints,
-- movement triggers, repair strategies) will be populated when the
-- linguistics specialist provides the specific rule definitions.
-- These are small tables (~5-10 rows each) that will be hand-curated.
--
-- The rule PK columns reference tokens that will be created in the
-- AB namespace when the rules are defined. Each rule becomes a token,
-- consistent with the project's "everything is a token" pattern.

-- ================================================================
-- VERIFICATION
-- ================================================================

\echo ''
\echo '=== New tables ==='
SELECT table_name
FROM information_schema.tables
WHERE table_schema = 'public'
  AND table_name IN ('ordering_rules', 'movement_rules', 'repair_rules')
ORDER BY table_name;

\echo ''
\echo '=== New columns on tokens ==='
SELECT column_name, data_type
FROM information_schema.columns
WHERE table_name = 'tokens' AND column_name IN ('count_mass', 'proper_common', 'aux_type')
ORDER BY column_name;

\echo ''
\echo '=== Token table column count ==='
SELECT COUNT(*) AS total_columns
FROM information_schema.columns
WHERE table_name = 'tokens' AND table_schema = 'public';
