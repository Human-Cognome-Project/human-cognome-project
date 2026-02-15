-- HCP Migration 006c: Initial Verb Classification (hcp_english)
--
-- Populates token_sub_cat junction table with ~45 high-frequency verbs
-- and their sub-categorization patterns from the linguistics catalog.
--
-- Uses INSERT...SELECT to resolve token_ids by name, avoiding hard-coded IDs.
-- For names with multiple tokens (fix, play), picks the first (lowest token_id).
--
-- Source: docs/research/english-sub-cat-patterns.md
-- Prerequisite: 006b_sub_cat_patterns.sql (pattern tokens + tables)

BEGIN;

-- ================================================================
-- TEMP TABLE: classification data
-- ================================================================
-- word_name: the word being classified
-- pattern_name: the sub-cat pattern (matches token name in AB.AB.FA)
-- freq: relative frequency weight (1.0=primary, 0.7=common alt, etc.)
-- spec_p: verb-specific preposition (for V_PREP, V_TRANS_PREP)
-- part: particle (for V_PARTICLE)

CREATE TEMP TABLE classification (
    word_name    TEXT NOT NULL,
    pattern_name TEXT NOT NULL,
    freq         REAL NOT NULL,
    spec_p       TEXT,
    part         TEXT
);

-- BE
INSERT INTO classification VALUES
('be', 'V_INTENS', 1.0, NULL, NULL);

-- HAVE
INSERT INTO classification VALUES
('have', 'V_TRANS', 1.0, NULL, NULL),
('have', 'V_NP_INF_II', 0.3, NULL, NULL),
('have', 'V_NP_ING', 0.3, NULL, NULL);

-- DO
INSERT INTO classification VALUES
('do', 'V_TRANS', 1.0, NULL, NULL),
('do', 'V_INTRANS', 0.5, NULL, NULL);

-- SAY
INSERT INTO classification VALUES
('say', 'V_THAT', 1.0, NULL, NULL),
('say', 'V_TRANS', 0.5, NULL, NULL),
('say', 'V_NP_THAT', 0.3, NULL, NULL);

-- GET
INSERT INTO classification VALUES
('get', 'V_TRANS', 1.0, NULL, NULL),
('get', 'V_INTENS', 0.7, NULL, NULL),
('get', 'V_NP_INF_II', 0.5, NULL, NULL),
('get', 'V_NP_ING', 0.3, NULL, NULL),
('get', 'V_COMPLEX', 0.3, NULL, NULL),
('get', 'V_PREP', 0.3, 'to', NULL);

-- MAKE
INSERT INTO classification VALUES
('make', 'V_TRANS', 1.0, NULL, NULL),
('make', 'V_COMPLEX', 0.7, NULL, NULL),
('make', 'V_BARE', 0.5, NULL, NULL),
('make', 'V_NP_INF_II', 0.3, NULL, NULL);

-- GO
INSERT INTO classification VALUES
('go', 'V_INTRANS', 1.0, NULL, NULL),
('go', 'V_PREP', 0.7, 'to', NULL),
('go', 'V_ING', 0.3, NULL, NULL);

-- KNOW
INSERT INTO classification VALUES
('know', 'V_THAT', 1.0, NULL, NULL),
('know', 'V_TRANS', 0.7, NULL, NULL),
('know', 'V_WH', 0.5, NULL, NULL),
('know', 'V_NP_INF_I', 0.3, NULL, NULL);

-- TAKE
INSERT INTO classification VALUES
('take', 'V_TRANS', 1.0, NULL, NULL),
('take', 'V_PARTICLE', 0.5, NULL, 'off'),
('take', 'V_DITRANS', 0.3, NULL, NULL),
('take', 'V_TRANS_PREP', 0.3, 'to', NULL);

-- COME
INSERT INTO classification VALUES
('come', 'V_INTRANS', 1.0, NULL, NULL),
('come', 'V_PREP', 0.7, 'to', NULL),
('come', 'V_INF', 0.3, NULL, NULL);

-- THINK
INSERT INTO classification VALUES
('think', 'V_THAT', 1.0, NULL, NULL),
('think', 'V_PREP', 0.5, 'about', NULL),
('think', 'V_NP_INF_I', 0.3, NULL, NULL),
('think', 'V_TRANS', 0.3, NULL, NULL);

-- SEE
INSERT INTO classification VALUES
('see', 'V_TRANS', 1.0, NULL, NULL),
('see', 'V_BARE', 0.5, NULL, NULL),
('see', 'V_NP_ING', 0.5, NULL, NULL),
('see', 'V_THAT', 0.3, NULL, NULL),
('see', 'V_WH', 0.3, NULL, NULL);

-- WANT
INSERT INTO classification VALUES
('want', 'V_INF', 1.0, NULL, NULL),
('want', 'V_TRANS', 0.7, NULL, NULL),
('want', 'V_NP_INF_II', 0.5, NULL, NULL);

-- GIVE
INSERT INTO classification VALUES
('give', 'V_DITRANS', 1.0, NULL, NULL),
('give', 'V_TRANS_PREP', 0.7, 'to', NULL),
('give', 'V_TRANS', 0.5, NULL, NULL);

-- TELL
INSERT INTO classification VALUES
('tell', 'V_NP_THAT', 1.0, NULL, NULL),
('tell', 'V_NP_INF_II', 0.7, NULL, NULL),
('tell', 'V_DITRANS', 0.5, NULL, NULL),
('tell', 'V_TRANS', 0.5, NULL, NULL);

-- ASK
INSERT INTO classification VALUES
('ask', 'V_WH', 1.0, NULL, NULL),
('ask', 'V_NP_INF_II', 0.7, NULL, NULL),
('ask', 'V_TRANS', 0.5, NULL, NULL),
('ask', 'V_THAT', 0.3, NULL, NULL);

-- FIND
INSERT INTO classification VALUES
('find', 'V_TRANS', 1.0, NULL, NULL),
('find', 'V_THAT', 0.5, NULL, NULL),
('find', 'V_COMPLEX', 0.5, NULL, NULL),
('find', 'V_NP_ING', 0.3, NULL, NULL),
('find', 'V_WH', 0.3, NULL, NULL);

-- PUT
INSERT INTO classification VALUES
('put', 'V_TRANS_PREP', 1.0, NULL, NULL);

-- SEEM
INSERT INTO classification VALUES
('seem', 'V_INTENS', 1.0, NULL, NULL),
('seem', 'V_INF', 0.5, NULL, NULL),
('seem', 'V_THAT', 0.3, NULL, NULL);

-- BELIEVE
INSERT INTO classification VALUES
('believe', 'V_THAT', 1.0, NULL, NULL),
('believe', 'V_TRANS', 0.5, NULL, NULL),
('believe', 'V_NP_INF_I', 0.5, NULL, NULL),
('believe', 'V_PREP', 0.3, 'in', NULL);

-- PERSUADE
INSERT INTO classification VALUES
('persuade', 'V_NP_INF_II', 1.0, NULL, NULL),
('persuade', 'V_TRANS', 0.3, NULL, NULL);

-- ENJOY
INSERT INTO classification VALUES
('enjoy', 'V_ING', 1.0, NULL, NULL),
('enjoy', 'V_TRANS', 0.5, NULL, NULL);

-- AVOID
INSERT INTO classification VALUES
('avoid', 'V_ING', 1.0, NULL, NULL),
('avoid', 'V_TRANS', 0.7, NULL, NULL);

-- EXPECT
INSERT INTO classification VALUES
('expect', 'V_INF', 1.0, NULL, NULL),
('expect', 'V_NP_INF_I', 0.7, NULL, NULL),
('expect', 'V_THAT', 0.5, NULL, NULL),
('expect', 'V_TRANS', 0.5, NULL, NULL);

-- HELP
INSERT INTO classification VALUES
('help', 'V_TRANS', 1.0, NULL, NULL),
('help', 'V_NP_INF_II', 0.7, NULL, NULL),
('help', 'V_BARE', 0.7, NULL, NULL),
('help', 'V_INF', 0.3, NULL, NULL);

-- LET
INSERT INTO classification VALUES
('let', 'V_BARE', 1.0, NULL, NULL),
('let', 'V_TRANS', 0.3, NULL, NULL);

-- KEEP
INSERT INTO classification VALUES
('keep', 'V_TRANS', 1.0, NULL, NULL),
('keep', 'V_ING', 0.7, NULL, NULL),
('keep', 'V_COMPLEX', 0.5, NULL, NULL),
('keep', 'V_PREP', 0.3, 'to', NULL);

-- FEEL
INSERT INTO classification VALUES
('feel', 'V_INTENS', 1.0, NULL, NULL),
('feel', 'V_TRANS', 0.5, NULL, NULL),
('feel', 'V_THAT', 0.5, NULL, NULL),
('feel', 'V_NP_ING', 0.3, NULL, NULL),
('feel', 'V_BARE', 0.3, NULL, NULL);

-- BECOME
INSERT INTO classification VALUES
('become', 'V_INTENS', 1.0, NULL, NULL);

-- REMAIN
INSERT INTO classification VALUES
('remain', 'V_INTENS', 1.0, NULL, NULL);

-- APPEAR
INSERT INTO classification VALUES
('appear', 'V_INTENS', 1.0, NULL, NULL),
('appear', 'V_INF', 0.5, NULL, NULL),
('appear', 'V_INTRANS', 0.5, NULL, NULL);

-- LOOK
INSERT INTO classification VALUES
('look', 'V_PREP', 1.0, 'at', NULL),
('look', 'V_INTENS', 0.7, NULL, NULL),
('look', 'V_PARTICLE', 0.5, NULL, 'up'),
('look', 'V_INTRANS', 0.3, NULL, NULL);

-- TURN
INSERT INTO classification VALUES
('turn', 'V_INTENS', 0.7, NULL, NULL),
('turn', 'V_TRANS', 0.7, NULL, NULL),
('turn', 'V_INTRANS', 0.5, NULL, NULL),
('turn', 'V_PREP', 0.5, 'to', NULL),
('turn', 'V_PARTICLE', 0.5, NULL, 'off');

-- LEAVE
INSERT INTO classification VALUES
('leave', 'V_TRANS', 1.0, NULL, NULL),
('leave', 'V_INTRANS', 0.7, NULL, NULL),
('leave', 'V_COMPLEX', 0.5, NULL, NULL),
('leave', 'V_DITRANS', 0.3, NULL, NULL);

-- SHOW
INSERT INTO classification VALUES
('show', 'V_TRANS', 1.0, NULL, NULL),
('show', 'V_DITRANS', 0.7, NULL, NULL),
('show', 'V_NP_THAT', 0.5, NULL, NULL),
('show', 'V_THAT', 0.5, NULL, NULL);

-- RUN
INSERT INTO classification VALUES
('run', 'V_INTRANS', 1.0, NULL, NULL),
('run', 'V_TRANS', 0.5, NULL, NULL),
('run', 'V_PREP', 0.5, 'into', NULL);

-- HEAR
INSERT INTO classification VALUES
('hear', 'V_TRANS', 1.0, NULL, NULL),
('hear', 'V_BARE', 0.5, NULL, NULL),
('hear', 'V_NP_ING', 0.5, NULL, NULL),
('hear', 'V_THAT', 0.3, NULL, NULL);

-- SPEAK
INSERT INTO classification VALUES
('speak', 'V_INTRANS', 1.0, NULL, NULL),
('speak', 'V_PREP', 0.7, 'about', NULL);

-- TALK
INSERT INTO classification VALUES
('talk', 'V_INTRANS', 1.0, NULL, NULL),
('talk', 'V_PREP', 0.7, 'to', NULL);

-- WAIT
INSERT INTO classification VALUES
('wait', 'V_INTRANS', 1.0, NULL, NULL),
('wait', 'V_PREP', 0.7, 'for', NULL),
('wait', 'V_INF', 0.3, NULL, NULL);

-- DEPEND
INSERT INTO classification VALUES
('depend', 'V_PREP', 1.0, 'on', NULL);

-- RELY
INSERT INTO classification VALUES
('rely', 'V_PREP', 1.0, 'on', NULL);

-- Core intransitives: laugh, smile, cry, sleep, arrive, exist, die
INSERT INTO classification VALUES
('laugh', 'V_INTRANS', 1.0, NULL, NULL),
('laugh', 'V_PREP', 0.3, 'at', NULL),
('smile', 'V_INTRANS', 1.0, NULL, NULL),
('cry', 'V_INTRANS', 1.0, NULL, NULL),
('cry', 'V_PREP', 0.3, 'about', NULL),
('sleep', 'V_INTRANS', 1.0, NULL, NULL),
('arrive', 'V_INTRANS', 1.0, NULL, NULL),
('exist', 'V_INTRANS', 1.0, NULL, NULL),
('die', 'V_INTRANS', 1.0, NULL, NULL);

-- Core transitives with intransitive alternation:
-- read, write, eat, drink, sing, play, drive, build, break, open,
-- close, move, carry, hold, pull, push, cut, hit, throw, catch,
-- draw, cook, wash, clean, fix
INSERT INTO classification VALUES
('read', 'V_TRANS', 1.0, NULL, NULL),   ('read', 'V_INTRANS', 0.5, NULL, NULL),
('write', 'V_TRANS', 1.0, NULL, NULL),  ('write', 'V_INTRANS', 0.5, NULL, NULL),
('eat', 'V_TRANS', 1.0, NULL, NULL),    ('eat', 'V_INTRANS', 0.5, NULL, NULL),
('drink', 'V_TRANS', 1.0, NULL, NULL),  ('drink', 'V_INTRANS', 0.5, NULL, NULL),
('sing', 'V_TRANS', 1.0, NULL, NULL),   ('sing', 'V_INTRANS', 0.5, NULL, NULL),
('play', 'V_TRANS', 1.0, NULL, NULL),   ('play', 'V_INTRANS', 0.5, NULL, NULL),
('drive', 'V_TRANS', 1.0, NULL, NULL),  ('drive', 'V_INTRANS', 0.5, NULL, NULL),
('build', 'V_TRANS', 1.0, NULL, NULL),  ('build', 'V_INTRANS', 0.5, NULL, NULL),
('break', 'V_TRANS', 1.0, NULL, NULL),  ('break', 'V_INTRANS', 0.5, NULL, NULL),
('open', 'V_TRANS', 1.0, NULL, NULL),   ('open', 'V_INTRANS', 0.5, NULL, NULL),
('close', 'V_TRANS', 1.0, NULL, NULL),  ('close', 'V_INTRANS', 0.5, NULL, NULL),
('move', 'V_TRANS', 1.0, NULL, NULL),   ('move', 'V_INTRANS', 0.5, NULL, NULL),
('carry', 'V_TRANS', 1.0, NULL, NULL),
('hold', 'V_TRANS', 1.0, NULL, NULL),
('pull', 'V_TRANS', 1.0, NULL, NULL),   ('pull', 'V_INTRANS', 0.5, NULL, NULL),
('push', 'V_TRANS', 1.0, NULL, NULL),   ('push', 'V_INTRANS', 0.5, NULL, NULL),
('cut', 'V_TRANS', 1.0, NULL, NULL),
('hit', 'V_TRANS', 1.0, NULL, NULL),
('throw', 'V_TRANS', 1.0, NULL, NULL),
('catch', 'V_TRANS', 1.0, NULL, NULL),
('draw', 'V_TRANS', 1.0, NULL, NULL),   ('draw', 'V_INTRANS', 0.5, NULL, NULL),
('cook', 'V_TRANS', 1.0, NULL, NULL),   ('cook', 'V_INTRANS', 0.5, NULL, NULL),
('wash', 'V_TRANS', 1.0, NULL, NULL),
('clean', 'V_TRANS', 1.0, NULL, NULL),
('fix', 'V_TRANS', 1.0, NULL, NULL);

-- SEND
INSERT INTO classification VALUES
('send', 'V_DITRANS', 1.0, NULL, NULL),
('send', 'V_TRANS', 0.7, NULL, NULL),
('send', 'V_TRANS_PREP', 0.7, 'to', NULL);

-- OFFER / PROMISE
INSERT INTO classification VALUES
('offer', 'V_DITRANS', 1.0, NULL, NULL),
('offer', 'V_INF', 0.7, NULL, NULL),
('offer', 'V_TRANS', 0.5, NULL, NULL),
('promise', 'V_DITRANS', 1.0, NULL, NULL),
('promise', 'V_INF', 0.7, NULL, NULL),
('promise', 'V_TRANS', 0.5, NULL, NULL);

-- TRY / ATTEMPT / MANAGE / FAIL
INSERT INTO classification VALUES
('try', 'V_INF', 1.0, NULL, NULL),
('try', 'V_TRANS', 0.3, NULL, NULL),
('attempt', 'V_INF', 1.0, NULL, NULL),
('manage', 'V_INF', 1.0, NULL, NULL),
('fail', 'V_INF', 1.0, NULL, NULL);

-- CONSIDER
INSERT INTO classification VALUES
('consider', 'V_TRANS', 1.0, NULL, NULL),
('consider', 'V_COMPLEX', 0.7, NULL, NULL),
('consider', 'V_ING', 0.5, NULL, NULL),
('consider', 'V_NP_INF_I', 0.3, NULL, NULL);

-- WONDER
INSERT INTO classification VALUES
('wonder', 'V_WH', 1.0, NULL, NULL);

-- STOP / START / BEGIN / CONTINUE / FINISH
INSERT INTO classification VALUES
('stop', 'V_ING', 1.0, NULL, NULL),
('stop', 'V_TRANS', 0.5, NULL, NULL),
('start', 'V_ING', 1.0, NULL, NULL),
('start', 'V_INF', 0.7, NULL, NULL),
('start', 'V_TRANS', 0.5, NULL, NULL),
('begin', 'V_ING', 1.0, NULL, NULL),
('begin', 'V_INF', 0.7, NULL, NULL),
('begin', 'V_TRANS', 0.5, NULL, NULL),
('continue', 'V_ING', 1.0, NULL, NULL),
('continue', 'V_INF', 0.7, NULL, NULL),
('finish', 'V_ING', 1.0, NULL, NULL),
('finish', 'V_TRANS', 0.5, NULL, NULL);

-- ================================================================
-- INSERT INTO token_sub_cat via JOIN
-- ================================================================
-- Resolves word tokens by name (picks lowest token_id for duplicates).
-- Resolves pattern tokens by name from AB.AB.FA namespace.

INSERT INTO token_sub_cat (
    tok_ns, tok_p2, tok_p3, tok_p4, tok_p5,
    pat_ns, pat_p2, pat_p3, pat_p4, pat_p5,
    frequency, specific_p, particle
)
SELECT
    w.ns, w.p2, w.p3, w.p4, w.p5,
    p.ns, p.p2, p.p3, p.p4, p.p5,
    c.freq, c.spec_p, c.part
FROM classification c
JOIN LATERAL (
    SELECT ns, p2, p3, p4, p5
    FROM tokens
    WHERE name = c.word_name
      AND p3 IN ('CA','CB','CC','CD','CE','CF','CG','CH','CI','CJ')
    ORDER BY token_id
    LIMIT 1
) w ON true
JOIN LATERAL (
    SELECT ns, p2, p3, p4, p5
    FROM tokens
    WHERE name = c.pattern_name
      AND ns = 'AB' AND p2 = 'AB' AND p3 = 'FA'
    LIMIT 1
) p ON true;

DROP TABLE classification;

COMMIT;

-- ================================================================
-- VERIFICATION
-- ================================================================

\echo ''
\echo '=== Classification summary ==='
SELECT COUNT(*) AS total_mappings,
       COUNT(DISTINCT tok_token) AS unique_words,
       COUNT(DISTINCT pat_token) AS unique_patterns
FROM token_sub_cat;

\echo ''
\echo '=== Mappings per pattern ==='
SELECT p.name AS pattern, COUNT(*) AS word_count
FROM token_sub_cat sc
JOIN tokens p ON p.token_id = sc.pat_token
GROUP BY p.name
ORDER BY word_count DESC;

\echo ''
\echo '=== Most versatile words (4+ patterns) ==='
SELECT w.name AS word, COUNT(*) AS pattern_count,
       string_agg(p.name, ', ' ORDER BY sc.frequency DESC) AS patterns
FROM token_sub_cat sc
JOIN tokens w ON w.token_id = sc.tok_token
JOIN tokens p ON p.token_id = sc.pat_token
GROUP BY w.name
HAVING COUNT(*) >= 4
ORDER BY pattern_count DESC, w.name;

\echo ''
\echo '=== Sample: GET patterns ==='
SELECT w.name, p.name AS pattern, sc.frequency, sc.specific_p, sc.particle
FROM token_sub_cat sc
JOIN tokens w ON w.token_id = sc.tok_token
JOIN tokens p ON p.token_id = sc.pat_token
WHERE w.name = 'get'
ORDER BY sc.frequency DESC;
