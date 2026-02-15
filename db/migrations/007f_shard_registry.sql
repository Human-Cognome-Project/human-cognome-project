-- HCP Migration 007f: Register new shards in shard_registry
-- Target: hcp_core
-- Depends on: 007d (PBM database), 007e (entity databases)
--
-- Registers:
--   vA → (future fiction PBM shard, not yet created)
--   uA, tA, sA → hcp_fic_entities
--   yA, xA, wA → hcp_nf_entities
--   zA already exists — update description only

BEGIN;

-- Update existing zA description for clarity
UPDATE shard_registry
SET description = 'Non-fiction PBMs and documents (English-primary)'
WHERE ns_prefix = 'zA';

-- Fiction entity shards
INSERT INTO shard_registry (ns_prefix, shard_db, description) VALUES
('uA', 'hcp_fic_entities', 'Fiction person entities (English-primary)'),
('tA', 'hcp_fic_entities', 'Fiction place entities (English-primary)'),
('sA', 'hcp_fic_entities', 'Fiction thing entities (English-primary)');

-- Non-fiction entity shards
INSERT INTO shard_registry (ns_prefix, shard_db, description) VALUES
('yA', 'hcp_nf_entities', 'Non-fiction person entities (English-primary)'),
('xA', 'hcp_nf_entities', 'Non-fiction place entities (English-primary)'),
('wA', 'hcp_nf_entities', 'Non-fiction thing entities (English-primary)');

-- Fiction PBM shard (registered but database not yet created — will be needed
-- when fiction PBM ingestion begins)
INSERT INTO shard_registry (ns_prefix, shard_db, description, active) VALUES
('vA', 'hcp_fic_pbm', 'Fiction PBMs (English-primary)', false);

-- ============================================================================
-- Verify
-- ============================================================================

SELECT ns_prefix, shard_db, active, description
FROM shard_registry
ORDER BY ns_prefix;

COMMIT;
