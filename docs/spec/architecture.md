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

## No Engine Lock-In

The architecture describes computational operations, not a specific implementation. The Python prototype in `work/` implements core data structures. Physics engine integration (Godot, Rapier, custom) is exploratory — the architecture must remain engine-agnostic.
