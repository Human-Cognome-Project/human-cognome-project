-- Migration 025: Fix envelope_queries — column names and LMDB target sub-db
--
-- Fixes three bugs in the bootstrap queries inserted by migration 021:
--
--   1. Column 'word' does not exist in hcp_english.tokens — actual column is 'name'
--   2. Column 'frequency_rank' does not exist — actual column is 'freq_rank'
--   3. lmdb_subdb values 'env_w_NN' / 'env_w_extra' are wrong — engine reads from
--      'env_vocab' (see sub-db ownership below)
--
-- Sub-db ownership clarified:
--   env_vocab   — EnvelopeManager (envelope-loaded vocab, evicted on switch)
--   w2t         — CacheMissResolver (runtime fills, persists across switches)
--   vbed_*      — BedManager (pre-compiled binary vocab, not touched here)
--
-- Separating env_vocab from w2t ensures cache-miss resolution work is not
-- discarded on every envelope switch.
--
-- Also confirmed: 'formal' category does not exist in hcp_english.tokens
-- (categories: archaic/dialect/casual/literary/NULL). Kept in fiction_victorian
-- query for forward compatibility — zero-cost IN clause with no matching rows.
--
-- Label-tier envelope queries (intrinsically capitalized tokens, no lowercase form)
-- are deferred — they will be a separate envelope query once the Label PoS
-- identifier in hcp_english is confirmed. Not a blocker for this migration.
--
-- Nomenclature note: 'layer = label' in hcp_english was an agent artifact and is
-- NOT the correct Label identifier. 'label' in hcp_core means structural/URI labels
-- (language-independent) — a separate concept. This collision needs cleanup
-- separately.

\connect hcp_core

-- ---- Fix english_common_10k queries (15 per-length queries, IDs 3–17) ----
-- 'word' appears as: lower(word), AS word, length(word) — all correctly become 'name'
-- 'frequency_rank' appears in WHERE and ORDER BY — both correctly become 'freq_rank'

UPDATE envelope_queries
SET
    query_text = replace(
        replace(query_text, 'word', 'name'),
        'frequency_rank', 'freq_rank'
    ),
    lmdb_subdb = 'env_vocab'
WHERE envelope_id = (
    SELECT id FROM envelope_definitions WHERE name = 'english_common_10k'
);

-- ---- Fix fiction_victorian extra vocab query (ID 2) ----

UPDATE envelope_queries
SET
    query_text = replace(
        replace(query_text, 'word', 'name'),
        'frequency_rank', 'freq_rank'
    ),
    lmdb_subdb = 'env_vocab'
WHERE envelope_id = (
    SELECT id FROM envelope_definitions WHERE name = 'fiction_victorian'
);

-- ---- Verify ----
SELECT
    eq.id,
    ed.name AS envelope,
    eq.priority,
    eq.lmdb_subdb,
    left(eq.query_text, 120) AS query_preview
FROM envelope_queries eq
JOIN envelope_definitions ed ON ed.id = eq.envelope_id
ORDER BY ed.name, eq.priority;
