# Native geometry preflight CPU evidence

This is the first portion of Segment 2 of the accepted #170 plan under #162/#156.
The [implementation contract](../../docs/PREPARED-GEOMETRY.md) distinguishes
current-state geometry queries from the remaining placement, live contact
installation and candidate-tick transaction. No GPU gate is claimed.

Tested source: `519ae7916b96d06d1c1c33b1e3c415bf13ceb5d7`.
Baseline #171 merge: `c6b85de0d513dcc8d60fa38bdcfb8d1640c07e66`.
Matched native backend tree: `6784224121f68de1bf1aa0765b54e80d20e355ad`.
Clang/LLVM 15.0.6, CPU-only build, four workers for each geometry mode and
CTest concurrency two; existing regression suites keep their own worker settings.
All 52 suites passed in 378.65 seconds on the final runtime/test source.

| Artifact | Contents |
|---|---|
| [acceptance.json](acceptance.json) | Source/backend pins, source and built-artifact hashes, eight profile manifests, timings, counter snapshots and observation limits. |
| [cpu-ctest.txt](cpu-ctest.txt) | Complete 52-suite CPU regression result, including all eight geometry modes. |
| [cpu-native-output.txt](cpu-native-output.txt) | Unedited geometry-mode sections from that same CTest run: commands, successful exit status, native counters, manifests and first populated-query times. |

Both host paths exercise P=70 and P=4,096 with sparse native holes/final rows,
sleepers, movement and stale observations. Four maximum modes populate every
P/G/E/C row and verify source pair orientation/order plus complete two-sided
CSR. Sparse modes run 32 further tick/query cycles each (128 altogether).
The small modes additionally cover independent C/R exhaustion, work limits
before/after discovery, C=0, exact tangency, coincident zero/positive extents,
unrepresentable finite-payload geometry, missing focus and disabled geometry.
The final binary contains these boundary assertions; their lack of individual
logging is not permission to omit them when reproducing the test.

Each measured query/inspection and ordinary bounded rejection observes zero
linked C++ new/new[] calls. Native requested bytes, known map/activity list
chunks/elements, pool counters/reservation, aligned cursor, tree/SNode IDs and
compilation counters remain unchanged from setup through fixture first use
and reuse. Those fixed native declarations are not a second scheduler.

The work formula counts unordered pairs and source/union rank comparisons;
additional bounded linear compaction/validation/CSR work is documented
separately. Maximal profiles deliberately measure the simple quadratic
baseline. Timings include the local synchronous boundary and are not a scaling
or throughput promise. Normal queries return only 272 summary bytes; explicit
candidate inspection copies prepared metadata arrays separately.

These are CPU observations, excluding arbitrary direct malloc, physical page
residency/faults and driver internals. Scratch excludes fixed C++ object/container
overhead, compiler memory and separately measured native root/pool alignment.
Ordinary vectors are not pinned. Database contracts/data are unchanged; this
run does not rerun PostgreSQL acceptance. The complete integrated CPU operation
matrix and local GPU/driver/transfer tests remain later work.

Reproduce with the [documented native commands](../../docs/PREPARED-GEOMETRY.md#cpu-evidence-and-remaining-work).
Source fingerprints can be checked against the tested commit using
`git show PIN:PATH`; later evidence-only commits do not change runtime code.
