# Human Cognome Project - Implementation Plan

## Overview

Build the Gedankenmodell of cognition: a system that performs structural decomposition and reconstruction of expression, not statistical simulation of output shapes.

**Core principle:** Explicit, traceable, reconstructable relationships grounded in semantic primitives - the "glass mind" alternative to probability shadows.

---

## Architecture

```
hcp/
├── core/                    # Foundation
│   ├── token_id.py          # Base-20 encoding, Token ID generation
│   ├── pair_bond.py         # FPB/FBR/PBM data structures
│   └── nsm_primitives.py    # 65 NSM primitives (mode-00)
│
├── atomizer/                # Input decomposition
│   ├── byte_atomizer.py     # Raw byte-level decomposition
│   ├── covalent_tables.py   # Unicode/ASCII bonding tables
│   └── tokenizer.py         # Byte -> Token promotion
│
├── storage/                 # Persistence
│   ├── schema.py            # SQLite/DuckDB schemas
│   ├── token_store.py       # Token registry
│   ├── bond_store.py        # PBM storage with FBR
│   └── compression/
│       ├── hcp_compress.py  # HCP semi-compression (common token ID extraction per spec)
│       └── normalize.py     # Standard compression on normalized PBM (isolated layer)
│
├── physics/                 # Interpretation engine
│   ├── engine.py            # Simulation coordinator
│   ├── rigid_body.py        # Known structures
│   ├── soft_body.py         # Unknown/error handling
│   ├── energy.py            # Energy minimization
│   ├── lod_manager.py       # Level of Detail stacking
│   └── forces/
│       ├── albedo.py        # Relevance/reflectivity
│       └── gravity.py       # Clustering/filtering
│
├── assembly/                # Reconstruction
│   ├── reconstructor.py     # PBM -> expression rebuild
│   └── validator.py         # Lossless verification
│
├── abstraction/             # Conceptual decomposition
│   ├── decomposer.py        # Token -> NSM paths
│   └── abstraction_meter.py # Layer counting
│
└── api/
    ├── cli.py               # Command-line interface
    └── demo.py              # MVP demonstration
```

---

## Technology Choices

| Component | Tool | Rationale |
|-----------|------|-----------|
| Language | Python 3.12 | Speed of development, iterate fast |
| Token storage | SQLite | Zero-config, proven, point lookups |
| Bond analytics | DuckDB | Columnar, aggregations on FBR |
| Physics/dynamics | OpenMM / Taichi | Molecular dynamics (OpenMM), fluid dynamics with GPU (Taichi) |
| Conceptual forces | Custom | Albedo/Gravity are HCP-specific |
| Tokenizer | Custom | Existing tokenizers don't do structural decomposition |
| Base-20 encoding | Custom | ~50 lines, foundational |
| CLI | Typer | Modern, type-hint based |
| Testing | pytest | Standard |
| Config | TOML | Human-readable, stdlib support |

**Note:** Quick implementations are acceptable, but nothing simulated - all components must be actual Gedankenmodell (functional execution of logic), not Phänomenmodell (output mimicry).

---

## Build Sequence

### Phase 1: Foundation
**Core data structures**

1. `core/token_id.py` - Base-20 encoder/decoder, TokenID dataclass
2. `core/pair_bond.py` - PairBond, FBR counter, PairBondMap
3. `storage/schema.py` - SQLite schema for token registry

**Deliverable:** Create Token IDs, form pair bonds, store/retrieve.

### Phase 2: Input Pipeline
**Text to tokens**

1. `atomizer/byte_atomizer.py` - Byte-level decomposition
2. `atomizer/covalent_tables.py` - Unicode mapping, bond strengths
3. `atomizer/tokenizer.py` - Byte sequence -> Token promotion

**Deliverable:** "Hello world" -> Token sequence -> stored PBM.

### Phase 3: Reconstruction
**Lossless round-trip**

1. `assembly/reconstructor.py` - PBM -> token sequence
2. `assembly/validator.py` - Hash verification

**Deliverable:** Text -> PBM -> Text with byte-perfect reconstruction.

### Phase 4: Physics Engine
**Error correction**

1. `physics/engine.py` - Pymunk integration, simulation loop
2. `physics/rigid_body.py` - Known token patterns
3. `physics/soft_body.py` - Unknown handling, boundary relaxation
4. `physics/energy.py` - Energy minimization (edit distance weighted by bond strength)

**Deliverable:** "helo wrold" -> physics -> "hello world"

### Phase 5: Conceptual Layer
**NSM grounding**

1. `core/nsm_primitives.py` - 65 primitives in mode-00
2. `abstraction/decomposer.py` - Token -> NSM path finding
3. `physics/forces/albedo.py` - Relevance scoring
4. `physics/forces/gravity.py` - Clustering

**Deliverable:** Word -> NSM decomposition with abstraction level.

### Phase 6: MVP Integration
**All components connected**

1. `api/cli.py` - Command-line interface
2. `api/demo.py` - Full pipeline demonstration

---

## MVP Definition

**"The Misspelled Sentence"** - demonstrates all components:

```
Input:  "The quik brwon fox jumps oevr the layz dog"

Process:
1. Atomize to bytes
2. Apply covalent bonding
3. Promote to tokens (unknowns -> soft bodies)
4. Create pair bonds, store as PBM
5. Physics: soft bodies find corrections via energy minimization
6. Reconstruct from PBM (lossless)
7. Show NSM decomposition for one word

Output:
- Corrected: "The quick brown fox jumps over the lazy dog"
- PBM format displayed
- Reconstruction verified (byte-perfect)
- NSM decomposition tree
```

**Success criteria:**
- All 256 byte codes handled
- Valid base-20 Token IDs with mode extraction
- Accurate FPB/FBR counts
- Byte-perfect reconstruction
- 3+ of 4 misspellings corrected
- At least 5 words have NSM decomposition paths
- Single CLI command runs full pipeline

---

## Pending Specification

**Random Seeds:** Will apply in interpretation and generation - controlled, purposeful randomness (not LLM-style chaotic sampling). Full spec TBD.

**Personality DBs:** To be discussed - mechanism for distinct ToM constructs.

---

## Risk Areas

### High: Physics Engine Design
The spec uses physics concepts metaphorically. "Energy" for tokens ≠ kinetic energy.

**Mitigation:** Define concrete algorithms:
- Energy = edit distance weighted by bond strength
- Minimization = gradient descent or simulated annealing
- Prototype on spelling correction first
- Expect 3-4 iterations on force system

### High: Lossless Reconstruction
PBM stores pairs + recurrence, but reconstruction may be ambiguous.

**Mitigation:**
- May need position hints or sequence numbers
- Prototype with simple sentences, find edge cases
- Refine PBM schema as needed (expect 2-3 iterations)

### Medium: NSM Completeness
No complete word->primitive mapping exists.

**Mitigation:** Use Wierzbicka/Goddard's published NSM primitive list as foundation. Manual mappings for ~100 common words. Sufficient for MVP, document as research area.

---

## Verification

After each phase:
1. Run pytest suite for that module
2. Manual CLI test of new capability
3. Verify integration with previous phases

Final MVP verification:
```bash
python -m hcp.api.demo "The quik brwon fox jumps oevr the layz dog"
```

Expected output shows: correction, PBM, reconstruction match, NSM decomposition.

---

## Critical Files (Implementation Order)

1. `hcp/core/token_id.py` - Everything depends on this
2. `hcp/core/pair_bond.py` - Core data model
3. `hcp/storage/schema.py` - Persistence layer
4. `hcp/physics/energy.py` - Risk area, needs iteration
5. `hcp/assembly/reconstructor.py` - Proves the concept works
