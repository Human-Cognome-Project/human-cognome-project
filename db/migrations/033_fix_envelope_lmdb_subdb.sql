-- Migration 033: Fix envelope_queries lmdb_subdb — env_vocab → w2t
--
-- Context:
--   BedManager::RebuildVocab() reads vocab from the LMDB 'w2t' sub-database.
--   All 24 envelope_queries rows had lmdb_subdb = 'env_vocab', which is a sub-db
--   the engine never reads.  Activating any envelope wrote word→token entries into
--   a dead sub-db, so RebuildVocab always came up empty.
--
-- Fix:
--   All 24 rows are word→token vocabulary queries; 'w2t' is the correct target.
--   Blanket update is safe — no non-vocabulary queries exist in the table.

\connect hcp_core

UPDATE envelope_queries SET lmdb_subdb = 'w2t';

-- Verify
SELECT id, description, lmdb_subdb
FROM envelope_queries
ORDER BY id;
