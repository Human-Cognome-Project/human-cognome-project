# Prepared admission plan lifetime after native bootstrap

**Disposition and implementation — 2026-11-18:** [Review 6990952403](https://github.com/Human-Cognome-Project/human-cognome-project/pull/377#issuecomment-6990952403) provisionally accepts this inspection at `0302c527` and authorizes the [private consumption implementation](PREPARED-BOOTSTRAP-CONSUMPTION.md), now locally qualified at `9568f7e9`. Its retained savings include replacement metadata and a 64-byte setup-peak increase. Hosted CI/review remain separate. The inspection and zero-saving baseline below retain their original source-pinned scope.

Status: **inspection and bounded proposal, awaiting review** under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). No runtime storage is released or replaced by this contribution. Tested source `b07adf29bc9cecbe4e50883615b8d9afa9ceeb7c`, tree `6650121a02b4001eb0299c1a48e42c0684ee72b7`, starts from current main `22ea6d55`. This independent inspection does not depend on unmerged #375/#376. Their reviewed retained preset/refusal stay separate; #376's documentation correction is accepted in [comment 6990429101](https://github.com/Human-Cognome-Project/human-cognome-project/pull/376#issuecomment-6990429101), with corrected-head CI/integration still pending. All merges remain maintainer operations.

## Concrete owner and use trace

`ActiveSession::submit_prepared` materializes the native layout/Harness, then calls `PreparedStudyOwner::prepare` and `bootstrap`. The owner pins the caller's immutable composed `ActiveAssembly`; the [structural plan builder](STRUCTURAL-ADMISSION-PLAN.md) allocates a separate sealed plan and its buffers in the supplied reservation domain. The owner also reserves actual body/field claim tables and pointer-free reporting scales. Those associations and full source pins have semantic responsibilities independent of bootstrap descriptors.

| Phase / symbol | Required plan or native state |
|---|---|
| `StructuralAdmissionPlan::prepare` | Validate full source/definition pins, typed fields, exact counts, explicit comparison/focus/geometry and finite bounds; emit commands/intrinsics/focus/hierarchy plus durable identity-to-command rows. |
| `PreparedStudyOwner::bootstrap`, before success | Use every bootstrap buffer in `Harness::stage_current_admission`; retain it through ordinary refusal and retry. |
| Bootstrap after native commit, before owner publication | Resolve full context/incarnation/address/record claims using each plan command's address, verify current native headers, then publish maps. Buffers cannot retire before these final checks. |
| Published `bootstrap` | Returns `NoChange` before reading replay descriptors. It does not replay the initial commands. |
| `AssociationRows::body` | **Still reads `plan.commands()[entry.command].exact_mass` after publication.** This is an actual remaining command-table dependency. |
| `report_metadata`, `AssociationRows::field`, completed observer | Need immutable study/definition/field identities, scales and actual claims; reports remain detached. Native counts do not supply those full semantic pins. |
| `PreparedStudyOwner::tick` | Advances the existing Harness and follows native generation; does not pass the bootstrap plan to the tick. Association loans expire on tick/generation changes even though backing stays alive. |
| Owner destruction | Releases plan and claim metadata before the borrowed Harness dies. The immutable source may have other owners; that source's bytes are outside this measurement. |

The producer/control layer chooses the study and comparisons; the plan validates their meaning; native execution owns claims, physical values, installed topology and completed ticks. This inspection proposes no Analyst policy or #324 formula.

## Actual requested backing

The new `prepared_plan_lifetime_test` uses the existing production builder/owner/bootstrap/tick with `prepared_owner_fixture::Input`, once in aggregate exposure and once in parts exposure. Each has two distinct active occurrences; parts uses the existing PrimaryStep/unit-64 binding and three comparisons. All source values are synthetic. This is neither an actual retained acquisition nor a public producer migration.

A current-thread C++ allocation observer independently records preparation requests. Each measured vector data pointer must match an actual request; capacities and element extents must agree. The six buffers must be disjoint. Hierarchy extents are checked against actual request sizes rather than inferred solely from row counts. Observer live bytes equal the selected domain's retained request bytes. The observer is paused during native operations and detached reports, then resumed for owner destruction; those operations are outside the preparation census. No process-wide/malloc/stack/heap-rounding/RSS/driver claim is made.

| Request / state (bytes) | Aggregate: 2 comparisons | Parts: 3 comparisons |
|---|---:|---:|
| Commands (8 / 11 rows) | 1216 | 1672 |
| Placement intrinsics (2 rows) | 112 | 112 |
| Focus (1 row) | 32 | 32 |
| Hierarchy nodes | 192 | 288 |
| Child indexes (1 / 2 rows) | 4 | 8 |
| Contributions (4 / 6 rows) | 256 | 384 |
| **Six-buffer candidate total** | **1812** | **2496** |
| Candidate excluding the still-read command table | 596 | 824 |
| **Entire selected owner/plan-domain retained requests** | **3966** | **4870** |
| Achieved runtime saving | **0** | **0** |
| Selected domain/observed preparation requests after owner destruction | **0** | **0** |

Each candidate buffer retains the same request/pointer through publication and a following native tick. The selected domain remains at 3966/4870 bytes through healthy refusal, retry, publication and ticking, then releases fully. These are retained request extents, not cumulative setup, whole-adoption peak, native backing or complete C+B+H+D accounting. Retirement after bootstrap would not by itself reduce initial preparation peak or prepaid work. Neither candidate total is an achieved saving or a guaranteed future net saving; any replacement metadata and object/header changes must be included in a later before/after measurement.

## Existing native support and limits

`Harness::stage_current_admission` validates pending references, commits exact `source_masses`/`source_counts`, resolves actual claims and stages/installs the existing live hierarchy. `PreparedStudyOwner::ready` checks the full published claims against current native headers. The witness checks both bodies' plan exact mass against the claimed native `source_masses` and detached report at tick 0 and tick 1, and matches all comparison identities/fields/levels/precision against the installed native inspection.

The native hierarchy already owns translated configuration, source headers/ratios and numerical banks in [`LiveHierarchy::Impl`](../src/field/live_hierarchy.cpp). Combined bootstrap calls `prepare_refresh`, `stage_refresh` and the existing commit path in [`live_hierarchy_refresh.inc`](../src/field/live_hierarchy_refresh.inc), resolving pending references into that owned execution state. It does not call the standalone `LiveHierarchy::install` entry. Neither path makes the plan's `LiveRefreshSpec` buffers the numerical execution authority. `Impl::owned` uses the pinned fork's [`taichi::lang::Ndarray`](../taichi/taichi/program/ndarray.cpp), allocated for the matching Program. `Program::fill_ndarray_fast_u32` and existing native commit/transport supply storage initialization and execution. The arrays have their own owner and lifetime; the new witness makes no claim that upstream Taichi understands HCP composition, exact definition revisions, field kinds or recipe pins.

The native exact source ledger is a possible already-owned source for the post-publication body's denominator, **only after full claim/currentness checks**. It is not the floating physical particle mass. A native descriptor is not a replacement for complete typed field/source identity. Pending command indexes are not stable live handles. Future prepared edits, refresh, source-mass changes and detached-plan reuse need their own contracts.

## Proposed smallest follow-up

Review an explicit **private preparing-owner consumption boundary** before implementing retirement:

1. Keep all sealed replay buffers through validation, ordinary refusal/retry, native commit and final claim verification. Standalone `StructuralAdmissionPlan` values retain their existing immutable lifetime and borrowed `hierarchy()` contract.
2. Remove the published body's command-table dependency using the checked native exact source ledger, or explicitly prepaid durable exact-mass metadata. Preserve full source/definition pins, typed occurrence/field rows, scales and claim guards. Any new retained bytes/work must be counted.
3. Only on the private successful owner route, retire bootstrap-only backing after its last use, with no allocation or ordinary failure after native commit. Do not silently invalidate the owner's exposed plan/association contracts; coordinate those readers explicitly while keeping standalone plans intact. No generic registry, new scheduler or parallel semantic authority is proposed.
4. Measure the actual whole owner before/after, including new headers/metadata, retained requests and temporary peak. Qualify exact/one-short bytes/work, allocation/partial-copy failure, refusal/retry, no-op bootstrap, source/owner/domain lifetime, stale/foreign claims, detached reports, both exposures/scales and following ticks across compiler/worker paths.

The present witness does **not** release buffers or prove that consumers tolerate their removal. Its finding is an ownership opportunity with an identified dependency, not permission to drop immutable provenance or reuse stale native rows. Runtime implementation awaits review. Validator/capture-buffer reuse, refreshed hierarchy transport, general prepared revisions, full accounting, GPU execution and #324 remain separate.

## Validation and evidence

All source-pinned checks pass with a clean checkout before/after:

- Quick local CI: matching fork/engine build, actual CPU backend, three CTests, 632 independent interval-math samples and placement capacity.
- Clang 15: **6/6** suites in 294.37 s: four new worker/path modes plus existing prepared-owner and builder controls.
- GCC 11: **3/3** suites in 123.53 s: new 1-worker/bulk and 2-worker/direct modes plus the builder.

The twelve new fixture observations (two exposures in six compiler/worker/path modes) reproduce the table, native tick 1 and full measured release. The matched fork tree is `8b31651be07d8bad0b2a6a261369e8690b9c3b53`. No retained database is read or written, and no GPU/native-Windows or full local regression qualification is claimed. Production/fork/database/viewport/workflow code is unchanged; only the witness and its CMake registration change before the evidence notes. GitHub CI remains authoritative and is submitted with the contribution. No existing run is manually duplicated.

Verified local bundle: `/tmp/hcp-plan-lifetime-b07adf29-evidence.tar.gz`, SHA256 `d314ab89601e77b3cef4c85867452ac691a107841252e3a6e086eed9e92eeba5`. Its manifest passes. It retains exact commands/configurations, source/tree/submodule/cleanliness and binary/runtime hashes, test/build output, the unchanged-production patch and separate precommit development logs. This machine-local hash does not publish the raw bundle. The final Markdown notes change no tested executable or registration. Next: review the private consumption proposal, then a separately qualified runtime implementation if accepted.
