# Native engine build and vetting ladder

The HCP engine model is backend-independent. Taichi owns compilation to the
hardware enabled in its matched build; HCP does not maintain separate CPU and
GPU physics implementations.

This document separates three different claims that must not be collapsed.

## Gate 1 — source/build reproducibility (CPU, CI)

Purpose: prove that a fresh clone of the curated repository can reconstruct the
modified Taichi source dependencies, build the native LLVM/x64 Taichi core
without the Python frontend, link the HCP C++ workspace against that build, and
run the native CPU functional tests.

This is the universal CI baseline. Passing it means:

- the curated fork is buildable from source;
- the HCP wrapper does not depend on workstation build artifacts;
- the HCP runtime path is native C++;
- CPU execution of the recovered engine/field tests is functional.

It does **not** prove GPU equivalence or production performance.

The CPU fork build intentionally uses `TI_WITH_PYTHON=OFF`. Taichi's Python
source remains in the curated fork as upstream build/test provenance, not as an
HCP runtime dependency.

## Gate 2 — accelerator qualification (development hardware)

Purpose: build the same Taichi fork with the accelerator backend enabled and run
the same HCP model through it.

For CUDA, the current recovered tests already:
- run CPU tests unconditionally;
- run CUDA variants when `taichi::is_cuda_api_available()` is true;
- compare CPU/CUDA model outcomes where deterministic comparison is meaningful;
- carry the dedicated >2^31 dense-array indexing regression on both backends.

This gate belongs on hardware that actually provides the accelerator. It is not
made a GitHub-hosted CI prerequisite.

A passing CPU build is not a substitute for this gate; it is evidence that the
source/model/runtime chain is sound independently of accelerator availability.

## Gate 3 — load and performance characterization

Purpose: determine how the modified allocation/capacity model behaves under
realistic HCP resident-set sizes, tree counts, run lengths, and hardware
pressure.

This remains empirical work. Existing recovered measurements are development
evidence, not guarantees.

Characterize at least:
- startup/runtime allocation by configured SNode/tree capacities;
- resident field bytes per useful HCP element;
- nested/tree metadata growth;
- warm tick throughput;
- CPU versus GPU throughput at increasing resident counts;
- behaviour near VRAM/resource limits;
- long-run stability and release/reuse behaviour.

Do not optimize against synthetic Taichi workloads that reintroduce rich
graphics/pixel/vertex assumptions HCP deliberately removed.

## Backend contract

`engine/cmake/FindTaichiEngine.cmake` mirrors the capabilities of the matched
Taichi build.

- CPU/LLVM components are mandatory.
- CUDA components are optional and auto-detected by default.
- `ENGINE_TAICHI_CUDA=ON` requires a complete CUDA-capable matched build.
- `ENGINE_TAICHI_CUDA=OFF` forces the HCP wrapper to ignore CUDA components.
- `ENGINE_TAICHI_CUDA=AUTO` uses CUDA only when the complete component/runtime
  set is present.

The HCP field formulas and control semantics must not branch merely because the
backend changes.
