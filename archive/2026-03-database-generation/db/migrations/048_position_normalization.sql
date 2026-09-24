-- SUPERSEDED by 049_position_arrays.sql — do not re-run.
-- The row-per-position approach (pbm_positions) caused Postgres tuple overhead
-- to balloon the database from 39 MB to 101 MB on the 9-doc dataset.
-- Migration 049 drops these tables and uses INTEGER[] on pbm_starters instead.
--
-- Migration 048: position normalization
--
-- Splits the packed base-50 TEXT position blobs in pbm_starters and
-- pbm_morpheme_positions into normalized rows. Postgres does the heavy
-- lifting from here — heap pages share repeated values, B-tree leaves
-- share key prefixes, INT positions are 4 bytes flat with no decode CPU.
--
-- BEFORE: pbm_starters.positions = "AAAAAEAAAA..." (4 base-50 chars per pos)
-- AFTER:  pbm_positions(starter_id, position) — one row per occurrence
--
-- Apply against: hcp_fic_pbm (and any future *_pbm shards)

\set ON_ERROR_STOP on

BEGIN;

-- ---------------------------------------------------------------------------
-- Helper: decode 4-char base-50 position string → INT
-- Inverse of HCPDbUtils::EncodePosition. Used only during migration.
-- ---------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION decode_b50_char(c CHAR) RETURNS INT AS $$
DECLARE
    code INT := ascii(c);
BEGIN
    IF code BETWEEN ascii('A') AND ascii('Z') THEN RETURN code - ascii('A'); END IF;
    IF code BETWEEN ascii('a') AND ascii('x') THEN RETURN 26 + (code - ascii('a')); END IF;
    RETURN 0;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION decode_b50_position(s TEXT) RETURNS INT AS $$
DECLARE
    pair1 INT;
    pair2 INT;
BEGIN
    pair1 := decode_b50_char(substring(s FROM 1 FOR 1)) * 50
           + decode_b50_char(substring(s FROM 2 FOR 1));
    pair2 := decode_b50_char(substring(s FROM 3 FOR 1)) * 50
           + decode_b50_char(substring(s FROM 4 FOR 1));
    RETURN pair1 * 2500 + pair2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

-- ---------------------------------------------------------------------------
-- pbm_positions: one row per token occurrence
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS pbm_positions (
    starter_id INTEGER NOT NULL REFERENCES pbm_starters(id) ON DELETE CASCADE,
    position   INTEGER NOT NULL,
    PRIMARY KEY (starter_id, position)
);

-- Migrate existing packed payloads
INSERT INTO pbm_positions (starter_id, position)
SELECT s.id,
       decode_b50_position(substring(s.positions FROM ((n - 1) * 4) + 1 FOR 4))
FROM   pbm_starters s
       CROSS JOIN LATERAL generate_series(1, length(s.positions) / 4) AS n
WHERE  s.positions IS NOT NULL
  AND  length(s.positions) >= 4
ON CONFLICT DO NOTHING;

ALTER TABLE pbm_starters DROP COLUMN positions;

-- ---------------------------------------------------------------------------
-- pbm_cap_flags: replaces pbm_morpheme_positions for FIRST_CAP / ALL_CAPS
-- (Other morpheme bits are no longer written — every form is its own token.)
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS pbm_cap_flags (
    doc_id   INTEGER NOT NULL REFERENCES pbm_documents(id) ON DELETE CASCADE,
    position INTEGER NOT NULL,
    flag     SMALLINT NOT NULL,  -- bit 0 = FIRST_CAP, bit 1 = ALL_CAPS
    PRIMARY KEY (doc_id, position)
);

INSERT INTO pbm_cap_flags (doc_id, position, flag)
SELECT m.doc_id,
       decode_b50_position(substring(m.positions FROM ((n - 1) * 4) + 1 FOR 4)),
       CASE WHEN m.morpheme = 'ALL_CAPS' THEN 2 ELSE 1 END
FROM   pbm_morpheme_positions m
       CROSS JOIN LATERAL generate_series(1, length(m.positions) / 4) AS n
WHERE  m.morpheme IN ('FIRST_CAP', 'ALL_CAPS')
  AND  m.positions IS NOT NULL
  AND  length(m.positions) >= 4
ON CONFLICT (doc_id, position) DO UPDATE
   SET flag = pbm_cap_flags.flag | EXCLUDED.flag;

DROP TABLE pbm_morpheme_positions;

-- Drop helper functions — only needed during migration
DROP FUNCTION decode_b50_position(TEXT);
DROP FUNCTION decode_b50_char(CHAR);

COMMIT;

-- Reclaim space and refresh stats (cannot run inside transaction)
VACUUM FULL pbm_starters;
VACUUM ANALYZE pbm_positions;
VACUUM ANALYZE pbm_cap_flags;
