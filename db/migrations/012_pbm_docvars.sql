-- HCP Migration 012: Document-local var tokens in PBM metadata
-- Target: hcp_fic_pbm
-- Depends on: 011 (PBM prefix tree)
--
-- Document-local vars: tokens specific to a single document that don't
-- warrant a concept token in the main vocabulary. Examples: edition numbers,
-- web addresses, OCR artifacts, formatting anomalies.
--
-- Var notation: DECIMAL pairs (e.g. 01.03, 42.07) — visually distinct from
-- base-50 pair token IDs (AA.AB.AC). Two-pair format gives 00.00–99.99 = 10,000
-- slots per document. Engine zero-pads leading 00 pairs to match token width
-- (e.g. 01.03 → 00.00.00.01.03 for 5-pair pipeline).
--
-- Lifecycle:
--   1. Kernel hits unrecognized text during PBM generation
--   2. Checks pbm_docvars for existing surface match → reuse if found
--   3. Otherwise assigns next consecutive decimal ID, writes entry
--   4. After review: resolved vars get deleted (patched to real token_ids
--      in bond data), gaps in sequence are fine
--   5. Survivors = permanent document-specific var tokens
--
-- No interaction with the main var DB (hcp_var) during PBM generation.
-- The var DB remains the engine's runtime scratch pad.

BEGIN;

-- ============================================================================
-- 1. pbm_docvars — Document-local var token definitions
-- ============================================================================
-- Full token entries scoped to a single document. The decimal ID appears
-- in bond data like any other token_id — the definition lives here.

CREATE TABLE pbm_docvars (
    doc_id      INTEGER NOT NULL REFERENCES pbm_documents(id),
    var_id      TEXT NOT NULL,               -- decimal pair, e.g. '01.03'
    surface     TEXT NOT NULL,               -- original text (reproduction key)
    gloss       TEXT,                        -- reviewer note, e.g. 'OCR artifact', '3rd edition marker'

    PRIMARY KEY (doc_id, var_id),
    CONSTRAINT pbm_docvars_surface_unique UNIQUE (doc_id, surface)
);

-- Find all vars for a document (bulk load)
CREATE INDEX idx_docvars_doc ON pbm_docvars (doc_id);

-- Reverse lookup: find which documents contain a specific surface form
-- (useful for cross-document var loading in future)
CREATE INDEX idx_docvars_surface ON pbm_docvars (surface);

-- ============================================================================
-- 2. Helper: mint_docvar — Atomic mint-or-return for document-local vars
-- ============================================================================
-- Kernel calls this during PBM generation. Returns existing decimal ID
-- if surface form already registered for this document, otherwise mints
-- the next consecutive ID.
--
-- Uses per-document sequence tracking via the max existing var_id.

CREATE OR REPLACE FUNCTION mint_docvar(p_doc_id INTEGER, p_surface TEXT)
RETURNS TEXT AS $$
DECLARE
    v_id TEXT;
    v_max INTEGER;
    v_next INTEGER;
BEGIN
    -- Check for existing var with this surface form in this document
    SELECT var_id INTO v_id
    FROM pbm_docvars
    WHERE doc_id = p_doc_id AND surface = p_surface;

    IF v_id IS NOT NULL THEN
        RETURN v_id;
    END IF;

    -- Find current max var_id for this document
    -- Decimal pairs: '01.03' → parse as integer (1 * 100 + 3 = 103)
    SELECT COALESCE(MAX(
        SPLIT_PART(var_id, '.', 1)::INTEGER * 100 +
        SPLIT_PART(var_id, '.', 2)::INTEGER
    ), -1) INTO v_max
    FROM pbm_docvars
    WHERE doc_id = p_doc_id;

    v_next := v_max + 1;

    -- Format as two-pair decimal: 0 → '00.00', 103 → '01.03', 9999 → '99.99'
    v_id := LPAD((v_next / 100)::TEXT, 2, '0') || '.' ||
            LPAD((v_next % 100)::TEXT, 2, '0');

    INSERT INTO pbm_docvars (doc_id, var_id, surface)
    VALUES (p_doc_id, v_id, p_surface);

    RETURN v_id;
END;
$$ LANGUAGE plpgsql;

-- ============================================================================
-- 3. pbm_var_bonds — Var pairings (B = decimal XX.YY)
-- ============================================================================
-- Decimal vars have no implicit prefix — store full var_id as-is.
-- Same structure as other bond subtables: starter_id FK, B-side columns, count.

CREATE TABLE pbm_var_bonds (
    starter_id  INTEGER NOT NULL REFERENCES pbm_starters(id),
    b_var_id    TEXT NOT NULL,           -- full decimal var_id, e.g. '01.03'
    count       INTEGER NOT NULL,

    PRIMARY KEY (starter_id, b_var_id)
);

-- Reverse lookup: find all starters bonded to a specific var
CREATE INDEX idx_var_bonds_b ON pbm_var_bonds (b_var_id);

-- ============================================================================
-- Verify
-- ============================================================================

SELECT tablename FROM pg_tables WHERE schemaname = 'public' ORDER BY tablename;

COMMIT;
