# PostgreSQL → LMDB Assembly Pipeline Design

**From:** hcp_db (DB Specialist)
**Date:** 2026-02-17
**Status:** Design proposal — awaiting review
**Builds on:** pbm-storage-schema-design.md (sections 8-9, LMDB assembly analysis)

---

## 1. Architecture Overview

```
PostgreSQL (WRITE layer — authoritative)
    │
    │  Assembly step: extract, pack, write
    ▼
LMDB (READ/DRAW layer — scoped, memory-mapped)
    │
    │  Thin glue: read, loop, call API
    ▼
OpenMM (Engine — physics simulation)
```

**PostgreSQL** is the sole writer. All data authoring, ingestion, and schema management happens here.

**LMDB** is read-only from the pipeline's perspective. PostgreSQL builds and rebuilds LMDB instances. The glue layer only reads from LMDB. No writes to LMDB during operation — only the assembler (PostgreSQL-side) writes.

**The glue layer** is trivial: read LMDB entries, loop, call `addParticle` / `addBond`. No tokenizers, no counters, no bond walkers.

### Game engine cache pattern

PostgreSQL pre-assembles each LMDB instance with the most likely tokens for the envelope's scope. If the engine encounters an unknown token at runtime, that's a **cache miss** — not an error. The assembler fetches the missing data from PostgreSQL, updates LMDB, and the engine continues. Same as a game engine hitting an unloaded texture.

---

## 2. LMDB Database Structure

Each activity envelope gets its own LMDB environment (directory with `data.mdb` + `lock.mdb`).

```
/var/hcp/envelopes/
├── env_001/
│   ├── data.mdb      ← single file, memory-mapped
│   └── lock.mdb      ← LMDB reader lock table
├── env_002/
│   ├── data.mdb
│   └── lock.mdb
└── ...
```

### 2.1 Named sub-databases (6 per envelope)

| Sub-database | Key format | Key size | Value format | Purpose |
|-------------|-----------|----------|-------------|---------|
| `vocab` | uint32 (token index) | 4 B | msgpack dict | Token index → metadata |
| `vocab_rev` | token_id string (UTF-8) | ~14 B avg | uint32 | Token ID → integer index |
| `bonds` | uint32(doc_idx) + uint64(pair) | 12 B | msgpack int | Per-document directional bonds |
| `compiled` | uint64(pair) | 8 B | msgpack int | Aggregated bonds across envelope |
| `docs` | uint32 (doc index) | 4 B | msgpack dict | Document registry within envelope |
| `meta` | string key (UTF-8) | variable | msgpack value | Envelope-level metadata |

### 2.2 Key encoding details

**Bond pair packing** — directional, A→B:
```python
pair = (idx_a << 32) | idx_b   # uint64: A in upper 32 bits, B in lower
key = struct.pack('<Q', pair)   # 8 bytes, little-endian
```

**Per-document bond key** — doc prefix enables range scans:
```python
key = struct.pack('<I', doc_idx) + struct.pack('<Q', pair)   # 12 bytes
# Range scan: all bonds for doc_idx=N start with struct.pack('<I', N)
```

**Directionality** is inherent: `(idx_a << 32 | idx_b)` is a different key from `(idx_b << 32 | idx_a)`. The bond `"the" → "cat"` and `"cat" → "the"` are separate LMDB entries with separate counts.

LMDB's default lexicographic key ordering means:
- All bonds from the same token_A are contiguous (A is the upper 32 bits = leading bytes)
- Prefix scan on token_A index efficiently retrieves all bonds from a starter

### 2.3 Value encoding

**msgpack** for all values. Measured sizes:

| Value type | Example | msgpack size |
|-----------|---------|-------------|
| Bond count 1-127 | `msgpack.packb(42)` | **1 byte** |
| Bond count 128-65535 | `msgpack.packb(953)` | 2-3 bytes |
| Vocab entry | `{id, name, cat, sub}` | **52 bytes** avg |
| Doc entry | `{doc_id, name, first_fpb_a, first_fpb_b}` | **97 bytes** |

99.9% of bond counts are < 128, so practically all bond values are **1 byte**. msgpack is more compact than fixed uint32 (4 bytes) for this data.

### 2.4 vocab entry schema

```python
msgpack.packb({
    'id':  'AB.AB.CD.AH.xN',   # Full token_id string (for display/debug)
    'name': 'the',               # Human-readable name
    'cat':  'word',              # Category
    'sub':  'determiner',        # Subcategory
    'ns':   'AB',                # Namespace prefix (for cache miss shard routing)
})
```

Extensible: force constants, sub-cat patterns, entity references can be added as fields without schema changes.

---

## 3. Assembly Process

### 3.1 Data flow

```
PostgreSQL shards
  ├─ hcp_core ───────── always load: 5,411 tokens (430 KB in LMDB)
  ├─ hcp_english ────── load tokens referenced by envelope bonds
  └─ hcp_en_pbm ────── extract bonds via PBM prefix tree
         │
         ▼
    UNION ALL extraction (prefix reconstruction in SQL)
         │
         ▼
    Stream flat (token_A, token_B, count) triples
         │
         ├─→ Collect unique token IDs
         │      │
         │      ▼
         │   Batch-fetch metadata from PostgreSQL
         │      │
         │      ▼
         │   Assign integer indices (sequential 0..N-1)
         │      │
         │      ▼
         │   Write vocab_db + vocab_rev (LMDB)
         │
         ├─→ Map token_A/B to integer indices
         │      │
         │      ▼
         │   Pack bond keys + msgpack count values
         │      │
         │      ▼
         │   Write bonds_db + compiled_db (LMDB)
         │
         └─→ Register docs + write meta (LMDB)
```

### 3.2 SQL extraction queries

**Core vocabulary** (always loaded, one query):
```sql
-- hcp_core: 5,411 tokens, ~430 KB in LMDB
SELECT token_id, name, category, subcategory, metadata
FROM tokens
WHERE ns = 'AA'
ORDER BY token_id;
```

**PBM bonds** (per document, uses the prefix tree schema):
```sql
-- Reconstructs full token IDs from the prefix tree
-- See pbm-storage-schema-design.md section 9 for full query
SELECT s.token_a_id,
       'AB.AB.' || wb.b_p3 || '.' || wb.b_p4
                 || COALESCE('.' || wb.b_p5, '') AS token_b_id,
       wb.count
FROM pbm_word_bonds wb
JOIN pbm_starters s ON s.id = wb.starter_id
WHERE s.doc_id = :doc_pk
UNION ALL
-- ... char_bonds (prepend 'AA.') ...
UNION ALL
-- ... marker_bonds (prepend 'AA.AE.') ...
```

**Document-referenced English tokens** (batch fetch after bond extraction):
```sql
-- hcp_english: Only tokens that appear in the envelope's bonds
SELECT token_id, name, category, subcategory
FROM tokens
WHERE token_id = ANY(:token_ids);
```

### 3.3 Assembly pseudocode

```python
def assemble_envelope(envelope_id, doc_pks, pg_conns):
    """Build an LMDB envelope from PostgreSQL data."""

    env = lmdb.open(
        f'/var/hcp/envelopes/{envelope_id}/',
        map_size=1 * 1024 * 1024 * 1024,  # 1 GB virtual (see §6.1)
        max_dbs=8,
    )

    # Open all sub-databases
    vocab_db     = env.open_db(b'vocab')
    vocab_rev_db = env.open_db(b'vocab_rev')
    bonds_db     = env.open_db(b'bonds')
    compiled_db  = env.open_db(b'compiled')
    docs_db      = env.open_db(b'docs')
    meta_db      = env.open_db(b'meta')

    # ── Phase 1: Load core vocabulary (always needed) ──
    token_to_idx = {}
    next_idx = 0

    with pg_conns['hcp_core'].cursor() as cur:
        cur.execute("SELECT token_id, name, category, subcategory "
                    "FROM tokens WHERE ns = 'AA' ORDER BY token_id")
        with env.begin(write=True) as txn:
            for token_id, name, cat, sub in cur:
                txn.put(struct.pack('<I', next_idx),
                        msgpack.packb({'id': token_id, 'name': name,
                                       'cat': cat, 'sub': sub, 'ns': 'AA'}),
                        db=vocab_db)
                txn.put(token_id.encode(), struct.pack('<I', next_idx),
                        db=vocab_rev_db)
                token_to_idx[token_id] = next_idx
                next_idx += 1

    # ── Phase 2: Extract bonds per document, collect vocabulary ──
    all_bonds = []          # (doc_idx, token_a, token_b, count)
    english_tokens = set()  # Token IDs needing metadata fetch

    for doc_idx, doc_pk in enumerate(doc_pks):
        bonds = extract_document_bonds(pg_conns['hcp_en_pbm'], doc_pk)
        for token_a, token_b, count in bonds:
            all_bonds.append((doc_idx, token_a, token_b, count))
            for t in (token_a, token_b):
                if t not in token_to_idx and t.startswith('AB'):
                    english_tokens.add(t)

    # ── Phase 3: Fetch English token metadata, assign indices ──
    if english_tokens:
        with pg_conns['hcp_english'].cursor() as cur:
            cur.execute("SELECT token_id, name, category, subcategory "
                        "FROM tokens WHERE token_id = ANY(%s)",
                        (list(english_tokens),))
            with env.begin(write=True) as txn:
                for token_id, name, cat, sub in cur:
                    txn.put(struct.pack('<I', next_idx),
                            msgpack.packb({'id': token_id, 'name': name,
                                           'cat': cat, 'sub': sub, 'ns': 'AB'}),
                            db=vocab_db)
                    txn.put(token_id.encode(), struct.pack('<I', next_idx),
                            db=vocab_rev_db)
                    token_to_idx[token_id] = next_idx
                    next_idx += 1

    # ── Phase 4: Write bonds ──
    compiled = Counter()

    with env.begin(write=True) as txn:
        for doc_idx, token_a, token_b, count in all_bonds:
            idx_a = token_to_idx[token_a]
            idx_b = token_to_idx[token_b]
            pair = (idx_a << 32) | idx_b

            # Per-document bond
            txn.put(struct.pack('<IQ', doc_idx, pair),
                    msgpack.packb(count), db=bonds_db)

            # Aggregate for compilation
            compiled[pair] += count

    with env.begin(write=True) as txn:
        for pair, total_count in compiled.items():
            txn.put(struct.pack('<Q', pair),
                    msgpack.packb(total_count), db=compiled_db)

    # ── Phase 5: Write document registry + metadata ──
    with env.begin(write=True) as txn:
        for doc_idx, doc_pk in enumerate(doc_pks):
            txn.put(struct.pack('<I', doc_idx),
                    msgpack.packb({'doc_pk': doc_pk,
                                   'doc_id': '...',  # from pbm_documents
                                   'name': '...'}),
                    db=docs_db)

        txn.put(b'envelope_id', msgpack.packb(envelope_id), db=meta_db)
        txn.put(b'token_count', msgpack.packb(next_idx), db=meta_db)
        txn.put(b'bond_count', msgpack.packb(len(compiled)), db=meta_db)
        txn.put(b'doc_count', msgpack.packb(len(doc_pks)), db=meta_db)

    return env
```

---

## 4. Envelope Lifecycle

### 4.1 States

```
 ┌─────────┐      assemble       ┌────────┐      glue reads     ┌────────┐
 │ PLANNED │ ──────────────────→ │ ACTIVE │ ←─────────────────→ │ ENGINE │
 └─────────┘                     └────────┘                     └────────┘
      │                               │
      │                          cache miss
      │                               │
      │                          ┌────────┐
      │                          │  PG    │ fetch shard,
      │                          │ UPDATE │ write to LMDB
      │                          └────────┘
      │                               │
      │                          ┌────────┐
      │                          │ STALE  │ scope changed
      │                          └────────┘
      │                               │
      │         re-assemble           │
      └──────────── or ──────────────→│
                                      │
                                 ┌────────┐
                                 │  TORN  │ close + delete
                                 │  DOWN  │
                                 └────────┘
```

### 4.2 Operations

**Create (PLANNED → ACTIVE):**
1. Orchestrator determines envelope scope (which documents/topics)
2. Assembler runs the full assembly process (§3.3)
3. LMDB environment is ready for readers

**Read (ACTIVE):**
- Glue layer opens read transactions on the LMDB environment
- Reads vocab, iterates bonds, calls OpenMM API
- Multiple concurrent readers allowed (LMDB MVCC)

**Cache miss (ACTIVE → UPDATE → ACTIVE):**
1. Glue encounters unknown token (not in `vocab_rev`)
2. Calls assembler's `ensure_token(token_id)`:
   - Assembler determines shard via namespace prefix + shard_registry
   - Queries PostgreSQL for token metadata
   - Assigns next integer index
   - Writes to vocab_db + vocab_rev in a single write transaction
   - Returns new index
3. Glue retries the lookup — token is now available
4. For missing documents: `ensure_document(doc_id)` extracts full bond set

**Refresh (scope change):**
- If the envelope's scope expands (new documents added to context):
  - Assembler extracts bonds for new documents
  - Fetches any new tokens
  - Writes new bonds + updates compiled bonds
  - Existing data untouched — additive only
- If scope contracts: simpler to tear down and re-assemble than surgically remove data

**Tear down:**
1. Close all reader transactions
2. Close LMDB environment
3. Delete the envelope directory (`data.mdb` + `lock.mdb`)

### 4.3 Cache miss handler

```python
class Assembler:
    """PostgreSQL-side LMDB writer. The ONLY entity that writes to LMDB."""

    def __init__(self, pg_conns, lmdb_env, sub_dbs):
        self.pg = pg_conns
        self.env = lmdb_env
        self.dbs = sub_dbs
        self.next_idx = self._get_current_vocab_size()

    def ensure_token(self, token_id):
        """Cache miss: token not in LMDB. Fetch from PG, write to LMDB."""
        # Check if already present (race condition guard)
        with self.env.begin(db=self.dbs['vocab_rev']) as txn:
            result = txn.get(token_id.encode())
            if result is not None:
                return struct.unpack('<I', result)[0]

        # Determine shard
        ns = token_id.split('.')[0]
        shard = self._lookup_shard(ns)

        # Fetch from PostgreSQL
        with self.pg[shard].cursor() as cur:
            cur.execute(
                "SELECT name, category, subcategory "
                "FROM tokens WHERE token_id = %s", (token_id,))
            row = cur.fetchone()
            if row is None:
                raise ValueError(f"Token {token_id} not found in {shard}")

        # Assign index and write to LMDB
        idx = self.next_idx
        self.next_idx += 1

        with self.env.begin(write=True) as txn:
            txn.put(struct.pack('<I', idx),
                    msgpack.packb({'id': token_id, 'name': row[0],
                                   'cat': row[1], 'sub': row[2], 'ns': ns}),
                    db=self.dbs['vocab'])
            txn.put(token_id.encode(), struct.pack('<I', idx),
                    db=self.dbs['vocab_rev'])

        return idx

    def ensure_document(self, doc_id):
        """Cache miss: document bonds not in LMDB. Extract from PG."""
        # ... similar pattern: extract bonds, ensure all tokens,
        #     write bonds + update compiled ...
```

---

## 5. Concurrency Model

### 5.1 LMDB's native concurrency

| Operation | Concurrency | Blocking |
|-----------|------------|----------|
| Read + Read | Fully concurrent | None (lock-free MVCC) |
| Read + Write | Concurrent | Writer doesn't block readers |
| Write + Write | Serialized | Second writer waits for first to commit |

### 5.2 Our usage pattern

**Within one envelope:**
- **Readers** (glue layer): multiple concurrent read transactions. Each reader sees a consistent snapshot. Non-blocking.
- **Writer** (assembler): single write transaction at a time. Doesn't block readers. Readers on older snapshots don't see uncommitted writes.
- When a write transaction commits, the **next** read transaction sees the new data. In-flight read transactions continue with their snapshot.

**Across envelopes:**
- Each envelope = separate LMDB environment = independent locks
- Assembler can write to envelope A and envelope B **simultaneously** (different environments, different write locks)
- "Dozens of shards simultaneously on the PostgreSQL side" — PostgreSQL handles concurrent connections to multiple shards natively. Each shard extraction runs as a separate query on the appropriate database.

### 5.3 Thread safety

```
Assembler Thread Pool (PostgreSQL side)
├── Thread 1: writing to envelope_001 LMDB
├── Thread 2: writing to envelope_002 LMDB
├── Thread 3: querying hcp_english for envelope_003
└── Thread 4: handling cache miss for envelope_001
    (waits for Thread 1's write txn to commit)

Glue Thread Pool (read side)
├── Thread A: reading envelope_001 LMDB → OpenMM
├── Thread B: reading envelope_001 LMDB → OpenMM  (concurrent, no conflict)
├── Thread C: reading envelope_002 LMDB → OpenMM
└── Thread D: encountered cache miss in envelope_001
    → calls Assembler.ensure_token() → blocks until write completes
```

---

## 6. Size Estimates and Scaling

### 6.1 Measured sizes (from Frankenstein PBM, real data)

| Component | Single novel | 10 novels | 100 novels | 1,000 novels |
|-----------|-------------|-----------|------------|-------------|
| Tokens | 7,431 | ~30,000 | ~80,000 | ~200,000 |
| Compiled bonds | 43,535 | ~200,000 | ~2,200,000 | ~10,000,000 |
| vocab_db | 406 KB | 1.6 MB | 4.3 MB | 10.7 MB |
| vocab_rev | 131 KB | 530 KB | 1.4 MB | 3.5 MB |
| compiled_db | 383 KB | 1.7 MB | 19.2 MB | 87.1 MB |
| bonds_db (per-doc) | 553 KB | 5.3 MB | 53.0 MB | ~530 MB |
| docs + meta | ~0.2 KB | ~1 KB | ~10 KB | ~100 KB |
| **Total (w/ overhead)** | **~1.2 MB** | **~4 MB** | **~27 MB** | **~110 MB** |

Core vocabulary (always loaded): **430 KB** for all 5,411 core tokens.

All well within the 2 GB shard target. A 1,000-novel compiled envelope is 110 MB.

### 6.2 map_size setting

LMDB's `map_size` is virtual address space, not physical allocation. LMDB only allocates actual pages as data is written. Set generously:

| Envelope type | Recommended map_size |
|--------------|---------------------|
| Single document | 64 MB |
| Small envelope (1-10 docs) | 256 MB |
| Medium envelope (10-100 docs) | 512 MB |
| Large envelope (100-1000 docs) | 1 GB |

If an envelope grows beyond map_size (e.g., many cache miss additions), the environment must be closed and re-opened with a larger size. Setting 1 GB as default avoids this for almost all cases.

### 6.3 Assembly timing estimates

| Phase | Single doc | 100 docs |
|-------|-----------|----------|
| Core vocab load (PG → LMDB) | ~50 ms | ~50 ms (once) |
| Bond extraction (PG UNION ALL) | ~100 ms | ~5 s |
| Token metadata fetch (PG batch) | ~50 ms | ~200 ms |
| LMDB writes (vocab + bonds) | ~20 ms | ~500 ms |
| Compilation (in-memory Counter) | ~5 ms | ~200 ms |
| **Total** | **~225 ms** | **~6 s** |

These are rough estimates. The bottleneck is PostgreSQL extraction, where the prefix tree optimization provides 60% I/O savings.

---

## 7. Connection to PBM Prefix Tree Schema

The PBM prefix tree (designed in pbm-storage-schema-design.md) serves as the **compressed storage format** in PostgreSQL. The assembly pipeline is the **decompression boundary**:

```
PBM prefix tree (PostgreSQL)     ← compressed, portable, efficient dumps
    │
    │  UNION ALL + prefix reconstruction (SQL)
    │  e.g., 'AB.AB.' || b_p3 || '.' || b_p4 || ...
    │
    ▼
Flat (token_A, token_B, count) triples     ← decompressed, streaming
    │
    │  Integer index mapping + msgpack
    │
    ▼
LMDB (integer-keyed, msgpack-valued)     ← re-compressed for read speed
```

**The prefix tree optimizes the WRITE layer.** Compact storage, portable dumps, efficient PostgreSQL I/O.

**LMDB optimizes the READ layer.** Integer keys for fast comparison, msgpack for compact values, memory-mapped for zero-copy access.

**The assembly step bridges them.** Prefix reconstruction is trivial string concatenation in SQL. Integer index assignment is a sequential enumerate. The heavy work is in PostgreSQL (query execution) and LMDB (B-tree insertion), both of which are written in C.

### 7.1 Why not keep the prefix tree in LMDB?

We could mirror the hub-spoke structure in LMDB (starters sub-db, reduced bond keys). This would preserve compression but add indirection at read time — every bond read would require two lookups (starter → token_A, then bond → count).

For the READ layer, flat integer keys are better:
- Single lookup per bond (no indirection)
- Fixed-width 8-byte keys — optimal for LMDB's B-tree
- Natural prefix ordering for range scans
- The engine (OpenMM) works with integer particle indices anyway

The prefix tree served its purpose: compact PostgreSQL storage → efficient extraction → small dumps. At the LMDB boundary, we flatten for read speed.

---

## 8. Glue Layer Read Pattern

The glue layer is the thin pass from LMDB to OpenMM. Here's what it does:

```python
def build_openmm_system(env, dbs, doc_idx=None):
    """Build an OpenMM system from an LMDB envelope."""

    system = openmm.System()
    force = openmm.HarmonicBondForce()

    # ── Step 1: Define particles from vocabulary ──
    with env.begin(db=dbs['vocab']) as txn:
        cursor = txn.cursor()
        for key, value in cursor:
            # Each vocab entry = one particle
            system.addParticle(1.0)  # mass

    # ── Step 2: Define bonds ──
    if doc_idx is not None:
        # Single document reconstruction
        bond_db = dbs['bonds']
        prefix = struct.pack('<I', doc_idx)
        with env.begin(db=bond_db) as txn:
            cursor = txn.cursor()
            if cursor.set_range(prefix):
                while cursor.key().startswith(prefix):
                    pair = struct.unpack('<Q', cursor.key()[4:])[0]
                    idx_a = pair >> 32
                    idx_b = pair & 0xFFFFFFFF
                    count = msgpack.unpackb(cursor.value())
                    force.addBond(idx_a, idx_b, 0.0, count)
                    if not cursor.next():
                        break
    else:
        # Compiled envelope
        with env.begin(db=dbs['compiled']) as txn:
            cursor = txn.cursor()
            for key, value in cursor:
                pair = struct.unpack('<Q', key)[0]
                idx_a = pair >> 32
                idx_b = pair & 0xFFFFFFFF
                count = msgpack.unpackb(value)
                force.addBond(idx_a, idx_b, 0.0, count)

    system.addForce(force)
    return system
```

This is the entire glue layer for PBM bond loading. Read LMDB, loop, call addParticle/addBond. Nothing else.

---

## 9. Future Extensions

**Force constants in LMDB:** Add a `forces` sub-database containing force infrastructure tokens and sub-categorization patterns from hcp_core/hcp_english. Same pattern: PostgreSQL extracts → msgpack → LMDB.

**Entity references:** Entity databases (people, places, things) can be loaded into LMDB sub-databases using the same vocabulary + relationship pattern.

**FastAPI assembler service:** The assembler could run as a FastAPI service, exposing endpoints like `POST /envelopes`, `GET /envelopes/{id}/status`, `POST /envelopes/{id}/ensure-token`. This separates the write path from the read path as distinct processes. FastAPI + uvicorn are already installed.

**Warm envelope pool:** Pre-assemble frequently-used envelopes and keep them in a pool. Background process monitors usage patterns and pre-warms anticipated envelopes (the "background caching" from Activity Envelopes architecture).

---

## 10. Open Questions

1. **Envelope scope determination:** Who decides which documents belong in an envelope? The orchestrator? A topic model? Manual selection? This design handles any answer — it takes a list of doc_pks and assembles.

2. **Per-document bonds vs compiled-only:** Should per-document bonds always be stored in LMDB, or only when single-document reconstruction is needed? Compiled-only envelopes would be ~50% smaller (no bonds_db). Per-document bonds add the ability to reconstruct any individual document without going back to PostgreSQL.

3. **Cache miss latency tolerance:** The game engine pattern assumes cache misses are "momentary hiccups." For a synchronous cache miss, the glue blocks while the assembler queries PostgreSQL + writes LMDB. Typical latency: 10-50 ms. Acceptable for a texture-loading hiccup?

4. **Envelope persistence:** Should envelopes persist across system restarts, or are they ephemeral? LMDB files on disk persist naturally, but the question is whether the orchestrator should rebuild them or reuse them.

5. **Compiled bond aggregation:** Current design uses simple sum: `compiled_count = sum(count across docs)`. Should this be weighted by document relevance, recurrence count, or other factors? The shared knowledge mentions "bond strengths × recurrence count in compilation."
