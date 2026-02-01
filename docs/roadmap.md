# Roadmap

## Phase 1: Foundation (current)

Establish the core data structures and prove they work on real data.

- Stabilize Token ID scheme and PBM storage format
- Ingest a real word list and build letter-level bonding tables
- Demonstrate PBM reconstruction for simple text
- Define the ~65 NSM primitives as first-class tokens
- Get external review of the spec

### MVP definition
A working system that can:
1. Accept a text input
2. Tokenize it to the defined Token ID scheme
3. Build a PBM for the input
4. Reconstruct the original text from the PBM
5. Demonstrate basic error correction (misspelled word → lowest energy-loss match)

## Phase 2: Conceptual Layer

Add meaning on top of structure.

- Implement conceptual force tagging (albedo, gravity)
- Build abstraction-level computation (Token → NSM primitive depth)
- Integrate physics engine for force-based assembly (engine TBD)
- LoD stacking with real energy-loss calculations

## Phase 3: Multi-Modality

Extend beyond text.

- Define mode namespaces for audio, visual, and other modalities
- Build covalent bonding tables for non-text formats
- Cross-modal conceptual mapping via shared NSM primitives

## Open Questions

- Which physics engine, if any? Godot, Rapier, custom, or pure mathematical model?
- How are conceptual forces quantified? What are the units?
- What is the right granularity for LoD levels?
- How does temporal flow (linear time interaction) integrate with the PBM model?
- How do we validate that the system captures "real" cognitive structure vs. imposing one?
