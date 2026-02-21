# Cache Miss Resolver — C++ Engine Spec

**Date:** 2026-02-21
**From:** DB Specialist
**For:** Physics Engine Specialist

## Overview

The cache miss resolver lives in the engine Gem as native C++. When an
LMDB read returns `MDB_NOTFOUND`, the resolver queries Postgres via libpq,
writes the result to LMDB, and the caller retries the read.

No Python, no socket hop. Direct libpq + liblmdb in-process.

## Architecture: Handler Registry

The current 7 sub-databases (w2t, c2t, l2t, t2w, t2c, forward, meta)
are **one use case** — vocabulary lookups. Future LMDB environments will
have different sub-databases with different structures, different Postgres
queries, and different value formats.

The resolver must be **generic**: a registry of handlers, one per sub-db
type. Each handler knows how to resolve its own cache misses.

```
CacheMissResolver
├── RegisterHandler("w2t", WordHandler)
├── RegisterHandler("c2t", CharHandler)
├── RegisterHandler("l2t", LabelHandler)
├── RegisterHandler("forward", ForwardWalkHandler)
├── RegisterHandler("bonds", BondHandler)          // future: PBM bonds
├── RegisterHandler("entities", EntityHandler)      // future: entity lookups
└── ...any number of handlers
```

### Handler Interface

```cpp
class ICacheMissHandler
{
public:
    virtual ~ICacheMissHandler() = default;

    /// Name of the LMDB sub-database this handler serves.
    virtual const char* GetSubDbName() const = 0;

    /// Resolve a cache miss. Called when mdb_get returns MDB_NOTFOUND.
    ///
    /// @param key      The LMDB key that missed (raw bytes from MDB_val)
    /// @param keyLen   Length of the key
    /// @param context  Request context (document metadata, source tags, etc.)
    /// @param result   Output: the resolved value to write to LMDB
    /// @return true if resolved (result is valid), false if unresolvable
    virtual bool Resolve(
        const char* key, size_t keyLen,
        const ResolveContext& context,
        ResolveResult& result) = 0;
};
```

### Context and Result

```cpp
/// Passed to every handler — carries document-level metadata.
/// Handlers use what they need, ignore the rest.
struct ResolveContext
{
    const char* docId = nullptr;        // Document being processed
    int position = -1;                  // Position in document (if applicable)
    const char** sourceTags = nullptr;  // Source tags for boilerplate scoping
    int sourceTagCount = 0;
    PGconn* pgConn = nullptr;           // Shared Postgres connection
};

/// Handler fills this on successful resolution.
struct ResolveResult
{
    const char* value = nullptr;    // Value bytes to write to LMDB
    size_t valueLen = 0;

    // Optional: additional sub-dbs to write (e.g., reverse lookups)
    struct ExtraWrite
    {
        const char* subDbName;
        const char* key;
        size_t keyLen;
        const char* value;
        size_t valueLen;
    };
    AZStd::vector<ExtraWrite> extraWrites;
};
```

### Resolver Core

```cpp
class CacheMissResolver
{
public:
    /// Register a handler for a sub-database.
    void RegisterHandler(AZStd::unique_ptr<ICacheMissHandler> handler);

    /// Called on MDB_NOTFOUND. Finds the handler for the sub-db,
    /// calls Resolve(), writes result to LMDB (primary + extras).
    /// Returns true if resolved (caller should retry the read).
    bool HandleMiss(
        MDB_env* env,
        MDB_dbi dbi,
        const char* subDbName,
        const char* key, size_t keyLen,
        const ResolveContext& context);

private:
    AZStd::unordered_map<AZStd::string, AZStd::unique_ptr<ICacheMissHandler>> m_handlers;
    PGconn* m_pgConn = nullptr;  // Connection pool or single connection
};
```

## Current Handlers (Vocabulary)

### WordHandler (w2t + t2w)

- **Sub-db:** `w2t`
- **Key:** word form (UTF-8 string)
- **Postgres query:** `SELECT token_id FROM tokens WHERE name = $1 LIMIT 1` on `hcp_english`
- **LMDB write:** key=word → value=token_id
- **Extra write:** t2w: key=token_id → value=word (reverse lookup)
- **Lowercase:** Try lowercase first, then exact match for labels

### CharHandler (c2t + t2c)

- **Sub-db:** `c2t`
- **Key:** single byte (UTF-8 char)
- **Postgres query:** None — computed directly. ASCII byte value → deterministic token_id `AA.AA.AA.AA.{encode_pair(byte)}`
- **LMDB write:** key=char → value=token_id
- **Extra write:** t2c: key=token_id → value=char

### LabelHandler (l2t)

- **Sub-db:** `l2t`
- **Key:** label name (e.g., "newline")
- **Postgres query:** `SELECT token_id FROM tokens WHERE name = $1 AND layer = 'label' LIMIT 1` on `hcp_english`
- **LMDB write:** key=label → value=token_id

### ForwardWalkHandler (forward)

- **Sub-db:** `forward`
- **Key:** space-separated prefix (e.g., "chunk_0 chunk_1")
- **Postgres query:** Checks boilerplate entity position tables scoped by source tags.
  Concatenates decomposed tokens within space-to-space boundaries for comparison.
  Returns "0" (no match), "1" (partial), or token_id (complete).
- **LMDB write:** key=prefix → value="0"/"1"/token_id
- **Note:** When no source tags present, returns "0" immediately (no Postgres query)

### VarHandler (w2t via var DB)

- **Triggered by:** Key starting with `AA.AE.AF.AA.AC` (var_request token)
- **Sub-db:** `w2t` (var tokens occupy word positions)
- **Postgres query:** `SELECT mint_var_token($1)` on `hcp_var`. Also inserts into `var_sources` if doc_id/position provided.
- **LMDB write:** key=chunk → value=var_id
- **Extra write:** t2w: key=var_id → value=chunk

## Value Format

**All current handlers use plain UTF-8 strings.** No msgpack, no binary
packing. Both key and value are UTF-8-encoded, null-terminated optional.

Future handlers can use any byte format — the resolver doesn't interpret
values, it just passes bytes from the handler to `mdb_put`. Each handler
owns its own serialization.

## Postgres Connection

Single libpq connection per resolver instance, or a small pool. Same
connection pattern as `HCPWriteKernel`:

```cpp
PGconn* conn = PQconnectdb(
    "dbname=hcp_english user=hcp password=hcp_dev host=localhost port=5432");
```

Handlers receive the connection via `ResolveContext.pgConn`. Different
handlers may query different databases — the resolver can maintain
connections per database:

```cpp
PGconn* GetConnection(const char* dbname);
// Lazy-opens and caches: hcp_english, hcp_core, hcp_var, hcp_fic_pbm, etc.
```

## LMDB Write Flow

```
1. Engine calls mdb_get(dbi, &key, &val)
2. Returns MDB_NOTFOUND
3. Engine calls resolver.HandleMiss(env, dbi, "w2t", key, keyLen, context)
4. Resolver finds WordHandler
5. WordHandler queries Postgres → gets token_id
6. WordHandler fills ResolveResult:
   - value = token_id
   - extraWrites = [{subDb="t2w", key=token_id, value=word}]
7. Resolver writes primary: mdb_put(w2t_dbi, key, value)
8. Resolver writes extras: mdb_put(t2w_dbi, token_id, word)
9. Returns true
10. Engine retries mdb_get(dbi, &key, &val) → hit
```

## Adding a New Handler

To add a new sub-db type (e.g., PBM bond lookups):

1. Create a class implementing `ICacheMissHandler`
2. Define the sub-db name, Postgres query, and value format
3. Register it: `resolver.RegisterHandler(AZStd::make_unique<BondHandler>())`
4. Create the LMDB sub-db: `mdb_dbi_open(txn, "bonds", MDB_CREATE, &dbi)`

The resolver core doesn't change. Each handler is self-contained.

Example stub:

```cpp
class BondHandler : public ICacheMissHandler
{
public:
    const char* GetSubDbName() const override { return "bonds"; }

    bool Resolve(const char* key, size_t keyLen,
                 const ResolveContext& ctx,
                 ResolveResult& result) override
    {
        // key = packed bond query (format TBD by PBM design)
        // Query hcp_fic_pbm for bond data
        // Write result in whatever format this sub-db uses
        // ...
        return true;
    }
};
```

## Key Points

- **The resolver is generic.** It doesn't know what data it's resolving.
  Handlers do.
- **Each sub-db owns its format.** w2t uses UTF-8 strings. A future bonds
  sub-db might use packed binary. The resolver just moves bytes.
- **Handlers can write to multiple sub-dbs** in one resolution (via
  extraWrites). This keeps forward + reverse lookups atomic.
- **No Python in the runtime path.** The Python resolver
  (`src/hcp/cache/resolver.py`) remains as a reference implementation
  and test tool.
- **Postgres connections are per-database**, lazily opened. Most handlers
  hit one database (hcp_english for words, hcp_var for var tokens, etc.).

## Reference Implementation

Python resolver: `src/hcp/cache/resolver.py`
LMDB contract: `docs/questions-for-db-specialist.md`
