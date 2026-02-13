-- 005b: Decompose token_id in hcp_core metadata table
-- Table is currently empty — this is a schema-only change.
--
-- Run against: hcp_core

BEGIN;

ALTER TABLE metadata
    ADD COLUMN ns TEXT,
    ADD COLUMN p2 TEXT,
    ADD COLUMN p3 TEXT,
    ADD COLUMN p4 TEXT,
    ADD COLUMN p5 TEXT;

ALTER TABLE metadata DROP COLUMN token_id;

ALTER TABLE metadata ADD COLUMN token_id TEXT GENERATED ALWAYS AS (
    ns || COALESCE('.' || p2, '') || COALESCE('.' || p3, '') ||
    COALESCE('.' || p4, '') || COALESCE('.' || p5, '')
) STORED;

CREATE INDEX idx_metadata_token ON metadata(ns, p2, p3, p4, p5);

COMMIT;
