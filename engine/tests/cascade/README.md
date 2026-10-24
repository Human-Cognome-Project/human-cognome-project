# Upward centroid candidate CPU evidence

Implements the bounded candidate-only component of [accepted #180](../../../docs/upward-centroid-integration-plan.md)
under #172/#177. [Implementation and limits](../../docs/CENTROID-CASCADE.md)
describe the native interface, work/resource accounting and remaining live integration.

Tested runtime/test source: `cb299f481f994364f61adfb910bd2fa86cbe1ed5`.
Baseline merge: `77b0d4b0cb22155224767a1e116c39608c69e83c`.
Matched native backend tree: `6784224121f68de1bf1aa0765b54e80d20e355ad`.
Clang/LLVM 15.0.6, x86_64 CPU-only native build, two workers per cascade mode,
CTest concurrency two. Actual CPU selection is checked in every mode.
All **62 CPU suites passed in 427.34 seconds**, including all eight cascade modes.

| Artifact | Contents |
|---|---|
| [acceptance.json](acceptance.json) | Source/backend/compiler pins, source/binary/raw hashes, eight profiles, manifests, measurements and native counter snapshots. |
| [cpu-ctest.txt](cpu-ctest.txt) | Complete native regression result on the pinned source. |
| [cpu-native-output.txt](cpu-native-output.txt) | All eight cascade sections from that same run, including traces, resource manifests and rejection/scale fixture results. |

The eight profiles cross BulkCopy/CpuDirect, P=70/G=9 versus P=4096/G=257,
and eight bodies with holes/motion versus maximal P/G/E demands. The first pair
also exercises invalid input/numerics, each resource limit and declared source
units. Each profile performs 32 tick/query/inspection cycles. Nonmaximal profiles
exercise all 53 f64 retained-bit counts, one/two/three comparison levels, skipped
levels, same-LoD grouping and both-sign publication traces. Wobble within an
interval is suppressed; crossing and origin bias remain visible. This does not
select production rounding or prove universal jitter suppression.

The oracle uses independent long-double weighted means for each node and
arithmetic quantization, with verified published children as parent inputs.
Unequal masses and cancellation, distinct copies, whole-body membership aliases
versus distinct source ordinals, and no extra grouping distance factor are
checked. Shared Q_b primitives retain their separate precision regression.

Full physical/source/control/activity/history/anchor/Demand snapshots check
candidate success and ordinary rejection. Poisoned BulkCopy staging distinguishes
current native reads. Test-only direct source faults and source-value traces
are not runtime input interfaces. A nonfinite source returns InvalidNumerics
before reduction; unexpected borrowed-binding failure marks execution unready.

Raw native metric columns are: phase, native_requested_bytes,
known_list_chunk_bytes, known_allocator_elements, pool_allocation_calls,
pool_requested_bytes, pool_raw_allocation_calls, pool_reserved_bytes,
aligned_host_cursor, tree_count, issued_snode_ids, compilation_seconds.
Every profile requires identical before/after backing/layout/compiler values.
Linked C++ new/new[] observations are zero during first binding/query/inspection
and repeated boundaries. Arbitrary direct malloc, physical page residency/faults
and driver internals are excluded. Payloads exclude fixed object/vector metadata,
compiler/runtime storage and pool alignment. Ordinary CPU vectors are not pinned.

Query work is logical validation/reduction visits, not instruction count.
Preparation's separately bounded comparisons/packing can be quadratic and scan
R×E ledger pairs; uploaded reserved-capacity bytes are reported independently.
Setup includes compilation/empty warm dispatch. First-query and explicit
inspection timings are separate; the repeated mean includes query plus inspection,
excluding ticks and snapshot/oracle checks. Timings are observations, not throughput
or comparative hardware claims.

No GPU execution/accuracy/allocation/transfer qualification or PostgreSQL rerun
is claimed. Database code/data are unchanged. Whole/disjoint cohorts and explicit
hierarchy are first-component scope, not a model-wide participation/selection rule.
No live field publication, force/wake/phase conversion, motion/contact transaction
or analyst policy is supplied. #172/#177/#156/#145 remain open.

Reproduce with the [native commands](../../docs/CENTROID-CASCADE.md#cpu-evidence-and-reproduction).
Compare source fingerprints with `git show PIN:PATH`; later documentation/evidence
commits do not change the tested runtime/test source. No generated binary is retained.
