-- HCP Migration 007e: Entity database schema (shared DDL)
-- Target: hcp_fic_entities AND hcp_nf_entities (run on both)
-- Depends on: 007a (namespace allocations)
--
-- 7 tables per entity-db-design.md Section 3:
--   1. tokens (entity registry)
--   2. entity_names (name assembly from label tokens)
--   3. entity_descriptions (typed free-text descriptions)
--   4. entity_properties (EAV key-value attributes)
--   5. entity_relationships (typed directional links)
--   6. entity_appearances (entity-to-PBM document links)
--   7. entity_rights (copyright/IP tracking)
--
-- Review fix applied:
--   - metadata JSONB column added to tokens table (review item §4.2.5)

BEGIN;

-- ============================================================================
-- 1. tokens (Entity Registry)
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
    name        TEXT NOT NULL,           -- Developer label (e.g., 'frodo_baggins')
    category    TEXT,                     -- Entity type: 'person', 'place', 'thing'
    subcategory TEXT,                     -- Sub-type: 'individual', 'settlement', 'artifact', etc.
    metadata    JSONB NOT NULL DEFAULT '{}'::jsonb,  -- Lightweight properties (per review)

    CONSTRAINT tokens_pkey PRIMARY KEY (token_id)
);

CREATE INDEX idx_tokens_prefix ON tokens (ns, p2, p3, p4, p5);
CREATE INDEX idx_tokens_ns ON tokens (ns);
CREATE INDEX idx_tokens_ns_p2 ON tokens (ns, p2);
CREATE INDEX idx_tokens_name ON tokens (name);
CREATE INDEX idx_tokens_category ON tokens (category);
CREATE INDEX idx_tokens_subcategory ON tokens (subcategory);

-- ============================================================================
-- 2. entity_names (Name Assembly)
-- ============================================================================

CREATE TABLE entity_names (
    -- Which entity (decomposed reference)
    entity_ns   TEXT NOT NULL,
    entity_p2   TEXT,
    entity_p3   TEXT,
    entity_p4   TEXT,
    entity_p5   TEXT,
    entity_id   TEXT NOT NULL GENERATED ALWAYS AS (
                    entity_ns || COALESCE('.' || entity_p2, '')
                              || COALESCE('.' || entity_p3, '')
                              || COALESCE('.' || entity_p4, '')
                              || COALESCE('.' || entity_p5, '')
                ) STORED,

    name_group  SMALLINT NOT NULL DEFAULT 0,  -- 0 = primary, 1+ = aliases
    name_type   TEXT NOT NULL DEFAULT 'primary',
                -- 'primary', 'alias', 'title', 'epithet', 'birth_name',
                -- 'nickname', 'pen_name', 'regnal_name', 'married_name'
    position    SMALLINT NOT NULL,             -- Order within name group

    -- Label token from language shard (decomposed reference)
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

    CONSTRAINT entity_names_pkey PRIMARY KEY (
        entity_ns, entity_p2, entity_p3, entity_p4, entity_p5,
        name_group, position
    )
);

CREATE INDEX idx_entity_names_entity ON entity_names (
    entity_ns, entity_p2, entity_p3, entity_p4, entity_p5
);
-- Reverse lookup: which entity has this label token in their name?
CREATE INDEX idx_entity_names_token ON entity_names (ns, p2, p3, p4, p5);
CREATE INDEX idx_entity_names_type ON entity_names (name_type);

-- ============================================================================
-- 3. entity_descriptions (Typed Free-Text)
-- ============================================================================

CREATE TABLE entity_descriptions (
    id              SERIAL PRIMARY KEY,

    -- Which entity (decomposed reference)
    entity_ns       TEXT NOT NULL,
    entity_p2       TEXT,
    entity_p3       TEXT,
    entity_p4       TEXT,
    entity_p5       TEXT,
    entity_id       TEXT NOT NULL GENERATED ALWAYS AS (
                        entity_ns || COALESCE('.' || entity_p2, '')
                                  || COALESCE('.' || entity_p3, '')
                                  || COALESCE('.' || entity_p4, '')
                                  || COALESCE('.' || entity_p5, '')
                    ) STORED,

    description_type TEXT NOT NULL,
                    -- People: 'appearance', 'personality', 'history', 'abilities', 'motivation'
                    -- Places: 'geography', 'culture', 'history', 'economy', 'climate'
                    -- Things: 'function', 'history', 'construction', 'significance', 'properties'
    description     TEXT NOT NULL,
    source_note     TEXT              -- Where this description was sourced from
);

CREATE INDEX idx_entity_desc_entity ON entity_descriptions (
    entity_ns, entity_p2, entity_p3, entity_p4, entity_p5
);
CREATE INDEX idx_entity_desc_type ON entity_descriptions (description_type);

-- ============================================================================
-- 4. entity_properties (EAV Key-Value)
-- ============================================================================

CREATE TABLE entity_properties (
    -- Which entity (decomposed reference)
    entity_ns   TEXT NOT NULL,
    entity_p2   TEXT,
    entity_p3   TEXT,
    entity_p4   TEXT,
    entity_p5   TEXT,
    entity_id   TEXT NOT NULL GENERATED ALWAYS AS (
                    entity_ns || COALESCE('.' || entity_p2, '')
                              || COALESCE('.' || entity_p3, '')
                              || COALESCE('.' || entity_p4, '')
                              || COALESCE('.' || entity_p5, '')
                ) STORED,

    key         TEXT NOT NULL,
    value       TEXT NOT NULL,

    CONSTRAINT entity_properties_pkey PRIMARY KEY (
        entity_ns, entity_p2, entity_p3, entity_p4, entity_p5, key
    )
);

CREATE INDEX idx_entity_props_entity ON entity_properties (
    entity_ns, entity_p2, entity_p3, entity_p4, entity_p5
);
CREATE INDEX idx_entity_props_key ON entity_properties (key);
CREATE INDEX idx_entity_props_key_value ON entity_properties (key, value);

-- ============================================================================
-- 5. entity_relationships (Typed Directional Links)
-- ============================================================================

CREATE TABLE entity_relationships (
    id              SERIAL PRIMARY KEY,

    -- Source entity (decomposed reference)
    source_ns       TEXT NOT NULL,
    source_p2       TEXT,
    source_p3       TEXT,
    source_p4       TEXT,
    source_p5       TEXT,
    source_id       TEXT NOT NULL GENERATED ALWAYS AS (
                        source_ns || COALESCE('.' || source_p2, '')
                                  || COALESCE('.' || source_p3, '')
                                  || COALESCE('.' || source_p4, '')
                                  || COALESCE('.' || source_p5, '')
                    ) STORED,

    -- Target entity (decomposed reference)
    target_ns       TEXT NOT NULL,
    target_p2       TEXT,
    target_p3       TEXT,
    target_p4       TEXT,
    target_p5       TEXT,
    target_id       TEXT NOT NULL GENERATED ALWAYS AS (
                        target_ns || COALESCE('.' || target_p2, '')
                                  || COALESCE('.' || target_p3, '')
                                  || COALESCE('.' || target_p4, '')
                                  || COALESCE('.' || target_p5, '')
                    ) STORED,

    relationship_type TEXT NOT NULL,      -- References AA.AF.AD token names
    qualifier       TEXT,                 -- Additional context (e.g., 'estranged', 'adoptive')
    temporal_note   TEXT,                 -- When the relationship applies
    source_note     TEXT                  -- Where this relationship was sourced
);

CREATE INDEX idx_entity_rel_source ON entity_relationships (
    source_ns, source_p2, source_p3, source_p4, source_p5
);
CREATE INDEX idx_entity_rel_target ON entity_relationships (
    target_ns, target_p2, target_p3, target_p4, target_p5
);
CREATE INDEX idx_entity_rel_type ON entity_relationships (relationship_type);

-- ============================================================================
-- 6. entity_appearances (Entity-to-PBM Links)
-- ============================================================================

CREATE TABLE entity_appearances (
    -- Which entity (decomposed reference)
    entity_ns   TEXT NOT NULL,
    entity_p2   TEXT,
    entity_p3   TEXT,
    entity_p4   TEXT,
    entity_p5   TEXT,
    entity_id   TEXT NOT NULL GENERATED ALWAYS AS (
                    entity_ns || COALESCE('.' || entity_p2, '')
                              || COALESCE('.' || entity_p3, '')
                              || COALESCE('.' || entity_p4, '')
                              || COALESCE('.' || entity_p5, '')
                ) STORED,

    -- Which PBM document (decomposed reference into z*/v* namespace)
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

    role        TEXT,                 -- 'protagonist', 'antagonist', 'mentioned',
                                     -- 'setting', 'mcguffin', 'subject', 'author', etc.
    prominence  TEXT,                 -- 'major', 'minor', 'background', 'mentioned'
    first_mention_position INTEGER,  -- Position in PBM content stream

    CONSTRAINT entity_appearances_pkey PRIMARY KEY (
        entity_ns, entity_p2, entity_p3, entity_p4, entity_p5,
        doc_ns, doc_p2, doc_p3, doc_p4, doc_p5
    )
);

CREATE INDEX idx_entity_app_entity ON entity_appearances (
    entity_ns, entity_p2, entity_p3, entity_p4, entity_p5
);
CREATE INDEX idx_entity_app_doc ON entity_appearances (
    doc_ns, doc_p2, doc_p3, doc_p4, doc_p5
);
CREATE INDEX idx_entity_app_role ON entity_appearances (role);

-- ============================================================================
-- 7. entity_rights (Copyright/IP Tracking)
-- ============================================================================

CREATE TABLE entity_rights (
    -- Which entity (decomposed reference)
    entity_ns       TEXT NOT NULL,
    entity_p2       TEXT,
    entity_p3       TEXT,
    entity_p4       TEXT,
    entity_p5       TEXT,
    entity_id       TEXT NOT NULL GENERATED ALWAYS AS (
                        entity_ns || COALESCE('.' || entity_p2, '')
                                  || COALESCE('.' || entity_p3, '')
                                  || COALESCE('.' || entity_p4, '')
                                  || COALESCE('.' || entity_p5, '')
                    ) STORED,

    rights_status   TEXT NOT NULL,
                    -- 'public_domain', 'copyrighted', 'trademarked', 'fair_use',
                    -- 'cc_by', 'cc_by_sa', 'unknown'
    rights_holder   TEXT,
    rights_note     TEXT,
    jurisdiction    TEXT,             -- 'US', 'UK', 'EU', 'international', etc.
    expiry_year     INTEGER,
    determination_date DATE,
    determination_source TEXT,        -- 'gutenberg_catalog', 'manual', etc.

    CONSTRAINT entity_rights_pkey PRIMARY KEY (
        entity_ns, entity_p2, entity_p3, entity_p4, entity_p5
    )
);

CREATE INDEX idx_entity_rights_status ON entity_rights (rights_status);
CREATE INDEX idx_entity_rights_holder ON entity_rights (rights_holder);

-- ============================================================================
-- Verify: list all tables
-- ============================================================================

SELECT tablename FROM pg_tables WHERE schemaname = 'public' ORDER BY tablename;

COMMIT;
