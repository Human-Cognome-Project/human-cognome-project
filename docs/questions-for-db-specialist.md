# LMDB Sub-DB Contract — Answers from DB Specialist

**Date:** 2026-02-19

## Agreed Layout: 7 Sub-Databases

The resolver now writes to the **same sub-db names** the C++ already expects.
No "3 new databases" — we keep w2t/c2t/l2t/t2w/t2c, replace `seq` with
`forward`, add `meta`.

| Sub-DB    | Key             | Value           | Purpose                        |
|-----------|-----------------|-----------------|--------------------------------|
| `w2t`     | word form (UTF-8) | token_id (UTF-8) | Word/var → token_id          |
| `c2t`     | char byte (UTF-8) | token_id (UTF-8) | Single char → token_id       |
| `l2t`     | label (UTF-8)     | token_id (UTF-8) | Structural label → token_id  |
| `t2w`     | token_id (UTF-8)  | word form (UTF-8) | Reverse: token → word        |
| `t2c`     | token_id (UTF-8)  | char byte (UTF-8) | Reverse: token → char        |
| `forward` | prefix (UTF-8)    | "0"/"1"/token_id | Boilerplate walk (see below) |
| `meta`    | (reserved)        | (reserved)       | Future eviction tracking     |

**All values are plain UTF-8 strings.** No msgpack, no structured formats.
Zero-copy mmap reads in C++: just cast `MDB_val.mv_data` to `const char*`.

## Answers

### 1. Token lookups (w2t, c2t, l2t)

These stay exactly as they were. Same key/value format.

- **w2t**: `"hello"` → `"AB.AB.CD.AH"` (word → token_id)
- **c2t**: `","` → `"AA.AA.AA.AA.BC"` (char → token_id)
- **l2t**: `"newline"` → `"AA.AB.AA.AJ"` (label → token_id)

Words and chars are separate dbs because a single character can be both
a word ("a" the article) and a char ('a' the byte). Different token_ids,
no collision.

Var tokens also go in w2t (they occupy word positions in documents).

### 2. `forward` sub-db (replaces `seq`)

**This is NOT the old seq format.** The old seq used tab-separated
continuation lists. The new forward db uses a simple 3-state encoding:

| LMDB result    | Meaning                | Engine action           |
|----------------|------------------------|-------------------------|
| Key not found  | Uncached — signal resolver | Wait for fill, re-query |
| `"0"`          | Cached negative (no match) | Stop walk, emit single |
| `"1"`          | Partial match (keep going) | Extend prefix, re-check |
| `"wA.XX.YY.ZZ"` | Complete match (token_id) | Emit sequence token    |

Key format: `"chunk_0 chunk_1 chunk_2"` — space-separated prefix being tested.

**No continuation list needed.** The engine doesn't get a list of "possible
next words" — it just tries the next word and gets yes/no back.

### 3. `meta` sub-db

Reserved for future eviction tracking (LRU timestamps, entry counts).
Empty for now.

### 4. Cache miss pipeline

The engine signals a miss explicitly. Flow:

1. C++ reads LMDB (e.g., w2t for "hello") → `MDB_NOTFOUND`
2. C++ calls resolver service with the miss request
3. Resolver queries Postgres, writes result to LMDB
4. C++ re-reads LMDB → hit

The resolver fills BOTH forward and reverse dbs on each miss
(e.g., writes w2t AND t2w for a word lookup).

### 5. Engine lookup mapping

| Engine call | Sub-DB | Notes |
|---|---|---|
| `LookupChunk(chunk)` | w2t first, then c2t for single chars | **Drop the continuations array** — see below |
| `LookupWord(word)` | w2t | Unchanged |
| `LookupChar(c)` | c2t | Unchanged |
| `LookupLabel(label)` | l2t | Unchanged |
| `CheckContinuation(acc, next)` | forward | Key = `"acc next"`, read value as 3-state |
| `TokenToWord(id)` | t2w | Unchanged |
| `TokenToChar(id)` | t2c | Unchanged |

## What Needs to Change in C++

### a. `Load()` — sub-db names
Replace `openDb("seq", &m_seq)` with `openDb("forward", &m_fwd)`.
The 5 existing dbs (w2t, c2t, l2t, t2w, t2c) stay as-is.

### b. Drop `LookupResult.continuations`
The old model returned a list of possible next words from the seq db.
This is gone. `LookupChunk` should return token_id only — no continuation
data. The `HasContinuations()` gate in the tokenizer must be removed.

### c. Always attempt forward walk (when source tags present)
The tokenizer currently only enters the continuation walk when
`HasContinuations()` is true. In the new model, always attempt the
forward check by peeking at the next chunk and checking
`"current_chunk next_chunk"` against the `forward` sub-db.

In practice: if no boilerplate exists for the document's source,
every forward check returns "0" from LMDB (one read, ~microsecond).

### d. `CheckContinuation` — new format
```cpp
AZStd::string val = LmdbGet(m_fwd, extended.c_str(), extended.size());
if (val.empty())
{
    // Key not found — uncached, signal resolver for backfill
    // After resolver fills, re-read will return "0", "1", or token_id
    return result; // Miss
}
if (val == "0")
{
    return result; // Cached negative — no match
}
if (val == "1")
{
    result.status = ContinuationResult::Continue;
}
else
{
    // Value is the sequence token_id
    result.status = ContinuationResult::Complete;
    result.sequenceId = val;
}
```

### e. `mdb_env_set_maxdbs` — bump to 8
Currently set to 10, so this is already fine.

## LMDB is Ready

Fresh cold cache at `data/vocab.lmdb/` with all 7 sub-dbs created and empty.
32 KB on disk. Resolver (`src/hcp/cache/resolver.py`) updated to match this
contract — plain UTF-8, correct sub-db names.
