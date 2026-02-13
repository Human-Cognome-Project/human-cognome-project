-- 005d: Create sense_glosses junction table (~8M+ rows)
--
-- This is the largest junction table: senses.gloss_tokens arrays
-- have up to 152 elements across 1.49M senses.
--
-- Run against: hcp_english

BEGIN;

DO $$
DECLARE cnt BIGINT;
BEGIN
    RAISE NOTICE '=== Creating sense_glosses junction table ===';

    CREATE TABLE sense_glosses (
        sense_id INTEGER NOT NULL REFERENCES senses(id),
        position SMALLINT NOT NULL,
        ns TEXT NOT NULL,
        p2 TEXT,
        p3 TEXT,
        p4 TEXT,
        p5 TEXT,
        token_id TEXT GENERATED ALWAYS AS (
            ns || COALESCE('.' || p2, '') || COALESCE('.' || p3, '') ||
            COALESCE('.' || p4, '') || COALESCE('.' || p5, '')
        ) STORED NOT NULL,
        PRIMARY KEY (sense_id, position)
    );

    RAISE NOTICE 'Table created, inserting rows...';

    INSERT INTO sense_glosses (sense_id, position, ns, p2, p3, p4, p5)
    SELECT s_row.id, (ord.n - 1)::SMALLINT, s.ns, s.p2, s.p3, s.p4, s.p5
    FROM senses s_row,
         LATERAL unnest(s_row.gloss_tokens) WITH ORDINALITY AS ord(token, n),
         LATERAL split_token_id(ord.token) AS s
    WHERE s_row.gloss_tokens IS NOT NULL
      AND array_length(s_row.gloss_tokens, 1) > 0;
    GET DIAGNOSTICS cnt = ROW_COUNT;
    RAISE NOTICE 'sense_glosses: % rows inserted', cnt;

    CREATE INDEX idx_sense_glosses_token ON sense_glosses(ns, p2, p3, p4, p5);
    RAISE NOTICE 'Index created';

    ALTER TABLE senses DROP COLUMN gloss_tokens;
    RAISE NOTICE 'sense_glosses: done';
END;
$$;

ANALYZE sense_glosses;
COMMIT;
