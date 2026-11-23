# Prepared source layout: reuse and ownership

November 23 disposition: [#417 decision 7108110435](https://github.com/Human-Cognome-Project/human-cognome-project/pull/417#issuecomment-7108110435)
source-accepts `0d1fd0a2` and authorizes the prepared-specific helper while keeping
registration and candidate lifetime with the session. The
[separate implementation](PREPARED-SOURCE-LAYOUT-HELPER.md) at `68448975` now
returns that extraction for review. The original inspected source, reuse decision
and unqualified native-materialization failure edge below are preserved.

Inspection for review under #343, following [#416 decision 7107054533](https://github.com/Human-Cognome-Project/human-cognome-project/pull/416#issuecomment-7107054533).
**Keep the prepared topology builder local. Reuse the existing native SNode
operations and `SNodeTreeOwner`; no existing assembly builder is a drop-in
replacement.** The smallest proposed follow-up isolates only the unregistered
graph construction and pointer binding in a private helper. Registration,
candidate ownership, bootstrap and map transfer stay in the session.

## Inspected source and evidence

Inspected #416 `adc3ad2903448cdc01f452468b1432cc6ed42db5`, tree
`a0df276df9aaffeaae791ea1c68fe1a3da3bea37`, native fork subtree
`a94a9eb350c94dfa2eb586bee49c53e0ee886648`. The maintainer source-accepted that
head and independently reconstructed its parent translation unit. #415 and #416
remain open with smoke passed and native CPU workflows running; main remains
`d78c6d86`. This inspection pins the reviewed #416 dependency and changes only
Markdown. Integration order remains maintainer-owned: #415, then #416 against
main after verifying its integrated tree.

All executable files are identical to tested #416 source `9c3f03a8`, tree
`e2d8cc44768719d773a29d7e23283e1bb9de5fdc`. Its verified local quick/eight Clang/
six GCC results are reused below; no native suite was rerun and no new native
execution is claimed. The local native-engine boundary and curated-fork integrity
checks pass. Source/evidence equivalence and archive manifests were checked directly.

## What the prepared path constructs

After [request preflight](PREPARED-REQUEST-PREFLIGHT.md),
[`ActiveSession::submit_prepared`](../src/assembly/active_session.cpp#L140)
allocates the existing staging-map candidate and authorizes its `Execution`.
Only then does it create a root `SNode`, bind `b.layout[0].node.layout`, optionally
attach `dense(Axis(0),1)` and bind index 1, and append the terminal `place` node
with `i32` type. The accepted subset is root-only or root plus one primary step.

`b.layout` already carries the validated role, parent and units. Construction
fills only its native pointers; it does not infer scale, field topology or model
semantics from SNode IDs/depth. The scalar place is a source-layout anchor, not
the prepared particle/field payload or admission mechanism.

The public request is passed by value. The caller's reusable request template
keeps null layout pointers; only the session's temporary binding table is patched.
Moving a `unique_ptr<SNode>` transfers ownership of the graph without moving its
pointees, so root/child addresses remain the same through registration. The later
plan builder verifies explicit source identity, pointer uniqueness, actual parent
adjacency, roles and units. It consumes these borrowed views; it does not own the
source tree. See [request](../src/assembly/prepared_submission.h),
[plan validation](../src/assembly/structural_admission_plan.cpp#L160) and
[caller-template control](../tests/active_prepared_submission_test.cpp#L28).

## Compared builders and consumers

| Existing code | Binding and ownership | Reuse decision |
|---|---|---|
| Production legacy `ActiveSession::submit` in [active_session.cpp](../src/assembly/active_session.cpp#L92) | Builds root/optional one-cell primary/terminal i32, but fills `Execution::nodes`. It binds and validates v1 `StudyInput` before registration, then constructs `FixedStudy`. The same Execution retirement guard handles the registered source. | Closest topology, different adapter/order and study contract. Reusing the whole route would import legacy publication/input behaviour. Keep those adapters separate; native primitives/guard are already shared. |
| [prepared_owner_fixture::Input](../tests/prepared_owner_fixture.h#L16) | Owns an unregistered root while assembling native pointer bindings, explicit test addresses and comparisons. `install_layout` registers it but does not retain the returned retirement handle. | Test setup, not an owning production builder. Its Runtime/Program lifetime does not replace candidate-scoped cleanup. Do not promote the fixture or its input policy. |
| [letter_projection_native_test::Layout](../tests/letter_projection_native_test.cpp#L22) | Owns an unregistered root/optional primary and a `StudyNode` vector for binding/validation; it does not materialize a complete prepared source tree. | Useful comparison of explicit binding roles, not a registration factory or lifecycle owner. |
| [assembly_test::layout](../tests/assembly_test.cpp#L24) and [prepared_sources::letters](../tests/prepared_sources.h#L37) | Test builders register with Program and return borrowed node/value data. The former covers secondary compression/naming-equivalent roles; the latter always creates a primary node even when not exposed in its binding vector. Neither returns a candidate retirement owner. | Broader or different physical shapes and lifetimes. Do not expand the accepted root/primary subset or hide registration behind them. |
| [structural_admission_plan_test::Example](../tests/structural_admission_plan_test.cpp#L50) | Builds a declared graph from recipe parents, including compression/naming roles, for bounded plan validation. It retains an unregistered root and test-only binding policy. | A logical validation fixture, not evidence that arbitrary recipe nodes can be materialized by the active submission operation. |
| [study.cpp::scale_bindings](../src/assembly/study.cpp#L54), `ActiveAssembly::bind` and structural plan validation | Consume supplied pointers and explicit roles/parents/units; validate and derive study values. They do not construct or register a source layout. | Keep their validation/binding responsibilities. Calling them is not a substitute for layout construction or retirement. |
| [Activity](../src/field/activity.cpp), [NativeSelectionWorkspace](../src/field/native_selection.cpp) and [RecordMap](../src/engine/record_map.cpp) | Construct sparse activity/candidate/declaration schemas, not the source role anchor. They use the same tree-retirement mechanism where applicable. | Reuse `SNodeTreeOwner`, not these unrelated physical schemas. |

No central production assembly layout factory was found in this scoped search.
The production source-layout construction sites are the two session paths above;
similar test-local functions do not establish a reusable ownership interface.

## Ownership and exception boundaries

| Stage | Actual owner and cleanup |
|---|---|
| Before candidate creation | Request preflight has not allocated the source tree. Early refusal leaves the existing execution map and native context unchanged, as the accepted witness checks. |
| Candidate exists, graph under construction | Local `unique_ptr<SNode>` owns the graph. Binding pointers are temporary non-owning views. A C++ construction exception destroys that local graph; the by-value request is discarded. The Execution's retirement handle is still null. This does not promise rollback of every consumed native identity. |
| Inside `Program::add_snode_tree` | The unique root moves into a temporary native `SNodeTree`; Program assigns an ID and materializes before storing the tree and returning its handle. The session cannot retire by a returned handle until the call succeeds. |
| Successful registration | The returned pointer is assigned immediately to `e.layout.node`, whose Program was set by `Execution` construction. Program owns the native `SNodeTree`/graph; the existing `SNodeTreeOwner` guard owns the responsibility to drain/retire its backing. |
| Later constructor, plan or bootstrap refusal | The unpublished staging map destroys the candidate. Reverse member order drains first, destroys prepared owner before Harness and legacy FixedStudy before its source, then retires the source through the still-live Program. The request's borrowed node pointers do not own or extend that lifetime. |
| Successful publication | The same pre-reserved map node transfers to `executions_`; layout ownership does not move to an independent builder. Session teardown synchronizes, clears executions, then destroys Runtime. |

Source: [Execution and session teardown](../src/assembly/active_session.cpp#L59),
[SNodeTreeOwner/Drain](../src/engine/snode_tree_owner.h),
[Program registration and retirement](../taichi/taichi/program/program.cpp#L251),
[native root ownership](../taichi/taichi/struct/snode_tree.h) and
[tree construction](../taichi/taichi/struct/snode_tree.cpp).
Normal retirement releases backing through the existing backend and recycles the
tree ID; Program's C++ tree slots/capacity and retained compiler/JIT state are
separate lifetimes. No all-resource reclamation claim follows from root retirement.

The reused tests inject Activity compilation, selection compilation and selection
launch faults after registration and verify retired roots, preserved siblings and
successful retry. They do not inject every failure inside native materialization
or final Program-vector insertion. C++ graph ownership at that earlier boundary
is visible in source; complete rollback of backend effects is not qualified here.
Keep that limit separate from the demonstrated candidate cleanup. The proposed
helper leaves the native registration call and its exception behaviour unchanged.

## Smallest justified follow-up

Propose one private, prepared-specific helper that takes the already validated
`AdmissionBindings &`, creates the same unregistered root/optional primary/place
graph, fills its existing pointer slots and returns `unique_ptr<SNode>`. It must
not take a Program, Runtime, staging map, Harness or candidate owner. Its source
can remain local to `active_session.cpp`; no new public type/library is needed.

Keep `e.layout.node = program.add_snode_tree(...)` in `submit_prepared`, in the
same position after staging and before Harness construction. Preserve the scalar
type, axis/extent, pointer identities, request-copy boundary, check order, source
guard and drain. Do not broaden layouts or route prepared input through v1 bind/
prepare. This separates graph preparation from registration without inventing a
general builder or a new lifecycle owner. It is a proposal for review, not an
implemented extraction or permission to move bootstrap/publication/map transfer.

Exit evidence should reconstruct the existing graph-building block, preserve
root-only/primary shape and binding roles, and reuse the accepted early refusal,
constructor unwind, late refusal/retry and bulk/direct survivor controls. Any
change to native registration-failure cleanup needs a separate bounded scope.

## Validation and handoff

Reused [#416 evidence](PREPARED-REQUEST-PREFLIGHT.md) at exact executable source
`9c3f03a8`: local quick CI, eight Clang and six GCC CTests; 132 matched phases and
24,024 row/dylib observations. The current inspection changed no executable file,
so this is verified prior evidence, not a new native test run or new GPU/concurrent
qualification. Native-engine boundary and curated-fork checks pass locally.

Verified local archive `/tmp/hcp-layout-adc3ad29-evidence.tar.gz`, SHA-256
`7f438f20ec0013ab6dc4189ee9e66ad1db64959b74f266b6a514ae831a21fb57`, retains inspected
source/tree/submodules and comparison sources, commands, source-equivalence proof,
review receipt, boundary results and the manifest-verified #416 bundle. Fresh
extraction verifies manifests and reused raw-log equality. This machine-local path
does not publish raw evidence remotely.

Return the keep-local/reuse disposition and private-helper proposal for review.
New authoritative GitHub CI and parent #415/#416 gates/integration remain separate.
No production storage, cleanup, admission credit, complete resource accounting,
registry, public Analyst API or numerical behaviour changes. #324 stays separate;
#75/#111/#123 remain held. All merges remain maintainer-only.
