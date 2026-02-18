# Continuation Index Design

**Status:** Design (pending review)
**Date:** 2026-02-18
**Author:** DB Specialist

## Summary

The continuation index enables detection of **boilerplate** — literal
repeated text blocks — during tokenization. Every LMDB lookup returns
**two things**: the token_id for the chunk AND a list of possible next
words from stored boilerplate sequences. The engine always has the
single token in hand and decides whether to extend.

**Scope: boilerplate only.** This is a reproduction system, not a
meaning system. Idiom detection is a conceptual mesh concern and does
not belong here.

## Core Mechanism

### LMDB Response Format

Every successful lookup returns both parts:

```
{
    token_id: "AB.AB.CD.AH",           -- always present, confirmed match
    continuations: ["end", "quick"]     -- may be empty
}
```

- **token_id** is never conditional — the chunk is resolved regardless.
- **continuations** is the forward-looking index: surface forms of
  possible next chunks that would extend into a stored boilerplate
  sequence.
- Empty continuations = simple token, no further checks needed.

### Engine Flow

1. Look up chunk → get token_id + continuations
2. If continuations empty → emit token_id, advance, done
3. If continuations non-empty → peek next chunk
4. Next chunk matches a continuation → consume it, check position 3
   of the matching sequence(s), continue peeking
5. Forward walk completes a full sequence → emit the sequence token
6. Forward walk dead-ends (no match at position N) → fall back to
   the single token_id already in hand from step 1

No wasted work: the single token is always resolved. The forward walk
is pure upside — if it finds something, great. If not, the answer was
already there.

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
boilerplate associated with that source during continuation lookups.

This keeps the search space tight:
- Gutenberg documents check Gutenberg boilerplate
- If the same block appears in multiple sources, it gets promoted to
  a shared boilerplate pool
- The engine passes the active source tag(s) with the lookup request

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
mini-document with its own position numbering.

Source scoping = filtering on `category = 'boilerplate'` and
`subcategory` matching the document's source tag.

**Entry point index:** Position 1 of each boilerplate entity provides
the entry point for continuation lookups.

```sql
-- Find all boilerplate starting with a given token, scoped to source
SELECT wp.doc_id AS seq_id
FROM doc_word_positions wp
JOIN tokens t ON t.token_id = wp.doc_id
WHERE wp.t_p3 = :chunk_p3 AND wp.t_p4 = :chunk_p4 AND wp.t_p5 = :chunk_p5
  AND wp.positions LIKE 'AA%'   -- position 1 encoded as first 4 chars
  AND t.category = 'boilerplate'
  AND t.subcategory = :source_tag;
```

The reverse lookup index on `(t_p3, t_p4, t_p5)` already exists in
migration 009.

### Building the Continuation Index for LMDB

When Postgres fills an LMDB cache miss, it also builds the
continuation data by querying the entity shard for boilerplate
starting with the matched token (scoped to active source):

1. Find all boilerplate entities where position 1 = the lookup token
2. For each, get the token at position 2 (the "next word")
3. Pack into the LMDB entry alongside the token_id

### Forward Walk Resolution

When the engine walks forward through a matching sequence, it reads
subsequent positions from the boilerplate entity's position data. If
the full sequence has been pre-fetched into LMDB on the first
continuation hit, the walk happens entirely in LMDB — no Postgres
round-trip after the initial load.

## LMDB Cache Structure

The continuation data is part of the standard LMDB value for a chunk:

```
Key:   chunk surface form (TEXT)
Value: msgpack {
    "t": "AB.AB.CD.AH",              -- token_id
    "c": [                            -- continuations (may be empty)
        {"n": "gutenberg", "s": "wA.XX.YY.ZZ"},
        {"n": "license",   "s": "wA.XX.YY.ZA"}
    ]
}
```

For walk-ahead, individual sequence positions can also be cached in
LMDB if the engine pre-fetches the full boilerplate on first hit.

## Size Estimates

Boilerplate is a small, bounded set:

- Per source: typically 5-20 distinct boilerplate blocks
- Per block: 20-500 tokens (headers, footers, license text)
- Continuation entries per LMDB lookup: usually 0, occasionally 1-3
  for common words that start a boilerplate block
- Total overhead: negligible

## Open Questions

1. **Pre-fetching** — when a continuation hits, should the cache miss
   pipeline pre-load the entire boilerplate into LMDB, or walk one
   position at a time? Full pre-fetch avoids repeated round-trips.
2. **Boilerplate detection** — how are boilerplate blocks identified
   initially? Manual curation per source? Automated detection of
   repeated text across documents sharing a source tag?
3. **Update path** — when new boilerplate is added, LMDB entries for
   its first token need continuation lists updated. Eviction handles
   this naturally (stale entry evicted, fresh one loaded on next miss).
