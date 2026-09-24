> **Current build note.** This file was recovered from the September native workspace and has been adjusted to the monorepo paths. Historical measurements still refer to the development host. See [README.md](README.md) for document status.

# Building against this engine

Mechanics of calling the modified fork in `engine/taichi/` from native C++.
Every item was established from the engine's own source and tests, or by
running it on this host on LLVM CPU and on the GTX 1070. Nothing here is a
statement about what should be computed.

## The engine is consumed as a build tree

Headers, static archives and LLVM runtime bitcode are one matched set from
one build. `cmake/FindTaichiEngine.cmake` takes a single root and derives all
three, so they cannot drift apart:

| Piece | Location under the engine root |
|---|---|
| Headers | the root itself, plus `external/include`, `external/spdlog/include`, `external/eigen`, `external/FP16/include`, `external/PicoSHA2`, `external/SPIRV-Tools/include` |
| Archives | `build-review/libtaichi_core_static.a` plus sixteen per-component archives under `build-review/taichi/` |
| Runtime bitcode | generated under `taichi/runtime/llvm/runtime_module`; the HCP CMake wrapper stages it with `slim_libdevice.10.bc` into the HCP build directory for `TI_LIB_DIR` |

`libtaichi_core_static.a` is not self-contained: the component archives hold
the codegen, runtime and device-interface objects, so all of them are linked
inside one `--start-group`/`--end-group`.

Compile definitions must match the engine's own, because they select code
paths inside its headers: `TI_INCLUDED`, `TI_WITH_LLVM`, `TI_WITH_CUDA`,
`TI_ARCH_x64`, `TI_ISE_NONE`. Linking the shared `libLLVM-15.so` works even
though the engine links LLVM statically.

There is also a C API in the engine tree (`c_api/include/taichi`, with
`build-review/libtaichi_c_api.so`, documented in `c_api/docs`). It loads
ahead-of-time modules rather than kernels built in process, so this workspace
does not use it. It stays available if a process boundary with a stable ABI
is wanted later.

## TI_LIB_DIR is mandatory

The engine resolves `runtime_x64.bc`, `runtime_cuda.bc` and
`slim_libdevice.10.bc` through the `TI_LIB_DIR` environment variable
(`taichi/util/lang_util.cpp`), and aborts with an error naming it if it is
unset. The generated `build/engine-env.sh` exports it; the CMake test sets it
itself.

## Configuration is read before the runtime exists

The full surface, with sources, is in `CONFIGURATION.md`.

`CompileConfig` is copied into a `Program` when the `Program` is constructed,
so any setting has to be in place before that call:

```cpp
default_compile_config.llvm_snode_capacity = n;   // only if you want to
Program program(Arch::cuda);
program.materialize_runtime();
```

`Runtime` in `src/engine/runtime.cpp` writes only the settings the caller
supplied and leaves everything else to the engine, which reads its own
environment variables for the LLVM startup capacities
(`CompileConfig::apply_llvm_runtime_environment`, exercised by
`docs/api-reference/llvm_capacity_config_test.cpp`). Capacities are
snapshotted at startup, and SNode identifiers keep counting past destroyed
layouts, so they bound identifiers issued over the life of an instance rather
than live objects.

## A top-level loop must carry a thread count

`IRBuilder::create_range_for` defaults `num_cpu_threads` to zero. The
outermost loop of a kernel becomes the offloaded task, and launching it with
zero threads aborts inside `threading.cpp` with
`Assertion failure: this->desired_num_threads > 0`. Pass
`program.compile_config().cpu_max_num_threads`, which `Runtime::cpu_threads`
returns. A nested loop runs serially inside the task and does not need it.
Python never hits this because its lowering fills the field in.

## Kernel names are the compilation key

Two kernels given the same name share one compilation, even when their IR
addresses different SNodes: the second kernel silently runs the first one's
code. The smoke test caught this by writing two independent trees through
identically named kernels and reading the first one back changed. Give every
compiled kernel a distinct name.

## Values that are not compiled in

A loop bound, a scalar or an array can be a kernel argument
(`create_arg_load`, `create_ndarray_arg_load`), so one compiled kernel serves
many launches. Only what changes the field layout, and therefore the SNode
tree, needs a new compilation. The smoke test launches one kernel twice with
different arguments to hold that property.

## Host and device transfer

Engine ndarrays carry data across the host boundary. `Ndarray::read_float`
and `write_float` stage one element per call; the device interface's
`upload_data` and `readback_data` move a whole buffer in one call, which is
what `src/engine/transfer.cpp` uses.

## Stale engine examples

`cpp_examples/` is excluded from the engine build (`TI_BUILD_EXAMPLES=OFF`)
and has drifted: `run_snode.cpp` compiles and then aborts at run time in
`type.cpp get_element_type`. The current calling convention is in
the engine's `tests/cpp/`, copied into `docs/api-reference/` with a note.
