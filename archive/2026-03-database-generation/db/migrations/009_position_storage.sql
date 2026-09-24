-- HCP Migration 009: Position-based document storage
-- Target: hcp_fic_pbm (fresh schema — replaces all existing tables)
-- Depends on: 007a (namespace allocations), 007f (shard registry)
--
-- Clean-slate positional document storage. Per-document: each unique
-- token maps to a packed list of positions where it appears.
-- PBM bond data is DERIVED at inference time, not stored.
--
-- Position encoding: base-50 pairs, 4 chars per position.
--   pair1 = position / 2500, pair2 = position % 2500
--   Each pair: hi = value / 50, lo = value % 50
--   Alphabet: ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx
--   Max position: 2499 * 2500 + 2499 = 6,249,999 (~6.25M slots)
--
-- Whitespace is implicit: gaps in position numbering = spaces.
-- Position 0 = document start, total_slots = document end.

BEGIN;

-- ============================================================================
-- Drop all existing tables (old bond-based schema — data is disposable)
-- ============================================================================

DROP TABLE IF EXISTS pbm_marker_bonds CASCADE;
DROP TABLE IF EXISTS pbm_char_bonds CASCADE;
DROP TABLE IF EXISTS pbm_word_bonds CASCADE;
DROP TABLE IF EXISTS pbm_starters CASCADE;
DROP TABLE IF EXISTS pbm_counters CASCADE;
DROP TABLE IF EXISTS position_metadata CASCADE;
DROP TABLE IF EXISTS document_relationships CASCADE;
DROP TABLE IF EXISTS document_provenance CASCADE;
DROP TABLE IF EXISTS non_text_content CASCADE;
DROP TABLE IF EXISTS tab_definitions CASCADE;
DROP TABLE IF EXISTS tbd_log CASCADE;
DROP TABLE IF EXISTS pbm_content CASCADE;
DROP TABLE IF EXISTS pbm_documents CASCADE;
DROP TABLE IF EXISTS tokens CASCADE;

-- ============================================================================
-- 1. documents — Document registry
-- ============================================================================
-- Decomposed token ID following standard HCP schema pattern.
-- Fiction PBM namespace: vA.AB.<century>.<p4>.<p5>

CREATE TABLE documents (
    ns   TEXT NOT NULL,
    p2   TEXT,
    p3   TEXT,
    p4   TEXT,
    p5   TEXT,

    doc_id TEXT GENERATED ALWAYS AS (
        ns ||
        COALESCE('.' || p2, '') ||
        COALESCE('.' || p3, '') ||
        COALESCE('.' || p4, '') ||
        COALESCE('.' || p5, '')
    ) STORED NOT NULL,

    -- Human-readable label for this expression
    name            TEXT NOT NULL,

    -- Language of this expression (decomposed token reference)
    lang_ns         TEXT,
    lang_p2         TEXT,
    lang_p3         TEXT,
    lang_p4         TEXT,
    lang_p5         TEXT,

    -- Structural stats
    total_slots     INTEGER,            -- Total position slots in document
    unique_tokens   INTEGER,            -- Count of distinct tokens
    metadata        JSONB NOT NULL DEFAULT '{}',

    PRIMARY KEY (doc_id)
);

-- The Thing entity (in sA/wA) holds the list of expressions,
-- not the other way around. No FK to Thing here.

CREATE INDEX idx_doc_prefix ON documents (ns, p2, p3, p4, p5);
CREATE INDEX idx_doc_ns ON documents (ns);
CREATE INDEX idx_doc_name ON documents (name);

-- ============================================================================
-- 2. doc_counters — Auto-increment for document addressing
-- ============================================================================

CREATE TABLE doc_counters (
    ns          TEXT NOT NULL,
    p2          TEXT NOT NULL,
    p3          TEXT NOT NULL,
    next_value  INTEGER NOT NULL DEFAULT 0,

    PRIMARY KEY (ns, p2, p3)
);

-- ============================================================================
-- 3. Position tables — prefix tree by token namespace
-- ============================================================================
-- Each subtable stores only the distinguishing segments of the token ID.
-- Position lists are packed base-50 text (4 chars per position).
-- Composite PK on (doc_id, token segments) — no surrogate ID needed.

-- Word tokens: AB.AB.p3.p4[.p5] — majority of tokens (~87%)
CREATE TABLE doc_word_positions (
    doc_id      TEXT NOT NULL REFERENCES documents(doc_id),
    t_p3        TEXT NOT NULL,
    t_p4        TEXT NOT NULL,
    t_p5        TEXT,
    positions   TEXT NOT NULL,

    PRIMARY KEY (doc_id, t_p3, t_p4, t_p5)
);

-- Reverse lookup: find all documents containing a specific word
CREATE INDEX idx_word_pos_token ON doc_word_positions (t_p3, t_p4, t_p5);

-- Character/punctuation tokens: AA.p2.p3.p4[.p5]
CREATE TABLE doc_char_positions (
    doc_id      TEXT NOT NULL REFERENCES documents(doc_id),
    t_p2        TEXT NOT NULL,
    t_p3        TEXT NOT NULL,
    t_p4        TEXT NOT NULL,
    t_p5        TEXT,
    positions   TEXT NOT NULL,

    PRIMARY KEY (doc_id, t_p2, t_p3, t_p4, t_p5)
);

CREATE INDEX idx_char_pos_token ON doc_char_positions (t_p2, t_p3, t_p4, t_p5);

-- Marker tokens: AA.AE.p3.p4 (newlines, structural markers)
CREATE TABLE doc_marker_positions (
    doc_id      TEXT NOT NULL REFERENCES documents(doc_id),
    t_p3        TEXT NOT NULL,
    t_p4        TEXT NOT NULL,
    positions   TEXT NOT NULL,

    PRIMARY KEY (doc_id, t_p3, t_p4)
);

CREATE INDEX idx_marker_pos_token ON doc_marker_positions (t_p3, t_p4);

-- Var tokens: var.* (unresolved sequences from var DB)
-- Full var_id stored — no decomposition, no implicit prefix.
CREATE TABLE doc_var_positions (
    doc_id      TEXT NOT NULL REFERENCES documents(doc_id),
    var_id      TEXT NOT NULL,           -- e.g., 'var.42'
    positions   TEXT NOT NULL,

    PRIMARY KEY (doc_id, var_id)
);

CREATE INDEX idx_var_pos_varid ON doc_var_positions (var_id);

-- ============================================================================
-- 4. document_provenance — Expression-specific source tracking
-- ============================================================================
-- Rights, metadata, and authorship live on the Thing entity in hcp_nf_entities.
-- This table tracks only what's specific to THIS expression (source file, encoding).

CREATE TABLE document_provenance (
    doc_id              TEXT NOT NULL REFERENCES documents(doc_id),

    -- Source tracking
    source_type         TEXT NOT NULL,      -- 'file', 'url', 'api', 'manual'
    source_path         TEXT,               -- Original file path or URL
    source_format       TEXT,               -- 'pdf', 'html', 'txt', etc.
    acquisition_date    DATE,
    source_checksum     TEXT,               -- SHA-256 of original source
    encoder_version     TEXT,

    -- Source catalog (for continuation index scoping)
    source_catalog      TEXT,               -- 'gutenberg', 'archive_org', etc.
    catalog_id          TEXT,               -- ID in source catalog

    PRIMARY KEY (doc_id)
);

CREATE INDEX idx_prov_catalog ON document_provenance (source_catalog, catalog_id);

-- ============================================================================
-- 5. document_relationships — Inter-document links
-- ============================================================================

CREATE TABLE document_relationships (
    id                      SERIAL PRIMARY KEY,
    source_doc_id           TEXT NOT NULL REFERENCES documents(doc_id),
    target_doc_id           TEXT NOT NULL,     -- May reference other shards
    relationship_type       TEXT NOT NULL,     -- 'extracted_from', 'derived_from', etc.
    extraction_range_start  INTEGER,
    extraction_range_end    INTEGER,
    notes                   TEXT
);

CREATE INDEX idx_docrel_source ON document_relationships (source_doc_id);
CREATE INDEX idx_docrel_target ON document_relationships (target_doc_id);
CREATE INDEX idx_docrel_type ON document_relationships (relationship_type);

-- ============================================================================
-- Verify
-- ============================================================================

SELECT tablename FROM pg_tables WHERE schemaname = 'public' ORDER BY tablename;

COMMIT;
