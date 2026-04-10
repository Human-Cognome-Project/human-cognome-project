-- Migration 046: Broadphase cascade envelope queries
--
-- Adds broadphase column to entries table for index-driven load ordering.
-- Rewrites all envelope queries to use entries table with broadphase cascade:
--   Tier 0: Labels (sparse, high category signal, capitalized runs only)
--   Tier 1: Special-char words (apostrophe, hyphen — structural detection)
--   Tier 2: Plain words ascending by length (language detection → domain narrowing)
--   Tier 3: Extended vocabulary (long/rare words, fallback)
--   Tier 4: Multi-word Labels and phrases (post word-level resolution)
--
-- Broadphase flags on entries.broadphase (smallint, bit field):
--   0 = plain word
--   1 = Label (proper noun, AD namespace)
--   2 = contains apostrophe
--   4 = contains hyphen
--   8 = contains digit
--  16 = contains period
--  32 = multi-word phrase
--  64 = morpheme
--
-- All queries use p3 for first_letter + length bucketing.
-- Broadphase controls load PRIORITY, not exclusion.
-- Unresolved tokens fall through to the next tier.

BEGIN;

-- ============================================================
-- Schema: broadphase column + indexes
-- ============================================================

ALTER TABLE entries ADD COLUMN IF NOT EXISTS broadphase smallint NOT NULL DEFAULT 0;

-- Populate (idempotent — OR preserves existing flags)
UPDATE entries SET broadphase = 0;
UPDATE entries SET broadphase = broadphase | 1 WHERE ns = 'AD';
UPDATE entries SET broadphase = broadphase | 2 WHERE word LIKE '%''%';
UPDATE entries SET broadphase = broadphase | 4 WHERE word LIKE '%-%';
UPDATE entries SET broadphase = broadphase | 8 WHERE word ~ '[0-9]';
UPDATE entries SET broadphase = broadphase | 16 WHERE word LIKE '%.%';
UPDATE entries SET broadphase = broadphase | 32 WHERE p2 = 'AB';
UPDATE entries SET broadphase = broadphase | 64 WHERE ns = 'AC';

CREATE INDEX IF NOT EXISTS idx_entries_broadphase ON entries(broadphase);
CREATE INDEX IF NOT EXISTS idx_entries_bp_ns_p3 ON entries(broadphase, ns, p3);

-- ============================================================
-- Clear old envelope definitions and queries
-- ============================================================

DELETE FROM envelope_queries;
DELETE FROM envelope_includes;
DELETE FROM envelope_definitions;

ALTER SEQUENCE envelope_definitions_id_seq RESTART WITH 1;
ALTER SEQUENCE envelope_queries_id_seq RESTART WITH 1;

-- ============================================================
-- Envelope definitions
-- ============================================================

INSERT INTO envelope_definitions (id, name, description, active) VALUES
(1, 'english_resolve',
 'Master English resolution envelope. Broadphase cascade: Labels → special chars → ascending length → extended → phrases.', true),
(2, 'english_labels',
 'Tier 0: Single-word Labels (AD.AA). Sparse in text, checked against capitalized runs only. High category signal for downstream loading.', true),
(3, 'english_special_chars',
 'Tier 1: Words containing apostrophe or hyphen. Structural detection routes to this tier.', true),
(4, 'english_common_asc',
 'Tier 2: Plain common vocabulary by ascending length. Shortest first for language detection and maximum early resolution.', true),
(5, 'english_extended',
 'Tier 3: Extended vocabulary (11+ chars, rare words). Fallback for unresolved tokens.', true),
(6, 'english_phrases',
 'Tier 4: Multi-word phrases (AB.AB) and multi-word Labels (AD.AB). Post word-level resolution.', true);

-- ============================================================
-- Composition: master includes tiers in cascade order
-- ============================================================

INSERT INTO envelope_includes (parent_id, child_id, priority) VALUES
(1, 2, 0),   -- Labels first
(1, 3, 10),  -- Special char words
(1, 4, 20),  -- Common vocab ascending
(1, 5, 30),  -- Extended vocab
(1, 6, 40);  -- Phrases and multi-word Labels

-- ============================================================
-- Tier 0: Labels
-- Ordered by length ascending within Labels.
-- Engine: only load against capitalized stream runs.
-- ============================================================

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (2, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AD'' AND p2 = ''AA''
 ORDER BY length(word), p3, word',
'Single-word Labels by ascending length', 0, 'w2t', 'text');

-- ============================================================
-- Tier 1: Special character words
-- Apostrophe words first (contractions are high-frequency),
-- then hyphenated.
-- Engine: only load when stream token contains the character.
-- ============================================================

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (3, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AB'' AND p2 = ''AA''
   AND (broadphase & 2) = 2
   AND (broadphase & 4) = 0
 ORDER BY length(word), p3, word',
'Apostrophe-only words (contractions, possessives) by ascending length', 0, 'w2t', 'text');

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (3, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AB'' AND p2 = ''AA''
   AND (broadphase & 4) = 4
   AND (broadphase & 2) = 0
 ORDER BY length(word), p3, word',
'Hyphen-only words by ascending length', 1, 'w2t', 'text');

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (3, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AB'' AND p2 = ''AA''
   AND (broadphase & 6) = 6
 ORDER BY length(word), p3, word',
'Words with both apostrophe and hyphen', 2, 'w2t', 'text');

-- ============================================================
-- Tier 2: Plain common vocabulary by ascending length
-- broadphase = 0 means no special characters, not a Label, not a phrase
-- Ordered within each length by p3 (first letter + length bucket)
-- ============================================================

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (5, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AB'' AND p2 = ''AA''
   AND broadphase = 0
   AND length(word) = 1
 ORDER BY word',
'Length 1: single-character words', 0, 'w2t', 'text');

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (5, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AB'' AND p2 = ''AA''
   AND broadphase = 0
   AND length(word) = 2
 ORDER BY p3, word',
'Length 2: plain two-letter words', 1, 'w2t', 'text');

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (5, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AB'' AND p2 = ''AA''
   AND broadphase = 0
   AND length(word) = 3
 ORDER BY p3, word',
'Length 3: plain three-letter words', 2, 'w2t', 'text');

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (5, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AB'' AND p2 = ''AA''
   AND broadphase = 0
   AND length(word) = 4
 ORDER BY p3, word',
'Length 4', 3, 'w2t', 'text');

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (5, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AB'' AND p2 = ''AA''
   AND broadphase = 0
   AND length(word) = 5
 ORDER BY p3, word',
'Length 5', 4, 'w2t', 'text');

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (5, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AB'' AND p2 = ''AA''
   AND broadphase = 0
   AND length(word) = 6
 ORDER BY p3, word',
'Length 6', 5, 'w2t', 'text');

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (5, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AB'' AND p2 = ''AA''
   AND broadphase = 0
   AND length(word) = 7
 ORDER BY p3, word',
'Length 7', 6, 'w2t', 'text');

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (5, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AB'' AND p2 = ''AA''
   AND broadphase = 0
   AND length(word) = 8
 ORDER BY p3, word',
'Length 8', 7, 'w2t', 'text');

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (5, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AB'' AND p2 = ''AA''
   AND broadphase = 0
   AND length(word) = 9
 ORDER BY p3, word',
'Length 9', 8, 'w2t', 'text');

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (5, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AB'' AND p2 = ''AA''
   AND broadphase = 0
   AND length(word) = 10
 ORDER BY p3, word',
'Length 10', 9, 'w2t', 'text');

-- ============================================================
-- Tier 3: Extended vocabulary (11+ chars)
-- ============================================================

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (5, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AB'' AND p2 = ''AA''
   AND broadphase = 0
   AND length(word) BETWEEN 11 AND 16
 ORDER BY length(word), p3, word',
'Length 11-16: extended plain vocabulary', 0, 'w2t', 'text');

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (5, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AB'' AND p2 = ''AA''
   AND broadphase = 0
   AND length(word) > 16
 ORDER BY length(word), p3, word',
'Length 17+: rare long words', 1, 'w2t', 'text');

-- ============================================================
-- Tier 4: Phrases and multi-word Labels
-- Post word-level resolution. Component words already resolved.
-- ============================================================

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (6, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AB'' AND p2 = ''AB''
 ORDER BY array_length(string_to_array(word, '' ''), 1) DESC, word',
'Multi-word phrases, longest first', 0, 'w2t', 'text');

INSERT INTO envelope_queries (envelope_id, shard_db, query_text, description, priority, lmdb_subdb, format)
VALUES (6, 'hcp_english',
'SELECT word, token_id FROM entries
 WHERE ns = ''AD'' AND p2 = ''AB''
 ORDER BY array_length(string_to_array(word, '' ''), 1) DESC, word',
'Multi-word Labels, longest first', 1, 'w2t', 'text');

COMMIT;
