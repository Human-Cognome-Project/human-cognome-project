# Position Storage History

## Timeline

1. **Migration 009** — Original positional storage (4 subtables: word, char, marker, var). PBM bonds derived at inference time, not stored.
2. **Migration 011** — Dropped 009's position tables, replaced with PBM bond prefix tree (starters + bond subtables). Bonds stored, positions gone.
3. **Discovery** — Euler path through bond graph isn't unique for reconstruction (common tokens like "the" have too many outgoing edges). Need both encodings.
4. **Migration 013 (first draft)** — Re-added 4 separate position subtables alongside bonds. Never applied to DB.
5. **Migration 013 (current)** — Positions stored as a column on `pbm_starters` instead. The starter table already is the unique token inventory per document — no need for separate tables.

## Current state

No positional data exists in the database. No documents have been ingested since migration 011 dropped the 009 tables. Migration 013 adds the `positions` column to starters but it's NULL until the engine writes position data during ingestion.

## What lives where

- **Bonds**: `pbm_starters` + `pbm_word_bonds` / `pbm_char_bonds` / `pbm_marker_bonds` / `pbm_var_bonds` — inference and aggregation
- **Positions**: `pbm_starters.positions` (base-50 packed) — exact reconstruction
- **Document stats**: `pbm_documents.total_slots` / `pbm_documents.unique_tokens`

Both encodings written during ingestion. Positions may become unnecessary once the conceptual mesh provides graph solver constraints.
