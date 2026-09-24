-- Migration 038: Replace freq_rank-gated vocab queries with full AB token load
--
-- The previous queries used `freq_rank IS NOT NULL` as a filter, which excluded
-- the majority of valid vocab. freq_rank was a temporary priority hint that became
-- a hard gate. Removed.
--
-- New design:
--   - Load all AB (common) tokens per length, plain characteristics first
--   - AD (labels) and URI (hcp_core) queries unchanged
--   - Tense/morpheme envelope queries (token_morph_rules joins) unchanged —
--     they return 0 rows until token_morph_rules is populated (organic growth)
--   - PBM bond data will drive priority ordering in the future (NAPIER)
--
-- Envelope 1 (english_common_10k): delete 15 per-length freq_rank queries (ids 3-17),
-- replace with one query per length loading all AB tokens ordered by characteristics
-- (characteristics=0 plain words first, loanwords/archaic last).

BEGIN;

-- Update envelope description to reflect actual scope
UPDATE envelope_definitions
   SET name        = 'english_vocab_full',
       description = 'Full common English vocabulary (AB namespace). All lengths 1-16. '
                     'Plain words (characteristics=0) before marked (loanwords, archaic, dialect). '
                     'Priority order grows from PBM bond data over time.'
 WHERE id = 1;

-- Drop the 15 per-length freq_rank queries
DELETE FROM envelope_queries WHERE id BETWEEN 3 AND 17;

-- Single replacement: all AB tokens, lengths 1-16, plain-first ordering.
-- One query per length so the engine can page by length if needed,
-- and so each length is a discrete unit in the manifest tracking.
INSERT INTO envelope_queries
    (envelope_id, shard_db, lmdb_subdb, priority, query_text, description)
VALUES
-- length 1 (a, I — single chars that need w2t for SingleChar lookups)
(1, 'hcp_english', 'w2t', 1,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 1
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 1 — single-char words (a, I) for SingleChar LMDB lookup'),

-- length 2
(1, 'hcp_english', 'w2t', 2,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 2
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 2 — all 2-letter words'),

-- length 3
(1, 'hcp_english', 'w2t', 3,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 3
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 3'),

-- length 4
(1, 'hcp_english', 'w2t', 4,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 4
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 4'),

-- length 5
(1, 'hcp_english', 'w2t', 5,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 5
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 5'),

-- length 6
(1, 'hcp_english', 'w2t', 6,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 6
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 6'),

-- length 7
(1, 'hcp_english', 'w2t', 7,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 7
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 7'),

-- length 8
(1, 'hcp_english', 'w2t', 8,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 8
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 8'),

-- length 9
(1, 'hcp_english', 'w2t', 9,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 9
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 9'),

-- length 10
(1, 'hcp_english', 'w2t', 10,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 10
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 10'),

-- length 11
(1, 'hcp_english', 'w2t', 11,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 11
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 11'),

-- length 12
(1, 'hcp_english', 'w2t', 12,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 12
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 12'),

-- length 13
(1, 'hcp_english', 'w2t', 13,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 13
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 13'),

-- length 14
(1, 'hcp_english', 'w2t', 14,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 14
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 14'),

-- length 15
(1, 'hcp_english', 'w2t', 15,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 15
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 15'),

-- length 16
(1, 'hcp_english', 'w2t', 16,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 16
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 16');

COMMIT;
