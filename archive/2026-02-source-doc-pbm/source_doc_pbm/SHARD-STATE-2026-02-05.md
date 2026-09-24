# Shard State Reference (2026-02-05)

_Snapshot from database construction instance at compact time_

## Database Architecture

**PostgreSQL** = construction workbench (writes, analysis, cross-shard assembly)
**LMDB** = future compiled inference layer (read-only, hot-swappable)
**Shards** = ~2GB target for fast memory swapping (scene elements, not RAM limits)

## Current Shards

### Core (`hcp_core`) — 12 MB / 2.3 MB dump
- **2,450 tokens** in AA namespace
- Byte codes, Unicode characters, structural markers
- `namespace_allocations` table (source of truth for addressing)
- Foundation layer — all other shards reference this

### English (`hcp_english`) — 2.1 GB Postgres / 685 MB dump
- **1,252,854 tokens** total
  - Words: 1,146,520 (noun 787K, verb 181K, adj 149K, adv 24K, etc.)
  - Affixes: 3,696 (prefix, suffix, infix, interfix, circumfix)
  - Derivatives: 93,514 (89K forms, 3.5K abbreviations, 493 contractions, 21 initialisms)
  - Multi-word: 9,084 (phrases, proverbs, prep phrases)
- **Kaikki dictionary data** (permanent, not temp):
  - entries: ~1.29M rows
  - senses: ~1.5M rows
  - forms: ~870K rows
  - relations: ~450K rows
- **26,781 multi-word entries** with NULL word_token — proper noun phrases awaiting x* (Things) shard
- All tokens atomized to character Token IDs (AA.AB.AA.*)
- Cross-shard y* references in relations (2,757 linked)

### Names (`hcp_names`) — 133 MB Postgres / 58 MB dump
- **150,528 tokens** in yA namespace
- Single-word name components (proper nouns, labels, name parts)
- Cross-linguistic — shared across all language shards
- 143,873 entries with senses/forms/relations (moved from english)
- Components extracted from multi-word names too (e.g., "South", "China", "Sea" all have y* tokens even though "South China Sea" entry stays in english)

## Token Addressing

Base-50: A-Z + a-z minus O/o (50 symbols, case-significant)

| Mode | Prefix | Contents |
|------|--------|----------|
| AA | Universal | Byte codes, characters, NSM primitives, structural |
| AB | Text | Language families (AB.AB = English) |
| vA | People | Named individuals (reserved) |
| wA | Places | Named locations (reserved) |
| xA | Things | Named things/orgs, common labels (reserved) |
| yA | Names | Name components — atoms that v/w/x decompose to |
| zA | PBMs | Stored expressions, documents, source PBMs |

### English Token Format (AB.AB.*)
```
AB.AB.{layer}{sub}.{high}.{low}

Layers (3rd pair, 1st char):
A = Affixes
B = Fragments (reserved)
C = Words
D = Derivatives
E = Multi-word
```

### Name Token Format (yA.*)
```
yA.{high}.{low}

Flat sequential count, no sub-classification.
6.25M addresses available.
```

## Key Implementation Details

### Atomization
- All tokens atomize to character Token IDs
- Character tokens at `AA.AB.AA.{category}.{n}`
- Stored as JSON array in `atomization` column
- Phrases atomize to word tokens if all components exist, else to characters

### Cross-Shard References
- Relations can reference tokens in other shards
- 2,757 relations currently point to y* name tokens
- ~40K relations still have missing target_tokens (obscure/archaic words not yet tokenized)

### Data Gaps
- 149,824 capitalized tokens still in english shard (initialisms, abbreviations, etc.) — classification deferred
- Multi-word proper nouns in english awaiting x* shard
- ~40K archaic/obscure words referenced but not tokenized

## Files

```
db/
├── core.sql      # 2.3 MB (LFS)
├── english.sql   # 685 MB (LFS)
└── names.sql     # 58 MB (LFS)

src/hcp/
├── core/
│   └── token_id.py    # Base-50 encoding, namespace constants
├── db/
│   ├── postgres.py    # Core shard connector
│   ├── english.py     # English shard connector
│   └── names.py       # Names shard connector
└── ingest/
    ├── words.py       # Kaikki → English tokens
    └── names.py       # Proper nouns → Names shard
```

## For Grammar/Analysis Instance

When processing text for PBM construction:
1. Tokenize words → look up in english shard first
2. Capitalized/unknown → check names shard
3. Still unknown → flag as TBD, include in submission manifest
4. Character fallback always available via core shard

The grammar analysis layer sits above this — it needs token streams, not raw text. The shards provide the vocabulary; grammar provides the structure.
