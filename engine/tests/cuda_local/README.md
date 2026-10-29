# Local GTX 1070 correction evidence

Tested engine/test source, committed after the runs without further code edits:
`758ad053629a6ca9beea5422de659aa38ba15e48`, including fork corrections
`5d0fb1b`. Base: `a05ae246762204de2055eeb556996d142c2a55fc`.
The following documentation/evidence commit changes no tested code.
`SOURCE_SHA256SUMS` and `BINARY_SHA256SUMS` identify the source files and local
executables. Binaries and large compiler/run artifacts are not committed.

Host: Linux x86-64, Clang/LLVM 15.0.7, NVIDIA driver 535.309.01; actual GPU0
GTX 1070 (sm_61). GPU1 was excluded. The matched Taichi substrate enabled CUDA;
engine options enabled viewport, warm integration and the explicit local CUDA
qualification gate. Build/compiler caches and captures used RAM-backed storage.
These are contributor local results, not hosted acceptance or a release build.

## Retained results

| Evidence | Meaning |
|---|---|
| `ctest-cpu-accepted-motion-all.txt` | 100/100 CPU tests, 2398.13 s, after the accepted-motion correction; CPU execution on a CUDA-enabled substrate. |
| `ctest-cuda-final.txt` | 4/4 CUDA qualification suites, 423.86 s. |
| `accepted-motion-{cpu,cuda}.txt` | Both signs, fraction bits 4/8/52, dt 0/0.5/1, subnormal braking from normal inputs, cached speed, activity and fixed backing. |
| `hierarchy-results.txt` | Exact extracted result lines for all ten requested C3 GPU case selections (11 case results), with SHA256 of each original full log. No case failures. Full logs remain local. |
| `capture-evidence.json` | Same active CUDA execution; 64 ticks and 9 captures, f64 round-trip and observation-neutrality checks. |
| `browser-evidence.json` | 89 offline browser checks on the three-body GPU capture. The live-watch subtests use retained CPU example captures and UI fixtures; this does not prove human viewing of a running GPU stream or the native directory picker. |
| `warm-active-nas-accepted-motion.txt` | 180 native NAS warm-to-active integration checks, with 24 addressed reads and zero private-source reads. |
| `nas-*-fingerprints.jsonl` | Ten cold-table counts and row-fingerprint aggregates, byte-equal to the pre-change validation baselines after removing psql command-status lines. No cold-data modification was required. |
| `precision-cuda.txt` | **Unresolved failure:** mixed f32-to-f64 subnormal promotion becomes zero. Same-width f32/f64 boundary checks pass first. This is not counted as a passing GPU precision suite. |

The `historical-*` files preserve investigation evidence from before the motion
correction. The subnormal failure is an exact diagnostic excerpt; the PTX logs
record actual sm_61 selector audits and missing-barrier negative controls during
compiler investigation. They are not final-source reruns. The final CUDA suites
also exercise the compiler's LLVM barrier and semantic packet audits.

## Reproduce against the existing matched native build

Source `BUILD/engine-env.sh`, expose only the GTX 1070 through
`CUDA_VISIBLE_DEVICES`, and keep CUDA/compiler scratch in a suitable RAM-backed
location. Do not accept CPU fallback.

```sh
ctest --test-dir BUILD -LE cuda-qualification -j2 --output-on-failure
ctest --test-dir BUILD -L cuda-qualification --output-on-failure
for case in basic forces singleton cumulative tree publication moving moving_permuted letters capture; do
  BUILD/live_hierarchy_test --backend cuda --case "$case"
done
```

The capture case above uses and cleans its temporary output unless
`--capture-output DIRECTORY` is supplied. It is a short fixture run, not an
interactive application. `--ticks 64 --every 8` controls the capture schedule,
not wall-clock pacing. Native warm integration uses locally configured libpq
connections and the existing owned warm test database; do not reset cold stores.

The [correction report](../../docs/CUDA-C3-QUALIFICATION.md) explains the changes
and the remaining live-insertion, admission, rendered-flow and precision issues.
Docker/application build work stays separate.
