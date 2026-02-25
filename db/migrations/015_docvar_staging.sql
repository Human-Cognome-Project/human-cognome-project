-- HCP Migration 015: Docvar staging — classification, grouping, review
-- Target: hcp_fic_pbm
-- Depends on: 012 (pbm_docvars)
--
-- During PBM encoding, unrecognized strings become docvars. The engine
-- classifies them into four categories and groups aliases (e.g., "Dr.",
-- "Doolittle", "Dr. Doolittle", "John Doolittle" → same person).
--
-- This migration adds:
--   1. var_category on pbm_docvars — engine-assigned classification
--   2. docvar_groups — alias groupings with suggested entity match
--   3. group_id on pbm_docvars — links var to its group
--
-- Workflow:
--   1. Engine creates docvars during encoding (existing flow)
--   2. Engine classifies each var (proper/lingo/sic/uri_metadata)
--   3. Engine groups aliases → creates docvar_groups entry
--   4. Engine optionally suggests entity match on the group
--   5. Reviewer confirms/rejects/reclassifies
--   6. On confirmation: bond data patched, entity records created/updated
--
-- Back-propagation: when the engine later resolves a stem that matches
-- an earlier var's morpheme root, it updates the var's equivalence and
-- group assignment directly. This is engine-side logic (C++ kernels),
-- the DB just needs to accept the updates.

BEGIN;

-- ============================================================================
-- 1. docvar_groups — Alias groupings per document
-- ============================================================================
-- All docvars in the same group are aliases for the same entity/concept.
-- The engine creates groups, optionally suggests an entity match.
-- Reviewer confirms or rejects.

CREATE TABLE docvar_groups (
    id              SERIAL PRIMARY KEY,
    doc_id          INTEGER NOT NULL REFERENCES pbm_documents(id),

    -- Engine-suggested entity match (cross-shard reference)
    -- NULL if no match suggested (new entity candidate)
    suggested_ns    TEXT,
    suggested_p2    TEXT,
    suggested_p3    TEXT,
    suggested_p4    TEXT,
    suggested_p5    TEXT,
    suggested_id    TEXT GENERATED ALWAYS AS (
                        CASE WHEN suggested_ns IS NOT NULL THEN
                            suggested_ns || COALESCE('.' || suggested_p2, '')
                                         || COALESCE('.' || suggested_p3, '')
                                         || COALESCE('.' || suggested_p4, '')
                                         || COALESCE('.' || suggested_p5, '')
                        END
                    ) STORED,

    -- Classification of what this group represents
    -- 'person', 'place', 'thing', 'lingo', 'sic', 'uri_metadata'
    entity_type     TEXT,

    -- Review state
    status          TEXT NOT NULL DEFAULT 'pending',
                    -- 'pending'   = engine suggestion, not yet reviewed
                    -- 'confirmed' = reviewer approved match/grouping
                    -- 'rejected'  = reviewer rejected, vars stay as docvars
                    -- 'promoted'  = entity created/updated, bond data patched

    reviewer_note   TEXT,

    -- Source of the suggestion
    suggested_by    TEXT NOT NULL DEFAULT 'engine'
                    -- 'engine', 'reviewer', 'backprop'
);

-- All groups for a document
CREATE INDEX idx_groups_doc ON docvar_groups (doc_id);
-- Find groups by suggested entity
CREATE INDEX idx_groups_suggested ON docvar_groups (suggested_id);
-- Filter by status (review queue)
CREATE INDEX idx_groups_status ON docvar_groups (status);

-- ============================================================================
-- 2. Add var_category and group_id to pbm_docvars
-- ============================================================================

-- Engine-assigned classification
-- 'proper'       = Label token or non-positional capitalization (entity candidate)
-- 'lingo'        = unrecognized lowercase (author vocabulary)
-- 'sic'          = non-word pattern preserved as-is (numbers, mixed chars)
-- 'uri_metadata' = URI/structured ref routed to provenance
ALTER TABLE pbm_docvars ADD COLUMN var_category TEXT;

-- Group membership (NULL = ungrouped)
ALTER TABLE pbm_docvars ADD COLUMN group_id INTEGER REFERENCES docvar_groups(id);

-- Find all vars in a group
CREATE INDEX idx_docvars_group ON pbm_docvars (group_id);
-- Filter vars by category
CREATE INDEX idx_docvars_category ON pbm_docvars (var_category);

-- ============================================================================
-- Verify
-- ============================================================================

SELECT tablename FROM pg_tables WHERE schemaname = 'public' ORDER BY tablename;

COMMIT;
