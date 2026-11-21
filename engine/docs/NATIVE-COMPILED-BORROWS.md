# Native HCP compiled-data borrows

Status: **implementation awaiting review**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Implements [#405 decision 7068182353](https://github.com/Human-Cognome-Project/human-cognome-project/pull/405#issuecomment-7068182353) from provisionally accepted `06e714045d59ee1f4eaf40b4a9011cbdbebc7007`. Main `ca098539` integrates qualified #403. Reviewed #404 `fbc9bc50` passed hosted smoke and [native run 44938429779](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/44938429779); #405's smoke passed and native qualification remains pending. Both dependencies await maintainer integration. All merges remain maintainer work.

Canonical tested source `2708b8b15234a49be555e4fa7f7b5a791e1b34c0`, tree `b7aaeec3c9e70c5a6a4c0c7bc250a6fdb9b4864d`, includes the native/HCP implementation, adapted executable witnesses and fork delta notes. Matching fork tree: `0158f33db6f5def4e1ca74fc658d188b81363723`. CompiledKernelData's native C++ class layout and HCP CompiledKernel's layout change; **rebuild matching native archives and wrappers together**. LLVM runtime bitcode and serialized compiled-data/AOT layouts are unchanged. Later report/handoff edits are Markdown only and preserve this fork tree.

## Existing native ownership, explicit borrows

[`CompiledKernelBorrow`](../taichi/taichi/program/compiled_kernel_borrow.h) is a move-only guard. [`KernelCompilationManager::borrow_or_compile`](../taichi/taichi/compilation_manager/kernel_compilation_manager.cpp) uses the existing cache and returns it only for LLVM/x64 non-AST Block kernels. [`Program::compile_kernel_borrow`](../taichi/taichi/program/program.cpp) preserves the existing compilation timing/profile category. It does not create another cache, launcher, allocator or execution authority.

The actual manager-owned CompiledKernelData retains one shared metadata pointer. Its lazily created metadata records that data/manager identity, the live guard count and a sticky untracked-client flag. Acquiring a guard increments once; moves transfer the guard without incrementing; destruction/replacement decrements once. Cache hits share the existing metadata. Guard observations are non-lazy and allocate nothing.

The original const-reference `Program::compile_kernel` and manager `load_or_compile` APIs remain available. On x64 they permanently mark a returned record as having untracked clients, even if that record also has guards. Unmanaged/cloned data has unavailable metadata and defaults to untracked. Unsupported architectures have no tracked acquisition path; AST/disk loading is explicitly outside this first guard API. All observations report `reclamation_supported=false`: no record is eligible for reclamation in this implementation.

Guards own **metadata only**. The cache continues to own compiled data and its IR; the guard does not extend Program, LLVM context, device or executable lifetime. Before returning a data reference, `get()` checks the live metadata pointer. Data destruction invalidates it and clears the owner identity. A remaining guard can safely observe expired metadata, reject `get()` and later release its metadata without dereferencing the deleted data. Whole-Program shutdown retains its existing behavior; this is not selective eviction or a shutdown lease.

## HCP lifetime and moves

[`CompiledKernel`](../src/engine/runtime.h) retains a native guard on its x64 path. The non-x64 path keeps its existing raw behavior. CPU context creation, preparation and launch validate the guard before accessing native compiled data or its Runtime. The guard is declared before the Kernel/context members so normal destruction releases the bound context and Kernel before the borrow.

Move construction transfers those owners. Explicit move assignment first destroys the destination's old bound context and Kernel, then releases/transfers its borrow and moves the source's remaining members. This avoids the premature guard release that member-order default assignment would cause. Moves leave the source empty; self-move preserves the borrow.

Program/context ownership must remain live and quiescent. External Invocation/launch contexts retain their existing caller obligation to keep their Kernel/Runtime alive; this change does not make them independently owning contexts. Cache mutation, acquire, release and observation must be serialized. Guard counts do not drain launches or prove concurrent reclamation safety. No launch handle, row reuse, ORC removal, admission rule, byte charge or numerical formula changes.

## Executable ownership checks

[`compiled_borrow_test`](../tests/compiled_borrow_test.cpp) verifies the actual cached pointer/manager identity and exact shared guard count. A refused shared candidate releases its guard; corrected retry shares the record and preserves actual sibling dispatch. Move construction, self-move and replacement transfer exactly one guard. Nonallocating release callbacks observe the old bound context and Kernel dying while their native guards are still counted, before assignment releases them.

A test-only wrapper throws after native acquisition but before HCP construction completes, and an enclosing owner separately throws after constructing its HCP member. Both release the acquired guard; retry reuses the retained record and dispatches successfully. Raw Program and direct-manager references set the sticky marker; later tracked acquisition cannot clear it. A clone stays unavailable/untracked. Unsupported architecture and AST/disk borrow requests reject. A native guard retained solely for inspection after normal Runtime shutdown rejects data access and releases its still-live metadata safely; it never queries deleted data.

The existing [sharing witness](NATIVE-RECLAMATION-DESIGN.md) still resolves seven definitions to four records and four registered kernel wrappers, with one separate runtime module. Its prepared/dynamic, one/two-worker paths remain qualified. The real [legacy/prepared refusal witnesses](COMPILER-JIT-LIFETIMES.md) now wrap the tracked compile entry point and preserve their original faults, source-root/claim lifetimes, retries, outputs and following ticks.

At every live-data sample, native tracked borrows equal the independently observed live HCP Kernel definitions; no raw-client marker appears in these HCP fixtures. Legacy warm and each parameter refusal retain thirteen live borrows, corrected retry has twenty-six, and all release before finalization while sixty-eight cached objects remain. Prepared refusal likewise releases candidate guards while keeping its surviving definitions valid. The before/finalization distinction remains: pending data releases during native finalization; kernel wrappers release later with Runtime destruction. The post-finalization sample never dereferences deleted data or queries its borrow metadata.

Prepared warm samples observe ninety guards and eight records with multiple live guards on both compiler profiles. The comparison retains the exact #404/#405 before histories. Existing native/compiler/count fields agree at each repeated phase, except compilation timing. New borrow observations are additional evidence, not an enumeration of all possible external C++ references or complete B.

## Actual ownership overhead

The source-pinned before-size executable uses reviewed #405 `06e71404` and its matching fork/Clang headers. The new native test observes the actual C++ allocation containing a live record's metadata identity, rather than a substitute shape.

| Measured surface on this x64/libstdc++ build | Before | After | Meaning |
| --- | ---: | ---: | --- |
| HCP CompiledKernel object | 32 bytes | 48 bytes | One 16-byte guard, embedded in the existing wrapper. |
| Native CompiledKernelData base | 16 | 32 | One 16-byte shared metadata pointer. |
| Concrete LLVM CompiledKernelData | 144 | 160 | Same 16-byte base increase; do not add both rows. |
| Actual metadata/control allocation per tracked record | None | 48 requested bytes | One shared allocation, retained by the record and any guards. |

Thus a cached LLVM record adds 16 inline bytes plus one observed 48-byte request, and each HCP wrapper adds 16 inline bytes. Guards sharing a record share that allocation; raw x64 records also allocate it when first marked. Default/empty/spare wrapper slots also grow; multiplying only live definitions by sixteen would undercount those slots. These are actual object extents and C++ requested bytes, not allocator/RSS/executable totals or a complete-owner peak census. The guard's extent is already included in the wrapper delta.

## Source-pinned local qualification

Actual backend: LLVM/x64 CPU, Release. Native substrate: Clang/LLVM 15.0.7, CUDA qualification OFF. Wrapper profiles use Clang 15 and GCC 11. Exact submodules and runtime bitcode SHA-256 `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada` are retained; matching native archives/wrappers were rebuilt for the C++ layout change.

- `HCP_LOCAL_BUILD_JOBS=4 bash engine/scripts/run_local_ci.sh quick /tmp/hcp-borrows-2708b8b1-quick`: matching build/backend, three CTests, 632 placement-math samples and capacity check pass.
- `bash /tmp/hcp-borrows-checks.sh clang`: fourteen CTests pass: four legacy worker/path modes, prepared one-worker bulk/two-worker direct, four sharing worker/dispatch modes, and guard/loaded-count/prepared-launch/owner-backing controls.
- The same command with `gcc`: ten CTests pass: one-worker bulk/two-worker direct for each real refusal fixture, one-worker prepared/two-worker dynamic sharing and the same four controls. This is a stated subset, not the full hosted matrix.
- The same command with `minimal`: warm/viewport OFF, guard and standalone one-worker prepared sharing CTests pass.
- `perl /tmp/hcp-borrows-audit.pl /tmp/hcp-borrows-2708b8b1`: Seventeen lifetime histories, 260 native count snapshots and 243 borrow snapshots pass with unchanged before histories. Before native/compiler/count histories match, and borrow probes match independently live definitions. Guard controls/overhead reports pass on all three profiles.

Focused profiles run concurrently on independent wrapper builds after the single matching native rebuild; CTests are serial within each profile. Source/tree/clean checks bracket all qualification. Numerical/field/assembly source, launch handles/rows, ORC and workflows are unchanged. Prior evidence is reused for exact comparisons rather than rerun to onboard.

Verified machine-local archive `/tmp/hcp-borrows-2708b8b1-evidence.tar.gz`, SHA-256 `d6dee187ed0159cd56d91424a3b7bb8468320c417d72bea881fbf06a3c9a043d`, retains full raw logs, exact commands/configurations, source/tree/submodules, binary/runtime hashes, before-size executable/source/recipe, ownership comparisons and development checks. It embeds verified #405 evidence (`63a0382ac95bd890a5ae59417347853d0201f63000b56f6be3b0fed4022434b3`), including the earlier real-refusal evidence. Root and all four profile manifests verify after fresh extraction. The hash identifies retained contributor evidence without publishing it.

Development logs preserve the initial fixture's attempted rebind of a frozen prepared context and the overly broad zero-allocation assertion around a whole compile API hit. The fixture keeps its original bindings; guard operations and native API costs are measured separately. The tracked compile method uses the existing profile category, avoiding extra profile-label requests. Final source-pinned qualification is independent of these development runs. Existing shutdown cache-lock warnings remain; no persistent-store/cache purge or manual dump is used.

## Remaining boundary

This prerequisite tracks HCP guard lifetimes and known raw-reference entry points; it does not prove the disappearance of arbitrary external C++ aliases. Valid derived references must stay within guard/owner lifetimes. No independent thread-safety, in-flight/drain, GPU execution, executable-byte recovery, complete B or full local regression claim is made. Hosted GitHub CI remains authoritative for its scope.

Review the guard/ABI change and authoritative CI before considering any reclamation implementation. The [accepted design](NATIVE-RECLAMATION-DESIGN.md) still requires a separate reviewed operation for selective unregistered-record removal, including serialized eligibility and unknown-client refusal. Loaded-record context ownership remains outside the tracked acquisition path. Registered retirement additionally needs exact native row/module association, stale-dispatch handling and ORC dependency/error proof. Byte accounting, admission changes, row reuse and resource removal remain separate; #324 is unchanged.

After preserving the existing `compile_kernel` profile category, repeated tracked and raw Program API hits each observe three C++ requests/263 bytes in the matched fixture. That includes existing native cache/profile work. Guard-only move, observation and release observe zero requests; cache-hit identity is unchanged and no second metadata allocation is created. Initial allocation and class-layout growth are setup ownership costs, not a saving or new admission charge. A guard retained after owner destruction can extend metadata lifetime only.
