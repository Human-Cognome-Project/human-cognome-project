# Human Cognome Project — Roadmap

## The Project

The Human Cognome Project is mapping the fundamental geometry of cognition through genomic style analysis of language and other forms of expression with the goal of creating fully structured cognitive models with healthy, stable auditable Theory of Mind and decision making processes.

By creating a compressed, fully reproducible, content agnostic, source storage and decomposition structure, computational tools like physics engines can represent cognition modeling as systems of physical forces acting on a conceptual mesh (the meaning of any element) and the texture layer (the specific language or other expression) that covers it.

The core functions of the resulting model draw heavily from genomic sequence mapping, which many will recognize more easily as current LLM functions at the most basic internal levels.

Language is the gateway to cognition, and text is its most easily analyzable artifact. Current development uses written English expression as the initial analysis point. The architecture is language-agnostic and modality-agnostic from the start.

The project is currently developing a portable method for ingestion of source materials and metadata, and generation of project compliant data files. The MVP for this aspect will address .txt and .md ingestion and reproduction in single and batch modes.

Immediately following that release, modeling of conceptual factors as physics forces will begin. The goal of this phase will be analysis and understanding of content and intent of an input source.

After development of the conceptual understanding framework, a formal Theory of Mind and inference engine will be incorporated into the front end as the core of the NAPIER (Not Another Proprietary Inference Engine, Really!) Digital Intelligence architecture.

NAPIER is where that theory becomes working code. All forms of expression decompose into tokens (particles), bonded by explicit pair relationships (PBMs), governed by context specific physics derived probability statistics — not generalized, frozen, massively averaged statistical weights. Lossless reconstruction proves comprehension. The same structural framework applies to any modality (text, audio, visual) and any sentience (human, AI, other).

## Software Stack

The stack is built from appropriately licensed open source tools that cover early development needs. These were chosen for practical fit, not brand loyalty — if better-fitting tools emerge, the modular architecture allows swapping components.

### O3DE 25.10.2 — Module Management & Build Front End

**What**: Open 3D Engine (Linux Foundation / Open 3D Foundation). Apache 2.0 licensed.

**Why O3DE**: Module management and build system front end. O3DE provides:
- **Asset Processor** — file watching, fingerprinting, caching, dependency tracking, parallel job processing, GUI dashboard, and headless batch mode
- **Gem system** — modular plugin architecture (our engine is the HCPEngine Gem)
- **Levels and Spawnables** — workspace management with dynamic entity spawning
- **EBus / AZ::Interface** — decoupled service communication between modules
- **SDK distribution** — contributors install the SDK + build just the Gem, no full engine compilation

**Future**: O3DE is the current front end. Any engine with comparable module management, asset processing, and plugin architecture could serve the same role. The HCPEngine Gem is the project's code — O3DE is the harness that runs it.

### PhysX 5.1.1 — Physics Library

**What**: NVIDIA PhysX 5, GPU-accelerated physics. BSD 3-Clause licensed. Built into O3DE natively.

**Why PhysX**: Extensive library of physics functions — particle systems, constraint solvers, collision detection, force computation, GPU acceleration. Tokens are particles, bonds are constraints, linguistic rules are forces. PhysX has the breadth of functions to support this without writing everything from scratch.

**Future**: PhysX proves the concept for early development. A custom physics engine with ~65 linguist-defined core forces is a future contributor project (see Phase 7). The architecture is designed so the physics layer can be replaced while everything above and below it stays. Any physics library that supports particle systems, constraints, and custom force functions could serve.

### C++ — Engine Language

Driven by O3DE — it's a C++ engine, so our Gem code is C++. All engine code (tokenizer, particle pipeline, storage, vocabulary) is C++. Python exists for tooling and scripts only.

**Future**: The engine language follows the engine front end. If the front end changes, the implementation language may change with it. The algorithms and data structures are what matter — they're documented independently of language.

### Databases — Purpose-Specific

Multiple databases, each chosen for a specific role:

| Database | Role | Why | Replaceable? |
|----------|------|-----|-------------|
| **PostgreSQL** | Source of truth | All persistent data — vocabulary, bonds, token definitions, namespace allocations, force patterns. Authoritative store. Everything else derives from it. | Any relational DB with comparable query capability. Schema is portable SQL. |
| **LMDB** | Runtime read cache | Memory-mapped, zero-copy reads. Self-filling cache that starts empty, fills from Postgres on demand. NOT a persistence layer. | Any memory-mapped key-value store with sub-database support. |
| **SQLite** | Portable dump serving | Embedded, zero-server, ships with the tool. Serves database dumps for standalone tools that don't need Postgres. | Any embedded DB. SQLite is hard to beat here. |

### Python — Tooling Only

Python handles ingestion scripts, reference implementations, and test infrastructure. It is NOT the runtime — all runtime processing is C++ in the engine.

**Future**: Tooling language is flexible. Contributors can write ingestion tools, format builders, and test scripts in whatever works. The interfaces (database schemas, file formats, token conventions) are what matter.

## Current Focus

English text, fiction first. Narrow channel to prove the core architecture before expanding.

**Why fiction first**: Fiction exercises the full range of language — dialogue, narrative, description, emotional register, structural variety. It also has agreed, well-documented conceptual shapes (narrative arcs, character relationships, story structures) with more clearly delineated relationships between elements than non-fiction or opinion-based works. This gives us concrete structural targets to validate against. Non-fiction follows the same pipeline but typically uses a narrower band of expression and has looser, more debatable structural relationships. If the system handles fiction faithfully, non-fiction is a subset.

**Why text first**: Text is the most easily analyzable artifact of language. Audio, visual, and other modalities use the same structural primitives (tokens, bonds, forces) but require additional front-end processing before entering the core pipeline.

**On input formats**: The engine ingests raw byte code streams and assembles them bottom-up — bytes to characters to words to larger structures — using whatever Unicode tables and token vocabulary it can recognize. In theory any input can be taken in now. "Supporting a new format" is primarily a vocabulary task: ensuring the tokens exist so the DI can recognize the byte patterns that format introduces, not building format-specific parsers.

**Inference as co-development**: Although full integration as the NAPIER system is a long-term project (Phase 9), forward inference elements will be tested and integrated as the engine and work support it. Inference is how we test conceptual mesh understanding — force mapping and inference are a co-development loop, not sequential phases. Expect inference experiments alongside any phase where force definitions or conceptual structure are being validated.

**On inference itself**: The core inference processes are not novel — they are the same weighted selection operations that current LLMs perform, cleaned up and reorganized for physics-native tools. The difference is structural. Current models apply a single massive statistical average across frozen weights. In reality, each token placement dynamically reshapes the constraints on every other token — some positions are fully determined by the conceptual shape being conveyed, others are loosely constrained and open to variation (style, personality, register). Not only the domain but the specific position of every token changes the parameters on every other one. A physics engine models this naturally: each particle exerts forces on all others, and the constraint landscape updates with every placement. This is why inference is one of the easier elements to develop once the structural representation is correct — the physics engine already does parallel constraint solving.

## Development Phases

### Phase 1: Source Workstation MVP (current)

A portable tool for ingesting source materials (.txt, .md), generating project-compliant PBM data files, and managing associated metadata. The GUI is primarily a metadata management interface — PBM maps and reconstructed content are on-demand output items, not the main workspace. Serious throughput runs via CLI and monitored directories with structured input files — O3DE's Asset Processor provides file watching, fingerprinting, and headless batch mode natively.

**Key deliverables:**
- Source workstation — ingest, tokenize, store, review documents
- PBM prefix tree storage (migration 011 — done)
- Self-filling LMDB cache with C++ resolver (done)
- Bond compiler for sub-word PBMs (char-to-word, byte-to-char — done)
- Physics detection scene for token resolution (working prototype)
- Lossless round-trip proof (pending)
- HCPDocumentBuilder via O3DE Asset Processor (pending)

### Phase 2: Document Comparison and Boilerplate Detection

First real multi-document PhysX use case. Two documents as parallel particle streams — same tokens attract, different tokens repel, convergence clusters reveal shared passages. Detects boilerplate (Gutenberg headers, repeated prefaces) and content overlap.

### Phase 3: Vocabulary and Language Tools

Inspectors, testers, vocabulary management. Contributor tooling for expanding language and format coverage.

### Phase 4: Grammar and Sentence Structure

Incorporate grammar construction rules. Part-of-speech bonding behaviour, syntactic patterns. Linguistics-driven force definitions (~65 core forces expected).

### Phase 5: NSM Decomposition

Decompose dictionary through abstraction layers to Natural Semantic Metalanguage primitives. Populate the conceptual mesh: AA.AA.AA.AB.* (65 atoms) and AA.AC.* (~2,719 molecules).

### Phase 6: Conversation and Dynamic Documents

Dynamic document spawning, activity envelope management, real-time interaction. O3DE Levels as conversation workspaces.

### Phase 7: Custom Physics Engine

Replace PhysX with a purpose-built physics engine implementing the full ~65+ linguist-defined core forces. Texture engine (surface language rules) + Mesh engine (conceptual structure). Everything above (tokenizer, builders, PBM, vocabulary) and below (databases, asset pipeline) stays — the physics layer is replaceable by design.

### Phase 8: Identity and Theory of Mind

Personality DB, relationship DB, ToM modeling. Multiple concurrent ToM constructs.

### Phase 9: Full Inference (NAPIER)

Complete response engine — deconstruction and generation as a unified physics system. Context specific physics derived probability, not frozen statistical weights.

### Phase 10: Multi-Modality

Extend structural primitives beyond text. Mode namespaces for audio, visual, and other expression types. Cross-modal mapping via shared NSM primitives.

## Contributor Expansion Points

The architecture is designed for contributor expansion at every level. Current development is narrow (English text) but the framework supports broad expansion. Note: many contributor tasks here are analytical, not implementation. The engine handles the processing — contributors identify patterns, define token vocabularies, and recognize structural shapes. People who think sideways and see deltas are more valuable than people who write parsers.

| Area | Current State | What's Needed | Difficulty |
|------|--------------|---------------|------------|
| **New format vocabulary** (PDF markup, HTML tags, Markdown syntax, etc.) | Engine ingests raw bytes now | Token definitions for format-specific byte patterns so the engine can recognize them. The engine does all detection via FEM — no format-specific parsers needed. | Easy-Medium |
| **Other languages** | Architecture ready, no implementation | Language shard vocabulary + sub-categorization patterns + force constants. The engine handles language detection natively — new languages need tokens and force rules, not detection code. | Medium-Hard |
| **Entity cataloging** | 110 texts cataloged, framework ready | Apply librarian pipeline to new texts | Easy |
| **Force definitions** | ~65 expected, categorization TBD | Linguistics expertise | Hard |
| **Test coverage** | Needs expansion | Edge cases, Unicode, adversarial inputs | Easy-Medium |
| **Physics engine** | PhysX proving concept | Custom engine with core forces (Phase 7) | Hard |
| **Other modalities** | Architecture ready, no implementation | Front-end processors + mode namespaces (Phase 10) | Hard |

## Area Roadmaps

Detailed roadmaps and task lists for each area:

| Area | Roadmap | Tasks | Agent Guide |
|------|---------|-------|-------------|
| **Engine** | [hcp-engine/ROADMAP.md](hcp-engine/ROADMAP.md) | [hcp-engine/TODO.md](hcp-engine/TODO.md) | [hcp-engine/AGENTS.md](hcp-engine/AGENTS.md) |
| **Database** | [db/ROADMAP.md](db/ROADMAP.md) | [db/TODO.md](db/TODO.md) | [db/AGENTS.md](db/AGENTS.md) |
| **Python tools** | [src/hcp/ROADMAP.md](src/hcp/ROADMAP.md) | [src/hcp/TODO.md](src/hcp/TODO.md) | [src/hcp/AGENTS.md](src/hcp/AGENTS.md) |
| **Specifications** | [docs/spec/](docs/spec/) | — | — |

## Performance Reality

Physics engines are the most optimized database and calculation tools ever built. Token IDs and bond strengths are simpler data than collision meshes and fluid dynamics. A gaming laptop runs cognitive modeling at real-time speeds. The bottleneck was never hardware — it was recognizing the tool already exists.

**Current benchmarks** (GTX 1070, 8GB VRAM — mid-range consumer GPU):
- *The Yellow Wallpaper* (~10,451 tokens): ~121ms full assembly/disassembly, 2 content misses (both legitimate — a Latin loan word and an unregistered possessive form)
- That 121ms includes cold-start LMDB — every token backfilled from Postgres on first encounter. LMDB persists between documents, so subsequent texts only hit Postgres for genuinely new vocabulary. Warm-cache performance is substantially faster.
- Projected *War and Peace*: ~1 second cold, likely faster warm
- This is on hardware PhysX considers entry-level — the data is simpler than what it was designed for
