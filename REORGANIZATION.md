# Repository reorganization — structural pass complete

This file records the 2026-09 repository reorganization. The current tree describes the recovered system without rewriting development history or mixing structural moves with behavioural refactors.

## Preserved active lineages

The integration branch joins two active descendants of the 2026-08-30 rebase:

- the Taichi `engine/v0-staging` lineage;
- the later field-engine + database/kernel-network `dbkernel-design-checkpoint` lineage.

They were merged with both parent histories preserved. No force-push or history rewrite is part of this work.

## Current architectural boundaries

- **Modified Taichi fork** supplies the portable runtime/compiler/device substrate.
- **Physics engine** implements field dynamics.
- **Engine harness** is the physics-engine control panel the future analyst will use to add/remove/adjust elements and control analytical runs.
- **Analyst functions** have not yet been designed or implemented.
- **Database/cache/record/WAL kernels** support the future analyst and keep working surfaces current.
- **Endpoint/box/scheduler, topology, bridges and thread management** are kernel-network infrastructure rather than parts of the engine harness.
- Research material is retained but clearly separated from runtime code.

## Current tree decisions

- `engine/src/engine/`: recovered native C++ runtime/compiler wrapper.
- `engine/src/field/`: recovered native C++ field/tick implementation.
- `engine/taichi/`: curated modified Taichi fork with pinned external submodules; machine-local environments and build products are excluded from source control.
- `archive/2026-09-planner-field-python/`: planner-generated Python field prototype, retained only as historical/experimental evidence.
- `archive/2026-09-taichi-v0-staging/`: the earlier v0-staging generation, preserved intact after reconciliation confirmed the later field engine does not depend on it.
- `kernels/database/`: PostgreSQL record operations + database/cache-manager family.
- `kernels/wal/`: WAL manager family, promoted to a peer kernel set with its design/build records.
- `network/endpoint/`: shared endpoint/box/scheduler substrate promoted intact from the DB development bundle; CI follows the new path.
- `research/{field,ledger,packages}/`: research basis and shareable packages, not runtime modules.
- `data/postgres/snapshots/`: landing area for reproducibility dumps from development PostgreSQL systems.
- `archive/2026-03-database-generation/`: pre-rebase database schemas, migrations, dumps and roadmap preserved as historical implementation.
- `archive/2026-02-source-doc-pbm/`: earlier PBM/public-query architecture preserved as historical design.
- `tools/legacy-extraction/`: retained read-only extraction/migration utilities from prior storage generations.

## Closeout state

- PR #63 reconciled the active histories and separated engine, kernel, network, research and archive material.
- PR #66 restored the native C++ engine; PR #67 curated the modified Taichi fork with pinned dependencies.
- PRs #68 and #69 established a from-source native CPU build, functional tests and matched-backend build checks in CI. The native engine has also run on development hardware. The high-resource index-cap regression remains outside hosted CI.
- Repository smoke tests cover the separated endpoint, database and WAL modules.

The structural reorganization is complete. Further field-model vetting, realistic load measurements, kernel interface work and future network modules are development work described in [ROADMAP.md](ROADMAP.md), not conditions for accepting the repository layout. Historical material remains recoverable through Git even after files are moved or later removed from the current tree.
