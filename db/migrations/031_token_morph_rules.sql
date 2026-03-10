-- Migration 031: token_morph_rules — pre-assigned inflection rules per (token, morpheme)
--
-- Context:
--   The inflection rule a word uses for each morpheme is characterising data about
--   the word itself — its inflection class. Two words sharing the same rule_id for PAST
--   are in the same conjugation class (analogous to Latin conjugation classes).
--
-- Purpose:
--   Avoids calling apply_inflection() (cursor + regex loop) at envelope assembly time.
--   Each (token, morpheme) pair gets a single row with pre-computed strip_suffix and
--   add_suffix, so assembly becomes a plain JOIN instead of a function call.
--
-- Population:
--   Rows are inserted during Kaikki Pass 2 (token_pos insertion), alongside token_pos rows,
--   in the same transaction. Only regular tokens get rows here — tokens with an explicit
--   irregular variant in token_variants for a given morpheme do NOT get a row for that
--   morpheme. The irregular IS the inflection.
--
-- __DOUBLING__ pre-computation:
--   apply_doubling_rule() is called once per token at population time. The doubled
--   consonant is absorbed into add_suffix:
--     tap  PAST        → strip='',  add='ped'  → 'tap'  + 'ped'  = 'tapped'
--     run  PROGRESSIVE → strip='',  add='ning' → 'run'  + 'ning' = 'running'
--     big  COMPARATIVE → strip='',  add='ger'  → 'big'  + 'ger'  = 'bigger'
--
-- Envelope query change (deferred — migration 032 after Pass 2):
--   Once this table is populated, envelope_queries rows in hcp_core can replace:
--     SELECT apply_inflection(t.name, 'PAST') AS name, t.token_id FROM tokens t
--     JOIN token_pos tp ON ... WHERE ... AND (tp.morpheme_accept & 2) != 0
--     AND NOT EXISTS (SELECT 1 FROM token_variants tv WHERE ...)
--   with:
--     SELECT left(t.name, length(t.name) - length(tmr.strip_suffix)) || tmr.add_suffix AS name,
--            t.token_id
--     FROM tokens t
--     JOIN token_pos tp  ON tp.token_id = t.token_id AND tp.pos = 'V_MAIN'
--     JOIN token_morph_rules tmr ON tmr.token_id = t.token_id AND tmr.morpheme = 'PAST'
--     WHERE ...
--   The presence of a token_morph_rules row implies regular form (no variant override
--   needed) — morpheme_accept bit check and NOT EXISTS on token_variants are implicit.
--   The envelope_queries are NOT updated in this migration because the table is empty
--   until Pass 2 runs. Updating them now would produce zero results.

\connect hcp_english

-- ============================================================
-- token_morph_rules
-- ============================================================

CREATE TABLE token_morph_rules (
    id           SERIAL  PRIMARY KEY,
    token_id     TEXT    NOT NULL REFERENCES tokens (token_id) ON DELETE CASCADE,
    morpheme     TEXT    NOT NULL,   -- 'PAST', 'PLURAL', 'PROGRESSIVE', '3RD_SING', etc.
    rule_id      INTEGER NOT NULL REFERENCES inflection_rules (id),

    -- Pre-computed assembly values (avoids join to inflection_rules at query time).
    -- For __DOUBLING__ rules: strip_suffix='', add_suffix = doubled_char + suffix.
    --   e.g. 'tap'  PAST        → strip='', add='ped'  (not 'ed') — doubling absorbed
    --   e.g. 'run'  PROGRESSIVE → strip='', add='ning'
    strip_suffix TEXT    NOT NULL DEFAULT '',
    add_suffix   TEXT    NOT NULL DEFAULT '',

    UNIQUE (token_id, morpheme)
);

CREATE INDEX idx_morph_rules_token    ON token_morph_rules (token_id);
CREATE INDEX idx_morph_rules_morpheme ON token_morph_rules (morpheme);

-- ============================================================
-- Verify
-- ============================================================

SELECT
    (SELECT count(*) FROM token_morph_rules) AS token_morph_rules_rows;

\d token_morph_rules
