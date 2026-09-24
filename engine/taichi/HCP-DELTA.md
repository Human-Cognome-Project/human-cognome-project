# HCP Taichi delta from upstream

The recovered fork is based on:

`taichi-dev/taichi@ba0e81dce559fb63a5958bf82feb1d00c55c02fe`
(2025-07-30).

The full recovery folder was compared path-by-path against that upstream tree.
After excluding machine-local environments/build outputs and restoring upstream
submodule gitlinks, the HCP fork differs in a bounded set of core files plus
HCP modernization records and tests.

## Modified upstream files

Runtime/configuration/capacity work:

- `c_api/src/taichi_llvm_impl.cpp`
- `taichi/program/compile_config.cpp`
- `taichi/program/compile_config.h`
- `taichi/runtime/llvm/llvm_runtime_executor.cpp`
- `taichi/runtime/llvm/llvm_runtime_executor.h`
- `taichi/runtime/llvm/runtime_module/runtime.cpp`
- `taichi/codegen/llvm/struct_llvm.cpp`
- `taichi/python/export_lang.cpp` — exposes the capacity settings through
  Taichi's upstream Python frontend; HCP itself does not use Python as its
  runtime interface.

Dense >2^31 indexing work:

- `taichi/codegen/llvm/codegen_llvm.cpp` — widened flattened ExternalPtr
  offset accumulation to i64 on the shared LLVM CPU/CUDA path.
- `taichi/program/ndarray.cpp` — widened shape-product accumulation feeding
  `Ndarray::nelement_` to `std::size_t`.

Runtime/AOT compatibility and cache work:

- `taichi/inc/constants.h`
- `taichi/analysis/offline_cache_util.cpp`
- `taichi/runtime/llvm/llvm_aot_module_loader.h`
- `taichi/runtime/llvm/llvm_offline_cache.cpp`
- `tests/cpp/llvm/llvm_offline_cache_test.cpp`

Build/test registration:

- `cmake/TaichiTests.cmake`

Repository hygiene:

- `.gitignore`

## Added regression tests

- `tests/cpp/program/llvm_capacity_config_test.cpp`
- `tests/python/test_snode_capacity.py` — upstream-frontend regression only;
  not part of the HCP runtime path.
- `tests/runtime/list_manager_test.cpp`

The HCP workspace also carries its separate native C++ >2^31 regression under
`../tests/index_cap_test.cpp`, which exercises the recovered wrapper and both
CPU/CUDA backends.

## Added HCP records

- `modernization/` — implementation status, install/configuration contract,
  independent review, investigation record and HCP handoff evidence.
- `benchmarks/snode_patterns.{py,md}` + results — development-time
  capacity/allocation/numerical probes. These are validation evidence, not the
  HCP production solver.
- `HCP-FORK.md` — recovery/curation status.

## Dependency provenance

The local recovery upload flattened upstream submodules into ordinary
directories. The curated tree restores the exact gitlink revisions from the
upstream base commit. Because this fork is nested inside the HCP monorepo, the
working submodule definitions are in the repository-root `/.gitmodules` with
paths prefixed by `engine/taichi/`.

## Not retained from the local working folder

- `.venv/`
- `build/`, `build-review/`
- Python/test caches
- staged `taichi_python*.so`
- generated/staged LLVM runtime bitcode
- generated `taichi/common/version.h` and `commit_hash.h`

These are reproducible machine/build products rather than source authority.

## Python boundary

The upstream Python frontend is currently retained as source because it is part
of Taichi's build/test distribution and some modernization regressions use it.
HCP bypasses it operationally: the HCP runtime path is native C++ → Taichi
IR/compiler/runtime → backend.

Further removal of upstream Python/frontend/UI source should happen only after
a clean native-only rebuild demonstrates that the files are unnecessary to
reproduce the HCP build.
