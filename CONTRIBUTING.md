# Contributing to the Human Cognome Project

HCP accepts code, research, testing, documentation and architecture critique. The repository is in an active structural migration, so current contribution guidance differs from the older August 2026 layout.

## Start here

1. [Covenant](covenant.md)
2. [Charter](charter.md)
3. [README.md](README.md)
4. [REORGANIZATION.md](REORGANIZATION.md)
5. [engine/ARCHITECTURE.md](engine/ARCHITECTURE.md)
6. the README/plan/tests for the subsystem you intend to change

Agents should also read [AGENTS.md](AGENTS.md).

## Current architecture

The field-engine stack is:

```text
Taichi runtime
    ↓
physics engine
    ↑
engine harness
    ↑
future analyst functions
```

The harness is the physics engine's control surface. Analyst functions are future work and have not yet been designed.

Database/cache/record/WAL kernels are separate autonomous components whose system role is to support the future analyst and keep its working surfaces current. Inbox/outbox endpoints and scheduling form common kernel-network infrastructure; configuration/topology, serialization bridges and thread management are later implementation work.

## Where work is currently useful

- **Field-engine cleanup.** Continue separating the active `engine/field/` implementation into physics, harness, oracle, and validation responsibilities without changing behaviour. The earlier Taichi v0-staging generation is preserved under `archive/2026-09-taichi-v0-staging/` for reference.
- **Field-engine separation.** Isolate physics from harness controls without changing results, preserving CPU-oracle/GPU equivalence tests.
- **Kernel-network build/test work.** Keep the current `kernels/db_kernel/` bundle buildable while its architectural seams are made explicit.
- **Analyst-supporting data surfaces.** Database/cache/WAL work should improve the surfaces the future analyst will consume; it should not invent analyst reasoning.
- **Topology/bridge design.** Configuration, local-memory endpoint mapping, serialization/transmission bridges and lower-frequency activation remain future implementation areas.
- **Research and validation.** Research lives under `research/`; validation artifacts should remain distinguishable from runtime implementation.

## Technical standards

- **Preserve history.** No force-push/history rewrite for cleanup. Normal Git history is the recovery path for moved or removed current-tree files.
- **Separate structure from behaviour.** Prefer pure moves first, then refactor in a later commit with tests.
- **Tests on behavioural changes.** Field-engine changes should retain deterministic/oracle comparisons where applicable. C++ kernel modules should remain independently testable where their existing contracts require it.
- **Location blindness.** Kernel logic should not branch on whether a counterpart is local or remote; that belongs to topology/bridge infrastructure.
- **No speculative analyst implementation.** Interfaces may expose future analyst-facing seams, but the analyst layer itself is not yet defined.
- **Database safety.** Existing disposable test-database conventions must stay explicit. Do not run reset/drop operations against persistent project data.
- **Secrets stay out of Git.** Snapshot manifests may describe provenance/version context but not credentials or private data.
- **Generated data is not automatically source.** Keep reproducibility artifacts, test fixtures and authored code distinguishable.

## Branch and PR workflow

While draft PR #63 is the active integration surface:

1. branch from `integration/kernel-network-reorg` for changes touching the reorganized runtime/kernel paths;
2. make focused commits;
3. open the PR back against `integration/kernel-network-reorg` unless specifically coordinating promotion to `main`.

Once the reorganization is promoted, normal work returns to branches from `main`.

## Current repository map

```text
human-cognome-project/
├── engine/
│   ├── field/              # later Taichi field-engine work
│   └── field/              # active later Taichi field-engine work
├── kernels/
│   └── db_kernel/          # preserved DB/cache/WAL development bundle
├── network/
│   └── endpoint/           # shared local box/endpoint/scheduler substrate
├── research/
│   ├── field/
│   ├── ledger/
│   └── packages/
├── data/
│   └── postgres/snapshots/
├── docs/
├── extraction/
├── db/                     # legacy DB tooling/dumps pending disposition
├── archive/
└── review/
```

By contributing, you agree that contributions are licensed under AGPL-3.0 and governed by the [Covenant](covenant.md).
