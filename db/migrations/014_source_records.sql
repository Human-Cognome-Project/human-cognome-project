-- HCP Migration 014: Source records, editions, and entity lists
-- Target: hcp_nf_entities
-- Depends on: 007e (entity database schema)
--
-- Works (novels, etc.) are non-fiction Things (wA namespace) that anchor
-- everything about a PBM: author, characters, places, objects, editions.
-- A novel is a real object that exists in the world, regardless of whether
-- its contents are fiction.
--
-- This creates the source record schema alongside existing entity tables.
-- All librarian data in entity_* tables is preserved.
--
-- Structure:
--   sources          — Work record (extends tokens entry for creative works)
--   source_editions  — Edition subtable (0 = reference full PBM, 1+ = deltas)
--   source_people    — Dramatis Personae (junction: Work → Person entities)
--   source_places    — Locae (junction: Work → Place entities)
--   source_things    — Rerum (junction: Work → Thing entities)
--   source_glossary  — Author-specific vocabulary (placeholder for corpus lingo)
--
-- Edition model:
--   Edition 0 is always the reference (full PBM). Subsequent editions store
--   only delta information against the reference. If the reference changes,
--   edition numbers are reordered and deltas recalculated (future optimization).
--   For Gutenberg single-edition works, there is one edition (0).
--
-- All entity references are decomposed per project schema pattern.
-- Cross-shard references (to hcp_fic_entities, hcp_fic_pbm) are TEXT token_ids.

BEGIN;

-- ============================================================================
-- 1. sources — Work record
-- ============================================================================
-- Extends the tokens table entry for Things that are creative works.
-- Each source row has a corresponding tokens row (wA.DA.* namespace).

CREATE TABLE sources (
    -- The Work's Thing token (decomposed)
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

    -- Work name lives in entity_names (assembled from label tokens).
    -- No surface form title here — everything is token_ids.

    -- Author (decomposed reference to Person token, y* namespace)
    author_ns   TEXT NOT NULL,
    author_p2   TEXT,
    author_p3   TEXT,
    author_p4   TEXT,
    author_p5   TEXT,
    author_id   TEXT NOT NULL GENERATED ALWAYS AS (
                    author_ns || COALESCE('.' || author_p2, '')
                              || COALESCE('.' || author_p3, '')
                              || COALESCE('.' || author_p4, '')
                              || COALESCE('.' || author_p5, '')
                ) STORED,

    first_pub_date  TEXT,                    -- Year or flexible date string
    metadata        JSONB NOT NULL DEFAULT '{}'::jsonb,  -- Genre, tags, etc.

    CONSTRAINT sources_pkey PRIMARY KEY (token_id),
    CONSTRAINT sources_decomposed_unique UNIQUE (ns, p2, p3, p4, p5)
);

-- Find all works by an author
CREATE INDEX idx_sources_author ON sources (author_id);
CREATE INDEX idx_sources_author_decomposed ON sources (author_ns, author_p2, author_p3, author_p4, author_p5);
-- Work name lookup via entity_names, not here

-- ============================================================================
-- 2. source_editions — Edition subtable
-- ============================================================================
-- Edition 0 = reference (full PBM). Edition 1+ = delta against reference.
-- pbm_doc_id is a cross-shard reference to hcp_fic_pbm.pbm_documents.doc_id

CREATE TABLE source_editions (
    -- Parent source (decomposed)
    source_ns   TEXT NOT NULL,
    source_p2   TEXT,
    source_p3   TEXT,
    source_p4   TEXT,
    source_p5   TEXT,
    source_id   TEXT NOT NULL GENERATED ALWAYS AS (
                    source_ns || COALESCE('.' || source_p2, '')
                              || COALESCE('.' || source_p3, '')
                              || COALESCE('.' || source_p4, '')
                              || COALESCE('.' || source_p5, '')
                ) STORED,

    edition_number  INTEGER NOT NULL DEFAULT 0,  -- 0 = reference

    -- PBM document link (cross-shard text reference)
    pbm_doc_id      TEXT,

    publisher       TEXT,
    pub_date        TEXT,
    source_catalog  TEXT,           -- 'gutenberg', 'archive_org', etc.
    catalog_id      TEXT,           -- ID within source catalog
    notes           TEXT,

    CONSTRAINT source_editions_pkey PRIMARY KEY (source_id, edition_number)
);

-- Reverse lookup: find which source/edition a PBM belongs to
CREATE INDEX idx_editions_pbm ON source_editions (pbm_doc_id);
-- Find editions by catalog reference
CREATE INDEX idx_editions_catalog ON source_editions (source_catalog, catalog_id);
-- Load all editions for a source
CREATE INDEX idx_editions_source ON source_editions (source_ns, source_p2, source_p3, source_p4, source_p5);

-- ============================================================================
-- 3. source_people — Dramatis Personae
-- ============================================================================
-- Junction: Work → Person entities. Cross-shard (person may be in
-- hcp_fic_entities u* or hcp_nf_entities y*).

CREATE TABLE source_people (
    -- Source (decomposed)
    source_ns   TEXT NOT NULL,
    source_p2   TEXT,
    source_p3   TEXT,
    source_p4   TEXT,
    source_p5   TEXT,
    source_id   TEXT NOT NULL GENERATED ALWAYS AS (
                    source_ns || COALESCE('.' || source_p2, '')
                              || COALESCE('.' || source_p3, '')
                              || COALESCE('.' || source_p4, '')
                              || COALESCE('.' || source_p5, '')
                ) STORED,

    -- Person (decomposed, cross-shard)
    person_ns   TEXT NOT NULL,
    person_p2   TEXT,
    person_p3   TEXT,
    person_p4   TEXT,
    person_p5   TEXT,
    person_id   TEXT NOT NULL GENERATED ALWAYS AS (
                    person_ns || COALESCE('.' || person_p2, '')
                              || COALESCE('.' || person_p3, '')
                              || COALESCE('.' || person_p4, '')
                              || COALESCE('.' || person_p5, '')
                ) STORED,

    role        TEXT,               -- 'protagonist', 'antagonist', 'minor', 'mentioned'

    CONSTRAINT source_people_pkey PRIMARY KEY (source_id, person_id)
);

-- Reverse: which works does this person appear in?
CREATE INDEX idx_src_people_person ON source_people (person_id);
CREATE INDEX idx_src_people_person_decomposed ON source_people (person_ns, person_p2, person_p3, person_p4, person_p5);
-- Load all people for a source
CREATE INDEX idx_src_people_source ON source_people (source_ns, source_p2, source_p3, source_p4, source_p5);

-- ============================================================================
-- 4. source_places — Locae
-- ============================================================================
-- Junction: Work → Place entities. Cross-shard (place may be in
-- hcp_fic_entities t* or hcp_nf_entities x*).

CREATE TABLE source_places (
    -- Source (decomposed)
    source_ns   TEXT NOT NULL,
    source_p2   TEXT,
    source_p3   TEXT,
    source_p4   TEXT,
    source_p5   TEXT,
    source_id   TEXT NOT NULL GENERATED ALWAYS AS (
                    source_ns || COALESCE('.' || source_p2, '')
                              || COALESCE('.' || source_p3, '')
                              || COALESCE('.' || source_p4, '')
                              || COALESCE('.' || source_p5, '')
                ) STORED,

    -- Place (decomposed, cross-shard)
    place_ns    TEXT NOT NULL,
    place_p2    TEXT,
    place_p3    TEXT,
    place_p4    TEXT,
    place_p5    TEXT,
    place_id    TEXT NOT NULL GENERATED ALWAYS AS (
                    place_ns || COALESCE('.' || place_p2, '')
                             || COALESCE('.' || place_p3, '')
                             || COALESCE('.' || place_p4, '')
                             || COALESCE('.' || place_p5, '')
                ) STORED,

    role        TEXT,               -- 'primary_setting', 'secondary', 'mentioned'

    CONSTRAINT source_places_pkey PRIMARY KEY (source_id, place_id)
);

-- Reverse: which works reference this place?
CREATE INDEX idx_src_places_place ON source_places (place_id);
CREATE INDEX idx_src_places_place_decomposed ON source_places (place_ns, place_p2, place_p3, place_p4, place_p5);
-- Load all places for a source
CREATE INDEX idx_src_places_source ON source_places (source_ns, source_p2, source_p3, source_p4, source_p5);

-- ============================================================================
-- 5. source_things — Rerum
-- ============================================================================
-- Junction: Work → Thing entities referenced by the work (objects,
-- organizations, artifacts — not the Work itself). Cross-shard.

CREATE TABLE source_things (
    -- Source (decomposed)
    source_ns   TEXT NOT NULL,
    source_p2   TEXT,
    source_p3   TEXT,
    source_p4   TEXT,
    source_p5   TEXT,
    source_id   TEXT NOT NULL GENERATED ALWAYS AS (
                    source_ns || COALESCE('.' || source_p2, '')
                              || COALESCE('.' || source_p3, '')
                              || COALESCE('.' || source_p4, '')
                              || COALESCE('.' || source_p5, '')
                ) STORED,

    -- Thing (decomposed, cross-shard)
    thing_ns    TEXT NOT NULL,
    thing_p2    TEXT,
    thing_p3    TEXT,
    thing_p4    TEXT,
    thing_p5    TEXT,
    thing_id    TEXT NOT NULL GENERATED ALWAYS AS (
                    thing_ns || COALESCE('.' || thing_p2, '')
                             || COALESCE('.' || thing_p3, '')
                             || COALESCE('.' || thing_p4, '')
                             || COALESCE('.' || thing_p5, '')
                ) STORED,

    role        TEXT,               -- 'central', 'significant', 'mentioned'

    CONSTRAINT source_things_pkey PRIMARY KEY (source_id, thing_id)
);

-- Reverse: which works reference this thing?
CREATE INDEX idx_src_things_thing ON source_things (thing_id);
CREATE INDEX idx_src_things_thing_decomposed ON source_things (thing_ns, thing_p2, thing_p3, thing_p4, thing_p5);
-- Load all things for a source
CREATE INDEX idx_src_things_source ON source_things (source_ns, source_p2, source_p3, source_p4, source_p5);

-- ============================================================================
-- 6. source_glossary — Author-specific vocabulary (placeholder)
-- ============================================================================
-- Corpus-scoped lingo: non-standard surface forms with equivalences to
-- standard tokens. "warn't" → wasn't, "sivilize" → civilize.
-- Full design TBD — this reserves the pattern.

CREATE TABLE source_glossary (
    -- Source (decomposed)
    source_ns   TEXT NOT NULL,
    source_p2   TEXT,
    source_p3   TEXT,
    source_p4   TEXT,
    source_p5   TEXT,
    source_id   TEXT NOT NULL GENERATED ALWAYS AS (
                    source_ns || COALESCE('.' || source_p2, '')
                              || COALESCE('.' || source_p3, '')
                              || COALESCE('.' || source_p4, '')
                              || COALESCE('.' || source_p5, '')
                ) STORED,

    surface     TEXT NOT NULL,               -- Author's spelling, e.g. "warn't"
    equivalence TEXT,                        -- Standard token_id (base-50 pairs)
    gloss       TEXT,                        -- Reviewer note, e.g. 'Twain dialect'

    CONSTRAINT source_glossary_pkey PRIMARY KEY (source_id, surface)
);

-- Find all glossary entries for a source
CREATE INDEX idx_glossary_source ON source_glossary (source_ns, source_p2, source_p3, source_p4, source_p5);
-- Reverse: which sources use this surface form?
CREATE INDEX idx_glossary_surface ON source_glossary (surface);

-- ============================================================================
-- Verify
-- ============================================================================

SELECT tablename FROM pg_tables WHERE schemaname = 'public' ORDER BY tablename;

COMMIT;
