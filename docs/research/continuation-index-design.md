# Continuation Index Design

**Status:** Design (updated 2026-02-18)
**Date:** 2026-02-18
**Author:** DB Specialist

## Summary

The continuation index enables detection of **boilerplate** — literal
repeated text blocks — during tokenization. The engine drives a simple
**boolean loop**: submit a growing prefix, get true/false back, keep
going or stop.

**Scope: boilerplate only.** This is a reproduction system, not a
meaning system. Idiom detection is a conceptual mesh concern and does
not belong here.

## Core Mechanism

### Boolean Forward Walk

The engine drives the walk. No arrays, no branching.

1. Engine has chunk_0 resolved as a single token
2. Engine peeks at chunk_1 (next space-to-space unit)
3. Engine submits `"chunk_0 chunk_1"` → true/false against boilerplate
   stores referenced by the document's source metadata
4. **False** → no boilerplate match. Emit chunk_0 as individual token.
5. **True** → concatenate next word: `"chunk_0 chunk_1 chunk_2"` → ask again
6. Loop until false or **token_id returned**
7. **Token_id** → Postgres detected the sequence is complete (next
   position is stream_end), returned the boilerplate entity's token_id
   directly. Engine emits the sequence token.

At any false along the way, the engine falls back to individual tokens
it already has in hand. No wasted work.

### Request Format

The initial request uses a space before the wildcard to avoid collision
with compound words: `"chunk_0 +chunk_1"` — the space makes it
unambiguous that this is a multi-token check, not a substring match.

### LMDB Cache

Forward walk results are cached as simple key → value:

```
Key:   "chunk_0 chunk_1"    (the prefix being tested)
Value: msgpack(true)         — partial match, keep going
       msgpack(false)        — no match
       msgpack("wA.XX.YY.ZZ") — complete match, token_id returned
```

Three-valued: true / false / token_id. Postgres handles the terminal
check internally — when the next position is stream_end, it returns
the boilerplate entity's token_id directly. No separate compiled
string lookup needed.

Vocab entries are separate:

```
Key:   "chunk"              (single token lookup)
Value: msgpack({"t": "AB.AB.CD.AH"})
```

## What Gets Stored

Literal repeated text blocks that appear across documents:

- **Source-specific boilerplate**: Project Gutenberg headers/footers,
  publisher license blocks, standard legal notices
- **Cross-source boilerplate**: text blocks shared by multiple sources
  (promoted from source-specific when detected)
- **Format boilerplate**: recurring chapter headings, standard
  structural patterns within a source corpus

**NOT stored here:** idioms, multi-word entities, or anything semantic.
Those are vocabulary tokens (idioms) or entity records (proper nouns)
handled by their respective systems.

## Source Scoping

Boilerplate is scoped by **source tag**. A source (e.g., Gutenberg.org)
is a Thing entity. Documents tagged with that source only check
boilerplate associated with that source during forward walks.

This keeps the search space tight:
- Gutenberg documents check Gutenberg boilerplate
- If the same block appears in multiple sources, it gets promoted to
  a shared boilerplate pool
- The engine passes the active source tag(s) with the document metadata

## Storage: Thing Entities

Boilerplate sequences are **Thing entities** in the existing entity
shards. No new tables or databases needed.

- Non-fiction sources → `wA` namespace (nf Things)
- Fiction sources → `sA` namespace (fic Things)

Each boilerplate block is a token in the entity shard's `tokens` table:

```
token_id:    wA.XX.YY.ZZ
name:        "Project Gutenberg License Header"
category:    "boilerplate"
subcategory: "gutenberg"            -- source tag
```

The boilerplate's internal token list uses the same position storage
tables (doc_word_positions, etc.) as any document — the entity IS a
mini-document with its own position numbering. The last position holds
the stream_end marker (`AA.AE.AF.AA.AB`).

### Postgres Query for Boolean Check

```sql
-- Does this prefix match any boilerplate for the given source?
-- prefix_tokens = ['chunk_0', 'chunk_1', 'chunk_2']
-- Returns: true (partial), stream_end (complete), or false

SELECT CASE
    WHEN EXISTS (
        -- Check if any boilerplate entity matches prefix at all positions
        -- and has MORE positions after → partial match
        ...
    ) THEN true
    WHEN EXISTS (
        -- Check if prefix matches complete sequence (next pos = stream_end)
        ...
    ) THEN 'AA.AE.AF.AA.AB'  -- stream_end
    ELSE false
END;
```

Exact query depends on position table encoding. The key insight: each
step of the boolean walk is a single EXISTS check, not a scan.

## Boilerplate Detection

Engine-side batch job across corpus identifies boilerplate:
- Detects repeated text blocks across documents sharing a source tag
- Writes results into entity shards as Thing entities
- DB just stores what the engine finds

## Size Estimates

Boilerplate is a small, bounded set:

- Per source: typically 5-20 distinct boilerplate blocks
- Per block: 20-500 tokens (headers, footers, license text)
- LMDB forward walk entries: one per prefix tested, boolean values
- Total overhead: negligible

## Open Questions

1. **Pre-caching** — should all valid boilerplate prefixes be
   pre-loaded into the LMDB forward db when a source tag becomes
   active? Small enough to be practical.
2. **Boilerplate detection** — engine batch job design TBD.
3. **Update path** — when new boilerplate is added, stale LMDB forward
   entries get evicted naturally. New prefixes get cached on first query.
