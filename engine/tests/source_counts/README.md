# Exact source-count preservation CPU evidence

Implements Segment A of [accepted #183](../../../docs/weighted-centroid-accounting-plan.md)
under #177/#172. [Implementation and resources](../../docs/SOURCE-COUNTS.md)
describe the metadata boundary. Weighted candidate arithmetic remains Segment B.

Tested runtime/test source: `c632d58a35dafa620fcc8f28ec82a844c7241e3d`.
Baseline: `f8c4203dc67376479d5a7018d870f9906406fc69`.
Matched native backend tree: `6784224121f68de1bf1aa0765b54e80d20e355ad`.
Clang/LLVM 15.0.6, x86_64 CPU-only native build, two workers per source-count
mode, CTest concurrency two. Actual CPU selection is checked in every mode.
All **66 CPU suites passed in 401.74 seconds**, including all four source-count modes.

| Artifact | Contents |
|---|---|
| [acceptance.json](acceptance.json) | Source/backend/compiler pins, source/binary/raw hashes, four profiles, manifests, measurements and native counters for known/legacy executions. |
| [cpu-ctest.txt](cpu-ctest.txt) | Complete native regression result on the pinned source. |
| [cpu-native-output.txt](cpu-native-output.txt) | All four source-count sections from the same run, including both executions' resource snapshots and typed manifests. |

Profiles cross BulkCopy/CpuDirect with P/G/E=70/9/348 and 4096/257/20478.
Both particle and participation banks fill during first loading. The source is
real assembly preparation with thirds, repeated parent ordinals, whole sibling/
membership rows and independently instantiated copies. The source denominator
9223372036854775931 (2^63+123) and corresponding counts remain exact uint64
metadata despite narrower numerical body mass/share values.

The companion legacy execution receives identical numerical inputs with unknown
count provenance. Bitwise comparison follows logical occurrence and field
identities, because separate native maps can allocate different record numbers.
Bodies, fields, source rows, activity/presence, history, focus, anchor and universal
mass agree through 32 real tick/retire/reuse cycles. Alternating known/unknown
metadata and source-row compaction preserve each surviving occurrence's data.
This qualifies metadata preservation, not new weighted physics.

Complete committed snapshots separately cover count-only edits, identical no-ops,
clearing provenance, stale/foreign references and ordinary rejection, including
injected preclaim failure. Invalid supplied zero, oversized count, absent
denominator, mismatched share and wrong-operation metadata reject. Count-only
changes advance source generation and invalidate earlier candidate bindings/
observations without changing physical/control/activity/history/Demand state.
Scratch S-1 rejects before native layout mutation.

The measured update calls, including first population, and recurring tick/
inspection calls observe zero linked C++ new/new[] allocations. Native metric
columns are: phase, native_requested_bytes, known_list_chunk_bytes,
known_allocator_elements, pool_allocation_calls, pool_requested_bytes,
pool_raw_allocation_calls, pool_reserved_bytes, aligned_host_cursor, tree_count,
issued_snode_ids, compilation_seconds. Every profile requires stable before/after
values for both independent executions. This excludes direct malloc, page
residency/faults and driver internals; typed payload excludes object/container
metadata, compiler/runtime memory and pool alignment. Host vectors are not pinned.

Setup timing covers both Harness constructors. Loading timing covers both
prepared populations. The count-only timing covers one update call. Cycle timing
covers both executions' structural updates, ticks, full inspections and parity/
metadata checks; it is not isolated kernel throughput or a comparative speed claim.

No numerical field/activity/cascade/geometry/assembly source behavior is converted.
The evidence hashes unchanged numerical source files separately, and the updater's
existing kernel builders/launchers are unchanged. Source-share consistency checks
use host double conversion; native u64-to-f64 weighted arithmetic is still a
Segment B qualification. No GPU execution or PostgreSQL rerun is claimed.
Database code/data are unchanged; #172/#177/#156/#145 remain open.

Reproduce with the [native commands](../../docs/SOURCE-COUNTS.md#cpu-evidence-and-remaining-work).
Verify source fingerprints with `git show PIN:PATH`; later documentation/evidence
commits do not change the tested runtime/test source. No generated binary is retained.
