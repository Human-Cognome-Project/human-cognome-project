# Engine

The engine in `/opt/project/taichi`, assembled and proven usable from here as
one linkable target. It starts an instance, allocates fields, compiles and
launches kernels, and moves data across the host boundary, on the LLVM CPU
path and on the GTX 1070.

It contains no model and no controller. Nothing here decides what is
computed, how a tick advances, or what a row means. Those are Patrick's to
specify, and they are written against this engine, not into it.

## Read in this order

1. `docs/DEVICES.md` for the machine: which GPU to target, what it holds, and
   the measured envelope any parameters have to fit inside.
2. `docs/CONFIGURATION.md` for what the engine reads and when, every claim
   citing the engine source.
3. `docs/ENGINE-NOTES.md` for the calling mechanics, including the three
   things that abort a run if you get them wrong.
4. `docs/api-reference/` for the engine's own C++ usage, with the stale
   example marked.

## Configure and run

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_COMPILER=/usr/lib/llvm-15/bin/clang++
cmake --build build -j 4
ctest --test-dir build --output-on-failure
. ./build/engine-env.sh && ./build/engine_devices
```

The check passes on this host on LLVM CPU and on the GTX 1070. The GTX 750 Ti
is visible but deferred: the engine's allocation call is refused by the card.
`docs/DEVICES.md` has the measurements and the detail.

To run a binary by hand, `. ./build/engine-env.sh` first: the engine aborts
without `TI_LIB_DIR`. A different engine checkout needs no edits here:
`-DENGINE_TAICHI_ROOT=... -DENGINE_TAICHI_BUILD=...`.

## What is here

| Path | Contents |
|---|---|
| `cmake/FindTaichiEngine.cmake` | Locates the engine build tree and exposes it as one target. Fails loudly when a piece is missing. |
| `src/engine/runtime.*` | Engine instance lifetime, and building, compiling and launching a kernel. Caller-supplied settings only; unset means the engine's own default. |
| `src/engine/device.*` | Device enumeration, which one the engine binds, visibility selection, and free-memory readings for measuring cost. |
| `src/engine/transfer.*` | Bulk host/device transfer for an engine ndarray. |
| `apps/engine_devices.cpp` | Prints the host and GPU inventory, the bound device, and what an instance and an optional field reserve. Measurement only. |
| `tests/engine_smoke_test.cpp` | Writes and reads a field through kernels on each architecture, with two independent trees on one live runtime. Computes nothing meaningful on purpose. |
| `docs/DEVICES.md` | The devices on this host, measured: what is usable, how a device is selected, what an instance and a field cost, and where the 750 Ti fails. |
| `docs/CONFIGURATION.md` | The engine configuration surface: what it reads from the environment, its own defaults, how capacities are enforced, and the runtime ABI marker. Every statement cites the engine source. |
| `docs/ENGINE-NOTES.md` | How to call the engine: what must be set, what aborts if it is not, what is compiled in and what is an argument. |
| `docs/api-reference/` | Verbatim engine sources that document its C++ API, with a note on which has gone stale. |

## Assembling a controller

A controller is a separate target in this workspace that links
`engine_support` and supplies everything the engine does not: the state
layout, the kernel bodies, the order a tick runs them, and the boundary in
and out. The engine offers it four things.

**An instance.** `engine::Runtime` owns one `Program` and every device
allocation made against it. `InstanceSettings` passes through the arch, the
two startup capacities, the device memory budget and the launch thread count,
each optional, and anything left unset keeps the engine's own default.
Capacities are fixed once the instance exists, so they are chosen before it is
constructed. Destroying the runtime releases the allocations and invalidates
everything built against it.

**State.** Build an `SNode` tree with `root->dense(Axis(0), n)` or
`root->pointer(...)`, give the leaf a `dt`, and hand the root to
`add_snode_tree`. Trees added to a live runtime coexist. Identifiers are
process-wide and monotonic, so a configured capacity bounds what one process
issues over its whole life, destroyed layouts included.

**Kernels.** Build the body with `IRBuilder`, then wrap it in
`engine::CompiledKernel` with a name and a parameter list. It compiles once
and launches many times, and argument ids follow the declaration order.
Three rules from `docs/ENGINE-NOTES.md` are load-bearing: the outermost loop
must carry `runtime.cpu_threads()` or the launch aborts; every compiled kernel
needs a distinct name, because the engine keys compilations by name and a
duplicate silently runs the wrong code; and a loop bound, a scalar or an array
can be a launch argument, so only a layout change forces a recompilation.

**A boundary.** `engine::upload` and `engine::readback` move a whole
`Ndarray` in one call. Kernels reach it with `create_ndarray_arg_load` and
`create_external_ptr`, and the field itself with `create_global_ptr`.
`runtime.synchronize()` before reading anything back.

`tests/engine_smoke_test.cpp` is the shortest complete example of all four,
and `docs/api-reference/ir_builder_test.cpp` is the engine's own authority on
the calling convention.

## State of this workspace

Nothing here is committed. The branch reference and `HEAD` in this repository
are owned by another user account, so git cannot update them from here; the
files are on disk and untracked. Commit them from an account that owns the
reference, or ask Patrick.

This is an MVP proof of concept. Rough edges are expected and acceptable;
finish is not the goal, and nothing here should be polished at the cost of
finding out whether the idea works.
