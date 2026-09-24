-- Migration 049: positions as INTEGER[] on pbm_starters
--
-- Supersedes 048. Patrick's correct mental model:
--   "for each token_id (defined by the arrayed structure for compression),
--    it lives at these points."
--
-- One row per (doc, token). Positions live as INTEGER[] on the same row.
-- Postgres stores int arrays compactly (~4 bytes per element) without the
-- per-row tuple-header overhead that killed the row-per-position approach
-- in 048.
--
-- Caps: only ALL_CAPS needs storage. FIRST_CAP is positional — TokenIdsToText
-- already capitalizes after . ? ! \n via capitalizeNext, and Label tokens
-- carry their intrinsic cap in entries.word ("John" not "john"). Stored as
-- nominal INTEGER[] on pbm_documents.
--
-- Apply against: hcp_fic_pbm.
-- ASSUMES: re-ingest of texts after this migration (9 docs, ~25 min wall).

\set ON_ERROR_STOP on

BEGIN;

-- Drop the row-per-position tables from 048
DROP TABLE IF EXISTS pbm_positions;
DROP TABLE IF EXISTS pbm_cap_flags;

-- Truncate everything else for clean re-ingest. CASCADE handles FK order.
TRUNCATE TABLE
    pbm_word_bonds,
    pbm_char_bonds,
    pbm_marker_bonds,
    pbm_var_bonds,
    pbm_starters,
    pbm_docvars,
    document_provenance,
    document_relationships,
    docvar_groups,
    pbm_documents
RESTART IDENTITY CASCADE;

-- Add positions INTEGER[] column to starters
ALTER TABLE pbm_starters
    ADD COLUMN positions INTEGER[] NOT NULL DEFAULT '{}';

-- Add nominal all_caps_positions INTEGER[] to documents
ALTER TABLE pbm_documents
    ADD COLUMN all_caps_positions INTEGER[] NOT NULL DEFAULT '{}';

COMMIT;
