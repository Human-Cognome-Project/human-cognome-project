-- Migration 041: Split positional modifiers out of pbm_starters
--
-- pbm_starters is the PBM meaning map — token positions only.
-- Morpheme and cap overlays move to pbm_morpheme_positions (sparse, per-doc).
-- Reconstruction: place tokens first, then apply each morpheme list as an
-- independent pass (parallelizable on GPU for large texts).
--
-- inflection_rules in hcp_english is the rule source — engine loads at startup.

\connect hcp_fic_pbm

BEGIN;

-- Drop the modifier column from the positional table
ALTER TABLE pbm_starters DROP COLUMN IF EXISTS modifiers;

-- Sparse morpheme/cap overlay lists, one row per (doc, morpheme)
CREATE TABLE pbm_morpheme_positions (
    id          serial      PRIMARY KEY,
    doc_id      integer     NOT NULL REFERENCES pbm_documents(id) ON DELETE CASCADE,
    morpheme    text        NOT NULL,   -- 'PAST','PLURAL','PROG','3RD','POSS','NEG',
                                        -- 'WILL','HAVE','BE','AM','COND',
                                        -- 'FIRST_CAP','ALL_CAPS'
    positions   text        NOT NULL,   -- base-50 packed, 4 chars per position
    UNIQUE (doc_id, morpheme)
);

CREATE INDEX idx_morph_pos_doc ON pbm_morpheme_positions (doc_id);

ALTER TABLE pbm_morpheme_positions OWNER TO hcp;
ALTER SEQUENCE pbm_morpheme_positions_id_seq OWNER TO hcp;
GRANT ALL ON pbm_morpheme_positions TO PUBLIC;
GRANT USAGE, SELECT ON SEQUENCE pbm_morpheme_positions_id_seq TO PUBLIC;

COMMIT;
