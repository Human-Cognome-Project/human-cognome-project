# Tokenizer Redesign — Space-to-Space with Continuation Index

**Status:** Design (pending DB and infra specialist review)
**Date:** 2026-02-18
**Author:** Patrick (design), Engine Specialist (documentation)

## Summary

Replaces the current character-type-based tokenizer with a space-to-space
analysis unit and multi-level resolution pipeline. The engine tokenizer
stays lean and fast — it handles known vocabulary and simple splits only.
Unresolved sequences route to a dedicated var db kernel in Postgres.

LMDB is no longer pre-populated. It fills itself from Postgres on cache
miss. Every lookup returns a token result AND a continuation index for
detecting longer stored sequences.

## Core Principle: Space-to-Space Analysis Unit

The fundamental tokenization unit is **everything between whitespace
boundaries**. The engine does not pre-split on character type. A chunk
like `don't`, `self-aware`, `http://example.com`, or `word,` is one
unit to analyze first.

True whitespace characters (space, tab, newline, CR) are the only
delimiters. Everything else stays together as the initial chunk.

Whitespace encoding is unchanged: spaces = gaps in position numbering,
newlines = structural tokens (`newline` label).

## Tokenization Flow

### Step 1 — Full Chunk Lookup (LMDB)

Take the space-to-space chunk as-is. Look it up in LMDB.

LMDB returns **two things** on a hit:
- **token_id** — the token for this chunk
- **continuation index** — a list of longer stored token sequences
  that begin with this token (boilerplate, source text, multi-word
  entities, format-common strings)

If the continuation index is non-empty, the engine peeks at subsequent
space-to-space chunks to detect a longer match. If the next chunk
matches a continuation, keep consuming. If not, commit the single
token and move on.

On LMDB miss, the cache miss pipeline fires (see below).

**Result:** Hit with no continuation → emit token, done.
Hit with continuation match → emit longer token, done.
Miss → proceed to Step 2.

### Step 2 — Punctuation/Separator Split

Split the chunk at punctuation and separator characters. Look up each
piece individually.

Example: `"Hello,"` → `"Hello"` + `","` — two lookups.

If all pieces resolve to known word tokens and/or known punctuation
tokens, emit them all and done.

**Result:** All pieces resolve → emit tokens, done.
Some pieces unresolved → proceed to Step 3.

### Step 3 — Greedy Walk (Missing Space Detection)

For unresolved alphabetic sequences, try splitting into known words
starting from the left (greedy longest match). On each match, test
the **remainder** as a single token — if the remainder is itself a
known token, done in two lookups instead of walking the full string.

This catches missing spaces: `"thecat"` → `"the"` + `"cat"`.

**Result:** Full sequence resolves to known words → emit tokens, done.
Still unresolved → proceed to Step 4.

### Step 4 — Var DB Kernel

Any sequence that doesn't resolve through Steps 1–3 routes to the
var db via Postgres.

The engine does NOT atomize to characters here. Postgres handles it:

1. Postgres receives the unresolved sequence
2. Postgres calls the var db
3. Var db creates a **var token**:
   - Token form = the atomized contents of the sequence
   - Var ID = a unique identifier for use in the record
4. Var ID is returned through the pipeline to the engine
5. Engine uses the var ID in the token stream like any other token

The var ID can be easily updated later — when the DI resolves what
the sequence actually is, or when it gets promoted to permanent
vocabulary.

**The var db is the DI's general-purpose staging cache** — not just
for unknown words. Anything to be handled later, or deemed temporary
storage, routes through here.

## Number Handling

Numbers are atomized to individual digit characters. Each digit is a
core char token with a shared prefix in the token namespace.

**Rationale:**
- Digits share a common token prefix → roughly 1 byte of
  distinguishing data per digit beyond the prefix
- Storing `"1234"` as four digit tokens costs approximately the same
  as the original four bytes
- No compression benefit from treating numbers as single tokens
  (they rarely repeat as exact sequences)
- Reconstruction is dead simple — no special number parsing needed
- Same principle as incompressible data in a zip file: passes through
  at ~1:1 without inflating the result

Numbers are split at the punctuation-split step (Step 2). Each digit
resolves from the core char set in LMDB. No var db needed.

**Note:** For specialized document shards (financial, scientific,
engineering), number handling rules could be adjusted if the data
profile justifies it. The architecture supports per-shard
customization without changing the core engine.

## LMDB Changes

### No Pre-Population

LMDB is not seeded or pre-populated. It starts empty (or near-empty)
and fills itself from Postgres via the cache miss pipeline.

### Cache Miss Pipeline

1. Engine looks up chunk in LMDB → miss
2. Level 2: Postgres resolves the token
3. Postgres writes the result to LMDB (token_id + continuation index)
4. If Postgres can't resolve at the top level → Postgres calls var db
   → var token created → var ID returned through pipeline to LMDB
5. Engine retries LMDB → hit

The engine never interacts with the var db directly. It always gets
tokens back from LMDB, regardless of whether they came from the main
vocabulary or were minted as var tokens behind the scenes.

### Continuation Index

Each LMDB entry stores not just the token_id but also an index of
longer stored sequences that begin with this token. This enables
detection of multi-word tokens, boilerplate, source citations, and
other recurring sequences without requiring the engine to know about
them in advance.

Format TBD — needs DB specialist input on how to structure the
continuation data in LMDB efficiently (likely a compact list of
next-expected-chunk → full-sequence-token-id pairs).

## Var DB Overview

**Purpose:** DI staging cache for anything unresolved or temporary.

**Not just unknown words** — any data to be handled later or deemed
temporary storage routes through the var db.

**Key properties:**
- Fully tokenized — var tokens are real tokens with real IDs
- Own rules entirely — separate from main vocabulary logic
- Atomization happens HERE, not in the engine tokenizer
- Var IDs are stable references that can be updated in-place when
  the DI resolves what the content actually is
- Expected to be busy — every novel word, every unrecognized
  sequence, every temporary datum routes here

**Setup:** DB specialist handles core db setup. DB or infra specialist
handles the level 2 cache miss pipeline (direct from Postgres).

## Logging DB

A separate logging database will be added later. Not part of the
tokenizer redesign but mentioned for completeness.

## What Changes in the Engine

### HCPTokenizer.cpp — Rewrite

Current code splits on `isalpha()` boundaries and does greedy
character-level atomization. This is replaced entirely:

- Remove `CharType` enum and `ClassifyChar()` — no more character
  type classification for splitting
- Remove `AtomizeUnknown()` — no more character-level greedy walk
  in the engine
- New flow: whitespace split → full chunk lookup → punctuation split
  → greedy word walk → var db handoff
- Lowercase fallback stays (primary form is always lowercase)

### HCPVocabulary.h/.cpp — Extend

- `LookupWord()` needs to return token_id + continuation index
  (not just token_id)
- LMDB sub-db schema may need adjustment for continuation data
- Cache miss must trigger Postgres pipeline (currently returns empty)

### HCPStorage.h/.cpp — New Kernel

- Var db write kernel — route unresolved sequences to Postgres
- Postgres handles var token creation and returns var ID
- Separate from the main document write kernel (HCPWriteKernel)

### build_vocab_lmdb.py — Remove or Repurpose

- No more pre-population script needed
- LMDB fills from Postgres on demand
- Script may be repurposed for testing/diagnostics only

## Migration Path

1. DB specialist sets up var db schema and core db changes
2. DB or infra specialist sets up level 2 cache miss pipeline
   (Postgres → LMDB, including continuation index)
3. Engine specialist rewrites HCPTokenizer.cpp to new flow
4. Engine specialist extends HCPVocabulary for continuation data
5. Test with Gutenberg batch — real compression numbers with full
   vocabulary available through cache miss pipeline

## Open Questions (for DB/Infra Review)

1. **Continuation index format** — How to store next-token hints
   efficiently in LMDB? Compact list? Separate sub-db?
2. **Var db schema** — Token form storage, var ID generation,
   update/promotion path
3. **Cache miss latency** — Engine needs to wait for pipeline on
   miss. Block? Async callback? Retry loop?
4. **LMDB eviction** — Does LMDB grow unbounded, or does it need
   eviction policy for long-running operation?
5. **Var token namespace** — Where do var tokens live in the token
   addressing scheme?
