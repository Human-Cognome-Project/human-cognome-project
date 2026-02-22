# HCP Roadmap — Master Reference

The Human Cognome Project maps cognition through physics-based structural analysis of language. Game and physics engines are database and calculation tools — kernel speed and pure function, not pixel output.

The inference model is **NAPIER**: *Not Another Proprietary Inference Engine, Really.*

## Current Focus: English Text Encoding

We are working in a narrow channel — English text — to prove the core architecture. Everything below is designed to generalize to other languages, modalities, and expression types once the foundation is proven.

## Area Roadmaps

| Area | Location | Focus |
|------|----------|-------|
| **Engine** | [hcp-engine/ROADMAP.md](hcp-engine/ROADMAP.md) | O3DE + PhysX 5 document processing pipeline, physics-native encoding |
| **Database** | [db/ROADMAP.md](db/ROADMAP.md) | PostgreSQL schema, migrations, LMDB cache, storage optimization |
| **Python tools** | [src/hcp/ROADMAP.md](src/hcp/ROADMAP.md) | Reference implementations, ingestion pipeline, test tooling |
| **Specifications** | [docs/spec/](docs/spec/) | Architecture, PBM format, token addressing, namespace reference |

## Project Phases

### Phase 1: Document Processing Workstation (current)
Encode Gutenberg texts into PBM storage via the O3DE engine. Validate lossless round-trip reconstruction.

**Key deliverables:**
- Source workstation — ingest, tokenize, store, review documents
- PBM prefix tree storage (migration 011 — done)
- Self-filling LMDB cache with C++ resolver (done)
- Bond compiler for sub-word PBMs (char→word, byte→char — done)
- Physics detection scene for token resolution (working prototype)
- Lossless round-trip proof (pending)

### Phase 2: Boilerplate and Comparison
PhysX particle pipeline for document comparison, boilerplate detection via rigid body matching, sub-PBM compression.

### Phase 3: Vocabulary and Language Tools
Inspectors, testers, vocabulary management. Contributor tooling for expanding language coverage.

### Phase 4: Grammar and Sentence Structure
Incorporate grammar construction rules. PoS bonding behaviour, syntactic patterns. Linguistics-driven force definitions (~65 core forces).

### Phase 5: NSM Decomposition
Decompose dictionary through abstraction layers to Natural Semantic Metalanguage primitives. Populate AA.AA.AA.AB.* (65 atoms) and AA.AC.* (~2,719 molecules).

### Phase 6: Conversation and Dynamic Documents
Dynamic document spawning, activity envelope management, real-time interaction.

### Phase 7: Custom Physics Engine
Full 65+ linguist-defined core forces. Texture engine (surface language) + Mesh engine (conceptual structure).

### Phase 8: Identity and Theory of Mind
Personality DB, relationship DB, ToM modeling as fluid dynamics. Multiple concurrent ToM constructs.

### Phase 9: Full Text Inference (NAPIER)
Complete response engine for language — deconstruction and generation as a unified physics system.

### Phase 10: Multi-Modality
Extend structural primitives beyond text. Mode namespaces for audio, visual. Cross-modal mapping via shared NSM primitives.

## Contributor Expansion Points

These are areas where contributors can make significant impact:

| Area | Status | What's Needed |
|------|--------|---------------|
| **Format builders** (PDF, EPUB, HTML, Markdown, Wikipedia) | Stubbed | Text extraction → existing tokenizer pipeline |
| **Other languages** | Architecture ready, no implementation | Language shard + sub-cat patterns + force constants |
| **Entity cataloging** | 110 texts done, framework ready | Apply librarian pipeline to new texts |
| **Force definitions** | ~65 expected, categorization TBD | Linguistics expertise needed |
| **Test coverage** | 65 tests, needs expansion | Edge cases, Unicode, adversarial inputs |
| **Documentation** | Good specs, some drift | Review and update |
| **Physics engine tuning** | Working prototype | PhysX parameter optimization |

## Software Stack

- **O3DE 25.10.2** — Game engine, module management, Asset Processor, Gem system
- **PhysX 5.1.1** — Physics engine (PBD particles, rigid/soft body, FEM, GPU acceleration)
- **C++** — Engine language (forced by O3DE)
- **PostgreSQL** — Source of truth (vocabulary, bonds, token definitions, namespace allocations)
- **LMDB** — Runtime read cache (self-filling, memory-mapped, zero-copy)
- **SQLite** — Portable dump serving for standalone tools
- **Python** — Tooling, ingestion, reference implementations, tests

## Performance Reality

Physics engines are the most optimized database engines ever built. Token IDs and bond strengths are simpler data than collision meshes. A gaming laptop runs cognitive modeling at real-time speeds. The bottleneck was never hardware — it was recognizing the tool already exists.

## Open Questions

See area-specific TODOs for current open questions. Major architectural decisions are documented in [docs/decisions/](docs/decisions/).
