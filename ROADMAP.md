# Roadmap

This roadmap reflects the repository and implementation state after the September 2026 development split. It replaces the August roadmap that still described the field engine as future work.

## 0. Repository reconciliation — active

- Preserve both active development histories.
- Align the filesystem with actual architectural roles.
- Separate runtime implementation, kernel-network components, research, data snapshots and historical material.
- Refresh contributor/agent entry points.
- Keep structural moves and behavioural refactors in separate commits.

Working record: [REORGANIZATION.md](REORGANIZATION.md), draft PR #63.

## 1. Reconcile the Taichi development lines

The August 31–September 1 `engine/{kernel,timestep,storage,ingest}` work and the later September `engine/field/` implementation developed on parallel branches.

Review by responsibility rather than age:

- physics that remains current;
- harness/control behaviour that remains useful;
- ingestion/storage/addressing work that belongs outside physics;
- experimental validation artifacts worth retaining;
- superseded implementation that should move to archive.

No component is discarded merely because it is earlier.

## 2. Establish the engine boundary cleanly — initial split complete

The active field engine is now separated into:

- **physics engine** — `engine/field/field_engine_physics.py`;
- **engine harness** — `engine/field/field_engine_harness.py`, the control interface used to add/remove/adjust elements, configure and advance runs, and inspect/manipulate engine state;
- **stable facade** — `engine/field/field_engine.py` preserves the CLI and existing imports;
- **validation** — `engine/field/field_engine_validation.py` plus the no-DB physics smoke test, kept outside engine behaviour.

Next work in this phase is deeper behavioural coverage and harness evolution, not recombining these responsibilities.

The Taichi runtime remains the execution substrate rather than an HCP architectural layer of its own making.

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
