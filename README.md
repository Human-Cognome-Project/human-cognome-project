# Human Cognome Project (HCP)

**New coding environment or agent:** start with the [fresh-instance onboarding/spec brief](docs/onboarding/AGENT-START.md).

The Human Cognome Project is an open-source experimental architecture for digital intelligence built around explicit structure, field dynamics, and auditable data rather than a monolithic statistical model.

The September repository reconciliation is complete, and a subsequent recovery pass restored the native C++ engine workspace that had remained local. See [REORGANIZATION.md](REORGANIZATION.md) for the provenance and recovery map.

## Start here

Start with [orientation](docs/onboarding/README.md) for objects, fields and
component boundaries, then the [current state](AGENTS.md#current-state) for
built, held and active work. [First run](docs/onboarding/FIRST-RUN.md) leads to
one native CPU smoke check and an optional retained visual capture.
Agents read [AGENTS.md](AGENTS.md) in full before working.

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

- **[engine/](engine/)** — canonical native C++ engine workspace: thin Taichi runtime support, native field physics, tests, build integration and harness design record.
- **[engine/taichi/](engine/taichi/)** — curated modified Taichi fork with pinned dependencies, separate from the HCP C++ wrapper.
- **[archive/2026-09-planner-field-python/](archive/2026-09-planner-field-python/)** — off-direction Python field prototype retained as historical/experimental evidence, not runtime code.
- **[archive/2026-09-taichi-v0-staging/](archive/2026-09-taichi-v0-staging/)** — the parallel August 31–September 1 Taichi generation, now archived intact as a predecessor/experimental record after reconciliation showed no runtime dependency from the later field engine.
- **[kernels/database/](kernels/database/)** — PostgreSQL record operations and database/cache-manager kernel family supporting the analyst's working surfaces.
- **[kernels/wal/](kernels/wal/)** — WAL manager kernel family, now a peer of the database/cache family.
- **[network/endpoint/](network/endpoint/)** — the shared in-memory endpoint/box/scheduler substrate, promoted out of the DB bundle because it is common kernel-network infrastructure.
- **[research/](research/)** — field/ledger research and convenience packages. Research informs the implementation but is not runtime code.
- **[data/postgres/snapshots/](data/postgres/snapshots/)** — reserved for reproducibility snapshots exported from development PostgreSQL systems.
- **[tools/legacy-extraction/](tools/legacy-extraction/)** — retained read-only migration/source tooling from earlier storage generations; not runtime code.
- **[archive/](archive/)** and **[review/](review/)** — preserved prior generations and the August 2026 rebase record.

## Reading and reference

Follow the [canonical reading paths](AGENTS.md#reading-paths).
The [documentation index](docs/README.md) links active plans and historical
research; [REORGANIZATION.md](REORGANIZATION.md) records repository provenance.

## Repository status

The September 2026 repository reconciliation was carried through `integration/kernel-network-reorg` and PR #63. It preserves both active development lineages through normal merge history; no history rewrite or force-push was used.

The current tree is the canonical starting point for new work. The native C++ engine recovery and curated Taichi fork are layered onto that reconciled history. CI builds and tests the native CPU engine from source; the engine has also run on development hardware. Earlier and off-direction generations remain under `archive/` and through Git history.

The review queue and acceptance boundaries are maintained in the
[current state](AGENTS.md#current-state); consult current PR reviews
before implementing a proposal.

## Governance

- [Covenant](covenant.md) — perpetual-openness commitment.
- [Charter](charter.md) — contributor governance.
- [CONTRIBUTING.md](CONTRIBUTING.md) — contribution workflow.
- [AGENTS.md](AGENTS.md) — operational guidance for contributing agents.
- [LICENSE](LICENSE) — AGPL-3.0.

