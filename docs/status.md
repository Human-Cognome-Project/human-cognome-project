# Project Status

_Last updated: 2026-02-05_

## What Exists

### Governance (stable)
- **Covenant** — Founder's Covenant of Perpetual Openness. Ratified.
- **Charter** — Contributor's Charter. Ratified.
- **License** — AGPL-3.0, governed by the Covenant.

### Specifications (first draft)
- **Data conventions** — core definitions, PBM storage algorithm, atomization, NSM decomposition
- **Token addressing** — base-50 scheme, 5-pair dotted notation, reserved namespaces (v-z for entities/names/PBMs)
- **Pair-bond maps** — FPB/FBR structure, reconstruction, compression, error correction
- **Architecture** — two-engine model, conceptual forces, LoD stacking, fluid dynamics framing
- **Identity structures** — personality DB (seed + living layer), relationship DB, integration with core data model

These are first-pass specs derived from working notes. They need review, critique, and refinement.

### Database Shards (Phase 1 complete)

**Core shard (`hcp_core`)** — 2.3 MB
- 2,450 AA-namespace tokens: byte codes, Unicode characters, structural markers
- Namespace allocations table
- Foundation for all other shards

**English shard (`hcp_english`)** — 685 MB
- 1,252,854 tokens across all layers:
  - Words: 1,146,520 (noun, verb, adj, adv, etc.)
  - Affixes: 3,696 (prefix, suffix, infix, etc.)
  - Derivatives: 93,514 (abbreviations, contractions, forms)
  - Multi-word: 9,084 (phrases, proverbs)
- Kaikki dictionary data: 1.3M entries, 1.5M senses, 870K forms, 450K relations
- All tokens atomized to character Token IDs
- Cross-shard references to y* name components

**Names shard (`hcp_names`)** — 58 MB
- 150,528 name component tokens (yA.* addressing)
- Single-word proper nouns, name parts, labels
- 143,873 entries with senses/forms/relations
- Cross-linguistic: shared across all language shards

### Python Implementation (`src/hcp/`)
- `core/` — Base-50 Token ID encoding, namespace constants
- `db/` — PostgreSQL connectors for core, english, names shards
- `ingest/` — Word and name component ingestion from Kaikki

### Legacy prototype (`work/hcp/`)
- Earlier exploration code, mostly superseded by `src/`
- May contain useful reference implementations

## What Doesn't Exist Yet

- PBM construction from real text (next phase)
- LMDB compiled inference layer
- Entity classification (v*/w*/x* shards for People/Places/Things)
- NSM primitive mappings beyond stub definitions
- Multi-modality support (only text addressed)
- Physics engine integration for interpretation
- Community infrastructure (issue templates, CI, discussion forums)

## Data Sources

- **Kaikki** — Wiktionary extracts used for initial English vocabulary. Open data, gratefully acknowledged.
