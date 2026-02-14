-- HCP Migration 007d: Create hcp_en_pbm database schema
-- Target: hcp_en_pbm (new database — must be created first)
-- Depends on: 007a-c (namespace allocations, tokens in hcp_core)
--
-- 8 tables per pbm-format-spec.md Section 6:
--   1. tokens (document registry)
--   2. pbm_content (content stream)
--   3. document_provenance (source + copyright/IP + rights details)
--   4. non_text_content (binary blobs)
--   5. tab_definitions (indent level mappings)
--   6. tbd_log (unresolved reference tracking)
--   7. document_relationships (PBM-to-PBM links)
--   8. position_metadata (per-position key-value data)
--
-- Review fixes applied:
--   - Redundant idx_pbm_content_doc index REMOVED (PK covers it)
--   - document_rights fields MERGED into document_provenance
--     (jurisdiction, expiry_year, determination_date, determination_source,
--      source_catalog, catalog_id)

BEGIN;

-- ============================================================================
-- 1. tokens (Document Registry)
-- ============================================================================

CREATE TABLE tokens (
    ns          TEXT NOT NULL,
    p2          TEXT,
    p3          TEXT,
    p4          TEXT,
    p5          TEXT,
    token_id    TEXT NOT NULL GENERATED ALWAYS AS (
                    ns || COALESCE('.' || p2, '')
                       || COALESCE('.' || p3, '')
                       || COALESCE('.' || p4, '')
                       || COALESCE('.' || p5, '')
                ) STORED,
    name        TEXT NOT NULL,           -- Human-readable document title
    category    TEXT,                     -- Document type: 'book', 'article', 'table', 'dictionary', etc.
    subcategory TEXT,                     -- Sub-type: 'textbook', 'novel', 'reference', etc.
    metadata    JSONB NOT NULL DEFAULT '{}'::jsonb,  -- Lightweight properties (page count, word count estimates)

    CONSTRAINT tokens_pkey PRIMARY KEY (token_id)
);

CREATE INDEX idx_tokens_ns ON tokens (ns);
CREATE INDEX idx_tokens_ns_p2 ON tokens (ns, p2);
CREATE INDEX idx_tokens_prefix ON tokens (ns, p2, p3, p4, p5);
CREATE INDEX idx_tokens_name ON tokens (name);
CREATE INDEX idx_tokens_category ON tokens (category);

-- ============================================================================
-- 2. pbm_content (Content Stream)
-- ============================================================================

CREATE TABLE pbm_content (
    -- Which PBM this content belongs to (decomposed reference)
    doc_ns      TEXT NOT NULL,
    doc_p2      TEXT,
    doc_p3      TEXT,
    doc_p4      TEXT,
    doc_p5      TEXT,
    doc_id      TEXT NOT NULL GENERATED ALWAYS AS (
                    doc_ns || COALESCE('.' || doc_p2, '')
                           || COALESCE('.' || doc_p3, '')
                           || COALESCE('.' || doc_p4, '')
                           || COALESCE('.' || doc_p5, '')
                ) STORED,

    -- Position in the content stream
    position    INTEGER NOT NULL,

    -- What token is at this position (decomposed reference)
    ns          TEXT NOT NULL,
    p2          TEXT,
    p3          TEXT,
    p4          TEXT,
    p5          TEXT,
    token_id    TEXT NOT NULL GENERATED ALWAYS AS (
                    ns || COALESCE('.' || p2, '')
                       || COALESCE('.' || p3, '')
                       || COALESCE('.' || p4, '')
                       || COALESCE('.' || p5, '')
                ) STORED,

    CONSTRAINT pbm_content_pkey PRIMARY KEY (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5, position)
);

-- NOTE: No idx_pbm_content_doc — the PK's B-tree already covers doc prefix + position queries.
-- For finding all PBMs that reference a specific token
CREATE INDEX idx_pbm_content_token ON pbm_content (ns, p2, p3, p4, p5);
-- For the generated doc_id (cross-shard joins)
CREATE INDEX idx_pbm_content_doc_id ON pbm_content (doc_id);

-- ============================================================================
-- 3. document_provenance (Source + Copyright/IP + Rights Details)
-- ============================================================================
-- Merges document_rights fields per review recommendation.

CREATE TABLE document_provenance (
    id              SERIAL PRIMARY KEY,

    -- Which PBM (decomposed reference)
    doc_ns          TEXT NOT NULL,
    doc_p2          TEXT,
    doc_p3          TEXT,
    doc_p4          TEXT,
    doc_p5          TEXT,
    doc_id          TEXT NOT NULL GENERATED ALWAYS AS (
                        doc_ns || COALESCE('.' || doc_p2, '')
                               || COALESCE('.' || doc_p3, '')
                               || COALESCE('.' || doc_p4, '')
                               || COALESCE('.' || doc_p5, '')
                    ) STORED,

    -- Source tracking
    source_type     TEXT NOT NULL,        -- 'file', 'url', 'api', 'manual'
    source_path     TEXT,                 -- Original file path or URL
    source_format   TEXT,                 -- 'pdf', 'html', 'txt', 'docx', etc.
    acquisition_date DATE,
    source_checksum TEXT,                 -- SHA-256 of original source
    encoder_version TEXT,                 -- Encoder that produced this PBM

    -- Content language (decomposed token reference)
    lang_ns         TEXT,
    lang_p2         TEXT,
    lang_p3         TEXT,
    lang_p4         TEXT,
    lang_p5         TEXT,
    lang_token_id   TEXT GENERATED ALWAYS AS (
                        lang_ns || COALESCE('.' || lang_p2, '')
                               || COALESCE('.' || lang_p3, '')
                               || COALESCE('.' || lang_p4, '')
                               || COALESCE('.' || lang_p5, '')
                    ) STORED,

    -- Copyright and IP (from original pbm-format-spec)
    rights_status       TEXT,             -- 'public_domain', 'copyrighted', 'licensed', 'unknown'
    copyright_holder    TEXT,
    copyright_year      SMALLINT,
    license_type        TEXT,             -- 'CC-BY-4.0', 'MIT', 'all_rights_reserved', 'fair_use', etc.
    reproduction_rights TEXT,             -- 'unrestricted', 'analysis_only', 'no_reproduction'
    ip_notes            TEXT,

    -- Rights details (merged from document_rights per review)
    jurisdiction            TEXT,         -- 'US', 'UK', 'EU', 'international', etc.
    expiry_year             INTEGER,      -- Year copyright expires (if known)
    determination_date      DATE,         -- When rights status was determined
    determination_source    TEXT,         -- How determined: 'gutenberg_catalog', 'manual', etc.
    source_catalog          TEXT,         -- 'gutenberg', 'archive_org', 'manual', etc.
    catalog_id              TEXT          -- ID in source catalog (e.g., Gutenberg book number)
);

CREATE INDEX idx_provenance_doc ON document_provenance (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5);
CREATE INDEX idx_provenance_doc_id ON document_provenance (doc_id);
CREATE INDEX idx_provenance_rights ON document_provenance (rights_status);
CREATE INDEX idx_provenance_catalog ON document_provenance (source_catalog, catalog_id);

-- ============================================================================
-- 4. non_text_content (Binary Blobs)
-- ============================================================================

CREATE TABLE non_text_content (
    id              SERIAL PRIMARY KEY,
    blob_id         TEXT NOT NULL UNIQUE,  -- Deterministic reference ID (SHA-256 of content)
    content_type    TEXT NOT NULL,         -- MIME type: 'image/png', 'application/pdf', etc.
    content         BYTEA NOT NULL,        -- The raw binary content
    original_name   TEXT,                  -- Original filename if known
    width_px        INTEGER,              -- For images: width in pixels
    height_px       INTEGER,              -- For images: height in pixels
    size_bytes      BIGINT NOT NULL       -- Size of the content in bytes
);

CREATE INDEX idx_non_text_blob_id ON non_text_content (blob_id);

-- ============================================================================
-- 5. tab_definitions (Indent Level Mappings)
-- ============================================================================

CREATE TABLE tab_definitions (
    -- Which PBM (decomposed reference)
    doc_ns                  TEXT NOT NULL,
    doc_p2                  TEXT,
    doc_p3                  TEXT,
    doc_p4                  TEXT,
    doc_p5                  TEXT,
    doc_id                  TEXT NOT NULL GENERATED ALWAYS AS (
                                doc_ns || COALESCE('.' || doc_p2, '')
                                       || COALESCE('.' || doc_p3, '')
                                       || COALESCE('.' || doc_p4, '')
                                       || COALESCE('.' || doc_p5, '')
                            ) STORED,

    indent_level            SMALLINT NOT NULL,  -- 1-8 (matches indent_level_N markers)
    original_representation TEXT NOT NULL,       -- 'tab', '2_spaces', '4_spaces', etc.
    semantic_role           TEXT,                -- 'code_indent', 'quote_nesting', 'list_nesting', etc.

    CONSTRAINT tab_definitions_pkey PRIMARY KEY (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5, indent_level)
);

-- ============================================================================
-- 6. tbd_log (Unresolved Reference Tracking)
-- ============================================================================

CREATE TABLE tbd_log (
    id                  SERIAL PRIMARY KEY,

    -- Which PBM (decomposed reference)
    doc_ns              TEXT NOT NULL,
    doc_p2              TEXT,
    doc_p3              TEXT,
    doc_p4              TEXT,
    doc_p5              TEXT,
    doc_id              TEXT NOT NULL GENERATED ALWAYS AS (
                            doc_ns || COALESCE('.' || doc_p2, '')
                                   || COALESCE('.' || doc_p3, '')
                                   || COALESCE('.' || doc_p4, '')
                                   || COALESCE('.' || doc_p5, '')
                        ) STORED,

    position            INTEGER NOT NULL,         -- Position in content stream
    tbd_index           INTEGER NOT NULL,         -- Sequential within ingestion run
    original_text       TEXT NOT NULL,             -- The unresolvable surface text
    resolution_status   TEXT NOT NULL DEFAULT 'unresolved',  -- 'unresolved', 'resolved', 'abandoned'

    -- Resolved token (decomposed reference, NULL until resolved)
    resolved_ns         TEXT,
    resolved_p2         TEXT,
    resolved_p3         TEXT,
    resolved_p4         TEXT,
    resolved_p5         TEXT,
    resolved_token_id   TEXT GENERATED ALWAYS AS (
                            resolved_ns || COALESCE('.' || resolved_p2, '')
                                       || COALESCE('.' || resolved_p3, '')
                                       || COALESCE('.' || resolved_p4, '')
                                       || COALESCE('.' || resolved_p5, '')
                        ) STORED,

    resolved_date       TIMESTAMP WITH TIME ZONE
);

CREATE INDEX idx_tbd_log_doc ON tbd_log (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5);
CREATE INDEX idx_tbd_log_status ON tbd_log (resolution_status);

-- ============================================================================
-- 7. document_relationships (PBM-to-PBM Links)
-- ============================================================================

CREATE TABLE document_relationships (
    id                      SERIAL PRIMARY KEY,

    -- Source PBM (decomposed reference)
    source_ns               TEXT NOT NULL,
    source_p2               TEXT,
    source_p3               TEXT,
    source_p4               TEXT,
    source_p5               TEXT,
    source_id               TEXT NOT NULL GENERATED ALWAYS AS (
                                source_ns || COALESCE('.' || source_p2, '')
                                          || COALESCE('.' || source_p3, '')
                                          || COALESCE('.' || source_p4, '')
                                          || COALESCE('.' || source_p5, '')
                            ) STORED,

    -- Target PBM (decomposed reference)
    target_ns               TEXT NOT NULL,
    target_p2               TEXT,
    target_p3               TEXT,
    target_p4               TEXT,
    target_p5               TEXT,
    target_id               TEXT NOT NULL GENERATED ALWAYS AS (
                                target_ns || COALESCE('.' || target_p2, '')
                                          || COALESCE('.' || target_p3, '')
                                          || COALESCE('.' || target_p4, '')
                                          || COALESCE('.' || target_p5, '')
                            ) STORED,

    relationship_type       TEXT NOT NULL,  -- 'extracted_from', 'derived_from', 'revision_of',
                                           -- 'translation_of', 'references', 'part_of'
    extraction_range_start  INTEGER,        -- Start position in source (for 'extracted_from')
    extraction_range_end    INTEGER,        -- End position in source (for 'extracted_from')
    notes                   TEXT
);

CREATE INDEX idx_docrel_source ON document_relationships (source_ns, source_p2, source_p3, source_p4, source_p5);
CREATE INDEX idx_docrel_target ON document_relationships (target_ns, target_p2, target_p3, target_p4, target_p5);
CREATE INDEX idx_docrel_type ON document_relationships (relationship_type);

-- ============================================================================
-- 8. position_metadata (Per-Position Key-Value Data)
-- ============================================================================

CREATE TABLE position_metadata (
    -- Which PBM (decomposed reference)
    doc_ns      TEXT NOT NULL,
    doc_p2      TEXT,
    doc_p3      TEXT,
    doc_p4      TEXT,
    doc_p5      TEXT,
    doc_id      TEXT NOT NULL GENERATED ALWAYS AS (
                    doc_ns || COALESCE('.' || doc_p2, '')
                           || COALESCE('.' || doc_p3, '')
                           || COALESCE('.' || doc_p4, '')
                           || COALESCE('.' || doc_p5, '')
                ) STORED,

    position    INTEGER NOT NULL,      -- Position in content stream
    key         TEXT NOT NULL,          -- 'url', 'ordinal', 'blob_id', 'language', 'alt_text', 'entity_ref', etc.
    value       TEXT NOT NULL,          -- The metadata value

    CONSTRAINT position_metadata_pkey PRIMARY KEY (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5, position, key)
);

CREATE INDEX idx_posmeta_doc ON position_metadata (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5);

-- ============================================================================
-- Verify: list all tables
-- ============================================================================

SELECT tablename FROM pg_tables WHERE schemaname = 'public' ORDER BY tablename;

COMMIT;
