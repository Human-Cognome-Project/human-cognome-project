# Engine responsibility map and first bounded extraction

**2026-11-16 extension:** #342/#346 are integrated on inspected main
`ee31e88a16d9e641311da523f3b9a7c455601e75`; the mechanical session split is
complete. The [prepared-admission seam inspection](PREPARED-ADMISSION-SEAM.md)
maps existing native bootstrap/reservation/claim machinery, retained ownership
gaps and the next proposed bounded plan-builder slice. It adds no runtime
behavior and awaits review. The original source-pinned map/diagnosis below is
historical; its then-pending statuses do not override current handoff notes.

Pass 1 proposal under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343)
and the [refactoring plan](../../docs/engine-refactoring-plan.md). Inspected source:
`22851781cd0477a445e571d1c446f3f094f0047b` (2026-11-15), Taichi tree
`8b31651be07d8bad0b2a6a261369e8690b9c3b53`, with matching initialized recursive
submodules. Paths and symbols below describe that source, not an accepted new
layout. No runtime move, numerical change or final Analyst API is implemented here.

## Baseline and the failure that constrains extraction

Main's runtime merge `64fd23d02eb33fa00129ce78bd20fc5ad225f8e5` passed native run
43713977014 and smoke 43713977016. Changes between that merge and the inspected
main are documentation. PR [#342](https://github.com/Human-Cognome-Project/human-cognome-project/pull/342)
remains separate at `0d0adec5b1beda5db55aa8d54358b6f7d2c0bb1b`; its structural
handoff is source-accepted for bounded continuation, not integrated. It does not
complete retained C3 execution.

The original standalone projection dependency failure is repaired. The distinct
failure in [run 43715143149, job 132815906747](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/43715143149/job/132815906747)
is still authoritative evidence: `live_intent_activity_2_bulk` failed with
`equal-force publication control changed force`; the aggregate failed. The run
passed the other 168 regression tests, standalone projection, native build,
database and four compiler profiles. No rerun or merge clears that result here.

### Focused diagnosis, not a repaired production path

`live_response_cases.inc::intent_activity` expects both leaf packets and their
parent to reach their final values after one ordinary tick's centroid poll.
However, [LIVE-HIERARCHY](LIVE-HIERARCHY.md#publication-history-and-recipient-checks)
explicitly permits complete sibling publications from different generations and
requires later child notifications to leave parent work pending. Native sparse
traversal is not a topological solve. The test's exact force equality is a valid
final-state control, but its one-poll premise needs separate qualification.

Personally executed on the inspected main, Clang/LLVM 15 Release, actual x64,
two workers, BulkCopy:

- Compared `live_response_cases.inc`, `live_hierarchy.cpp` and `intent_ir.h`
  between main and #342's recorded head: no source differences.
- Rebuilt `live_hierarchy_test` against this checkout's CPU-only substrate.
- `ctest --test-dir engine/build -R '^live_intent_activity_2_bulk$'
  --repeat until-fail:20 --output-on-failure`: 20 passes, 172.09 seconds.
  This does not establish that the hosted failure was a flake.
- A separate C++ probe includes the pinned test source and reuses its `Fixture`,
  `Controls`, native production kernels and source-derived diagnostic reference.
  After the same source edit, existing test-only `LiveHierarchyAccess::poll`
  selects nodes in order `0,2,1` (left leaf, root, right leaf), then `2` (root).
  It changes no production source, formula or equality assertion.

| Completed probe cut | Root X / mass | Recipient diagnostic X force | Retained body force | Body wake |
| --- | --- | --- | --- | --- |
| Initial | -0.5 / 2 | 0.375 | Initial evaluation | Consumed |
| Leaf 0 → root → leaf 1 | 1.2 / 5 | -3.4387943786982245 | Unchanged | Pending |
| Following root publication | -1 / 8 | 0.375 exactly | Unchanged | Pending |

The middle root is the complete reduction `(4*2 + 1*(-2))/5`, not a torn packet.
The later root combines both changed leaves. The probe asserts the unequal
intermediate force and restored exact final equality; it also checks the final
candidate `324/499`, retained force and pending recipient wake. It isolates a
permitted publication-order counterexample to the test premise. The hosted log
does not print the failing operands/packets, so this is not proof of that run's
exact schedule or proof against every possible engine defect.

Evidence is retained locally in `/tmp/hcp-publication-diagnosis.ZLM7xF`:
original hosted job log, baseline CTest log/source pin, `ordered_probe.cpp`, its
exact build commands and output, configuration and binary hashes. Archive:
`/tmp/hcp-publication-diagnosis-22851781.tar.gz`, SHA256
`17fff794d4786a26c641559b4dd541254b05924d9d80a3f29d1515289f0479d8`.
The probe exits zero; post-PASS Taichi offline
cache-lock warnings are retained. This is personally executed local evidence,
not independently inspected hosted probe evidence. No GPU, database, browser,
full regression or performance qualification was performed for this diagnosis.

**Next corrective slice:** preserve the exact equal-force and unchanged-native-
accumulator assertions, but establish a completed held-input field boundary
before applying the final-state expectation. Add deterministic parent-before/
between-child publication controls, retain native pending body/parent work, and
separate intermediate observations from final equality. Do not clear the wake,
loosen the comparison, serialize production traversal or install a per-depth
scheduler. Submit this test correction separately before behavioral extraction
relies on the failing control. A new hosted result must qualify the correction;
the old failure remains recorded.

## Responsibilities and dependency boundaries

The units below are inspection boundaries, not eight new libraries. The
`engine_support → field_core → study_assembly → active_workbench` dependency
direction and separate pure projection targets are visible in
[engine/CMakeLists.txt](../CMakeLists.txt). Caller choices flow toward execution;
completed observations flow back out. Supporting kernels retain their own owners.

| Work unit and purpose | Current owner, callers and dependencies | Mutable state and lifetime | Privacy and invariants | Evidence and extraction treatment |
| --- | --- | --- | --- | --- |
| Composition / occurrence identity: turn pinned definitions into distinct exposed occurrences | `LetterProjection`, `project_letters`, `normalize_structural_letters`, `MembershipFields::bind_structure`; called by `ActiveAssembly` and private exchange preparation. Dependencies: warm value types, `study_input.h`, reservation helpers; standalone CMake targets have no Taichi/libpq requirement. | Projection study UUID, recipe, occurrence/provenance/exposure tables and attached allocation domains. Copies/detached reports and moves have different accounting lifetimes. | Shared recipe definitions remain separate from private overlays. Explicit `ScaleRole`, exact parent counts/ordinals and per-occurrence identity govern representation; native slot/depth is not conceptual scale. `member_of` attachments are distinct from ordered composition parents. | Four standalone letter-projection CTests and native projection/membership tests. Retain the pure boundary. Do not move `ActiveSession` headers into these suites; #342 shows the failure mode. |
| Logical active inputs: validate caller-owned shared/local assembly values | `ActiveAssembly::{assemble,revise,restart,focus,bind,origins}` in `active_workbench.*`; called by retained capture, warm-active integration and native tests. Depends on projection/membership values and the existing `enclosing_radius` validation in `study.cpp`. | Owns logical projection, owner/instance/generation IDs, optional `LocalInput` and focus selection; no Program or native arrays. Revisions/restarts create distinct logical generations. | Private inputs enter here with scope/owner/version checks; shared manager never acquires them. Supplied legacy placement is not the v2 generated-admission contract. Main lacks #342's `bind_structure`/explicit focus-override behavior. | `active_workbench_test`; #342 adds native structural checks on its own branch. Separate logical and session implementation first; a fully runtime-free logical header/target needs a later geometry dependency decision. |
| Retained execution ownership: own native session, bindings and reports | `ActiveSession::Execution`, `submit/refresh/tick/report/accept/get` in `active_workbench.cpp`; uses `Runtime`, SNode layout, `FixedStudy`. Called by `letter_capture.cpp`, integration/tests. | Session owns Runtime and executions; each execution owns assembly snapshot, StudyNodes, FixedStudy and completed-tick count. Destructor synchronizes, clears studies/kernels, then destroys Runtime. Program owns native tree lifetime. | Handles bind session/study/generation; owner checks protect reports. A failed legacy refresh replaces the logical generation and disables ticking; it is not the prepared transaction's reject-with-old-owner guarantee. Report copies strip native layout pointers. | `active_workbench_test`, warm-active integration and viewport snapshot tests. **First proposed move:** put only the `ActiveSession` implementation into `active_session.cpp`, retaining the public header/target and behavior. |
| Fixed legacy preparation and publication: convert one bounded layout to field arrays | `prepare(StudyInput)`, `FixedStudy::{stage,publish}` in `study.cpp`; called by ActiveSession and legacy fixtures. Depends on field_core, exact definition/occurrence values and designated StudyNodes. | Fixed particle/group/edge/contact counts, prepared metadata and owned Harness. Publication restages arrays/centroids; refresh requires unchanged shape. | Validation precedes numerical publication; failure invalidates readiness. Supplied state and f32 conversion belong to this legacy path. They must not be reused as original f64 structural input or operational Add. | Assembly/geometry/source-count and native projection tests. Keep preparation behavior out of a mechanical session move; no geometry/numerical refactor in that PR. |
| Prepared operations / coherent logical publication: validate native edits and commit mappings | Harness's private `stage_current_admission` in `prepared_update.cpp`, Placement bootstrap, `detail::ExchangeOwner`, `ExchangeBindings`, `ExchangeCoordinator`. Tests/private friends drive these primitives; ActiveSession does not yet own this bridge. | Harness owns prepared backing, generations, handles and TickExecution; ExchangeOwner borrows Harness and owns current/prospective projections, occurrence/field maps and bounded staging/coordinator data. Harness must outlive it; calls are exclusive. | Generated nearest-free placement, source-bound exact counts, prospective focus/comparison identity and commit-before-map publication remain required. Reject/retry and survivor preservation differ from legacy refresh. No public enduring Add API or complete combined accounting is inferred. | Admission/exchange/owner-lifetime cases in `live_hierarchy_test`, capacity/backing/allocation tests. Coordinate the future bridge with the session owner; do not add another producer or use `PreparedFixtureAccess` as production admission. |
| Numerical execution: field/ancestor/contact contributions, one integration, publication/activity | `Harness::tick_values`, `LiveHierarchy::{force,poll}`, `Impl::build_kernels`, `contribution`, `numerical_kernels.cpp`, `Activity`; transactional prepared path uses `TickExecution::{tick,run_physics,run_hierarchy}`. Dependencies: native runtime/IR, accepted numeric and publication contracts. | Authoritative numerical arrays, two-slot hierarchy packets/selectors, old-input history and native activity sets. Scratch/mirrors/inspection copies have distinct roles. Kernels and bindings are Program-bound. | Participating mass generates forces; whole-body mass absorbs the sum. Body phase precedes contact/centroid wake, Q applies after combined calculation, and accepted displacement determines wide retained velocity. No hidden tail, host scheduler or same-poll global cascade. | Worker × bulk/direct native cases, arithmetic references, emitted selector/semantic IR audits, compiler profiles, unchanged-backing checks. Keep numerical corrections under #324 separate from file moves. |
| Completed observation / serialization: copy one coherent completed state | `viewport::{bind,observe,snapshot}` in `completed_state.cpp`/`snapshot.cpp`; `Harness::inspect_*`, `ActiveSession::report`; browser `model.mjs`/`viewer.mjs` consume snapshots. | Bindings retain exact record/context/incarnation and generation; reports/candidates are borrowed or copied according to their interface. Observation owns no second live numerical state. | Owner, generation, tick, frame and occurrence checks precede output. Omitted, unavailable, budget-refused, absent/coincident/unsupported candidate states stay distinct. Retained flat ActiveSession reports unavailable with null prepared generation. | `viewport_snapshot_test`, model/browser/schema checks and diagnostic state-hash controls. Preserve the existing completed-state seam; optional extraction of serializers must not add producer policy. |
| Caller decisions / temporary controls and supporting peers | `letter_capture.cpp`, `capture_options.h`, `signal_guard.h`, `run-capture.sh` adapt CLI choices and pacing; bounded `live_hierarchy_test.cpp::capture_current` is a fixture producer. SharedManager/Toolbox, DB/WAL and endpoint/scheduler remain separate owners. | Caller owns requested study/options and shared lease/output lifecycle. Browser watch reads files. No implemented general Analyst decision-maker or network command API was found in these entry paths. | SharedManager gates scope before lookup/source invocation. Private reports stay with the active owner. File pacing, snapshot validation and browser display do not decide admission, physics, relevance or stability. | Option/signal/snapshot tests; warm-active DB acceptance; browser/model tests. Keep adapters outside execution ownership, and avoid inventing a universal Analyst API or second scheduler. |

Composition/value preparation can be understood independently of kernel math and
SQL, using its standalone tests. Caller option parsing can be understood with
its snapshot/control contract. Session ownership needs the joint context of
logical generations, FixedStudy failure behavior and Program lifetime. Prepared
commit/map publication needs native generation/handle and exchange contracts
together. Packet publication, native wake and downstream force reads genuinely
need joint context; splitting them without their invariants would obscure the
ordering issue above.

## One existing retained study lifecycle

This is the concrete **legacy retained** route, not a claim that retained C3
admission exists. The entry is
[letter_capture.cpp](../../tools/viewport/native/letter_capture.cpp).

1. The CLI validates options and the actual backend, then opens read-only cold
   sources and a shared warm store. `SharedManager::read/publish/acquire` gate
   shared identity; the caller's lease guard pairs acquire/release. This route
   performs shared warm publication when actually run; this inspection did not
   access or write any database.
2. `ActiveAssembly::assemble` projects the acquired recipe and validates logical
   owner/local inputs; `focus` supplies the caller's chosen focus. This is an
   Analyst-facing choice surface, not Analyst inference.
3. `ActiveSession::submit` creates the designated SNode layout, calls `bind` and
   `prepare`, registers the tree with Program and constructs FixedStudy. There
   is no prepared record-claim/generated-placement admission at this point.
4. `FixedStudy::stage` fills the fixed legacy arrays and calls `Harness::publish`,
   seeds centroids and installs focus. Successful publication enables ticks.
5. `ActiveSession::tick → FixedStudy::tick → Harness::tick` advances the same
   legacy execution; its session completed-tick counter advances on success.
   It does not install a C3 hierarchy.
6. `viewport::observe(ActiveSession, owner, handle, request)` gets an authorized
   completed report and tick, verifies them again, and labels requested C3 intent
   unavailable. `snapshot` serializes the report; atomic file writes and browser
   watch are presentation/output, not a calculation path.
7. A caller may use `revise` plus `ActiveSession::refresh` on equal-shaped legacy
   input. `get` rejects stale handles; failed refresh prevents the old result
   being presented as current. This separate control exists in session tests;
   the retained capture CLI does not expose a general live refresh command.
8. The caller releases its shared lease, including failure/signal paths. Session
   destruction separately synchronizes and destroys execution objects before
   Runtime finalizes Program. These are distinct ownership events.

The missing transition is explicit: #342 supplies a coordinate-free structural
handoff, but an owning prepared admission/commit and occurrence-map publication
must connect it to C3. Reconstructing FixedStudy or inventing host candidate
coordinates would not implement that transition.

## One completed C3 tick and its independent observation

The concrete `intent_activity` fixture owns a prepared Harness, installs a
validated hierarchy and calls `Harness::tick64`. Its source initialization uses
test-only access; it is not the retained producer's missing admission path.

`Harness::tick64 → tick_values` validates ready state, parameters, generation-
bound prepared bindings and installed hierarchy. `Activity::begin` establishes
invalidation/focal inputs; `launch_clear` clears active-body accumulators.
`LiveHierarchy::force` gathers each active body's local/inherited contributions;
`launch_contact` adds canonical contacts. `launch_integrate` applies whole-body
inertia, the existing brake/radial damping and cumulative Q once, and writes
wide retained velocity from accepted displacement. Contact wake follows body
consumption. `LiveHierarchy::poll` then traverses native active centroids, reduces
leaves/complete child packets, publishes complete inactive packets, exchanges
selectors, and notifies parents/recipient bodies. Pending work survives for later
phases/ticks. The counterexample above lies inside this last traversal.

Runtime synchronization precedes Demand/error readback and completed-tick
increment; mutation failures disable readiness. `inspect_live_responses` and
`inspect_live_intent` subsequently observe the completed boundary without
changing model state. The intent diagnostic runs `IntentAccumulator` over
extracted local/inherited rows; it is the existing combined diagnostic, not the
new composition-only structural quantity proposed in #324.

The separate contact-safe path `Harness::tick_prepared → TickExecution::tick`
validates work/geometry budgets, uses staged backing/selection and invokes
`run_physics`; commit/failure and copied mirrors are additional responsibilities.
It must be consulted before moving common mechanics, rather than assuming the
fixture's direct tick is the only execution path.

## Native machinery: retained wrappers and bounded reuse questions

| Current mechanism | Inspected native machinery | Required semantics / proposed verification |
| --- | --- | --- |
| `NativeSet` wraps sparse binary activity | `native_set.h` builds pointer/bitmasked SNodes and emits `SNodeOpStmt`; Taichi `codegen/llvm/codegen_llvm.cpp` lowers activate/deactivate to native runtime operations. | Already native reuse. Retain explicit ancestor activation, one activity set/type and separate persistent values. Native-selection and pending-input tests must guard any simplification; a host queue is not a substitute. |
| HCP range/struct traversal and per-node reductions | Taichi `ir/ir_builder.cpp::{create_range_for,create_struct_for}` expresses worker/block and serialized-inner-loop controls; HCP hierarchy reductions consume native selections and explicit source/child rows. | Native traversal provides execution, not a topological composition solve. Preserve exact ratios, repeated occurrences, mixed complete packets, Q and pending notifications. Inspect actual emitted modules and ordered native controls before claiming an equivalent reduction or threading optimization. |
| `CompiledKernel::Invocation` / prepared argument wrapper | `runtime.cpp` delegates compilation/launch to Program; `CompiledKernel::prepare` uses LLVM KernelLauncher's `prepare_kernel`, while prepared contexts retain bound array/scalar storage. | Existing native preparation is already reused. Keep Kernel/Program identity and bound shape/lifetime checks. Further replacement needs allocation/reuse evidence and the existing owner-resource ledger; do not reopen accepted accounting work by guessing it is duplicate. |
| Hierarchy two-slot XYZ/mass publication and recipient checks | Native IR atomic OR-zero acquires and XOR-one exchanges, backend lowering, and existing sparse selection. | The HCP complete-packet/history/force-baseline protocol is model-specific. No inspected native operation supplies this entire contract. Retain it until an equivalent is demonstrated by selector/semantic IR audits plus numerical and pending-work controls. |
| Custom force, intent, brake and geometry expressions | HCP builds those expressions with Taichi IR/native arrays and dispatch; SNode storage itself defines no composition-derived intent formula. | Separate structural parent roles from membership/comparison attachments, then derive weighting/frame/publication semantics under #324. No replacement math, scheduler or structural-centroid claim follows from this map. |

This is a first source inspection, not a complete native-reuse audit or evidence
that replacing any remaining wrapper is safe or worthwhile.

## Proposed PR sequence and first extraction acceptance

1. **This map (Pass 1):** submit the source pin, lifecycle/tick trace, controlled
   failure diagnosis and boundaries for maintainer review. Runtime code stays
   unchanged. Preserve held #75/#111/#123 and pending #342.
2. **Qualification correction:** independently repair the test's publication-cut
   assumption with deterministic order controls and exact final equality. Add
   operand/packet diagnostics for future failures. Run the affected worker/path
   matrix locally and submit for authoritative hosted CPU qualification. Review
   any new evidence against the original failure before accepting #342.
3. **First mechanical extraction (Pass 2):** move `ActiveSession::Execution` and
   its methods from `active_workbench.cpp` to `active_session.cpp`; keep
   `active_workbench.h`, class/API names, target and link dependencies intact.
   Keep the small validation helper's behavior identical. Leave ActiveAssembly,
   preparation, geometry and numerical logic unchanged. Make no standalone-
   logical-target claim: `enclosing_radius` still lives in native-linked study
   preparation. Update CMake in the same commit.
4. **Inspect reuse and connect ownership in further reviewed slices:** use the
   separated session lifecycle to design the missing prepared bootstrap/commit
   owner, with exact structural input and post-commit maps. Review native-reuse
   candidates before creating abstractions around them. Establish complete
   reservation/lifetime/reject-retry tests for that bridge. Keep #324 derivation
   and later numerical changes in separate PRs.

The first move is reviewable with `active_workbench.*`, `study.*`, engine CMake,
the standalone projection CMake, and the active workbench tests. Acceptance:
identical submit/refresh/stale-handle/owner/report/destruction behavior; no new
state owner, SQL/private-input path, allocation policy or numerical behavior;
unchanged standalone dependency boundary; focused active-workbench and four
standalone tests plus applicable local/hosted CI. Integrate reviewed #342 first
or explicitly record it as a stacked dependency if its logical methods are
present. Do not silently drop its focus/restart distinction during the move.

Review the local source/evidence before provisional continuation. Hosted CI
remains authoritative for final qualification. A local pass or this proposed map
does not accept an extraction, clear the original failed run, or qualify GPU,
retained C3, complete accounting or Analyst implementation.
