# Native precision prerequisite CPU evidence

Implements the first slice of [accepted #176](../../../docs/native-precision-truncation-plan.md)
under #172. [Implementation](../../docs/NATIVE-PRECISION.md) and
[source/state audit](../../docs/PRECISION-STATE-AUDIT.md) retain the boundary
between these primitives and the later live precision/LoD-centroid conversion.
Toward-zero clearing is experimental; production rounding remains open.

Tested runtime/test source: `ea6c7c2f1635dab67d006cf3d9d8c085b9cc8abc`.
Baseline: `925d8c7ac8175ca6afe399c631fd35e710d4cfcc` (#176 merge).
Matched native backend tree: `6784224121f68de1bf1aa0765b54e80d20e355ad`.
Clang/LLVM 15.0.6, CPU-only native build, two workers per precision mode,
CTest concurrency two. All **54 CPU suites passed in 356.47 seconds**.

| Artifact | Contents |
|---|---|
| [acceptance.json](acceptance.json) | Source/audit/backend pins, source/binary/raw hashes, sixteen boundary/arithmetic profiles, logical host/backend payload and binding bytes, first-query timing and native counter snapshots. |
| [cpu-ctest.txt](cpu-ctest.txt) | Complete 54-suite native regression result, including the two new precision modes. |
| [cpu-native-output.txt](cpu-native-output.txt) | Precision mode sections from that same run: actual backend, four calculation/store pairings on each host path, results and measurements. |
| [cpu-cuda-refusal.txt](cpu-cuda-refusal.txt) | Exit 1 for requested CUDA on this CPU-only build; not GPU execution. |

Boundary probes check **99,792 result cells**, including every legal retained-bit
count in both containers and 32 recurring dispatch/readbacks per profile.
Arithmetic probes run another 32 scalar-update/discard cycles per pairing/path,
plus explicit boundary traces where the chosen calculation can represent them.
Runtime inputs prevent a constant-folded result from substituting for the
calculation/transport test. Each kernel has an independent f32 diagnostic beside
its explicitly selected positional calculation width.

An independent long-double arithmetic oracle checks trimming; known conversion
encodings separately check native normal ties and the declared unsupported
narrowing range. The matched fork forces FTZ. Nonzero f64→f32 inputs below
`2^-126` are conservatively rejected, including cases that could round up to the
smallest normal or down to signed zero. Same-container stored subnormals and
f32-subnormal promotion are checked, but arbitrary gradual-underflow arithmetic
is not qualified. Global floating-point mode is unchanged.

The scalar probe preserves useful f64 cancellation/increment detail, distinguishes
combined calculation from intermediate filtering, discards repeated subthreshold
positional detail, preserves an independent velocity value and records crossing
behavior on both signs. It does not prove live centroid/activation behavior or
universal stability. #177's cascade and #172's live conversion remain unbuilt.

Prepared first-nonempty and recurring probe boundaries observe zero linked C++
new/new[] calls. Known native requested/pool/cursor, tree/SNode and compilation
counters remain stable. These probes add no SNode trees; list/element counters
are therefore empty, rather than a test of new sparse allocation. Observation
excludes arbitrary direct malloc, physical residency/faults and driver internals.
Payload totals exclude C++ object/container overhead, compiler memory and native
pool alignment. CPU direct buffers are ordinary borrowed vectors, not pinned.

The evidence script verified 26 live field/assembly source files unchanged from
the audit baseline. Source hashes identify the audited inputs; they do not prove
the future conversion. No database code/data or retained stores were changed,
and no PostgreSQL acceptance rerun is claimed.

Reproduce using the [native commands](../../docs/NATIVE-PRECISION.md#cpu-qualification-and-reproduction).
Check source fingerprints with `git show PIN:PATH`; later documentation/evidence
commits do not change the tested runtime/test code. No generated binary is retained.
