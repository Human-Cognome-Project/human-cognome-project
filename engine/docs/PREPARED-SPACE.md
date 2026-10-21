# Native declaration storage investigation

Status: **CPU storage probe built, 2026-10-21; production #156 remains unbuilt.**
[Plan #160](../../docs/pooled-space-live-insertion-plan.md) accepts this native-first
investigation separately from its unresolved physical initialization conventions.
This probe creates symbolic definitions, never physical objects, coordinates,
mass, fields or memberships. It changes no database, warm recipe, workbench or
simulation interface.

## Storage decision supported by this probe

Use a native dense byte span for potential declarations, with native presence
enumeration bounded by the separately prepared numerical-record budget, as the
candidate for the next foundation slice. Keep the existing independent awake
mask for calculation work. A potential address is not a numerical record or a
physical coordinate. This is a measured storage recommendation, not acceptance
of a production admission API or a new scheduler.

The `dense-budgeted` fixture reserves one `i8` declaration per padded potential
entry, four native bitmasked reference records, and four independent 16-float
numerical records. Its native struct-for enumerates those four reference slots;
it does not scan the potential span. The four explicit fixture bindings are not
a proposed production slot allocator. Zero declarations finish preparation and
every release cycle. There is no numerical record per empty potential entry.

Making the entire potential span bitmasked also gives cheap point edits, but
the fork still checks its masks across that span during enumeration. Retaining
every sparse pointer ancestor has a related empty-container cost. Native
pointer recycling avoids retaining all those live ancestors, at the cost of
native free-list/GC work. Cold virtual pointer capacity alone is insufficient:
the first edit requests new backing.

## Measured CPU evidence

Baseline main: `1d98e89cd5992dbfbcd51054cfbcf60f887729ba` (merged #160).
Taichi baseline subtree: `d191c5ce8a04639da6069dde0ec1e35cce9abe7a`, plus the
native pool diagnostics and raw-bound repair in this change (measured subtree
`f4bbe8187cfcf19056ac51e338460e76bb5a4faa`). LLVM/Clang 15, x86-64 AMD EPYC
9V74, two runtime CPU threads. Each layout/span runs in a separate process,
sequentially, for 20 define/release cycles. These are bounded wall-time samples
from this cloud environment, not performance guarantees or GPU evidence.

The following bytes per potential entry use the **increment in native host-pool
cursors**, including internal alignment, tree-page rounding, native metadata,
lazy chunk slack, and the fixed 256-byte numerical/12-byte output buffers. Base
runtime storage and unused raw pool reservation are separate. Logical padding
is included in the cost but never exposed as valid application addresses.

| Fully prepared layout | 1,048,576 entries | 16,777,216 entries |
|---|---:|---:|
| Dense declarations + four native reference records | 9.028 B | 1.502 B |
| Flat bitmasked declarations | 9.145 B | 1.626 B |
| Pointer blocks retained, leaf bits released | 13.672 B | 4.128 B |
| Pointer blocks recycled through native GC | 13.930 B | 4.378 B |

The native enumeration lists reserve **8 MiB** of chunk backing even for the
small dense-budgeted fixture. Consequently, one byte is the nominal declaration
cost, not the total footprint at every scale. At 16,777,216 entries, the
dense-budgeted tree and buffers advance the host cursor by **25,194,724 bytes**.
The host pool separately requests at least **1 GiB** of raw virtual reservation
for its general bump allocator, plus the exclusive root/buffer requests. That
unused reservation is not counted as consumed declaration data or claimed as
resident physical memory. Raw request lengths for small buffers also differ
from OS page-rounded mappings.

For the 16,777,216-entry cases:

| Layout | Four definitions, subsequent mean | Four releases, subsequent mean | Empty enumeration mean |
|---|---:|---:|---:|
| Dense-budgeted | 5.37 µs | 5.25 µs | 12.30 µs |
| Flat bitmasked | 21.53 µs | 27.29 µs | 297,515 µs |
| Pointer retained | 23.70 µs | 18.12 µs | 26,749 µs |
| Pointer recycled | 13.36 µs | 2,014 µs | 1,963 µs |

Definition/release timings include four host context constructions and kernel
launches, then synchronization. Enumeration also includes counter reset,
readback and synchronization. They are not isolated device instruction timings.
One release call repeats an already released entry after the survivor check.
The recycled fixture intentionally retires the whole pointer index range using
native deactivation and compiler-inserted GC; its release time includes that
scan, recycling and zero-fill. This is not a proposed per-object release policy.
Other layouts release only the four leaf definitions. Native container/list
work remains distinct from the zero calculation-body entries when empty.

All four prepared layouts retain identical native requested bytes, native chunk
backing, allocated pointer-node count, pool request/raw-request counts, raw
reservation size, host cursors, SNode/tree counts and compilation-time counter
through first definition, repeated edits, release and reuse. The cold pointer
control instead requests **4,792,320 additional native bytes** on first touch
at both spans. Its smaller partially populated footprint does not represent
fully prepared capacity.

Raw phase rows, setup/first-dispatch time, compiler time, native list lengths and
pool counters are retained in [the evidence directory](../tests/prepared_space/evidence).
All 26 CPU CTest suites passed after the raw-bound repair; the
[acceptance record](../tests/prepared_space/acceptance.json) pins source and evidence
hashes, configuration, checks and limitations.
The [probe source](../tests/prepared_space_probe.cpp) checks exact point values,
native enumeration count/checksums, tail padding rejection, unchanged survivors,
redefinition, release/reuse and the independent numerical-buffer contents.
Rejected bounds/definition codes never dispatch an edit. This guard is a test
fixture, not resource exhaustion/admission handling. The 70-entry CTest cases
exercise non-power-of-two logical spans against padded native storage.

## What the counters establish

The native runtime's existing `total_requested_memory` includes dynamic managers,
directory pages, chunks and ambient nodes, but excludes alignment and ordinary
root/ndarray allocations. The probe reports root bytes/page rounding and arrays
separately. Its chunk total covers traversal and pointer allocator lists;
`free_elements` is the native free-list length and `free_used` its consumed
prefix, not two independent object counts.

The small `HostMemoryPool::statistics()` / `DeviceMemoryPool::statistics()`
addition reads the existing pools under their existing locks. It counts pool
requests, their requested bytes, cumulative raw allocation calls and current raw
reservation request bytes. Host cursors additionally expose alignment consumed
by the native bump allocator. Cumulative counters survive `reset()`; current
reservations/cursors reflect live native bookkeeping. The diagnostics add no
allocation, release, reservation or scheduling policy.

These are **native pool diagnostics**, not a complete host-heap/driver trace.
Host vectors, launch-context/JIT metadata, OS resident memory, first-write page
faults and CUDA module loading are not counted. Native backing counters do not
establish OS prefaulting or physical residency. CUDA's asynchronous driver
allocation path bypasses
`DeviceMemoryPool`; pool counters must not be presented as all driver calls.
GPU reserved-pool suballocation is visible through the runtime's requested-byte
counter, but device alignment/cursor use needs additional observation. The probe
warms the actual edit/read/enumeration dispatches before the prepared boundary;
the cold control deliberately does not warm its edit dispatch. A stable native
counter is not proof that the complete future admission operation is allocation
free. Host metadata and driver tracing remain qualification work.

## Native raw-bound repair

The source audit found that `UnifiedAllocator::allocate` set a new chunk's end
to `chunk.head + allocation_size` after advancing the head by the first request.
This extended its apparent capacity beyond the raw reservation. The one-line
repair uses `chunk.data + allocation_size` as the end.

[native_pool_test](../tests/native_pool_test.cpp) requests a full 1-GiB raw span
then another page. It checks that the second request obtains a distinct raw
reservation, correct cursor/request accounting, and cumulative counters across
reset. Neither large buffer is touched. Against the pre-repair native archive
from `92199bc` the test fails with `native pool reused bytes outside a raw
reservation`; the rebuilt corrected archive passes. This checks native raw
boundaries, not the future engine's finite supporting-budget exhaustion API.

## Reproduction and local GPU handoff

Build the matched native fork and workspace using [the engine instructions](../README.md).
The new diagnostic methods require rebuilding the fork libraries too; do not
link old archives against the new headers. No Python, PostgreSQL or viewport is
needed. With the build environment loaded:

```sh
cmake --build engine/build --target prepared_space_probe -j 2
ctest --test-dir engine/build -R '^prepared_space_' --output-on-failure
TI_LIB_DIR="$PWD/engine/build/taichi-runtime" \
  engine/build/prepared_space_probe --layout dense-budgeted --span 16777216 --repeats 20
```

On the matched CUDA build, repeat each layout with `--cuda`:

```sh
export TI_LIB_DIR="$PWD/engine/build/taichi-runtime"
for probe_layout in dense-budgeted flat-bitmasked pointer-cold pointer-retained pointer-recycled; do
  engine/build/prepared_space_probe --cuda --layout "$probe_layout" --span 16777216 --repeats 20
done
```

The probe requests a bounded 0.5-GiB CUDA reservation and refuses a CPU fallback.
Retain output and a driver allocation trace separately, including the driver's
native memory-pool mode. **No GPU run is claimed here.** The existing
`native_activity_test --cuda` remains a separate check of native awake traversal
and retained sleeping numerical values; symbolic definitions do not replace it.

## Remaining foundation work

This chooses a candidate layout and measures native provisioning/recycling. It
does not complete the plan's first foundation deliverable or #156. Next prepare
the supported numerical/field/edge/contact/reference/scratch budgets and their
actual kernel bindings together; prove within-budget operations, supporting
budget exhaustion, state preservation and native presence/awake phases on that
combined path. Use native facilities for record reuse before proposing any new
mechanism. Test larger present-record budgets; four reference records are bounded
evidence, not a general working-set limit or throughput qualification.

Production identity mapping, atomic admission, live relationships, placement,
occupancy, survivor updates, reports, recipe migration and LoD changes remain
unimplemented. Plan #160's bootstrap, initial-motion and numerical conventions
still require explicit review before production insertion acceptance. Neither
the storage fixtures nor their fixed reference bindings supply those conventions.
