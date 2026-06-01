# Source Workstation — Design Notes

**Date:** 2026-02-20
**Status:** Active design discussion with Patrick

## Overview

The Source Workstation is a linear database workstation for ingestion of source materials, metadata capture, encoding, and verified reconstruction. It operates in both GUI (O3DE Asset Processor) and CLI (batch) modes.

"Source" rather than "document" because the architecture supports stubs for other input streams beyond text.

## What Ingestion Must Do

1. **Fully encode the source** — tokenize, position-map, store
2. **Capture direct source metadata** — title, author, publication date, provenance, source format
3. **Capture secondary element metadata** — per-element annotations within the source (see below)
4. **Replicate everything from source on demand** — lossless round-trip of text + all metadata
5. **Display both compressed and full formats** — as a reference implementation, show the encoding and the reconstruction (as alternate views, not side-by-side)

## Secondary Metadata

Secondary metadata is structured data about elements within a source. Think of it as the cast list at the beginning of a play, or the proper noun glossary in a fantasy novel.

**Standard fields per source type**: Fiction has predefined categories (characters, places, objects, etc.). These are templates that come with the source type definition.

**Custom metadata from submitter**: The reviewer/submitter can propose additional metadata fields beyond the standard template. Example: a Klingon dictionary accompanying Star Trek source material, or a glossary of invented terms in a sci-fi novel.

**Who fills it in**: At MVP, metadata is filled by the human or agent reviewer. The assumption is that the entity choosing to encode something understands it well enough to submit metadata around it. If not, it's absent until someone does — that's part of the reconstruction of the record.

For well-known works (Gutenberg, etc.), an agent can fill standard metadata from training knowledge without reading the text. The review step is lightweight QA, not original research.

Automated classification comes later when the inference engine can self-classify.

## Why Fiction First

Fiction appears more complex but is simpler and more linear in reality. The entity space is self-contained — a novel defines its own characters, places, and rules. No external verification needed.

Non-fiction references the real world, which requires external entity resolution — a much harder problem.

Starting with Gutenberg (public domain) means no IP issues and metadata is easily researched or already in LLM training data.

## Three Data Models

Users choose on install and can adjust. The workstation code is the same — only the data fill strategy differs.

### Model 1: Full DB Download

Everything local. All vocabulary, all shards, all metadata. Heaviest install, fully self-contained. No network dependency after initial download.

### Model 2: Custom Assembly

Purpose-built local database assembled from query criteria. NOT "pick shards from a menu" — it's cross-cutting queries.

Example: Someone working on Old English wants all tokens across all shards that have a valid language tag for Old English, regardless of which shard they live in. The master Postgres assembles a SQLite file matching their work intent.

This is a materialized view exported as a portable SQLite, scoped to a specific purpose.

### Model 3: Core Only + Fill on Demand

Starts with hcp_core only (AA namespace — punctuation, control tokens, structural markers, NSM primitives). Core is non-optional and small (~2 MB dump). Everything else fills from master server API on cache miss, or operates in offline mode with gaps until resolved.

All three models use the same local SQLite for the workstation's working data. Different fill strategies, same tool.

## Cache Architecture

The same cache-miss pattern repeats at every layer:

| Layer | Local Store | Miss Target | Fill Mechanism |
|-------|-------------|-------------|----------------|
| **Runtime engine** | LMDB | Local Postgres | Cache miss → Postgres resolves → LMDB fills |
| **Workstation** | SQLite | Master API | Cache miss → API query → SQLite fills |
| **Future** | Local prediction | Background pull | Active envelope predicts needs → pre-warms cache |

Speeds are relative to load. API pulls take a moment, but as the system develops, predictive background pulls will pre-warm the cache based on the active work context.

## Master Server API

- Currently: API on the dev machine's PostgreSQL
- Eventually: dedicated server with more resources
- Purpose: serves vocabulary/metadata queries to remote workstations
- Contributors never need direct Postgres access or credentials — they hit the API

## Offline Mode

Offline mode is an extended var shunt. The same var_request mechanism that handles unknown words in normal operation handles everything in offline mode — just at full scale.

### Var Scale by Data Model

| Mode | Var frequency |
|------|--------------|
| Full DB, online | Rare — occasional unknown word |
| Custom assembly | Vars for anything outside working set |
| Core only, online | Vars on first encounter, resolved as you go |
| Core only, offline | Everything beyond core is var initially |

### How Offline Encoding Works

The system does its best to segment text into tokens using structural cues (spaces, punctuation, common patterns). Anything it can't resolve to a known token ID gets stored as a var with a provisional position in the map.

The position map is always built. The structure is always captured. Token IDs are deferred — vars are placeholders resolved during reconciliation when connectivity returns.

Reconciliation is the same operation regardless of scale: "here are unresolved chunks, give me proper IDs, update the map."

### Offline Var Discrimination (punctuation dedup)

In offline mode, punctuation leading/trailing new words can create duplicate vars. Two optimizations:

1. **On var creation** — fallback atomization around punctuation checks existing vars. Strip leading/trailing punctuation from the chunk and check if the bare form already exists as a var. "hello," doesn't create a new var if "hello" is already one — it decomposes to var("hello") + punctuation(",").

2. **On new var definition** — retroactively scan existing vars for matches. If "hello" becomes a distinct var, scan for "hello," / "hello." / "(hello)" etc. and decompose them into var + known punctuation.

The var pool is essentially a local vocabulary that grows during the session and gets the same discrimination passes as the real tokenizer. Self-refines as it goes, reducing the reconciliation workload.

## Input Format Standard

### Source + Metadata Pairing

Each source file has a companion JSON metadata file. The JSON is authoritative — it contains the reference to its source file.

```
texts/00345_Dracula.txt
texts/00345_Dracula.json    ← contains source filename reference + metadata

texts/00074_Tom_Sawyer.txt
texts/00074_Tom_Sawyer.json
```

- The JSON file is the authority for linking to the source, not filesystem naming conventions
- A JSON can reference a source file anywhere (different directory, different name)
- If no JSON exists for a source file, one is auto-generated with a matching filename
- The naming convention is the sensible default, not a requirement

### Gutenberg Example

Current Gutenberg data has two bulk JSON files (`metadata.json`, `metadata_batch2.json`) with per-book entries containing: Gutenberg ID, title, authors (name, birth/death years), subjects, bookshelves, languages, copyright status, download count, format URLs. The text files have the Gutenberg ID embedded in the boilerplate header (`[eBook #345]`). No direct link between JSON entries and local text files.

For the workstation, these bulk JSONs need to be split into per-source companion files with explicit source filename references.

### Directory Scan (batch processing)

Three-step sweep, same logic for CLI batch and monitored folder:

1. **Process paired files** — find all `.json` files, follow their source reference, process each pair
2. **Flag failures** — any `.json` whose source is missing → attempt fuzzy match (filename, title). Can't match → flag for correction during review. Skeleton JSON gets linked to proper source later.
3. **Process orphans** — remaining `.txt`/`.md` files with no `.json` → process standalone, auto-generate companion `.json` with matching filename

Everything in the folder gets handled. Nothing silently dropped.

### Ingestion is Separate from Review

Ingestion runs without gates — processes everything, does its best, outputs to a review queue. No interactive prompts during ingestion (unless CLI auto-accept is explicitly disabled for step-by-step confirmation).

All ingested items land in a **review queue** for approval. Review is a separate workflow, possibly by a different person, at a different time. Overnight batch fills the queue — reviewer works through it in the morning.

## Ingestion Workflow

```
1. Source arrives (drop file / CLI batch / monitored folder)
2. JSON pairing (match metadata to source, or generate skeleton)
3. Text extraction (format-specific — .txt is trivial, PDF/EPUB need libraries)
4. Tokenization + position encoding (automated — uses local vocab, vars for misses)
5. Direct metadata capture from JSON (title, author, etc.)
6. Storage (positions + all metadata — complete encoding)
7. → Review queue (all items await approval)
```

Steps 1-6 are automated. Step 7 is the handoff to review.

**Review workflow** (separate from ingestion):
- Work through queue in metadata editor
- Accept, correct, or reject each item
- Fill in secondary metadata (element definitions — characters, places, etc.)
- Resolve orphaned JSONs / unmatched sources
- Enrich skeleton metadata
- Verify reconstruction (popup)
- Submit via git (branch + PR)

## Processing: CPU Now, GPU Later

Tokenization and ingestion are CPU processes for now — more than sufficient for document-scale work. The O3DE Asset Processor runs multiple jobs in parallel across CPU threads automatically.

Current dev hardware: AMD FX-6100 (6 threads) + GTX 1070. Modest, but the project goal is to make full inference work on consumer hardware — not require data center resources.

GPU parallelization of tokenization is a future optimization for when the engine handles massive concurrent input streams (severed I/O, continuous data flow). The tokenizer API stays the same — only the execution backend changes.

## Vocabulary Backend

The vocabulary interface (LookupChunk, CheckContinuation, LookupChar) is backend-agnostic. Two implementations exist side by side:

- **SQLite** — always bundled (tiny), portable, no server
- **Postgres** (libpq) — installed as standard dependency

Both are always present. An internal flag controls which is active. User switches modes → flip the flag. No reinstall, no conditional logic. Each implementation is simple and self-contained.

At runtime, the engine uses LMDB (memory-mapped, fills from whichever DB backend is active). The workstation tools query SQLite or Postgres directly.

## Two Streams: Encode and Decode

Both streams are physics operations in the same engine, same scene. Not algorithms that use physics — the physics IS the operation.

### Encode Stream (Disassembly)

1. **Byte stream enters** — raw bytes from source, particles in the physics scene
2. **Rigid body detection** — PhysX broadphase/narrowphase matches bytes against vocabulary hierarchy (bytes → chars → words). Largest match wins. LoD stacking via hierarchical token naming. Encoding + language + tokenization in one pass.
3. **Unmatched → soft body → FEM** — FEM uses PBM bond forces (letter-level = spelling model) to reshape toward known patterns. Error correction, adaptation. Unresolvable → var system.
4. **Result: string of rigid bodies in the engine** — the source exists as a live physical state. Each significant token is a rigid body. Spaces are voids.
5. **Bifurcation** — every significant token (excluding single whitespace) splits into twin rigid bodies
6. **Binding** — each twin binds to its nearest neighbor, spaces squish out. Each bonded pair IS a dumbbell (PBM bond as a physical object).
7. **Stack and count** — identical dumbbells stack. Stack height = bond count. The set of stacks IS the per-document PBM.
8. **Metadata capture** from companion JSON
9. **Storage** — the PBM (stacked dumbbells) + metadata. This is the document.

All steps 1-7 are physics operations, embarrassingly parallel. A 9000-token document is a trivial physics scene — PhysX handles millions of particles at 60fps for games. Near-instantaneous on consumer hardware.

### Decode Stream (Reassembly)

1. **Mate finding** — each dumbbell finds another that shares a particle (broadphase matching). "the→cat" and "cat→sat" share "cat" → merge.
2. **Chain growth** — merging continues, building longer chains
3. **FEM resolution** — pulls the chain into the right shape (resolves ordering when a bond type has count > 1)
4. **Anchor** — stream_start token (AA.AE.AF.AA.AA) = position 0. stream_end = terminus. Chain grows between anchors.
5. **Whitespace reformation** — punctuation stickiness rules determine what's adjacent vs what has a void (space). Non-sticky boundaries become whitespace.

Lossless round-trip proven when forces resolve to a single stable state — the original document. Also near-instantaneous (same parallel physics operations in reverse).

**Decode is display-only by default.** The canonical record is immutable until a reviewed change is applied.

To edit: explicitly unlock → opens a delta file → enters editing mode. Edits are proposed changes, not direct modifications to the record.

## GUI Layout

The workstation GUI is built on O3DE's platform (Qt-based). The main workspace is the metadata editor — that's where the actual work happens. Text views are supporting popups.

### Main Workspace

- **Source queue / file list** — files waiting, in progress, completed, failed
- **Metadata editor** — THE workspace. Standard fields, custom fields, var resolution
- **Var list** — unresolved items needing attention

### Popups (on demand, dismiss when done)

- **"Show reconstruction"** — proof of lossless encoding. Display only. Not an editing panel — if you want to modify the source, modify the source file and re-ingest.
- **"Show encoded"** — flat representation of the tables and contents being created. Educational view showing what the data actually looks like. Not something to adjust at a source level.

### Decode / Curation View

- **Default**: display-only view of reconstruction + metadata as-is
- **Unlock to edit**: explicitly switch to edit mode → creates a delta file → changes tracked as proposals
- **Submit**: delta goes through git review workflow (see below)

### CLI Equivalents

```
source ingest <file>         # encode stream
source decode <file>         # reconstructed output (stdout)
source encoded <file>        # table structure tree (stdout)
source vars <file>           # list unresolved vars
source edit <file>           # create delta, open for editing
source submit <file>         # submit delta for review (creates branch + PR)
```

## Submission Flow: Git-Based

All submissions — source ingestion, metadata edits, var reconciliation — flow through git as branches with PRs.

### Why Git

- **Email notifications** on all submissions — built in
- **Review/approve/reject** workflow — built in
- **Full audit trail** — built in
- **Access control** via git permissions
- **Community discussion** on PRs
- No custom review system needed

### How It Works

1. User completes work (encodes source, edits metadata, resolves vars)
2. Workstation creates a git branch
3. Commits the delta (new source record, metadata changes, resolved vars)
4. Opens a PR
5. Reviewer approves/requests changes via standard PR workflow
6. Merge = canonical record updated

The GUI hides the git mechanics from non-technical users — they see "submit" and "approved/pending." The infrastructure is standard git all the way down.

Contributors who know git already know the workflow. Contributors who don't just use the buttons.

## Workstation Configuration

Simple flags, applied uniformly to all databases:

| Setting | Options | Notes |
|---------|---------|-------|
| **Updates** | yes / no | Pull from master server at all |
| **Mode** | live / scheduled | Real-time fill on demand, or batch sync at intervals |
| **Source server** | URL | Which API endpoint (default: master, could be local mirror) |
| **Catalog** | yes / no | Local cached copy of submission catalog for browsing |

Catalog connection is mandatory for reproduction of other works — you need it to see what's available. The yes/no is about local caching, not connectivity.

For encoding, the catalog duplicate check can happen later — encode locally, submit, the server calculates deltas against existing baselines at merge time.

### Version Storage

First submission of a source is the full baseline PBM. Any subsequent versions (different editions, corrections, updated metadata) are stored as PBM deltas — stacks added, removed, or count changed. No positional offsets needed.

Massive space savings at scale — instead of 50 copies of Shakespeare, one baseline PBM and 49 stack diffs. Delta size scales with actual changes, not document size. Reassembly physics resolves any version from baseline + applied deltas.

Same principle as git: baselines and diffs all the way down.

## MVP Scope

- .txt and .md ingestion and reproduction
- Single and batch modes (GUI + CLI)
- Standard fiction metadata templates
- Custom metadata field submission
- Reconstruction and encoded format views
- Delta-based editing with git submission
- Stubs for other input streams

## Module Breakdown (in progress)

Modules are compact, self-contained routines — each a candidate for GPU promotion. Module boundaries are future kernel boundaries. "Layer simple done right."

**Critical framing: physics is literal, not metaphorical.** These modules are regions of a physical flow system. Data flows through them as a continuous process. The physics engine doesn't simulate cognition — it IS the computation. Rigid body detection IS pattern matching. FEM resolution IS inference. Flow states ARE understanding.

### Physics-First Modules

| Module | Scope | Notes |
|--------|-------|-------|
| **Vocabulary (Rigid Body DB)** | Query router + DB endpoints (SQLite, Postgres). Single module, two implementations, flag switches. Vocabulary IS the rigid body database — every token at every LoD is a known rigid body definition. | Front-end routes by prefix: namespace → vocab DB, var.* → var DB. DB endpoints are data wrappers on the same API. |
| **Detection Scene** | THE core module. Replaces sequential tokenizer, encoding detection, and language discrimination. Byte stream enters as particles → PhysX broadphase/narrowphase detects matches against vocabulary rigid bodies → largest match wins → unmatched → FEM soft body → unresolved → var. One pass does encoding + language + tokenization. | CPU/GPU flag from day one. Same operations, same code. PhysX provides broadphase (SAP/ABP/GPU), narrowphase (BVH34), FEM soft body solver. |
| **Var System** | Receives "var.chunk" via front-end router, creates/finds var token, returns ID. Punctuation dedup back-check on new bare vars. Synchronous — detection scene always gets a token back. | Var DB = NAPIER's scratch pad / short-term memory. Workstation is one client. |
| **Position Encoder** | Encode (token stream → base-50 position map) and decode (position map → token stream). Pure math, single module. | Trivially parallelizable. |
| **Byte Stream Provider** | Per-format, pluggable. Extracts raw bytes from source format (.txt/.md now, stubs for PDF/EPUB/HTML). Does NOT decode — hands raw bytes to the detection scene. | The engine owns the full path from bytes to tokens. Format-specific part is just "get me the bytes." |
| **Metadata Input Reader** | JSON parser now. Thin layer feeding the metadata API. | External format → internal API translator. Other readers (YAML, web forms, direct API) feed same system. |
| **Metadata System** | Internal handling: standard templates per source type, custom fields, storage structure. Programmatic API. | All front-ends (JSON reader, GUI, CLI) are clients. |
| **Directory Scanner** | File discovery, 3-step sweep (paired → broken JSONs → orphans), ordering. | Serves CLI batch and monitored folder modes. |
| **Source Registration** | Assigns provisional source ID (var), tracks provenance. | Every source gets an identity. All new IDs are vars — master DB assigns permanent IDs at integration. |

### What Collapsed Into the Detection Scene

The physics-first approach eliminates several modules that were artifacts of sequential-algorithm thinking:

- ~~**Tokenizer (4-step pipeline)**~~ → Rigid body detection in the physics scene
- ~~**Language Discriminator**~~ → Falls out of detection (vocabulary shard that matches IS the language)
- ~~**Format Detector**~~ → Byte-level detection handles this (format patterns are rigid bodies too)
- ~~**Encoding Detection**~~ → Falls out of byte-level rigid body hierarchy (word → letter → byte code atomization with encoding variations). The match IS the encoding.

### Infrastructure Modules

| Module | Scope | Notes |
|--------|-------|--------|
| **Local Store** | Writes PBM (stacked dumbbells) + metadata to SQLite. All new doc_ids and new entities are vars locally. | Permanent IDs assigned only by master DB at integration (dupe check, reconciliation). |
| **Cache Miss Resolver** | Same pattern every layer (local miss → upstream query → local fill). | Single configurable resolver with endpoint config. Same logic, different targets. |
| **Master API Client** | Queries master Postgres for vocab/metadata. | Master DB = DI's core knowledge base / long-term memory. Centralized ingestion protects knowledge integrity. |
| **Reconstruction** | PBM → original text via physics reassembly (mate finding → chain growth → FEM → anchor → whitespace). Lossless proof. | Reassembly IS physics — same engine, reverse direction. Near-instantaneous. |
| **Ingestion Orchestrator** | Orchestrates full encode stream: scan → pair → byte extract → detection scene → bifurcate/bind/stack/count → store → queue. | |
| **Review Queue** | Tracks ingested items awaiting approval. | Gate between perception (workstation) and memory (master DB). |
| **Delta Manager** | Baseline PBM + stack deltas for versions. Stacks added/removed/count changed. No positional offsets. | Boilerplate = known PBM stacks + linking dumbbell. |
| **Git Integration** | Branch creation, commit, PR submission. | All submissions flow through git. GUI hides mechanics. |
| **Config Manager** | Workstation settings (updates, mode, source server, catalog, CPU/GPU flag). | All DBs same update cycle. |

### Architecture Notes from Module Discussion

**Data access is a flow router, not a monolithic layer.** The vocabulary module is a query front-end that inspects prefixes and dispatches to the right DB. Namespace prefixes (AA.*, AB.*, var.*) are routing signals. Each DB has its own data wrapper. The tokenizer queries and gets a token back — it doesn't know or care which DB answered.

**Var lifecycle.** Unresolved chunk → prepend "var." → front-end routes to var DB (strips prefix) → var DB creates/finds entry → returns token ID → tokenizer continues. New var token is now "known" for rest of session. Punctuation dedup back-check only runs when new bare var is created.

**Local vs master ID authority.** Workstation works entirely in var space for new content. Existing vocabulary has permanent IDs. New document IDs, new words, new entities are all vars locally. Can be marked "reviewed" but permanent ID assignment happens at master DB during integration (dupe check, multi-source reconciliation, ID space management). Git submission carries vars; merge process resolves to permanent IDs.

**Master DB = DI's core knowledge base.** Not just infrastructure — it's NAPIER's long-term memory. Centralized ingestion protects knowledge integrity. Review gate between var space and permanent ID space = gate between perception and memory.

**Physics-literal architecture.** Modules are regions of a flow system, not discrete pipeline stages. Data flows over the conceptual mesh (accumulated knowledge). Frequent patterns carve channels, new input deposits, erosion reshapes. PBM bonds are the flow channels. The engine runs continuously — mesh is actively shaped by flow.

**Rigid body hierarchy.** Everything is hierarchies of rigid bodies. Word → letters → byte codes. Engine detects at highest LoD first (rigid body detection). Decomposes only when needed. FEM resolves soft bodies (unresolved composites) to new rigid bodies. This is the inference mechanism.

**PBMs at every LoD level.** PBMs aren't just word-to-word bonds. The same mechanism applies at every level of the rigid body hierarchy:

| Level | What PBMs describe | Growth |
|-------|-------------------|--------|
| **Byte → Character** | Byte ordering within characters (encoding patterns) | Static — defined by encoding standards. Ships with hcp_core. |
| **Character → Word** | Letter ordering within words (spelling model) | Grows with vocabulary. New words = new letter-level bonds. |
| **Word → Sequence** | Token ordering in text (what we have now) | Grows with every document ingested. |
| **Sequence → Topic** (future) | Concept ordering patterns | Grows with conceptual understanding. |

Letter-level PBMs are the spelling model. Assembled from the vocabulary itself — every word IS a letter-level position map. After "t→h", "e" has a strong bond, "a" has a bond, "i" has a bond. Directional, counted, derived — same as word-level PBMs. Updates automatically as new words enter vocabulary.

**FEM uses PBM bonds as forces.** When "teh" enters as a soft body, letter-level PBMs provide the forces: "t→h→e" has strong bonds, "t→e→h" has weak bonds. The FEM solver uses bond strengths to reshape the soft body — letters want to rearrange to satisfy known patterns. Missing letters create pull (vacancy). Extra letters create repulsion. Same physics at every scale.

Nothing is unresolvable — even unknown patterns get characterized by their internal PBM bond structure. That structure either matches something later, or becomes a new rigid body.

**Static vs growing layers.** Byte-to-character is static bedrock (Unicode/encoding standards don't change). Everything above is alive — growing, eroding, reshaping with every new vocabulary entry and every document ingested. The static layer ships with hcp_core (~2MB, non-optional). The living layers are what the engine learns.

**Encoding inference (future).** The vocabulary structure (word → letter → byte code atomization with encoding variations) enables the engine to infer encoding from byte patterns without external detection. Same rigid body hierarchy — if word-level match fails, decompose to letter level, then byte level. Encoding falls out of the matching process. For MVP, trust declared encoding.

**GPU promotion path.** Each compact module is a candidate for GPU parallelization via O3DE. Modules that frequently run together will be consolidated into runtime kernels later. Simple modules promote; complex tangled ones don't. "Layer simple done right."

**Concurrent flow at scale.** At NAPIER maturity: thousands of input streams flowing simultaneously over multiple conceptual meshes (personal ToM + predictive ToM for each modeled entity). GPU parallelization is a requirement, not an optimization — the physics IS parallel.

**Tokenization IS physics — not a sequential algorithm.** (Key architectural decision, 2026-02-20)

The vocabulary is a rigid body database. Every token at every LoD level (byte sequences → characters → words → phrases) is a known rigid body. The input byte stream enters as particles. PhysX broadphase/narrowphase detects matches against the vocabulary. The largest match wins (rigid body detection at highest LoD). What doesn't match is a soft body candidate (FEM) or a var.

This means there is no separate "tokenizer module." There is a physics scene that processes input. Encoding detection, language identification, and tokenization are all one detection pass — the byte patterns that assemble into known characters that assemble into known words ARE the encoding and language. Wrong encoding doesn't produce matching rigid bodies.

The hierarchical token naming (AA.AB.AC...) IS the LoD definition. Each level of the name is a level of rigid body nesting. PBM bonds map directly to physics constraints between bodies.

PhysX provides the infrastructure: broadphase (SAP, ABP, GPU SAP), narrowphase (BVH34), FEM soft body solver (GPU-accelerated, tetrahedral meshes), PBD particle systems. O3DE doesn't wrap the soft body/FEM/particle APIs — we access PhysX directly in the Gem.

**CPU/GPU flag, not CPU-then-GPU migration.** Physics operations run on CPU or GPU from day one. Same operations, same code path. GPU flag detected on install. CPU mode is slower but fully functional. GPU mode parallelizes — multiple documents simultaneously on consumer hardware (e.g., GTX 1070). No minimum hardware threshold to function. Scaling is linear with hardware capability.

This is the core value proposition: current AI requires data center resources. HCP/NAPIER runs on consumer hardware because physics-based computation (rigid body detection, force computation) is fundamentally more efficient than statistical weight matrix multiplication. The Source Workstation is the first proof of this — document ingestion via physics on modest hardware.

## Open Questions

- Standard metadata field templates for fiction — what fields?
- PBM storage format — how are stacked dumbbells serialized? DB schema for PBM storage.
- API design for master server queries
- Reconciliation cycle UX — how does the user see/manage unresolved vars?
- Git repo structure for submissions — one repo for all sources? Per-collection?
- Delta file format — what does a PBM delta look like on disk?
- Cache miss resolver: single configurable module or per-layer?
- Physics scene mapping — how do bytes map to PhysX spatial positions for broadphase?
- Punctuation stickiness rules — which punctuation sticks to which side? Where defined?
- Position encoder module — still needed with PBM-first storage? Or is base-50 encoding only for the position map scaffolding?
- Letter-level PBM compilation — when/how are char→word PBMs assembled from vocabulary?
- Existing code transition — migrate HCPParticlePipeline to physics-first, or rebuild?
