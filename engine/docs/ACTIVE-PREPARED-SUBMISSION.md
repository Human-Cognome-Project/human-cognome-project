# Explicit prepared-study submission at active control

Status: **contributor implementation, awaiting review**, authorized by [#371 review 5956829592](https://github.com/Human-Cognome-Project/human-cognome-project/pull/371#pullrequestreview-5956829592), under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Tested source: `ae9b64856f6e29eabf0d076e851fb87c5f8de830`. Normal merge `8849c13e` preserves starting main `2c7ec4d0` and provisionally accepted #371 `4e8ac30c` (including #370). Dependency integration/checks are recorded below separately.

The [handoff inspection](RETAINED-C3-HANDOFF.md) found that prepared native bootstrap and claim mapping existed, while legacy `ActiveSession::submit` accepted only v1. This change adds **one explicit `submit_prepared` operation**, provisions its native context inside that existing session, and delegates to `PreparedStudyOwner::prepare/bootstrap`. It does not convert viewport reports into inputs or choose a comparison policy. Actual retained database acquisition/qualification remains separate.

## Request and supported boundary

Include `assembly/prepared_submission.h` to use `PreparedStudySubmission` and `PreparedSubmissionResult`. The caller supplies:

- A pinned `shared_ptr<const ActiveAssembly>` v2 source and the active owner UUID.
- Existing `AdmissionBindings`: exact source stamp, body definitions/intrinsics/physical masses, field declarations, explicit ordered comparisons/precision, explicit focus and root frame.
- Explicit layout **roles/parents/units**, with every `StudyNode::layout` pointer null. The session creates the matching live Program-owned nodes. No external native layout pointer is accepted.
- `PreparedCapacity`, a metadata `ReservationDomain`, independent plan/metadata/placement/cell/hierarchy work allowances and bulk/direct host path.

The implemented subset is shared v2, wide f64, actual LLVM/x64, root frame zero, nonempty focus, and the existing letter layout shape: root at units 1 plus an optional single primary step at units 64. The source recipe and supplied roles/parents must agree. Existing lower plan validation remains authoritative for complete source accounting, physical intrinsics and comparison/coverage constraints. V1 sources, local overlays, unavailable inputs, external layout pointers, unsupported layouts/units/profiles/backends and nonzero root frames refuse explicitly. No starts, inferred topology, automatic focus, exposure revision or new mathematics is introduced.

The request is consumed **by value** so its temporary layout pointer slots can be filled internally without mutating a caller's retained request template. A caller can move a disposable request or retain/copy one for refusal/retry. A successful execution retains the original immutable source pin through the existing prepared owner; it does not clone that assembly into a second execution-side logical authority.

## Provisioning and publication

`ActiveSession` still owns one Runtime and its execution map. `submit_prepared` checks authorization, supported input shape/source stamp and duplicate study identity before creating a candidate. Native scratch refusal uses the existing `PreparedStorage::required_scratch` calculator and reports `Budget::ScratchBytes` without context construction.

The candidate owns its Program-registered source layout, new empty prepared Harness and prepared logical owner. The session materializes root/primary layout nodes, builds the Harness using the supplied capacity/path, then invokes the existing `PreparedStudyOwner::prepare`. Source/plan/metadata refusal details remain separate in the result. If preparation succeeds, it calls the owner's existing combined bootstrap with the supplied placement/cell/hierarchy allowances. No standalone C3-1 installer is called.

A private staging map allocates the execution map node **before native publication**. Only after bootstrap succeeds and the logical owner reports ready is that node transferred to the public execution map using C++17 node insertion. The detached return-handle storage is prepared before bootstrap and moved into the result after map insertion. An absent `result.handle` means no authorized session execution was published. Authorization errors and unexpected context/backend failures throw; they do not publish a candidate handle. Ordinary refusal destroys the candidate and reports the reservation snapshot after its metadata is released; it never replaces an existing study.

`Execution` teardown drains, destroys prepared owner, then Harness, then the owned source layout. The [source-tree ownership correction](SESSION-SOURCE-TREE-OWNERSHIP.md) at `e226b76a` expresses this through the existing owner/drain and reverse member destruction, also ensuring legacy FixedStudy dies before its source tree. Session destruction clears executions before finalizing Runtime. The source layout uses the existing Taichi tree destruction path; no retained store, Git history or resource pool is reset. Native IDs may remain monotonic and Program compiler/cache storage may persist until Runtime teardown. This is not unlimited streaming, full setup rollback or process-fatal backend recovery.

The [native lifetime inspection](PREPARED-SUBMISSION-LIFETIMES.md) at `cf030244`
qualifies that teardown boundary more precisely on integrated `c2b2d4d2`: source
and record-map roots retire, while Activity and candidate-selection trees remain
Program-owned after their C++ components die. Repeated late refusals each leave
two live roots in the fixture; compiled data has its separate cache lifetime.
Its proposed component-local tree guards change no runtime here and do not
claim complete setup rollback, pooled-memory release or streaming qualification.

The [authorized ownership correction](NATIVE-TRAVERSAL-TREE-OWNERSHIP.md) at
`8c32cba4` subsequently implements #399 review 7063069841. Activity and selection
now retain their own registered trees, drain before dependent members die and
retire afterwards during normal teardown or constructor unwind. Repeated
plan/placement and injected partial-constructor refusals leave their roots
retired while the sibling and native authority remain intact. Compiler/JIT and
pool/list high-water keep their separate lifetimes and accounting limits.

## Control and observation

Prepared handles use the existing logical `ExecutionHandle` and owner authorization. `ready` dispatches to the actual owner; `completed_ticks` reads the prepared Harness's native completed-tick counter. `tick_prepared(handle, owner, Parameters64, ...)` delegates to the existing contact-safe owner tick, including native-generation following. Legacy `tick(Parameters)` and `refresh` refuse a prepared handle before mutation; v1 submit/refresh/tick behavior is retained. One execution per study is published: prepared resubmission/revision is not an implicit replacement operation.

`ActiveSession::report`, `accept`, demand and timing access support the prepared entry through the same owner/native state. Prepared reports use wide physical vectors; `PreparedStudy` rows remain identity/scale metadata rather than copied source starts. Reports are detached values, not admission input or independent current execution state. Existing logical owner/generation checks do not turn an old detached physical report into a fresh completed-tick observation.

The previous viewport-only prepared claim metadata construction moves into the prepared owner as one shared builder. `PreparedStudyOwner::report` reads native physical values for core callers. The viewport owner observer uses the same metadata builder and its existing native observer, avoiding a second physical download. A private authorized session adapter dispatches `viewport::observe(ActiveSession,...)` to that existing prepared observer. Native comparison rows, full claims, generation and completed tick are therefore exposed through the new entry without a second mapping or numerical solve.

Legacy session observation continues to report C3 diagnostics as unavailable. The retained capture executable still deliberately calls v1 normalization/submit; it is not automatically migrated by this API. No database lookup or warm publication is performed by the new entry.

## Build boundary

A separate mechanical commit creates `active_assembly` for the unchanged logical `active_workbench.cpp`. The public `active_workbench` target retains session implementation and links the existing prepared owner. `structural_admission_plan` links the logical target, avoiding an active-session/prepared-owner dependency cycle. Existing clients still link `active_workbench`; no second Runtime owner, field implementation or scheduler is added.

The source sequence separates this dependency change (`a077f305`), shared report builder (`37e19839`) and submission behavior (`ae9b6485`). Field numerical/tick kernels, Taichi fork, common allocation machinery, database code and workflows are unchanged from #371. The small hierarchy-entry counter helper is shared between the inspection witness and new entry tests; it only instruments their Linux GNU/Clang executables.

## Focused acceptance witness

`active_prepared_submission_test` exercises the **new public session entry**, not a fixture-only call to native bootstrap. It keeps a prior legacy study live while submitting aggregate and parts v2 sources. It verifies:

- Wrong owner, external native pointer and v1 input refusal; explicit insufficient native scratch refusal before native hierarchy work.
- Placement-work refusal with no handle, no hierarchy commit and no retained owner metadata; corrected retry succeeds. A later plan-work refusal also leaves the already published prepared study's full claims and numerical values unchanged. Prior legacy state stays ready and bit-identical throughout those refusals.
- Two successful studies produce exactly two existing combined hierarchy commits and zero standalone installs. Duplicate study submission cannot replace one. Caller-selected two/three comparison trees, scale units and actual claimed context/incarnation/address/record identity are exposed through session observation.
- Core reports agree with native viewport physical values. Foreign owner/session/generation requests fail. Legacy tick/refresh cannot mutate a prepared execution.
- Source pins survive release of caller references, a refused tick budget preserves currentness, both prepared handles complete a native tick with current generation/observation, and the legacy study still ticks. Earlier detached observations remain unchanged. Normal session teardown releases the source pins with the explicit owner/Harness/Runtime ordering.

The cases cover bounded fixtures, including one root-only and one primary-step source. They do not qualify the complete retained English/French/Dutch study, local/private overlays or general live representation changes. Request/native-context setup and detached reporting allocate; no zero-allocation claim is made for submission as a whole. The existing plan/owner reservation covers its documented metadata surface, while Runtime/layout/Harness/compiler and caller request storage remain separate resource surfaces. No full C+B+H+D accounting claim is added.

## Local evidence and integration gates

All final profile result files exit zero at the tested source:

- Quick CI passes three native CTests, 632-sample placement arithmetic and placement capacity.
- Clang 15 passes **13/13 scoped CTests in 741.69 seconds**: all four new entry modes, two existing prepared-owner modes, two retained-handoff modes, two existing viewport-owner modes, structural admission, legacy active workbench and the legacy transfer experiment.
- GCC 11 passes **4/4 scoped CTests in 233.06 seconds**: 1-worker bulk / 2-worker direct new entry, legacy workbench and the existing 2-worker direct prepared observer.

The six new-entry modes cover **12 successful prepared submissions** and per-mode placement/plan refusal/retry, prior legacy/prepared preservation, actual claims, source/authorization checks and following ticks. They observe two combined native commits and zero standalone installs per mode. Field/fork/common/database/workflow diffs remain empty. The earlier unpinned link/development logs and compiler errors (BundleRef/RecordHandle comparisons requiring their existing typed checks) are retained separately; the corrected development probe is not counted as pinned qualification. Post-PASS cache-lock warnings remain unchanged.

#370 head `eee4ef07` passed native workflow [44299547257](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/44299547257) and smoke [44299546912](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/44299546912). As requested by review 5956829592, it was integrated with a normal merge as main `15299964`, and #371 was retargeted to main. Its native workflow [44306271650](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/44306271650) remains pending in this handoff. Normal merge `718e79a0` brings qualified main into this branch with a tree **identical** to tested `ae9b6485`. The new operation's review/hosted checks and #371's remaining gate stay separate.

Verified machine-local bundle: `/tmp/hcp-active-prepared-ae9b6485-evidence.tar.gz`, SHA-256 `d08628d5b0c253f349ca445771d62551abfbd0dc283c0598ddcd001769fc3f9e`. Profile and bundle manifests were checked. It retains the source sequence/patch, full current logs/commands, source cleanliness, compiler/runtime/binary hashes and unqualified development logs. Publishing a hash does not publish this local raw bundle.

Reproduction on the matched CPU build:

```sh
bash engine/scripts/run_local_ci.sh quick /absolute/new/quick-evidence
ctest --test-dir engine/build -j 1 --no-tests=error --output-on-failure \
  -R '^(active_prepared_submission_[12]_(bulk|direct)|viewport_prepared_owner_(1_bulk|2_direct)|retained_c3_handoff_(1_bulk|2_direct)|prepared_study_owner_(1_bulk|2_direct)|active_workbench_test|active_transfer_experiment|structural_admission_plan_test)$'
```

The retained script records GCC configuration and its selected new-entry/legacy/observer cases, source pin/cleanliness, compiler/runtime/binary hashes and full logs. Scope is local CPU/x64. GitHub CI remains authoritative; no full local regression, no-unroll matrix, database or CUDA execution is claimed. Refresh transport, reservation redesign, full retained acquisition and #324 remain separate.
