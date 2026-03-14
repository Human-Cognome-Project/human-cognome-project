-- HCP Migration 034: Per-document position caps table
-- Target: hcp_fic_pbm (and future nf_pbm shards)
-- Depends on: 013 (position storage), pbm_documents
--
-- Design rationale:
--
--   Cap flags serve two distinct consumers:
--
--   1. Token-level (vocab):  Label tokens carry an intrinsic LABEL morph bit
--      indicating the token is a proper noun / title.  This lives in hcp_english
--      and informs semantic analysis — "London" is a Label regardless of context.
--
--   2. Position-level (this table):  Which specific positions in a specific
--      document were written with a capital letter.  This is a reconstruction
--      reference only.  Sentence-initial caps (positional) and Label caps
--      (intrinsic) both appear here — reconstruction doesn't need to distinguish.
--
--   Storing position caps separately:
--     - Keeps pbm_starters lean (no cap columns on every row)
--     - Sparse: only capitalized positions have rows (~15-30% of tokens)
--     - Single SELECT per reconstruct call: build a lookup map, apply during pass
--     - Removes the capitalizeNext / cap-suppression runtime bookkeeping
--
--   The modifier bits on pbm_starters.modifiers currently encode firstCap/allCaps
--   in bits 0-1.  Those bits remain for now (they carry morph data alongside).
--   This table is the clean long-term reference; the modifier bits are a legacy
--   inline encoding that can be stripped once this table is proven.
--
-- Cap types:
--   1 = firstCap  (first character capitalized, rest lowercase)
--   2 = allCaps   (entire token in uppercase)

BEGIN;

-- ============================================================================
-- Position caps table
-- ============================================================================

CREATE TABLE pbm_position_caps (
    id          SERIAL PRIMARY KEY,
    doc_pk      INTEGER NOT NULL REFERENCES pbm_documents(id) ON DELETE CASCADE,
    position    INTEGER NOT NULL,
    cap_type    SMALLINT NOT NULL  -- 1=firstCap, 2=allCaps
);

-- Primary access pattern: load all caps for a document in one pass
CREATE INDEX idx_poscaps_doc ON pbm_position_caps (doc_pk);

-- Uniqueness: at most one cap entry per position per document
CREATE UNIQUE INDEX idx_poscaps_doc_pos ON pbm_position_caps (doc_pk, position);

-- ============================================================================
-- Verify
-- ============================================================================

SELECT tablename FROM pg_tables WHERE schemaname = 'public' ORDER BY tablename;

COMMIT;
