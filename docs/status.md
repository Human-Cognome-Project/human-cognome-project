# Project Status

_Last updated: 2026-03-10_

## What Exists

### Governance (stable)
- **Covenant** -- Founder's Covenant of Perpetual Openness. Ratified.
- **Charter** -- Contributor's Charter. Ratified.
- **License** -- AGPL-3.0, governed by the Covenant.

### Specifications (first draft)
- **Data conventions** -- core definitions, PBM storage algorithm, atomization, NSM decomposition
- **Token addressing** -- base-50 scheme, 5-pair dotted notation, reserved namespaces (v-z for entities/names/PBMs)
- **Pair-bond maps** -- FPB/FBR structure, reconstruction, compression, error correction
- **Architecture** -- two-engine model, conceptual forces, LoD stacking
- **Identity structures** -- personality DB (seed + living layer), relationship DB, integration with core data model

These are first-pass specs derived from working notes. They need review, critique, and refinement.

### Engine: O3DE + PhysX 5 PBD Superposition Pipeline

The primary inference engine is an O3DE 25.10.2 C++ Gem using PhysX 5 GPU-accelerated Position Based Dynamics for tokenization and resolution. ~21,300 lines of C++ across 35 source modules.

**Headless daemon** (`HCPEngine.HeadlessServerLauncher`) listens on port 9720 with JSON socket API for health, ingest, retrieve, list, tokenize, and physics-based ingestion.

**Pipeline:**
```
DecodeUtf8ToCodepoints
  -> Phase 1: byte->char PBD superposition (16K codepoints/chunk, Unicode-aware)
  -> ExtractRunsFromCollapses (with capitalization suppression)
  -> Phase 2: char->word via persistent VocabBeds (BedManager, 5 GPU workspaces: 3 primary + 2 extended, triple-pipelined via RunPipelinedCascade)
    -> TryInflectionStrip (PBD is the existence check, silent-e fallback)
    -> TryVariantNormalize (V-1 g-drop: -in'→-ing; V-3 archaic: -eth→base; runs after inflection strip)
    -> AnnotateManifest (multi-word entity recognition)
    -> ScanManifestToPBM
    -> StorePBM + StorePositions
```

**Benchmarks (2026-03-04, GTX 1070 headless, pipelined):**

| Text | Size | Tokens | Vars | Wall Time | Previous (pre-pipeline) |
|------|------|--------|------|-----------|--------------------------|
| Dracula | 890 KB | 199,368 | 110 | **28.6s** | 166.5s (2026-03-02) |
| A Study in Scarlet | 269 KB | 56,061 | 54 | **12.2s** | 133.0s (2026-03-02) |
| The Yellow Wallpaper | 47 KB | 10,856 | 37 | — | — |
| The Sign of Four | — | — | 52 | — | — |

V-1 variant normalization (2026-03-04): 12/12 dialect g-drops resolved in Sign of Four. Resolution rates >97% on test corpus.

**Key engine modules** (`hcp-engine/Gem/Source/`):

| Module | Purpose |
|--------|---------|
| `HCPVocabBed.h/cpp` | VocabBed (persistent PBD per word length) + BedManager (orchestrator) |
| `HCPVocabulary.h/cpp` | LMDB reader + affix loader. 48 max DBIs. |
| `HCPSuperpositionTrial.h/cpp` | Phase 1 byte->char PBD |
| `HCPWordSuperpositionTrial.h/cpp` | CharRun extraction with cap suppression |
| `HCPEntityAnnotator.h/cpp` | Multi-word entity recognition from LMDB |
| `HCPEngineSystemComponent.h/cpp` | Top-level orchestrator. Owns BedManager + all DB kernels. |
| `HCPSocketServer.h/cpp` | TCP socket API on port 9720 |
| `HCPParticlePipeline.h/cpp` | PBD particle system: Phase 1 + Phase 2 scenes |
| `HCPEnvelopeManager.h/cpp` | Activity envelope cache lifecycle (LMDB hot cache) |
| `HCPCacheMissResolver.h/cpp` | LMDB cache miss -> Postgres fill |
| `HCPBondCompiler.h/cpp` | Sub-word PBM bond tables (char->word, byte->char) |
| `HCPTokenizer.h/cpp` | 7-step resolution cascade |

**Decomposed DB kernel modules** (split from monolith 2026-03-01):

| Module | Purpose |
|--------|---------|
| `HCPDbConnection.h/cpp` | Shared PGconn* wrapper |
| `HCPPbmWriter.h/cpp` | StorePBM + StorePositions |
| `HCPPbmReader.h/cpp` | LoadPBM + LoadPositions |
| `HCPDocumentQuery.h/cpp` | Document listing, detail, provenance, metadata |
| `HCPDocVarQuery.h/cpp` | Document-local var queries |
| `HCPBondQuery.h/cpp` | Bond queries for tokens/starters |
| `HCPDbUtils.h` | Base-50 encode/decode, token helpers |

### Source Workstation

Standalone Qt binary (`HCPWorkstation`, 14 MB) with dual-mode architecture:
- **Offline mode**: Embedded DB kernels + LMDB vocab. Browse docs, edit metadata, view bonds/text without daemon.
- **Connected mode**: All offline features + physics operations via socket to daemon on port 9720.
- **DB abstraction**: `IDatabaseBackend` with Postgres and SQLite implementations.
- Files: `Source/Workstation/` (7 source files), `hcpengine_workstation_files.cmake`

### Pre-Compiled LMDB Vocab

Offline-compiled LMDB vocabulary for zero-SQL runtime:
- 809,303 entries (37% reduction from 1.28M via morphological stripping), ~34 MB on disk
- Per-word-length sub-databases (`vbed_02`..`vbed_16`), frequency-ordered
- Labels (tier 0) -> freq-ranked (tier 1) -> unranked remainder (tier 2)
- Entity sub-databases: 723 sequences total (fic + nf), verified via dry-run. DB cleaned 2026-03-05; LMDB recompile pending (compile_entity_lmdb.py needs review for variant/morph categories before next full compile).
- Frequency data: Wikipedia 2023 + OpenSubtitles merged. 176K tokens ranked.
- Scripts: `compile_vocab_lmdb.py`, `compile_entity_lmdb.py`, `merge_frequency_ranks.py`

### Database Shards (6 databases, 32 migrations applied)

**Core shard (`hcp_core`)** -- AA namespace
- ~5,200 tokens: byte codes, Unicode characters, structural markers, NSM primitives
- Namespace allocations, shard registry (10 entries)
- Activity envelope schema (definitions, queries, composition, audit log)
- Punctuation and single-character tokens live here, not in hcp_english
- LMDB sub-db split: `env_vocab` (envelope-loaded, evicted on switch) vs `w2t` (cache-miss fills, persistent)

**English shard (`hcp_english`)** -- AB/AD/AE namespaces
- **Kaikki rebuild COMPLETE** (2026-03-10, 6 passes from enwiktionary 2026-03-03 dump)
- **Namespace layout**: AB = common vocab (root tokens), AD = Labels (N_PROPER), AE = Initialisms, AF = reserved for idioms (deferred)
- **DB totals**: ~1,421,622 tokens, 502,295 token_pos, 579,388 morph_rules, 501,696 glosses, 45,023 variants
- **Pass results**:
  - Pass 1: 21,019 root tokens (AB namespace)
  - Pass 2: 496,364 token_pos entries + 601,457 token_morph_rules
  - Pass 3: 495,765 glosses
  - Pass 4: 29,289 irregular variants + 15,734 archaic forms
  - Pass 5: 26,926 loanword tags
  - Pass 6: 4,188 Labels (AD namespace) + 1,743 initialisms (AE namespace)
- **Known data quality issue**: V_MAIN over-assigned by Kaikki (~2K tokens that aren't primarily verbs); flagged for downstream cleanup
- **New schema tables** (migrations 029-032, roots-only design):
  - `token_pos` -- (token_id, pos, is_primary, gloss_id) junction table, `pos_tag` enum (N_COMMON, N_PROPER, V_MAIN, V_AUX, V_COPULA, ADJ, ADV, PREP, CONJ_COORD, CONJ_SUB, DET, INTJ, PART, NUM)
  - `token_glosses` -- (id, token_id, pos, gloss_text, nsm_prime_refs, nuance_note, status)
  - `token_variants` -- (token_id, canonical_id, morph_type, characteristic_bits)
  - `token_morph_rules` -- (migration 031) morphological rule storage per token
  - `inflection_rules` + tense envelope functions
  - 32-bit `characteristics` bitmask: register (FORMAL/CASUAL/SLANG/VULGAR/DEROGATORY/LITERARY/TECHNICAL), temporal (ARCHAIC/DATED/NEOLOGISM), geographic (DIALECT/BRITISH/AMERICAN/AUSTRALIAN)
- Migration 032: `envelope_queries` updated to new schema JOINs
- Lowercase-normalized (migration 016), particle_key indexed (migration 018)
- Frequency ranks from OpenSubtitles + Wikipedia (migration 019)
- Migration 026: 141K proper nouns tagged `proper_common='proper'` for Label tier 0 broadphase
- Migration 027: 141,755 proper noun names lowercased (canonical form in DB)
- Migration 028: 58,784 uppercase/lowercase collision pairs wired as variants; 12,325 canonicals tagged proper
- All tokens atomized to character Token IDs

**Fiction PBM shard (`hcp_fic_pbm`)** -- v* PBMs
- PBM prefix tree storage (migration 011): documents -> starters -> bond subtables
- Positional token lists on starters (migration 013)
- Positional modifiers (migration 020): morph bits + cap flags
- Document-local vars (migration 012), var staging pipeline (migration 015)

**Fiction entities (`hcp_fic_entities`)** -- u*/t*/s* namespaces
- Fiction people, places, things from cataloged texts
- 584 tokens; entity names cleaned 2026-03-05 (lowercase_underscore, 1-indexed positions, phantom refs fixed)

**Non-fiction entities (`hcp_nf_entities`)** -- y*/x*/w* namespaces
- Source records, editions, entity lists (migration 014)
- Work entities (wA.DA.*), author entities
- 116 tokens; 27 missing place name rows inserted 2026-03-05

**Var shard (`hcp_var`)** -- short-term memory for unresolved sequences
- Var token dedup by surface form
- var_sources with doc_token_id for cross-shard references (migration 022)

DB dumps: gzipped with SHA-256 checksums. `load.sh` handles all 6 databases with auto-creation and checksum verification.

### JSON Ingestion Pipeline (2026-03-07, commits 9797ed4, 125b998)

Batch JSON ingestion support wired into the `ingest`/`phys_ingest` socket API:
- `.json` files dispatched to `DispatchJsonFile` (handles both single-object and array/batch JSON)
- Array: iterates entries, creates stubs via `CreateDocumentStub`, returns batch summary
- `phys_ingest` with `catalog`+`catalog_id`: looks up existing stub, calls `FillPBMData` to fill PBM bonds
- 110 Gutenberg texts ingested: batch JSON creates stubs, TXT files fill bond data via `FillPBMData`

### Python Tooling (`scripts/`)
- `compile_vocab_lmdb.py` -- LMDB compiler with morphological stripping
- `compile_entity_lmdb.py` -- Entity LMDB compiler (fic/nf sub-dbs)
- `merge_frequency_ranks.py` -- Frequency data merge (Wikipedia + OpenSubtitles -> Postgres)
- `ingest_texts.py` -- Text ingestion via socket API
- `run_benchmark.py` -- Benchmark runner

### Schema & Kaikki Design Docs

- `docs/hcp-english-schema-design.md` -- New hcp_english schema (DB Specialist, 2026-03-10). Answers Q1-Q6 from restructure spec. PoS enum, characteristic bitmask layout, migration path (rebuild alongside, no in-place migration).
- `docs/kaikki-analysis.md` -- Full analysis of Kaikki English dump (1,454,988 entries, 2026-03-10). PoS distribution, form-of senses, Label/proper noun count, characteristic tag corpus, multi-pass population plan.
- `docs/kaikki-tag-mapping.md` -- Kaikki tag → HCP characteristic bit mapping table.
- `docs/kaikki-population-plan.md` -- 6-pass population plan: Pass 1 roots → Pass 2 delta variants → Pass 3 alt-of → Pass 4 loanwords → Pass 5 derogatory/vulgar → Pass 6 Labels.
- `docs/language-shard-restructure-spec.md` -- Strategic spec that drove schema design. Roots-only DB, regular inflections by rule, morpheme acceptance flags, envelope-tense-awareness.
- `docs/grammar-identifier-spec.md` -- Grammar identifier spec (SVO detection, PoS tagging, morpheme-first resolution).
- `docs/envelope-vocab-spec.md` -- Rewritten 2026-03-07 to reflect migrations 025-028 decisions (env_vocab sub-db, Label tier query, LMDB sub-db separation).

### Decision Records (`docs/decisions/`)
- 001: Token ID decomposition
- 002: Names shard elimination (hcp_names merged into hcp_english)
- 005: Decompose all token references

### Research Documents (`docs/research/`)
- PBM storage schema, entity DB design, source workstation design
- Sub-categorization patterns, concept mesh decomposition, force patterns
- PhysX evaluation docs in `hcp-engine/docs/`

## What's In Progress

- **LMDB compiler update** (next up) -- Must be extended to read new `token_pos` + `token_morph_rules` tables and emit PoS + characteristic bits in vbed entries. Coordination item between DB specialist and engine specialist.
- **Engine specialist review** -- Resolution ordering needs review given new PoS data; verification run on full corpus required to validate rebuild quality.
- **Envelope-based variant loading** -- Wire variant DB entries (env_archaic / env_dialect / env_casual) into resolve loop with VARIANT morph bits (bits 12-15). Variant forms stored in DB; loading path not yet wired.
- **Pipeline bugs (found 2026-03-10)** -- SingleChar runs ("I", "a") get empty matchedTokenId (preAssignedTokenId never set); Numeric runs use "NUM" pseudo-token (not a real token_id). Fix pending engine specialist.
- **Entity LMDB recompile** -- Entity DB cleaned (2026-03-05); `compile_entity_lmdb.py` needs review for variant/morph category support before next compile.
- **Label propagation** -- If word appears as Label anywhere, restore firstCap on all suppressed instances.

## Recently Completed

- **English shard rebuild from Kaikki COMPLETE** (2026-03-10, commits `33b6b42` + `d31a3d2`) -- All 6 passes done. 21,019 root tokens (AB namespace), 496,364 token_pos, 601,457 token_morph_rules, 495,765 glosses, 29,289 irregular + 15,734 archaic variants, 26,926 loanword tags, 4,188 Labels (AD), 1,743 initialisms (AE). DB totals: ~1,421,622 tokens, 502,295 token_pos, 579,388 morph_rules, 501,696 glosses, 45,023 variants. Known data quality issue: V_MAIN over-assigned ~2K tokens (not primarily verbs); flagged for downstream cleanup.
- **Migrations 031-032** (2026-03-10) -- 031: `token_morph_rules` table added. 032: `envelope_queries` updated to new schema JOINs.
- **Migrations 025-030** (2026-03-07–2026-03-10) -- 025: envelope query fixes (word→name, frequency_rank→freq_rank, lmdb_subdb→env_vocab). 026: Label tier broadphase (141K proper nouns tagged `proper_common='proper'`). 027: 141,755 proper nouns lowercased. 028: 58,784 collision pairs wired as variants. 029-030: new hcp_english schema tables (token_pos, token_glosses, token_variants) + inflection functions + tense envelopes.
- **Kaikki data analysis** (2026-03-10, `docs/kaikki-analysis.md`) -- Full analysis of 2026-03-03 enwiktionary dump: 1,454,988 entries, 1,323,204 unique lowercase words, 84,051 multi-PoS, 193K proper nouns, 525,941 form-of senses, rich characteristic tag inventory. 6-pass population plan documented.
- **hcp_english schema design** (2026-03-10, `docs/hcp-english-schema-design.md`) -- DB Specialist design: token_pos junction, token_glosses, token_variants, pos_tag enum, 32-bit characteristics bitmask, migration path decided (rebuild alongside, no in-place migration).
- **JSON ingestion pipeline** (2026-03-07, commits `9797ed4` + `125b998`) -- Batch JSON support: DispatchJsonFile, CreateDocumentStub, FillPBMData, GetDocPkByCatalogId, phys_ingest metadata path. 110 Gutenberg texts ingested.
- **CONTRIBUTING.md language policy** (2026-03-07, commit `700495d`) -- C++ for engine runtime, Python for tooling/scripts only. Policy codified.
- **Codex review fixes** (commit `01f451e`) -- psycopg2→psycopg3, stale AGENTS.md/CONTRIBUTING.md refs fixed.
- **Envelope vocab spec rewrite** (2026-03-07, `docs/envelope-vocab-spec.md`) -- Reflects all migrations 025-028 decisions; env_vocab vs w2t sub-db split; Label tier query design.
- **PR reviews** (#44, #45) -- HTML text extractor and LMDB verification script from contributor `dhitalprashant77-lab`. Reviewed and feedback posted (first external contributions).
- **V-1/V-3 variant normalization** (2026-03-04, commit `d59c2fa`) -- `TryVariantNormalize` in `HCPVocabBed.cpp`. V-1 g-drop (`-in'`→`-ing`, dialect speech) and V-3 archaic (`-eth` 3rd person). Validated: 12/12 dialect g-drops resolved in Sign of Four.
- **Entity data cleanup** (2026-03-05) -- `hcp_fic_entities` and `hcp_nf_entities` cleaned in-place. tokens.name normalized (lowercase_underscore). 27 missing nf place name rows inserted. 723 sequences verified.

## What Doesn't Exist Yet

- Initialisms engine-side resolution (AE namespace populated, engine wiring pending)
- Web address mini-language (language-independent, in hcp_core)
- Concept force modeling (~65 core NSM forces -- categorization, axes, definitions TBD)
- Full text inference (Phase 3 -- NAPIER)
- Multi-modality support (audio, visual)
- Identity structures implementation (personality DB, relationship DB)
- Community infrastructure (issue templates, CI, discussion forums)
- Cross-platform build (Windows planned)

## Data Sources

- **Kaikki** -- Wiktionary extracts for English vocabulary. Open data.
- **Wikipedia word frequency** (2023, MIT license) -- 2.7M entries
- **OpenSubtitles word frequency** (CC BY-SA 4.0) -- 1.6M entries
- **Project Gutenberg** -- Source texts for PBM construction and benchmarking

## Docs Referencing Old hcp_english Structure (needs future update)

The following docs still reference the old hcp_english schema (1.4M Wiktionary tokens, `canonical_id` on `tokens` table, `tokens.category`, old migration counts). They are not wrong for their historical context but need updating now that the Kaikki rebuild is complete:

- `docs/roadmap.md` -- references old vocab numbers
- `docs/variant-rules-proposal.md` -- references `tokens.canonical_id`
- `docs/variant-forms-audit-2026-03-04.md` -- pre-migration-028 variant analysis
- `docs/brief-lmdb-vocab-compilation.md` -- references old schema columns
- `docs/spec/cache-miss-resolver-spec.md` -- references old token table layout
- `docs/spec/namespace-reference.md` -- migration count and token counts stale
- `docs/research/pbm-storage-schema-design.md` -- early design doc, pre-schema-rebuild
- `docs/research/concept-mesh-decomposition.md` -- references old token structure
- `docs/research/force-pattern-db-review.md` -- references old schema
- `docs/decisions/001-token-id-decomposition.md` -- historical, not stale but predates new PoS tables
- `docs/decisions/002-names-shard-elimination.md` -- historical
