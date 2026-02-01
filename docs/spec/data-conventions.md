# HCP Data Conventions

## Preamble

The data conventions of the HCP must be expansive, interpretable, computationally coherent, substrate-agnostic, and universally applicable. All forms of expression are treated as structures that can be decomposed into common elements — analogous to molecular or genetic structures — permitting analysis, comparison, recreation, and synthesis across all modalities.

## Core Definitions

**Token** — A discrete element assigned computational relevance within the system.

**Token ID** — The namespaced identifier for a stored Token. See [token-addressing.md](token-addressing.md) for the full scheme.

**Mode / Modality** — Primary identifier of expression structure and processing path (text, audio, visual, conceptual, etc.).

**Dimension** — An atomic element of a Token inherent to its processing.

**Scope** — The current envelope of operation: a document, a sentence, a song, a lyric, a physics LoD envelope.

**Forward Pair-Bond (FPB)** — A record of any distinct ordered combination of two tokens as they appear in the scope being processed. The fundamental unit of structural storage.

**Forward Bond Recurrence (FBR)** — The count of how many times a specific FPB appears in the scope being processed.

**Pair-Bond Map (PBM)** — The complete data schema for a stored expression (document, utterance, or other scoped structure). See [pair-bond-maps.md](pair-bond-maps.md).

**Conceptual Force** — Labels attached to a Token that designate interaction and assembly patterns in the physics engine. Configuration and strength are scope-dependent. See [architecture.md](architecture.md).

## Structural Storage Algorithm

All stored structures follow a common notation to ensure exact reproduction, cross-compatibility, and efficient storage. For each scope, a PBM is stored:

```
TokenID(0).TokenID(1).FBR
```

Where:
- `TokenID(0)` = current token
- `TokenID(1)` = next relevant token (whitespace and structural non-meaning-bearing tokens excluded; formatting tokens are always relevant)
- `FBR` = recurrence count of this specific pair in the scope

This enables lossless reproduction via molecular reassembly — a complete scoped structure can be replicated from its PBM. Aggregated PBM values provide context-specific guidance on structural cohesion.

**Compression note:** Common TokenID elements within a PBM can be factored out and assumed, reducing storage overhead.

## Atomization and Covalent Bonding

All Tokens atomize to 256 byte codes at the lowest level. For each modality, a covalent bonding table maps byte codes to the modality's format table (Unicode/ASCII for text, frequency tables for audio, etc.).

Spelling correction and fuzzy input interpretation use bonding-strength guides derived from analyzing complete word lists as scoped structures. See [architecture.md](architecture.md) for the energy-loss minimization approach.

## NSM Primitive Decomposition

Every form of human expression decomposes through layers of abstraction to ~65 Natural Semantic Metalanguage (NSM) primitives. These are fundamental enough that most non-human communication can also be expressed as NSM combinatorics.

Each Token has a computable abstraction level: the number of combinatoric layers between it and NSM primitives. Higher abstraction implies less inherent confidence without sufficient lower-abstraction foundations.

NSM primitives occupy the `00`-mode namespace alongside byte codes and other universal/computational elements.

## Token Data Elements

**Expression Tokens** are any Tokens in the texture layer — the final presented form of a concept. An expression Token can contain any number of sub-Token rigid-body assemblies for LoD scaling.
