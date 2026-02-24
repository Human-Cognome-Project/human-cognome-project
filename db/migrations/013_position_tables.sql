-- HCP Migration 013: Position data on starters + document stats
-- Target: hcp_fic_pbm
-- Depends on: 011 (bond tables), 012 (docvars)
--
-- PBM bonds alone can't uniquely reconstruct original text — the Euler path
-- isn't unique when common tokens have many outgoing edges. Both encodings
-- stored side by side:
--   - Bond tables (011/012): inference, aggregation, language model
--   - Positions on starters (this migration): exact reconstruction
--
-- Key insight: pbm_starters already IS the unique token inventory per
-- document. Adding a positions column there eliminates the need for
-- separate position subtables entirely.
--
-- Coverage: every token that bonds forward has a starter row. Three layers
-- ensure the final token is always covered:
--   1. Most documents end with punctuation — already a starter from earlier
--   2. stream_end marker bond anchors the sequence terminus
--   3. Fallback: total_slots on pbm_documents = last position + 1
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
-- 2. Add positions column to pbm_starters
-- ============================================================================
-- Base-50 packed position list. Every position where this token appears
-- in the document. 4 chars per position, concatenated.
-- NULL until position data is written (bonds can exist without positions).

ALTER TABLE pbm_starters ADD COLUMN positions TEXT;

-- ============================================================================
-- Verify
-- ============================================================================

SELECT tablename FROM pg_tables WHERE schemaname = 'public' ORDER BY tablename;

COMMIT;
