# Var DB Schema Design

**Status:** Design (pending review)
**Date:** 2026-02-18
**Author:** DB Specialist

## Summary

The var DB is the DI's general-purpose short-term memory cache. Any
unresolved sequence, temporary datum, or staging content routes here.
Var tokens are real tokens with real IDs that work in the position
stream like any other token. The DI organizes the internal namespace
later — for now, `var.` prefix with arbitrary suffix.

## Design Principles

1. **Flexibility over structure** — this is the one place where rigid
   conventions actively hurt. Minimal constraints, maximum adaptability.
2. **Real tokens** — var tokens participate in position streams, LMDB
   lookups, and bond inference identically to permanent tokens.
3. **Separate database** — `hcp_var` as its own PostgreSQL database.
   Var data is fundamentally different from vocabulary: high churn,
   no decomposed hierarchy, DI-managed lifecycle.
4. **Atomization lives here** — the engine doesn't break down unknown
   sequences. It sends the raw chunk; var DB owns the analysis.

## Token ID Format

```
var.<arbitrary>
```

- Prefix `var.` is fixed — instantly distinguishable from standard
  namespaces (AA, AB, zA, etc.) which use 2-char prefixes.
- Suffix is unconstrained. The DI can use any scheme: sequential
  counters, content hashes, descriptive labels, whatever fits.
- NOT decomposed — no ns/p2/p3/p4/p5 columns. Single TEXT field.
- The DI can organize a namespace structure later when it's ready.

## Schema

### Core Table: `var_tokens`

```sql
CREATE TABLE var_tokens (
    -- Primary identifier
    var_id      TEXT PRIMARY KEY,       -- 'var.<arbitrary>'

    -- What this var token represents
    form        TEXT NOT NULL,          -- Original surface text ('supercalifragilistic')
    atomization JSONB,                  -- Broken-down representation (DI's analysis)

    -- Lifecycle
    status      TEXT NOT NULL DEFAULT 'active',
                                       -- 'active', 'promoted', 'retired', 'merged'
    promoted_to TEXT,                   -- Permanent token_id if promoted

    -- Metadata — intentionally loose
    metadata    JSONB NOT NULL DEFAULT '{}',

    -- Timestamps
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at  TIMESTAMPTZ NOT NULL DEFAULT now()
);

-- Surface text lookup (same chunk → same var token)
CREATE UNIQUE INDEX idx_var_form ON var_tokens (form) WHERE status = 'active';

-- Status filtering
CREATE INDEX idx_var_status ON var_tokens (status);

-- Recency
CREATE INDEX idx_var_created ON var_tokens (created_at);
```

**Key decisions:**

- **Unique on form (active only):** Same raw chunk from different
  documents gets the same var token. If "Llanfairpwllgwyngyll" appears
  in two documents, one var token, two position entries. The partial
  unique index lets retired/promoted forms be re-created if needed.
- **Atomization as JSONB:** The var DB owns analysis. Format is
  whatever the DI produces — character breakdown, partial word matches,
  language guess, etc. Unconstrained by design.
- **Metadata as JSONB:** Anything the DI wants to attach. Source hints,
  confidence scores, frequency counts, candidate resolutions. No fixed
  schema for this — it's the DI's scratch space.
- **No decomposed references:** var_id is plain TEXT. No ns/p2/p3/p4/p5.
  Foreign keys to var tokens use the var_id string directly.

### Provenance Table: `var_sources`

Track where var tokens are used — this is the **update index** for
the librarian. When a var token is promoted, var_sources gives the
exact doc_id + position pairs to patch in the position streams.
No searching required — direct lookup.

```sql
CREATE TABLE var_sources (
    id          SERIAL PRIMARY KEY,
    var_id      TEXT NOT NULL REFERENCES var_tokens(var_id),

    -- Where this was encountered (update index for librarian)
    doc_id      TEXT NOT NULL,          -- Document token_id (from pbm_documents)
    position    INTEGER NOT NULL,       -- Position in document stream
    context     TEXT,                   -- Surrounding text for disambiguation

    -- When
    seen_at     TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX idx_varsrc_var ON var_sources (var_id);
CREATE INDEX idx_varsrc_doc ON var_sources (doc_id);
```

**Why separate from var_tokens:** A var token can be encountered many
times across many documents. The sources table captures each occurrence
without bloating the core token record. Critically, this is the
**direct update path** — when the librarian promotes a var token, it
queries `var_sources WHERE var_id = X` to get every (doc_id, position)
pair that needs patching. No full-table scans of position streams.

## Pipeline Contract

### Creation (cache miss with `<var>` header)

1. Engine sends raw chunk + doc_id + position to Postgres with `<var>` header
2. Postgres checks `var_tokens` for **existing active token** with matching form
3. **Existing var hit:** Return existing var_id, log source (doc_id, position)
4. **No existing var:** Mint new var_token, log source, return var_id
5. Write `chunk → var_id` to LMDB
6. Engine retries LMDB → hit

**Important:** Step 2 catches var tokens whose LMDB entries were evicted.
The var token still exists in Postgres — don't mint a duplicate. The
unique index on `form WHERE status = 'active'` enforces this.

```
Engine → LMDB miss → Postgres (<var> header, chunk, doc_id, position)
                      ├─ SELECT var_id FROM var_tokens WHERE form = chunk AND status = 'active'
                      ├─ If found: use existing var_id
                      ├─ If not: mint_var_token(chunk) → new var_id
                      ├─ INSERT INTO var_sources (var_id, doc_id, position)
                      ├─ Write to LMDB (var_id + empty continuation index)
                      └─ Return var_id
Engine ← LMDB retry → hit
```

### ID Minting

For now, simple sequential: `var.1`, `var.2`, etc. using a PostgreSQL
sequence. The DI can adopt a richer scheme later without changing the
pipeline — the suffix is opaque to everything outside the var DB.

```sql
CREATE SEQUENCE var_id_seq;

-- Minting function
CREATE OR REPLACE FUNCTION mint_var_token(p_form TEXT)
RETURNS TEXT AS $$
DECLARE
    v_id TEXT;
BEGIN
    v_id := 'var.' || nextval('var_id_seq')::TEXT;
    INSERT INTO var_tokens (var_id, form)
    VALUES (v_id, p_form);
    RETURN v_id;
END;
$$ LANGUAGE plpgsql;
```

### Promotion (Librarian Workflow)

When the librarian resolves what a var token actually is:

1. Create the permanent token in the appropriate vocabulary shard
   (e.g., hcp_english for a newly recognized English word)
2. Update `var_tokens`: set `status = 'promoted'`, `promoted_to = <permanent_id>`
3. **Rewrite position streams** — query `var_sources WHERE var_id = X`
   to get every (doc_id, position) pair. Update the token_id at each
   position in the document shard's position tables.
4. Update LMDB: `form → permanent_id` replaces `form → var_id`
5. Optionally clean up var_sources rows for the promoted token

**var_sources makes rewriting cheap:** No scanning position tables.
Direct lookup gives exact locations. A var token used in 47 positions
across 12 documents = 47 targeted UPDATEs, not a full-table scan.

The librarian handles this in batch — review active var tokens sorted
by frequency, promote the real words, retire the junk.

### Retirement

Var tokens that are no longer needed (junk, one-off encoding errors):

1. Update `var_tokens`: set `status = 'retired'`
2. The partial unique index on `form` releases the form text for re-use
3. LMDB entries can be evicted normally

### Merging

Two var tokens discovered to represent the same thing:

1. Pick the canonical var_id (lower number, or DI's choice)
2. Set loser to `status = 'merged'`, `promoted_to = <winner var_id>`
3. Same no-rewrite principle as promotion

## Size Estimates

Var tokens should be a small fraction of total vocabulary:

- Typical novel: ~200-500 unknown sequences (names, foreign words,
  typos, unusual compounds)
- 100 novels: ~5,000-15,000 unique var tokens (many shared across docs)
- Each var_token row: ~200-500 bytes (form + atomization + metadata)
- 15,000 var tokens: ~3-7 MB — trivial

The var DB stays small because most sequences eventually promote to
permanent vocabulary or get retired.

## Interaction with tbd_log (Migration 007d)

The `tbd_log` table in 007d tracks unresolved references per-document
per-position. The var DB supersedes this for the tokenizer pipeline —
var tokens ARE the resolution (temporary but real). The `tbd_log` may
still serve a role for tracking non-token unresolved references (entity
links, semantic annotations, etc.), but for tokenization, var DB is
the path.

## Open Questions

1. **LMDB continuation index for var tokens** — always empty on
   creation, but can a var token later acquire continuations? (e.g.,
   var token for "Dr" + continuation for "Dr. Smith")
2. **Batch resolution tooling** — DI needs a way to query "show me all
   active var tokens sorted by frequency" for bulk promotion. Simple
   SQL query, but might want a convenience view or function.
3. **Cross-shard var references** — var tokens live in hcp_var, but
   position streams live in document shards (hcp_fic_pbm, etc.).
   Cross-database FK not possible in PostgreSQL — enforced at
   application layer.
