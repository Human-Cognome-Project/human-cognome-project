-- Migration 030: Inflection helper functions + tense-aware envelope definitions
--
-- Part 1 (hcp_english): Postgres functions for regular inflection assembly.
--   These implement the rules in the `inflection_rules` table as callable
--   functions, so assembly queries in envelope_queries can use them.
--   The engine C++ kernel mirrors these for position-map reconstruction
--   without a DB round-trip (document fidelity requirement).
--
-- Part 2 (hcp_core): Tense-aware sub-envelope definitions.
--   Short words (len 2-4) loaded in all tenses upfront (function words, small set).
--   Tense-specific sub-envelopes (english_past, english_progressive, etc.) activated
--   on demand when the engine detects tense context from resolved function words.
--
-- CVC doubling heuristic:
--   Doubles if: root ends in [consonant][vowel][doubable_consonant]
--   AND the vowel is NOT part of a digraph (prev char is NOT a vowel)
--   AND root does not end in a known unstressed-suffix pattern
--   Doubable consonants: b d f g m n p r t (NOT: h j k l q v w x y z)
--   Note: -l doubling is British English (travel→travelling); American: traveled.
--   We use American convention (no -l doubling) as the default.
--   Explicit irregular variants in token_variants override this function.

\connect hcp_english

-- ============================================================
-- apply_doubling_rule(root, suffix)
-- Returns the inflected form applying CVC consonant doubling where applicable.
-- ============================================================

CREATE OR REPLACE FUNCTION apply_doubling_rule(root TEXT, suffix TEXT)
RETURNS TEXT
LANGUAGE plpgsql IMMUTABLE STRICT
AS $$
DECLARE
    len     INT  := length(root);
    c_last  CHAR := right(root, 1);           -- final char
    c_vowel CHAR := substring(root, len-1, 1); -- penultimate char
    c_prev  CHAR := CASE WHEN len >= 3 THEN substring(root, len-2, 1) ELSE '' END;
BEGIN
    -- Minimum length for doubling consideration
    IF len < 2 THEN
        RETURN root || suffix;
    END IF;

    -- Final char must be a doubable consonant
    -- b d f g m n p r t — NOT h j k l q v w x y z
    IF c_last NOT IN ('b','d','f','g','m','n','p','r','t') THEN
        RETURN root || suffix;
    END IF;

    -- Penultimate must be a vowel
    IF c_vowel NOT IN ('a','e','i','o','u') THEN
        RETURN root || suffix;
    END IF;

    -- Prev char (3rd from end) must be a consonant or start-of-word.
    -- If prev is also a vowel → digraph (rain, beat, suit) → no doubling.
    IF len >= 3 AND c_prev IN ('a','e','i','o','u') THEN
        RETURN root || suffix;
    END IF;

    -- Suppress doubling for common unstressed-suffix patterns where
    -- the word is 2+ syllables with stress on the first (open, happen, etc.)
    -- Heuristic: if root length >= 4 and ends in -en, -on, -an, -er, -or
    -- these are typically unstressed final syllables → no double.
    -- Threshold >= 4 catches 'open'(4) but not 'pen'(3), which SHOULD double.
    -- Exceptions (begin, occur, etc.) are handled by explicit token_variants.
    IF len >= 4 AND right(root, 2) IN ('en','on','an','er','or') THEN
        RETURN root || suffix;
    END IF;

    -- CVC confirmed → double the final consonant
    RETURN root || c_last || suffix;
END;
$$;

-- ============================================================
-- apply_inflection(root, morpheme)
-- Top-level dispatcher: applies the correct inflection rule to root.
-- Uses inflection_rules table for simple cases; calls apply_doubling_rule
-- for __DOUBLING__ sentinel cases.
-- ============================================================

CREATE OR REPLACE FUNCTION apply_inflection(root TEXT, morpheme TEXT)
RETURNS TEXT
LANGUAGE plpgsql IMMUTABLE STRICT
AS $$
DECLARE
    rec     RECORD;
    result  TEXT;
BEGIN
    FOR rec IN
        SELECT strip_suffix, add_suffix, condition
        FROM inflection_rules
        WHERE inflection_rules.morpheme = apply_inflection.morpheme
        ORDER BY priority ASC
    LOOP
        -- Check condition regex
        IF root ~ rec.condition THEN
            IF rec.strip_suffix = '__DOUBLING__' THEN
                result := apply_doubling_rule(root, rec.add_suffix);
            ELSE
                -- Strip suffix from root end, then add new suffix
                IF rec.strip_suffix = '' THEN
                    result := root || rec.add_suffix;
                ELSE
                    result := left(root, length(root) - length(rec.strip_suffix))
                              || rec.add_suffix;
                END IF;
            END IF;
            RETURN result;
        END IF;
    END LOOP;

    -- No rule matched (shouldn't happen with priority=99 catch-all)
    RETURN root;
END;
$$;

-- Quick smoke test
SELECT
    apply_inflection('walk',   'PAST')        AS walked,
    apply_inflection('like',   'PAST')        AS liked,
    apply_inflection('tap',    'PAST')        AS tapped,
    apply_inflection('run',    'PROGRESSIVE') AS running,
    apply_inflection('make',   'PROGRESSIVE') AS making,
    apply_inflection('city',   'PLURAL')      AS cities,
    apply_inflection('cat',    'PLURAL')      AS cats,
    apply_inflection('kiss',   'PLURAL')      AS kisses,
    apply_inflection('happy',  'ADVERB_LY')   AS happily,
    apply_inflection('quick',  'ADVERB_LY')   AS quickly,
    apply_inflection('big',    'COMPARATIVE') AS bigger,
    apply_inflection('fast',   'COMPARATIVE') AS faster,
    apply_inflection('open',   'PAST')        AS opened_not_openned,
    apply_inflection('rain',   'PROGRESSIVE') AS raining_not_rainning;

-- ============================================================
-- Part 2: Tense-aware envelope definitions in hcp_core
-- ============================================================

\connect hcp_core

-- Short-word function vocabulary (len 2-4, ALL tenses loaded upfront).
-- These establish grammatical context (tense, number, mood) during resolution.
-- Archaic function words included: thee, thou, thy, hath, doth, etc.
INSERT INTO envelope_definitions (name, description)
VALUES
    ('english_function_words',
     'Function words len 2-4, all tenses. Establishes grammatical context for tense detection.'),
    ('english_past_tense',
     'Regular past tense forms for top-ranked verbs (len 5+). Activated on past-tense context detection.'),
    ('english_progressive',
     'Regular progressive forms for top-ranked verbs (len 5+). Activated on progressive context detection.'),
    ('english_plural',
     'Regular plural forms for top-ranked nouns (len 5+). Activated as needed.');

-- ---- english_function_words queries ----
-- Load all 2-4 letter tokens (base forms) — covers function words in all bases.
-- Short enough that loading in multiple tenses is cheap (few hundred rows each).

INSERT INTO envelope_queries
    (envelope_id, shard_db, query_text, description, priority, lmdb_subdb)
SELECT
    id, 'hcp_english',
    'SELECT t.name, t.token_id '
    'FROM tokens t '
    'WHERE length(t.name) BETWEEN 2 AND 4 '
    '  AND t.freq_rank IS NOT NULL '
    'ORDER BY t.freq_rank ASC',
    'Base forms of function words (len 2-4)', 1, 'env_vocab'
FROM envelope_definitions WHERE name = 'english_function_words';

-- Past tense of short function words (walked→walked, went→went already in variants)
INSERT INTO envelope_queries
    (envelope_id, shard_db, query_text, description, priority, lmdb_subdb)
SELECT
    id, 'hcp_english',
    'SELECT apply_inflection(t.name, ''PAST'') AS name, t.token_id '
    'FROM tokens t '
    'JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''V_MAIN'' '
    'WHERE length(t.name) BETWEEN 2 AND 4 '
    '  AND t.freq_rank IS NOT NULL '
    '  AND (tp.morpheme_accept & 2) != 0 '
    '  AND NOT EXISTS ('
    '    SELECT 1 FROM token_variants tv '
    '    WHERE tv.canonical_id = t.token_id AND tv.morpheme = ''PAST'''
    '  ) '
    'ORDER BY t.freq_rank ASC',
    'Regular past tense of short verbs (len 2-4)', 2, 'env_vocab'
FROM envelope_definitions WHERE name = 'english_function_words';

-- Progressive of short verbs
INSERT INTO envelope_queries
    (envelope_id, shard_db, query_text, description, priority, lmdb_subdb)
SELECT
    id, 'hcp_english',
    'SELECT apply_inflection(t.name, ''PROGRESSIVE'') AS name, t.token_id '
    'FROM tokens t '
    'JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''V_MAIN'' '
    'WHERE length(t.name) BETWEEN 2 AND 4 '
    '  AND t.freq_rank IS NOT NULL '
    '  AND (tp.morpheme_accept & 4) != 0 '
    '  AND NOT EXISTS ('
    '    SELECT 1 FROM token_variants tv '
    '    WHERE tv.canonical_id = t.token_id AND tv.morpheme = ''PROGRESSIVE'''
    '  ) '
    'ORDER BY t.freq_rank ASC',
    'Regular progressive of short verbs (len 2-4)', 3, 'env_vocab'
FROM envelope_definitions WHERE name = 'english_function_words';

-- 3rd sing of short verbs
INSERT INTO envelope_queries
    (envelope_id, shard_db, query_text, description, priority, lmdb_subdb)
SELECT
    id, 'hcp_english',
    'SELECT apply_inflection(t.name, ''3RD_SING'') AS name, t.token_id '
    'FROM tokens t '
    'JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''V_MAIN'' '
    'WHERE length(t.name) BETWEEN 2 AND 4 '
    '  AND t.freq_rank IS NOT NULL '
    '  AND (tp.morpheme_accept & 8) != 0 '
    '  AND NOT EXISTS ('
    '    SELECT 1 FROM token_variants tv '
    '    WHERE tv.canonical_id = t.token_id AND tv.morpheme = ''3RD_SING'''
    '  ) '
    'ORDER BY t.freq_rank ASC',
    'Regular 3rd-sing of short verbs (len 2-4)', 4, 'env_vocab'
FROM envelope_definitions WHERE name = 'english_function_words';

-- ---- english_past_tense: long verbs (len 5+), activated on past-tense detection ----
INSERT INTO envelope_queries
    (envelope_id, shard_db, query_text, description, priority, lmdb_subdb)
SELECT
    id, 'hcp_english',
    'SELECT apply_inflection(t.name, ''PAST'') AS name, t.token_id '
    'FROM tokens t '
    'JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''V_MAIN'' '
    'WHERE length(t.name) >= 5 '
    '  AND t.freq_rank IS NOT NULL '
    '  AND (tp.morpheme_accept & 2) != 0 '
    '  AND NOT EXISTS ('
    '    SELECT 1 FROM token_variants tv '
    '    WHERE tv.canonical_id = t.token_id AND tv.morpheme = ''PAST'''
    '  ) '
    '  AND (t.characteristics & 256) = 0 '
    'ORDER BY t.freq_rank ASC LIMIT 5000',
    'Regular past tense for top long verbs (len 5+)', 1, 'env_vocab'
FROM envelope_definitions WHERE name = 'english_past_tense';

-- ---- english_progressive: long verbs (len 5+) ----
INSERT INTO envelope_queries
    (envelope_id, shard_db, query_text, description, priority, lmdb_subdb)
SELECT
    id, 'hcp_english',
    'SELECT apply_inflection(t.name, ''PROGRESSIVE'') AS name, t.token_id '
    'FROM tokens t '
    'JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''V_MAIN'' '
    'WHERE length(t.name) >= 5 '
    '  AND t.freq_rank IS NOT NULL '
    '  AND (tp.morpheme_accept & 4) != 0 '
    '  AND NOT EXISTS ('
    '    SELECT 1 FROM token_variants tv '
    '    WHERE tv.canonical_id = t.token_id AND tv.morpheme = ''PROGRESSIVE'''
    '  ) '
    '  AND (t.characteristics & 256) = 0 '
    'ORDER BY t.freq_rank ASC LIMIT 5000',
    'Regular progressive for top long verbs (len 5+)', 1, 'env_vocab'
FROM envelope_definitions WHERE name = 'english_progressive';

-- ---- english_plural: long nouns (len 5+) ----
INSERT INTO envelope_queries
    (envelope_id, shard_db, query_text, description, priority, lmdb_subdb)
SELECT
    id, 'hcp_english',
    'SELECT apply_inflection(t.name, ''PLURAL'') AS name, t.token_id '
    'FROM tokens t '
    'JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''N_COMMON'' '
    'WHERE length(t.name) >= 5 '
    '  AND t.freq_rank IS NOT NULL '
    '  AND (tp.morpheme_accept & 1) != 0 '
    '  AND NOT EXISTS ('
    '    SELECT 1 FROM token_variants tv '
    '    WHERE tv.canonical_id = t.token_id AND tv.morpheme = ''PLURAL'''
    '  ) '
    '  AND (t.characteristics & 256) = 0 '
    'ORDER BY t.freq_rank ASC LIMIT 5000',
    'Regular plural for top long nouns (len 5+)', 1, 'env_vocab'
FROM envelope_definitions WHERE name = 'english_plural';

-- Verify
SELECT ed.name, count(eq.id) AS query_count
FROM envelope_definitions ed
LEFT JOIN envelope_queries eq ON eq.envelope_id = ed.id
GROUP BY ed.name
ORDER BY ed.name;
