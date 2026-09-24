# Repository reorganization status

This file is the working map for the 2026-09 repository reorganization. The goal is to make the repository describe the actual system without rewriting development history or mixing structural moves with behavioural refactors.

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
- `engine/taichi/`: intended location of the modified Taichi fork, curated separately because the recovered local folder contains source, vendored externals, build products and machine-local environment files.
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

## Next review passes

1. Vet the recovered native C++ engine/field workspace against its settled design and operational record.
2. Curate the modified Taichi fork: preserve HCP changes and exact dependency revisions while excluding machine-local `.venv` and reproducible build products from canonical source.
3. Continue stabilizing the separated `kernels/database/`, `kernels/wal/`, and `network/` interfaces with coordinated include/build/test updates.
4. Establish the future configuration/topology and thread/bridge module locations when implementation begins.
5. Keep root README, contributor and agent guidance synchronized as the implementation tree stabilizes.
6. Keep the smoke suite green as the reorganized tree evolves. The kernel/network/database tests remain active; native engine build/runtime CI will be restored once the curated Taichi fork and its reproducible dependency/build boundary are in place.

The initial structural reconciliation reached its promotion gate with the full smoke suite green. Historical material remains recoverable through Git even after files are moved or later removed from the current tree.
