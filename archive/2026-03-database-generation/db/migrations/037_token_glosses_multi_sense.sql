-- Migration 037: token_glosses multi-sense support + token_pos gloss_id removal
--
-- Fixes three schema issues identified before Kaikki re-population:
--
-- 1. token_glosses: drop UNIQUE(token_id, pos), add sense_number, new unique
--    constraint UNIQUE(token_id, pos, sense_number). Allows multiple senses
--    per (token, PoS) as Kaikki presents them (e.g. "bank" N_COMMON has many).
--
-- 2. token_glosses: add tags TEXT[] for Kaikki domain/topic tags
--    (e.g. 'medicine', 'legal', 'botany', 'archaic'). GIN-indexed for
--    array containment queries.
--
-- 3. token_pos: drop gloss_id column + FK. With multiple senses, a single
--    pointer to one gloss is meaningless. Glosses are looked up directly
--    via (token_id, pos, sense_number).
--
-- DB is clean (no data) so no data migration needed.

\connect hcp_english

BEGIN;

-- ============================================================
-- 1. token_glosses: drop old unique, add sense_number + tags
-- ============================================================

ALTER TABLE token_glosses
    DROP CONSTRAINT token_glosses_token_id_pos_key;

ALTER TABLE token_glosses
    ADD COLUMN sense_number SMALLINT NOT NULL DEFAULT 1,
    ADD COLUMN tags TEXT[] NOT NULL DEFAULT '{}';

ALTER TABLE token_glosses
    ADD CONSTRAINT token_glosses_token_id_pos_sense_key
        UNIQUE (token_id, pos, sense_number);

CREATE INDEX idx_glosses_tags ON token_glosses USING GIN (tags);

-- ============================================================
-- 2. token_pos: drop gloss_id FK and column
-- ============================================================

ALTER TABLE token_pos
    DROP CONSTRAINT fk_token_pos_gloss,
    DROP COLUMN gloss_id;

-- ============================================================
-- Verify
-- ============================================================

SELECT column_name, data_type, is_nullable, column_default
FROM information_schema.columns
WHERE table_name = 'token_glosses'
ORDER BY ordinal_position;

SELECT column_name FROM information_schema.columns
WHERE table_name = 'token_pos'
ORDER BY ordinal_position;

COMMIT;
