-- Migration 041: Fix inflection_rules, token_morph_rules, and token_variants
--
-- Root cause: inflection_rules was never populated after schema creation.
-- Cascade of failures:
--   1. pass4_insert_variants.py: compute_regular_form() returned None for all inputs
--      because the table was empty. The filter (if expected is None: skip) never
--      fired → every form-of entry was inserted as IRREGULAR into token_variants,
--      including regular forms like "walked", "cats", "bigger".
--   2. pass2_insert_token_pos.py: token_morph_rules population depended on
--      inflection_rules for rule matching → produced nothing.
--   3. envelope_queries 20-25 (migration 032): all JOIN token_morph_rules →
--      return zero rows → inflected forms missing from LMDB.
--
-- Fix (three parts, single transaction):
--   Part 1 — Populate inflection_rules with standard English morphological rules.
--   Part 2 — Delete regular forms from token_variants using apply_inflection().
--             Truly irregular forms (ran, mice, went, etc.) don't match any rule
--             and remain. Null-morpheme archaic/dialect rows are untouched.
--   Part 3 — Populate token_morph_rules for all regular (token, morpheme) pairs.
--             Pre-computes strip/add so envelope assembly is a plain JOIN.
--             Doubling (tap→tapped, run→running) is absorbed into add_suffix.
--
-- Note on kernel hard-coding: the C++ engine currently mirrors these rules in
-- TryInflectionStrip (HCPVocabBed.cpp). That should eventually read from this
-- table so the engine is language-independent. Flagged as future engine work.
--
-- Expected runtime: Part 2 ~1-3 min (312K deletions via apply_inflection cursor).
--                   Part 3 ~5-15 min (1.5M+ token_morph_rules inserts).

\connect hcp_english

BEGIN;

-- ====================================================================
-- Part 1: Populate inflection_rules
-- ====================================================================

INSERT INTO inflection_rules
    (morpheme, priority, condition, strip_suffix, add_suffix, description)
VALUES

-- ── PAST ─────────────────────────────────────────────────────────────
-- Priority 1: silent-e drop  (like→liked, race→raced, love→loved)
('PAST', 1,  '[^aeiou]e$',          'e',            'ed',   'silent-e drop: like→liked'),
-- Priority 2: CVC doubling   (tap→tapped, pin→pinned — apply_doubling_rule handles
--             digraph suppression, unstressed-final-syllable suppression)
--             Tokens where doubling is wrong (begin, occur) have explicit irregular
--             variants in token_variants and are excluded by NOT EXISTS in Part 3.
('PAST', 2,  '[aeiou][bdfgmnprt]$', '__DOUBLING__', 'ed',   'CVC doubling: tap→tapped'),
-- Priority 99: default +ed   (walk→walked, push→pushed, finish→finished)
('PAST', 99, '.*',                  '',             'ed',   'default: walk→walked'),

-- ── PROGRESSIVE ──────────────────────────────────────────────────────
('PROGRESSIVE', 1,  '[^aeiou]e$',          'e',            'ing',  'silent-e drop: make→making'),
('PROGRESSIVE', 2,  '[aeiou][bdfgmnprt]$', '__DOUBLING__', 'ing',  'CVC doubling: tap→tapping'),
('PROGRESSIVE', 99, '.*',                  '',             'ing',  'default: walk→walking'),

-- ── PLURAL ───────────────────────────────────────────────────────────
-- (words ending in -f/-fe → -ves are irregular: knife→knives, leaf→leaves — in token_variants)
('PLURAL', 1,  '(s|x|z|ch|sh)$', '',  'es',  'sibilant: kiss→kisses, watch→watches'),
('PLURAL', 2,  '[^aeiou]y$',      'y', 'ies', 'consonant+y: city→cities'),
('PLURAL', 99, '.*',              '',  's',   'default: cat→cats'),

-- ── 3RD_SING ─────────────────────────────────────────────────────────
-- Include verbs ending in -o (go→goes, do→does, echo→echoes)
('3RD_SING', 1,  '(s|x|z|ch|sh|o)$', '',  'es',  'sibilant/o: kiss→kisses, go→goes'),
('3RD_SING', 2,  '[^aeiou]y$',        'y', 'ies', 'consonant+y: fly→flies'),
('3RD_SING', 99, '.*',                '',  's',   'default: walk→walks'),

-- ── COMPARATIVE ──────────────────────────────────────────────────────
('COMPARATIVE', 1,  '[^aeiou]e$',          'e',            'er',  'silent-e: nice→nicer'),
('COMPARATIVE', 2,  '[aeiou][bdfgmnprt]$', '__DOUBLING__', 'er',  'CVC: big→bigger, sad→sadder'),
('COMPARATIVE', 3,  '[^aeiou]y$',          'y',            'ier', 'consonant+y: happy→happier'),
('COMPARATIVE', 99, '.*',                  '',             'er',  'default: fast→faster'),

-- ── SUPERLATIVE ──────────────────────────────────────────────────────
('SUPERLATIVE', 1,  '[^aeiou]e$',          'e',            'est',  'silent-e: nice→nicest'),
('SUPERLATIVE', 2,  '[aeiou][bdfgmnprt]$', '__DOUBLING__', 'est',  'CVC: big→biggest'),
('SUPERLATIVE', 3,  '[^aeiou]y$',          'y',            'iest', 'consonant+y: happy→happiest'),
('SUPERLATIVE', 99, '.*',                  '',             'est',  'default: fast→fastest'),

-- ── ADVERB_LY ────────────────────────────────────────────────────────
('ADVERB_LY', 1,  '[^aeiou]y$', 'y',  'ily', 'consonant+y: happy→happily'),
('ADVERB_LY', 2,  'le$',        'le', 'ly',  'le-drop: simple→simply, gentle→gently'),
('ADVERB_LY', 99, '.*',         '',   'ly',  'default: quick→quickly');

-- Smoke test — all must be true before proceeding
DO $$
BEGIN
    ASSERT apply_inflection('walk',   'PAST')        = 'walked',   'walk PAST';
    ASSERT apply_inflection('like',   'PAST')        = 'liked',    'like PAST';
    ASSERT apply_inflection('tap',    'PAST')        = 'tapped',   'tap PAST';
    ASSERT apply_inflection('open',   'PAST')        = 'opened',   'open PAST (no double)';
    ASSERT apply_inflection('rain',   'PAST')        = 'rained',   'rain PAST (digraph, no double)';
    ASSERT apply_inflection('run',    'PROGRESSIVE') = 'running',  'run PROGRESSIVE';
    ASSERT apply_inflection('make',   'PROGRESSIVE') = 'making',   'make PROGRESSIVE';
    ASSERT apply_inflection('walk',   'PROGRESSIVE') = 'walking',  'walk PROGRESSIVE';
    ASSERT apply_inflection('cat',    'PLURAL')      = 'cats',     'cat PLURAL';
    ASSERT apply_inflection('city',   'PLURAL')      = 'cities',   'city PLURAL';
    ASSERT apply_inflection('kiss',   'PLURAL')      = 'kisses',   'kiss PLURAL';
    ASSERT apply_inflection('go',     '3RD_SING')    = 'goes',     'go 3RD_SING';
    ASSERT apply_inflection('fly',    '3RD_SING')    = 'flies',    'fly 3RD_SING';
    ASSERT apply_inflection('walk',   '3RD_SING')    = 'walks',    'walk 3RD_SING';
    ASSERT apply_inflection('big',    'COMPARATIVE') = 'bigger',   'big COMPARATIVE';
    ASSERT apply_inflection('happy',  'COMPARATIVE') = 'happier',  'happy COMPARATIVE';
    ASSERT apply_inflection('fast',   'COMPARATIVE') = 'faster',   'fast COMPARATIVE';
    ASSERT apply_inflection('big',    'SUPERLATIVE') = 'biggest',  'big SUPERLATIVE';
    ASSERT apply_inflection('happy',  'SUPERLATIVE') = 'happiest', 'happy SUPERLATIVE';
    ASSERT apply_inflection('simple', 'ADVERB_LY')   = 'simply',   'simple ADVERB_LY';
    ASSERT apply_inflection('quick',  'ADVERB_LY')   = 'quickly',  'quick ADVERB_LY';
    ASSERT apply_inflection('happy',  'ADVERB_LY')   = 'happily',  'happy ADVERB_LY';
    RAISE NOTICE 'All smoke tests passed.';
END $$;

SELECT count(*) AS inflection_rules_inserted FROM inflection_rules;

-- ====================================================================
-- Part 2: Delete regular forms from token_variants
-- ====================================================================
-- apply_inflection() now works. Any variant whose surface form matches
-- the regular rule output is derivable by rule and should not be stored.
-- Null-morpheme rows (archaic alt-of, dialect spelling variants) are
-- untouched — they have no morpheme label and no rule applies to them.

WITH deleted AS (
    DELETE FROM token_variants tv
    USING tokens t
    WHERE t.token_id = tv.canonical_id
      AND tv.morpheme IN (
            'PLURAL', 'PAST', 'PROGRESSIVE', '3RD_SING',
            'COMPARATIVE', 'SUPERLATIVE'
          )
      AND apply_inflection(t.name, tv.morpheme) = tv.name
    RETURNING tv.morpheme
)
SELECT morpheme, count(*) AS deleted
FROM deleted
GROUP BY morpheme
ORDER BY deleted DESC;

-- Verify what remains — should be irregular/archaic/dialect forms only
SELECT
    COALESCE(morpheme, '(null — archaic/dialect alt-of)') AS morpheme,
    count(*) AS remaining
FROM token_variants
GROUP BY morpheme
ORDER BY remaining DESC;

-- ====================================================================
-- Part 3: Helper function for per-token rule matching
-- ====================================================================
-- match_inflection_rule(name, morpheme) returns the first matching rule
-- with pre-computed strip/add (doubling absorbed into add_suffix).
-- Used below for bulk INSERT into token_morph_rules.

CREATE OR REPLACE FUNCTION match_inflection_rule(
    p_name     TEXT,
    p_morpheme TEXT
)
RETURNS TABLE (rule_id INT, strip_suffix TEXT, add_suffix TEXT)
LANGUAGE plpgsql STABLE STRICT
AS $$
DECLARE
    rec RECORD;
BEGIN
    FOR rec IN
        SELECT id, condition,
               inflection_rules.strip_suffix AS rsstrip,
               inflection_rules.add_suffix   AS rsadd
        FROM inflection_rules
        WHERE morpheme = p_morpheme
        ORDER BY priority ASC
    LOOP
        IF p_name ~ rec.condition THEN
            rule_id := rec.id;
            IF rec.rsstrip = '__DOUBLING__' THEN
                -- Absorb doubling into add_suffix so assembly is plain string concat.
                -- strip_suffix = '' (no stripping needed).
                -- add_suffix = (doubled form) - (root) = the tail after root.
                strip_suffix := '';
                add_suffix   := right(
                                    apply_doubling_rule(p_name, rec.rsadd),
                                    -(length(p_name))
                                );
            ELSE
                strip_suffix := rec.rsstrip;
                add_suffix   := rec.rsadd;
            END IF;
            RETURN NEXT;
            RETURN;  -- First match only
        END IF;
    END LOOP;
    -- No match: return empty (shouldn't happen with priority=99 catch-all)
END;
$$;

-- ====================================================================
-- Part 3: Populate token_morph_rules
-- ====================================================================
-- One row per (token, morpheme) for every REGULAR token.
-- Tokens with an irregular variant in token_variants are excluded —
-- the variant IS the inflection; no morph_rule row needed.
-- Doubling-absorbed add_suffix means assembly is:
--   left(name, length(name) - length(strip_suffix)) || add_suffix
-- which resolves to a plain string concat for all cases.

-- ── PAST (V_MAIN / V_AUX / V_COPULA, morpheme_accept bit 1 = 2) ──────
INSERT INTO token_morph_rules (token_id, morpheme, rule_id, strip_suffix, add_suffix)
SELECT DISTINCT t.token_id, 'PAST', mf.rule_id, mf.strip_suffix, mf.add_suffix
FROM tokens t
JOIN token_pos tp ON tp.token_id = t.token_id
    AND tp.pos IN ('V_MAIN', 'V_AUX', 'V_COPULA')
    AND (tp.morpheme_accept & 2) != 0
CROSS JOIN LATERAL (SELECT * FROM match_inflection_rule(t.name, 'PAST')) AS mf
WHERE NOT EXISTS (
    SELECT 1 FROM token_variants tv
    WHERE tv.canonical_id = t.token_id AND tv.morpheme = 'PAST'
)
ON CONFLICT (token_id, morpheme) DO NOTHING;

-- ── PROGRESSIVE (bit 2 = 4) ───────────────────────────────────────────
INSERT INTO token_morph_rules (token_id, morpheme, rule_id, strip_suffix, add_suffix)
SELECT DISTINCT t.token_id, 'PROGRESSIVE', mf.rule_id, mf.strip_suffix, mf.add_suffix
FROM tokens t
JOIN token_pos tp ON tp.token_id = t.token_id
    AND tp.pos IN ('V_MAIN', 'V_AUX', 'V_COPULA')
    AND (tp.morpheme_accept & 4) != 0
CROSS JOIN LATERAL (SELECT * FROM match_inflection_rule(t.name, 'PROGRESSIVE')) AS mf
WHERE NOT EXISTS (
    SELECT 1 FROM token_variants tv
    WHERE tv.canonical_id = t.token_id AND tv.morpheme = 'PROGRESSIVE'
)
ON CONFLICT (token_id, morpheme) DO NOTHING;

-- ── 3RD_SING (bit 3 = 8) ─────────────────────────────────────────────
INSERT INTO token_morph_rules (token_id, morpheme, rule_id, strip_suffix, add_suffix)
SELECT DISTINCT t.token_id, '3RD_SING', mf.rule_id, mf.strip_suffix, mf.add_suffix
FROM tokens t
JOIN token_pos tp ON tp.token_id = t.token_id
    AND tp.pos IN ('V_MAIN', 'V_AUX', 'V_COPULA')
    AND (tp.morpheme_accept & 8) != 0
CROSS JOIN LATERAL (SELECT * FROM match_inflection_rule(t.name, '3RD_SING')) AS mf
WHERE NOT EXISTS (
    SELECT 1 FROM token_variants tv
    WHERE tv.canonical_id = t.token_id AND tv.morpheme = '3RD_SING'
)
ON CONFLICT (token_id, morpheme) DO NOTHING;

-- ── PLURAL (N_COMMON, bit 0 = 1) ─────────────────────────────────────
INSERT INTO token_morph_rules (token_id, morpheme, rule_id, strip_suffix, add_suffix)
SELECT DISTINCT t.token_id, 'PLURAL', mf.rule_id, mf.strip_suffix, mf.add_suffix
FROM tokens t
JOIN token_pos tp ON tp.token_id = t.token_id
    AND tp.pos = 'N_COMMON'
    AND (tp.morpheme_accept & 1) != 0
CROSS JOIN LATERAL (SELECT * FROM match_inflection_rule(t.name, 'PLURAL')) AS mf
WHERE NOT EXISTS (
    SELECT 1 FROM token_variants tv
    WHERE tv.canonical_id = t.token_id AND tv.morpheme = 'PLURAL'
)
ON CONFLICT (token_id, morpheme) DO NOTHING;

-- ── COMPARATIVE (ADJ, bit 4 = 16) ────────────────────────────────────
INSERT INTO token_morph_rules (token_id, morpheme, rule_id, strip_suffix, add_suffix)
SELECT DISTINCT t.token_id, 'COMPARATIVE', mf.rule_id, mf.strip_suffix, mf.add_suffix
FROM tokens t
JOIN token_pos tp ON tp.token_id = t.token_id
    AND tp.pos = 'ADJ'
    AND (tp.morpheme_accept & 16) != 0
CROSS JOIN LATERAL (SELECT * FROM match_inflection_rule(t.name, 'COMPARATIVE')) AS mf
WHERE NOT EXISTS (
    SELECT 1 FROM token_variants tv
    WHERE tv.canonical_id = t.token_id AND tv.morpheme = 'COMPARATIVE'
)
ON CONFLICT (token_id, morpheme) DO NOTHING;

-- ── SUPERLATIVE (ADJ, bit 5 = 32) ────────────────────────────────────
INSERT INTO token_morph_rules (token_id, morpheme, rule_id, strip_suffix, add_suffix)
SELECT DISTINCT t.token_id, 'SUPERLATIVE', mf.rule_id, mf.strip_suffix, mf.add_suffix
FROM tokens t
JOIN token_pos tp ON tp.token_id = t.token_id
    AND tp.pos = 'ADJ'
    AND (tp.morpheme_accept & 32) != 0
CROSS JOIN LATERAL (SELECT * FROM match_inflection_rule(t.name, 'SUPERLATIVE')) AS mf
WHERE NOT EXISTS (
    SELECT 1 FROM token_variants tv
    WHERE tv.canonical_id = t.token_id AND tv.morpheme = 'SUPERLATIVE'
)
ON CONFLICT (token_id, morpheme) DO NOTHING;

-- ====================================================================
-- Verification
-- ====================================================================

SELECT
    (SELECT count(*) FROM inflection_rules)   AS inflection_rules,
    (SELECT count(*) FROM token_morph_rules)  AS token_morph_rules,
    (SELECT count(*) FROM token_variants)     AS token_variants_remaining;

SELECT morpheme, count(*) AS rows
FROM token_morph_rules
GROUP BY morpheme
ORDER BY rows DESC;

-- Spot-check a few known cases
SELECT
    -- walk PAST should be: strip='', add='ed'
    (SELECT strip_suffix = '' AND add_suffix = 'ed'
     FROM token_morph_rules tmr
     JOIN tokens t ON t.token_id = tmr.token_id
     WHERE t.name = 'walk' AND tmr.morpheme = 'PAST')          AS walk_past_ok,

    -- tap PAST should be: strip='', add='ped' (doubling absorbed)
    (SELECT strip_suffix = '' AND add_suffix = 'ped'
     FROM token_morph_rules tmr
     JOIN tokens t ON t.token_id = tmr.token_id
     WHERE t.name = 'tap' AND tmr.morpheme = 'PAST')           AS tap_past_ok,

    -- city PLURAL should be: strip='y', add='ies'
    (SELECT strip_suffix = 'y' AND add_suffix = 'ies'
     FROM token_morph_rules tmr
     JOIN tokens t ON t.token_id = tmr.token_id
     WHERE t.name = 'city' AND tmr.morpheme = 'PLURAL')        AS city_plural_ok,

    -- run should have NO token_morph_rules PAST row (has irregular 'ran')
    NOT EXISTS (
        SELECT 1 FROM token_morph_rules tmr
        JOIN tokens t ON t.token_id = tmr.token_id
        WHERE t.name = 'run' AND tmr.morpheme = 'PAST'
    )                                                           AS run_has_no_morph_rule,

    -- ran should still be in token_variants (irregular)
    EXISTS (
        SELECT 1 FROM token_variants tv
        JOIN tokens t ON t.token_id = tv.canonical_id
        WHERE t.name = 'run' AND tv.name = 'ran' AND tv.morpheme = 'PAST'
    )                                                           AS ran_still_in_variants;

COMMIT;
