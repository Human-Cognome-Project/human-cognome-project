-- Migration 017: Pronoun "I" exception — restore uppercase as canonical
--
-- English "I" is intrinsically capitalized (not positional). Migration 016
-- created spurious lowercase duplicates during blanket normalization.
-- Remove them — "I" and its contractions stay uppercase in the vocabulary.
-- The engine transform logic preserves "I" through the lowercase phase
-- boundary when detected in context (space-I-space, space-I-apostrophe).
--
-- Affected: "i", "i's", "i'm'a", "i'ma" — all have uppercase counterparts.
-- NOT affected: "i'faith", "i'jam", "i'tikaf", etc. — legitimate loanwords.

\set ON_ERROR_STOP on

BEGIN;

-- Remove spurious lowercase duplicates that have uppercase originals
DELETE FROM tokens
WHERE name IN ('i', 'i''s', 'i''m''a', 'i''ma')
  AND EXISTS (
    SELECT 1 FROM tokens t2
    WHERE t2.name = CASE
      WHEN tokens.name = 'i'      THEN 'I'
      WHEN tokens.name = 'i''s'   THEN 'I''s'
      WHEN tokens.name = 'i''m''a' THEN 'I''m''a'
      WHEN tokens.name = 'i''ma'  THEN 'I''ma'
    END
  );

-- Verify: "I" uppercase forms remain
DO $$
DECLARE
  cnt integer;
BEGIN
  SELECT count(*) INTO cnt FROM tokens WHERE name IN ('I', 'I''s', 'I''m''a', 'I''ma');
  IF cnt < 2 THEN
    RAISE EXCEPTION 'Expected at least I and I''s to survive, got % rows', cnt;
  END IF;
  RAISE NOTICE 'Pronoun I exception: uppercase forms verified (% entries)', cnt;
END $$;

-- Verify: no lowercase "i" standalone remains
DO $$
DECLARE
  cnt integer;
BEGIN
  SELECT count(*) INTO cnt FROM tokens WHERE name = 'i';
  IF cnt > 0 THEN
    RAISE EXCEPTION 'Lowercase "i" still present after cleanup';
  END IF;
  RAISE NOTICE 'Confirmed: lowercase "i" removed';
END $$;

COMMIT;
