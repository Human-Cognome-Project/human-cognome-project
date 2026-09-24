-- Migration 026: Populate proper_common for Label tier (tier 0 broadphase)
--
-- Labels (proper nouns — always capitalized, no lowercase form) are tier 0 in
-- the vocab broadphase. The engine only checks capitalized CharRuns against tier 0,
-- skipping it entirely for lowercase runs. Without tier 0 data, envelope activation
-- has no Label tier and the broadphase optimization does nothing.
--
-- This migration tags the safe group: layer='C' (Wiktionary "Capitalised" category)
-- combined with subcategory='label' (Wiktionary proper noun/place/person marker),
-- WHERE name != lower(name) — excludes numeric/date labels (1812, 1984, etc.) that
-- share the same layer/subcategory but are not proper nouns.
--
-- Scope: 141,756 entries. Examples: London, England, Christmas, Sunday, January,
-- Shakespeare, Vronsky, Cranly, etc.
--
-- Deferred:
--   ~156K remaining uppercase entries (layer='word'/noun, derivative/form, etc.)
--   need per-group review. Patrick to prioritize.
--
--   name lowercasing is migration 027. Names stay uppercase in the DB for now.
--   The envelope query uses lower(name) at SELECT time, so LMDB keys are already
--   lowercase. One known collision to resolve before lowercasing: Gorky/gorky.
--
-- Collision check run: 1 clash (Gorky AB.AB.CA.HX.ZD vs gorky AB.AB.CC.AS.li).
-- Not a problem for this migration — only proper_common is being set here.

\connect hcp_english

UPDATE tokens
SET proper_common = 'proper'
WHERE layer = 'C'
  AND subcategory = 'label'
  AND name != lower(name);

-- Partial index for envelope query performance
-- (full seqscan on 1.4M rows at activation time is acceptable but this is cheap)
CREATE INDEX IF NOT EXISTS idx_tokens_proper_common
    ON tokens (proper_common)
    WHERE proper_common IS NOT NULL;

-- Verify
SELECT proper_common, count(*) FROM tokens GROUP BY proper_common ORDER BY count DESC;

-- ============================================================

\connect hcp_core

-- Add Label tier query to english_common_10k at priority 1.
-- Existing length-bucketed freq-ranked queries are priorities 2–16.
-- fiction_victorian inherits this via its envelope_includes child on english_common_10k.
--
-- No LIMIT: all Labels should be available for the capitalized run broadphase.
-- lower(name) produces lowercase LMDB keys even though DB names are still uppercase.
--
-- Future: extract into a standalone 'labels_english' envelope once the engine's
-- recursive composition (CollectQueries) is implemented. For now, embed directly.

INSERT INTO envelope_queries
    (envelope_id, shard_db, query_text, description, priority, lmdb_subdb)
SELECT
    id,
    'hcp_english',
    'SELECT lower(name) AS name, token_id '
    'FROM tokens '
    'WHERE proper_common = ''proper'' '
    '  AND length(name) BETWEEN 2 AND 16 '
    'ORDER BY freq_rank ASC NULLS LAST',
    'Label tier (tier 0): proper nouns — always capitalized, no lowercase form',
    1,
    'env_vocab'
FROM envelope_definitions
WHERE name = 'english_common_10k';

-- Verify envelope query order
SELECT
    eq.id,
    ed.name AS envelope,
    eq.priority,
    eq.lmdb_subdb,
    eq.description
FROM envelope_queries eq
JOIN envelope_definitions ed ON ed.id = eq.envelope_id
ORDER BY ed.name, eq.priority;
