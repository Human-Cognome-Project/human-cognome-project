# B1 native selection qualification — CPU evidence

Tested source: `ada217a68c1af546e51ebb69f2f0c43ac42b8248`, based on accepted
#199 merge `13d3c42cbf5c5247432686273a493158563ef515`. The original C3 builder
refactor is `2bd0c60`; the workspace, qualification and CI coverage are `ada217a`.
The source was clean at validation start. Subsequent evidence/status commits do
not change these tested binaries. `source-and-binaries.txt` pins both executables
and the runtime bitcode; `toolchain.txt` and `cmake.txt` preserve build settings.

This is the first B1 gate under the [accepted plan](../../../../docs/native-admission-contact-transaction-plan.md),
submitted for review in #200. It qualifies transaction-local native selection
with the actual shared C3 force/poll builders. The [implementation contract](../../../docs/NATIVE-SELECTION.md)
states its preconditions and scope. A complete candidate physical/contact tick,
typed capacity/work rejection and operational admission are not enabled here.
The particle/contact phase driver is explicitly test-only.

## Results

| Check | Local result |
|---|---|
| Complete CPU regression | **105/105**, 1616.66 seconds, parent `TI_LIB_DIR` unset. |
| Selection matrix | **8/8** modes, **48/48** case groups. |
| Compiler evidence | **168** new modules parsed; **56/56** existing modules byte-identical. |
| PostgreSQL-to-active | **180/180** checks; zero private reads and unchanged cold fingerprints. |

The complete local CPU result and per-mode inventory are recorded in
`results.json`, `cpu-ctest.txt.gz` and `LastTest.log.gz`. Eight modes cover
one/two workers, BulkCopy/CpuDirect and default/no-unroll LLVM shapes. Default
functional logs are extracted from that same complete CTest run; the four scalar
functional runs are retained separately. Each mode passes all six case groups.

The selection matrix requires exact committed snapshots through abandon/retry,
particle calculation before contact wakes, complete same-poll centroid work,
at most one visit/publication per candidate poll, preserved late pending work,
nonzero selectors/unequal packets/distinct dirty history and an independent
long-double combined-force reference. Deliberate unsafe runtime controls yield
five expected rejections per one-worker mode and four per two-worker mode,
including a deterministic one-worker frozen-cut witness: 36 in total.

Each mode retains 32 rejected and 32 committed candidate C3 reuse attempts using
letter/UTF-8 intrinsic fixtures and permuted native order. Additional held-input
native polls discharge lag without another body calculation. The actual C3
fixture has 70 body slots and four installed comparisons. Separate native mask
profiles `(P,G)=(0,0),(70,70),(4096,70),(70,4096)` each exercise 32
stage/abandon/publication rounds and three setup rejections before any SNode
creation. These are selection-capacity witnesses, not dense 4096-body physical
qualification. Unused padding stays inactive.

Known runtime backing, native list/pool/layout/compiler counters remain stable
through measured reuse, with zero observed linked C++ new/new[]. The prepared
workspace binding payload is 132 bytes. [prepared_metrics.h](../../prepared_metrics.h) defines the raw
counter columns. This excludes arbitrary malloc, residency/page faults,
driver resources and pinned transfers; it is not a complete tick reservation.

## Compiler evidence

`ir/` retains 168 actual modules: 21 per configuration, comprising three mask
primitives, three actual candidate C3 kernels, two numerical test-copy drivers,
one test phase driver and three mask kernels for each of four capacity profiles.
Every module was parsed by LLVM. Configuration files retain actual x64 target,
CPU/worker/compiler settings, native SNode/tree identifiers and complete
argument/shape ABI metadata.

Native mask ownership/SC auditing has 96 expected unsafe-control rejections and
24 safe redundant-atomic controls across the eight functional modes. Existing
C3 semantic packet/selector controls run on the actual candidate force/poll
kernels as well: 480 expected unsafe rejections and 32 safe-control passes.
The bounded mask auditor follows the pinned LLVMRuntime ABI;
it is not a universal pointer-alias or compiler-equivalence proof.

The shared-builder refactor also preserves all 56 preexisting C3/refresh modules
byte for byte. `baseline-module-comparison.json` records each current hash and
its already-retained accepted baseline artifact. `original-audit/` retains this
run's configuration, collection and audit output. Identical old modules are
referenced rather than duplicated.

## Integration and backend

The source-pinned PostgreSQL-to-active run passes 180/180 checks with 24 addressed
reads, zero private reads and unchanged cold fingerprints. Existing owned cold
stores were read-only; only the new warm fixture
`hcp_warm_d_20261031025718_26277` was created and retained. `integration/` records
provenance, raw output, libraries and before/after fingerprints. No persistent
store was reset or dropped.

`backend.txt` records the actual x64 CPU instance and absence of a visible CUDA
device/driver. This evidence does not qualify CUDA selection, complete B1 tick
atomicity, certified placement, bootstrap/admission, enduring owners or Docker.
The accepted #196 motion rule and existing mixed-precision findings are unchanged.

Hosted native run [41162316957 / job 124422351715](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/41162316957/job/124422351715)
tests `ada217a` separately. Its result is pending when this local record is
prepared; local success is not substituted for hosted acceptance.

## Reproduction and integrity

Build the matched CPU engine as in [first run](../../../../docs/onboarding/FIRST-RUN.md),
then source `BUILD/engine-env.sh`. Run:

```sh
env -u TI_LIB_DIR ctest --test-dir BUILD --output-on-failure -j 2
for workers in 1 2; do
  for host_path in bulk direct; do
    args=()
    if [ "$host_path" = direct ]; then args+=(--direct); fi
    BUILD/live_hierarchy_test --threads "$workers" --case selection \
      --compiler-no-unroll "${args[@]}"
    for shape in default no-unroll; do
      shape_args=()
      if [ "$shape" = no-unroll ]; then
        shape_args+=(--compiler-no-unroll)
      fi
      output="selection-ir/${workers}-${host_path}-${shape}"
      mkdir -p "$output"
      HCP_B1_IR_DIR="$output" BUILD/live_hierarchy_test \
        --threads "$workers" --case selection_ir "${args[@]}" "${shape_args[@]}"
      for module in "$output/"*.ll; do
        llvm-as "$module" -o /dev/null
      done
    done
  done
done
```

The existing `--case ir` collector and `--audit-ir DIRECTORY` preserve the
original C3 audit. The owned-container integration recipe is
`engine/scripts/run_warm_active_acceptance.sh`; it requires retained cold stores
and creates a new warm fixture. `uncompressed-sha256.json` records original
lengths and SHA256 for losslessly compressed evidence; `SHA256SUMS` checks the
retained files themselves. No executable, runtime library or database dump is
vendored into this evidence directory.
