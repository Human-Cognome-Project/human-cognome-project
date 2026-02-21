-- HCP Migration 011: PBM prefix tree storage
-- Target: hcp_fic_pbm (replaces positional storage from migration 009)
-- Depends on: 009 (being replaced), 007f (shard registry)
--
-- PBM bond storage: token dumbbells (token_A, token_B, count).
-- Three-level prefix tree: document → starters → bond subtables.
-- Subtables split by token_B namespace, storing only distinguishing segments.
--
-- The prefix tree is a storage/dump compression scheme. Reads reconstruct
-- full token_ids via UNION ALL with string concatenation. The application
-- always sees flat (token_A, token_B, count) triples.
--
-- Directionality: bonds are directional. "the→cat" and "cat→the" are
-- separate entries with different counts. The starter IS the A-side.

BEGIN;

-- ============================================================================
-- Drop positional storage (migration 009 — empty, no data loss)
-- ============================================================================

DROP TABLE IF EXISTS document_relationships CASCADE;
DROP TABLE IF EXISTS document_provenance CASCADE;
DROP TABLE IF EXISTS doc_var_positions CASCADE;
DROP TABLE IF EXISTS doc_marker_positions CASCADE;
DROP TABLE IF EXISTS doc_char_positions CASCADE;
DROP TABLE IF EXISTS doc_word_positions CASCADE;
DROP TABLE IF EXISTS doc_counters CASCADE;
DROP TABLE IF EXISTS documents CASCADE;

-- ============================================================================
-- 1. pbm_documents — Document registry (1 per PBM)
-- ============================================================================

CREATE TABLE pbm_documents (
    id          SERIAL PRIMARY KEY,

    -- Document token address (decomposed)
    ns          TEXT NOT NULL,
    p2          TEXT,
    p3          TEXT,
    p4          TEXT,
    p5          TEXT,
    doc_id      TEXT NOT NULL GENERATED ALWAYS AS (
                    ns || COALESCE('.' || p2, '')
                       || COALESCE('.' || p3, '')
                       || COALESCE('.' || p4, '')
                       || COALESCE('.' || p5, '')
                ) STORED,

    name        TEXT NOT NULL,                     -- Human-readable title

    -- Crystallization seed: the first forward pair bond
    first_fpb_a TEXT,
    first_fpb_b TEXT,

    metadata    JSONB NOT NULL DEFAULT '{}'::jsonb,

    CONSTRAINT pbm_documents_doc_id_unique UNIQUE (doc_id)
);

CREATE INDEX idx_pbm_doc_ns ON pbm_documents (ns, p2, p3, p4, p5);
CREATE INDEX idx_pbm_doc_name ON pbm_documents (name);

-- ============================================================================
-- 2. pbm_starters — Hub nodes (1 per unique token_A per document)
-- ============================================================================
-- Integer PK used as compact FK by all bond subtables.
-- Eliminates doc_id and token_A repetition from every bond row.

CREATE TABLE pbm_starters (
    id          SERIAL PRIMARY KEY,
    doc_id      INTEGER NOT NULL REFERENCES pbm_documents(id),

    -- Full decomposed token_A
    a_ns        TEXT NOT NULL,
    a_p2        TEXT,
    a_p3        TEXT,
    a_p4        TEXT,
    a_p5        TEXT,
    -- Generated full token_A for cross-shard lookups
    token_a_id  TEXT NOT NULL GENERATED ALWAYS AS (
                    a_ns || COALESCE('.' || a_p2, '')
                         || COALESCE('.' || a_p3, '')
                         || COALESCE('.' || a_p4, '')
                         || COALESCE('.' || a_p5, '')
                ) STORED,

    CONSTRAINT pbm_starters_unique UNIQUE (doc_id, a_ns, a_p2, a_p3, a_p4, a_p5)
);

-- Load all starters for a document
CREATE INDEX idx_starters_doc ON pbm_starters (doc_id);
-- Find all documents containing a specific starter token
CREATE INDEX idx_starters_token ON pbm_starters (a_ns, a_p2, a_p3, a_p4, a_p5);

-- ============================================================================
-- 3. pbm_word_bonds — Word pairings (B = AB.AB.* — ~87.6% of bonds)
-- ============================================================================
-- Only stores distinguishing segments of token_B. Implicit prefix: AB.AB

CREATE TABLE pbm_word_bonds (
    starter_id  INTEGER NOT NULL REFERENCES pbm_starters(id),
    b_p3        TEXT NOT NULL,
    b_p4        TEXT NOT NULL,
    b_p5        TEXT,
    count       INTEGER NOT NULL,

    PRIMARY KEY (starter_id, b_p3, b_p4, b_p5)
);

-- Reverse lookup: find all starters bonded to a specific word
CREATE INDEX idx_word_bonds_b ON pbm_word_bonds (b_p3, b_p4, b_p5);

-- ============================================================================
-- 4. pbm_char_bonds — Punctuation/character pairings (B = AA.* — ~11.8%)
-- ============================================================================
-- ASCII bytes (AA.AA.AA.AA.*) and Unicode chars (AA.AB.AA.*) share AA prefix.

CREATE TABLE pbm_char_bonds (
    starter_id  INTEGER NOT NULL REFERENCES pbm_starters(id),
    b_p2        TEXT NOT NULL,
    b_p3        TEXT NOT NULL,
    b_p4        TEXT NOT NULL,
    b_p5        TEXT,
    count       INTEGER NOT NULL,

    PRIMARY KEY (starter_id, b_p2, b_p3, b_p4, b_p5)
);

CREATE INDEX idx_char_bonds_b ON pbm_char_bonds (b_p2, b_p3, b_p4, b_p5);

-- ============================================================================
-- 5. pbm_marker_bonds — Structural marker pairings (B = AA.AE.* — ~0.5%)
-- ============================================================================
-- Markers have no p5. Implicit prefix: AA.AE

CREATE TABLE pbm_marker_bonds (
    starter_id  INTEGER NOT NULL REFERENCES pbm_starters(id),
    b_p3        TEXT NOT NULL,
    b_p4        TEXT NOT NULL,
    count       INTEGER NOT NULL,

    PRIMARY KEY (starter_id, b_p3, b_p4)
);

CREATE INDEX idx_marker_bonds_b ON pbm_marker_bonds (b_p3, b_p4);

-- ============================================================================
-- 6. document_provenance — Expression-specific source tracking
-- ============================================================================
-- Rights, metadata, authorship live on the Thing entity in hcp_nf_entities.
-- This tracks only what's specific to THIS expression (source file, encoding).

CREATE TABLE document_provenance (
    doc_id              INTEGER NOT NULL REFERENCES pbm_documents(id),

    source_type         TEXT NOT NULL,      -- 'file', 'url', 'api', 'manual'
    source_path         TEXT,
    source_format       TEXT,               -- 'pdf', 'html', 'txt', etc.
    acquisition_date    DATE,
    source_checksum     TEXT,               -- SHA-256 of original source
    encoder_version     TEXT,

    -- Source catalog (for boilerplate scoping)
    source_catalog      TEXT,               -- 'gutenberg', 'archive_org', etc.
    catalog_id          TEXT,               -- ID in source catalog

    PRIMARY KEY (doc_id)
);

CREATE INDEX idx_prov_catalog ON document_provenance (source_catalog, catalog_id);

-- ============================================================================
-- 7. document_relationships — Inter-document links
-- ============================================================================

CREATE TABLE document_relationships (
    id                      SERIAL PRIMARY KEY,
    source_doc_id           INTEGER NOT NULL REFERENCES pbm_documents(id),
    target_doc_id           TEXT NOT NULL,     -- May reference other shards
    relationship_type       TEXT NOT NULL,
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
