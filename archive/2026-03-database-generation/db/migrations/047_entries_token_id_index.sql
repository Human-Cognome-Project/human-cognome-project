-- Migration 047: Add index on entries.token_id
--
-- Required for the enrichment self-join in envelope assembly:
--   LEFT JOIN entries _e ON _e.token_id = _b.token_id
-- Without this index, each assembly query does a sequential scan of 1.5M rows.
-- With it, hash join builds from the index — fast even for full-table queries.

CREATE INDEX IF NOT EXISTS idx_entries_token_id ON entries(token_id);
