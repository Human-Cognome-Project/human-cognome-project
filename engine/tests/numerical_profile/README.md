# Live trial numerical profile CPU evidence

C1 implementation accepted in #187 under accepted [#186](../../../docs/live-numerical-profile-plan.md).
[Behavior, resources and limits](../../docs/LIVE-NUMERICAL-PROFILE.md) describe
the selected live f64 path. The default legacy f32 path remains unchanged.
Live hierarchy and the #156 CPU operation exit remain separate acceptance work.

Runtime/test source: `61c3dd636deac7c422e51461d38144934991f229`.
Accepted plan merge: `d21a4b5d0dfa7d9b7074a5bf5707382576d163f6`.
Matched native backend tree: `6784224121f68de1bf1aa0765b54e80d20e355ad`.
Clang/LLVM 15.0.6, x86_64 CPU-only build, two native workers per new mode,
CTest concurrency two. Each new mode checks actual CPU selection.
The later evidence/documentation commit does not change tested source.
Maintainer verification on reviewed PR head `fd3c5208ec37046ef2d08346fd5db4d0ba89474f`:
all nine hosted checks passed; inspected native CPU job `121160291530` reports
**82/82 suites in 685.51 seconds**. This is hosted evidence, not a maintainer-local
or GPU run. The acceptance/status-only commit changes no runtime/test source.

Contributor run: all **82 CPU suites passed in 524.92 seconds**. The 14 viewport model/schema
tests and bundle build passed separately on the same pinned source.

| Artifact | Contents |
|---|---|
| [acceptance.json](acceptance.json) | Exact source/backend/compiler pins, source and binary hashes, eight profiles, full typed inventories, timings, transfers and before/after native counters. |
| [cpu-ctest.txt](cpu-ctest.txt) | Complete 82-suite regression, including the previous 74 suites. |
| [cpu-native-output.txt](cpu-native-output.txt) | All eight new modes from that same run, with manifests, numerical shapes, watch-point traces and behavioral results. |
| [viewport-tests.txt](viewport-tests.txt) | Fourteen model/schema tests and the optional viewer bundle build. Native snapshot serialization is included in the CPU regression. |

The matrix crosses BulkCopy/CpuDirect, P=70/G=9 versus P=4096/G=257,
and eight bodies with holes/final rows versus full particle/participation
capacity. Each main profile uses body/field b=48 and runs 32
tick/query/inspection/commit/rebind cycles. All G records are declared; only the
three source fields contain members. These are prepared numerical fixtures,
not production placement or a populated language-store exercise.

Useful 2^-30 accepted motion survives at x=1, remaining below one f32 ULP over
all 32 ticks. Ordered exact-count mass/centres are checked against independent
long-double references with the declared publication cut. A separate long-double
scalar step checks fractional force, f64 parameters, brake/integration and
both-sign Q_32 publication. Full b=52, body52/field4 and Q_4/Q_0 cases cover
typed starts, different publication grids, within-cell attempts and crossings.

Small modes also exercise thirds, repeated ordinals, physical mass different
from structural denominator, same-f32-share count-only numerical repair,
assembly-derived letter counts and independent occurrences. A solitary
occurrence with repeated ordinals and a coarser field centre has no fictitious
self-force. Collective accepted member motion crosses a field publication cell
through fresh reductions including a sleeping member. Selective and all-loaded
f64 executions agree over 32 combined field/contact/radial-control ticks.

The persistent-activity watch cases record 32 displacement attempts, zero
published changes and 33 body evaluations (two initially, then one per tick),
with zero centroid reduction edges. Physical velocity survives, no positional
tail is recovered, and one native particle remains awake. This is positional
quiescence with continuing activity, not complete computational settling.
Elapsed cost is recorded separately for each host path and precision. Changing
force crosses a boundary, exact fixed state consumes activity, and a later
contact wakes/evaluates both index sides. These are bounded observations, not a
permanent activity policy or a proof of natural clustering.

The 32-cycle watch runs took 8.56–10.97 ms including tick reports and explicit
fixture inspection. Main-profile mean ticks were 0.192–0.598 ms under concurrent
CTest execution. Scratch reservations are 202,296 bytes at P=70/G=9 and
9,601,656 bytes at P=4096/G=257, including binding reserve; actual retained
binding metadata is 11,520 bytes in both. Full-capacity source edits at P=4096
average 7.46–7.69 ms, including validation, transfers and coherent commit.
These costs remain visible rather than being described as free declarations.

Exact source coverage rejects unknown/inconsistent metadata and checked u64
overflow before publication. Rejection snapshots cover numerical/source data,
full handles/relationships, focus/anchor, activity/history, Demand and readiness.
Successful ordinal-only commits preserve survivor physical/history arrays;
the existing commit boundary may refresh focus from current published fields.
Thirty-two retirement/reuse cycles preserve another occurrence and reject a
retired handle even after its row is reused. Stale source generations and
candidate tokens reject. Poisoned BulkCopy staging proves current native query
reads. Native source corruption and invalid post-mutation arithmetic make the
execution unready. Independent profile/descriptor/scratch/work limits and an
empty trial without optional geometry/hierarchy are covered.

Raw metric columns are: phase, native_requested_bytes, known_list_chunk_bytes,
known_allocator_elements, pool_allocation_calls, pool_requested_bytes,
pool_raw_allocation_calls, pool_reserved_bytes, aligned_host_cursor, tree_count,
issued_snode_ids and compilation_seconds. Each main mode requires identical
setup/after-cycle values and zero observed linked C++ new/new[] in guarded first
and repeated runtime boundaries. This excludes arbitrary direct malloc,
driver internals, page residency/faults and caller/test snapshots.

The scratch manifest sum is checked against S. Numerical shapes and manifest
terms independently equal their actual count/width/copy products. The f64 body,
field, motion, control and repair surfaces coexist with f32 source shares,
u64 source descriptors and unchanged integer metadata. Committed/prospective
descriptor peak is charged in four copies; there is no populated f32 shadow
simulation. Core numerical shapes separately inventory host/native copies:
primary staging has an extra owned native copy in BulkCopy, whereas CpuDirect
borrows it; Activity shapes are native with separate host workspaces in S.
S includes unused binding reservation, not resident RAM. Native map roots and
fixed call-frame arrays are separate. Fixed object sizes and inactive small focal objects are separate
from selected payload. Host vectors are unpinned. Setup includes compilation
and empty warm dispatch; timings under CTest concurrency are not throughput or
GPU measurements.

The report-value round trip preserves `1+2^-40` and explicit numerical metadata
in native JSON/browser validation. Legacy snapshots without metadata still use
f32 validation. ActiveSession/FixedStudy still create legacy execution; this
does not implement an f64 operational owner or qualify browser rendering
precision. No browser interaction rerun or GPU qualification is claimed here.

Source f32 values do not regain lost detail when widened. Raw counts convert
to native f64 before division, with ordinary conversion/reduction error and
exact host integer coverage. Trial Q is experimental toward-zero bit clearing,
with origin/crossing bias; no production rounding or per-LoD ladder is selected.
Finite normal-or-zero guards do not undo the backend's FTZ mode or detect every
already-flushed arithmetic underflow. No general gradual-underflow claim is made.

No database changes, new scheduler, live child-parent propagation, inherited
response or mandatory per-tick full candidate solve are included. Production
admission, moving-contact transactions, rebase, the #156 CPU exit and local GPU
qualification remain open under their existing contracts.

Reproduce using the [native commands](../../docs/LIVE-NUMERICAL-PROFILE.md#evidence-and-remaining-work).
Compare hashes with `git show PIN:PATH`; no generated native binary is retained.
