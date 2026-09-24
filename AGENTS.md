# Agent guidance

This file is the operational entry point for AI coding/review agents working on HCP.

## Read before changing code

1. [REORGANIZATION.md](REORGANIZATION.md)
2. [engine/ARCHITECTURE.md](engine/ARCHITECTURE.md)
3. the README/plan/test files inside the component you will touch

During the current repository migration, do not infer architecture from historical folder placement. Several directories were intentionally used as agent-isolation boundaries while the design was moving faster than the repository structure.

## Architecture invariants

- The Taichi **runtime**, **physics engine**, and **engine harness** are distinct concepts.
- The **engine harness is the control interface for the physics engine**. It is not the DB manager, WAL manager, messaging system, topology system or thread manager.
- **Analyst functions do not exist yet.** Do not invent an analyst architecture or implement speculative analyst behaviour.
- Database/cache/record/WAL kernels exist to support the future analyst and keep its working surfaces current.
- Kernel components communicate through active inbox/outbox endpoints and should remain location-blind: local versus remote placement is a topology/bridge concern.
- The future configuration routine resolves endpoint paths; the future thread manager handles lower-frequency/system-facing bridge activation.
- `kernels/db_kernel/` is still a preserved development bundle. Its endpoint substrate is wider infrastructure even though it currently lives beside database/WAL code.

## Current development state

The active reorganization branch is `integration/kernel-network-reorg` (draft PR #63). It joins the active Taichi and db-kernel histories without rewriting either one.

If you are contributing to paths affected by the reorganization, branch from the integration branch rather than from stale `main`.

## Work discipline

- Code and executable tests determine what is built. Design notes determine intent only where they explicitly say a decision is settled.
- Preserve distinctions such as **BUILT**, **planned**, **deferred**, and **experimental**. Do not promote a plan to implementation by paraphrase.
- Do not split `kernels/db_kernel/` merely to improve appearance. Its C++ relative includes/build instructions must be migrated together with tests.
- Do not refactor `engine/field/` into physics/harness modules in the same commit as structural moves. Establish behaviour-preserving boundaries first.
- Never force-push or rewrite shared history for cleanup. Normal commits, merges and moves keep prior versions recoverable.
- Avoid destructive database operations outside disposable test databases. Never point reset/drop-schema tests at data that must be kept.
- Keep credentials and private data out of Git. PostgreSQL reproducibility exports belong under `data/postgres/snapshots/` and use Git LFS for compressed dumps.
- Add or preserve tests for behavioural changes. Large generated run artifacts should not be treated as source code merely because an experiment produced them.

## Contribution flow

Use focused branches and reviewable commits. During the reorganization, prefer small commits that do one of:

- history-preserving integration;
- pure structural move;
- documentation correction;
- behaviour refactor;
- test/build repair.

Do not combine all of those in a single cleanup commit.
