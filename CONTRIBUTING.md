# Contributing to the Human Cognome Project

**New coding environment or agent:** start with the [fresh-instance onboarding/spec brief](docs/onboarding/AGENT-START.md).

HCP accepts code, research, testing, documentation and architecture critique. The structural migration is complete; current guidance reflects the recovered native C++ engine, curated Taichi fork and separated kernel/network architecture.

## Start here

Begin with [orientation](docs/onboarding/README.md), then follow its
[reading paths](docs/onboarding/README.md#reading-paths), including the Covenant
and Charter. Check [current state](AGENTS.md#current-state) before selecting work.
Agents read AGENTS.md in full.

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

Use [AGENTS current state](AGENTS.md#current-state) and
[active and deferred work](AGENTS.md#active-and-deferred-work) to choose work.
This page does not maintain a second priority queue. Follow the owning issue's
accepted plan and latest review disposition before implementing or writing data.
Research contributions remain welcome under `research/`; they do not silently
change runtime contracts.

## Reading evidence and status

Code and executable evidence establish what is built; accepted decisions govern
intent. AGENTS is the maintained summary. If these disagree, identify and reconcile
the discrepancy rather than treating a summary as proof or an old fixture as the
production contract.

The audit trail lets a future reader reconstruct what was accepted, on which
evidence and with which limits, without access to the original chat.

For a PR, read its scope, the owning plan's acceptance criteria, the latest
maintainer findings, then the evidence tied to the reviewed source revision.
Distinguish tests personally run, hosted CI results and contributor-retained
logs. Source hashes identify inputs; they do not by themselves prove behavior.
A merged plan authorizes only its stated scope and does not mean it is implemented.

Keep claims bounded: fixture execution is not production insertion; requested
CUDA is not verified CUDA; functional execution is not performance/pinning
qualification; visible motion is not established linguistic settling.
A held proposal is not an implementation contract. Record remaining work by its
owning issue rather than burying it in acceptance prose.

When a merge or review changes Built/Held/Active status, update
[AGENTS current state](AGENTS.md#current-state) and remove conflicting status
claims from entry points. Keep detailed measurements and source pins with the
component evidence. Onboarding links to those records rather than copying them.
If a build target/path changes, update the [first-run check](docs/onboarding/FIRST-RUN.md)
with the same change; its named test must remain part of hosted CI.

## Issues and bounded PRs

Use a persistent issue for a larger concern spanning decisions or multiple PRs.
Record the problem, accepted direction, open questions, dependencies and acceptance
criteria there; link the owning repository contract rather than duplicating it.
Each plan or implementation PR names its owning issue and the acceptance subset it
delivers. Use `Refs #N` for partial work and `Closes #N` only when the full issue's
acceptance is satisfied. Update the issue with reviewed source/evidence and
remaining work after a merge. A plan merge does not close its implementation issue.
Small self-contained fixes need not acquire a new umbrella issue.

## Technical standards

- **Preserve history.** No force-push/history rewrite for cleanup. Normal Git history is the recovery path for moved or removed current-tree files.
- **Separate structure from behaviour.** Prefer pure moves first, then refactor in a later commit with tests.
- **Tests on behavioural changes.** Native engine changes should retain CPU/backend equivalence and model-anchored regression checks where applicable. C++ kernel modules should remain independently testable where their existing contracts require it.
- **Location blindness.** Kernel logic should not branch on whether a counterpart is local or remote; that belongs to topology/bridge infrastructure.
- **No speculative analyst implementation.** Interfaces may expose future analyst-facing seams, but the analyst layer itself is not yet defined.
- **Compiled implementation path.** HCP engine/data/network components are C++. Project Python is authorized only for bulk database population during bootstrap, including preparation and validation of those batches. Offline operation alone is not an exception.
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
│   └── taichi/             # curated modified Taichi fork
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
│   └── onboarding/        # orientation and first native check
├── tools/
│   ├── viewport/          # optional read-only study inspection
│   └── legacy-extraction/  # retained read-only migration toolkit
├── archive/
└── review/
```

By contributing, you agree that contributions are licensed under AGPL-3.0 and governed by the [Covenant](covenant.md).

