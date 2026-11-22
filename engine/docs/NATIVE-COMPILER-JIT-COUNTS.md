# Native compiler and JIT ownership counts

Status: **qualified and integrated in #404**, [decision 7067817542](https://github.com/Human-Cognome-Project/human-cognome-project/pull/404#issuecomment-7067817542), under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Implements [#403 decision 7067047506](https://github.com/Human-Cognome-Project/human-cognome-project/pull/403#issuecomment-7067047506) from provisionally accepted `a326ca774d319c5899b15592beb3c3a7051011c0`. Main `456cb681` integrates qualified #400–#402. #403's smoke passed and its authoritative native qualification was still running when this contribution began. All merges remain maintainer work.

Maintainer update: #403 passed hosted native/smoke and is integrated at main `ca098539`, with no file differences from reviewed `a326ca77`. #404 passed hosted native/smoke and is integrated at main `87ff545c`, with no differences from reviewed `fbc9bc50`. The authorized [reclamation design/inspection](NATIVE-RECLAMATION-DESIGN.md) follows separately. The original qualification below retains its exact source.

Latest disposition (2026-11-22): #405 is integrated at main `92e4ceaf` with
reviewed `06e71404`'s tree. #406 `33da9a0f` is provisionally accepted in
[decision 7081114329](https://github.com/Human-Cognome-Project/human-cognome-project/pull/406#issuecomment-7081114329):
the provenance finding is resolved, smoke passed and native qualification remains pending.
The authorized [serialized unregistered-record operation](NATIVE-UNREGISTERED-REMOVAL.md)
at `505ab435` now awaits review. It requires the manager's full bounded eligibility
checks; guard observations alone remain insufficient. It adds a sticky CPU
registration-start marker/eight overlapping native bytes, with existing guards,
count semantics and JIT/ORC owners preserved. The following original source/evidence
sections retain their stated revisions; later scope is recorded in the new report.

Canonical tested source `b36dcd28a8bfc0532f6de8c38ab270d5c6605238`, tree `24bac6730087461fabe568c4534e6ce45d6d7bca`, contains the native APIs and both kinds of executable evidence. API commit `2b7e244c2df0d9306e39dd079a2a32d6615b9c57` precedes the standalone positive disk-load fixture. The exact fork tree is `3c1fd8859bfc04b9d1587f3e907478b8a6bc54c4`; rebuild the matching fork archives and HCP wrapper together. Later report/handoff edits are Markdown only and preserve this fork tree.

## Existing owners supply the observations

[`compiler_jit_counts.h`](../taichi/taichi/program/compiler_jit_counts.h) defines value snapshots and `Unavailable`, `Absent`, `Present` and `Incomplete` status. The APIs are const, non-lazy observations under **live, quiescent ownership**. They do not allocate, compile, load disk objects, create owners, register modules, synchronize execution or mutate native state. They introduce no owner fields, virtual methods, production registry or execution authority.

| Query | Native owner and reported scope |
| --- | --- |
| `KernelCompilationManager::ownership_counts(entry_budget)` | Existing pending map entries/non-null compiled objects, disk metadata entries/non-null loaded objects, and inspected entry count. |
| `ProgramImpl::compiler_ownership_counts(entry_budget)` | Existing `kernel_com_mgr_`; returns `Absent` when missing, without calling the lazy manager getter. |
| `JITSession::ownership_counts(runtime_module, module_budget)` | Existing module vector, distinguished by the executor's existing initial runtime-module identity; reports total/runtime/kernel modules and inspected module count. |
| `LlvmRuntimeExecutor::jit_ownership_counts(module_budget)` | Existing `jit_session_` and `runtime_jit_module_`; returns `Absent` when the session is missing. |
| `LlvmProgramImpl::jit_ownership_counts(module_budget)` | Existing runtime executor; returns `Absent` when missing. |

`Unavailable` is the default unqueried value. `Absent` means the requested owner does not yet exist; an existing empty owner instead returns `Present` zero counts. The snapshot's `owner` is a borrowed identity, not ownership or permission to query after destruction. Existing registered CPU row observations remain separate and are reconciled by the witnesses; this patch adds no second launcher count registry.

Budgets bound **inspection work only**. Compiler preflight checks both map sizes with overflow-safe subtraction; a short budget reports those sizes and `Incomplete`, with no scan or object count. JIT preflight likewise rejects a short module budget. A nonempty session without a known runtime identity returns `Incomplete` instead of labeling every module a kernel. A supplied runtime identity must match exactly one existing wrapper; an unmatched identity also returns `Incomplete`. Callers must check status before treating zero object/role fields as measurements. Empty, partially initialized sessions are observable without materializing their initial module.

`Present` validates the selected counts. **`complete` remains false in every snapshot**: borrowers, LLVM arenas/type relationships, ORC executable allocations and other resources are outside this scope. Counts and serialized disk sizes are not retained RAM, complete B, an admission charge or an achieved saving. No eviction, lifecycle policy, numerical kernel, runtime bitcode, AOT schema or existing native class layout changes.

## Executable reconciliation

The existing [lifetime observer](COMPILER-JIT-LIFETIMES.md) still captures actual definitions, cached data, kernel wrappers and their release. Each delete wrapper forwards to the original native operator. A test-only wrapper around the existing JIT-session factory observes its returned session **before** initial-runtime registration: repeated zero-budget queries see an existing empty owner and create no module. A fresh Program's compiler query remains `Absent` even at an unlimited budget.

An isolated manager fixture observes an empty owner and then two synthetic metadata entries with no loaded object. Exact work reports two visits/zero objects; one-short work reports `Incomplete`/zero visits. Observation leaves its owned temporary directory unchanged. Metadata serialization and native constructor reads occur outside the queries.

[`compiler_jit_counts_test`](../tests/compiler_jit_counts_test.cpp) separately provides the positive loaded-object case. It serializes genuine native compiled data to its own temporary `.tic`, creates two metadata entries and supplies a compiler spy that throws if compilation is attempted. The first query reports two metadata entries/zero loaded objects. An **explicit existing native disk load outside observation** produces one loaded object; repeated queries then report one, and short work remains incomplete. The loaded object has no launch handle, repeated explicit loading returns the same object, and the runtime still has its one initial module and no kernel module. Cleanup removes only files/directories created by this fixture. It uses C++ and `engine_support`, with neither warm integration nor viewport required.

Live/refusal/teardown count snapshots agree with the independent release flags and CPU launcher witnesses:

| Boundary | Pending compiled objects | Kernel JIT wrappers | Initial runtime module | Total JIT modules |
| --- | ---: | ---: | ---: | ---: |
| Fresh Runtime, compiler absent | 0 | 0 | 1 | 1 |
| Legacy negative-parameter refusal | 26 | 12 | 1 | 13 |
| Legacy executions gone, before finalization | 68 | 36 | 1 | 37 |
| Legacy after explicit finalization | 0 | 36 | 1 | 37 |
| Prepared first late plan refusal | 168 | 164 | 1 | 165 |
| Prepared executions gone, before finalization | 749 | 725 | 1 | 726 |
| Prepared after explicit finalization | 0 | 725 | 1 | 726 |

Each real fixture repeats exact-budget queries and tries one-short work at its sampled boundaries; repeated observations create no owner/object/module. Nonempty JIT sessions queried without runtime identity report incomplete roles. HCP's Block kernels use the pending memory-only map, with zero loaded disk objects. After finalization, the observer reads copied release flags, the live native manager/session and live launcher; it never dereferences freed compiled data. All captured definitions, data and kernel wrappers release at their respective original boundaries.

## Source-pinned local qualification

Actual backend: LLVM/x64 CPU, Release. Native substrate: Clang/LLVM 15, CUDA qualification disabled; focused wrappers use Clang 15 and GCC 11. Warm/viewport are enabled except in the standalone minimal configuration. Exact submodule revisions and runtime bitcode SHA-256 `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada` are retained. The unchanged runtime bitcode does not replace the required matching native archive/wrapper rebuild.

- `HCP_LOCAL_BUILD_JOBS=4 bash engine/scripts/run_local_ci.sh quick /tmp/hcp-native-counts-b36dcd28-quick`: matching fork/wrapper build, backend, three CTests, 632 placement-math samples and capacity check pass.
- `bash /tmp/hcp-native-counts-checks.sh b36dcd28a8bfc0532f6de8c38ab270d5c6605238 clang`: eleven CTests, covering all four legacy and all four prepared worker/path modes, the positive disk-load fixture, prepared launch and owner backing.
- The same command with `gcc`: seven CTests, covering one-worker bulk/two-worker direct for each refusal fixture and the same three focused controls. This is a stated GCC subset, not the full hosted matrix.
- `bash /tmp/hcp-native-counts-minimal.sh`: warm/viewport OFF, standalone count target builds and its one CTest passes against the matching native substrate.
- `perl /tmp/hcp-native-counts-audit.pl /tmp/hcp-native-counts-b36dcd28`: 198 native count snapshots reconcile with independent lifetime and launcher observations. All twelve existing native phase histories match their exact accepted #402 compiler/profile, except compile timing. All eight overlapping #403 compiler-lifetime histories also match. New counts reconcile at every sampled boundary.

The focused profiles run concurrently on independent wrapper builds after one matching fork build; each has serial CTests and at most two build/native workers. Clean source/tree checks bracket every profile. Accepted before evidence is reused, not rerun to onboard. HCP production `engine/src`, numerical kernels, common/network/tools and workflows are unchanged from #403; native fork changes are the read-only additions described above.

Verified machine-local archive: `/tmp/hcp-native-counts-b36dcd28-evidence.tar.gz`, SHA-256 `6850a166a123c496b9f906372727ccb20aa630da20b73b538549c117e9a1a475`. It retains commands, exact source/tree/submodules/configurations, full CTest/native/lifetime/count logs, development logs, source-equivalence checks and binary/runtime hashes. It embeds the verified #402 native baseline (`49892580cf2e959374bddddf5539154e9727f41316a38e5e483b6add465365c3`) and #403 lifetime baseline (`7d5aea8cfdb684499cfb0cea58022d14d2ca9064350fbef1713eab8a748d75af`). Root and all four profile manifests verify after fresh extraction. The archive/hash identifies contributor-retained evidence and does not publish it.

Existing shutdown cache-lock warnings remain in raw logs. No manual dump, forced cache purge, persistent-store reset or history rewrite is used. Queries are not independently thread-safe against concurrent native mutation. GPU execution, alternate IR/optimization modes, full local regression, full executable memory and complete B are not qualified. New authoritative GitHub CI is separate from these local CPU results.

## Next bounded work

Review this API and its authoritative CI before adopting it elsewhere. For later reclamation, inspect the smallest safe native ownership unit and define shared live compiled-data borrowers, launch-handle invalidation/row reuse, task-pointer lifetime and ORC resource retirement together. The prepared fixture's 877 successful compile calls sharing 749 cached objects already rules out assuming one disposable object per candidate definition. Initial-runtime modules must remain distinct. A design/inspection slice should establish supported backend and context ordering before any eviction implementation. Byte accounting and admission-policy changes need separate review; these count queries authorize neither. GPU and #324 remain independent.
