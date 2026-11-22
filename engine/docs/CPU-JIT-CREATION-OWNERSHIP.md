# Native CPU JIT creation ownership

**Continuation — 2026-11-22:** the working conversation accepts #411's bounded
naming proposal and requests continuation. Direct refs still show #411 open at
`0945bca6`; no new published review/comment was present at the initial check.
The [checked-name implementation](CPU-JIT-NAME-ISSUANCE.md) at `14783678` now
returns real inner retry, bounds and preserved dispatch/shutdown evidence for
review. The original inspection below keeps its `e3667ef7` findings and pins;
its guarded retry is historical, not a new claim of recovered retry at that source.
No cleanup or retirement behavior is added by the naming implementation.

Inspection/proposal under #343, following [#410 review 7083898212](https://github.com/Human-Cognome-Project/human-cognome-project/pull/410#issuecomment-7083898212).
This contribution adds one native C++ witness and CMake registrations. All native
SDK and HCP production is exactly reviewed #410. No cleanup, retirement, row reuse,
new ownership ledger, admission change or achieved saving is implemented.

## Source and state

Tested source `e3667ef796c9fda616869e18dc67cf1de6ef671a`, tree
`38f536abdb9641e8a0537eb0266c66958255cd1a`, fork subtree
`d412616af631a24773b27ff47006090c6756a41b`. Parent #410 is
`b1e29134d3f443f225a74cd5c80a2e22004c2a3e`. The only two source changes are
[`cpu_jit_creation_inspection.cpp`](../tests/cpu_jit_creation_inspection.cpp)
and its CMake registrations. Later handoff files are Markdown only.

At the final state check, main remains `ff82d2cc`, integrating qualified #407.
Resolved #408 `4747d113` remains open; both its smoke and native 45050309075
have passed. #410 is provisionally accepted at `b1e29134`, with smoke passed and
native 45054560768 pending. The current branch is stacked on #410; integration
and all merge events remain maintainer work. A chat acceptance is a review
signal; refs/reviews and relevant handoffs were checked directly.

The [registration-attempt implementation](NATIVE-REGISTRATION-ATTEMPTS.md)
continues to identify issued CPU slots and retain expired record identity, while
marking pre-return JIT effects incomplete. Its source-specific costs and earlier
[registration failures](NATIVE-REGISTRATION-FAILURES.md) remain independently
pinned. This inspection does not qualify registered-resource cleanup.

## Actual native sequence and ownership

The owning implementation is [`JITSessionCPU::add_module`](../taichi/taichi/runtime/cpu/jit_cpu.cpp).
The session mutex serializes this sequence; it does not turn it into a transaction.

| Boundary | Actual owner and persistence | Attribution/limit |
|---|---|---|
| Format current `module_counter_`, create `JITDylib` | LLVM `ExecutionSession` retains the dylib immediately. | The CPU slot already exists, but its observation cannot see a dylib before the factory returns. Counter advancement is later. |
| Install current-process generator | Dylib owns its generator; process/library facilities are shared. | Destroying a wrapper is not library unloading. Failure can precede generator installation. |
| Add cloned module via `IRCompileLayer` | The `ThreadSafeModule` moves into an ORC materialization unit associated with that dylib's default resource tracker. | Original compiled IR stays in the native compilation manager. Accepted IR is not a measurement of emitted object code. |
| Append dylib to `all_libs_` | Session vector stores a non-owning pointer for session-wide lookup. | This can expose accepted IR before any wrapper is constructed or published. |
| Construct `JITModuleCPU` | Temporary unique pointer borrows session/dylib addresses. | Its ordinary destructor contains no ORC resource removal. |
| Append to base `modules` | Existing session wrapper vector becomes the wrapper owner. | Published wrapper counts exclude earlier dylibs/IR/exposure. |
| Increment counter and return | Naming counter advances only after wrapper storage. CPU caller later publishes its task pointers and handle. | Inner exceptions can leave the current dylib name occupied while the counter stays unchanged. |

Default resource-tracker selection is explicit in the installed LLVM 15 `Layer.h`
overload; the native code does not create or retain a separate per-wrapper
tracker. A distinct dylib can be attributed to one captured creation call, but
that does not establish an independently unloadable resource unit.

LLVM's [15.0.7 execution-session source](https://github.com/llvm/llvm-project/blob/llvmorg-15.0.7/llvm/lib/ExecutionEngine/Orc/Core.cpp)
requires unique dylib names and retains them in the session. Its creation path
asserts uniqueness; it is not a duplicate-name `Expected` refusal protocol.
The local package reports LLVM 15.0.7 with assertions OFF. This inspection does
not call creation with a reused name or rely on a particular release-build result.

The generator and IR-add operations are wrapped in `cantFail` in native code.
Those LLVM error results do not define an ordinary recoverable refusal path.
`TI_ASSERT(dylib_expect)` and lookup errors likewise do not consume errors into a
rollback protocol; native logging throws a string. These paths are source-inspected,
not crash/error-injected here. No unchanged-state or cleanup guarantee is inferred.

## Shared runtime and service dependencies

The [`TaichiLLVMContext` linker](../taichi/taichi/runtime/llvm/llvm_context.cpp)
clones needed runtime IR into each linked kernel module and eliminates unused
functions. Equal helper names are not evidence of one shared executable backing.
The original runtime module remains a distinct session wrapper.

At dispatch, the CPU launcher supplies the executor's same `LLVMRuntime` through
`RuntimeContext`. [`cpu_parallel_range_for`](../taichi/taichi/runtime/llvm/runtime_module/runtime.cpp)
uses that runtime's thread-pool and callback pointers. The
[executor initializer](../taichi/taichi/runtime/llvm/llvm_runtime_executor.cpp)
installs the existing `ThreadPool::static_run`, assertion callback and optional
profiler services. Those owners, device allocations, result buffers and LLVM
contexts outlive particular HCP definitions and are not private resources of a
JITModule wrapper. The witness observes actual range dispatch through the same
native thread-pool owner across three distinct surviving kernels and a retry.
It does not enumerate every imported symbol, profiler or external raw client.

`JITModuleCPU::lookup_function` searches its own dylib. Session `lookup` searches
`all_libs_`; it does not establish the complete dependency graph. Cross-module
and process-generator dependencies, materialization errors and in-flight work
need explicit treatment before selective cleanup can be considered.

## Focused native witness

Each history runs three fresh Runtime cases with actual 64-cell CPU range kernels.
It uses fixed observers, existing native query APIs and Linux link wrappers at
session/factory/dylib creation, lookup, thread service and session teardown.
It neither changes LLVM/SDK code nor dispatches a failed candidate.

| Case, immediately after candidate attempt | Dylibs | Accepted candidate symbol | Candidate in actual global lookup order | Stored wrappers, including runtime |
|---|---:|---|---|---:|
| Successful control | 5 | Present | Yes | 5 |
| C++ allocation failure after real dylib creation, before generator installation | 5 | Absent | No | 4 |
| Test exception after real factory return, before CPU caller receives wrapper | 5 | Present | Yes | 5 |

The three warm kernels plus runtime account for the first four modules. The
allocation witness arms the next current-thread C++ request after the real
`createJITDylib` returns. The actual exception unwinds cloned IR and releases the
native mutex; the session-owned empty dylib remains. The CPU record retains
`CreatingModule` / `Incomplete` with no confirmed module. Symbol-flags lookup
checks definition presence without requesting the failed candidate's code address.
The absent-symbol LLVM error is explicitly consumed in the fixture.

A guarded retry records the next native name request, again `4`, then throws
before LLVM creation. It confirms unchanged naming state while preserving LLVM's
unique-name precondition. **It is not a successful inner-failure retry.** After
this window the witness continues existing survivor dispatch and normal shutdown;
it makes no claim that fresh module creation recovered.

After the real factory-return exception, the session already owns the wrapper,
IR and lookup exposure. Ordinary retry uses name `5`, appends CPU slot 4, shares
the same compiled-record identity and actually dispatches. Six dylibs/wrappers
then remain, including the failed attempt's published backend wrapper. This
preserves the distinction between inner partial effects and the accepted outer
failure/retry controls.

All survivor handles 0/1, task addresses and outputs remain stable. Global lookup
of an already warm task captures the actual search-order vector without looking
up or executing a failed task. Existing records expire at finalization, while
all five/six captured dylibs remain live. Native `endSession` removes named
visibility later; selected dylib C++ requests release by Runtime destruction.
Across all profiles, each selected dylib object requests 272 bytes: five/six
objects account for 1,360/1,632 requested bytes, released by shutdown.
These selected object requests do not measure IR, executable sections, allocator
or ORC overhead, whole-owner peak, RSS or complete B.

The initial attempt to intercept LLVM's virtual IR-layer call did not reach that
boundary; its failed witness and subsequent compile repair are retained as
unqualified development evidence. Interior allocation failures after IR acceptance,
lookup-vector growth and wrapper construction/publication remain source-inspected.
The final witness claims only the public boundaries above. No LLVM abort,
materialization-error or teardown-error path is qualified.

## Smallest proposed change, for review

Reserve a checked, unique CPU dylib naming ID **before** the fallible creation
sequence. Reuse the current session-local counter as an issuance counter; the
wrapper count already comes from `modules.size()`. Check exhaustion before
incrementing. Successful creation keeps its existing numbering; failed creation
can leave a gap, and retry receives a fresh name. This needs no new persistent
field or second registry and does not attempt to remove a retained dylib.

The acceptance subset would repeat this witness with a real retry after the
inner allocation failure: old dylib `4` retained, new dylib `5`, a fresh CPU slot,
same compiled record, correct survivor/retry dispatch and unchanged shutdown.
It must preserve pre-return incompleteness and all accepted outer controls.
This is a proposal only; no naming change is implemented here.

Explicit attribution of earlier dylib/tracker/exposure state to the caller's
existing CPU attempt is a later bounded interface question. Any such design must
preserve the runtime/service roles and invalidatable owner lifetime, retain
incomplete observation on backend errors and measure its actual ownership cost.
Cleanup/retirement requires separate review of lookup exposure, dependencies,
materialization, context/service lifetime, failure reporting and stale dispatch.
A wrapper address or count alone supplies none of those guarantees.

## Qualification and evidence

At the clean tested source: quick local CI, 18 Clang CTests, 12 GCC 11 wrapper
CTests and seven warm/viewport-OFF CTests pass against the matching unchanged
Clang/LLVM 15 CPU SDK. Seven new histories cover 21 creation windows/shutdowns,
seven guarded inner retries and seven successful outer retries. All original
seven registration/attempt histories match the #410 evidence, including 112
registration snapshots, storage costs and expired-identity/dispatch controls.
Borrow, removal, actual-architecture, loaded-count, prepared-launch and sharing
checks pass in the selected profiles. No database, GPU, full compiler matrix or
concurrency qualification is claimed for this fixture-only change.

Archive `/tmp/hcp-jit-e3667ef7-evidence.tar.gz`, SHA-256 `d7df1032f72d17374ba3a27742d40f66efe9b2a8e68fb3b67aad5747a0cba65b`, retains
source/tree/submodule pins, commands/toolchains, build/test logs, binary/runtime
hashes, installed LLVM headers and assertion mode, unchanged-production proof,
comparison audit, development failures and the manifest-verified #410 evidence
archive. It is machine-local, not published raw evidence. GitHub CI is
authoritative; the PR supplies new hosted qualification without duplicating
existing runs. Return the inspection and naming proposal for review before
implementation. Held #75/#111/#123 and separate #324 are unchanged.
