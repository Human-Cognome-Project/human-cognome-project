# Engine and Analyst-facing refactoring plan

Authorized direction: 2026-11-15. Owner: [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Numerical derivation remains [#324](https://github.com/Human-Cognome-Project/human-cognome-project/issues/324).

## Purpose and boundaries

Break mixed, monolithic systems into coherent work units that can be understood and changed with the relevant context. Make ownership, inputs, outputs and lifetimes explicit. Adopt existing native Taichi machinery where it supplies the required behavior. Smaller files are useful only when they improve these responsibilities.

The viewport and temporary API front end are controls through which a human or agent can emulate Analyst functions until their concrete implementation is formalized. The viewport may remain useful longer. Identify which existing operations are Analyst-facing and which belong inside execution ownership; do not let temporary front ends become the numerical authority. Preserve the distinction between the conceptually designed Analyst and its still-unimplemented concrete decision-making. Do not create a speculative universal Analyst API, new orchestration framework or second scheduler.

This plan authorizes inspection and bounded proposals, followed by focused reviewed implementation. It does not fix a final module layout, select a new intent formula, or declare an SNode storage relationship to be a composition relationship. Preserve [system boundaries](onboarding/SYSTEM-BOUNDARIES.md), [work discipline](onboarding/WORK-DISCIPLINE.md), and applicable component contracts.

## Agent focus: coherence frames and Analyst control

User clarification, 2026-11-16. Parent values begin at smaller constituent levels
and aggregate through the composition hierarchy up to the top. This prior
structure establishes the frame required for coherence. Whole-particle fields
can affect the degrees of freedom available within that frame. "Free space"
here means mathematical degrees of freedom, not merely unoccupied spatial volume.

Under #324, inspect how accumulated parent structure defines that freedom and
how whole-particle response operates within it. Merely separating two additive
force totals does not establish this relationship. Trace a nested/repeated
composition from parent contributions to the whole-particle response, identifying
where the current implementation preserves, loses or has not yet implemented
the distinction. State the mathematical representation and supported conditions
before proposing an algorithm; do not silently choose projection, clipping,
rigidity or a new constraint solver. This is the intended model to investigate,
not a claim that existing code already enforces it. Keep structural intent,
contextual influence and actual tick trajectory separately observable.

The Analyst occupies the player/controller role in the gaming-engine structure.
Humans (currently the user) and LLM agents exercise or supplement that role while
the concrete Analyst implementation is developed. The viewport and temporary API
are control/observation interfaces for that role, not the Analyst itself and not
numerical authority. Preserve three explicit responsibilities:

| Responsibility | Owned decisions or behavior |
| --- | --- |
| Analyst/player/controller | Select the study, focus and interventions through the applicable operations. |
| Viewport/API/harness controls | Translate authorized requests and expose completed observations; keep display and request handling separate from simulation policy. |
| Engine execution | Own validated composition/instances, lifecycle, coherence mechanics and numerical response; enforce supported-operation boundaries. |

For the next inspection, map an Analyst request through its control adapter into
engine execution and back to completed observation. Identify any decisions or
state that sit in the wrong layer. Pair that trace with the native nested-instance
and accounting comparison below. Propose focused changes by responsibility;
keep mathematical changes, native replacements and mechanical moves separately
reviewable. Do not implement speculative Analyst decision-making to complete
this separation.

### Catalogue updates and minimal disruption

User clarification, 2026-11-16: the Minecraft comparison also expresses an update
boundary. As far as supported dependencies allow, the shared catalogue, active
instances, engine execution and Analyst controls should evolve with minimal
disruption to the combined experience. Publishing a new reusable item or
interaction changes availability; it should not inherently rebuild unrelated
active instances or restart the whole study. Existing pinned instances must
remain coherent; adoption of a changed definition is a distinct operation.

Inspect catalogue publication, revision pins, native compilation/layout reuse,
instance adoption and invalidation separately. Trace an unused catalogue
addition, a revision used by one of two instances, and an interaction introduced
for only one instance. Identify exactly which state or compiled artifact must
change and why. Verify actual Taichi restrictions; this requirement does not
assert arbitrary mutation of a compiled SNode layout. Prefer the smallest
supported update/rebuild boundary and document unavoidable wider invalidation.
Do not introduce automatic migration policy or a new hot-reload framework as
part of the inspection.

## Inspection clarification: nested assemblies and live instances

Maintainer direction, 2026-11-16, following the user's clarification. The intended
object model is reusable structural assemblies instantiated within an active
study, with scene/instance-dependent attachments distinguished from primary
composition. One resulting active SNode tree is compatible with construction
from nested assemblies. Distinct identity for each live occurrence does not by
itself require flattening the object into a separately owned host ledger for
every constituent. Rollup is intended to enable this compositional use, retaining
the object's internal structure while exposing the selected operating level.

The user's coherence framing is explicit: structure is the frame of what is
required for coherence; additional elements supply the degrees of freedom within
those frames. Degrees of freedom is intended in its mathematical sense, not merely
as spatial movement. Inspect how that relationship is represented and maintained across
nested instances and rollups. Do not reduce this distinction to a catalogue of
attachment types, or assume that a coherence frame means a fixed spatial origin,
a rigid body, or a newly selected constraint/force law.

This is the inspection target, not a finding that Taichi supplies a game-engine
object-class or scene-attachment API. Verify the pinned fork's exact construction,
compilation, storage-instance and ownership mechanisms. Distinguish an SNode
layout node, a compiled layout/tree, runtime storage cells/instances, and an HCP
conceptual object occurrence. Establish their actual mappings rather than
assuming either equivalence or incompatibility from terminology.

The native-reuse pass must cover:

- Trace a reusable composed definition instantiated twice in one study, including
  a nested repeated subassembly. Identify which layout/compilation can be reused,
  which storage/state is per instance, and how the native path identifies,
  activates, updates, releases and reuses each instance. Determine whether
  subtree construction/reuse requires cloning, indexed storage, separate trees,
  or another existing mechanism; document actual support and limitations.
- Attach a contextual relationship to only one instance. Show how primary
  composition remains distinct from that attachment and whether Taichi directly
  represents the relationship or HCP must supply explicit typed metadata.
  Neither raw SNode parentage nor an arbitrary membership edge establishes
  semantic composition without the validated mapping.
- Trace aggregate/parts exposure and rollup through those same structures.
  Determine what remains nested, what is exposed for execution, what is merely
  navigation, and which identities survive exposure changes. Do not expand
  hidden participants or create additional physical mass merely to navigate.
- Compare current occurrence maps, claim/recycle tables, generation/lifetime
  guards, reservation and capacity ledgers against native structure/runtime
  ownership. For each, name the native authority, HCP-specific semantics still
  needed, redundant state that can be removed, and missing native support.
  Include resource backing and release accounting, not only particle kernels
  and compiler dispatch. Preserve exact provenance and stale-reference protection
  while assessing whether they can attach to native instances.
- Separate upstream Taichi mechanisms, fork extensions and existing HCP wrappers
  in the evidence. Reusing our bootstrap is useful but does not complete the
  inspection of Taichi itself. Use exact paths/symbols and a focused instance/
  attachment/lifetime witness; inspect emitted artifacts when making claims
  about compilation reuse or execution behavior.

Treat the current framework as a concrete comparison surface that can be
simplified or replaced. Its existence is not justification for retaining
parallel lifecycle/accounting machinery. Do not add Python to operational paths
to obtain a convenient abstraction. Setup expense alone is not a soft gate
against reuse that benefits sustained operation.

Connect the resulting structural-versus-contextual mapping to #324: primary
identity/composition supplies the proposed structural-intent accounting;
nonstructural influences and the actual tick trajectory remain separately
observable. This inspection selects no force, brake or orbital-stability law and
does not relabel the existing combined candidate as structural intent.

Deliver a bounded source-pinned comparison under #343 before treating additional
object-instance/lifecycle wrappers as necessary. Existing accepted work remains
accepted; coordinate the inspection with owner integration and propose any
replacement as a separate reviewed change. A complete mathematical redesign is
not a prerequisite for this structural inspection.

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

2026-11-17 contributor (awaiting review):
[Owner-local immutable recipe](../engine/docs/EXCHANGE-RECIPE-SHARING.md) at
`6be856c7` implements #363's explicitly authorized private spare-sharing path.
Initial adoption and public/domain value APIs keep independent recipes;
mutable views and native authority remain separate. Admission/census count the
single owner-owned Bundle, with actual header/control/domain lifetime effects
included in measured 26,191 / 28,515 / 26,963-byte retained reductions. Public
v1/v2 copy/assignment/move and partial-construction unwind are qualified alongside
all native owner guards. Quick CI, 16 Clang/eight GCC CTests, both standalone
suites, both six-test warm compatibility suites, 48 owner mode runs and eight
standalone exchanges pass. Main `c63a1c24` and reviewed unmerged #363 are
preserved; review and hosted gates remain separate.
No cross-owner deduplication, generic wrapper or new execution authority is added.

2026-11-17 contributor (awaiting review):
[Projection recipe copy attribution](../engine/docs/EXCHANGE-RECIPE-COPY-COST.md)
at `ac4ab9cf` selects the two identical immutable Bundles in the owner's committed/
spare views. Actual separate backing costs 25,735–28,059 requested bytes per copy
in three fixtures; embedded headers are counted separately, not as new requests.
Live view state remains independent while const recipe reads/staging allocate
nothing. Proposed owner-private sharing preserves full meaning, detached public/
domain copies and native authority; it is not implemented. Quick CI, four native
Clang/GCC inspections and both full single-owner integration paths pass. Main
`ad38b6cd` and explicit source-accepted #362 ancestry are preserved; production
is unchanged. Review/hosted gates and indexed/catalogue/producer/accounting/#324
remain separate.

2026-11-17 contributor (awaiting review):
[Private exchange direction sharing](../engine/docs/EXCHANGE-DIRECTION-SHARING.md)
at `faa7fc8e` implements #361's authorized one-payload scope. Explicit target
state on the existing binding coordinates owner validation, capture and commands;
standalone one-way semantics and all full pins/native authority remain. Actual
one/four-selection owner fixtures save 2,998/11,992 retained requested bytes with
table, shared-block and census effects included; no cross-owner/revision sharing
or new wrapper/registry. Quick CI, 16 Clang/eight GCC CTests, both standalone
suites, 48 owner mode runs and eight standalone exchange cases pass, including
equal alternatives, exact/one-short work, all allocation unwind, weak/domain
lifetime and following ticks. Main `3fd1936e` includes #360; unmerged accepted
#361 ancestry is explicit and its native run now passes. Review/hosted gates
and indexed/catalogue/producer/accounting/#324 work remain separate.

2026-11-17 contributor (awaiting review):
[reverse declaration graph cost](../engine/docs/EXCHANGE-DIRECTION-COPY-COST.md)
at `657d0d87` measures the owner's duplicate immutable forward/reverse payloads.
Actual retained reverse copying costs 26–80 requests / 2,046–6,304 requested
bytes across three fixtures; all buffers and weak/domain lifetimes are checked.
Explicitly swapped const alias roles allocate nothing and retain complete pins,
order/counts/mass/node/stored mass. Equal cardinalities do not identify direction.
Proposed sharing uses the existing private binding's explicit operation role;
coordinate owner/capture/command/census readers and preserve all native authority.
Runtime is unchanged relative to source-accepted, unmerged #360. Main `45b5cf07`
and integrated #358/#359 ancestry are preserved. Quick CI, engine recipe and
fresh standalone Clang/GCC suites pass all 3,938 recipe controls. Review this
candidate before implementation; hosted CI remains authoritative and indexed/
catalogue/producer/accounting/#324 work remains separate.

2026-11-17 contributor (awaiting review):
[exchange leaf authority](../engine/docs/EXCHANGE-LEAF-AUTHORITY.md) at `0750a2cd`
implements #359's authorized second-inventory removal. Canonical byte-source
paths and parents retain all ordered semantic pins/counts, while the separate
leaf vector/copy/export and actual construction/census charges disappear.
Complete source validation, matching/permutation, stored-mass provenance,
detached/shared lifetime and native state remain. Actual collector retained
requests fall by 1,992/3,450 bytes; mixed reverse copy by 2,490; the native owner
fixture by 2,072 including headers. Quick CI, 16 Clang/eight fresh GCC CTests,
both standalone suites and selected single/multiple-owner exchanges passed.
Main `d1139a89` and unmerged source-accepted #359/#358 are preserved. Review/
hosted qualification and indexed/catalogue/producer/accounting/#324 remain separate.

2026-11-17 contributor (awaiting review):
[exchange leaf-inventory cost](../engine/docs/EXCHANGE-LEAF-INVENTORY-COST.md)
at `87061d3b` selects a concrete second structural view: each flat leaf repeats
fields already owned by canonical byte-source paths and direct parents. Runtime
binding/staging consumes those required semantic parents, not the flat table;
native handles cannot replace their complete HCP pins. Actual public/bounded/
reverse-copy factories retain 712/890 duplicate nested requested bytes for
eight/ten leaves plus distinct vector capacity. Borrowed traversal allocates
nothing and preserves ordered full identity after source death. This is a
measured proposal, not removal. Quick CI and engine/standalone Clang/GCC values
passed, including 4,048 structural recipe controls. Main `67861a37` and explicit
source-accepted #358 ancestry remain; production is identical to #358. Review/
hosted qualification and indexed/catalogue/producer/accounting/#324 remain separate.

2026-11-17 contributor (awaiting review):
[membership target-pin authority](../engine/docs/MEMBERSHIP-PIN-AUTHORITY.md)
at `4f27d26d` implements #357's accepted removal from integrated main `fd0817f6`.
One complete target pin supplies each membership field object; explicit kind,
context/count/revision, short addresses and detached/shared-catalogue lifetime
remain. Collector, bind/exchange/plan readers and independent retained census
migrate together. Actual live/catalogue fixtures retain 1,084/2,168 fewer
requested bytes for eight/sixteen rows, without weakening validation or quotas.
Extended GCC qualification also found an existing audit range-lifetime bug and
projection-copy unwind failure; separate repairs preserve all original guards
and add standalone refusal coverage. Final local quick CI, 15 Clang CTests,
seven fresh GCC CTests, both standalone suites and exchange/compiler/lifecycle
invocations passed. Review/authoritative hosted qualification remain pending;
native claims, candidate state, broader adoption/accounting and #324 are separate.

2026-11-16 contributor (awaiting review):
[membership field-key cost comparison](../engine/docs/MEMBERSHIP-FIELD-KEY-COST.md)
at `ec21b153` attributes duplicate row ObjectKeys beside full target pins in the
actual live/bounded/catalogue collector. Eight/sixteen rows retain 380/760 nested
requested bytes plus in-vector extent; this is a measured candidate, not removal
savings. A scoped proposal reuses the existing pin object while preserving an
explicit field kind, shorter group addresses, context/revision/count semantics
and census/consumer validation. Local quick CI, value/native controls and fresh
standalone Clang/GCC suites passed. Production is identical to the accepted #356
dependency, with current main `0bd4db55` preserved. Review/hosted qualification
remain separate; no new registry, owner, native guard or #324 formula is added.

2026-11-16 contributor (awaiting review):
[owned plan study authority](../engine/docs/PLAN-STUDY-AUTHORITY.md) at `57615368`
implements #355's accepted row-UUID removal. Full supplied source/occurrence
validation precedes numeric-only emission; existing header + numeric rendering
preserves detached identity, independent lifetime and native claim alignment.
Actual historical/new producers show 170/2,040 fewer requested bytes for two/24
rows in cumulative, retained and scoped peak measurements, matching Clang/GCC.
Local quick CI, 12 focused native tests, five fresh GCC tests and four standalone
tests passed; builder checks include all 162 C++ allocation failures. Main
`a06362be` and explicit source-accepted #355/#354 history are preserved. Hosted
qualification/review remain separate; claims/candidate state and #324 are intact.

2026-11-16 contributor (awaiting review):
[plan study-identity cost comparison](../engine/docs/PLAN-STUDY-IDENTITY-COST.md)
at `52be505c` attributes actual per-occurrence UUID buffers that repeat the
existing owned plan source-header study. Two/24 rows retain 74/888 nested
requested bytes plus in-vector member extent; existing header access is free
of allocation and preserves standalone lifetime. Local quick CI and all 4,465
builder checks passed under Clang/fresh GCC. The proposed coordinated row
cleanup awaits review; no production representation changes. Explicit dependency
#354 is reconciled at `eff64deb` with main `1337bbcc`, preserving #353 ancestry
and #354's reviewed runtime/tests. Native guards/maps, candidate state and #324
remain separate.

2026-11-16 contributor (awaiting review): the
[origin definition-key comparison](../engine/docs/ORIGIN-ASSOCIATION-COST.md)
at `fcef214c` uses current main `9518d043` and explicit source-accepted #352
dependency `9a86962e`. The checked boundary confirms one concrete within-row
duplicate: `Origin.definition` equals the object owned by its full pin, with
independent backing and the same lifetime. Actual attribution finds eight
allocations / 208 retained requested bytes in the four-origin fixture; clearing
rows releases both copies, and direct pin-member access allocates nothing.
Quick/focused Clang and fresh GCC checks passed. No production field/map is
removed; the comparison specifies current readers and invalid-provenance
questions for a separately reviewed accessor/removal with equivalence controls.

2026-11-16 contributor (awaiting review):
[Origin pin authority](../engine/docs/ORIGIN-PIN-AUTHORITY.md) at `417aed19`
implements #353's accepted duplicate-key removal on main `5f45f566`. The complete
shared/local pin supplies the definition through a checked borrowed accessor;
all producers/readers preserve alternatives, occurrence identity and detached
lifetime. Actual producer requests, retained bytes and scoped peak decrease
against both the old-shaped reference and an isolated historical producer.
Local quick CI, 16 focused Clang tests and five fresh GCC tests passed, including
shared/local snapshot/label equivalence and invalid provenance refusal. Review
and authoritative hosted CI remain pending. #353's old-member witness requires
integration reconciliation; native claims, candidate state and #324 are unchanged.

2026-11-16 contributor (awaiting review):
[checked prepared associations](../engine/docs/PREPARED-ASSOCIATIONS.md) at
`33cffcb3` implement the accepted first view/consumer slice on main `677d151f`.
Existing immutable source/plan and actual handle arrays are borrowed behind
generation/tick/full-handle validation; one observer consumes its temporary
detached metadata. Complete snapshot/readback equivalence and measured savings
(39 C++ allocations / 3,847 requested bytes in the fixture) passed across the
CPU worker/path matrix and GCC checks. View/loan overhead is 24/40 stack bytes,
with no new owning storage or source-owner reference. All maps, candidate state,
native publication and operation sets remain unchanged. See the evidence for
scope/limits; hosted qualification and review remain pending before acceptance.

2026-11-16 maintainer: #350 and #351 accepted and integrated after full hosted qualification (reviews 5923994794/5925874274). Next is one checked private association view and consumer over existing PreparedStudyOwner source/plan, preserving actual claim/publication state and measuring copy savings. Follow PRIVATE-INSTANCE-BINDING.md; direct indexed guard protocol, mixed-revision provenance and #324 math remain separate. Both handoffs record exact CI and combined-merge limits.

2026-11-16 contributor (awaiting review): the
[private instance-binding contract](../engine/docs/PRIVATE-INSTANCE-BINDING.md)
is pinned to main `fa7f09e4` and explicitly depends on source-accepted unmerged
#350. It compares both current owners and specifies exact source/role pins,
indexed-storage lifetime/ABA guards, shared immutable associations, preserved
candidate state and commit-before-publication. It proposes one checked consumer
of the existing prepared plan/source before any map removal. This is a
documentation proposal, not an implemented binding or representation migration;
mixed revisions, heterogeneous layouts, producer migration, full accounting and
#324 math remain separate.

2026-11-16 maintainer: #349 is integrated by this merge after full hosted qualification (178 regression passes and all native gates). #350 native nested-instance inspection is source-accepted in review 5923994794, hosted matrix pending. Next is a private instance-binding contract, comparing current owners against native indexed storage and preserving exact provenance, semantic incarnation and atomic publication. Its bounded integer witness does not implement production mixed-revision adoption, D7 rollup or coherence mathematics. Broader catalogue/provenance and producer changes remain separately reviewed. See both handoffs for exact source and CI status.

2026-11-16 maintainer: #347 is integrated; #348 is integrated by this merge after full hosted qualification. #349 is source-accepted in review 5923001157 with hosted native matrix pending. Its private owner bridge does not complete the broader native-instance/accounting inspection. The next task is that inspection, including catalogue-update dependency scope above, before more lifecycle wrappers or public expansion. See the handoffs for the pending Fields-address correction and exact CI evidence.

2026-11-16 contributor inspection (awaiting review): the
[prepared-admission seam map](../engine/docs/PREPARED-ADMISSION-SEAM.md) is pinned
to current main `ee31e88a`. It identifies the existing atomic native bootstrap,
record/dispatch/reservation mechanisms, missing explicit comparison bindings and
fallible postcommit adoption risk. Proposed next slice: bounded structural
command-plan preparation with explicit owner bindings, before retained commit
or a new wrapper. Runtime behavior and #324 mathematics remain unchanged;
retained C3 and combined accounting remain incomplete.

2026-11-16 maintainer update: #342's corrected structural handoff and #346's mechanical ActiveSession implementation split are integrated after successful combined qualification. The moved session block/helper and retained logical code were independently compared against the reviewed dependency. Next is a focused native-reuse and ownership inspection at the prepared-admission seam, proposing the smallest bounded implementation before introducing wrappers. Retained C3 execution, the broader reuse audit and #324 mathematics remain incomplete. Older progress entries below retain their historical context.

2026-11-15: [the responsibility map](../engine/docs/RESPONSIBILITY-MAP.md) from #344 and the publication-cut correction from #345 are accepted and integrated after successful hosted qualification. The correction retains exact equality and native pending-work controls; no production formula/traversal changed. #342 still requires combined qualification with the correction before integration. The next bounded implementation is the mechanical ActiveSession split described in the map, preserving explicit dependencies. The broader native-reuse audit, retained C3 bridge and #324 mathematics remain separate pending work.

**Contributor Pass 2 implementation (awaiting review):** the
[mechanical session split](../engine/docs/ACTIVE-WORKBENCH.md#native-implementation-split--2026-11-15)
is implemented at public source `7b998501`, explicitly dependent on #342's
`9b5c1926` combined head. Session methods move unchanged to `active_session.cpp`;
the public header, target/dependencies and logical focus/restart behavior are
preserved. Quick local CI, source-identity witnesses, two native owner/observer
tests and four GCC 11 standalone tests passed. No behavior refactor, new logical
library, native replacement or completed admission bridge is claimed. The move
awaits maintainer review and hosted qualification; reviewed local evidence can
support provisional continuation. Subsequent reuse/ownership work remains
bounded by this plan rather than implied by the file split.
