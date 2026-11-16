# Prepared admission: native reuse and retained ownership seam

**Contributor owner integration — 2026-11-16 (awaiting review):**
[bounded private owner evidence](PREPARED-STUDY-OWNER.md) at public `a97b6049`
reserves metadata before native bootstrap, publishes actual claims, follows
contact-changing tick generations and reuses completed observation. No public
ActiveSession migration, additional instance framework or new mathematics is
implemented. The native nested-instance/control/accounting inspection below
remains required before further wrappers.

**Inspection direction — 2026-11-16:** the [nested-assembly and live-instance inspection brief](../../docs/engine-refactoring-plan.md#inspection-clarification-nested-assemblies-and-live-instances) sharpens #343's native-reuse pass. Inspect reusable structural assemblies, distinct nested runtime instances and contextual attachments, including lifecycle/resource accounting already supplied by the Taichi fork. One active tree can be the assembled result; do not assume a parallel flat occurrence/lifecycle framework is necessary. Verify actual native mechanisms and HCP-specific gaps with source evidence; this is a requirement to inspect, not a claim of an existing Taichi object/attachment API. Relate the mapping to #324's structural-intent separation without changing formulas. Coordinate with owner integration; earlier source acceptance stands. CI statements below retain their recorded observation time.

Contributor inspection/proposal under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-16. Inspected main: `ee31e88a16d9e641311da523f3b9a7c455601e75`;
Taichi tree: `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, with matching
initialized recursive submodules. This extends the historical
[responsibility map](RESPONSIBILITY-MAP.md), after integration of #342/#346.
It implements no admission adapter or retained C3 execution. Symbol references
describe the inspected source; the next slice below is proposed for review.

**Subsequent contributor implementation — 2026-11-16 (awaiting review):** the
[private structural plan builder](STRUCTURAL-ADMISSION-PLAN.md) implements the
accepted next metadata-preparation slice at public `7bb1e670`, explicitly
dependent on #347. Exact source/binding, refusal/unwind and lifetime controls
pass locally under Clang 15 and GCC 11 after a retained GCC cleanup failure was
corrected. Native commit, actual claim/map publication and retained C3 execution
remain subsequent work. The original inspection/proposal below retains its
source pin and historical status.

## Finding and governing boundaries

The engine already has a private, combined D1 bootstrap transaction. The missing
retained bridge must translate pinned structural identity into that transaction
and publish the actual committed associations. It must not implement another
placement solver, record allocator, hierarchy repair or tick authority.

[D1–D7 and owner publication](../../docs/core-operation-readiness-plan.md),
[native admission](NATIVE-ADMISSION.md), [live refresh](LIVE-REFRESH.md), and
[owner resources](OWNER-RESOURCES.md) govern this seam. They require exact source
counts, complete intrinsic geometry, generated placement, explicit prospective
comparison/focus identity, healthy preclaim rejection/retry, and native commit
before occurrence-map publication. Private inputs remain at the active owner;
the shared manager must reject private scopes before lookup. The current
structural handoff rejects local overlays rather than silently admitting them.

`ActiveSession` still constructs `FixedStudy`. `ActiveAssembly::bind_structure`
does not change that. A second retained execution route requires deliberate
ownership and access changes; merely renaming the fixed route would not supply
the missing behavior.

## Exact data handoff and remaining decisions

| Boundary and exact symbols | Available source and required treatment |
| --- | --- |
| Logical input: [active_workbench.cpp](../src/assembly/active_workbench.cpp), `ActiveAssembly::bind_structure`; [letter_projection.cpp](../src/assembly/letter_projection.cpp), `LetterProjection::bind_structure` | V2 definitions, all alternatives, selected exposure, contacts, explicit ordered focal fields and caller-owned `StudyNode` bindings. Preserve explicit empty focus override versus inherited recipe focus. Binding checks recipe parent/role and nonnull layout; it does not perform native admission. |
| Identity: [letter_projection.h](../src/assembly/letter_projection.h), `OccurrenceKey`, `FieldKey`, `ProjectedOccurrence`, `ParentContribution` | Study UUID plus numeric source entry/path identifies an occurrence; definition `ToolRef` pins revision. `project_letters` uses `OccurrenceIdLess`/`OccurrenceId::operator<`, not lexical serialized-text order. V2 `slot` is -1. Alternatives are distinct identities; only active exposure becomes native bodies/participation. Keep typed keys alongside the string-valued handoff. |
| Exact source: [study_input.h](../src/assembly/study_input.h), `Definition`, `ParentOccurrence`, `StructuralStudyInput`; [membership_fields.cpp](../src/assembly/membership_fields.cpp), `MembershipFields::{collect,bind_structure}` | Definition mass and ordered parent counts are uint64; intrinsic centres/radii are double. Membership collection preserves per-occurrence identity and deduplicates attachments within that occurrence. An all-alternative membership catalogue is not a live source set and `bind_structure` rejects it. No position or velocity exists in `StructuralOccurrence`. |
| Native layout, geometry and addresses: [study.cpp](../src/assembly/study.cpp), `scale_bindings`, `enclosing_radius_impl`; [exchange_coordinator.h](../src/assembly/exchange_coordinator.h), `ExchangeIncoming` | Existing native preparation validates real SNode parentage and explicit roles: PrimaryStep changes units by 64; SecondaryCompression/NamingEquivalent preserve units. This is not raw-depth semantics. Existing enclosure validation consumes complete components about their centroid. Reuse/factor that validation deliberately; `scale_bindings` is private legacy preparation and includes its f32 range check. Never reconstruct original geometry, counts or units from `PreparedStudy`'s rounded staging. The preparing owner must bind native kind/address, operative units/frame and complete geometry identity/revision; cold ObjectKey address, native declaration address and claimed record slot are different identities. |
| Comparison/focus: [live_hierarchy.h](../src/field/live_hierarchy.h), `LiveRefreshNode`, `LiveRefreshContribution`, `LiveRefreshSpec`; [cascade.h](../src/field/cascade.h), `CascadeNode` | **Missing from the structural handoff:** comparison identities, explicit ordered comparison topology/cohorts, precision and their association with native fields. Scale nodes and membership edges do not supply this tree. The native validator requires one owner per present field, children before parents, one root, compatible exact coverage and leaf participation for every body. Arbitrary overlapping memberships cannot be silently turned into this supported tree. An explicit preparing-owner descriptor is required; unsupported/missing bindings must reject. Focus must resolve against that same prospective field/source inventory, preserving requested order. |

`LiveHierarchy::prepare_refresh` in
[live_hierarchy_refresh.inc](../src/field/live_hierarchy_refresh.inc) checks
`spec.node_count == candidate.fields`, unique field/comparison identities,
supported f64 precision, child/coverage consistency, the unique last root and
complete field ownership. [CascadeSourceView::contribution](../src/field/cascade_source.h)
checks full handles, source generation/row/ordinal and the exact numerator and
denominator; rounded f32 shares are derived checks, never original counts.
Physical numeric mass and the exact structural denominator have distinct native
roles; a conversion policy must state its supported range rather than assert
that every uint64 is exactly representable as double.

The descriptor is a missing owner input, not a proposed automatic Analyst
selection rule. Do not derive comparison identity from SNode depth, add a
synthetic root to make arbitrary memberships pass, drop fields, or interpret
composition as a new force formula. #324's structural-intent derivation remains
separate. Supplied structural contact declarations also need explicit supported
validation; they cannot replace native prospective occupancy/contact discovery.

## Native mechanisms to retain

| Responsibility | Existing implementation and reuse boundary |
| --- | --- |
| Declare limits and provision backing | [PreparedCapacity](../src/field/prepared_capacity.h) checks capacity/arithmetic; it does not allocate execution backing. `Harness(Runtime&, PreparedCapacity, HostPath)` and `Harness::prepare_empty_execution` in [field.cpp](../src/field/field.cpp) provision/warm fixed arrays, maps, native activity, update/geometry, hierarchy, tick and placement machinery as enabled by the declared budgets. A ready empty execution has no present source. `BootstrapCommands` separately bounds whole-study D1; `command_capacity()` shares max(Batch, BootstrapCommands) backing without widening ordinary Batch. |
| Claim/recycle records | [PreparedRecordMap::{define_batch,release_batch}](../src/engine/record_map.cpp) validates declarations and full context/address/incarnation/record handles, then uses prepared native kernels. The constructor preprovisions the pool with SNode activation/deactivation. Fork [internal_functions.h](../taichi/taichi/runtime/llvm/runtime_module/internal_functions.h) supplies `prepared_node_acquire_index`, `prepared_node_recycle_index`, `prepared_node_collect` over the existing NodeManager free lists/GC. Acquisition/recycling is exclusive and quiescent; a separate host free list or allocator is unnecessary. Homogeneous map batches alone are not the whole physics transaction. |
| Reuse compiled dispatch | [CompiledKernel::prepare](../src/engine/runtime.cpp) delegates to fork [LLVM::KernelLauncher::prepare_kernel](../taichi/taichi/runtime/llvm/kernel_launcher.cpp). Originating Kernel/Program, runtime and ABI checks precede prepared binding acquisition. Existing frozen arrays/scalars and prepared launches retain the native authority; no second executor/thread scheduler is required. |
| Generate starts and certify geometry | `Placement::stage` in [placement.cpp](../src/field/placement.cpp), using `PlacementIntrinsic` from [placement.h](../src/field/placement.h). Each intrinsic names a DefineParticle command, canonical order, numeric mass, radius and complete geometry identity/revision. The native solver writes candidate rows directly. D1 explicitly selects the canonical first exposed participant of the first requested focal field at local zero, then uses already placed focal contributions. Native zero-motion initialization and certified nearest-free placement remain authoritative. No caller starts or host readback/replay. |
| Preflight/commit one generation | Private `Harness::stage_current_admission` in [prepared_update.cpp](../src/field/prepared_update.cpp) stages placement and calls `update_prepared_impl` with BootstrapPreflight/BootstrapCommit or GeneratedPreflight/GeneratedCommit. Bootstrap requires empty/unpopulated source, no installed hierarchy and explicit nonempty focus. It validates the prospective source/hierarchy/focus and contact union/work peaks before claims. Commit requires prepared tick execution. Bootstrap publishes source, hierarchy, focus and contacts together; there is no need for a separate fallible postcommit hierarchy installation. Preflight issues no durable admission token. |
| Resolve actual claims | [PreparedStorage](../src/field/prepared_storage.h) reserves `resolved`, claim arrays, prospective headers and destinations. Commit verifies native context/incarnation/address/kind and bijective destinations, then replaces provisional command references with actual claims. `UpdateRef::pending(command)` and refresh `field_command`/`source_command` reference declarations without guessing native handles. `UpdateResult` reports status/generation/work, not a claim table. The owner must consume `PreparedStorage::resolved[command_index]` before later operations reuse this scratch. |
| Publish owner maps after commit | [ExchangeCoordinator::{prepare,execute}](../src/assembly/exchange_coordinator.cpp) already reserves commands/intrinsics/hierarchy tables, builds pending references, calls private admission, fills preallocated next maps from `resolved`, then swaps them. [ExchangeOwner::execute](../src/assembly/exchange_owner.cpp) publishes maps/projection/exposure after that success under the same exclusive boundary. Exceptions after native mutation mark Harness unready; they are not healthy unchanged rejections. Reuse this publication pattern, not the exchange operation itself as initial admission. |
| Bound host preparation | [PreparationWork](../src/assembly/preparation_work.h) prepays scans/comparisons/key copies; [BootstrapReservations](../../common/bootstrap_reservations.h), [ReservationDomain and reserved containers](../../common/reservation_allocator.h) gate actual selected allocation requests/lifetimes. The engine namespace aliases these helpers; no new generic allocator/accounting framework is needed. Preserve distinct caller-input, setup-work, outer-bootstrap and selected-buffer gates. |

## Ownership, failure and accounting constraints

The existing [active_session.cpp](../src/assembly/active_session.cpp)
`ActiveSession::Execution` owns its assembly snapshot, nodes and FixedStudy;
the session owns Runtime. Destruction synchronizes, clears executions and only
then releases Runtime/Program. A later prepared execution must preserve this
order: Runtime outlives Harness, and Harness outlives every borrowed exchange
owner/binding, prepared context and native view. The new owner must also retain
the pinned source, typed identity tables and reservation domains for their
actual lifetimes. A borrowed `LiveRefreshSpec` must not outlive its backing plan
or survive mutation/reallocation of those tables. Completed observation must use
the published generation, completed tick and full handles, not scratch aliases.

`Harness` currently grants private access to `ExchangeOwner` and
`ExchangeCoordinator`, not `ActiveSession`. Production integration needs a
narrow reviewed owner access path; copying a testing friend into production
would bypass the seam. `LetterExchangeContract` describes an existing declared
aggregate↔parts change; it is not a general initial-admission declaration.

`ExchangeOwner::adopt` is also **not an empty bootstrap factory**. It requires a
ready structural execution, complete trusted committed maps, and subsequently
copies/allocates projection, memberships, declarations, bindings and staging
tables. Native bootstrap followed by fallible adoption cannot be advertised as
one healthy atomic admission. Before enabling retained commit, stage/reserve
all owner metadata and publication space before claims, leaving only bounded
nonallocating resolution/swaps afterward. Any genuinely unexpected postclaim
failure must leave the execution unready and unpublished. Prove that boundary
with allocation/failure injection; do not assume adoption is noexcept.

Likewise, legacy `ActiveSession::refresh` replaces the logical generation before
publication and invalidates the execution on failure. That is incompatible with
the operational guarantee that an ordinary preclaim rejection retains the
healthy previous assembly, handle, generation, numerical state and retry path.
Keep the legacy route distinct until an explicit prepared operation owns that
guarantee. Restart hints, focus/rebase, ordinary additions and D7 exchange retain
their separate contracts; D1 support alone does not implement all of them.

Resource evidence is partial. `PreparedCapacity` limits, reserved backing
censuses and allocator request ledgers are different surfaces. The
[borrowed-resource inventory](BORROWED-RESOURCES.md) still lacks the complete
trusted B ledger; selected owner gates do not establish combined C+B+H+D
admission. Do not add overlapping totals as disjoint bytes, equate allocator
requests with RSS, or claim a complete accounting envelope for a new wrapper.
This inspection identifies reuse sites; it measures no new allocation or
performance result and is not a general native/compiler audit.

## Smallest proposed implementation and its exit evidence

**Next PR: a private bounded structural admission plan builder, without enabling
retained execution.** Place the adapter beside existing assembly preparation;
keep its native types/tests in the native target. Leave standalone projection
targets free of Taichi/PostgreSQL. Do not introduce a new public Analyst/session
API in this slice.

Inputs would borrow the owner-pinned V2 projection/live structural handoff and
explicit preparing-owner bindings for layout/units/frame, native kind/address,
geometry identity/revision and the supported comparison descriptor above. The
implementation must define and validate these concrete associations before
emission. Missing or incompatible topology is a refusal, not permission to
invent policy. This is an internal proposal, not an implemented input type.

Outputs would own bounded existing `UpdateCommand`, `PlacementIntrinsic`,
ordered focus `UpdateRef`, `LiveRefreshNode`/children/contribution tables, plus
typed occurrence/field-to-command indexes for later claim resolution. Use the
existing reservation/work helpers, declared BootstrapCommands and checked
index/count arithmetic. Emit only selected live occurrences in numeric
entry/path order, exact uint64 source provenance and complete original double
geometry through validated operative conversion. Derive numerical shares from
exact counts. Preserve distinct repeated occurrences and field kinds. A failed
build exposes no usable partial plan. No native claims, positions, committed
handles, map publication or healthy execution replacement occur in this slice.

Required focused evidence for that PR:

- Aggregate and parts exposures, repeated definitions at distinct entry/paths,
  exact parent ordinals/counts and membership deduplication. Include numeric
  path order that differs from lexical text order and geometry distinguishable
  before f32 rounding.
- Pending particle/field/participation references and explicit comparison/focus
  associations match the same emitted prospective source. Test missing,
  duplicate, cross-study/revision, inactive and incompatible bindings, plus
  explicit empty versus inherited focus. No guessed slot/handle appears.
- Exact capacity/work limits and one-short rejection, checked overflow,
  allocation refusal/unwind, source lifetime and retry without a partial plan.
  Whole bootstrap may exceed ordinary Batch only within BootstrapCommands.
- Existing standalone projection tests remain independently buildable. Run
  native adapter tests and applicable local CI, source-pin the evidence, and
  submit for authoritative GitHub CI. A command-plan pass does not qualify
  native admission or retained ticking.

After review, a separate owner-integration PR can prepare an empty Harness,
reserve owner metadata, invoke the **existing private bootstrap** under one
exclusive boundary, resolve actual claims and publish the completed owner/map.
That slice needs production access rather than fixture admission, preclaim
unchanged-state/retry witnesses, postclaim unready controls, no allocation during
publication, correct destruction order, and native tick/completed-observer
coverage. Reuse the existing `admission_bootstrap` cases in
[placement_cases.inc](../tests/placement_cases.inc) as acceptance
controls, not `testing::PlacementAccess` as a production entry point. Retained
capture may claim C3 availability only after this bridge is implemented and
qualified. Broader accounting, other operations and #324 remain separate work.

## Evidence scope for this proposal

Personally inspected the symbols and native fork paths linked above at the
recorded source. No baseline regression was rerun merely for onboarding, and no
new admission/ownership behavior was executed. Contribution-specific checks,
tested public revision, local evidence location and hosted status belong in
the proposal PR. Local checks support only their recorded scope; maintainer
review may permit provisional continuation, while GitHub CI remains
authoritative for integration. Historical failures and qualification remain in
their existing component records.
