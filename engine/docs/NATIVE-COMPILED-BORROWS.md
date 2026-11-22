# Native HCP compiled-data borrows

Status: **provisionally accepted in #406**, under #343; current disposition and original qualification are distinguished below.

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

Canonical tested source `65acfbe473f8d66cacdc6426afe9f8d2ce7ddaed`, tree `74e6f88109e73b7af06380ff56384d4ea07df002`, includes the native/HCP guard implementation, actual-architecture correction, regression and fork delta notes. Matching fork tree: `be80342e40f44bc0955aebb129528e9e7a497c09`. The original guard feature changes native/HCP C++ class layouts; this correction adds no further layout change. Rebuild matching native archives and wrappers together. LLVM runtime bitcode and serialized compiled-data/AOT layouts are unchanged. Later report/handoff edits are Markdown only and preserve this fork tree.

## Existing native ownership, explicit borrows

[`CompiledKernelBorrow`](../taichi/taichi/program/compiled_kernel_borrow.h) is a move-only guard. [`KernelCompilationManager::borrow_or_compile`](../taichi/taichi/compilation_manager/kernel_compilation_manager.cpp) uses the existing cache and returns it only for LLVM/x64 non-AST Block kernels. [`Program::compile_kernel_borrow`](../taichi/taichi/program/program.cpp) preserves the existing compilation timing/profile category. It does not create another cache, launcher, allocator or execution authority.

The actual manager-owned CompiledKernelData retains one shared metadata pointer. Its lazily created metadata records that data/manager identity, the live guard count and a sticky untracked-client flag. Acquiring a guard increments once; moves transfer the guard without incrementing; destruction/replacement decrements once. Cache hits share the existing metadata. Guard observations are non-lazy and allocate nothing.

The original const-reference `Program::compile_kernel` and manager `load_or_compile` APIs remain available. They permanently mark raw exposure of an actual returned x64 record, regardless of the requested architecture, even if that record also has guards. Tracked acquisition validates the returned record's actual architecture before issuing a guard. Unmanaged/cloned data has unavailable metadata and defaults to untracked. Unsupported architectures have no tracked acquisition path; AST/disk loading is explicitly outside this first guard API. All observations report `reclamation_supported=false`: no record was eligible for reclamation at that prerequisite source.

Guards own **metadata only**. The cache continues to own compiled data and its IR; the guard does not extend Program, LLVM context, device or executable lifetime. Before returning a data reference, `get()` checks the live metadata pointer. Data destruction invalidates it and clears the owner identity. A remaining guard can safely observe expired metadata, reject `get()` and later release its metadata without dereferencing the deleted data. Whole-Program shutdown retains its existing behavior; this is not selective eviction or a shutdown lease.

## Actual architecture correction — 2026-11-22

[#406 review 7068740186](https://github.com/Human-Cognome-Project/human-cognome-project/pull/406#issuecomment-7068740186) identified that the memory-cache leg returns an existing name-keyed object without comparing request architecture. The original raw marker used `compile_config.arch`, so a copied CUDA-named config could return the same tracked x64 record without marking its exposure. Original configuration-only rejection tests missed this case.

Raw marking now uses `data.arch()`. Tracked acquisition retains its x64/Block input checks and additionally refuses an unsupported returned record before allocating or issuing borrow metadata. The native key/lookup behavior and raw return are preserved; the returned object's architecture governs provenance.

[`compiled_borrow_provenance_test`](../tests/compiled_borrow_provenance_test.cpp) first creates real tracked x64 data, then requests that exact cached key through the raw manager API with a CUDA-named config. The exact same record becomes permanently marked; tracked reacquisition preserves the marker, cache/JIT counts remain unchanged, and actual CPU sibling dispatch succeeds.

A second, metadata-only compiler fixture seeds a native manager with an unsupported-architecture record, then requests the same cached key with an x64 tracked config. Acquisition refuses, the compiler call count stays one, and no borrow metadata is created. The fixture performs no GPU code generation, registration or driver execution.

Both regression assertions fail on reviewed production `9cbce926` with the new test, then pass after this correction. The expected failure, empty production diff, source/test/patch, build commands and corrected qualification are retained. The earlier head's hosted pass did not cover these new assertions.

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

After preserving the existing `compile_kernel` profile category, repeated tracked and raw Program API hits each observe three C++ requests/263 bytes in the matched fixture. That includes existing native cache/profile work. Guard-only move, observation and release observe zero requests; cache-hit identity is unchanged and no second metadata allocation is created. Initial allocation and class-layout growth are setup ownership costs, not a saving or new admission charge. A guard retained after owner destruction can extend metadata lifetime only.

## Source-pinned local qualification

Actual backend: LLVM/x64 CPU, Release. Native substrate: Clang/LLVM 15.0.7, CUDA qualification OFF. Wrapper profiles use Clang 15 and GCC 11. Exact submodules and runtime bitcode SHA-256 `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada` are retained; matching native archives/wrappers were rebuilt for the C++ layout change.

- `HCP_LOCAL_BUILD_JOBS=4 bash engine/scripts/run_local_ci.sh quick /tmp/hcp-provenance-65acfbe4-quick`: matching build/backend, three CTests, 632 placement-math samples and capacity check pass.
- `bash /tmp/hcp-provenance-checks.sh clang`: fifteen CTests pass: four legacy worker/path modes, prepared one-worker bulk/two-worker direct, four sharing worker/dispatch modes, and guard/provenance/loaded-count/prepared-launch/owner-backing controls.
- The same command with `gcc`: eleven CTests pass: one-worker bulk/two-worker direct for each real refusal fixture, one-worker prepared/two-worker dynamic sharing and the same five controls. This is a stated subset, not the full hosted matrix.
- The same command with `minimal`: warm/viewport OFF, guard, provenance and standalone one-worker prepared sharing CTests pass.
- `perl /tmp/hcp-provenance-audit.pl /tmp/hcp-provenance-65acfbe4`: Seventeen lifetime histories, 260 native-count snapshots, 243 borrow snapshots and six provenance assertions across three profiles pass. Normal before histories, guard controls and overhead reports remain preserved. The two new provenance assertions pass on Clang, GCC and the minimal configuration.

Focused profiles run concurrently on independent wrapper builds after the single matching native rebuild; CTests are serial within each profile. Source/tree/clean checks bracket all qualification. Only the compilation-manager correction, fork delta note, CMake registration and new regression differ from reviewed `9cbce926`; numerical/field/assembly, guard/move/unwind, launch handles/rows, ORC and workflows are unchanged. Prior evidence is reused for exact comparisons rather than rerun to onboard.

Verified machine-local archive `/tmp/hcp-provenance-65acfbe4-evidence.tar.gz`, SHA-256 `d2b3f3dc8373e31313e15c680158145f52e970a291f4871b4b7299adbde6ea30`, retains raw logs, exact commands/configurations, source/tree/submodules, binary/runtime hashes, expected regression failure and corrected observations. It embeds verified original #406 evidence (`d6dee187ed0159cd56d91424a3b7bb8468320c417d72bea881fbf06a3c9a043d`), including actual before-size evidence and the earlier lifetime histories. Root and all four profile manifests verify after fresh extraction. The hash identifies retained contributor evidence without publishing it.

The original before bundle preserves earlier guard-development fixture/allocation assertions and their corrections. This correction additionally retains the expected failing provenance regression against unchanged reviewed production, then independent clean-source passing qualification. Existing shutdown cache-lock warnings remain; no persistent-store/cache purge or manual dump is used.

## Remaining boundary

This prerequisite tracks HCP guard lifetimes and known raw-reference entry points; it does not prove the disappearance of arbitrary external C++ aliases. Valid derived references must stay within guard/owner lifetimes. No independent thread-safety, in-flight/drain, GPU execution, executable-byte recovery, complete B or full local regression claim is made. Hosted GitHub CI remains authoritative for its scope.

The maintainer provisionally accepted this prerequisite and approved the separately reviewed unregistered-record operation linked above. The [accepted design](NATIVE-RECLAMATION-DESIGN.md) still requires a separate reviewed operation for selective unregistered-record removal, including serialized eligibility and unknown-client refusal. Loaded-record context ownership remains outside the tracked acquisition path. Registered retirement additionally needs exact native row/module association, stale-dispatch handling and ORC dependency/error proof. Byte accounting, admission changes, row reuse and resource removal remain separate; #324 is unchanged.
