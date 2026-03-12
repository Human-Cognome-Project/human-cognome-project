-- Migration 038: Create hcp_envelope database, migrate envelope data from hcp_core
--
-- Moves the envelope system out of hcp_core into its own dedicated database.
-- hcp_core envelope tables are dropped after data is confirmed migrated.
--
-- Note: envelope_queries carries an existing 'format' column (TEXT NOT NULL DEFAULT 'text')
-- not in the new spec — retained rather than silently dropped. Engine specialist
-- can remove it in a follow-up migration once confirmed unused.
--
-- Note: envelope 1 (english_vocab_full) queries 27-42 are known bad (bulk-load all AB
-- tokens). Migrated as-is; engine specialist will replace with scoped queries.

-- ============================================================
-- 1. Create database
-- ============================================================

\connect postgres
CREATE DATABASE hcp_envelope OWNER hcp;

\connect hcp_envelope

-- ============================================================
-- 2. Schema
-- ============================================================

CREATE TABLE envelope_definitions (
    id          SERIAL PRIMARY KEY,
    name        TEXT NOT NULL UNIQUE,
    description TEXT,
    active      BOOLEAN NOT NULL DEFAULT false,
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE envelope_queries (
    id          SERIAL PRIMARY KEY,
    envelope_id INTEGER NOT NULL REFERENCES envelope_definitions(id) ON DELETE CASCADE,
    priority    INTEGER NOT NULL DEFAULT 0,
    shard_db    TEXT NOT NULL,
    lmdb_subdb  TEXT NOT NULL,
    query_text  TEXT NOT NULL,
    description TEXT,
    format      TEXT NOT NULL DEFAULT 'text',   -- retained from hcp_core; drop when confirmed unused
    UNIQUE (envelope_id, priority, lmdb_subdb)
);

CREATE INDEX idx_envelope_queries_envelope_id ON envelope_queries (envelope_id);

CREATE TABLE envelope_includes (
    parent_id   INTEGER NOT NULL REFERENCES envelope_definitions(id) ON DELETE CASCADE,
    child_id    INTEGER NOT NULL REFERENCES envelope_definitions(id) ON DELETE CASCADE,
    priority    INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY (parent_id, child_id),
    CHECK (parent_id <> child_id)
);

CREATE TABLE envelope_activations (
    id              BIGSERIAL PRIMARY KEY,
    envelope_id     INTEGER NOT NULL REFERENCES envelope_definitions(id) ON DELETE CASCADE,
    entries_loaded  INTEGER DEFAULT 0,
    load_time_ms    DOUBLE PRECISION DEFAULT 0,
    evicted_entries INTEGER DEFAULT 0,
    activated_at    TIMESTAMPTZ NOT NULL DEFAULT now(),
    deactivated_at  TIMESTAMPTZ
);

CREATE INDEX idx_envelope_activations_envelope_id ON envelope_activations (envelope_id);

-- ============================================================
-- 3. Migrate data from hcp_core
-- ============================================================

INSERT INTO envelope_definitions (id, name, description, active, created_at)
SELECT id, name, description, active, created_at
FROM dblink('dbname=hcp_core', '
    SELECT id, name, description, active, created_at FROM envelope_definitions ORDER BY id
') AS t(id int, name text, description text, active bool, created_at timestamptz);

INSERT INTO envelope_queries (id, envelope_id, priority, shard_db, lmdb_subdb, query_text, description, format)
SELECT id, envelope_id, priority, shard_db, lmdb_subdb, query_text, description, format
FROM dblink('dbname=hcp_core', '
    SELECT id, envelope_id, priority, shard_db, lmdb_subdb, query_text, description, format
    FROM envelope_queries ORDER BY id
') AS t(id int, envelope_id int, priority int, shard_db text, lmdb_subdb text, query_text text, description text, format text);

INSERT INTO envelope_includes (parent_id, child_id, priority)
SELECT parent_id, child_id, priority
FROM dblink('dbname=hcp_core', '
    SELECT parent_id, child_id, priority FROM envelope_includes
') AS t(parent_id int, child_id int, priority int);

INSERT INTO envelope_activations (id, envelope_id, entries_loaded, load_time_ms, evicted_entries, activated_at, deactivated_at)
SELECT id, envelope_id, entries_loaded, load_time_ms, evicted_entries, activated_at, deactivated_at
FROM dblink('dbname=hcp_core', '
    SELECT id, envelope_id, entries_loaded, load_time_ms, evicted_entries, activated_at, deactivated_at
    FROM envelope_activations ORDER BY id
') AS t(id int, envelope_id int, entries_loaded int, load_time_ms float8, evicted_entries int, activated_at timestamptz, deactivated_at timestamptz);

-- Reset sequences to current max
SELECT setval('envelope_definitions_id_seq', (SELECT MAX(id) FROM envelope_definitions));
SELECT setval('envelope_queries_id_seq',     (SELECT MAX(id) FROM envelope_queries));
SELECT setval('envelope_activations_id_seq', (SELECT MAX(id) FROM envelope_activations));

-- ============================================================
-- 4. Verify counts match hcp_core
-- ============================================================

SELECT
    (SELECT count(*) FROM envelope_definitions) AS defs,
    (SELECT count(*) FROM envelope_queries)     AS queries,
    (SELECT count(*) FROM envelope_includes)    AS includes,
    (SELECT count(*) FROM envelope_activations) AS activations;

-- ============================================================
-- 5. Drop envelope tables from hcp_core
-- ============================================================

\connect hcp_core

DROP TABLE IF EXISTS envelope_activations;
DROP TABLE IF EXISTS envelope_queries;
DROP TABLE IF EXISTS envelope_includes;
DROP TABLE IF EXISTS envelope_definitions;

-- Verify gone
SELECT tablename FROM pg_tables
WHERE schemaname = 'public' AND tablename LIKE 'envelope%';
