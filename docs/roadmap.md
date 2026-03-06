# Roadmap

The HCP uses game and physics engines as database and calculation tools. Kernel speed and pure function are the primary goal, not pixel output.

## The goal

A full response inference engine built on physical structural principles -- not statistical weight matrices. Cognition modeled as mechanics, not curve-fitting.

The inference model is called **NAPIER**: *Not Another Proprietary Inference Engine, Really.*

(John Napier invented logarithms -- computational shortcuts that transformed impossible calculations into tractable ones. The same principle applies here: recognizing that the hard computation is already solved, just mislabeled as "physics engines.")

## Engine Stack

- **O3DE 25.10.2** -- primary game engine with native PhysX 5 integration
- **PhysX 5** -- GPU-accelerated PBD (Position Based Dynamics) for particle-based tokenization and resolution
- **PostgreSQL** -- persistent write layer, authoritative schema, ingestion
- **LMDB** -- pre-compiled read cache (vocab/entities), self-filling for envelopes
- **C++** -- engine code (O3DE Gem). Python for tooling/scripts only.

## Phase 1: Linguistic Engine

Creation of a linguistic interpretation database and engine using game engine kernel methods, with English as the first language example.

### 1. Text byte code PBMs -- COMPLETE
PBMs for byte code combinations. Phase 1 PBD superposition resolves bytes to Unicode codepoints. 100% settlement on all tested inputs.

### 2. Kaikki English dictionary -- COMPLETE
All of Kaikki English mapped as first language reference database. 1.4M tokens ingested and cross-referenced. Pre-compiled to LMDB (809K entries after morphological stripping).

### 3. Character to word and morpheme patterns -- COMPLETE
Physics-based char-to-word resolution via persistent VocabBeds. Inflection stripping with PBD existence check. Silent-e fallback. Morphological normalization (16-bit morph field). 94-98.5% resolution on full novel-length texts.

### 4. Grammar and sentence structure -- PARTIAL
Capitalization normalization as phase-boundary transform. Positional modifiers (morph bits + cap flags) in token position maps. PBM prefix tree storage with bond tables. Entity recognition (multi-word sequence matching). Grammar force patterns defined but not yet integrated into physics pipeline.

### 5. NSM decomposition of the dictionary -- PARTIAL
AA namespace populated with NSM primitives and structural tokens. ~65 core forces expected (categorization, axes, definitions TBD -- linguist-driven). Concept mesh decomposition research complete. Force pattern integration into engine awaiting core force definitions.

## Phase 2: Identity and Theory of Mind

Personality DB (seed + living layer), relationship DB, and Theory of Mind modeling.

- Seed management -- creation, storage, versioning
- Living-layer accumulation -- how the DI's self-model evolves
- Relationship structures -- the DI's social graph
- ToM via superposition -- NSM action scope delimiters (I, you, we) as superposition statements. One/thing discrimination gates agency. Communication = shaping ToM constructs.

**Design insight (2026-03-02)**: Theory of Mind is not a separate module -- it falls out of superposition mechanics. Same particles, different observation points, different resolutions.

## Phase 3: Full Text Inference

The complete response engine for language -- linguistic deconstruction and generation operating as a unified system. NAPIER as the orchestration layer.

- Activity envelopes for context management (schema built, mechanism ready)
- Staged cache management: LMDB (hot) -> Postgres (warm) -> Disk/LFS (cold)
- Envelope system = editable subconscious. NAPIER inspects and rewrites its own envelope definitions.

## Phase 4: Multi-Modality

Extend the same structural primitives beyond text to other forms of expression.

- Define mode namespaces for audio, visual, and other modalities
- Build covalent bonding tables for non-text formats
- Cross-modal conceptual mapping via shared NSM primitives

## Current Focus (2026-03-06)

- **Envelope-based variant loading** -- Wire variant DB entries (env_archaic / env_dialect / env_casual) with VARIANT morph bits (bits 12-15). Variant forms in DB; loading path not yet wired.
- **Label propagation** -- Restore firstCap on all instances if word appears as Label anywhere.
- **Entity LMDB recompile** -- Entity DB cleaned (2026-03-05); `compile_entity_lmdb.py` needs review for variant/morph category support before next compile.
- **Source Workstation** -- Packaging: installer for multiple configs, viewer first.

## Recently Completed (2026-03-04 – 2026-03-05)

- **V-1/V-3 variant normalization** (2026-03-04, `d59c2fa`) -- `TryVariantNormalize` in resolve loop. V-1 g-drop (`-in'`→`-ing`) and V-3 archaic (`-eth`). 12/12 dialect g-drops resolved in Sign of Four.
- **Entity data cleanup** (2026-03-05) -- `hcp_fic_entities` and `hcp_nf_entities` normalized in-place. 723 sequences verified via dry-run.

## Performance Reality

This is not computationally hard. Physics engines already solve harder problems at consumer hardware speeds.

99% of modern software is GUI on database functions. The enterprise world runs these operations through abstraction layers designed in the 1970s, requiring server farms for workloads that a gaming GPU handles casually. Physics engines are the most optimized database engines ever built -- they're just called "physics engines" because they process vertex positions instead of customer records.

The HCP operates on this insight: Token IDs and bond strengths are simpler data than collision meshes and particle systems. A gaming laptop has more than enough power to run cognitive modeling at real-time speeds. The bottleneck was never hardware -- it was recognizing that the tool already exists.

Shards are scene elements. Load the next configuration. Hot-swap context like texture streaming. The game industry solved memory management for exactly this use case decades ago.

**Current benchmark floor** (GTX 1070, 8GB, 15 SMs -- 2016 hardware): full novel (890KB, Dracula) processed in **28.6 seconds** with triple-pipelined GPU/CPU overlap. Modern hardware (RTX 4090: 128 SMs, 24GB) makes everything faster without code changes. Architecture decisions driven by correctness + scalability, not development hardware performance.
