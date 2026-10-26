# C2b-1 cumulative publication CPU evidence

Implementation/evidence **for review** under accepted
[#191](../../../docs/cumulative-publication-integration-plan.md).
[Behavior, reader boundaries and limits](../../docs/CUMULATIVE-PUBLICATION.md)
describe the existing single-level trial correction. C2b-2 remains held until
this implementation is accepted; no production hierarchy is installed.

Runtime/test source: `8db7289398b2523519adb3c77ccf7cee852efa3c`.
Accepted plan merge: `e82e6fa72e66d3647c25e13c940ce825a50a0788`.
Native backend tree: `6784224121f68de1bf1aa0765b54e80d20e355ad`.
Clang/LLVM 15.0.6, Release, CPU-only matched build; CTest concurrency two.
Documentation/evidence commits do not change this tested source. Hosted review
and acceptance are separate from this contributor run.

All **88 CPU suites passed in 892.49 seconds** on that source, including the
previous 84 suites. The four new modes passed all 88 bounded cases, 1,280 quiet
ticks and 256 count/focal edit cycles. No GPU or browser execution is claimed.

| Artifact | Contents |
|---|---|
| [acceptance.json](acceptance.json) | Source/backend/compiler pins and hashes, complete regression result, eight profile manifests and four cumulative matrices, numerical witnesses, allocation/backing observations and binary hashes. |
| [cpu-ctest.txt](cpu-ctest.txt) | Full CPU regression, including all 84 previous suites and four added modes. |
| [cpu-native-output.txt](cpu-native-output.txt) | The eight existing numerical-profile modes and four new cumulative modes from that same run. |

The four new modes cross one/two native worker configurations with BulkCopy and
CpuDirect. Each verifies actual CPU selection and executes 22 cases: both-sign
combined field/contact threshold crossing and cancellation, forward/reverse
contacts, full-bit control, five centroid witnesses per sign, quiet velocity/
contact/control recovery, final three-axis radial speed, and two count/focal
reuse fixtures. These are native prepared fixtures, not production admission.

The independent body reference combines force before applying the unchanged
integration/brake. At Q_4, each outward field/contact contribution alone leaves
the target at ±1.5, while the combination publishes ±1.5625. Cancellation gives
zero target movement/speed. A quiet recipient notified after integration must
evaluate changed contact geometry in its next calculation; both index orders
cross prepared holes and the last valid row. No same-type concurrent publication
or centroid-to-centroid installation is being qualified here.

Centroid references use exact binary weights/positions, including 1/4 and 3/4,
repeated ordinals and a sleeping member whose physical mass differs from its
denominator. Individually subthreshold normalized contributions combine across
the field's Q boundary; opposing contributions cancel. Distinct body52/field4
settings expose one final destination Q. Thirty-two explicit all-loaded reference
reductions of fixed fine positions retain the same subthreshold coarse result,
without accumulating a positional tail. This is a test control, not a scheduler.

Each mode includes ten 32-tick quiet windows. They require zero body evaluations,
motion attempts, publications, force/contact work, centroid reductions and
invalidation scans, while physical velocity and final cached speed remain intact.
Inactivity is the selected publication policy, not zero velocity or exact force
equilibrium. A later parameter/control/contact input must wake relevant work.
Public `invalidate()` still rejects ticks by withdrawing readiness.

Each mode also performs 32 same-f32-share count edits at an unchanged Q centre,
requiring two affected-body evaluations per edit (64 total), and 32 derived-focus
changes, requiring all three bodies including an unrelated radial recipient to
evaluate (96 total). Committed edits preserve survivor physical state and previous
published history. Upload/readback bytes and total elapsed including inspection
are retained. Prepared focus replacement remains unsupported; source edits
change the derived focus through existing supported operations.

The eight profile modes retain 32 tick/query/commit/rebind cycles across both
host paths, P=70/G=9 and P=4096/G=257, padded holes and full source demand.
They preserve typed source, numerical, history, optional query/geometry and
failure/reuse coverage. Their four Q_4/Q_0 watch traces now require one attempt,
zero publications, two initial body evaluations and no continuing work over
32 ticks. Historical [C1 evidence](../numerical_profile/README.md) and
[C2a evidence](../native_handoff/README.md) retain their original source pins.

No numerical array or native activity set is added. Binding the existing speed
array costs one trial integration argument. On this CPU ABI the conservative
binding reservation increases 64 bytes and retained binding metadata increases
56 bytes; all 22 numerical shapes retain their widths/extents. Main-profile
scratch is 202,360 bytes at P=70/G=9 and 9,601,720 bytes at P=4096/G=257;
actual retained binding bytes are 11,576. The smaller new fixture without optional
query/geometry capacity has S=141,744 and 8,488 binding bytes. The ordinary trial
tick omits one determiner dispatch; setup still prepares that kernel.

Raw before/after metric columns are native requested bytes, known list chunks,
known allocator elements, pool allocation calls/requested/raw calls/reserved,
aligned host cursor, tree count, issued SNode IDs and compilation seconds.
Each case/profile requires unchanged measured backing/layout/compiler counters.
Guarded first population, tick/inspection and update operations report zero
linked C++ new/new[] including workers. Test snapshots/fixtures, arbitrary
direct malloc, driver internals, physical page faults and residency are outside
that claim. Existing manifests charge typed host/native copies separately;
compiler/runtime/fixed object metadata and pool alignment are separate. Ordinary
host vectors are unpinned; no GPU memory-ordering or performance claim is made.

The 224-byte trial tick-report readback and typed full inspection shapes are
unchanged. Timings include concurrent CTest contention and distinguish setup,
tick/inspection, quiet windows and update/reuse costs; they are observations,
not throughput guarantees. No report/browser surface changed, so browser tests
were not rerun. The full CPU regression includes existing native capture tests.

Q remains experimental toward-zero significand clearing. Its inward/outward cell
bias is explicit; opposing-force controls do not assume both isolated steps stay
in the old cell. Native FTZ/finite-domain limits remain unchanged. Neither
epsilon sleep rules nor hidden positional residue recovery is introduced.

Reproduce using the [native commands](../../docs/CUMULATIVE-PUBLICATION.md#resources-and-reproduction).
The full coupled multilevel math/response, #156 operational CPU exit and #145
GPU qualification remain open. No database/cache-manager/analyst change is included.
