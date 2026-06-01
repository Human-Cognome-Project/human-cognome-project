# LMDB Sub-DB Contract — Updated 2026-03-01

**Revision**: Removed `forward` sub-db (boilerplate push on cache miss replaces forward walk).
Added `vbed_*` (pre-compiled vocab), `env_*` (envelope-loaded data), `_manifest` (eviction tracking).

## Sub-Database Layout

| Sub-DB | Key | Value | Format | Owner |
|--------|-----|-------|--------|-------|
| `w2t` | word form (UTF-8) | token_id (UTF-8) | text | CacheMissResolver |
| `c2t` | 4-byte codepoint | token_id (UTF-8) | text | CacheMissResolver |
| `l2t` | label form (UTF-8) | token_id (UTF-8) | text | CacheMissResolver |
| `t2w` | token_id (UTF-8) | word form (UTF-8) | text | CacheMissResolver |
| `t2c` | token_id (UTF-8) | 4-byte codepoint | text | CacheMissResolver |
| `vbed_02`..`vbed_16` | `"data"` / `"meta"` | packed binary / VBedMeta struct | binary | BedManager |
| `env_*` | query-defined key (UTF-8) | query-defined value (UTF-8) | text | EnvelopeManager |
| `_manifest` | `env:<name>\0<sub_db>` | 8-byte timestamp | binary | EnvelopeManager |

### Ownership rule

Each module owns its sub-db namespace. Modules must NOT write to another module's sub-dbs.
- **BedManager** owns `vbed_*` — pre-compiled binary vocab loaded at startup
- **CacheMissResolver** owns `w2t`, `c2t`, `l2t`, `t2w`, `t2c` — filled on cache miss from Postgres
- **EnvelopeManager** owns `env_*` and `_manifest` — filled on envelope activation from stored queries

### Format heterogeneity

`vbed_*` uses packed binary (memcpy to VRAM). All others use plain UTF-8.
This is acceptable — different modules, different access patterns. Binary is essential
for GPU buffer loading; text is clean for string lookups.

### `_manifest` format

One key per (envelope, sub_db) pair for eviction tracking:
- **Key**: `env:<envelope_name>\0<sub_db_name>` (null byte separator after envelope name)
- **Value**: activation timestamp (8 bytes, epoch seconds)
- **Eviction**: `MDB_SET_RANGE` to seek `env:<name>\0`, iterate forward, break when prefix no longer matches. Drop each listed sub-db via `mdb_drop`, delete manifest entry via `mdb_cursor_del`.
- **Record**: single `mdb_put` per (envelope, sub_db) pair — no read-modify-write.

## Cache Miss Pipeline

1. C++ reads LMDB (e.g., w2t for "hello") → `MDB_NOTFOUND`
2. C++ calls resolver handler for the sub-db type
3. Handler queries Postgres, writes result to LMDB
4. C++ re-reads LMDB → hit

The resolver fills BOTH forward and reverse dbs on each miss
(e.g., writes w2t AND t2w for a word lookup).

## Engine Lookup Mapping

| Engine call | Sub-DB | Notes |
|---|---|---|
| `LookupWord(word)` | w2t | Word → token_id |
| `LookupCodepoint(cp)` | c2t | 4-byte codepoint key → token_id |
| `LookupChar(c)` | c2t | Delegates to LookupCodepoint |
| `LookupLabel(label)` | l2t | Label → token_id |
| `TokenToWord(id)` | t2w | Reverse: token → word |
| `TokenToChar(id)` | t2c | Reverse: token → codepoint |

## Envelope Query Contract

Envelope queries (stored in `envelope_queries` table in hcp_core) must return exactly
2 columns: (key, value). The EnvelopeManager pipes results directly into the target
LMDB sub-db via `mdb_put(key, value)`. The query shapes data; the pipe is dumb.

## Removed: `forward` sub-db

The `forward` sub-db and CheckContinuation/forward-walk mechanism have been removed.
Boilerplate detection is now handled by push-on-cache-miss: when the WordHandler resolves
a cache miss, it also pushes any relevant boilerplate sequences for the active source.
