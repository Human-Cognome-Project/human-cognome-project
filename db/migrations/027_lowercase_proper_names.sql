-- Migration 027: Lowercase proper noun names (safe 141K group)
--
-- Lowercases the `name` field for the 141,755 tokens tagged proper_common='proper'
-- in migration 026 (layer='C', subcategory='label') that have no lowercase collision.
--
-- Patrick's rule: ALL tokens stored lowercase in `name`. Capitalisation metadata
-- lives in proper_common / flags, not in the name itself.
--
-- Scope: 141,755 entries (excludes 1 known collision: Gorky AB.AB.CA.HX.ZD
-- vs gorky AB.AB.CC.AS.li — deferred to migration 028 dedup pass).
--
-- The `particle_key` generated column recalculates automatically on UPDATE
-- (GENERATED ALWAYS AS STORED recomputes when name changes).
-- The `token_id` generated column is based on ns/p2/p3/p4/p5, not name — stable.
--
-- Investigation result on the 13,364 broader collision set:
-- These are NOT from a post-migration-016 import. The majority (9,489) are within
-- the CA/CA sub-namespace — the original Wiktionary import. Migration 016 missed
-- them. Deduplication strategy for these 13,364 cases + 45,444 exact name
-- duplicates is deferred pending Patrick's input (migration 028).

\connect hcp_english

UPDATE tokens
SET name = lower(name)
WHERE proper_common = 'proper'
  AND name != lower(name)
  AND NOT EXISTS (
    SELECT 1 FROM tokens t2
    WHERE t2.name = lower(tokens.name)
      AND t2.token_id != tokens.token_id
  );

-- Verify
SELECT
  count(*) FILTER (WHERE name != lower(name))         AS still_uppercase,
  count(*) FILTER (WHERE proper_common = 'proper')    AS proper_tagged,
  count(*) FILTER (WHERE proper_common = 'proper'
                     AND name != lower(name))         AS proper_still_uppercase
FROM tokens;
