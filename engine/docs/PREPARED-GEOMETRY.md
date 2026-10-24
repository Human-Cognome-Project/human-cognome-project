# Current native geometry preflight

Status: **first portion of Segment 2, implemented for review, 2026-10-23**.
Baseline is #171 merged at `c6b85de0d513dcc8d60fa38bdcfb8d1640c07e66`.
The accepted [#170 plan](../../docs/core-operation-readiness-plan.md) continues
to govern the full operation path under #162/#156.

`Harness::query_prepared_geometry(expected_generation, work_limit)` reads
current resident backend values and prepares candidate occupancy/contact
metadata. It includes sleeping physical bodies, distinguishes field records
from occupants, and computes the current selected focal point after motion.
Its outputs give later admission and candidate-tick work a common geometry
foundation. The query does not move bodies, install contacts, modify native
activity, publish a generation or advance a tick.

Engine-owned nearest-free search, bootstrap, structural admission, native
contact installation and candidate tick publication remain unbuilt. Existing
ticks still use the explicit contact ledger and publish movement directly.
A successful query before a tick cannot guarantee that contacts after that
tick fit capacity. The accepted requirement to validate candidate motion and
its contact coverage before any live movement remains outstanding.

## Observation and ownership

The caller exclusively owns a synchronized prepared Harness throughout query
and inspection, just as for coherent updates. The query uses native particle
presence, not the awake set, for occupancy. A native struct traversal marks
present record rows; a bounded P-sized pass compacts them into deterministic
record order. No potential-address V scan or separate activity scheduler is
introduced. Awake counts are diagnostics only.

Positions, masses, radii and source contact endpoints come from current
backend arrays. BulkCopy host staging may be stale. Fixed prepared bindings
keep CpuDirect borrowed arrays stable. Focal reduction reads current published
group centres in selected order, using the established relative f32 arithmetic
around the cached focal reference. This matters immediately after a tick:
the cached begin-of-tick origin can precede the newest group positions.
The result includes the unchanged double frame anchor.

`GeometryFocus` distinguishes empty geometry, missing selection, valid positive
selection and invalid selected fields. Missing selection does not prevent an
independent contact query; it never authorizes placement at a fabricated origin.
Future admission must apply D1 or require a valid populated focus. Finite
payloads whose contact/focal arithmetic exceeds the supported f32 domain return
`InvalidNumerics` without changing committed state.

The result token contains the existing native map context, structural generation,
successful tick observation count and query serial. Inspection rejects a foreign
context, later generation/tick, or superseding query, including rejected queries.
These counters only identify observations; they never select or schedule work.
Unexpected backend/binding exceptions mark execution unready. Ordinary resource,
generation or numerical rejection leaves live readiness and state intact.

## Contact union and resource bounds

All unordered present-body pairs are examined. Contact coverage shares the
accepted f32 geometry helper with the existing contact force kernel. It includes
positive penetration and positive-radius coincidence. Coincidence still has
zero directional force under the unchanged law; retaining coverage does not
invent a separating force. Exact tangency and coincident zero extents do not
create a derived contact. This contact predicate is not yet D4's outward-rounded
placement/search certificate.

Explicit source rows are always included, even without penetration. Reversed
and repeated source rows refer to one physical union pair, and the source-row
mapping retains every row. The source ledger, headers and ordinals are unchanged.
Union order is first unique explicit source order, preserving its original A/B
orientation, followed by derived-only pairs in canonical record order. Both
particle-to-contact CSR directions are built in candidate storage. This ordering
preserves explicit force accumulation conventions for the later installer.

The optional setup budget `GeometryWork` enables the four query kernels and
their actual typed storage. Its default zero allocates no geometry ndarrays,
vector payloads or bindings and returns `Unsupported`. Existing prepared scratch
profiles remain unchanged; inline control members are C++ object overhead.
A call may choose a smaller work limit; exceeding the configured
limit is rejected before dispatch. With N present bodies, S explicit source rows
and U physical union pairs, reserved comparison work is:

```text
Q = N*(N-1)/2 + S*S + U*U
```

The first two terms are checked before pair discovery; the union ordering term
is checked after exact discovery and before finalization. Counts are wide enough
for the supported dimensions. Source and union rank ordering are deliberately
simple bounded native kernels, O(S²) and O(U²). Presence compaction, endpoint
validation and CSR passes add bounded linear work in P, N, S and U. Source
duplicate groups advance once rather than rescanning the source list per pair.
This is a measured baseline, not a spatial acceleration or scalability claim.
The matched native Program/IR interfaces expose sparse traversal, atomics and
range kernels; they have no callable prepared sort/scan operation used here.

Discovery counts exact union demand even when U exceeds C; guarded writes never
truncate a successful candidate or overwrite the reserve. It also checks
prospective references `P_live + G_live + 2*E_live + 2*U + F_live` against R.
These are current-query demands, not full admission old-plus-new peak checks.
Exhaustion names GeometryWork, C or R and invalidates candidate inspection only.

## Storage and transfers

The [storage manifest](../src/field/prepared_storage.cpp) adds these actual
arrays when enabled; p/c below mean physical sentinel strides `max(P/C,1)`:

| Storage | Backend elements | Host inspection elements |
|---|---:|---:|
| Compacted occupancy | p i32 | p i32 |
| Temporary pairs / provenance | 4c i32 | 0 |
| Final pairs / flags | 3c i32 | 3c i32 |
| Source order / duplicate group end | 2c i32 | 0 |
| Source mapping / union permutation | 2c i32 | c i32 |
| Both CSR directions / cursors | 6p + 2c i32 | 4p + 2c i32 |
| Counters / focus | 16 i64 + 4 f32 | 16 i64 + 4 f32 |

Geometry adds 33 parameters across four prepared contexts: collect 10, source
order 4, discover 9 and finalize 10. Total binding reservation uses 184 parameters
and 26 contexts; actual used payload is separately reported. Setup compiles,
binds and dispatches every family while still empty. Scratch includes actual
host/backend payloads and binding reserve; C++ object/container metadata,
compiler storage, native roots and pool alignment are separately accounted as
in the existing prepared path. CPU host buffers remain ordinary vectors.

A normal successful query reads only 272 bytes (16 counters twice and four
f32 focal outputs) and explicitly synchronizes three times. It uploads no
numerical payload. Returned counters distinguish pair/order work, native
presence visits and P compaction slots. `inspect_prepared_geometry(result)` is
an explicit diagnostic copy into already reserved host vectors: occupancy p,
pairs 2c, flags c, source map c, offsets 4p and orders 2c i32. It downloads no
body/group numerical payload, has fixed capacity strides and logical counts
from the successful result, and remains borrowed until the next inspection.

## CPU evidence and remaining work

The [retained evidence](../tests/prepared_geometry) pins source, backend and
build artifacts. All **52 CPU suites passed in 378.65 seconds**, including the
eight geometry modes. With geometry enabled, the two existing capacity profiles
reserve 127,364 and 5,637,524 scratch bytes respectively. Actual bindings are
9,928 bytes against an 11,776-byte reserve in both CPU paths. Profiles with
GeometryWork=0 retain their earlier scratch/binding totals.

Eight modes cover P=70/P=4,096, both CPU host paths, sparse
holes including the last row, and maximum P/G/E/C source fixtures. Tests include
sleeping/coincident bodies, explicit/derived deduplication, preserved source
order/orientation, complete two-sided CSR, current focus after motion, 32 repeated
tick/query cycles per sparse profile, stale/foreign observations, independent
work/C/R rejection, C=0 sentinels, exact tangency, zero extents, unsupported
arithmetic and the disabled path. Live numerical/contact/activity/history and
Demand observations are compared before/after query and ordinary rejection.

Measured hot query and inspection boundaries observe zero linked C++ new/new[]
calls. Known native map/activity list backing, pool requests/reservations,
aligned cursor, tree/SNode and compiler counters are checked from setup through
first population and reuse. These observations exclude arbitrary direct malloc,
physical residency/page faults and driver internals. Timings are local bounded
observations, not a throughput promise.

The maximal P=4,096/C=8,192 profiles perform 142,604,288 comparison-work units;
their first populated queries measured 59.4 ms (BulkCopy) and 89.0 ms (CpuDirect)
in this concurrent CPU run. These timings exclude setup and explicit candidate
inspection, and should not be generalized to GPU or other hardware.

Reproduce using the matched [native build](PREPARED-EXECUTION.md#reproduction):

```sh
cmake --build engine/build -j3
ctest --test-dir engine/build -R '^prepared_geometry_' --output-on-failure -j2
ctest --test-dir engine/build --output-on-failure -j2
```

The remainder of Segment 2 still needs the certified nearest-free search,
structural-to-native admission/bootstrap, candidate movement/control/activity
publication and resulting contact installation, and explicit host transport
capabilities/pinning. Segments 3/4 retain the enduring owner lifecycle and full
integrated CPU operation matrix. Actual CUDA execution, transfers/pinning and
GPU allocation checks remain the later local #145 gate. No database writes,
Python runtime, private-manager path or new analyst policy is introduced.
