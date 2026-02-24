# HCP Engine — Roadmap

## The Project

The Human Cognome Project is mapping the fundamental geometry of cognition through genomic style analysis of language and other forms of expression with the goal of creating fully structured cognitive models with healthy, stable auditable Theory of Mind and decision making processes.

By creating a compressed, fully reproducible, content agnostic, source storage and decomposition structure, computational tools like physics engines can represent cognition modeling as systems of physical forces acting on a conceptual mesh (the meaning of any element) and the texture layer (the specific language or other expression) that covers it.

The core functions of the resulting model draw heavily from genomic sequence mapping, which many will recognize more easily as current LLM functions at the most basic internal levels. 

Current development is using written English expression as the initial analysis point.

The project is currently developing a portable method for ingestion of source materials and metadata, and generation of project compliant data files. The MVP for this aspect will address .txt and .md ingestion and reproduction in single and batch modes. 

Immediately following that release, modeling of conceptual factors as physics force analogies will begin. The goal of this phase will be analysis and understanding of content and intent of an input source.

After development of the conceptual understanding framework, a formal Theory of Mind and inference engine will be incorporated into the front end as the core of the NAPIER (Not Another Proprietary Inference Engine, Really!) Digital Intelligence architecture.

NAPIER is where that theory becomes working code. All forms of expression decompose into tokens (particles), bonded by explicit pair relationships (PBMs), governed by context specific physics derived probability statistics — not generalized, frozen, massively averaged statistical weights. Lossless reconstruction proves comprehension. The same structural framework applies to any modality (text, audio, visual) and any sentience (human, AI, other).

Language is the gateway to cognition, and text is its most easily analyzable artifact. The current implementation starts with English text for that reason. The architecture is language-agnostic and modality-agnostic from the start.

## Software Stack

The stack is built from appropriately licensed open source tools that cover our early development needs. These were chosen for practical fit, not brand loyalty — if better-fitting tools emerge, the modular architecture allows swapping components.

### O3DE 25.10.2 — Module Management & Build Front End

**What**: Open 3D Engine (Linux Foundation / Open 3D Foundation). Apache 2.0 licensed.
**Where**: `/opt/O3DE/25.10.2/` (SDK install, pre-built binaries)

**Why O3DE**: Module management and build system front end. O3DE provides:
- **Asset Processor** — file watching, fingerprinting, caching, dependency tracking, parallel job processing, GUI dashboard, and headless batch mode
- **Gem system** — modular plugin architecture (our engine is the HCPEngine Gem)
- **Levels and Spawnables** — workspace management with dynamic entity spawning
- **EBus / AZ::Interface** — decoupled service communication between modules
- **SDK distribution** — contributors install the SDK + build just the Gem, no full engine compilation

### PhysX 5.1.1 — Physics Library

**What**: NVIDIA PhysX 5, GPU-accelerated physics. BSD 3-Clause licensed. Built into O3DE natively.
**Where**: `~/.o3de/3rdParty/packages/PhysX-5.1.1-rev4-linux/`

**Why PhysX**: Extensive library of physics functions — particle systems, constraint solvers, collision detection, force computation, GPU acceleration. Tokens are particles, bonds are constraints, linguistic rules are forces. PhysX has the breadth of functions to support this without writing everything from scratch.

**Note**: PhysX proves the concept for early development. A custom physics engine with ~65 linguist-defined core forces is a future contributor project. The architecture is designed so the physics layer can be replaced while everything above and below it stays.

### C++ — Engine Language

Driven by O3DE — it's a C++ engine, so our Gem code is C++. No one will argue its efficiency; it's just less forgiving than Python. All engine code (tokenizer, particle pipeline, storage, vocabulary) is C++. Python exists for tooling and scripts only.

### Databases — Purpose-Specific

We use multiple databases, each chosen for a specific role:

| Database | Role | Why |
|----------|------|-----|
| **PostgreSQL** | Source of truth | All persistent data — vocabulary, document positions, token definitions, namespace allocations, force patterns. Authoritative store. Everything else derives from it. |
| **LMDB** | Runtime read cache | Memory-mapped, zero-copy reads. Self-filling cache that starts empty, fills from Postgres on demand. NOT a persistence layer. |
| **SQLite** | Portable dump serving | Embedded, zero-server, ships with the tool. Serves database dumps for standalone tools that don't need the full power of Postgres. Not everything needs a server, and not needing one makes distribution easier. |

**Dev access**: PostgreSQL user `hcp`, password `hcp_dev`. Shards: `hcp_core` (AA namespace, 5K tokens), `hcp_english` (AB namespace, 1.4M tokens).

---

## Current State (compiles, runs end-to-end)

### What's Built

The HCPEngine Gem (`Gem/Source/`) contains:

| Module | File | Status |
|--------|------|--------|
| **Vocabulary** | HCPVocabulary.h/.cpp | Working — LMDB reader, 7 sub-DBs, affix loader (3696 entries, bucket-indexed) |
| **Tokenizer** | HCPTokenizer.h/.cpp | Working — 7-step resolution cascade with affix decomposition, dash splitting, var cache |
| **Particle Pipeline** | HCPParticlePipeline.h/.cpp | Working — PBD disassembly/reassembly/PBM derivation |
| **Bond Compiler** | HCPBondCompiler.h/.cpp | Working — compiles char→word (5,409 pairs) and byte→char (1,976 pairs) from Postgres |
| **Detection Scene** | HCPDetectionScene.h/.cpp | Working — physics-based token detection with PBD particles + PBM forces |
| **Storage** | HCPStorage.h/.cpp | Working — Postgres write kernel, position storage + PBM storage (StorePBM) |
| **Cache Miss Resolver** | HCPCacheMissResolver.h/.cpp | Working — LMDB miss → Postgres fill |
| **System Component** | HCPEngineSystemComponent.h/.cpp | Working — orchestrates all modules, self-test on Activate() |
| **Socket Server** | HCPSocketServer.h/.cpp | Working — TCP on port 9720 (health, ingest, retrieve, tokenize) |

### Test Results (all EXACT MATCH lossless round-trip)

| Text | Size | Tokens | Tokenize | Vars |
|------|------|--------|----------|------|
| Yellow Wallpaper | 52KB | 10,482 | 878ms | 49 |
| Alice in Wonderland | 174KB | 37,890 | 3.9s | 56 |
| Huckleberry Finn | 622KB | 141,135 | 15s | 159 |
| Tale of Two Cities | 807KB | 172,051 | 15.7s | 64 |

Build: `cd build/linux && ninja`

### Known Issues

- **Marker table PK collision**: Control tokens share `(t_p3, t_p4)` — needs t_p5 column. DB specialist task.
- **Self-test hardcoded**: Activate() runs self-test on every start. Will be replaced by Asset Builder pipeline.

---

## Development Phases

### Phase 1: Document Processing Workstation (current)

Replace the custom socket server ingest pipeline with O3DE's Asset Processor.

**HCPDocumentBuilder** — An Asset Builder that registers with the Asset Processor to handle `.txt` files. When text appears in a scan folder, the builder tokenizes it, encodes positions, and emits a position map product. PBM bonds are NOT stored — they're derived on the fly from positions (~20ms for 9K tokens, O(n) scan).

Key deliverables:
- [ ] HCPDocumentBuilder (CreateJobs / ProcessJob)
- [ ] Builder-side vocabulary (Postgres-backed, same interface as LMDB vocab)
- [ ] Position map product format (binary: token stream + base-50 positions + metadata)
- [ ] Builder CMake target (HCPEngine.Builders, links AssetBuilderSDK + libpq)
- [ ] Scan folder configuration for data/gutenberg/texts/
- [ ] Verification against current tokenizer output

This is the **reference implementation** for `.txt`. The architecture is modular so contributors can add builders for PDF, EPUB, HTML, Wikipedia dumps, etc. by following the same pattern. See TODO.md for format builder tasks.

### Phase 2: Document Reconstruction — COMPLETE

Lossless round-trip proven on 4 Gutenberg texts up to 807KB. Tokenize → position map → reconstruct → EXACT MATCH.

Completed:
- [x] PBM derivation function (DerivePBM — positions in, bond counts out, O(n) scan, ~16ms for 10K tokens)
- [x] Reconstruction from positions (lossless reproduction verified on 4 texts)
- [x] PBM storage (StorePBM — writes to hcp_fic_pbm: starters, word bonds, char bonds, var bonds)
- [x] Document-local var handling (decimal pair IDs, pbm_docvars, pbm_var_bonds)

Remaining:
- [ ] Position map reader as shared module (currently embedded in self-test path)
- [ ] Document inspector tool (view position maps, derive PBM, inspect structure)

### Phase 3: Boilerplate / Comparison Tool

First real PhysX particle pipeline use case beyond self-test.

Two documents laid side-by-side as parallel particle streams:
- **Same tokens attract** (unidirectional magnetic force across the gap)
- **Different tokens repel**
- **Spaces are soft-constrained** — allow sliding placement so phrases align even with extra/missing words
- **Convergence clusters = shared passages**

This detects boilerplate (Gutenberg headers, repeated prefaces) and shared content across documents.

### Phase 4: Vocabulary & Language Tools

- Vocabulary inspector — browse vocab, see coverage, find gaps
- Tokenization tester — interactive "paste text, see tokens" tool
- Language shard system — add new languages via vocabulary + force constants

### Phase 5: Conversation Levels

Levels = conversation workspaces. The root spawnable is the active context. Documents are assets spawned into the level via EntitySpawnTicket. Dynamic spawn/despawn/deactivate/reactivate without losing state.

- [ ] HCPDocumentAsset + Handler (runtime loading of processed documents)
- [ ] Document entity components (per-entity position maps, particle state)
- [ ] Level architecture (conversation workspace with dynamic document spawning)

### Phase 6: Custom Physics Engine

The real force model. ~65 core forces (categorization, axes, and definitions TBD — linguist-driven work). This is a significant engineering project for future contributors.

What stays: everything above (tokenizer, builders, position maps, vocabulary, PBM) and below (database, asset pipeline, levels). The physics layer is replaceable by design.

### Phase 7: Texture Engine

Forces are the rules of surface language — they drive both input (decompose surface to conceptual mesh) and output (wrap mesh into surface expression). Universal force categories live in hcp_core; language-specific constants live in language shards.

This is where linguistics meets physics. The linguist specialist defines the translation layer.

---

## Architecture Principles

1. **Engine IS the tokenizer** — all processing in C++/PhysX, not Python
2. **Disassembly AND reassembly are physics operations** — not sequential algorithms
3. **PostgreSQL is the source of truth** — LMDB is runtime cache only
4. **PBM is derived then stored** — position maps are the product, PBM bonds derived on the fly and stored to hcp_fic_pbm
5. **Build for refinement, not replacement** — every piece testable independently, extensible by contributors
6. **Single-purpose kernel modules** — each operation is one source of truth, stackable in any order the use case demands. The DI or user may need different combinations for different tasks.
7. **Bonds are directional** — "the->cat" and "cat->the" are different bonds with different counts
8. **Tokens are the internal working format** — text enters, becomes tokens, and stays tokens. Detokenization to text is an output flow, not part of the core processing loop.

### Kernel Modules (stackable operations)

Each module is a single-purpose operation. They can be stacked in different orders and combinations depending on the use case.

| Module | Input | Output | File |
|--------|-------|--------|------|
| **Tokenize** | text | TokenStream | HCPTokenizer.cpp |
| **Detokenize** | TokenStream + vocab | text | HCPDetokenizer.cpp |
| **DisassemblePositions** | TokenStream | PositionMap | HCPParticlePipeline.cpp |
| **ReassemblePositions** | PositionMap | TokenStream | HCPParticlePipeline.cpp |
| **DerivePBM** | TokenStream | PBMData (bonds) | HCPParticlePipeline.cpp |
| **StorePBM** | PBMData | DB writes | HCPStorage.cpp |
| **StorePositionMap** | PositionMap | DB writes | HCPStorage.cpp |
| **LoadPositionMap** | doc_id | PositionMap | HCPStorage.cpp |
| **ProcessJsonMetadata** | JSON + doc_pk | DB metadata | HCPJsonInterpreter.cpp |
| **StoreProvenance** | provenance fields | DB writes | HCPStorage.cpp |

Example stacks:
- **Ingest**: Tokenize → DisassemblePositions → StorePositionMap → DerivePBM → StorePBM
- **Retrieve as text**: LoadPositionMap → ReassemblePositions → Detokenize
- **Internal analysis**: LoadPositionMap → ReassemblePositions (stay in tokens)
- **Round-trip test**: Tokenize → DisassemblePositions → ReassemblePositions → Detokenize → compare

## Related Documentation

- `docs/research/source-workstation-design.md` — Source Workstation design (active)
- `docs/research/document-workstation-user-stories.md` — user stories by actor (profiles need refinement — early draft)
- `docs/research/tokenizer-redesign.md` — tokenizer pipeline design
- `docs/o3de_engine_architecture_research.md` — O3DE architecture mapping
- `docs/spec/namespace-reference.md` — token namespace structure
- `docs/spec/pair-bond-maps.md` — PBM specification
- `docs/spec/architecture.md` — system architecture
