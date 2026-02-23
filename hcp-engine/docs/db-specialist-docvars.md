# Document-Local Vars (Decimal Tokens) — DB Specialist Notes for Engine

**Date:** 2026-02-23
**From:** DB specialist
**Migration:** `db/migrations/012_pbm_docvars.sql` (target: hcp_fic_pbm)

## What This Is

During PBM generation, the kernel will encounter unrecognized text — things the discriminator can't resolve to a known token. Instead of routing these through the var DB (hcp_var), the kernel assigns a **document-local var** directly in PBM metadata.

These are **full token entries** scoped to one document: they have an ID, a surface form, and an optional reviewer gloss.

## Decimal Pair Notation

All var IDs use **decimal pairs**, not hex:
- `01.03`, `42.07`, `00.00`
- Visually distinct from hex token IDs (`AA.AB.AC`)
- Two-pair format: `00.00` through `99.99` = 10,000 slots per document

### Zero-Padding for Engine Token Width

The DB stores the canonical short form (`01.03`). The engine pads with leading `00` pairs to match its working token width:
- 5-pair pipeline: `01.03` → `00.00.00.01.03`
- The value is the same, just left-padded. This is a rendering concern at read time, not a storage concern.

## Schema

```sql
pbm_docvars (
    doc_id      INTEGER NOT NULL REFERENCES pbm_documents(id),
    var_id      TEXT NOT NULL,           -- decimal pair, e.g. '01.03'
    surface     TEXT NOT NULL,           -- original text
    gloss       TEXT,                    -- reviewer note

    PRIMARY KEY (doc_id, var_id),
    UNIQUE (doc_id, surface)            -- dedup key
)
```

## Kernel Integration

### During PBM Generation

1. Kernel encounters unrecognized text (or discriminator flags it as var)
2. **Dedup check**: query `pbm_docvars` for `(doc_id, surface)` — if found, reuse existing `var_id`
3. **Mint new**: assign next consecutive decimal ID (00.00, 00.01, 00.02, ...)
4. The decimal var_id appears in bond data like any other token — `(token_A, var_id, count)` or `(var_id, token_B, count)`

There's a Postgres function `mint_docvar(doc_id, surface)` that handles the dedup-or-mint atomically, but the kernel can also manage its own counter in memory and batch-insert at the end — whichever fits the pipeline better.

### No Var DB Interaction

PBM generation does **not** touch the var DB (hcp_var). Document-local vars are written directly to `pbm_docvars`. The var DB remains the engine's runtime scratch pad for live inference.

### After Review

Two outcomes per var:
1. **Resolved** — reviewer identifies it as a real word, OCR artifact, etc. The var entry gets deleted from `pbm_docvars` and bond data is patched to use the real token_id. Gaps in the decimal sequence are fine.
2. **Confirmed doc-specific** — edition numbers, URLs, formatting quirks that don't need a concept token. These stay permanently in `pbm_docvars`. The reviewer can annotate via the `gloss` field.

### Bond Storage for Decimal Vars

Decimal vars appear in the existing PBM bond tables. The A-side or B-side of a bond can be a decimal var_id. Since decimal notation is visually and structurally distinct from hex token IDs, the bond tables don't need any schema changes — the var_id is just a TEXT value like any other token.

Decimal vars route to a dedicated **`pbm_var_bonds`** subtable:

```sql
pbm_var_bonds (
    starter_id  INTEGER NOT NULL REFERENCES pbm_starters(id),
    b_var_id    TEXT NOT NULL,       -- full decimal var_id, e.g. '01.03'
    count       INTEGER NOT NULL,

    PRIMARY KEY (starter_id, b_var_id)
)
```

No prefix stripping — decimal vars have no implicit namespace prefix, so the full `XX.YY` is stored as-is. The UNION ALL query for reconstructing full bonds just passes `b_var_id` through directly.

## Future: Cross-Document Var Loading

Planned but not built yet: loading specific decimal vars from other PBM sets for cross-document reference on volatile elements. The `idx_docvars_surface` index supports reverse lookup by surface form across documents.

## Summary for Quick Reference

| Property | Value |
|----------|-------|
| Table | `pbm_docvars` in hcp_fic_pbm |
| ID format | Decimal pairs: `XX.YY` (00.00–99.99) |
| Scope | Per-document |
| Dedup | On `(doc_id, surface)` |
| Assignment | Consecutive, gaps OK |
| Engine padding | Leading `00` pairs to match token width |
| Var DB interaction | None during PBM generation |
| Mint function | `mint_docvar(doc_id, surface) → var_id` |
