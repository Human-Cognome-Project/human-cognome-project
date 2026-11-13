# Runtime coverage and the combined-admission boundary

**Program vector implementation (awaiting review, 2026-11-13):** following #317
review5891422350, native Program::lifecycle_storage now reports its object and four
lifecycle vector capacities at the actual owner, without factories or execution changes.
Map/free-ID/pointee coverage stays explicitly unavailable; full coverage remains false.
The source-pinned audit below is historical where it describes this exporter as future.
Required mixed metadata and string-contract directions are accepted; compiler exclusion
remains conditional. See the [export contract](BORROWED-RESOURCES.md#program-lifecycle-object-and-vectors-awaiting-review-2026-11-13).

**Program/compiler follow-up (awaiting review, 2026-11-13):** the
[ownership and scope proposal](PROGRAM-ACCOUNTING-SCOPE.md) refines the remaining
Program rows below. It identifies retained tree graphs, HCP-owned kernels outside the
Program kernel vector, mixed compiler provenance/ABI ownership and distinct launcher
copies. Cache serialization size cannot certify memory. Concrete scope/string proposals
and the next read-only Program exporter are specified; no exclusion is accepted here.

**Thread-pool follow-up (awaiting review, 2026-11-13):** the
[existing pool storage export](BORROWED-RESOURCES.md#existing-thread-pool-storage-awaiting-review-2026-11-13)
adds read-only object and handle-vector capacity evidence to runtime_bookkeeping.
It reuses Taichi's existing owner; no execution/thread machinery changes. Worker counts
are handle counts, not active work. OS stacks and hidden thread allocations remain
unmeasured; full coverage stays false. Remaining Program ownership, string accounting,
compiler/JIT scope and trusted aggregation are still open. Historical audit rows below
remain source-pinned; they are not claims that later bounded exports are absent.

**Launcher follow-up (awaiting review, 2026-11-13):** the
[shared registry capacity export](BORROWED-RESOURCES.md#shared-launcher-registry-capacity-awaiting-review-2026-11-13)
observes existing Taichi ownership without recreating compilation/dispatch. CPU context,
task, parameter/path/shape capacities are exposed with a preflighted parameter budget;
name capacity remains a separate potentially-inline charge. Program/thread ownership,
string accounting policy, trusted aggregation and deferred GPU coverage remain open.
The matrix below remains the pinned audit baseline; compiler/JIT exclusion is not accepted.

**Implementation follow-up (awaiting review, 2026-11-13):** the
[runtime-global request slice](BORROWED-RESOURCES.md#runtime-global-requests-awaiting-review-2026-11-13)
now exports the CPU object/table/temporary/random/result requests through the existing
common reservation interface and canonical pool bridge. The source-pinned matrix below
records the audit baseline, not a claim that those exporters are still absent. GPU
preallocation inspection, trusted aggregation and remaining shared owners stay open.
Compiler/JIT exclusion remains proposed. Next implementation focus is shared launcher
metadata, followed by Program/thread coverage or concrete scope decisions.

Source audit and **proposed policy**, 2026-11-13. Inspected reviewed #312
`9d459141c854e923e5eefd6bac776713cd5bb08d`, native tree
`c2c0c512b5b7aa2ed05e30fbb1c3a9df7b40b7d8`, following
[#312 review 5890043642](https://github.com/Human-Cognome-Project/human-cognome-project/pull/312#pullrequestreview-5890043642).
Main is qualified through #311; #312 is provisionally source-accepted, not merged
at this audit. This document implements no exclusion, ledger or admission gate.

## Claim and units

The accepted design boundary in [Adoption accounting](ADOPTION-ACCOUNTING.md#finite-combined-admission-design-boundary)
permits an explicitly defined, disjoint conservative **accounted-byte** policy for
C+B+H+D(t). C is caller capacity; B must come from the trusted borrowed owner; H
and D are separately tracked domain/bootstrap and selected requests. Caller
snapshot S overlaps existing storage and is not an additional byte charge.

Recommendation: first implement that named accounted-byte policy, preserving each
component's units and coverage. Do not call its mixed capacity/request sum an
all-requested-byte ledger. An all-requested-byte policy needs actual external
request records that the existing capacity censuses do not provide. Neither
policy measures heap usable size, resident pages, total process memory or GPU VRAM.

Existing exclusions in selected D enforcement do not automatically exclude the
same resources from borrowed B. A missing exporter is not an exclusion policy.
No required family may become zero merely because its backend returns unavailable.

## Source-traced coverage

“Blocker” below means a gap in the intended complete borrowed execution claim;
it does not invalidate the narrower exports already reviewed. “Proposed exclusion”
requires review and an explicitly narrower claim before it can remove a blocker.

| Owner/family | Current source and observation | Disposition for B |
|---|---|---|
| Harness numerical/scratch and optional operation owners | [Owner inventory](OWNER-RESOURCES.md), [PreparedStorage](../src/field/prepared_storage.h), [CompiledKernel](../src/engine/runtime.h) describe distinct owners and borrowed contexts. Layout totals are not identity-bearing inventories. | **Blocker:** enumerate all required owners, host capacities/objects and contexts under the Harness lifetime boundary; do not sum scratch estimates and their component buffers twice. |
| Arrays, roots, owned imports, node/list/ambient requests | Common native exports described in [Borrowed resources](BORROWED-RESOURCES.md) supply bounded records and canonical owner/pool bridges. | **Blocker:** assemble and validate required-family coverage and deduplicate aliases; an individual successful query is not a complete Harness census. |
| Prepared invocation metadata | [LaunchContextBuilder](../taichi/taichi/program/launch_context_builder.h) exposes buffers, scalar/array capacities and four binding maps, including shared domains. | Existing partial surfaces; **blocker:** owner/context enumeration, lifetime validation and disjoint aggregation. A binding-map entry budget does not pay for the separate array-backing scan. |
| Runtime host bookkeeping | [RuntimeBookkeeping](../taichi/taichi/program/runtime_metadata.h) and [executor](../taichi/taichi/runtime/llvm/llvm_runtime_executor.h) expose executor base, root manager and three maps. | Existing partial surfaces; count retained descriptors even after backing retirement. `available` is not completeness; each required map must be available and non-overflowing. |
| Native runtime object and fixed tables/buffers | [runtime_initialize](../taichi/taichi/runtime/llvm/runtime_module/runtime.cpp) allocates LLVMRuntime, roots, root sizes, list-manager pointers, node-manager pointers, ambient pointers, ambient request sizes, temporaries and random states. | **Blocker:** export actual owner-bound requests and backing coverage. Ambient request-size metadata is already exposed; do not charge it again. Node/list objects do not cover the pointer tables that reference them. |
| Runtime result and preallocation backing | [materialize_runtime](../taichi/taichi/runtime/llvm/llvm_runtime_executor.cpp) allocates the CPU result buffer from HostMemoryPool; GPU setup places result storage inside the preallocated runtime-object allocation. | **Blocker:** result request and containing allocation/offset identity need explicit coverage. Prepared invocation result buffers are a different family. GPU coverage remains unavailable until implemented and qualified. |
| Shared launch registry | [CPU launcher](../taichi/taichi/runtime/cpu/kernel_launcher.h) owns context-vector capacity, task-function vectors and parameter vectors with nested argument paths. [Registration](../taichi/taichi/runtime/cpu/kernel_launcher.cpp) populates these after JIT creation. | **Blocker:** these are retained execution metadata, not merely compiler scratch. A prepared context's scalar/map report does not cover them. Export through the common native launcher boundary, without HCP CPU casts. |
| Program lifecycle ownership | [Program](../taichi/taichi/program/program.h) owns tree/function/array/argpack/texture containers and accessors; [ProgramImpl](../taichi/taichi/program/program_impl.h) owns compilation and launcher managers. | **Blocker:** separate object/container requests, borrowed pointers and already-counted pointees. No generic `sizeof(Program)` claim can cover nested ownership. Empty unsupported families require explicit known absence. |
| Thread-pool host ownership | [ThreadPool](../taichi/taichi/system/threading.h) has a thread vector and synchronization/task state; the executor owns it separately. | **Blocker:** host object/vector and additional library-owned request coverage or a reviewed scoped exclusion. OS thread stacks/kernel objects are a different metric; that does not erase observable host allocations. |
| Compiler/JIT/profiler infrastructure | Executor LLVM context/JIT session and Program/ProgramImpl compiler/kernel/profiler owners retain additional objects and generated code. | **Proposed exclusion:** shared compilation/code-generation infrastructure from a narrowly named prepared-execution accounted-byte policy. Not accepted here. Mixed containers must be partitioned; execution registry metadata cannot be hidden under “compiler”. Lifetimes remain required even if bytes are excluded. |
| Shared pool bookkeeping and backing | HostMemoryPool exports actual map/allocator requests and raw allocation identities with separate rounded virtual extents. | Count each selected canonical backing and shared metadata domain once. A whole pool and its contained allocations are not disjoint charges. An unrelated pool allocation is not silently attributable to one Harness. |

## Metric exclusions versus unresolved coverage

Heap-internal metadata/usable-size rounding, physical residency, OS kernel objects
and driver-internal storage are outside a native requested-storage/accounted-capacity
claim by definition. This does not establish that they are small, bounded or safe
under a system-memory limit. Rounded virtual mapping extents are separately known
for supported host pools; they are not resident bytes or valid logical view capacity.

Conversely, known counter objects/control blocks, container allocations and retained
host/device buffers cannot be dismissed as “allocator overhead”. They are explicit
requests and belong to the applicable owner unless a reviewed policy excludes that
owner. Current `complete=false` flags must remain false until the declared families
are actually covered; a prose recommendation cannot turn them true.

## Required choices before implementing the sum

1. Bind a versioned scope/units policy to the trusted Harness ledger. Caller input
   cannot supply authoritative B or choose exclusions ad hoc. Declare which optional
   owners are required, known absent or unsupported for this execution profile.
2. Choose canonical storage granularity for native backing. Raw allocation requests,
   rounded extents and contained logical requests are different quantities. Charge
   the chosen backing once; retain contained requests as coverage/alias evidence,
   not additional disjoint bytes. Shared pools spanning runtimes need a shared
   accounting boundary or explicit attribution policy before per-Harness totals add.
3. Bind object/context/runtime and allocator-domain identities to live quiescent
   owners. Deduplicate per object/domain, not per query, pointer field or tree.
   Retained runtime descriptors prove bookkeeping storage only, not live backing.
4. Prepay enumeration, canonical lookup, comparison and ledger construction work.
   Fixed runtime-table reads do not pay for dynamic Harness scans or logarithmic
   pool lookup. Keep byte limits, cumulative allocation work and logical setup work
   independent; no conversion of one exhausted dimension into another.
5. Refuse missing/foreign/expired/overflowing evidence before mutation. If B can
   change during execution, account simultaneous old/new backing rather than adding
   independent peaks. Setup copies, rehashes and owner replacement remain separate
   peak work even when retained-state queries are complete.

## Next bounded implementation and acceptance controls

Start with native runtime-global request exports: LLVMRuntime object, the remaining
fixed pointer/size tables, temporaries, random states and runtime result storage.
Use actual initialization capacities/requests and a common ProgramImpl boundary;
extend existing native reservation mechanisms rather than inspect backend internals
from HCP. Preserve the already-exported ambient metadata subset and resolve each
request to canonical pool/preallocation backing without charging it twice.

Then cover shared launch-registry metadata and Program/thread ownership, or obtain
reviewed, precisely named exclusions for the intended policy. Compiler/JIT exclusion
is a policy decision, not a prerequisite to the next request-export implementation.
Only after these decisions and required-family coverage should the trusted Harness
snapshot and combined gate be integrated.

Controls must distinguish unsupported from known-empty/absent, capacity from active
length, distinct equal-sized allocations from aliases, foreign/retired/reused owners,
and unavailable subfamilies from top-level availability. Preserve exact-budget retry,
no mutation on refusal, checked arithmetic, allocation-free inspection where promised,
partial-construction cleanup, source-pinned CPU evidence and separate GPU qualification.

## Validation of this audit

Source declarations, initialization/registration paths and existing common exports
were read directly at the pin above. No executable code, metric, completeness flag,
admission behavior or accepted policy changed. No native rebuild, model execution,
GPU/database/browser run or duplicate hosted qualification was performed. This is
source-traced evidence and a proposed scope for review, not new execution evidence.
