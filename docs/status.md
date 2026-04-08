# Project Status

_Last updated: 2026-04-08_

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
- Per-word-length sub-databases (`vbed_02`..`vbed_16`), frequency-ordered
- Labels (tier 0) -> freq-ranked (tier 1) -> unranked remainder (tier 2)
- Entity sub-databases: 723 sequences total (fic + nf), verified via dry-run. DB cleaned 2026-03-05; LMDB recompile pending.
- Frequency data: Wikipedia 2023 + OpenSubtitles merged. 176K tokens ranked.
- LMDB population done via envelope system at runtime (EnvelopeManager::ExecuteQuery). No offline compilation script needed.

### Database Shards (11 databases)

**Core shard (`hcp_core`)** -- AA namespace
- ~5,470 tokens: byte codes, Unicode characters, structural markers, NSM primitives, URI elements
- AA.AG: URI Elements (56 tokens -- protocols, file formats, programming tools, standards, TLDs)
- Namespace allocations, shard registry (3 entries — stale/TBD, was intended as shard traffic manager but not maintained)
- Activity envelope schema (definitions, queries, composition, audit log)
- Punctuation and single-character tokens live here, not in hcp_english
- LMDB sub-db split: `env_vocab` (envelope-loaded, evicted on switch) vs `w2t` (cache-miss fills, persistent)

**English shard (`hcp_english`)** -- AB/AC/AD namespaces
- **Full Kaikki re-ingestion COMPLETE** (2026-04-07). Previous tree-model approach superseded.
- **1,493,964 tokens** across 5 namespaces:
  - AB.AA: 980,039 single words
  - AB.AB: 279,873 multi-word phrases
  - AC.AA: 4,123 morphemes
  - AD.AA: 171,203 single-word Labels (proper nouns)
  - AD.AB: 58,726 multi-word Labels (compound proper nouns)
- **All text self-tokenized** — glosses, examples, etymologies, contexts consumed to token_id arrays. Zero raw text remaining.
- **9.2M sense categories** for broadphase filtering (29,549 distinct names)
- **1.84M relations** (synonym, derived, related, hyponym, etc.), 91% resolved to target token_ids
- **1,249 english_characters** linked to 19,388 core Unicode tokens
- **Schema tables** (`entries` is primary — decomposed ns/p2/p3/p4/p5 for B-tree indexed traversal):
  - `entries` — word, pos, token_id (decomposed), spelling (char seq array), morphology (token_id array)
  - `senses` — tokenized_gloss, tokenized definitions
  - `sense_categories` — broadphase category tags (29,549 distinct)
  - `sense_examples` — tokenized usage examples
  - `relations` — word relationships with target_token_id resolution
  - `forms` — inflected forms
  - `phrase_components` — phrase → ordered component word token mapping
  - `english_characters` — character → core Unicode token links
  - `inflection_rules` — engine morphological stripping rules
- Token_id decomposition: ns/p2/p3/p4/p5 columns enable Postgres B-tree index traversal for broadphase (same structure as entity DBs). p3 encodes first_letter + char_length (words) or first_letter + word_count (phrases).
- Old `tokens`/`token_pos`/`token_variants` tables dropped. Engine C++ code needs updating to use `entries` table.

**Fiction PBM shard (`hcp_fic_pbm`)** -- v* PBMs
- PBM prefix tree storage: documents -> starters -> bond subtables
- Positional token lists on starters, positional modifiers (morph bits + cap flags)
- Document-local vars, var staging pipeline

**Entity databases (6 language-independent DBs, split by literary/non-literary × person/place/thing)**

| Database | Namespace | Tokens | Contents |
|----------|-----------|--------|----------|
| `hcp_nf_things` | wA | 142,513 | Languages, science, technology, organizations, holidays, other |
| `hcp_nf_people` | yA | 3,917 | Historical persons, religious figures, mythology (all traditions) |
| `hcp_nf_places` | xA | 1,452 | Countries, continents, constellations, planets |
| `hcp_fic_people` | uA | 962 | Fictional characters, literary individuals |
| `hcp_fic_places` | tA | 10 | Fictional settings |
| `hcp_fic_things` | sA | 1 | Fictional objects |

- Entity tables: `tokens` (decomposed ns/p2-p5), `entity_names` (links to shard Labels), `entity_descriptions`, `entity_properties`, `entity_relationships`, `entity_appearances`, `entity_rights`
- nf DBs also have source tables: `sources`, `source_editions`, `source_glossary`, `source_people/places/things`
- Token_ids use sequential encoding; Dewey-derived classification deferred
- Old consolidated `hcp_nf_entities` and `hcp_fic_entities` dropped

**Var shard (`hcp_var`)** -- short-term memory for unresolved sequences
- Var token dedup by surface form
- var_sources with doc_token_id for cross-shard references
- `envelope_working_set` table for warm cache assembly

**Envelope shard (`hcp_envelope`)** -- envelope management
- Dedicated DB for envelope lifecycle and cache coordination

DB dumps: gzipped with SHA-256 checksums. `load.sh` handles all databases with auto-creation and checksum verification.

### JSON Ingestion Pipeline (2026-03-07, commits 9797ed4, 125b998)

Batch JSON ingestion support wired into the `ingest`/`phys_ingest` socket API:
- `.json` files dispatched to `DispatchJsonFile` (handles both single-object and array/batch JSON)
- Array: iterates entries, creates stubs via `CreateDocumentStub`, returns batch summary
- `phys_ingest` with `catalog`+`catalog_id`: looks up existing stub, calls `FillPBMData` to fill PBM bonds
- 110 Gutenberg texts ingested: batch JSON creates stubs, TXT files fill bond data via `FillPBMData`

### Python Tooling (`scripts/`)

Operational/convenience scripts only — not part of the engine pipeline.

- `setup_kaikki_schema.py`, `load_kaikki_fast.py` -- Kaikki ingestion
- `self_tokenize.py` -- Self-tokenization (text → token_id arrays)
- `mint_multiword_ids.py`, `mint_category_phrases.py` -- Phrase minting
- `build_phrase_components.py`, `phrase_resolve_tokenized.py`, `resolve_partial_phrases.py` -- Phrase resolution
- `create_nf_people_entities.py`, `create_nf_place_entities.py`, `create_nf_other_entities.py` -- NF entity creation
- `create_multiword_entities.py`, `sweep_entity_glosses.py` -- Entity sweep
- `resolve_relations.py`, `resolve_relations_pass2.py` -- Relation resolution
- `split_entity_dbs.py` -- Entity DB split (2→6)
- `migrate_to_proper_schema.py` -- Schema rename (kk_→proper names)
- `merge_frequency_ranks.py` -- Frequency data merge
- `ingest_texts.py` -- Text ingestion via socket API
- `run_benchmark.py` -- Benchmark runner
- `hcp_client.py` -- Socket API client library
- Deprecated scripts moved to `scripts/deprecated/`

### Schema & Kaikki Design Docs

- `docs/hcp-english-schema-design.md` -- hcp_english schema design: tree model, PoS enum, characteristic bitmask layout.
- `docs/kaikki-analysis.md` -- Full analysis of Kaikki English dump (1,454,988 entries). PoS distribution, form-of senses, Label/proper noun count.
- `docs/kaikki-curation-standards.md` -- Curation rules, encoding spec, and phase documentation.
- `docs/kaikki-tag-mapping.md` -- Kaikki tag → HCP characteristic bit mapping table.
- `docs/kaikki-population-plan.md` -- 6-phase population plan (superseded by curation standards doc but retained for historical context).
- `docs/language-shard-restructure-spec.md` -- Strategic spec: roots-only DB, regular inflections by rule, envelope-tense-awareness.
- `docs/grammar-identifier-spec.md` -- Grammar identifier spec (SVO detection, PoS tagging, morpheme-first resolution).
- `docs/envelope-vocab-spec.md` -- Envelope system: env_vocab sub-db, Label tier query, LMDB sub-db separation.
- `docs/db-uri-elements-spec.md` -- AA.AG URI element namespace design.
- `docs/prefix-predictive-pipeline-design.md` -- Prefix stripping pipeline design.

### Decision Records (`docs/decisions/`)
- 001: Token ID decomposition
- 002: Names shard elimination (hcp_names merged into hcp_english)
- 005: Decompose all token references

### Research Documents (`docs/research/`)
- PBM storage schema, entity DB design, source workstation design
- Sub-categorization patterns, concept mesh decomposition, force patterns
- PhysX evaluation docs in `hcp-engine/docs/`

## What's In Progress

- **Engine C++ migration** -- Update 6 C++ files that reference old `tokens` table to use `entries` table with decomposed ns/p2/p3/p4/p5 columns. Files: HCPEnvelopeManager.cpp, HCPVocabulary.cpp, HCPBondCompiler.cpp, HCPPbmReader.cpp, HCPCacheMissResolver.cpp, HCPStorage.cpp.
- **LMDB recompilation** -- Recompile vocab LMDB from clean `entries` table data (1.49M tokens vs old 553K).
- **98% surface reconstruction target** -- Engine testing against clean data with phrase/entity resolution. Goal: resolve and reconstruct majority of Gutenberg texts.
- **Entity DB encoding** -- Dewey-derived classification for p2+p3 (deferred, using sequential). Fiction doc encoding: author_id (3 pairs) + work_number (1 pair).
- **Remaining relation resolution** -- 169K unresolved: 40K character-level artifacts, 11K foreign language (deferred to shard creation), 117K references to entries not yet in DB.
- **Possessive pipeline** -- See-it-once-and-record: mint possessive/plural-possessive forms on first encounter during ingestion.
- **Phrase resolution in engine** -- 2-stage broadphase: p3 regex (letter + length per word in phrase), then content comparison. Currently batch Python only.
- **Pipeline bugs (found 2026-03-10)** -- SingleChar runs ("I", "a") get empty matchedTokenId; Numeric runs use "NUM" pseudo-token. Fix pending.
- **NSM modeling** -- Post-reconstruction goal. Physics-based concept decomposition using the clean token graph.
- **Label propagation** -- If word appears as Label anywhere, restore firstCap on all suppressed instances.

## Recently Completed

- **Kaikki curation COMPLETE** (2026-03-17) -- Full 6-phase curation of Kaikki English dump (1,454,988 Wiktionary entries). Final state: 569,471 tokens (all AB namespace), 619,433 PoS branches (15 types), 789,548 glosses, 89,075 variants. Tree model replaces old AD/AE namespace separation. Regular inflections (517,069 entries) skipped — engine handles at runtime. N_PROPER branches: 124,171 (proper nouns, abbreviations, initialisms). Curation scripts documented in `docs/kaikki-curation-standards.md`.
- **Migrations 033-044** (2026-03-10–2026-03-17) -- AD/AE namespace collapse, envelope working set, inflection rule fixes, PBM morpheme positions, archaic filter fixes, prefix rules, EWS morpheme propagation.
- **LMDB sliding window** (2026-03-14) -- Hot cache holds 3 slices (~15K entries) instead of full 535K. ResolveLengthCycle exhausts all warm-set slices per length.
- **Prefix stripping rules** (2026-03-14) -- Data-driven, DB-sourced prefix stripping in engine resolve loop.
- **Variant morph bits** (2026-03-14) -- Archaic reconstruction + delete_doc support. Variant morph bits propagated through warm cache.
- **JSON ingestion pipeline** (2026-03-07) -- Batch JSON support: DispatchJsonFile, CreateDocumentStub, FillPBMData. 110 Gutenberg texts ingested.
- **V-1/V-3 variant normalization** (2026-03-04, commit `d59c2fa`) -- `TryVariantNormalize`: V-1 g-drop (`-in'`→`-ing`) and V-3 archaic (`-eth`). 12/12 dialect g-drops resolved in Sign of Four.
- **Entity data cleanup** (2026-03-05) -- `hcp_fic_entities` and `hcp_nf_entities` normalized in-place. 723 sequences verified.

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

## Historical Docs (reference only)

The following docs predate the Kaikki curation rebuild (2026-03-17) and tree model adoption. They document historical decisions and are retained for context but should not be treated as current specifications:

- `docs/variant-rules-proposal.md` -- pre-tree-model variant design (references `tokens.canonical_id`)
- `docs/variant-forms-audit-2026-03-04.md` -- pre-tree-model variant analysis
- `docs/brief-lmdb-vocab-compilation.md` -- references old schema columns; LMDB now populated via envelope system
- `docs/spec/cache-miss-resolver-spec.md` -- references old token table layout
- `docs/spec/namespace-reference.md` -- migration count and token counts stale
- `docs/research/pbm-storage-schema-design.md` -- early design doc
- `docs/research/concept-mesh-decomposition.md` -- references old token structure
- `docs/research/force-pattern-db-review.md` -- references old schema
- `docs/decisions/001-token-id-decomposition.md`, `002-names-shard-elimination.md` -- historical decisions, still valid
