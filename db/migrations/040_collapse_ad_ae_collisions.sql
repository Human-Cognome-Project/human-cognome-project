-- Migration 040: Collapse AD/AE N_PROPER namespace collisions
--
-- Problem: 392 surface forms exist in BOTH AD (proper noun) AND AE (initialism)
-- namespaces, both tagged N_PROPER in token_pos. The UNIQUE constraint on
-- envelope_working_set (envelope_id, lmdb_subdb, word) rejects the second
-- insert during Label tier COPY, rolling back the entire assembly.
--
-- Fix: Collapse to single AD token_id per surface form.
--   1. Add all-caps variant (e.g. "ADA") to token_variants on the AD token.
--   2. Migrate the 41 meaningful AE glosses to the AD token (sense_number +1).
--   3. DELETE the AE collision tokens — CASCADE drops their token_pos,
--      token_glosses, token_morph_rules, token_variants rows.
--
-- After this migration the N_PROPER envelope query returns only AD tokens
-- for these surface forms, and the warm-layer COPY succeeds.

\connect hcp_english

BEGIN;

-- -----------------------------------------------------------------------
-- Step 1: Add all-caps surface variant on each AD token
-- e.g. "ada" (AD token) gets variant "ADA"
-- -----------------------------------------------------------------------
INSERT INTO token_variants (canonical_id, name, note)
SELECT
    t_ad.token_id,
    upper(t_ad.name),
    'initialism surface form, collapsed from AE namespace (migration 040)'
FROM tokens t_ad
JOIN tokens t_ae ON lower(t_ad.name) = lower(t_ae.name)
WHERE split_part(t_ad.token_id, '.', 1) = 'AD'
  AND split_part(t_ae.token_id, '.', 1) = 'AE'
  AND EXISTS (SELECT 1 FROM token_pos WHERE token_id = t_ad.token_id AND pos = 'N_PROPER')
  AND EXISTS (SELECT 1 FROM token_pos WHERE token_id = t_ae.token_id AND pos = 'N_PROPER')
ON CONFLICT (canonical_id, name, COALESCE(morpheme, '')) DO NOTHING;

-- -----------------------------------------------------------------------
-- Step 2: Migrate meaningful AE glosses to AD token
-- Only for glosses that are NOT the generic 'proper name' / 'proper noun'
-- New sense_number = max existing sense_number for that token + 1
-- -----------------------------------------------------------------------
INSERT INTO token_glosses (token_id, pos, gloss_text, sense_number, status, tags)
SELECT
    t_ad.token_id,
    tg.pos,
    tg.gloss_text,
    COALESCE(
        (SELECT MAX(existing.sense_number) FROM token_glosses existing
         WHERE existing.token_id = t_ad.token_id AND existing.pos = tg.pos),
        0
    ) + 1,
    tg.status,
    tg.tags
FROM tokens t_ae
JOIN tokens t_ad ON lower(t_ad.name) = lower(t_ae.name)
  AND split_part(t_ad.token_id, '.', 1) = 'AD'
JOIN token_glosses tg ON tg.token_id = t_ae.token_id
WHERE split_part(t_ae.token_id, '.', 1) = 'AE'
  AND EXISTS (SELECT 1 FROM token_pos WHERE token_id = t_ad.token_id AND pos = 'N_PROPER')
  AND EXISTS (SELECT 1 FROM token_pos WHERE token_id = t_ae.token_id AND pos = 'N_PROPER')
  AND tg.gloss_text NOT IN ('proper name', 'proper noun')
ON CONFLICT (token_id, pos, sense_number) DO NOTHING;

-- -----------------------------------------------------------------------
-- Step 3: Delete the AE collision tokens
-- ON DELETE CASCADE removes: token_pos, token_glosses, token_morph_rules,
-- token_variants rows for these AE token_ids.
-- -----------------------------------------------------------------------
DELETE FROM tokens
WHERE token_id IN (
    SELECT t_ae.token_id
    FROM tokens t_ad
    JOIN tokens t_ae ON lower(t_ad.name) = lower(t_ae.name)
    WHERE split_part(t_ad.token_id, '.', 1) = 'AD'
      AND split_part(t_ae.token_id, '.', 1) = 'AE'
      AND EXISTS (SELECT 1 FROM token_pos WHERE token_id = t_ad.token_id AND pos = 'N_PROPER')
      AND EXISTS (SELECT 1 FROM token_pos WHERE token_id = t_ae.token_id AND pos = 'N_PROPER')
);

COMMIT;
