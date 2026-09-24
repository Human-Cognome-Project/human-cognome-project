> **Current configuration reference.** Recovered from the September native workspace and adjusted to current repository paths. Hardware measurements/default observations remain dated evidence, not performance guarantees.

# Configuring the engine

Every statement here cites the modified engine source now recovered under
`engine/taichi/`. Nothing in this file recommends a value: the numbers
below are the engine's own defaults, not a policy for this workspace.

## Where the surface is

`CompileConfig` in `taichi/program/compile_config.h` is the whole
configuration surface, one struct of plain members. The global instance
`default_compile_config` is copied into a `Program` when the `Program` is
constructed (`taichi/program/program.cpp:75`), which then calls
`CompileConfig::fit()`. So configuration happens before an instance exists,
and changing the global afterwards does not affect a running instance.

`src/engine/runtime.cpp` in this workspace wraps that sequence: it applies
the engine environment, then the caller's explicit settings, constructs the
`Program`, and restores the global so one instance's settings do not leak
into the next in the same process.

## What the engine reads from the environment

| Variable | Read by | Effect |
|---|---|---|
| `TI_LIB_DIR` | `taichi/util/lang_util.cpp:35` | Directory holding `runtime_x64.bc`, `runtime_cuda.bc` and `slim_libdevice.10.bc`. No default: the engine aborts with an error naming the variable. |
| `TI_LLVM_SNODE_CAPACITY` | `CompileConfig::apply_llvm_runtime_environment`, `taichi/program/compile_config.cpp:70` | Startup capacity for SNode metadata slots. |
| `TI_LLVM_SNODE_TREE_CAPACITY` | same | Startup capacity for SNode tree slots. |

The two capacity variables are parsed strictly: empty or unset leaves the
current value, and anything that is not a whole-string positive `int` raises
`"must be a positive integer representable by int"`. Both are validated
before either is applied (`compile_config.cpp:71-88`).

`apply_llvm_runtime_environment()` is not called by `Program` construction.
In the engine it is called only from the C API
(`c_api/src/taichi_llvm_impl.cpp:26`). A native caller that wants those
variables honoured has to call it, which is what `InstanceSettings::
read_environment` does here; set it false to ignore the environment.

The engine has no setting for the CUDA device index: it binds visible
ordinal zero (`taichi/rhi/cuda/cuda_context.cpp:22`), so the device is chosen
by `CUDA_VISIBLE_DEVICES` before CUDA starts. `TI_VISIBLE_DEVICE` is read only
by the Vulkan loader (`taichi/rhi/vulkan/vulkan_loader.cpp:111`). Nothing
native reads a device-memory variable either: `device_memory_GB` has to be set
on the config. `DEVICES.md` has the devices on this host, how to select one,
and what an instance and a field measured.

## Engine defaults

| Member | Default | Source |
|---|---|---|
| `llvm_snode_capacity` | 1024 | `compile_config.h:64` |
| `llvm_snode_tree_capacity` | 512 | `compile_config.h:65` |
| `device_memory_GB` | 1 | `compile_config.cpp:66` |
| `device_memory_fraction` | 0 | `compile_config.cpp:67` |
| `cpu_max_num_threads` | `std::thread::hardware_concurrency()` | `compile_config.cpp:55` |
| `offline_cache` | false | `compile_config.h:94` |

Read the default out of the engine rather than repeating it: the smoke test
compares against `CompileConfig{}.llvm_snode_capacity` for exactly this
reason.

## How capacities are enforced

- Both must be positive when the LLVM executor is constructed, or
  `"LLVM SNode and SNode tree capacities must be positive"`
  (`llvm_runtime_executor.cpp:43`).
- An SNode identifier at or above the capacity fails at structure
  compilation (`codegen/llvm/struct_llvm.cpp:254`) and when a tree or a field
  is loaded (`llvm_runtime_executor.cpp:401-414`). The message names the
  offending identifier and the configured capacity.
- `SNode::counter` is a process-wide static (`taichi/ir/snode.cpp:12`), so
  identifiers keep rising across `Program` instances in one process, and
  destroyed layouts still count. `SNode::reset_counter()` exists
  (`taichi/ir/snode.h:348`); it is only safe with nothing live.
- The runtime computes its own preallocation from the two capacities, by
  calling `runtime_get_memory_requirements` with them
  (`llvm_runtime_executor.cpp:713`), and logs the resulting size at trace
  level. Native callers set the log level through `taichi/common/logging.h`.
  That is the measurement to use for budgeting rather than any figure quoted
  elsewhere.

## Compiled artefacts and the runtime ABI

`kLlvmRuntimeAbiVersion` is `1` (`taichi/inc/constants.h:13`). An offline
cache or ahead-of-time directory carries a `runtime_abi` file holding that
number; loading warns and refuses when it is missing or different
(`llvm_offline_cache.cpp:126-136`, written at `358-362`). Keep the file with
the module. Offline caching is off by default, so kernels built in process
are recompiled each run.

## What this workspace exposes

`InstanceSettings` in `src/engine/runtime.h` passes through the arch choice,
the two capacities, the device memory budget and the CPU thread count, each
optional, plus `read_environment`. Anything else in `CompileConfig` can be
set the same way by extending that struct; the pattern is one `if` per
member, applied before the `Program` is constructed.
