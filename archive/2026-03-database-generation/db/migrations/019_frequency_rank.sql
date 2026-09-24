-- Migration 019: Add frequency rank to tokens for PBD tier ordering
--
-- freq_rank determines tier assignment in the engine's PBD vocab beds.
-- Lower rank = higher frequency = tier 0 (checked first).
-- Source: OpenSubtitles 50K frequency list (hermitdave/FrequencyWords).
-- Bootstrap data — will be replaced by PBM aggregate counts when corpus is large enough.
--
-- Words not in the frequency list get NULL (sorted last = lowest tier).

BEGIN;

-- Add the column
ALTER TABLE tokens ADD COLUMN IF NOT EXISTS freq_rank INTEGER;

-- Create temp table for bulk load
CREATE TEMP TABLE freq_import (
    word TEXT NOT NULL,
    freq_count BIGINT NOT NULL,
    rank INTEGER NOT NULL
);

-- Load from file (must be accessible to postgres)
-- Run from shell: psql -c "\copy freq_import(word, freq_count, rank) FROM PROGRAM 'awk ''{print $1\"\t\"$2\"\t\"NR}'' /tmp/en_freq_50k.txt'"
-- Or use the INSERT approach below after loading the temp table externally.

-- Update tokens with frequency rank where name matches
UPDATE tokens t
SET freq_rank = f.rank
FROM freq_import f
WHERE t.name = f.word
  AND t.ns LIKE 'AB%';

-- Index for engine queries (ORDER BY freq_rank)
CREATE INDEX IF NOT EXISTS idx_tokens_freq_rank ON tokens (freq_rank)
    WHERE freq_rank IS NOT NULL;

-- Stats
DO $$
DECLARE
    matched INTEGER;
    total_ab INTEGER;
    top100_matched INTEGER;
BEGIN
    SELECT count(*) INTO total_ab FROM tokens WHERE ns LIKE 'AB%';
    SELECT count(*) INTO matched FROM tokens WHERE ns LIKE 'AB%' AND freq_rank IS NOT NULL;
    SELECT count(*) INTO top100_matched FROM tokens WHERE ns LIKE 'AB%' AND freq_rank <= 100;

    RAISE NOTICE '019: % / % AB tokens have freq_rank (%.1f%%)',
        matched, total_ab, 100.0 * matched / NULLIF(total_ab, 0);
    RAISE NOTICE '019: % tokens in top-100 frequency', top100_matched;
END $$;

DROP TABLE freq_import;

COMMIT;

ANALYZE tokens;
