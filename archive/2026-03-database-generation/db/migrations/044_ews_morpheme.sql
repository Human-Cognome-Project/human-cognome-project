-- Migration 044: Propagate variant morpheme bits through warm cache pipeline.
-- Adds morpheme column to envelope_working_set so token_variants like "stood"→PAST
-- carry their morph bit all the way into LMDB, resolution, and reconstruction.
--
-- Root cause: query 26 put "stood"→stand_token_id in the warm set (and LMDB)
-- but with no morpheme information, so reconstruction showed "stand" instead of "stood".
-- Fix: include tv.morpheme in the SELECT so AssembleQuery can store it in the warm set,
-- and the engine's LMDB value format carries it through to ResolutionResult.morphBits.

-- 1. Add morpheme column to warm working set
ALTER TABLE envelope_working_set ADD COLUMN IF NOT EXISTS morpheme TEXT NULL;

-- 2. Update query 26 (token_variants → warm set, in hcp_envelope) to include tv.morpheme.
--    Run this statement against hcp_envelope:
--    UPDATE envelope_queries SET query_text = '...' WHERE id = 26;
--    (Applied manually — envelope_queries lives in hcp_envelope, not hcp_var.)
--
-- Statement to run against hcp_envelope:
--   UPDATE envelope_queries
--      SET query_text =
--   'SELECT tv.name, tv.canonical_id AS token_id, tv.morpheme
--    FROM token_variants tv
--    WHERE tv.morpheme IS NOT NULL
--      AND length(tv.name) BETWEEN 2 AND 16
--    ORDER BY tv.name ASC'
--    WHERE id = 26;
