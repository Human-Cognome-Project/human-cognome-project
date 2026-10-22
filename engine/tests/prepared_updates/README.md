# Coherent-update CPU evidence

The [implementation](../../docs/PREPARED-UPDATES.md) follows the contract accepted
in #167 under #162. This is submitted implementation evidence, not acceptance of
production #156 insertion or GPU qualification under #145.

Source pin: `862e5e89c61827f10b2482390a53f8955a3b72e3`.
Curated native backend tree: `6784224121f68de1bf1aa0765b54e80d20e355ad`.
Clang/LLVM 15.0.6, CPU-only matched native rebuild, two engine workers per test,
CTest concurrency two. Warm integration and viewport targets were enabled in the
workspace build; the CTest run does not itself execute PostgreSQL acceptance.

| Artifact | Contents |
|---|---|
| [acceptance.json](acceptance.json) | Source/backend pins, 44 source SHA-256 values, built-artifact fingerprints, complete per-profile manifests, counters, timings and observation limits. |
| [cpu-ctest.txt](cpu-ctest.txt) | All 43 native CPU suites pass in 343.60 seconds. |
| [cpu-full-suite-output.txt](cpu-full-suite-output.txt) | Complete native output for the 43-suite run. |
| [cpu-update-ctest.txt](cpu-update-ctest.txt) | Six update suites pass again in 93.73 seconds after diagnostic-only test logging additions. |
| [cpu-native-output.txt](cpu-native-output.txt) | Final source-pinned update output: ten 32-cycle profiles, complete shared-definition cases, maximum batches, rejection/failure cases, manifests and native measurements. |
| [cpu-no-cuda-refusal.txt](cpu-no-cuda-refusal.txt) | Negative check: `--cuda` refuses this CPU-only native build. This is not GPU execution evidence. |

Runtime code and complete-occurrence assertions are identical between the full
run and final focused run. Final binary fingerprints and profile measurements
in acceptance.json describe the latter. No executable, archive or generated
runtime bitcode is committed.

The ten profiles cover 320 structural-update/tick cycles, both CPU host paths,
P=70/P=4,096, aggregate letters, byte-only and mixed forms. Two additional
complete-English-definition cases copy all sibling/value-parent/membership
bindings and preserve independent copies through modification/retirement.
Maximum-batch cases populate every usable record and retire/reuse the bank.
All measured update/rejection/calculation/inspection boundaries have zero
observed linked C++ new/new[] calls and stable native backing/layout/compiler
counters. See the implementation document for exact observation scope and costs.

Reproduce with the matched [build and test commands](../../docs/PREPARED-UPDATES.md#reproduction-and-remaining-scope).
Source hashes can be checked against the pin with `git show PIN:PATH`; raw
artifact hashes cover the retained files. CUDA-enabled local commands must
reject CPU fallback and collect actual device/driver evidence separately.
