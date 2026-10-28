# C3 compiler audit portability

Correction source: `3eee41eb8faa6a6ccb8a9ca688b8c8060e52d30c`.
Metadata-only revision: `d7621117cc03b62eac498c50093761434b95c5ec`.
All 24 local optimized modules are byte-identical across those revisions;
see [module identity check](metadata-module-comparison.json).
Backend tree: `6784224121f68de1bf1aa0765b54e80d20e355ad`.
This changes test auditing and evidence collection, not engine physics,
publication, quantization, work selection or active-stream capture.
C3-1 still requires maintainer acceptance; C3-2 is held and CUDA is unqualified.

## Failure and comparison

The [historical hosted log](hosted-count-failure-v2.txt) retains run
40604795329/job 122541493905 on PR head `7e03e93` (checkout merge
`e4269129f2bf93b648063c445d2d8a7dca1d466b`). It passed 92/96 suites in
1225.44 seconds. All four hierarchy suites rejected one force child/parent
reader pair because the audit required three pairs; integration/backend were
skipped. Both observed readers were SC and had complete packet loads.
The full optimized modules from that earlier run were not retained.

The diagnostic collector at `f048461` retained actual optimized modules on
hosted Clang/LLVM 15.0.7, run 40697430088,
artifact `native-c3-compiler-ir`/12820839140. Those new default force/poll
modules have the same unrolled shapes as the local 15.0.6 build. In all four
modes, force, poll and control IR are identical after removing only the
`llvm.ident` compiler-version string; see [comparison hashes](initial-module-comparison.json).
The patch version alone is therefore not established as the cause of the
earlier different shape. The original failure is not relabelled or reproduced
by claiming these newer modules came from it.
That diagnostic run was cancelled after retaining the modules, as was the
superseded `3eee41e` run 40703217310. Neither is final hosted acceptance evidence.

Final-source hosted capture at `d762111` records checkout merge
`cda8be13689e5e1f7b1de3acdbf68d8e7587d68d`, Clang/LLVM 15.0.7 and detected
CPU `znver3`, run 40706207825/job 122878906581. Its
`native-c3-compiler-ir` artifact 12825935787 retains all eight configurations
(default/scalar × one/two workers × bulk/direct), their full optimized modules,
manifests and passing LLVM-verified controls. All 24 modules are identical to
the corresponding local 15.0.6 outputs after removing only `llvm.ident`;
see [final comparison](final-module-comparison.json). Local parsing/auditing
of all eight retained hosted configurations and their controls also passed;
see [cross-configuration output](cross-hosted-final.txt).
The [per-configuration control summary](module-controls.json) records all
480 hosted negative rejections, 32 positive passes and 24 explicit ABI checks.

LLVM's test options `-unroll-count=1 -unroll-max-count=1
-unroll-full-max-count=1` produce real scalar optimized modules from the same
production kernel builder. `--compiler-no-unroll` selects this test control.
It recreates the earlier one-pair reader shape without editing production IR,
changing physical laws or accepting a whitelist of counts.

| Emitted path | Default compiler shape | No-unroll control | Required semantics |
|---|---|---|---|
| Force response | optional one-iteration prologue, then two iterations per loop body | one iteration per loop body | every influence in `[offsets[0,body], offsets[1,body])`, child/parent columns 1/2 from the same row |
| Parent child reduction | four iterations per main body plus scalar remainder | one iteration per body | child list `[ChildBegin, ChildBegin+ChildCount)`; complete acquired XYZ/mass packet for each child |
| Leaf reduction | direct body inputs; no child-selector path | same | source list `[SourceBegin, SourceBegin+SourceCount)` |
| Owner entry/publication | one owner acquire and conditional publication | same | owner list supplies the node; inactive-slot writes and exchange refer to that same node |
| Wake response | one paired reader in the recipient's influence loop | same | each recipient's complete influence range, same-row child/parent inputs for before/after calculations |

For force, the default prologue processes `(end-begin)&1` entries and the
main loop advances by two; the scalar form advances by one to the same end.
For poll reductions the main loop advances by four, with `length&3` remainder;
the scalar form advances by one to the same length. Empty ranges retain their
guards. These observations describe the retained modules, not fixed totals
that the acceptance code expects.

## Executable coverage

The [test-only analyzer](../../live_hierarchy_audit.h) attributes ndarray data
and shape accesses through the production argument-buffer ABI. It normalizes
index arithmetic and only treats OR as addition when LLVM proves disjoint bits.
Packet word `k` must address `((4*selector+k)*stride+node)` using one SC acquire.
Repeated safe loads are allowed; the required component set remains XYZ/mass.

Dominance/post-dominance associate each emitted work-counter operation with
the required owner, child or paired response readers. A reader cannot satisfy
two distinct emitted iterations. Every relevant node-descriptor load also
requires a matching acquired packet. Publication must refer to its acquired
owner and complete the inactive packet's four writes. Plain, unknown and
demoted selector operations reject. Site totals are diagnostic output only.

Each force/poll control suite LLVM-verifies its mutated module before checking
audit rejection. Across the two modules it exercises 60 negative mutations:
per-role demotion, unknown RMW operation, ordinary load/store, missing acquire,
missing complete packet path, duplicate component, mixed selector and wrong
node; wrong response column; and broken publication ordering, exchange,
ownership or write completeness. Four positives accept redundant safe packet
loads and complete callback clones with redirected live uses and changed site
totals. Controls run on native and parsed retained modules in both shapes.

This is a structural audit of emitted access paths, not a proof of arbitrary
compiler equivalence. Deleting an entire loop together with all of its
diagnostics is outside this audit's independent coverage. The unchanged full
numerical/reference, packet/history, no-lost-input, quiet, continuous moving
and prepared-resource checks remain necessary and execute in all four modes.
Foreign modules are parsed/verified/audited, never JIT-executed locally.

## Contributor results

Clang/LLVM 15.0.6, actual x64, local detected CPU `znver3`. The full regression
and native matrices ran at `3eee41e`; metadata collection at `d762111` retained
identical modules and reran all eight module-audit configurations. The latter
commit changes only collector output and workflow evidence, not the analyzer
or numerical test paths.

| Local matrix | Modes passed | Observation windows | Settled / evolving | Negative / positive controls |
|---|---|---|---|---|
| Default compiler | 4/4 | 660 | 650 / 10 | 240 / 16 |
| No-unroll control | 4/4 | 660 | 649 / 11 | 240 / 16 |

All 40 moving sequences completed 32 perturbations/reuse rounds. All 152
publication probes passed. Settled/evolving is an observation, not a changed
pass rule or a bitwise-trajectory equivalence claim across worker schedules.
Every hard arithmetic/publication/history/pending-input/resource check passed.
The [full regression](cpu-ctest.txt) passed **96/96 in 1570.98 seconds**.
[Default](cpu-native.txt.gz) and [scalar](cpu-no-unroll.txt.gz) raw outputs are retained.

[PostgreSQL integration](integration.txt) passed 180 checks, with zero private
reads and equal [before](cold-before.sha256)/[after](cold-after.sha256) cold
fingerprints. It created only the fresh warm fixture
`hcp_warm_d_20261028141132_15095` in the existing owned container. The retained
core/Linguistics source databases were read-only. [CPU backend](backend.txt)
verification passed. Runs overlapped independent test/evidence processes;
elapsed test times are not engine throughput measurements.

## Hosted results

[Run 40706207825/job 122878906581](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/40706207825/job/122878906581)
completed successfully at PR source `d762111`, checkout merge
`cda8be13689e5e1f7b1de3acdbf68d8e7587d68d`. Clang/LLVM 15.0.7, actual x64,
detected CPU `znver3`. The [full regression](hosted-ctest.txt) passed
**96/96 in 2062.37 seconds**. The separate four-mode scalar step also passed.

| Hosted matrix | Modes passed | Observation windows | Settled / evolving | Negative / positive controls |
|---|---|---|---|---|
| Default compiler | 4/4 | 660 | 633 / 27 | 240 / 16 |
| No-unroll control | 4/4 | 660 | 642 / 18 | 240 / 16 |

All 40 moving sequences, 32 perturbation/reuse rounds per sequence, 152
publication probes and hard invariants passed. These observation totals retain
their own execution; they do not replace the earlier v2 or local observations.
[Default output](hosted-native.txt.gz) and [scalar output](hosted-no-unroll.txt.gz)
come from the completed native evidence artifact 12828322059.

[Fresh PostgreSQL integration](hosted-integration.txt) passed **180 checks**,
including `SOURCE addressed_reads=24 private_reads=0`. Restored cold sources
`hcp_core`/`hcp_linguistics` retained identical
[before](hosted-cold-before.sha256)/[after](hosted-cold-after.sha256) fingerprints.
Only the new warm fixture `hcp_warm_d_20261028152759_92` was created.
The subsequent [backend check](hosted-backend.txt) reported actual x64 and passed.
The [complete job log](hosted-job.txt) retains build inputs and step outcomes.
Projection-values, smoke run 40706208049 and viewport run 40706207867 passed.
No hosted integration/backend step was skipped in this final run.

[Machine-readable results](results.json) retain distinct source/configuration
pins. [SHA256SUMS](SHA256SUMS) hashes the review artifacts. Large native
transcripts are losslessly compressed; their original byte hashes
are in [native-log-hashes.json](native-log-hashes.json). Representative
force/poll/control modules from both shapes and compilers are compressed under
`ir/`. The hosted compiler artifact contains every captured configuration;
the comparison manifests hash all corresponding modules. This evidence-only
revision changes no source tested at `d762111`. C3-1 awaits maintainer review;
C3-2 remains held and CUDA execution remains unqualified.

## Reproduction

Use the standard pinned CPU build from the [handoff](../../../../docs/c3-cpu-capture-handoff.md).
For each one/two-worker and bulk/direct combination:

```sh
mkdir -p /tmp/c3-ir
HCP_C3_IR_DIR=/tmp/c3-ir TI_LIB_DIR="$PWD/engine/build/taichi-runtime" \
  engine/build/live_hierarchy_test --threads 1 --case ir
TI_LIB_DIR="$PWD/engine/build/taichi-runtime" \
  engine/build/live_hierarchy_test --threads 1 --audit-ir /tmp/c3-ir
TI_LIB_DIR="$PWD/engine/build/taichi-runtime" \
  engine/build/live_hierarchy_test --threads 1 --compiler-no-unroll
ctest --test-dir engine/build --output-on-failure -j2
```

Add `--direct` for CpuDirect. Collect/audit the scalar modules in a separate
directory with `--compiler-no-unroll` on both commands. Manifests record actual
x64, workers/path, JIT opt level 1, SIMD width 8, strict C3 math, unroll control,
target triple/data layout and the production argument offsets. Native builds
use C++17 Release `-O3 -DNDEBUG`; the modules retain their target attributes.
The program's `opt_level=1` is separate from the CPU backend's LLVM optimizer,
which explicitly runs O3 with loop/SLP vectorization and host CPU detection
(`codegen/cpu/codegen_cpu.cpp`). Final manifests record that detected CPU,
empty additional optimizer feature string, PIC/Small target settings and
Aggressive code generation. The local detected CPU is `znver3`. The older
hosted failure/diagnostic captures did not record their CPU name; it cannot
be retrospectively inferred from the generic runtime-function attributes.
The original `f048461` manifests predate explicit shape offsets; that limitation
is reported, while their target, signature and all 26 data offsets match.
Final captures include both relevant shape offsets explicitly.

Validation results and artifact hashes are recorded separately with their
source/configuration pins. Earlier v1/v2 evidence remains unchanged.
