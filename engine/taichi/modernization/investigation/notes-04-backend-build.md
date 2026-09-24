# Notes 04 — backend, AOT and build architecture

Contemporaneous working notes. Appended as I go. Agent 04.

Territory: `taichi/rhi/`, `taichi/aot/`, `c_api/`, `cmake/` + root `CMakeLists.txt`.

---

## Entry 1 — orientation

Read `modernization/PROJECT-PLAN.md` in full and `modernization/deferred-backends.md`.

Key items bearing on my territory:
- 6.1 — `taichi_max_num_snodes = 1024` must become an install-time factor. I need
  to establish how a CMake option could reach `taichi/inc/constants.h`.
- 6.2 — 64-bit addressing; note that SPIR-V 64-bit ints are a *declared capability*.
  That points straight at `taichi/rhi/device_capability.*`.
- 6.3 — loosen architecture for adaptive module loading. Backends.
- 5.2 target tiers: Maxwell / Pascal / Ampere.

Directory inventory:
- `taichi/rhi/`: amdgpu, arch.cpp/h, common, cpu, cuda, device_capability.cpp/h,
  device.cpp/h, dummy.cpp, dx, dx12, impl_support.h, interop, llvm, metal, opengl,
  public_device.h, vulkan.
- `taichi/aot/`: graph_data.cpp/h, module_builder.cpp/h, module_data.h,
  module_loader.cpp/h. Six sources only — smaller than I expected for something
  called "AOT".
- `cmake/`: TaichiCore.cmake, TaichiCAPI.cmake, TaichiCXXFlags.cmake,
  TaichiTargets.cmake, TaichiConfig.cmake.in, utils.cmake, others.

Starting with the build layer since 6.1's threading question is the most concrete.

---

## Entry 2 — build-time parameterisation: is there any precedent?

**VERIFIED.** `taichi/inc/constants.h` is a plain hand-written header. It is NOT
generated. `taichi_max_num_snodes = 1024` sits at line 12, `taichi_max_num_indices
= 12` at line 5, `kMaxNumSnodeTreesLlvm = 512` at line 13.

But there IS an existing configure_file precedent, and it is directly usable:

- `CMakeLists.txt:203` `configure_file(taichi/common/version.h.in ${CMAKE_SOURCE_DIR}/taichi/common/version.h)`
- `CMakeLists.txt:204` `configure_file(taichi/common/commit_hash.h.in ${CMAKE_SOURCE_DIR}/taichi/common/commit_hash.h)`

Note carefully: both configure **into `${CMAKE_SOURCE_DIR}`**, not the binary dir.
`taichi/common/version.h.in` contains `#define TI_VERSION_MAJOR @TI_VERSION_MAJOR@`
etc. So the project already writes generated headers back into the source tree.

Why that detail matters — this is the trap for 6.1:

`taichi/runtime/llvm/runtime_module/CMakeLists.txt:8` compiles `runtime.cpp` with a
**separate, hand-rolled clang invocation** inside `add_custom_target`:

```
COMMAND ${CLANG_EXECUTABLE} ${CLANG_OSX_FLAGS} -c runtime.cpp -o "runtime_${rtm_arch}.bc"
        -fno-exceptions -emit-llvm -std=c++17 -D "ARCH_${rtm_arch}" -I ${PROJECT_SOURCE_DIR};
```

Its only include path is `-I ${PROJECT_SOURCE_DIR}`. It does not inherit any CMake
target's `target_include_directories`. And `runtime.cpp:25` includes
`taichi/inc/constants.h`, and `runtime.cpp:567-569` is exactly the three 1024-sized
arrays named in brief 6.1.

So: if a generated `constants.h` were emitted into the *binary* directory in the
normal CMake way, the LLVM runtime module would silently keep compiling against the
stale source-tree copy, and the host-side C++ and the device-side runtime bitcode
would disagree on the size of `element_lists`, `node_allocators` and
`ambient_elements`. That is a silent ABI split, not a build error.

Two consequences, both verified from the file text:
1. Generating into `${CMAKE_SOURCE_DIR}` (as version.h already does) sidesteps it.
2. Or the `-I` list at runtime_module/CMakeLists.txt:8 must be extended.

I am not choosing between these. Recording both. (Escalation candidate.)

Also verified: that same custom target is run once per arch —
`runtime_module/CMakeLists.txt:29-31` loops `foreach(arch IN LISTS HOST_ARCH
CUDA_ARCH DX12_ARCH AMDGPU_ARCH)`. The arch list is populated in the root
`CMakeLists.txt:154-164`: `CUDA_ARCH` set to "cuda" if `TI_WITH_CUDA`,
`AMDGPU_ARCH` set if `TI_WITH_AMDGPU`, `DX12_ARCH` set if `TI_WITH_DX12`. So the
number of `.bc` artefacts is already a function of the backend toggles. This is the
closest thing in the tree to "per-target module built at install time".

Files in my territory that include constants.h:
- `taichi/rhi/llvm/allocator.h:6`

SURPRISE: nothing under `taichi/aot/` or `c_api/` includes `taichi/inc/constants.h`
directly. `c_api/src/taichi_core_impl.h:121` only mentions it in a comment:
"32 is a magic number in `taichi/inc/constants.h`." — a hardcoded duplicate of
`taichi_max_num_args_extra`. Worth checking.

---

## Entry 3 — how backends are *built*: the TI_WITH_* toggles

Read `cmake/TaichiCore.cmake` in full (438 lines) and `taichi/rhi/CMakeLists.txt`
(120 lines).

**VERIFIED — the toggle list**, `cmake/TaichiCore.cmake:1-11`:

| Option | Line | Default |
|---|---|---|
| `USE_STDCPP` | 1 | OFF |
| `TI_WITH_LLVM` | 2 | ON |
| `TI_WITH_METAL` | 3 | ON |
| `TI_WITH_CUDA` | 4 | ON |
| `TI_WITH_CUDA_TOOLKIT` | 5 | OFF |
| `TI_WITH_AMDGPU` | 6 | OFF |
| `TI_WITH_OPENGL` | 7 | ON |
| `TI_WITH_VULKAN` | 8 | OFF |
| `TI_WITH_DX11` | 9 | OFF |
| `TI_WITH_DX12` | 10 | OFF |
| `TI_WITH_GGUI` | 11 | OFF |

Matches `modernization/deferred-backends.md`. Each carries a `# wheel-tag:` comment.

**VERIFIED — toggles are coerced by host platform before use**, not just by the user:
- `TaichiCore.cmake:35-53` — on APPLE, CUDA/OPENGL/AMDGPU are forced OFF; on
  non-APPLE, METAL is forced OFF.
- `TaichiCore.cmake:55-60` — on WIN32, AMDGPU forced OFF.
- `TaichiCore.cmake:62-64` — `TI_WITH_VULKAN` implies `TI_WITH_GGUI ON`.
- `TaichiCore.cmake:66-69` — OPENGL forced OFF if `external/glad/src/gl.c` missing.
- `TaichiCore.cmake:71-75` — `NOT TI_WITH_LLVM` forces CUDA, CUDA_TOOLKIT, DX12 OFF.

So there is already a *host-inspection-drives-configuration* step in the build. It
is coarse (OS-level, submodule-presence-level), not device-level. Relevant to 6.3
and to the install-time loader in brief section 5.

**VERIFIED — each toggle does exactly three things**, no more:
1. Appends `-DTI_WITH_<X>` to `CMAKE_CXX_FLAGS` (e.g. `TaichiCore.cmake:103`, `109`,
   `115`, `119`, `123`, `127`, `131`; duplicated in `taichi/rhi/CMakeLists.txt:43`,
   `49`, `55`, `61`).
2. Gates an `add_subdirectory()` for that backend's rhi / codegen / runtime /
   program_impl.
3. Gates `target_link_libraries()` of the resulting static lib into `taichi_core`
   or `ti_device_api`.

There is no registry, no plugin manifest, no dlopen. **Backend availability is
purely a link-time fact plus a preprocessor define.** This is the central structural
finding for 6.3.

**VERIFIED — the RHI aggregation target.** `taichi/rhi/CMakeLists.txt:3-4` creates
`ti_device_api` as a STATIC library whose own sources are only `arch.cpp`,
`device.cpp`, `device_capability.cpp` (lines 7-9). Every backend is a separate
static lib linked into it: `metal_rhi` (:45), `opengl_rhi` (:51), `dx_rhi` (:57),
`vulkan_rhi` (:81), `cpu_rhi` (:86), `cuda_rhi` (:90), `amdgpu_rhi` (:95),
`dx12_rhi` (:100), `llvm_rhi` (:104), plus unconditional `interop_rhi` (:108) and
`common_rhi` (:111).

`taichi/rhi/CMakeLists.txt:114-115` then builds `ti_device_api_shared` as a SHARED
library over the same static lib, with `dummy.cpp` (:120) added only to satisfy the
MSVC linker. **This is the closest thing in the tree to a loadable backend module**,
and it is one monolithic .so containing every enabled backend, not one per backend.
Noting for 6.3.

**DEAD CODE found, recording, NOT proposing removal.**
`TaichiCore.cmake:104` globs `taichi/runtime/cuda/runtime.cpp` and `:110` globs
`taichi/runtime/amdgpu/runtime.cpp`. **Neither file exists** — verified by `ls` of
`taichi/runtime/cuda/` (jit_cuda.cpp/.h, kernel_launcher.cpp/.h, CMakeLists.txt)
and `taichi/runtime/amdgpu/` (jit_amdgpu.cpp/.h, kernel_launcher.cpp/.h,
CMakeLists.txt). Both globs are empty.
Separately, `TaichiCore.cmake:111` appends to `TAIHI_CORE_SOURCE` — a typo for
`TAICHI_CORE_SOURCE`, a variable used nowhere else (grepped: only :77, :105, :137,
:340 use the correct spelling). Harmless today only because the glob it appends is
empty. If anyone ever restores an amdgpu runtime.cpp it will be silently dropped.

**VERIFIED — SPIR-V is unconditional.** `TaichiCore.cmake:286-292`:
"# SPIR-V codegen is always there, regardless of Vulkan". `external/SPIRV-Tools`,
`taichi/codegen/spirv` and `taichi/runtime/gfx` are added unconditionally; only the
*linking* of `spirv_codegen`/`gfx_runtime` into taichi_core is gated (`:294-297`) on
OPENGL/VULKAN/DX11/METAL. SPIRV-Cross is added at `:299-302` for OPENGL/DX11/METAL
only — i.e. Vulkan consumes SPIR-V directly, the others go through cross-compilation.
Consistent with deferred-backends.md.

---

## Entry 4 — backend *selection and registration* at runtime

**VERIFIED.** There is no registration mechanism in the usual sense. No registry,
no factory map, no self-registering statics. Selection is a single hand-written
if/else chain in the `Program` constructor, `taichi/program/program.cpp:80-133`,
with every branch wrapped in `#ifdef TI_WITH_<X>`:

| Arch | program.cpp lines | Guard | Availability probe | Impl class |
|---|---|---|---|---|
| llvm archs (x64/arm64/cuda/amdgpu) | 80-83 | `TI_WITH_LLVM` | none | `LlvmProgramImpl` |
| dx12 | 84-92 | `TI_WITH_LLVM` + `TI_WITH_DX12` | `directx12::is_dx12_api_available()` | `Dx12ProgramImpl` |
| metal | 96-102 | `TI_WITH_METAL` | `metal::is_metal_api_available()` | `MetalProgramImpl` |
| vulkan | 103-109 | `TI_WITH_VULKAN` | `vulkan::is_vulkan_api_available()` | `VulkanProgramImpl` |
| dx11 | 110-116 | `TI_WITH_DX11` | `directx11::is_dx_api_available()` | `Dx11ProgramImpl` |
| opengl | 117-123 | `TI_WITH_OPENGL` | `opengl::initialize_opengl(false)` | `OpenglProgramImpl` |
| gles | 124-130 | `TI_WITH_OPENGL` | `opengl::initialize_opengl(true)` | `OpenglProgramImpl` |
| anything else | 131-133 | — | — | `TI_NOT_IMPLEMENTED` |

The `#else` of each guard is `TI_ERROR("This taichi is not compiled with X")`
(:90, :101, :108, :115, :122, :129). The availability probes are `TI_ASSERT`, not
graceful fallbacks (:87, :98, :105, :112, :119, :126).

Two things follow that matter for 6.3:
1. Adding or swapping a backend implementation means editing this chain. There is
   no seam a module could be loaded into.
2. The arch *enum* is not gated at all. `taichi/rhi/arch.h:7-12` builds
   `enum class Arch` from `taichi/inc/archs.inc.h` unconditionally (13 entries:
   x64, arm64, js, cuda, metal, opengl, dx11, dx12, opencl, amdgpu, vulkan, gles).
   `taichi/rhi/arch.cpp` contains **zero** `#ifdef TI_WITH_*`. So `Arch` is a
   complete, build-independent vocabulary, while the *implementations* behind it
   are build-gated. That split is already the right shape for 6.3; the gap is that
   nothing consults a table of what is actually present — it just tries and errors.

`CompileConfig::fit()` at `taichi/program/compile_config.cpp:67-76` does almost
nothing: forces `check_out_of_bound` under debug, forces
`demote_dense_struct_fors` for spirv archs, disables offline cache if needed.
There is **no arch fallback logic in it**, despite `program.cpp:140` commenting
"Must have handled all the arch fallback logic by this point." Recording this
because it is where install-time selection would naturally have landed and it
is empty. (Escalation candidate — I am not deciding what should go there.)

Default arch comes from `CompileConfig::CompileConfig()`,
`compile_config.cpp:9-11`: `arch = host_arch()` then `simd_width =
default_simd_width(arch)`. `host_arch()` (`taichi/rhi/arch.cpp:68-76`) is a pure
preprocessor decision on `TI_ARCH_x64` / `TI_ARCH_ARM`, else `RHI_NOT_IMPLEMENTED`.

---

## Entry 5 — capability detection: TWO parallel systems, and they disagree

This surprised me. There are two entirely separate capability mechanisms.

### System A — `Extension`, per-*arch*, static table, compile-time

`taichi/program/extension.h:17-24`, enum built from
`taichi/inc/extensions.inc.h` (10 entries: sparse, quant, mesh, quant_basic,
data64, adstack, bls, assertion, extfunc).

`taichi/program/extension.cpp:8-33` is a hardcoded `static
std::unordered_map<Arch, std::unordered_set<Extension>>`.

**`Extension::data64` — directly relevant to brief 6.2:**
- HAS data64: `Arch::x64` (:12), `Arch::arm64` (:16), `Arch::cuda` (:20).
- LACKS data64: `Arch::amdgpu` (:22, only `assertion`), `Arch::metal` (:23, empty
  set), `Arch::opengl` (:24, only `extfunc`), `Arch::gles` (:25, empty),
  `Arch::vulkan` (:26, empty), `Arch::dx11` (:27, empty).
- `Arch::dx12`, `Arch::js`, `Arch::opencl` are **absent from the map entirely** —
  `arch2ext[arch]` on a missing key default-constructs an empty set (:31), so they
  silently support nothing.
- `extensions.inc.h:6` comments data64 as "Metal doesn't support 64-bit data
  buffers yet..."

So on the whole portable SPIR-V spine — vulkan, opengl, gles, metal, dx11 — 64-bit
data is declared unsupported by a **static table keyed on arch, not on device**.
That is a hard blocker sitting in front of 6.2 for every non-CUDA GPU path.

`extension.cpp:29-30` carries a commented-out attempt at making it dynamic:
```
  // if (with_opengl_extension_data64())
  // arch2ext[Arch::opengl].insert(Extension::data64); // TODO: singleton
```
An abandoned attempt to feed real device detection into this table. Recording, not
resolving.

### System B — `DeviceCapability`, per-*device*, runtime-populated

`taichi/rhi/device_capability.h:11-15`, enum from
`taichi/inc/rhi_constants.inc.h:8-35` under `PER_DEVICE_CAPABILITY` (25 entries).

Relevant ones for 6.2:
- `spirv_has_int64` — `rhi_constants.inc.h:14`
- `spirv_has_atomic_int64` — `rhi_constants.inc.h:17`
- `spirv_version` — `rhi_constants.inc.h:11`
- `spirv_has_physical_storage_buffer` — `rhi_constants.inc.h:28`

Storage is `DeviceCapabilityConfig` (`device_capability.h:20-33`), a plain
`std::map<DeviceCapability, uint32_t>` with `contains`/`get`/`set`
(`device_capability.cpp:29-42`). `get()` on a missing key returns 0 (:38) — i.e.
**absent silently means "not supported", level 0**. No error, no distinction
between "not probed" and "probed and absent". That is exactly the failure mode
brief 6.2 warns about ("anywhere it silently remains 32 bit").

Note the header comment `rhi_constants.inc.h:1-7`: capabilities are meant to be
"shared intelligence between the runtime environment and the code generator",
about the on-device program only — explicitly not API version/platform facts.

The same file also holds `PER_BUFFER_FORMAT` (:37-82), `PER_IMAGE_DIMENSION`
(:84-88), `PER_IMAGE_LAYOUT` (:90-101), all guarded by their own `#ifdef`.

**These two systems are not connected.** System A decides language-level features
by arch at compile time; System B decides codegen details by device at runtime.
Next: find who *sets* System B's caps and who *reads* `spirv_has_int64`.

---

## Entry 6 — who SETS device capabilities, and who READS spirv_has_int64

Storage lives on the RHI `Device` base: `taichi/rhi/public_device.h:617`
`DeviceCapabilityConfig caps_{};`, with accessors `get_caps()` at :852 and
`set_caps()` at :855.

**VERIFIED — only four backends ever call `set_caps`.** Grepped all of `taichi/rhi/`:

| Backend | set_caps site | Real device query? | Sets `spirv_has_int64`? |
|---|---|---|---|
| Vulkan | `vulkan/vulkan_device_creator.cpp:876` (built from :476) | YES | YES, :632 |
| Vulkan (bare `VulkanDevice::init_vulkan_structs`) | `vulkan/vulkan_device.cpp:1580` (built :1578) | no — only `spirv_version = 0x10000` :1579 | no |
| Metal | `metal/metal_device.mm:1168` (built by `collect_metal_device_caps` :1017-…) | YES | YES, :1052, gated on `feature_64_bit_integer_math` |
| OpenGL | `opengl/opengl_device.cpp:529` (built :508) | partly | YES, :511, gated on `!is_gles()` |
| DX11 | `dx/dx_device.cpp:565` (built :563) | NO — sets only `spirv_version=0x10300` :564 | no |

**CUDA, AMDGPU, CPU, DX12 RHI devices never call `set_caps` at all.** Their
`caps_` stays default-empty, so `get()` returns 0 for every capability
(`device_capability.cpp:33-39`). Verified by grepping `set_caps` across
`taichi/rhi/` — `cuda/`, `amdgpu/`, `cpu/`, `dx12/` do not appear.

That is consistent (those go through LLVM codegen, which consults `Extension`
instead of `DeviceCapability`) but it means **there is no device-capability
record whatsoever for the CUDA path**, which is the path the 5.2 target tiers
are about.

Vulkan's detection is a genuine physical-device query,
`vulkan_device_creator.cpp:623-632`:
```
  vkGetPhysicalDeviceFeatures(physical_device_, &device_supported_features);
  ...
  if (device_supported_features.shaderInt64) {
    device_features.shaderInt64 = true;
    caps.set(DeviceCapability::spirv_has_int64, true);
  }
```
This is the only place in the tree that asks hardware whether it has 64-bit ints.

**Readers of `spirv_has_int64`** — all in SPIR-V codegen, none in RHI:
- `taichi/codegen/spirv/spirv_ir_builder.cpp:64`
- `taichi/codegen/spirv/spirv_ir_builder.cpp:166`
- `taichi/codegen/spirv/spirv_ir_builder.cpp:312` (`if (!caps_->get(...))`)
- `taichi/codegen/spirv/spirv_ir_builder.cpp:326` (`if (!caps_->get(...))`)
- `taichi/rhi/metal/metal_device.mm:132` (`caps.contains(...)`)
(codegen detail is agent 02's territory; recording the sites for the seam.)

**`spirv_has_atomic_int64` is declared and never used.** Declared at
`taichi/inc/rhi_constants.inc.h:17`; exposed in the C++ header-only C-API wrapper
at `c_api/include/taichi/cpp/taichi.hpp:1121`. Grep across `taichi/`, `c_api/src`,
`tests/`, `cpp_examples/` finds **no `set` and no `get`** of it anywhere else.
So a target that has 64-bit ints but not 64-bit atomics cannot currently be
expressed in effect, only named. Relevant to 6.2's stub-architecture note.
Recording, not resolving.

---

## Entry 7 — where the 5.2 target tiers actually live today

**VERIFIED.** CUDA compute capability is detected at runtime in
`taichi/rhi/cuda/cuda_context.cpp:29-33` via
`CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR`/`_MINOR`, combined at :73
(`compute_capability_ = cc_major * 10 + cc_minor`), then **clamped at :75-77**:
```
  if (compute_capability_ > 86) {
    compute_capability_ = 86;
  }
```
and formatted at :83 as `mcpu_ = fmt::format("sm_{}", compute_capability_)`.

Mapping to brief 5.2: GTX 750 (Maxwell) = sm_50, GTX 1070 (Pascal) = sm_61,
RTX 3060 (Ampere) = sm_86. So the tiers are *observable* today, and the clamp at
86 sits exactly on the "Upper" tier boundary — anything newer than Ampere is
reported as Ampere.

Also detected at `cuda_context.cpp:80-82`:
`CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_BLOCK_OPTIN` into
`max_shared_memory_bytes_`. And total/free device memory at :86-88 via
`get_total_memory()` / `get_free_memory()` (:88-96). The 2 GB sizing in the
Baseline tier is therefore queryable.

Consumers of `get_compute_capability()` (grepped, outside cuda_context itself):
- `taichi/codegen/cuda/codegen_cuda.cpp:408`
- `taichi/runtime/llvm/llvm_context.cpp:424` (baked in as an LLVM constant)
- `taichi/runtime/llvm/llvm_context.cpp:504`
- `taichi/python/export_lang.cpp:1230` (exposed to Python)

AMDGPU has a parallel `get_compute_capability()` at
`taichi/rhi/amdgpu/amdgpu_context.h:85`.

Note this is all **runtime JIT-time** detection, not install-time. Brief section 5
wants the decision moved to install time. Recording the gap; not resolving it.

---

## Entry 8 — the AOT module path: what it can and cannot do

### Structure

`taichi/aot/` is six files, 836 lines total:
`graph_data.cpp` 146, `graph_data.h` 183, `module_builder.cpp` 68,
`module_builder.h` 81, `module_data.h` 139, `module_loader.cpp` 94,
`module_loader.h` 125.

`AotModuleBuilder` (`taichi/aot/module_builder.h:17-79`) is an abstract base with
`add`/`add_field`/`add_kernel_template`/`add_graph`, `dump()` pure virtual (:37-38),
and three `_per_backend` hooks that default to `TI_NOT_IMPLEMENTED`
(:48-56, :58-65, :67-71) with `add_per_backend` pure virtual (:46-47).

`aot::Module` (`taichi/aot/module_loader.h:69-122`) is the load side.

### CAN do

**VERIFIED — the module format already carries a declared capability set.**
`aot::ModuleData::required_caps` is `std::map<std::string, uint32_t>` at
`taichi/aot/module_data.h:125`, serialised via `TI_IO_DEF` at :135. The gfx variant
`gfx::TaichiAotData::required_caps` at `taichi/runtime/gfx/aot_utils.h:20`,
serialised at :23. It is populated at build time from the `DeviceCapabilityConfig`
handed to the builder — `taichi/runtime/gfx/aot_module_builder_impl.cpp:22-24`:
```
  for (const auto &pair : caps.to_inner()) {
    ti_aot_data_.required_caps[to_string(pair.first)] = pair.second;
  }
```
So *"this module needs spirv_has_int64"* is already an expressible, serialisable,
round-tripping fact. That is the existing seam for 6.3.

**VERIFIED — target caps are settable from outside, as strings.**
`Program::make_aot_module_builder(Arch, const std::vector<std::string> &caps)` at
`taichi/program/program.cpp:541-557` takes capability *strings* and runs them
through `translate_devcaps()` (`program.cpp:514-539`), which parses `name` or
`name=value` (`:517-531`, `spirv_version=1.3` style per the comment at :515-517)
and defaults `spirv_version` to `0x10300` if absent (:534-537).

This means the AOT builder can already be told to target a capability set that is
**not** the one the build machine has. That is build-time parameterisation of the
target, and it already exists. It is string-keyed and unvalidated beyond
`str2devcap` throwing on an unknown name (`device_capability.cpp:12`).

**VERIFIED — the gfx module is a directory of SPIR-V blobs plus JSON.**
`taichi/runtime/gfx/aot_module_loader_impl.cpp` reads `metadata.json` (:34-46),
then one `<taskname>.spv` per task (:48-69, magic checked against `0x07230203` at
:62), then `graphs.json` (:71-84). Read through `io::VirtualDir`, so it works from
a filesystem directory or a packed container.

### CANNOT do

**VERIFIED — `aot::Module::load` covers only the SPIR-V archs plus dx12.**
`taichi/aot/module_loader.cpp:31-58` is a second hardcoded if/else chain, each arm
`#ifdef`-guarded:
vulkan :32-35, opengl :36-39, gles :40-43, dx11 :44-47 (all → `gfx::make_aot_module`),
dx12 :48-51 (→ `directx12::make_aot_module`), metal :52-55 (→ `gfx::make_aot_module`).
Falls through to `TI_NOT_IMPLEMENTED` at :57.
**`Arch::x64`, `Arch::arm64`, `Arch::cuda`, `Arch::amdgpu` are not handled at all.**
The LLVM archs — including CUDA, the primary GPU path for the target hardware —
have no route through `aot::Module::load`. (The LLVM C-API has a separate entry
point, `c_api/src/taichi_llvm_impl.cpp:78`; checking that next.)

Note the failure shape: if arch matches but the `#ifdef` is false, the `return` is
preprocessed away and control reaches `TI_NOT_IMPLEMENTED` — so "built without this
backend" and "backend has no AOT support" produce the same error.

**VERIFIED, and this is the big one — the capability compatibility check is dead
code.**

`c_api/src/taichi_gfx_impl.cpp:18-29` is the only capability gate at module load:
```
  const DeviceCapabilityConfig &current_devcaps = params.runtime->get_ti_device()->get_caps();
  const DeviceCapabilityConfig &required_devcaps = aot_module->get_required_caps();
  for (const auto &pair : required_devcaps.devcaps) {
    uint32_t current_version = current_devcaps.get(pair.first);
    uint32_t required_version = pair.second;
    if (current_version != required_version) {
      return Error(TI_ERROR_INCOMPATIBLE_MODULE, to_string(pair.first).c_str());
    }
  }
```
But `get_required_caps()` is **never overridden**. Exhaustive grep of the whole repo
excluding `external/` for `get_required_caps` returns exactly two hits: the
declaration+default at `taichi/aot/module_loader.h:97-100` (returns a
function-static empty `DeviceCapabilityConfig`) and the call site above. `AotModuleImpl`
in `taichi/runtime/gfx/aot_module_loader_impl.cpp` does not override it, and
`aot_module_loader_impl.h` contains no `caps` member at all (grepped, no hits).

So: `required_caps` is written into `metadata.json` at build time, deserialised back
into `ti_aot_data_.required_caps` at load time, and then **never connected to the
check**. The loop always iterates an empty map and always passes. There is at
present **no enforcement anywhere that a module's declared requirements match the
device**.

**Second defect in the same check, independent of the first.** It compares with
`!=`, not `<`. Even once wired up, a device reporting `spirv_version = 0x10500`
would be rejected by a module built for `0x10300`. Exact-match is the wrong
relation for "pick the best module this hardware can run". For boolean caps like
`spirv_has_int64` (always set to 1) exact-match happens to behave, but for the
levelled caps it does not.

I am recording both. I am NOT proposing a fix — that is a design decision.
(Escalation.)

**VERIFIED — the check exists only in the C API, and only on the gfx path.**
`Runtime::create_aot_module` is declared at `c_api/src/taichi_core_impl.h:142`;
the gfx override is `c_api/src/taichi_gfx_impl.h:14` /
`taichi_gfx_impl.cpp:7-35`. The LLVM runtime instead overrides the deprecated
`load_aot_module` (`c_api/src/taichi_llvm_impl.h:27`,
`taichi_llvm_impl.cpp:78`). Nothing on the Python/`Program` path performs a
capability check at load.

---

## Entry 9 — the LLVM AOT path is a separate, parallel mechanism

`c_api/src/taichi_llvm_impl.cpp:78-121` `LlvmRuntime::load_aot_module` does **not**
go through `aot::Module::load`. It calls `taichi::lang::LLVM::make_aot_module`
directly. Two arms: cpu (:82-90) and cuda (:91-104, `#ifdef TI_WITH_CUDA`, with
`TI_ASSERT(config.arch == Arch::cuda)` at :93, else `TI_NOT_IMPLEMENTED` :103).
AMDGPU and DX12 have no LLVM AOT load path here.

**No capability check of any kind on this path.**

`LlvmAotModule` (`taichi/runtime/llvm/llvm_aot_module_loader.h:18-92`):
- reuses the **offline cache** file format —
  `cache_reader_(LlvmOfflineCacheFileReader::make(module_path))` at :25, plus
  `graphs.tcb` read at :28-29.
- `version()` returns hardcoded `0` (:36-38).
- `get_root_size()` returns hardcoded `0` (:40-42).
- does not override `get_required_caps()`.
- `make_new_kernel_template` is `TI_NOT_IMPLEMENTED` (:74-77).

`LlvmOfflineCache::Format` exists (`taichi/runtime/llvm/llvm_offline_cache.h:23`)
and defaults to `Format::LL` (:141, :180). So the LLVM AOT artefact is
LLVM IR/bitcode per kernel, loaded and JIT-linked. Structurally that *is* a
loadable module; what it lacks is any declared target requirement.

(Deeper cache-format detail is agent 03's territory. Recording the seam only.)

---

## Entry 10 — the install surface, and the full thread for 6.1

**VERIFIED — the toggle list is re-expressed in three separate places.** Each has
to agree with the others:
1. `cmake/TaichiCore.cmake:1-11` — the `option()` declarations and the
   platform coercions at :25-75.
2. `taichi/rhi/CMakeLists.txt:42-105` — RHI subdirectory + link gating, and it
   *re-appends* the same `-DTI_WITH_*` flags at :43, :49, :55, :61.
3. `cmake/TaichiCAPI.cmake:31-60` — which `c_api/src/*.cpp` files are compiled in.

**VERIFIED — how a value gets from outside into the build.**
`setup.py:110-162` `get_cmake_args()`:
- `setup.py:114` — `cmake_args = shlex.split(os.getenv("TAICHI_CMAKE_ARGS", "").strip())`.
  This env var is the **only** channel for `-DTI_WITH_*=ON/OFF` and for any other
  cache variable. Raw passthrough, no validation.
- `setup.py:144-148` — appends `-DTI_VERSION_MAJOR=`, `_MINOR=`, `_PATCH=`.

So there is already a worked, end-to-end example of a numeric build parameter:
`TI_VERSION_MAJOR` → `setup.py:144` → CMake cache var → root `CMakeLists.txt:203`
`configure_file` → `taichi/common/version.h.in:2` `@TI_VERSION_MAJOR@` →
`taichi/common/version.h` in the **source tree** → included by C++.

**The complete thread 6.1 would need**, stated as observed facts, not as a proposal:
1. A cache variable (the existing `option()`s are all booleans; a numeric one
   would need `set(... CACHE STRING ...)`, no precedent for that in this tree —
   grepped `cmake/` and root, every `option()` is boolean and there is no
   `CACHE STRING` for a build parameter).
2. A `.in` template for `taichi/inc/constants.h`, mirroring
   `taichi/common/version.h.in`.
3. `configure_file(...)` in root `CMakeLists.txt` near :203-204.
4. **The destination must be reachable by the standalone clang command at
   `taichi/runtime/llvm/runtime_module/CMakeLists.txt:8`**, whose only include
   path is `-I ${PROJECT_SOURCE_DIR}` (see Entry 2). The existing precedent
   writes into `${CMAKE_SOURCE_DIR}`, which satisfies this.
5. The value would also need to reach anything that duplicates a constant as a
   literal. One such duplicate found: `c_api/src/taichi_core_impl.h:120-121`
   ```
     // 32 is a magic number in `taichi/inc/constants.h`.
     std::array<uint64_t, 32> host_result_buffer_;
   ```
   That is `taichi_result_buffer_entries = 32` (`constants.h:20`) hardcoded, not
   `taichi_max_num_snodes` — so it is not in 6.1's path, but it is evidence that
   constants do get duplicated across the c_api boundary rather than included.
   I found no hardcoded `1024` duplicate of `taichi_max_num_snodes` in my
   territory (grepped `taichi/rhi/`, `taichi/aot/`, `c_api/`, `cmake/`).

**VERIFIED — the installed package records nothing about which backends were built.**
`cmake/TaichiConfig.cmake.in` is 5 lines: `@PACKAGE_INIT@`, include
`TaichiTargets.cmake`, `check_required_components("Runtime")`.
`cmake/TaichiTargets.cmake:56-88` declares a single IMPORTED target
`taichi_c_api` with a platform-dependent library path. Neither file carries any
`TI_WITH_*` state, any capability set, or any device-tier information. It is
hand-written, per the comment at `cmake/TaichiCAPI.cmake:144-146`
("We used to generate this by `install(EXPORT ...)` ... we turn to a hand written
script instead").

So a downstream install-time loader has **no supported way to ask an installed
Taichi which backends it contains**, other than trying and catching the
`TI_ERROR("This taichi is not compiled with X")`.

`cmake/TaichiCAPI.cmake:179-184` installs the LLVM runtime `.bc` directory
alongside the library when `TI_WITH_LLVM`. Those `.bc` files are the per-arch
device runtimes produced by `runtime_module/CMakeLists.txt:29-31`. They are the
one artefact in the tree that is already *shipped as a loadable module selected
by target*.

---

## Entry 11 — the ONE adaptive module mechanism that already works

**VERIFIED.** `taichi/runtime/llvm/llvm_context.cpp:209-211`:
```
std::string get_runtime_fn(Arch arch) {
  return fmt::format("runtime_{}.bc", arch_name(arch));
}
```
Loaded from `runtime_lib_dir()` (`taichi/util/lang_util.cpp:30-46`), which is
`compiled_lib_dir` if set, else the `TI_LIB_DIR` environment variable (:35-45,
`TI_ERROR_IF` if unset). Same directory holds `slim_libdevice.<major>.bc`
(`llvm_context.cpp:213-218`) and the AMDGPU device libraries
(`llvm_context.cpp:630`: `ocml.bc` and others).

These `.bc` files are produced by
`taichi/runtime/llvm/runtime_module/CMakeLists.txt:29-31`, one per arch in
`HOST_ARCH CUDA_ARCH DX12_ARCH AMDGPU_ARCH`, and installed by
`runtime_module/CMakeLists.txt:13` and `cmake/TaichiCAPI.cmake:179-184`.

So the pattern of *"build several device modules at build time, pick one by name
at load time, from a directory located by an environment variable"* **already
exists and is in production use**. It keys on `arch_name(arch)` only. It does not
key on device tier, compute capability, or capability set. Nothing in the naming
scheme prevents it doing so; nothing in the tree does it.

Caveat: `runtime_module/CMakeLists.txt:13` has a bug —
`install(FILES ".../runtime_${arch}.bc" ...)` uses `${arch}` (the outer foreach
variable) inside a function whose parameter is named `rtm_arch`. Inside
`compile_llvm_runtime()`, `${arch}` is not the function argument. It happens to
resolve because the caller's loop variable `arch` is still in scope at
`:29-31`, but the `COMMAND` on :8 correctly uses `${rtm_arch}`. Recording as
observed; not proposing a change.

---

## Entry 12 — build tooling constraint on adding a non-boolean option (6.1)

`.github/workflows/scripts/ti_build/cmake.py` parses the CMake files to build a
menu of options.
- `cmake.py:17` `OPTION_RE` matches only `option(NAME "desc" ON|OFF)` with an
  optional `# wheel-tag:` suffix. It is collected from `CMakeLists.txt` and
  `cmake/*.cmake` (`cmake.py:147`).
- `cmake.py:44-46` reads `TAICHI_CMAKE_ARGS` and re-parses `-DNAME=value` via
  `DEF_RE` (:18).
- `cmake.py:84` `assert not wheel_tag, "Set a non boolean value to an option with wheel-tag"`.

**VERIFIED by reading the code paths**: a `set(TI_MAX_NUM_SNODES 1024 CACHE STRING ...)`
would NOT be picked up by `OPTION_RE`, so `option_definitions.get(name, ("", None, ""))`
at :57 yields `default=None`, `is_bool=False`, `wheel_tag=""`. The assertion at :60
(`not is_bool or ...`) passes, and the one at :84 passes because `wheel_tag` is empty.
`render()` at :98 emits `-DNAME=value`. So a numeric build parameter passes through
this tooling cleanly and simply does not appear in the wheel tag. No blocker.

Every existing `option()` in `CMakeLists.txt` and `cmake/*.cmake` is boolean —
grepped, there is **no `CACHE STRING` build parameter anywhere in the tree**. A
numeric install-time factor would be the first of its kind.

---

## Entry 13 — width audit of my territory, for 6.2

The RHI's own addressing is **already 64-bit**, which surprised me given 6.2's
framing. Verified in `taichi/rhi/public_device.h`:

| Thing | Type | Line |
|---|---|---|
| `DeviceAllocationId` | `uint64_t` | 85 |
| `DeviceAllocation::alloc_id` | `DeviceAllocationId` | 89 |
| `DeviceAllocation::get_ptr(offset)` | `uint64_t` | 92 |
| `DevicePtr::offset` | `uint64_t` | 123 |
| `Device::AllocParams::size` | `uint64_t` | 623 |
| `get_memory_physical_pointer` | returns `uint64_t` | 635 |
| `kBufferSizeEntireSize` | `size_t` max | 51 |
| buffer bind sizes | `size_t` | 154, 172, 364, 392, 407 |

Genuinely 32-bit in the RHI, and matching the underlying graphics/compute APIs:
- `ShaderResourceSet` binding indices — `uint32_t` (:152, :161, :170, :179, :188, :199)
- `Device::dispatch(x, y, z)` — `uint32_t` (:426-428)
- `ComputeSize{x,y,z}` — `uint32_t` (:431-433)
- image extents/offsets — `uint32_t` (:286-306)

So the 32-bit surface in my territory is dispatch geometry and binding indices,
not addressing. Whether the dispatch dimensions are part of 6.2's "addressing"
is a judgement call I am not making. (Escalation.)

Note: `DeviceCapability` itself is `uint32_t` (`device_capability.h:11`) and
capability *levels* are `uint32_t` (`device_capability.h:22`). `spirv_version` is
stored as a packed hex int, e.g. `0x10300` for 1.3 (`program.cpp:536`,
`vulkan_device_creator.cpp:526-532`, `metal_device.mm:1045`, `dx_device.cpp:564`,
`opengl_device.cpp:528`, `vulkan_device.cpp:1579`).

---

## Entry 14 — the C API backend/capability surface (6.3 territory)

`c_api/include/taichi/*.h` are **generated** from `c_api/taichi.json` by
`misc/generate_c_api.py` (via `misc/taichi_json.py`), then checked in.
`misc/taichi_json.py:401` opens `c_api/taichi.json`.

**VERIFIED — the two enums are handled asymmetrically, and it matters for 6.3.**

`TiCapability` is **auto-derived from the source of truth**. In `taichi.json`:
```
{"name": "capability", "type": "enumeration", "since": "v1.4.0",
 "inc_cases": "PER_DEVICE_CAPABILITY"}
```
`misc/taichi_json.py:182-183` resolves `inc_cases` through `load_inc_enums()`
(:102-116), which globs `taichi/inc/*.inc.h` (:103), regexes `MACRO(NAME)` (:108),
and assigns **values by position** — `cases[key][case_name] = len(cases[key])`
(:115).

Consequence, verified by counting: `taichi/inc/rhi_constants.inc.h` has 25
`PER_DEVICE_CAPABILITY` entries, and `c_api/include/taichi/taichi_core.h:380-407`
has 25 real `TI_CAPABILITY_*` values (0-24) plus `TI_CAPABILITY_MAX_ENUM`. They
line up: `spirv_has_int64` is `rhi_constants.inc.h:14`, the 5th entry, and
`TI_CAPABILITY_SPIRV_HAS_INT64 = 4` at `taichi_core.h:385`.

**So the C API capability enum is a positional ABI over an .inc.h file.**
Inserting a new capability anywhere except the end silently renumbers every
capability after it in the public C ABI, and the header is checked in so the
mismatch would not even show up until regeneration. Constraint on any 6.2/6.3
work that adds a capability. Recording, not resolving.

`TiArch`, by contrast, is a **hand-written list in `taichi.json`**:
```
{"name": "arch", "type": "enumeration", ...,
 "cases": {"reserved":0,"vulkan":1,"metal":2,"cuda":3,"x64":4,"arm64":5,"opengl":6,"gles":7}}
```
Eight cases. `taichi/inc/archs.inc.h` has **thirteen**. Missing from the C API:
`js`, `dx11`, `dx12`, `opencl`, `amdgpu`. So the C API cannot name the AMDGPU or
DirectX backends at all, even when they are compiled in.

**`ti_get_available_archs`** (`c_api/src/taichi_core_impl.cpp:171-205`) is the
only backend-enumeration API in the tree. It caches into a
`thread_local std::vector<TiArch>` (:176) and probes six archs (:178-195).
The probes are at `taichi_core_impl.cpp:13-61`:
- `is_vulkan_available()` :13-19 — `#ifdef TI_WITH_VULKAN` → real
  `vulkan::is_vulkan_api_available()`, else `false`.
- `is_opengl_available()` :21-27 — same shape, real probe.
- `is_cuda_available()` :29-35 — same shape, `taichi::is_cuda_api_available()`.
- `is_x64_available()` :37-45 — pure preprocessor, `TI_WITH_LLVM` + `__x86_64__` etc.
- `is_arm64_available()` :47-53 — pure preprocessor.
- `is_metal_available()` :55-61 — `__APPLE__` + `TI_WITH_METAL`, then real probe.

This is the existing "what can this install actually run" API. It answers at arch
granularity only; it says nothing about device tier or capability level.

**`ti_get_runtime_capabilities`** (`taichi_core_impl.cpp:336-366`) reads
`runtime->get().get_caps()` and copies out `TiCapabilityLevelInfo` pairs.
**`ti_set_runtime_capabilities_ext`** (:317-334) does the reverse — it lets a
caller **override** the device's capability set wholesale via
`runtime2->get().set_caps(std::move(devcaps))` (:331). No validation against what
the hardware actually reports.

Together these two are the closest existing thing to an install-time capability
negotiation surface. Note that on the CUDA path `get_caps()` returns the empty
default (Entry 6), so `ti_get_runtime_capabilities` reports zero capabilities for
CUDA.

---

## Entry 15 — a wrongly shaped stub, per brief 5.1

Brief 5.1: "the correct stub architecture must be built into the MVP base. A
wrongly shaped stub is worse than no stub." Recording one I found.

`taichi/rhi/dx12/dx12_api.cpp:6-12`:
```
bool is_dx12_api_available() {
#ifdef TI_WITH_DX12
  return true;
#else
  return false;
#endif
}
```
It reports *build configuration*, not *hardware availability*. It never touches a
device. So `TI_ASSERT(directx12::is_dx12_api_available())` at
`taichi/program/program.cpp:87` is unconditionally true whenever DX12 is compiled
in, and `make_dx12_device()` at `dx12_api.cpp:14-16` then returns `nullptr`.

Whole DX12 RHI: `dx12_api.h` 20 lines, `dx12_api.cpp` 26 lines, `CMakeLists.txt`
16 lines. Matches `modernization/deferred-backends.md`'s "20-line stub".

Contrast the correctly shaped ones: `vulkan::is_vulkan_api_available()`,
`opengl::is_opengl_api_available()`, `metal::is_metal_api_available()`,
`taichi::is_cuda_api_available()` all do real probing (see Entry 14 call sites).

I am NOT proposing this be fixed or removed. Recording it because 6.3 and 5.1 both
turn on stub shape.

---

## Entry 16 — remaining sweep

`taichi/rhi/device.cpp:46-113` `check_memcpy_capability` / cross-device copy is
gated by nested `#if TI_WITH_VULKAN` / `TI_WITH_LLVM` / `TI_WITH_CUDA`
(:3-13 includes, :53-76 body, :86, :94, :113). Note it uses `#if` rather than
`#ifdef` throughout — works because `-DTI_WITH_X` defines the macro to 1 and an
undefined identifier evaluates to 0 in `#if`, but it is a different idiom from the
`#ifdef` used in `program.cpp` and `module_loader.cpp`.

`taichi/rhi/interop/` is 4 files, all Vulkan↔CPU (`vulkan_cpu_interop.cpp` 72
lines) and Vulkan↔CUDA (`vulkan_cuda_interop.cpp` 230 lines). Linked
unconditionally at `taichi/rhi/CMakeLists.txt:107-108`.

Sizes confirming `deferred-backends.md`:
`taichi/rhi/cpu/` cpu_device.cpp 164 + cpu_device.h 146.
`taichi/rhi/dx12/` 20 + 26 lines of code.

Nothing under `taichi/aot/` or `c_api/` references `taichi_max_num_snodes`,
`kMaxNumSnodeTreesLlvm` or `taichi_max_num_indices`. Confirmed by grepping all
three names across `taichi/`, `c_api/`, `tests/`, `cpp_examples/`, `python/`: the
only hits in my territory are `taichi/rhi/llvm/allocator.h:6` including
`constants.h` (and it uses none of the three — its members are `std::size_t` and
`uint8_t*`, `allocator.h:22-25`).

Done investigating. Writing the report.

---

## Entry 17 — correction to Entry 12

I said in Entry 12 "there is no `CACHE STRING` build parameter anywhere in the
tree". Re-grepped to be precise. Exact result across `cmake/`, root
`CMakeLists.txt`, `taichi/`, `c_api/`:
- `CMakeLists.txt:41` — `set(CMAKE_BUILD_TYPE "Release" CACHE STRING ... FORCE)`.
  That is CMake's own builtin, not a project parameter.
- `cmake/TaichiTests.cmake:7`, `cmake/TaichiCAPITests.cmake:7`,
  `taichi/rhi/CMakeLists.txt:18,19,20,23` — all `CACHE BOOL` forcing third-party
  submodule options (gtest, glfw).
- `c_api/cmake/FindTaichi.cmake:83` — `CACHE PATH` for the install dir, and that
  file is for *consumers* of an installed Taichi, not for building it.

Corrected claim: **there is no project-defined non-boolean CMake cache variable
controlling the build of Taichi itself.** 24 `option()` calls total across those
files, all boolean. A numeric install-time factor would be the first.

---

## Entry 18 — last two verifications, after drafting the report

While drafting I had listed two things as inferred. Went back and read them.

1. **`TI_NOT_IMPLEMENTED` terminates.** `taichi/common/logging.h:108`:
   `#define TI_NOT_IMPLEMENTED TI_ERROR("Not supported.");`
   and `taichi/common/logging.h:40-44`:
   ```
   #define TI_ERROR(...)                      \
     {                                        \
       SPD_AUGMENTED_LOG(error, __VA_ARGS__); \
       TI_UNREACHABLE;                        \
     }
   ```
   So the fall-through at `taichi/aot/module_loader.cpp:57` and the missing-return
   paths in the `#ifdef`-elided arms are well-defined. Upgraded to VERIFIED in the
   report. (`taichi/rhi/public_device.h:32` defines the RHI's own
   `RHI_NOT_IMPLEMENTED` separately.)

2. **`ti_get_available_archs` has a documented contract**, which I had not read.
   `c_api/include/taichi/taichi_core.h:883-897`: an arch is available only if
   "1. The Runtime library is compiled with its support; 2. The current platform is
   installed with a capable hardware or an emulation software", and otherwise
   "a call to `ti_create_runtime` with that arch is guaranteed failing". It also
   warns the returned order is undefined.

   That is exactly the two-part availability question section 6.3 is about, already
   stated as the C API's contract. It is the closest thing to a specification of
   backend availability in the tree. Added to the report at section 3.3.

Also re-verified the c_api line citations used in the report by reading
`taichi_core.h:880-900` and `:920-960`: `ti_get_available_archs` at :898-899,
`ti_create_runtime` at :927-933, `ti_set_runtime_capabilities_ext` at :941-945,
`ti_get_runtime_capabilities` at :950-955. Correct as cited.

Report written to `modernization/investigation/report-04-backend-build.md`.
No files modified outside the two I was assigned.

---
---

# REVISION PASS — after adversarial review

Two adversary files received: `adversary-04-1.md`, `adversary-04-2.md`. They
disagree with each other in places. Below I re-opened the source for every
disputed claim and adjudicated it myself. Entries continue the numbering.

---

## Entry 19 — my `Extension::data64` claim is FALSE. Withdrawing it.

Both adversaries found this independently. I checked it myself before conceding.

```
grep -rn "is_extension_supported" taichi/ c_api/ tests/ --include=*.cpp --include=*.h --include=*.mm
```
Six call sites in the C++ core, plus the declaration and the pybind export:
- `taichi/program/program.cpp:149` — `Extension::assertion`
- `taichi/codegen/llvm/codegen_llvm.cpp:2726` — `Extension::bls`
- `taichi/transforms/compile_to_offloads.cpp:92, 205, 218, 236` — `Extension::mesh`
- `taichi/transforms/compile_to_offloads.cpp:245, 288` — `Extension::quant`
- `taichi/python/export_lang.cpp:1225` — the pybind export

**No call site passes `data64`.** Confirmed by a second grep for `data64` across
`taichi/`, `c_api/`, `tests/` (C++ only): the only hits are the three table rows
that *contain* it (`extension.cpp:12, 16, 20`), the commented-out line
(`:29-30`), and the enum declaration (`extensions.inc.h:6`). No reader.

The actual consumers are Python: `python/taichi/lang/misc.py:186` exposes it in
the extension name list, and it is used as a test-skip predicate — e.g.
`tests/python/test_svd.py:8`, `tests/python/test_quant_atomics.py:44`,
`tests/python/test_ad_basics.py:249`, `tests/python/test_floor_dtype_argument.py:43`.
Brief 1.2 puts the Python front end out of scope.

**So the table blocks nothing in the C++ core.** My report's sentence — "a
compile-time blocker sitting in front of 6.2 for every non-CUDA GPU path",
marked [V] — is false. The table content and every line citation under it are
correct; the *consequence* I drew from them is not. This is the failure mode the
adversaries name: generalising one step past what I read, while marking the
generalisation verified.

Escalation 4 in my report is correspondingly deflated. It is not two capability
systems in conflict at the point where 64-bit data is decided. It is one live
system (`DeviceCapability`) and one declaration that only out-of-scope code
reads. Restating it as a scope question, not a reconciliation question.

Additional detail neither I nor my first pass caught: `extension.cpp:31` reads
`arch2ext[arch]` — non-const `operator[]` on a function-static
`std::unordered_map` — so a lookup on a key absent from the initialiser (dx12,
js, opencl) **inserts** into shared static state, unsynchronised. I reported the
observable behaviour ("default-constructs an empty set") without noticing it
mutates. Recording; not proposing anything.

---

## Entry 20 — my `ti_device_api_shared` claim is FALSE on both halves. Withdrawing it.

I said (report §2.2, marked [V]): "This is the only shared-library backend
artefact, and it contains every enabled backend at once."

**Half one — content.** `ls -la taichi/rhi/dummy.cpp` → **0 bytes**. Confirmed by
`wc -c` as well. The target's sources are `public_device.h`
(`taichi/rhi/CMakeLists.txt:114`) and that empty file (`:120`). A header is not a
translation unit, so the shared object has exactly one compiled source and it is
empty. Linking a static archive into a shared object extracts only members that
resolve undefined symbols, and an empty translation unit has none. It therefore
contains no backend at all, let alone every one. Adversary 04-2 made this
argument on link semantics; adversary 04-1 initially had only the weaker "it is
unused" and then adopted 04-2's version. I verified 04-2's is the correct one.

**Half two — artefact.** Grep for `ti_device_api_shared` across the whole tree
excluding `external/` returns exactly three lines, all in
`taichi/rhi/CMakeLists.txt`: `:114`, `:115`, `:120`. Nothing links it. There is
no `install(TARGETS ti_device_api_shared ...)` anywhere. Every real consumer
links the **static** `ti_device_api`: `cmake/TaichiCore.cmake:146`,
`taichi/ui/ggui/CMakeLists.txt:48`, `taichi/runtime/llvm/CMakeLists.txt:42`,
`taichi/runtime/program_impls/vulkan/CMakeLists.txt:24`.

It would also export nothing even if it were populated, because
`cmake/TaichiCore.cmake:18` sets `CMAKE_CXX_VISIBILITY_PRESET hidden` globally.

My phrasing invited the reader to treat this as an existing shared-object seam
for 6.3. There is no such seam. Withdrawing.

---

## Entry 21 — three more of my [V] claims that do not survive

Checked each myself.

**(a) "Each toggle does exactly three things and nothing else" — FALSE.**
Four counter-examples, all read directly:
- `cmake/TaichiCore.cmake:104-105` — `TI_WITH_CUDA` also drives a `file(GLOB)`
  and a `list(APPEND)`.
- `cmake/TaichiCore.cmake:62-64` — `TI_WITH_VULKAN` forces `TI_WITH_GGUI ON`.
- `cmake/TaichiCore.cmake:278-283` — `TI_WITH_CUDA` with `TI_WITH_CUDA_TOOLKIT`
  runs `find_package(CUDAToolkit REQUIRED)` (:279) and links `CUDA::cupti` (:283).
- `CMakeLists.txt:154-164` — `TI_WITH_CUDA`, `TI_WITH_AMDGPU` and `TI_WITH_DX12`
  set `CUDA_ARCH`, `AMDGPU_ARCH`, `DX12_ARCH`, which drive the bitcode arch loop
  at `taichi/runtime/llvm/runtime_module/CMakeLists.txt:29`.

Ironically I *documented* the fourth of these in my own §2.3 and still wrote
"nothing else" in §2.1. Withdrawing the absolute.

**(b) "No project-defined non-boolean CMake cache variable controlling the build
of Taichi itself" — FALSE.** `cmake/TaichiCXXFlags.cmake:149`:
```
set(HOST_ARCH ${ARCH} CACHE INTERNAL "Host arch")
```
where `ARCH` is set to `"x64"` (:139), `"arm64"` (:142) or `"x86"` (:145). That
is project-defined, non-boolean, cached, and it is the **first element** of the
`foreach` at `runtime_module/CMakeLists.txt:29` that decides which
`runtime_<arch>.bc` files get built. I even cited that loop. My entry 17
"correction" narrowed the claim to `CACHE STRING` and then the report restated
the general version anyway. Withdrawing.

Consequently my §2.3 statement that `HOST_ARCH CUDA_ARCH DX12_ARCH AMDGPU_ARCH`
"are set in root `CMakeLists.txt:154-164`" is wrong for one of the four. Only
`CUDA_ARCH` (:155), `AMDGPU_ARCH` (:159) and `DX12_ARCH` (:163) are set there.

**(c) "The only hardware query for 64-bit integers in the entire tree is
`vulkan_device_creator.cpp:623-632`" — FALSE, and it contradicts my own table
three paragraphs earlier.** My §4.2 table already recorded Metal setting
`spirv_has_int64` gated on `feature_64_bit_integer_math`. That flag comes from
real device queries: `[mtl_device supportsFamily:]` at
`taichi/rhi/metal/metal_device.mm:1027-1036`, reduced at `:1038`, consumed at
`:1051-1053`. Two backends query hardware for 64-bit integers, not one.

---

## Entry 22 — my `set_caps` census was under-scoped and presented as complete

I grepped only `taichi/rhi/` and said so *in the notes*, but the report's summary
point 3 presents "only four backends ever call `set_caps`" as the producer
picture. Re-ran over `taichi/` and `c_api/` together. **Eight call sites**,
excluding the declaration at `taichi/rhi/public_device.h:855`:

| # | site | scope |
|---|---|---|
| 1 | `taichi/rhi/metal/metal_device.mm:1168` | rhi |
| 2 | `taichi/rhi/vulkan/vulkan_device.cpp:1580` | rhi |
| 3 | `taichi/rhi/vulkan/vulkan_device_creator.cpp:876` | rhi |
| 4 | `taichi/rhi/opengl/opengl_device.cpp:529` | rhi |
| 5 | `taichi/rhi/dx/dx_device.cpp:565` | rhi |
| 6 | `c_api/src/taichi_opengl_impl.cpp:12` | c_api |
| 7 | `c_api/src/taichi_vulkan_impl.cpp:53` | c_api |
| 8 | `c_api/src/taichi_core_impl.cpp:331` | c_api, host-supplied |

Five RHI backends, not four — I counted "four backends" because Vulkan has two
sites. The table itself was right; the sentence above it was not. And three
producers sit outside the scope I grepped. I cited site 8 separately in §3.3 and
never reconciled the counts.

Adversary 04-2 claimed pass B said Metal does not call `set_caps`. I checked:
`metal_device.mm:1168` does call it. My table was right on Metal and pass B was
wrong. Recording, since I was asked to adjudicate where the two disagree.

---

## Entry 23 — the c_api OpenGL producer overwrites correct GLES detection

Found by adversary 04-1, verified by me, and it is 6.2-relevant.

`taichi/rhi/opengl/opengl_device.cpp:506-529`, `GLDevice::GLDevice`, builds caps
with real detection: `spirv_has_int64` and `spirv_has_float64` only when
`!is_gles()` (:509-513, comment at :510 "64bit isn't supported in ES profile"),
plus `spirv_has_int16`/`float16` gated on `GLAD_GL_NV_gpu_shader5` (:515-518),
`GLAD_GL_AMD_gpu_shader_int16` (:520-522) and `GLAD_GL_AMD_gpu_shader_half_float`
(:524-526). Then `set_caps` at :529.

`c_api/src/taichi_opengl_impl.cpp:4-13`, `OpenglRuntime::OpenglRuntime`, then
constructs a **fresh** `DeviceCapabilityConfig` (:8) and calls `set_caps` again
(:12), discarding all of the above. It sets `spirv_has_int64` (:9) and
`spirv_has_float64` (:10) **unconditionally, with no `is_gles()` guard**, and
drops the int16/float16 detection entirely.

Scope caveat I checked before overstating it: `ti_create_runtime`
(`c_api/src/taichi_core_impl.cpp:251-306`) has no `TI_ARCH_GLES` case, and
`OpenglRuntime` is constructed for `Arch::opengl` (`taichi_opengl_impl.cpp:5`),
whose `GLDevice` calls `initialize_opengl(false, true)` (`opengl_device.cpp:507`)
— the `false` being "not GLES". So a GLES context is not reachable through this
path today. `TI_ARCH_GLES = 7` exists in the enum
(`c_api/include/taichi/taichi_core.h:373`) with no runtime constructor.

The verifiable defect is therefore: the C API replaces detected capabilities
with a hardcoded set, losing the int16/float16 detection outright and losing the
GLES guard on 64-bit. Whether the GLES guard is currently reachable is a separate
question and I am not claiming it is.

---

## Entry 24 — what I missed that is the most 6.2-relevant fact in my territory

`spirv_has_physical_storage_buffer` — buffer device address, i.e. 64-bit pointers
*inside shaders*. I cited its declaration line (`rhi_constants.inc.h:28`) in my
report and never checked it. Both adversaries flag this; adversary 04-1 calls it
"the single most 6.2-relevant fact in the territory". I verified the whole census.

**Two producers. Both compiled out.**
- `taichi/rhi/vulkan/vulkan_device_creator.cpp:826`, inside
  `#if !defined(__APPLE__) && false` opened at `:825` and closed at `:827`. The
  comment at `:824` reads "(penguinliong) Temporarily disabled (until device
  capability is ready)", and `:822-823` reference taichi issue 6295. Note `:821`
  guards it on `device_supported_features.shaderInt64` — so the intended
  behaviour was to couple shader 64-bit pointers to hardware 64-bit integers.
- `c_api/src/taichi_vulkan_impl.cpp:48`, inside a `/* */` block spanning
  `:46-51`, preceded at `:45` by "(penguinliong) Will bring it back after devcap."

**Twelve live consumers**, all of which therefore permanently take the false
branch. I opened and counted all twelve:
`taichi/rhi/vulkan/vulkan_device.cpp:1772, 1792, 2147, 2509`;
`taichi/codegen/spirv/spirv_ir_builder.cpp:73, 113`;
`taichi/codegen/spirv/spirv_codegen.cpp:783, 2340, 2416, 2491`;
`taichi/runtime/gfx/runtime.cpp:96`;
`taichi/runtime/program_impls/gfx/gfx_program.h:81`.

The sharpest is the last. `gfx_program.h:79-83`:
```
  std::string get_kernel_argument_data_layout() override {
    auto has_buffer_ptr = runtime_->get_ti_device()->get_caps().get(
        DeviceCapability::spirv_has_physical_storage_buffer);
    return "1" + std::string(has_buffer_ptr ? "b" : "-");
  };
```
always returns `"1-"`.

So on the SPIR-V spine a substantial part of the 64-bit addressing machinery is
already written and compiled, and is switched off at the capability layer rather
than absent. That is a materially different starting position for 6.2 than my
report implied. Adding it.

---

## Entry 25 — three more things I missed, verified

**(a) A third hardcoded dispatch chain.** `ti_create_runtime`,
`c_api/src/taichi_core_impl.cpp:247-309`, a `switch (arch)` with `#ifdef`-guarded
cases: `TI_ARCH_VULKAN` :252-267, `TI_ARCH_OPENGL` :268-274, `TI_ARCH_X64`
:275-281, `TI_ARCH_ARM64` :282-287, `TI_ARCH_CUDA` :288-293, `TI_ARCH_METAL`
:295-301, `default` → `TI_CAPI_NOT_SUPPORTED(arch)` :302-305. No gles, dx11,
dx12 or amdgpu case. My report claimed there were two chains. There are three.

**(b) The CUDA device index is pinned to 0, on a two-card box.**
`taichi/rhi/cuda/cuda_context.cpp:21-22`:
```
  driver_.device_get_count(&dev_count_);
  driver_.device_get(&device_, 0);
```
The count is read and then ignored. And `ti_create_runtime` rejects any non-zero
`device_index` for every arch except Vulkan —
`TI_CAPI_NOT_SUPPORTED_IF_RV(device_index != 0)` at `:270` (opengl), `:277`
(x64), `:283` (arm64), `:289` (cuda), `:297` (metal). Only Vulkan honours it, via
`set_vulkan_visible_device(std::to_string(device_index))` at `:254-255`.

Brief 3.1 names two NVIDIA cards in this box, a GTX 750 and a GTX 1070. Today
CUDA can only ever see device 0.

**(c) `ProgramImpl::get_device_caps()` is a second, separate path to an empty
capability set.** `taichi/program/program_impl.h:170-172` returns `{}`. Exactly
one override exists: `GfxProgramImpl::get_device_caps`
(`taichi/runtime/program_impls/gfx/gfx_program.cpp:82`, declared
`gfx_program.h:85`). It is surfaced by `Program::get_device_caps`
(`taichi/program/program.h:136-137`) and feeds kernel compilation at
`taichi/aot/graph_data.cpp:36` and six sites in
`taichi/program/snode_rw_accessors_bank.cpp:45, 54, 68, 80, 89, 100`.

So the LLVM archs compile kernels against an empty capability set through this
path, independently of the RHI `Device::caps_` emptiness I already documented.
Two mechanisms, same hole.

---

## Entry 26 — the AMDGPU precedent: module selection keyed on DETECTED HARDWARE

I cited `llvm_context.cpp:630` in passing as "the AMDGPU device libraries" and
missed what it is. Re-read `taichi/runtime/llvm/llvm_context.cpp:625-655`:

```
  auto isa_version = AMDGPUContext::get_instance().get_mcpu().substr(3, 4);   // :629
  std::string libdevice_files[] = {"ocml.bc",
                                   ...
                                   "oclc_isa_version_" + isa_version + ".bc",  // :637
                                   ...};
  for (auto &libdevice : libdevice_files) {                                    // :641
    std::string lib_dir = runtime_lib_dir() + "/";                             // :642
    auto libdevice_module = module_from_bitcode_file(lib_dir + libdevice, ...) // :643
```

The bitcode filename is composed from the **detected ISA version of the actual
device**, not from the arch name. That is materially different from
`get_runtime_fn(arch)` at `:209-211`, which keys on `arch_name(arch)` alone, and
it is the closest thing in the tree to what 6.3 asks for: the right module
selected for the hardware present.

The `.bc` files come from `external/amdgpu_libdevice/*.bc`, installed by
`cmake/TaichiCore.cmake:434-438`. They are vendor-supplied, not built here — so
the precedent is for *selection*, not for *production*. Stating it that way.

---

## Entry 27 — GAP: no arch and no version handshake on AOT load

I framed 6.3's load-time problem as "one dead capability check to wire up". That
is one third of it. Verified all three handshakes:

**Capability** — as I reported: `get_required_caps()` never overridden, two grep
hits only (`taichi/aot/module_loader.h:97-100` and
`c_api/src/taichi_gfx_impl.cpp:21`). Stands.

**Version** — `aot::Module::version()` is pure virtual at
`taichi/aot/module_loader.h:85`. Three overrides:
- `taichi/runtime/gfx/aot_module_loader_impl.cpp:114-116` — `TI_NOT_IMPLEMENTED`,
  i.e. it aborts.
- `taichi/runtime/dx12/aot_module_loader_impl.cpp:67-69` — `TI_NOT_IMPLEMENTED`.
- `taichi/runtime/llvm/llvm_aot_module_loader.h:36-38` — returns `0`.

**No caller anywhere.** I noted the LLVM one returns 0 and did not notice the
other two abort or that nothing calls any of them.

**Arch** — `aot::Module::arch()` is pure virtual at `module_loader.h:84`. The gfx
override (`aot_module_loader_impl.cpp:111-113`) and the dx12 override
(`dx12/aot_module_loader_impl.cpp:64-66`) both return `device_api_backend_`,
which is the arch handed in **from the runtime** at
`taichi/aot/module_loader.cpp:31`, not anything read from the module. No caller
anywhere either — I grepped `arch()` across `taichi/` and `c_api/` and every hit
is on a kernel-level `CompiledKernelData`, never on an `aot::Module`.

And `metadata.json` records no arch at all: the serialised field list is
`TI_IO_DEF(kernels, fields, required_caps, root_buffer_size)`
(`taichi/runtime/gfx/aot_utils.h:23`), mirrored at `taichi/aot/module_data.h:135`.

**Consequence:** a module built for Vulkan can be handed to an OpenGL runtime and
nothing objects.

The pattern exists one layer down and was not lifted to the module:
`taichi/compilation_manager/kernel_compilation_manager.cpp:225`
`TI_ASSERT(loaded->arch() == arch)`, plus
`taichi/runtime/cuda/kernel_launcher.cpp:191` and
`taichi/runtime/amdgpu/kernel_launcher.cpp:154`.

---

## Entry 28 — GAP: `required_caps` has one producer, and the generic struct is dead

Adversary 04-2's finding; verified. There are **two** structs, not one:
- `gfx::TaichiAotData::required_caps` (`taichi/runtime/gfx/aot_utils.h:20`,
  serialised `:23`) — written at
  `taichi/runtime/gfx/aot_module_builder_impl.cpp:23`. Live.
- `aot::ModuleData::required_caps` (`taichi/aot/module_data.h:125`, serialised
  `:135`) — **never written by anybody.** Its only subclass in the tree is
  `ModuleDataDX12 : public aot::ModuleData`
  (`taichi/runtime/dx12/aot_module_builder_impl.h:11`, instantiated at
  `taichi/runtime/dx12/aot_module_loader_impl.cpp:118`), and the DX12 builder
  populates no caps.

My report described "the AOT module format" as carrying a capability set that
round-trips, citing both structs as though they were one channel. The generic one
is a declaration with no producer. Correcting.

---

## Entry 29 — GAP: changing the SNode ceiling invalidates nothing

My central 6.1 finding is that host C++ and the device bitcode could silently
disagree on the size of the three arrays at `runtime.cpp:567-569`. That stands.
The same hazard exists one layer up and I did not reach it.

`taichi/util/offline_cache.h:95-100`:
```
  if (ver[0] != TI_VERSION_MAJOR || ver[1] != TI_VERSION_MINOR ||
      ver[2] != TI_VERSION_PATCH) {
    ...
    return LoadMetadataError::kVersionNotMatched;
  }
```
The offline cache validates on the Taichi version and **nothing else**. And
`LlvmAotModule` reuses that same reader —
`cache_reader_(LlvmOfflineCacheFileReader::make(module_path))` at
`taichi/runtime/llvm/llvm_aot_module_loader.h:25`.

A rebuild that changes `taichi_max_num_snodes` does not move `TI_VERSION`. So
kernels compiled against the old struct layout will be loaded and run against the
new one, from both the offline cache and an LLVM AOT module.

I had explicitly scoped the offline cache *format* to agent 03. That scoping was
defensible for the format; the *invalidation seam* is in territory 04's half of
6.1 and I should have reached it. Adding it as a finding and an escalation.

Separate but adjacent, and it cuts the other way:
`taichi/analysis/offline_cache_util.cpp:88-95`
`get_offline_cache_key_of_device_caps` serialises `caps.devcaps` directly at
`:92` — the raw `std::map<DeviceCapability, uint32_t>`, keyed by the **numeric
enum value** — and the result is hashed into the kernel key at `:185` and `:190`.
So renumbering `PER_DEVICE_CAPABILITY` changes offline-cache keys as well as the
public C ABI. My report treated positional renumbering purely as a C ABI
question. Both consumers exist.

Net: the capability map *is* hashed into the kernel key; the SNode ceiling is
hashed into nothing.

---

## Entry 30 — GAP: a CMake operator-precedence bug in my own territory

`taichi/rhi/CMakeLists.txt:17`:
```
if (TI_WITH_OPENGL OR TI_WITH_VULKAN AND NOT ANDROID)
```
CMake binds `NOT` tighter than `AND`, and `AND` tighter than `OR`. So this parses
as `TI_WITH_OPENGL OR (TI_WITH_VULKAN AND (NOT ANDROID))`. The `NOT ANDROID`
guard therefore does not apply to the OpenGL arm: with OpenGL on and `ANDROID`
set, GLFW is still added as a subdirectory (`:29`), linked (`:30`) and put on the
public include path (`:31-34`).

I read this file line by line and enumerated every `target_link_libraries` in it,
and did not look at the condition on line 17. Recording. Whether Android is a
target at all is not settled anywhere in the brief, so I am not proposing a
change.

---

## Entry 31 — GAP: `CUDA_VERSION` already does exactly what 6.1 needs

This is the most consequential thing I missed, because my escalation 1 stops
precisely where this example resolves it. Verified end to end:

1. `cmake/TaichiCore.cmake:97-100`:
   ```
   ## This version var is only used to locate slim_libdevice.10.bc
   if(NOT CUDA_VERSION)
       set(CUDA_VERSION 10.0)
   endif()
   ```
   A default that yields to `-DCUDA_VERSION=...` on the cmake command line, and
   therefore to `TAICHI_CMAKE_ARGS` (`setup.py:114`) and to `DEF_RE` in
   `.github/workflows/scripts/ti_build/cmake.py:18`.
2. `CMakeLists.txt:149-152` forces it to `"0.0"` when CUDA is off.
3. `taichi/common/version.h.in:5`: `#define CUDA_VERSION "@CUDA_VERSION@"`,
   substituted by the `configure_file` at `CMakeLists.txt:203`.
4. `taichi/common/core.cpp:87-89`: `get_cuda_version_string()` returns it.
5. `taichi/runtime/llvm/llvm_context.cpp:213-218`: parses it and builds the
   bitcode filename `slim_libdevice.<major>.bc`.

**A non-boolean CMake value, carried into C++ through a generated header, used at
run time to choose which device bitcode to load.** That is the 6.1 mechanism, and
it already exists in this tree, three lines above the CUDA `-D` block I cited.

One hazard I checked before offering it. `taichi/rhi/cuda/cupti_toolkit.cpp:263`
uses `#if CUDA_VERSION >= 11040` (also `:502`, `:725`) — an integer comparison
that would misbehave against the string macro. It does not collide today because
`taichi/common/version.h` has only three includers:
`taichi/common/core.cpp:7`, `taichi/util/offline_cache.h:12`,
`taichi/runtime/llvm/llvm_offline_cache.cpp:14`. `cupti_toolkit.cpp` is not among
them; its `CUDA_VERSION` is the CUDA Toolkit's own macro. But
`taichi/inc/constants.h` is included far more widely, so if a generated
`constants.h` introduced macros rather than `constexpr` values, this class of
collision becomes live. That belongs in the escalation, not in the findings.

---

## Entry 32 — GAP: the `-I` route is not a variant, it is a defect

My escalation 1 offered two routes for 6.1: generate into `${CMAKE_SOURCE_DIR}`,
or extend the `-I` list at `runtime_module/CMakeLists.txt:8`. Adversary 04-2 is
right that these are not symmetric, and I accept it on reading the command again:

```
COMMAND ${CLANG_EXECUTABLE} ... -I ${PROJECT_SOURCE_DIR};
```

If a generated header is placed elsewhere and that directory is *added* to the
include path, the checked-in `taichi/inc/constants.h` still exists and
`-I ${PROJECT_SOURCE_DIR}` is still on the command line. Two headers of the same
relative path are then both visible and `-I` order decides the winner. That is a
silently order-dependent build — the same class of failure the whole finding is
about. Whichever route is chosen, the source-tree header has to stop being a
header, not merely be shadowed.

Demoting it from "one of two options" to "a defect in that option".

Related, and it changes the hygiene question underneath escalation 1: the
artefact side **already** pollutes the source tree.
`taichi/runtime/llvm/runtime_module/CMakeLists.txt:10` sets
`WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"`, so `runtime_<arch>.bc` is
produced next to `runtime.cpp`, and `:13` installs it from
`${CMAKE_SOURCE_DIR}/taichi/runtime/llvm/runtime_module/`. The in-file TODO at
`:9` says exactly this: "it's better to avoid polluting the source dir, keep in
build". So "generate into the source tree" is not a new concession; it is the
existing convention for both headers and bitcode.

---

## Entry 33 — GAP: the umbrella C header, and a toggle that does not exist

`c_api/include/taichi/taichi.h:1-29` gates every per-backend public header behind
a `TI_WITH_*` macro: `TI_WITH_VULKAN` :9, `TI_WITH_OPENGL` :13, `TI_WITH_CUDA`
:17, `TI_WITH_CPU` :21, `TI_WITH_METAL` :25.

These are **consumer-side** macros. The installed package defines none of them:
`cmake/TaichiConfig.cmake.in` is five lines and `cmake/TaichiTargets.cmake:56-88`
sets only `IMPORTED_LOCATION`, `IMPORTED_SONAME` and
`INTERFACE_INCLUDE_DIRECTORIES`. So a downstream consumer that includes
`taichi/taichi.h` gets `taichi_core.h` and nothing else unless it defines the
macros itself, guessing what the binary was built with. This is the
consumer-facing form of my escalation 10, and it is sharper than what I had.

This is the **fourth** re-expression of the toggle list. My report named three.

And `TI_WITH_CPU` is defined nowhere. Grep across the repo excluding `external/`
and `build/` returns `c_api/include/taichi/taichi.h:21` and `:23` — and
`docs/lang/articles/deployment/tutorial.md:288`, which lists it under "Other
commonly used CMake options" alongside `TI_WITH_OPENGL` and `TI_WITH_CUDA`. So it
is *documented as a build option* while being neither an `option()` nor a define
anywhere. `taichi_cpu.h` is therefore unreachable through the umbrella header,
although `cmake/TaichiCAPI.cmake:33` installs it whenever `TI_WITH_LLVM` is on.

---

## Entry 34 — ADJUDICATING the one place the adversaries genuinely disagree

**The question:** is the `!=` at `c_api/src/taichi_gfx_impl.cpp:25` a defect, or
a contract question?

- Adversary 04-1: a contract question. It cites
  `c_api/include/taichi/taichi_core.h:411-412` and says `!=` "is the only
  relation the documented type supports", so it "is not a defect".
- Adversary 04-2: both reports were right to escalate it as a problem; adds only
  that fixing it alone is insufficient. It does not cite `:411-412`.

I was told not to adopt whichever sounds more certain. Going to source.

**Fact 1 — the documentation.** `c_api/include/taichi/taichi_core.h:409-416`:
```
// Structure `TiCapabilityLevelInfo` (1.4.0)
//
// An integral device capability level. It currently is not guaranteed that a
// higher level value is compatible with a lower level value.
typedef struct TiCapabilityLevelInfo {
  TiCapability capability;
  uint32_t level;
} TiCapabilityLevelInfo;
```
Adversary 04-1 quoted this accurately. Adversary 04-2 did not have it.

**Fact 2 — but the engine already relies on ordering.** I grepped every
comparison on `spirv_version`. There are **nine ordered comparisons** in the
engine's own code:
- `<` — `taichi/codegen/spirv/spirv_ir_builder.cpp:556, 625, 671, 679, 741, 777`
  (all `< 0x10300`), and `taichi/rhi/vulkan/vulkan_device_creator.cpp:577`
  (`< 0x10400`).
- `>` — `taichi/codegen/spirv/spirv_codegen.cpp:1908` (`> 0x10300`).
- `>=` — `taichi/codegen/spirv/spirv_ir_builder.h:424` (`>= 0x10400`).

**My adjudication. Neither adversary has it right, and 04-1 overstates.**

The doc says "not **guaranteed**", which is weaker than "not ordered". It
declines to promise ordering; it does not assert disorder. `!=` is *consistent*
with that, and so is any other conservative relation — the doc leaves the
relation undefined rather than fixing it at `!=`.

Meanwhile the SPIR-V code generator treats `spirv_version` as fully ordered in
nine places, including choosing which SPIR-V constructs to emit. So the codebase
already contains the ordering assumption the C API declines to publish, for at
least one capability.

So the accurate statement is: **the relation is undecided in the published
contract and already assumed ordered internally for `spirv_version`.** `!=` is
neither a plain bug (04-2's framing, which I originally shared) nor forced by the
documentation (04-1's framing). It is a genuine contract decision, and the source
shows the contract is already contradicted internally for the one capability that
is actually a version number.

That distinction matters practically: a change to `>=` would be sound for
`spirv_version` and meaningless-to-harmful for the booleans-stored-as-levels,
which is a per-capability question the current single-relation loop cannot
express. Moving it to Escalations framed that way, and saying explicitly that I
adjudicated it rather than adopting either adversary.

---

## Entry 35 — citation corrections, all re-checked

| My report said | Source says | Checked by |
|---|---|---|
| `archs.inc.h` has 13 entries | **12** (`grep -c "^PER_ARCH"` → 12) | me |
| `TiArch` 8 cases against 13 | 8 enumerators incl. `TI_ARCH_RESERVED=0`, so **7 of 12** named; 5 missing (js, dx11, dx12, opencl, amdgpu) — my missing-list was right | me |
| `taichi/aot/` is 6 files | **7** (`ls -1 | wc -l` → 7), 836 lines; my own list named seven | me |
| `taichi_core_impl.h:120-121` | comment `:121`, array `:122` | me |
| `runtime_lib_dir` `:30-46` | closes at **`:47`** | me |
| `version.h.in:2-6` | file is **5 lines** | me |
| `-DTI_WITH_*` appends: 7 sites | **9** — add `:94` (LLVM) and `:281` (CUDA_TOOLKIT) | me |
| `program.cpp` `#else` arms: 6 | **7** — add `:94`, the LLVM one, which is the outermost and largest branch | me |
| `cuda_context.cpp:80-82` shared mem | `:79-81` | me |
| `cuda_context.cpp:88-96` memory | `get_total_memory` `:88-92`, `get_free_memory` `:94-98` | me |
| `public_device.h:426-433` dispatch+ComputeSize | `dispatch` `:426-428`, `ComputeSize` `:430-434` | me |
| `spirv_has_atomic_int64` grep: 2 hits | **3** — add `python/taichi/lang/enums.py:27` | me |
| "only four backends call `set_caps`" | five RHI sites + three c_api = **eight** (Entry 22) | me |
| `HOST_ARCH` set at `CMakeLists.txt:154-164` | set at `cmake/TaichiCXXFlags.cmake:149`; only `CUDA_ARCH` `:155`, `AMDGPU_ARCH` `:159`, `DX12_ARCH` `:163` are set in the root file | me |

Adversary 04-2 also flagged my §3.1 "Thirteen entries" propagating into
escalation 9. Corrected there too.

Now rewriting `report-04-backend-build.md` in place.

---

## Entry 36 — revision complete

`report-04-backend-build.md` rewritten in place. 1111 lines.

Structure changed to carry the new material: section 2 now separates the toggle
mechanism (2.1) from what a toggle *also* does (2.2), and 2.4 leads with the
`CUDA_VERSION` thread rather than the version macros. Section 4 gained 4.3
(physical storage buffer), 4.4 (the ordering adjudication) and 4.6 (the second
consumer of the capability ordinals). Section 5.2 now covers all three absent
handshakes plus cache invalidation rather than only the dead capability check.
Section 9 is new and lists every withdrawal, downgrade, citation fix, added
finding and adjudication.

Escalations went from 11 to 15. Three of the original 11 were restated rather
than kept: the old escalation 1 (two routes for the generated header) now says
only one route is sound; the old escalation 2 (the `!=` comparison) is now a
per-capability contract question; the old escalation 4 (two conflicting
capability systems) is now a scope question about whether `Extension` is live at
all in this fork.

Two things I want on the record about my own first pass, since the adversaries
were right about the pattern and not just the instances.

All five withdrawn claims share one shape: I read a set of file:line facts
correctly, then wrote a general sentence one step beyond them, and marked the
general sentence [V]. "Only four backends" from a grep I had scoped to one
directory. "Exactly three things and nothing else" while my own section three
paragraphs later documented a fourth. "The only hardware query" while my own
table listed two. The citations were not the problem; the quantifiers were. In a
method that runs on the [V]/[I] split, a [V] on a generalisation is worse than no
mark at all. I have tried to keep this revision's absolutes down to ones I
actually enumerated, and to mark the composed consequences [I] even where every
component is [V].

Second: I scoped the offline cache to agent 03 and stated that scoping openly,
which was defensible for the file format and not defensible for the invalidation
seam. The seam is in 6.1's path and sits in my territory. Recording it as a
scoping error rather than a scoping decision.

Files touched this pass: `report-04-backend-build.md` and this notes file. No
source file, no other agent's file.

---

# Amendment pass — entries 37-43

Seven claims arrived from `adversary2-04-1.md` and `adversary2-04-2.md`, both of
which judge this report correct and the pair not complete. Instruction: verify
each against source, and where an adversary is wrong say so and change nothing.
One entry per claim. I did not treat agreement between the two adversary files as
evidence for anything; every figure below I re-derived myself in this pass.

## Entry 37 — CLAIM 1, the `-D "ARCH_${rtm_arch}"` on the clang line. UPHELD, exactly.

The claim: the bitcode command line this report quotes verbatim carries a
`-D "ARCH_${rtm_arch}"` neither report discusses; it is a configure-time value
compiled into the bitcode, consumed at roughly 21 sites in the runtime source,
producing one installed variant per value, selected at run time by filename; and
it narrows this report's escalation 1.

What I ran, and what came back.

`cat -n taichi/runtime/llvm/runtime_module/CMakeLists.txt`, all 43 lines. Line 8
is the whole clang invocation and carries, in order: `-fno-exceptions`,
`-emit-llvm`, `-std=c++17`, `-D "ARCH_${rtm_arch}"`, `-I ${PROJECT_SOURCE_DIR}`.
`rtm_arch` is the function parameter of `COMPILE_LLVM_RUNTIME` declared at `:3`.
The loop is `:29-31`, `foreach(arch IN LISTS HOST_ARCH CUDA_ARCH DX12_ARCH
AMDGPU_ARCH)` calling `compile_llvm_runtime(${arch})`. Output file
`runtime_${rtm_arch}.bc` at `:8`; install at `:13`.

Where the four list values come from, each read directly:
- `HOST_ARCH` — `cmake/TaichiCXXFlags.cmake:149`,
  `set(HOST_ARCH ${ARCH} CACHE INTERNAL "Host arch")`, with `ARCH` set to `"x64"`
  at `:139`, `"arm64"` at `:142`, `"x86"` at `:145`.
- `CUDA_ARCH` — `CMakeLists.txt:154-156`. `AMDGPU_ARCH` — `:158-160`.
  `DX12_ARCH` — `:162-164`. Each is a three-line `if (TI_WITH_X) set(...)
  endif()`.

Consumption count. `grep -n "ARCH_" taichi/runtime/llvm/runtime_module/runtime.cpp`
returns **19** lines: 51, 119, 155, 347, 771, 798, 801, 860, 1144, 1156, 1167,
1293, 1343, 1431, 1549, 1633, 1651, 1861, 1865. All are preprocessor
conditionals on `ARCH_cuda`, `ARCH_amdgpu` or `ARCH_x64`. Two more in headers of
the same directory: `locked_task.h:7` `#if ARCH_x64 || ARCH_arm64` and
`node_pointer.h:15` `#if defined(ARCH_cuda)`. I then checked those two headers
are actually in the bitcode translation unit rather than merely nearby:
`grep -rn "locked_task.h\|node_pointer.h"` over `taichi/` returns exactly two
includes, both in `runtime.cpp`, at `:1660` (`node_pointer.h`) and `:1872`
(`locked_task.h`), and nothing else in the tree includes either. So **21** sites,
all inside the one TU. The adversary's "roughly 21" is exact at 21.

Selection by filename: `taichi/runtime/llvm/llvm_context.cpp:209-211`,
`get_runtime_fn(Arch arch)` returning `fmt::format("runtime_{}.bc",
arch_name(arch))`. Already in this report at 5.3.

So the claim holds in every part. The mechanism is: a CMake variable → a `-D`
macro name on a standalone clang line → preprocessor conditionals inside the
bitcode → one `.bc` artefact per value → run-time selection by filename. That is
the complete shape items 6.1 and 6.3 need, and it is already running.

**Consequence for escalation 1, which is the point of the claim.** Revision 2
said "Only one route is sound: the source-tree header must stop being a header."
I reached that by examining the `-I` list and the generated-header route. I never
read the `-D` on the same line. The sentence is withdrawn. Escalation 1 is now a
choice among three routes: a generated `constants.h`, a `-D` on that line, or
both. I am not choosing; that is the project owner's.

Two constraints I checked before writing the route up, because offering a route
that cannot work would be worse than not offering it:

1. `taichi/inc/constants.h:12` is `constexpr int taichi_max_num_snodes = 1024;`.
   A `-D taichi_max_num_snodes=4096` macro-expands the declaration itself to
   `constexpr int 4096 = 1024;`, which does not compile. The route needs a
   distinct macro name that `constants.h` reads. Adversary 04-2 raises the same
   caveat and is right. Recorded as a constraint, not designed around — designing
   the macro is not my assignment.
2. The existing `-D` form defines a bare name (`ARCH_x64`), not a value. `-D
   NAME=VALUE` is the same flag, so nothing about the mechanism forbids a value;
   what is untested in this tree is a value rather than a name.

And one thing I noticed that neither adversary states: `get_runtime_fn` formats
the arch and **nothing else**, so if a second parameter varies per variant, the
filename either grows or the ceiling is fixed once per build. That is a real fork
and I have put it in escalation 16 rather than resolving it.

## Entry 38 — CLAIM 2, nine ordered comparisons or thirteen. UPHELD.

`grep -rn "spirv_version" --include=*.cpp --include=*.h --include=*.mm taichi/
c_api/ | grep -v external | grep -E "[<>]"` returns 15 lines, two of which are
not comparisons (`spirv_ir_builder.cpp:15` pushing the value into the header,
`:17` a `TI_TRACE`). The remaining **13** are comparisons:

- `<` — `spirv_ir_builder.cpp:556, 625, 671, 679, 741, 777`;
  `vulkan_device_creator.cpp:577`. Seven.
- `>` — `spirv_codegen.cpp:1908`. One.
- `>=` — `spirv_ir_builder.h:424`; `spirv_codegen.cpp:2669, 2671, 2673, 2675`.
  Five.

My nine were all correct. The four I missed are the ones both adversaries name,
and I read the block rather than counting it. `grep -n "" | sed -n '2664,2684p'`
on `spirv_codegen.cpp`:

```
2666:  uint32_t spirv_version = params.caps.get(DeviceCapability::spirv_version);
2668:  spv_target_env target_env;
2669:  if (spirv_version >= 0x10600) {        -> SPV_ENV_VULKAN_1_3
2671:  } else if (spirv_version >= 0x10500) { -> SPV_ENV_VULKAN_1_2
2673:  } else if (spirv_version >= 0x10400) { -> SPV_ENV_VULKAN_1_1_SPIRV_1_4
2675:  } else if (spirv_version >= 0x10300) { -> SPV_ENV_VULKAN_1_1
2677:  } else {                               -> SPV_ENV_VULKAN_1_0
2681:  spirv_opt_ = std::make_unique<spvtools::Optimizer>(target_env);
```

This is in `KernelCodegen::KernelCodegen` and it selects the target environment
for the **whole module**, not a construct-level branch. It is the strongest
instance of ordered use in the tree and it is the one I did not have. Corrected
in 4.4 and in section 10.

Worth being plain about why this matters even though it favours my own
conclusion: it was a **[V]**-marked census backed by a stated grep, inside the
section I wrote specifically to adjudicate a dispute between the two adversaries.
That is the same failure shape round one sent me back for. The adjudication
itself is unchanged — "not guaranteed" is still weaker than "not ordered", and
the engine still assumes ordering — but the evidence for it is now correct and
stronger.

## Entry 39 — CLAIM 3, the host does not lay out the runtime struct. UPHELD.

The claim: revision 2 describes a host-versus-device layout disagreement that
cannot happen, because the host never lays out the runtime struct.

`grep -rn "struct LLVMRuntime\|class LLVMRuntime"` over `taichi/` and `c_api/`
returns five lines and only one is a definition:
- `taichi/program/context.h:11` — `struct LLVMRuntime;`
- `taichi/rhi/llvm/llvm_device.h:8` — `struct LLVMRuntime;`
- `taichi/runtime/llvm/runtime_module/runtime.cpp:136` and `:337` — forward
  declarations inside the bitcode TU itself
- `taichi/runtime/llvm/runtime_module/runtime.cpp:552` — `struct LLVMRuntime {`,
  the **only** definition in the tree, and it is in the bitcode TU.

Host use is by opaque pointer: `context.h:18` `LLVMRuntime *runtime{nullptr};`
inside `RuntimeContext`, and `llvm_device.h:14` `LLVMRuntime *runtime{nullptr};`
inside `LlvmRuntimeAllocParams`. Neither dereferences a member.

Where the host gets a type for it: `TaichiLLVMContext::get_runtime_type` at
`llvm_context.cpp:1004-1011`, which is
`llvm::StructType::getTypeByName(get_this_thread_runtime_module()->getContext(),
"struct." + name)` — the type is read **out of the loaded bitcode module**, and
errors if it is not there. Called for this struct at `codegen_llvm.cpp:2694-2698`
(`TaskCodeGenLLVM::get_runtime`, bitcasting the `RuntimeContext_get_runtime`
result to a pointer to it).

Field access is through accessors compiled inside the bitcode:
`STRUCT_FIELD_ARRAY(LLVMRuntime, element_lists)` at `runtime.cpp:616` and
`RUNTIME_STRUCT_FIELD_ARRAY(LLVMRuntime, element_lists)` at `:750`, reached from
the host by name — `runtime_query<void *>("LLVMRuntime_get_element_lists", ...)`
at `llvm_runtime_executor.cpp:327-329`.

So there is no second layout to disagree with the first. Revision 2's sentence
"host C++ and device bitcode would disagree on the size of `element_lists`,
`node_allocators` and `ambient_elements`" describes an ABI mismatch that the
source does not permit. Replaced.

What actually goes wrong, which I also verified rather than taking from the
adversary. The host's only use of the constant is
`struct_llvm.cpp:266`, `TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);`
— and the plan's own section 6.1 correction already records that this assertion
does not guard those arrays, because it bounds a per-tree count while the arrays
are indexed by the process-global `SNode::id`. Adversary 04-2 calls it "guarding
the write at `struct_llvm.cpp:266`"; that is wrong, on the plan's own record and
on the line itself, and adversary 04-1 corrects it. I have not adopted 04-2's
phrasing. The write that overflows is `runtime.cpp:1005`,
`runtime->element_lists[i] = runtime->create<ListManager>(...)`, inside the
loop at `:1003` over `root_id` to `root_id + num_snodes` — inside the bitcode,
where the array lives.

The section's conclusion is unchanged: the value must reach the clang invocation.
It is in fact strengthened, because the arrays exist **only** in the bitcode.

## Entry 40 — CLAIM 4, "permanently" is false. UPHELD.

Read `c_api/src/taichi_core_impl.cpp:317-334` in full.
`ti_set_runtime_capabilities_ext(TiRuntime, uint32_t capability_count, const
TiCapabilityLevelInfo *capabilities)` builds a local `DeviceCapabilityConfig`,
loops the caller's array at `:326-330` doing
`devcaps.set((DeviceCapability)cap_level_info.capability, cap_level_info.level)`,
and at `:331` calls `runtime2->get().set_caps(std::move(devcaps))`. No
validation against the hardware anywhere in the function.

I then checked that this reaches the same storage the twelve consumers read,
rather than assuming it. `Runtime::get()` is `virtual taichi::lang::Device &`
(`c_api/src/taichi_core_impl.h:131`). `Device::set_caps` is
`public_device.h:855`, writing the single member `caps_` declared at `:617`;
`get_caps` at `:852` returns that same member. `GfxProgramImpl::get_device_caps`
(`gfx_program.cpp:82-85`) returns `runtime_->get_ti_device()->get_caps()`, and
`gfx_program.h:81` reads it directly. So the override lands on the field every
consumer reads.

And the header-only wrapper ships a typed setter for this exact capability:
`grep -n "spirv_has_physical_storage_buffer" c_api/include/taichi/cpp/taichi.hpp`
→ `:1156`, `Self &spirv_has_physical_storage_buffer(bool value = true)`.

So "all permanently taking the false branch" is false. What is true, and what I
have written instead: no in-tree producer sets it, both producers are compiled
out, and the only way it is set today is a host calling the C API override.

The composition — that a host calling it before kernel compilation puts all
twelve on the true branch — I have marked **[I]**, not **[V]**. I did not build
or run anything. What I verified is the write path and the read sites.

Uncomfortable but worth recording plainly: this report already lists that
function twice, as `set_caps` site 8 in the 4.2 table and as escalation 11, and
revision 2 still wrote "permanently" three sections later. The two adversary
files disagree here — 04-1 found it, 04-2 re-verified the whole census in §3.2
and did not challenge the word. 04-1 is right.

It changes what escalation 13 asks. There is a third answer beside "revive the
producers" and "leave it": exercise the path today from outside the core with no
source change. Escalation 13 now says so and points back at 11.

## Entry 41 — CLAIM 5, the GGUI line number. UPHELD.

`grep -n "TI_WITH_GGUI" cmake/TaichiCore.cmake`:

```
11:option(TI_WITH_GGUI "Build with GGUI" OFF)                      # wheel-tag: ggui
63:    set(TI_WITH_GGUI ON)
383:    if(TI_WITH_GGUI)
384:        target_compile_definitions(${CORE_WITH_PYBIND_LIBRARY_NAME} PRIVATE -DTI_WITH_GGUI)
```

`:383` is the guard; the `target_compile_definitions` call is `:384`. Revision 2
carried `:383` in two places, 2.1 and the section 6 build-gating map. Both
corrected.

Provenance, since it was put to me as part of the claim and it is worth the file
recording: adversary 04-1 states in its round-two §4.2 that it gave me `:383` as
a correction in round one, that pass B had `:384` and correctly refused the
correction, and that the error is its own. I did not take that on the adversary's
word — the grep above is what settles it. But it is a straightforward case of a
correction being adopted without re-checking, which is worth one line in my own
notes rather than only in the adversary's.

## Entry 42 — CLAIM 6, "no dlopen of Taichi's own code" is uncited. UPHELD.

`grep -n "dlopen\|DynamicLoader\|dynamic_loader"` over both my files: exactly two
hits, `report-04-backend-build.md:25` (now `:37` as amended) and
`notes-04-backend-build.md:139`. Both
assert it; neither cites anything. The report marks it **[V]**. Standing
instruction 5 asks for file paths and line numbers.

The claim is true, and here is the census that should have been there.

`grep -rn "dlopen\|LoadLibrary\|DynamicLoader"` over `taichi/` and `c_api/`
excluding `external/`, no file-type filter:

- The one implementation: `taichi/common/dynamic_loader.cpp`, `dlopen` at `:29`
  inside `load_dll`, `LoadLibraryA` at `:27` for Windows, and an `RTLD_NOLOAD`
  probe at `:16` in `check_lib_loaded`.
- Four construction sites, every one a **vendor** library:
  - `taichi/rhi/cuda/cuda_driver.cpp:34` (from `load_lib`, name chosen at
    `:28-32`), plus `:89`, `:98`, `:107`.
  - `taichi/rhi/amdgpu/amdgpu_driver.cpp:33`, Linux-only by `static_assert` at
    `:31`.
  - `taichi/rhi/vulkan/vulkan_loader.cpp:98`, `runtime_lib_dir() +
    "/libMoltenVK.dylib"`, under `#if defined(__APPLE__)`. Third-party, and it
    ships beside the runtime, so it is the closest thing to a counter-example and
    it is still not Taichi's own code.
- `taichi/system/dynamic_loader.h` is a **dead second copy**: no
  `taichi/system/dynamic_loader.cpp` exists (`ls` confirms only the `.h`), no
  file in the tree includes it (all five includers name
  `taichi/common/dynamic_loader.h`), and no CMake file references it. Adversary
  04-2 flagged pass B for citing this dead copy in round one; I want it recorded
  on my side too, since my uncited claim was about the same subject.

One precision the census forces, and I have not written it into the report
because it would overreach the sentence it supports: Taichi's own code *is*
loaded at run time — the `runtime_<arch>.bc` modules, `llvm_context.cpp:209-218`
— but through LLVM bitcode parsing, not through `dlopen`. The summary sentence is
about backend *selection* having no plugin mechanism, and that stands.

## Entry 43 — CLAIM 7, the CUDA path never populates device capabilities. WRONG AS PUT. Nothing changed for it.

The claim as it reached me: "neither report states that the CUDA path never
populates the device capability system at all, while the SPIR-V backends do".

This report states it, twice, and has since revision 2.

- Summary point 3, report line 50-52 as amended: "**Capability detection is populated for
  four backends and empty for the rest, including CUDA** — by two independent
  routes, the RHI device and `ProgramImpl::get_device_caps`. **[V]**"
- Section 4.2, report line 548-555 as amended: "**CUDA, AMDGPU, CPU and DX12 RHI devices
  never call `set_caps`.** Their `caps_` stays empty and every `get()` returns
  0." followed by the paragraph beginning "That is internally consistent ... but
  it means there is no device-capability record whatsoever on the CUDA path,
  which is the path the section 5.2 target tiers are about."
- Section 4.2 again, the "second, independent route to the same emptiness"
  paragraph on `ProgramImpl::get_device_caps`.

The section title itself is "`DeviceCapability` — per-device, runtime-populated,
**and empty on CUDA**".

I re-verified the underlying fact anyway rather than only defending the text:
`grep -rn "set_caps"` over `taichi/` and `c_api/` with no file-type filter
returns nine lines, one declaration (`public_device.h:855`) and eight call sites,
none of them under `taichi/rhi/cuda/`, `taichi/rhi/cpu/`, `taichi/rhi/llvm/`,
`taichi/rhi/amdgpu/` or `taichi/rhi/dx12/`. The fact holds; so does the report's
existing statement of it. **I have changed nothing on this claim.**

What adversary 04-2 actually wrote is narrower and is right. Its §3.2 says
"Neither report says in one place that 6.2 splits into two differently-shaped
halves along `arch_uses_spirv`", and 04-1 reaches the same point from its §3.2
qualification 1. That is a gap and it was not covered: the two component facts
are in the report, in different sections, and the consequence for sizing item 6.2
is not drawn anywhere. So I have added section 4.7 and escalation 17 for the
consequence, and left the component statements alone.

On how 4.7 is framed, because plan section 5.2 governs it and the framing is not
free. The temptation is to write "the capability system serves the SPIR-V path
and not the CUDA path". That would be wrong twice over. First, `arch_uses_llvm`
(`arch.cpp:54-57`) is `{x64, arm64, cuda, dx12, amdgpu}` — it carries CPU-only
operation, which plan section 2 makes a first-class target, and two non-NVIDIA
GPU archs. The spine with no capability system is not "the vendor spine". Second,
per plan section 5.2 the reference path is the portable one, so where a path is
better served the finding is a gap in the less-served path, never a reason to
prefer the better-served one. 4.7 therefore records the asymmetry as a hole and
says explicitly that it is not a reason to weight 6.2 toward either spine. What
it changes is sizing, and only sizing.

## Entry 44 — what this pass did not do

Held to the amendment scope. No source file touched. No other agent's file
touched. Only `report-04-backend-build.md` and this notes file.

I opened no new line of investigation. Everything read in this pass was read to
verify one of the seven claims or to close the gap a claim named. Where a claim
named a consequence I could not verify by reading — the true-branch composition
in entry 40, the silent-failure mode in entry 39, the split in entry 43 — it is
marked **[I]** in the report, not **[V]**.

Nothing here is a decision. Escalation 1 gained two routes and lost its
"only one route is sound"; escalations 16 and 17 are new; escalation 13 gained a
third answer. All of them are questions, all unresolved, all for the project
owner.

Six of the seven claims held. One did not, and I have said so in entry 43 with
the report line numbers that already carried the fact, rather than adding text to
satisfy it.

---

# Framing pass — entries 45-51

Plan section 5.2 was amended after I was dispatched: the three cards are
**fidelity** classes, not capability classes, and explicitly not a vendor target.
Section 2.2 is new — hardware changes speed and granularity, never what the
system can do — and section 2.3 is new, stating that item 6.1 is the project and
that the shelved 64-bit work is stub material rather than a path to the ceiling.
I re-read all three before touching anything.

Six claims plus one new finding arrived. Same rule as the last pass: verify
before acting, and where an adversary is wrong say so.

## Entry 45 — reading the amended plan before doing anything

Three changes matter to my territory and I want them recorded in my own words
before I act on them, because two of them cut against sentences I wrote.

1. **5.2, fidelity not capability.** The cards define how fast and how finely,
   not what is possible. So a sentence of mine like "the path the section 5.2
   target tiers are about" was doing two wrong things at once: treating one arch
   as *the* tier path, and treating tiers as a property of hardware capability.
2. **2.2, the test.** "A change that lets some hardware do something other
   hardware cannot do at all is a violation, not an optimisation." This is a test
   I have to apply to things I was about to hold up as models. It changes what I
   can say about Metal (entry 49).
3. **2.3, priority.** "Item 6.1 is the project ... including the shelved 64-bit
   addressing work, which is stub material under section 5.1 rather than a path
   to the ceiling." This bounds how far I can take the re-weighting in entry 47.
   Re-weighting *within* 6.2 is not the same as raising 6.2.

## Entry 46 — CLAIM 1, six vendor-leaning statements. UPHELD, six found in my report.

I grepped my own report for `target tiers|section 5.2|NVIDIA|Brief 3.1|tiers
are|Maxwell|separate axis|the path the` and read each hit in context. Six
statements are factually sound and take their conclusion from the card list
rather than from source. All six restated in place; the table is report 9.7.

The two that were worst, and why:

- **4.2.** "...no device-capability record whatsoever on the CUDA path, which is
  the path the section 5.2 target tiers are about." The census above that
  sentence names four archs — CUDA, AMDGPU, CPU and DX12 — and then the sentence
  narrows to one, because of the cards. Checked: `arch_uses_llvm` at
  `arch.cpp:54-57` is `{x64, arm64, cuda, dx12, amdgpu}`. `x64` and `arm64` are
  CPU-only operation, which plan section 2 makes a first-class target, not a
  fallback. So my own evidence said "five archs including the CPU" and I wrote
  "the CUDA path".
- **4.5, the title.** "The section 5.2 target tiers" as a heading over a section
  whose entire content is `taichi/rhi/cuda/cuda_context.cpp`. A reader scanning
  headings would conclude the tiers live on CUDA. Retitled to what the section
  covers.

The other four — the 3.3 closing line, the 4.5 card mapping, escalation 7 and
escalation 12 — are the same shape and are restated the same way. No fact in any
of the six changed. I did not soften or drop a single citation; what I changed is
the sentence that told the planner what the citation meant.

One I decided **not** to change, and I want the reasoning on record. 5.3 calls
the AMDGPU ISA-version selection "the closest existing precedent for what 6.3
asks". AMDGPU is a vendor path, so the sentence could look like the same error.
It is not: the claim there is about the *shape* of the mechanism (select a module
by detected hardware, not by arch name), and the shape is vendor-neutral. I have
left it and named it as a model in escalation 7 alongside Metal.

## Entry 47 — CLAIM 2, the 64-bit pointer machinery was weighted backwards. UPHELD, with two limits.

The claim: I treated the switched-off machinery as a side axis because it sits on
the portable backends; those backends are the reference path, so it is the
primary form of 6.2 and the vendor path is the accelerant.

That is right and the error is mine twice over — report 4.3's closing and section
7's "separate axis" note. Both were written when I was reading section 5.2 as
naming a target. Plan section 5.2 as amended says the reference path is the
portable one and a vendor path is "an optional accelerant, never the baseline and
never the thing correctness is measured against". Written-and-disabled machinery
on the reference path is therefore the leading existing form of 6.2 here, and
escalation 13's "is it in scope" was the wrong question.

**Limit 1, which the brief itself supplied and which I verified from my own cited
lines rather than taking on trust.** The claim came with: a separate
investigation established this machinery gates ndarray, argpack and return-struct
addressing and does NOT reach SNode addressing. I checked the five consumer sites
this report already cites, by reading the enclosing function of each:

- `spirv_codegen.cpp:783` — inside `visit(ExternalPtrStmt *stmt)`, which opens at
  `:734`. External tensor / ndarray pointer.
- `spirv_codegen.cpp:2340` — inside `compile_args_struct()`, opens `:2326`.
- `spirv_codegen.cpp:2416` — inside `compile_argpack_struct()`, opens `:2401`.
- `spirv_codegen.cpp:2491` — inside `compile_ret_struct()`, opens `:2483`.
- `taichi/runtime/gfx/runtime.cpp:96` — substituting a device address for
  allocations typed `kNone` or `kNdarray` (`:91-94`).

None is on the SNode or root-buffer path. That corroborates the separate finding
from inside my own territory. I have **not** audited the SNode path myself — that
is agents 02 and 03 — so the report records the corroboration and marks the wider
claim as coming from elsewhere. The instruction said do not overstate in either
direction, and the honest position is: my five sites agree, and five sites are
not an audit.

**Limit 2, which the brief did not supply and which plan 2.3 forces.** Plan
section 2.3 now says item 6.1 is the project and the shelved 64-bit work is stub
material rather than a path to the ceiling. So "primary form of item 6.2" must
not be read as "primary work". Both hold: this is the leading edge of 6.2, and
6.2 is not the leading edge of the project. I have written both into 4.3 rather
than letting the re-weighting drift upward, because the drift is exactly the
overstatement the brief warned against.

## Entry 48 — CLAIM 3, the NEW FINDING. Verified in full. It holds, and one part is stronger than claimed.

The claim: the portable backends already query tier data and discard it, so the
real gap is not that the portable path lacks tier information — both detect it
and neither publishes it.

**Vulkan memory properties.** `vulkan_device.cpp:2515-2516` calls
`vkGetPhysicalDeviceMemoryProperties(physical_device_, &properties)` into a
local. I read the whole consumption: the vector at `:2518-2519` and the loop at `:2521-2532` read
`properties.memoryTypes[i].propertyFlags`, tests
`VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`, and writes an external-handle-type flag
per memory type (`OPAQUE_WIN32_BIT` under `_WIN64`, else `OPAQUE_FD_BIT`), then
`:2534` hands the array to the VMA allocator info. Then I checked the negative:
`grep -rn "memoryHeaps\|heapSize" taichi/rhi/vulkan/` returns **nothing**. The
device memory budget is asked for and never looked at. Claim holds.

That one matters more than it looks. The Baseline tier in plan 5.2 is "2 GB
class" and its requirement is "calculation optimised for 2 GB". CUDA answers that
question — `cuda_context.cpp:88-92` and `:94-98`, total and free memory — and
Vulkan asks the driver and throws the answer away. Per plan 5.2 that is a gap in
the portable path, recorded as such, and not evidence that the portable path
matters less.

**Vulkan stored properties.** `VkPhysicalDeviceProperties vk_device_properties_`
is a member at `vulkan_device.h:739`, filled at `vulkan_device.cpp:1596` (last
statement of the `VulkanDevice` init), exposed by `get_vk_physical_device_props()`
at `vulkan_device.h:710-712`. Grep for both names across `taichi/` and `c_api/`:
five hits, of which two are the declaration and the definition, one is the fill,
and **two are reads** — `:1125-1128`, `limits.maxComputeWorkGroupCount[0..2]`
bounding `VulkanCommandList::dispatch` and returning
`RhiResult::not_supported` past it, and `:1945`, `limits.timestampPeriod`
converting a profiler duration to milliseconds. Claim holds exactly: two fields.

**Vulkan device name.** `vulkan_device_creator.cpp:432-440` loops every physical
device and logs `properties.deviceName` via `RHI_DEBUG_SNPRINTF` +
`RHI_LOG_DEBUG`; `:504-518` re-queries and logs name plus API version. Claim
holds — but I found one qualification the claim does not make and the report has
to: that second local query is **not** purely a log. `:523` takes
`physical_device_properties.apiVersion` and `:525-533` ladders it into
`caps.set(DeviceCapability::spirv_version, ...)`. So Vulkan does publish one
field into the capability map. It is an API version, not a hardware class, so the
finding survives; but "reads the device name only into a debug log" is true while
"the properties query is only a log" would not be, and I wrote the narrower one.

**OpenGL workgroup limits.** `opengl_api.cpp:20-21` declares
`int opengl_max_block_dim = 1024;` and `int opengl_max_grid_dim = 1024;` with the
comment at `:18-19` explaining the hardcoded values are the spec minimum "in case
glGetIntegerv didn't work properly". `initialize_opengl` fills them at `:231` and
`:234`. Then the negative: grep for both names across `taichi/` and `c_api/`
returns **six** lines and they are the two declarations, the two
`glGetIntegeri_v` calls and the two `TI_TRACE` lines at `:233` and `:236`.
Nothing else in the tree reads either one, and no header declares them `extern`.

So on OpenGL this is **stronger than the claim**. The claim says the values are
detected and not published. They are detected and not *used* — write-only
globals, and the hardcoded 1024 fallback the comment justifies is therefore never
consulted either. I have written the stronger version into 4.8 and said why.

**The composed statement**, which is the point of the finding: every backend that
can ask its hardware about class asks, and none tells anything downstream. That
composition I have marked **[I]**; each census is **[V]**. New section 4.8, and
escalation 18.

This does displace a reading I had been carrying since revision 1 without ever
testing it — that tier data was a CUDA asset. It was not. I never checked the
portable side, and the reason I never checked is the framing the plan has now
corrected.

## Entry 49 — CLAIM 4, Metal as the model. UPHELD, with a plan 2.2 caveat that must travel with it.

Read `collect_metal_device_caps` in full, `metal_device.mm:1017` onward.

- `:1019-1025` define the family constants including
  `kMTLGPUFamilyApple3` through `Apple8`.
- `:1027-1036` query `[mtl_device supportsFamily:]` for `MTLGPUFamilyMac2` and
  Apple7 down to Apple3, each lower family OR-ed with the one above it, so the
  ladder is monotone by construction rather than by luck.
- `:1038-1042` reduce it to five named features:
  `feature_64_bit_integer_math = family_apple3`,
  `feature_floating_point_atomics = family_apple7 | family_mac2`, and three
  permute/reduction ones.
- `:1044-1068` write those into a `DeviceCapabilityConfig`, returned at `:1069`.
- `MetalDevice::MetalDevice` at `:1162-1169` calls
  `collect_metal_device_caps` and `set_caps` at `:1168`.
- And it is read back portably: `:131-135` does
  `caps.contains(DeviceCapability::spirv_has_int64)` to select MSL 2.3.

So the four-stage chain detect → name → publish → consume exists end to end, on a
portable backend, and it is the thing escalation 7 was asking someone to invent.
Claim holds.

**But I will not hand this to the planner as a clean model.** Plan section 2.2 is
new and it is explicitly a test to apply to every proposed change: hardware must
change speed and granularity, never what the system can do; a change letting some
hardware do what other hardware cannot is "a violation, not an optimisation".
What Metal's ladder publishes is feature admission — below Apple3 there is no
`spirv_has_int64` at all. That is the pattern 2.2 warns about, sitting inside the
mechanism I am recommending as a model.

The two are separable and I have written them separately: the **mechanism** is
the model, the **content** is not. And the deeper question it exposes is that
every entry in `rhi_constants.inc.h:8-35` is a feature bit or a version number,
so there is currently no way to say "faster and finer" in that vocabulary at all
— only "can" and "cannot". That is escalation 19, and I am not resolving it.
Worth noting that `rhi_constants.inc.h:1-7` invites exactly this and names CUDA
compute capability as something that "can be listed here".

## Entry 50 — CLAIM 5, GLES unreachable in the C API. UPHELD; already partly recorded.

My report already carried the core of this at 4.2 as a scope caveat, so the
claim is not new to the file. What I verified and added is the full picture.

- `Arch::gles` in chain 1: `taichi/program/program.cpp:124`, with
  `opengl::initialize_opengl(true)` at `:126`.
- In chain 2: `taichi/aot/module_loader.cpp:40`.
- Classified portable: `taichi/rhi/arch.cpp:64`, inside `arch_uses_spirv`.
- Chain 3, `ti_create_runtime`: **no case**. Confirmed by re-reading
  `c_api/src/taichi_core_impl.cpp:247-309`.
- `ti_get_available_archs`: **no probe**. Read `:171-205` in full; the six probes
  are vulkan, metal, cuda, x64, arm64, opengl, at `:178-195`.
- `TI_ARCH_GLES = 7` exists at `c_api/include/taichi/taichi_core.h:373`.

So the arch is nameable and dispatchable in-tree and unreachable through the C
API. Recorded in 4.2 and folded into escalation 20, because it is the same
decision as claim 6 seen from the other side.

## Entry 51 — CLAIM 6, the unconditional int64 on OpenGL, weighed against sections 2 and 5.3. UPHELD.

The two code paths, both re-read:

- `GLDevice::GLDevice`, `opengl_device.cpp:506-529`. `if (!is_gles())` at `:509`
  guards `spirv_has_int64` and `spirv_has_float64` at `:511-512`, with the
  comment at `:510` "64bit isn't supported in ES profile". Then extension-gated
  int16/float16 at `:515-526` on `GLAD_GL_NV_gpu_shader5`,
  `GLAD_GL_AMD_gpu_shader_int16` and `GLAD_GL_AMD_gpu_shader_half_float`.
- `OpenglRuntime::OpenglRuntime`, `c_api/src/taichi_opengl_impl.cpp:4-13`. Builds
  a fresh `DeviceCapabilityConfig` at `:8` and sets `spirv_has_int64` at `:9` and
  `spirv_has_float64` at `:10` **with no guard**, `spirv_version` at `:11`, then
  `get_gl().set_caps(...)` at `:12`, replacing what `GLDevice` detected.

The weighing the brief asked for. Plan section 2 puts hardware agnosticism and
edge hardware first and says the e-waste hardware "is the proof rather than a
compromise". Section 5.3 says modern capability is "an opportunity to take, never
a prerequisite to run". The `!is_gles()` guard serves exactly that interest: it
keeps a low-end profile from being told it has something it does not. The C API
copy discards it.

Two honest qualifications, both in the report:

1. **It cannot fire today.** `GLDevice` calls `initialize_opengl(false, true)` at
   `:507`, so `is_gles()` is false on this path, and per entry 50 GLES has no C
   API entry point anyway. The over-declaration is latent, not live. What makes
   it worth recording rather than dismissing is that GLES is precisely the
   profile the section 2 edge devices would present, so this is a defect aimed at
   the constituency the plan exists to serve.
2. **The live half cuts the other way.** The dropped int16/float16 detection is
   lost now, on desktop GL, and that is capability the hardware really has and
   the C API fails to declare. Under section 5.3 that is the mirror error:
   refusing an opportunity that is present.

Both directions are one defect — the C API builds a capability set by assertion
where the RHI builds one by detection — which is why I put it in one escalation,
20, alongside the wholesale override at `taichi_core_impl.cpp:331` rather than as
two.

## Entry 52 — what this pass did and did not do

Scope held. No source file touched. No other agent's file touched. Only
`report-04-backend-build.md` and this notes file.

**Nothing was withdrawn as false in this pass, and no citation was corrected.**
Every count, census and file:line in the report is as it was after the amendment
pass. What changed is six sentences that told the planner what the facts meant,
plus new material for the one new finding.

Added: report 4.8 (both spines detect, neither publishes), 4.9 (Metal's chain,
with the section 2.2 caveat), the GLES and OpenGL-assertion weighing in 4.2, a
hardware-detection block in the section 6 map, and escalations 18, 19 and 20.
Restated: 3.3, 4.2, 4.5 including its title, 4.3, 4.7, section 7, and escalations
7, 12 and 13.

Nothing here is a decision. Escalations 18, 19 and 20 are questions and the three
restated ones are still questions. Where the re-weighting could have run away
with itself — 6.2 becoming "the primary work" rather than "the primary form of
6.2" — plan section 2.3 bounds it and I have written the bound in rather than
leaving the reader to apply it.

## Entry 53 — reading the plan before touching anything, and what had changed under me

Standing instruction 6 says the plan changes under you and to re-read before
finalising. I read `PROJECT-PLAN.md` in full at its **2026-09-09** stamp before
opening any other file, because the brief for this pass says section 10 item 6
exists because of exactly this failure on my own territory.

Two sections are new to me and one is corrected. All three matter.

**Section 2.2a, commonised base, vendor branches welcome.** "A vendor backend is
CUDA, AMDGPU, DirectX, or anything else tied to one supplier or one operating
system. The two COMPILATION SPINES are not vendors: LLVM carries CPU-only
operation, and SPIR-V carries the portable GPU path. **Work on either spine is
BASE work.** SPIR-V specific is not vendor specific." And under BASE, in scope:
"**Item 6.2, 64-bit addressing, in full.** This includes the shelved SPIR-V path
and the `at_buffer` pointer-width collision ... The two halves are not
separable."

**Section 5.1a, the install-time configuration agent sets the SNode field size.**
"Per build already means per device configuration ... The filename scheme does
not need to grow, and no common denominator has to be found." Also: baking the
constant into per-architecture bitcode at build time "is the CORRECT shape. It
was never a defect to work around." Also: concurrent operation on both cards is
confirmed empirically and outranks any code-reading conclusion.

**Section 5.2's CORRECTION dated 2026-09-09.** "There are two compilation spines
and neither one is the vendor spine." "Vendor lock is a property of individual
backends, not of a spine." "Where one spine is better served than the other, that
is a gap in the less-served spine to record, never evidence that it matters
less."

I then grepped the plan for the phrases my revision 4 used:
`grep -n 'reference path\|accelerant' PROJECT-PLAN.md` returns **nothing**.
`grep -n '5\.1a' report-04-backend-build.md notes-04-backend-build.md` returned
nothing before this pass. So revision 4's central framing cites a document that
does not contain it, and three of my escalations were written blind to a section
that answers them.

Recorded here first, before any edit, so the order of operations is legible: I
read the plan, then established the framing was ungrounded, then went to source
for the six claims. Not the other way round.

## Entry 54 — CLAIM 1, the stale framing. UPHELD, and it is mine to fix, not to defend.

The claim: 32 statements across the two reports call the SPIR-V spine "the
reference path" and the LLVM spine "the vendor path" or "the accelerant"; my
report attributes it to plan section 5.2 at roughly line 875; the plan contains
neither phrase.

**Verified three ways.**

1. The plan grep above: zero hits for either phrase.
2. My own report, `grep -n 'reference path\|accelerant'`: seven hits, at lines
   716, 718, 720, 875, 1292, 1409, 1601. Six "reference path" and one
   "accelerant". Both round-three adversaries enumerate the same seven for pass A
   and twenty-five for pass B; seven plus twenty-five is thirty-two, and their
   total reconciles to their list. I derived my seven from my own grep and did
   not take the number on trust.
3. Where it came from: my own notes, entry 47, at lines 1946-1949 —
   "Plan section 5.2 as amended says the reference path is the portable one and a
   vendor path is 'an optional accelerant, never the baseline and never the thing
   correctness is measured against'." That is a quotation of an intermediate
   section 5.2. So revision 4 applied the instruction it was given, faithfully,
   and the instruction was stale by the time it was applied.

**The load-bearing part, which is why this is not a wording fix.** Line 875 reads
"**On the SPIR-V spine — the reference path under plan section 5.2**". That is a
citation of the governing document for a proposition the document does not
contain. Standing instruction 7 says a verified citation does not verify the
claim attached to it; this is that failure one level up, where the cited section
does not support the sentence citing it. And lines 716-718 and 1292 draw a
**ranking** from it: one half of item 6.2 called "primary". The plan supplies no
ranking, and section 2.2a puts both halves in BASE scope without ordering them.

**The self-contradiction.** Line 875 and lines 890-893 are both revision-4 text
in the same section 4.7. `:875` calls the SPIR-V spine the reference path under
plan 5.2; `:890-893` says "It is not a reason to weight 6.2 toward either spine.
The LLVM spine is not 'the vendor path': it carries `Arch::x64` and
`Arch::arm64`, which is CPU-only operation, a first-class target." I read both in
place. They cannot both stand. `:890-893` matches the current plan and is the one
I kept; `:875` is withdrawn.

**What I did NOT do.** I did not delete the withdrawn wording. Every other
retraction in this report is kept legible below its replacement and this one is
handled the same way — the correction-log row at `:1601` is marked withdrawn in
place rather than removed, and section 9.8 tabulates all seven with what each now
says. Two adversary files cite these lines; deleting them would break the record
they are checking against. This is the same principle as the plan's own
"recorded so it is not re-derived" paragraphs.

**What replaced the ranking.** Nothing that ranks. The source supports a
two-STATE statement and I wrote that: on `arch_uses_spirv` the machinery is
written and switched off (twelve consumers, two disabled producers); on
`arch_uses_llvm` there is no capability representation at all. Both are BASE work
under 2.2a. The difference is recorded as a gap on the less-served spine, per
5.2, and as nothing else.

## Entry 55 — CLAIM 2, GLES through the C API. UPHELD, and my escalation 20 rested on a false premise.

The claim: my escalation resting on GLES being unreachable through the C API is
FALSE, because `ti_import_opengl_runtime` is a public exported function taking a
`use_gles` parameter.

I did not take this on the claim. I opened every file.

- `c_api/include/taichi/taichi_opengl.h:33-36` — `TI_DLL_EXPORT TiRuntime
  TI_API_CALL ti_import_opengl_runtime(TiOpenglRuntimeInteropInfo *interop_info,
  bool use_gles);`. Exported, documented with a `// Function` comment, two
  parameters, the second a `bool use_gles`.
- `c_api/src/taichi_opengl_impl.cpp` — I read the whole file, 121 lines. `:21-29`
  is the body. `:26` `set_gles_override(use_gles)`. `:28`
  `return ti_create_runtime(TI_ARCH_OPENGL, 0);`.
- `taichi/rhi/opengl/opengl_api.cpp:35-37` — `set_gles_override` writes
  `use_gles_override`, an anonymous-namespace `std::optional<bool>` at `:32`.
- `c_api/src/taichi_core_impl.cpp:269-272` — the `TI_ARCH_OPENGL` case,
  `new OpenglRuntime`.
- `OpenglRuntime`'s member init list, `taichi_opengl_impl.cpp:4-7`, constructs
  `device_` **before** the constructor body runs. `GLDevice::GLDevice`
  (`taichi/rhi/opengl/opengl_device.cpp:506`) calls
  `initialize_opengl(false, true)` at `:507`.
- `opengl_api.cpp:56-59` — `if (use_gles_override.has_value()) { use_gles =
  use_gles_override.value(); unset_gles_override(); }`. **The argument `false` is
  replaced by the override.**
- `opengl_api.cpp:239` `kUseGles = use_gles;`, read by `is_gles()` at `:248-250`.
- `opengl_device.cpp:509-513` — `if (!is_gles())`, comment `:510` "64bit isn't
  supported in ES profile". So on GLES the RHI **correctly refuses**
  `spirv_has_int64` and `spirv_has_float64`. `set_caps` at `:529`.
- `taichi_opengl_impl.cpp:8-12` — the constructor **body** then builds a fresh
  three-entry config and installs it wholesale, asserting `spirv_has_int64` at
  `:9` and `spirv_has_float64` at `:10`.

So the ordering is: detect correctly, then overwrite with the opposite. The
mis-declaration is live, through a documented exported function, and my
"cannot fire today" is false. It survived four revisions of my report, both
round-two adversaries (`adversary2-04-1.md` §9.3.3 raised the unreachability as
its own finding; `adversary2-04-2.md:455-457` endorsed my caveat) and one
round-three adversary. Four documents carry it. `adversary3-04-2.md` §5 found it
and `adversary3-04-1.md` did not.

**The converse case, which I also verified because it is the same override.**
`opengl_api.cpp:46-54` is an early return when `supported` already has a value,
and it sits **before** `:56`. So if `initialize_opengl` ran earlier in the
process, the override is never read and never cleared.
`ti_get_available_archs` (`taichi_core_impl.cpp:171-205`) probes OpenGL at `:193`
via `is_opengl_available` (`:21-27`) → `is_opengl_api_available()` with default
`use_gles = false` (`opengl_api.h:13`) → `initialize_opengl(use_gles, true)`
(`opengl_api.cpp:244-246`). I opened all four. So a host that enumerates archs
first **silently loses** its later GLES request.

`reset_opengl` (`opengl_api.cpp:252-256`) clears both `supported` and `kUseGles`;
its only caller is `taichi/runtime/program_impls/opengl/opengl_program.cpp:29`.
Noted because it means `kUseGles` has two writers, not the one the in-file
comment at `:23` implies.

**Why I weighted it rather than filing it.** Plan sections 2 and 5.3 put the
low-end profile first. GLES is what the low end presents. The guard the C API
discards exists for exactly that and says so in its own comment. And it engages
plan section 2.2 in the harder direction: the RHI declines a capability the
hardware lacks and the C API asserts it, which is hardware being told it can do
something it cannot.

I marked `[V]` for every row of the chain and `[I]` only for the two composed
outcomes, which I read rather than ran. That distinction is standing instruction
7 and I applied it deliberately here because the temptation was to mark the
composition verified on the strength of eight verified rows.

## Entry 56 — CLAIM 3, `TI_VISIBLE_DEVICE`. UPHELD, and absent from everything.

`taichi/rhi/vulkan/vulkan_loader.cpp:111-113`, read in place:
```
    const char *id = std::getenv("TI_VISIBLE_DEVICE");
    if (id) {
      set_vulkan_visible_device(id);
    }
```
It is the last statement of the `std::call_once` body opened at `:87` inside
`VulkanLoader::init` (`:86`).

Consumption, `vulkan_device_creator.cpp:442-456`, read in place: `:442` reads
`VulkanLoader::instance().visible_device_id`; `:445` `std::stoi`; `:446` range
test against `device_count`; `:448-451` `RHI_LOG_ERROR` with the format string
"TI_VISIBLE_DEVICE=%d is not valid, found %d devices available"; `:452`
`else if (get_device_score(devices[id], test_surface))`; `:453-454` accept.

**Absence confirmed by my own greps, not by the claim.** `TI_VISIBLE_DEVICE`,
`visible_device_id` and `call_once` return nothing over
`report-04-backend-build.md` and `notes-04-backend-build.md` before this pass.
Both round-three adversaries state the same for the other report and for all four
prior adversary files, and both found it independently of each other.

**Why it belongs in this territory rather than being a curiosity.** Plan section
5 puts configuration at install time and 5.1a puts the agent **outside the
process**. An environment variable read at loader init is precisely an
out-of-process channel. It is the second one in this tree — `TI_LIB_DIR` in
`runtime_lib_dir()` (`taichi/util/lang_util.cpp:35-45`) is the first, and my
report already cites that one for module location. So my device-selection
finding was not wrong, it was half the picture: on Vulkan there are two routes
in and I had found one.

I resisted the reading that this is "the intended install-time interface". The
code does not say that. `[I]` and into the escalation.

## Entry 57 — CLAIM 4, the selector is process-global and the environment wins first. UPHELD.

Three separate facts, each read.

1. **Process-global.** `VulkanLoader::instance()` is a function-local static,
   `vulkan_loader.h:14-17`. `visible_device_id` is a plain `std::string` member
   at `:32`, public. `set_vulkan_visible_device` (`vulkan_loader.cpp:144-146`)
   writes that one member. Every runtime in the process shares it. Serial
   creation happens to work because the value is consumed during construction at
   `vulkan_device_creator.cpp:442`.
2. **The environment read is inside `std::call_once`,** `vulkan_loader.cpp:87`,
   so it fires once per process.
3. **The ordering.** `ti_create_runtime` writes the caller's index at
   `c_api/src/taichi_core_impl.cpp:254-255`, then constructs
   `VulkanRuntimeOwned` (`:256-262`), whose chain reaches
   `VulkanDeviceCreator::VulkanDeviceCreator`, which calls
   `VulkanLoader::instance().init()` at `vulkan_device_creator.cpp:236`. So the
   `call_once` body runs **after** the C API wrote the argument.

Therefore: first Vulkan runtime in a process with the variable set — environment
wins, argument silently lost. Second and later — the flag has fired, argument
stands. And which case applies depends on whether `init()` ran earlier, for
instance through `is_vulkan_api_available()` reached from
`ti_get_available_archs` at `taichi_core_impl.cpp:179`, which I checked exists.

`[V]` for each of the three facts. `[I]` for the composed outcome. I did not run
it.

**A boundary I held.** Plan section 5.1a records that the owner has run both
cards concurrently, says it is direct observation, says it outranks code reading,
and says no agent needs to investigate it. This finding is about one C API
argument being overridable. It is not a claim about concurrency and I wrote that
constraint into the report rather than leaving a reader to infer I had strayed.

## Entry 58 — CLAIM 5, plan section 5.1a, and the positive result.

Three of my escalations were written without sight of 5.1a. Escalation 16 asked
"what the variant filename carries" and "where the host-side bound comes from".
Section 5.1a answers both in terms: "per build already means per device
configuration"; "the filename scheme does not need to grow, and no common
denominator has to be found."

**The positive result, which I checked rather than asserted.** The claim put to
me was that one ceiling per build is what the existing build loop already
produces with no filename change. I verified the mechanism:

- `runtime_module/CMakeLists.txt:29-31` is
  `foreach(arch IN LISTS HOST_ARCH CUDA_ARCH DX12_ARCH AMDGPU_ARCH)` calling
  `compile_llvm_runtime(${arch})`. **The loop varies one thing.**
- `:8` interpolates `rtm_arch` in exactly two places: the `-o` filename and the
  `-D "ARCH_${rtm_arch}"` token. Nothing else on the line varies per iteration.
- So a second `-D` carrying a ceiling takes the same value on every iteration, by
  construction, and `get_runtime_fn` (`llvm_context.cpp:209-211`) needs no
  change.
- The one host-side coupling is `TI_ASSERT((int)snodes.size() <=
  taichi_max_num_snodes)` at `taichi/codegen/llvm/struct_llvm.cpp:266`, which I
  opened and read. It compiles into `taichi_core` in the same configure that runs
  the loop.

So host and every bitcode variant read the same configured value. `[V]` for the
loop, the clang line and the assertion; `[I]` for "therefore they cannot
disagree", which is reasoning about a build I did not run. I marked it that way
deliberately — this is the exact shape of claim standing instruction 7 warns
about, where three verified citations tempt you to mark the composition verified.

**What I did about escalation 16.** I marked it CLOSED BY THE PLAN, struck the
question through, kept the text, and stated that only the planner may enter the
closure. I did not delete it: two adversary files cite it as open and a reader
needs to see what was closed and on what ground. What remains open and is carried
forward is the sizing rule, which is plan section 8.1 item 2, and the route
choice, which is my escalation 1.

## Entry 59 — CLAIM 6, `get_device_score` missing from my 4.8 census. UPHELD.

`grep -c get_device_score report-04-backend-build.md` returned **0** before this
pass. Both round-three adversaries flagged it and both are right. It is the
strongest instance of the finding section 4.8 exists to make and it was absent
from every revision.

Read in place, `taichi/rhi/vulkan/vulkan_device_creator.cpp:203-229`:
`vkGetPhysicalDeviceFeatures` `:206`; `vkGetPhysicalDeviceProperties` `:208`;
queue-family completeness × 1000 `:212-217`; `features.wideLines` × 100 `:219`;
integrated GPU × 500 `:220-222`; discrete GPU × 1000 `:223-225`; API minor
version × 100 `:226`; return `:228`.

**The qualification I added rather than accepting the tidy version.** Report B
says "the score is a local that dies there", and `adversary3-04-1.md` §4.2 says
that is true of `:462` and false of `:452`. I read both call sites. At `:462`,
inside the automatic pick `:458-468`, the score ranks and the number dies at
`:465`. At `:452` it is `else if (get_device_score(devices[id], test_surface))`
— a **boolean admission gate** on a device the caller explicitly named. A named
device scoring zero is dropped and the automatic pick runs, and the only error
log at `:448-451` covers the out-of-range case, not this branch.

That partly cuts against my own section 4.8 thesis, so I wrote it in as a
qualification on the thesis rather than omitting it. The publication claim
survives — nothing downstream learns the score — but "detected and discarded" is
not true of `:452`, where the decision survives even though the number does not.
Standing instruction 9's corollary: a scope ruling removes work, not evidence,
and a true sentence that complicates the bookkeeping stays.

`:219` adding 100 for `features.wideLines` when scoring for a compute workload is
recorded because plan section 1.3 says not to judge a backend by its graphics
support. Recording, not proposing.

## Entry 60 — where an adversary is wrong, the disputes I did not settle, and what this pass did not do

**Where an adversary is wrong, stated with lines, and I changed nothing for it.**
`adversary3-04-1.md` §6 item 1 and §3.4 item 4 say my escalation 13 "opens by
asserting the SPIR-V machinery is the primary existing form of 6.2 on the
strength of the framing", listing it among four places the framing is
load-bearing. The opening parenthesis did carry the framing and I restated it.
But the escalation's substance — twelve consumers, two disabled producers, the
`shaderInt64` coupling at `vulkan_device_creator.cpp:821`, and the third answer
through `ti_set_runtime_capabilities_ext` — never rested on the ranking and none
of it changed. Calling the whole escalation framing-dependent overstates it. Fact
unchanged, framing fixed, and I say which.

**The dispute I did not settle, because the source does not.**
`adversary3-04-2.md` §12.5 and §13 say my "the gap covers four archs including
CPU" over-reaches, because on `x64` and `arm64` there is no second host device
for an index to name. I checked its evidence and it is right on the facts:
`taichi/rhi/cpu/` is `CMakeLists.txt`, `cpu_device.cpp` and `cpu_device.h`; a
grep for `device_index`, `device_get_count` or `num_devices` across
`taichi/rhi/cpu/` and `taichi/rhi/llvm/` returns nothing; `host_arch()`
(`taichi/rhi/arch.cpp:68-76`) returns one arch.
`adversary3-04-1.md` claim 7 reads the same facts the other way, that CPU
coverage is what lets the finding stand enum-wide without the owner's card list.
Both readings are consistent with the source. **Whether a refused capability with
nothing to address counts as a gap is a judgement, and plan section 5.2 asks that
gaps be recorded per spine, so the answer changes the size of the LLVM-spine gap
from three archs to four.** I put both positions in 3.3 and raised escalation 21
rather than picking. Standing instruction 2.

**The reconciliation I accepted.** `adversary3-04-1.md` §5.6 is right that
`driver_.device_get(&device_, 0)` pins an ordinal, not a card, and that
`CUDA_VISIBLE_DEVICES` remaps ordinals outside Taichi. I re-ran the negative grep
myself over `taichi/`, `c_api/` and `python/`: zero hits. That restates the
severity as one-process-one-card, and it matters because plan section 5.1a
records concurrent two-card operation as direct observation that outranks code
reading. Without the qualification my finding invites a reader to weigh the two
against each other. I added it.

**Scope held.** No source file touched. No other agent's file touched. Only
`report-04-backend-build.md` and this notes file.

**Nothing was decided that the plan leaves open.** Escalation 16 is marked closed
**by the plan**, not by me, with its text retained and the closure attributed.
Escalations 21 and 22 are new and both are questions. Nothing was removed on the
grounds that it looked surplus, and no withdrawn wording was deleted — every
retraction in this pass sits visible below its replacement, which is how this
report has handled every prior correction and how the plan handles its own.

**Plan version worked against: 2026-09-09.** Stated in the report header and in
section 9.8, per standing instruction 6.
