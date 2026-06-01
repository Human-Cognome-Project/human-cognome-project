# Engine Response: LMDB Vocab Compilation

**From**: Engine Specialist
**To**: DB Specialist
**Date**: 2026-02-28
**Re**: brief-lmdb-vocab-compilation.md

## Answers to Open Questions

### 1. GPU particle struct definition

The engine does NOT need pre-built particle position arrays in LMDB. Particle positions are trivially derived from word characters at load time:

```
position[c] = PxVec4(charIndex, 0.0, ascii(word[c]) * Z_SCALE, 0.0)
```

This derivation is ~microseconds per pack. What the engine needs per entry:

| Field | Type | Width | Notes |
|-------|------|-------|-------|
| word | char[] | wordLength bytes | Fixed-width per sub-db (all entries share length). Null-padded if shorter. |
| tokenId | char[] | 14 bytes | Fixed-width. DB confirmed all token_ids are exactly 14 bytes (`AB.XX.YY.ZZ.WW` format). |

That's it. Two fixed-width fields per entry. No bondCount, no freqRank, no tierIndex needed at runtime — ordering in the buffer IS the tier assignment.

**Byte width per entry**: `wordLength + 14`

Examples:
- `vbed_05`: 5 + 14 = 19 bytes/entry
- `vbed_10`: 10 + 14 = 24 bytes/entry

**Endianness**: Not applicable — no integer fields in the entry data. Pure char arrays. Little-endian host (x86_64) if we ever add numeric fields.

### 2. Label tag encoding

**No per-entry flag needed.** Labels are tier 0 — they come first in the ordered sequence. The metadata key provides `label_count`, which tells the engine where tier 0 ends.

The broadphase optimization is host-side, not particle-side:
- Tier 0 (Label check): engine only loads runs tagged with `firstCap` or `allCaps` (from CharRun metadata set at the Phase 1→2 boundary). Non-capitalized runs skip tier 0 entirely.
- Tier 1+ (common vocab): all remaining runs loaded normally.

This means tier 0 is both small (limited Label pool) AND sparse (most runs in common text are lowercase → skip). Double efficiency win.

If future flexibility is needed (e.g., sub-categories within Labels), a 1-byte flags field per entry is cheap to add. But not needed now.

### 3. Tier boundary delivery

**Separate metadata key per word length.**

Key: `vbed_05_meta`
Value: packed struct (little-endian):

```c
struct VBedMeta {
    uint32_t total_entries;     // Total entries in this sub-db
    uint32_t label_count;       // Tier 0 boundary (entries 0..label_count-1)
    uint32_t tier1_end;         // Tier 1 boundary (entries label_count..tier1_end-1)
    uint32_t tier2_end;         // Tier 2 boundary (entries tier1_end..tier2_end-1)
    // Anything beyond tier2_end is tier 3 / var fallback
};
```

16 bytes. The engine reads this once per length at startup, then uses offsets to slice the contiguous entry buffer into tier packs.

### 4. Granularity

**One contiguous buffer per word length.** Not per particle_key bucket.

The workspace model combines all firstChar groups (a-z) into one load. Per-bucket (26 separate reads per length per tier) would add unnecessary I/O overhead and complicate the load path.

The engine slices the buffer by tier boundaries from the metadata key. Within a tier slice, all entries are loaded into one VocabPack regardless of first character. The host-side hash lookup handles matching.

## Runtime Load Path (Post-LMDB)

```
Startup:
  For each word length 2..16:
    1. mdb_get("vbed_05_meta") → VBedMeta (16 bytes, one read)
    2. mdb_get("vbed_05") → pointer into mmap'd entry buffer (zero-copy)
    3. For each tier 0..N:
       a. Slice entry buffer by tier boundaries
       b. BuildVocabPackFromBuffer(entries, tierStart, tierEnd, wordLength)
          - Derive particle positions from word chars
          - Build tierLookup hash map for host-side settlement check
       c. Store in m_vocabPacks[(wordLength << 8) | tier]
    4. Done — no Postgres, no sorting, no tier assignment
```

Estimated init time: <100ms for all lengths (current Postgres path: ~7 seconds).

## What Changes in Engine Code

| Current | After LMDB |
|---------|-----------|
| `TierAssembly::BuildFromDatabase()` — Postgres queries per bucket, sort, assign tiers | Offline compilation step (Python script) |
| `BedManager::BuildVocabPackForTier()` — iterates ChamberVocab entries | `BuildVocabPackFromBuffer()` — reads LMDB mmap'd region, derives particles |
| `BedManager::Initialize()` — calls BuildFromDatabase + BuildVocabPackForTier | Opens LMDB, reads metadata + entry buffers, builds packs |
| ~7 second init | <100ms init |

The resolve path (`ResolveTierBatch`, `ResolveLengthCycle`, workspace loading) stays identical. Only the data source changes.

## Confirmation on Entry Order

Agree with the proposed order:

```
[ Labels (tier 0) | freq_rank ordered (tier 1) | freq_rank + bondCount fallback (tier 2) | remainder ]
```

Labels first for the broadphase negative-filter reason. Within Labels, order by freq_rank (ranked labels first, then alphabetical). Within non-Label tiers, order by freq_rank ascending (most frequent first), unranked by bondCount descending as fallback.

## Morphological Normalization — Strip Regular Forms, Bit-Mark the Inflection

### Concept

Strip predictable inflected forms from the vocab and resolve against base forms only. The inflection type is recorded as a bit field on the resolved token. This reduces vocab entry count significantly (regular English inflections account for a large share of entries) and deepens duplicate stacking at resolve time.

**Rule**: if the surface form is a regular, predictable derivation of the base form, strip it. If the surface form is irregular (no algorithmic relationship to the base), keep it as its own entry.

- `cats` → strip `-s` → resolve `cat` + `PLURAL` bit. Regular.
- `mice` → keep as `mice`. Irregular — surface form bears no predictable relationship to `mouse`.
- `walked` → strip `-ed` → resolve `walk` + `PAST` bit. Regular.
- `went` → keep as `went`. Irregular — not derivable from `go`.
- `running` → strip `-ning` → resolve `run` + `PROG` bit. Regular (double consonant rule).
- `cat's` → strip `-'s` → resolve `cat` + `POSS` bit. Regular.

### Proposed Morph Bit Field (16-bit, on resolved token)

```
Bit 0:  PLURAL        -s, -es, -ies→y
Bit 1:  POSS          -'s (possessive)
Bit 2:  POSS_PLURAL   -s' (plural possessive)
Bit 3:  PAST          -ed (regular past tense)
Bit 4:  PROG          -ing (progressive / gerund)
Bit 5:  3RD           -s (3rd person singular verb)
Bit 6:  NEG           -n't (negation contraction)
Bit 7:  COND          -'d (would/had contraction)
Bit 8:  WILL          -'ll (will contraction)
Bit 9:  HAVE          -'ve (have contraction)
Bit 10: BE            -'re (are contraction)
Bit 11: AM            -'m (am contraction)
Bits 12-15: reserved
```

### Stripping Rules for DB Compilation

These rules are applied during LMDB compilation. The compiler checks whether the stripped base exists in the vocab. If the base exists, the inflected form is dropped. If the base does NOT exist, the inflected form stays as its own entry.

**Priority order** (check longest suffix first to avoid ambiguity):

| Suffix | Strip | Restore | Base must exist | Notes |
|--------|-------|---------|-----------------|-------|
| -n't | remove | — | yes (does, do, have, ...) | Contraction. Base is the verb. |
| -'re | remove | — | yes | |
| -'ve | remove | — | yes | |
| -'ll | remove | — | yes | |
| -'s | remove | — | yes | Possessive. Skip if base not in vocab. |
| -'m | remove | — | yes | |
| -'d | remove | — | yes | |
| -ies | remove, add -y | yes (city, party, ...) | Plural of -y words. |
| -ves | remove, add -f or -fe | yes (knife, wife, ...) | Plural of -f/-fe words. Check both. |
| -ses, -xes, -zes, -ches, -shes | remove -es | yes | Plural after sibilant. |
| -es | remove -es | yes | General -es plural. |
| -s | remove -s | yes | General plural / 3rd person. Ambiguous — see below. |
| -ied | remove, add -y | yes | Past tense of -y verbs (carried → carry). |
| -ed | remove -ed | yes | Regular past. Also check -e + -d (baked → bake). |
| -ning, -ting, -ping, etc. | remove doubled consonant + -ing | yes | Double consonant rule (running → run). |
| -ing | remove -ing | yes | Also check -e + -ing (baking → bake). |

**Ambiguity: -s (plural vs. 3rd person verb)**

Both `cats` (plural of `cat`) and `walks` (3rd person of `walk`) strip the same way. The bit assignment depends on the base word's category. For LMDB compilation, we don't need to disambiguate — just strip and mark as `PLURAL | 3RD` (both bits). The consuming system resolves the ambiguity from context. Alternatively, if the DB has part-of-speech tags, use them.

### What DB Needs to Implement

1. **Suffix stripping function**: given a word, attempt each strip rule in priority order. Return `(base, morph_bits)` or `NULL` if no rule applies.
2. **Base existence check**: query hcp_english for the stripped base. Only strip if base exists.
3. **Dedup pass**: after stripping, multiple inflected forms may map to the same base. Keep only the base entry in LMDB. Record the morph bits that were observed (union of all stripped forms' bits) — this is metadata for validation, not stored in LMDB.
4. **Irregular exception list**: common irregulars that should NOT be stripped even if a rule matches. E.g., `bus` should not strip `-s` to `bu`. The base existence check handles most of these (no `bu` in vocab), but an explicit exception list catches edge cases.

### Impact Estimate

Rough estimate based on English morphology patterns:
- Regular plurals: ~30-40% of noun entries are inflected forms
- Regular verb forms (-ed, -ing, -s): ~20-30% of verb entries
- Possessives/contractions: already handled by decomposition, but now consolidated

Conservative estimate: **25-35% reduction in total vocab entries**. This directly reduces tier pack sizes, improves settlement speed, and deepens duplicate stacking (all inflected forms of "walk" stack onto one resolution of base "walk").

### Engine-Side Changes

The engine's morph bit field lives in `ResolutionResult` (host-side manifest metadata). It does NOT enter the PBD path. At resolve time:

1. Before loading into workspace: attempt suffix strip on the run text
2. If stripped: resolve the base form, set morph bits on the result
3. If strip fails (no rule matches or base not in vocab): resolve the original form

This is the same pattern as the current apostrophe/hyphen decomposition, but generalized and applied earlier in the pipeline.

## RESOLVED: tokenId Max Width

DB confirmed: all token_ids are exactly 14 bytes (`AB.XX.YY.ZZ.WW` decomposed format). Entry width = `wordLength + 14`.

## RESOLVED: Frequency Data

DB merged Wikipedia word frequency 2023 (MIT, 2.7M) + OpenSubtitles full (CC BY-SA-4.0, 1.6M) via geometric mean ranking. Coverage: 176,463 / 1,395,462 ranked (12.6%), up from 2.7%.

## RESOLVED: LMDB v1 Compiled

First compilation at `data/vocab.lmdb/` — 30.2 MB, 1,284,915 entries across 15 sub-databases. Tier-ordered per agreed scheme. Pre-morphological-normalization; v2 after stripping.

## Next: Engine LMDB Reader

Engine-side work to consume the compiled LMDB:

1. **Add LMDB read path to BedManager::Initialize()** — open `data/vocab.lmdb/`, read metadata keys, build VocabPacks from mmap'd entry buffers
2. **New function: `BuildVocabPackFromLMDB()`** — reads entry buffer slice for a tier, derives particle positions from word chars, builds tierLookup hash
3. **Postgres path becomes fallback** — keep `BuildFromDatabase()` for development/debugging, but default to LMDB at runtime
4. **Morph stripping at resolve time** — engine attempts suffix strip before workspace load, sets morph bits on result. Independent of LMDB format (stripping is runtime, not compile-time... but compile-time dedup means fewer entries to check against)
