# Engine and Analyst-facing refactoring plan

Authorized direction: 2026-11-15. Owner: [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Numerical derivation remains [#324](https://github.com/Human-Cognome-Project/human-cognome-project/issues/324).

## Purpose and boundaries

Break mixed, monolithic systems into coherent work units that can be understood and changed with the relevant context. Make ownership, inputs, outputs and lifetimes explicit. Adopt existing native Taichi machinery where it supplies the required behavior. Smaller files are useful only when they improve these responsibilities.

The viewport and temporary API front end are controls through which a human or agent can emulate Analyst functions until their concrete implementation is formalized. The viewport may remain useful longer. Identify which existing operations are Analyst-facing and which belong inside execution ownership; do not let temporary front ends become the numerical authority. Preserve the distinction between the conceptually designed Analyst and its still-unimplemented concrete decision-making. Do not create a speculative universal Analyst API, new orchestration framework or second scheduler.

This plan authorizes inspection and bounded proposals, followed by focused reviewed implementation. It does not fix a final module layout, select a new intent formula, or declare an SNode storage relationship to be a composition relationship. Preserve [system boundaries](onboarding/SYSTEM-BOUNDARIES.md), [work discipline](onboarding/WORK-DISCIPLINE.md), and applicable component contracts.

## First task for the fresh coding context

1. Establish the actual workspace/repository, local changes, branch and submodule pins. Fetch current main and inspect open PRs/reviews before selecting work. Preserve existing work; do not reset a checkout or retained database to obtain a clean start. Maintainer remote checks do not certify the local/WSL checkout.
2. Record the source SHA and current CI. Main through #341 is qualified at runtime merge `64fd23d02eb33fa00129ce78bd20fc5ad225f8e5` (native 43713977014, smoke 43713977016). Later main changes through `6cccc100` are documentation. Read the current handoff for subsequent changes.
3. Preserve and investigate #342's outstanding failure. Its corrected head `0d0adec5b1beda5db55aa8d54358b6f7d2c0bb1b` passed projection-values and native active_workbench_test, but run 43715143149/job 132815906747 failed live_intent_activity_2_bulk with `equal-force publication control changed force`. Other regression tests passed (168/169 overall). Distinguish an ordering defect, numerical difference and test assumption using focused evidence; do not assume a flake or loosen the assertion. Keep a necessary fix separate from refactoring. Do not duplicate a running suite or replay an accepted baseline just to onboard.
4. Produce a source-pinned responsibility map and proposed PR sequence before moving code. Inspection can proceed while the failure is being diagnosed; behavioral extraction must have a clear baseline and known-failure treatment. Do not treat pending #342 as integrated.
5. Submit that first map/sequence as a small documentation PR referencing #343. The next implementation slice should be concrete enough to review without reconstructing the full system. Ask about actual semantic ambiguity; routine organization does not require repeated user permission.

The first deliverable must trace one existing study lifecycle (assemble/admit/publish/tick/observe/refresh/release) and one completed tick using actual paths and symbols. Record what is absent as absent. For each proposed work unit give its purpose, current owner, callers, dependencies, mutable state/lifetime, privacy boundary, invariants, tests and proposed extraction. Include which parts can be understood independently and which genuinely need joint context.

## Responsibility map to investigate

These are candidate responsibilities, not six mandated libraries.

| Responsibility | Questions for source inspection |
| --- | --- |
| Composition and identity | Where are object definitions, primary composition paths, occurrence roles and native SNode mappings established? How are nonstructural attachments distinguished? |
| Active study ownership | Who owns admission, prepared storage, generations, bindings, publication, refresh and teardown? Which responsibilities are mixed in ActiveAssembly/ActiveSession? |
| Engine mechanics | Where do structural reductions, other field contributions, integration, braking, activity and quantization execute? Which are already native Taichi operations? |
| Analyst-facing operations | Which operations let a caller select/assemble a study, request changes, advance work and inspect results? Separate caller decisions from engine-owned validation and execution. |
| Temporary controls | Which CLI/API/viewport code merely adapts human or agent requests? Identify current concrete surfaces before proposing an interface; do not infer an API implementation from the project vocabulary. |
| Observation and evidence | Which reports, capture serializers, fixtures and viewport readers expose completed state? Preserve read-only observation, identities and unavailable/refused/absent distinctions. |

Starting inspection locations include engine/src/assembly/active_workbench.*, engine/src/assembly/letter_projection.*, membership_fields.*, engine/src/field/, engine/src/engine/, and tools/viewport/native/. Find the actual orchestration and public entry points through their callers. These locations are anchors, not evidence of a completed audit. Database/cache/WAL and endpoint components remain peers with their existing owners; inspect cross-boundary calls without folding those systems into the harness.

## Focused passes and exit evidence

### Pass 1: map and select

Inventory mixed responsibilities and dependencies. For each native-reuse opportunity, identify the existing custom mechanism, the relevant Taichi code/compiler behavior, required semantics and verification method. A data tree does not automatically supply a physics calculation: inspect traversal, reductions, storage, dispatch, synchronization and compilation rather than assuming equivalence.

Exit: a bounded responsibility map, a short ordered PR list, explicit dependencies/risks, and one proposed extraction. No new execution or mathematical behavior.

### Pass 2: separate responsibilities while preserving behavior

Move coherent work behind explicit ownership boundaries. Keep mechanical relocation distinguishable from logic changes; update build targets, includes and tests with each slice. Retain test coverage in the correct layer: pure structural/projection tests must still build without Taichi/PostgreSQL, while native ownership tests run in their native suite. #342's first failure is a concrete warning against crossing that boundary casually.

Exit for each slice: unchanged public behavior and numerical contracts, no duplicate state authority, explicit lifetime/dependency rules, focused passing evidence and applicable hosted qualification. Preserve failures and distinguish local reports from independently inspected evidence.

### Pass 3: use native machinery

Replace redundant mechanisms only after verifying native equivalents and required semantics. Inspect emitted compiler artifacts where an optimization or threading claim depends on compilation. Preserve backend support, exact identity, resource/accounting boundaries and mathematical coherence. Setup cost alone is not a reason to reject a worthwhile continuing-operation improvement.

Findings from this pass may reshape Pass 2 before extraction; do not build a new abstraction around machinery that should be removed. Keep replacement commits separate from pure moves. Record native mechanisms retained/reused, wrappers still required and unresolved opportunities with reasons. No custom thread-specific machinery merely to reproduce existing Taichi behavior.

### Pass 4: inspect and implement the mathematical distinction separately

Under #324, investigate structural intent derived from composition/primary identity, with nonstructural fields separately accounted for in motion. Trace the actual native SNode representation and operations. Establish weighting, frame, publication time, invalidation and rolled-up/repeated-path semantics. Do not equate raw tree depth with conceptual scale.

The existing combined candidate is an implemented diagnostic, not yet this structural reference. Track structural intent drift, motion relative to it and nonstructural contributions independently. A structural reference can move as its contributing structure changes. Preserve participating-mass force generation followed by whole-body inertia. No weighting law, brake, sleep predicate or orbital classifier is preselected. The purpose is better separated observation and analysis; interpretation of trajectories is not an extra acceptance gate.

Numerical inspection can inform earlier boundaries, but mathematical changes need their own derivation, independent controls and source-pinned validation. Do not bury them in file moves or native-reuse substitutions.

## Relationship to ongoing work

The retained-to-C3 bridge remains incomplete. #341 adds explicit diagnostic requests and honest unavailability for retained flat execution. #342 exposes a structural source handoff but does not admit or execute a retained C3 hierarchy. Owned prepared admission and committed occurrence mapping remain required. Coordinate that work with the ownership map; do not duplicate it in a second producer or replay geometry in a host approximation.

Full accounting/admission acceptance and GPU qualification keep their existing limits. Held #75/#111/#123 are unchanged. This refactoring plan does not activate them or authorize implementing future Analyst decisions.

## PR and context handoff

Each PR names the pass, source base, responsibility changed, preserved behavior, exact validation and remaining limits. Prefer one coherent boundary per PR; no fixed file-count or line-count rule. Update current state and this plan's progress rather than creating competing queues.

At each stopping point leave: current branch/SHA and dirty-work disposition; accepted versus pending changes; commands/results and evidence locations; known failures with exact reproduction; decisions and unresolved questions; and the next small task with its required files. A fresh context should resume without repeating broad exploration. Keep historical evidence available through Git and component records.

## Progress

2026-11-15: [the responsibility map](../engine/docs/RESPONSIBILITY-MAP.md) from #344 and the publication-cut correction from #345 are accepted and integrated after successful hosted qualification. The correction retains exact equality and native pending-work controls; no production formula/traversal changed. #342 still requires combined qualification with the correction before integration. The next bounded implementation is the mechanical ActiveSession split described in the map, preserving explicit dependencies. The broader native-reuse audit, retained C3 bridge and #324 mathematics remain separate pending work.
