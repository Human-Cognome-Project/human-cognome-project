# Adversary 04-1 — backend, AOT and build architecture

Judging `report-04-backend-build.md` (pass A) and `report-04b-backend-build.md`
(pass B) against the source. Notes consulted where the reports disagreed or a
claim looked unsupported.

Every file:line below I opened myself. Where I correct a report, the correction
carries its own citation.

---

## 1. Verdicts up front

| Question | Verdict |
|---|---|
| Is pass A correct? | Yes, with two substantive corrections and eight citation slips. One claim is false in a way that would misdirect 6.2 work. |
| Is pass B correct? | Yes, with two substantive corrections and eight citation slips. One claim is false in a way that would misdirect 6.3 work. |
| Is pass A complete? | No. Misses the single most 6.2-relevant fact in the territory. |
| Is pass B complete? | No. Misses three areas the brief specifically asks about. |
| Is the **pair** complete? | Close. Five gaps survive both. |
| Major revision required? | **No.** A correction pass, not a re-run. Neither report is structurally wrong. |
| Were the passes independent? | **Yes.** The disclosed contamination is immaterial and the evidence for independence is strong. See section 7. |

Both reports are unusually well cited. I opened roughly ninety of their
file:line references and the great majority land exactly. That is the reason
the errors below are worth naming individually: the density of correct
citations makes the wrong ones dangerous, because a reader will not spot-check.

---

## 2. Where the two reports contradict each other, resolved

### 2.1 Does the Metal backend call `set_caps`? — **A is right, B is wrong**

B, section 2.2: "Metal builds its config at `taichi/rhi/metal/metal_device.mm:1044-1070`
and returns it rather than calling `set_caps`." B's table of `set_caps` sites
therefore has no Metal row.

Source: `taichi/rhi/metal/metal_device.mm:1162-1169`:

```
MetalDevice::MetalDevice(MTLDevice_id mtl_device) : mtl_device_(mtl_device) {
  ...
  DeviceCapabilityConfig caps = collect_metal_device_caps(mtl_device);
  set_caps(std::move(caps));
}
```

`set_caps` is called at `metal_device.mm:1168`. `collect_metal_device_caps` is
the function at `:1017-1070`; returning a config and calling `set_caps` on the
result are not alternatives, and B treated them as such. A's table row —
"Metal | `metal/metal_device.mm:1168`, built by `collect_metal_device_caps`
from :1017 | yes | yes, :1052, gated on `feature_64_bit_integer_math`" — is
correct in every column.

B contradicts itself here: B's own section 2.3 lists Metal's
`spirv_has_int64` producer at `metal_device.mm:1051-1053` and its consumer at
`metal_device.mm:132`, which only works if the caps reach the device. The
consumer at `:132` reads `device.get_caps()` (`metal_device.mm:119`), i.e. the
member `set_caps` writes.

### 2.2 How many `set_caps` call sites are there? — **both undercount; B's "exhaustive" is falsified**

B, section 2.2: "`set_caps` is called from exactly seven places (exhaustive
grep over `taichi/` and `c_api/`)".

Source, grep over `taichi/` and `c_api/` excluding the declaration at
`taichi/rhi/public_device.h:855`, gives **eight**:

| # | site |
|---|---|
| 1 | `taichi/rhi/vulkan/vulkan_device_creator.cpp:876` |
| 2 | `taichi/rhi/vulkan/vulkan_device.cpp:1580` |
| 3 | `taichi/rhi/metal/metal_device.mm:1168` |
| 4 | `taichi/rhi/opengl/opengl_device.cpp:529` |
| 5 | `taichi/rhi/dx/dx_device.cpp:565` |
| 6 | `c_api/src/taichi_opengl_impl.cpp:12` |
| 7 | `c_api/src/taichi_vulkan_impl.cpp:53` |
| 8 | `c_api/src/taichi_core_impl.cpp:331` |

B has 1, 2, 4, 5, 6, 7, 8 and is missing 3. A's scope was narrower and stated
as such ("Grepped all of `taichi/rhi/`"); within that scope A has 1-5 and is
complete. A's report does cite site 8 separately (§3.3) but never mentions 6
or 7, and A's §6 map of "per-backend capability population" omits all three
c_api producers.

Neither report's number is right. The correct statement is eight sites, five
in the RHI and three in the C API.

### 2.3 `cmake/TaichiCore.cmake:111` — **A is right on substance, B overstates**

B, section 7: the misspelled `TAIHI_CORE_SOURCE` at line 111 means
"`taichi/runtime/amdgpu/runtime.cpp` is never added to `TAICHI_CORE_SOURCE`.
Line 105 spells it correctly for the CUDA equivalent."

A, section 6.2: both the CUDA glob at `:104` and the AMDGPU glob at `:110`
name files that do not exist, so both globs are empty and the typo is
harmless today.

Source. `ls taichi/runtime/cuda/` returns `CMakeLists.txt`, `jit_cuda.cpp`,
`jit_cuda.h`, `kernel_launcher.cpp`, `kernel_launcher.h`. `ls
taichi/runtime/amdgpu/` returns `CMakeLists.txt`, `jit_amdgpu.cpp`,
`jit_amdgpu.h`, `kernel_launcher.cpp`, `kernel_launcher.h`. Neither
`runtime.cpp` exists. A verified this; B did not, and B's phrasing implies a
live defect where there is a dormant one. A also correctly reports that the
correct spelling appears at `TaichiCore.cmake:77, 105, 137, 340` and the
misspelling nowhere else. I confirmed 77, 105 and 137 directly.

### 2.4 How many archs are in `archs.inc.h`? — **B is right**

`taichi/inc/archs.inc.h:4-17` contains **12** `PER_ARCH` entries: x64, arm64,
js, cuda, metal, opengl, dx11, dx12, opencl, amdgpu, vulkan, gles.

A says "Thirteen entries" (§3.1) and then lists twelve names. The same wrong
count propagates to §3.3 ("`TiArch` ... has **eight** cases against
`archs.inc.h`'s thirteen") and to escalation 9 ("`TiArch` covers 8 of the 13
archs"). The correct statement: `TiArch`
(`c_api/include/taichi/taichi_core.h:358-375`) has 8 enumerators of which one
is `TI_ARCH_RESERVED = 0`, so it names **7 of 12** archs; the 5 with no
`TiArch` value are js, dx11, dx12, opencl and amdgpu — which is A's own list,
correct. B has 12 and the same missing-five list (§1.2).

### 2.5 How many files in `taichi/aot/`? — **B is right**

Seven files, 836 lines. A says "six files, 836 lines" (§5) and then
enumerates seven that sum to 836. The miscount originates in A's own notes,
entry 1 ("Six sources only"). B says seven (§3.4). No consequence beyond the
number itself.

### 2.6 Non-boolean CMake variables — **both incomplete, in opposite directions**

A, §2.4: "There is no project-defined non-boolean CMake cache variable
controlling the build of Taichi itself", listing `CMAKE_BUILD_TYPE`
(`CMakeLists.txt:41`), the `CACHE BOOL` forcings at `cmake/TaichiTests.cmake:7`,
`cmake/TaichiCAPITests.cmake:7` and `taichi/rhi/CMakeLists.txt:18-23`, and the
`CACHE PATH` at `c_api/cmake/FindTaichi.cmake:83`.

B, §5.4: "The only non-boolean cache entries are `CMAKE_BUILD_TYPE`
(`CMakeLists.txt:41`) and the internal `HOST_ARCH`
(`cmake/TaichiCXXFlags.cmake:149`)."

Source: `grep -rn CACHE cmake/*.cmake CMakeLists.txt c_api/cmake/*.cmake
taichi/rhi/CMakeLists.txt` returns exactly the union of the two lists.
`cmake/TaichiCXXFlags.cmake:149` is `set(HOST_ARCH ${ARCH} CACHE INTERNAL
"Host arch")` — a non-boolean cache entry that A missed, and one that does
control the build: it is the first element of the `foreach` at
`taichi/runtime/llvm/runtime_module/CMakeLists.txt:29` that decides which
`runtime_<arch>.bc` files are produced. `c_api/cmake/FindTaichi.cmake:83` is a
`CACHE PATH` that B missed, in a directory B says it checked.

A's absolute claim is falsified by `HOST_ARCH`. B's absolute claim is falsified
by `FindTaichi.cmake:83`. Both are also falsified by a bigger omission — see
section 5, item S2.

I also verified A's count of 24 `option()` calls, all boolean. That is exact.

### 2.7 Both cite `runtime_lib_dir` — **B's range is right**

`taichi/util/lang_util.cpp:30-47`. A cites `:30-46`, clipping the closing
brace. Trivial, recorded for the file.

### 2.8 The `-D` propagation list — **B is complete, A is not**

A, §2.1, lists the appends to `CMAKE_CXX_FLAGS` at `TaichiCore.cmake:103, 109,
115, 119, 123, 127, 131`. Verified correct, but two are missing: LLVM at
`:94` and CUDA_TOOLKIT at `:281`. B lists all nine (§1.3) and additionally
notes that GGUI alone uses `target_compile_definitions` — which is at
`TaichiCore.cmake:383`, not `:384` as B says.

Same pattern in the `#else` arms: A lists `program.cpp:90, 101, 108, 115, 122,
129` and misses `:94` (`TI_ERROR("This taichi is not compiled with LLVM")`).
B lists all seven.

---

## 3. Errors in pass A that neither the other report nor A itself caught

### A-1. `Extension::data64` is **not** a compile-time blocker. This is A's one materially wrong conclusion.

A, §4.1: "So on the whole portable SPIR-V spine — vulkan, opengl, gles, metal,
dx11 — 64-bit data is declared unsupported **by a static table keyed on arch,
not on device**. That is a compile-time blocker sitting in front of 6.2 for
every non-CUDA GPU path. **[V]**"

The table is real and A's line citations for it are exact
(`taichi/program/extension.cpp:8-33`, x64 :12, arm64 :16, cuda :20, amdgpu
:22, metal :23, opengl :24, gles :25, vulkan :26, dx11 :27, the abandoned
comment :29-30, the default-construction at :31). What is wrong is the
consequence.

`Extension::data64` is never queried from C++. Grepping
`is_extension_supported` across `taichi/` and `c_api/` returns six call sites
and none of them passes `data64`:

- `taichi/program/program.cpp:149` — `Extension::assertion`
- `taichi/codegen/llvm/codegen_llvm.cpp:2726` — `Extension::bls`
- `taichi/transforms/compile_to_offloads.cpp:92, 205, 218, 236` — `Extension::mesh`
- `taichi/transforms/compile_to_offloads.cpp:245, 288` — `Extension::quant`

The only consumers of `data64` are the Python binding at
`taichi/python/export_lang.cpp:1225` and, through it, the Python test harness
at `tests/test_utils.py:249`, which uses it to decide which tests to skip
(e.g. `tests/python/test_svd.py:8`). The Python front end is out of scope per
brief 1.2.

So the table blocks nothing in the C++ core. It is a declaration that only
out-of-scope code reads. A's escalation 4 ("`Extension::data64` is a
compile-time per-arch table that contradicts the runtime per-device
detection") is still a real inconsistency worth recording, but its weight
drops sharply: it is not a gate that 6.2 has to open, it is a stale label.
The planner should not size 6.2 work against it.

### A-2. `ti_device_api_shared` is built, never installed, and has no consumer.

A, §2.2: "`taichi/rhi/CMakeLists.txt:114-115` then produces
`ti_device_api_shared` ... **This is the only shared-library backend artefact,
and it contains every enabled backend at once.**"

The construction is exactly as A describes (`taichi/rhi/CMakeLists.txt:114`
`add_library(ti_device_api_shared SHARED public_device.h)`, `:115` linking the
static lib, `:120` the MSVC `dummy.cpp` workaround). What A does not say is
that nothing uses it. Grepping `ti_device_api` across `cmake/`,
`CMakeLists.txt`, `c_api/` and `taichi/` returns consumers only of the
**static** target: `cmake/TaichiCore.cmake:146`,
`taichi/ui/ggui/CMakeLists.txt:48`, `taichi/runtime/llvm/CMakeLists.txt:42`,
`taichi/runtime/program_impls/vulkan/CMakeLists.txt:24`. There is no
`install(TARGETS ti_device_api_shared ...)` anywhere. It would also export
nothing, because `cmake/TaichiCore.cmake:18` sets
`CMAKE_CXX_VISIBILITY_PRESET hidden` globally.

A's phrasing invites the reader to treat this as an existing shared-object
seam that 6.3 could build on. It is not one today.

### A-3. Citation slips

- `c_api/src/taichi_core_impl.h:120-121` (§2.5) — the comment is at `:121` and
  the array at `:122`. B has this right.
- `taichi/common/version.h.in:2-6` (§2.3) — the file is 5 lines.
- `taichi/rhi/cuda/cuda_context.cpp:80-82` for the shared-memory attribute
  (§4.3) — the call spans `:79-81`.
- `taichi/rhi/cuda/cuda_context.cpp:88-96` for total/free memory (§4.3) —
  `get_total_memory` is `:88-92`, `get_free_memory` is `:94-98`.
- `taichi/rhi/public_device.h:426-433` for `dispatch` and `ComputeSize`
  (escalation 5) — `dispatch` is `:426-428`, `ComputeSize` is `:430-434`.

A's other structural citations I checked are exact, including every backend
link line in `taichi/rhi/CMakeLists.txt` (`:45, 51, 57, 81, 86, 90, 95, 100,
104, 108, 111`), the whole of `taichi/rhi/arch.cpp`, all four
`get_compute_capability` consumers, `cmake/TaichiCAPI.cmake:144-146` and
`:179-184`, `cmake/TaichiConfig.cmake.in` being five lines,
`cmake/TaichiTargets.cmake:56-88`, and the `TI_NOT_IMPLEMENTED` /
`TI_UNREACHABLE` chain at `taichi/common/logging.h:108` and `:40-44`.

### A-4. What A missed that B found

Listing, because it bears on the completeness verdict.

- `spirv_has_physical_storage_buffer` is dead. See section 4.
- `ti_create_runtime` (`c_api/src/taichi_core_impl.cpp:247-309`) as a third
  hardcoded selection chain. A's §6 map of 6.3 touchpoints does not contain it.
- `c_api/src/taichi_opengl_impl.cpp:9-12` sets `spirv_has_int64`
  unconditionally, with no GLES check, overwriting the GLES-aware caps that
  `GLDevice::GLDevice` had just set at `opengl_device.cpp:509-513`. Directly
  6.2-relevant.
- `ProgramImpl::get_device_caps` (`taichi/program/program_impl.h:170-172`)
  returns `{}` and has exactly one override,
  `GfxProgramImpl::get_device_caps` (`taichi/runtime/program_impls/gfx/gfx_program.cpp:82-85`).
  This is a second, separate path by which the LLVM archs see an empty
  capability set, and it feeds kernel compilation
  (`taichi/program/program.h:136-137`, consumed at
  `taichi/aot/graph_data.cpp:36` and six sites in
  `taichi/program/snode_rw_accessors_bank.cpp`).
- The CUDA device index is pinned to 0 (`taichi/rhi/cuda/cuda_context.cpp:22`,
  enforced at `c_api/src/taichi_core_impl.cpp:289-290`), on a development box
  the brief says has two NVIDIA cards.
- The AMDGPU device libraries are selected by **detected hardware**, not by
  arch: `taichi/runtime/llvm/llvm_context.cpp:637` builds
  `"oclc_isa_version_" + isa_version + ".bc"` from
  `AMDGPUContext::get_instance().get_mcpu().substr(3, 4)` at `:629`. A cites
  `llvm_context.cpp:630` in passing as "the AMDGPU device libraries" and does
  not notice this. It is the closest existing precedent in the tree for what
  6.3 asks for, and B is right to single it out.

---

## 4. Errors in pass B that neither the other report nor B itself caught

### B-1. B omits the `Extension` system entirely.

`taichi/program/extension.h:17-24`, `taichi/inc/extensions.inc.h:1-10` and the
per-arch table at `taichi/program/extension.cpp:8-33` do not appear anywhere in
B's report or its section 6 map. Given A-1 above, the table turns out to be
inert in C++, so B's omission costs less than it looks. But B's section 6
claims to enumerate "capability description and matching" in the territory,
and the tree contains two capability vocabularies, not one. A found both.

### B-2. B omits the installed-package surface.

`cmake/TaichiConfig.cmake.in` (5 lines: `@PACKAGE_INIT@`, an include of
`TaichiTargets.cmake`, `check_required_components("Runtime")`) and
`cmake/TaichiTargets.cmake:56-88` (one `IMPORTED` `taichi_c_api` with
platform-dependent paths) carry no `TI_WITH_*` state, no capability data and
no tier information. Brief section 5 puts a loader at install time; what an
installed Taichi records about its own configuration is squarely territory 04.
A covers this (§3.4 and escalation 10); B does not mention it at all.

### B-3. B omits the DX12 stub, which brief 5.1 specifically asks about.

`taichi/rhi/dx12/dx12_api.cpp:6-12` returns `true` whenever `TI_WITH_DX12` is
defined and never touches a device, so `TI_ASSERT(directx12::is_dx12_api_available())`
at `taichi/program/program.cpp:87` is unconditionally true in a DX12 build,
and `make_dx12_device()` at `dx12_api.cpp:14-16` then returns `nullptr`. The
whole DX12 RHI is `dx12_api.h` (20 lines) and `dx12_api.cpp` (26 lines). A
found this and framed it against brief 5.1's "a wrongly shaped stub is worse
than no stub". Verified exactly as A states. B has nothing on it.

(`modernization/deferred-backends.md` already records DX12 as a 20-line stub,
so this is not new to the planner, but the *shape* of the failure — an
availability probe that reports build configuration rather than hardware,
guarding a factory that returns null — is new, and it is precisely the
category the brief flags.)

### B-4. B cites the dead copy of `DynamicLoader`.

B, §4.1, cites `taichi/system/dynamic_loader.h:12-34` as the definition.
Nothing includes that header. Every user includes
`taichi/common/dynamic_loader.h`: `taichi/rhi/cuda/cuda_driver.h:5`,
`taichi/rhi/cuda/cuda_driver.cpp:3`, `taichi/rhi/amdgpu/amdgpu_driver.h:5`,
`taichi/rhi/amdgpu/amdgpu_driver.cpp:3`, `taichi/rhi/vulkan/vulkan_loader.h:7`,
and the implementation itself at `taichi/common/dynamic_loader.cpp:1`. B's
§7 correctly flags that the two headers differ by `check_lib_loaded`, but §4.1
anchors the finding on the copy that is not in use.

B's conclusion survives intact, and I verified it independently: `dlopen`,
`dlsym`, `LoadLibraryA` and `GetProcAddress` appear in `taichi/` and `c_api/`
only at `taichi/common/dynamic_loader.cpp:16, 27, 29, 36, 38`, plus glad's own
GL entry-point loading at `taichi/rhi/opengl/opengl_api.cpp:96-100, 183-187`.
Nothing loads Taichi's own compiled code as a shared object.

### B-5. "A numeric SNode ceiling would be the first non-boolean project-level build parameter" is false.

See section 5, item S2. This is B's one materially wrong conclusion, and it
sits directly under B's escalation 1, which is the escalation the planner is
most likely to act on for 6.1.

### B-6. `arch_is_gpu` classifies js explicitly, not by omission.

B, §1.2: "`arch_is_gpu` 59-61 (defined as `!arch_is_cpu`, so opencl and js are
classified by omission)". `taichi/rhi/arch.cpp:43` names js explicitly:
`if (arch == Arch::x64 || arch == Arch::arm64 || arch == Arch::js)`. js is an
explicit CPU. Only opencl falls through by omission, and it lands on the GPU
side.

### B-7. `get_total_memory` is on `LlvmDevice`, not the RHI `Device` base.

B, §2.5: "`taichi/rhi/cuda/cuda_device.h:151-153` exposes `get_total_memory()`
on the RHI `Device` interface." It overrides
`LlvmDevice::get_total_memory` (`taichi/rhi/llvm/llvm_device.h:34`).
`taichi/rhi/public_device.h` has no such member. The distinction matters for
6.3: a tier-aware selector reading device memory through the RHI base class
cannot, today; it would have to go through the LLVM sub-interface or
`CUDAContext` directly. B's own inference in §4.3 — that no memory size
reaches the C API — is correct and I confirmed it: `get_total_memory` has no
caller in `c_api/`, only `taichi/runtime/llvm/llvm_runtime_executor.cpp:612`
and the AMDGPU/CUDA overrides.

### B-8. Citation slips

- `ti_get_available_archs` is `c_api/src/taichi_core_impl.cpp:171-205`, not
  `:171-202`. A has this right.
- `TiCapability` is `c_api/include/taichi/taichi_core.h:380-407`, not
  `:380-406`.
- GGUI's `target_compile_definitions` is `cmake/TaichiCore.cmake:383`, not
  `:384`.
- The SPIR-V/gfx link gate is `cmake/TaichiCore.cmake:295-298`, not `:294-297`;
  the unconditional `add_subdirectory` calls are at `:291-292`.
- `cuda_context.cpp` calls `device_get_count` at `:21` and `device_get` at
  `:22`, not `:22-23`.
- `cmake/TaichiCore.cmake:1-11` holds **eleven** options; B lists ten and
  silently drops `USE_STDCPP` at `:1`. A has the full list.

B's other structural citations I checked are exact, including the entire
`spirv_has_physical_storage_buffer` producer/consumer census, the
`taichi_max_num_snodes` and `kMaxNumSnodeTreesLlvm` census, the platform
coercion sub-ranges, `misc/taichi_json.py:102-116, 182-185`,
`c_api/taichi.json:110-123` and `:125-129`, the AOT producer/serialiser/
deserialiser chain at `aot_module_builder_impl.cpp:22-24` and `:57-61` and
`aot_module_loader_impl.cpp:43-46`, and `llvm_context.cpp:209-211, 213-218,
358-362, 574-591, 625-655`.

### B-9. B's strongest single finding, which I confirm in full

`spirv_has_physical_storage_buffer` — the buffer-device-address capability,
i.e. 64-bit pointers inside shaders — has **two producer sites and both are
disabled**:

- `taichi/rhi/vulkan/vulkan_device_creator.cpp:826`, inside
  `#if !defined(__APPLE__) && false` opened at `:825` and closed at `:827`,
  with the comment at `:824` "(penguinliong) Temporarily disabled (until device
  capability is ready)".
- `c_api/src/taichi_vulkan_impl.cpp:48`, inside a `/* */` block spanning
  `:46-51`, preceded at `:45` by "Will bring it back after devcap."

And twelve live consumers, all of which therefore permanently take the false
branch: `taichi/rhi/vulkan/vulkan_device.cpp:1772, 1792, 2147, 2509`;
`taichi/codegen/spirv/spirv_ir_builder.cpp:73, 113`;
`taichi/codegen/spirv/spirv_codegen.cpp:783, 2340, 2416, 2491`;
`taichi/runtime/gfx/runtime.cpp:96`;
`taichi/runtime/program_impls/gfx/gfx_program.h:80-83`. I counted and opened
all twelve.

The last is the sharpest, exactly as B says:
`GfxProgramImpl::get_kernel_argument_data_layout` at `gfx_program.h:79-83`
returns `"1" + std::string(has_buffer_ptr ? "b" : "-")` and therefore always
returns `"1-"`.

Brief 6.2 is 64-bit addressing. On the SPIR-V spine, a substantial part of
that machinery is already written and compiled and is switched off at the
capability layer rather than absent. A missed this entirely, while citing the
capability's own declaration line (`taichi/inc/rhi_constants.inc.h:28`) in
§4.2. This is the largest asymmetry between the two reports.

---

## 5. What **both** reports missed

Five items. All are inside territory 04 and all bear on section 6 of the brief.

### S1. There is no compatibility gate on AOT load at all — not merely a dead capability check.

Both reports converge on "the capability check exists and is dead"
(`c_api/src/taichi_gfx_impl.cpp:18-29`, `get_required_caps` never overridden).
That is correct and I verified it: grep over `taichi/` and `c_api/` for
`get_required_caps` returns exactly two hits, the default body at
`taichi/aot/module_loader.h:97-100` and the call site at
`taichi_gfx_impl.cpp:21`.

But the same is true of the other two handshakes, and neither report says so.

**Version.** `aot::Module::version()` is pure virtual at
`taichi/aot/module_loader.h:85`. The gfx override at
`taichi/runtime/gfx/aot_module_loader_impl.cpp:114-116` is
`TI_NOT_IMPLEMENTED`. The LLVM override at
`taichi/runtime/llvm/llvm_aot_module_loader.h:36-38` returns 0. **Neither is
ever called** — grep for `version()` across `taichi/aot/`, `c_api/src/`,
`taichi/runtime/gfx/` and `taichi/runtime/llvm/` returns only the declaration
and the two overrides. A notes that the LLVM one returns 0; neither notes that
the gfx one aborts, or that nothing calls either.

**Arch.** `aot::Module::arch()` likewise has no caller anywhere in `taichi/` or
`c_api/`. The gfx override at `aot_module_loader_impl.cpp:111-113` returns
`device_api_backend_`, which is the arch handed in from the *runtime* at
`taichi/aot/module_loader.cpp:31`, not anything read from the module. And
`metadata.json` records no arch: the serialised field list is
`TI_IO_DEF(kernels, fields, required_caps, root_buffer_size)`
(`taichi/runtime/gfx/aot_utils.h:23`), mirrored at
`taichi/aot/module_data.h:135`.

Consequence: an AOT module built for Vulkan can be handed to an OpenGL runtime
and nothing will object. Both reports frame 6.3's problem as "one dead check to
wire up". The accurate frame is "no handshake of any kind exists; the
capability slot is the only one that was even wired half-way".

By contrast, the *kernel* level does carry and check an arch:
`taichi/compilation_manager/kernel_compilation_manager.cpp:225`
(`TI_ASSERT(loaded->arch() == arch)`),
`taichi/runtime/cuda/kernel_launcher.cpp:191` and
`taichi/runtime/amdgpu/kernel_launcher.cpp:154`. So the pattern exists one
layer down and was not lifted to the module.

### S2. A non-boolean CMake value is **already** threaded into a generated header. `CUDA_VERSION`.

This is the item I most want the planner to have, because it lands directly on
A's escalation 1 and B's escalation 1.

- `cmake/TaichiCore.cmake:98-99`: `if(NOT CUDA_VERSION) set(CUDA_VERSION 10.0)
  endif()` — a default, overridable with `-DCUDA_VERSION=...` on the cmake
  command line, and therefore through `TAICHI_CMAKE_ARGS`
  (`setup.py:114`) and through `DEF_RE` in
  `.github/workflows/scripts/ti_build/cmake.py:18`.
- `CMakeLists.txt:149-152`: forced to `"0.0"` when CUDA is off.
- `taichi/common/version.h.in:5`: `#define CUDA_VERSION "@CUDA_VERSION@"`,
  substituted by the `configure_file` at `CMakeLists.txt:203`.
- Consumed at `taichi/common/core.cpp:87-89` (`get_cuda_version_string`) and
  from there at `taichi/runtime/llvm/llvm_context.cpp:216-218`, where it
  selects the bitcode filename `slim_libdevice.<major>.bc`.

So the tree already contains, end to end, exactly the mechanism 6.1 needs: a
non-boolean value set at configure time, carried into C++ through a generated
header, and used at run time to pick which device bitcode to load. B's claim
that a numeric ceiling "would be the first non-boolean project-level build
parameter" (§5.4) is false. A's narrower claim about *cache* variables (§2.4)
is technically survivable, since `CUDA_VERSION` is a plain variable, but A
presents "no non-boolean parameter exists" as a constraint on 6.1 when a
working example is three lines above the CUDA `-D` block A does cite.

One caution I checked before offering this: `taichi/rhi/cuda/cupti_toolkit.cpp:263`
uses `#if CUDA_VERSION >= 11040`, which would misbehave against a string
macro. It does not include `taichi/common/version.h` — the only includers are
`taichi/common/core.cpp:7`, `taichi/util/offline_cache.h:12` and
`taichi/runtime/llvm/llvm_offline_cache.cpp:14` — so there is no collision
today. There would be one if a generated `constants.h` were included more
widely, which is a real design consideration for 6.1 and belongs in the
escalation, not in a report's findings.

### S3. Changing the SNode ceiling will not invalidate offline caches or LLVM AOT modules.

A's central 6.1 insight is that host C++ and the device bitcode could disagree
on the size of `element_lists`, `node_allocators` and `ambient_elements`
(`taichi/runtime/llvm/runtime_module/runtime.cpp:567-569`), silently. Correct,
and the include-path mechanism A cites is exactly as described:
`runtime.cpp:25` includes `taichi/inc/constants.h`, resolved only through the
`-I ${PROJECT_SOURCE_DIR}` on `taichi/runtime/llvm/runtime_module/CMakeLists.txt:8`.

The same hazard exists one layer up and neither report reaches it. The LLVM
offline cache validates against the Taichi version and nothing else:
`taichi/util/offline_cache.h:95-99` rejects a cache only when
`ver[0] != TI_VERSION_MAJOR || ver[1] != TI_VERSION_MINOR || ver[2] != TI_VERSION_PATCH`.
`LlvmAotModule` reuses that reader —
`cache_reader_(LlvmOfflineCacheFileReader::make(module_path))` at
`taichi/runtime/llvm/llvm_aot_module_loader.h:25`. A rebuild that changes
`taichi_max_num_snodes` does not change `TI_VERSION`, so kernels compiled
against the old struct layout will be loaded and run against the new one.

This straddles territory 03. A explicitly scoped the offline cache format out
("the runtime struct layer and the LLVM offline cache format (agent 03)");
that is a defensible scoping call, not a fault, but the seam is in territory
04's half of 6.1 and someone has to own it. B does not mention the offline
cache at all.

### S4. `taichi/rhi/CMakeLists.txt:17` has a CMake operator-precedence bug.

```
if (TI_WITH_OPENGL OR TI_WITH_VULKAN AND NOT ANDROID)
```

CMake binds `AND` tighter than `OR`, so this reads
`TI_WITH_OPENGL OR (TI_WITH_VULKAN AND NOT ANDROID)`. The `NOT ANDROID` guard
therefore does not apply to the OpenGL arm: with OpenGL on and `ANDROID` set,
GLFW is still added as a subdirectory (`:29`), linked (`:30`) and put on the
public include path (`:31-34`).

Both reports read this file line by line — A enumerates every `target_link_libraries`
in it, B cites `:42-105` twice — and neither flagged it. I am recording it, not
proposing a change; whether Android is a target at all is not settled anywhere
in the brief.

### S5. The C API documents that capability levels are **not** ordered, which undercuts both reports' `!=` finding.

A calls the `!=` at `c_api/src/taichi_gfx_impl.cpp:25` a "second, independent
defect in the same check" and says "exact-match is the wrong relation" (§5.2,
escalation 2). B calls it "a second issue in the same code" and says a device
exceeding the requirement "should presumably be accepted" (§3.2, escalation 5).

`c_api/include/taichi/taichi_core.h:409-416` says otherwise:

```
// Structure `TiCapabilityLevelInfo` (1.4.0)
//
// An integral device capability level. It currently is not guaranteed that a
// higher level value is compatible with a lower level value.
```

Against the published contract, `!=` is the only relation the type supports.
It is not a defect; it is the conservative reading of a documented
non-guarantee. Changing it to `>=` is a change to the public contract, and it
is only sound for the capabilities that happen to be ordered (`spirv_version`)
and not for the ones that are booleans stored as levels. Both reports present
this as a bug to be fixed; it is a contract to be decided. I have moved it to
escalations accordingly.

Both reports also miss that `str2devcap` (`taichi/rhi/device_capability.cpp:6-13`)
is the exact inverse of the `to_string` used by the producer at
`aot_module_builder_impl.cpp:23` — B does say this (§3.2) and is right; A does
not. So the missing link really is one loop over `ti_aot_data_.required_caps`
calling `str2devcap`. B's diagnosis is the more actionable of the two.

### Minor, recorded only

`is_extension_supported` reaches its table through non-const `operator[]` on a
function-static `std::unordered_map` (`taichi/program/extension.cpp:31`), so a
lookup on a missing key (dx12, js, opencl) inserts into the static map. A
correctly reports the observable behaviour ("default-constructs an empty
set"); neither notes that it mutates shared static state without
synchronisation. Not proposing anything; recorded because 6.3 would add arches
to this table's callers.

---

## 6. What both reports got right, and I could not break

I tried. These held under direct reading:

- The absence of any registry, factory or plugin mechanism for backends. Three
  hardcoded chains: `taichi/program/program.cpp:80-133`,
  `taichi/aot/module_loader.cpp:31-58`,
  `c_api/src/taichi_core_impl.cpp:247-309`. Both reports' tables of the first
  two are accurate row by row; only B has the third.
- `taichi/rhi/arch.cpp` contains no `TI_WITH_*` conditioning; every predicate
  is a pure function of the enum, with `host_arch()` at `:68-76` the sole
  preprocessor decision. Both correct. A's function-by-function line map is
  exact.
- `CompileConfig::fit()` (`taichi/program/compile_config.cpp:67-76`) contains
  no arch fallback, against the comment at `taichi/program/program.cpp:140`.
  A found this; it is a genuine and useful observation.
- The `configure_file` precedent and its source-tree destination
  (`CMakeLists.txt:203-204`), and the fact that the bitcode build resolves
  `taichi/inc/constants.h` through `-I ${PROJECT_SOURCE_DIR}` and inherits
  nothing else. Both correct, from complementary angles: A frames it as "a
  binary-dir header would be missed", B as "a `CMAKE_CXX_FLAGS` `-D` would be
  missed". Both framings are true and neither is the whole picture; together
  they are.
- The positional derivation of `TiCapability` from
  `taichi/inc/rhi_constants.inc.h` via `misc/taichi_json.py:115`
  (`cases[key][case_name] = len(cases[key])`), and the resulting ABI
  constraint. Verified: 25 `PER_DEVICE_CAPABILITY` entries at
  `rhi_constants.inc.h:10-34`, 25 `TI_CAPABILITY_*` values 0-24 at
  `taichi_core.h:381-405`, `spirv_has_int64` fifth in the file (`:14`) and
  `TI_CAPABILITY_SPIRV_HAS_INT64 = 4` (`:385`). Both reports correct.
- `spirv_has_atomic_int64` (`rhi_constants.inc.h:17`) has no producer and no
  consumer. Verified: the only other hits in the tree are
  `c_api/include/taichi/taichi_core.h:388` and the header-only wrapper at
  `c_api/include/taichi/cpp/taichi.hpp:1121-1122`. Both reports correct.
- The census of `taichi_max_num_snodes` (three sites) and
  `kMaxNumSnodeTreesLlvm` (two sites). B's is exact. A's negative finding —
  no hardcoded `1024` duplicate in the territory, and
  `taichi/rhi/llvm/allocator.h:6` includes `constants.h` without using any of
  the three — is also correct.
- `ti_build/cmake.py`: `OPTION_RE` at `:17` matching only the boolean form,
  the assertion at `:60` passing for an unknown name, the assertion at `:84`
  forbidding a wheel-tag on a non-boolean, and `render()` at `:98` emitting
  `-DNAME=value`. Both reports correct; A's trace through
  `option_definitions.get(name, ("", None, ""))` at `:57` is precise, and B's
  consequence for artefact naming is the more useful one.
- The `.bc` adaptive-module pattern: `get_runtime_fn` at
  `llvm_context.cpp:209-211`, `module_from_file` at `:358-362`,
  `runtime_lib_dir()` at `lang_util.cpp:30-47`, production at
  `runtime_module/CMakeLists.txt:29-31`, installation at `:13` and
  `cmake/TaichiCAPI.cmake:179-184`. Both correct.
- The `${arch}` / `${rtm_arch}` mismatch at
  `runtime_module/CMakeLists.txt:13` versus `:8`. Both found it independently
  and both describe the scoping correctly.
- The `cuda_runtime-...-sm_60.bc` rule being commented out at
  `runtime_module/CMakeLists.txt:36-43`, and the link being skipped because
  `get_custom_cuda_library_path` (`lang_util.cpp:18-28`) returns empty when the
  file is absent, checked at `llvm_context.cpp:576-577`. B only; correct.
- The CUDA compute-capability detection, the clamp to 86 at
  `cuda_context.cpp:75-77`, and `mcpu_ = "sm_{}"` at `:83`. Both correct.
  A's list of the four `get_compute_capability` consumers is exhaustive and
  right.

---

## 7. Independence

**I judge the two passes genuinely independent.** The disclosure at B's
escalation 11 and notes entry 19 is immaterial, and the evidence against
contamination is stronger than the disclosure is against it.

Four reasons.

1. **B is wrong about Metal in a way A is right about** (section 2.1). Any
   meaningful reading of A's material would have corrected it. A single shared
   fact of that prominence — which backends populate `set_caps` — cannot
   survive contact between the two files.
2. **The self-corrections are internal to each pass.** A's notes entry 17
   corrects its own entry 12; B's entry 16 corrects its own entry 9 and entry
   20 corrects its own entry 8. Each correction is reached from the pass's own
   earlier error, not from the other pass's better answer.
3. **The discovery orders are unrelated.** A opens on the build layer
   (notes entry 2, "starting with the build layer since 6.1's threading
   question is the most concrete") and reaches capabilities at entry 5. B opens
   on the arch enum and reaches build parameterisation at entry 4 and the
   `CMAKE_CXX_FLAGS` finding at entry 6. Neither structure is derivable from
   the other.
4. **Each found substantial material the other missed, in both directions.**
   Sections 3 and 4 above list six A-only findings and six B-only findings.
   Contamination would show as convergence, and there is very little.

On the disclosed paragraph itself — c_api header generation from
`taichi.json` — the two treatments differ in citation and in emphasis. A cites
`misc/taichi_json.py:102-116, 182-183` and `:401`, and frames it as "the
capability enum is a positional ABI". B cites `:102-116`, `:108`, `:115`,
`:182-185` plus `c_api/taichi.json:125-129`, and frames it as an ABI
constraint on adding capabilities. Both are the conclusion any reader who
opens `taichi_json.py:115` reaches; there is nothing in either that requires
having seen the other. B's disclosure was correct to make and I would discount
nothing on account of it.

---

## 8. Verdicts, stated separately

### Correct?

**Pass A: yes, with one materially wrong conclusion.** The `Extension::data64`
claim (§4.1, "a compile-time blocker sitting in front of 6.2 for every
non-CUDA GPU path", marked `[V]`) is false and, because it is marked verified,
would be believed. Everything else is either right or a citation slip. A's
`[V]`/`[I]` discipline is otherwise honest — the consolidated section 8 of A's
report accurately separates what was read from what was reasoned, including
the card-to-`sm_` mapping, which A correctly flags as external knowledge.

**Pass B: yes, with one materially wrong conclusion.** The Metal `set_caps`
claim (§2.2) is false, marked `[V]`, and inside an "exhaustive grep" claim
that is itself false. B's separate assertion that a numeric ceiling would be
the first non-boolean build parameter (§5.4) is also false and sits under B's
most consequential escalation. B's `[V]`/`[I]` discipline is likewise honest,
and B's inference markers on §2.4 ("I verified the code is unreachable; I did
not verify it is correct") are exactly the right shape.

Neither error invalidates the report around it.

### Complete?

**Pass A: no.** It misses `spirv_has_physical_storage_buffer`, which is the
single most 6.2-relevant fact in the territory, while citing the capability's
declaration line. It also misses the third selection chain, the three c_api
capability producers including the GLES-incorrect one, and
`ProgramImpl::get_device_caps`.

**Pass B: no.** It misses the `Extension` vocabulary, the installed-package
surface (which brief section 5 makes load-bearing), and the DX12 stub shape
(which brief 5.1 makes load-bearing).

**The pair: nearly complete.** With the corrections in sections 2-4 applied
and the two halves merged, the only remaining gaps I found are S1 through S5.

### Major revision required?

**No.** Neither report needs to go back to be re-done. What is required is:

1. Two sentence-level retractions: A's `Extension::data64` conclusion, B's
   Metal `set_caps` claim (and B's "first non-boolean parameter" claim).
2. The citation corrections in sections 2.7, 3.3 and 4.8.
3. A merge, because each report's blind spot is the other's strongest
   material.
4. The five shared gaps folded in.

If the planner's bar is "one report the planner can act on unaccompanied",
neither clears it. If the bar is "the pair, merged and corrected, maps the
territory", it clears comfortably.

---

## 9. Escalations

Unresolved. Each needs a decision the project owner has not made. I am not
resolving any of them.

1. **Does the AOT module format need arch and version handshakes at all?**
   (S1.) Today there are none: `version()` is uncalled and aborts on the gfx
   path (`aot_module_loader_impl.cpp:114-116`), `arch()` is uncalled and
   returns the caller's own arch (`:111-113`), and `metadata.json` records no
   arch (`aot_utils.h:23`). Both reports' escalations treat the dead
   capability check as the gap; it is one third of the gap.

2. **Is `>=` even permissible for capability levels?** (S5.)
   `c_api/include/taichi/taichi_core.h:411-412` states that a higher level is
   not guaranteed compatible with a lower one. Both reports propose relaxing
   `c_api/src/taichi_gfx_impl.cpp:25` from `!=`; doing so contradicts the
   published contract, and the contract may be what needs changing instead.
   Whether the C ABI is fixed at all in this fork is A's escalation 8 and is
   the same question.

3. **Must a changed SNode ceiling invalidate offline caches and LLVM AOT
   modules, and keyed on what?** (S3.) `taichi/util/offline_cache.h:95-99`
   keys on `TI_VERSION` alone, and `llvm_aot_module_loader.h:25` inherits it.
   A ceiling change does not move `TI_VERSION`.

4. **Is `CUDA_VERSION` the intended precedent for 6.1?** (S2.) It is a working
   non-boolean CMake value carried through `configure_file`
   (`CMakeLists.txt:203`, `taichi/common/version.h.in:5`) into a runtime
   bitcode filename decision (`llvm_context.cpp:216-218`). It writes a
   generated header into the source tree, which is the hygiene question A's
   escalation 1 and B's escalation 1 both stop at. If a generated
   `constants.h` follows this pattern, the macro-name collision risk noted in
   S2 needs a ruling too.

5. **Does `taichi/rhi/CMakeLists.txt:17` need fixing, and is Android a target
   at all?** (S4.) The precedence bug is real; whether it matters depends on a
   scope decision that is not in the plan.

I am not carrying forward the escalations already raised by A (11 items) and B
(11 items). Those stand as written except where sections 3, 4 and 5 above
change their weight: A's escalation 4 is much weaker than A believes, A's
escalation 2 and B's escalation 5 are governed by escalation 2 here, and B's
escalation 1 rests on a false premise corrected in S2.

---

## 10. Divergence from adversary 04-2

To be appended after reading `adversary-04-2.md`.

`adversary-04-2.md` exists. I read it after finishing sections 1-9 above, which
are unchanged. This section records where we diverge and which of us the source
supports. I re-opened every file 04-2 cites that I had not already opened.

### 10.1 Where we agree

We independently reached the same resolution on every head-to-head contradiction
between A and B, and on the same two "materially wrong conclusion" verdicts:
A's `Extension::data64` blocker claim and B's Metal `set_caps` claim. We both
find the passes genuinely independent, on substantially the same evidence — the
shape and direction of the blind spots rather than the prose. 04-2's divergence
table (its §5) matches my sections 2 and 3 item for item, including the
correct scoring of item 8 as "neither".

We also both flag: `HOST_ARCH` refuting A's non-boolean claim; B citing the dead
`taichi/system/dynamic_loader.h`; the `TAIHI_CORE_SOURCE` typo being inert
because neither `runtime.cpp` exists; the `USE_STDCPP` omission from B's option
list; and the same set of range slips.

### 10.2 Where 04-2 is right and I missed it

Six items. I verified each before conceding.

1. **`taichi/rhi/dummy.cpp` is a zero-byte file.** Confirmed by `ls`: 0 bytes.
   With `add_library(ti_device_api_shared SHARED public_device.h)`
   (`taichi/rhi/CMakeLists.txt:114`) and `dummy.cpp` as its only compiled source
   (`:120`), the shared object contains no translation unit with undefined
   symbols, so linking the static `ti_device_api` archive extracts nothing.
   My section 3 item A-2 established only that the target is unused; 04-2
   additionally refutes the *content* half of A's claim ("it contains every
   enabled backend at once") on link semantics. 04-2's version is stronger and
   I adopt it.

2. **A's "Each toggle does exactly three things and nothing else"
   (`report-04:56`, marked `[V]`) is false.** Verified counter-examples:
   `cmake/TaichiCore.cmake:104-105` (CUDA also drives a `file(GLOB)` and a list
   append), `:62-64` (Vulkan forces `TI_WITH_GGUI` on), `:278-283`
   (CUDA + CUDA_TOOLKIT pulls `find_package(CUDAToolkit REQUIRED)` and links
   `CUDA::cupti`), and `CMakeLists.txt:154-164` (CUDA, AMDGPU and DX12 set the
   arch variables that drive the bitcode loop at
   `taichi/runtime/llvm/runtime_module/CMakeLists.txt:29`). I read all four
   blocks while checking other claims and did not notice the contradiction.

3. **A contradicts itself on 64-bit hardware queries.** `report-04:321-322`
   says the Vulkan `shaderInt64` check is "the only hardware query for 64-bit
   integers in the entire tree"; A's own table at `report-04:307` records
   Metal's. Verified: `taichi/rhi/metal/metal_device.mm:1027-1036` issues real
   `[mtl_device supportsFamily:]` queries, reduced to
   `feature_64_bit_integer_math = family_apple3` at `:1038` and consumed at
   `:1051-1053`. I had opened all of this and did not spot that A's "only"
   contradicts A's own §4.2.

4. **`c_api/include/taichi/taichi.h:9-27` is a fourth re-expression of the
   toggle list, and `TI_WITH_CPU` is defined nowhere.** Verified: the umbrella
   header gates `taichi_vulkan.h` `:9`, `taichi_opengl.h` `:13`,
   `taichi_cuda.h` `:17`, `taichi_cpu.h` `:21`, `taichi_metal.h` `:25`, and a
   grep for `TI_WITH_CPU` across all sources outside `external/` returns
   exactly `taichi.h:21` and `:23`. So `taichi_cpu.h` is unreachable through the
   umbrella header although `cmake/TaichiCAPI.cmake:33` installs it whenever
   `TI_WITH_LLVM` is on. This is the consumer-facing form of the install-time
   question and it is a better finding than anything either report has on the
   topic. I missed it entirely.

5. **`aot::ModuleData::required_caps` is never written by anybody.** Verified:
   the only write in the tree is
   `taichi/runtime/gfx/aot_module_builder_impl.cpp:23`, into
   `gfx::TaichiAotData` (`taichi/runtime/gfx/aot_utils.h:20`). The generic field
   at `taichi/aot/module_data.h:125` has one subclass, `ModuleDataDX12`
   (`taichi/runtime/dx12/aot_module_builder_impl.h:11`), which adds `dxil_codes`
   and populates no caps. Both reports describe a single generic channel; there
   are two structs and only one is live. This sharpens my own S1 rather than
   competing with it.

6. **Capability renumbering also moves the offline-cache key.** Verified and
   live, not dead: `taichi/analysis/offline_cache_util.cpp:88-95` serialises
   `caps.devcaps` — the raw `std::map<DeviceCapability, uint32_t>`, keyed by the
   numeric enum — and it is called at `:185` and folded into the hash at `:190`.
   Both reports treat positional renumbering purely as a C ABI question.

Smaller points of 04-2 I also confirm and had not made: `taichi/rhi/CMakeLists.txt:28`
uses `target_compile_definitions`, so B's "only GGUI uses it" is true only
within `TaichiCore.cmake` (04-2 §4.5); the bitcode `.bc` files are already
produced into the source tree because
`runtime_module/CMakeLists.txt:10` sets `WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"`
and `:13` installs from `${CMAKE_SOURCE_DIR}` (04-2 §6.4); and the argument that
extending `-I` is not symmetric with generating a header, because the checked-in
`taichi/inc/constants.h` would still be visible and `-I` order would decide the
winner (04-2 §6.5). That last is the sharpest analytical point in either
adversary file.

### 10.3 Where I have material 04-2 does not

Five items, all verified in section 5 above.

1. **No arch or version handshake exists on AOT load either** (my S1).
   `aot::Module::version()` (`taichi/aot/module_loader.h:85`) has no caller; the
   gfx override at `taichi/runtime/gfx/aot_module_loader_impl.cpp:114-116` is
   `TI_NOT_IMPLEMENTED` and the LLVM override at
   `taichi/runtime/llvm/llvm_aot_module_loader.h:36-38` returns 0.
   `aot::Module::arch()` has no caller either; the gfx override at
   `aot_module_loader_impl.cpp:111-113` returns the arch handed in from the
   runtime at `taichi/aot/module_loader.cpp:31`, and `metadata.json` records no
   arch at all (`aot_utils.h:23`). 04-2's §6.2 covers the producer side of
   `required_caps` and stops there. The three handshakes are all absent, not
   one.

2. **`CUDA_VERSION` is already a non-boolean CMake parameter carried into a
   generated header and used to pick a device bitcode file** (my S2).
   `cmake/TaichiCore.cmake:98-99` → `CMakeLists.txt:203` →
   `taichi/common/version.h.in:5` → `taichi/common/core.cpp:87-89` →
   `taichi/runtime/llvm/llvm_context.cpp:216-218`, selecting
   `slim_libdevice.<major>.bc`.

   This is a direct divergence, not just an addition. 04-2's §3.5 endorses pass
   B's §5.4 without qualification ("Pass B has this right"). B's §5.4 also
   contains "A numeric SNode ceiling would be the first non-boolean
   project-level build parameter", which `CUDA_VERSION` falsifies. 04-2
   endorsed a paragraph containing a false claim. The source supports me here.

3. **A changed SNode ceiling invalidates nothing** (my S3).
   `taichi/util/offline_cache.h:95-99` validates on `TI_VERSION_MAJOR/MINOR/PATCH`
   only, and `taichi/runtime/llvm/llvm_aot_module_loader.h:25` inherits that
   reader. This is a different mechanism from 04-2's §6.3, which is about
   capability renumbering changing the *kernel* key. Both are real and they
   compose: the capability map is hashed into the kernel key, but the SNode
   ceiling is not hashed into anything.

4. **`taichi/rhi/CMakeLists.txt:17` has a CMake operator-precedence bug** (my
   S4). `if (TI_WITH_OPENGL OR TI_WITH_VULKAN AND NOT ANDROID)` parses as
   `TI_WITH_OPENGL OR (TI_WITH_VULKAN AND NOT ANDROID)`. 04-2 read this file
   closely enough to verify all twelve of A's link-line citations in it and did
   not flag line 17.

5. **The C API documents that capability levels are not ordered** (my S5).
   `c_api/include/taichi/taichi_core.h:411-412`: "It currently is not
   guaranteed that a higher level value is compatible with a lower level
   value."

### 10.4 The two real judgement divergences

**Divergence 1 — is the `!=` at `c_api/src/taichi_gfx_impl.cpp:25` a defect?**

04-2's escalation 5 says "Both reports escalate it and both are right to",
adding only that fixing it is insufficient on its own. That accepts A's and B's
framing, in which exact-match is the wrong relation.

I hold that `taichi_core.h:411-412` settles the factual half against that
framing: the published contract states that a higher level is not guaranteed
compatible with a lower one, so `!=` is the only relation the documented type
supports. Changing it to `>=` is a change to the public contract, and it is
sound only for the genuinely ordered capability (`spirv_version`), not for the
booleans stored as levels. 04-2 does not cite `:411-412` and I believe did not
see it.

The source supports me on the fact. Whether the contract should change is the
planner's, and both of us put it in escalations, so the practical difference is
one of framing: 04-2 hands the planner "a bug with a prerequisite", I hand it
"a contract decision". I think mine is the safer brief.

**Divergence 2 — does pass A need major revision?**

04-2: "MAJOR REVISION REQUIRED: yes for pass A. Targeted correction for pass B."
Me, section 8: no for either.

Having verified 04-2's three additional false `[V]` claims (items 2 and 3 of
§10.2, plus the `ti_device_api_shared` content half), the tally against A is
five `[V]`-marked general statements that A's own citations do not support:
`Extension::data64` as a blocker, `ti_device_api_shared`, "exactly three things
and nothing else", "no non-boolean cache variable", and "the only hardware query
for 64-bit integers". That is more than I had, and it is a pattern rather than a
scatter: A generalises one step past what it read, while marking the
generalisation `[V]`. In a method that runs on the `[V]`/`[I]` distinction, that
is a reliability problem in the instrument, and 04-2 is right to weight it
heavily.

I move toward 04-2 on severity and still stop short of its verdict. My reason:
none of the five inverts a load-bearing finding. A's structural results — the
bitcode-runtime threading trap for 6.1, the dead AOT capability gate, the
positional capability ABI, `CompileConfig::fit()` being empty, the DX12 stub
shape, and the install surface carrying no configuration — all survive
unamended, and three of those six are A-only. What A needs is five retractions
and a merge, which is a correction pass over a sound document, not a
re-derivation. If the planner's threshold for "major revision" is "the `[V]`
marks in this file can no longer be trusted at face value", 04-2's verdict is
the right one and I would not argue against it. On that specific question the
arbiter should take 04-2's reading over mine, because the count of unsupported
`[V]` claims is the evidence and 04-2 assembled more of it than I did.

### 10.5 Net

Neither adversary file is complete alone. 04-2 has six findings I lack, I have
five it lacks, and the two sets barely overlap — which is the same signal the
explore pair produced one level down. Merged, the two adversary files plus the
two reports cover the territory; the open items are the escalations in my
section 9 and 04-2's section 7, which are also largely disjoint and should be
carried forward together.
