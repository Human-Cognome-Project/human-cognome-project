-- Migration 021: Activity envelope schema
-- Mechanism only — bootstrap examples included. NAPIER owns policy later.
--
-- Three-layer cache hierarchy:
--   LMDB (hot, mmap'd, engine reads)
--   → Postgres activity envelopes (warm, assembled query sets)
--   → Disk/LFS (cold, full shard collection)
--
-- An envelope = a named set of queries whose results form a working set.
-- Executing the envelope's queries produces the data to load into LMDB.
-- Envelopes are composable: child envelopes inherit parent queries.

\connect hcp_core

-- ---- Envelope definitions ----
CREATE TABLE IF NOT EXISTS envelope_definitions (
    id              SERIAL PRIMARY KEY,
    name            TEXT NOT NULL UNIQUE,
    description     TEXT,
    active          BOOLEAN NOT NULL DEFAULT false,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now()
);

COMMENT ON TABLE envelope_definitions IS
    'Named activity envelopes — each defines a working set for LMDB cache loading';

-- ---- Envelope queries ----
-- Each query targets a specific shard DB and produces exactly 2 columns (key, value)
-- for direct LMDB insertion. The query shapes data; the envelope system just pipes it.
CREATE TABLE IF NOT EXISTS envelope_queries (
    id              SERIAL PRIMARY KEY,
    envelope_id     INTEGER NOT NULL REFERENCES envelope_definitions(id) ON DELETE CASCADE,
    shard_db        TEXT NOT NULL,           -- Target Postgres DB name (e.g. 'hcp_english')
    query_text      TEXT NOT NULL,           -- Self-contained SQL returning (key, value)
    description     TEXT,
    priority        INTEGER NOT NULL DEFAULT 0,   -- Execution order within envelope (lower = first)
    lmdb_subdb      TEXT NOT NULL,           -- Target LMDB sub-database name (env_* namespace)
    format          TEXT NOT NULL DEFAULT 'text'  -- Result format: 'text' (UTF-8 key/value pairs), future: 'packed_vbed'
);

CREATE INDEX IF NOT EXISTS idx_envelope_queries_envelope_id
    ON envelope_queries(envelope_id);

COMMENT ON TABLE envelope_queries IS
    'Stored queries returning (key, value) pairs for LMDB cache loading. Strict 2-column contract.';

-- ---- Envelope composition (nesting) ----
-- parent includes child. Query union: overlapping queries share results.
CREATE TABLE IF NOT EXISTS envelope_includes (
    parent_id       INTEGER NOT NULL REFERENCES envelope_definitions(id) ON DELETE CASCADE,
    child_id        INTEGER NOT NULL REFERENCES envelope_definitions(id) ON DELETE CASCADE,
    priority        INTEGER NOT NULL DEFAULT 0,   -- Child load order within parent
    PRIMARY KEY (parent_id, child_id),
    CHECK (parent_id != child_id)
);

COMMENT ON TABLE envelope_includes IS
    'Envelope composition — parent inherits all queries from child envelopes';

-- ---- Activation audit log ----
CREATE TABLE IF NOT EXISTS envelope_activations (
    id              SERIAL PRIMARY KEY,
    envelope_id     INTEGER NOT NULL REFERENCES envelope_definitions(id) ON DELETE CASCADE,
    activated_at    TIMESTAMPTZ NOT NULL DEFAULT now(),
    deactivated_at  TIMESTAMPTZ,
    entries_loaded  INTEGER DEFAULT 0,
    load_time_ms    DOUBLE PRECISION DEFAULT 0,
    evicted_entries INTEGER DEFAULT 0
);

CREATE INDEX IF NOT EXISTS idx_envelope_activations_envelope_id
    ON envelope_activations(envelope_id);

COMMENT ON TABLE envelope_activations IS
    'Audit trail of envelope activations/deactivations and cache load stats';

-- ============================================================
-- Bootstrap examples — prove the pattern, NAPIER refines later
-- ============================================================
--
-- LMDB sub-db ownership:
--   vbed_*   — BedManager (pre-compiled binary vocab, DO NOT target from envelopes)
--   w2t, c2t, l2t, t2w, t2c — CacheMissResolver
--   env_*    — EnvelopeManager (envelope-loaded data, text key/value pairs)
--   _manifest — EnvelopeManager eviction tracking
--
-- Envelope sub-db naming: env_w_XX for word data, env_c_XX for char, etc.
-- Final naming convention pending engine specialist confirmation.

-- Example 1: english_common_10k — top 10K frequency-ranked words
INSERT INTO envelope_definitions (name, description)
VALUES ('english_common_10k',
        'Top 10K frequency-ranked English words. Covers ~80% of running text.')
ON CONFLICT (name) DO NOTHING;

-- Queries for english_common_10k: one per word length bucket (2-16)
-- Each query pulls freq-ordered entries from hcp_english, limited to top entries per length
DO $$
DECLARE
    env_id INTEGER;
    wlen INTEGER;
BEGIN
    SELECT id INTO env_id FROM envelope_definitions WHERE name = 'english_common_10k';
    IF env_id IS NULL THEN RETURN; END IF;

    FOR wlen IN 2..16 LOOP
        INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb)
        VALUES (
            env_id,
            'hcp_english',
            format(
                'SELECT lower(word) AS word, token_id '
                'FROM tokens '
                'WHERE length(word) = %s AND frequency_rank IS NOT NULL '
                'ORDER BY frequency_rank ASC '
                'LIMIT %s',
                wlen,
                CASE WHEN wlen <= 5 THEN 1500 ELSE 500 END
            ),
            format('Top freq-ranked %s-letter words', wlen),
            wlen,  -- priority = word length (shorter words first)
            'env_w_' || lpad(wlen::text, 2, '0')
        )
        ON CONFLICT DO NOTHING;
    END LOOP;
END $$;

-- Example 2: fiction_victorian — composes common + fiction-specific vocab
INSERT INTO envelope_definitions (name, description)
VALUES ('fiction_victorian',
        'Victorian fiction vocabulary. Composes english_common_10k + period-specific terms.')
ON CONFLICT (name) DO NOTHING;

-- Compose: fiction_victorian includes english_common_10k
DO $$
DECLARE
    parent_id INTEGER;
    child_id INTEGER;
BEGIN
    SELECT id INTO parent_id FROM envelope_definitions WHERE name = 'fiction_victorian';
    SELECT id INTO child_id FROM envelope_definitions WHERE name = 'english_common_10k';
    IF parent_id IS NOT NULL AND child_id IS NOT NULL THEN
        INSERT INTO envelope_includes (parent_id, child_id, priority)
        VALUES (parent_id, child_id, 0)
        ON CONFLICT DO NOTHING;
    END IF;
END $$;

-- fiction_victorian adds archaic/literary terms (placeholder query — refine when entity DB has era tags)
DO $$
DECLARE
    env_id INTEGER;
BEGIN
    SELECT id INTO env_id FROM envelope_definitions WHERE name = 'fiction_victorian';
    IF env_id IS NULL THEN RETURN; END IF;

    INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb)
    VALUES (
        env_id,
        'hcp_english',
        'SELECT lower(word) AS word, token_id '
        'FROM tokens '
        'WHERE category IN (''archaic'', ''literary'', ''formal'') '
        'ORDER BY frequency_rank ASC NULLS LAST '
        'LIMIT 5000',
        'Archaic/literary/formal vocabulary for Victorian fiction',
        100,  -- after common vocab
        'env_w_extra'
    )
    ON CONFLICT DO NOTHING;
END $$;
