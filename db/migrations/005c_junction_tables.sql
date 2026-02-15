-- 005c: Create junction tables for token ID arrays (small + medium)
--
-- Replaces TEXT[] array columns with junction tables containing
-- decomposed (ns, p2, p3, p4, p5) columns + position for ordering.
--
-- Tables created (ordered by size):
--   relation_tags      ~14K rows    (from relations.tag_tokens)
--   form_components    ~400K rows   (from forms.form_tokens)
--   sense_tags         ~1.6M rows   (from senses.tag_tokens)
--   entry_etymology    ~2M rows     (from entries.etymology_tokens)
--   form_tags          ~2.3M rows   (from forms.tag_tokens)
--
-- Run against: hcp_english

-- ============================================================================
-- relation_tags (~14K rows from relations.tag_tokens)
-- ============================================================================
BEGIN;

DO $$
DECLARE cnt BIGINT;
BEGIN
    RAISE NOTICE '=== Creating relation_tags junction table ===';

    CREATE TABLE relation_tags (
        relation_id INTEGER NOT NULL REFERENCES relations(id),
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
        PRIMARY KEY (relation_id, position)
    );

    INSERT INTO relation_tags (relation_id, position, ns, p2, p3, p4, p5)
    SELECT r.id, (ord.n - 1)::SMALLINT, s.ns, s.p2, s.p3, s.p4, s.p5
    FROM relations r,
         LATERAL unnest(r.tag_tokens) WITH ORDINALITY AS ord(token, n),
         LATERAL split_token_id(ord.token) AS s
    WHERE r.tag_tokens IS NOT NULL
      AND array_length(r.tag_tokens, 1) > 0;
    GET DIAGNOSTICS cnt = ROW_COUNT;
    RAISE NOTICE 'relation_tags: % rows inserted', cnt;

    CREATE INDEX idx_relation_tags_token ON relation_tags(ns, p2, p3, p4, p5);

    ALTER TABLE relations DROP COLUMN tag_tokens;
    RAISE NOTICE 'relation_tags: done';
END;
$$;

ANALYZE relation_tags;
COMMIT;

-- ============================================================================
-- form_components (~400K rows from forms.form_tokens)
-- ============================================================================
BEGIN;

DO $$
DECLARE cnt BIGINT;
BEGIN
    RAISE NOTICE '=== Creating form_components junction table ===';

    CREATE TABLE form_components (
        form_id INTEGER NOT NULL REFERENCES forms(id),
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
        PRIMARY KEY (form_id, position)
    );

    INSERT INTO form_components (form_id, position, ns, p2, p3, p4, p5)
    SELECT f.id, (ord.n - 1)::SMALLINT, s.ns, s.p2, s.p3, s.p4, s.p5
    FROM forms f,
         LATERAL unnest(f.form_tokens) WITH ORDINALITY AS ord(token, n),
         LATERAL split_token_id(ord.token) AS s
    WHERE f.form_tokens IS NOT NULL
      AND array_length(f.form_tokens, 1) > 0;
    GET DIAGNOSTICS cnt = ROW_COUNT;
    RAISE NOTICE 'form_components: % rows inserted', cnt;

    CREATE INDEX idx_form_components_token ON form_components(ns, p2, p3, p4, p5);

    ALTER TABLE forms DROP COLUMN form_tokens;
    RAISE NOTICE 'form_components: done';
END;
$$;

ANALYZE form_components;
COMMIT;

-- ============================================================================
-- sense_tags (~1.6M rows from senses.tag_tokens)
-- ============================================================================
BEGIN;

DO $$
DECLARE cnt BIGINT;
BEGIN
    RAISE NOTICE '=== Creating sense_tags junction table ===';

    CREATE TABLE sense_tags (
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

    INSERT INTO sense_tags (sense_id, position, ns, p2, p3, p4, p5)
    SELECT s_row.id, (ord.n - 1)::SMALLINT, s.ns, s.p2, s.p3, s.p4, s.p5
    FROM senses s_row,
         LATERAL unnest(s_row.tag_tokens) WITH ORDINALITY AS ord(token, n),
         LATERAL split_token_id(ord.token) AS s
    WHERE s_row.tag_tokens IS NOT NULL
      AND array_length(s_row.tag_tokens, 1) > 0;
    GET DIAGNOSTICS cnt = ROW_COUNT;
    RAISE NOTICE 'sense_tags: % rows inserted', cnt;

    CREATE INDEX idx_sense_tags_token ON sense_tags(ns, p2, p3, p4, p5);

    ALTER TABLE senses DROP COLUMN tag_tokens;
    RAISE NOTICE 'sense_tags: done';
END;
$$;

ANALYZE sense_tags;
COMMIT;

-- ============================================================================
-- entry_etymology (~2M rows from entries.etymology_tokens)
-- ============================================================================
BEGIN;

DO $$
DECLARE cnt BIGINT;
BEGIN
    RAISE NOTICE '=== Creating entry_etymology junction table ===';

    CREATE TABLE entry_etymology (
        entry_id INTEGER NOT NULL REFERENCES entries(id),
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
        PRIMARY KEY (entry_id, position)
    );

    INSERT INTO entry_etymology (entry_id, position, ns, p2, p3, p4, p5)
    SELECT e.id, (ord.n - 1)::SMALLINT, s.ns, s.p2, s.p3, s.p4, s.p5
    FROM entries e,
         LATERAL unnest(e.etymology_tokens) WITH ORDINALITY AS ord(token, n),
         LATERAL split_token_id(ord.token) AS s
    WHERE e.etymology_tokens IS NOT NULL
      AND array_length(e.etymology_tokens, 1) > 0;
    GET DIAGNOSTICS cnt = ROW_COUNT;
    RAISE NOTICE 'entry_etymology: % rows inserted', cnt;

    CREATE INDEX idx_entry_etymology_token ON entry_etymology(ns, p2, p3, p4, p5);

    ALTER TABLE entries DROP COLUMN etymology_tokens;
    RAISE NOTICE 'entry_etymology: done';
END;
$$;

ANALYZE entry_etymology;
COMMIT;

-- ============================================================================
-- form_tags (~2.3M rows from forms.tag_tokens)
-- ============================================================================
BEGIN;

DO $$
DECLARE cnt BIGINT;
BEGIN
    RAISE NOTICE '=== Creating form_tags junction table ===';

    CREATE TABLE form_tags (
        form_id INTEGER NOT NULL REFERENCES forms(id),
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
        PRIMARY KEY (form_id, position)
    );

    INSERT INTO form_tags (form_id, position, ns, p2, p3, p4, p5)
    SELECT f.id, (ord.n - 1)::SMALLINT, s.ns, s.p2, s.p3, s.p4, s.p5
    FROM forms f,
         LATERAL unnest(f.tag_tokens) WITH ORDINALITY AS ord(token, n),
         LATERAL split_token_id(ord.token) AS s
    WHERE f.tag_tokens IS NOT NULL
      AND array_length(f.tag_tokens, 1) > 0;
    GET DIAGNOSTICS cnt = ROW_COUNT;
    RAISE NOTICE 'form_tags: % rows inserted', cnt;

    CREATE INDEX idx_form_tags_token ON form_tags(ns, p2, p3, p4, p5);

    ALTER TABLE forms DROP COLUMN tag_tokens;
    RAISE NOTICE 'form_tags: done';
END;
$$;

ANALYZE form_tags;
COMMIT;
