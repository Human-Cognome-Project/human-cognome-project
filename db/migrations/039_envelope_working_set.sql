-- Migration 039: Create envelope_working_set in hcp_var
--
-- The warm layer of the three-layer architecture:
--   Cold (hcp_english) → Warm (hcp_var) → Hot (LMDB)
--
-- EnvelopeManager assembles this table from cold shard queries during
-- activation. All expensive work (ordering, category tagging, priority
-- calculation) is done here so the engine reads a pre-sorted set from
-- LMDB without touching the cold shard at runtime.
--
-- Feedback loop: engine signals detected patterns (archaic usage, tense,
-- domain) → priority_delta updated live → next LMDB slice write reflects
-- new ordering. No full re-assembly needed.
--
-- envelope_id is a logical reference to hcp_core.envelope_definitions —
-- no FK constraint because this is a cross-database reference.

\connect hcp_var

BEGIN;

CREATE TABLE envelope_working_set (
    id                 BIGSERIAL    PRIMARY KEY,
    envelope_id        INTEGER      NOT NULL,
    shard_db           TEXT         NOT NULL,
    lmdb_subdb         TEXT         NOT NULL,
    word               TEXT         NOT NULL,
    token_id           TEXT         NOT NULL,
    word_length        SMALLINT     NOT NULL,
    ns                 TEXT         NOT NULL,
    characteristics    INTEGER      NOT NULL DEFAULT 0,
    category           TEXT,
    base_priority      INTEGER      NOT NULL DEFAULT 0,
    priority_delta     INTEGER      NOT NULL DEFAULT 0,
    effective_priority INTEGER GENERATED ALWAYS AS (base_priority + priority_delta) STORED,
    assembled_at       TIMESTAMPTZ  NOT NULL DEFAULT now(),

    UNIQUE (envelope_id, lmdb_subdb, word)
);

-- Sliced reads: all rows for a given envelope+subdb+length, priority-ordered
CREATE INDEX idx_ews_slice
    ON envelope_working_set (envelope_id, lmdb_subdb, word_length, effective_priority);

-- Feedback updates: boost/demote an entire category in one UPDATE
CREATE INDEX idx_ews_feedback
    ON envelope_working_set (envelope_id, category);

-- t2w reverse lookup by token
CREATE INDEX idx_ews_token
    ON envelope_working_set (token_id);

COMMIT;
