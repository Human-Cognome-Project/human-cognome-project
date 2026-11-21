# Native lifetime after refused prepared submission

Status: **inspection provisionally accepted in #399**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Starts from integrated main `c2b2d4d276803cdacca6cef353c505ede9be6755`, whose tree equals qualified #398 head `0ddb8b35`. [Maintainer acceptance 7062570528](https://github.com/Human-Cognome-Project/human-cognome-project/pull/398#issuecomment-7062570528) confirms #396–#398 passed hosted native/smoke and authorizes the next responsibility/native-reuse inspection. Tested source `cf0302448e2954da7eed089f1109f325b5335dfb`, tree `eddfcf5032898ea0ce2dcc44ec096e5394b405c6`, adds only a native inspection executable and four CTests. Production, native ownership and achieved saving are unchanged: **zero runtime saving**.

[#399 review 7063069841](https://github.com/Human-Cognome-Project/human-cognome-project/pull/399#issuecomment-7063069841)
accepts final `c0b8a216`, verifies all ten post-test changes are Markdown and
authorizes the [component-local runtime correction](NATIVE-TRAVERSAL-TREE-OWNERSHIP.md).
Original ownership and zero-saving measurements below remain pinned to this
inspection source; later root retirement is measured separately. Its remaining
hosted qualification and all maintainer integration remain separate.

## Request, control and current owners

The caller selects a source, exact declarations/comparisons/focus, finite capacities and work allowances. `ActiveSession::submit_prepared` is the control adapter; it checks authorization, source stamp, supported layout/profile and scratch before staging a prospective execution. It does not select Analyst policy or move numerical authority out of Harness. The session owns one Runtime/Program and the visible execution map.

After early checks, [active_session.cpp](../src/assembly/active_session.cpp) creates a source SNode tree and prepared Harness, then calls the existing `PreparedStudyOwner::prepare`. Plan-work or metadata refusal therefore occurs after native context provisioning. Placement-work refusal occurs still later, through the existing owner bootstrap. Rejection clears the staging map and publishes no execution handle; healthy retry constructs a fresh context. `Execution::~Execution` destroys the prepared logical owner, Harness and source layout in that order.

The C++ Harness lifetime and all native lifetimes are not identical:

| Component / resource | Current native ownership and release |
|---|---|
| Session source layout | `Execution::layout` retains the registered tree. Its destructor calls `Program::destroy_snode_tree`. |
| Two prepared record maps | [record_map.cpp](../src/engine/record_map.cpp) owns a `Tree` guard declared before kernels/contexts; reverse destruction synchronizes and retires its tree after dependent members. |
| Activity traversal tree | [Activity::Activity](../src/field/activity.cpp) attaches two native sets, registers the tree and discards the returned handle. Activity has no tree-release guard. Program keeps its tree/backing live after Activity dies. |
| Candidate-selection traversal tree | [NativeSelectionWorkspace](../src/field/native_selection.cpp) likewise registers two candidate sets and discards the tree handle. Its committed sets borrow Activity; candidate backing remains Program-owned after workspace destruction. |
| HCP Kernel definitions and prepared contexts | `engine::CompiledKernel` owns its Kernel definition and prepared invocation/context members have their own component lifetimes. These are distinct from native compiled-data ownership. |
| Compiled kernel data | [Program::compile_kernel](../taichi/taichi/program/program.cpp) delegates to [KernelCompilationManager](../taichi/taichi/compilation_manager/kernel_compilation_manager.cpp), whose cache owns the returned compiled data. HCP wrapper/Kernel destruction does not evict that cache. |

[NativeSet](../src/field/native_set.h) is a borrowed traversal view. It must remain non-owning: copying a view does not establish exclusive ownership of its underlying tree. [Harness member order](../src/field/field.h) already destroys placement/tick/hierarchy dependents before Activity. The issue is retaining/releasing the two owning tree handles at the appropriate existing component boundary, rather than adding another allocator, free list or scheduler.

## Real submission witness

[prepared_submission_inspection.cpp](../tests/prepared_submission_inspection.cpp) calls the public session submission API, existing bootstrap, native tick and completed viewport observer. Test-only linker wrappers observe the HCP Runtime constructor/destructor, native tree add/destroy and native compile calls. They do not expose private fields, mutate backends, remove caches or change production code. The witness covers:

- A root-only aggregate study, warmed through a native tick and completed observation.
- Scratch refusal before provisioning, plus duplicate-study refusal with no new tree, SNode or compile call.
- Two repeated zero-plan-work refusals and one zero-placement-work refusal for a second primary-step/parts study. Each has no published handle, no retained metadata-domain bytes and no new hierarchy commit.
- After every refusal, the first study stays ready with the same completed generation/tick, full body/field claims, comparison metadata and bit-identical completed f64 particle/group arrays. Its observed root tokens remain live.
- Healthy retry publishes the second study via the existing combined bootstrap. Both studies then tick without new tree, SNode or compile calls. Exactly two combined commits and no standalone installation occur.
- Caller source references can die while successful executions retain their pins. Session teardown releases those pins; the test queries native root tokens immediately before Runtime finalization and never queries a dead device, stale tree pointer or compiled pointer after Program destruction.

Materialized roots are classified from the actual layout: the two-pointer/bitmasked native-set shape is distinct from the source layout and dense/pointer/bitmasked record-map shape in this fixture. Root records are copied value tokens, then queried through the independently live owning device. Reused tree IDs never substitute for allocation-token lifetime. This is an observation of the bounded supported fixture, not a generic graph enumerator or new logical identity scheme.

## Measured native lifetimes

The native root/compiled-object counts below agree across the final compiler/worker/path profiles; all raw quantities, including timing and path-dependent pool counters, remain separate in the evidence.

| Phase | Root adds / destroys | Live / retired observed roots | Live root requested bytes | Tree slots / free IDs | Compile calls / distinct objects |
|---|---:|---:|---:|---:|---:|
| Empty / early scratch refusal | 0 / 0 | 0 / 0 | 0 | 0 / 0 | 0 / 0 |
| Warm first study / duplicate refusal | 5 / 0 | 5 / 0 | 20,480 | 5 / 0 | 97 / 88 |
| First plan refusal | 10 / 3 | 7 / 3 | 28,672 | 10 / 3 | 194 / 168 |
| Repeated plan refusal | 15 / 6 | 9 / 6 | 36,864 | 12 / 3 | 291 / 248 |
| Placement refusal | 20 / 9 | 11 / 9 | 45,056 | 14 / 3 | 388 / 328 |
| Healthy retry / following ticks | 25 / 9 | 16 / 9 | 65,536 | 16 / 0 | 485 / 408 |

Immediately before Runtime finalization, after the C++ executions are gone, ten set roots remain live / 40,960 requested bytes; every observed source/record root token is retired. The witness checks through the still-live device at that boundary. It does not query tokens after the device dies or claim that process-global pool reservation disappears at Runtime destruction.

Only three of each refused candidate's five roots retire: source layout and two record-map trees. The two set roots stay live. Each requested root is 4,096 bytes in this CPU fixture, so each late refusal retains 8,192 additional requested root bytes. Native node/list backing, host tree descriptors, compiled data and allocator metadata are additional surfaces. This root figure is not their combined total.

Tree slots and IDs show native reuse: after the first refusal, three IDs become free; the next attempt reuses those slots while its two unreleased set trees increase the high-water by two. Native SNode IDs still advance, and reused tree IDs do not revive old allocation tokens. The source/record tokens remain retired, the native-set tokens remain live and unrelated study tokens remain live throughout.

Each constructed context makes 97 observed native compile calls. The warmed first context has 88 distinct compiled-data objects; each subsequent failed or successful context adds 80. Some calls therefore reuse existing native compiled objects, while many bindings remain context-specific. Program's `kernels` vector is empty in these samples because HCP owns its Kernel definitions directly; that empty host registry does not imply an empty compilation-manager cache. Compiled objects remain safely readable through their Program-owned lifetime after candidate C++ teardown. Distinct objects are those returned through observed HCP calls, not a complete compiler/cache enumeration. No compiled-cache byte total is exported or inferred from object counts.

Runtime requested bytes and pool requested/reserved counters are retained in the raw samples. They include cumulative and process-wide quantities. They are not live allocation sums, physical residency, complete B or disjoint quantities to add to root bytes. The native allocation-record capacity is retained host bookkeeping, separately reported from backing. No admission/accounting policy changes here.

## Compiled artifacts and supported reuse

The Clang one-worker bulk profile serializes the actual first two native compiled objects from each of five context constructions: ten LLVM modules plus ten metadata files. The audit matches every task name to an emitted function, validates the empty scalar/return ABI and verifies all modules with LLVM 15 `llvm-as`. Five activation kernels each have one range task; five deactivation kernels each have a range task plus native GC, fifteen tasks total. Their node-manager selectors specialize to node IDs 5 / 38 / 71 / 104 / 137 in this run. These are compiler/storage IDs, not HCP composition or occurrence identity.

The pinned fork's non-AST `make_kernel_key` uses the kernel name; it does not prove layout compatibility from equal capacity. Record-pool preparation names include the actual node ID. Emitted modules contain the corresponding native node-manager access and activation/deactivation path. Replacing an old context with a new tree ID/shape is not permission to launch an old bound kernel against it. Existing native tree destruction removes field/accessor cache associations for that tree, while unrelated compiled data and trees retain their own Program lifetime.

The warmed observation and following ticks issue no observed tree add/destroy or compile calls. This supports continued use of those existing bindings in this fixture; it does not establish cross-process cache equivalence, arbitrary layout mutation, heterogeneous/ragged composition or production mixed-revision adoption. The earlier [nested-instance inspection](NATIVE-INSTANCE-INSPECTION.md) retains those separate limitations.

## Smallest proposed implementation

Retain the native tree handle in each of the existing Activity and NativeSelectionWorkspace owners, using the already exercised `Program::destroy_snode_tree` mechanism. Keep NativeSet borrowed, and retain only the candidate tree in the selection workspace; committed sets still belong to Activity. Establish the guard immediately when registration succeeds so later constructor failure unwinds it. Destroy dependent kernels and prepared contexts first, drain relevant work and retire the tree while Runtime/Program remain alive. Use the record-map guard as the concrete reference; do not introduce a per-constituent ledger or a second native lifecycle authority.

A reviewed follow-up must prove constructor-failure ordering, repeated refusal/retry, source/record/set token retirement, tree-ID reuse with old tokens still retired, preserved sibling claims/numerics, following ticks, source/weak release and unchanged supported legacy/prepared behavior. Measure actual root and pool/list behavior after that implementation: retiring a root does not guarantee all runtime-pool high-water is released. Keep compiler/JIT cache retention and any safe eviction proposal separate. No cache purge, root retirement or measured saving is implemented by this inspection.

This is a bounded #343 responsibility/lifetime proposal. Broader selective adoption, mixed-revision provenance, complete borrowed-resource aggregation, GPU qualification and #324 coherence mathematics remain separate. The operation contract's ordinary-streaming requirement is not qualified by repeated full-context submission; this experiment is setup refusal/retry.

## Source-pinned local qualification

Final profiles pin canonical source `cf030244`, tree `eddfcf50`, with a clean worktree before/after. Clang/LLVM 15 and GCC 11 Release wrappers use fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53` and runtime-bitcode SHA-256 `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`; warm integration/viewport are enabled and CUDA qualification disabled. Configuration, exact commands, submodules, binaries and raw output are retained.

Local quick CI passes its three CTests, 632 placement arithmetic controls and placement-capacity case. The new witness passes all four Clang worker/host-path combinations and GCC one-worker bulk/two-worker direct. The invocation arguments match the four registered CTest commands; only Clang one-worker bulk adds optional artifact emission, from an evidence directory. All four CTest registrations are verified. These are six real native executions, not a claim that a complete CTest regression matrix ran locally.

The cross-profile comparison checks 78 sample records and 65 pairs against the first GCC fixture, including identical native root/token-lifetime, SNode/tree high-water, observed compile/compiled-object, allocation-record capacity and runtime-request counters. Timing and process-wide/path-dependent pool requested/reserved values are retained separately rather than asserted equal. Across the six independent Runtime executions the witness observes 150 materialized roots, eighteen late refusals, twelve early refusals, twelve combined commits and no standalone installer, plus healthy retries and following ticks. Final root queries occur only while each Runtime/device is alive.

Unchanged-production and integrated-parent tree proofs permit reuse of the qualified #398 baseline; it was not rerun merely to onboard. This test-only scope does not qualify a future runtime change, complete B, GPU, native Windows, database behavior or the full hosted compiler/regression matrix. Existing post-PASS `ticache.lock` warnings remain in raw output. No cache, retained data or Git history was cleared. Hosted GitHub CI is authoritative for its scope.

Verified machine-local bundle: `/tmp/hcp-submission-inspection-cf030244-evidence.tar.gz`, SHA-256 `14f559938bb044304b857882f4d02e17dd26e8049b6a742a33bae23add4fc6bb`. It retains all three final profiles, commands/configurations/submodules, source-before/after cleanliness, binary/runtime hashes, raw native observations, emitted artifacts, cross-profile comparison, artifact audit and separate development evidence. The final archive was extracted afresh and its root plus every profile manifest verified. Local/canonical tree equality, integrated-parent tree equality and empty production/fork/boundary diffs are retained. A local hash identifies contributor evidence; the raw bundle is not remotely available through this record.

The initial unpinned test assumed all five roots retired and failed after observing five additions but only three destroys. Its raw log is preserved; that failure exposed the ownership distinction above. The revised witness observes current source behavior rather than claiming it has been fixed. An earlier test-only compile attempt used a nonexistent completed-state member and lacked the existing handle-comparison header; the interfaces were corrected before source pinning. The first local artifact audit assumed one task per module; native deactivation also emits GC. Its partial output/script are retained, and the corrected audit checks both tasks. These development/audit assumptions did not change production or weaken an accepted runtime assertion.

The authorized [runtime follow-up](NATIVE-TRAVERSAL-TREE-OWNERSHIP.md) measures actual retirement, constructor unwind and preserved operation. Reconcile its review/authoritative CI before maintainer integration. Original inspection results remain source-pinned, with no saving at that source; compiler-cache work and mathematics stay separate.
