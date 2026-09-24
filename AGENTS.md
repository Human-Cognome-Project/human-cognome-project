# Agent guidance

This file is the operational entry point for AI coding/review agents working on HCP.

## Read before changing code

1. [REORGANIZATION.md](REORGANIZATION.md)
2. [engine/ARCHITECTURE.md](engine/ARCHITECTURE.md)
3. the README/plan/test files inside the component you will touch

Do not infer architecture from historical folder placement. Several directories and branches were used as agent-isolation boundaries while the design was moving faster than the repository structure.

## Architecture invariants

- The modified Taichi **runtime/compiler substrate**, HCP **physics engine**, and HCP **engine harness** are distinct concepts.
- The canonical engine path is native **C++**. Taichi's Python frontend is upstream/vendor tooling, not the HCP runtime surface.
- Project Python is restricted to convenient bootstrap, migration, import/export, or other offline I/O. Do not implement physics, database/WAL kernels, endpoint/network infrastructure, or analyst runtime behavior in Python.
- The **engine harness is the control interface for the physics engine**. It is not the DB manager, WAL manager, messaging system, topology system or thread manager.
- **Analyst functions do not exist yet.** Do not invent an analyst architecture or implement speculative analyst behaviour.
- Database/cache/record/WAL kernels exist to support the future analyst and keep its working surfaces current.
- Kernel components communicate through active inbox/outbox endpoints and should remain location-blind: local versus remote placement is a topology/bridge concern.
- The future configuration routine resolves endpoint paths; the future thread manager handles lower-frequency/system-facing bridge activation.
- `kernels/database/`, `kernels/wal/`, and `network/endpoint/` are peers. Do not collapse WAL or shared endpoint infrastructure back under the database/cache hierarchy.

## Current development state

The structural repository reconciliation is complete on `main`. Native engine recovery restored the C++ workspace and curated Taichi fork; hosted CI builds and tests the native CPU path from source. Further architecture and performance work is tracked separately from the reorganization.

Branch new work from current `main` unless a specific recovery/integration branch is explicitly named for the task.

## Work discipline

- Code and executable tests determine what is built. Design notes determine intent only where they explicitly say a decision is settled.
- Preserve distinctions such as **BUILT**, **planned**, **deferred**, and **experimental**. Do not promote a plan to implementation by paraphrase.
- When moving kernel families, migrate their include paths, build instructions and tests together. Preserve working interfaces before refactoring behaviour.
- Preserve the native engine boundary under `engine/src/`: `engine_support` wraps the Taichi runtime/compiler substrate; `field_core` owns field/tick mechanics; the wider analyst-facing harness remains a distinct control layer. Do not promote the archived Python prototype back into the runtime path.
- Never force-push or rewrite shared history for cleanup. Normal commits, merges and moves keep prior versions recoverable.
- Avoid destructive database operations outside disposable test databases. Never point reset/drop-schema tests at data that must be kept.
- Keep credentials and private data out of Git. PostgreSQL reproducibility exports belong under `data/postgres/snapshots/` and use Git LFS for compressed dumps.
- Add or preserve tests for behavioural changes. Large generated run artifacts should not be treated as source code merely because an experiment produced them.

## Contribution flow

Use focused branches and reviewable commits. For structural changes, prefer small commits that do one of:

- history-preserving integration;
- pure structural move;
- documentation correction;
- behaviour refactor;
- test/build repair.

Do not combine all of those in a single cleanup commit.
