-- HCP Migration: Shared helper functions
-- Install in each target database BEFORE running shard-specific migrations.
--
-- Usage:
--   psql -d hcp_core    -f 000_helpers.sql
--   psql -d hcp_english -f 000_helpers.sql
--   psql -d hcp_names   -f 000_helpers.sql

-- ============================================================================
-- split_token_id(TEXT) → (ns, p2, p3, p4, p5)
-- ============================================================================
-- Decomposes a dotted token_id string into five TEXT components.
-- Variable-depth tokens (1-5 pairs) are supported; unused slots return NULL.
-- Each pair is always exactly 2 characters (base-50 alphabet).
--
-- Examples:
--   split_token_id('AA.AA.AA.AA.Ak') → (AA, AA, AA, AA, Ak)
--   split_token_id('yA.Ap.Jj')       → (yA, Ap, Jj, NULL, NULL)
--   split_token_id('AB')             → (AB, NULL, NULL, NULL, NULL)

CREATE OR REPLACE FUNCTION split_token_id(
    tid TEXT,
    OUT ns TEXT,
    OUT p2 TEXT,
    OUT p3 TEXT,
    OUT p4 TEXT,
    OUT p5 TEXT
) AS $$
DECLARE
    parts TEXT[];
    n INTEGER;
BEGIN
    IF tid IS NULL THEN
        RETURN;
    END IF;

    parts := string_to_array(tid, '.');
    n := array_length(parts, 1);

    IF n < 1 OR n > 5 THEN
        RAISE EXCEPTION 'Invalid token_id format: "%" (expected 1-5 dot-separated pairs)', tid;
    END IF;

    ns := parts[1];
    IF n >= 2 THEN p2 := parts[2]; END IF;
    IF n >= 3 THEN p3 := parts[3]; END IF;
    IF n >= 4 THEN p4 := parts[4]; END IF;
    IF n >= 5 THEN p5 := parts[5]; END IF;
END;
$$ LANGUAGE plpgsql IMMUTABLE STRICT;


-- ============================================================================
-- join_token_id(ns, p2, p3, p4, p5) → TEXT
-- ============================================================================
-- Reconstructs a dotted token_id from components. Inverse of split_token_id.
-- NULL components are skipped.
--
-- Useful for ad-hoc queries; the tokens table uses a GENERATED column instead.

CREATE OR REPLACE FUNCTION join_token_id(
    ns TEXT,
    p2 TEXT,
    p3 TEXT,
    p4 TEXT,
    p5 TEXT
) RETURNS TEXT AS $$
BEGIN
    RETURN ns ||
        COALESCE('.' || p2, '') ||
        COALESCE('.' || p3, '') ||
        COALESCE('.' || p4, '') ||
        COALESCE('.' || p5, '');
END;
$$ LANGUAGE plpgsql IMMUTABLE;
