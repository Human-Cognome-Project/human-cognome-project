-- Migration 045: Fix 'o'-starting words in wrong P3 bucket
--
-- The base-50 alphabet excludes O/o (ambiguity with zero). The token_id
-- generator's LETTER_MAP also excluded 'o', causing all 15,644 words
-- starting with 'o' to fall back to index 0 = 'A' (the 'a' bucket).
--
-- Fix: assign 'o' to B50 index 27 = 'c' (after z=24, '=25, -=26).
-- P3 first char changes from 'A' to 'c'; second char (length) unchanged.
-- token_id is a generated column — it auto-recalculates when p3 changes.
--
-- FK constraints are ON DELETE CASCADE only (no ON UPDATE CASCADE),
-- so we drop them, remap all child references, update p3, then re-add.

BEGIN;

-- 1. Build remap table: old token_id → new token_id
CREATE TEMP TABLE _o_remap AS
SELECT
    token_id AS old_id,
    ns || '.' || p2 || '.c' || substr(p3, 2, 1) || '.' || p4 || '.' || p5 AS new_id
FROM tokens
WHERE name ~ '^o' AND left(p3, 1) = 'A';

-- Sanity check: no collisions in target space (P3 'c*' should be empty)
DO $$
BEGIN
    IF EXISTS (SELECT 1 FROM tokens WHERE left(p3, 1) = 'c') THEN
        RAISE EXCEPTION 'P3 c* bucket is not empty — aborting';
    END IF;
END $$;

-- 2. Drop FK constraints
ALTER TABLE token_pos DROP CONSTRAINT token_pos_token_id_fkey;
ALTER TABLE token_glosses DROP CONSTRAINT token_glosses_token_id_fkey;
ALTER TABLE token_morph_rules DROP CONSTRAINT token_morph_rules_token_id_fkey;
ALTER TABLE token_variants DROP CONSTRAINT token_variants_canonical_id_fkey;

-- 3. Update child tables (point to new token_ids before parent changes)
UPDATE token_pos tp
SET token_id = r.new_id
FROM _o_remap r
WHERE tp.token_id = r.old_id;

UPDATE token_glosses tg
SET token_id = r.new_id
FROM _o_remap r
WHERE tg.token_id = r.old_id;

UPDATE token_morph_rules tm
SET token_id = r.new_id
FROM _o_remap r
WHERE tm.token_id = r.old_id;

-- token_variants.canonical_id: variant forms whose canonical token starts with 'o'
UPDATE token_variants tv
SET canonical_id = r.new_id
FROM _o_remap r
WHERE tv.canonical_id = r.old_id;

-- 4. Update tokens.p3 (regenerates token_id via generated column)
UPDATE tokens
SET p3 = 'c' || substr(p3, 2, 1)
WHERE name ~ '^o' AND left(p3, 1) = 'A';

-- 5. Verify: all new token_ids match what child tables expect
DO $$
DECLARE
    orphan_pos INTEGER;
    orphan_gloss INTEGER;
    orphan_variant INTEGER;
BEGIN
    SELECT count(*) INTO orphan_pos
    FROM token_pos tp
    WHERE NOT EXISTS (SELECT 1 FROM tokens t WHERE t.token_id = tp.token_id);

    SELECT count(*) INTO orphan_gloss
    FROM token_glosses tg
    WHERE NOT EXISTS (SELECT 1 FROM tokens t WHERE t.token_id = tg.token_id);

    SELECT count(*) INTO orphan_variant
    FROM token_variants tv
    WHERE NOT EXISTS (SELECT 1 FROM tokens t WHERE t.token_id = tv.canonical_id);

    IF orphan_pos > 0 OR orphan_gloss > 0 OR orphan_variant > 0 THEN
        RAISE EXCEPTION 'Orphaned refs: pos=%, glosses=%, variants=%',
            orphan_pos, orphan_gloss, orphan_variant;
    END IF;
END $$;

-- 6. Re-add FK constraints
ALTER TABLE token_pos
    ADD CONSTRAINT token_pos_token_id_fkey
    FOREIGN KEY (token_id) REFERENCES tokens(token_id) ON DELETE CASCADE;

ALTER TABLE token_glosses
    ADD CONSTRAINT token_glosses_token_id_fkey
    FOREIGN KEY (token_id) REFERENCES tokens(token_id) ON DELETE CASCADE;

ALTER TABLE token_morph_rules
    ADD CONSTRAINT token_morph_rules_token_id_fkey
    FOREIGN KEY (token_id) REFERENCES tokens(token_id) ON DELETE CASCADE;

ALTER TABLE token_variants
    ADD CONSTRAINT token_variants_canonical_id_fkey
    FOREIGN KEY (canonical_id) REFERENCES tokens(token_id) ON DELETE CASCADE;

DROP TABLE _o_remap;

COMMIT;
