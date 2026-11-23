# Checked CPU JIT name issuance

**Updated disposition — 2026-11-22:** [Review 7084634921](https://github.com/Human-Cognome-Project/human-cognome-project/pull/412#issuecomment-7084634921)
provisionally accepts #412 `37170176` and authorizes the
[fresh-ID attribution inspection](CPU-JIT-ATTRIBUTION.md) at `b533ff93`.
#411's decision is now [recorded at 7084632898](https://github.com/Human-Cognome-Project/human-cognome-project/pull/411#issuecomment-7084632898).
Main `6441a740` integrates qualified #408; #410 targets main, with #410 → #411 →
#412 still open for native qualification/integration. The original naming report
below preserves its `14783678` source/cost/evidence and historical state. The new
inspection changes no production association, cleanup or admission behavior.

This bounded #343 implementation follows the naming proposal in the
[CPU JIT ownership inspection](CPU-JIT-CREATION-OWNERSHIP.md). The maintainer's
acceptance and request to proceed in the working conversation authorize this
slice. At the initial direct check, #411 remained open at `0945bca6`, with no new
published review/comment; no merge or changed remote source was inferred.

## Source and behavior

Tested source `14783678792e7ac2e08a8e2270fbcf4ade115840`, tree
`c7bb559a77526578a6d733a0fb13e28b7e16b464`, fork subtree
`9c9e42288b502447a39f4c3c0c8205ebeb45837e`. Parent #411 is
`0945bca68c351be904c01569a25a47eb96c42048`. The four source changes are the CPU
JIT implementation, its checked-ID helper, the existing native witness and
`HCP-DELTA.md`. Later handoff changes are Markdown only.

[`JITSessionCPU::add_module`](../taichi/taichi/runtime/cpu/jit_cpu.cpp) now issues
an ID under its existing mutex **before** formatting or calling
`ExecutionSession::createJITDylib`. The previous increment after wrapper
publication is removed. The existing integer counter becomes an issuance counter;
wrapper counts still come from existing wrapper storage. A failed attempt burns
its issued ID, including failure before a dylib exists. Successful creation keeps
its original numbering. No new persistent field or second registry is introduced.

The internal [`issue_jit_module_id`](../taichi/taichi/runtime/cpu/jit_module_id.h)
helper checks exhaustion before incrementing. Supported IDs are `0` through
`INT_MAX-1`; `INT_MAX` is the exhausted sentinel. Refusal throws `std::overflow_error`
without changing that counter. The real helper is tested at zero, the final
supported value and exhaustion. Reaching exhaustion through billions of
native attempts is not exercised; source ordering establishes that the guard
precedes new backend work. No test manipulates private native session storage.

## Real failure and retry evidence

The existing native fixture now replaces #411's guarded inner retry with an
ordinary retry that reaches LLVM and dispatches. It also adds a pre-creation
allocation-failure boundary. Each history uses three warm range kernels plus the
initial runtime module, then one candidate:

| Candidate boundary before retry | Retained dylibs | Stored wrappers, including runtime | Actual retry |
|---|---:|---:|---|
| Successful control | 5 | 5 | No retry |
| Injected C++ failure before real dylib creation | 4; name `4` absent | 4 | Name `5`, CPU slot 4, five final dylibs/wrappers |
| Real C++ failure after dylib creation, before generator installation | 5; empty name `4` retained | 4 | Name `5`, CPU slot 4, six final dylibs but five wrappers |
| Test exception after real factory return | 5; IR/exposure/wrapper retained | 5 | Name `5`, CPU slot 4, six final dylibs/wrappers |

Both inner retries share the original compiled-record identity and actually write
the expected 64-cell output. Failed CPU slot 3 remains failed, handle-free and
`CreatingModule` / `Incomplete`, without a confirmed module link. Retry slot 4
publishes its own handle/module association. The empty failed dylib stays outside
the actual global lookup order, including after retry. Partial resources are
retained by their existing native owner; name issuance does not clean them up.

Survivor handles 0/1, task addresses, prepared/dynamic outputs and the existing
shared native thread service remain stable. All selected dylibs remain through
compiled-data finalization; candidate record identity becomes expired. Normal
session/Runtime teardown releases the five/six selected dylib requests. There
is no failed-task dispatch, LLVM duplicate-name call, row reuse or manual cleanup.

Seven Clang/GCC/minimal histories cover 28 creation/shutdown windows, fourteen
successful inner retries, seven outer retries and seven direct boundary checks.
The three original pre-retry ownership/exposure states match #411. Its original
empty-dylib case ended at a guarded retry with five dylibs; the new real retry
ends with six. The original source/report/evidence remains pinned at `e3667ef7`.

The seven original registration and attempt histories still match exactly,
including 112 registration snapshots, 14 fault windows, storage costs and
expired-identity controls. Their original source-specific evidence remains
separate from this qualification.

## Ownership cost and limits

The helper adds no persistent field or allocation on successful issuance.
Native virtual interfaces and object layouts are unchanged. The normal path adds the exhaustion check and moves the
existing increment before creation. Rebuild the native CPU archive and relink
matching clients. Native compiled-data/HCP wrapper/guard/control extents remain
those qualified in #410; the original storage/ABI outputs match in this run.
Runtime bitcode remains
`4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`.
Serialized/AOT formats and numerical kernels are unchanged.

Observed dylib object requests remain 272 bytes apiece. Five/six selected objects
account for 1,360/1,632 requested bytes. After the new successful inner retry,
the original failed dylib still accounts for one of those six objects. That
272-byte request is separate from IR, emitted code, trackers, generators and
allocator overhead. This is no whole-owner peak, complete B, RSS, admission
credit or saving claim; successful retry creates its ordinary additional module.

Pre-return observation remains incomplete. Native generator/IR `cantFail` errors,
lookup-vector/wrapper publication allocation errors, materialization failures and
teardown errors are not newly qualified. Resource attribution and cleanup,
registered retirement, shared service/dependency safety, stale dispatch, GPU and
concurrent use require their separate reviews.

## Qualification and handoff

At the clean tested source, quick local CI, 18 Clang CTests, 12 GCC 11 wrapper
CTests and seven warm/viewport-OFF CTests pass against the matching rebuilt
Clang/LLVM 15 CPU SDK. The selected set includes actual JIT retry/dispatch/shutdown,
registration attempts/failures, borrow/unwind/expiration, actual-architecture
provenance, unregistered removal, loaded/native counts, prepared launch and sharing.
No database, GPU or full compiler matrix is claimed by this local subset.
GitHub CI is authoritative and this contribution supplies new hosted qualification.

Archive `/tmp/hcp-jit-names-14783678-evidence.tar.gz`, SHA-256 `62b60e4be66c6891f641401c89ecc87c9b06289e56f7a61ed6856429146c5005`,
contains source/command/toolchain/build/binary pins, clean-state brackets, exact
scope proof, before/after audit, development logs and the manifest-verified #411
comparison archive. It remains machine-local, not remotely published raw evidence.

At the final direct dependency check, main remains `ff82d2cc`; #408 is open with
both hosted gates passed. #410/#411 remain open with smoke passed and native gates
pending. This branch stays stacked on #411 for maintainer review/integration.
Return the naming implementation and source-qualified evidence for review before
selecting broader resource work. Held #75/#111/#123 and separate #324 are unchanged.
All merge events remain maintainer-only.
