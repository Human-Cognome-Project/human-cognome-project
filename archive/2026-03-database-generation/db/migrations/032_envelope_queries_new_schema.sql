-- Migration 032: Update envelope_queries to use new hcp_english schema
--
-- Replaces two kinds of legacy queries in hcp_core.envelope_queries:
--
-- 1. apply_inflection() function calls (queries 20-25):
--    Old pattern called apply_inflection(t.name, morpheme) which opened a cursor
--    over inflection_rules and ran regex matches per row — O(rules) per call.
--    New pattern JOINs token_morph_rules where strip/add are pre-computed.
--    The JOIN also supersedes:
--      - (tp.morpheme_accept & N) != 0  — implicit: only tokens with a morph_rules
--        row for the given morpheme are returned
--      - NOT EXISTS (SELECT 1 FROM token_variants WHERE morpheme = ...)  — implicit:
--        token_morph_rules rows only exist for regular forms (irregular forms are in
--        token_variants only, not in token_morph_rules)
--
-- 2. Old-column queries (queries 2, 18):
--    Query 18 used proper_common = 'proper' (retired column) — replaced with
--    JOIN token_pos WHERE pos = 'N_PROPER'.
--    Query 2 used category IN ('archaic', 'literary', 'formal') (retired column) —
--    replaced with token_pos.characteristics bitmask:
--      FORMAL=bit0(1), LITERARY=bit5(32), ARCHAIC=bit8(256)

\connect hcp_core

-- ============================================================
-- 1. Short verbs (len 2-4) — PAST, PROGRESSIVE, 3RD_SING
-- ============================================================

UPDATE envelope_queries
SET query_text =
    'SELECT left(t.name, length(t.name) - length(tmr.strip_suffix)) || tmr.add_suffix AS name, '
    't.token_id '
    'FROM tokens t '
    'JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''V_MAIN'' '
    'JOIN token_morph_rules tmr ON tmr.token_id = t.token_id AND tmr.morpheme = ''PAST'' '
    'WHERE length(t.name) BETWEEN 2 AND 4 '
    '  AND t.freq_rank IS NOT NULL '
    'ORDER BY t.freq_rank ASC'
WHERE id = 20;

UPDATE envelope_queries
SET query_text =
    'SELECT left(t.name, length(t.name) - length(tmr.strip_suffix)) || tmr.add_suffix AS name, '
    't.token_id '
    'FROM tokens t '
    'JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''V_MAIN'' '
    'JOIN token_morph_rules tmr ON tmr.token_id = t.token_id AND tmr.morpheme = ''PROGRESSIVE'' '
    'WHERE length(t.name) BETWEEN 2 AND 4 '
    '  AND t.freq_rank IS NOT NULL '
    'ORDER BY t.freq_rank ASC'
WHERE id = 21;

UPDATE envelope_queries
SET query_text =
    'SELECT left(t.name, length(t.name) - length(tmr.strip_suffix)) || tmr.add_suffix AS name, '
    't.token_id '
    'FROM tokens t '
    'JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''V_MAIN'' '
    'JOIN token_morph_rules tmr ON tmr.token_id = t.token_id AND tmr.morpheme = ''3RD_SING'' '
    'WHERE length(t.name) BETWEEN 2 AND 4 '
    '  AND t.freq_rank IS NOT NULL '
    'ORDER BY t.freq_rank ASC'
WHERE id = 22;

-- ============================================================
-- 2. Long verbs/nouns (len 5+) — PAST, PROGRESSIVE, PLURAL
-- ============================================================

UPDATE envelope_queries
SET query_text =
    'SELECT left(t.name, length(t.name) - length(tmr.strip_suffix)) || tmr.add_suffix AS name, '
    't.token_id '
    'FROM tokens t '
    'JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''V_MAIN'' '
    'JOIN token_morph_rules tmr ON tmr.token_id = t.token_id AND tmr.morpheme = ''PAST'' '
    'WHERE length(t.name) >= 5 '
    '  AND t.freq_rank IS NOT NULL '
    '  AND (t.characteristics & 256) = 0 '
    'ORDER BY t.freq_rank ASC LIMIT 5000'
WHERE id = 23;

UPDATE envelope_queries
SET query_text =
    'SELECT left(t.name, length(t.name) - length(tmr.strip_suffix)) || tmr.add_suffix AS name, '
    't.token_id '
    'FROM tokens t '
    'JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''V_MAIN'' '
    'JOIN token_morph_rules tmr ON tmr.token_id = t.token_id AND tmr.morpheme = ''PROGRESSIVE'' '
    'WHERE length(t.name) >= 5 '
    '  AND t.freq_rank IS NOT NULL '
    '  AND (t.characteristics & 256) = 0 '
    'ORDER BY t.freq_rank ASC LIMIT 5000'
WHERE id = 24;

UPDATE envelope_queries
SET query_text =
    'SELECT left(t.name, length(t.name) - length(tmr.strip_suffix)) || tmr.add_suffix AS name, '
    't.token_id '
    'FROM tokens t '
    'JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''N_COMMON'' '
    'JOIN token_morph_rules tmr ON tmr.token_id = t.token_id AND tmr.morpheme = ''PLURAL'' '
    'WHERE length(t.name) >= 5 '
    '  AND t.freq_rank IS NOT NULL '
    '  AND (t.characteristics & 256) = 0 '
    'ORDER BY t.freq_rank ASC LIMIT 5000'
WHERE id = 25;

-- ============================================================
-- 3. Label tier (query 18): proper_common → token_pos N_PROPER
-- ============================================================
-- Old: WHERE proper_common = 'proper' AND canonical_id IS NULL
-- New: JOIN token_pos WHERE pos = 'N_PROPER'
-- Notes:
--   - canonical_id IS NULL was the old variant-exclusion guard;
--     token_pos only holds direct entries, so no equivalent needed.
--   - names in tokens are always lowercase (migration 016); lower() call removed.
--   - N_PROPER population comes from Kaikki Pass 6.

UPDATE envelope_queries
SET query_text =
    'SELECT t.name, t.token_id '
    'FROM tokens t '
    'JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''N_PROPER'' '
    'WHERE length(t.name) BETWEEN 2 AND 16 '
    'ORDER BY t.freq_rank ASC NULLS LAST'
WHERE id = 18;

-- ============================================================
-- 4. Victorian fiction (query 2): category → token_pos.characteristics
-- ============================================================
-- Old: WHERE category IN ('archaic', 'literary', 'formal')
-- New: token_pos.characteristics bitmask — FORMAL=bit0(1), LITERARY=bit5(32), ARCHAIC=bit8(256)
-- EXISTS subquery avoids duplicate rows when a token has multiple PoS entries
-- with the characteristic set; outer ORDER BY freq_rank is preserved.

UPDATE envelope_queries
SET query_text =
    'SELECT t.name, t.token_id '
    'FROM tokens t '
    'WHERE EXISTS ( '
    '    SELECT 1 FROM token_pos tp '
    '    WHERE tp.token_id = t.token_id '
    '      AND (tp.characteristics & (1 | 32 | 256)) != 0 '
    ') '
    'ORDER BY t.freq_rank ASC NULLS LAST '
    'LIMIT 5000'
WHERE id = 2;

-- ============================================================
-- Verify: confirm all 8 rows updated
-- ============================================================

SELECT id, description, left(query_text, 100) AS query_preview
FROM envelope_queries
WHERE id IN (2, 18, 20, 21, 22, 23, 24, 25)
ORDER BY id;
