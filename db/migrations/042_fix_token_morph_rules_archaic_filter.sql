-- Migration 042: Fix token_morph_rules — archaic variants should not block regular forms
--
-- Problem (from migration 041): the NOT EXISTS filter used:
--   WHERE tv.canonical_id = t.token_id AND tv.morpheme = 'PAST'
-- This correctly blocked tokens with canonical irregular replacements (ran, made,
-- went) but also blocked tokens that only have ARCHAIC or DIALECT supplementary
-- forms (walk'd, walkt, likedst, tapt, etc.). Those archaic forms don't replace
-- "walked" or "liked" — they supplement them. So those tokens got no PAST row
-- in token_morph_rules, leaving their regular inflected forms out of envelopes.
--
-- Fix: the NOT EXISTS check should only block when the variant is a NON-ARCHAIC,
-- NON-DATED, NON-DIALECT form — i.e., a canonical irregular replacement with no
-- temporal/geographic modifier bits set:
--
--   characteristics = IRREGULAR only (2097152):  canonical replacement → block
--   characteristics = IRREGULAR | ARCHAIC (2097408): supplement → do NOT block
--   characteristics = IRREGULAR | DIALECT (2101248): supplement → do NOT block
--
-- This migration adds the missing rows. The ON CONFLICT DO NOTHING clause means
-- tokens already present from migration 041 are untouched.
--
-- Exclusion mask: bits 8 (ARCHAIC=256), 9 (DATED=512), 12 (DIALECT=4096).
-- A variant blocks the morph_rule only if NONE of these bits are set.

\connect hcp_english

BEGIN;

-- ── PAST ─────────────────────────────────────────────────────────────────────
INSERT INTO token_morph_rules (token_id, morpheme, rule_id, strip_suffix, add_suffix)
SELECT DISTINCT t.token_id, 'PAST', mf.rule_id, mf.strip_suffix, mf.add_suffix
FROM tokens t
JOIN token_pos tp ON tp.token_id = t.token_id
    AND tp.pos IN ('V_MAIN', 'V_AUX', 'V_COPULA')
    AND (tp.morpheme_accept & 2) != 0
CROSS JOIN LATERAL (SELECT * FROM match_inflection_rule(t.name, 'PAST')) AS mf
WHERE NOT EXISTS (
    SELECT 1 FROM token_variants tv
    WHERE tv.canonical_id = t.token_id
      AND tv.morpheme = 'PAST'
      AND (tv.characteristics & (256 | 512 | 4096)) = 0  -- not archaic/dated/dialect
)
ON CONFLICT (token_id, morpheme) DO NOTHING;

-- ── PROGRESSIVE ──────────────────────────────────────────────────────────────
INSERT INTO token_morph_rules (token_id, morpheme, rule_id, strip_suffix, add_suffix)
SELECT DISTINCT t.token_id, 'PROGRESSIVE', mf.rule_id, mf.strip_suffix, mf.add_suffix
FROM tokens t
JOIN token_pos tp ON tp.token_id = t.token_id
    AND tp.pos IN ('V_MAIN', 'V_AUX', 'V_COPULA')
    AND (tp.morpheme_accept & 4) != 0
CROSS JOIN LATERAL (SELECT * FROM match_inflection_rule(t.name, 'PROGRESSIVE')) AS mf
WHERE NOT EXISTS (
    SELECT 1 FROM token_variants tv
    WHERE tv.canonical_id = t.token_id
      AND tv.morpheme = 'PROGRESSIVE'
      AND (tv.characteristics & (256 | 512 | 4096)) = 0
)
ON CONFLICT (token_id, morpheme) DO NOTHING;

-- ── 3RD_SING ─────────────────────────────────────────────────────────────────
INSERT INTO token_morph_rules (token_id, morpheme, rule_id, strip_suffix, add_suffix)
SELECT DISTINCT t.token_id, '3RD_SING', mf.rule_id, mf.strip_suffix, mf.add_suffix
FROM tokens t
JOIN token_pos tp ON tp.token_id = t.token_id
    AND tp.pos IN ('V_MAIN', 'V_AUX', 'V_COPULA')
    AND (tp.morpheme_accept & 8) != 0
CROSS JOIN LATERAL (SELECT * FROM match_inflection_rule(t.name, '3RD_SING')) AS mf
WHERE NOT EXISTS (
    SELECT 1 FROM token_variants tv
    WHERE tv.canonical_id = t.token_id
      AND tv.morpheme = '3RD_SING'
      AND (tv.characteristics & (256 | 512 | 4096)) = 0
)
ON CONFLICT (token_id, morpheme) DO NOTHING;

-- ── PLURAL ───────────────────────────────────────────────────────────────────
INSERT INTO token_morph_rules (token_id, morpheme, rule_id, strip_suffix, add_suffix)
SELECT DISTINCT t.token_id, 'PLURAL', mf.rule_id, mf.strip_suffix, mf.add_suffix
FROM tokens t
JOIN token_pos tp ON tp.token_id = t.token_id
    AND tp.pos = 'N_COMMON'
    AND (tp.morpheme_accept & 1) != 0
CROSS JOIN LATERAL (SELECT * FROM match_inflection_rule(t.name, 'PLURAL')) AS mf
WHERE NOT EXISTS (
    SELECT 1 FROM token_variants tv
    WHERE tv.canonical_id = t.token_id
      AND tv.morpheme = 'PLURAL'
      AND (tv.characteristics & (256 | 512 | 4096)) = 0
)
ON CONFLICT (token_id, morpheme) DO NOTHING;

-- ── COMPARATIVE ──────────────────────────────────────────────────────────────
INSERT INTO token_morph_rules (token_id, morpheme, rule_id, strip_suffix, add_suffix)
SELECT DISTINCT t.token_id, 'COMPARATIVE', mf.rule_id, mf.strip_suffix, mf.add_suffix
FROM tokens t
JOIN token_pos tp ON tp.token_id = t.token_id
    AND tp.pos = 'ADJ'
    AND (tp.morpheme_accept & 16) != 0
CROSS JOIN LATERAL (SELECT * FROM match_inflection_rule(t.name, 'COMPARATIVE')) AS mf
WHERE NOT EXISTS (
    SELECT 1 FROM token_variants tv
    WHERE tv.canonical_id = t.token_id
      AND tv.morpheme = 'COMPARATIVE'
      AND (tv.characteristics & (256 | 512 | 4096)) = 0
)
ON CONFLICT (token_id, morpheme) DO NOTHING;

-- ── SUPERLATIVE ──────────────────────────────────────────────────────────────
INSERT INTO token_morph_rules (token_id, morpheme, rule_id, strip_suffix, add_suffix)
SELECT DISTINCT t.token_id, 'SUPERLATIVE', mf.rule_id, mf.strip_suffix, mf.add_suffix
FROM tokens t
JOIN token_pos tp ON tp.token_id = t.token_id
    AND tp.pos = 'ADJ'
    AND (tp.morpheme_accept & 32) != 0
CROSS JOIN LATERAL (SELECT * FROM match_inflection_rule(t.name, 'SUPERLATIVE')) AS mf
WHERE NOT EXISTS (
    SELECT 1 FROM token_variants tv
    WHERE tv.canonical_id = t.token_id
      AND tv.morpheme = 'SUPERLATIVE'
      AND (tv.characteristics & (256 | 512 | 4096)) = 0
)
ON CONFLICT (token_id, morpheme) DO NOTHING;

-- ── Verification ─────────────────────────────────────────────────────────────

SELECT morpheme, count(*) AS rows
FROM token_morph_rules
GROUP BY morpheme
ORDER BY rows DESC;

SELECT
    (SELECT strip_suffix = '' AND add_suffix = 'ed'
     FROM token_morph_rules tmr
     JOIN tokens t ON t.token_id = tmr.token_id
     WHERE t.name = 'walk' AND tmr.morpheme = 'PAST')          AS walk_past_ok,

    (SELECT strip_suffix = '' AND add_suffix = 'ped'
     FROM token_morph_rules tmr
     JOIN tokens t ON t.token_id = tmr.token_id
     WHERE t.name = 'tap' AND tmr.morpheme = 'PAST')           AS tap_past_ok,

    (SELECT strip_suffix = 'e' AND add_suffix = 'ed'
     FROM token_morph_rules tmr
     JOIN tokens t ON t.token_id = tmr.token_id
     WHERE t.name = 'like' AND tmr.morpheme = 'PAST')          AS like_past_ok,

    -- make has irregular "made" (pure IRREGULAR, no archaic) → should NOT have morph_rule
    NOT EXISTS (
        SELECT 1 FROM token_morph_rules tmr
        JOIN tokens t ON t.token_id = tmr.token_id
        WHERE t.name = 'make' AND tmr.morpheme = 'PAST'
    )                                                           AS make_no_morph_rule_ok,

    -- run: irregular "ran" → no morph_rule
    NOT EXISTS (
        SELECT 1 FROM token_morph_rules tmr
        JOIN tokens t ON t.token_id = tmr.token_id
        WHERE t.name = 'run' AND tmr.morpheme = 'PAST'
    )                                                           AS run_no_morph_rule_ok,

    -- ran still in token_variants
    EXISTS (
        SELECT 1 FROM token_variants tv
        JOIN tokens t ON t.token_id = tv.canonical_id
        WHERE t.name = 'run' AND tv.name = 'ran' AND tv.morpheme = 'PAST'
    )                                                           AS ran_still_in_variants;

COMMIT;
