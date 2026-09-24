# Contributing to the Human Cognome Project

HCP accepts code, research, testing, documentation and architecture critique. The initial structural migration is complete; current guidance reflects the recovered native C++ engine and separated kernel/network architecture.

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

- **Native engine validation and harness evolution.** Preserve the C++ `engine_support` / `field_core` boundary, vet the recovered native field implementation against the settled design record, and continue developing the wider harness as the future analyst's engine control surface. The planner Python prototype is archived and must not become runtime code.
- **Kernel-network build/test work.** Keep `kernels/database/`, `kernels/wal/`, and `network/endpoint/` independently buildable while their interfaces stabilize.
- **Analyst-supporting data surfaces.** Database/cache/WAL work should improve the surfaces the future analyst will consume; it should not invent analyst reasoning.
- **Topology/bridge design.** Configuration, local-memory endpoint mapping, serialization/transmission bridges and lower-frequency activation remain future implementation areas.
- **Research and validation.** Research lives under `research/`; validation artifacts should remain distinguishable from runtime implementation.

## Technical standards

- **Preserve history.** No force-push/history rewrite for cleanup. Normal Git history is the recovery path for moved or removed current-tree files.
- **Separate structure from behaviour.** Prefer pure moves first, then refactor in a later commit with tests.
- **Tests on behavioural changes.** Native engine changes should retain CPU/backend equivalence and model-anchored regression checks where applicable. C++ kernel modules should remain independently testable where their existing contracts require it.
- **Location blindness.** Kernel logic should not branch on whether a counterpart is local or remote; that belongs to topology/bridge infrastructure.
- **No speculative analyst implementation.** Interfaces may expose future analyst-facing seams, but the analyst layer itself is not yet defined.
- **Compiled runtime path.** HCP engine/data/network hot paths are C++. Python is permitted only for bootstrap, migration, import/export, build/test assistance, or comparable offline convenience.
- **Database safety.** Existing disposable test-database conventions must stay explicit. Do not run reset/drop operations against persistent project data.
- **Secrets stay out of Git.** Snapshot manifests may describe provenance/version context but not credentials or private data.
- **Generated data is not automatically source.** Keep reproducibility artifacts, test fixtures and authored code distinguishable.

## Branch and PR workflow

1. branch from current `main`;
2. make focused commits;
3. open the PR back against `main`, unless a named recovery/integration branch is explicitly coordinating a larger import.

## Current repository map

```text
human-cognome-project/
├── engine/
│   ├── src/engine/         # native Taichi runtime/compiler wrapper
│   ├── src/field/          # native field/tick mechanics
│   ├── tests/              # native engine/field/index-cap regression tests
│   ├── docs/               # recovered engine/harness design + vetting record
│   └── taichi/             # modified Taichi fork (curated separately)
├── kernels/
│   ├── database/           # PostgreSQL record + database/cache-manager family
│   └── wal/                # WAL manager family
├── network/
│   └── endpoint/           # shared local box/endpoint/scheduler substrate
├── research/
│   ├── field/
│   ├── ledger/
│   └── packages/
├── data/
│   └── postgres/snapshots/
├── docs/
├── tools/
│   └── legacy-extraction/  # retained read-only migration toolkit
├── archive/
└── review/
```

By contributing, you agree that contributions are licensed under AGPL-3.0 and governed by the [Covenant](covenant.md).
