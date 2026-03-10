-- Migration 029: New hcp_english schema — token_pos, token_glosses,
--                token_variants, inflection_rules
--
-- Implements the schema designed in docs/hcp-english-schema-design.md.
-- This is a NON-DESTRUCTIVE migration: existing tokens columns are kept
-- intact for the transition period. New tables are added alongside.
-- Old columns (layer, subcategory, aux_type, category, canonical_id) are
-- retired AFTER Kaikki population validates replacements.
--
-- Changes to existing `tokens` table:
--   + source_language CHAR(3)   — ISO 639-1/3 for loanword roots
--   + characteristics INTEGER   — 32-bit bitmask (register/temporal/geographic)
--
-- New tables:
--   token_pos        — PoS records per (token, PoS) pair
--   token_glosses    — Gloss + NSM prime refs per (token, PoS) pair
--   token_variants   — Irregular/archaic/dialect/spelling variant forms
--   inflection_rules — Parameterised regular inflection rules for assembly queries
--
-- Token_id coordinate scheme for NEW Kaikki entries (Patrick 2026-03-10):
--   p3 = starting character of word (a=AA, b=AB, c=AC, ..., z=AZ)
--   p4 = word length in chars  (1=AA, 2=AB, 3=AC, ..., 50=BX)
--   p5 = sequential within (starting_char, length) bucket, 0-indexed
--   Capacity: 2500 per bucket (base-50 p5). Dense buckets (e.g. s-words
--   len 7) may approach limit — monitored during population.
--
-- Existing token_ids are preserved exactly. New allocation uses the above.

\connect hcp_english

-- ============================================================
-- 1. Add new columns to tokens
-- ============================================================

ALTER TABLE tokens
    ADD COLUMN IF NOT EXISTS source_language CHAR(3),
    ADD COLUMN IF NOT EXISTS characteristics  INTEGER NOT NULL DEFAULT 0;

CREATE INDEX IF NOT EXISTS idx_tokens_source_lang
    ON tokens (source_language)
    WHERE source_language IS NOT NULL;

CREATE INDEX IF NOT EXISTS idx_tokens_characteristics
    ON tokens (characteristics)
    WHERE characteristics != 0;

-- ============================================================
-- 2. pos_tag enum
-- ============================================================

CREATE TYPE pos_tag AS ENUM (
    'N_COMMON',    -- common noun
    'N_PROPER',    -- proper noun / Label (always-capitalize)
    'N_PRONOUN',   -- pronoun
    'V_MAIN',      -- main verb
    'V_AUX',       -- auxiliary verb (be, have, do, will, shall, may...)
    'V_COPULA',    -- copula (be as linking verb)
    'ADJ',         -- adjective
    'ADV',         -- adverb
    'PREP',        -- preposition
    'CONJ_COORD',  -- coordinating conjunction (and, but, or, nor...)
    'CONJ_SUB',    -- subordinating conjunction (because, although...)
    'DET',         -- determiner (the, a, some, this, my...)
    'INTJ',        -- interjection (oh, ah, hey...)
    'PART',        -- particle (up in "give up")
    'NUM'          -- numeral
);

-- ============================================================
-- 3. token_pos — PoS records
-- ============================================================

CREATE TABLE token_pos (
    id              SERIAL      PRIMARY KEY,
    token_id        TEXT        NOT NULL REFERENCES tokens (token_id) ON DELETE CASCADE,
    pos             pos_tag     NOT NULL,

    -- Most common PoS for this token (fast single-lookup path)
    is_primary      BOOLEAN     NOT NULL DEFAULT false,

    -- Capitalisation property (N_PROPER only)
    --   'start_cap' = first letter capitalised (London, Patrick)
    --   'all_cap'   = all caps initialism used as word (fbi, nato)
    --   NULL        = not applicable
    cap_property    TEXT        CHECK (cap_property IN ('start_cap', 'all_cap')),

    -- FK to gloss (nullable — glosses are populated iteratively)
    gloss_id        INTEGER,    -- FK → token_glosses.id, set after gloss population

    -- What regular inflections this (token, PoS) accepts (bitmask)
    --   bit 0: MORPH_PLURAL         bit 1: MORPH_PAST
    --   bit 2: MORPH_PROGRESSIVE    bit 3: MORPH_3RD_SING
    --   bit 4: MORPH_COMPARATIVE    bit 5: MORPH_SUPERLATIVE
    --   bit 6: MORPH_ADVERB_LY      bit 7: MORPH_POSSESSIVE
    --   bit 8: MORPH_GERUND
    morpheme_accept INTEGER     NOT NULL DEFAULT 0,

    -- Per-PoS register/temporal/geographic characteristics (bits 0–19)
    --   Register:  FORMAL=0 CASUAL=1 SLANG=2 VULGAR=3 DEROGATORY=4
    --              LITERARY=5 TECHNICAL=6
    --   Temporal:  ARCHAIC=8 DATED=9 NEOLOGISM=10
    --   Geographic: DIALECT=12 BRITISH=13 AMERICAN=14 AUSTRALIAN=15
    characteristics INTEGER     NOT NULL DEFAULT 0,

    UNIQUE (token_id, pos)
);

CREATE INDEX idx_token_pos_token   ON token_pos (token_id);
CREATE INDEX idx_token_pos_pos     ON token_pos (pos);
CREATE INDEX idx_token_pos_primary ON token_pos (token_id)
    WHERE is_primary = true;
CREATE INDEX idx_token_pos_chars   ON token_pos (characteristics)
    WHERE characteristics != 0;

-- ============================================================
-- 4. token_glosses — Gloss records
-- ============================================================

CREATE TABLE token_glosses (
    id              SERIAL      PRIMARY KEY,
    token_id        TEXT        NOT NULL REFERENCES tokens (token_id) ON DELETE CASCADE,
    pos             pos_tag     NOT NULL,

    -- Primary gloss text (Kaikki first sense, or manually written)
    gloss_text      TEXT        NOT NULL,

    -- NSM prime references: JSON array of NSM prime token_ids
    --   e.g. ["AA.AB.AC.AA.AB", "AA.AB.AC.AA.AC"]
    --   NULL = not yet mapped to NSM
    nsm_prime_refs  JSONB,

    -- Free-text disambiguation note
    nuance_note     TEXT,

    -- Population status
    status          TEXT        NOT NULL DEFAULT 'DRAFT'
                    CHECK (status IN ('DRAFT', 'REVIEWED', 'CONFIRMED')),

    created_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT now(),

    UNIQUE (token_id, pos)
);

CREATE INDEX idx_glosses_token  ON token_glosses (token_id);
CREATE INDEX idx_glosses_status ON token_glosses (status);

-- Wire gloss_id FK constraint now that token_glosses exists
ALTER TABLE token_pos
    ADD CONSTRAINT fk_token_pos_gloss
    FOREIGN KEY (gloss_id) REFERENCES token_glosses (id)
    ON DELETE SET NULL;

-- ============================================================
-- 5. token_variants — Irregular/special surface forms
-- ============================================================

CREATE TABLE token_variants (
    id              SERIAL      PRIMARY KEY,

    -- Root token this variant resolves to
    canonical_id    TEXT        NOT NULL REFERENCES tokens (token_id) ON DELETE CASCADE,

    -- Surface form — always lowercase
    name            TEXT        NOT NULL,

    -- PoS scope (NULL = applies to all PoS roles of canonical)
    pos             pos_tag,

    -- Morpheme / variant type:
    --   Core inflection irregulars: PAST, PLURAL, PROGRESSIVE
    --   Archaic inflection irregulars: ARCHAIC_PAST, ARCHAIC_PLURAL
    --   Dialect/register: DIALECT, EYE_DIALECT, CONTRACTION, CONTRACTION_NEG
    --   Orthographic: SPELLING_US, SPELLING_UK, SPELLING_AU, SPELLING_VARIANT
    --   Provenance: MISSPELLING
    morpheme        TEXT,

    -- Characteristic bitmask — derivation bits (20–27) + register/geo bits
    --   STANDARD_RULE=20 IRREGULAR=21 SPELLING_VARIANT=22 EYE_DIALECT=23
    --   BORROWING=24 COMPOUND=25 ABBREVIATION=26
    --   + applicable bits from register/temporal/geographic dimensions
    characteristics INTEGER     NOT NULL DEFAULT 0,

    -- Source language when BORROWING bit (24) is set (ISO 639-1/3)
    source_language CHAR(3),

    -- Optional note (e.g. "misspelling of 'receive'", "Scottish dialect form")
    note            TEXT
);

-- UNIQUE on (canonical_id, name, morpheme) — uses index to handle NULL morpheme
-- (UNIQUE constraint can't use COALESCE; unique index can)
CREATE INDEX idx_variants_canonical  ON token_variants (canonical_id);
CREATE INDEX idx_variants_name       ON token_variants (name);
CREATE INDEX idx_variants_morpheme   ON token_variants (morpheme)
    WHERE morpheme IS NOT NULL;
CREATE INDEX idx_variants_chars      ON token_variants (characteristics)
    WHERE characteristics != 0;
CREATE UNIQUE INDEX idx_variants_unique
    ON token_variants (canonical_id, name, COALESCE(morpheme, ''));

-- ============================================================
-- 6. inflection_rules — Regular inflection assembly rules
-- ============================================================

CREATE TABLE inflection_rules (
    id              SERIAL      PRIMARY KEY,
    morpheme        TEXT        NOT NULL,   -- 'PAST', 'PLURAL', 'PROGRESSIVE', etc.
    priority        INTEGER     NOT NULL,   -- Lower = checked first
    condition       TEXT        NOT NULL,   -- POSIX regex applied to root name
    strip_suffix    TEXT        NOT NULL DEFAULT '',
    add_suffix      TEXT        NOT NULL DEFAULT '',
    description     TEXT,

    UNIQUE (morpheme, priority)
);

-- Seed standard rules (complete set for core morphemes)
-- Applied in priority order: first condition match wins.
-- 'doubling' entries are placeholders — the doubling pattern (CVC root)
-- requires a stored function, not a simple regex+suffix. Marked by
-- strip_suffix='__DOUBLING__' as a signal to the assembly query.

INSERT INTO inflection_rules (morpheme, priority, condition, strip_suffix, add_suffix, description) VALUES
-- PAST tense
('PAST',  1,  '[^aeiou]e$',                       'e',   'ed',  'silent-e drop: like→liked, move→moved'),
('PAST',  2,  '[aeiou][bcdfghjklmnprstvwxz]$',    '__DOUBLING__', 'ed', 'CVC doubling: tap→tapped (fn required)'),
('PAST',  99, '.*',                                '',    'ed',  'default: walk→walked'),

-- PLURAL
('PLURAL', 1,  '(s|x|z|ch|sh)$',                 '',    'es',  'sibilant: kiss→kisses, box→boxes'),
('PLURAL', 2,  '[^aeiou]y$',                      'y',   'ies', 'consonant+y: city→cities, baby→babies'),
('PLURAL', 3,  '[^aeiou]fe?$',                    'fe',  'ves', '-fe→-ves: knife→knives (partial rule)'),
('PLURAL', 99, '.*',                               '',    's',   'default: cat→cats'),

-- PROGRESSIVE (-ing)
('PROGRESSIVE', 1,  '[^aeiou]e$',                 'e',   'ing', 'silent-e drop: make→making, ride→riding'),
('PROGRESSIVE', 2,  '[aeiou][bcdfghjklmnprstvwxz]$', '__DOUBLING__', 'ing', 'CVC doubling: run→running (fn required)'),
('PROGRESSIVE', 99, '.*',                          '',    'ing', 'default: walk→walking'),

-- 3RD_SING (3rd person singular present)
('3RD_SING', 1,  '(s|x|z|ch|sh|[^aeiou]o)$',     '',    'es',  'sibilant/o: pass→passes, go→goes'),
('3RD_SING', 2,  '[^aeiou]y$',                    'y',   'ies', 'consonant+y: fly→flies, try→tries'),
('3RD_SING', 99, '.*',                             '',    's',   'default: walk→walks'),

-- COMPARATIVE (-er)
('COMPARATIVE', 1,  '[^aeiou]e$',                 '',    'r',   'e-final: large→larger, nice→nicer'),
('COMPARATIVE', 2,  '[aeiou][bcdfghjklmnprstvwxz]$', '__DOUBLING__', 'er', 'CVC doubling: big→bigger (fn required)'),
('COMPARATIVE', 3,  '[^aeiou]y$',                 'y',   'ier', 'consonant+y: happy→happier'),
('COMPARATIVE', 99, '.*',                          '',    'er',  'default: fast→faster'),

-- SUPERLATIVE (-est)
('SUPERLATIVE', 1,  '[^aeiou]e$',                 '',    'st',  'e-final: large→largest, nice→nicest'),
('SUPERLATIVE', 2,  '[aeiou][bcdfghjklmnprstvwxz]$', '__DOUBLING__', 'est', 'CVC doubling: big→biggest (fn required)'),
('SUPERLATIVE', 3,  '[^aeiou]y$',                 'y',   'iest','consonant+y: happy→happiest'),
('SUPERLATIVE', 99, '.*',                          '',    'est', 'default: fast→fastest'),

-- ADVERB_LY (-ly from adjective)
('ADVERB_LY', 1,  '[^aeiou]le$',                  'e',   'y',   'le-final: simple→simply, gentle→gently'),
('ADVERB_LY', 2,  '[^aeiou]y$',                   'y',   'ily', 'consonant+y: happy→happily, easy→easily'),
('ADVERB_LY', 3,  'ic$',                          '',    'ally','ic-final: basic→basically, logic→logically'),
('ADVERB_LY', 99, '.*',                            '',    'ly',  'default: quick→quickly, slow→slowly'),

-- POSSESSIVE ('s)
('POSSESSIVE', 1,  's$',                           '',    '''s', 's-final: boss→boss''s'),
('POSSESSIVE', 99, '.*',                           '',    '''s', 'default: cat→cat''s');

-- ============================================================
-- Verify
-- ============================================================

SELECT
    (SELECT count(*) FROM token_pos)        AS token_pos_rows,
    (SELECT count(*) FROM token_glosses)    AS token_glosses_rows,
    (SELECT count(*) FROM token_variants)   AS token_variants_rows,
    (SELECT count(*) FROM inflection_rules) AS inflection_rules_rows;

SELECT morpheme, count(*) AS rules
FROM inflection_rules
GROUP BY morpheme
ORDER BY morpheme;

\d token_pos
\d token_glosses
\d token_variants
\d inflection_rules
