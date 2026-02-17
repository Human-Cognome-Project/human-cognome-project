# PBM Storage Schema Design — Prefix Tree Optimization

**From:** hcp_db (DB Specialist)
**Date:** 2026-02-17
**Status:** Design proposal — awaiting review

---

## 1. Problem Statement

The old `hcp_en_pbm` schema (migration 007d) stored PBMs as positional content streams — flat rows of `(doc_id, position, token_id)`. This was the wrong format. PBMs are **aggregated bond pairs**: `(token_A, token_B, count)`.

A naive replacement would store flat bond rows:

```sql
-- DON'T DO THIS
CREATE TABLE pbm_bonds (
    doc_ns TEXT, doc_p2 TEXT, doc_p3 TEXT, doc_p4 TEXT, doc_p5 TEXT,  -- 5 cols repeated per row
    a_ns TEXT, a_p2 TEXT, a_p3 TEXT, a_p4 TEXT, a_p5 TEXT,            -- 5 cols repeated per row
    b_ns TEXT, b_p2 TEXT, b_p3 TEXT, b_p4 TEXT, b_p5 TEXT,            -- 5 cols repeated per row
    count INTEGER
);
```

This repeats the full doc_id in every row. It repeats the full token_A for every pairing from the same starter. And it stores full 5-segment token_B addresses even though all English words share the `AB.AB` prefix. For a single novel, that's 43K+ rows of 16 values each.

The requirement is: **hierarchical subtables / prefix tree** where common prefixes are stored once and pairings branch from there, using only distinguishing segments. The optimization must be in the **schema design**, not PostgreSQL-specific features, because dumps must be portable.

---

## 2. Data Analysis — What PBM Bonds Actually Look Like

Analysis of Frankenstein (78K words, 94K stream entries):

### Token namespaces that appear in PBM streams

| Type | Namespace | Shared prefix | Distinguishing segments | Count in hcp_core/english |
|------|-----------|---------------|------------------------|---------------------------|
| English words | `AB.AB.p3.p4.p5` | `AB.AB` | p3, p4, p5 | ~1.4M tokens |
| ASCII punctuation | `AA.AA.AA.AA.p5` | `AA.AA.AA.AA` | p5 only | ~100 chars |
| Unicode punctuation | `AA.AB.AA.p4.p5` | `AA.AB.AA` | p4, p5 | ~2,100 chars |
| PBM markers | `AA.AE.p3.p4` | `AA.AE` | p3, p4 | 91 markers |

### Bond pair distribution (Frankenstein, 43,535 unique pairs)

| B-side type | Rows | % of total | Stored segments |
|-------------|------|-----------|-----------------|
| Word bonds (`B = AB.AB.*`) | 38,146 | 87.6% | b_p3, b_p4, b_p5 |
| Character bonds (`B = AA.AA.*` or `AA.AB.*`) | 5,151 | 11.8% | b_p2, b_p3, b_p4, b_p5 |
| Marker bonds (`B = AA.AE.*`) | 238 | 0.5% | b_p3, b_p4 |

### Starter distribution

- **7,430** unique starters (unique token_A values per document)
- Top starter: "the" (`AB.AB.CD.AH.xN`) with 1,723 distinct pairings
- "and", "of", "," each have 500-1,200 distinct pairings
- 75% of bond count (fbr) is 1 (pair seen once). Long tail.

---

## 3. Schema Design — Three-Level Prefix Tree

```
pbm_documents (1 per PBM)
└── pbm_starters (1 per unique token_A per document)
    ├── pbm_word_bonds    (B = AB.AB.* — 87.6% of bonds)
    ├── pbm_char_bonds    (B = AA.{AA,AB}.* — 11.8%)
    └── pbm_marker_bonds  (B = AA.AE.* — 0.5%)
```

**Level 0 — Document:** doc_id stored once. Metadata, provenance, first_fpb seed.

**Level 1 — Starters (hubs):** One row per unique token_A per document. Full decomposed token_A. Integer PK used as compact FK by all bond subtables. Eliminates doc_id and token_A repetition from every bond row.

**Level 2 — Bond subtables (leaves):** Three tables split by token_B's namespace. Each stores only the **distinguishing segments** of token_B — the implicit prefix is encoded in which table the row lives in.

### Why three subtables, not one

| Table | Implicit B prefix | Stored B segments | Columns saved per row |
|-------|-------------------|-------------------|-----------------------|
| `pbm_word_bonds` | `AB.AB` | p3, p4, p5 | 2 (ns, p2 omitted) |
| `pbm_char_bonds` | `AA` | p2, p3, p4, p5 | 1 (ns omitted) |
| `pbm_marker_bonds` | `AA.AE` | p3, p4 | 3 (ns, p2, p5 omitted) |

The split is by B-side namespace. A-side type doesn't matter — starters from any namespace can have bonds in any subtable.

---

## 4. Table Definitions

### 4.1 pbm_documents — Document Registry

One row per PBM document. Anchor for all other tables.

```sql
CREATE TABLE pbm_documents (
    id          SERIAL PRIMARY KEY,

    -- Document token address (decomposed)
    ns          TEXT NOT NULL,
    p2          TEXT,
    p3          TEXT,
    p4          TEXT,
    p5          TEXT,
    doc_id      TEXT NOT NULL GENERATED ALWAYS AS (
                    ns || COALESCE('.' || p2, '')
                       || COALESCE('.' || p3, '')
                       || COALESCE('.' || p4, '')
                       || COALESCE('.' || p5, '')
                ) STORED,

    name        TEXT NOT NULL,                     -- Human-readable title
    category    TEXT,                               -- 'book', 'article', etc.
    subcategory TEXT,                               -- 'novel', 'reference', etc.

    -- Crystallization seed: the first forward pair bond
    -- Stored as token_ids (not decomposed) for simplicity — only 1 per document
    first_fpb_a TEXT,
    first_fpb_b TEXT,

    metadata    JSONB NOT NULL DEFAULT '{}'::jsonb,

    CONSTRAINT pbm_documents_doc_id_unique UNIQUE (doc_id)
);

CREATE INDEX idx_pbm_doc_ns ON pbm_documents (ns, p2, p3, p4, p5);
CREATE INDEX idx_pbm_doc_name ON pbm_documents (name);
```

### 4.2 pbm_starters — Hub Nodes

One row per unique (document, token_A) combination. The integer `id` is the compact FK used by all bond tables.

```sql
CREATE TABLE pbm_starters (
    id          SERIAL PRIMARY KEY,
    doc_id      INTEGER NOT NULL REFERENCES pbm_documents(id),

    -- Full decomposed token_A
    a_ns        TEXT NOT NULL,
    a_p2        TEXT,
    a_p3        TEXT,
    a_p4        TEXT,
    a_p5        TEXT,
    -- Generated full token_A for cross-shard lookups
    token_a_id  TEXT NOT NULL GENERATED ALWAYS AS (
                    a_ns || COALESCE('.' || a_p2, '')
                         || COALESCE('.' || a_p3, '')
                         || COALESCE('.' || a_p4, '')
                         || COALESCE('.' || a_p5, '')
                ) STORED,

    CONSTRAINT pbm_starters_unique UNIQUE (doc_id, a_ns, a_p2, a_p3, a_p4, a_p5)
);

-- Load all starters for a document
CREATE INDEX idx_starters_doc ON pbm_starters (doc_id);
-- Find all documents containing a specific starter token
CREATE INDEX idx_starters_token ON pbm_starters (a_ns, a_p2, a_p3, a_p4, a_p5);
```

### 4.3 pbm_word_bonds — Word Pairings (87.6% of all bonds)

Token B is an English word (`AB.AB.*`). Only stores the distinguishing segments: p3, p4, p5.

```sql
CREATE TABLE pbm_word_bonds (
    starter_id  INTEGER NOT NULL REFERENCES pbm_starters(id),
    -- Distinguishing segments of token_B (implicit prefix: AB.AB)
    b_p3        TEXT NOT NULL,
    b_p4        TEXT NOT NULL,
    b_p5        TEXT,
    count       INTEGER NOT NULL,

    PRIMARY KEY (starter_id, b_p3, b_p4, b_p5)
);

-- Reverse lookup: find all starters bonded to a specific word
CREATE INDEX idx_word_bonds_b ON pbm_word_bonds (b_p3, b_p4, b_p5);
```

**Reconstructing full token_B:** `'AB.AB.' || b_p3 || '.' || b_p4 || COALESCE('.' || b_p5, '')`

### 4.4 pbm_char_bonds — Punctuation/Character Pairings (11.8%)

Token B is an ASCII byte code (`AA.AA.AA.AA.*`) or Unicode character (`AA.AB.AA.*`). Both share the `AA` namespace prefix.

```sql
CREATE TABLE pbm_char_bonds (
    starter_id  INTEGER NOT NULL REFERENCES pbm_starters(id),
    -- Distinguishing segments of token_B (implicit prefix: AA)
    b_p2        TEXT NOT NULL,
    b_p3        TEXT NOT NULL,
    b_p4        TEXT NOT NULL,
    b_p5        TEXT,
    count       INTEGER NOT NULL,

    PRIMARY KEY (starter_id, b_p2, b_p3, b_p4, b_p5)
);

CREATE INDEX idx_char_bonds_b ON pbm_char_bonds (b_p2, b_p3, b_p4, b_p5);
```

**Reconstructing full token_B:** `'AA.' || b_p2 || '.' || b_p3 || '.' || b_p4 || COALESCE('.' || b_p5, '')`

**Note:** For ASCII punctuation, b_p2=AA, b_p3=AA, b_p4=AA are repetitive but only ~5,000 rows per novel. The B-tree PK prefix-compresses these identical values automatically. In dumps, the repetition is minimal given the low row count.

### 4.5 pbm_marker_bonds — Structural Marker Pairings (0.5%)

Token B is a PBM marker (`AA.AE.*`). Only stores p3, p4 (markers have no p5).

```sql
CREATE TABLE pbm_marker_bonds (
    starter_id  INTEGER NOT NULL REFERENCES pbm_starters(id),
    -- Distinguishing segments of token_B (implicit prefix: AA.AE)
    b_p3        TEXT NOT NULL,
    b_p4        TEXT NOT NULL,
    count       INTEGER NOT NULL,

    PRIMARY KEY (starter_id, b_p3, b_p4)
);

CREATE INDEX idx_marker_bonds_b ON pbm_marker_bonds (b_p3, b_p4);
```

**Reconstructing full token_B:** `'AA.AE.' || b_p3 || '.' || b_p4`

### 4.6 Supporting Tables (carried from 007d, unchanged)

These tables from the original 007d design are correct and should be retained:

| Table | Purpose | Changes from 007d |
|-------|---------|-------------------|
| `document_provenance` | Source, copyright, rights | FK → `pbm_documents(id)` instead of decomposed doc ref |
| `document_relationships` | PBM-to-PBM links (sub-PBMs) | FK → `pbm_documents(id)` |
| `non_text_content` | Binary blobs (images, etc.) | No change |
| `tab_definitions` | Indent level mappings per doc | FK → `pbm_documents(id)` |
| `tbd_log` | Unresolved reference tracking | FK → `pbm_documents(id)` |

**Dropped from 007d:**
- `pbm_content` — positional content stream (the wrong format)
- `position_metadata` — per-position key-value (no positions in bond format)

---

## 5. Read/Write Patterns

### Write (after encoding a document)

```
1. INSERT into pbm_documents → get doc_pk (integer)
2. For each unique token_A in the bond set:
   INSERT into pbm_starters (doc_id=doc_pk, a_ns, a_p2, ...) → get starter_pk
3. For each bond (token_A, token_B, count):
   a. Look up starter_pk from step 2
   b. Determine B's namespace prefix
   c. INSERT into the appropriate bond subtable with reduced segments
```

Bulk insert friendly: steps 2 and 3 can use executemany/COPY.

### Read (load a document's PBM for OpenMM)

```
1. SELECT * FROM pbm_starters WHERE doc_id = ?
   → dict of {starter_pk: full_token_A}

2. Three queries, one per bond subtable:
   SELECT starter_id, b_p3, b_p4, b_p5, count FROM pbm_word_bonds
     WHERE starter_id IN (SELECT id FROM pbm_starters WHERE doc_id = ?)
   -- Reconstruct: (token_A_from_starter, 'AB.AB.' || b_p3 || ... , count)

   SELECT starter_id, b_p2, b_p3, b_p4, b_p5, count FROM pbm_char_bonds WHERE ...
   -- Reconstruct: (token_A_from_starter, 'AA.' || b_p2 || ... , count)

   SELECT starter_id, b_p3, b_p4, count FROM pbm_marker_bonds WHERE ...
   -- Reconstruct: (token_A_from_starter, 'AA.AE.' || b_p3 || ... , count)

3. Merge results → flat list of (token_A, token_B, count)
4. Feed to OpenMM as bond definitions with k=count
```

### Cross-document queries

Find all documents containing a specific bond:
```sql
SELECT d.doc_id, d.name, wb.count
FROM pbm_word_bonds wb
JOIN pbm_starters s ON s.id = wb.starter_id
JOIN pbm_documents d ON d.id = s.doc_id
WHERE s.a_ns = 'AB' AND s.a_p2 = 'AB' AND s.a_p3 = 'CD' AND s.a_p4 = 'AH' AND s.a_p5 = 'xN'  -- "the"
  AND wb.b_p3 = 'CA' AND wb.b_p4 = 'GK' AND wb.b_p5 = 'ih';  -- "world"
```

---

## 6. Size Estimates

### Per document (Frankenstein baseline: 78K words, 43,535 bond pairs)

| Component | Rows | Est. dump size |
|-----------|------|---------------|
| Document | 1 | negligible |
| Starters | 7,430 | 399 KB |
| Word bonds | 38,146 | 1,118 KB |
| Char bonds | 5,151 | 176 KB |
| Marker bonds | 238 | 6 KB |
| **Total (hub-spoke)** | **50,966** | **~1.7 MB** |
| Flat alternative | 43,535 | ~4.2 MB |

**Savings: 60%** — primarily from eliminating doc_id and token_A repetition, plus reduced token_B segments.

### Projected corpus sizes

| Corpus size | Hub-spoke dump | Flat dump | Savings |
|-------------|---------------|-----------|---------|
| 100 novels | ~170 MB | ~415 MB | 250 MB saved |
| 1,000 novels | ~1.7 GB | ~4.2 GB | 2.5 GB saved |

Supporting tables (provenance, relationships, etc.) add a fixed ~1 KB per document — negligible.

---

## 7. Portability Notes

The schema uses standard SQL features:
- `GENERATED ALWAYS AS ... STORED` — SQL:2003 standard (PostgreSQL 12+, SQLite 3.31+)
- `SERIAL` — PostgreSQL; replace with `INTEGER PRIMARY KEY AUTOINCREMENT` for SQLite
- All data types are TEXT and INTEGER — universally portable
- No PostgreSQL-specific features (partitioning, BRIN indexes, etc.)
- No stored procedures or triggers

Dump format: standard INSERT statements. Import order: documents → starters → bonds → supporting tables.

---

## 8. Directionality

PBM bonds are **directional** — forward bonding, standard molecular notation. "the→cat" and "cat→the" are different entries with different counts.

Analysis of Frankenstein confirms this matters: 3,936 pairs (9%) exist in both directions with very different counts. Example: "comma→and" = 953 vs "and→comma" = 24.

**The schema preserves directionality inherently.** The starter IS the A-side (from). The bond's B columns ARE the B-side (to). If both "the→cat" and "cat→the" exist, they have different starter_ids and live as separate rows. No additional mechanism needed — the hub-spoke structure IS directional by construction.

---

## 9. PostgreSQL → LMDB Assembly

### Architecture context

PostgreSQL is the **WRITE** layer. LMDB is the **READ/DRAW** layer. PostgreSQL assembles LMDB instances as pre-scoped, memory-mapped databases for active envelopes. The prefix tree optimization reduces data volume in this assembly step — that is why portable, efficient dumps matter.

### The extraction query

A single UNION ALL reconstructs full token IDs from the prefix tree:

```sql
-- Extract all bonds for a document, reconstructing full token_B
SELECT s.token_a_id,
       'AB.AB.' || wb.b_p3 || '.' || wb.b_p4
                 || COALESCE('.' || wb.b_p5, '') AS token_b_id,
       wb.count
FROM pbm_word_bonds wb
JOIN pbm_starters s ON s.id = wb.starter_id
WHERE s.doc_id = :doc_pk

UNION ALL

SELECT s.token_a_id,
       'AA.' || cb.b_p2 || '.' || cb.b_p3 || '.' || cb.b_p4
             || COALESCE('.' || cb.b_p5, '') AS token_b_id,
       cb.count
FROM pbm_char_bonds cb
JOIN pbm_starters s ON s.id = cb.starter_id
WHERE s.doc_id = :doc_pk

UNION ALL

SELECT s.token_a_id,
       'AA.AE.' || mb.b_p3 || '.' || mb.b_p4 AS token_b_id,
       mb.count
FROM pbm_marker_bonds mb
JOIN pbm_starters s ON s.id = mb.starter_id
WHERE s.doc_id = :doc_pk;
```

Output: flat `(token_A, token_B, count)` triples. Prefix reconstruction is cheap string concatenation, done in SQL — the application gets clean, fully-qualified triples.

### LMDB key format options

| Option | Key format | Key size | Value | Total/bond | Notes |
|--------|-----------|----------|-------|-----------|-------|
| A. String keys | `"token_A\0token_B"` | ~29 bytes | uint32 | ~33 bytes | Readable, debuggable |
| B. Packed segments | `[a_ns..a_p5][b_ns..b_p5]` | 20 bytes fixed | uint32 | 24 bytes | Fixed-width, fast comparison |
| C. Integer vocab | `uint32(a_idx) + uint32(b_idx)` | 8 bytes fixed | uint32 | 12 bytes | Most compact, aligns with OpenMM |

**Recommendation: Option C (integer vocabulary).** OpenMM works with integer particle indices internally. The assembly step naturally enumerates unique tokens → integer indices. The vocabulary mapping is not overhead — it's inherent to how molecular engines work.

```
LMDB sub-databases:
  atoms:   uint32(index) → token_id_string    (vocabulary)
  bonds:   uint64(a_idx << 32 | b_idx) → uint32(count)
  meta:    "doc_id" → "zA.AB.CA.AA.AA"
           "first_fpb_a" → uint32(idx_a)
           "first_fpb_b" → uint32(idx_b)
           "token_count" → uint32(N)
           "bond_count"  → uint32(M)
```

### The assembly pipeline

```
Single document:
  PostgreSQL (prefix tree)
    → UNION ALL extraction (reconstruct token_B prefixes in SQL)
    → Stream triples to application
    → Enumerate: assign integer indices to unique tokens
    → Write LMDB: atoms db + bonds db + meta db

Envelope compilation (multiple documents):
  PostgreSQL
    → Extract bonds for each document in the envelope
    → Aggregate: Counter[(token_A, token_B)] += count  (across docs)
    → Enumerate combined vocabulary
    → Write compiled LMDB
```

### How the prefix tree helps assembly

1. **Less PostgreSQL I/O:** 60% fewer bytes read from disk during extraction
2. **Smaller wire transfer:** Less data crosses the PG → application boundary
3. **Reconstruction is trivial:** String prefix concatenation is nanoseconds, done in SQL
4. **Tree doesn't propagate to LMDB:** LMDB gets flat, integer-indexed data. The tree serves its purpose in the storage/transfer layer and is unwound during assembly

This is analogous to compressed storage: store compressed, transfer compressed, decompress at destination for fast access. The prefix tree IS the compression scheme for the PostgreSQL layer.

### LMDB size estimates

| Scope | Bonds (est.) | Vocab | LMDB size (Option C) |
|-------|-------------|-------|---------------------|
| Single novel (Frankenstein) | 43,535 | 7,431 tokens | ~641 KB |
| 100-novel envelope (compiled, ~50% overlap) | ~2.2M | ~80K tokens | ~26 MB |
| 1,000-novel envelope | ~10M | ~200K tokens | ~124 MB |

All well within the 2 GB shard target. PBM data is inherently compact — a 78,000-word novel compresses to 641 KB of bond data.

### Key insight

The prefix tree optimizes the **write/store** layer (PostgreSQL). The **read/draw** layer (LMDB) uses flat integer-indexed bonds — the most compact possible representation. The assembly step is the decompression boundary: prefix tree → flat. This is a clean separation of concerns: PostgreSQL optimizes for storage efficiency and dump portability; LMDB optimizes for read speed and memory density.

---

## 10. Future Optimizations (deferred)

**PoS sub-trees for word bonds:** Patrick mentioned "possibly sub-treed by PoS." The p3 segment in English word tokens encodes the PoS category (CA=nouns, CB=verbs, etc.). Word bonds could be further split by B's p3 into PoS-specific tables (noun_bonds, verb_bonds, etc.). Deferred until we see whether PoS-specific queries are common enough to justify the extra tables.

**Starter-side reduction:** Currently starters store full decomposition (5 segments). Since most starters are words (AB.AB.*), we could split starters by type and store reduced segments. Deferred — starter table is small relative to bonds.

**Sub-PBM compilation:** Bond counts from child PBMs could be pre-aggregated into parent PBM rows. This is an engine concern, not a schema concern — document_relationships already tracks the parent-child links.

---

## 11. Open Questions for Review

1. **Database naming:** Should we recreate `hcp_en_pbm` for non-fiction (zA) and use `hcp_fic_pbm` for fiction (vA)? Or a single database covering both namespaces?

2. **first_fpb storage:** Currently proposed as two TEXT columns on pbm_documents. Should this be decomposed, or is TEXT sufficient since it's 1 pair per document?

3. **Supporting table FKs:** The 007d supporting tables used decomposed doc references (doc_ns, doc_p2, ...). This design proposes switching to integer FK → pbm_documents(id). Simpler JOINs, smaller dumps. Any objection?

4. **Coordination with PBM specialist:** The read/write patterns in section 5 need review by whoever owns the Python DB layer. The current `pbm.py` uses the old flat format and will need updating.
