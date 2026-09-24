# Notes 04B — backend, AOT and build architecture

Contemporaneous working notes. Appended as I go. Entries numbered.

## 1. Territory survey (start)

Read `modernization/PROJECT-PLAN.md` in full first. Governing items for me:
- 6.1 parameterise `taichi_max_num_snodes = 1024` as an install-time factor →
  need the CMake→header threading mechanism.
- 6.2 64-bit addressing; SPIR-V Int64 is a declared capability → device
  capability detection.
- 6.3 loosen architecture for adaptive module loading → backend selection,
  registration, AOT module path, c_api.
- 5.2 target tiers (Maxwell/Pascal/Ampere) → what capability info exists to
  distinguish them.

Directory listing:
- `taichi/rhi/`: arch.{h,cpp}, device.{h,cpp}, device_capability.{h,cpp},
  public_device.h, impl_support.h, dummy.cpp, CMakeLists.txt, and per-backend
  dirs: amdgpu cpu cuda dx dx12 llvm metal opengl vulkan common interop.
- `taichi/aot/`: graph_data.{h,cpp}, module_builder.{h,cpp}, module_data.h,
  module_loader.{h,cpp}.
- `c_api/`: cmake docs include src taichi.json tests unity version_scripts.
- `cmake/`: TaichiCore.cmake, TaichiCAPI.cmake, TaichiCXXFlags.cmake,
  TaichiTargets.cmake, TaichiConfig.cmake.in, utils.cmake, Distributed.cmake,
  PythonNumpyPybind11.cmake, TaichiExamples.cmake, TaichiCAPITests.cmake,
  CMakeGraphVizOptions.cmake.

## 2. Arch enum and how a backend is *named*

`taichi/rhi/arch.h:7-12` defines `enum class Arch : int` by x-macro over
`taichi/inc/archs.inc.h`. That file lists 12 archs: x64, arm64, js, cuda,
metal, opengl, dx11, dx12, opencl, amdgpu, vulkan, gles.

Classification predicates are hardcoded switches in `taichi/rhi/arch.cpp`:
- `arch_is_cpu` 42-48 (x64/arm64/js)
- `arch_is_cuda` 50-52
- `arch_uses_llvm` 54-57 (x64, arm64, cuda, dx12, amdgpu)
- `arch_is_gpu` 59-61 — literally `!arch_is_cpu`, so `opencl` and `js`
  classification is by omission, not by declaration.
- `arch_uses_spirv` 63-66 (opengl, gles, vulkan, dx11, metal)
- `host_arch` 68-76 — compile-time `TI_ARCH_x64` / `TI_ARCH_ARM` only.
- `default_simd_width` 82-93 — x64=8, cuda=32, arm64=4, else NOT_IMPLEMENTED.

Surprise worth recording: the Arch enum is **not** conditioned on any
`TI_WITH_*`. Every arch always exists as an enum value regardless of what was
built. Availability is a separate, later question (see entry 5).

## 3. Device capabilities

`taichi/rhi/device_capability.h:11-15` — `enum class DeviceCapability : uint32_t`
x-macro over `taichi/inc/rhi_constants.inc.h:8-35`.
`DeviceCapabilityConfig` (h:20-33) is just a `std::map<DeviceCapability,
uint32_t>` with contains/get/set; `get` returns 0 for missing
(`device_capability.cpp:33-39`). So "absent" and "level 0" are the same thing.

The capability list is **entirely SPIR-V oriented**. All 25 entries are
`spirv_*` except `reserved`. Relevant to 6.2:
- `spirv_has_int64` at `taichi/inc/rhi_constants.inc.h:14`
- `spirv_has_atomic_int64` at line 17
There is **no** capability describing CUDA compute capability, memory size,
or any LLVM-side 64-bit property, despite the header comment at
`rhi_constants.inc.h:1-7` explicitly saying CUDA compute capability *could* be
listed here. That is a gap for section 5.2 target tiers.

## 4. Build-time parameterisation precedent

Only two `configure_file` calls exist in the whole build:
`CMakeLists.txt:203-204`, generating `taichi/common/version.h` from
`taichi/common/version.h.in` and `taichi/common/commit_hash.h` from
`.../commit_hash.h.in`. Both write **into `${CMAKE_SOURCE_DIR}`**, i.e.
in-source generated headers, not into the binary dir.
`taichi/common/version.h.in:1-5` shows the form: plain `@VAR@` substitution
into `#define`s.

`taichi/inc/constants.h` is a plain checked-in header with no `.in` template.
`taichi_max_num_snodes = 1024` is at `taichi/inc/constants.h:12`;
`taichi_max_num_indices = 12` at line 5; `kMaxNumSnodeTreesLlvm = 512` at
line 13. All are `constexpr int` at file scope, no namespace.

## 5. TI_WITH_* toggles — the actual mechanism

`cmake/TaichiCore.cmake:1-11` declares the toggles:
TI_WITH_LLVM(ON), TI_WITH_METAL(ON), TI_WITH_CUDA(ON), TI_WITH_CUDA_TOOLKIT(OFF),
TI_WITH_AMDGPU(OFF), TI_WITH_OPENGL(ON), TI_WITH_VULKAN(OFF), TI_WITH_DX11(OFF),
TI_WITH_DX12(OFF), TI_WITH_GGUI(OFF). Plus USE_STDCPP.

Toggles are then *coerced* by platform, lines 35-75:
- APPLE forces CUDA/OPENGL/AMDGPU OFF (36-47); non-APPLE forces METAL OFF (49-52).
- WIN32 forces AMDGPU OFF (55-60).
- VULKAN ON forces GGUI ON (62-64).
- missing `external/glad/src/gl.c` forces OPENGL OFF (66-69).
- `NOT TI_WITH_LLVM` forces CUDA, CUDA_TOOLKIT, DX12 OFF (71-75).

Propagation to C++ is by **appending `-DTI_WITH_X` to the global
`CMAKE_CXX_FLAGS`**, not by `target_compile_definitions`:
lines 94 (LLVM), 103 (CUDA), 109 (AMDGPU), 115 (DX12), 119 (METAL),
123 (OPENGL), 127 (DX11), 131 (VULKAN), 281 (CUDA_TOOLKIT).
GGUI is the exception: `target_compile_definitions` at line 384, python target
only.

Linking is per-toggle `add_subdirectory` + `target_link_libraries` on the
`taichi_core` OBJECT library (line 137):
- LLVM block 148-243: always cpu_codegen/cpu_runtime (192-196); CUDA 198-205;
  AMDGPU 207-214; DX12 216-227; then llvm_codegen/llvm_runtime/
  llvm_program_impl 229-235.
- gfx_program_impl if any of METAL/OPENGL/DX11/VULKAN 245-248.
- metal 250-253, opengl 255-258, dx11 260-263, vulkan 265-268.
- SPIR-V codegen subdir is added unconditionally (286-292) but only *linked*
  when one of METAL/OPENGL/DX11/VULKAN is on (294-297). Comment on line 286
  says "SPIR-V codegen is always there, regardless of Vulkan" — that is true of
  the subdirectory, not of the link.

BUG spotted (upstream, pre-existing, not mine to fix): line 111 writes
`list(APPEND TAIHI_CORE_SOURCE ...)` — misspelled, missing the C. So
`taichi/runtime/amdgpu/runtime.cpp` is **never** added to `TAICHI_CORE_SOURCE`.
Contrast line 105 which spells it correctly for CUDA. Recording as an
observation only; NOT proposing a change.

## 6. The LLVM runtime module is compiled OUTSIDE CMAKE_CXX_FLAGS — key for 6.1

`taichi/runtime/llvm/runtime_module/CMakeLists.txt:3-15`. The `.bc` runtime is
built by a bare `add_custom_target` invoking clang directly, line 8:

    ${CLANG_EXECUTABLE} ${CLANG_OSX_FLAGS} -c runtime.cpp -o runtime_${rtm_arch}.bc
      -fno-exceptions -emit-llvm -std=c++17 -D ARCH_${rtm_arch} -I ${PROJECT_SOURCE_DIR}

This command line does **not** include `${CMAKE_CXX_FLAGS}`. Consequence for
6.1: threading the SNode ceiling as a `-D` appended to `CMAKE_CXX_FLAGS`
(the existing TI_WITH_* pattern, TaichiCore.cmake:94-131) would reach
`taichi_core` but would **NOT** reach `runtime.cpp` — which is precisely the
file whose lines 567-569 size the three arrays. That path would silently
produce a host/device mismatch.

Verified: `taichi/runtime/llvm/runtime_module/runtime.cpp:25` includes
`"taichi/inc/constants.h"`, resolved by the `-I ${PROJECT_SOURCE_DIR}` on
line 8. Verified the three arrays at runtime.cpp:567-569:
  `ListManager *element_lists[taichi_max_num_snodes];`
  `NodeManager *node_allocators[taichi_max_num_snodes];`
  `Ptr ambient_elements[taichi_max_num_snodes];`
and adjacent at 562-563 `roots[kMaxNumSnodeTreesLlvm]` /
`root_mem_sizes[kMaxNumSnodeTreesLlvm]`.

Therefore a `configure_file`-generated `constants.h` (following the
`version.h.in` precedent at CMakeLists.txt:203) reaches BOTH the host build and
the clang .bc build, because both resolve the include through the source tree.
That is the one mechanism I can verify covers the whole surface.

Loop driving the .bc builds: line 29
`foreach(arch IN LISTS HOST_ARCH CUDA_ARCH DX12_ARCH AMDGPU_ARCH)`.
`HOST_ARCH` is set at `cmake/TaichiCXXFlags.cmake:149`; `CUDA_ARCH`,
`AMDGPU_ARCH`, `DX12_ARCH` are set in the root `CMakeLists.txt:154-164`
guarded by the corresponding TI_WITH_*. Note the install() at line 13 uses
`${arch}` (the outer foreach variable) while the command uses `${rtm_arch}`
(the function parameter); they coincide only via CMake's parent-scope
visibility inside functions. Observation only.

## 7. Runtime backend selection — there is no registry

`taichi/program/program.cpp:53-161`, the `Program::Program(Arch desired_arch)`
constructor. Selection is a **hardcoded if/else chain wrapped in `#ifdef`**:

| line | condition | impl |
|---|---|---|
| 80-95  | `arch_uses_llvm(config.arch)` and arch != dx12 | `LlvmProgramImpl` |
| 82-92  | arch == dx12 | `Dx12ProgramImpl`, asserts `directx12::is_dx12_api_available()` |
| 96-102 | `Arch::metal` | `MetalProgramImpl`, asserts `metal::is_metal_api_available()` |
| 103-109| `Arch::vulkan` | `VulkanProgramImpl`, asserts `vulkan::is_vulkan_api_available()` |
| 110-116| `Arch::dx11` | `Dx11ProgramImpl`, asserts `directx11::is_dx_api_available()` |
| 117-123| `Arch::opengl` | `OpenglProgramImpl`, asserts `opengl::initialize_opengl(false)` |
| 124-130| `Arch::gles` | `OpenglProgramImpl`, asserts `opengl::initialize_opengl(true)` |
| 131-133| else | `TI_NOT_IMPLEMENTED` |

Each `#else` arm is `TI_ERROR("This taichi is not compiled with X")`
(90, 94, 101, 108, 115, 122, 129).

So: **a backend "becomes available" by being compiled in (`TI_WITH_X` defined,
which makes an `#ifdef` arm exist) and then by a per-backend
`is_*_api_available()` runtime probe that is a hard `TI_ASSERT`, not a
fallback.** There is no registration table, no factory map, no dynamic
loading of backend code. The set of possible backends is closed at compile
time in this single function. Includes are likewise `#ifdef`-guarded at
program.cpp:19-43.

Note program.cpp:9-10 includes opengl_program.h and metal_program.h
*unconditionally*, then again under `#ifdef` at 28-29 and 40-41. Observation.

This function is the single largest thing 6.3 touches.

`config.fit()` is called at program.cpp:77 before selection; need to check
whether it does any arch coercion (entry 10).

## 8. Where device capabilities are actually detected

`set_caps` is called from exactly 7 places (verified by exhaustive grep over
`taichi/` and `c_api/`):
- `taichi/rhi/vulkan/vulkan_device.cpp:1580` — constructor, sets only
  `spirv_version = 0x10000`.
- `taichi/rhi/vulkan/vulkan_device_creator.cpp:876` — the real one.
- `taichi/rhi/opengl/opengl_device.cpp:529`
- `taichi/rhi/dx/dx_device.cpp:565`
- `c_api/src/taichi_opengl_impl.cpp:12`
- `c_api/src/taichi_core_impl.cpp:331`
- `c_api/src/taichi_vulkan_impl.cpp:53`
Metal builds a `DeviceCapabilityConfig` in `taichi/rhi/metal/metal_device.mm:
1044-1070` and returns it (its caller assigns it; not via `set_caps`).

**Verified: nothing in `taichi/rhi/cpu/`, `taichi/rhi/cuda/`,
`taichi/rhi/llvm/` or `taichi/rhi/amdgpu/` sets any capability.** Grep for
`set_caps|caps` over those dirs returns nothing.
`ProgramImpl::get_device_caps()` at `taichi/program/program_impl.h:170-172`
returns `{}` by default and the **only** override in the tree is
`GfxProgramImpl::get_device_caps` at
`taichi/runtime/program_impls/gfx/gfx_program.cpp:82-85`.
So for CPU, CUDA, AMDGPU and DX12 the capability set is empty. Consequence:
there is currently **no capability mechanism at all on the LLVM side**, which
is the side that matters for the GTX 750 / 1070 tiers in brief 5.2.

Where int64 is decided per backend:
- Vulkan: `vulkan_device_creator.cpp:630-632`, gated on
  `VkPhysicalDeviceFeatures::shaderInt64`.
- OpenGL: `opengl_device.cpp:509-513` — set unconditionally for desktop GL,
  explicitly NOT set for GLES ("64bit isn't supported in ES profile").
- Metal: `metal_device.mm:1051-1053`, gated on `feature_64_bit_integer_math`.
- DX11: `dx_device.cpp:563-565` sets only `spirv_version`. **int64 is never
  set for DX11.**
- c_api OpenGL: `c_api/src/taichi_opengl_impl.cpp:9` sets
  `spirv_has_int64` unconditionally, with no GLES check — differs from
  `opengl_device.cpp:510`. Recording the divergence, not judging it.

Surprise, and directly relevant to 6.2: `spirv_has_physical_storage_buffer` —
the 64-bit buffer-device-address capability — is **dead code**. At
`taichi/rhi/vulkan/vulkan_device_creator.cpp:821-828` the set() is inside
`#if !defined(__APPLE__) && false`. The `&& false` means it is never set on
any platform. Comment on line 824 says "(penguinliong) Temporarily disabled
(until device capability is ready)."

Consumer side: `taichi/rhi/metal/metal_device.mm:131-135` reads
`spirv_has_int64` to pick MSL version 2.3.

## 9. AOT: what the module path can and cannot do

`taichi/aot/` is thin. Five source files, ~500 lines total.

Second hardcoded dispatch: `aot::Module::load(Arch, std::any)` at
`taichi/aot/module_loader.cpp:31-58`. Arms: vulkan(32-35), opengl(36-39),
gles(40-43), dx11(44-47), dx12(48-51), metal(52-55), else
`TI_NOT_IMPLEMENTED`(57). Everything except dx12 routes to
`gfx::make_aot_module`.

**Verified: there is NO arm for x64, arm64, cuda, amdgpu, js or opencl.**
The whole LLVM family cannot be loaded through `aot::Module::load` at all.
Note also the failure shape: if the arch matches but `TI_WITH_X` is undefined,
the `#ifdef` body vanishes, control falls out of the if/else chain, and it
reaches `TI_NOT_IMPLEMENTED` at line 57 with no message naming the arch.
Includes at module_loader.cpp:3-4 are unconditional.

There IS a builder side for LLVM: `Program::make_aot_module_builder`
(`program.cpp:541-557`) accepts `arch_uses_llvm(...)` (line 548). And
`taichi/runtime/llvm/llvm_aot_module_builder.*` exists. So LLVM AOT can be
*written* but not *loaded* through `aot::Module::load`. Recording as fact;
whether some other path loads it is beyond my territory (see Escalations).

### The capability gate for AOT modules exists and is inert

This is the single most important finding for 6.3.

- Producer: `taichi/runtime/gfx/aot_module_builder_impl.cpp:22-24` writes the
  builder's `DeviceCapabilityConfig` into
  `TaichiAotData::required_caps` as `{name string -> uint32 level}`.
- Storage: `taichi/runtime/gfx/aot_utils.h:20` field,
  serialised at line 23 via `TI_IO_DEF`. Mirrored in the generic
  `aot::ModuleData` at `taichi/aot/module_data.h:125` and 135.
- Serialised to `metadata.json` at
  `aot_module_builder_impl.cpp:57-61`.
- Deserialised back into `ti_aot_data_` at
  `taichi/runtime/gfx/aot_module_loader_impl.cpp:43-46`.
- Consumer: `c_api/src/taichi_gfx_impl.cpp:18-29` compares the loaded module's
  required caps against the live device's caps and returns
  `TI_ERROR_INCOMPATIBLE_MODULE` on mismatch.

**But the chain is broken in the middle.** `aot::Module::get_required_caps()`
at `taichi/aot/module_loader.h:97-100` returns a static empty
`DeviceCapabilityConfig`. Exhaustive grep over `taichi/` and `c_api/` for
`get_required_caps` returns exactly two hits: the base definition and the
c_api call site. **No subclass overrides it.** `gfx::AotModuleImpl`
(aot_module_loader_impl.cpp:23-190) overrides `get_graph`, `get_root_size`,
`arch`, `version`, `make_new_kernel`, `make_new_kernel_template`,
`make_new_field` — but not `get_required_caps`. So `required_devcaps.devcaps`
is always empty at taichi_gfx_impl.cpp:22 and the loop never runs. The
compatibility check is dead.

Also: `str2devcap` (`device_capability.cpp:6-13`) is the inverse of the
`to_string` used by the builder at aot_module_builder_impl.cpp:23, but its
only callers are `translate_devcaps`/`make_aot_module_builder`
(`program.cpp:530`, `544`). The loader never calls it. That is the missing
link — the strings are read into a `std::map<std::string,uint32_t>` and
stop there.

Second point on the same code: the comparison at
`c_api/src/taichi_gfx_impl.cpp:25` is `current_version != required_version`,
exact equality, not `>=`. A device that exceeds the module's requirement would
be rejected. Flagging, not resolving.

### Target-tier selection at AOT build time already has a shape

`translate_devcaps` at `taichi/program/program.cpp:514-539` parses caps from
a `std::vector<std::string>` in the form `spirv_has_int8` or
`spirv_version=1.3` (comment lines 515-517), defaulting spirv_version to
0x10300 if absent (535-537). `Program::make_aot_module_builder`
(541-557) passes the result to `program_impl_->make_aot_module_builder(cfg)`.
So the *target* capability set for an AOT module is already an explicit input
rather than a probe of the local device. That is the closest existing thing to
"build a module for tier X".

Caveat at program.cpp:545-547 is an upstream FIXME: building a Metal AOT
module still requires being on macOS, because the runtime backend and the
target AOT backend are coupled.

## 10. c_api — the third hardcoded dispatch, plus the one real detection API

`cmake/TaichiCAPI.cmake:20-64` builds the c_api source list per toggle:
- always `c_api/src/taichi_core_impl.cpp` (21)
- `TI_WITH_LLVM` → taichi_llvm_impl.cpp + taichi_cpu.h (31-33), and
  `TI_WITH_CUDA` adds only the *header* taichi_cuda.h (35-37)
- `TI_WITH_OPENGL OR TI_WITH_VULKAN OR TI_WITH_METAL` → taichi_gfx_impl.cpp (40-42)
- opengl (44-47), metal (49-52), vulkan (54-60)
There is **no c_api impl or header for dx11, dx12, amdgpu or gles**.

Runtime creation: `ti_create_runtime(TiArch, uint32_t device_index)` at
`c_api/src/taichi_core_impl.cpp:247-309`. A `switch` on `TiArch` with each
case wrapped in `#ifdef`: VULKAN 252-267, OPENGL 268-274, X64 275-281,
ARM64 282-287, CUDA 288-293, METAL 294-301, `default:` →
`TI_CAPI_NOT_SUPPORTED(arch)` 302-305. `device_index` is accepted only for
Vulkan (via `set_vulkan_visible_device`, line 254); every other arm asserts
`device_index != 0` is unsupported (256, 277, 284, 290, 296).

**Availability probing.** `ti_get_available_archs(uint32_t*, TiArch*)` at
`c_api/src/taichi_core_impl.cpp:171-202`. Builds a thread_local vector by
calling six free functions defined at the top of the same file:
`is_vulkan_available` 13-19, `is_opengl_available` 21-27,
`is_cuda_available` 29-35, `is_x64_available` 37-45,
`is_arm64_available` 47-53, `is_metal_available` 55-61. Each is
`#ifdef TI_WITH_X` around a real probe, `false` otherwise. x64/arm64 are
pure preprocessor, no probe. The list omits gles, dx11, dx12 and amdgpu
entirely (179-196).

This is the nearest thing in the tree to the brief's install-time loader
examining the system. It answers "which arch", never "which tier of that
arch" — there is no memory size, no compute capability, no device name.

**Capability query/override across the C boundary already exists:**
- `ti_get_runtime_capabilities` at
  `c_api/src/taichi_core_impl.cpp:336-...` reads `Device::get_caps()`.
- `ti_set_runtime_capabilities_ext` at
  `c_api/src/taichi_core_impl.cpp:317-334` writes them via `set_caps`
  (line 331). It casts the raw `uint32` straight to
  `taichi::lang::DeviceCapability` (line 329), so the C ABI value and the
  x-macro ordinal in `taichi/inc/rhi_constants.inc.h` must stay in lockstep.
  Adding a capability anywhere but the end of that file changes the ABI.
  Recording as a constraint on 6.2/6.3, not proposing anything.

Given entry 8, `ti_get_runtime_capabilities` on an x64 or CUDA runtime
returns an empty set today.

## 11. Where 6.1 lands, gathered

`taichi_max_num_snodes` referenced at (exhaustive grep pending, entry 12):
declaration `taichi/inc/constants.h:12`.

## 12. Exhaustive grep for the 6.1 / 6.2 constants (whole tree minus external/)

`taichi_max_num_snodes` — only three sites:
- `taichi/inc/constants.h:12` (declaration)
- `taichi/codegen/llvm/struct_llvm.cpp:266` (`TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);`)
- `taichi/runtime/llvm/runtime_module/runtime.cpp:567,568,569` (the three arrays)
**None in `taichi/rhi/`, `taichi/aot/`, `c_api/` or `cmake/`.** So 6.1 does
not touch my territory at its *use* sites at all; it touches my territory only
at the *threading mechanism* (entry 6) and the build/install packaging.

`kMaxNumSnodeTreesLlvm` — `constants.h:13`, `runtime.cpp:562,563`.

`taichi_max_num_indices` (6.2's related constant) — 20 sites, all in
ir/program/codegen/transforms/runtime, none in rhi/aot/c_api/cmake:
snode.h:28,30,78,81; snode.cpp:21,90,105;
launch_context_builder.cpp:230,247,269,302,322;
export_lang.cpp:559,1222; offline_cache_util.cpp:110;
struct_llvm.cpp:171; codegen_llvm.cpp:2363 (comment);
demote_dense_struct_fors.cpp:19,23; scalar_pointer_lowerer.cpp:33,36,40;
runtime.cpp:289,1012.

## 13. No dynamic module loading exists

`DynamicLoader` (`taichi/system/dynamic_loader.h:12-34`, impl
`taichi/common/dynamic_loader.cpp`) is the only dlopen wrapper. Its users,
exhaustively: `taichi/rhi/cuda/cuda_driver.cpp:34,89,98,107`,
`taichi/rhi/amdgpu/amdgpu_driver.cpp:33`,
`taichi/rhi/vulkan/vulkan_loader.cpp:98`. Every one loads a **vendor driver
library** (libcuda / libamdhip / libvulkan). Nothing loads Taichi's own code
dynamically. So 6.3 has no existing plugin substrate to extend; the only
"module loading" in the tree is the AOT metadata+SPIR-V path of entry 9.

Odd: `taichi/system/dynamic_loader.h` and `taichi/common/dynamic_loader.h`
both exist and differ — the common one additionally declares
`static bool check_lib_loaded(const std::string&)` (used at
cuda_driver.cpp:54). Two headers for one class. Observation only.

## 14. CUDA tier information exists but is outside DeviceCapability

Directly relevant to brief 5.2. `taichi/rhi/cuda/cuda_context.cpp:20-86`
(constructor of `CUDAContext`) detects:
- device name, line 25-27
- compute capability major/minor via
  `CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR/MINOR`, lines 29-33
- memory-pool support, 35-63
- total/free memory via `mem_get_info`, `get_total_memory` 88-92,
  `get_free_memory` 94-98
- `CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_BLOCK_OPTIN`, lines 79-82

It computes `compute_capability_ = cc_major * 10 + cc_minor` (line 73) and
`mcpu_ = "sm_" + compute_capability_` (line 83) for NVPTX codegen.

**Surprise: line 75-77 clamps `compute_capability_` to at most 86.** An
Ampere RTX 3060 is sm_86, so it passes through unchanged; anything newer
(Ada sm_89, Hopper sm_90, Blackwell) is silently compiled as sm_86. The
brief's 5.2 says "Beyond the upper tier, rough upwards scaling is expected" —
this clamp is where that stops.

Also line 22-23: `driver_.device_get_count(&dev_count_)` then
`driver_.device_get(&device_, 0)` — **CUDA device index is hardcoded to 0.**
`ti_create_runtime` rejects any non-zero `device_index` for CUDA
(`c_api/src/taichi_core_impl.cpp:289-290`). Only Vulkan has a visible-device
selector (`set_vulkan_visible_device`, `taichi/rhi/vulkan/vulkan_loader.cpp:
144-146`, consumed at `vulkan_device_creator.cpp:442-459`). This box has a
GTX 750 and a GTX 1070; on CUDA only device 0 is reachable from the core.

None of `compute_capability_`, `get_total_memory()` or
`max_shared_memory_bytes_` is ever written into a `DeviceCapabilityConfig`
(entry 8 grep). They reach codegen through `CUDAContext` directly.
`taichi/rhi/cuda/cuda_device.h:151-153` exposes `get_total_memory()` on the
`Device` interface.

## 15. Build-time option plumbing outside CMake itself

`.github/workflows/scripts/ti_build/cmake.py` is the wrapper that turns
options into a build.
- `OPTION_RE` line 17 scrapes `option(NAME "desc" ON|OFF)` — **and only that
  exact form** — from `CMakeLists.txt` and `cmake/*.cmake`
  (`collect_options` called at line 147).
- The trailing `# wheel-tag: xx` comment on each option (TaichiCore.cmake:2-11,
  root CMakeLists.txt:71,73,206,207,232) feeds `render_wheel_tag` (109-116),
  which joins the tags of enabled options into the wheel filename. This is the
  existing "one artefact per feature combination" mechanism.
- `DEF_RE` line 18 parses `-DNAME=value` out of the `TAICHI_CMAKE_ARGS`
  environment variable (`parse_initial_args`, 43-46).
- Line 84: `assert not wheel_tag, "Set a non boolean value to an option with
  wheel-tag"`. A **non-boolean option cannot carry a wheel tag.** So an
  integer SNode-ceiling option would pass through `-D` fine but could not be
  encoded into the wheel name by this mechanism as it stands.

## 16. Correction to entry 9 — there are TWO AOT load paths, not one

I was wrong to imply LLVM AOT modules cannot be loaded. They can, but
**not through `aot::Module::load`**. The LLVM path is a separate,
parallel one:

`c_api/src/taichi_llvm_impl.cpp:78-121`, `capi::LlvmRuntime::load_aot_module`,
calls `taichi::lang::LLVM::make_aot_module` directly
(`taichi/runtime/llvm/llvm_aot_module_loader.cpp:94-98`), constructing
`LLVM::LlvmAotModule` (`llvm_aot_module_loader.h:18-92`). It branches on
`arch_is_cpu(config.arch)` at line 82; the else arm is CUDA under
`#ifdef TI_WITH_CUDA` (92-104), `TI_NOT_IMPLEMENTED` otherwise.

So the two module formats and paths are:

| | gfx / dx12 | LLVM |
|---|---|---|
| entry | `aot::Module::load` (module_loader.cpp:31) | `LlvmRuntime::load_aot_module` (taichi_llvm_impl.cpp:78) |
| impl | `gfx::AotModuleImpl` | `LLVM::LlvmAotModule` |
| format | `metadata.json` + per-task `.spv`, JSON | `LlvmOfflineCacheFileReader` + `graphs.tcb`, binary |
| caps check | present but dead (entry 9) | **none at all** |
| SNode trees | hardcoded `num_snode_trees = 1` (`gfx/aot_module_loader_impl.cpp:136-139`, "we only support a single SNodeTree during AOT") | `cache_reader_->get_num_snode_trees()` (`llvm_aot_module_loader.h:48-50`), loop at `taichi_llvm_impl.cpp:114-118` |
| root size | `ti_aot_data_.root_buffer_size` | `return 0` (`llvm_aot_module_loader.h:40-42`) |

The dispatcher for both is `Runtime::load_aot_module` /
`Runtime::create_aot_module`, virtuals on the c_api `Runtime` base at
`c_api/src/taichi_core_impl.h:133-145`. `load_aot_module` is marked
`[[deprecated("create_aot_module")]]` (line 133) and by default forwards to
`create_aot_module`; `LlvmRuntime` overrides `load_aot_module` and never
implements `create_aot_module`, whose base is `TI_NOT_IMPLEMENTED`
(142-145). `ti_load_aot_module` (`taichi_core_impl.cpp:623-640`) has an
upstream comment at 629-630: "Should call `create_aot_module` directly after
all backends adapted to it."

`ti_create_aot_module` (`taichi_core_impl.cpp:641-660`) takes a `.tcm` zip
via `VirtualDir::from_zip`, so the gfx path can load a module from memory.
The LLVM path cannot; `LlvmAotModule`'s constructor takes a filesystem
directory path (`llvm_aot_module_loader.h:20-30`).

## 17. The c_api enums are generated from taichi/inc/*.inc.h

Verified by reading the generator, not by assumption:
`misc/taichi_json.py:102-116` `load_inc_enums()` globs `taichi/inc/*.inc.h`,
matches lines of the form `MACRO(case)` with a regex (line 108), and assigns
each case an ordinal equal to its position in the file (line 115,
`len(cases[key])`). `Enumeration.__init__` at
`misc/taichi_json.py:182-185` uses that when the JSON entry has
`"inc_cases"`. `c_api/taichi.json:125-129` declares TiCapability with
`"inc_cases": "PER_DEVICE_CAPABILITY"`. `misc/generate_c_api.py` (last lines,
`Module.load_all` then `generate_module_header`) writes the headers, which are
then checked in.

So `TiCapability` in `c_api/include/taichi/taichi_core.h:380-406`
(values 0..24) is mechanically derived from
`taichi/inc/rhi_constants.inc.h:10-34`. Inserting a capability anywhere but
at the end of that file renumbers the C ABI. Regeneration is a manual script
run, not a build step — the headers are checked in.

`TiArch` is **not** generated from `archs.inc.h`. `c_api/taichi.json:110-123`
spells its cases out by hand: reserved 0, vulkan 1, metal 2, cuda 3, x64 4,
arm64 5, opengl 6, gles 7. That ordering is unrelated to
`taichi/inc/archs.inc.h` (x64 0, arm64 1, js 2, cuda 3, metal 4, opengl 5,
dx11 6, dx12 7, opencl 8, amdgpu 9, vulkan 10, gles 11). Two independent arch
numberings that must be translated between. dx11, dx12, amdgpu, js and opencl
have no `TiArch` value at all.

## 18. constants.h values are hand-duplicated across the C boundary

`c_api/src/taichi_core_impl.h:121-122`:
`std::array<uint64_t, 32> host_result_buffer_;` with the comment
"32 is a magic number in `taichi/inc/constants.h`". That 32 is
`taichi_result_buffer_entries` (`constants.h:20`), copied as a literal.
Relevant to 6.1 as a pattern: a constants.h value made build-time
parameterisable would not automatically propagate to hand-copied literals
like this one. I did not audit for other copies beyond this one.

## 19. Process note — accidental exposure

While grepping for `taichi.json` across the repo I did not exclude
`modernization/` and three lines of the other pass's notes file scrolled past
in the grep output. I did not open the file and did not read further. The
overlapping subject (c_api header generation from taichi.json) I then
verified independently from `misc/taichi_json.py` and `c_api/taichi.json`
before writing entry 17. All later greps exclude `modernization/`.

## 20. `spirv_has_physical_storage_buffer` is never set anywhere. Correction and expansion of entry 8.

Exhaustive grep over `taichi/` and `c_api/` for the symbol. Two producer
sites, **both disabled**:
- `taichi/rhi/vulkan/vulkan_device_creator.cpp:826`, inside
  `#if !defined(__APPLE__) && false` (guard opens line 825, closes 827).
- `c_api/src/taichi_vulkan_impl.cpp:48`, inside a `/* ... */` comment block
  spanning lines 46-51, preceded by "(penguinliong) Will bring it back after
  devcap." on line 45.

(I initially recorded the c_api site as live. It is not; it is commented out.)

Consumer sites, all of which therefore permanently take the false branch:
- `taichi/rhi/vulkan/vulkan_device.cpp:1772, 1792, 2147, 2509`
- `taichi/codegen/spirv/spirv_ir_builder.cpp:73, 113`
- `taichi/codegen/spirv/spirv_codegen.cpp:783, 2340, 2416, 2491`
- `taichi/runtime/gfx/runtime.cpp:96`
- `taichi/runtime/program_impls/gfx/gfx_program.h:80-83`

The last one is the sharpest: `GfxProgramImpl::get_kernel_argument_data_layout`
returns `"1" + (has_buffer_ptr ? "b" : "-")`. With the capability never set it
always returns `"1-"`. So the entire buffer-device-address (64-bit shader
pointer) code path in SPIR-V codegen is written and compiled but unreachable
in every configuration. Directly relevant to 6.2: the machinery for 64-bit
addressing on the SPIR-V side partly exists and is switched off at the
capability layer, not at the codegen layer.

`spirv_has_int64` consumers, for contrast, are live:
`taichi/codegen/spirv/spirv_ir_builder.cpp:64, 166, 312, 326`
and `taichi/rhi/metal/metal_device.mm:132`.
`spirv_has_atomic_int64` (`taichi/inc/rhi_constants.inc.h:17`) has **no
producer and no consumer anywhere** — grep returns only the .inc.h line.

## 21. Every CMake option in the project is boolean

Checked `cmake/*.cmake`, root `CMakeLists.txt`, `c_api/cmake/`. Every
`option()` is ON/OFF. The only non-boolean cache entries are
`CMAKE_BUILD_TYPE` (`CMakeLists.txt:41`, `CACHE STRING`) and the internal
`HOST_ARCH` (`cmake/TaichiCXXFlags.cmake:149`, `CACHE INTERNAL`); the
gtest ones (`cmake/TaichiTests.cmake:7`, `cmake/TaichiCAPITests.cmake:7`) are
`CACHE BOOL`.

So 6.1's install-time factor would be the first numeric project-level build
parameter. Combined with entry 15 (wheel-tag asserts non-boolean options carry
no tag) and entry 6 (the .bc clang invocation ignores `CMAKE_CXX_FLAGS`), the
constraints on how the value can be threaded are tight.

## 22. `taichi/aot/` size and content, for the record

183 + 81 + 139 + 125 lines of headers, 146 + 68 + 94 of source; 836 lines
total across the seven files. `graph_data.h` holds `aot::Arg` (37-104),
`IValue` (107-133), `aot::Kernel` (136-154), `CompiledDispatch` (156-163)
and `CompiledGraph` (165-176). `CompiledGraph::run` takes a
`std::unordered_map<std::string, IValue>` (line 169); `IValue::val` is a
`uint64` (line 109). The whole `taichi/aot/*.cpp|*.h` set is globbed into
`TAICHI_CORE_SOURCE` at `cmake/TaichiCore.cmake:86` — unconditionally, with
no TI_WITH_* guard. That is why `module_loader.cpp` can include the gfx and
dx12 loader headers unconditionally (entry 9) and rely on `#ifdef` inside the
function body.

## 23. There IS one runtime module load of Taichi's own code: the LLVM .bc

I said in entry 13 that nothing loads Taichi's own code dynamically. That is
true of shared objects, but not of LLVM bitcode. Correcting/expanding:

`TaichiLLVMContext::module_from_file(const std::string &file)` at
`taichi/runtime/llvm/llvm_context.cpp:358-...` calls
`module_from_bitcode_file(fmt::format("{}/{}", runtime_lib_dir(), file), ctx)`
(361-362). The filename comes from `get_runtime_fn(Arch arch)` at
`llvm_context.cpp:209-211`:

    return fmt::format("runtime_{}.bc", arch_name(arch));

`runtime_lib_dir()` is `taichi/util/lang_util.cpp:30-47`: the global
`compiled_lib_dir` if set, otherwise the `TI_LIB_DIR` environment variable,
error if neither (33-45).

So at run time the LLVM backend loads a per-arch bitcode module by name from
an installed directory. The names are exactly the ones produced by the
`COMPILE_LLVM_RUNTIME` loop in
`taichi/runtime/llvm/runtime_module/CMakeLists.txt:29-31` and installed by
line 13 of that file (and re-installed for the c_api at
`cmake/TaichiCAPI.cmake:179-184`).

**This is the existing adaptive-module-loading mechanism in the tree**, and it
is on the LLVM side, keyed on `arch_name(arch)` alone — one module per arch,
no tier, no capability. It is also the module that contains the three
`taichi_max_num_snodes` arrays (entry 6). Anything 6.3 does for the LLVM
family plausibly starts here rather than at `aot::Module::load`.

Other bitcode loaded the same way, for completeness:
- `slim_libdevice.<cuda_major>.bc` — `libdevice_path()` at
  `llvm_context.cpp:213-218`, installed at `cmake/TaichiCore.cmake:429-432`.
- `cuda_runtime-cuda-nvptx64-nvidia-cuda-sm_60.bc` —
  `get_custom_cuda_library_path()` at `taichi/util/lang_util.cpp:18-28`,
  loaded at `llvm_context.cpp:574-591`. Its build rule
  (`runtime_module/CMakeLists.txt:17-26`) is commented out at lines 36-43, so
  the file is not produced; `get_custom_cuda_library_path` returns "" when it
  is absent (lang_util.cpp:23-25) and the link is skipped. Hardcoded sm_60.
- AMDGPU: a fixed list of ROCm device-library `.bc` files at
  `llvm_context.cpp:628-655`, including
  `"oclc_isa_version_" + isa_version + ".bc"` — an example of a *variant*
  bitcode module selected by detected hardware version. Installed from
  `external/amdgpu_libdevice/` at `cmake/TaichiCore.cmake:434-438`.

The AMDGPU `isa_version` pattern is the only place in the tree where a
bitcode module filename varies with detected hardware rather than with arch.

---

# Revision pass — after adversarial review

Entries 24 onward are the second pass, working from
`adversary-04-1.md` and `adversary-04-2.md`. Every fact below I re-opened in
the source myself before accepting it, including the ones both adversaries
agree on. Where the two adversaries diverge I say which the source supports.

## 24. Metal DOES call set_caps. My error, and its root cause.

Both adversaries caught this. Verified:
`taichi/rhi/metal/metal_device.mm:1162-1169`, the `MetalDevice` constructor:

    DeviceCapabilityConfig caps = collect_metal_device_caps(mtl_device);   // :1167
    set_caps(std::move(caps));                                             // :1168

So `collect_metal_device_caps` (the function at :1017-1070 whose *tail* I read
at :1044-1070) returns a config and its caller calls `set_caps` on the result.
I treated "returns a config" and "calls set_caps" as alternatives. They are
not.

**Root cause, which matters more than the fact.** My grep in entry 8 was

    grep -rn "set_caps" --include=*.cpp --include=*.h taichi/ c_api/

with no `--include=*.mm`. Metal's device is `metal_device.mm`. I then wrote
"exactly seven places (exhaustive grep over `taichi/` and `c_api/`)". The
grep was exhaustive over the file types I asked for and I described it as
exhaustive over the directories. That is the error that made the miscount
load-bearing: without the exhaustiveness claim a reader would re-check; with
it, they would not.

Re-run without the extension filter, excluding the declaration at
`taichi/rhi/public_device.h:855`, gives **eight** call sites:

| # | site | scope |
|---|---|---|
| 1 | `taichi/rhi/metal/metal_device.mm:1168` | rhi |
| 2 | `taichi/rhi/opengl/opengl_device.cpp:529` | rhi |
| 3 | `taichi/rhi/vulkan/vulkan_device.cpp:1580` | rhi |
| 4 | `taichi/rhi/dx/dx_device.cpp:565` | rhi |
| 5 | `taichi/rhi/vulkan/vulkan_device_creator.cpp:876` | rhi |
| 6 | `c_api/src/taichi_opengl_impl.cpp:12` | c_api |
| 7 | `c_api/src/taichi_core_impl.cpp:331` | c_api, host-supplied |
| 8 | `c_api/src/taichi_vulkan_impl.cpp:53` | c_api |

Five in the RHI, three in the C API.

I am striking the word "exhaustive" from every grep claim in the report and
replacing it with the actual command scope, so a reader can tell what was and
was not searched. Checking my other greps for the same defect: the
`get_required_caps`, `spirv_has_physical_storage_buffer`,
`taichi_max_num_snodes` and `DynamicLoader` greps in entries 9, 12, 13 and 20
also omitted `*.mm`. Re-run with `.mm` included, all four are unchanged —
no Metal file contains any of those symbols. The `spirv_has_atomic_int64`
claim does change; see entry 31.

## 25. The AMDGPU CMake typo is inert, because the file does not exist

Both adversaries caught this. Verified by listing the directories:

    taichi/runtime/amdgpu/  → CMakeLists.txt jit_amdgpu.cpp jit_amdgpu.h
                              kernel_launcher.cpp kernel_launcher.h
    taichi/runtime/cuda/    → CMakeLists.txt jit_cuda.cpp jit_cuda.h
                              kernel_launcher.cpp kernel_launcher.h

**Neither `runtime.cpp` exists.** So `file(GLOB TAICHI_AMDGPU_RUNTIME_SOURCE
"taichi/runtime/amdgpu/runtime.cpp")` at `cmake/TaichiCore.cmake:110` matches
nothing, and the misspelled append at :111 appends an empty list to a variable
nobody reads. The correctly spelled CUDA equivalent at :104-105 also appends
nothing.

I wrote "So `taichi/runtime/amdgpu/runtime.cpp` is **never** added", which
asserts the file exists and is being dropped. Wrong. The typo is a dormant
defect: harmless today, and it would silently stay harmless if that file were
ever added. That is a different and weaker claim than the one I made, and I
should have run `ls` before writing it. I checked the CUDA line as a control
and drew the wrong conclusion from the contrast, instead of checking the
inputs to both globs.

## 26. My central 6.1 premise was too strong. `CUDA_VERSION` is the existing precedent.

This is the correction that changes a conclusion rather than a fact. Raised by
adversary 04-1 as S2; adversary 04-2 endorsed my §5.4 without qualification
and is wrong to have done so.

I wrote (report §5.4): "a numeric SNode ceiling would be the first
non-boolean project-level build parameter", and built escalation 1 on the idea
that only a generated header could work and that this was novel territory.

Verified end to end, the mechanism already exists and is in use:

| step | site |
|---|---|
| default, overridable by `-DCUDA_VERSION=` | `cmake/TaichiCore.cmake:98-100`, `if(NOT CUDA_VERSION) set(CUDA_VERSION 10.0) endif()` |
| forced when CUDA is off | `CMakeLists.txt:149-152`, `set(CUDA_VERSION "0.0")` |
| substituted into a generated header | `taichi/common/version.h.in:5`, `#define CUDA_VERSION "@CUDA_VERSION@"`, by the `configure_file` at `CMakeLists.txt:203` |
| exposed to C++ | `taichi/common/core.cpp:87-89`, `get_cuda_version_string()` returns it |
| used to select a device bitcode file | `taichi/runtime/llvm/llvm_context.cpp:213-218`, `libdevice_path()` builds `slim_libdevice.<major>.bc` |

So a non-boolean CMake value already travels configure → generated header →
host C++ → run-time choice of which bitcode module to load. My claim is
falsified and escalation 1 rests on a false premise.

**But the precedent covers only half of 6.1, and neither adversary says which
half.** `CUDA_VERSION` is never compiled *into* a bitcode module; it is read by
host C++ to pick between prebuilt `.bc` files. `runtime.cpp` does not include
`taichi/common/version.h` — grep for `version.h` and `CUDA_VERSION` in
`taichi/runtime/llvm/runtime_module/runtime.cpp` returns nothing, and the only
includers of `taichi/common/version.h` in the tree are
`taichi/common/core.cpp:7`, `taichi/util/offline_cache.h:12` and
`taichi/runtime/llvm/llvm_offline_cache.cpp:14`.

The SNode ceiling is different in kind: it sizes a struct that both the host
and the bitcode must agree on (`runtime.cpp:567-569`). So:
- `CUDA_VERSION` proves the *configure_file into a generated header* route
  works and is established practice. That half of my escalation was
  unnecessary hand-wringing.
- It does not prove a generated header reaches the **bitcode** build. That is
  proved separately, and still stands: `runtime.cpp:25` includes
  `taichi/inc/constants.h`, resolved only through the
  `-I ${PROJECT_SOURCE_DIR}` on
  `taichi/runtime/llvm/runtime_module/CMakeLists.txt:8`, and that clang
  command line carries no `${CMAKE_CXX_FLAGS}`.

The two facts compose into a stronger and simpler statement than either of my
originals, which is what goes in the revised report.

## 27. The generated headers are gitignored, which retires half of my escalation 1

Neither adversary has this, and adversary 04-1's escalation 4 explicitly rests
on the source-tree question being open ("It writes a generated header into the
source tree, which is the hygiene question A's escalation 1 and B's
escalation 1 both stop at").

Verified:
- `ls taichi/common/version.h taichi/common/commit_hash.h` → **neither file
  exists** in this checkout. They are produced only by a configure run.
- `.gitignore:63-64` lists `/taichi/common/version.h` and
  `/taichi/common/commit_hash.h`.

So the established pattern is: generate into the source tree, and gitignore
the product. It is not "bake a value into a checked-in file", which is what I
was worried about. My escalation 1 as written asks a question the repository
has already answered by convention.

What survives is narrower and real: `taichi/inc/constants.h` is a **checked-in**
header, not a gitignored generated one, so making it generated means deleting
a tracked file and adding a `.in`. That is a different decision from the one I
escalated.

## 28. The `-I` alternative is not symmetric with generating a header

Adversary 04-2 §6.5 makes this point against pass A's escalation; it applies
to me too, since my report leaves "a generated `constants.h`" unqualified as
to where it is generated. Verified by reading the command at
`taichi/runtime/llvm/runtime_module/CMakeLists.txt:8`: the include path is
exactly one entry, `-I ${PROJECT_SOURCE_DIR}`.

If a generated `constants.h` were emitted into the binary directory and that
directory prepended to `-I`, the checked-in `taichi/inc/constants.h` would
still be on disk and still visible through the existing `-I`, so which of the
two wins is decided by include-directory ordering. That is an
order-dependent build with two headers of the same relative path. Generating
into the source tree, as `version.h` does, avoids that by there being only
one file. Recording the asymmetry; not choosing.

Related, and also from 04-2 §6.4, verified: the bitcode artefacts are
*already* produced into the source tree —
`runtime_module/CMakeLists.txt:10` sets
`WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"` and :13 installs from
`${CMAKE_SOURCE_DIR}/taichi/runtime/llvm/runtime_module/`. The in-file TODO at
:9 says "it's better to avoid polluting the source dir, keep in build". So if
6.3 multiplies device modules per arch, the multiplication lands in the source
tree under the mechanism as it stands.

## 29. Gap: there is no arch or version handshake on AOT load either

Adversary 04-1's S1. 04-2 does not have it. Verified independently and I find
one more override than 04-1 did.

`aot::Module` declares three pure virtuals at
`taichi/aot/module_loader.h:84-86`: `arch()`, `version()`, `get_root_size()`.

`version()` overrides, all three of them:
- gfx: `taichi/runtime/gfx/aot_module_loader_impl.cpp:114-116` →
  `TI_NOT_IMPLEMENTED`
- dx12: `taichi/runtime/dx12/aot_module_loader_impl.cpp:67-69` →
  `TI_NOT_IMPLEMENTED` (04-1 mentions only gfx and LLVM)
- LLVM: `taichi/runtime/llvm/llvm_aot_module_loader.h:36-38` → `return 0`

`arch()` overrides:
- gfx: `aot_module_loader_impl.cpp:111-113` → `device_api_backend_`, which is
  the arch the *caller* passed in at `taichi/aot/module_loader.cpp:31`
- dx12: `dx12/aot_module_loader_impl.cpp:64-66` → same
- LLVM: `llvm_aot_module_loader.h:32-34` → `executor_->get_config().arch`,
  the runtime's own arch

**No caller anywhere.** Grep for `version()` across `taichi/aot/`,
`taichi/runtime/gfx/`, `taichi/runtime/llvm/`, `taichi/runtime/dx12/` and
`c_api/src/` returns exactly four lines: the declaration and the three
overrides. A pattern-grep for `<something>->arch()` or `<something>.arch()` on
a module object across `taichi/` and `c_api/` returns nothing.

And the module file records no arch: `TI_IO_DEF(kernels, fields,
required_caps, root_buffer_size)` at `taichi/runtime/gfx/aot_utils.h:23`,
mirrored at `taichi/aot/module_data.h:135`.

Consequence: every one of the three possible handshakes on AOT load is absent.
`arch()` returns the caller's own answer back to it, `version()` is never
asked, and the capability check is wired but dead (my §3.2). My report framed
6.3's AOT problem as "one dead check to wire up". The accurate frame is "no
handshake exists; the capability slot is the only one wired even half-way".

Worth noting the pattern exists one layer down and was not lifted: the
*kernel* level does carry and check an arch —
`taichi/compilation_manager/kernel_compilation_manager.cpp:225`,
`taichi/runtime/cuda/kernel_launcher.cpp:191`,
`taichi/runtime/amdgpu/kernel_launcher.cpp:154`.

## 30. Gap: `aot::ModuleData::required_caps` is never written by anybody

Adversary 04-2 §6.2. Verified.

There are **two** structs with a `required_caps` field, and my report
described them as one channel:
- `gfx::TaichiAotData` (`taichi/runtime/gfx/aot_utils.h:20`, serialised :23) —
  written at `taichi/runtime/gfx/aot_module_builder_impl.cpp:23`. Live.
- `aot::ModuleData` (`taichi/aot/module_data.h:125`, serialised :135) —
  grep for `required_caps` across `taichi/` and `c_api/` finds exactly one
  write in the tree, the gfx one above. **Nothing ever writes this one.**

Its only subclass anywhere is `ModuleDataDX12 : public aot::ModuleData`
(`taichi/runtime/dx12/aot_module_builder_impl.h:11`), which adds
`dxil_codes` and populates no caps.

So the declaration side of the capability story is thinner than I said. My
§3.2 step 2 called `aot::ModuleData` a "generic mirror"; it is a dead
parallel struct.

## 31. Gap: capability renumbering also moves offline-cache keys

Adversary 04-2 §6.3. Verified, and it is live code, not dead.

`get_offline_cache_key_of_device_caps` at
`taichi/analysis/offline_cache_util.cpp:88-95` serialises `caps.devcaps`
directly at :92 — the raw `std::map<DeviceCapability, uint32_t>`, keyed by the
**numeric enum value**. It is called at :185 and folded into the picosha2 hash
at :190.

So the positional ABI constraint I raised in §2.6 has a second consumer:
inserting a `PER_DEVICE_CAPABILITY` anywhere but the end of
`taichi/inc/rhi_constants.inc.h` silently changes every offline-cache key as
well as the C ABI. I had only the C ABI.

Also correcting entry 20 while I am here: I wrote that
`spirv_has_atomic_int64` grep "returns only the declaration". Re-run including
`.mm` and the C++ wrapper, it returns four lines outside `external/`:
`taichi/inc/rhi_constants.inc.h:17`,
`c_api/include/taichi/taichi_core.h:388`,
`c_api/include/taichi/cpp/taichi.hpp:1121-1122` (a setter in the header-only
C++ wrapper) and `python/taichi/lang/enums.py:27`. The substantive point is
unchanged and I restate it precisely: no `caps.set` and no `caps.get` for it
anywhere in the core. Both adversaries also miscounted this one — 04-2 lists
three lines and omits `taichi_core.h:388`.

## 32. Gap: `taichi/rhi/CMakeLists.txt:17` is a CMake precedence bug

Adversary 04-1's S4. 04-2 read the same file closely and did not flag it.
Verified by reading the line:

    if (TI_WITH_OPENGL OR TI_WITH_VULKAN AND NOT ANDROID)

CMake binds `AND` tighter than `OR`, so this parses as
`TI_WITH_OPENGL OR (TI_WITH_VULKAN AND NOT ANDROID)`. The `NOT ANDROID` guard
therefore does not cover the OpenGL arm. With OpenGL on and `ANDROID` set,
GLFW is still added as a subdirectory (:29), linked (:30) and put on the
public include path (:31-34).

I read this file line by line and cited `:42-105` twice without noticing line
17. Recording it as an observation. Whether Android is a target at all is not
settled anywhere in the brief, so I am not proposing anything.

## 33. Gap: the public umbrella header gates on macros the install never defines

Adversary 04-2 §6.1. 04-1 conceded it. Verified in full, and it is the
sharpest install-time finding in my territory.

`c_api/include/taichi/taichi.h` is 29 lines. After `taichi_platform.h` (:5)
and `taichi_core.h` (:7) it gates every per-backend public header on a
`TI_WITH_*` macro:

| line | macro | header |
|---|---|---|
| 9-11 | `TI_WITH_VULKAN` | `taichi_vulkan.h` |
| 13-15 | `TI_WITH_OPENGL` | `taichi_opengl.h` |
| 17-19 | `TI_WITH_CUDA` | `taichi_cuda.h` |
| 21-23 | `TI_WITH_CPU` | `taichi_cpu.h` |
| 25-27 | `TI_WITH_METAL` | `taichi_metal.h` |

These are **consumer-side** macros. The installed package defines none of
them: `cmake/TaichiConfig.cmake.in` is five lines (`@PACKAGE_INIT@`, an
include of `TaichiTargets.cmake`, `check_required_components("Runtime")`), and
`cmake/TaichiTargets.cmake:56-88` sets only `IMPORTED_LOCATION`,
`IMPORTED_SONAME`/`IMPORTED_IMPLIB` and `INTERFACE_INCLUDE_DIRECTORIES` on one
`IMPORTED` `taichi_c_api` target. No `INTERFACE_COMPILE_DEFINITIONS`, no
capability data, no tier data.

So a downstream consumer that includes `taichi/taichi.h` gets `taichi_core.h`
and nothing else, unless it defines the macros itself by guessing what the
binary was built with.

And **`TI_WITH_CPU` is defined nowhere in the repository.** Grep across all
C++, CMake, Python and JSON sources outside `external/` and `modernization/`
returns exactly two lines: `c_api/include/taichi/taichi.h:21` and `:23`, the
`#ifdef` and its `#endif` comment. So `taichi_cpu.h` is unreachable through
the umbrella header in every configuration, even though
`cmake/TaichiCAPI.cmake:31-33` installs it whenever `TI_WITH_LLVM` is on.

This is the fourth re-expression of the toggle list in my territory, after
`cmake/TaichiCore.cmake:1-11`, the `#ifdef` chains in `program.cpp` /
`module_loader.cpp` / `taichi_core_impl.cpp`, and
`cmake/TaichiCAPI.cmake:31-60`. It is the one facing the outside world, and it
is the concrete form of the install-time question in brief section 5. My §4.3
inferred that the C API "answers which arch and never which tier"; this is
worse than that — at the header level it does not answer which arch either.

## 34. Adjudicating the `!=` divergence. The adversaries split; the source is with 04-1.

04-1 (S5, divergence 1): `c_api/include/taichi/taichi_core.h:411-412`
documents that capability levels are not ordered, so `!=` is the only relation
the type supports; both reports were wrong to call it a defect, and it is a
contract question.

04-2 (escalation 5): "Both reports escalate it and both are right to", adding
only that fixing it is insufficient alone. 04-2 does not cite :411-412 and I
believe did not see it.

Verified. `c_api/include/taichi/taichi_core.h:409-416`:

    // Structure `TiCapabilityLevelInfo` (1.4.0)
    //
    // An integral device capability level. It currently is not guaranteed that a
    // higher level value is compatible with a lower level value.
    typedef struct TiCapabilityLevelInfo {
      TiCapability capability;
      uint32_t level;
    } TiCapabilityLevelInfo;

The same sentence appears in the published documentation at
`c_api/docs/taichi/taichi_core.h.md:281`. It is authored prose, not generated
from `c_api/taichi.json` — the JSON entry at :131-143 carries only the two
fields and no description. So it is a deliberate published statement.

**I side with 04-1 on classification.** I wrote in §3.2 that a device
exceeding the module's requirement "should presumably be accepted", which
assumes an ordering the published contract explicitly declines to promise.
That was me supplying an assumption, and it is exactly the thing the brief
tells me not to do. Reclassifying from defect to contract question.

**Two facts neither adversary has, which sharpen it in opposite directions.**

First, against the disclaimer: the tree already relies on ordering for the one
genuinely graded capability. `taichi/rhi/vulkan/vulkan_device_creator.cpp:577`
does `if (caps.get(DeviceCapability::spirv_version) < 0x10400)`, and consumers
compare `>= 0x10400` at `taichi/codegen/spirv/spirv_ir_builder.h:424` and
`taichi/codegen/spirv/spirv_codegen.cpp:2673`. So the internal code treats
`spirv_version` as ordered while the C API documentation says levels are not
guaranteed to be.

Second, and this refines 04-1 rather than contradicting it: 04-1 says `>=`
"is only sound for the capabilities that happen to be ordered
(`spirv_version`) and not for the ones that are booleans stored as levels".
For a boolean stored as 1, `current >= required` is in fact the correct
relation — "I have it, you need it" passes; "I lack it, you need it" fails.
Tallying every level ever written (`caps.set` across `taichi/` and `c_api/`,
including `.mm`): 29 sites pass `true`, 10 pass `1`, and 13 pass a
`spirv_version` value (0x10000, 0x10300, 0x10400, 0x10500). There is **no
capability in the tree today for which `>=` would give a wrong answer.**

So the objection to `>=` is not about any existing capability. It is that the
disclaimer reserves the right to introduce an unordered enumeration later, and
`>=` would silently mis-handle it. That is a real reason to leave `!=` alone
and a real reason the decision belongs to the owner, not to me.

## 35. Smaller corrections both adversaries raised, each re-checked

Accepted and verified individually:

- `arch_is_gpu`: I wrote "opencl and js are classified by omission".
  `taichi/rhi/arch.cpp:43` names js explicitly in `arch_is_cpu`
  (`arch == Arch::x64 || arch == Arch::arm64 || arch == Arch::js`). Only
  opencl falls through, landing on the GPU side. (04-1 B-6.)
- `get_total_memory` is **not** on the RHI `Device` base. It is
  `taichi/rhi/llvm/llvm_device.h:34`, `TI_NOT_IMPLEMENTED` by default,
  overridden at `taichi/rhi/cuda/cuda_device.h:151-153` and
  `taichi/rhi/amdgpu/amdgpu_device.h:120-122`. Grep for `get_total_memory` in
  `taichi/rhi/public_device.h` returns zero hits. Its only consumer is
  `taichi/runtime/llvm/llvm_runtime_executor.cpp:612`; nothing in `c_api/`
  calls it. This matters for 6.3: a tier-aware selector cannot read device
  memory through the RHI base class. (04-1 B-7.)
- `DynamicLoader`: I cited `taichi/system/dynamic_loader.h:12-34` as the
  definition. Nothing includes that header. Every include is
  `taichi/common/dynamic_loader.h`: `taichi/rhi/cuda/cuda_driver.h:5` and
  `cuda_driver.cpp:3`, `taichi/rhi/amdgpu/amdgpu_driver.h:5` and
  `amdgpu_driver.cpp:3`, `taichi/rhi/vulkan/vulkan_loader.h:7`, and the
  implementation at `taichi/common/dynamic_loader.cpp`. My conclusion is
  unaffected; the anchor was the unused copy. (Both adversaries.)
- `cmake/TaichiCore.cmake:1-11` holds **eleven** `option()` calls, not ten.
  I dropped `USE_STDCPP` at :1. Counted: 11. (Both adversaries.)
- `ti_get_available_archs` runs `c_api/src/taichi_core_impl.cpp:171-205`, not
  `:171-202`. (Both.)
- `TiCapability` is `c_api/include/taichi/taichi_core.h:380-407`, not
  `:380-406`; the closing `} TiCapability;` is at :407. (04-1.)
- GGUI's `target_compile_definitions` is `cmake/TaichiCore.cmake:384`.
  Verified: grep gives :384. **04-1 is wrong to say :383.** But 04-2 is right
  that my "only GGUI uses `target_compile_definitions`" is false across the
  build: `taichi/rhi/CMakeLists.txt:28` uses it for `TI_WITH_GLFW`. I am
  keeping my line number and narrowing my claim.
- The SPIR-V/gfx unconditional `add_subdirectory` calls are
  `cmake/TaichiCore.cmake:291-292` and the link gate is `:295-298`, not
  `:294-297`. Verified by grep. (04-1.)
- `cuda_context.cpp` calls `device_get_count` at `:21` and `device_get` at
  `:22`. I cited `:22-23`. (Both.)

Rejected after checking: nothing. Every correction either adversary put to me
held up in the source.

---

# Amendment pass. Entries 36-44.

Round two judged this report correct and the pair not complete. Five claims
were handed to me for adjudication, with an instruction to verify each against
source and to say so where a claim is wrong. I did not read the paired agent's
report or notes at any point in this pass; I read the two round-two adversary
files and the source.

## 36. Claim 1. UPHELD in full. `-D "ARCH_${rtm_arch}"` is exactly what I said did not exist.

My escalation 1 ended: "Nothing in the tree yet compiles a configure-time value
*into* the bitcode, which is what the ceiling requires." I quoted the clang
command line containing the counter-example directly above that sentence, in
both passes, and never read the flag.

Verified, each step opened:

- `taichi/runtime/llvm/runtime_module/CMakeLists.txt:8` carries
  `-D "ARCH_${rtm_arch}"`. `rtm_arch` is the parameter of the
  `COMPILE_LLVM_RUNTIME` function declared at `:3`.
- The values are configure-time. `HOST_ARCH` at
  `cmake/TaichiCXXFlags.cmake:149`, `set(HOST_ARCH ${ARCH} CACHE INTERNAL
  "Host arch")`, where `ARCH` is derived from `CMAKE_SYSTEM_PROCESSOR` above.
  `CUDA_ARCH` at `CMakeLists.txt:155`, `AMDGPU_ARCH` at `:159`, `DX12_ARCH` at
  `:163`, each inside `if (TI_WITH_*)`.
- The loop at `runtime_module/CMakeLists.txt:29` iterates them and calls
  `compile_llvm_runtime(${arch})` at `:30`.
- Consumption count. I grepped `ARCH_` across the whole
  `runtime_module/` directory: **21 lines**, all inside the bitcode translation
  unit. 19 in `runtime.cpp` at `:51, 119, 155, 347, 771, 798, 801, 860, 1144,
  1156, 1167, 1293, 1343, 1431, 1549, 1633, 1651, 1861, 1865`, plus
  `node_pointer.h:15` and `locked_task.h:7`. Both headers are included by
  `runtime.cpp` itself, at `:1660` and `:1872`, so both are inside the same
  compile. The claim said "roughly 21 sites"; it is 21 lines exactly. Every one
  is a preprocessor test: `#if ARCH_cuda`, `#if ARCH_cuda || ARCH_amdgpu`,
  `#ifdef ARCH_amdgpu`, `#if ARCH_x64 || ARCH_arm64`.
- One artefact per value, `runtime_${rtm_arch}.bc` at `:8`, installed at `:13`.
- Selected at run time by filename: `get_runtime_fn` at
  `taichi/runtime/llvm/llvm_context.cpp:209-211` formats
  `runtime_{arch_name}.bc`, and `module_from_file` at `:358-362` loads it from
  `runtime_lib_dir()`.

What it means for 6.1: the sentence in my escalation is false and the
escalation has to be rewritten rather than trimmed, because it was the surviving
core of the escalation after my revision retired the rest of it. Rewritten at
report 5.3.1 and escalation 1, which now presents three routes instead of one.

One caveat I checked before writing it up. `-D "ARCH_x64"` defines a bare macro
to 1. `taichi/inc/constants.h:12` is `constexpr int taichi_max_num_snodes =
1024;`, a declared name, so `-D taichi_max_num_snodes=4096` would macro-expand
the declaration into `constexpr int 4096 = 1024;` and fail to compile. The `-D`
route therefore needs a distinct macro name that `constants.h` reads. That is a
design decision and it is in the escalation, not in the findings.

## 37. Claim 2. UPHELD, both halves. My stated mechanism was wrong and my wording did foreclose a permitted route.

**The host never lays out `LLVMRuntime`.** Verified:

- `grep -rn "struct LLVMRuntime" taichi/ c_api/` returns five lines. Two are
  host forward declarations, `taichi/program/context.h:11` and
  `taichi/rhi/llvm/llvm_device.h:8`, both bare `struct LLVMRuntime;` and both
  used as an opaque pointer, at `context.h:18` and `llvm_device.h:14`. Three
  are in `runtime.cpp`: `:136`, `:337` and the sole definition at `:552`.
- `runtime.cpp` is not compiled into the host library at all.
  `cmake/TaichiCore.cmake:77-91` globs `"taichi/runtime/*.h"` and
  `"taichi/runtime/*.cpp"` **non-recursively**, so
  `taichi/runtime/llvm/runtime_module/runtime.cpp` never enters
  `TAICHI_CORE_SOURCE`.
- The host's LLVM type comes from the loaded bitcode.
  `TaichiLLVMContext::get_runtime_type` at
  `taichi/runtime/llvm/llvm_context.cpp:1004-1011` calls
  `llvm::StructType::getTypeByName(get_this_thread_runtime_module()->getContext(),
  "struct." + name)`, and `TaskCodeGenLLVM::get_runtime` at
  `taichi/codegen/llvm/codegen_llvm.cpp:2694-2698` bit-casts to it.
- Fields are read through accessors compiled inside the bitcode.
  `llvm_runtime_executor.cpp:328` calls
  `runtime_query<void *>("LLVMRuntime_get_element_lists", ...)`; that symbol is
  generated by `STRUCT_FIELD_ARRAY(LLVMRuntime, element_lists)` at
  `runtime.cpp:616` and `RUNTIME_STRUCT_FIELD_ARRAY(LLVMRuntime, element_lists)`
  at `:750`.
- `LlvmRuntimeExecutor::get_llvm_runtime` at
  `llvm_runtime_executor.cpp:767-769` is a `static_cast` on a `void *`, which
  needs no complete type.
- The host's only use of the constant is
  `taichi/codegen/llvm/struct_llvm.cpp:266`,
  `TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);`.

So "host and device would disagree on the runtime struct layout, silently" at
my 5.3 names a failure that cannot occur. The real failure of a host-only `-D`
is that the assertion bound rises while the bitcode arrays stay at their old
size, and the overflow lands inside the bitcode at the three write sites
`runtime.cpp:1005` (`element_lists`), `:1029` (`node_allocators`) and `:1038`
(`ambient_elements`). My conclusion — the value has to reach the clang line —
survives and is in fact strengthened, because those arrays exist **only** in
the bitcode. Corrected in place at 5.3 with the retracted sentence quoted.

Related and worth recording: the plan's own 6.1 correction already says the
assertion at `struct_llvm.cpp:266` does not guard those arrays, because it
bounds a per-tree count while the arrays are indexed by the process-global
`SNode::id`. So the host's single use of the constant is a bound that does not
protect the thing it appears to protect. I have not restated that as a finding
of mine; it is the planner's, and I cite it.

**The second half of the claim also holds.** My 5.2 read: the ceiling "sizes a
struct that the host and the bitcode must agree on, so it has to be compiled
into the bitcode, not consulted beside it". Brief section 5 permits configuring
"either by custom compilation or by activating the correct prebuilt binary,
whichever proves easier". My sentence rules the second out. It should not: the
value must be fixed at *some* bitcode compile, which does not mean one install
may hold only one value. Every piece of a prebuilt-variant route is already in
service for the arch parameter (entry 36). New subsection 5.3.2 records that
both routes are open and names the single host-side coupling, the assertion
bound, which install-time configuration satisfies by construction and run-time
adaptation does not. New escalation 17.

## 38. Claim 3. UPHELD. I called a branch permanent while citing the function that unpins it, twice.

`ti_set_runtime_capabilities_ext` at `c_api/src/taichi_core_impl.cpp:317-334`
builds a `DeviceCapabilityConfig` from caller-supplied values at `:326-330` and
installs it with `runtime2->get().set_caps(std::move(devcaps))` at `:331`. No
validation of any kind against the device. `Runtime::get()` is the pure virtual
at `c_api/src/taichi_core_impl.h:131` returning `taichi::lang::Device &`.

That lands on `Device::caps_` at `taichi/rhi/public_device.h:617`, get and set
at `:852-857`, which is the same storage every consumer reads: directly, as at
`taichi/runtime/gfx/runtime.cpp:95-96` (`device_->get_caps().get(...)`), or via
`GfxProgramImpl::get_device_caps` at
`taichi/runtime/program_impls/gfx/gfx_program.cpp:82-85`
(`runtime_->get_ti_device()->get_caps()`). The header-only C++ wrapper ships a
typed setter for this exact capability at
`c_api/include/taichi/cpp/taichi.hpp:1156`.

I cite that function at my 2.2 (site 7 of the `set_caps` table) and describe it
at my 4.3, and never reconciled either with "never set on any path, on any
platform" and "permanently take the false branch" at my 2.4. Both retracted.

Two limits I verified rather than assumed, and put in the report as `[I]`:
the codegen consumers read the caps current at compile time
(`spirv_codegen.cpp:2666` reads `params.caps`), so an override has to precede
compilation; and since nothing validates the value, a host can assert a
capability the device was never created with. Escalation 9 rewritten to carry
three answers rather than two.

I also had to fix a second instance of the same absolute: my 2.2 said the
capability set is "permanently empty" for CPU, CUDA, AMDGPU and DX12. Same
override applies. Corrected there, with the point that on that spine such
values would be inert anyway — see entry 40.

## 39. Claim 4. UPHELD for the census. My report made no count; it cited three of thirteen.

The claim as passed to me says the census "across both reports" undercounts and
puts the figure at thirteen. Checking my own file first: my 3.3 names three
sites (`vulkan_device_creator.cpp:577`, `spirv_ir_builder.h:424`,
`spirv_codegen.cpp:2673`) as examples and states no number, so I had no wrong
count to correct — but three of thirteen with no count stated is thin support
for a claim doing that much work in an escalation.

I ran the census myself. Two greps, because four of the thirteen go through a
local variable:

1. `get\((cap::|(taichi::lang::)?DeviceCapability::)[a-z_0-9]+\)\s*(<|>|<=|>=)`
   over `taichi/` and `c_api/` including `.mm` returns **nine**:
   `spirv_ir_builder.cpp:556, 625, 671, 679, 741, 777` (all `< 0x10300`),
   `vulkan_device_creator.cpp:577` (`< 0x10400`), `spirv_ir_builder.h:424`
   (`>= 0x10400`), `spirv_codegen.cpp:1908` (`> 0x10300`).
2. Capability values read into a local: exactly two in the tree,
   `spirv_codegen.cpp:1151` (`bool debug = ...spirv_has_non_semantic_info`, a
   truth test) and `spirv_codegen.cpp:2666`
   (`uint32_t spirv_version = params.caps.get(...)`). The second is compared
   **four** times, at `:2669, 2671, 2673, 2675`.

Nine plus four is thirteen. The four are one construct: a graded ladder at
`:2669-2678` picking `spv_target_env` between `SPV_ENV_VULKAN_1_3`, `_1_2`,
`_1_1_SPIRV_1_4`, `_1_1` and `_1_0`, feeding the optimizer at `:2680`. It is
the strongest single instance of the engine treating a capability level as
ordered, and the one site I had cited, `:2673`, is one rung of it.

I also confirmed a negative worth stating: `spirv_version` is the **only**
capability in the vocabulary ever compared with an ordered operator. A broad
grep for any `DeviceCapability` line carrying a comparison against a hex
literal returns the same nine, all `spirv_version`. Every other capability is
read for truth.

Full census now in report 3.3 as a table, with the ladder described.

## 40. Adjacent to claim 4, and not asked for: my capability-level tally was wrong, so I recounted it.

Both round-two adversaries independently tested the tally at my 3.3 — "29 sites
pass `true`, 10 pass `1`, and 13 pass a `spirv_version` value" — and both got a
different answer. It is a `[V]`-marked count of mine, in the revision whose
stated remedy was to state the scope actually searched, and leaving it standing
while correcting the census beside it would be indefensible. So I recounted.

`grep -rnE "\.set\((taichi::lang::)?DeviceCapability::" --include=*.cpp
--include=*.h --include=*.mm taichi/ c_api/` returns **54** lines: 30 passing
`true`, 10 passing `1` (all `metal_device.mm`), 14 passing a `spirv_version`
value. My 52 was short on two of the three figures.

Five of the 54 are not live device-level writes, and I state them rather than
silently excluding them: two compiled out (`vulkan_device_creator.cpp:826`,
`c_api/src/taichi_vulkan_impl.cpp:48`), two commented out
(`metal_device.mm:1058, 1059`), and one that sets a build target rather than a
device level (`program.cpp:536`, inside `translate_devcaps`). 49 live.

My conclusion is unaffected: every live level is a boolean stored as `true` or
`1`, or a packed `spirv_version`, and `>=` is the correct relation for both
kinds. Only the arithmetic was wrong.

## 41. Claim 5. NOT UPHELD as stated. This report does state it, at 2.2, in bold.

The claim: "neither report states that the CUDA path never populates the device
capability system at all, while the SPIR-V backends do."

That is wrong as to my file. Report 04b lines 221-222 read, marked `[V]`:
"**Nothing in `taichi/rhi/cpu/`, `taichi/rhi/cuda/`, `taichi/rhi/llvm/` or
`taichi/rhi/amdgpu/` sets any capability.**" Lines 226-228 draw the
consequence for CPU, CUDA, AMDGPU and DX12, and line 233 opens
"**This is the central gap for brief section 5.2.**" It has been there since
the first pass. I changed nothing on account of the claim as worded, and I have
re-verified the underlying fact: a `set_caps` grep with no file-type filter
over those four directories plus `taichi/rhi/dx12/` returns nothing.

What the source adversary actually wrote is narrower and is a fair hit: neither
report says **in one place** that item 6.2 divides into two differently shaped
halves along `arch_uses_spirv`, with the brief's named tiers on the half where
nothing exists. That gap is real and I have closed it at new subsection 2.7.

While writing it I checked something neither adversary states and which makes
the gap wider than either had it: the LLVM spine does not **read** capabilities
either. `LLVM::KernelCompiler::compile` takes
`const DeviceCapabilityConfig &device_caps`
(`taichi/codegen/llvm/kernel_compiler.cpp:32`, declared `kernel_compiler.h:22`)
and never uses it in the body at `:30-48`.
`LlvmProgramImpl::make_aot_module_builder` takes
`const DeviceCapabilityConfig &caps`
(`taichi/runtime/program_impls/llvm/llvm_program.cpp:79`, declared
`llvm_program.h:67`) and dispatches on `config->arch` alone at `:80-95`. Those
four lines are every occurrence of `DeviceCapability` or `get_caps()` across
`taichi/codegen/llvm/`, `taichi/runtime/llvm/`, `taichi/runtime/cuda/`,
`taichi/runtime/cpu/`, `taichi/runtime/amdgpu/`, `taichi/runtime/dx12/`,
`taichi/runtime/program_impls/llvm/` and the five RHI directories. The
interface is threaded through the spine and terminates unread.

## 42. Framing of entry 41 against brief 5.2 and section 2. Deliberate.

The instruction with claim 5 was to record it as a gap in coverage and not as a
reason to prefer either path. I have gone one step further than "not a reason
to prefer", because the obvious careless reading of this finding is "the
vendor path is underserved, therefore serve it", and plan section 5.2 says the
opposite of that.

The predicates: `arch_uses_spirv` is `{opengl, gles, vulkan, dx11, metal}` at
`taichi/rhi/arch.cpp:63-66`; `arch_uses_llvm` is
`{x64, arm64, cuda, dx12, amdgpu}` at `:54-57`.

The three cards in brief 5.2 sit on the LLVM spine. Plan 5.2 says explicitly
that this is incidental and defines capability classes only. What is **not**
incidental is that `taichi/rhi/cpu/` is on that same spine, and plan section 2
makes CPU-only operation a first class target rather than a fallback. So the
correct statement of the gap is that the capability system describes the
portable SPIR-V spine and does not describe the spine that carries the
hardware-agnostic baseline. That is a gap in the portable path, recorded as
such at report 2.7 and escalation 8, which I rewrote to lead with the CPU fact
rather than the NVIDIA one.

## 43. What I did not touch, and why.

- The two round-one objections I declined on scope grounds — the `Extension`
  system and the DX12 availability stub — remain declined. An adversary judged
  that declination sound on the reasoning that restating the paired agent's
  findings would destroy the divergence the method depends on. My 8.4 still
  says plainly that this leaves my report unable to stand alone.
- The grep census claim in my 2.2 was tested by both adversaries against all
  four `.mm` files in the tree and holds. Not touched.
- The CMake precedence condition at `taichi/rhi/CMakeLists.txt:17` and the
  absent AOT load handshake were both re-verified exact by both adversaries.
  Not touched.
- Adversary 04-1's own correction to itself, that pass A's GGUI citation of
  `cmake/TaichiCore.cmake:383` is wrong and `:384` is right, confirms the line
  number I kept in entry 35 and refused to change. Nothing to do.
- I opened no new lines of investigation. Everything in entries 36-42 is either
  a claim put to me or a fact needed to state one of those claims correctly.

## 44. Three small `[V]` corrections of mine, swept up in this pass.

Both adversaries raised these and both are right.

- `TI_ARCH_METAL` in `ti_create_runtime`. I gave the range as
  `c_api/src/taichi_core_impl.cpp:294-301`. Line 294 is
  `#endif  // TI_WITH_LLVM`; `#ifdef TI_WITH_METAL` is `:295`, the case body
  `:296-300`, the `#endif` `:301`. Corrected to `:295-301` at report 1.1.
- `spirv_has_atomic_int64`. My 2.3 said the grep "returns four lines" and then
  listed four sites. Case-sensitively it returns **three**:
  `taichi/inc/rhi_constants.inc.h:17`,
  `c_api/include/taichi/cpp/taichi.hpp:1121`,
  `python/taichi/lang/enums.py:27`. The fourth site I listed is real but is a
  different string, `TI_CAPABILITY_SPIRV_HAS_ATOMIC_INT64` at
  `c_api/include/taichi/taichi_core.h:388`, upper case; a case-insensitive
  search also turns up the generated docs mirror at
  `docs/lang/articles/c-api/taichi_core.md:409`. The list was right and the
  description of the search was not. Corrected, with both spellings stated.
- The tally, entry 40.

That is three grep-description or count errors surviving into a third pass, all
of the same shape: a number or a search scope asserted more confidently than
the command behind it supports. It is the same failure mode as the `.mm`
filter in entry 24 and I am recording the pattern rather than only the
instances.

---

# Framing pass. Entries 45-51.

Plan section 5.2 was amended after my amendment pass: the three cards are
incidental test hardware defining FIDELITY classes, not a vendor target;
section 2 governs; the portable path is the reference path. Six claims arrived
with that. As before I verified each against source before acting, and I did
not read the paired agent's files.

## 45. Claim 1. The recommendation in my 4.2. Decision: withdrawn as written, re-argued on source grounds.

The sentence was: "**[I]** For the LLVM family, which is where the brief's
named target cards live, this is where adaptive module loading plausibly
starts, not at `aot::Module::load`."

Two things are wrong with it under the amended 5.2, and they are separable.
First, "plausibly starts" is a recommendation, not a description, and my
standing instruction is to map. Second, its stated premise is the card list,
which now defines fidelity classes and nothing more, so the premise carries no
weight at all.

**Decision: withdraw the sentence, keep the observation, re-argue it.** I
considered outright withdrawal. What stopped me is that the observation
underneath does not depend on the cards and is load-bearing for 6.3: this is
the only place in the tree where Taichi selects a variant of its own compiled
output after the build, and it serves `arch_uses_llvm`, which includes
`taichi/rhi/cpu/` — the CPU-only first class target of section 2, which is a
portable-path fact, not a vendor one. Deleting the whole paragraph would lose
that.

So 4.2 now describes two loci rather than nominating one:

- the `.bc` locus, `get_runtime_fn` (`llvm_context.cpp:209-211`) plus
  `module_from_file` (`:358-362`), serving `arch_uses_llvm` including CPU;
- the AOT locus, `aot::Module::load` (`taichi/aot/module_loader.cpp:31-58`),
  serving `arch_uses_spirv`, with `.spv` per task and a `.tcm` archive
  readable from memory (3.1).

And it states their differing condition — the first has a working parameterised
production and install chain (5.3.1) with no tier in the selector, the second
has a capability channel that is written and never read (3.3) and no arch or
version handshake (3.2) — without ranking them. The one ranking the plan does
supply runs against my withdrawn sentence, and I say so in the report.

## 46. Claim 2. Six card-dependent statements. Found and restated.

`grep -n "NVIDIA\|Maxwell\|Pascal\|Ampere\|GTX\|RTX\|named target cards"` over
my report returned six sites, which matches the claim's "roughly six":

| site | what it argued from the cards | restated to argue from |
|---|---|---|
| 2.2, the central gap | "The target tiers are Maxwell, Pascal and Ampere NVIDIA cards. Those run on the CUDA (LLVM) backend" | the capability system is the only vocabulary for describing a device, and the spine it does not cover carries CPU-only operation, a first class target under section 2 |
| 2.5, the clamp | "GTX 750 is sm_50, GTX 1070 sm_61, RTX 3060 sm_86, so the three named tiers pass through" | a fixed ceiling on a detection path with no way to surface itself; the card positions are demoted to a parenthetical about the test box |
| 2.5, the device pin | "The development box in brief 3.1 has two NVIDIA cards" | multi-device selection exists on the portable path (`taichi_core_impl.cpp:254-255`) and on no other; the box is a testing consequence |
| 4.2 | entry 45 | entry 45 |
| escalation 10 | "whether the two named target cards must be independently addressable" | whether an install must address more than one device at all |
| escalation 11 | "RTX 3060 is sm_86 so the brief's stated upper tier is unaffected" | the clamp is where upward scaling stops, whatever hardware is present |

Escalation 8 was already restated this way in the amendment pass (entry 42), so
it did not need touching again.

Worth recording because it cuts the other way twice. On device selection the
**portable path is the better served one**: `set_vulkan_visible_device` is
honoured at `c_api/src/taichi_core_impl.cpp:254-255`, and every other arch
rejects a non-zero index at `:270, 277, 283, 289, 297` — I checked all five.
Plan 5.2's consequence clause is about a vendor path being better served than
the portable one, which is not the case here, so there is no gap in the
portable path to record. I state the direction rather than silently reusing the
gap framing.

## 47. Claim 3. Re-weighting upheld, with a limit I verified myself.

The claim: the switched-off 64-bit pointer machinery was weighted as a side
axis because it sits on the portable backends, and that is backwards, because
those backends are the reference path.

I accept it. My 2.4 called it "the SPIR-V side" of 6.2 and my 2.7 table implied
the LLVM spine was where the item really lived, and both readings came from the
card list. Corrected at 2.4, at the 2.7 table, and in escalation 9. Nothing
about the source changed; only which spine the plan measures against.

The claim also carried a scope note from a separate investigation — that this
machinery gates ndarray, argpack and return-struct addressing and does not
reach SNode addressing — with an instruction not to overstate in either
direction. I re-derived it rather than accepting it, by finding the enclosing
function of every gate:

- `taichi/codegen/spirv/spirv_codegen.cpp:783` sits in
  `void visit(ExternalPtrStmt *stmt)` opened at `:734`; the true branch loads a
  `u64` address from the args buffer and does 64-bit arithmetic at `:786-792`.
- `:2340` sits in `compile_args_struct` opened at `:2326`.
- `:2416` sits in `compile_argpack_struct` opened at `:2402`.
- `:2491` sits in `compile_ret_struct` opened at `:2483`.
- `taichi/runtime/gfx/runtime.cpp:95-96` gates taking a physical pointer for an
  ndarray device allocation.
- `taichi/runtime/program_impls/gfx/gfx_program.h:79-83` is the argument data
  layout string.

And the negative half: SNode and root-buffer access in SPIR-V goes through
`BufferType::Root` bound buffers — `visit(GetRootStmt *)` at
`spirv_codegen.cpp:351`, `visit(GetChStmt *)` at `:359`,
`visit(SNodeLookupStmt *)` at `:468` — and none of the twelve consumers is in
that path.

So both are true and I have written both: real 64-bit pointer machinery on the
reference path, covering argument and return addressing rather than the SNode
addressing item 6.2 is about. The consequence I added to 2.7 is that 6.2 has
three shapes, not two, because the SNode half is unbuilt on **both** spines.

## 48. Claim 4. NEW FINDING, verified, and stronger than claimed in one place.

Vulkan, all four sub-claims hold:

- **Memory properties, used only for handle types.**
  `VulkanDevice::create_vma_allocator` calls
  `vkGetPhysicalDeviceMemoryProperties` at
  `taichi/rhi/vulkan/vulkan_device.cpp:2515-2516` and reads only
  `memoryTypeCount` and `memoryTypes[i].propertyFlags` at `:2518-2532`, to fill
  a vector of `VkExternalMemoryHandleTypeFlags`. `grep memoryHeap` across
  `taichi/rhi/vulkan/` returns **nothing**, so heap sizes are never read. The
  one call that would answer "how much memory" is made and the answer is used
  for something else.
- **Full properties stored, two fields consumed.** `vk_device_properties_` at
  `vulkan_device.h:739`, accessor `:710-712`, filled at `vulkan_device.cpp:1596`.
  Grep for the member and the accessor across `taichi/` and `c_api/` returns
  exactly two consumers: `limits.maxComputeWorkGroupCount[0..2]` in
  `VulkanCommandList::dispatch` at `:1125-1129`, and `limits.timestampPeriod`
  at `:1945`.
- **Device name to a debug log only.** `deviceName` at
  `vulkan_device_creator.cpp:512` inside an `RHI_DEBUG_SNPRINTF` /
  `RHI_LOG_DEBUG` block spanning `:507-518`, and at `:438` in the enumeration
  log.
- **Not claimed, and I found it while checking the rest:** `get_device_score`
  at `vulkan_device_creator.cpp:203-229` scores devices by
  `properties.deviceType`, +500 integrated and +1000 discrete at `:220-225`,
  plus 100 per API minor at `:226`. Called at `:452` and `:462`. The score is a
  local and dies at the call site. That is a hardware-class judgement being
  made on the reference path today and thrown away.

OpenGL, and here the claim understates:
`taichi/rhi/opengl/opengl_api.cpp:20-21` defines `opengl_max_block_dim = 1024`
and `opengl_max_grid_dim = 1024`, with the comment at `:18-19` saying these are
the specification minima "in case glGetIntegerv didn't work properly" — the
hardcoded fallback the claim describes. They are filled at `:231` and `:234`.
The claim was that neither reaches the capability map or the C API. In fact a
grep for both names over `taichi/` and `c_api/` returns six lines total: two
definitions, two fills, two `TI_TRACE` prints. **No header declares them and
nothing anywhere consumes them.** They are written and never read.

Recorded at new subsection 2.8. The claim's framing — both paths detect tier
data, neither publishes it, and that is a smaller and better-shaped piece of
work than inventing a vendor vocabulary — is the planner's judgement to make,
so it went to escalation 19 rather than into the findings as a recommendation.

## 49. Claim 5. Metal. Verified, and it is the cleanest model in the tree.

`collect_metal_device_caps` at `taichi/rhi/metal/metal_device.mm:1017-1070`:

- reads a vendor hardware-class ladder at `:1027-1036`,
  `[mtl_device supportsFamily:]` against `MTLGPUFamilyMac2` and Apple families
  3 through 7, each rung OR-ing in the one above so the ladder is monotone;
- derives five named features at `:1038-1042`
  (`feature_64_bit_integer_math = family_apple3`, floating point atomics, quad
  and simd scoped permutes, simd scoped reductions);
- writes **only portable vocabulary**: `spirv_version` and
  `spirv_has_int8/int16/float16/subgroup_basic` unconditionally at `:1044-1049`,
  `spirv_has_int64` under the 64-bit feature at `:1051-1053`, subgroup vote and
  ballot at `:1063-1064`, subgroup arithmetic at `:1067`.

The architectural point, which is why it is the model: it adds **no** Metal
entry to `taichi/inc/rhi_constants.inc.h`. A vendor tier ladder is read and its
consequences are expressed in the shared vocabulary, so generic code consumes
them without knowing about Metal families. That is the shape a CUDA or CPU tier
answer could take without extending the vocabulary per vendor, which is what
escalation 8 was worried about. Folded into 2.8.

## 50. Claim 6. Verified, and the defect is larger than stated, with a scope limit that must be stated too.

The claim: the C API asserts 64-bit integer support on OpenGL unconditionally,
discarding the check that protects low-end devices.

Larger than stated: `OpenglRuntime::OpenglRuntime`
(`c_api/src/taichi_opengl_impl.cpp:4-13`) constructs its `GLDevice` member
first, so `GLDevice::GLDevice` (`taichi/rhi/opengl/opengl_device.cpp:506-529`)
has already run detection and called `set_caps` at `:529`. The C API then
builds a fresh three-entry config at `:8-11` and calls
`get_gl().set_caps(std::move(caps))` at `:12`, **replacing the whole map**. So
besides asserting `spirv_has_int64` and `spirv_has_float64` unconditionally, it
drops the detected `spirv_has_int16` and `spirv_has_float16` that
`opengl_device.cpp:515-526` sets from `GL_NV_gpu_shader5`,
`GL_AMD_gpu_shader_int16` and `GL_AMD_gpu_shader_half_float`.

Scope limit, which I record so this is not overstated. `GLDevice::GLDevice`
calls `initialize_opengl(false, true)` at `:507`, asking for desktop GL, and
`ti_create_runtime` has no `TI_ARCH_GLES` case
(`c_api/src/taichi_core_impl.cpp:251-305`). But `initialize_opengl` returns
early once it has run (`opengl_api.cpp:46-54`) and `kUseGles` is assigned once
at `:239`, so in a process where a GLES context was established first —
`Program(Arch::gles)` reaches `opengl::initialize_opengl(true)` at
`taichi/program/program.cpp:126` — `is_gles()` is true, `GLDevice` correctly
withholds 64-bit, and `OpenglRuntime` asserts it anyway. The defect is latent
rather than always live, and what makes it latent is initialisation ordering
rather than any check. I also checked `set_gles_override`
(`opengl_api.cpp:35-37`): declared and defined, no caller anywhere in
`taichi/` or `c_api/`.

Weighed against sections 2 and 5.3 as instructed: GLES is the profile the low
end runs, the detection at `opengl_device.cpp:509-513` with its comment "64bit
isn't supported in ES profile" is exactly the protection those sections ask
for, and the C boundary discards it. This is a gap in the portable path in the
same sense as 2.7 — the reference path is where the protection was written and
where it is being lost. New subsection 2.9, escalation 20.

## 51. What I did not do in this pass.

- I did not change any finding that was already argued from source. Only the
  six card-dependent statements in entry 46 were restated, and none of their
  conclusions moved.
- I did not touch the declined scope items, the paired agent's files, or any
  source file.
- I did not resolve any of the six claims into a recommendation. The three that
  invite one — which locus 6.3 starts at, what an install-time loader should be
  able to ask, and whether the C API OpenGL override is a defect — went to
  escalations 17, 19 and 20 unresolved.
- Pattern worth recording alongside entry 44. The three grep-description errors
  there were about confidence exceeding the command. This pass found a
  different failure: a conclusion that was factually sound but reached through
  a premise the brief had labelled incidental. Both of my passes before this
  one carried it, and it survived two adversarial rounds because every
  individual fact in the sentence was true. Verifying facts does not catch it;
  only checking what the conclusion rests on does.

---

# Round-three pass, entries 52-59

Working against `PROJECT-PLAN.md` as it stands on 2026-09-09. I re-read the
plan in full before touching the report, per standing instruction 6, and two
sections in it are new to me: 2.2a and 5.1a. Neither existed when any earlier
pass of this report was written, and between them they decide four of the items
put to me.

## 52. The instruction I was given is narrower than the plan, and I say so.

I was asked to remove the "reference path" / "vendor path" / "accelerant"
framing. Before doing that I checked the plan for the phrases themselves.
`grep -n 'reference path\|accelerant' PROJECT-PLAN.md` returns nothing.
`grep -n 'vendor path' PROJECT-PLAN.md` returns nothing. `grep -n 'portable
path'` returns exactly one line, `:481`, and it is inside the 2026-09-09
CORRECTION in section 5.2, which reads: an earlier version of this section
opposed "the portable path" to "a vendor-specific path", and "That is the wrong
axis".

So the axis the plan withdraws is not only the two phrases I was named. It is
the opposition itself, whatever words carry it. My report used "the portable
path" against "the vendor path" to mean the same thing as "reference path"
against "accelerant", in twelve places the adversaries' phrase greps could not
see, because both adversaries grepped only the two named phrases. Standing
instruction 9's corollary is the exact shape of that: a count is only as
complete as the thing that generated the list.

My own enumeration, run on the file as it stood before I edited it:

    grep -n "reference path\|accelerant\|vendor path\|portable path\|vendor half" \
      report-04b-backend-build.md | cut -d: -f1 | sort -n | uniq | wc -l
    -> 37

Decomposed: 20 lines carrying "reference path" (42, 48, 359, 379, 500, 515,
522, 530, 621, 640, 956, 1433, 1656, 1868, 1871, 1875, 1883, 1953, 1959, 1989);
5 carrying "accelerant" (360, 417, 430, 501, 1893); 13 carrying "vendor path"
or "portable path" (252, 360, 424, 432, 505, 528, 562, 639, 1653, 1839, 1878,
1887, 1958), of which 360 is already counted for "accelerant". 20 + 5 + 12 =
37. Adversary 3 pass 1 counted 25 in this file and 7 in pass A's, totalling 32
across the pair; its 25 for mine reconciles exactly with my first two groups.
The extra 12 are the ones neither adversary's grep could see.

I removed all 37 and said in the report that the instruction was narrower than
the plan and why. Where a withdrawn sentence is quoted inside a retraction the
phrase survives in quotation marks; that is deliberate, it is how every earlier
correction in this report is handled, and a final grep confirms every remaining
occurrence sits inside a retraction, a change log or a quotation of the plan's
own correction.

## 53. What the framing actually cost, as opposed to what it looked like.

Nothing factual. I checked each of the 37 before rewriting it and no census,
count, line number or `[V]` claim depends on the axis. What depended on it was
a RANKING between the two halves of item 6.2, and the ranking appears in four
places: 2.4, 2.7's "Ordering" paragraph, escalation 9's re-weighting, and 4.2's
closing bullet.

Plan section 2.2a settles the point directly and it is worth quoting because it
is stronger than "do not rank": "Item 6.2, 64-bit addressing, in full. This
includes the shelved SPIR-V path and the `at_buffer` pointer-width collision.
... The two halves are not separable." So a ranking is not merely unsupported,
it is contrary to a settled decision. My escalation 18 previously asked whether
6.2 is one work item or two; that question is closed by 2.2a and I narrowed the
item to the sizing observation that survives it, which is that one plan item
covers three differently shaped pieces of work.

The worst instance is 4.2, and it is worse in kind than the others. It did not
merely use the axis; it said "the ranking that plan 5.2 does supply". That is a
claim about the governing document, and the document does not contain it.
Standing instruction 7 says a verified citation does not verify the claim
attached to it. This is that failure one level up: a section cited in support
of a proposition it does not carry. I have said so in place rather than quietly
deleting the sentence.

Direction, recorded because it explains why two adversarial rounds missed it.
My first pass leaned toward CUDA on the strength of the owner's cards. The
framing pass inverted the lean and pointed it the other way. Both are the same
error and the second was harder to see, because it looked like the correction
of the first.

## 54. GLES. My own notes contain the false [V] that caused this.

Claim put to me: my escalation resting on GLES being unreachable through the C
API is false. Verified, and it is worse than an omission — the falsifying
caller is one grep away and I ran a grep that should have found it.

Notes entry 50 says, in my own words: "I also checked `set_gles_override`
(`opengl_api.cpp:35-37`): declared and defined, no caller anywhere in `taichi/`
or `c_api/`."

    grep -rn "set_gles_override" taichi/ c_api/
    taichi/rhi/opengl/opengl_api.h:10        declaration
    taichi/rhi/opengl/opengl_api.h:11        (unset_)
    taichi/rhi/opengl/opengl_api.cpp:35      definition
    taichi/rhi/opengl/opengl_api.cpp:39      (unset_)
    taichi/rhi/opengl/opengl_api.cpp:58      unset_, inside initialize_opengl
    taichi/rhi/opengl/opengl_api.cpp:257     unset_, inside reset_opengl
    c_api/src/taichi_opengl_impl.cpp:26      THE CALLER

Seven lines and one of them is the caller. I cannot reconstruct what I ran that
returned nothing; what I can say is that the sentence was marked as checked and
was not.

The chain, opened step by step this pass:

- `c_api/include/taichi/taichi_opengl.h:33-36` declares
  `TI_DLL_EXPORT TiRuntime TI_API_CALL ti_import_opengl_runtime(
  TiOpenglRuntimeInteropInfo *interop_info, bool use_gles);`. Public, exported,
  documented, with the GLES flag right there in the signature.
- `c_api/src/taichi_opengl_impl.cpp:21-29` is the body. `:26`
  `set_gles_override(use_gles);` then `:28`
  `return ti_create_runtime(TI_ARCH_OPENGL, 0);`.
- `ti_create_runtime` `TI_ARCH_OPENGL` at `taichi_core_impl.cpp:269-273`
  constructs `OpenglRuntime`, whose `GLDevice` member calls
  `initialize_opengl(false, true)` at `opengl_device.cpp:507`.
- `opengl_api.cpp:56-59` replaces that `false` with the override.
- `:239` sets `kUseGles = use_gles`.
- `opengl_device.cpp:509-513` then correctly withholds int64 and float64,
  comment `:510` "64bit isn't supported in ES profile".
- `taichi_opengl_impl.cpp:8-12` installs a fresh three-entry config over the
  top, asserting both.

So the OpenGL runtime asserts 64-bit integer and float support over a device
its own detection has just refused it on, and the route is a documented public
function called with a documented argument. The "latent rather than always
live" framing in my 2.9 and escalation 20 is retracted.

The converse, which I checked while I was in the file because the ordering cuts
both ways. The override is consumed at `:56-59`, which is AFTER the early
return at `:46-54`. `ti_get_available_archs` probes OpenGL at
`taichi_core_impl.cpp:193` via `is_opengl_available` (`:21-27`) via
`is_opengl_api_available()` whose default is `use_gles = false`
(`opengl_api.h:13`) forwarding to `initialize_opengl(use_gles, true)`
(`opengl_api.cpp:244-246`). So enumerate-then-import loses the `use_gles`
argument silently, and the stale override is not even cleared, because
`unset_gles_override` is only reached at `:58` past the early return and at
`:257` inside `reset_opengl`.

Which of the two fires depends on whether an unrelated API call was made
earlier in the process. That is the composed consequence and I marked it `[I]`:
I read the paths, I did not run them. Every link in them is `[V]`.

Direction against the plan, stated because it inverts my escalation: plan
sections 2 and 5.3 put the low end first and GLES is the profile the low end
presents. The RHI declines a capability the hardware lacks and the C boundary
claims it anyway, which engages plan section 2.2 in the harder direction —
hardware being told it can do something it cannot.

Adjacent `[V]` correction found in the same file: I wrote that `kUseGles` "is
set once at `:239`". Two writers, `:239` and `:254` in `reset_opengl`, whose
only caller is `taichi/runtime/program_impls/opengl/opengl_program.cpp:29`. The
in-file comment at `:23` says "set at most once in initialize_opengl below",
which is true of the function and not of the variable, and I took the comment's
scope for the variable's.

## 55. TI_VISIBLE_DEVICE. Verified, and the ordering is the sharp half.

Claims 3 and 4 put to me, both verified.

`taichi/rhi/vulkan/vulkan_loader.cpp:111-113`, at the end of
`VulkanLoader::init`, reads `TI_VISIBLE_DEVICE` from the environment and calls
`set_vulkan_visible_device(id)`. Consumed at `vulkan_device_creator.cpp:442-456`
— `std::stoi` `:445`, range check `:446`, error log `:448-451`, admission
`:452-455` subject to `get_device_score`, fallback `:458-468`.

Process-global: `set_vulkan_visible_device` (`vulkan_loader.cpp:144-146`)
writes `VulkanLoader::instance().visible_device_id`; `instance()` is a
function-local static (`vulkan_loader.h:14-17`) and the field is a plain public
member at `:32`.

Ordering, which is the part I had to trace rather than read off:

    ti_create_runtime TI_ARCH_VULKAN      taichi_core_impl.cpp:253
      set_vulkan_visible_device(index)    :254-255   <- caller's argument
      new VulkanRuntimeOwned              :256-262
        VulkanDeviceCreator member        taichi_vulkan_impl.h:42-43
          VulkanLoader::instance().init() vulkan_device_creator.cpp:236
            std::call_once body           vulkan_loader.cpp:87-115
              getenv TI_VISIBLE_DEVICE    :111-113   <- overwrites it

So on the FIRST Vulkan runtime in a process the environment wins; on later ones
the `call_once` has already fired and the argument stands. Whether `init()` ran
earlier decides which, and `ti_get_available_archs` runs it at
`taichi_core_impl.cpp:178` through `is_vulkan_api_available`
(`vulkan_loader.cpp:140-142`).

Why it belongs in this territory rather than being a curiosity: plan section
5.1a puts the configuration agent OUTSIDE the process. An environment variable
read at loader init is exactly the channel such an agent can use with no C API
call. It is the second one in my territory; the first is `TI_LIB_DIR` at
`taichi/util/lang_util.cpp:35-45`, which I already cite for module location. So
my 2.5 described device selection as one channel and there are two.

Scope, and I am deliberate about this. Plan section 5.1a says concurrent
operation across two cards is confirmed empirically, outranks code reading, and
needs no agent investigation. I am not investigating it and this finding does
not bear on it: it is about which of two inputs names the device for one
runtime, not about whether two runtimes can run at once. Recorded in the report
in those words.

## 56. The downgrade of multi-device selection. Withdrawn, and I record what I do not settle.

Claim 5. My 2.5 closed "Practical consequence for testing, not for
architecture", and my 8.1c added "on device selection the portable path is the
one better served, so there is no gap in it to record". Both go.

Against plan section 5.1a: "Where a machine holds several GPUs, the agent may
need to choose between them, or to use more than one." And: "The development
box in section 3.1 holds a GTX 750 and a GTX 1070, so this is the ordinary case
rather than an edge one." Choosing between cards at install time is the settled
model, so device selection is architecture. And against plan section 5.2: a
better-served spine is "a gap in the less-served spine to record, never
evidence that it matters less", so "no gap to record" is exactly the inference
the section forbids.

One thing I do NOT settle, and I record both readings in escalation 10 rather
than picking. The two adversaries disagree. Pass 2 says counting `x64` and
`arm64` in the gap is over-reach because there is no second host CPU to name; I
checked and a grep for `device_index`, `device_get_count` and `num_devices`
across `taichi/rhi/cpu/` and `taichi/rhi/llvm/` returns nothing, so the source
supports the premise. Pass 1 says CPU coverage is what lets the finding stand
enum-wide without the owner's card list, which is also true. The source settles
that there is no CPU device index; it does not settle whether a refusal with
nothing to refuse is a gap. That is a judgement and it is not mine.

Also folded in, from pass 1 and re-run by me: `driver_.device_get(&device_, 0)`
(`cuda_context.cpp:22`) pins an ORDINAL, and
`grep -rn CUDA_VISIBLE_DEVICES taichi/ c_api/ python/` returns nothing, so the
driver's own remapping decides which physical card ordinal 0 is. That is what
makes the finding consistent with the concurrency plan section 5.1a records as
observed, and without it the finding reads as a challenge to an observation the
plan says outranks code reading. Severity is one process one card.

## 57. Section 5.1a, and the positive result nobody stated.

Three of my escalations — 2, 4 and 17 — ask mechanism questions that section
5.1a answers. I had never seen the section; `grep -n '5\.1a'` over my report and
notes as they stood returns nothing.

What it settles, quoted: "Baking the constant into per-architecture bitcode at
build time is the CORRECT shape. It was never a defect to work around." "No
runtime mutability of the SNode ceiling is required, or wanted." "Per build
already means per device configuration." "The filename scheme does not need to
grow, and no common denominator has to be found."

The positive statement, which I derived from machinery I had already verified
rather than from new reading, and which is new to this territory:

- `runtime_module/CMakeLists.txt:29-31` loops archs and calls
  `compile_llvm_runtime(${arch})`. Inside the function the clang line at `:8`
  is identical on every iteration except the `${rtm_arch}` substitutions, so
  the only things that vary are the `ARCH_` token and the output filename.
- `runtime.cpp:25` includes `taichi/inc/constants.h`, resolved through the one
  `-I ${PROJECT_SOURCE_DIR}` on that same line. So every artefact of one
  configure compiles against the same header in the same tree, and therefore
  carries the same ceiling.
- The host's only use of the constant is `struct_llvm.cpp:266`, compiled into
  `taichi_core` in the same configure, and `:12` makes each
  `generate_llvm_runtime_${rtm_arch}` target a dependency of
  `${CORE_LIBRARY_NAME}`.

So under section 5.1a's model — host and bitcode configured at the same moment
— host and every variant agree by construction, with `get_runtime_fn`
(`llvm_context.cpp:209-211`) untouched. My 5.2 recorded the constraint that a
second parameter either grows the filename scheme or fixes one ceiling per
build. Section 5.1a chooses the second, and the second is what the build
already does. That is now 5.3.3 in the report.

I marked the "agree by construction" step `[I]`, because it follows from the
deployment model the plan states rather than from anything the build enforces:
nothing stops someone installing a host binary and bitcode from two different
configures.

Escalation 4 is the one I withdrew furthest, and I was careful not to delete a
true sentence to tidy the bookkeeping, per standing instruction 9's last
paragraph. Two facts recorded under it are not touched by 5.1a and stay as
observations: the wheel-tag mechanism cannot encode a non-boolean option
(`ti_build/cmake.py:84`), and multiplied device modules land in the source tree
under `runtime_module/CMakeLists.txt:10`.

## 58. Metal's content, and the two citation slips.

Claim 7, both halves verified.

The caveat I did not carry. My 2.8 called Metal "the exception, and it is the
model" and drew only the positive lesson. What it publishes is admission:
`metal_device.mm:1038` is `bool feature_64_bit_integer_math = family_apple3;`
and `:1051-1053` sets `spirv_has_int64` only under it. Below Apple 3 the key is
absent; `DeviceCapabilityConfig::get` returns 0 for a missing key
(`device_capability.cpp:33-39`); the consumers at
`spirv_ir_builder.cpp:64, 166, 312, 326` and the MSL version choice at
`metal_device.mm:131-135` take the no-64-bit branch. So a hardware generation
decides what the system can express, which is the shape plan section 2.2 calls
a violation. Plan section 6.3 now carries exactly this distinction as a
finding, and my report did not, so only one half of the pair was testing it.

I did NOT decide what follows. Plan section 2.2 says it is "a TEST to apply to
every proposed change", and Metal's ladder is inherited upstream code rather
than a proposed change. Whether the test reaches inherited behaviour is not
stated anywhere in the plan. New escalation 23 records the question.

Citation slips, both mine, both in `[V]` sentences:

1. The optimizer line. I re-numbered `spirv_codegen.cpp:2660-2681`: `:2666`
   reads `spirv_version`, `:2669-2679` is the ladder, `:2680` is blank, `:2681`
   is `spirv_opt_ = std::make_unique<spvtools::Optimizer>(target_env);`. I had
   `:2680` in 3.3 and again in the section 6 map. Both corrected.
2. `get_device_score`. I wrote "the score is a local that dies there". True at
   `:462`, where the ranking loop keeps only the winner. False at `:452`, which
   is `else if (get_device_score(devices[id], test_surface))` and uses the
   score as a boolean admission gate on an explicitly requested device. A
   zero-scoring named device is dropped and the automatic pick runs, with no
   log on that branch; the only error log at `:448-451` covers out-of-range
   only. Narrowed in 2.8, and the question it raises under section 5.1a — an
   install-time agent naming a device should get that device or a failure, not
   a silent substitution — is new escalation 22.

## 59. What I did not do in this pass.

- I edited only `report-04b-backend-build.md` and appended here. No source
  file, no other agent's file, nothing in `PROJECT-PLAN.md`.
- I did not decide anything is unnecessary and I did not delete a withdrawn
  claim. Every retraction sits above the wording it replaces, which is how this
  report has handled corrections since the first revision.
- I did not resolve any of the new questions. Escalations 21, 22 and 23 record
  them; escalations 4 and 17 are narrowed against a plan section rather than
  closed by me, because no agent may enter a plan decision. Where I say a
  section 5.1a decision closes a question, I am reporting what the plan says,
  not making the decision.
- I did not adopt either adversary's position on whether the CPU refusal is a
  gap. Both readings are consistent with the source and the source does not
  choose.
- I did not re-derive the paired agent's findings and I am not restating them
  as mine. The `Extension` system and the DX12 availability stub remain
  declined, as recorded in 8.4.
- Pattern worth recording alongside entries 44 and 51. Entry 44's failures were
  confidence exceeding the command. Entry 51's was a conclusion resting on a
  premise the plan had labelled incidental. This pass found a third and it is
  the most durable: a conclusion resting on a premise the plan had CHANGED
  under me, correct when written and wrong when read. Nothing about verifying
  facts catches that. Only re-reading the governing document at the moment of
  finalising does, which is what standing instruction 6 asks for and what I had
  not done.
