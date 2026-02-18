# Architecture

## Two-Engine Model: Texture vs. Mesh

The HCP separates expression from cognition:

- **Texture engine** — handles the surface form of expression (language, sound, image). Any distinct modality is computationally equivalent to a texture pack applied to the underlying cognitive engine. Expression rules determine what the result renders as.
- **Mesh/cognitive engine** — handles conceptual meaning, relationships, and dynamics. Conceptual forces define what the render must convey. This is the shared substrate across all forms of expression.

This separation means the same cognitive structure can be expressed through any modality, and different modalities can be compared at the conceptual level.

## Physics Engine as Computation Model

This is not analogy. Physics engines are specialized, high-speed database operations with optimized formulas for spatial relationships. The specific computations they optimize — collision detection, force propagation, rigid/soft body dynamics — map directly to the operations needed for cognitive modeling.

The HCP uses physics engine concepts literally:
- **Rigid bodies** — recognized token assemblies at a given LoD (a known word, a grammatical structure)
- **Soft bodies** — unrecognized or ambiguous assemblies that need flexible resolution
- **Forces** — transformation and indexing rules applied per-iteration to tagged tokens
- **Energy loss** — the cost metric for corrections and interpretations; lower energy = better fit

## Conceptual Forces

At the level where expression carries meaning (roughly word-level in most languages), every Token receives conceptual force designations:

**Albedo (reflectivity)** — indicates how strongly a Token represents a given concept in the current scope. High-albedo tokens are the most indicative of their tagged concept.

**Gravitational force** — provides clustering and filtering factors for assembling complex token strings from the available token space.

Forces are scope-dependent. A Token's force profile changes based on the current processing context.

## Level of Detail (LoD) Stacking

Input analysis and output construction use LoD stacking combined with energy-loss minimization:

1. At each LoD level, recognized structures are treated as rigid bodies composed of their atomic components.
2. Unrecognized structures convert to soft-body handling.
3. Energy-loss calculations (derived from aggregated PBM values) determine the most likely interpretation.
4. This repeats at each level, from byte codes up through words, phrases, sentences, and beyond.

The specific energy-loss calculations at each level are derived from the PBM statistics for that scope.

## Fluid Dynamics of Conceptual Mapping

Conceptual forces act as boundary conditions for particle structures. The assembly of LoD rigid bodies is treated as a fluid modeling state, enabling:

- Flow of linear time interaction patterns
- Multiple Theory of Mind (ToM) constructs operating simultaneously
- Dynamic rebalancing as new context arrives

## 3D Semantic Space

The three spatial dimensions that physics engines natively compute on are reassigned to semantic dimensions:

- **X: Context linearity** — position in discourse flow. Conversation history has geometric shape. Tangents literally branch away spatially.
- **Y: Concept fidelity** — how closely expression tracks the target concept(s). High = precise, low = approximate/drifting.
- **Z: Level of abstraction** — the LoD dimension. Bytes at the bottom, words above, phrases, clauses, discourse at the top.

Every spatial operation the engine provides becomes semantically meaningful in this space:
- **Proximity query** = "what's nearby in context, concept, AND abstraction level?"
- **Collision detection** = two things occupying the same contextual/conceptual/abstraction space — ambiguity, redundancy, or conflict
- **Clustering** = finding groups that are contextually related, conceptually aligned, and at the same LoD
- **Force propagation** = influence spreading through semantically structured space

## Particle Physics Model

The computational model is particle physics, not molecular dynamics. The distinction matters:

- **Molecular dynamics** assumes bonds are defined upfront. Structure is given, then evolved. Topology is frozen.
- **Particle physics** discovers structure through force interactions. Particles attract, cluster, crystallize. Structure **emerges**.

For inference — where the goal is to discover what bonds should form, not evaluate known ones — particle physics is the natural model. Assembly and disassembly are the same soft body operation running in opposite directions.

### Rigid/Soft Body Dynamics

Rigid body status is **contextual and reversible**, not permanent:

- **Rigid** = currently settled, recognized at this LoD level. A known word, a parsed phrase, a resolved meaning.
- **Soft** = unresolved, still finding its configuration. An unknown word, an ambiguous parse, an unclear concept.
- Rigid bodies can **go soft again** when later context challenges the resolution. A word you thought was correct gets contradicted by a subsequent sentence — it softens, re-resolves.
- Rigidity propagates upward through LoD levels: a misspelling resolves (character level) → word resolves (word level) → phrase ambiguity resolves (phrase level).
- Ambiguity at any level forces soft body behavior, progressively searching for resolution at that level and below.

### PBM Assembly/Disassembly via Soft Body Alignment

PBM reconstruction and decomposition are the same physical operation in opposite directions:

**Reconstruction (output):** Bond pairs from the PBM spawn as paired particles. Each pair looks for matches at both ends. When matching tokens meet, pairs merge at that point and separate from the other end. The separation event **is** the whitespace — it falls out of the physics rather than being inserted by rule. Punctuation particles are "sticky" — they suppress the separation gap. First and last tokens anchor the endpoints; the pairs fit together only one way.

**Decomposition (input):** Every token splits into two instances of itself — one as the right end of its left pair, one as the left end of its right pair. Keep them paired with neighbors. Count occurrences. The result is the PBM.

No force bonds are needed for basic reconstruction. This is simple soft body alignment — particles finding their matching configuration.

## Engine Architecture

The architecture is layered:

- **Orchestrator** (game engine) — scene management, spatial queries in the 3D semantic space, multiplayer infrastructure for ToM constructs, compute shader dispatch, optional visualization
- **Texture engine** (particle physics) — surface expression assembly/disassembly, PBM operations, soft/rigid body mechanics, spring forces, deformation
- **Mesh/cognitive engine** (TBD) — conceptual structure, semantic relationships, contextual force fields. Computational model still open.
- Both engines run in parallel, bidirectionally coupled through the orchestrator, at multiple LoD levels simultaneously

The orchestrator's rendering pipeline and the inference computation operate on the same 3D semantic space. Whether that space is rendered to a display is an independent choice — the engine always computes as if rendered, a viewport is just an output tap that can be opened or closed. The GPU should not pay the pixel rendering tax when no display is attached; compute shaders handle the heavy lifting independently of the rendering pipeline.

The engine layer is interchangeable above the data. PBMs live in PostgreSQL (write layer) and LMDB (read/draw layer). The interpretation layer between data and engine is the stable interface — engine implementations can be swapped underneath without affecting the data or each other.

### Theory of Mind via Multiplayer

Each ToM construct is a concurrent simulation — not a data record about another entity, but an active model of their cognitive process that produces predictions. The game engine's multiplayer infrastructure maps to this:

- Each ToM perspective = a "player" with its own physics state, its own envelope of active structures
- Per-node authority models "who controls what"
- Visibility filters control what each perspective sees — literally modeling communication vs. internal state
- Collision detection between ToM simulations = conceptual conflict, disagreement, or points requiring negotiation
- The delta between what a ToM simulation predicted and what actually happened is the learning signal

ToM requires **maintained divergence** — each perspective deliberately maintains a different view of shared state. The gap between models is the useful information.

## No Engine Lock-In

The architecture describes computational operations, not a specific implementation. The interpretation layer between data and engine is the portable interface — engine implementations underneath can be swapped. Current exploration: PhysX 5 for particle physics (texture engine), O3DE for orchestration, NVIDIA Warp available for custom GPU kernels if needed. These are current best options, not commitments.

## Why This Is Fast

Physics engines are database engines. The game industry spent decades building the fastest parallel data processing systems on the planet — they just called them "physics engines" because they were querying polygon positions instead of customer records.

The operations are identical:
- **Collision detection** = spatial query
- **Force propagation** = relational update
- **Rigid body grouping** = join operation
- **LoD management** = query optimization
- **Particle systems** = batch insert/update

Consumer GPUs run millions of particles at 60+ fps, translating into millions of pixels — on a $400 card, not server hardware. The entire enterprise database industry runs on server farms to achieve what a gaming laptop does casually for water ripples.

Token lookup is a hash table hit. Atomization traversal is pointer chasing. PBM construction is pair counting. None of this approaches the computational density of soft body deformation or fluid dynamics. The HCP is feeding simpler data into engines already optimized for harder problems.

The performance ceiling for cognitive modeling isn't future hardware — it's already in every gaming PC, every phone with a GPU, every console. The bottleneck was never compute. It was recognizing what tool the problem actually needed.
