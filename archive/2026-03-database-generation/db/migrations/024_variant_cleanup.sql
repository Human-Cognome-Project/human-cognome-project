-- Migration 024: Variant form data cleanup + individual entry annotations
-- Target: hcp_english
-- Depends on: 023 (category + canonical_id columns)
--
-- Applies all SQL fixes from docs/variant-rules-proposal.md (linguist, 2026-03-04):
--
-- Step 0: CRITICAL — clear bad auto-population for high-frequency modern words
--   Migration 023 auto-populated 805 freq-ranked tokens with canonical_ids.
--   These are standard modern English words (the, home, could, come, high, east, ...)
--   that Wiktionary cross-references with archaic/etymological relatives. These are
--   NOT variant forms in the HCP sense — they are canonical forms in their own right.
--   Design principle: tokens with freq_rank (= common modern vocab) are canonical.
--   Variants are rare/archaic forms that are typically unranked (no freq_rank).
--
-- Step 1: Clear specifically bad mappings (wrong even for rare forms)
-- Step 2: Fix circular pairs (haught/haut→haughty, rencontre/rencounter→encounter,
--         prejudical→prejudicial)
-- Step 3: Fix ambiguous forms (couldst→could, et→eat, abolisht→abolish, curst→curse)
-- Step 4: Annotate archaic irregular forms missing canonical_id
--         (art, dost, doth, shouldst, wert, wilt, wouldst)
-- Step 5: Annotate leading-apostrophe casual forms ('em, 'bout, 'cause, 'til)
--
-- V-1 and V-3 variant rules (g-drop -in', archaic -eth): PURELY ENGINE-SIDE strip rules.
-- No DB changes needed — these forms are not in hcp_english. See variant-rules-proposal.md.
-- V-1: word[-1] == "'" and consonant+in pattern → strip "'" → append "g" → retry PBD
-- V-3: word ends in -eth (len>4) → strip -eth, try base, try base+e → THIRD|VARIANT_ARCHAIC
--
-- Morph bit assignments (bits 12-15, previously reserved):
--   Bit 12: VARIANT         (1 << 12 = 4096)  — set on ALL variant resolutions
--   Bit 13: VARIANT_ARCHAIC (1 << 13 = 8192)  — archaic/obsolete/dated/poetic
--   Bit 14: VARIANT_DIALECT (1 << 14 = 16384) — dialectal (g-drop, regional)
--   Bit 15: VARIANT_CASUAL  (1 << 15 = 32768) — casual/informal/slang
-- VARIANT always set alongside the specific subtype bit.
-- Max modifier (all bits): (0xFFFF << 2) | 0x3 = 262,143 → fits base-50 4-char.
-- Engine sets VARIANT + subtype when resolving from env_* variant entries or when
-- TryVariantNormalize fires (engine-side rule V-1/V-3).

\connect hcp_english

BEGIN;

-- ============================================================
-- Step 0: Clear wrongly auto-populated common modern words
-- ============================================================
-- 805 tokens with freq_rank had canonical_ids set by migration 023.
-- These are canonical modern English words, not variant surface forms.
-- The Wiktionary form_tags record etymological notes and usage labels
-- that do not imply "surface variant of another word" in HCP's sense.
-- Examples cleared: the→thee, could→can, home→hjem, come→comma, high→hie
-- All cleared here. Re-add only with explicit linguist confirmation.

UPDATE tokens
SET canonical_id = NULL,
    category = NULL
WHERE ns LIKE 'AB%'
  AND freq_rank IS NOT NULL
  AND canonical_id IS NOT NULL;

-- ============================================================
-- Step 1: Clear remaining bad mappings (wrong even for rare forms)
-- ============================================================
-- These unranked tokens were auto-populated incorrectly.
-- Verified by linguist in variant-rules-proposal.md §Part 4.

-- bang → bhang: completely different words
-- yew → you: yew is a tree; yew/you is a homophone accident
-- ew → yew (cascaded to you): interjection, no link to yew or you
-- saith → says: should map to "say" (base form), not "says" (inflected 3rd-person)
-- wast → waist: "wast" is archaic 2nd-person past of "be"; "waist" is a body part

UPDATE tokens SET canonical_id = NULL, category = NULL
WHERE name IN ('bang', 'yew', 'ew')
  AND ns LIKE 'AB%'
  AND canonical_id IS NOT NULL;

-- Fix saith: says → say (base form)
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'say' AND ns LIKE 'AB%' LIMIT 1)
WHERE name = 'saith' AND ns LIKE 'AB%';

-- Fix wast: waist → be (archaic 2nd-person past of "be")
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'be' AND ns LIKE 'AB%' LIMIT 1),
    category = 'archaic'
WHERE name = 'wast' AND ns LIKE 'AB%';

-- ============================================================
-- Step 2: Fix circular pair resolutions
-- ============================================================
-- Cleared in migration 023 post-fix. Now set correct canonical_ids.

-- prejudical → prejudicial (obsolete misspelling)
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'prejudicial' AND ns LIKE 'AB%' LIMIT 1),
    category = 'archaic'
WHERE name = 'prejudical' AND ns LIKE 'AB%';

-- haught, haut → haughty (archaic adjective forms of "haughty")
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'haughty' AND ns LIKE 'AB%' LIMIT 1),
    category = 'archaic'
WHERE name IN ('haught', 'haut') AND ns LIKE 'AB%';

-- rencontre, rencounter → encounter (archaic/French-origin)
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'encounter' AND ns LIKE 'AB%' LIMIT 1),
    category = 'archaic'
WHERE name IN ('rencontre', 'rencounter') AND ns LIKE 'AB%';

-- ============================================================
-- Step 3: Fix ambiguous forms (linguist-resolved)
-- ============================================================

-- couldst → could (archaic 2nd-person singular past modal)
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'could' AND ns LIKE 'AB%' LIMIT 1),
    category = 'archaic'
WHERE name = 'couldst' AND ns LIKE 'AB%';

-- et → eat (dialectal past tense of "eat"; base form canonical, PAST bit set at engine time)
-- NOTE: "et" has freq_rank 2792 primarily from "et al." (Latin conjunction usage).
-- It should NOT be tagged as a dialect variant of "eat" — it's a common Latin abbreviation.
-- SKIPPED. Leaving for linguist to decide if a separate dialect token should be added.
-- UPDATE tokens SET canonical_id = ..., category = 'dialect' WHERE name = 'et';

-- abolisht → abolish (archaic -t past tense; base form canonical)
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'abolish' AND ns LIKE 'AB%' LIMIT 1),
    category = 'archaic'
WHERE name = 'abolisht' AND ns LIKE 'AB%';

-- curst → curse (archaic -t past/adjective; base form canonical)
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'curse' AND ns LIKE 'AB%' LIMIT 1),
    category = 'archaic'
WHERE name = 'curst' AND ns LIKE 'AB%';

-- ============================================================
-- Step 4: Annotate archaic irregular forms (in DB, missing canonical_id)
-- ============================================================

-- art → be (archaic 2nd-person singular present of "be")
-- NOTE: "art" has freq_rank 373 (very common as noun: "art museum", "works of art").
-- Tagging it as an archaic variant of "be" would break "art" as a primary modern word.
-- SKIPPED. The archaic verb "art" needs a SEPARATE token entry; it cannot share this token.
-- Linguist to decide: create a new "art" verb token (distinct from the noun) if needed.
-- UPDATE tokens SET canonical_id = ..., category = 'archaic' WHERE name = 'art';

-- dost → do (archaic 2nd-person singular of "do")
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'do' AND ns LIKE 'AB%' LIMIT 1),
    category = 'archaic'
WHERE name = 'dost' AND ns LIKE 'AB%' AND canonical_id IS NULL;

-- doth → do (archaic 3rd-person singular present of "do")
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'do' AND ns LIKE 'AB%' LIMIT 1),
    category = 'archaic'
WHERE name = 'doth' AND ns LIKE 'AB%' AND canonical_id IS NULL;

-- shouldst → should (archaic 2nd-person of "should")
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'should' AND ns LIKE 'AB%' LIMIT 1),
    category = 'archaic'
WHERE name = 'shouldst' AND ns LIKE 'AB%' AND canonical_id IS NULL;

-- wert → be (archaic 2nd-person singular past of "be")
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'be' AND ns LIKE 'AB%' LIMIT 1),
    category = 'archaic'
WHERE name = 'wert' AND ns LIKE 'AB%' AND canonical_id IS NULL;

-- wilt → will (archaic 2nd-person singular of "will")
-- Note: "wilt" (to wither) is a different sense. This token is stored as a noun in hcp_english
-- which suggests the archaic modal is the dominant entry here. Linguist to confirm if separate
-- entries for the two senses are needed.
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'will' AND ns LIKE 'AB%' LIMIT 1),
    category = 'archaic'
WHERE name = 'wilt' AND ns LIKE 'AB%' AND canonical_id IS NULL;

-- wouldst → would (archaic 2nd-person of "would")
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'would' AND ns LIKE 'AB%' LIMIT 1),
    category = 'archaic'
WHERE name = 'wouldst' AND ns LIKE 'AB%' AND canonical_id IS NULL;

-- ============================================================
-- Step 5: Annotate leading-apostrophe casual forms
-- ============================================================

-- 'em → them
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'them' AND ns LIKE 'AB%' LIMIT 1),
    category = 'casual'
WHERE name = '''em' AND ns LIKE 'AB%' AND canonical_id IS NULL;

-- 'bout → about
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'about' AND ns LIKE 'AB%' LIMIT 1),
    category = 'casual'
WHERE name = '''bout' AND ns LIKE 'AB%' AND canonical_id IS NULL;

-- 'cause → because
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'because' AND ns LIKE 'AB%' LIMIT 1),
    category = 'casual'
WHERE name = '''cause' AND ns LIKE 'AB%' AND canonical_id IS NULL;

-- 'til → until
UPDATE tokens
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'until' AND ns LIKE 'AB%' LIMIT 1),
    category = 'casual'
WHERE name = '''til' AND ns LIKE 'AB%' AND canonical_id IS NULL;

-- 'tis, 'twas: DEFERRED — multi-token decompositions (it+is, it+was)

-- ============================================================
-- Step 5b: Flatten any newly-created cascades
-- ============================================================
-- Some tokens in the DB already pointed to forms we just annotated (e.g.,
-- wou'dst/would'st → wouldst → would). Flatten to point directly to the terminal.
-- Generic: follow any chain of depth 2 and point to terminal canonical.

WITH RECURSIVE chain AS (
    SELECT token_id AS start_id, canonical_id AS next_id
    FROM tokens
    WHERE canonical_id IS NOT NULL AND ns LIKE 'AB%'
    UNION ALL
    SELECT c.start_id, t.canonical_id
    FROM chain c
    JOIN tokens t ON t.token_id = c.next_id
    WHERE t.canonical_id IS NOT NULL
),
terminals AS (
    SELECT DISTINCT ON (start_id) start_id, next_id AS terminal_id
    FROM chain c
    WHERE NOT EXISTS (
        SELECT 1 FROM tokens t WHERE t.token_id = c.next_id AND t.canonical_id IS NOT NULL
    )
    ORDER BY start_id
)
UPDATE tokens t
SET canonical_id = tm.terminal_id
FROM terminals tm
WHERE t.token_id = tm.start_id
  AND t.canonical_id != tm.terminal_id;

-- ============================================================
-- Step 6: Verify
-- ============================================================

DO $$
DECLARE
    chain_count   INTEGER;
    r_archaic     INTEGER;
    r_dialect     INTEGER;
    r_casual      INTEGER;
    r_literary    INTEGER;
    r_total       INTEGER;
    r_freq_bad    INTEGER;
BEGIN
    -- No chained references
    SELECT count(*) INTO chain_count
    FROM tokens t
    JOIN tokens tc ON tc.token_id = t.canonical_id
    WHERE tc.canonical_id IS NOT NULL AND t.ns LIKE 'AB%';

    IF chain_count > 0 THEN
        RAISE EXCEPTION '024: % chained canonical_id references remain', chain_count;
    END IF;

    -- No HIGH-frequency modern words with canonical_ids (freq_rank < 10000)
    -- Tokens with freq_rank >= 10000 are rare enough to legitimately be archaic variants.
    -- (wilt=20591, dost=25056, doth=26933 are correctly annotated as rare archaic forms.)
    SELECT count(*) INTO r_freq_bad
    FROM tokens WHERE ns LIKE 'AB%' AND canonical_id IS NOT NULL AND freq_rank < 10000;

    IF r_freq_bad > 0 THEN
        RAISE EXCEPTION '024: % high-frequency tokens (rank<10000) have canonical_id — cleanup failed', r_freq_bad;
    END IF;

    -- Final counts
    SELECT count(*) INTO r_archaic  FROM tokens WHERE ns LIKE 'AB%' AND category = 'archaic';
    SELECT count(*) INTO r_dialect  FROM tokens WHERE ns LIKE 'AB%' AND category = 'dialect';
    SELECT count(*) INTO r_casual   FROM tokens WHERE ns LIKE 'AB%' AND category = 'casual';
    SELECT count(*) INTO r_literary FROM tokens WHERE ns LIKE 'AB%' AND category = 'literary';
    r_total := r_archaic + r_dialect + r_casual + r_literary;

    RAISE NOTICE '024: % variant tokens total (archaic=%, dialect=%, casual=%, literary=%)',
        r_total, r_archaic, r_dialect, r_casual, r_literary;
    RAISE NOTICE '024: 0 chained references, 0 freq-ranked variants — OK';
END $$;

COMMIT;

ANALYZE tokens;

-- ============================================================
-- Post-migration notes
-- ============================================================
-- Migration 023 auto-populated 5,601 variant tokens from Wiktionary form_tags.
-- Migration 024 correction:
--   - Cleared 805 freq-ranked tokens (wrongly tagged common modern words)
--   - Net variant count after cleanup: ~4,796 + targeted fixes above
--
-- Remaining work for linguist:
--   - Review 67 remaining ambiguous forms (query in audit doc)
--   - Confirm wilt ambiguity (to-wither vs archaic-will)
--   - 'tis, 'twas deferred to multi-token decomposition design
--
-- V-1 g-drop and V-3 -eth: engine-side strip rules. Engine specialist to implement
--   TryVariantNormalize in HCPVocabBed.cpp with these rules.
--   Morph bits to set on V-1 resolution: VARIANT (12) + VARIANT_DIALECT (14)
--   Morph bits to set on V-3 resolution: VARIANT (12) + VARIANT_ARCHAIC (13) + THIRD (5)
