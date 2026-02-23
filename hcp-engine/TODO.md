# HCP Engine — TODO

Last updated: 2026-02-19

## Legend

- **[BLOCKED]** — waiting on another task or person
- **[READY]** — can be picked up now
- **[IN PROGRESS]** — someone is working on it
- **[DONE]** — completed, kept for context until next cleanup

---

## Phase 1: Document Processing Workstation

### Engine Tasks

- [ ] **[READY]** Create HCPDocumentBuilder skeleton — builder + component + module, register with Asset Processor, handle `.txt` files, emit dummy product. Verify AP discovers it.
- [ ] **[READY]** Create HCPDocumentBuilderVocab — Postgres-backed vocabulary reader with same interface as HCPVocabulary (LookupChunk, CheckContinuation, LookupChar). Verify lookups match LMDB.
- [ ] **[BLOCKED]** Wire tokenizer into ProcessJob — reuse HCPTokenizer with Postgres-backed vocab. Blocked on builder skeleton + vocab.
- [ ] **[BLOCKED]** Position map product format — binary format: token stream + base-50 positions + metadata header. Blocked on tokenizer integration.
- [ ] **[READY]** Add `.Builders` CMake target — separate GEM_MODULE linking AssetBuilderSDK + libpq. Does NOT link PhysX5, LMDB, or CUDA.
- [ ] **[READY]** Create `hcpengine_builder_files.cmake` — source list for builder target.
- [ ] **[BLOCKED]** Scan folder configuration — point Asset Processor at `data/gutenberg/texts/`. Blocked on working builder.
- [ ] **[BLOCKED]** End-to-end verification — tokenization output matches self-test (Yellow Wallpaper: 9,122 tokens, 18,840 slots). Blocked on full pipeline.

### DB Tasks

- [ ] **[READY]** Confirm Postgres read access pattern for builder — builder runs inside Asset Processor process, needs w2t, c2t, l2t, forward table access.
- [ ] **[READY]** Fix marker table PK collision — control tokens share `(t_p3, t_p4)`, needs t_p5 column added to `doc_marker_positions`.
- [ ] **[READY]** Confirm forward table is populated for full vocabulary.
- [ ] **[BLOCKED]** Review position map product format — confirm sufficient for aggregation layer (PBM derived on the fly). Blocked on format spec.

---

## Phase 2: Document Reconstruction

- [ ] **[BLOCKED]** Position map reader (shared module) — blocked on product format from Phase 1.
- [ ] **[BLOCKED]** PBM derivation function — positions in, bond counts out. O(n) scan. Blocked on position map reader.
- [ ] **[BLOCKED]** Reconstruction proof — text in, position map, text back. Lossless round-trip. Blocked on PBM derivation.
- [ ] **[BLOCKED]** Document inspector tool — view position maps, derive PBM, inspect structure. Blocked on reader + derivation.

---

## Phase 3: Comparison Tool

- [ ] **[BLOCKED]** Boilerplate/plagiarism detector — two documents as parallel particle streams, magnetic forces, convergence clustering. First real PhysX use case. Blocked on Phase 2.

---

## Format Builders (contributor tasks)

These follow the reference `.txt` implementation. See AGENTS.md for the pattern.

- [ ] **[BLOCKED]** PDF builder — text extraction via poppler/pdftotext. Blocked on reference implementation.
- [ ] **[BLOCKED]** EPUB builder — unzip + XHTML parsing. Blocked on reference implementation.
- [ ] **[BLOCKED]** HTML builder — markup stripping, structure preservation. Blocked on reference implementation.
- [ ] **[BLOCKED]** Markdown builder — may share `.txt` builder with minimal additions. Blocked on reference implementation.
- [ ] **[BLOCKED]** Wikipedia dump builder — MediaWiki markup parsing. Blocked on reference implementation.

---

## Infrastructure

- [ ] **[READY]** SQLite vocabulary backend for standalone distribution — builder currently assumes Postgres (libpq). Standalone Asset Processor packages should query a SQLite dump instead, removing the Postgres server dependency for contributors doing document processing.
- [ ] **[READY]** Root-level AGENTS.md / CLAUDE.md that references subdirectory files.
- [ ] **[READY]** Contributor setup documentation — SDK install, project build, Postgres setup.

---

## Cleanup

- [ ] **[READY]** Purge "7 force types" from repo docs — memory files done, research docs and specs still contain stale references. Files: `openmm-evaluation.md`, `english-force-patterns.md`, `force-pattern-db-requirements.md`, `force-pattern-db-review.md`, `concept-mesh-decomposition.md`, `particle-vs-md-operations.md`, `namespace-reference.md`, `006_force_patterns.sh`, `006a_force_infrastructure_tokens.sql`.
- [ ] **[READY]** Remove socket server once Asset Builder pipeline is proven and stable.

---

## Future (not yet planned in detail)

- Custom physics engine (~65 core forces, linguist-defined)
- Conversation levels (documents as entities in level workspaces)
- HCPDocumentAsset + runtime handler
- LoD tiers (multiple aggregation levels per document)
- Vocabulary inspector tool
- Tokenization tester tool
- Language shard system (new languages via vocabulary + force constants)
- Texture engine (linguistic force bonding — surface language rules)
