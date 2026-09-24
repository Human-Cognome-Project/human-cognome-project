-- HCP Migration 010: Create hcp_var database schema
-- Target: hcp_var (new database — must be created first)
-- Depends on: 009 (position storage)
--
-- The var DB is the DI's general-purpose short-term memory cache.
-- Any unresolved sequence routes here via the <var> cache miss pipeline.
-- Var tokens use 'var.<arbitrary>' IDs — no decomposed hierarchy.
--
-- Key properties:
--   - Flexible by design — minimal constraints, DI organizes later
--   - var_tokens: real tokens usable in position streams
--   - var_sources: update index for librarian promotion workflow
--   - Unique on surface form (active only) — no duplicate minting

BEGIN;

-- ============================================================================
-- 1. var_tokens — Core var token registry
-- ============================================================================

CREATE TABLE var_tokens (
    var_id      TEXT PRIMARY KEY,       -- 'var.<arbitrary>'
    form        TEXT NOT NULL,          -- Original surface text
    atomization JSONB,                  -- DI's breakdown (unconstrained format)
    status      TEXT NOT NULL DEFAULT 'active',
                                       -- 'active', 'promoted', 'retired', 'merged'
    promoted_to TEXT,                   -- Permanent token_id or winner var_id
    metadata    JSONB NOT NULL DEFAULT '{}',
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at  TIMESTAMPTZ NOT NULL DEFAULT now()
);

-- Same surface form → same var token (active only)
-- Partial index releases form on retirement/promotion for re-use
CREATE UNIQUE INDEX idx_var_form ON var_tokens (form) WHERE status = 'active';

CREATE INDEX idx_var_status ON var_tokens (status);
CREATE INDEX idx_var_created ON var_tokens (created_at);

-- ============================================================================
-- 2. var_id_seq — Sequential ID minting
-- ============================================================================
-- DI can adopt a richer scheme later — suffix is opaque outside var DB.

CREATE SEQUENCE var_id_seq;

-- ============================================================================
-- 3. mint_var_token — Atomic mint-or-return function
-- ============================================================================
-- Cache miss pipeline calls this. Checks for existing active token first
-- (handles LMDB eviction case — var token exists but LMDB entry was purged).

CREATE OR REPLACE FUNCTION mint_var_token(p_form TEXT)
RETURNS TEXT AS $$
DECLARE
    v_id TEXT;
BEGIN
    -- Check for existing active var token with this surface form
    SELECT var_id INTO v_id
    FROM var_tokens
    WHERE form = p_form AND status = 'active';

    IF v_id IS NOT NULL THEN
        RETURN v_id;
    END IF;

    -- Mint new var token
    v_id := 'var.' || nextval('var_id_seq')::TEXT;
    INSERT INTO var_tokens (var_id, form)
    VALUES (v_id, p_form);
    RETURN v_id;
END;
$$ LANGUAGE plpgsql;

-- ============================================================================
-- 4. var_sources — Update index for librarian promotion
-- ============================================================================
-- Tracks every (doc_id, position) where a var token appears.
-- On promotion: query var_sources WHERE var_id = X → get exact locations to patch.

CREATE TABLE var_sources (
    id          SERIAL PRIMARY KEY,
    var_id      TEXT NOT NULL REFERENCES var_tokens(var_id),
    doc_id      TEXT NOT NULL,          -- Document token_id (from document shards)
    position    INTEGER NOT NULL,       -- Position in document stream
    context     TEXT,                   -- Surrounding text for disambiguation
    seen_at     TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX idx_varsrc_var ON var_sources (var_id);
CREATE INDEX idx_varsrc_doc ON var_sources (doc_id);

-- ============================================================================
-- Verify
-- ============================================================================

SELECT tablename FROM pg_tables WHERE schemaname = 'public' ORDER BY tablename;

COMMIT;
