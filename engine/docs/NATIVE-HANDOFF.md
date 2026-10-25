# C2a native same-type handoff investigation

Status: **implementation/findings for review** under accepted [#188](../../docs/live-hierarchy-handoff-plan.md),
following C1 #187. This adds a native C++ diagnostic executable and two CPU tests.
Production engine, runtime/compiler fork, numerical laws and live bindings are
unchanged. [Retained evidence](../tests/native_handoff/README.md) identifies the
exact tested source, backend, compiler and measurement boundaries.

## Findings

Existing native activity cannot safely be extended by simply notifying parents
from C1's centroid propagation traversal. The executable reproduces three
deterministic negative controls:

| Control | Observed failure in the deliberately unsafe sequence |
|---|---|
| Sleeping parent admitted into propagation | Child 3 calculated; parent 4 did not. Parent 4 enters the live-bit traversal, consumes its new notification, and leaves a stale value behind a quiet mask. |
| Already-active parent consumed after notification | Parent reads the old child, child publishes/notifies, then parent consumes. The newer child is stranded with no pending native bit. |
| Partial in-place publication | An observer reads `(2,2,3,4)`, neither complete old `(1,2,3,4)` nor complete new `(2,4,6,8)`. Atomic activity alone does not make four data words coherent. |

These are controls, not bugs installed in the production C1 path. Native bit
activation can admit a later peer into the same traversal, as the earlier
[native activity test](../tests/native_activity_test.cpp) already demonstrated.
Container lists are not frozen snapshots of individual activity bits.

The positive probe demonstrates a **CPU-qualified protocol using existing
native functions**, one NativeSet and two preallocated numerical publication
slots per record. It requires no fork extension, lock, spin, scheduling epoch,
second awake mask, custom queue or host/per-depth solve. The slots contain one
published packet and one inactive write destination; they are not two physical
simulations. Their selector addresses numerical data and does not select work.

## Tested protocol and lifetime

Each native struct-for poll may visit a record at most once. For an entered
record:

1. Consume its existing native activity bit at entry, before reading inputs.
2. Acquire each child's current publication selector using native
   `atomic_or(selector, 0)` and its returned value. Read all four components
   from that selected slot. A child may supply the prior or current poll's
   complete publication; no fixed visitation order is required.
3. Calculate the probe result. If it changes, write the entire inactive slot.
4. Publish that slot through native `atomic_xor(selector, 1)` with sequentially
   consistent ordering, then activate the parent through NativeSet.
5. Do not consume this record again at the end of the poll or in a later
   propagation-only traversal. A later notification must remain pending.

Each record has one writer and at most one publication per poll. Thus a reader
that acquired the old selector can keep reading its old slot while the writer
fills the other slot; a reader of the new selector sees completed values. The
old slot is not reused until the next poll, after all current readers complete.
The test synchronizes ordered dispatch completion at that boundary. These
lifetime constraints are essential: two slots alone do not protect arbitrary
repeated/concurrent writers, overlapping polls or concurrent structural edits.

Under these ordering/lifetime conditions, notification before consumption means
the subsequent acquire observes that publication or a newer complete one.
Notification after consumption leaves the native bit
pending, whether or not the current evaluation already saw the new data. A
peer newly admitted later in this traversal performs the full consume/read/
calculation/publication operation; it is never a propagation-only consumer.
Incidental same-poll progress is allowed, and a notification after visitation
carries to a following ordinary poll. Duplicate activations collapse naturally.

The controls explicitly exercise publication on both sides of the consume/read
cut. Stress also covers both relative record orders, same mask word, adjacent
and distant pointer blocks, fan-in and nested dependencies. The fixture checks
at-most-one visit per poll, complete packets, delivered final input, quiet state
and later perturbation/reuse. Success is not inferred from quiet bits alone.

## Native source and compiled-code checks

- [IRBuilder](../taichi/taichi/ir/ir_builder.cpp) supplies atomic OR/XOR and the
  returned prior value; no custom atomic or memory manager is introduced.
- [LLVM lowering](../taichi/taichi/codegen/llvm/codegen_llvm.cpp),
  `integral_type_atomic`, emits sequentially consistent atomic RMW operations.
  CPU [runtime atomics](../taichi/taichi/runtime/llvm/runtime_module/atomic.h)
  used by native bitmask operations also request sequential consistency.
- [Atomic demotion](../taichi/taichi/transforms/demote_atomics.cpp) can turn
  frontend atomics into ordinary operations for serial/loop-unique accesses.
  An atomic requested in source is therefore not an unconditional publication
  guarantee. The generic probe kernel reads own and indirect child selectors
  through the same argument. Its actual compiled LLVM module is inspected;
  tests require retained selector-read and selector-publication RMW sites with
  sequentially consistent ordering. Raw instructions are retained with results.
- [Struct-for lowering](../taichi/taichi/transforms/offload.cpp) and
  [LLVM loop guards](../taichi/taichi/codegen/llvm/codegen_llvm.cpp) explain
  live-bit admission. Prepared pointer blocks/list high-water marks avoid
  ordinary allocation on newly notified rows.

The audit inspects the compiled module retained by Program, before the launcher
JIT's executable copy. Native execution on the matched LLVM15 CPU backend is
separate evidence. Future compiler transformations, specialized/split kernels
or another backend must preserve the ordering/lifetime contract rather than
assuming this kernel's qualification transfers automatically. CPU observations
do not establish CUDA execution or memory-ordering qualification.

## Probe, resources and cost

[native_handoff_test.cpp](../tests/native_handoff_test.cpp) uses packets
`(t,2t,3t,4t)` as an exact four-word coherence witness. Its toy dependency
calculation propagates a maximum tag. This is not centroid arithmetic, inherited
response or a physics-settling result. Probe indexes are diagnostic inputs, not
a proposed production identity API. C2a adds no production hierarchy.

Two test modes configure one/two native CPU workers and verify actual CPU
selection. Each covers 48 cases: twelve topologies, initially sleeping/all-active
parents, and one/64 duplicate notifications. Every case runs 32 reset/reuse
rounds, then perturbs the quiet leaves again without resetting numerical state.
Pairs, fan-in, forward/reverse five-node chains and a permuted 63-node tree use
70 records with holes and final valid record 69. Each input has a declared
16-poll observation limit followed by explicit value/activity checks. Per-poll
visit counters are test observations and never feed native selection.

All arrays, pointer blocks, traversal-list high-water marks, kernels and prepared
contexts are warmed before resource observations. The test-only compiler wrapper
uses the same native compile/prepare/launch facilities and exposes the compiled
module for audit; it changes no production API. Typed inventories include actual
host/native copies and prepared context bytes. Native publication storage is
`8*N*sizeof(double)` plus `N*sizeof(int)` for selectors. Ordinary host inspection
copies and diagnostic metadata are charged separately. Fixed C++ containers,
LLVM/JIT/runtime structures, native roots and pool reservation are outside this
typed payload; requested/reserved native memory is observed independently.

Every case separates first poll and total poll-kernel time, inspection time,
logical work, configuration uploads and readback bytes. Each observation reads
10,640 bytes at N=70; configuration uploads 1,400 bytes. Total case elapsed time
also includes reset/seeding, native diagnostic clearing and host assertions.
Setup includes compilation and warm dispatch, excluding Runtime construction.
These small concurrent-test timings are not throughput or GPU measurements.
Linked C++ new/new[] observations exclude arbitrary malloc, page residency/faults
and driver internals. Host vectors are not pinned.

## Following review boundary

The findings satisfy C2a's native investigation, not the following live release.
A new integration contract must adopt or revise the single-writer/one-publication
per-poll rule, numerical slot lifetime, all-reader selector access and native
activity cut. Account for publication/history/repair overlap and every existing
consumer, including force, focus, geometry, inspection and structural updates.
Existing eight-row field storage is not automatically free publication space;
its reduction/history meanings and writers must be reconciled before reuse.

Distinct comparison/native-field ownership, source-compatible counts, complete
initialization, local/inherited whole-object response with one body integration,
and coupled multilevel settling/recovery remain under #177/#172. The optional
candidate query stays outside the mandatory poll path. C1's persistent sub-grid
activity watch point is unchanged. #156 operations and #145 GPU qualification
remain separate; no database, cache-manager or analyst work is included.
