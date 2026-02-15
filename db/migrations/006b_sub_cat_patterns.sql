-- HCP Migration 006b: Sub-categorization Patterns (hcp_english)
--
-- Creates:
--   1. 30 sub-cat pattern tokens in AB.AB.FA namespace
--   2. sub_cat_slots table (~40 slot definitions)
--   3. token_sub_cat junction table (word-to-pattern mapping)
--   4. Initial classification of ~45 high-frequency verbs
--
-- Phase 2 (category tokens) is SKIPPED: existing p3 namespaces
-- (CA=N, CB=V, CC=A, etc.) ARE the structural categories.
--
-- Namespace allocation:
--   AB.AB.FA.AA.* = verb patterns (17)
--   AB.AB.FA.AB.* = noun patterns (5)
--   AB.AB.FA.AC.* = adjective patterns (5)
--   AB.AB.FA.AD.* = preposition patterns (3)
--
-- Prerequisite: hcp_english with decomposed token schema

BEGIN;

-- ================================================================
-- SUB-CAT PATTERN TOKENS (30 patterns)
-- ================================================================
-- Each pattern is a proper token. The name is the human-readable
-- shortcode (V_TRANS, etc.) and metadata holds the NSM frame.

-- --- Verb patterns (17) at AB.AB.FA.AA.* ---

INSERT INTO tokens (ns, p2, p3, p4, p5, name, layer, subcategory, metadata) VALUES
('AB','AB','FA','AA','AA', 'V_INTRANS',      'sub_cat', 'verb', '{"slot_count": 0, "nsm_frame": "X does something.", "description": "Intransitive — no complement"}'),
('AB','AB','FA','AA','AB', 'V_TRANS',         'sub_cat', 'verb', '{"slot_count": 1, "nsm_frame": "X does something to Y.", "description": "Transitive — NP direct object"}'),
('AB','AB','FA','AA','AC', 'V_DITRANS',       'sub_cat', 'verb', '{"slot_count": 2, "nsm_frame": "X does something to Y with Z.", "description": "Ditransitive — NP indirect + NP direct object"}'),
('AB','AB','FA','AA','AD', 'V_INTENS',        'sub_cat', 'verb', '{"slot_count": 1, "nsm_frame": "X is something.", "description": "Intensive/copular — AP/NP/PP subject-predicative"}'),
('AB','AB','FA','AA','AE', 'V_COMPLEX',       'sub_cat', 'verb', '{"slot_count": 2, "nsm_frame": "X does something to Y. Y is Z.", "description": "Complex transitive — NP direct object + AP/NP/PP object-predicative"}'),
('AB','AB','FA','AA','AF', 'V_PREP',          'sub_cat', 'verb', '{"slot_count": 1, "nsm_frame": "X does something P Y.", "description": "Prepositional verb — PP with specific preposition"}'),
('AB','AB','FA','AA','AG', 'V_TRANS_PREP',    'sub_cat', 'verb', '{"slot_count": 2, "nsm_frame": "X does something to Y P Z.", "description": "Transitive + prepositional complement — NP + PP"}'),
('AB','AB','FA','AA','AH', 'V_PARTICLE',      'sub_cat', 'verb', '{"slot_count": 1, "nsm_frame": "X does something to Y.", "description": "Phrasal verb — verb + particle + NP direct object"}'),
('AB','AB','FA','AA','AI', 'V_THAT',          'sub_cat', 'verb', '{"slot_count": 1, "nsm_frame": "X knows/thinks [that ...].", "description": "That-clause complement"}'),
('AB','AB','FA','AA','AJ', 'V_WH',            'sub_cat', 'verb', '{"slot_count": 1, "nsm_frame": "X wants-to-know [wh ...].", "description": "Wh-clause complement"}'),
('AB','AB','FA','AA','AK', 'V_INF',           'sub_cat', 'verb', '{"slot_count": 1, "nsm_frame": "X wants-to-do something.", "description": "To-infinitive complement (subject control)"}'),
('AB','AB','FA','AA','AL', 'V_ING',           'sub_cat', 'verb', '{"slot_count": 1, "nsm_frame": "X does/feels something [ongoing].", "description": "-ing participle complement"}'),
('AB','AB','FA','AA','AM', 'V_BARE',          'sub_cat', 'verb', '{"slot_count": 2, "nsm_frame": "X sees/hears Y do something.", "description": "NP + bare infinitive (perception/causative)"}'),
('AB','AB','FA','AA','AN', 'V_NP_INF_I',      'sub_cat', 'verb', '{"slot_count": 1, "nsm_frame": "X thinks Y is/does Z.", "description": "NP + to-infinitive Type I (raising/believe-type)"}'),
('AB','AB','FA','AA','AP', 'V_NP_INF_II',     'sub_cat', 'verb', '{"slot_count": 2, "nsm_frame": "X does something to Y. Y does Z.", "description": "NP + to-infinitive Type II (control/persuade-type)"}'),
('AB','AB','FA','AA','AQ', 'V_NP_ING',        'sub_cat', 'verb', '{"slot_count": 2, "nsm_frame": "X sees/finds Y doing something.", "description": "NP + -ing complement (perception)"}'),
('AB','AB','FA','AA','AR', 'V_NP_THAT',       'sub_cat', 'verb', '{"slot_count": 2, "nsm_frame": "X says to Y: [proposition].", "description": "NP + that-clause (communication verbs)"}');

-- --- Noun patterns (5) at AB.AB.FA.AB.* ---

INSERT INTO tokens (ns, p2, p3, p4, p5, name, layer, subcategory, metadata) VALUES
('AB','AB','FA','AB','AA', 'N_BARE',          'sub_cat', 'noun', '{"slot_count": 0, "description": "No complement — vast majority of nouns"}'),
('AB','AB','FA','AB','AB', 'N_PP_OF',         'sub_cat', 'noun', '{"slot_count": 1, "description": "PP[of] complement — often deverbal (destruction of, fear of)"}'),
('AB','AB','FA','AB','AC', 'N_PP_VAR',        'sub_cat', 'noun', '{"slot_count": 1, "description": "PP complement with lexically specified preposition (belief in, anger at)"}'),
('AB','AB','FA','AB','AD', 'N_THAT',          'sub_cat', 'noun', '{"slot_count": 1, "description": "That-clause complement (the claim that, the fact that)"}'),
('AB','AB','FA','AB','AE', 'N_INF',           'sub_cat', 'noun', '{"slot_count": 1, "description": "To-infinitive complement (desire to, attempt to)"}');

-- --- Adjective patterns (5) at AB.AB.FA.AC.* ---

INSERT INTO tokens (ns, p2, p3, p4, p5, name, layer, subcategory, metadata) VALUES
('AB','AB','FA','AC','AA', 'A_BARE',          'sub_cat', 'adjective', '{"slot_count": 0, "description": "No complement — majority of adjectives"}'),
('AB','AB','FA','AC','AB', 'A_PP',            'sub_cat', 'adjective', '{"slot_count": 1, "description": "PP complement with lexically specified preposition (fond of, angry at)"}'),
('AB','AB','FA','AC','AC', 'A_THAT',          'sub_cat', 'adjective', '{"slot_count": 1, "description": "That-clause complement (aware that, glad that)"}'),
('AB','AB','FA','AC','AD', 'A_INF_A',         'sub_cat', 'adjective', '{"slot_count": 1, "description": "To-infinitive Type A — subject controls lower subject (eager to, reluctant to)"}'),
('AB','AB','FA','AC','AE', 'A_INF_B',         'sub_cat', 'adjective', '{"slot_count": 1, "description": "To-infinitive Type B — subject is lower object (easy to, impossible to)"}');

-- --- Preposition patterns (3) at AB.AB.FA.AD.* ---

INSERT INTO tokens (ns, p2, p3, p4, p5, name, layer, subcategory, metadata) VALUES
('AB','AB','FA','AD','AA', 'P_NP',            'sub_cat', 'preposition', '{"slot_count": 1, "description": "NP complement — default for all prepositions"}'),
('AB','AB','FA','AD','AB', 'P_S',             'sub_cat', 'preposition', '{"slot_count": 1, "description": "S complement — temporal prepositions (after, before, until, since)"}'),
('AB','AB','FA','AD','AC', 'P_ING',           'sub_cat', 'preposition', '{"slot_count": 1, "description": "-ing clause complement (without leaving, by working)"}');

-- ================================================================
-- SUB_CAT_SLOTS TABLE
-- ================================================================
-- Defines the argument structure for each pattern.
-- Pattern reference is decomposed per Decision 005.
-- Slot categories (NP, PP, S', to-VP, etc.) are LoD-level labels
-- stored as descriptive TEXT, not token references.

CREATE TABLE sub_cat_slots (
    -- Pattern reference (decomposed)
    pat_ns TEXT NOT NULL, pat_p2 TEXT, pat_p3 TEXT, pat_p4 TEXT, pat_p5 TEXT,
    pat_token TEXT GENERATED ALWAYS AS (
        pat_ns ||
        COALESCE('.' || pat_p2, '') ||
        COALESCE('.' || pat_p3, '') ||
        COALESCE('.' || pat_p4, '') ||
        COALESCE('.' || pat_p5, '')
    ) STORED NOT NULL,

    slot_num     SMALLINT NOT NULL,    -- 1, 2, 3

    -- Slot properties (LoD-level labels, not token references)
    slot_cat     TEXT NOT NULL,        -- required category: NP, PP, AP, S', S'', to-VP, -ing-VP, bare-VP
    slot_func    TEXT NOT NULL,        -- grammatical function: dO, iO, sP, oP, PC, comp
    specific_p   TEXT,                 -- for PP slots: which preposition (NULL = any)
    control_type TEXT,                 -- for clausal slots: subj_ctrl, obj_ctrl, raising, NULL

    PRIMARY KEY (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num)
);

CREATE INDEX idx_sub_cat_slots_pat ON sub_cat_slots(pat_ns, pat_p2, pat_p3, pat_p4, pat_p5);

-- ================================================================
-- SLOT DEFINITIONS
-- ================================================================
-- Verb pattern slots

-- V_INTRANS: 0 slots (nothing to insert)

-- V_TRANS: 1 slot
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AA','AB', 1, 'NP', 'dO');

-- V_DITRANS: 2 slots
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AA','AC', 1, 'NP', 'iO'),
('AB','AB','FA','AA','AC', 2, 'NP', 'dO');

-- V_INTENS: 1 slot (AP/NP/PP — multiple categories allowed)
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AA','AD', 1, 'AP|NP|PP', 'sP');

-- V_COMPLEX: 2 slots
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AA','AE', 1, 'NP', 'dO'),
('AB','AB','FA','AA','AE', 2, 'AP|NP|PP', 'oP');

-- V_PREP: 1 slot (specific preposition on the word, not the pattern)
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AA','AF', 1, 'PP', 'PC');

-- V_TRANS_PREP: 2 slots
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AA','AG', 1, 'NP', 'dO'),
('AB','AB','FA','AA','AG', 2, 'PP', 'PC');

-- V_PARTICLE: 1 slot (particle on the word, not the pattern)
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AA','AH', 1, 'NP', 'dO');

-- V_THAT: 1 slot
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AA','AI', 1, 'S''', 'comp');

-- V_WH: 1 slot
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AA','AJ', 1, 'S''''', 'comp');

-- V_INF: 1 slot
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func, control_type) VALUES
('AB','AB','FA','AA','AK', 1, 'to-VP', 'comp', 'subj_ctrl');

-- V_ING: 1 slot
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func, control_type) VALUES
('AB','AB','FA','AA','AL', 1, '-ing-VP', 'comp', 'subj_ctrl');

-- V_BARE: 2 slots
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AA','AM', 1, 'NP', 'dO');
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AA','AM', 2, 'bare-VP', 'comp');

-- V_NP_INF_I: 1 complex slot (NP is subject of lower clause, not main object)
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func, control_type) VALUES
('AB','AB','FA','AA','AN', 1, 'NP+to-VP', 'comp', 'raising');

-- V_NP_INF_II: 2 slots
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AA','AP', 1, 'NP', 'dO');
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func, control_type) VALUES
('AB','AB','FA','AA','AP', 2, 'to-VP', 'comp', 'obj_ctrl');

-- V_NP_ING: 2 slots
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AA','AQ', 1, 'NP', 'dO'),
('AB','AB','FA','AA','AQ', 2, '-ing-VP', 'comp');

-- V_NP_THAT: 2 slots
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AA','AR', 1, 'NP', 'iO'),
('AB','AB','FA','AA','AR', 2, 'S''', 'comp');

-- Noun pattern slots

-- N_BARE: 0 slots

-- N_PP_OF: 1 slot
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func, specific_p) VALUES
('AB','AB','FA','AB','AB', 1, 'PP', 'comp', 'of');

-- N_PP_VAR: 1 slot (preposition specified per noun, not per pattern)
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AB','AC', 1, 'PP', 'comp');

-- N_THAT: 1 slot
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AB','AD', 1, 'S''', 'comp');

-- N_INF: 1 slot
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AB','AE', 1, 'to-VP', 'comp');

-- Adjective pattern slots

-- A_BARE: 0 slots

-- A_PP: 1 slot
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AC','AB', 1, 'PP', 'comp');

-- A_THAT: 1 slot
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AC','AC', 1, 'S''', 'comp');

-- A_INF_A: 1 slot (subject control)
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func, control_type) VALUES
('AB','AB','FA','AC','AD', 1, 'to-VP', 'comp', 'subj_ctrl');

-- A_INF_B: 1 slot (tough-movement — subject is lower object)
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func, control_type) VALUES
('AB','AB','FA','AC','AE', 1, 'to-VP', 'comp', 'tough_mvmt');

-- Preposition pattern slots

-- P_NP: 1 slot
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AD','AA', 1, 'NP', 'comp');

-- P_S: 1 slot
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AD','AB', 1, 'S', 'comp');

-- P_ING: 1 slot
INSERT INTO sub_cat_slots (pat_ns, pat_p2, pat_p3, pat_p4, pat_p5, slot_num, slot_cat, slot_func) VALUES
('AB','AB','FA','AD','AC', 1, '-ing-VP', 'comp');

-- ================================================================
-- TOKEN_SUB_CAT JUNCTION TABLE
-- ================================================================
-- Maps word tokens to their sub-categorization patterns.
-- This is THE authority on structural capabilities (per ling specialist).
-- Dual-PoS words (run, give) get entries for all patterns they support,
-- regardless of which p3 namespace their primary entry sits in.

CREATE TABLE token_sub_cat (
    -- Word token reference (decomposed)
    tok_ns TEXT NOT NULL, tok_p2 TEXT, tok_p3 TEXT, tok_p4 TEXT, tok_p5 TEXT,
    tok_token TEXT GENERATED ALWAYS AS (
        tok_ns ||
        COALESCE('.' || tok_p2, '') ||
        COALESCE('.' || tok_p3, '') ||
        COALESCE('.' || tok_p4, '') ||
        COALESCE('.' || tok_p5, '')
    ) STORED NOT NULL,

    -- Pattern reference (decomposed)
    pat_ns TEXT NOT NULL, pat_p2 TEXT, pat_p3 TEXT, pat_p4 TEXT, pat_p5 TEXT,
    pat_token TEXT GENERATED ALWAYS AS (
        pat_ns ||
        COALESCE('.' || pat_p2, '') ||
        COALESCE('.' || pat_p3, '') ||
        COALESCE('.' || pat_p4, '') ||
        COALESCE('.' || pat_p5, '')
    ) STORED NOT NULL,

    frequency    REAL NOT NULL DEFAULT 0.5,  -- relative frequency weight (0.0-1.0)
    specific_p   TEXT,                        -- verb-specific preposition (for V_PREP, V_TRANS_PREP)
    particle     TEXT,                        -- for V_PARTICLE: the particle word

    PRIMARY KEY (tok_ns, tok_p2, tok_p3, tok_p4, tok_p5,
                 pat_ns, pat_p2, pat_p3, pat_p4, pat_p5)
);

-- Reverse index: find all words with a given pattern
CREATE INDEX idx_token_sub_cat_pat ON token_sub_cat(
    pat_ns, pat_p2, pat_p3, pat_p4, pat_p5
);

-- Token lookup: find all patterns for a given word
CREATE INDEX idx_token_sub_cat_tok ON token_sub_cat(
    tok_ns, tok_p2, tok_p3, tok_p4, tok_p5
);

COMMIT;

-- ================================================================
-- VERIFICATION
-- ================================================================

\echo ''
\echo '=== Sub-cat pattern tokens ==='
SELECT token_id, name, subcategory
FROM tokens
WHERE ns = 'AB' AND p2 = 'AB' AND p3 = 'FA'
ORDER BY token_id;

\echo ''
\echo '=== Slot definitions ==='
SELECT pat_token, slot_num, slot_cat, slot_func, specific_p, control_type
FROM sub_cat_slots
ORDER BY pat_token, slot_num;

\echo ''
\echo '=== Pattern count by PoS ==='
SELECT subcategory, COUNT(*) AS patterns
FROM tokens
WHERE ns = 'AB' AND p2 = 'AB' AND p3 = 'FA'
GROUP BY subcategory ORDER BY subcategory;

\echo ''
\echo '=== Slot count ==='
SELECT COUNT(*) AS total_slots FROM sub_cat_slots;
