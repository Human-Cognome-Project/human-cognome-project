# Human Cognome Project (HCP)

The Human Cognome Project is an open-source experimental architecture for digital intelligence built around explicit structure, field dynamics, and auditable data rather than a monolithic statistical model.

The repository is currently being reorganized so its filesystem matches the system that has actually been developed. See [REORGANIZATION.md](REORGANIZATION.md) for the live migration map.

## Current system shape

```text
Taichi runtime
    ↓
physics engine
    ↑
engine harness
    ↑
future analyst functions
```

The **physics engine** implements the field dynamics. The **engine harness** is its control interface: the surface used to add, remove and adjust elements, control runs, and inspect engine state for analysis.

The **analyst functions have not yet been designed or implemented**.

Alongside the field-engine stack is a loosely coupled kernel network:

```text
future analyst
    ├── engine harness → physics engine
    └── analyst-supporting work surfaces
          ├── database/cache kernels
          ├── record operations
          └── WAL manager

kernel-network infrastructure
    ├── inbox/outbox endpoints
    ├── scheduler/activation
    ├── future topology/configuration
    ├── future serialization/transmission bridges
    └── future thread manager
```

Each kernel is intended to operate at its own cadence. Counterparts communicate through active inbox/outbox endpoints; local versus remote placement is meant to be resolved beneath the kernel by direct-memory paths or serialization bridges.

## Current implementation

- **[engine/field/](engine/field/)** — later September Taichi field-engine development. The current files still mix physics, harness, CPU-oracle and validation concerns; that split is a planned refactor, not something to infer from filenames.
- **[archive/2026-09-taichi-v0-staging/](archive/2026-09-taichi-v0-staging/)** — the parallel August 31–September 1 Taichi generation, now archived intact as a predecessor/experimental record after reconciliation showed no runtime dependency from the later field engine.
- **[kernels/db_kernel/](kernels/db_kernel/)** — the preserved PostgreSQL/cache/WAL development bundle. Its remaining co-location reflects controlled development and existing build relationships, not final architecture.
- **[network/endpoint/](network/endpoint/)** — the shared in-memory endpoint/box/scheduler substrate, promoted out of the DB bundle because it is common kernel-network infrastructure.
- **[research/](research/)** — field/ledger research and convenience packages. Research informs the implementation but is not runtime code.
- **[data/postgres/snapshots/](data/postgres/snapshots/)** — reserved for reproducibility snapshots exported from development PostgreSQL systems.
- **[archive/](archive/)** and **[review/](review/)** — preserved prior generations and the August 2026 rebase record.

## Read first

For current development:

1. [REORGANIZATION.md](REORGANIZATION.md)
2. [engine/ARCHITECTURE.md](engine/ARCHITECTURE.md)
3. [engine/field/README.md](engine/field/README.md) or [kernels/README.md](kernels/README.md), depending on the work
4. the local module README/plan/test files for the component being changed

For the research basis and project context:

- [docs/physics-basis.md](docs/physics-basis.md)
- [docs/architecture.md](docs/architecture.md)
- [docs/data-protocol.md](docs/data-protocol.md)
- [research/](research/)
- [MANIFESTO.md](MANIFESTO.md)

## Repository status

The active reorganization is staged on `integration/kernel-network-reorg` and tracked in draft PR #63. It preserves both active development lineages with a normal merge; no history rewrite or force-push is part of the migration.

Until that work is promoted, `main` does not contain the complete current development state.

## Governance

- [Covenant](covenant.md) — perpetual-openness commitment.
- [Charter](charter.md) — contributor governance.
- [CONTRIBUTING.md](CONTRIBUTING.md) — contribution workflow.
- [AGENTS.md](AGENTS.md) — operational guidance for contributing agents.
- [LICENSE](LICENSE) — AGPL-3.0.
