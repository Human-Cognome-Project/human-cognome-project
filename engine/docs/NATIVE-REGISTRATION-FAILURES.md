# Native CPU registration failures and retry ownership

Status: **inspection/proposal awaiting review**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343) and the [refactoring plan's inspection sequence](../../docs/engine-refactoring-plan.md#purpose-and-boundaries). This is independent work on main `92e4ceaf8858111b03b36599dfe86e124da4bed4`, which integrates qualified #405 with reviewed `06e71404`'s exact tree. The [accepted reclamation design](NATIVE-RECLAMATION-DESIGN.md#proposed-implementation-sequence-for-review) requires partial-registration association before any registered retirement. This contribution supplies executable observations and a bounded proposal, without changing production behavior.

At the handoff check, #406 remains open at `33da9a0f`, with provisional acceptance in [decision 7081114329](https://github.com/Human-Cognome-Project/human-cognome-project/pull/406#issuecomment-7081114329); its smoke and [native CPU gate](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/45035647465) have passed. #407 remains open at `fccae608`, stacked on #406, with its handoff still awaiting review. Its smoke passed and [native CPU gate](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/45040186843) remains pending. A chat “accepted” is review notation; actual refs, published disposition and handoffs determine changes/status. This inspection imports neither pending implementation and does not qualify their combined tree. Every merge remains maintainer work.

Tested source `816b8edb8e92c91d265799319f1c2e5a9a73f65e`, tree `d0bb9e01d443b9ea7d877ab24375ef9e91dfe0ed`, adds only [`native_registration_failure_inspection.cpp`](../tests/native_registration_failure_inspection.cpp) and CMake registrations. Fork tree `3c1fd8859bfc04b9d1587f3e907478b8a6bc54c4`, all HCP production, numerical code and existing fixtures are unchanged from main. Later handoff/report files are Markdown only. No runtime field/layout, association table, eviction, row reuse, ORC removal, charge or admission policy is introduced; achieved resource saving is zero.

## Finding: records, attempts, slots and modules differ

[`cpu::KernelLauncher::register_llvm_kernel`](../taichi/taichi/runtime/cpu/kernel_launcher.cpp) allocates a handle ID and resizes `contexts_` before cloning IR/parameters and creating a JIT module. It resolves tasks into a local vector, populates the context and only then stamps the data handle. An exception can therefore leave a default row with no published handle. A later failure can additionally leave a session-owned module wrapper. Retrying the same cached record appends another row; a successfully stamped handle is reused thereafter.

The new fixture warms two actual HCP i32-store kernels, then injects exceptions at the existing executor call boundary before native module creation and after it returns. These are test-injected outer boundaries, not LLVM backend-error or cleanup-error tests. Each candidate's ordinary context/definition dies; its cached data remains owned by the manager. Retry resolves to that exact data object and performs real native dispatch. A subsequent same-record revisit registers nothing new.

| Completed phase | Pending compiled objects | Records with handles | Launch slots | Stored kernel module wrappers |
| --- | ---: | ---: | ---: | ---: |
| Two survivors | 2 | 2 | 2 | 2 |
| Failure before module creation | 3 | 2 | 3 | 2 |
| Its successful retry | 3 | 3 | 4 | 3 |
| Failure after module creation | 4 | 3 | 5 | 4 |
| Its successful retry / all definitions gone | 4 | 4 | 6 | 5 |
| After Program finalization | 0 | 0 | 6 | 5 |

One initial runtime module is separate throughout: the final total is six module wrappers. Failed rows 2 and 4 remain empty, with zero task/parameter capacity and no successful handle. Retries use IDs 3 and 5; surviving IDs 0 and 1 remain unchanged. Six registration attempts belong to four compiled records. The later record has two distinct simultaneously retained native modules, including the one whose creation returned before the injected exception. The failed module is never looked up or dispatched by the fixture. No executable-byte amount or materialization state is inferred from its wrapper.

The [`compiler/JIT count APIs`](NATIVE-COMPILER-JIT-COUNTS.md) remain truthful bounded observations with `complete=false`. `contexts_.size()` counts slots, including failed default rows. `kernel_modules` counts stored non-runtime wrappers, including the abandoned successful creation. Neither is a count of successfully registered records or a one-to-one ownership relation. Four records, six slots and five kernel wrappers all describe the same actual owner state here.

## Identity, dispatch and shutdown evidence

The fixed-capacity observer captures actual definition/data/module identities and forwards all delete operations to the existing allocator. The current record comes from the existing HCP test friend/native cache lookup. At the intercepted JIT call, the row has already been appended; under serialized ownership the observer copies its last index and the native returned module identity. The source order and later stamped retry IDs establish that association. This is a fixture observation, not a production registration registry.

Each module input differs from the manager's retained IR pointer, preserving native cloning. Failed outputs remain zero; both survivors' independent outputs, handle IDs and resolved task symbols survive the failures, retries and row growth. Subsequent ordinary prepared/dynamic launches write the expected values. Symbols are queried only on live successful modules, and copied function addresses are never called. The initial runtime module is identified independently through the existing non-lazy executor getter.

After all eight HCP definitions die, four cached records remain. Delete observations then confirm that all four data objects die during ordinary Program finalization while all six module wrappers remain live; those wrappers die later with Runtime/session destruction. No compiled-data pointer is dereferenced after finalization. This proves the observed object boundaries, not selective executable unloading, complete dependency coverage or the absence of external raw references. Quiescent/lifetime obligations remain those of the existing native owner.

## Source-only limits inside module creation

[`JITSessionCPU::add_module`](../taichi/taichi/runtime/cpu/jit_cpu.cpp) creates a JITDylib, adds its generator and IR, appends the dylib pointer to `all_libs_`, allocates/stores the JITModule wrapper, then increments `module_counter_`. Wrapper storage is later than several backend side effects. Ordinary allocation failure within this sequence may precede a counted wrapper; the counter increment is also late. Calls using `cantFail` are not an ordinary unchanged refusal protocol. These ordering facts are source-inspected; this fixture does not inject inside that sequence, establish its recovery or measure unlisted dylibs/resources.

The module wrappers borrow dylib/session pointers and have no unloading destructor. The session retains lookup exposure and ends the ORC session during normal teardown. Abandoned creation therefore cannot be treated as an independently released executable just because registration never published a handle. The existing design's LLVM/context, dependency, stale-dispatch, backend-error and service-pointer exclusions still apply.

## Proposed next native boundary, for review

Extend the existing native registration owners with explicit **attempt ownership before any registered retirement**. A compiled record may have several successful/partial attempts; a single handle-to-module pair cannot describe them all. Each attempt must account for its allocated slot, any created dylib/resources and any wrapper, including failure before wrapper publication. Keep native manager, CPU launcher and session as the owners; introduce no HCP cache, execution authority or generic external registry.

The reviewed protocol should publish enough association before each side effect, distinguish pending/failed/usable state and define allocation/publication failure without losing ownership. No successful-handle stamp must imply that earlier attempts disappeared. Keep surviving IDs stable, retain failed slots without reuse, and give inspection separate slot, usable-registration and module/resource completeness semantics. Bound and measure the metadata/work cost and test its allocation/unwind paths before any accounting/admission claim.

Association identities must remain safe across compiled-data finalization and the longer session lifetime; a retained raw data pointer is not a lifetime protocol. Initial runtime modules and cached service pointers remain pinned at the Runtime/session boundary. Guard/client coverage, live Program/context, owner drain and lookup/task-pointer invalidation remain prerequisites for future removal. Resolve failure inside JIT creation, cross-module dependencies and ORC partial-cleanup behavior separately. This proposal selects neither a final field layout nor a cleanup API and enables no registered removal. Return the inspection and proposal for maintainer review.

## Source-pinned local qualification

Actual backend: LLVM/x64 CPU, Release, Clang/LLVM 15.0.7 native substrate, CUDA OFF. Focused wrappers use Clang 15 and GCC 11. The fork was rebuilt for this exact main-based source rather than using the different #406/#407 ABI. Recursive submodules and runtime bitcode SHA-256 `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada` remain unchanged.

- `HCP_LOCAL_BUILD_JOBS=4 bash engine/scripts/run_local_ci.sh quick /tmp/hcp-registration-816b8edb-quick`: matching native/wrapper build/backend, three CTests, 632 placement-math samples and capacity check passed.
- `bash /tmp/hcp-registration-checks.sh clang`: seven CTests passed: all four new worker/dispatch modes and existing prepared-launch, loaded-count and prepared-sharing controls.
- The same command with `gcc`: five CTests passed, including new one-worker prepared/two-worker dynamic cases and the same three controls.
- The same command with `minimal`: warm/viewport OFF, one-worker prepared registration and existing sharing CTests passed.
- `perl /tmp/hcp-registration-audit.pl /tmp/hcp-registration-816b8edb`: all seven registration histories/112 snapshots, fourteen fault windows, seven shutdowns and three unchanged sharing histories/42 count snapshots reconcile.

Each profile brackets exact source/tree/clean checks. The original #405 inspection source `3534439e` is executable-equivalent to reviewed `06e71404` and integrated main `92e4ceaf`; the new source's production diff is empty. Its unchanged real refusal/lifetime evidence is reused, rather than rerun merely to onboard. The sharing control's three current histories/42 native-count snapshots match the verified before logs. The seven new registration cases add 112 native-count snapshots and fourteen injected fault windows, with actual retry/dispatch and seven shutdown observations.

Verified machine-local archive `/tmp/hcp-registration-816b8edb-evidence.tar.gz`, SHA-256 `b8b20252061e2be5e0868adc1cde16d9a67141ef5aa1b02fd8ae33d89d235e7d`, retains raw logs, commands/configurations, source/tree/submodules, binary/runtime hashes, unchanged-production/equivalence proofs, exact source snapshots and the development correction. It embeds verified #405 before evidence (`63a0382ac95bd890a5ae59417347853d0201f63000b56f6be3b0fed4022434b3`). Reused working logs match manifest-verified archived copies. Root and all four profile manifests pass after fresh extraction. This identifies retained contributor evidence without publishing the bundle.

Development initially expected the link wrapper to capture the internal initial-runtime module creation call. That assumption failed before the first snapshot; the fixture now observes its identity through the existing getter. The failed log, exact before source, correction and passing development run are retained. Production was unchanged throughout. Existing shutdown cache-lock warnings remain in raw logs; no persistent store/cache purge, manual dump or reset is used.

Hosted GitHub CI is authoritative for its scope. Full local regression/compiler matrices, GPU, concurrent acquisition/drain, actual registered/ORC cleanup, inner JIT failures, complete resource bytes/B and #324 remain separately qualified work.
