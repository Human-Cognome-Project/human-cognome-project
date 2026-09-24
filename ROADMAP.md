# Roadmap

This roadmap reflects the repository and implementation state after the September 2026 development split. It replaces the August roadmap that still described the field engine as future work.

## 0. Repository reconciliation — initial structural pass complete

- Preserve both active development histories.
- Align the filesystem with actual architectural roles.
- Separate runtime implementation, kernel-network components, research, data snapshots and historical material.
- Refresh contributor/agent entry points.
- Keep structural moves and behavioural refactors in separate commits.

Working record: [REORGANIZATION.md](REORGANIZATION.md), PR #63.

## 1. Recovered native engine — source and build baseline established

The September native C++ engine/harness workspace remained local while repository reconciliation proceeded around an off-direction Python prototype. The native workspace has now been recovered; the Python prototype is preserved under `archive/2026-09-planner-field-python/` and is not canonical runtime.

The native workspace and modified Taichi fork are now in `engine/` with pinned dependencies and an HCP delta record. Hosted CI builds Taichi and the HCP C++ workspace from source, then runs native CPU smoke/field tests. The engine has also run on development hardware. The >2^31 dense-index regression remains a targeted high-resource test rather than a hosted CI gate.

Further field-model vetting against the design record and realistic load/performance characterization remain development work. Existing measurements are evidence, not guarantees.

## 2. Establish the native engine boundary cleanly — recovered, partially vetted

- **modified Taichi fork** — portable compiler/runtime/device substrate;
- **native engine support** — `engine/src/engine/`, mechanism only;
- **field physics** — `engine/src/field/`, native C++ field/tick mechanics;
- **engine harness** — wider control structure around the physics engine, still evolving;
- **validation** — native smoke/field/index-cap tests plus the recovered vetting record.

The recovered class `field::Harness` predates the later system-level harness terminology; do not assume that class alone is the complete analyst-facing harness.

Python is not part of the HCP engine/data runtime path.

## 3. Stabilize analyst-supporting work surfaces

Continue the PostgreSQL/cache/record/WAL kernel work so the future analyst can rely on current, reconciled working surfaces.

- preserve independently testable record operations;
- complete cache-manager tiers and WAL coupling as the settled design requires;
- establish reproducible development database snapshots under `data/postgres/snapshots/`;
- keep persistent database state distinct from schema, fixtures and runtime code.

The analyst's reasoning/functions are still out of scope here.

## 4. Complete the kernel-network substrate

Build the remaining pieces of the loose Beowulf-style kernel network:

- environment/configuration discovery;
- endpoint advertising and topology resolution;
- direct-memory local inbox/outbox pairing;
- serialization and transmission bridge pairs for remote/system boundaries;
- thread management for lower-frequency/system-facing endpoints;
- activation of less-frequently-needed kernels when relevant inboxes become occupied.

Kernel behaviour should remain independent of physical locality.

## 5. Analyst functions — future design phase

Only after the engine harness and working-data surfaces are stable should analyst functions be designed.

The analyst is expected to use:

- the engine harness as the physics-engine control panel; and
- analyst-supporting database/cache/WAL kernels for current work surfaces.

No analyst implementation should be inferred from the present DB-facing request interfaces.

## 6. Wider NAPIER integration

Integrate the stabilized field engine, kernel network and analyst layer into the broader NAPIER/HCP system while preserving the project's open, auditable architecture.

## Parallel research track

The field/ledger research basis remains under [research/](research/), with current explanatory material under [docs/](docs/). Research can continue independently of repository restructuring, but research artifacts should not be mistaken for runtime modules.
