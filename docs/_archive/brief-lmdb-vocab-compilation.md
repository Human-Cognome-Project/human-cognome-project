# Brief: Pre-Compiled LMDB Vocab Tables for GPU Loading

**From**: DB Specialist
**To**: Engine Specialist
**Date**: 2026-02-28
**Status**: Design — ready for engine-side input on buffer format

## Problem

Current hot path: Postgres query per particle_key bucket → parse rows → sort by freq_rank → assign tiers → build particle arrays → copy to VRAM. This is the data access bottleneck.

## Solution

Pre-compile LMDB tables per word length, ordered for direct GPU consumption. At runtime the path becomes:

1. `mdb_get()` → pointer into mmap'd region (zero-copy)
2. `memcpy` / `cudaMemcpy` → VRAM

No SQL, no network, no sorting, no tier assembly at runtime. The compilation step runs offline whenever vocab changes.

## LMDB Layout — One Sub-DB Per Word Length

Sub-databases: `vbed_02` through `vbed_16` (matching existing bed range).

Each sub-db contains entries ordered sequentially. The engine reads forward from the start — tier boundaries are just entry counts, not separate lookups.

## Entry Order Within Each Length

**Tier 0 = Labels first**, regardless of count. Then frequency-ordered common vocab.

```
[ Labels (subcategory='label') | freq_rank 1..N | unranked by bondCount ]
```

### Why Labels first

- **Small, bounded pool** — 142K total across all lengths, but per-length counts are manageable (e.g., len=5 has 14K labels, len=10 has 11K).
- **Negative filtering** — labels almost never match in common text, so eliminate them early.
- **Engine broadphase optimization** — because the block is tagged as label entries, broadphase can scan only those particles on tier 0, ignoring non-label tagged entries. This is a type filter, not a brute-force check.

### Label counts per word length (current DB)

| Length | Labels | Non-Labels | Freq-Ranked |
|--------|--------|------------|-------------|
| 2 | 136 | 1,745 | 349 |
| 3 | 2,103 | 9,666 | 1,345 |
| 4 | 6,296 | 21,026 | 3,197 |
| 5 | 14,142 | 35,065 | 4,489 |
| 6 | 24,304 | 61,222 | 5,598 |
| 7 | 29,095 | 95,164 | 5,943 |
| 8 | 25,519 | 125,946 | 5,276 |
| 9 | 18,125 | 145,662 | 4,320 |
| 10 | 11,123 | 149,004 | 3,042 |
| 11 | 5,752 | 135,331 | 1,868 |
| 12 | 2,677 | 113,906 | 1,085 |
| 13 | 1,274 | 90,731 | 549 |
| 14 | 651 | 69,324 | 244 |
| 15 | 387 | 51,687 | 90 |
| 16 | 260 | 37,592 | 40 |

## What the Engine Needs to Tell DB

**The value format.** Each LMDB entry needs to be laid out exactly as the GPU particle system expects. DB will pack the binary buffer, but engine owns the struct definition. Specifically:

1. **What fields per vocab entry?** — Current `TieredVocabEntry` has: word (string), tokenId (string), bondCount, freqRank, tierIndex. What does the GPU particle actually need? Just position + phase + invMass? Or token ID for result lookup?

2. **Fixed-width or variable?** — If entries are fixed-width, the entire sub-db value can be a single contiguous buffer (one `mdb_get`, one `memcpy`). Variable-width strings (word, tokenId) would need an index or padding.

3. **Label tag encoding** — How should labels be marked in the binary buffer so broadphase can filter? A flag byte? A separate particle group? The engine's broadphase scan drives this choice.

4. **Tier boundary metadata** — Should the entry count per tier be stored as a header in the LMDB value, or can the engine just read a separate metadata key (e.g., `vbed_05_meta` → `{label_count: 14142, tier1_start: 18631, ...}`)?

## Frequency Data

- **Current source**: OpenSubtitles 50K list (`/tmp/en_freq_50k.txt`), applied via migration 019.
- **Coverage**: 37,487 / 1,395,462 tokens ranked (~2.7%). Top 50K covers the high-frequency core.
- **Future**: PBM aggregate frequency counts from processed corpus will replace external lists. Same LMDB compilation, different ordering source.
- **Patrick mentioned** the engine may have downloaded an MIT-licensed common words list (Wiktionary-derived). If that exists, we can merge it with the current freq_rank data for better coverage.

## DB-Side Compilation Plan

1. **Query per word length**: Labels first (ordered by freq_rank within labels, then alphabetical), then non-labels ordered by freq_rank (NULL last).
2. **Pack into binary buffer** using engine-specified struct layout.
3. **Write to LMDB** — one key per word length, value = contiguous packed buffer.
4. **Metadata key** per length — entry counts, tier boundaries, label count.
5. **Script**: Python compilation routine, runs offline. Connects to Postgres, writes LMDB. Can be re-run whenever vocab or frequency data changes.

## What DB Needs From Engine

- [ ] GPU particle struct definition (fields, byte widths, endianness)
- [ ] Label tag encoding preference (flag field vs. separate particle group)
- [ ] Tier boundary delivery preference (header in buffer vs. metadata key)
- [ ] Confirmation: is one contiguous buffer per word length the right granularity, or should it be per particle_key bucket (letter+length)?

## Current Engine References

- `TieredVocabEntry` — `HCPResolutionChamber.h:49`
- `ChamberVocab` / `AssignTiers()` — `HCPResolutionChamber.cpp:112`
- `BuildFromDatabase()` — `HCPResolutionChamber.cpp:161`
- `VocabPack` — `HCPVocabBed.h:46`
- `BedManager::Resolve()` — `HCPVocabBed.cpp`

---

## DB Response (2026-02-28) — Answers to Open Items

### tokenId Max Width — RESOLVED

All token_ids in hcp_english are **exactly 14 bytes** (5-pair decomposed format `AB.XX.YY.ZZ.WW`). This is invariant across all 1.4M tokens. The LMDB compiler uses 14-byte fixed width, not 32.

**Entry size per sub-db**: `wordLength + 14` bytes (e.g., vbed_05 = 19 bytes/entry).

### Frequency Lists — RESOLVED

Merged two sources into `freq_rank`:
- **Wikipedia word frequency 2023** (MIT License, 2.7M entries) — primary, written English
- **OpenSubtitles full** (CC BY-SA-4.0, 1.6M entries) — supplementary, dialogue English

Merge strategy: geometric mean of ranks for words in both sources, Wikipedia-only words ranked by Wikipedia position, OpenSubtitles-only words appended after.

**New coverage**: 176,463 / 1,395,462 tokens ranked (12.6%) — up from 37,487 (2.7%). Top 50K covers ~80% of any running text per Zipf's law.

Scripts: `scripts/merge_frequency_ranks.py`, `scripts/compile_vocab_lmdb.py`

### LMDB Compilation — DONE (v1, pre-morphological-normalization)

First compilation complete at `data/vocab.lmdb/` (30.2 MB). 1,284,915 entries across 15 sub-databases, tier-ordered per agreed scheme. Verification passes. Will recompile after morphological stripping is applied.

### Morphological Normalization — ACKNOWLEDGED

Engine's morph bit field spec and stripping rules received. DB will implement:

1. **Suffix stripping function** following the priority-ordered rule table
2. **Base existence verification** against hcp_english
3. **Dedup pass** — remove inflected forms where base exists, keep base only
4. **Irregular exception handling** — base existence check is the primary guard; explicit exception list for edge cases
5. **Recompile LMDB** after stripping — expect ~25-35% entry reduction

The `-s` ambiguity (plural vs. 3sg verb) will use `PLURAL | 3RD` dual-mark as engine suggested — context resolves downstream.

Implementation in progress.
