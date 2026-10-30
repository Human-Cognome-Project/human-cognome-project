# C3-1 coherent dynamics and active capture

The [C3-2 combined refresh evidence](refresh-v1/README.md) is a separate
implementation accepted in #198 under #197. It retains its own source, full CPU matrix,
compiler modules and integration provenance; the historical records below are
unchanged. C3-2 CPU acceptance is complete; CUDA refresh qualification remains
separate. Historical held/failure statements below describe their pinned runs.

Latest compiler-audit revision: `3eee41e`, with target/optimizer metadata at
`d762111`. The [separate portability evidence](compiler-v3/README.md) retains
the original hosted 92/96 count failure, actual local/hosted optimized modules,
semantic path/packet/ownership checks, expanded controls and the default/scalar
matrices. Local regression passed 96/96 in 1570.98 seconds; fresh hosted
regression passed 96/96 in 2062.37 seconds, followed by all four scalar modes,
180 PostgreSQL integration checks and CPU backend verification. This test-only
correction changes no engine physics, quantization, activity or capture behavior.
The v2 observations below retain their original source pins and results.

Current contract: [maintainer direction](../../../docs/coherent-dynamics-gpu-snapshots.md),
recorded in `ea7b828` / `c644575`. Runtime/test/capture source:
`85ba956917a413d6a4a7d378a1ce6041be557d35`. This is a separately versioned
acceptance revision; the v1 artifacts and their failures below remain unchanged.

All **96/96 CPU suites passed in 1314.32 seconds**. Four native modes completed
all **660 moving windows** (initial + 32 perturbations for each of five fixtures
per mode): **645 settled observations and 15 evolving observations**. All hard
invariants passed. The 76 signed/cell-boundary/cancellation probes passed; held
convergence/quiet and prepared reuse checks remained enabled.

Fresh PostgreSQL-to-active integration passed **180 checks**, with zero private
reads and unchanged restored cold-source fingerprints. Actual CPU backend checks
passed. The 128-tick same-execution capture retained 17 snapshots, f64 values and
per-node overrides; the viewer passed 87 browser checks and 15 model tests.
CUDA-enabled fork/client builds and their CPU ABI capture passed. **Actual CUDA
execution is unqualified; C3 CUDA requests remain explicitly rejected.**

Moving observations end at 4,096 ticks or 32 consecutive quiet ticks, then the
next declared perturbation continues from the same live state, including all
pending bits and physical velocity. Every fixture completes the initial window
and 32 perturbations. Settled/evolving is an observation status. A finite-domain,
arithmetic, packet/history, source identity, lost-input or resource violation
still fails. The terminal held-body intervention runs after all reuse checks;
it never repairs an intermediate moving state. Held-input 16/8 convergence and
justified quiet controls remain hard requirements. No physics/Q setting was tuned
for stillness. Signed toward/away, cell/exponent/zero-boundary and cancellation
probes retain the absolute publication error in a declared local Q8 ULP.

| Artifact | Contents |
|---|---|
| [coherent-v2.json](coherent-v2.json) | New pins/hashes, four mode summaries, observation status, invariants, resource reuse, integration/backend/browser checks and CUDA build limits. |
| [cpu-ctest-v2.txt](cpu-ctest-v2.txt) | Full revised CPU regression. |
| [cpu-native-v2.txt](cpu-native-v2.txt) | All four revised native mode outputs. |
| [integration-v2.txt](integration-v2.txt) | Fresh restored-source PostgreSQL-to-active integration and unchanged cold fingerprints. |
| [backend-v2.txt](backend-v2.txt) | CPU backend verification. |
| [capture-v2](capture-v2/) | First/final/latest snapshots from the same active C3 stream, native capture costs and browser evidence. |

The [handoff](../../../docs/c3-cpu-capture-handoff.md) gives exact build/run commands
and separates compiled, executed and unexecuted CUDA work. C3-1 still requires
maintainer acceptance; C3-2 is held.

---

# Historical v1 CPU investigation

Status: **draft for review; coupled settling acceptance is blocked**.
Plan: [#194](../../../docs/production-hierarchy-response-plan.md).
Implementation: [fixed installed hierarchy](../../docs/LIVE-HIERARCHY.md).

Runtime source: `bb934d642975a18c1c354e237eed0530780a01cf`.
Final test source: `eeb8ce9a3fb035aca4154d039877339b1ac6dc32`.
Accepted baseline: `004aaea0f2a2ae1381027425721e39ca3cb18af5`.
Backend tree: `6784224121f68de1bf1aa0765b54e80d20e355ad`.
Clang/LLVM 15.0.6, Release `-O3 -DNDEBUG`, CPU-only, strict native math.
The two later test commits add field-order coverage and full-round diagnostics;
they change no runtime or earlier CPU suite. Documentation/evidence changes
follow these pins. Contributor runs are separate from hosted checks.

## Interpretation note — 2026-10-30

The raw output and original test confirm that each failed moving window completed
4,096 ticks before failing its settling criterion, not a CPU processing timeout.
Observed window elapsed times were 710.63–1,679.86 ms, including inspection and
reference checks; accumulated tick times were 505.34–1,270.44 ms. These three/four
body fixtures support Patrick's observation of practical CPU calculation speed,
without establishing large-population throughput or a controlled path speedup.
See the [verified timing table](../../../docs/coherent-dynamics-gpu-snapshots.md#historical-tick-limits-and-cpu-execution--2026-10-30).
Original failure results remain unchanged.

## Results and blocker

The original **96/96 CPU suites passed in 1296.85 seconds** on the runtime
source pin, before adding the moving field-order witness. That successful run
does not clear the final test source's gate.

Final-source native modes (4/4 failed):

| Workers / path | Mode result | Failed 4,096-tick gate |
|---|---|---|
| 1 / bulk | FAIL | three_level_overlap_permuted, round 23 |
| 1 / direct | FAIL | three_level_overlap_permuted, round 23 |
| 2 / bulk | FAIL | two_level_permuted, round 6 |
| 2 / direct | FAIL | two_level_permuted, round 2 |

All standard two-level, three-level overlap and letter/byte fixtures completed
their initial settle and 32 recoveries in these final modes.
The additional field-order gate remains failed. Completed independent arithmetic,
publication, compiler, isolation and resource checks remain separately visible.
The final default CTest modes therefore include a known failing test; this is
a draft contribution, not an accepted C3-1 release.

Changing only native field records changes the order in which a single worker
encounters an active parent and its children. It does not change the calculation
tree, source counts, initial coordinates, contact pair, focus, gravity, timestep,
body/field Q or recovery procedure. Different trajectories are permitted; failure
to meet the declared settling gate remains a failure. No result here establishes
unbounded divergence or a permanent limit cycle.

At a failure, the test holds bodies fixed and continues only pending centroid
work. Reference agreement after that continuation distinguishes this observed
failure from a hierarchy that cannot converge on stationary inputs. It does not
prove every concurrent delivery case or identify the coupled numerical cause.
The retained final 64 samples and full-round force/speed/position extrema,
published travel and direction/velocity reversals support further diagnosis.
No physics, precision policy, activity threshold or tick limit was changed to
turn a failed result into a pass. C3-2 remains held.

For the one-worker overlap witness at recovery 23, body 0 records 291 velocity
reversals and a maximum published step of 314.671875. Its final two samples move
from -25.875 to -25.8125 with final velocity about 0.000719 and dt = 0.5. Clearing
fraction bits in the resulting negative position produces that next cell toward
zero; the exact current-Q reference agrees. This illustrates the strongly coarse
Q_8 fixture's publication behavior, not a demonstrated cause or proposed change
to the accepted precision primitive. Near-centroid response and publication lag
also remain part of the coupled system under investigation.

| Artifact | Contents |
|---|---|
| [investigation.json](investigation.json) | Source/artifact hashes, build pins, separate regression and final mode results, moving/held records, compiler audit summaries, resource inventory and limits. |
| [cpu-ctest.txt](cpu-ctest.txt) | Complete 96-suite regression on the original runtime/test pin. |
| [cpu-native-output.txt](cpu-native-output.txt) | All four final-source mode outputs, including failed gates and diagnostic traces. |

## Reproduction and coverage

Run after the standard native build:

```sh
cmake --build engine/build -j3
ctest --test-dir engine/build -R '^live_hierarchy_' --output-on-failure -j2
ctest --test-dir engine/build --output-on-failure -j2
```

For the isolated ordering witness:

```sh
TI_LIB_DIR="$PWD/engine/build/taichi-runtime" \
  engine/build/live_hierarchy_test --threads 1 --case moving_permuted
```

The final four modes cross one/two native workers with BulkCopy/CpuDirect and
assert actual LLVM/x64 selection. Tests exercise accepted force examples;
distinct occurrences, ordinals/aliases and compatible overlap; partial root
coverage and counts above 2^53; singleton/unary/same-level contexts; cumulative
crossing/cancellation; mass-only propagation; a permuted 70-node tree across
mask/pointer boundaries; every actual compiled selector/packet reader; rejected
installation/updates; post-mutation unready behavior; and first-use/reuse resources.

Held-input checks retain the 16-poll limit plus eight quiet polls. Moving tests
retain the 4,096-tick limit plus 32 quiet observation ticks and 32 perturbation/
recovery rounds without resetting publications. Both field layouts use body
Q_8 and field Q_6 (retained fraction bits inside f64 containers), G =
0.0771063443835106, dt = 0.5, contact stiffness = 1, and radial reach strength = 1.
These deliberately coarse test settings are not a recommended production policy.
Full input declarations precede execution in the [test](../live_hierarchy_test.cpp).

The letter/UTF-8 fixture uses native normalization, projection, membership and
assembly preparation over existing bounded seed fixtures: English A as an
aggregate and Dutch accented i as its two UTF-8 byte parts, with intrinsic test
geometry and common-frame conversion. It exercises nonzero local/inherited
forces, contact and radial damping. It is not a retained-database ingestion or
operational admission test.

Independent test categories continue after a failure, keeping the mode's exit
status nonzero. Within a moving category a failed two-level fixture skips that
category's subsequent three-level fixture. The other moving layout and letter
category still run. Per-round reuse checks are reached only if that fixture
completes all 32 recoveries; a failed fixture does not claim them.

## Numerical and publication checks

An independent long-double ordered force sum is checked with a sum-of-magnitudes
floating error bound. The independent long-double velocity equation uses the
verified native combined force; its error bound includes pre-cancellation
intermediate magnitudes in braking/radial damping. Position publication must equal
one Q of the old position plus verified final native velocity times dt. A different
high-precision cell at a rounding boundary is counted, not used as an activity
tolerance. None of these reference bounds select work or sleep in the engine.

Per-node cumulative visits/publications enforce one writer/publication per poll.
Old selected slots remain unchanged through that poll. Each parent output must
match a combination of complete pre/post child packets; synchronized history
and mirrors are checked independently. The compiler audit attributes every
required selector and its four packet loads, rather than relying on an aggregate
atomic count. It rejects cloned-module demotion, plain load and missing-reader
controls. This is actual retained Program LLVM, not disassembly or CUDA ordering.
The output does not serialize every individual acquired input as a trace.

The centroid-only continuation after a failure is a bounded test diagnostic,
not an ordinary tick, replacement scheduler or passing coupled settling result.
Physical state remains bitwise unchanged throughout it. Quiet published positions
do not imply zero physical velocity or force under the accepted C2b-1 policy.

## Resources and limits

Raw output distinguishes typed payload, fixed objects, binding/cache bytes,
setup/install cost, ordinary tick cost and observation/reference overhead.
At fixture capacity, the new arrays use 60,344 native and 79,072 host bytes;
publication/selector storage is 4,760 native bytes, and old history 2,240.
The full source scratch manifest is 504,028 bytes, including 16,624 bytes of actual
bindings and 2,960 unused binding reserve. Fixed objects, inline counters,
compiler/runtime storage and pool reservations are separate and must not be
naively added to payload totals. Existing borrowed source surfaces are explicit.

Known native requested/list/pool/cursor/tree/compiler metrics and guarded linked
C++ new/new[] checks retain their limits: they do not account for arbitrary malloc,
page faults, driver residency or complete process memory. Host vectors are not
pinned. Ordinary kernel time is separate from synchronization, readback and
reference overhead; concurrent runs are not throughput promises.

The final native test was compiled and linked separately against the unchanged
runtime libraries while the original pinned CTest run finished. It did not replace
any binary used by that run. The JSON retains the exact compile/link arguments.
Hosted CI was not awaited; no browser interface changed. No database writes,
GPU execution or #156 operational completion are claimed. Full GPU qualification
remains under #145, after the #156 CPU operation exit and separate C3-1 acceptance.
