-- HCP Migration 013: Re-add positional tables alongside bond tables
-- Target: hcp_fic_pbm
-- Depends on: 011 (bond tables), 012 (docvars)
--
-- PBM bonds alone can't uniquely reconstruct original text — the Euler path
-- isn't unique when common tokens have many outgoing edges. Both encodings
-- stored side by side:
--   - Bond tables (011/012): inference, aggregation, language model
--   - Position tables (this migration): exact reconstruction
--
-- This may become unnecessary once the conceptual mesh provides stronger
-- disambiguation constraints for graph solvers. For now we need both.
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
-- 1. Add total_slots and unique_tokens to pbm_documents
-- ============================================================================

ALTER TABLE pbm_documents ADD COLUMN total_slots   INTEGER;
ALTER TABLE pbm_documents ADD COLUMN unique_tokens  INTEGER;

-- ============================================================================
-- 2. doc_word_positions — Word tokens (AB.AB.* — ~87% of tokens)
-- ============================================================================
-- Only stores distinguishing segments. Implicit prefix: AB.AB
-- FK to pbm_documents(id) as INTEGER, matching bond table convention.

CREATE TABLE doc_word_positions (
    doc_id      INTEGER NOT NULL REFERENCES pbm_documents(id),
    t_p3        TEXT NOT NULL,
    t_p4        TEXT NOT NULL,
    t_p5        TEXT,
    positions   TEXT NOT NULL,          -- base-50 packed, 4 chars per position

    PRIMARY KEY (doc_id, t_p3, t_p4, t_p5)
);

-- Reverse lookup: find all documents containing a specific word
CREATE INDEX idx_word_pos_token ON doc_word_positions (t_p3, t_p4, t_p5);

-- ============================================================================
-- 3. doc_char_positions — Character/punctuation tokens (AA.* — ~12%)
-- ============================================================================
-- ASCII bytes (AA.AA.AA.AA.*) and Unicode chars (AA.AB.AA.*) share AA prefix.

CREATE TABLE doc_char_positions (
    doc_id      INTEGER NOT NULL REFERENCES pbm_documents(id),
    t_p2        TEXT NOT NULL,
    t_p3        TEXT NOT NULL,
    t_p4        TEXT NOT NULL,
    t_p5        TEXT,
    positions   TEXT NOT NULL,

    PRIMARY KEY (doc_id, t_p2, t_p3, t_p4, t_p5)
);

CREATE INDEX idx_char_pos_token ON doc_char_positions (t_p2, t_p3, t_p4, t_p5);

-- ============================================================================
-- 4. doc_marker_positions — Structural markers (AA.AE.* — ~0.5%)
-- ============================================================================
-- Newlines, structural markers. Implicit prefix: AA.AE

CREATE TABLE doc_marker_positions (
    doc_id      INTEGER NOT NULL REFERENCES pbm_documents(id),
    t_p3        TEXT NOT NULL,
    t_p4        TEXT NOT NULL,
    positions   TEXT NOT NULL,

    PRIMARY KEY (doc_id, t_p3, t_p4)
);

CREATE INDEX idx_marker_pos_token ON doc_marker_positions (t_p3, t_p4);

-- ============================================================================
-- 5. doc_var_positions — Var tokens (decimal pairs — docvars and var DB)
-- ============================================================================
-- Full var_id stored as-is — decimal pairs (e.g. '01.03').
-- Handles both document-local vars (from pbm_docvars) and var DB vars.
-- Both use decimal notation, no ambiguity.

CREATE TABLE doc_var_positions (
    doc_id      INTEGER NOT NULL REFERENCES pbm_documents(id),
    var_id      TEXT NOT NULL,
    positions   TEXT NOT NULL,

    PRIMARY KEY (doc_id, var_id)
);

CREATE INDEX idx_var_pos_varid ON doc_var_positions (var_id);

-- ============================================================================
-- Verify
-- ============================================================================

SELECT tablename FROM pg_tables WHERE schemaname = 'public' ORDER BY tablename;

COMMIT;
