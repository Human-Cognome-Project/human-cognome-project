# Session-owned native source trees

Status: **accepted and integrated in #402 at `456cb681`**, after #400/#401, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). [Decision 7066447453](https://github.com/Human-Cognome-Project/human-cognome-project/pull/402#issuecomment-7066447453) confirms native 44921322375 and smoke 44921322416 passed and main's files match reviewed `2062f00e`. Original tested `e226b76a` implementation/cost evidence below remains unchanged. The approved continuation is the separate [compiler/JIT ownership inspection](COMPILER-JIT-LIFETIMES.md); eviction/accounting changes require review. All merges remain maintainer work.

Canonical tested source `e226b76a630ae072717128cc283cf999f028b4cc`, tree `2f4f36c046ec0721d23f9d07d602ae2d4d483c78`, changes Execution ownership and its native witness. Numerical kernels, fork, public header/operations, source identities, native allocation authority and prepared admission/publication semantics are unchanged. The [accepted before inspection](LEGACY-SUBMISSION-LIFETIMES.md) retains its original `a0ec440b` measurements and zero saving.

## Lifetime and native authority

[`ActiveSession::Execution`](../src/assembly/active_session.cpp) now uses the existing noncopyable `SNodeTreeOwner` as its first member and its borrowed `Drain` as its last member. Both constructors set the existing Program pointer before registration. Both legacy and prepared submit paths capture the returned registered-tree handle immediately. The legacy path continues to own its original ActiveAssembly value; prepared submission retains its original source through the prepared owner.

The former manual teardown body is removed. Reverse member destruction now drains first while native dependencies exist, destroys the reserved prepared owner before its Harness, destroys legacy FixedStudy before its source layout, and retires the source tree through the first guard while Runtime/Program still live. Constructor/operation unwind uses the same established mechanism after successful registration. A failure inside FixedStudy/Harness construction first unwinds those partial objects, with their existing component guards, before Execution retires the source layout. Native registration's own exception handling is unchanged.

Program continues to own native tree descriptors/backing and performs destruction, token retirement and tree-ID reuse. The owner/drain is an execution-local retirement obligation, not a new registry, resource pool, allocator, scheduler or second execution authority. Session still synchronizes and clears all executions before finalizing Runtime. As in #400, the guards use Program-wide barriers at lifecycle boundaries; teardown latency is not benchmarked. Legacy supplied-start/equal-count refresh, prepared v2/f64 entry/tick, owner checks and source-pin boundaries remain intact.

## Actual root and host costs

The matching original experiment warms one aggregate legacy study, refuses two attempts at a primary-step study through real parameter errors, corrects and submits it, and ticks both. All six Clang/GCC profiles agree on the following native source-root records:

| Completed original boundary | Live source roots before → after | Requested source-root bytes before → after | Requested bytes retired earlier |
| --- | ---: | ---: | ---: |
| Warm sibling | 1 → 1 | 4,096 → 4,096 | 0 |
| Negative-parameter refusal | 2 → 1 | 8,192 → 4,096 | 4,096 |
| Nonfinite-parameter refusal | 3 → 1 | 12,288 → 4,096 | 8,192 |
| Corrected retry and following ticks | 4 → 2 | 16,384 → 8,192 | 8,192 |

Each failed candidate's actual source-root token is retired before refusal returns. Both warmed sibling tokens stay live; successful source and Activity tokens survive through following ticks. Source and Activity tree IDs can then be reused by new layouts while every old candidate allocation token remains retired. At the original following-tick boundary, native tree-slot high-water falls from six to four, with unchanged issued SNode IDs (39); tree slots and SNode IDs are different quantities.

The expanded after witness additionally fails Activity kernel construction after native registration, checks absence of the failed execution and both published siblings, retries that study and ticks it. After all three successful Executions are cleared, all twelve captured source/Activity roots are retired before Runtime finalization, including the six source roots. Native tree slots reach six and all six IDs are available; issued SNode IDs reach 59. The extra control has its own provisioning/compilation and is not treated as the original experiment's shorter cumulative workload.

Clang 15's actual record-layout compilation of the before and after translation units measures Execution **1,600 → 1,608 bytes**, an 8-byte host-object increase. The existing sixteen-byte Program/tree-pointer pair is replaced by the sixteen-byte owner plus eight-byte drain reference. No separate guard allocation is introduced. Failed Execution objects release this host storage on unwind; successful objects retain it for their existing session lifetime. This host cost is separate from the native requested-root reductions and does not establish complete B or RSS.

## Pool and compiler boundaries

Through each matching original phase, observed compile calls/unique objects, issued SNode IDs, runtime cumulative requests and pool requested counts are unchanged. Each full legacy Harness still produces thirteen observed compiled-data objects; the two parameter failures grow the observed count from thirteen to thirty-nine, and corrected retry reaches fifty-two. Program's own Kernel vector remains empty. Destroying definitions, contexts and trees does not evict compilation-manager/JIT data.

After the extra partial-constructor control and retry, the after witness has sixty-eight observed compiled objects. They remain readable through their established Program lifetime. Counts are not a compiled-byte measurement or proof of a safe eviction boundary.

For one-worker bulk, pool reserved after the two parameter refusals stays at 1,073,750,600 bytes, versus before values 1,073,754,696 and 1,073,758,792. At the original following-tick boundary it is 1,073,759,472 versus 1,073,767,664 before. These observations overlap root backing and are not added to the source-root reductions. All profile/path pool counts are retained in the raw comparison. After expanded execution teardown, the observed pool still reserves its 1,073,741,824-byte arena despite all captured roots being retired. Cumulative requested counts and reserved high-water are not a disjoint release ledger, complete B or RSS.

## Native failure and ordering evidence

[`legacy_submission_inspection`](../tests/legacy_submission_inspection.cpp) retains all before controls and four Linux GNU/Clang CTests. Its linker-only observers now also trace real synchronization/launch and Harness destruction. Every source retirement follows a completed drain and either completed dependent Harness destruction or the explicitly selected partial-constructor unwind. Normal published Harness destruction begins after a drain, and its source token stays live until the entire real destructor returns. No freed tree pointer or destroyed Runtime/device is queried.

The two parameter exceptions are real validation failures. The added `std::bad_alloc` is injected only by the test linker wrapper during Activity kernel compilation; it exercises real constructor unwind without a production fault switch. It leaves both source and Activity tokens retired, preserves both published siblings' ready state/completed counts/bitwise reports and native backing, then permits the same study to submit and tick successfully. Root-only and primary-step layouts, early/duplicate refusal, owner/generation rejection, detached report lifetime and following ticks remain covered.

The existing prepared session witness and public legacy/prepared/viewport checks qualify the same shared Execution change. Prepared owner still dies before Harness, and the established record/activity/selection retirement and metadata/source-pin release checks remain active.

## Contributor local qualification and handoff

All final qualification uses the clean canonical source/tree above, exact fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, pinned submodules and matching runtime bitcode SHA-256 `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`. Actual backend is LLVM/x64 CPU, Release, warm/viewport enabled and CUDA qualification disabled.

- `bash engine/scripts/run_local_ci.sh quick /tmp/hcp-session-tree-e226b76a-quick`: exit zero; matching fork/wrapper build, backend check, three CTests, 632 independent placement-math samples and placement-capacity case pass.
- `bash /tmp/hcp-session-tree-checks.sh e226b76a630ae072717128cc283cf999f028b4cc clang`: eighteen CTests pass. Four modes each cover legacy lifetime, prepared lifetime, prepared public submission and prepared viewport ownership; legacy workbench and viewport snapshot also pass.
- The same command with `gcc`: ten CTests pass with GCC 11 against the same Clang/LLVM 15 substrate. Each family uses one-worker bulk and two-worker direct, plus workbench/snapshot. This is the recorded GCC subset, not all four modes or hosted parity.
- `perl /tmp/hcp-session-tree-audit.pl /tmp/hcp-legacy-layout-a0ec440b /tmp/hcp-session-tree-e226b76a`: six legacy runs, 72 roots, 78 samples and 36 source retirements, thirty after completed Harness destruction and six through partial-constructor unwind. Exact original-phase root reductions and unchanged compile/runtime/pool-request counters match all six accepted before profiles. Six prepared runs preserve their accepted native phase counters and retire all 324 captured roots before Runtime finalization.
- Clang 15 record-layout compilation measures the actual before/after Execution sizes from source-pinned translation units, using the current matching include/define configuration. It introduces no production size/inspection API. Source-before/after, full compiler output, command and selected layout records are retained.

Verified machine-local archive: `/tmp/hcp-session-tree-e226b76a-evidence.tar.gz`, SHA-256 `49892580cf2e959374bddddf5539154e9727f41316a38e5e483b6add465365c3`. It contains all final profile commands/configurations/toolchains/submodules, clean source/tree before/after checks, unchanged component and local/canonical tree proofs, binaries/runtime hashes, full native logs and cross-profile/before-after audit, record-layout compilation, development build/run logs and scripts. The accepted legacy before bundle (`ac8fea42290e1a895a4d3c8b8db66bb82ad30ebe4cfad5c63809d41bdfe83c59`) and prepared before bundle (`90dd21dd618af0ae25c8fd6bcb617aa7373e5b83aa8d1cc1a035476fb6c1fa45`) are embedded; both their archives and root/profile manifests are verified. Fresh extraction of this final archive verifies its root and all three final profile manifests. This local hash identifies contributor-retained evidence without publishing the raw bundle.

The development build and expanded native witness passed before source pinning; final qualification was run independently on the canonical clean commit. Shutdown's existing cache-lock warnings remain in the raw logs. No retained store, cache or Git history was cleared. Numerical/compiler sources are unchanged, and no IR optimization/threading claim is made; accepted broader/compiler suites were not repeated merely to onboard. Full local regression, database and GPU execution, compiler/JIT bytes and complete B are not qualified here. Hosted native/smoke subsequently passed and this correction is integrated in #402; its original local qualification scope above remains unchanged.

Next: the [compiler/JIT ownership inspection](COMPILER-JIT-LIFETIMES.md) follows #402's explicit continuation decision. Review its findings and bounded native observation proposal before eviction or byte-accounting changes. The original source-tree cleanup and cost evidence remain accepted; complete B, broader adoption/revision policy, GPU and #324 stay separate.

For that inspection, start with [CompiledKernel/Runtime](../src/engine/runtime.h), the fork's [KernelCompilationManager](../taichi/taichi/compilation_manager/kernel_compilation_manager.cpp), [LlvmProgramImpl](../taichi/taichi/runtime/program_impls/llvm/llvm_program.cpp), [LLVMRuntimeExecutor](../taichi/taichi/runtime/llvm/llvm_runtime_executor.cpp) and [JIT session](../taichi/taichi/jit/jit_session.cpp), together with the existing refusal witnesses and accepted fork guidance. Preserve the two before archives and this after evidence; do not rerun qualified baselines merely to onboard.
