# Repository reorganization status

This file is the working map for the 2026-09 repository reorganization. The goal is to make the repository describe the actual system without rewriting development history or mixing structural moves with behavioural refactors.

## Preserved active lineages

The integration branch joins two active descendants of the 2026-08-30 rebase:

- the Taichi `engine/v0-staging` lineage;
- the later field-engine + database/kernel-network `dbkernel-design-checkpoint` lineage.

They were merged with both parent histories preserved. No force-push or history rewrite is part of this work.

## Current architectural boundaries

- **Taichi runtime** executes the accelerated implementation.
- **Physics engine** implements field dynamics.
- **Engine harness** is the physics-engine control panel the future analyst will use to add/remove/adjust elements and control analytical runs.
- **Analyst functions** have not yet been designed or implemented.
- **Database/cache/record/WAL kernels** support the future analyst and keep working surfaces current.
- **Endpoint/box/scheduler, topology, bridges and thread management** are kernel-network infrastructure rather than parts of the engine harness.
- Research material is retained but clearly separated from runtime code.

## Current tree decisions

- `engine/field/`: later September field-engine implementation, moved intact; internal physics/harness split deferred.
- `archive/2026-09-taichi-v0-staging/`: the earlier v0-staging generation, preserved intact after reconciliation confirmed the later field engine does not depend on it.
- `kernels/database/`: PostgreSQL record operations + database/cache-manager family.
- `kernels/wal/`: WAL manager family, promoted to a peer kernel set with its design/build records.
- `network/endpoint/`: shared endpoint/box/scheduler substrate promoted intact from the DB development bundle; CI follows the new path.
- `research/{field,ledger,packages}/`: research basis and shareable packages, not runtime modules.
- `data/postgres/snapshots/`: landing area for reproducibility dumps from development PostgreSQL systems.

## Next review passes

1. Keep the archived Taichi v0-staging generation available as an experimental/predecessor reference while current field-engine work proceeds from `engine/field/`.
2. Separate physics-engine code from engine-harness control code without changing behaviour.
3. Decompose the preserved DB-kernel development bundle only with coordinated include/build/test updates.
4. Establish the future configuration/topology and thread/bridge module locations when implementation begins.
5. Refresh root README, contributor and agent guidance after the implementation tree has stabilized.
6. Run tests/CI and only then promote the reorganization to `main`.

Historical material remains recoverable through Git even after files are moved or later removed from the current tree.
