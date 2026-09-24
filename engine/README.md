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
| `apps/` | Device/runtime inspection utilities. |
| `tests/` | Native smoke, field and >2^31 dense-index regression tests. |
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
tree. By default the CMake integration resolves:

```text
source:  engine/taichi/
build:   engine/taichi/build-review/
runtime: engine/taichi/python/taichi/_lib/runtime/
```

All three must come from the same fork/build.

Once the fork has been built:

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
