# Engine and Analyst-facing refactoring plan

Current disposition (2026-11-21). Main `456cb681` integrates qualified #400–#402
with the exact reviewed `2062f00e` tree. The [compiler/JIT lifetime inspection](../engine/docs/COMPILER-JIT-LIFETIMES.md)
at `fb614350` follows #402 decision 7066447453 from integrated main. Definitions,
compiled data, launcher rows and JIT/ORC resources have distinct owners and
release boundaries. Legacy parameter refusal adds data without registration;
prepared refusal retains both data and registered kernel wrappers. Eight scoped
Clang/GCC cases and local quick pass; production and runtime saving are unchanged.
Review the proposed non-lazy native counts/ownership observations before eviction
or byte-accounting changes. Complete B, GPU and #324 stay separate. Earlier
pending/provisional progress below retains its source handoff state; the above
integration supersedes those pending statuses. Every merge is maintainer work.

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

The default retained capture remains v1/flat, with #341's prepared-C3 diagnostic unavailable in that mode. Structural source, sealed plan, bounded prepared ownership, combined native bootstrap and actual claim mapping are built. The [explicit active prepared submission](../engine/docs/ACTIVE-PREPARED-SUBMISSION.md) and [opt-in English pair profile](../engine/docs/RETAINED-PREPARED-CAPTURE.md) are integrated through #372/#373. #374's provisionally accepted actual-source refusal route resolves the earlier missing-connection obstacle using verified isolated restores. The remaining profile blocker is explicit membership comparison choice: English A has four unique inherited targets. The [membership decision](../engine/docs/RETAINED-MEMBERSHIP-DECISION.md) proposes a complete six-node qualification chain, unchanged ParentValue focus and concrete bounds, with native fixture evidence. Production still refuses unsupported memberships; no full retained-C3 execution or inferred Analyst policy follows. Do not duplicate native admission/installers or replay report geometry as authoritative state.

Full accounting/admission acceptance and GPU qualification keep their existing limits. Held #75/#111/#123 are unchanged. This refactoring plan does not activate them or authorize implementing future Analyst decisions.

## PR and context handoff

Each PR names the pass, source base, responsibility changed, preserved behavior, exact validation and remaining limits. Prefer one coherent boundary per PR; no fixed file-count or line-count rule. Update current state and this plan's progress rather than creating competing queues.

At each stopping point leave: current branch/SHA and dirty-work disposition; accepted versus pending changes; commands/results and evidence locations; known failures with exact reproduction; decisions and unresolved questions; and the next small task with its required files. A fresh context should resume without repeating broad exploration. Keep historical evidence available through Git and component records.

## Progress

2026-11-21 contributor (compiler/JIT ownership inspection awaiting review):
[Separate native owners and release boundaries](../engine/docs/COMPILER-JIT-LIFETIMES.md)
at `fb614350` follows #402 decision 7066447453 from integrated `456cb681`.
Existing refusal witnesses now observe definition/data/kernel-JIT-wrapper
release through test-only linker wrapping and existing non-lazy provenance/
launcher queries. Legacy refusal retains thirteen data objects without a new
JIT wrapper; prepared late refusal retains eighty data objects and seventy-eight
wrappers. Cached HCP data dies during explicit finalization, with launcher rows/
JIT wrappers surviving until Runtime member destruction. All prior native phase
counters match accepted evidence. Five Clang/three GCC cases and quick CI pass.
Production, eviction, byte accounting and saving remain unchanged. Review the
native read-only count/ownership proposal; do not infer full B, complete ORC
memory or safe per-candidate eviction from these counts. #400–#402 are qualified
and integrated parent-first; hosted qualification for this inspection is separate.

2026-11-21 contributor (Execution source-tree correction awaiting review):
[Owned source layouts, dependency order and measured costs](../engine/docs/SESSION-SOURCE-TREE-OWNERSHIP.md)
at `e226b76a` implements #401 review 7064542786 from `bed13aa7`. The existing
owner/drain replaces the raw pointer pair/manual teardown in Execution. Both
submit paths capture their registered source handles; prepared owner dies before
Harness and legacy FixedStudy before source retirement. Late legacy refusal
retires 4,096 requested source-root bytes; Execution increases 8 host bytes.
The native witness preserves both layout forms, real parameter failures,
partial Activity construction, old tokens/ID reuse, sibling values/counts,
healthy retry and following ticks. Scoped local quick and Clang/GCC session/
viewport checks pass. Review/hosted qualification and parent-first maintainer
integration remain separate. Next inspect compiler/JIT ownership and native
cache lifetime; do not infer eviction or full B from root retirement.

2026-11-21 contributor (legacy submission source-layout inspection awaiting review):
[Actual token lifetimes and minimal ownership proposal](../engine/docs/LEGACY-SUBMISSION-LIFETIMES.md)
at `a0ec440b` follows #400 acceptance 7064023495 from `32bf8764`. A real
negative/nonfinite parameter refusal retains one source root / 4,096 requested
bytes while Activity retires; four source roots remain live before Runtime
finalization after all Executions are destroyed. The test covers root-only and
primary-step layouts, early/owner/generation refusal, sibling reports/counts,
corrected retry and following ticks. Production is unchanged and saving is zero.
Review ordered Execution ownership before runtime implementation; preserve
FixedStudy-before-source retirement and existing prepared ordering. Local quick,
four Clang and two GCC cases pass. Hosted CI and integration remain separate.

2026-11-21 contributor (native traversal-tree lifetime correction awaiting review):
[Component-owned retirement and actual costs](../engine/docs/NATIVE-TRAVERSAL-TREE-OWNERSHIP.md)
at `8c32cba4` implements #399 review 7063069841 from `c0b8a216`. Separate
mechanical `1077e89e` extracts the existing record-map guard. Activity and
candidate selection now drain before dependencies die and retire their native
trees afterwards, including constructor unwind. NativeSet remains borrowed.
Both roots / 8,192 requested bytes retire after each late refusal; the guard
cost is 24 bytes per host component. Pool/list and compiler quantities remain
separate. Scoped CPU CI covers partial construction, post-launch failure,
repeated refusal/retry, old tokens across ID reuse, sibling state and ticks,
plus native activity/selection/contact and surrounding owner/launch behavior.
Runtime review, hosted CI and maintainer integration remain separate. After
acceptance, inspect legacy source-layout lifetime before another ownership
change; compiler/JIT eviction and #324 mathematics are not part of this fix.

2026-11-21 contributor (prepared-submission lifetime inspection provisionally accepted in #399):
[Native release boundaries and bounded tree ownership](../engine/docs/PREPARED-SUBMISSION-LIFETIMES.md)
at `cf030244` follows #398 review 7062570528 from integrated `c2b2d4d2`.
Real session plan/placement refusals retire three roots but retain two native
set roots under Program ownership, alongside separately retained compiled data.
The proposal keeps NativeSet borrowed and stores owning tree handles in the
existing Activity/selection components using native destruction. Constructor
unwind, dependency ordering, retirement/reuse and actual pool/list effects must
be qualified in a reviewed runtime follow-up. Production and achieved saving
remain unchanged. Local CPU evidence covers both host paths/workers and
preserved sibling claims/numerics, repeated refusal, retry and following ticks.
Compiler/JIT eviction, broader accounting/adoption, GPU and #324 stay separate.

2026-11-20 contributor (checked byte-owner runtime awaiting review):
[Independent index, complete provenance and actual costs](../engine/docs/PROJECTION-BYTE-OWNER-INDICES.md)
at `ec5ca687` implements #393 review 7047998750 from `521afbda`. Private rows
shrink 464→376 bytes, preserving an independent aggregate-owner selector, both
source checks and complete public identities/pins. Membership follows that index
without its old temporary owner map. Both-view retained/peak reductions are
676 / 2,704 / 1,278 bytes; stronger guard work and separate output costs are
explicit. Scoped local CI passes through actual foreign/wrong-owner consumers,
full lifetime/quota/work refusal/retry and frozen native publication/following ticks.
Runtime review, new authoritative CI and parent-first integration remain separate.

2026-11-20 contributor (projected byte-owner inspection accepted in #393):
[Actual complete-key ownership and checked association](../engine/docs/PROJECTION-BYTE-OWNER-KEYS.md)
at `502f555d` follows accepted #392 `51912690`. Fourteen-row output retains nine
independent owner UUID requests / 333 bytes; real mixed owners retain 296 per view.
Inline 96-byte owner keys and empty path headers are separate attribution within
the existing rows. Production is unchanged and runtime saving is zero. Scoped
local CI passes, including actual language/foreign-context consumer refusals,
independent byte/owner index replay, full pin/path/role, exact work/no-new retry,
detached outputs and frozen native publication/ticks. Review 7047998750 accepts
final `521afbda` and authorizes the independent owner-index runtime above. The
original inspection remains source-pinned with zero saving at that source.

2026-11-20 contributor (projection-owned study runtime accepted in #392):
[Checked contextual indices, complete output and replacement costs](../engine/docs/PROJECTION-HEADER-IDENTITY.md)
at `68062bde` implements #391 review 7045231362 from `26dec2a0`. Private rows shrink
512→464 bytes under the existing owned header, with independent numeric source
validation and complete public/provenance keys. Readers no longer accept detached
numeric rows. Both owner views save 510 / 2,040 / 1,020 retained/peak bytes after
actual overhead; exports retain their costs. Scoped local CI passes through
foreign-context actual consumers, malformed source/index/node/path, full value/
domain/work refusal/retry and frozen native publication/following ticks. Review
7047017029 accepts continuation from final `51912690`; the remaining native
regression gate and maintainer integration remain separate.

2026-11-20 contributor (projected row-study inspection accepted in #391):
[Actual strings, complete keys and header ownership](../engine/docs/PROJECTION-STUDY-IDENTITY.md)
at `6a208a14` follows accepted #390 `db6a4bda`. Public fourteen-row output owns
518 repeated study bytes; mixed owners own 444 per view beside existing headers.
Byte-owner studies and inline capacity are separate, with zero runtime saving.
Local CI passes: 304 existing measurements stay unchanged, forty owner views,
sixteen factory/copy observations and real foreign/max-entry/path/work controls.
Review 7045231362 authorizes the header/numeric-key runtime above with checked
reader ownership and preserved full exports. Original inspection evidence remains
source-pinned; broader provenance and #324 remain separate.

2026-11-20 contributor (projected definition-index runtime accepted in #390):
[Checked recipe indices, explicit owning output and net costs](../engine/docs/PROJECTION-DEFINITION-INDICES.md)
at `223e874a` implements #389 review 7023633130 from main `646df179`. Contextual
reads preserve source/node/full-pin meaning while owning exports detach complete
provenance; private rows shrink 640→512 bytes. Both real views save 1,302 / 5,208 /
2,604 retained/peak bytes after index overhead. Quick CI, sixteen Clang/thirteen
GCC CTests, thirteen/eight hierarchy cases and both standalone suites pass,
including malformed in-range indices, source/domain/work/quota retry and frozen
native publication/ticks. Export costs stay separate; increased membership guard
work and temporary factory overhead are explicit. Review 7044020673 accepts scoped
continuation; remaining authoritative qualification and integration stay separate.

2026-11-19 contributor (projected definition-pin inspection integrated in #389):
[Existing recipe lifetime, actual pins and public row values](../engine/docs/PROJECTION-DEFINITION-PINS.md)
at `3de88e61` follows accepted #388 `fa83360e`. Real committed/spare rows repeat
definition buffers despite an already shared exact recipe; production saving is
zero at that source. Focused local CI passes with 186 unchanged cost records,
forty owner-view observations and 1,808 detached row-copy allocation refusals.
#389 accepted the checked recipe-index/public-output direction; the runtime above
measures actual replacement overhead and preserves provenance/native publication.

2026-11-19 contributor (unique membership target table accepted in #388):
[Owned table, checked indices and actual costs](../engine/docs/MEMBERSHIP-TARGET-TABLE.md)
at `6da22846` implements #387 review 7020608389. Targets deduplicate by complete
scope/address/revision within the catalogue, preserving every row and detached
value. Native owner retained/peak requests fall 546 / 2,979 / 1,357 bytes after
overhead; increased live peak/setup work and single-row allocation are explicit.
Staged copy assignment keeps table/index associations intact through refusal;
allocation-free moves transfer both tables. The source-pinned record owns local
CI, 58 comparisons, new small controls, failure/retry/lifetime/publication evidence
and remaining limits. Main `f228581d` integrates qualified #387; #388's runtime
review accepts the implementation and measured tradeoffs. Remaining hosted
qualification and maintainer integration stay separate.

2026-11-19 contributor (membership target-pin inspection accepted in #387):
[Actual repeated full pins and catalogue lifetime](../engine/docs/MEMBERSHIP-TARGET-PINS.md)
at `01b09c77` follows accepted #386 `1c10f14c`. Real returned live/catalogue/copy
pins request 676 / 1,352 bytes; five observed representatives occupy 424. Native
owners request 267 / 1,068 / 534 pin bytes while each uses one exact target.
Inline ToolRef attribution is separate; production and runtime saving remain
unchanged. Quick CI, membership suites, five Clang/three GCC native cases and
GCC 11/13 standalone suites pass, including 36 target observations, 4,922 checks
and 52 unchanged factory/owner/export records. Context/full revision, source and
detached lifetime, work/allocation refusal, publication and following ticks remain.
Review catalogue-local unique full-pin ownership and checked indices with actual
replacement overhead. The recipe reference is not a payload lifetime pin; native
claims do not replace semantic revisions. Verified evidence and the bounded
proposal remain separate from implementation. Main `6dcf3fef` integrates #384;
#386 is accepted with remaining hosted regressions pending. All merges remain
maintainer work, with #385/#386 and this inspection explicitly stacked.

2026-11-19 contributor (membership header runtime integrated in #386):
[Catalogue-owned study and detached exports](../engine/docs/MEMBERSHIP-HEADER-IDENTITY.md)
at `e7733a8c` implements #385 review 7016915894 from accepted `fc0e7f65`.
Numeric entry/path stays private under one owned study header; public count/index
extraction retains full owning keys. Net retained/requested peaks fall 303 /
1,212 / 606 bytes in real owners and 680 / 1,360 in live/catalogue factories.
Copy/export overhead is measured separately. Quick CI, nine Clang/seven GCC
CTests, thirteen Clang/eight GCC native cases and GCC 11/13 standalone suites
pass, including 34 actual before/after comparisons, 27 owner observations,
4,886 value checks and 872 export allocation refusals. Foreign-study association,
source/catalogue/domain/weak death, exact work/unwind and atomic native
publication/following ticks pass. Verified evidence includes the reused before
archive and failed development compile. #386 passed hosted qualification and
is integrated at main `0e3c0fdf`.

2026-11-19 contributor (membership study inspection accepted in #385):
[Catalogue header identity](../engine/docs/MEMBERSHIP-STUDY-IDENTITY.md) at
`47c97926`, based on accepted combined #384 `4d4695ac`, attributes actual row
study strings already owned by the catalogue header: 296 / 592 bytes in live/
catalogue factories and 111 / 444 / 222 in real owners. Inline UUID extent is
reported separately; runtime saving is zero. Header/numeric comparison and full
rendering preserve context, entry/path, complete target pin/count and detached
catalogue copies. Quick CI, native membership suites, five Clang/three GCC
hierarchy cases and GCC 11/13 standalone suites pass, with 4,527 value controls,
twenty owner and sixteen factory/copy observations. #385 accepts the retained-row
and detached-value boundary; the implementation above preserves native source/
claims/publication and prices association work. The maintainer integrated #383 into
#384 and #382 into main `c961fe86`; combined #384 hosted native and this new
review/CI remain separate. All merge events stay with the maintainer.

2026-11-19 contributor (private parent runtime provisionally accepted in #384):
[Owner recipe parents](../engine/docs/EXCHANGE-RECIPE-PARENTS.md) at `45ff81f3`
implements #383's specified strong-pin/private-index representation on accepted
#382 `e7915da7`. Ordered parents resolve from the exact owner recipe; public/value
copies detach. Actual retained/requested peaks fall 1,245 / 4,980 / 2,217 bytes
after overhead, while equal-three setup work rises 444 units and public value
extents grow. Quick CI, thirteen Clang/eight GCC native cases and GCC 11/13
standalone suites pass, including twelve comparisons, nine private pin/index/
copy/weak lifetime controls and full owner refusal/unwind/publication/ticks.
Initial Clang assignment accounting failure and the targeted reconstruction
fix remain in source/evidence. Main `952e1651` integrates #381; accepted #382/#383
remain separate gates. Maintainer integration must adapt #383's historical
private-copy observer to indexed parents, retain public controls and qualify
the combined tree. New hosted CI/runtime review and all merges remain separate.

2026-11-19 contributor (provisionally accepted in #382; historical measurements):
[One adoption-local validator](../engine/docs/ADOPTION-LOCAL-VALIDATOR.md) at
`3d9424e4` implements #381's accepted scope. One buffer set spans the declaration
loop, releases before spare/coordinator setup, and retains full validation on
the existing shared meter. Invalidation precedes inventory/checked-sum refusal.
Actual whole-construction requests fall by 129,840 / 86,560 bytes for four/three
selections, with unchanged retained/observed peaks and unchanged single-selection
control. Quick CI, thirteen Clang/eight GCC cases pass: 27 production allocation
observations, 54 pre-stage inventory refusals, full owner budget/lifetime/native
publication/tick and standalone controls. Twelve actual before/after comparisons
agree; baseline binaries and checked evidence are retained. #382 `e7915da7`
is provisionally accepted in comment 7013865241; hosted qualification remains
separate. Main `952e1651` now integrates #381, superseding the original pending
dependency state. All merge events remain maintainer work.

2026-11-18 inspection/proposal (accepted in #381; historical measurements):
[Adoption-validator buffer reuse](../engine/docs/ADOPTION-VALIDATOR-REUSE.md) at
`244ef0b0`, independently based on main `4bb937d8`, measures five real temporary
buffer requests / 43,280 bytes per selection and proves existing scratch replay
against full fresh facts. Four/three selections expose 129,840/86,560 candidate
cumulative requested-byte reductions; runtime saving is zero. Quick CI, four
Clang/two GCC modes and the complete mixed-owner control pass, nineteen observed
fixtures with order/mask/source/work/refusal/retry and lifetime checks. The authorized
runtime follow-up above preserves the shared meter/full validation and measures
actual constructor overlap, budgets, unwind and native publication. The original
pending integration states are superseded by main `3e76bf1e`; this inspection's
local evidence remains source-pinned. GitHub CI remains authoritative.

2026-11-18 contributor (awaiting runtime review):
[Native refresh metadata transport](../engine/docs/NATIVE-REFRESH-METADATA-TRANSPORT.md)
at `6196eca8` follows #379's explicit authorization. Three second host uploads
become full reserved native copies inside the existing postclaim boundary.
Host request reduction is 8,816 bytes at H8 / 9,312 at H70, with native-copy bytes
reported separately. Other staging/metadata, work, allocations/bindings and
numerical kernels remain. Quick CI, nine Clang/five GCC CTests and fifteen shared
case runs pass, including exact padding/u64, real bootstrap/admission/exchange,
recycled claim remapping, survivors/retirement, healthy retry, late failure and
following ticks. Verified evidence retains 36 refresh observations, 30 refresh
fault/guard controls, 24 shared commits and the corrected development remap setup.
Stacked on unmerged #379; authoritative GitHub CI and maintainer integration are
separate gates. All merges remain with the maintainer.

2026-11-18 inspection (provisionally accepted in #379):
[C3 refresh metadata transport inspection](../engine/docs/REFRESH-METADATA-TRANSPORT.md)
at `9043bcb8` observes three full reserved inputs uploaded identically to candidate
and live arrays. The second uploads total 8,816 bytes at H8 or 9,312 at H70;
independent full-extent native copies match exactly, while truncated copies fail.
Quick CI, five Clang and three GCC suites pass, including 36 actual observations
and existing refresh regressions. Runtime/fork behavior at the inspection source
is unchanged; achieved saving is zero. The authorized runtime follow-up above
covers the shared commit routine's bootstrap/admission/exchange callers.
The inspection starts independently from main `22ea6d55`; accepted
#375–#378 remain open, with their evidence/integration gates kept on those branches.
GitHub CI is authoritative; all merges remain with the maintainer.

2026-11-18 contributor (awaiting review):
[Retained membership comparison/focus/resource decision](../engine/docs/RETAINED-MEMBERSHIP-DECISION.md)
at `9d9d491f` follows #374 comment 6985194174 from current main `66edfb18`.
Read-only source closure adds inherited Germanic/Indo-European targets to the two
direct memberships, with NULL masses and per-occurrence diamond deduplication.
The proposed complete equal-cohort chain uses six fields/nodes/rounds, five links
and current ParentValue focus. Executable plan controls reject omission, forests,
overlapping forks and four one-short budgets; six native fixture modes preserve
source NULLs, actual claims, complete rows and a following tick. Quick CI, six
Clang and three GCC suites pass. Unary complement responses are zero; no new
category-force semantics or #324 mathematics is supplied. Review the explicit
decision before production preset activation and genuine retained ticking.
Production/fork/database/workflows remain unchanged, #374's final gates separate
and all merges with the maintainer.

2026-11-18 integrated in #378 (historical source-pinned local record):
[Private bootstrap consumption](../engine/docs/PREPARED-BOOTSTRAP-CONSUMPTION.md)
at `9568f7e9` implements #377's reviewed lifetime boundary. Typed rows move after
final claim checks; replay storage retires before publication. Exact mass uses
the checked native ledger, composed pins remain, and standalone plans keep their
lifetime. Actual retained requests fall by 2445/3129 bytes with setup peak +64.
Quick CI, 17 Clang and seven GCC suites pass; final test-only view scoping
`14ff61f9` passes focused Clang/GCC and historical comparisons. Main `4bb937d8`
now integrates the maintainer-reviewed #377/#378. Full accounting, GPU and #324
remain separate.

2026-11-18 integrated in #377 (historical inspection record):
[Prepared plan lifetime inspection](../engine/docs/PREPARED-PLAN-LIFETIME.md) at
`b07adf29` traces retained bootstrap descriptors and the published body's exact-mass
dependency against existing native source/hierarchy ownership. Actual six-buffer
requests total 1812/2496 bytes in aggregate/parts fixtures, retained through tick 1
and fully released at owner teardown. No runtime saving is implemented. Quick CI,
six Clang and three GCC suites pass. The authorized private consumption in #378
is now integrated, preserving typed source/claim and standalone plan lifetimes.
Other numerical/accounting scopes stay separate.

2026-11-18 hosted correction for #374: `65b502e9` keeps container bash/PostgreSQL
utilities on their own runtime and uses the staged loader/library path only for
the native command; missing-library checks fail closed without rg. The new local/
hosted transport regression passes and both restored defects fail its controls.
[Inspected corrected database evidence](../engine/docs/RETAINED-PREPARED-QUALIFICATION.md#hosted-transport-correction-and-inspected-evidence--2026-11-18)
from job `134949499123`/artifact `14447427435` confirms actual retained membership
refusal, warm before/after counts, zero publications/leases and preserved cold
data. Native build/database pass at the exact corrected tree; full matrix/review
gates remain separate. Runtime/probe semantics and unknown memberships are
unchanged. This qualifies the refusal route, with a retained tick still open.
All merges remain with the maintainer.

2026-11-18 contributor (awaiting review):
[Actual retained-source qualification](../engine/docs/RETAINED-PREPARED-QUALIFICATION.md)
at `8fa4dbb1` invokes the actual opt-in executable against verified isolated
retained restores. English A carries English/Alphabet (en) memberships with NULL
whole mass; the explicit preset refuses those comparison choices before warm
publication/acquisition or native admission. Both memberships and all cold records
are preserved. Local source-backed refusal, populated-store guard/policy
preservation, cold fingerprints and shell/orchestration checks pass. Existing
database CI now transports the real executable/probe into a fresh v2 warm fixture
and records the precise refusal; its success does not qualify a completed tick.
Docker delivery remains hosted evidence. Corrected #373 `0e2add68` requires fresh,
dedicated qualification stores for both profiles. All merges remain with the
maintainer. Next is the explicit membership comparison/focus/resource decision
and a positive retained witness; no inferred weighting or #324 implementation.

2026-11-18 contributor (awaiting review):
[Opt-in retained prepared capture](../engine/docs/RETAINED-PREPARED-CAPTURE.md)
at `3999550e` implements #372 review 5958354207's bounded producer entry.
The trace distinguishes pinned exact definitions from explicit selection,
geometry, mass, comparison, precision, focus and resource choices. Default v1
is retained; the named CPU profile uses shared v2 publication/acquisition,
`submit_prepared`, native ticks and the existing observer. Quick CI, eight Clang
and four GCC suites pass, including six synthetic-source profile modes with
actual claims/completed native ticks. Genuine retained acquisition is blocked
by missing connection settings/service and a stopped server; no retained store
changed. #371's native/smoke passed and #372's native remains pending at this
handoff. All merges remain with the maintainer. Review/hosted qualification and
matching source acquisition precede any retained end-to-end claim.

2026-11-18 contributor (awaiting review):
[Explicit prepared active submission](../engine/docs/ACTIVE-PREPARED-SUBMISSION.md)
at `ae9b6485` implements #371 review 5956829592 through the existing session.
The runtime provisions layout/Harness internally, prepares and bootstraps with
the existing logical owner, and transfers a preallocated map node only after
publication. Owner/Harness/Runtime lifetimes and source pins are explicit; core
and viewport observations share committed-claim metadata. Legacy v1 is retained.
Quick CI, 13 scoped Clang suites and four GCC suites pass, including six entry
modes/12 submissions, resource/work refusal/retry, healthy prior executions,
actual identities and following ticks. No field math/native-fork change or full
accounting/retained-DB qualification is claimed. #370 is qualified and merged as
`15299964`; #371 is retargeted to main, native CI pending. Normal merge `718e79a0`
has the exact tested tree. Review this operation and its authoritative hosted
gate before integration. Retained producer/client qualification, general prepared
revision, refresh transport and #324 remain separate.

2026-11-18 contributor (awaiting review):
[Retained-study/C3 handoff inspection](../engine/docs/RETAINED-C3-HANDOFF.md)
at `ff27ecac` demonstrates the existing prepared-owner bootstrap/claim/report path
and the missing retained session entry. Six Clang/GCC modes reproduce the saved
study's construction paths/notations/masses with fixtures, prove v1 remains flat
and refuses v2 without losing its execution, and run 12 bounded aggregate/parts
prepared examples through actual native commit/tick/observation. Linker counters
show one combined commit and zero standalone installs per prepared bootstrap.
Quick CI and all four new Clang CTests pass. Byte pins remain fixture-only; no
current DB or full retained-C3 execution is claimed. Production equals provisional
#370 `eee4ef07`, preserved with main `2c7ec4d0` in normal merge `543cfbf4`.
Review the prepared request/context lifetime boundary before a runtime operation;
dependency/hosted gates remain distinct. Refresh transport and #324 stay separate.

2026-11-18 contributor (awaiting review):
[Native initial C3 seed transport](../engine/docs/NATIVE-HIERARCHY-SEED-TRANSPORT.md)
at `98da1f2f` implements #369 review 5955551516 on integrated main `2c7ec4d0`.
Validated private descriptors and native copies replace the numerical host round
trip; actual native destination initialization establishes inactive zeros. At
H=70 actual host traffic reduces by 8,680 bytes, with 4,480 native zero writes
and 192 native copy-request bytes separately accounted for three nodes. Public
inspection, guards, work/refusal/retry and publication/failure rules remain.
Quick CI, 16 scoped Clang CTests, dirty refresh, two GCC seed probes, four GCC
controls and four GCC public suites pass. Historical binaries establish actual
before totals; both-bank/Q31/dirty-source-and-destination/16-tick history and
three exception-boundary controls pass. Review/authoritative hosted CI remain
pending. Refresh, reservations, validator/capture reuse and #324 stay separate;
follow-on inspection requires a concrete owner/lifetime/native-cost trace.

2026-11-17 contributor (awaiting review):
[Hierarchy seed transport inspection](../engine/docs/HIERARCHY-SEED-TRANSPORT.md)
at `c2c553c1`, based on integrated main `22273dd2`, traces C3's full centroid
inspection readback and live seed upload. Actual H=70 readback is 4,448 bytes
versus 248 reported; the extra 4,200 inspection bytes and 4,480 seed-upload bytes
identify a bounded transport opportunity. Existing Taichi intra-device slices
copy the actual published XYZ/mass into isolated two-bank scratch bit for bit,
with Q31 sensitivity and poisoned inactive-capacity controls. Quick CI, six
Clang/GCC probe configurations (18 fixtures) and eight existing basic/publication
case runs pass. Production is unchanged; failed development controls are retained.
Review the proposal before a runtime replacement. Preserve validation, currentness,
public inspection, inactive slots and publication/failure rules. Full regression,
GPU execution, refresh transport, validator/capture reuse and #324 remain separate;
hosted CI remains authoritative and pending.

2026-11-17 contributor (awaiting review):
[Prepaid-map adoption capture](../engine/docs/ADOPTION-SOURCE-CAPTURE.md) at
`da083031` implements #367 review 5948664040. Temporary validators read the
initial owner snapshots synchronously with the shared setup meter, keeping
independent capture buffers, all source checks and independent reservation bounds.
No stored loan, new wrapper or standalone ownership change. Actual whole-owner
construction removes 578/2,444/1,779 requests and 48,369/205,000/149,169 cumulative
requested bytes; setup work reduces by 84,895/377,396/267,927 units. Retained
bytes/whole peaks remain unchanged. Quick CI, 48 owner modes, 12 standalone
cases, both compilers' old/new request/ABI probes and exact/short/bound/unwind
controls pass. Main `8c3d0b93` and provisional #367/#366 are explicit through
normal merge `7029c078`; review and authoritative hosted CI remain separate.
Next inspect another concrete duplicated association through its ownership and
measurements. Capture-buffer reuse/shrinking, indexed guards, broader producers,
full accounting and #324 stay separate.

2026-11-17 contributor (awaiting review):
[Temporary adoption validation](../engine/docs/ADOPTION-BINDING-COST.md) at
`8972cd49` measures each per-selection stack binding in actual adoption.
Exact-constructor request/release traces match all eight selected roles in the
three fixtures. The temporary's owned maps, constructor-local trusted/staged maps
and independent source-validation facts have separate costs/lifetimes; retained
candidate maps are independently attributed and unchanged. Temporary cumulative
requests total 91,649 / 378,120 / 279,009 bytes; these are candidate setup costs,
not implemented savings. Work/full native-fact/mask/domain/weak controls pass
alongside quick CI, four dedicated Clang/GCC probes and six full changed owner
cases. Production equals provisional #366; current main `8c3d0b93` and that
dependency are preserved in normal merge `266bdd8b`. The proposal for private
synchronous reads of initial owner maps needs review before runtime changes.
Standalone ownership, capture checks/buffers, one initial prepayment and the
candidate transaction remain. Indexed guards, broader producers, full accounting
and #324 stay separate; hosted CI remains authoritative.

2026-11-17 contributor (awaiting review):
[Direct owner-map publication](../engine/docs/EXCHANGE-OWNER-SOURCE-MAPS.md) at
`4f74ad11` implements #365 review 5946869103's bounded removal. Current maps
are synchronous const call arguments; independent candidates publish directly
into the owner after source access ends. Captured facts, generation/readiness,
all native/semantic validation and standalone owned adapters remain. No stored
loan or new registry/wrapper. Actual whole-owner before/after runs remove 562
allocations and 47,180 retained/scoped-peak requested bytes per fixture, with
unchanged 608/680/376-byte owner/coordinator/binding extents. Census, removed
reservation/copy work and lifetime witnesses are coordinated. Initial input
prepayment and temporary adoption validators remain. Quick CI, 16 Clang/eight
GCC CTests, all 48 owner and 48 standalone exchange mode runs and both compilers'
request/ABI/source-death/unwind controls pass. Main `068fe63a` and accepted
#365/#364 remain explicit dependencies through normal merge `bd4d1658`.
Review/authoritative hosted gates remain separate. Next, inspect the temporary
adoption validation maps as a separate measured surface after this review;
indexed guards, broader producers, full accounting and #324 stay separate.

2026-11-17 contributor (awaiting review):
[Binding-map request and publication inspection](../engine/docs/EXCHANGE-BINDING-MAP-COST.md)
at `120658ca` measures the third source-map pair's 47,180 requested bytes across
562 allocations, with embedded headers and whole-owner overlap separated.
Actual capture copies owner associations; native publication rotates candidate
through binding into owner. New detached full-value snapshots qualify 160
rotations, native claims, stale old authority and following ticks. The bounded
proposal passes current owner maps through synchronous private source reads and
publishes the existing candidate directly, preserving candidate/captured state,
standalone ownership and all validation. It is not implemented. Quick CI, four
three-fixture probes and six full Clang/GCC owner cases pass; source matches
reviewed #364. Main `068fe63a` and its unmerged dependency are explicit. Review
and hosted gates remain separate; no new registry or native guard is introduced.

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
