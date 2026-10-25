# Weighted centroid candidate CPU evidence

Segment B of [accepted #183](../../../docs/weighted-centroid-accounting-plan.md),
following accepted #184. [Implementation and limits](../../docs/WEIGHTED-CENTROID-CASCADE.md)
describe the explicit source-row interface and exact compatible-coverage rule.
This is candidate-only evidence for review. It changes no live physics and does
not make a complete candidate evaluation a required per-tick solver.

Tested runtime/test source: `58ccae54da69bede55b49e6e3bc8e05eb58f3f8e`.
Baseline #184 merge: `eca654b3e1a5c2a00e1a96d06c7db15d450e8fe3`.
Matched native backend tree: `6784224121f68de1bf1aa0765b54e80d20e355ad`.
Clang/LLVM 15.0.6, x86_64 CPU-only native build, two workers per weighted mode,
CTest concurrency two. Every weighted mode checks actual CPU selection.
All **74 CPU suites passed in 454.04 seconds**, including eight weighted modes
and the original whole-body, source-accounting, precision and live-field suites.

| Artifact | Contents |
|---|---|
| [acceptance.json](acceptance.json) | Source/backend/compiler pins, source/binary/raw hashes, eight profiles, observed widths, full manifests, timings and native counter snapshots. |
| [cpu-ctest.txt](cpu-ctest.txt) | Complete 74-suite result on the pinned source. |
| [cpu-native-output.txt](cpu-native-output.txt) | All eight weighted sections from that same regression, including ratios, publication traces, resource and rejection results. |

The matrix crosses BulkCopy/CpuDirect, P=70/G=9 versus P=4096/G=257, and eight
bodies with holes/motion versus full P/G/E/R/C/N/H/L demands. Each profile performs
32 tick/query/inspection cycles. Nonmaximal modes also test all 53 retained-bit
profiles, full/partial direct comparisons, compatible two/three-level overlaps,
skipped levels, repeated source ordinals and same-LoD grouping. Both-sign traces
show same-cell suppression and boundary crossing with no tail recovery.

Integer coverage is checked independently by full occurrence handle and source
row/count/denominator. A numerical long-double reference uses current physical
mass times each exact count ratio at leaves and verified published child centres
and participating masses at parents. It bounds ordinary conversion, product,
sum and centroid error; integer accounting uses no epsilon. The native unsigned
conversion probes use an independent integer-bit nearest-even binary64 encoding
oracle, including counts around 2^53, 2^63 and UINT64_MAX. Ratio relative error is
bounded by four binary64 epsilons. JSON retains u64 counts as decimal strings.
The thirds result `0.33333333333333331` differs from the legacy f32 share
`0.3333333432674408`; the new path does not pass through that narrowed share.

Supplemental small profiles cover unknown counts/denominators, foreign/stale/
retired/recycled handles and rows, source compaction and count-only edits with
unchanged rounded shares, aliases versus distinct ordinals, mismatched coverage,
both overflow paths, unsupported profiles, independent limits and disabled
weighted reservation. The legacy whole-body API still accepts unknown accounting
and rejects fractional/ambiguous participation. Real native assembly preparation
supplies a/b/c counts plus an independent b copy, three comparison levels and
source units 1/64 through primary/compression/naming bindings, without another
distance multiplier.

Snapshots preserve physical arrays, exact source metadata, full handles/ledgers,
focus/anchor, native activity/history and Demand across success and ordinary
rejection. Poisoned BulkCopy staging proves current native reads. Nonfinite bodies
return InvalidNumerics before reduction. Deliberate test-only native share
corruption contradicting pinned source accounting makes execution unready.

Raw metric columns are: phase, native_requested_bytes, known_list_chunk_bytes,
known_allocator_elements, pool_allocation_calls, pool_requested_bytes,
pool_raw_allocation_calls, pool_reserved_bytes, aligned_host_cursor, tree_count,
issued_snode_ids, compilation_seconds. Every main profile requires identical
before/after values. First bind/query/inspection and repeated boundaries observe
zero linked C++ new/new[] calls. This excludes arbitrary direct malloc, physical
page residency/faults and driver internals. Typed payload excludes fixed object/
vector metadata, caller inputs, compiler/runtime storage and pool alignment.
Host vectors are ordinary memory, not pinned.

Setup includes compilation/empty warm dispatch. Bind separately reports bounded
host preparation visits and full-capacity uploads; its scans can be quadratic.
Query work counts logical visits, not every instruction or arithmetic operation.
A successful query reads 184 bytes with two explicit synchronizations; inspection
readback is separate. The repeated mean measures query plus inspection, excluding
ticks, snapshots and oracle work. Timings are bounded observations, not hardware
throughput or GPU performance claims.

No PostgreSQL or GPU acceptance rerun is claimed; database code/data are unchanged.
General overlap matching, live precision/force/wake/ripple settling and coherent
motion/contact integration remain outstanding. #172/#177/#156/#145 stay open.
Reproduce with the [native commands](../../docs/WEIGHTED-CENTROID-CASCADE.md#cpu-evidence-and-reproduction).
Compare fingerprints with `git show PIN:PATH`; the later documentation/evidence
commit changes no tested runtime/test source. No generated binary is retained.
