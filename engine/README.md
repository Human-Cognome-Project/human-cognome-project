# Engine

HCP's engine path is native C++.

It links directly against the project's modified Taichi fork rather than using
Taichi's Python frontend. The fork provides the mass-particle execution
machinery and portable backend compiler/device layer; the HCP C++ workspace
supplies the concrete field formulas, state, lifecycle and control structure.

The modified fork is expected at:

```text
engine/taichi/
```

## Layout

| Path | Role |
|---|---|
| `src/engine/` | Thin native wrapper around the Taichi runtime: instance lifetime, kernel compile/launch, devices, transfers. |
| `src/field/` | Native C++ field/tick implementation. |
| `src/monitor/` | Read-only measurement of downloaded field state, accumulated per observed tick (integrity, touched surface, lag reversals, net motion, brake regime, cost). |
| `apps/` | Device/runtime inspection utilities; `field_probe` runs a field scenario and writes monitor samples as CSV. |
| `tests/` | Native smoke, field, monitor and >2^31 dense-index regression tests. |
| `cmake/` | Locates the matching Taichi source/build/runtime set. |
| `scripts/` | Native runtime environment helpers. |
| `docs/` | Recovered design, vetting, configuration and measurement record. |
| `taichi/` | Modified Taichi fork (curated separately from the HCP wrapper). |

See [ARCHITECTURE.md](ARCHITECTURE.md) before changing boundaries.

## Python boundary

Taichi upstream ships a primarily Python-facing abstraction layer for arbitrary
models and rich data. HCP deliberately does not use that as its engine surface:
our formulas and working structures are simple enough to be expressed directly
in C++.

Python in the Taichi fork is vendor frontend/build/test material. Project
Python should be limited to convenient bootstrap, migration, import/export or
other offline I/O. It must not become the physics engine, runtime harness,
database kernel, WAL kernel, or kernel-network implementation.

The previous planner-generated Python field engine has been archived under
`archive/2026-09-planner-field-python/`.

## Building

The native wrapper expects the modified fork source and a matching Taichi build
tree. The external dependencies are normal Git submodules rooted at
`engine/taichi/external/`.

After cloning:

```sh
git submodule update --init --recursive
```

Build the fork first. The HCP model is backend-independent: Taichi compiles the
same native model for the hardware enabled in its build.

The reproducibility baseline is CPU-only because it is universally available
in CI and proves the source/build/runtime path without requiring accelerator
hardware:

```sh
cmake -S engine/taichi -B engine/taichi/build-review -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_COMPILER=/usr/lib/llvm-15/bin/clang \
      -DCMAKE_CXX_COMPILER=/usr/lib/llvm-15/bin/clang++ \
      -DLLVM_DIR=/usr/lib/llvm-15/lib/cmake/llvm \
      -DCLANG_EXECUTABLE=/usr/lib/llvm-15/bin/clang \
      -DTI_WITH_PYTHON=OFF -DTI_WITH_C_API=ON \
      -DTI_WITH_CUDA=OFF -DTI_WITH_AMDGPU=OFF \
      -DTI_WITH_OPENGL=OFF -DTI_WITH_VULKAN=OFF \
      -DTI_WITH_METAL=OFF -DTI_WITH_GGUI=OFF \
      -DTI_BUILD_TESTS=OFF -DTI_BUILD_EXAMPLES=OFF
cmake --build engine/taichi/build-review --target taichi_c_api -j 2
```

A development-machine CUDA build enables `-DTI_WITH_CUDA=ON` in a separate
Taichi build tree. The HCP wrapper reads that build's CUDA setting;
`ENGINE_TAICHI_CUDA=AUTO` is the default, with `ON` and `OFF` available as
consistency checks. The native wrapper currently selects LLVM/x64 or CUDA;
other Taichi backends require HCP linkage and functional qualification.

Taichi generates native LLVM runtime bitcode from source. The HCP wrapper stages
the artifacts required by the backends present in the matched build into its
own build directory for `TI_LIB_DIR`; it does not depend on Taichi's Python
package staging path.

Then build the HCP wrapper:

```sh
cmake -S engine -B engine/build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_COMPILER=/usr/lib/llvm-15/bin/clang++
cmake --build engine/build -j 4
ctest --test-dir engine/build --output-on-failure
. ./engine/build/engine-env.sh
./engine/build/engine_devices
```

A different fork/build can still be supplied with
`ENGINE_TAICHI_ROOT` / `ENGINE_TAICHI_BUILD`.

See [docs/BUILD-VETTING.md](docs/BUILD-VETTING.md) for the CPU/GPU/load
qualification ladder.

## Current status

The native workspace was recovered from a local development branch on
2026-09-24 after the repository had already been reorganized around an
off-direction Python prototype.

The recovered record contains substantial functional validation on LLVM CPU and
CUDA, including the field model and a >2^31 dense-array indexing test. The code
does viable calculations, but the complete recovered stack is still only
partially vetted. Performance and scaling under realistic HCP load have not
been established.

Historical machine-specific measurements and development decisions are retained
under `docs/`; read their dates and status labels rather than treating every
measurement as a current guarantee.
