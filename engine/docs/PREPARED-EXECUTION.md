# Combined prepared backing and native bindings

Status: **accepted and merged in #166 at `4e7b193`, 2026-10-22**. Implements
the [backing/binding section accepted in #165](../../docs/prepared-execution-backing-plan.md)
under #162, following #164's accepted record mapping. Baseline `main`:
`a9a63be117a27975a9ae2727d0763717febfbb1d`.

The prepared Harness finishes setup empty, then runs initial labelled fixtures
through the existing field kernels and retains the execution for subsequent
ticks. Coherent changes to a ticking construct, production insertion and the
remaining #156 integration are not implemented by this step.
All eight hosted checks and all 37 CPU suites passed at the accepted head
`34c01e0`. The [next coherent-update section](../../docs/prepared-coherent-update-plan.md)
was accepted in #167; its implementation remains unbuilt.

## Working storage and native selection

The [capacity constructor](../src/field/field.h) allocates particle, field,
participation and contact arrays at independent capacity strides. It preserves
the legacy fixed-count constructor/getters and adds explicit capacity and
presence accessors. FixedStudy and ActiveSession keep their existing supplied
fixture behavior. Legacy publish/upload/reseed/focal-rebase methods cannot reset
a prepared execution.

Particle presence comes from the accepted native record map. A separate bounded
map supplies field presence and reuse metadata; its small field-key span does
not duplicate the particle potential span. Native awake selection remains one
particle set and one centroid set. Consuming awake bits retains sleeping payloads,
mass and membership. Seed, universal mass and all-present invalidation test
presence before reading numerical records. Their bounded capacity/header checks
are separate from selective native traversal; numerical discovery never scans
the potential span `V`.

[PreparedStorage](../src/field/prepared_storage.h) retains working/prospective
metadata, batch input/results/retirement/reference buffers, a prospective CSR
copy, reusable CSR counts/cursors, validation and derived-result workspace, and
inspection output. Corresponding backend workspace is allocated and zeroed
during setup. The prospective reserve is backing for the next coherent-update
step, not evidence that a prospective commit algorithm already runs.

The original 22 numerical array families remain in Harness/Activity. Initial
fixture CSR construction now uses the reserved workspace, retaining source
contribution order and both contact index directions. It performs no temporary
vector allocation. Ordinary ticks do not rebuild those indexes or republish
the fixture.

Reference demand is `present P + present G + 2 live E + 2 live C + selected F`.
Repeated endpoint occurrences remain distinct charges. Physical representations
may use existing endpoint arrays; a reference count does not demand a duplicate
handle allocation. The evidence configurations retain `R=928/F=9/B=32` and
`R=53,762/F=257/B=256`, with the accepted 70/4,096 particle capacities.

The workspace manifest reports each extent, actual element size and retained
copy count. Scratch reservation includes host/backend workspace, native-map
command buffers, Activity reset/focus storage and an explicitly derived binding
reserve. The binding reserve covers the 122 parameters of 20 supported contexts:
up to 24 ABI bytes per parameter plus actual native scalar/array metadata slot
sizes; CUDA additionally reserves device arguments, minimum result/argument
buffers and its cached context-pointer argument. Actual retained binding bytes
and unused reserve are reported separately.

These are payload/reservation calculations. C++ object/container overhead and
compiler memory are not a process-wide heap quota. Native root/layout padding,
list chunk slack, aligned pool consumption and raw reservation are measured
separately. Resource preflight checks known byte arithmetic and rejects
insufficient scratch before constructing layouts. `CapacityError` carries the
named budget, required amount and limit. The current backend workspace byte
axis is limited to 1 GiB and native batches to 1,048,576 entries; unsupported
shapes fail during setup. Neither is a runtime installation default.

## Reusable native dispatch and ownership

The native LLVM launch extension freezes validated flat scalar and rank-1/2
primitive-array bindings. It snapshots bound array type/rank/extent, validates
the signature and device, resolves pointers and caches typed scalar offsets.
Engine wrappers retain the contexts and stable shapes. Scalar changes use
native typed setters; array rebinding and stale/foreign kernel contexts are
rejected. The ordinary launch path remains available for other signatures.

Preparation records and checks the context's originating Kernel and Program.
Before registration or binding mutation, the native boundary verifies the
Program's executor and its existing compilation record for the exact target,
then compares compiled parameter/return types, layouts and buffer sizes with
the source context. The ownership lookup reads the existing compiler cache;
it creates no second registry and cannot create a compiler on a rejected path.
This also rejects scalar/array ABI mismatches behind a shared cache key.
Contexts without native Kernel provenance use the ordinary launch path.
[The #166 review correction](https://github.com/Human-Cognome-Project/human-cognome-project/pull/166#issuecomment-6339361944)
and [follow-up evidence](../tests/prepared_execution/origin-guard-acceptance.json)
cover unprepared foreign contexts and successful rightful reuse after rejection.

BulkCopy uses owned backend arrays and fixed host staging. CpuDirect borrows
fully reserved host arrays. Harness checks host pointer identity and length
before prepared work/readback; invalidating those buffers makes execution
unready. Current numerical values remain execution state; diagnostics/readback
are not a second placement authority.

CPU launches bypass recurring parameter-vector construction. CUDA prepares
owned argument/result buffers using the existing device allocator and retains
its launch argument vector. It avoids the ordinary launcher's per-dispatch
temporary maps and driver argument/result allocations. Prepared CUDA calls
complete on the existing ordered stream before returning. This deliberately
supports one synchronized owner and establishes no asynchronous-overlap or
latency guarantee.

Persistent launch resources belong to the context. Context replacement completes
the displaced binding before releasing its borrowed host buffers. Normal
destruction waits before freeing device buffers. Harness destroys numerical
bindings, then Activity bindings, before the payload arrays they borrow.
Partial setup owns each successful allocation
immediately; dispatch exceptions mark the binding unusable. Cleanup attempts
both buffer releases even if synchronization fails. Runtime/Program must outlive
the contexts and arrays. Host failure injection checks ownership cleanup and
refusal after failure; actual failed-device recovery is not claimed.

All usable native awake blocks and traversal-list high-water marks are exercised
using storage-only setup operations. No physical kernel runs while temporary
setup bits represent absent records. The supported numerical, validation,
recycling and inspection families receive real empty dispatches before ready.
Ready inspection confirms zero presence, awake work, relationships and mass.

## Bounded map batches

Map batches validate input and every affected reference before changing that
map. They reuse fixed command/result arrays. Definition checks current free
capacity; it cannot spend an uncommitted retirement. Release queues native
recycled indexes and runs the existing serial GC once per batch. The single-item
interface remains supported, including zero-budget and stale-handle behavior.

No new free queue or scheduling system is introduced. Host duplicate checks
are `O(B²)` for the bounded batch, lookup still enumerates present references,
and native collection is worst-case `O(P+B)` for a release batch. Collection and
transfer/synchronization costs remain real work. Homogeneous map batching is
not a coherent particle/field/relationship transaction.

## CPU proof and boundaries

All 37 CPU suites passed. After the final completion-order corrections, all
11 affected capacity/map/binding/execution suites passed on the matched rebuild.
The retained profiles use that final source.

| Particle capacity | Scratch reservation, both CPU paths | Retained binding payload | Binding reserve |
|---:|---:|---:|---:|
| 70 | 163,064 bytes | 6,520 bytes | 7,808 bytes |
| 4,096 | 7,907,568 bytes | 6,520 bytes | 7,808 bytes |

These scratch totals include the binding reserve. Numerical array payloads,
host/backend copy counts and compiled native roots are reported separately.

[The test-only fixture adapter](../tests/prepared_fixture.h) supplies starts
after the empty ready boundary. It claims arbitrary potential addresses,
creates holes and uses the last numerical record. It seeds one fixture only;
there is no new production coordinate/velocity input. Runtime structural
staging commands contain no supplied starts.

The [execution tests](../tests/prepared_execution_test.cpp) compare actual
Harness/Activity results with the established dense execution for 32 ticks,
including a change to all-present polling. They cover the 70/4,096 record
profiles in BulkCopy and CpuDirect, relationship-dense working sets and existing
aggregate-letter, byte-only and mixed projection/membership fixtures. They
check inclusive mass, all particle/group numerical fields including centroid
history, typed field identity, source contribution order and absence of awake
holes. Force/contact body counts demonstrate actual numerical execution.

The tested first population, ticks and inspection make zero observed C++
`new/new[]` calls. Runtime requested bytes, chunks, node allocation count, pool
requests/raw requests, raw reservation, aligned host cursor, SNode/tree counts
and compiler counters stay unchanged after ready through first nonempty use
and the 32 ticks. The old 16 allocations / 104 requested bytes per sampled map
command are removed on the new prepared path.

Additional checks cover each logical limit, scratch rejection before layout
creation, zero-capacity sentinels, immutable host buffers, typed/rank signature
rejection, ordinary launch compatibility, stale/foreign batch references,
invalid whole batches and host ownership/failure cleanup. Linked C++ allocation
observation does not cover arbitrary direct malloc, page faults/residency or
CUDA driver internals. Native counter stability is not universal allocation-free
or resident-memory proof.

[Retained acceptance and raw evidence](../tests/prepared_execution) pin source,
backend, reservations and validation. CUDA source/runtime compilation is checked
in the cloud; GPU execution and driver traces remain local under #145.

## Reproduction

Rebuild the matched fork components and regenerate runtime bitcode. The native
C++ launch-context layout changes; mixing old component archives with these
headers is unsupported. LLVM RuntimeContext/NodeManager struct layouts are
unchanged. Generated bitcode and build products remain untracked.

```sh
cmake --build engine/taichi/build-review --target taichi_c_api -j 2
cmake -S engine -B engine/build
cmake --build engine/build -j 2
ctest --test-dir engine/build --output-on-failure
. engine/build/engine-env.sh
engine/build/prepared_launch_test
engine/build/prepared_execution_test
engine/build/prepared_execution_test --direct
engine/build/prepared_execution_test --large
engine/build/prepared_execution_test --large --direct
engine/build/prepared_execution_test --letters
engine/build/prepared_execution_test --letters --direct
```

For a matched CUDA-enabled build, run `prepared_launch_test --cuda` and the
three bulk execution commands (plain, `--large`, `--letters`) with `--cuda`.
Tests reject CPU fallback. Record GPU/driver, pool mode and actual driver
allocation traces separately; CpuDirect is CPU-only. Cloud CPU results do not
qualify GPU behavior, pinning or the browser.

The next #162 step must apply coherent record/relationship changes while ticking,
preserve survivors and untouched reduction residue, handle mass/focus wake
dependencies and prove failure publication. #162 and #156 remain open.
