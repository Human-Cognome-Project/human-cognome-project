# Prepared native record mapping

Status: **first #162 implementation step, accepted and merged in #164,
2026-10-21**, at `41310a512a137712651720b7bea5012238079900`.
The [accepted #163 plan](../../docs/prepared-capacity-foundation-plan.md) calls
for the mapping/adapter decision to be reviewed before wider field-kernel
changes. This step supplies that proof. Combined prepared field execution and
production insertion under #156 remain unbuilt. The
[next backing/binding section](../../docs/prepared-execution-backing-plan.md)
is drafted for review, following the
[final #164 acceptance](https://github.com/Human-Cognome-Project/human-cognome-project/pull/164#issuecomment-6326052999).

## Mapping and native adapter

[`PreparedRecordMap`](../src/engine/record_map.h) owns a byte declaration span
of logical size `V` and independent native reference capacity `P`. A zero byte
has no presence. Nonzero byte values are opaque declarations, not new particle
types or a placement rule. Potential addresses do not prescribe coordinates.

The layout has three siblings under one root:

- Dense `u8` potential declarations.
- A native pointer SNode used only to provision `P` recyclable native indexes.
  Its cell contains one dummy byte, never a physical particle or numerical body.
- A native bitmasked reference table with an `i32` potential address and `i64`
  incarnation per row. Presence is separate from the existing awake sets.

Preparation activates then deactivates exactly `P` pointer nodes. The fork's
ordinary compiler-inserted GC creates its native free list. Pointer activation
does not expose the resulting allocator index for binding separate numerical
arrays; dynamic append also lacks individual release/reuse. Two narrow runtime
functions therefore expose the existing `NodeManager` mechanism:

- `prepared_node_acquire_index` consumes one prepared native free-list index.
  Exhaustion leaves its cursor unchanged and never extends the data list.
- `prepared_node_release_index` recycles that native index and calls the existing
  serial `node_gc`. Ownership and duplicate-release validation happen first.

There is no second allocator, free queue or scheduling system. The previously
unused native allocator lock is initialized explicitly. Acquisition uses the
existing native lock helper. Collection requires an exclusive, synchronized
engine boundary; no ordinary pointer activation or concurrent collection may
use this private allocator. Destruction synchronizes and destroys its SNode tree;
the runtime and all borrowed bindings must have the documented lifetimes.

A returned record indexes fixed-capacity numerical arrays independently of its
potential address. The test binds a two-axis array at stride `P`, enumerates
native presence, and updates per-record numerical counters. New incarnations
initialize only reused rows; surviving counters continue. These are labelled
storage fixtures, not a replacement force implementation or physical starts.
The current Harness and Activity kernels are unchanged.

## Reference safety and costs

Handles contain execution context, potential address, native record and an
incarnation. A successful definition consumes the next incarnation; release and
reuse invalidate older handles, including same-address/same-record reuse.
Context identity is assigned only during preparation. It is bounded execution
safety, not adoption of held #123's general identity proposal. Successful changes
advance the map's committed generation. Invalid input, duplicates, exhaustion,
stale handles and foreign handles preserve its committed state and readiness.
Unexpected backend/internal failures make the map unready; transactional fault
recovery is not established by this proof.

The mapping makes these tradeoffs explicit:

| Operation | Bounded work |
|---|---|
| Define, inspect or change kind through a valid handle | Point access plus native claim/validation and synchronized dispatch |
| Lookup a potential address | Native traversal over present reference rows; `O(live P)` bodies, plus checks across the padded native reference mask |
| Release | Point validation followed by existing serial GC compaction over the remaining native free list; worst-case `O(P)` per release |
| Numerical fixture traversal | Present rows only; fixed `P` stride; no scan over `V` |

Lookup intentionally avoids a full-width reverse pointer at every potential
entry. No hash table or custom index is added. Actual release/lookup costs must
inform subsequent batching; they are not promised constant-time or optimized
GPU operations. Single-record synchronized commands are not yet coherent
multi-family batches.

The nominal storage is one byte per potential entry plus address/incarnation,
pointer and mask metadata proportional to padded `P`. Native alignment, list
chunks and pointer-data/free/recycle chunk slack are additional. Only logical
`P` indexes can be claimed; power-of-two padding is unavailable. `P=0` works
without exposing the one-cell layout sentinel as a record.

## Budgets and launch bindings

[`PreparedCapacity`](../src/field/prepared_capacity.h) validates independent
potential, particle, field, participation, contact, reference, selected-focus,
batch and scratch-byte limits before allocation. It computes the 22 current
Harness/Activity numerical array shapes at capacity strides with checked wide
byte products and sums. Zero logical limits retain native array sentinels without
admitting items. Exhaustion reports the named budget, required amount and limit.

This is limit/layout arithmetic, not a prepared execution. It does not allocate
all those arrays or derive/certify complete prospective relationship, reference,
inspection and batch scratch backing. Those actual reservations and combined
preflight/commit checks belong to the next #162 step. The map alone enforces its
potential and record limits.

Command input/output arrays and launch contexts are created and bound once.
Repeated context use exposed two native launcher lifetime defects: CPU relabelled
an ndarray descriptor as a raw pointer, and CUDA retained freed temporary launch
buffer pointers in the context. The patch preserves the CPU descriptor and
restores CUDA host pointers after launch. Both launchers also retain their
registered metadata by reference instead of copying it each dispatch.

These repairs do **not** eliminate all launch overhead. The linked CPU C++
`new/new[]` observation still records **16 calls / 104 requested bytes** per
sampled map define, release and lookup, despite stable contexts/native backing.
The native launcher still constructs parameter-index vectors and writes array
pointer metadata. Removing that recurring work belongs to the next prepared
binding step. There is no universal allocation-free claim here. Direct `malloc`,
page faults/residency and driver allocations are outside this observation.
The CUDA launcher still makes async argument/result allocations per dispatch;
the native pool counters do not cover those calls.

## CPU evidence

Baseline `main`: `7bf88199cb913c594023b3b448460444e97cfb45` (#163).
Taichi baseline subtree: `f4bbe8187cfcf19056ac51e338460e76bb5a4faa`, plus the
adapter/launcher changes pinned in the acceptance record. LLVM/Clang 15.0.6,
x86-64 AMD EPYC 9V74, two runtime CPU threads. These profiles are tests, not
installation defaults.

| Test profile | `V` | `P` / padded `P` | SNode root bytes | Map setup host cursor increment |
|---|---:|---:|---:|---:|
| Bulk and direct CPU | 1,048,576 | 70 / 128 | 1,052,688 | 9,650,216 |
| Larger bulk CPU | 16,777,216 | 4,096 / 4,096 | 16,908,800 | 25,505,832 |

The setup cursor increment includes native alignment, list chunk slack, root
page rounding and 80 command-array bytes. It excludes base runtime storage and
the later numerical fixture arrays. Dynamic runtime requested bytes increase by
8,571,217 in each profile. Native chunk backing is 8,536,064 bytes. Raw pool
reservation is reported separately; the CPU general allocator reserves at least
1 GiB of virtual address space. None of these measures establishes resident
physical memory.

Each profile checks first define/release immediately after empty preparation,
then 32 cycles of arbitrary noncontiguous addresses, holes, release and reuse.
The larger profile performs 69,633 definitions and matching releases. Native
requested bytes, chunks, allocated node count, pool request/raw-request counts,
reservation, aligned host cursor, SNode/tree count and compiler-time counter
stay exactly unchanged after preparation through those updates. Exhaustion also
leaves the native claim cursor unchanged. Exact lookup/handle checks, unsigned
byte 255, independent numerical strides, surviving values, stale/double release,
foreign context, zero budget and guaranteed same-address/record reuse pass.
Budget tests separately exercise each limit and arithmetic overflow.

All 30 CPU CTest suites pass. The final test-only potential-padding assertion
was then checked by rerunning all four focused suites. The CUDA launcher source
passes a syntax check with `TI_WITH_CUDA`, and the runtime compiles with
`ARCH_cuda`; no GPU execution or driver tracing is claimed.
[Retained evidence](../tests/record_mapping) pins the measured source and CPU
results.

## Reproduction and local GPU follow-up

Build the matched fork and regenerate runtime bitcode before configuring the
workspace. The adapter adds runtime symbols; an old staged bitcode file cannot
run the new map. Existing runtime struct layouts are unchanged.

```sh
cmake --build engine/taichi/build-review --target taichi_c_api -j 2
cmake -S engine -B engine/build
cmake --build engine/build -j 2
ctest --test-dir engine/build --output-on-failure
. engine/build/engine-env.sh
engine/build/record_map_test --records 70 --span 1048576 --cycles 32
engine/build/record_map_test --records 70 --span 1048576 --cycles 32 --direct
engine/build/record_map_test --records 4096 --span 16777216 --cycles 32
```

After a matched CUDA-enabled fork/workspace build, use the same two bulk
commands with `--cuda`. The executable rejects a CPU-only build or actual CPU
fallback. Record actual backend, GPU/driver and async pool mode, and trace async
driver allocations separately from native pool counters. The direct profile is
CPU only. Local GPU qualification remains under #145; passing this storage proof
does not qualify real field kernels, browser behavior or #156 live insertion.
