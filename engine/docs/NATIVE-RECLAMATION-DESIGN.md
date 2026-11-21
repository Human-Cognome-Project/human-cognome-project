# Native reclamation units, borrowers and launch resources

Status: **provisionally accepted in #405**, [decision 7068182353](https://github.com/Human-Cognome-Project/human-cognome-project/pull/405#issuecomment-7068182353), under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Follows [#404 decision 7067817542](https://github.com/Human-Cognome-Project/human-cognome-project/pull/404#issuecomment-7067817542) from provisionally accepted `fbc9bc503b33f74e800c80bcb68861bf938aaab3`. Current main `ca098539` integrates qualified #403 with its reviewed tree. #404 targets main; its smoke passed and authoritative native qualification remained pending at this handoff. All merges remain maintainer work.

Maintainer update: reviewed #404 passed hosted native/smoke; #405 is provisionally accepted with its native gate pending. Its authorized [native HCP borrower prerequisite](NATIVE-COMPILED-BORROWS.md) is implemented separately, with metadata-only guards and no reclamation. The original design/inspection evidence below retains its source.

Canonical tested source `3534439e7b26c55df78ebd514380cdb8d3c2fa11`, tree `13f4e93a146ca69599e8c51081e76eb0d3424260`, adds only a native inspection fixture and its CMake registrations. HCP production, numerical code, the existing refusal witnesses and fork tree `3c1fd8859bfc04b9d1587f3e907478b8a6bc54c4` are unchanged from #404. Later report/handoff changes are Markdown only. No eviction, borrower API, launch invalidation, ORC removal, byte charge, admission change or achieved saving is implemented.

## Finding and proposed boundary

**Whole-Program/Runtime shutdown is the currently implemented release boundary for these combined resources. There is no supported per-record retirement operation.** A dead candidate definition is not a sufficient retirement unit. The smallest proposed unit depends on what has happened to its actual native cached record:

| Record state | Smallest proposed unit | Required proof before any future retirement |
| --- | --- | --- |
| Compiled, never registered | One manager-owned CompiledKernelData and its retained IR/task/ABI metadata. | Complete tracked borrower coverage, no unknown raw-reference escape, no in-flight use, correct LLVM-context lifetime. No launch handle may exist. |
| Registered CPU kernel | The shared compiled record, its native launch row, all that row's task pointers, and the kernel JIT wrapper/JITDylib resources associated with registration. | All of the above, plus checked launch invalidation, no incoming executable dependency, exact module association, backend cleanup/error handling and stable surviving rows. |
| Initial runtime module / runtime service pointers | Runtime/session boundary. | Excluded from per-kernel reclamation. Existing runtime identity and service dependencies must remain alive. |

These are **proposed eligibility/ownership units**, not newly available APIs or a proof that current callers have no remaining references. A compiled record can serve several definitions. One registered module can contain multiple offloaded tasks, so a task function is not an independently owned native unit here. Erasing a cache entry does not retire a launcher row or unload executable code.

## Source ownership and handle trace

| Surface | Actual source and consequence |
| --- | --- |
| HCP borrower | [`CompiledKernel`](../src/engine/runtime.h) owns `kernel_` and `bound_context_`, while `compiled_` is a raw borrowed pointer. Member destruction drops the bound context before the Kernel definition. `Invocation` may also own a separate context; callers must keep its Kernel/Runtime alive. There is no compiled-record lease or reference count. |
| Native cache | [`Program::compile_kernel`](../taichi/taichi/program/program.cpp) returns `const CompiledKernelData&` from the existing [compilation manager](../taichi/taichi/compilation_manager/kernel_compilation_manager.cpp). For HCP Block kernels, the name is the memory-cache key. A cache hit returns the same object, not one owned copy per requesting definition. Disk metadata and loaded objects have a separate map/lifetime. |
| Compiled object | [`CompiledKernelData`](../taichi/taichi/codegen/compiled_kernel_data.h) owns an optional integer launch handle; there is no public reset/retirement protocol. [LLVM InternalData](../taichi/taichi/codegen/llvm/compiled_kernel_data.h) owns retained IR/tasks and copied ABI metadata. Newly compiled modules use the compiler's context; loaded modules parse into the object's own `llvm_ctx_`. Context/type relationships must be checked for the exact record path. |
| CPU registration | [`register_llvm_kernel`](../taichi/taichi/runtime/cpu/kernel_launcher.cpp) reuses an existing data handle. Otherwise it gets the next ID, resizes `contexts_`, clones the retained IR, creates a JIT module, resolves every task and stores raw function pointers plus copied parameters in that row, then sets the data handle. The CPU row does not retain a checked compiled-record/module ownership link. |
| Dispatch and preparation | [LLVM launcher checks](../taichi/taichi/runtime/llvm/kernel_launcher.cpp) validate origin/runtime/cache provenance and full ABI before preparation. Prepared state stores executor identity, integer launch ID and `usable`. CPU dispatch indexes `contexts_` by ID and calls its task pointers. It has no row incarnation, active/tombstone check or free-list retirement protocol. |
| ORC | [`JITSessionCPU`](../taichi/taichi/runtime/cpu/jit_cpu.cpp) creates a distinct JITDylib per added module, adds the cloned IR through the compile layer and stores both `all_libs_` raw pointers and unique JITModuleCPU wrappers. A wrapper borrows session/dylib and has no unloading destructor. The ExecutionSession/object/compile layers own the executable resources. |

Two reuse cases must remain distinct. Another definition of the **same cached record** reuses that record's existing handle. A **different record** gets a new append-only row even after the old definition is gone. Compacting `contexts_` would renumber surviving handles. Reassigning an integer slot without an incarnation check could let an old prepared state pass the current executor/ID check and reach another record's tasks. Existing origin/ABI checks do not establish safe slot reuse after retirement.

LLVM 15.0.7's [IR layer](https://github.com/llvm/llvm-project/blob/llvmorg-15.0.7/llvm/include/llvm/ExecutionEngine/Orc/Layer.h) attaches `add(JITDylib&, ...)` to that dylib's default resource tracker. The fork retains no explicit per-kernel ResourceTracker. LLVM's [Core implementation](https://github.com/llvm/llvm-project/blob/llvmorg-15.0.7/llvm/lib/ExecutionEngine/Orc/Core.cpp) distinguishes explicit resource removal from tracker destruction: destroying a non-defunct tracker transfers its resources to the default tracker. [`removeJITDylib`](https://github.com/llvm/llvm-project/blob/llvmorg-15.0.7/llvm/include/llvm/ExecutionEngine/Orc/Core.h) closes a dylib and clears resources/references, with errors returned to its caller; static destructors are not run by that API. The current CPU destructor instead calls `endSession` for the whole session. These source facts identify candidate mechanisms, not a tested HCP per-kernel removal path.

The existing `all_libs_` search and each wrapper's dylib pointer would need coordinated invalidation before closing a dylib. Native LLVM errors can occur after resource mutation; an implementation cannot assume every error is an unchanged ordinary refusal. The simple witness below does not audit all imported/exported symbols or prove the absence of dependencies between arbitrary compiled modules.

Loaded objects need a separate context proof: [`LLVMCompiledKernel::clone`](../taichi/taichi/codegen/llvm/codegen_llvm.cpp) calls `llvm::CloneModule`, while CPU registration supplies the executor's ThreadSafeContext when handing the clone to ORC. The positive disk fixture did not register or unload its loaded object. No loaded-record-to-JIT context lifetime guarantee is established by that fixture or this inspection.

## New executable sharing witness

[`native_reclamation_inspection`](../tests/native_reclamation_inspection.cpp) constructs actual HCP CompiledKernel wrappers with identical i32-store bodies for a repeated key, alongside an independent sibling key. The existing test friend reads the actual definition/data identities; no production owner state changes. It reuses #403/#404's delete-forwarding/module/finalization observer and count APIs.

| Completed boundary | Definitions created / live | Distinct cached records live | Registered CPU rows / kernel wrappers |
| --- | ---: | ---: | ---: |
| Two warmed survivors | 2 / 2 | 2 | 2 / 2 |
| Shared candidate prepared, before injected refusal | 3 / 3 | 2 | 2 / 2 |
| Shared candidate destroyed | 3 / 2 | 2 | 2 / 2 |
| Distinct unregistered candidate compiled | 4 / 3 | 3 | 2 / 2 |
| Unregistered candidate destroyed | 4 / 2 | 3 | 2 / 2 |
| Corrected retry registered | 5 / 3 | 3 | 3 / 3 |
| Retry definition destroyed | 5 / 2 | 3 | 3 / 3 |
| Same record revisited | 6 / 3 | 3 | 3 / 3 |
| Different record registered | 7 / 3 | 4 | 4 / 4 |
| All definitions gone | 7 / 0 | 4 | 4 / 4 |
| After explicit Program finalization | 7 / 0 | 0 | 4 / 4 |

The shared candidate has a different live Kernel definition but exactly the warmed record pointer and row ID 0. Preparing its context adds no row/module. Its injected refusal occurs before dispatch, destroys that context/definition and leaves both survivors' existing outputs unchanged. A separate candidate is refused after compile but before preparation: its data remains cached with no handle. Retry returns that same object and first registers row 2. After the retry definition dies, another definition of that record reuses row 2. A different record then appends row 3; it does not reuse row 2. Seven successful compile API returns resolve to four distinct records.

The fixture snapshots task addresses through the captured live modules' symbol lookups, using the stored task names. Those addresses and both surviving handles remain unchanged across candidate teardown and row growth; actual subsequent HCP launches write the correct independent arrays/scalars. Private CPU row pointers are not exposed or enumerated by this test. The registration source establishes that rows store those resolved functions, while normal dispatch verifies surviving behavior. No function is called through a copied raw address.

Both **prepared** and ordinary **dynamic** dispatch run, with one/two CPU workers. There is one distinct initial runtime module throughout: total JIT count at the final live boundary is five, not four. Finalization releases all four observed pending data objects; kernel wrappers release later during Runtime destruction. The final sample never dereferences freed compiled data. The witness's two refusals are test-injected ownership boundaries, not additional claims about production admission failures or actual eviction. Zero observed definitions does not prove zero external borrowers: even this test retains non-owning observation identities while the native owner remains alive.

## Reconcile with the real refusal/sibling evidence

The [accepted count contribution](NATIVE-COMPILER-JIT-COUNTS.md), tested at `b36dcd28`, already ran all four legacy and prepared worker/path modes on Clang and the stated GCC subset. Its twelve native histories matched #402, eight overlapping lifetime histories matched #403, and 198 count snapshots reconciled. That evidence is retained and reused here: `b36dcd28` → reviewed `fbc9bc50` is executable-equivalent, and this inspection changes neither production/fork nor either real refusal witness/shared observer.

Real legacy parameter refusal retains thirteen compiled objects and adds no JIT wrapper; prepared late refusal adds eighty objects and seventy-eight wrappers. The prepared fixture returns 749 distinct cached records from 877 successful compile API calls. Its survivors, retries, following ticks and source-root retirement stay under their original evidence. These totals do not identify which individual records are borrowed by a surviving sibling, and they cannot authorize per-candidate deletion. The new controlled fixture demonstrates exact shared pointer/handle identity and surviving dispatch without assigning unsupported per-record provenance to the old aggregate histories.

## Proposed implementation sequence, for review

**First bounded follow-up: native borrower protection only, with no eviction.** Extend the existing compilation-manager record boundary with an explicit scoped borrow token and native record identity, rather than adding an HCP cache/registry. Cover acquire, cache hit, construction failure, moves and release. A token must protect both pending and any explicitly supported loaded-object path. Start with CPU HCP memory-cache records; keep disk-loaded records and unsupported/untracked clients pinned until their paths are reviewed.

The current const-reference API cannot prove reference disappearance. Preserve it by marking records with escaped/untracked references ineligible for selective reclamation, unless a reviewed caller audit supplies complete coverage. Counts alone, including `complete=false` snapshots, cannot substitute for that proof. Direct manager callers, native Program/Kernel clients, clones and external launch contexts must be considered; an HCP-only count of definitions is insufficient. The reviewed token design must validate the live owner/record before any pointer dereference, not detect invalidity after dereferencing freed data.

In HCP, the existing CompiledKernel owner should retain the native borrow until its prepared/bound contexts and Kernel definition have died. Preserve Runtime-before-dependent destruction requirements and the accepted execution-owner drain. External Invocation/context lifetimes remain explicit caller obligations. Moving a wrapper must transfer exactly one borrow; constructor/unwind failure must release it. Native ownership metadata, setup work and host overhead require actual measurement in that implementation; no byte charge is selected here.

Only after those prerequisites and review should selective **unregistered** pending-memory removal be considered: under serialized, quiescent ownership, refuse unknown/live borrowers, a present launch handle or concurrent/in-flight use, then destroy just that record while its LLVM context is alive. Candidate inspection must have an explicit entry/work limit; incomplete coverage cannot establish eligibility. Do not dump/purge the manager. Do not treat an unloaded disk metadata row as a loaded object or invalidate `updated_data_` entry pointers by casually erasing metadata. This narrower path still needs its own implementation review and tests.

**Registered CPU retirement is a later, larger operation.** Its proposed contract must include all of the following before code is enabled:

1. Associate the actual native compiled record, row and JIT wrapper/dylib during registration, including partial-registration failure. The current order allocates an ID/row before JIT/task resolution completes; rollback/poison handling cannot be assumed. Keep the association in existing native owners, with no second execution authority.
2. Preflight tracked borrower completeness, exact provenance, initial-runtime exclusion, module dependencies and backend support. Serialize borrow acquisition/registration against retirement; drain outstanding launches through the existing execution owner. Locking a JIT container alone does not stop a thread executing an already resolved task.
3. Make dispatch/preparation reject an inactive or stale row **before** reading its task pointers. Preserve surviving IDs. Prefer tombstones without slot reuse in an initial registered-retirement design; no vector compaction/renumbering. Any later slot reuse requires a native owner/incarnation-bearing handle and matching prepared-state validation on every path, with overflow/exhaustion rules. Existing integer IDs are insufficient.
4. Remove the exact dylib from native lookup exposure under the session's existing synchronization, then use a reviewed ORC removal operation and check its result. Preserve the initial runtime dylib and cached runtime service pointers. Audit imported/exported symbols and applicable host object/EH-frame handling first. Raw function pointers must never remain dispatchable after executable removal.
5. After successful backend release, destroy detached task/parameter storage, the wrapper and eligible compiled data in the required context order. Do not infer that nulling/deleting a wrapper releases ORC resources. If cleanup partially mutates resources and returns an error, retain diagnostic/ownership state and refuse affected execution until a reviewed recovery/shutdown path; do not report an unchanged resource rejection.
6. Keep inspection truthful. Tombstones would make `contexts_.size()` a slot count rather than an active registration count; freed nested vectors do not necessarily reduce outer capacity. Module/row observations and their completeness semantics must be updated together. No prospective reduction is charged to B, treated as admission credit or reported as achieved saving by this design.

This sequence proposes review boundaries, not approval to implement all steps at once. Borrow protection must precede reclamation. A caller that can still use an old compiled pointer/handle prevents destruction, even if its original candidate has failed.

## Backend and teardown limits

| Backend / boundary | Established support and limit |
| --- | --- |
| Linux x64 CPU, LLVM 15 | Actual sharing, prepared/dynamic dispatch and shutdown witnesses pass. ORC supplies explicit removal mechanisms, but the fork has no enabled per-record retirement path. Only this backend is proposed for the first borrower work. |
| Other CPU hosts/object layers | CPU source selects a different object layer on Apple arm64. No new execution or executable-removal qualification here. |
| CUDA | Source inspection only. Launcher rows retain JIT-module pointers/task descriptors; JITModuleCUDA caches function pointers around a driver module handle. Its wrapper has no unloading destructor; the CUDAContext unload loop is commented out. CPU wrapper deletion is not a GPU unload design. Existing CUDA guard/hardware obligations remain. |
| AMDGPU | Source inspection only. Rows retain JIT-module pointers; the module wrapper stores a driver handle without an unload destructor. No retirement or hardware qualification is inferred. |
| Other backends | Outside this LLVM inspection; unsupported for the proposal. |

Existing whole-Program ordering remains: HCP drains and destroys dependent contexts/definitions first; Program finalization synchronizes and clears pending HCP cached data through its existing cache path; Runtime member destruction later ends the CPU ORC session. LlvmProgramImpl clears its field cache before releasing the executor, and executor members destroy the JIT session before its LLVM context. Loaded compiled data has its own parsing context and separate lifetime. The witness qualifies observed HCP memory-cache data and kernel wrappers, not every LLVM arena, disk object, executable allocation or runtime service.

Before a future retirement implementation can be accepted, add exact shared-borrow release order/last-borrow tests; refused and successful retries with a surviving sibling; unregistered versus registered/loaded/initial-runtime refusal; repeated retirement and handle/slot ABA controls; dynamic and prepared stale dispatch rejection; in-flight/drain/concurrent acquisition controls; partial registration and ORC cleanup failures; corrected reuse; and measured ownership overhead. Preserve existing full origin/ABI refusal, native publication and following-tick witnesses. No test should execute a dangling task or deliberately dereference freed compiled data to demonstrate rejection.

## Source-pinned local evidence

Actual backend is LLVM/x64 CPU, Release, Clang/LLVM 15.0.7 native substrate, CUDA qualification OFF. Focused wrappers use Clang 15 and GCC 11. Submodule revisions and runtime bitcode SHA-256 `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada` are unchanged and retained.

- `HCP_LOCAL_BUILD_JOBS=4 bash engine/scripts/run_local_ci.sh quick /tmp/hcp-reclamation-3534439e-quick`: matching native/wrapper build, backend, three CTests, 632 placement-math samples and capacity check pass.
- `bash /tmp/hcp-reclamation-checks.sh clang`: seven CTests pass: all four new worker/dispatch modes, the existing loaded-count fixture, prepared launch and owner backing.
- The same command with `gcc`: five CTests pass: one-worker prepared/two-worker dynamic plus the same three existing controls, against the matching Clang/LLVM substrate.
- The same command with `minimal`: warm/viewport OFF, the standalone inspection target builds and its one-worker prepared CTest passes.
- `perl /tmp/hcp-reclamation-audit.pl /tmp/hcp-reclamation-3534439e`: Seven sharing cases and 98 native count snapshots pass. Exact source/tree/clean checks bracket all profiles; source equivalence preserves the reused real-refusal evidence.

Verified machine-local archive `/tmp/hcp-reclamation-3534439e-evidence.tar.gz`, SHA-256 `63a0382ac95bd890a5ae59417347853d0201f63000b56f6be3b0fed4022434b3`, retains raw logs, commands/configurations, binaries/runtime hashes, submodules, source equivalence, development checks and installed LLVM ORC headers/version. It embeds verified #404 before evidence, SHA-256 `6850a166a123c496b9f906372727ccb20aa630da20b73b538549c117e9a1a475`, including the original real-refusal histories and their comparison. Root and all four profile manifests verify after fresh extraction. The hash identifies retained contributor evidence without publishing it.

Development prepared/dynamic runs pass; existing shutdown cache-lock warnings remain in raw logs. No persistent cache/store purge, manual dump or Git history rewrite is used. Full local regression, GPU execution, executable-byte recovery, actual resource removal, concurrent safety and complete B are unqualified. New hosted GitHub CI is authoritative for its scope. Review this source-pinned design before enabling any eviction, byte accounting or admission change; the recommended next bounded prerequisite is native borrower protection without reclamation. #324 remains separate.
