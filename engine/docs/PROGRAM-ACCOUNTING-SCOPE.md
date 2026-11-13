# Program ownership and accounting scope

**Ndarray host follow-up (awaiting review, 2026-11-13):** the
[native wrapper report](BORROWED-RESOURCES.md#ndarray-host-wrapper-storage-awaiting-review-2026-11-13)
adds object identity/size and shape/total-shape vector capacities at the existing Ndarray
owner. Wrapper metadata is separate from backing, including after backing retirement.
Debug strings/shared types and aggregate pointee coverage remain incomplete; this fixed
query does not enumerate Program/Harness owners. Earlier audit descriptions are pinned
history where superseded. Compiler exclusions remain conditional.

**Field/accessor follow-up (awaiting review, 2026-11-13):** the
[two remaining map exporters](BORROWED-RESOURCES.md#field-expression-and-accessor-map-requests-awaiting-review-2026-11-13)
complete direct Program map node/bucket observation through existing native owners.
Aggregate map availability now checks all five reports; it does not certify nested
strings, shared pointees/control blocks or full Program storage. Native ownership and
execution remain unchanged apart from disclosed accounting overhead. Earlier missing-map
descriptions below are pinned history where superseded; compiler exclusions stay conditional.

**Free-ID follow-up (awaiting review, 2026-11-13):** the
[native deque request export](BORROWED-RESOURCES.md#free-id-deque-requests-awaiting-review-2026-11-13)
observes Program's existing stack through shared allocator requests, including rebound
internal pointer storage. No copy/pop is used for inspection and LIFO semantics remain
native. Free-ID container availability is now explicit; field/accessor maps, pointees
and full coverage remain incomplete. Earlier descriptions below are pinned history
where superseded. Compiler exclusions remain conditional.

**Program registry follow-up (awaiting review, 2026-11-13):** the
[three-registry request export](BORROWED-RESOURCES.md#program-registry-requests-awaiting-review-2026-11-13)
reuses native shared counting allocators for Program function/ndarray/argpack maps.
Node/bucket requests and counter/control-block storage are separate from pointees and
external key strings. Field/accessor maps, free-ID storage and nested ownership remain
unavailable; aggregate map/full coverage remains false. Earlier audit/export descriptions
below are pinned history where superseded. Compiler exclusion remains conditional.

**Program vector implementation (awaiting review, 2026-11-13):** following #317
review5891422350, native Program::lifecycle_storage now reports its object and four
lifecycle vector capacities at the actual owner, without factories or execution changes.
Map/free-ID/pointee coverage stays explicitly unavailable; full coverage remains false.
The source-pinned audit below is historical where it describes this exporter as future.
Required mixed metadata and string-contract directions are accepted; compiler exclusion
remains conditional. See the [export contract](BORROWED-RESOURCES.md#program-lifecycle-object-and-vectors-awaiting-review-2026-11-13).

**Source audit and proposal, awaiting review, 2026-11-13.** Follows
[#316 review 5891228994](https://github.com/Human-Cognome-Project/human-cognome-project/pull/316#pullrequestreview-5891228994).
Inspected source `055d4d3ae160d7815c33d08ae310ae9689b2e459`, which integrates
qualified main `e8b503d7` through #314 with reviewed #315/#316. Native tree:
`32de5736fc1aa974fdd8fe4454177b33f96f5f09`. No executable behavior, exclusion,
completeness flag or admission policy changes here. This refines the remaining
[coverage audit](RUNTIME-COVERAGE.md); it does not reopen accepted exports.

## Existing ownership facilities and missing observations

| Owning source | Existing behavior | Accounting consequence / missing observation |
|---|---|---|
| [Program](../taichi/taichi/program/program.h) | Embeds CompileConfig, container control objects and the accessor bank; separately owns ProgramImpl and optional profiler. | Report Program object once. Embedded member object sizes are subsets, not additional charges. Dynamic config strings, derived implementation storage and pointees need separate coverage. `sizeof(ProgramImpl)` cannot cover a derived implementation. |
| [Program lifecycle](../taichi/taichi/program/program.cpp) | Owns kernel/function/tree/texture vectors and ndarray/argpack maps. Function map borrows Function pointers from the function vector. | Vector capacity is distinct from pointee bytes; map node/bucket requests are distinct from mapped pointees. Function map aliases must not duplicate Function objects. Empty maps can retain buckets; zero entries is not zero storage. |
| Program tree destruction and reuse | `destroy_snode_tree` removes accessor entries and destroys backend backing, then pushes an ID onto `free_snode_tree_ids_`. It does not erase the owning tree-vector slot. `add_snode_tree` replaces that slot on reuse. | Retired backing is not disappearance of the retained SNodeTree/SNode graph. Slot count is not live-tree count. The default stack uses deque storage: `size()*sizeof(int)` misses retained blocks/map storage. Inspection must not pop/copy the stack to enumerate it. |
| [SNodeTree](../taichi/taichi/struct/snode_tree.h), [SNode field map](../taichi/taichi/program/snode_expr_utils.h), [accessor bank](../taichi/taichi/program/snode_rw_accessors_bank.h) | Tree owns its root graph; field map holds shared FieldExpression owners; accessor rows borrow Kernel pointers. | Root device backing does not cover the host graph. Shared expression/control-block ownership needs identity-based treatment. Accessor map storage is owned, its Kernel references are aliases. Removing an accessor row does not erase a Program-owned Kernel. |
| [HCP CompiledKernel](../src/engine/runtime.cpp) | Creates its own unique Kernel directly, then borrows CompiledKernelData from Program compilation. It does not insert that Kernel into `Program::kernels`. | A Program-vector census cannot certify all Harness kernels. Enumerate HCP-owned kernels at their actual owner and deduplicate only true aliases; keep their Program/compiler lifetimes. |
| [ProgramImpl factories](../taichi/taichi/program/program_impl.cpp) | Lazily creates the compilation manager and launcher. `dump_cache_data_to_disk` also obtains the manager through its factory. | New observation must inspect existing ownership without calling factories, dumping cache, compiling or registering. Existing `owns_compiled_kernel` and launcher inspection already demonstrate noncreating queries. |
| [KernelCompilationManager](../taichi/taichi/compilation_manager/kernel_compilation_manager.h) and [implementation](../taichi/taichi/compilation_manager/kernel_compilation_manager.cpp) | Owns pending/cached maps, CompiledKernelData pointees, compiler and strings; updated-data vector borrows map-row pointers. `owns_compiled_kernel` checks cached key and exact data pointer in both maps. | This is mixed ownership, not wholly disposable compiler scratch. Retain existing provenance authority. Charge map/vector storage separately from pointees; updated-data pointers do not own another copy. `KernelData::size` is populated from serialized file position during dump, initially zero for newly compiled data; it is not retained-memory evidence. |
| [LLVM CompiledKernelData](../taichi/taichi/codegen/llvm/compiled_kernel_data.h) and [preparation](../taichi/taichi/runtime/llvm/kernel_launcher.cpp) | Owns argument/return metadata alongside LLVM context/module data. Preparation validates exact provenance and ABI against the originating Kernel/context before registration. | ABI/provenance metadata cannot be silently excluded with compiler internals. Types are borrowed identities; their shared TypeFactory is not owned per parameter. A compiled launch handle is not an independent lifetime authority. |
| [CPU registration](../taichi/taichi/runtime/cpu/kernel_launcher.cpp) | Clones compiled module data, copies parameter metadata, resolves JIT functions and stores separate launcher context vectors. | Compiled argument metadata and launcher parameter metadata are distinct retained copies, even when values match. Do not deduplicate by equal names, shapes or signature. Existing launcher exports cover only their own copy. |
| [Executor](../taichi/taichi/runtime/llvm/llvm_runtime_executor.h), [LLVM context](../taichi/taichi/runtime/llvm/llvm_context.h), [JIT session](../taichi/taichi/jit/jit_session.h) | Own context/session, module owners and per-thread compilation state; launcher task pointers borrow generated functions. | Keep native ownership and code lifetimes. Module/function identity does not supply code allocation bytes. Do not create a parallel JIT registry or replace compilation/thread facilities to account them. |
| [Profiler factory](../taichi/taichi/program/kernel_profiler.cpp) | Returns null when disabled, otherwise a backend profiler with retained observations. | Null is known absence; a present uninspected profiler is unavailable coverage, not zero. Inspect the actual owner rather than assuming the setting proves absence. |

## Proposed decisions for review

### 1. Keep observable ownership metadata in the required scope

Recommend a versioned **prepared-execution accounted-storage** policy under
[the accepted combined-admission boundary](ADOPTION-ACCOUNTING.md#finite-combined-admission-design-boundary).
Include Program/implementation object storage, retained lifecycle containers,
execution/provenance/ABI metadata, selected native backing, launcher storage and
the existing thread-pool object/handle storage. Compiler naming is not grounds
to omit mixed metadata. Count whole object storage once even where an embedded
member belongs to an otherwise excluded subsystem; do not subtract guessed sizes.

The Program-wide report describes a shared runtime owner. Charge it once at the
shared runtime accounting boundary. A per-Harness report may reference that owner
but cannot independently add the whole shared charge for every Harness. Do not
infer a selected subset from pointer reachability or discard unrelated retained
Program rows without a separately reviewed attribution rule. Shared global pool
backing still needs the existing canonical/domain treatment; Program ownership
does not establish exclusive ownership of the global pool.

### 2. Narrow the proposed compiler exclusion to implementation payloads

Recommend excluding LLVM IR/context internals, generated executable-code storage,
backend compiler/JIT implementation internals, transient compilation work and
shared TypeFactory internals from this explicitly named accounting policy. This
is a **proposal**, not a completeness certificate or a system-memory bound.
Retain the owners and provenance dependencies even if their payload bytes are
excluded. Keep separately visible outer objects/containers and ABI metadata in
scope; avoid charging an embedded subobject twice. The actual native owner must
partition mixed payloads before a family can report coverage.

This exclusion does not cover execution registries, runtime tables, pool maps,
array/tree backing, parameter metadata or known allocator counter/control-block
requests. Profiler state and optional argpack/texture families are not automatically
excluded: known absence can satisfy the initial profile; present unsupported
families must refuse a full aggregate. Thread stacks/kernel resources remain
outside this metric, but hidden user-space thread allocations need an explicit
scope decision too. None become accepted exclusions by this document's presence.

### 3. Do not add the current string surcharge to a disjoint total

`name_capacity_charge_bytes` currently sums capacity plus terminator even when
storage may be inside the already-counted parameter row. Preserve it as diagnostic
evidence until a native string-storage contract distinguishes contained storage
from an external buffer. Short names, including empty strings, are not proof of
inline storage; long-name heuristics and implementation-specific fixed thresholds
are not an acceptable portable classifier.

Recommend a separate native result with contained/external/unavailable status and
checked external accounted capacity. A supported implementation must demonstrate
its containment classification and disjoint external extent under the qualified
host standard library; otherwise return unavailable. Do not change HCP model
code by vendor or guess an address-layout rule. If such inspection cannot provide
the promised evidence, use allocation observation at the native string owner
boundary in a separately reviewed change. Capacity remains accounted capacity,
not an actual allocation request. An all-requested-byte policy requires measured
requests instead. No string-type replacement is authorized by this proposal.

Before consuming these fields, add the review-requested empty, nonempty short,
long/external, retained-capacity and nonempty element-shape fixtures. Check repeated
equal values in distinct owners, exact/short work budgets, overflow refusal and
allocation-free inspection. Keep separate compiled/launcher copies separate.

## Next bounded implementation

Start with read-only Program lifecycle metadata: object identity/size and checked
kernel/function/tree/texture vector capacities, followed by owned map and free-ID
container request observation using existing native accounting mechanisms. Locate
the query at the actual owner; do not move Program containers into ProgramImpl
solely for inspection. Expose explicit per-family availability, not one flag that
silently treats missing map/deque/pointee coverage as zero. Do not call lazy factories.

Keep count and storage evidence separate from live-object/backing evidence.
One/two-tree creation, destruction and slot reuse must preserve the difference
between retained host metadata and retired device backing. Array deletion may be
deferred by `used_in_kernel`; test actual ownership rather than assuming an erase.
Empty and populated containers, independent equal-sized owners, allocation-free
queries and checked arithmetic are required controls. No full Program/B claim
until the nested graph, maps, strings and optional families have explicit coverage
or accepted exclusions. This first exporter can proceed before exclusion approval.

Then cover mixed compiler provenance/ABI metadata without replacing its existing
lookup authority, qualify the string policy and complete Harness owner enumeration.
Only after reviewed scope, lifetime, required-family coverage, canonical/domain
deduplication and prepaid work should C+B+H+D be enforced. Fixed metadata reads do
not prepay nested graph traversal, key comparison, provenance lookup or aggregation.
Compilation/registration/setup peaks remain separate from retained-state inspection.

## Evidence limits

This audit traces declarations, lifecycle paths, compilation/provenance checks,
registration copies and cache serialization at the pinned source. It adds no
executable code and runs no model, native rebuild, GPU, database or browser workload.
Earlier CPU/hosted evidence remains earlier evidence. The scope recommendations
require review; accepted exporters retain their existing incomplete-coverage flags.
