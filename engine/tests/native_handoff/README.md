# C2a native handoff CPU evidence

Implementation/findings for review under accepted [#188](../../../docs/live-hierarchy-handoff-plan.md).
See [NATIVE-HANDOFF.md](../../docs/NATIVE-HANDOFF.md) for the native source audit,
protocol, essential lifetime constraints and remaining live integration work.

Runtime/test source: `faf145ed5a19f26bede3096ab38eee2dd8100ed2`.
Accepted plan merge: `cbc9e90efc97354f423d7a384b3e8c35c0bd2435`.
Matched native backend tree: `6784224121f68de1bf1aa0765b54e80d20e355ad`.
Clang/LLVM 15.0.6, Release native CPU-only build, CTest concurrency two.
New modes explicitly configure one/two native workers and require actual CPU.
This commit changes only test source and CMake; production code/fork are unchanged.
The following documentation/evidence commit changes no tested source.
All **84 CPU suites passed in 528.64 seconds**, including both new modes and
the unchanged 82-suite regression.

| Artifact | Contents |
|---|---|
| [acceptance.json](acceptance.json) | Source/backend/compiler and binary hashes, two modes, all case measurements, typed manifests, compiled atomic instructions and native counter snapshots. |
| [cpu-ctest.txt](cpu-ctest.txt) | Complete 84-suite regression: the existing 82 plus both new modes. |
| [cpu-native-output.txt](cpu-native-output.txt) | Both complete native probe sections from that regression, including negative controls and first-use/recovery observations. |

The deterministic controls expose propagation-only consumption of an unevaluated
parent, late consumption after a newer child notification, and a partial in-place
four-word publication. Positive ordered controls exercise publication before
and after the consume/read cut and 4,096 duplicate native notifications to final
valid record 69. Stress success is not substituted for those harmful-order controls.

Each mode runs 48 cases: twelve topologies, initially sleeping/all-active parents,
and one/64 notifications. Each case has 32 reset/reuse rounds and two published
inputs, with a perturbation after quiet state that reuses both numerical slots
without resetting retained values. Pairs cross same/adjacent/distant mask blocks
in both record orders. Fan-in, forward/reverse five-node chains and a permuted
63-node tree use N=70 with holes and a padded pointer layout. Each input has a
declared 16-poll observation limit. Checks require complete final values and no
stranded consequential change, not merely empty activity bits.

Both initial delivery and later recovery took 1–5 native polls in these fixtures.
Across both worker modes, 6,144 input transitions passed with zero torn reads or
lost final inputs. This is a bounded packet result, not a universal propagation
latency bound or a coupled-physics settling result.

The tested protocol consumes native activity at entry, acquires a complete child
packet through native atomic selector reads, fills its inactive publication slot,
atomically publishes the selector and then activates its parent. A record may
publish once per native poll; all readers finish before the next poll can reuse
an old slot. One NativeSet selects work. The numerical selector is not an awake
mask, queue, timestamp or scheduling epoch. The compiler audit checks actual
Program-retained LLVM selector RMW sites and sequentially consistent ordering;
raw instructions are included. Frontend atomic requests can otherwise be demoted.

Packets `(t,2t,3t,4t)` and maximum-tag propagation are coherence/notification
witnesses. They implement no centroid equation, local/inherited force or physics
settling criterion. No production hierarchy is installed. The lifetime, all-reader
access and compiler-ordering conditions must be reviewed in the following live
integration contract; arbitrary overlapping polls/writers are unsupported.
CUDA behavior is not established by CPU execution or the compiled CPU IR audit.

Raw counter columns are phase, native_requested_bytes, known_list_chunk_bytes,
known_allocator_elements, pool_allocation_calls, pool_requested_bytes,
pool_raw_allocation_calls, pool_reserved_bytes, aligned_host_cursor, tree_count,
issued_snode_ids and compilation_seconds. Before/after snapshots must match.
All pointer blocks/list high-water marks, arrays, kernels and prepared contexts
are warmed before first measured cases; guards observe zero recurring linked
C++ new/new[]. This excludes arbitrary malloc, physical residency/faults and
driver internals. Host vectors remain ordinary unpinned memory.

Every typed manifest term equals count × element bytes × copies. It charges
native data, host diagnostic copies and prepared bindings; fixed containers,
caller graph fixtures, LLVM/JIT/runtime structures, native roots and pool
reservation are separate. Both publication slots total 4,480 native bytes and
selectors 280 native bytes at N=70. Context metadata and diagnostics are not
hidden inside that numerical payload.
Total typed payload and prepared bindings are 27,832 bytes per probe; retained
binding metadata accounts for 2,912 bytes. Native roots, pool reservation and
compiler/runtime memory remain separately observed/excluded as described above.

Case totals include reset/seeding, both inputs, inspection and host assertions.
The first poll, total native poll time, inspection time, logical work and transfer
bytes are reported separately. Each inspection reads 10,640 bytes; each graph
configuration uploads 1,400 bytes. Per-record visit counters are test observations
and do not select work. Setup includes layout, compile and warm dispatch, excluding
Runtime construction. Timings under CTest concurrency are bounded observations,
not engine throughput or GPU performance measurements.

No database, cache-manager, analyst or GPU acceptance is included. #177/#172's
comparison/response ownership and coupled settling, #156's operation exit and
#145's GPU qualification remain open. C1's sub-grid activity experiment is unchanged.

Reproduce after the normal native build:

```sh
ctest --test-dir engine/build -R '^native_handoff_' --output-on-failure -j2
ctest --test-dir engine/build --output-on-failure -j2
```

Compare source hashes with `git show PIN:PATH`. No generated native binary or
full generated LLVM module is retained; the critical compiled instructions are
captured in the raw output and JSON.
