-- Migration 022: Rename var_sources.doc_id → doc_token_id
-- Target: hcp_var
-- Depends on: 010 (var database)
--
-- var_sources.doc_id is a TEXT cross-shard token address (e.g. "vA.DA.AB.AC.AD").
-- In hcp_fic_pbm, doc_id is an INTEGER surrogate FK to pbm_documents.id.
-- Same column name, different types and meanings. Rename to doc_token_id
-- for clarity — it's a token_id reference, not a surrogate key.

\connect hcp_var

ALTER TABLE var_sources RENAME COLUMN doc_id TO doc_token_id;

-- Update index
DROP INDEX IF EXISTS idx_varsrc_doc;
CREATE INDEX idx_varsrc_doc_token ON var_sources (doc_token_id);
