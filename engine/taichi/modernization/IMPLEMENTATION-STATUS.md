# Implementation status

Updated 2026-09-10. This implementation milestone is built and validated on LLVM CPU and GTX 1070 CUDA, including the native C API loader.

## Scope agreed with user

Preserve Taichi machinery. Expand simultaneous SNode pattern scope with install/reconfiguration settings loaded at instance startup. No live resizing required. System probing and user policy belong in a loader; the core consumes explicit capacities. Develop on GTX 1070, qualify GTX 750 Ti later, leave hooks for newer hardware. Rendering unnecessary. Simplicity means explainable purpose; declaration is cheap, calculation costs, allocation is expensive.

## Current work

- Root agent: build baseline, integration review, validation and documentation.
- limits agent: LLVM startup capacities, table allocation and bound checks, non-contiguous node IDs, regression tests.
- addressing agent: paged ListManager directory and standalone concurrency/boundary tests.
- backends agent: benchmarks/snode_patterns.py, isolated numerical/scaling probe.

## Build environment

Project-local Python environment: `/opt/project/taichi/.venv` (Python 3.12). Installed pybind11 2.13.6, numpy, pytest, scipy, astunparse, colorama, dill, rich. Existing LLVM/Clang 15.0.7. Main checkout initially had only untracked modernization research, no engine edits.

Pristine baseline worktree `/tmp/taichi-baseline` at ba0e81dce. Submodule directories link to the existing checkout dependencies; do not mutate those links/dependencies. Build directory `/tmp/taichi-baseline/build-review`. Log `/tmp/taichi-baseline-build.log`.

Configure command:

```sh
cmake -S /tmp/taichi-baseline -B /tmp/taichi-baseline/build-review -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/usr/lib/llvm-15/bin/clang -DCMAKE_CXX_COMPILER=/usr/lib/llvm-15/bin/clang++ -DLLVM_DIR=/usr/lib/llvm-15/lib/cmake/llvm -DCLANG_EXECUTABLE=/usr/lib/llvm-15/bin/clang -DPYTHON_EXECUTABLE=/opt/project/taichi/.venv/bin/python -DTI_WITH_CUDA=ON -DTI_WITH_OPENGL=OFF -DTI_WITH_VULKAN=OFF -DTI_WITH_METAL=OFF -DTI_WITH_GGUI=OFF -DTI_WITH_C_API=OFF -DTI_BUILD_TESTS=ON -DTI_BUILD_EXAMPLES=OFF -DCMAKE_INSTALL_PREFIX=/tmp/taichi-baseline
cmake --build /tmp/taichi-baseline/build-review --target taichi_python taichi_cpp_tests -j 3
```

Both GPUs visible; GPU 0 GTX 1070 8GiB; GPU 1 GTX 750 Ti 2GiB. Use device 0 for this implementation cycle. Host has 16GiB RAM and six CPU threads; builds deliberately use three jobs.

## Original validation checklist (progress below)

1. Finish baseline build and smoke CPU/CUDA; preserve baseline measurements.
2. Build modified core, review ABI/cache implications and all initialization call sites.
3. Run native ListManager boundary/concurrency tests and capacity Python regressions.
4. Compare numerical and allocation/scaling results, including >1024 SNode IDs and >512 trees under explicit capacities.
5. Document environment variables, fixed startup semantics, remaining lifetime limits and backend scope. Do not imply completed 64-bit indexing migration.

Sandbox process startup currently fails with a bwrap network-namespace error; shell work uses explicit escalation. Normal apply_patch is unreliable for the same reason. No permanent system dependency installation or changes have been made.


## Progress after baseline build

Baseline core builds with stock LLVM15, and CPU/CUDA initialization succeeds.
Synthetic 8-pattern f64 gravity/centroid probes pass on both CPU and GTX1070.
110 patterns reproduce the old per-tree1024 rejection. The strengthened
interleaved sparse-tree test reproduces a baseline CPU segmentation fault.
Standalone paged-list test passes normally and under ASan/UBSan.

Modified CPU/CUDA core built and passed the checks below. Final build with native
C API enabled is in /opt/project/taichi/build-review; current log is
/tmp/taichi-native-loader-build.log.
JIT ABI version and AOT runtime_abi marker validation have been added.

Baseline memory profiler originally threw invalid format specifier because of
{:n} integer formatting. Exactly that diagnostic formatting was changed to {}
in baseline llvm_runtime_executor.cpp to enable allocation measurements; baseline
allocator/capacity/kernel behavior is unchanged. Modified core has the same
format correction. cmake --install baseline failed in an unrelated SPIRV-Tools
.pc installation; built taichi_python*.so and runtime_*.bc plus slim_libdevice.10.bc
were copied into baseline python/taichi/_lib/{core,runtime} for local use.


Native compiled sizes: baseline ListManager=1,048,616 bytes, LLVMRuntime=35,256;
modified ListManager=2,088 bytes, LLVMRuntime=2,536 (table storage now separate).
ASan/UBSan native allocation test passes. ThreadSanitizer executable cannot start
on this host: unexpected memory mapping; do not report a TSan pass.
Baseline 8-pattern pointer probe (4 sampled,16 slots each,f64) requested
139,466,600 dynamic bytes after construction and159,389,544 after execution on
both CPU and CUDA, excluding alignment. JSON at /tmp/taichi-baseline-cpu-profile.json
and /tmp/taichi-baseline-cuda-profile.json. Baseline and modified core/test/C API builds completed.


## Verified implementation, 2026-09-10

- Startup capacities replace LLVM fixed metadata arrays. Defaults remain 1024
  SNode slots and 512 tree slots. Python environment/explicit arguments work;
  native C API capacity environment parsing is built and verified end to end.
- Actual SNode IDs are initialized, including interleaved tree construction.
  Scalar-place leaves no longer allocate unused iteration lists.
- ListManager uses a 2 KiB page directory and lazily allocated 4 KiB pointer
  pages. CPU publication uses acquire/release; Pascal-compatible GPU publication
  uses existing integer atomics and fences. CUDA established-element reads use
  volatile pointer loads after allocation acquisition or prior-task completion.
- LLVM JIT keys contain the runtime ABI version. AOT artifacts require a matching
  runtime_abi sidecar; missing/stale markers are rejected.
- Capacity tests: 11 CPU and 11 CUDA passed. Combined final CUDA capacity/sparse
  suite: 64 passed, 2 skipped, 2 deselected (huge and nested fill/clear excluded).
  The excluded nested stress case fills 16,777,216 entries ten times; it was not
  part of the bounded development-box validation.
  Existing CPU sparse suite: 52 passed, 1 skipped, 1 deselected (huge excluded).
- CPU and CUDA cache: 25 passed each. CPU and CUDA field/dynamic/bitmasked AOT: 3 passed each.
  Final focused C++ configuration/tree/cache tests: 5 passed.
- Standalone boundary/concurrency test passes normally and with ASan/UBSan.
  TSan cannot start on this host; AMDGPU and GTX 750 Ti are not qualified.
- CPU and CUDA 512-pattern probes construct 6,657 SNodes with capacity 8,192
  and a 128 MiB sparse pool. Four sampled patterns pass independent numerical
  checks; unsampled patterns prove construction only. Allocation-request
  counters: 11,737,128 bytes at construction, 31,692,840 after execution.
- Comparable 8-pattern construction counters fall from 139,466,600 to 4,318,248
  bytes. These are cumulative requested bytes, not resident memory.
- A larger CUDA probe (one pattern, 16,384 particles, 100 ticks) found a roughly
  2x regression when every directory lookup used an atomic RMW. The final
  established-element lookup removes that regression: baseline 12.783 ms,
  initial RMW version 26.099 ms, final version 13.284 ms. Single measurements,
  not statistical performance guarantees. All numerical checks passed.
- Raw benchmark records are preserved in benchmarks/snode_patterns_results.jsonl.

Remaining limits: monotonic SNode IDs still count destroyed layouts toward the
instance lifetime budget; sparse metadata is not fully reclaimed on tree
removal; logical indices are not globally widened; other backends and hardware
are not qualified. No automatic resource-probing policy or production interaction
model is introduced. See INSTALL-CONFIGURATION.md for the supported contract.


## Final native integration

Enabled the existing C API with `cmake -S . -B build-review -DTI_WITH_C_API=ON`.
Built `taichi_python taichi_cpp_tests taichi_list_manager_tests taichi_c_api`
with `-j 3`; a final incremental build incorporates all source edits. Build logs:
/tmp/taichi-native-loader-build.log and /tmp/taichi-final-incremental-build.log.
The pre-existing test use of tmpnam causes a linker warning, not a build failure.
Built Python module and runtime bitcode are staged in python/taichi/_lib.

A ctypes probe loads build-review/libtaichi_c_api.so and an AOT module containing
1,025 scalar fields under a sparse pointer, exported with an initialization
kernel. Through native C API calls:

- Environment capacity 2048: creates runtime, loads >1024-SNode AOT, executes
  initialization kernel and synchronizes successfully.
- Capacity 1024: cleanly rejects that AOT with the configured-capacity error.
- Capacity 0: cleanly rejects runtime creation with the environment parsing error.

Probe scripts and logs are in /tmp/taichi-native-capacity-probe.py,
/tmp/taichi-capacity-aot-generate.py, and /tmp/taichi-native-capacity-*.log.
Final C++ output: /tmp/taichi-cpp-final.log (5 passed). Final native list harness
also passes. These native checks qualify capacity consumption and loading, not
an independent numerical validation of the production mathematical model.

Final staged Python CPU pointer probe also passes under LC_ALL=C, including
memory profiling after removal of the obsolete global locale mutation.


## Dense Ndarray flat-index widened to i64, 2026-09-14

Partial answer to the "logical indices are not globally widened" limit noted
above: `TaskCodeGenLLVM::visit(ExternalPtrStmt*)` in
`taichi/codegen/llvm/codegen_llvm.cpp` now accumulates `linear_index` in i64
instead of i32 (i64 zero constant; per-axis sizes and each index operand are
`SExt`-ed to i64 before the multiply/add that folds them into the flattened
offset). This lifts the dense-Ndarray flat-index cap from ~2.1e9 cells and is
behaviour-preserving below 2^31, since sign-extending i32 values and widening
the accumulator changes no result representable in i32. The path is shared by
the LLVM codegen used by both CPU and CUDA, so both backends pick it up from
this one edit.

Deferred: the sparse-path i32s (`ListManager::num_elements`,
`PhysicalCoordinates`) are untouched — the dense Ndarray path bypasses them,
so they are out of scope here. SPIR-V/Vulkan, Metal and DX12 have their own,
separate ExternalPtr codegen, still in i32 (see deferred-backends.md); widen
those when those backends are qualified. This change has not been validated
against an actual >2.1e9-element ndarray — the engine's fixed-size regression
suite (`engine/build`, 57 assertions, cpu+cuda) passed unchanged, which
confirms behaviour preservation at small N but cannot exercise the lifted
cap; that needs a dedicated host run with a >2^31-element ndarray.


## Ndarray construction-time element count widened to 64-bit, 2026-09-14

Closes the gap the previous entry flagged: `Ndarray::nelement_` (declared
`std::size_t`) was computed by `std::accumulate` seeded with a plain int `1`,
so the shape-product ran in 32-bit and overflowed for any array whose total
element count exceeds 2^31 — producing a garbage allocation size and
aborting at construction, before the ExternalPtr codegen path (above) was
ever reached. Fixed in both `Ndarray` constructors in
`taichi/program/ndarray.cpp` by seeding the accumulate with `std::size_t(1)`
and multiplying via `std::multiplies<std::size_t>()`, matching `nelement_`'s
own type. The neighbouring `1LL`-seeded `total_num_scalar` warning check is
untouched (still just an index-boundary warning, not a hard cap).

Together with the ExternalPtr i64 widening above, this lifts the dense-Ndarray
cap end-to-end — construction, allocation, and indexing all now handle
>2^31-element arrays. Validated by the new `index_cap_test`
(`repo/engine/tests/index_cap_test.cpp`): a `{2, N}` u8 ndarray with
N=1,090,000,000 (2N=2,180,000,000 > 2^31) constructs successfully and a HIGH
sentinel written at `{1, N-1}` (flattened offset 2,179,999,999) round-trips
correctly, on both cpu and cuda. `engine/build`'s regression suite remains
57/57 (`field_test`) and now also carries `index_cap_test` passing on both
backends.

Deferred, as noted above: `snode.cpp` has the same accumulate-seed pattern
(an int-seeded product feeding a warning check there) — flagged but not
fixed here, since it's on the sparse path and this fix targets the dense
harness only; real-bug status on that path is unverified.
