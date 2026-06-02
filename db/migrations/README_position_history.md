# Position Storage History

## Timeline

1. **Migration 009** — Original positional storage (4 subtables: word, char, marker, var). PBM bonds derived at inference time, not stored.
2. **Migration 011** — Dropped 009's position tables, replaced with PBM bond prefix tree (starters + bond subtables). Bonds stored, positions gone.
3. **Discovery** — Euler path through bond graph isn't unique for reconstruction (common tokens like "the" have too many outgoing edges). Need both encodings.
4. **Migration 013 (first draft)** — Re-added 4 separate position subtables alongside bonds. Never applied to DB.
5. **Migration 013 (current)** — Positions stored as a column on `pbm_starters` instead. The starter table already is the unique token inventory per document — no need for separate tables.
6. **Migration 041** — Split positional modifiers into `pbm_morpheme_positions` (sparse morpheme/cap overlay lists). **SUPERSEDED by 049.**
7. **Migration 048** — Attempted row-per-position normalization (`pbm_positions` table). Postgres tuple overhead blew up database size (39 MB -> 101 MB on 9 docs). **SUPERSEDED by 049.**
8. **Migration 049 (current)** — Positions as `INTEGER[]` on `pbm_starters` (one row per doc x token). `all_caps_positions INTEGER[]` on `pbm_documents`. Drops tables from 041 and 048. Requires re-ingest after applying.

## Current state (as of 049)

- **Positions**: `pbm_starters.positions INTEGER[]` — one row per (doc, token), compact array storage (~4 bytes/element).
- **Cap storage**: `pbm_documents.all_caps_positions INTEGER[]` — only ALL_CAPS stored. FIRST_CAP is positional (derivable from punctuation context). Label tokens carry intrinsic cap in `entries.word`.
- **No morph-bit storage**: every form is its own token. No `pbm_morpheme_positions` table.
- **Bonds**: `pbm_starters` + `pbm_word_bonds` / `pbm_char_bonds` / `pbm_marker_bonds` / `pbm_var_bonds` — inference and aggregation.
- **Document stats**: `pbm_documents.total_slots` / `pbm_documents.unique_tokens`.

Both encodings (bonds + positions) written during ingestion. Positions may become unnecessary once the conceptual mesh provides graph solver constraints.
