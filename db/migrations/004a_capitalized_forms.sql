-- 004a: Add capitalized form variants for all existing English entries
-- Decision 002: Every word gets its capitalized form as an alternate spelling.
-- Rule: lowercase primary → capitalized alternate (tagged "capitalization")
--
-- Run against: hcp_english

DO $$
DECLARE
    inserted_count BIGINT;
    pre_count BIGINT;
BEGIN
    SELECT COUNT(*) INTO pre_count FROM forms;
    RAISE NOTICE 'Forms before: %', pre_count;

    INSERT INTO forms (entry_id, form_text, tag_tokens)
    SELECT
        e.id,
        UPPER(LEFT(e.word, 1)) || SUBSTRING(e.word FROM 2),
        ARRAY['AB.AB.CA.At.DE']::TEXT[]  -- "capitalization" tag token
    FROM entries e
    WHERE LEFT(e.word, 1) ~ '[a-z]'                                    -- starts lowercase
      AND UPPER(LEFT(e.word, 1)) || SUBSTRING(e.word FROM 2) <> e.word  -- different after capitalize
      AND NOT EXISTS (
          SELECT 1 FROM forms f
          WHERE f.entry_id = e.id
            AND f.form_text = UPPER(LEFT(e.word, 1)) || SUBSTRING(e.word FROM 2)
      );

    GET DIAGNOSTICS inserted_count = ROW_COUNT;
    RAISE NOTICE 'Step 1 complete: % capitalized forms inserted', inserted_count;
    RAISE NOTICE 'Forms after: %', pre_count + inserted_count;
END;
$$;
