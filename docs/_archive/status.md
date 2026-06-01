# Project Status

_Last updated: 2026-04-16_

Core development is shifting to NSM concept modeling. Document storage and higher-order tokenization are paused until story-level constructs are evaluated in concept space.

## What Exists

### Governance (stable)
- **Covenant** -- Founder's Covenant of Perpetual Openness. Ratified.
- **Charter** -- Contributor's Charter. Ratified.
- **License** -- AGPL-3.0, governed by the Covenant.

### Specifications (first draft)
- **Data conventions** -- core definitions, PBM storage algorithm, atomization, NSM decomposition
- **Token addressing** -- base-50 scheme, 5-pair dotted notation, reserved namespaces
- **Pair-bond maps** -- FPB/FBR structure, reconstruction, compression, error correction
- **Architecture** -- two-engine model, conceptual forces, LoD stacking
- **Identity structures** -- personality DB (seed + living layer), relationship DB, integration with core data model

These are first-pass specs derived from working notes. They need review, critique, and refinement.

### Engine: O3DE + PhysX 5 PBD Superposition Pipeline

The primary inference engine is an O3DE 25.10.2 C++ Gem using PhysX 5 GPU-accelerated Position Based Dynamics for tokenization and resolution. ~21,300 lines of C++ across 35 source modules.

**Headless daemon** (`HCPEngine.HeadlessServerLauncher`) listens on port 9720 with JSON socket API for health, ingest, retrieve, list, tokenize, and physics-based ingestion.

All data enters the system as 4-byte codepoint streams, regardless of source content. Assembly through letter constructs and word hierarchies is managed via physics-based superposition resolution chambers. All languages and file formats can theoretically enter via the same mechanism.

**Pipeline:**
```
DecodeUtf8ToCodepoints
  -> Phase 1: byte->char PBD superposition (16K codepoints/chunk, Unicode-aware)
  -> ExtractRunsFromCollapses (with capitalization suppression)
  -> Phase 2: char->word via persistent VocabBeds (BedManager, 5 GPU workspaces: 3 primary + 2 extended, triple-pipelined via RunPipelinedCascade)
    -> TryInflectionStrip (PBD is the existence check, silent-e fallback)
    -> TryVariantNormalize (V-1 g-drop: -in'->-ing; V-3 archaic: -eth->base)
    -> AnnotateManifest (multi-word entity recognition: 603 fic + 95 nf name sequences)
    -> ScanManifestToPBM
    -> StorePBM + StorePositions
```

**Storage model:**
- Every form or conjugation of every word is its own token -- no morph-bit reconstruction at read time.
- Positions stored as `INTEGER[]` on `pbm_starters` (one row per doc x token).
- Only `ALL_CAPS` positions stored on `pbm_documents`. `FIRST_CAP` is positional (derivable from punctuation context). Label tokens carry intrinsic capitalization in `entries.word`.
- Recording flow: engine resolves manifest against LMDB, writer batch-fetches `entries` by id, upserts `pbm_starters` with 5-part token columns + `INTEGER[]` positions.

**Stress-test results (2026-04-13, 9 documents):**

| Doc | Slots | Starters | Vars | Var % |
|-----|------:|---------:|-----:|------:|
| Yellow Wallpaper | 11,577 | 2,216 | 34 | 0.29 |
| Alice in Wonderland | 39,816 | 3,927 | 89 | 0.22 |
| Jekyll & Hyde | 36,294 | 5,418 | 70 | 0.19 |
| Frankenstein | 92,963 | 8,967 | 141 | 0.15 |
| Wuthering Heights | 152,058 | 11,669 | 381 | 0.25 |
| Pride & Prejudice | 162,166 | 8,717 | 315 | 0.19 |
| Crime & Punishment | 264,092 | 12,025 | 394 | 0.15 |
| Moby Dick | 268,162 | 22,437 | 1,573 | 0.59 |
| The Great Gatsby | 67,764 | 7,817 | 209 | 0.31 |

~1.1M total token positions. Var rates 0.15--0.59% (Moby Dick highest due to nautical vocabulary). Reconstruction verified end-to-end -- reported issues are nominal whitespace/cap nudges, no meaning drift. Above 98% accuracy with 99% of words stored at highest-level word construct token_IDs.

**Benchmarks (2026-03-04, GTX 1070 headless, pipelined):**

| Text | Size | Tokens | Vars | Wall Time | Previous (pre-pipeline) |
|------|------|--------|------|-----------|--------------------------|
| Dracula | 890 KB | 199,368 | 110 | **28.6s** | 166.5s (2026-03-02) |
| A Study in Scarlet | 269 KB | 56,061 | 54 | **12.2s** | 133.0s (2026-03-02) |
| The Yellow Wallpaper | 47 KB | 10,856 | 37 | -- | -- |
| The Sign of Four | -- | -- | 52 | -- | -- |

**Key engine modules** (`hcp-engine/Gem/Source/`):

| Module | Purpose |
|--------|---------|
| `HCPVocabBed.h/cpp` | VocabBed (persistent PBD per word length) + BedManager (orchestrator) |
| `HCPVocabulary.h/cpp` | LMDB reader + affix loader. 48 max DBIs. |
| `HCPSuperpositionTrial.h/cpp` | Phase 1 byte->char PBD |
| `HCPWordSuperpositionTrial.h/cpp` | CharRun extraction with cap suppression |
| `HCPEntityAnnotator.h/cpp` | Multi-word entity recognition from LMDB (603 fic + 95 nf sequences) |
| `HCPEngineSystemComponent.h/cpp` | Top-level orchestrator. Owns BedManager + all DB kernels. |
| `HCPSocketServer.h/cpp` | TCP socket API on port 9720 |
| `HCPParticlePipeline.h/cpp` | PBD particle system: Phase 1 + Phase 2 scenes |
| `HCPEnvelopeManager.h/cpp` | Activity envelope cache lifecycle (LMDB hot cache) |
| `HCPCacheMissResolver.h/cpp` | LMDB cache miss -> Postgres fill |
| `HCPBondCompiler.h/cpp` | Sub-word PBM bond tables (char->word, byte->char) |
| `HCPTokenizer.h/cpp` | 7-step resolution cascade + TokenIdsToText reconstruction |

**Decomposed DB kernel modules** (split from monolith 2026-03-01):

| Module | Purpose |
|--------|---------|
| `HCPDbConnection.h/cpp` | Shared PGconn* wrapper |
| `HCPPbmWriter.h/cpp` | StorePBM + StorePositions (INTEGER[] per doc x token) |
| `HCPPbmReader.h/cpp` | LoadPBM + LoadPositions (reads INTEGER[] directly) |
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

LMDB is an ephemeral working surface for the engine. It holds only what makes lookups fast -- padded spelling + a 4-byte INT back-reference to `entries.id`. Not a mirror of Postgres structure.

- Per-word-length sub-databases (`vbed_02`..`vbed_16`), frequency-ordered
- Labels (tier 0) -> freq-ranked (tier 1) -> unranked remainder (tier 2)
- Entity sub-databases: 698 sequences total (603 fic + 95 nf)
- Frequency data: Wikipedia 2023 + OpenSubtitles merged. 176K tokens ranked.
- LMDB population done via envelope system at runtime (EnvelopeManager::ExecuteQuery). No offline compilation script needed.
- Broadphase makes inactive particles free -- load freely, activate selectively. Over-loading costs ephemeral disk; under-loading costs latency.

### Envelope System

Composable query plans that move data from Postgres to LMDB for engine consumption.

- **Three-layer cache**: LMDB (hot, mmap'd) -> Postgres (warm, envelope working set) -> disk/LFS (cold).
- Envelope activation runs chained prepared statements in a single Postgres transaction, streams results into LMDB.
- Envelopes are first-class DB objects, composable -- overlapping queries share results, nesting = query union.
- Schema in `hcp_envelope` database.

### Database Shards (11 databases)

**Core shard (`hcp_core`)** -- AA namespace
- ~5,470 tokens: byte codes, Unicode characters, structural markers, NSM primitives, URI elements
- AA.AG: URI Elements (56 tokens -- protocols, file formats, programming tools, standards, TLDs)
- Namespace allocations, shard registry (3 entries -- stale/TBD)
- Activity envelope schema (definitions, queries, composition, audit log)
- Punctuation and single-character tokens live here, not in hcp_english

**English shard (`hcp_english`)** -- AB/AC/AD namespaces
- Full Kaikki re-ingestion COMPLETE (2026-04-07).
- **1,493,964 entries** across 5 namespaces:
  - AB.AA: ~980K single words
  - AB.AB: ~280K multi-word phrases (+ 338,599 phrase_components rows)
  - AC.AA: 4,123 morphemes
  - AD.AA: ~170K single-word Labels (proper nouns)
  - AD.AB: 58,726 multi-word Labels (compound proper nouns)
- **6,498 misspelling entries** tagged via alt_of_tags, linked to canonical forms
- All text self-tokenized -- glosses, examples, etymologies, contexts consumed to token_id arrays. Zero raw text remaining.
- 9.2M sense categories for broadphase filtering (29,549 distinct names)
- 1.84M relations (synonym, derived, related, hyponym, etc.), 91% resolved to target token_ids
- 1,249 english_characters linked to 19,388 core Unicode tokens
- **Primary table**: `entries` (decomposed ns/p2/p3/p4/p5 for B-tree indexed traversal, plus word, pos, spelling, morphology)

**Fiction PBM shard (`hcp_fic_pbm`)** -- v* PBMs
- PBM prefix tree storage: documents -> starters -> bond subtables (word, char, marker, var)
- Positional token lists as `INTEGER[]` on `pbm_starters`
- `all_caps_positions INTEGER[]` on `pbm_documents`
- Document-local vars, var staging pipeline
- Word bonds exist in schema but are NOT yet computed; char and var bonds populated during ingest

**Envelope shard (`hcp_envelope`)** -- envelope management
- Dedicated DB for envelope lifecycle and cache coordination

**Var shard (`hcp_var`)** -- short-term memory for unresolved sequences
- Var token dedup by surface form
- var_sources with doc_token_id for cross-shard references
- `envelope_working_set` table for warm cache assembly

**Entity databases (6 language-independent DBs, split by literary/non-literary x person/place/thing)**

| Database | Namespace | Tokens | Contents |
|----------|-----------|--------|----------|
| `hcp_nf_things` | wA | 142,513 | Languages, science, technology, organizations, holidays, other |
| `hcp_nf_people` | yA | 3,917 | Historical persons, religious figures, mythology (all traditions) |
| `hcp_nf_places` | xA | 1,452 | Countries, continents, constellations, planets |
| `hcp_fic_people` | uA | 962 | Fictional characters, literary individuals |
| `hcp_fic_places` | tA | 10 | Fictional settings |
| `hcp_fic_things` | sA | 1 | Fictional objects |

DB dumps: gzipped with SHA-256 checksums. `load.sh` handles all databases with auto-creation and checksum verification.

### Token_ID Namespace

Token_ID namespace is intentionally vast and easily expandable. The HCP adopts a see-it-and-mint-it philosophy for anything recognized as a surface form of a word or concept. Every form or conjugation of every word, including misspellings, is included as a distinct entry with links to core meanings and conceptual structures. Only strictly idiosyncratic use cases with no significant concept effect are excluded.

### Ingestion

- Codepoint-level recognition beds resolve all established Unicode and private endpoints.
- All resolution mechanisms above codepoint level are focused on English-based, serial ingestion of md and txt files with an accompanying JSON metadata resolver.
- Batch processing (compile full resolution candidate list from multiple manifests before invoking resolution) and stream processing (constant insertion into persistently rotating resolution chambers) are both architecturally supported.
- All elements are modularized. Contributors adding other language shards can insert path discriminators at appropriate resolution levels. Latin Extended character sets insert at spelling level. Other space-delineated languages fork immediately following that operation. Non-space-delineated languages require a new resolution chamber feed mechanism.

### JSON Ingestion Pipeline (2026-03-07)

Batch JSON ingestion support wired into the `ingest`/`phys_ingest` socket API:
- `.json` files dispatched to `DispatchJsonFile` (handles both single-object and array/batch JSON)
- Array: iterates entries, creates stubs via `CreateDocumentStub`, returns batch summary
- `phys_ingest` with `catalog`+`catalog_id`: looks up existing stub, calls `FillPBMData` to fill PBM bonds
- 110 Gutenberg texts cataloged: batch JSON creates stubs, TXT files fill bond data via `FillPBMData`

### Python Tooling (`scripts/`)

Operational/convenience scripts only -- not part of the engine pipeline.

- `setup_kaikki_schema.py`, `load_kaikki_fast.py` -- Kaikki ingestion
- `self_tokenize.py` -- Self-tokenization (text -> token_id arrays)
- `mint_multiword_ids.py`, `mint_category_phrases.py` -- Phrase minting
- `build_phrase_components.py`, `phrase_resolve_tokenized.py`, `resolve_partial_phrases.py` -- Phrase resolution
- `create_nf_people_entities.py`, `create_nf_place_entities.py`, `create_nf_other_entities.py` -- NF entity creation
- `create_multiword_entities.py`, `sweep_entity_glosses.py` -- Entity sweep
- `resolve_relations.py`, `resolve_relations_pass2.py` -- Relation resolution
- `split_entity_dbs.py` -- Entity DB split (2->6)
- `migrate_to_proper_schema.py` -- Schema rename (kk_->proper names)
- `merge_frequency_ranks.py` -- Frequency data merge
- `ingest_texts.py` -- Text ingestion via socket API
- `run_benchmark.py` -- Benchmark runner
- `hcp_client.py` -- Socket API client library
- Deprecated scripts moved to `scripts/deprecated/`

### Decision Records (`docs/decisions/`)
- 001: Token ID decomposition
- 002: Names shard elimination (hcp_names merged into hcp_english)
- 005: Decompose all token references

### Research Documents (`docs/research/`)
- PBM storage schema, entity DB design, source workstation design
- Sub-categorization patterns, concept mesh decomposition, force patterns
- PhysX evaluation docs in `hcp-engine/docs/`

## What's Deferred

The following items are paused while core development moves to NSM concept modeling:

- **Phrase/idiom resolution chambers** -- 279,873 AB.AB entries + 338,599 phrase_components rows exist. Minted but not wired to resolution at ingest time.
- **Proper entity resolution** -- Requires linking to dramatis or other reference lists for holistic constructs.
- **Possessive handling** -- Needs a focused design pass. Should not be a morpheme bit on the base token.
- **Label propagation** -- Restore firstCap on all suppressed instances of a Label token.
- **Known initialisms** (U.S., U.K., etc.)
- **LMDB shape simplification** -- Drop structured token_id parts from LMDB entries; keep spelling + INT back-ref only.
- **SQLite distribution format** -- Needs to track the Postgres shape (currently diverged).
- **Multi-doc batch resolve** -- Socket API accepts one doc per call; batch entry point deferred.
- **Entity matching** -- Connecting PBM documents to author/work entities.
- **Delta audit completion** -- ~19% complete (87K of 443K tokens assessed). Lengths 8-16 remain.
- **Entity DB encoding** -- Dewey-derived classification for p2+p3 (deferred, using sequential).
- **Remaining relation resolution** -- 169K unresolved (40K char-level, 11K foreign, 117K missing).
- **Cross-platform build** (Windows planned)

## What Doesn't Exist Yet

- Concept force modeling (~65 core NSM forces -- categorization, axes, definitions TBD)
- Full text inference (Phase 3 -- NAPIER)
- Multi-modality support (audio, visual)
- Identity structures implementation (personality DB, relationship DB)
- Community infrastructure (issue templates, CI, discussion forums)
- GUI ingestion (workstation is view-only; ingestion via socket API)

## Data Sources

- **Kaikki** -- Wiktionary extracts for English vocabulary. Open data.
- **Wikipedia word frequency** (2023, MIT license) -- 2.7M entries
- **OpenSubtitles word frequency** (CC BY-SA 4.0) -- 1.6M entries
- **Project Gutenberg** -- Source texts for PBM construction and benchmarking

## Historical Docs (reference only)

The following docs predate recent passes and are retained for context. They should not be treated as current specifications:

- `docs/variant-rules-proposal.md` -- pre-tree-model variant design (references `tokens.canonical_id`)
- `docs/variant-forms-audit-2026-03-04.md` -- pre-tree-model variant analysis
- `docs/brief-lmdb-vocab-compilation.md` -- references old schema columns; LMDB now populated via envelope system
- `docs/spec/cache-miss-resolver-spec.md` -- references old token table layout
- `docs/spec/namespace-reference.md` -- migration count and token counts stale
- `docs/research/pbm-storage-schema-design.md` -- early design doc
- `docs/research/concept-mesh-decomposition.md` -- references old token structure
- `docs/research/force-pattern-db-review.md` -- references old schema
- `docs/decisions/001-token-id-decomposition.md`, `002-names-shard-elimination.md` -- historical decisions, still valid
