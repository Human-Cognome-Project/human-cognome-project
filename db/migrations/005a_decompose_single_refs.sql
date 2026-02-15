-- 005a: Decompose single token_id reference columns in hcp_english
--
-- Tables: entries (word_token, pos_token), forms (form_token),
--         relations (relation_token, target_token)
--
-- Pattern per column:
--   1. Add decomposed columns (prefix_ns, prefix_p2, ..., prefix_p5)
--   2. Populate via split_token_id()
--   3. Drop old monolithic column + its index
--   4. Add GENERATED column with same name (reconstructs from parts)
--   5. Add compound B-tree index on decomposed columns
--
-- Each table is its own transaction for safe incremental progress.
--
-- Run against: hcp_english

-- ============================================================================
-- ENTRIES: word_token + pos_token
-- ============================================================================
BEGIN;

DO $$
DECLARE cnt BIGINT;
BEGIN
    RAISE NOTICE '=== entries: word_token decomposition ===';

    -- Add decomposed columns
    ALTER TABLE entries
        ADD COLUMN word_ns TEXT,
        ADD COLUMN word_p2 TEXT,
        ADD COLUMN word_p3 TEXT,
        ADD COLUMN word_p4 TEXT,
        ADD COLUMN word_p5 TEXT;

    -- Populate from existing column (single function call per row)
    UPDATE entries e SET
        word_ns = s.ns, word_p2 = s.p2, word_p3 = s.p3,
        word_p4 = s.p4, word_p5 = s.p5
    FROM (
        SELECT id, (split_token_id(word_token)).*
        FROM entries WHERE word_token IS NOT NULL
    ) s
    WHERE e.id = s.id;
    GET DIAGNOSTICS cnt = ROW_COUNT;
    RAISE NOTICE 'word_token: % rows decomposed', cnt;

    -- Replace monolithic column with generated column
    DROP INDEX IF EXISTS idx_entries_word;
    ALTER TABLE entries DROP COLUMN word_token;
    ALTER TABLE entries ADD COLUMN word_token TEXT GENERATED ALWAYS AS (
        word_ns || COALESCE('.' || word_p2, '') || COALESCE('.' || word_p3, '') ||
        COALESCE('.' || word_p4, '') || COALESCE('.' || word_p5, '')
    ) STORED;
    CREATE INDEX idx_entries_word ON entries(word_ns, word_p2, word_p3, word_p4, word_p5);

    RAISE NOTICE '=== entries: pos_token decomposition ===';

    ALTER TABLE entries
        ADD COLUMN pos_ns TEXT,
        ADD COLUMN pos_p2 TEXT,
        ADD COLUMN pos_p3 TEXT,
        ADD COLUMN pos_p4 TEXT,
        ADD COLUMN pos_p5 TEXT;

    UPDATE entries e SET
        pos_ns = s.ns, pos_p2 = s.p2, pos_p3 = s.p3,
        pos_p4 = s.p4, pos_p5 = s.p5
    FROM (
        SELECT id, (split_token_id(pos_token)).*
        FROM entries WHERE pos_token IS NOT NULL
    ) s
    WHERE e.id = s.id;
    GET DIAGNOSTICS cnt = ROW_COUNT;
    RAISE NOTICE 'pos_token: % rows decomposed', cnt;

    DROP INDEX IF EXISTS idx_entries_pos;
    ALTER TABLE entries DROP COLUMN pos_token;
    ALTER TABLE entries ADD COLUMN pos_token TEXT GENERATED ALWAYS AS (
        pos_ns || COALESCE('.' || pos_p2, '') || COALESCE('.' || pos_p3, '') ||
        COALESCE('.' || pos_p4, '') || COALESCE('.' || pos_p5, '')
    ) STORED;
    CREATE INDEX idx_entries_pos ON entries(pos_ns, pos_p2, pos_p3, pos_p4, pos_p5);

    -- Verify entries
    SELECT COUNT(*) INTO cnt FROM entries WHERE word_token IS NOT NULL;
    RAISE NOTICE 'entries.word_token non-null: %', cnt;
    SELECT COUNT(*) INTO cnt FROM entries WHERE pos_token IS NOT NULL;
    RAISE NOTICE 'entries.pos_token non-null:  %', cnt;
END;
$$;

ANALYZE entries;
COMMIT;

-- ============================================================================
-- FORMS: form_token
-- ============================================================================
BEGIN;

DO $$
DECLARE cnt BIGINT;
BEGIN
    RAISE NOTICE '=== forms: form_token decomposition ===';

    ALTER TABLE forms
        ADD COLUMN form_ns TEXT,
        ADD COLUMN form_p2 TEXT,
        ADD COLUMN form_p3 TEXT,
        ADD COLUMN form_p4 TEXT,
        ADD COLUMN form_p5 TEXT;

    UPDATE forms f SET
        form_ns = s.ns, form_p2 = s.p2, form_p3 = s.p3,
        form_p4 = s.p4, form_p5 = s.p5
    FROM (
        SELECT id, (split_token_id(form_token)).*
        FROM forms WHERE form_token IS NOT NULL
    ) s
    WHERE f.id = s.id;
    GET DIAGNOSTICS cnt = ROW_COUNT;
    RAISE NOTICE 'form_token: % rows decomposed', cnt;

    DROP INDEX IF EXISTS idx_forms_form;
    ALTER TABLE forms DROP COLUMN form_token;
    ALTER TABLE forms ADD COLUMN form_token TEXT GENERATED ALWAYS AS (
        form_ns || COALESCE('.' || form_p2, '') || COALESCE('.' || form_p3, '') ||
        COALESCE('.' || form_p4, '') || COALESCE('.' || form_p5, '')
    ) STORED;
    CREATE INDEX idx_forms_form ON forms(form_ns, form_p2, form_p3, form_p4, form_p5);

    SELECT COUNT(*) INTO cnt FROM forms WHERE form_token IS NOT NULL;
    RAISE NOTICE 'forms.form_token non-null: %', cnt;
END;
$$;

ANALYZE forms;
COMMIT;

-- ============================================================================
-- RELATIONS: relation_token + target_token
-- ============================================================================
BEGIN;

DO $$
DECLARE cnt BIGINT;
BEGIN
    RAISE NOTICE '=== relations: relation_token decomposition ===';

    ALTER TABLE relations
        ADD COLUMN rel_ns TEXT,
        ADD COLUMN rel_p2 TEXT,
        ADD COLUMN rel_p3 TEXT,
        ADD COLUMN rel_p4 TEXT,
        ADD COLUMN rel_p5 TEXT;

    UPDATE relations r SET
        rel_ns = s.ns, rel_p2 = s.p2, rel_p3 = s.p3,
        rel_p4 = s.p4, rel_p5 = s.p5
    FROM (
        SELECT id, (split_token_id(relation_token)).*
        FROM relations WHERE relation_token IS NOT NULL
    ) s
    WHERE r.id = s.id;
    GET DIAGNOSTICS cnt = ROW_COUNT;
    RAISE NOTICE 'relation_token: % rows decomposed', cnt;

    DROP INDEX IF EXISTS idx_relations_type;
    ALTER TABLE relations DROP COLUMN relation_token;
    ALTER TABLE relations ADD COLUMN relation_token TEXT GENERATED ALWAYS AS (
        rel_ns || COALESCE('.' || rel_p2, '') || COALESCE('.' || rel_p3, '') ||
        COALESCE('.' || rel_p4, '') || COALESCE('.' || rel_p5, '')
    ) STORED;
    CREATE INDEX idx_relations_type ON relations(rel_ns, rel_p2, rel_p3, rel_p4, rel_p5);

    RAISE NOTICE '=== relations: target_token decomposition ===';

    ALTER TABLE relations
        ADD COLUMN tgt_ns TEXT,
        ADD COLUMN tgt_p2 TEXT,
        ADD COLUMN tgt_p3 TEXT,
        ADD COLUMN tgt_p4 TEXT,
        ADD COLUMN tgt_p5 TEXT;

    UPDATE relations r SET
        tgt_ns = s.ns, tgt_p2 = s.p2, tgt_p3 = s.p3,
        tgt_p4 = s.p4, tgt_p5 = s.p5
    FROM (
        SELECT id, (split_token_id(target_token)).*
        FROM relations WHERE target_token IS NOT NULL
    ) s
    WHERE r.id = s.id;
    GET DIAGNOSTICS cnt = ROW_COUNT;
    RAISE NOTICE 'target_token: % rows decomposed', cnt;

    DROP INDEX IF EXISTS idx_relations_target;
    ALTER TABLE relations DROP COLUMN target_token;
    ALTER TABLE relations ADD COLUMN target_token TEXT GENERATED ALWAYS AS (
        tgt_ns || COALESCE('.' || tgt_p2, '') || COALESCE('.' || tgt_p3, '') ||
        COALESCE('.' || tgt_p4, '') || COALESCE('.' || tgt_p5, '')
    ) STORED;
    CREATE INDEX idx_relations_target ON relations(tgt_ns, tgt_p2, tgt_p3, tgt_p4, tgt_p5);

    -- Verify
    SELECT COUNT(*) INTO cnt FROM relations WHERE relation_token IS NOT NULL;
    RAISE NOTICE 'relations.relation_token non-null: %', cnt;
    SELECT COUNT(*) INTO cnt FROM relations WHERE target_token IS NOT NULL;
    RAISE NOTICE 'relations.target_token non-null:   %', cnt;
END;
$$;

ANALYZE relations;
COMMIT;
