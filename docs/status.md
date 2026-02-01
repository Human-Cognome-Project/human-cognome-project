# Project Status

_Last updated: 2026-02-01_

## What Exists

### Governance (stable)
- **Covenant** — Founder's Covenant of Perpetual Openness. Ratified.
- **Charter** — Contributor's Charter. Ratified.
- **License** — AGPL-3.0, governed by the Covenant.

### Specifications (first draft)
- **Data conventions** — core definitions, PBM storage algorithm, atomization, NSM decomposition
- **Token addressing** — base-20 scheme, 5-pair dotted notation, reserved namespaces
- **Pair-bond maps** — FPB/FBR structure, reconstruction, compression, error correction
- **Architecture** — two-engine model, conceptual forces, LoD stacking, fluid dynamics framing

These are first-pass specs derived from working notes. They need review, critique, and refinement.

### Python prototype (`work/hcp/`)
- `core/` — TokenID, PairBond, NSM primitives, byte codes
- `storage/` — TokenStore, BondStore, schema definitions
- `abstraction/` — Decomposer, AbstractionMeter
- `assembly/` — Reconstructor, Validator
- `atomizer/` — Tokenizer, covalent tables
- `api/` — CLI, browser demo
- `tests/` — Core and integration tests

The prototype implements basic data structures. It is exploratory and not stable.

### Godot exploration (`work/godot/`)
- Initial exploration of Godot as a physics engine for the cognitive model. Not committed to.

## What Doesn't Exist Yet

- Consensus on physics engine choice
- Working PBM reconstruction from real data
- Any external data sources integrated (Kaikki, word frequency lists, etc.)
- NSM primitive mappings beyond stub definitions
- Multi-modality support (only text is partially addressed)
- Community infrastructure (issue templates, CI, discussion forums)
- Performance testing or benchmarks
