-- Migration 040: Seed hcp_envelope with envelope data recovered from hcp_core
--
-- hcp_core envelope tables were dropped during migration 038 (dblink not installed,
-- data migration failed before drop ran). This migration reconstructs all envelope
-- data from the migration chain (021→025→026→030→032→033→038_clean_vocab).
--
-- envelope_activations (9 runtime rows) are unrecoverable — not stored in migrations.
-- These were audit/telemetry rows only; no functional data lost.
--
-- Schema note: hcp_envelope adds UNIQUE(envelope_id, priority, lmdb_subdb) which
-- hcp_core did not enforce. N_PROPER label query moved to priority=0 (was 1 in hcp_core)
-- to avoid collision with per-length AB query at priority=1.

\connect hcp_envelope

BEGIN;

-- ============================================================
-- 1. Envelope definitions (6 rows)
-- ============================================================

INSERT INTO envelope_definitions (id, name, description, active) VALUES
(1, 'english_vocab_full',
    'Full common English vocabulary (AB namespace). All lengths 1-16. '
    'Plain words (characteristics=0) before marked (loanwords, archaic, dialect). '
    'Priority order grows from PBM bond data over time.',
    true),
(2, 'fiction_victorian',
    'Victorian fiction vocabulary. Composes english_vocab_full + period-specific terms.',
    true),
(3, 'english_function_words',
    'Function words len 2-4, all tenses. Establishes grammatical context for tense detection.',
    true),
(4, 'english_past_tense',
    'Regular past tense forms for top-ranked verbs (len 5+). Activated on past-tense context detection.',
    false),
(5, 'english_progressive',
    'Regular progressive forms for top-ranked verbs (len 5+). Activated on progressive context detection.',
    false),
(6, 'english_plural',
    'Regular plural forms for top-ranked nouns (len 5+). Activated as needed.',
    false);

SELECT setval('envelope_definitions_id_seq', 6);

-- ============================================================
-- 2. Envelope queries
-- ============================================================

-- ---- Envelope 1: english_vocab_full ----
-- NOTE: queries 27-42 (bulk AB load) are known bad per engine specialist.
--       Migrated as-is; will be replaced with properly scoped queries.

-- N_PROPER label tier (priority=0, before per-length vocab)
INSERT INTO envelope_queries (envelope_id, shard_db, lmdb_subdb, priority, query_text, description) VALUES
(1, 'hcp_english', 'w2t', 0,
 'SELECT t.name, t.token_id
    FROM tokens t
    JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''N_PROPER''
   WHERE length(t.name) BETWEEN 2 AND 16
   ORDER BY t.freq_rank ASC NULLS LAST',
 'Label tier (N_PROPER): proper nouns for capitalized run broadphase');

-- Per-length AB token queries (lengths 1-16)
INSERT INTO envelope_queries (envelope_id, shard_db, lmdb_subdb, priority, query_text, description) VALUES
(1, 'hcp_english', 'w2t', 1,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 1
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 1 — single-char words (a, I) for SingleChar LMDB lookup'),
(1, 'hcp_english', 'w2t', 2,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 2
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 2'),
(1, 'hcp_english', 'w2t', 3,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 3
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 3'),
(1, 'hcp_english', 'w2t', 4,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 4
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 4'),
(1, 'hcp_english', 'w2t', 5,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 5
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 5'),
(1, 'hcp_english', 'w2t', 6,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 6
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 6'),
(1, 'hcp_english', 'w2t', 7,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 7
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 7'),
(1, 'hcp_english', 'w2t', 8,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 8
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 8'),
(1, 'hcp_english', 'w2t', 9,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 9
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 9'),
(1, 'hcp_english', 'w2t', 10,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 10
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 10'),
(1, 'hcp_english', 'w2t', 11,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 11
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 11'),
(1, 'hcp_english', 'w2t', 12,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 12
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 12'),
(1, 'hcp_english', 'w2t', 13,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 13
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 13'),
(1, 'hcp_english', 'w2t', 14,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 14
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 14'),
(1, 'hcp_english', 'w2t', 15,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 15
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 15'),
(1, 'hcp_english', 'w2t', 16,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE t.ns = ''AB'' AND length(t.name) = 16
   ORDER BY t.characteristics ASC, t.name ASC',
 'AB length 16');

-- ---- Envelope 2: fiction_victorian ----
INSERT INTO envelope_queries (envelope_id, shard_db, lmdb_subdb, priority, query_text, description) VALUES
(2, 'hcp_english', 'w2t', 100,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE EXISTS (
       SELECT 1 FROM token_pos tp
       WHERE tp.token_id = t.token_id
         AND (tp.characteristics & (1 | 32 | 256)) != 0
   )
   ORDER BY t.freq_rank ASC NULLS LAST
   LIMIT 5000',
 'Archaic/literary/formal vocabulary for Victorian fiction (characteristics bitmask: FORMAL=1, LITERARY=32, ARCHAIC=256)');

-- ---- Envelope 3: english_function_words ----
INSERT INTO envelope_queries (envelope_id, shard_db, lmdb_subdb, priority, query_text, description) VALUES
(3, 'hcp_english', 'w2t', 1,
 'SELECT t.name, t.token_id
    FROM tokens t
   WHERE length(t.name) BETWEEN 2 AND 4
     AND t.freq_rank IS NOT NULL
   ORDER BY t.freq_rank ASC',
 'Base forms of function words (len 2-4)'),
(3, 'hcp_english', 'w2t', 2,
 'SELECT left(t.name, length(t.name) - length(tmr.strip_suffix)) || tmr.add_suffix AS name, t.token_id
    FROM tokens t
    JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''V_MAIN''
    JOIN token_morph_rules tmr ON tmr.token_id = t.token_id AND tmr.morpheme = ''PAST''
   WHERE length(t.name) BETWEEN 2 AND 4
     AND t.freq_rank IS NOT NULL
   ORDER BY t.freq_rank ASC',
 'Regular past tense of short verbs (len 2-4)'),
(3, 'hcp_english', 'w2t', 3,
 'SELECT left(t.name, length(t.name) - length(tmr.strip_suffix)) || tmr.add_suffix AS name, t.token_id
    FROM tokens t
    JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''V_MAIN''
    JOIN token_morph_rules tmr ON tmr.token_id = t.token_id AND tmr.morpheme = ''PROGRESSIVE''
   WHERE length(t.name) BETWEEN 2 AND 4
     AND t.freq_rank IS NOT NULL
   ORDER BY t.freq_rank ASC',
 'Regular progressive of short verbs (len 2-4)'),
(3, 'hcp_english', 'w2t', 4,
 'SELECT left(t.name, length(t.name) - length(tmr.strip_suffix)) || tmr.add_suffix AS name, t.token_id
    FROM tokens t
    JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''V_MAIN''
    JOIN token_morph_rules tmr ON tmr.token_id = t.token_id AND tmr.morpheme = ''3RD_SING''
   WHERE length(t.name) BETWEEN 2 AND 4
     AND t.freq_rank IS NOT NULL
   ORDER BY t.freq_rank ASC',
 'Regular 3rd-sing of short verbs (len 2-4)');

-- ---- Envelope 4: english_past_tense ----
INSERT INTO envelope_queries (envelope_id, shard_db, lmdb_subdb, priority, query_text, description) VALUES
(4, 'hcp_english', 'w2t', 1,
 'SELECT left(t.name, length(t.name) - length(tmr.strip_suffix)) || tmr.add_suffix AS name, t.token_id
    FROM tokens t
    JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''V_MAIN''
    JOIN token_morph_rules tmr ON tmr.token_id = t.token_id AND tmr.morpheme = ''PAST''
   WHERE length(t.name) >= 5
     AND t.freq_rank IS NOT NULL
     AND (t.characteristics & 256) = 0
   ORDER BY t.freq_rank ASC LIMIT 5000',
 'Regular past tense for top long verbs (len 5+)');

-- ---- Envelope 5: english_progressive ----
INSERT INTO envelope_queries (envelope_id, shard_db, lmdb_subdb, priority, query_text, description) VALUES
(5, 'hcp_english', 'w2t', 1,
 'SELECT left(t.name, length(t.name) - length(tmr.strip_suffix)) || tmr.add_suffix AS name, t.token_id
    FROM tokens t
    JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''V_MAIN''
    JOIN token_morph_rules tmr ON tmr.token_id = t.token_id AND tmr.morpheme = ''PROGRESSIVE''
   WHERE length(t.name) >= 5
     AND t.freq_rank IS NOT NULL
     AND (t.characteristics & 256) = 0
   ORDER BY t.freq_rank ASC LIMIT 5000',
 'Regular progressive for top long verbs (len 5+)');

-- ---- Envelope 6: english_plural ----
INSERT INTO envelope_queries (envelope_id, shard_db, lmdb_subdb, priority, query_text, description) VALUES
(6, 'hcp_english', 'w2t', 1,
 'SELECT left(t.name, length(t.name) - length(tmr.strip_suffix)) || tmr.add_suffix AS name, t.token_id
    FROM tokens t
    JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = ''N_COMMON''
    JOIN token_morph_rules tmr ON tmr.token_id = t.token_id AND tmr.morpheme = ''PLURAL''
   WHERE length(t.name) >= 5
     AND t.freq_rank IS NOT NULL
     AND (t.characteristics & 256) = 0
   ORDER BY t.freq_rank ASC LIMIT 5000',
 'Regular plural for top long nouns (len 5+)');

SELECT setval('envelope_queries_id_seq', (SELECT MAX(id) FROM envelope_queries));

-- ============================================================
-- 3. Envelope includes (fiction_victorian includes english_vocab_full)
-- ============================================================

INSERT INTO envelope_includes (parent_id, child_id, priority) VALUES (2, 1, 0);

-- ============================================================
-- 4. Verify
-- ============================================================

SELECT ed.id, ed.name, ed.active,
       count(eq.id) AS query_count
FROM envelope_definitions ed
LEFT JOIN envelope_queries eq ON eq.envelope_id = ed.id
GROUP BY ed.id, ed.name, ed.active
ORDER BY ed.id;

SELECT parent_id, child_id FROM envelope_includes;

COMMIT;
