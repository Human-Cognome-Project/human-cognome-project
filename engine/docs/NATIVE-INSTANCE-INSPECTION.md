# Native nested instances, catalogue updates and ownership

Inspection for [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-16. Production source: `cf352531ede06487ed5c6e7f1c489f8f40407c82`.
Executable witness: `f0b40962e43db77738528df356f963dd89b5c759`.
This is an inspection and proposal, not a production representation migration.
It preserves the [refactoring brief](../../docs/engine-refactoring-plan.md),
[operation contract](../../docs/core-operation-readiness-plan.md) and
[system boundaries](../../docs/onboarding/SYSTEM-BOUNDARIES.md).

## Findings and decision boundary

One native compiled layout can hold separate nested runtime instances. A new
SNode subtree per conceptual occurrence, a second host allocator and a parallel
per-constituent owner are not prerequisites for that bounded case. Native indexed
storage does not itself supply HCP composition, source revisions, contextual
relationships, physical participation or stale occurrence protection.

The current HCP framework is a comparison surface, not the required final object
model. The native witness supports investigating a smaller instance binding on
existing native storage before adding lifecycle wrappers. It does not establish
that current flat numerical arrays can simply be replaced by the demonstrated
nested integer layout. That replacement must preserve the existing solver,
placement, exposure, transaction and identity contracts.

Catalogue publication and active adoption already have separate source paths.
The important gap is selective adoption: one current warm bundle/projection
allows one revision per definition identity, not two revisions of that identity
assigned to two simultaneous occurrences. Native value compatibility alone
cannot fix this provenance limitation.

## Four different meanings of structure

| Entity | Actual authority and evidence | Consequence |
| --- | --- | --- |
| SNode layout node | `SNode::insert_children`, `dense`, `pointer`; parent owns child `unique_ptr`s in [snode.cpp](../taichi/taichi/ir/snode.cpp) | A host layout descriptor; its ID is not an HCP occurrence ID. `SNode` copy construction is `TI_NOT_IMPLEMENTED`, not a supported clone API. |
| Compiled layout/tree | `Program::add_snode_tree`, `SNodeTree`, `LlvmProgramImpl::compile_snode_tree_types`, `StructCompilerLLVM::run/generate_types` | A Program owns a root and compiled field types; dense children become arrays of child cell types, pointer containers reference separately allocated cells. One compiled layout supports repeated cells. |
| Runtime storage instance | Index coordinates plus active pointer cells; `Pointer_activate/deactivate`, `NodeManager::allocate/recycle/gc_serial` | `(instance, subassembly, part)` selects distinct storage using the same layout nodes and kernels. Deactivation/reuse does not mint a semantic incarnation. |
| HCP conceptual occurrence | Study UUID plus numeric `OccurrenceId{entry,path}`, exact definition/source pins and validated participation | Must map explicitly to storage. Native parentage alone proves neither ordered composition nor conceptual LoD. Contextual membership is a different relation. |

Sources: [Program tree lifetime](../taichi/taichi/program/program.cpp),
[SNodeTree ownership](../taichi/taichi/struct/snode_tree.cpp),
[LLVM layout compilation](../taichi/taichi/runtime/program_impls/llvm/llvm_program.cpp),
[generated layout types](../taichi/taichi/codegen/llvm/struct_llvm.cpp),
[pointer operations](../taichi/taichi/runtime/llvm/runtime_module/node_pointer.h),
[native allocation/recycling](../taichi/taichi/runtime/llvm/runtime_module/runtime.cpp).

These are upstream-derived Taichi mechanisms in the pinned modified fork, not
a game-engine object-class/attachment API. HCP's prepared allocator accessors,
reservation/report interfaces and ownership tokens are fork extensions; the
`engine_support`, field and assembly layers are HCP C++ clients. The witness
uses ordinary native pointer activation/deactivation and compiler GC, not
`PreparedRecordMap` or a new allocator.

## Focused executable witness

[native_instance_inspection_test.cpp](../tests/native_instance_inspection_test.cpp)
constructs one layout:

```text
root
  pointer(instance, 2)
    dense(subassembly, 2)
      dense(part, 2)
        i32 value
    i32 revision
    i32 attachment
```

Both instances begin with the same reusable construction: two repeated nested
subassemblies, each with two parts. Two native pointer cells hold separate state;
constructing them issues no additional SNode IDs. The source definition is the
single test layout-construction function, not a mock catalogue implementation.
Four compiled kernel definitions write a selected instance, attach a value,
traverse/read, and retire an instance. Launch arguments select instances and
values. The witness checks:

- Independent eight-part state through a native three-index sparse struct-for.
- Value-compatible revision adoption in instance zero only; instance one stays
  on its original integer revision/value. These integers demonstrate storage
  selection, not production ToolRef adoption or structural validation.
- An attachment only on instance zero, with every primary part unchanged. Its
  meaning is explicit typed test metadata; Taichi does not interpret it as an
  HCP contextual edge or add it to composition automatically.
- Detached aggregate observations of 86 and 46 from the part values, retaining
  all parts and coordinates. This is integer observation, not production D7
  rollup, physical mass, exposure migration or coherence mechanics.
- Adding/materializing an unrelated tree, then destroying it, preserves the
  old instance state and usable reader. The unrelated root has a distinct
  allocation token that becomes Retired; the original root remains Live.
- Retiring instance zero removes its parts from sparse traversal while leaving
  instance one intact. Native GC recycles/clears its cell; reactivation at the
  same coordinate holds revision three and no old attachment. The native data
  list remains two elements rather than allocating a third.
- Repeated compatible value/attachment updates and reactivation preserve
  sampled backing, SNode/tree counts and cumulative kernel compilation time.
  Initial allocation and unrelated-tree materialization are distinct phases.

`--emit-ir` writes the four input `.tir` files and opts into the fork's existing
structural, generic kernel and optimized CPU LLVM writers. Use a fresh evidence
directory as cwd. Emitted layout types, indexed accesses, list generation and
GC are inspected alongside the measured results; absence of new compilation
is not inferred merely from equal numerical output. Reusing the same compiled
objects is the claim; cross-process cache equivalence is not tested.

This homogeneous bounded construction needs indexed storage, not cloning or
separate trees per instance. Arbitrary heterogeneous/ragged definitions, deeper
conceptual LoD mappings and changed compiled field types remain unsupported by
this witness. A changed schema requires constructing/compiling appropriate
layout and affected kernels; mutating an already materialized SNode graph is
not a demonstrated update mechanism.

## Catalogue dependency and invalidation trace

`SharedManager::gate` rejects private scope before storage preparation/lookup.
`SharedManager::publish` calls `Toolbox::prepare/publish`; publication commits
tool/bundle revisions and a bundle head. It does not call Program, execution
owners or viewport controls. `acquire` checks the requested current head and
creates an owned lease; `read_pinned` reads the retained immutable revision
through that lease. Releasing a lease does not delete retained definitions.
See [SharedManager](../../kernels/database/warm/shared_manager.cpp) and
[Toolbox](../../kernels/database/warm/toolbox.cpp).

| Required case | Actual current path and smallest supported boundary | Missing support / qualification limit |
| --- | --- | --- |
| Unused catalogue addition | Publish the new shared bundle/definition. Existing acquired bundles, ActiveAssembly copies, native trees and compiled kernels are not implicitly refreshed. No engine compilation is called by publication. | Source-inspected here; no database write or new publication experiment. Continuous discovery/notification is not supplied by this inspection. The native witness separately tests even an unrelated compiled tree addition without invalidating existing execution. |
| Revise a definition used by only one of two instances | Existing pins remain valid. Explicitly acquire the desired revision and assemble an admissible target. Native compatible values can update only one indexed instance without compiling its sibling again, as witnessed. | `Bundle.tools` / projection index use ObjectKey uniqueness; occurrences name ObjectKey, while the selected ToolRef supplies revision. Two revisions of that same ObjectKey in one bundle are not expressible. This needs a reviewed per-instance provenance/assembly change before claiming selective production adoption. |
| Introduce an interaction for only one instance | Active private contextual values enter `ActiveAssembly::revise(LocalInput)` at the owner boundary; shared membership contributions are resolved per occurrence by `membership_fields`. Neither is automatically an ordered construction parent. Existing native typed storage can hold a per-instance attachment value without touching its sibling. | Legacy refresh changes the execution generation and republishes the FixedStudy input; it is not a minimal native per-instance transaction. New field kinds, capacity growth or new kernel logic may need preparation/rebuild. Arbitrary attachment introduction is not implemented by the integer witness. |

[Toolbox](../../kernels/database/warm/toolbox.cpp) preserves immutable pinned
revisions; [bundle validation](../../kernels/database/warm/values.cpp) and
[projection](../src/assembly/letter_projection.cpp) index exact
definitions and checks referenced revisions. The distinction between a bundle
head update, source epoch/policy invalidation, pin reads and explicit adoption
must survive any refactoring. An unchanged pin is not permission to read an
unowned/private source.

Native restrictions are concrete. `LlvmProgramImpl::cache_field` does not update
an existing compiled field descriptor in place. Destroy clears that tree's field
cache; `LlvmRuntimeExecutor::destroy_snode_tree` removes its LLVM tree and root
backing. Program removes cached field accessor kernels and may reuse tree IDs.
Do not launch kernels that reference a destroyed/replaced tree. Unrelated
existing trees are independent in this witness, not globally restarted.

`KernelCompilationManager::make_kernel_key` uses the kernel name for this
non-AST IR path. Distinct newly compiled bindings therefore require distinct
names; equal names are not a safe layout-compatibility test. The AST path's
offline key hashes configuration/IR and SNode metadata, but that is not the
client path exercised here. Sources: [compilation manager](../taichi/taichi/compilation_manager/kernel_compilation_manager.cpp)
and [offline key inputs](../taichi/taichi/analysis/offline_cache_util.cpp).

## Compare identity, lifetime and accounting authorities

| Current HCP state | Native authority already present | HCP semantics / replacement assessment |
| --- | --- | --- |
| Occurrence maps in plans, owner, binding and coordinator | Program owns layout; indices and active cells select storage | Study/source identity and exact numeric occurrence paths still need a validated mapping. Repeated copies of layout/source associations are a candidate for one immutable shared binding plus staged transaction deltas. This witness does not prove every copy redundant; candidate/committed maps currently enable atomic publication. |
| PreparedRecordMap claim/recycle tables | NodeManager owns data/free/recycled lists | The map already borrows this allocator through fork prepared accessors. Potential-address declarations, independently bounded records, context/incarnation and kind are additional HCP semantics. Do not add a second free list. Direct native instance binding may eliminate some indirection, but needs an equivalent bounded capacity and stale-handle witness first. |
| Live/count/generation/handle state | Sparse active bits, pointer activation, tree lifetime | Native coordinates can be reused with no semantic generation. Owner identity, completion generation, exact source pins and recycled-occurrence rejection remain necessary. Host cached counts may be derived, but are not safely removable without preserving bounded preflight and commit consistency. |
| Reservation/work and prepared-capacity ledgers | Native root, NodeManager/ListManager backing, pool and device allocation records | Admission/work ceilings are policy; native requests and backing lifetime are facts. Use existing records as allocation authority, deduplicate shared backing tokens, and keep logical limits separate from bytes. Capacity is not an independently allocated physical pool. |
| Owner teardown / native release | Program/tree/backend/pool own backing | Clearing occurrence metadata or releasing a warm lease is not native retirement. Cell recycle retains data-list chunks; root destruction retires its allocation record. Host SNode/tree descriptors can survive backend retirement until Program slot replacement. Runtime teardown is a further lifetime boundary. |
| Borrowed storage/accounting reports | Fork's root/device/pool tokens and native host-storage/reservation reports | Existing exports already expose useful ownership. Complete graph enumeration, alias deduplication and all-family accounting remain incomplete; do not add per-constituent ledgers to invent a complete total. A Live root record, retained pool reservation, host capacity, requested bytes and RSS are different observations. |

Exact comparison surfaces: [record map](../src/engine/record_map.cpp),
[prepared allocator accessors](../taichi/taichi/runtime/llvm/runtime_module/internal_functions.h),
[exchange owner](../src/assembly/exchange_owner.cpp),
[owner resource inventory](OWNER-RESOURCES.md),
[borrowed resource coverage and limits](BORROWED-RESOURCES.md),
[root backing manager](../taichi/taichi/runtime/llvm/snode_tree_buffer_manager.cpp).
The test samples selected allocator lists and global pool request/reservation
counters; it does not enumerate the complete memory graph or qualify full B.

## Exposure, coherence and controller responsibilities

The current projection retains recipe construction/provenance and all supported
aggregate/byte alternatives. Selected exposure controls physical participation;
navigation is not permission to activate hidden participants or create mass.
`LetterProjection::prepare_exposure` preserves study/occurrence identity while
changing selection; exchange preparation replaces the participating native
claims transactionally. Source revisions and composition are not inferred from
membership or storage depth. The current `ActiveSession::submit` only builds
the bounded root/optional-primary-step scale layout and executes FixedStudy;
it is not a general nested structural instance implementation.

The #349 private PreparedStudyOwner is source-accepted at
`da865d3cc8fbc2bbb42731957ec89cc8c055d42c`, but is not integrated in this base.
It reuses native combined bootstrap, records actual claims and exposes native
completed observations. Its acceptance stands; this inspection adds no wrapper
on top of it and does not migrate the legacy capture producer.

For #324, prior composition is intended to establish a coherence frame and its
mathematical degrees of freedom. Current explicit comparison topology,
contributions and centroid/response rows preserve some parent accumulation;
`LiveResponseInspection` exposes identities and exact fractions. The combined
`LiveIntentInspection` candidate accumulates eligible responses and excludes
contacts. Neither that candidate nor the integer aggregate witness specifies
the mathematical admissible set or how whole-particle contextual response acts
within it. This remains a derivation gap, not a reason to select rigidity,
projection, clipping or a new force law. Structural intent, contextual influence
and actual accepted trajectory must remain distinct.

One actual request trace is [letter_capture.cpp](../../tools/viewport/native/letter_capture.cpp):
caller-supplied options select duration/ticks and runtime settings; the fixture
selects per-entry exposure and English sibling focus. The adapter
publishes/acquires a shared pin, assembles/focuses input, then submits it to
ActiveSession. Session owns native lifetime and calls FixedStudy/Harness for
execution. After a completed tick, `viewport::observe(session,...)` and
`snapshot` export a coherent observation. The separate prepared observer in
[completed_state.cpp](../../tools/viewport/native/completed_state.cpp) validates
generation and complete record handles before synchronized native readback;
it copies completed values, not a new simulation state authority.

Controller choices currently coexist with CLI/fixture construction in capture;
the adapter must not grow into Analyst reasoning or numerical policy. Execution
validation, native allocation/claims, placement and ticking belong inside engine
ownership. Report construction currently repeats logical/native associations;
that is an opportunity to consume an engine-owned immutable binding, with
observation copies outside commits. No speculative public Analyst API is needed.

## Smallest proposed follow-up

First specify a private instance-binding contract against actual native indexed
storage: exact occurrence/source pins, semantic roles, coordinate/claim lifetime
and generation, with immutable provenance shared across readers. Compare it
against PreparedStudyOwner and ExchangeOwner before changing either. Include
the two-revisions/one-ObjectKey refusal as an explicit supported-subset boundary.
Then a separately reviewed implementation can prove one-instance update and
release/reuse with stale bindings rejected, while the other instance, hidden
parts, source pins, completed observations and sampled native backing remain
correct. Remove a proven duplicate map only with that equivalence evidence.

Catalogue provenance changes, controller/producer migration, complete accounting
and #324 mathematics remain separate reviewable changes. This inspection does
not authorize dynamic compiled-layout mutation or automatic migration policy.

## Local evidence

Tested public source `f0b40962e43db77738528df356f963dd89b5c759`, matching
the pinned fork and exact external submodules. Clang/LLVM 15 Release, actual
CPU, no GPU or database writes. The later comparison/handoff changes are
Markdown only; the witness and CMake wiring remain identical to this source.

- `run_local_ci.sh quick`: complete exit 0; all native targets rebuilt; three
  CTests passed, 632 independent placement interval samples passed and
  `placement_capacity` passed.
- Focused CTest: `native_instance_inspection_1` and `_2` passed, 68 checks each,
  1.70 seconds total. A separate one-worker `--emit-ir` execution passed all
  68 checks and retained four input IR files, two structural LLVM modules,
  fourteen generic task modules and four optimized CPU kernel modules.
- Personally inspected/emission-audited `%S1_ch = { [2 x %S2_ch], i32, i32 }`,
  nested `%S2_ch`/`%S3_ch`, the reader's pointer/dense list generation and
  struct-for, writer `Pointer_activate`, release `Pointer_deactivate` and
  compiler-inserted `node_gc`. Optimized reader has no pointer activation call.

| Sample | Native data-list elements | Selected list backing bytes | Runtime requested bytes | SNode IDs issued / Program tree slots |
| --- | --- | --- | --- | --- |
| Two live instances | 2 | 8,781,824 | 17,205,608 | 7 / 1 |
| After unrelated tree add/destroy and first instance retirement | 2 | 8,912,896 | 17,344,872 | 10 / 2 |
| Same coordinate reactivated | 2 | 8,912,896 | 17,344,872 | 10 / 2 |

Both worker profiles produced those counts. Cumulative kernel compilation time
remained constant across launches (0.281024 seconds with one worker, 0.282626
with two). Program tree slots include the retired unrelated tree's retained
host slot, not two live backend roots. The combined middle phase includes
unrelated-tree setup and first native recycle; these samples do not isolate
their individual allocation costs. This is not a prewarmed no-allocation
admission test. Pool reservation and selected list bytes are different measures;
no sum here is full B or physical residency.

Raw evidence: `/tmp/hcp-native-instances-f0b40962-quick` and
`/tmp/hcp-native-instances-f0b40962-witness`, including source/configuration,
binary hashes, command logs, artifact audit and per-file SHA256 manifests.
Archive `/tmp/hcp-native-instances-f0b40962.tar.gz`, SHA256
`9fe003014431010e2902c7be8be541c04856aec8cecbf925ff12deef5fa23be4`.
Machine-local evidence is contributor execution, not remotely available raw
evidence or maintainer acceptance. Full regression/compiler/database matrices,
GPU and native Windows were not run locally for this slice. GitHub CI remains
authoritative; the two new CTests join its automatic regression sharding.
