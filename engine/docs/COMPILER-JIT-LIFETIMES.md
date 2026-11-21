# Compiler and JIT ownership across session refusal

Status: **contributor inspection awaiting review**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Follows [#402 decision 7066447453](https://github.com/Human-Cognome-Project/human-cognome-project/pull/402#issuecomment-7066447453). Main `456cb681` integrates qualified #400–#402 and has the exact reviewed `2062f00e` tree. This task inspects existing owners and proposes a bounded follow-up; it implements no eviction, resource gate, byte-accounting change or runtime saving. All merges remain maintainer work.

Canonical tested source `fb6143504d9c032a9fdc930ff0bd409ef932755c`, tree `0152cf8f19c73988d6a4b83ecb022a0b0568842c`, changes only the existing two refusal witnesses, a shared test observer and their linker options. Production/fork/numerical code is unchanged. The accepted [source-tree correction](SESSION-SOURCE-TREE-OWNERSHIP.md) retains its own source-pinned cleanup measurements; this report separates the remaining compiler/JIT lifetimes.

## Native owner map

| Object | Actual owner | Relevant lifetime / borrow |
| --- | --- | --- |
| Kernel definition and CHI IR | HCP `CompiledKernel::kernel_`, a unique Kernel | Dies with its HCP component, including refusal/unwind. Program's own `kernels_` vector does not own these definitions. |
| Bound launch context / prepared CPU state | HCP `CompiledKernel::bound_context_` and `LaunchContextBuilder::prepared` | Context dies before its originating Kernel. Origin Kernel/Program, compiled-record and ABI checks govern preparation; these contexts are distinct from registered launcher rows. |
| Cached compiled data and retained LLVM IR | `ProgramImpl::kernel_com_mgr_` → `KernelCompilationManager::caching_kernels_` / loaded `cached_data_.kernels` → unique CompiledKernelData | HCP borrows `const CompiledKernelData*`. LLVM InternalData owns its IR module, tasks and copied ABI metadata; the compiled module's LLVM context and interned type relationships must also remain valid. |
| Registered CPU launch entry | `ProgramImpl::kernel_launcher_` → CPU `contexts_` vector | Owns task function-pointer and parameter vectors. The compiled object stores a launch ID; rows do not disappear when the HCP definition or bound context dies. |
| Kernel JIT wrapper | Runtime executor → JITSession → `modules`, unique JITModuleCPU wrappers | Each wrapper borrows its CPU session and JITDylib. Registration transfers a distinct cloned IR module into ORC; a wrapper does not itself own that LLVM IR object. |
| Executable JIT resources | JITSessionCPU's ORC ExecutionSession / compile and object layers | Session owns JITDylibs and associated executable resources. Its destructor calls `endSession`; this is distinct from cache-entry deletion and subsequent wrapper destruction. No executable-byte census is provided here. |

The relevant source path is [HCP CompiledKernel](../src/engine/runtime.h), [native Program dispatch](../taichi/taichi/program/program.cpp), [ProgramImpl ownership](../taichi/taichi/program/program_impl.h), [compilation manager](../taichi/taichi/compilation_manager/kernel_compilation_manager.cpp), [LLVM compiled-data layout](../taichi/taichi/codegen/llvm/compiled_kernel_data.h), [CPU registration](../taichi/taichi/runtime/cpu/kernel_launcher.cpp), [JITSession](../taichi/taichi/jit/jit_session.h) and [CPU ORC session](../taichi/taichi/runtime/cpu/jit_cpu.cpp).

## Compile, cache hit and registration are separate

HCP constructs a native Kernel directly from a Block. `ir_is_ast()` is false; the compilation manager uses `get_name()` as the cached key and selects **MemCache**, independently of the offline-cache flag. Disabling disk eligibility does not disable the in-memory map. `load_or_compile` first checks that existing map; compiled-data references can therefore be shared by distinct definitions. The observer validates the actual cached name and the existing non-lazy `owns_compiled_kernel` provenance lookup while each definition is alive.

`LLVM::CompiledKernelData::InternalData` retains a compiled LLVM module and ABI/task metadata. CPU `register_llvm_kernel` first checks the data's optional handle. If absent, it allocates a launcher row, clones the retained module, sends the clone through `LlvmRuntimeExecutor::create_jit_module`, resolves task functions, fills the parameter/function vectors and records the launch ID. Repeated registration of that data reuses its existing handle. Preparation may register without a physical launch; compilation alone does not require registration.

The observer checks each transferred IR input is distinct from all observed live cached IR modules. CPU `JITSessionCPU::add_module` moves that IR into an ORC ThreadSafeModule, creates a JITDylib, and retains a separate JITModuleCPU wrapper. The transferred IR's later internal ORC lifetime and executable allocations are not measured by counting wrappers.

## Actual refusal and teardown observations

| Completed legacy boundary | Definitions created / still live | Cached compiled-data objects live | Registered CPU rows / captured kernel JIT wrappers |
| --- | ---: | ---: | ---: |
| Warm sibling | 13 / 13 | 13 | 12 / 12 |
| Negative-parameter refusal | 26 / 13 | 26 | 12 / 12 |
| Nonfinite-parameter refusal | 39 / 13 | 39 | 12 / 12 |
| Corrected retry ready | 52 / 26 | 52 | 18 / 18 |
| Following ticks | 52 / 26 | 52 | 24 / 24 |
| Executions gone, before explicit finalization | 69 / 0 | 68 | 36 / 36 |
| After explicit finalization, before Runtime members die | 69 / 0 | 0 | 36 / 36 |

The final two rows include the extra partial-constructor refusal and corrected third study already present in #402's witness. One failed compile accounts for 69 observed definitions versus 68 successful compile calls/data objects. All six one/two-worker bulk/direct compiler profiles agree on these lifetime counts.

The source/layout/activity roots still retire according to #402; root retirement changes none of these cache/launcher owners. A real parameter refusal destroys its thirteen Kernel definitions while retaining thirteen compiled-data objects and adding **no** registered launch entry or kernel JIT wrapper. Corrected submission registers six entries before its first tick; that tick brings it to twelve. Successful contexts keep their compiled-data borrowers until their component teardown.

Prepared late plan/placement refusal has a different footprint: Harness preparation already registers most compiled data, even when the candidate is never published.

| Completed prepared boundary | Definitions created / still live | Cached compiled-data objects live | Registered CPU rows / captured kernel JIT wrappers |
| --- | ---: | ---: | ---: |
| Warm sibling | 97 / 90 | 88 | 86 / 86 |
| First plan refusal | 194 / 90 | 168 | 164 / 164 |
| Repeated plan refusal | 291 / 90 | 248 | 242 / 242 |
| Placement refusal | 388 / 90 | 328 | 320 / 320 |
| Executions gone, before explicit finalization | 879 / 0 | 749 | 725 / 725 |
| After explicit finalization | 879 / 0 | 0 | 725 / 725 |

Each of the original three late refusals adds eighty compiled-data objects and seventy-eight JIT wrappers/launcher rows while leaving only the sibling's ninety definitions live. The expanded fixture's final totals include its existing three constructor/launch faults, corrected retries and successful studies. It makes 877 successful observed compile calls, returns 749 distinct cached objects, and observes 879 definition instances including two compile faults. This is direct evidence of cached-data sharing and of launch registrations surviving their original definitions. The Clang one-worker bulk and GCC two-worker direct profiles agree; other prepared profiles are retained accepted evidence, not rerun or claimed as new observer qualification.

The complete legacy fixture includes two parameter failures, partial Activity construction and three successful executions. Before finalization, all 69 observed definitions and all native roots are gone, but 68 compiled-data objects and 36 registered kernel JIT wrappers remain. The prepared witness separately exercises late work/placement refusal, constructor/launch faults, native claims, sibling values and corrected retry. These are observed HCP objects, not an enumeration of every compiler/runtime allocation.

## Finalization differs from Runtime destruction

HCP Runtime's destructor explicitly calls `Program::finalize`. Native Program synchronizes, finalizes the executor, runs its existing offline-cache clean/dump path, resets the default configuration and resets the host pool. CPU executor `finalize()` marks finalization; it does **not** destroy the JIT session at that point.

`KernelCompilationManager::dump` discards pending MemCache entries at this boundary. On successful cache locking, only disk-eligible entries move to the temporary serialization map; clearing the pending map destroys HCP's MemCache data. On cache-lock failure it clears the pending map directly. The recorded runs retain the existing lock warnings and exercise that latter branch; the successful-lock branch is source-traced, not newly executed. Loaded disk-cache data has a separate owning map and may remain until manager destruction. No manual dump, forced purge or cache clearing command is used by this inspection.

Immediately after real finalization returns, all observed HCP compiled-data objects have been released, while all registered launcher rows and captured kernel JIT wrappers still exist. The observer reads only copied release flags and the independently live launcher here, never a freed compiled-data pointer. Natural Runtime member destruction then destroys ProgramImpl. LlvmProgramImpl explicitly releases its field-cache data and runtime executor; executor member order destroys the JIT session before its LLVM context. The CPU session ends ORC execution resources before its base module-wrapper vector dies. All observed wrappers release by the end of Runtime destruction. No JIT function or native device is queried after its owner dies.

This ordering also explains why replacing ordinary component cleanup with a call to `dump()` would be wrong: live HCP compiled-data pointers can still exist, and pending-memory clearing does not coordinate launch rows or JIT resources. Existing shutdown is a whole-Program boundary.

## Inspection limits and bounded proposal

[`compiler_lifetime_trace.h`](../tests/compiler_lifetime_trace.h) uses test-only linker wrapping of existing unsized/sized delete, kernel-module creation and explicit finalization. Each delete forwards to the original operator; it only marks copied address records. It observes definitions at existing compile calls and returned compiled-data objects, rather than adding a compiler cache, allocator, global production registry or new native API. Metadata mutations are muted inside the observer to avoid recursive observations.

The wrapper catches kernel-module creation from CPU registration through the static native archives. Runtime's initial JIT module uses an internal same-translation-unit call and is deliberately outside that captured count. Observed wrapper count matches the existing public CPU launcher context count. ORC internals, the initial runtime module, other caches, loaded disk objects not returned through these HCP calls, LLVM arenas/type factories and complete executable memory remain outside the census. Existing `LaunchRegistryStorage::complete` is not promoted to full ownership/byte completeness. No object count, serialized file size or `CacheData::KernelData::size` is treated as retained RAM: the latter is initialized to zero for new memory entries and populated as serialized disk bytes during dump.

There is no native per-entry retirement contract here. CPU contexts grow in a vector, JITSession retains wrappers, `remove_module` is only a commented declaration, and `all_libs_`/ORC dylib state is independent from compiled-data ownership. Prepared contexts validate originating Kernel/Program, exact compiled record and ABI before registering. Evicting by a refused candidate's compilation list would also need to preserve any shared live compiled-data borrowers and all registered task pointers.

**Proposed next bounded change, for review:** add non-lazy read-only ownership/count observations to the existing compilation manager and JIT session, surfaced through their current Program/Runtime owners. Distinguish pending compiled objects, loaded disk metadata versus loaded objects, registered CPU rows, kernel wrappers and the initial runtime module; use existing native identities and absent/present/incomplete statuses. Validate refusal, teardown and finalization without registration or mutation. Report counts/relationships with explicit incompleteness and no new byte charge or gate. Do not add a second registry or implement eviction in that step. Any later reclamation proposal must first define live borrowing, handle invalidation/row reuse, ORC resource retirement, context ordering and supported backends together.

## Contributor local qualification

All checks use the clean canonical source/tree above, exact fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, matching submodules and runtime bitcode SHA-256 `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`. Actual backend is LLVM/x64 CPU, Release, warm/viewport enabled, CUDA qualification disabled.

- `bash engine/scripts/run_local_ci.sh quick /tmp/hcp-compiler-lifetime-fb614350-quick`: passed matching fork/wrapper build, backend check, three CTests, 632 independent placement-math samples and placement-capacity case.
- `bash /tmp/hcp-compiler-lifetime-checks.sh fb6143504d9c032a9fdc930ff0bd409ef932755c clang`: five CTests pass, covering all four legacy worker/path modes and prepared one-worker bulk.
- The same command with `gcc`: three CTests pass, covering legacy one-worker bulk/two-worker direct and prepared two-worker direct using GCC 11 against the same Clang/LLVM 15 substrate. This is the stated subset, not all prepared modes or hosted parity.
- Focused profiles run concurrently on independent wrapper builds after quick CI's single matching fork build; each uses at most two build/native workers and serial CTests on a twenty-CPU host.
- `perl /tmp/hcp-compiler-lifetime-audit.pl /tmp/hcp-compiler-lifetime-fb614350`: eight release summaries pass. Every existing native phase counter except compilation timing matches the exact accepted #402 profile; source roots, live claims, retries and sibling/tick controls are preserved. All captured definitions, compiled data and kernel JIT wrappers release at their respective boundaries. Finalized data is never dereferenced.

Verified machine-local archive: `/tmp/hcp-compiler-lifetime-fb614350-evidence.tar.gz`, SHA-256 `7d5aea8cfdb684499cfb0cea58022d14d2ca9064350fbef1713eab8a748d75af`. It retains exact profile commands/configurations/toolchains/submodules, source/tree before/after and clean checks, unchanged-production/baseline/local-canonical/integrated-reviewed equivalence, binary/runtime hashes, full native/compiler observations, the before/after comparison, owner-source tree pins, development build/run logs and scripts. It embeds verified #402 before evidence, SHA-256 `49892580cf2e959374bddddf5539154e9727f41316a38e5e483b6add465365c3`. Both before and final root/profile manifests verify, including fresh extraction of the final bundle. The hash identifies contributor-retained local evidence without publishing it.

Development observer builds/runs passed before pinning; final qualification was independent on the clean canonical commit. Existing shutdown cache-lock warnings remain in the raw logs. No forced purge, manual dump, persistent-store reset or Git history rewrite was performed. Accepted broad suites were not repeated to onboard. Full local regression, new compiler IR/optimization modes, database/GPU execution, complete compiler/JIT bytes and complete B are not qualified. Hosted GitHub CI remains authoritative for its scope. Review the findings and bounded observation proposal before implementing eviction or changing byte accounting.
