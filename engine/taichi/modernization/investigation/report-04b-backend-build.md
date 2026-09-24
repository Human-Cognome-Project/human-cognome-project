# Report 04B — backend, ahead-of-time and build architecture

Explore agent 04B. Territory: `taichi/rhi/`, `taichi/aot/`, `c_api/`,
`cmake/` and the root `CMakeLists.txt`. Focus: adaptive module loading and
build-time parameterisation.

Contemporaneous notes: `modernization/investigation/notes-04b-backend-build.md`.
Entries 1-23 are the first pass; entries 24-35 are the revision; entries 36-44
are the amendment pass; entries 45-51 are the framing pass; entries 52-59 are
the round-three pass.

**Plan version. This pass was worked against `PROJECT-PLAN.md` as it stands on
2026-09-09**, re-read in full before finalising per standing instruction 6. Two
sections that govern this report did not exist when any earlier pass of it was
written: **section 2.2a**, which states what "vendor" means and rules that work
on either compilation spine is BASE work, and **section 5.1a**, which settles
that an install-time configuration agent sets the SNode field size and that per
build already means per device configuration. Where an instruction I was given
conflicts with the plan, the plan wins and I say so; that happens once, in item
1 below.

**Round-three amendment pass.** Both round-three adversaries judge this report
correct on facts and not complete, and consensus is not reached. What changed,
in order of weight:

1. **The reference-path / vendor-path axis is withdrawn throughout.** It
   entered this report from an intermediate wording of plan section 5.2 that
   the 2026-09-09 correction withdraws by name, and section 2.2a now settles
   the point in terms: the two compilation spines are not vendors, because LLVM
   carries CPU-only operation and SPIR-V carries the portable GPU path, so work
   on either spine is BASE work. **My own enumeration:
   `grep -n "reference path\|accelerant\|vendor path\|portable path\|vendor
   half"` over the previous revision of this file returned 37 distinct lines**
   — 20 carrying "reference path", 5 carrying "accelerant", and 12 further
   lines carrying "the portable path" or "the vendor path" as the same axis,
   with line 360 counted once because it carried two of the phrases. Every one
   is restated so its conclusion follows from source, and every attribution of
   the axis to the plan is removed. **No census, count or citation changes on
   this account, and no fact is withdrawn.** What is withdrawn is a ranking
   between the two halves of item 6.2 that I was not entitled to make.
2. **One of those 37 asserted that the plan supplies the ranking.** My 4.2
   closed "the ranking that plan 5.2 does supply runs the other way from my
   withdrawn sentence: the reference path is the portable one." The plan
   supplies no such ranking, and section 2.2a says the two halves of item 6.2
   are not separable. Corrected at 4.2.
3. **A live defect, and it inverts my escalation 20. GLES is reachable through
   the C API.** `ti_import_opengl_runtime` is a public exported function taking
   a `use_gles` parameter (`c_api/include/taichi/taichi_opengl.h:33-36`) whose
   body sets the GLES override at `c_api/src/taichi_opengl_impl.cpp:26`. My
   scope caveat at 2.9 and my notes entry 50 both said the opposite, on a grep
   of mine that missed the one caller. Rewritten at 2.9 and escalation 20.
4. **Two findings absent from this report, both device selection.**
   `TI_VISIBLE_DEVICE` is an out-of-process device selector
   (`taichi/rhi/vulkan/vulkan_loader.cpp:111-113`), and it collides
   order-dependently with the C API `device_index`. New subsection 2.10.
5. **My downgrade of multi-device selection is withdrawn.** "Practical
   consequence for testing, not for architecture" (2.5, escalation 10) and "no
   gap in it to record" (8.1c) are wrong against plan section 5.1a, where
   choosing between cards at install time is the model. Restated at 2.5 and
   escalation 10.
6. **A positive result nobody had stated, now stated: one ceiling per build is
   what the existing build loop already produces, with no filename change.**
   New at 5.3.3. It closes escalations 4 and 17 as mechanism questions against
   section 5.1a; both are restated to carry forward only what 5.1a leaves open.
7. **The plan section 2.2 caveat on Metal's content** is added at 2.8: what
   Metal publishes is feature admission, and below Apple3 a device loses
   `spirv_has_int64` outright.
8. Three citation corrections to `[V]` sentences of mine: the optimizer line is
   `:2681` not `:2680` (two places), `get_device_score` at
   `vulkan_device_creator.cpp:452` is not a score that dies at the call site,
   and `kUseGles` has two writers rather than one.

Section 8.1d lists every change in this pass and 8.2 lists what was newly read
to make it. Escalations 21, 22 and 23 are new.

**Amended after round-two adversarial review.** Round two judged this report
correct and the pair not complete. Five claims were put to me for adjudication.
Four held and one did not. What changed, in order of weight:

1. My first escalation said "nothing in the tree yet compiles a configure-time
   value into the bitcode". That is **false**, and the counter-example is on the
   clang command line this report already quoted:
   `-D "ARCH_${rtm_arch}"`. Rewritten, not trimmed, at 5.3.1 and escalation 1.
2. My stated mechanism for the threading trap was wrong. The host does not lay
   out `LLVMRuntime` at all. The conclusion survives; the reason does not.
   Corrected at 5.2 and 5.3, and the prebuilt route my wording foreclosed is
   reopened at 5.3.2.
3. "Never set on any path" and "permanently" for
   `spirv_has_physical_storage_buffer` are wrong: a C API entry point I cite
   twice elsewhere installs arbitrary capabilities wholesale. Corrected at 2.4.
4. The ordering census is expanded from three cited sites to the full thirteen,
   including the graded ladder that selects the SPIR-V target environment
   (3.3), and my capability-level tally is corrected from 52 to 54 with the
   exclusions stated.
5. The claim that neither report states the CUDA path never populates the
   capability system is **wrong as to this report** — 2.2 states it at lines
   221-233 and calls it the central gap for brief 5.2. What was genuinely
   missing is the consequence, now stated in one place at 2.7: item 6.2 divides
   into two differently shaped halves along `arch_uses_spirv`.

Section 8.1b lists every change in that pass, and 8.2 lists what was newly
read to make it. Escalations in section 9 keep their numbers; 1, 8 and 9 are
rewritten and 17 and 18 are new.

**Framing pass, after an intermediate amendment to plan section 5.2. Its
first half stands; its second half is withdrawn above.** The three cards define
fidelity classes and are not a vendor target, and section 2 governs: that part
is unchanged and is still in force. Six statements in this report reached sound
conclusions by way of the card list, and one of them was a recommendation
rather than a description. All six are restated so the conclusion follows from
the source, at 2.2, 2.5 twice, 4.2, and escalations 10 and 11; escalation 8 was
already rewritten that way in the amendment pass. **What that pass got wrong is
what it put in the card list's place.** It re-weighted the switched-off 64-bit
pointer machinery at 2.4 and 2.7 by naming one spine the reference and the
other the accelerant, which the current plan rules out. The verified
qualification recorded alongside that re-weighting survives it: the machinery's
scope is argument and return addressing rather than SNode addressing. Two new
subsections record findings put to me and verified: 2.8, both spines detect
tier data and neither publishes it, with Metal as the one backend that
translates hardware classes into portable vocabulary; and 2.9, the C API
replaces OpenGL's detected capabilities wholesale and discards the check that
protects low-end devices. Section 8.1c lists the changes. Escalations 19 and 20
are new.

**Revised after adversarial review.** Three errors in my first pass are
corrected in place and labelled **Correction** where they sit: Metal does call
`set_caps` (2.2), the AMDGPU CMake typo is inert because the file does not
exist (7), and my central premise for 6.1 was too strong because a non-boolean
CMake value already reaches a generated header and selects a bitcode file
(5.2). Five gaps that survived both explore passes are now covered: the absent
arch and version handshake on AOT load (3.2), artefact invalidation (5.6), a
CMake operator-precedence bug (7), the installed package recording nothing
about its own build (4.4), and the capability-ordering contract (3.3).
Section 8.1 lists every change; the escalations in section 9 are renumbered.

Everything below is marked **[V]** verified by reading the code cited, or
**[I]** inferred. Every claim carries a file:line. Where a grep supports a
claim I now state the scope actually searched rather than calling it
exhaustive; the reason is in 2.2.

---

## 1. Backend selection and registration as it stands

### 1.1 There is no registry

**[V]** Backend availability is decided in three separate hardcoded
if/else or switch chains, each wrapped in `#ifdef TI_WITH_*`. There is no
factory map, no registration table, and no dynamic loading of backend code.
The set of possible backends is closed at compile time in each of these three
places independently.

**Chain 1 — the Program constructor.**
`taichi/program/program.cpp:53-161`, `Program::Program(Arch desired_arch)`.

| program.cpp | condition | implementation |
|---|---|---|
| 80-95 | `arch_uses_llvm(config.arch)`, arch != dx12 | `LlvmProgramImpl` |
| 82-92 | arch == dx12 | `Dx12ProgramImpl`, asserts `directx12::is_dx12_api_available()` (87) |
| 96-102 | `Arch::metal` | `MetalProgramImpl`, asserts `metal::is_metal_api_available()` (98) |
| 103-109 | `Arch::vulkan` | `VulkanProgramImpl`, asserts `vulkan::is_vulkan_api_available()` (105) |
| 110-116 | `Arch::dx11` | `Dx11ProgramImpl`, asserts `directx11::is_dx_api_available()` (112) |
| 117-123 | `Arch::opengl` | `OpenglProgramImpl`, asserts `opengl::initialize_opengl(false)` (119) |
| 124-130 | `Arch::gles` | `OpenglProgramImpl`, asserts `opengl::initialize_opengl(true)` (126) |
| 131-133 | otherwise | `TI_NOT_IMPLEMENTED` |

Each `#else` arm is `TI_ERROR("This taichi is not compiled with X")`
(program.cpp:90, 94, 101, 108, 115, 122, 129). Backend headers are
`#ifdef`-guarded at program.cpp:19-43.

The availability probes are **hard assertions, not fallbacks**. A machine
without a working Vulkan loader aborts rather than degrading.

**Chain 2 — AOT module load.** `taichi/aot/module_loader.cpp:31-58`.
Arms for vulkan (32-35), opengl (36-39), gles (40-43), dx11 (44-47),
dx12 (48-51), metal (52-55); `TI_NOT_IMPLEMENTED` at 57. All but dx12 route to
`gfx::make_aot_module`.

**Chain 3 — c_api runtime creation.**
`c_api/src/taichi_core_impl.cpp:247-309`, `ti_create_runtime(TiArch, uint32_t)`.
A switch with `#ifdef`-guarded cases: VULKAN 252-267, OPENGL 268-274,
X64 275-281, ARM64 282-287, CUDA 288-293, METAL 295-301,
`default: TI_CAPI_NOT_SUPPORTED(arch)` 302-305.
(Amended: I gave the METAL range as 294-301. Line 294 is
`#endif  // TI_WITH_LLVM`; the `#ifdef TI_WITH_METAL` is at :295, the case body
at :296-300, the `#endif` at :301.)

The three chains cover **different arch sets**. Chain 1 has dx11 and dx12;
chain 3 has neither. Chain 2 has dx11, dx12 and metal but no LLVM arch at all.
None has amdgpu, which is otherwise a full backend (`taichi/rhi/amdgpu/`,
`cmake/TaichiCore.cmake:207-214`).

### 1.2 The Arch enum is unconditional

**[V]** `taichi/rhi/arch.h:7-12` defines `enum class Arch : int` by x-macro
over `taichi/inc/archs.inc.h`, which lists 12 archs (x64, arm64, js, cuda,
metal, opengl, dx11, dx12, opencl, amdgpu, vulkan, gles). No `TI_WITH_*`
conditions it. Every arch value exists in every build; availability is a
separate question answered by the chains above.

Classification is by hardcoded switch in `taichi/rhi/arch.cpp`:
`arch_is_cpu` 42-48, `arch_is_cuda` 50-52, `arch_uses_llvm` 54-57,
`arch_is_gpu` 59-61 (defined as `!arch_is_cpu`; js is named explicitly as a
CPU at arch.cpp:43, so only opencl falls through by omission, landing on the
GPU side), `arch_uses_spirv` 63-66, `host_arch` 68-76
(compile-time `TI_ARCH_x64` / `TI_ARCH_ARM` only), `default_simd_width` 82-93.

**[V]** There is a second, unrelated arch numbering in the C API.
`c_api/taichi.json:110-123` spells `TiArch` out by hand: reserved 0, vulkan 1,
metal 2, cuda 3, x64 4, arm64 5, opengl 6, gles 7. That ordering has no
relation to `archs.inc.h` (x64 0, arm64 1, js 2, cuda 3, metal 4, opengl 5,
dx11 6, dx12 7, opencl 8, amdgpu 9, vulkan 10, gles 11). dx11, dx12, amdgpu,
js and opencl have no `TiArch` value.

### 1.3 The TI_WITH_* toggles

**[V]** `cmake/TaichiCore.cmake:1-11` holds eleven `option()` calls:
USE_STDCPP(OFF) at :1, then TI_WITH_LLVM(ON), TI_WITH_METAL(ON),
TI_WITH_CUDA(ON), TI_WITH_CUDA_TOOLKIT(OFF), TI_WITH_AMDGPU(OFF),
TI_WITH_OPENGL(ON), TI_WITH_VULKAN(OFF), TI_WITH_DX11(OFF), TI_WITH_DX12(OFF),
TI_WITH_GGUI(OFF). (My first pass listed ten and dropped USE_STDCPP.)

Coerced by platform before use, TaichiCore.cmake:35-75 — APPLE forces
CUDA/OPENGL/AMDGPU off (36-47), non-APPLE forces METAL off (49-52), WIN32
forces AMDGPU off (55-60), VULKAN forces GGUI on (62-64), a missing
`external/glad/src/gl.c` forces OPENGL off (66-69), and `NOT TI_WITH_LLVM`
forces CUDA, CUDA_TOOLKIT and DX12 off (71-75).

Propagated to C++ by **appending `-DTI_WITH_X` to the global
`CMAKE_CXX_FLAGS`**, not by `target_compile_definitions`:
TaichiCore.cmake:94 (LLVM), 103 (CUDA), 109 (AMDGPU), 115 (DX12), 119 (METAL),
123 (OPENGL), 127 (DX11), 131 (VULKAN), 281 (CUDA_TOOLKIT), and again
redundantly in `taichi/rhi/CMakeLists.txt:43, 49, 55, 61`. GGUI is the
exception **within `TaichiCore.cmake`**: `target_compile_definitions` at
`cmake/TaichiCore.cmake:384`, python target only. Across the build it is not
unique — `taichi/rhi/CMakeLists.txt:28` uses
`target_compile_definitions(${TAICHI_DEVICE_API} PUBLIC TI_WITH_GLFW)`. (My
first pass claimed uniqueness across the whole build.)

Linking is per-toggle `add_subdirectory` + `target_link_libraries` on the
`taichi_core` OBJECT library (TaichiCore.cmake:137): LLVM block 148-243
(cpu 192-196, cuda 198-205, amdgpu 207-214, dx12 216-227, llvm 229-235),
gfx 245-248, metal 250-253, opengl 255-258, dx11 260-263, vulkan 265-268.
The RHI half is the same shape in `taichi/rhi/CMakeLists.txt:42-105`.

**[V]** SPIR-V codegen and the gfx runtime are added as subdirectories
unconditionally (`cmake/TaichiCore.cmake:291-292`, under the comment at :286)
but linked only when one of metal/opengl/dx11/vulkan is enabled (:295-298).

**[V]** The c_api source list is assembled the same way at
`cmake/TaichiCAPI.cmake:20-64`. There is **no c_api implementation or public
header for dx11, dx12, amdgpu or gles**.

---

## 2. Device capability detection

### 2.1 What the mechanism is

**[V]** `taichi/rhi/device_capability.h:11-15` declares
`enum class DeviceCapability : uint32_t` by x-macro over
`taichi/inc/rhi_constants.inc.h:8-35`. 25 entries: `reserved` plus 24
`spirv_*`. `DeviceCapabilityConfig` (device_capability.h:20-33) is a
`std::map<DeviceCapability, uint32_t>` with contains/get/set;
`get` returns 0 for a missing key (device_capability.cpp:33-39), so "absent"
and "level 0" are indistinguishable.

Storage is one member on the RHI `Device` base:
`taichi/rhi/public_device.h:617` `DeviceCapabilityConfig caps_{}`, accessed
via `get_caps()` / `set_caps()` at public_device.h:852-857.

### 2.2 What is actually detected, and where

**[V]** `set_caps` has **eight** call sites. Grep over `taichi/` and `c_api/`
with no file-type filter, excluding the declaration at
`taichi/rhi/public_device.h:855`:

| site | scope | what it sets |
|---|---|---|
| `taichi/rhi/metal/metal_device.mm:1168` | rhi | the result of `collect_metal_device_caps` (:1017-1070) |
| `taichi/rhi/vulkan/vulkan_device.cpp:1580` | rhi | `spirv_version = 0x10000` only |
| `taichi/rhi/vulkan/vulkan_device_creator.cpp:876` | rhi | the real Vulkan detection |
| `taichi/rhi/opengl/opengl_device.cpp:529` | rhi | GL / GLES caps |
| `taichi/rhi/dx/dx_device.cpp:565` | rhi | `spirv_version = 0x10300` only |
| `c_api/src/taichi_opengl_impl.cpp:12` | c_api | int64, float64, spirv_version |
| `c_api/src/taichi_core_impl.cpp:331` | c_api | whatever the host passes in |
| `c_api/src/taichi_vulkan_impl.cpp:53` | c_api | spirv_version only (see 2.4) |

Five in the RHI, three in the C API.

**Correction.** My first pass said seven sites and described Metal as building
a config "rather than calling `set_caps`", inside a claim that the grep was
exhaustive. Both are wrong. `MetalDevice::MetalDevice` at
`taichi/rhi/metal/metal_device.mm:1162-1169` calls
`collect_metal_device_caps(mtl_device)` at :1167 and `set_caps(std::move(caps))`
at :1168; returning a config and calling `set_caps` on it are not alternatives.
The cause was a grep that passed `--include=*.cpp --include=*.h` and no
`--include=*.mm`, described afterwards as exhaustive over the directories.

I have therefore removed the word "exhaustive" from every grep claim in this
report and replaced it with the scope actually searched. Re-running the other
greps in this report without the file-type filter: the `get_required_caps`,
`spirv_has_physical_storage_buffer`, `taichi_max_num_snodes` and
`DynamicLoader` censuses are unchanged, since no `.mm` file contains those
symbols. `spirv_has_atomic_int64` does change; see 2.3.

**[V] Nothing in `taichi/rhi/cpu/`, `taichi/rhi/cuda/`, `taichi/rhi/llvm/` or
`taichi/rhi/amdgpu/` sets any capability.**
`ProgramImpl::get_device_caps()` at `taichi/program/program_impl.h:170-172`
returns `{}` and the only override in the tree is
`GfxProgramImpl::get_device_caps` at
`taichi/runtime/program_impls/gfx/gfx_program.cpp:82-85`. So for CPU, CUDA,
AMDGPU and DX12 nothing in the core ever populates the capability set, and
`ti_get_runtime_capabilities` on an x64 or CUDA runtime returns nothing. (A
host can still install values through
`ti_set_runtime_capabilities_ext` — see 2.4 — but nothing on that spine reads
them, so they would be inert.)

**This is a gap on the `arch_uses_llvm` spine, and it is the central one in
section 2.** Restated twice. My original wording reached this conclusion by
naming the three cards, which plan 5.2 says define fidelity classes and nothing
else; the framing pass then called the uncovered spine "the portable path",
which is the axis plan section 2.2a withdraws. Neither is needed. The
capability system is the only vocabulary in the tree for describing what a
device can do, and the spine it does not cover is the one carrying **CPU-only
operation, which plan section 2 makes a first class target rather than a
fallback**. That the three cards happen to sit on the same uncovered spine is
incidental and adds nothing to the finding. Per plan section 5.2 this is
recorded as a gap in the less-served spine and is not evidence that either
spine matters less. The full consequence is at 2.7.

### 2.3 64-bit integer capability per backend (brief 6.2)

**[V]**

| backend | site | condition |
|---|---|---|
| Vulkan | `taichi/rhi/vulkan/vulkan_device_creator.cpp:630-632` | `VkPhysicalDeviceFeatures::shaderInt64` |
| OpenGL | `taichi/rhi/opengl/opengl_device.cpp:509-513` | set for desktop GL, **not** for GLES ("64bit isn't supported in ES profile", line 510) |
| Metal | `taichi/rhi/metal/metal_device.mm:1051-1053` | `feature_64_bit_integer_math` |
| DX11 | `taichi/rhi/dx/dx_device.cpp:563-565` | **never set** |
| CUDA / CPU / AMDGPU / DX12 | — | no capability mechanism at all (2.2) |
| c_api OpenGL | `c_api/src/taichi_opengl_impl.cpp:9` | set unconditionally, **no GLES check** — diverges from `opengl_device.cpp:510`, and 2.9 shows the divergence is reachable through a public entry point |

Consumers: `taichi/codegen/spirv/spirv_ir_builder.cpp:64, 166, 312, 326` and
`taichi/rhi/metal/metal_device.mm:132` (picks MSL 2.3).

**[V]** `spirv_has_atomic_int64` (`taichi/inc/rhi_constants.inc.h:17`) is
never set and never read in the core: no `caps.set` and no `caps.get` for it
anywhere in `taichi/` or `c_api/`. A case-sensitive grep for
`spirv_has_atomic_int64` over the tree excluding `external/`, `build/` and
`modernization/` returns **three** lines — the x-macro entry above, a setter in
the header-only C++ wrapper at `c_api/include/taichi/cpp/taichi.hpp:1121`, and
the Python enum at `python/taichi/lang/enums.py:27`. A fourth site exists under
a different spelling: the generated C enumerator
`TI_CAPABILITY_SPIRV_HAS_ATOMIC_INT64` at
`c_api/include/taichi/taichi_core.h:388`, upper case, which a case-insensitive
search also finds mirrored in the generated docs at
`docs/lang/articles/c-api/taichi_core.md:409`. All are declarations or
pass-through plumbing. (Amended: I wrote that the grep "returns four lines" and
then listed the upper-case enumerator among them. The list was right and the
description of the search that produced it was not. My first pass before that
said "grep returns only the declaration", which understated it. The substantive
point is unchanged across all three versions.)

### 2.4 The 64-bit device-address capability has no live in-tree producer

**Correction.** My first two passes both said this capability is "never set on
any path, on any platform" and that its consumers "permanently" take the false
branch. Both words are wrong, and the counter-example is a function this report
already cites twice, at 2.2 site 7 and again at 4.3. The corrected statement
heads this section; the retracted one is kept below it so the change is legible.

**[V]** `spirv_has_physical_storage_buffer` — the buffer-device-address
capability, i.e. 64-bit pointers inside shaders — has **no live producer inside
the tree**. Both in-tree producer sites are disabled:

- `taichi/rhi/vulkan/vulkan_device_creator.cpp:826`, inside
  `#if !defined(__APPLE__) && false` (guard at 825-827). Comment on line 824:
  "(penguinliong) Temporarily disabled (until device capability is ready)."
- `c_api/src/taichi_vulkan_impl.cpp:48`, inside a `/* */` block spanning
  lines 46-51, preceded by "Will bring it back after devcap." (line 45).

It has twelve live consumers, which take the false branch on every path the
core itself can reach: `taichi/rhi/vulkan/vulkan_device.cpp:1772, 1792, 2147,
2509`; `taichi/codegen/spirv/spirv_ir_builder.cpp:73, 113`;
`taichi/codegen/spirv/spirv_codegen.cpp:783, 2340, 2416, 2491`;
`taichi/runtime/gfx/runtime.cpp:96`;
`taichi/runtime/program_impls/gfx/gfx_program.h:80-83`.

The last decides the kernel argument data layout:
`return "1" + std::string(has_buffer_ptr ? "b" : "-");` — `"1-"` on every path
the core reaches by itself.

**[V] But the branch is not permanent, and the C API can turn it on.**
`ti_set_runtime_capabilities_ext` (`c_api/src/taichi_core_impl.cpp:317-334`)
builds a `DeviceCapabilityConfig` from caller-supplied `TiCapabilityLevelInfo`
values at `:326-330` and installs it wholesale with
`runtime2->get().set_caps(std::move(devcaps))` at `:331`, with **no validation
against the device**. `Runtime::get()` (`c_api/src/taichi_core_impl.h:131`)
returns the same `taichi::lang::Device` whose `caps_`
(`taichi/rhi/public_device.h:617`, get/set at `:852-857`) every one of those
twelve consumers reads — directly, as at `taichi/runtime/gfx/runtime.cpp:95-96`
(`device_->get_caps().get(...)`), or through
`GfxProgramImpl::get_device_caps`, which is
`runtime_->get_ti_device()->get_caps()` at
`taichi/runtime/program_impls/gfx/gfx_program.cpp:82-85`. The header-only C++
wrapper ships a typed setter for this exact capability at
`c_api/include/taichi/cpp/taichi.hpp:1156`.

**[I]** So a host that calls that entry point before kernels are compiled puts
all twelve consumers on the true branch with no source change. Two limits on
that, which I verified rather than assumed: the codegen consumers read the caps
that were current when the kernel was compiled
(`spirv_codegen.cpp:2666` reads `params.caps`), so the override has to precede
compilation; and because nothing validates the value against the device, a host
can assert a capability the underlying device was never created with. Whether
this is an evaluation route, an install-time override hook, or a hazard, the
code does not say. Escalated at 9.9.

**[I]** This means a substantial part of the 64-bit addressing machinery
required by brief 6.2 is already written and compiled, and is switched off at
the capability layer rather than absent. I have not audited whether that code
is correct; I verified only that it is unreachable from inside the core.

**Weighting, twice withdrawn, and now not made at all.** My revision called
this "the SPIR-V side" of 6.2 and treated it as a side axis, on the reasoning
that the named cards sit elsewhere. The framing pass inverted that and called
this machinery "the primary existing form of item 6.2" because these backends
were "the reference path". **Both statements rank the two halves of item 6.2,
and neither the source nor the plan supplies a ranking.** Plan section 2.2a
states that the two halves of the limit are not separable and that work on
either spine is BASE work; section 5.2 states that neither spine is the vendor
spine. What is left when the ranking goes is the description, which is what
this subsection actually verified: machinery for 64-bit pointers exists on
`arch_uses_spirv`, is compiled, and is switched off at the capability layer.
Nothing about the code changed across any of the three versions.

**[V] But its scope is narrower than "64-bit addressing", and I checked which
statements it actually gates before saying so.** The four codegen consumers sit
in `visit(ExternalPtrStmt *)` (`taichi/codegen/spirv/spirv_codegen.cpp:734`,
branch at `:783`, 64-bit pointer arithmetic at `:786-792`),
`compile_args_struct` (`:2326`, branch at `:2340`), `compile_argpack_struct`
(`:2402`, branch at `:2416`) and `compile_ret_struct` (`:2483`, branch at
`:2491`). The fifth, `taichi/runtime/gfx/runtime.cpp:95-96`, gates taking a
physical pointer for an ndarray device allocation, and the sixth,
`gfx_program.h:79-83`, is the argument data layout string. **[V]** SNode and
root-buffer addressing does not pass through any of them: it goes through
`BufferType::Root` bound buffers at `visit(GetRootStmt *)` (`:351`),
`visit(GetChStmt *)` (`:359`) and `visit(SNodeLookupStmt *)` (`:468`), none of
which reads the capability.

**[I]** So both things are true at once, and neither should be overstated. It
is real, working 64-bit pointer machinery on `arch_uses_spirv`, and it covers
ndarray, argpack and return-struct addressing rather than the SNode addressing
that item 6.2 is about. Reviving it would not by itself widen an SNode address.
(The scope half was established by a separate investigation and put to me; I
re-derived it from the enclosing functions rather than accepting it.)

### 2.5 CUDA tier information exists, outside DeviceCapability

**[V]** `taichi/rhi/cuda/cuda_context.cpp:20-86` (the `CUDAContext`
constructor) detects, via the CUDA driver: device name (25-27), compute
capability major and minor (29-33), memory-pool support (35-63), total and
free memory (`get_total_memory` 88-92, `get_free_memory` 94-98), and
`CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_BLOCK_OPTIN` (79-82).

It computes `compute_capability_ = cc_major * 10 + cc_minor` (line 73) and
`mcpu_ = "sm_" + compute_capability_` (line 83), used by NVPTX codegen.
Accessors at `taichi/rhi/cuda/cuda_context.h:73-75` and `61-63`.

**[V]** `get_total_memory()` is **not** on the RHI `Device` base. It is
declared on the LLVM sub-interface at `taichi/rhi/llvm/llvm_device.h:34`,
`TI_NOT_IMPLEMENTED` by default, and overridden at
`taichi/rhi/cuda/cuda_device.h:151-153` and
`taichi/rhi/amdgpu/amdgpu_device.h:120-122`. `taichi/rhi/public_device.h` has
no such member. Its only consumer in the tree is
`taichi/runtime/llvm/llvm_runtime_executor.cpp:612`; nothing in `c_api/` calls
it. (My first pass placed it on the RHI `Device` interface.) This matters for
6.3: a tier-aware selector cannot read device memory through the RHI base
class today; it would have to go through `LlvmDevice` or `CUDAContext`.

**[V] None of these values is ever written into a `DeviceCapabilityConfig`.**
They reach codegen through `CUDAContext` directly.

Two constraints on this channel, which is CUDA-specific and therefore vendor
specific in the sense plan section 2.2a defines. Restated twice: in the framing
pass so the finding stands on the source rather than on which cards the test box
holds, and again in the round-three pass to remove the spine ranking and the
downgrade of the second item.

- **[V]** `cuda_context.cpp:75-77` clamps `compute_capability_` to at most 86,
  so anything newer is silently compiled as sm_86. This is a ceiling on how far
  the CUDA backend scales, and it sits outside the capability system entirely,
  so nothing can see or report it. (Where the currently owned cards fall
  relative to 86 is a fact about the test box, not about the finding: for the
  record, sm_50, sm_61 and sm_86 all pass through unclamped.)
- **[V]** `cuda_context.cpp:21-22` calls `device_get_count` then
  `device_get(&device_, 0)` — **the CUDA device ordinal is hardcoded to 0**,
  and `ti_create_runtime` rejects any non-zero `device_index` for CUDA
  (`c_api/src/taichi_core_impl.cpp:289-290`). Vulkan alone has a device
  selector (`set_vulkan_visible_device`,
  `taichi/rhi/vulkan/vulkan_loader.cpp:144-146`, consumed at
  `vulkan_device_creator.cpp:442-459`), and
  `c_api/src/taichi_core_impl.cpp:254-255` honours `device_index` for Vulkan
  and for nothing else. **Recorded per plan section 5.2 as a gap on the spine
  that lacks it, not as evidence about either spine.**

  **Withdrawn: "practical consequence for testing, not for architecture."** The
  framing pass closed this bullet that way and its 8.1c added "on device
  selection the portable path is the one better served, so there is no gap in
  it to record". Both are wrong against plan section 5.1a, which settles that
  configuration happens at install time, that a machine holding several GPUs is
  the ordinary case rather than an edge one, and that the agent "may need to
  choose between them, or to use more than one". Choosing a device is therefore
  part of the settled deployment mechanism, and an install-time agent that
  cannot name a device through the C API on CUDA is blocked on it. Escalation
  10 carries the question.

  **[V] The pin is on an ordinal, not on a physical card, and that is what
  reconciles it with the plan's empirical statement.**
  `driver_.device_get(&device_, 0)` takes CUDA driver ordinal 0, and which
  physical card that is depends on the driver's own `CUDA_VISIBLE_DEVICES`,
  which is set outside Taichi: `grep -rn 'CUDA_VISIBLE_DEVICES'` over
  `taichi/`, `c_api/` and `python/` returns nothing, so Taichi neither reads nor
  sets it. Two processes with different values of that variable each get a
  different card. Plan section 5.1a records that the owner has run the engine on
  both cards at once and that the observation outranks any code reading; this
  finding does not contradict it, and its severity is one process one card, not
  one machine one card. Recorded because without the distinction the finding
  reads as a challenge to an observation the plan says outranks it.

### 2.6 The capability ABI is generated and order-sensitive

**[V]** `misc/taichi_json.py:102-116` (`load_inc_enums`) globs
`taichi/inc/*.inc.h`, regex-matches `MACRO(case)` lines (108) and assigns each
case an ordinal equal to its **position in the file** (115).
`misc/taichi_json.py:182-185` uses that when a JSON entry declares
`inc_cases`. `c_api/taichi.json:125-129` declares `TiCapability` with
`"inc_cases": "PER_DEVICE_CAPABILITY"`. `misc/generate_c_api.py` writes the
headers, which are then checked in — regeneration is a manual script run, not
a build step.

So `TiCapability` at `c_api/include/taichi/taichi_core.h:380-407` (values 0
through 24) mirrors `taichi/inc/rhi_constants.inc.h:10-34` exactly, and
`c_api/src/taichi_core_impl.cpp:329` casts the raw `uint32` straight to
`DeviceCapability`. **Inserting a capability anywhere but at the end of
`rhi_constants.inc.h` renumbers the C ABI.**

**[V] The ordinals have a second consumer, which my first pass missed.**
`get_offline_cache_key_of_device_caps` at
`taichi/analysis/offline_cache_util.cpp:88-95` serialises `caps.devcaps`
directly at :92 — the raw `std::map<DeviceCapability, uint32_t>`, keyed by the
numeric enum value. It is called at :185 and folded into the kernel hash at
:190. So renumbering silently changes every offline-cache key as well as the C
ABI. Both consumers bind the same positional numbering, and one of them is
invisible from the header.

### 2.7 The capability system covers one of the two backend spines. New.

The facts in this subsection are all stated elsewhere in section 2. What was
missing, and what round two asked for, is the consequence stated in one place.

**[V]** The two spines are defined by predicates in the same file.
`taichi/rhi/arch.cpp:63-66`, `arch_uses_spirv` is
`{opengl, gles, vulkan, dx11, metal}`. `taichi/rhi/arch.cpp:54-57`,
`arch_uses_llvm` is `{x64, arm64, cuda, dx12, amdgpu}`. Every arch is on
exactly one of the two, except `js` and `opencl`, which are on neither.

**[V]** The 25-entry capability vocabulary at
`taichi/inc/rhi_constants.inc.h:8-35` is `reserved` plus 24 `spirv_*` entries.
Every one of the eight `set_caps` call sites in 2.2 is on the SPIR-V spine or
in the C API in front of it. A grep for `set_caps` under `taichi/rhi/cpu/`,
`taichi/rhi/cuda/`, `taichi/rhi/llvm/`, `taichi/rhi/amdgpu/` and
`taichi/rhi/dx12/`, with no file-type filter, returns nothing. So the whole
LLVM spine has an empty `DeviceCapabilityConfig` unless a host installs one
through the C API (2.4), and `ti_get_runtime_capabilities` on an x64 or CUDA
runtime returns nothing.

**[V] It does not read them either.** A grep for `DeviceCapability` and
`get_caps()` across `taichi/codegen/llvm/`, `taichi/runtime/llvm/`,
`taichi/runtime/cuda/`, `taichi/runtime/cpu/`, `taichi/runtime/amdgpu/`,
`taichi/runtime/dx12/`, `taichi/runtime/program_impls/llvm/` and the five RHI
directories above returns four lines, and all four are unused parameters:
`LLVM::KernelCompiler::compile` takes `const DeviceCapabilityConfig &device_caps`
(`taichi/codegen/llvm/kernel_compiler.cpp:32`, declared at
`kernel_compiler.h:22`) and never touches it in the body at `:30-48`, and
`LlvmProgramImpl::make_aot_module_builder` takes
`const DeviceCapabilityConfig &caps`
(`taichi/runtime/program_impls/llvm/llvm_program.cpp:79`, declared at
`llvm_program.h:67`) and dispatches on `config->arch` alone at `:80-95`. The
interface is threaded through the LLVM spine and terminates unread.

**The consequence for item 6.2, which neither pass stated in one place.** The
work item divides into two halves of different shape along that predicate:

| spine | state of the 64-bit machinery | what 6.2 means there |
|---|---|---|
| `arch_uses_spirv`, which carries the portable GPU path | written, compiled, switched off at the capability layer (2.4). Twelve consumers, including the argument-layout decision at `gfx_program.h:79-83` and the 64-bit pointer arithmetic at `spirv_codegen.cpp:786-792`. Scope is ndarray, argpack and return-struct addressing, **not** SNode addressing (2.4) | finish and enable existing code, decide what turns it on (2.4, escalation 9), and separately build the SNode half, which this machinery does not cover |
| `arch_uses_llvm`, which carries CPU-only operation as well as CUDA, AMDGPU and DX12 | no capability representation of any kind exists to switch (2.2). The tier facts that do exist sit outside the system entirely, on `CUDAContext` (2.5) | build the representation first; there is nothing to enable |

**This is recorded as a gap in coverage, per plan 5.2, and is not an argument
for either spine.** Read against plan section 2, the asymmetry falls in the
place that matters most: CPU-only operation is a first class target, and
`taichi/rhi/cpu/` is on the spine with no capability representation at all. The
three cards named in plan 5.2 happen to sit on that same spine, which is
incidental. The finding is that the capability system describes `arch_uses_spirv`
and does not describe `arch_uses_llvm`, and that item 6.3's adaptive selection
has nothing to select on for CPU, CUDA, AMDGPU or DX12 today. The vocabulary
extension this implies is escalated at 9.8, not proposed here.

**Ordering: withdrawn, and nothing replaces it.** The framing pass added a
paragraph here ranking the two halves of item 6.2, on the ground that one spine
was the reference path. **Plan section 2.2a says the two halves of the limit
are not separable and that work on either spine is BASE work, and plan section
5.2 says neither spine is the vendor spine, so no ranking is available from the
plan and none is derivable from the source.** My revision before the framing
pass ranked them the other way, on the strength of the card list; both rankings
are withdrawn and the table above is the whole of what this subsection
concludes. One fact recorded alongside the withdrawn ranking survives it and is
worth keeping in view: what exists on `arch_uses_spirv` covers argument and
return addressing, not SNode addressing, so **a third quantity — the SNode half
— is neither built nor represented on either spine**. Escalation 18 carries
that; whether the plan wants an order between the halves, and on what ground,
is the planner's and the owner's.

### 2.8 The SPIR-V backends already detect tier data and discard it. New.

Put to me in the framing pass as a claim; verified here and it holds, in one
place more strongly than claimed. 2.5 records that CUDA detects tier data and
keeps it outside the capability system. The same is true on `arch_uses_spirv`,
and there the detected data is not merely kept elsewhere, it is thrown away.

**[V] Vulkan asks for memory properties and uses the answer only for handle
types.** `VulkanDevice::create_vma_allocator` calls
`vkGetPhysicalDeviceMemoryProperties(physical_device_, &properties)` at
`taichi/rhi/vulkan/vulkan_device.cpp:2515-2516`, then reads only
`properties.memoryTypeCount` and `properties.memoryTypes[i].propertyFlags` at
`:2518-2532`, to build a vector of external-memory handle-type flags. A grep
for `memoryHeap` across `taichi/rhi/vulkan/` returns nothing: **heap sizes are
never read**. So the one call that would answer "how much memory does this
device have" is made, and the answer is used for something else.

**[V] Vulkan stores full device properties and consumes two fields.**
`vk_device_properties_` (`taichi/rhi/vulkan/vulkan_device.h:739`, accessor at
`:710-712`) is filled at `vulkan_device.cpp:1596`. Its only two consumers in
the whole tree are `limits.maxComputeWorkGroupCount[0..2]` in the dispatch
bounds check at `:1125-1129`, and `limits.timestampPeriod` for profiling at
`:1945`. Everything else in the struct is carried and unused.

**[V] Vulkan reads the device name only into debug logs.**
`physical_device_properties.deviceName` at
`taichi/rhi/vulkan/vulkan_device_creator.cpp:512`, inside an
`RHI_DEBUG_SNPRINTF` / `RHI_LOG_DEBUG` block at `:507-518`; and again at `:438`
when enumerating devices.

**[V] Vulkan also scores devices by class and discards the score.**
`get_device_score` (`vulkan_device_creator.cpp:203-229`) reads
`properties.deviceType`, adding 500 for `VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU`
and 1000 for `VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU` at `:220-225`, plus 100 per
API minor version at `:226`. It is called at `:452` and `:462`, and this is the
closest thing in the tree to a hardware-class judgement, with nothing
downstream able to see it.

**Correction, and it narrows a `[V]` sentence of mine.** I wrote "the score is
a local that dies there". That is true of the call at `:462`, where the loop
ranks devices and keeps only the winner, and **false of the call at `:452`**,
which reads
`else if (get_device_score(devices[id], test_surface))` and uses the score as a
**boolean admission gate on an explicitly requested device**. I re-read
`:442-468`: a device named through `device_index` or `TI_VISIBLE_DEVICE` whose
score is zero leaves `has_visible_device` false at `:443`, and the automatic
pick at `:458-468` runs instead. The only error log on that path, `:448-451`,
covers the out-of-range case only, so the substitution is silent. What dies is
the numeric value; the decision does not. Escalation 22 records the question
this raises under plan section 5.1a.

**[V] OpenGL queries workgroup limits into file-scope globals that nothing
reads.** `taichi/rhi/opengl/opengl_api.cpp:20-21` defines
`int opengl_max_block_dim = 1024;` and `int opengl_max_grid_dim = 1024;`, with
the comment at `:18-19` saying these are the OpenGL specification minima used
"in case glGetIntegerv didn't work properly". `initialize_opengl` fills them at
`:231` and `:234` from `GL_MAX_COMPUTE_WORK_GROUP_COUNT` and
`GL_MAX_COMPUTE_WORK_GROUP_SIZE`. A grep for both names across `taichi/` and
`c_api/` returns six lines: the two definitions, the two `glGetIntegeri_v`
calls and the two `TI_TRACE` lines that print them. **No header declares them
and nothing consumes them.** The claim put to me was that they reach neither
the capability map nor the C API; they in fact reach nothing at all.

**[V] None of it reaches the capability map or the C API.** None of these
values is ever written into a `DeviceCapabilityConfig`, and 4.3 establishes
that there is no memory size, device class or device name anywhere in the C
API surface.

**[V] Metal is the exception, and its MECHANISM is the model.**
`collect_metal_device_caps` (`taichi/rhi/metal/metal_device.mm:1017-1070`)
reads a vendor hardware-class ladder at `:1027-1036`
(`[mtl_device supportsFamily:]` for `MTLGPUFamilyMac2` and Apple families 3
through 7, each rung folding in the one above), derives five named features at
`:1038-1042`, and then writes **only portable vocabulary**: `spirv_version` and
`spirv_has_int8/int16/float16/subgroup_basic` unconditionally at `:1044-1049`,
`spirv_has_int64` under `feature_64_bit_integer_math` at `:1051-1053`,
subgroup vote and ballot at `:1063-1064`, subgroup arithmetic at `:1067`. It
adds no Metal-specific capability entry to `rhi_constants.inc.h`. So a vendor
tier ladder is already being translated into the shared vocabulary and consumed
by generic code.

**[V] Its CONTENT is not the model, and this report did not carry the caveat.**
Added in the round-three pass, put to me and verified line by line. What Metal
publishes is **feature admission, not fidelity**. `:1038` is
`bool feature_64_bit_integer_math = family_apple3;` and `:1051-1053` sets
`spirv_has_int64` only under it, so below Apple family 3 the capability is
absent; `DeviceCapabilityConfig::get` returns 0 for a missing key
(`taichi/rhi/device_capability.cpp:33-39`), and the four consumers at
`taichi/codegen/spirv/spirv_ir_builder.cpp:64, 166, 312, 326` take the
no-64-bit branch, as does the MSL version choice at `metal_device.mm:131-135`.
A hardware generation therefore decides what the system can express, which is
the shape plan section 2.2 calls a violation rather than an optimisation, and
plan section 6.3 now records the mechanism-versus-content distinction as a
finding. My 2.8 previously drew only the positive lesson and did not test it.
**Whether an inherited upstream behaviour falls inside a test the plan states
for proposed changes is not mine to settle; I record the shape and escalate it
at 9.23.**

**[I]** Two consequences for item 6.3, neither of which requires inventing a
vendor vocabulary. Both spines already detect tier-shaped data; neither
publishes it. And Metal shows the shape a publishing step would take: detect
whatever the platform offers, express the consequence in portable terms.
Whether the vocabulary needs new entries at all, or whether the existing ones
plus a published memory size would do, is not mine to settle — escalated at
9.19.

### 2.9 The C API discards OpenGL's low-end protection. New.

2.3 records that `c_api/src/taichi_opengl_impl.cpp:9` sets `spirv_has_int64`
with no GLES check while `taichi/rhi/opengl/opengl_device.cpp:509-513` guards
it on `!is_gles()`. Put to me in the framing pass as something to weigh against
brief sections 2 and 5.3, which put edge hardware first. Weighed, and the fact
is worse than the row in 2.3 says.

**[V] It is a wholesale replacement, not an added entry.** `OpenglRuntime`
(`c_api/src/taichi_opengl_impl.cpp:4-13`) constructs its `GLDevice` member
first, so `GLDevice::GLDevice` (`opengl_device.cpp:506-529`) has already run
its detection and called `set_caps` at `:529`. The C API constructor then
builds a fresh three-entry config at `:8-11` and installs it with
`get_gl().set_caps(std::move(caps))` at `:12`, replacing the whole map. So it
does not only assert `spirv_has_int64` and `spirv_has_float64` unconditionally;
it also **drops** the detected `spirv_has_int16` and `spirv_has_float16`, which
`opengl_device.cpp:515-526` sets from `GL_NV_gpu_shader5`,
`GL_AMD_gpu_shader_int16` and `GL_AMD_gpu_shader_half_float`. Detection is
performed and then overwritten by a constant.

**RETRACTED, and this is the one live defect this pass adds. The scope limit I
stated is false: GLES is reachable through the C API.** My previous wording is
kept below the correction so the change is legible. It read: "There is also no
`TI_ARCH_GLES` case in `ti_create_runtime` ... so the C API cannot ask for GLES
directly. The defect is therefore latent rather than always live, and what
makes it latent is an accident of ordering rather than a check." The clause
about `ti_create_runtime` is true and the conclusion drawn from it is not,
because `ti_create_runtime` is not the only public entry point that reaches an
OpenGL runtime. The cause of my error is in my own notes at entry 50: I wrote
that `set_gles_override` has "no caller anywhere in `taichi/` or `c_api/`".
`grep -rn 'set_gles_override' taichi/ c_api/` returns seven lines, and one of
them is a caller, `c_api/src/taichi_opengl_impl.cpp:26`. This is standing
instruction 9's corollary exactly: my search saw the definition and I did not
check that it saw the whole class.

**[V] The chain, every step opened in this pass.**

| step | site |
|---|---|
| public exported entry point with a `use_gles` parameter | `c_api/include/taichi/taichi_opengl.h:33-36`, `TI_DLL_EXPORT TiRuntime TI_API_CALL ti_import_opengl_runtime(TiOpenglRuntimeInteropInfo *interop_info, bool use_gles);` |
| the argument sets the override | `c_api/src/taichi_opengl_impl.cpp:26`, `set_gles_override(use_gles)`, defined at `taichi/rhi/opengl/opengl_api.cpp:35-37`, writing the file-static `use_gles_override` at `:31-33` |
| then the ordinary runtime is created | `taichi_opengl_impl.cpp:28`, `return ti_create_runtime(TI_ARCH_OPENGL, 0);` → `new OpenglRuntime` at `c_api/src/taichi_core_impl.cpp:271` |
| whose `GLDevice` member asks for desktop GL | `GLDevice::GLDevice`, `opengl_device.cpp:506-507`, `initialize_opengl(false, true)` |
| and the override replaces that argument | `opengl_api.cpp:56-59`, `if (use_gles_override.has_value()) { use_gles = use_gles_override.value(); unset_gles_override(); }` |
| `kUseGles` becomes true | `opengl_api.cpp:239`, `kUseGles = use_gles;` |
| detection then correctly withholds 64-bit | `opengl_device.cpp:509-513`, `if (!is_gles())`, comment `:510` "64bit isn't supported in ES profile" |
| and the C API asserts it anyway | `taichi_opengl_impl.cpp:8-12`, a fresh three-entry config installed wholesale, `spirv_has_int64` at `:9`, `spirv_has_float64` at `:10` |

So a host calling `ti_import_opengl_runtime(info, true)` gets a runtime whose
own device detection has just refused 64-bit integer and float support, with
both asserted over the top and the detected 16-bit entries dropped in the same
replacement. **The mis-declaration is live through a documented public
function**, not latent, and not dependent on any accident of ordering.

**[V] The converse case, which is the same ordering seen from the other side.**
The override is consumed at `opengl_api.cpp:56-59`, which sits **after** the
early return at `:46-54`. If `initialize_opengl` has already run in the
process, `supported` has a value, the function returns at `:48` or `:52`, and
`use_gles_override` is never read and never cleared. `ti_get_available_archs`
probes OpenGL at `c_api/src/taichi_core_impl.cpp:193` through
`is_opengl_available` (`:21-27`), which calls `is_opengl_api_available()` with
its default argument; the default is `use_gles = false`
(`taichi/rhi/opengl/opengl_api.h:13`) and it forwards to
`initialize_opengl(use_gles, true)` (`opengl_api.cpp:244-246`). So a host that
enumerates archs before importing a runtime — which is the install-time
sequence plan section 5 describes — locks `kUseGles` false, and its later
`ti_import_opengl_runtime(info, true)` **silently loses the GLES request** and
runs desktop GL.

**[V] Correction to an adjacent `[V]` sentence of mine.** I wrote that
`kUseGles` "is set once at `:239`". There are two writers:
`opengl_api.cpp:239` and `opengl_api.cpp:254`, inside `reset_opengl`, whose
only caller in the tree is
`taichi/runtime/program_impls/opengl/opengl_program.cpp:29`. The in-file
comment at `opengl_api.cpp:23` says "kUseGles is set at most once in
initialize_opengl below", which is true of that function and not of the
variable. `reset_opengl` also clears `supported` at `:253` and calls
`unset_gles_override()` at `:257`.

**[I]** Composed consequence, and I mark it inferred because I read the paths
rather than ran them: whichever of the two sequences a host takes, one of two
documented behaviours is wrong, and which one fires depends on whether an
unrelated API call was made earlier in the process.

**[I]** Weighed against plan sections 2 and 5.3, which put edge hardware first
and make modern capability an opportunity rather than a prerequisite: GLES is
the profile the low end presents, the detection at `opengl_device.cpp:509-513`
is exactly the protection those sections ask for, and the C boundary discards
it on a path reached by calling a documented function with a documented
argument. It also engages plan section 2.2 in the harder direction: the RHI
declines to claim a capability the hardware lacks and the C API then claims it,
which is hardware being told it can do something it cannot. Escalated at 9.20,
which is rewritten because the question has changed.

### 2.10 There is a second device-selection channel, and it is an environment variable. New.

Absent from this report, from my notes and from every prior file in this
territory. Put to me in the round-three pass and verified here.
`grep -rn 'TI_VISIBLE_DEVICE\|visible_device_id' modernization/investigation/`
returns nothing outside this pass's own files.

**[V] The channel.** `taichi/rhi/vulkan/vulkan_loader.cpp:111-113`, at the end
of `VulkanLoader::init`:

```
    const char *id = std::getenv("TI_VISIBLE_DEVICE");
    if (id) {
      set_vulkan_visible_device(id);
    }
```

It is consumed at `taichi/rhi/vulkan/vulkan_device_creator.cpp:442-456`:
`std::stoi` at `:445`, a range check against `device_count` at `:446` with the
log `"TI_VISIBLE_DEVICE=%d is not valid, found %d devices available"` at
`:448-451`, and admission of `devices[id]` at `:452-455` only if
`get_device_score` returns non-zero, falling through to the automatic pick at
`:458-468` otherwise.

**[V] It matters because plan section 5.1a puts the configuration agent outside
the process.** An environment variable read at loader init is a channel an
external install-time agent can use with no C API call and no source change. It
is the second such channel in this territory: the first is `TI_LIB_DIR`, read
by `runtime_lib_dir()` at `taichi/util/lang_util.cpp:35-45`, which this report
already cites at 4.2 for module location. So on Vulkan there are **two** routes
to naming a device and 2.5 covers one of them. Where a spine or backend lacks
such a route, that is a gap recorded per plan section 5.2, and nothing here
ranks the spines.

**[V] The selector is a process-global singleton.**
`set_vulkan_visible_device` (`vulkan_loader.cpp:144-146`) writes
`VulkanLoader::instance().visible_device_id`. `VulkanLoader::instance()` is a
function-local static (`vulkan_loader.h:14-17`) and `visible_device_id` is a
plain public member at `:32`. Every runtime in the process shares one selector.
Serial creation works because the value is consumed during construction, at
`vulkan_device_creator.cpp:442`.

**[V] The environment overwrites the caller's argument, order-dependently, and
the order is the one that loses the argument on the first runtime.**
`VulkanLoader::init` wraps its whole body in `std::call_once` at
`vulkan_loader.cpp:87-115`, and the environment read sits inside that body at
`:111-113`. `VulkanDeviceCreator::VulkanDeviceCreator` calls
`VulkanLoader::instance().init()` at `vulkan_device_creator.cpp:236`. That
constructor runs as a member of `VulkanRuntimeOwned`
(`c_api/src/taichi_vulkan_impl.h:42-43`), which `ti_create_runtime` constructs
at `c_api/src/taichi_core_impl.cpp:256-262` — **after** it has written the
caller's `device_index` at `:254-255`.

**[I]** So on the first Vulkan runtime created in a process, with
`TI_VISIBLE_DEVICE` set, the `call_once` body overwrites the `device_index` the
caller passed. On the second and later runtimes the `call_once` has already
fired and the caller's argument stands. Which of the two wins depends on
whether `init()` ran earlier, for instance through `is_vulkan_api_available()`
(`vulkan_loader.cpp:140-142`) reached from `ti_get_available_archs` at
`taichi_core_impl.cpp:178`. Marked inferred because I read the paths rather than
ran them; every link in them is `[V]`.

**Scope, stated so this is not misread.** Plan section 5.1a records that
concurrent operation across two cards is confirmed empirically by the project
owner, that this outranks any code-reading conclusion, and that no agent needs
to investigate it. **I am not investigating or contradicting that.** This
finding is about which of two inputs names the device for one runtime, not
about whether two runtimes can run at once. Escalated at 9.21.

---

## 3. What the AOT module path can and cannot do today

### 3.1 There are two parallel AOT paths, with two module formats

**[V]**

| | gfx / dx12 | LLVM |
|---|---|---|
| entry point | `aot::Module::load` (`taichi/aot/module_loader.cpp:31-58`) | `capi::LlvmRuntime::load_aot_module` (`c_api/src/taichi_llvm_impl.cpp:78-121`) |
| factory | `gfx::make_aot_module` (`taichi/runtime/gfx/aot_module_loader_impl.cpp:194-198`) | `LLVM::make_aot_module` (`taichi/runtime/llvm/llvm_aot_module_loader.cpp:94-98`) |
| implementation | `gfx::AotModuleImpl` (aot_module_loader_impl.cpp:23-190) | `LLVM::LlvmAotModule` (`taichi/runtime/llvm/llvm_aot_module_loader.h:18-92`) |
| on-disk format | `metadata.json` + one `.spv` per task | `LlvmOfflineCacheFileReader` + `graphs.tcb`, binary |
| load from memory | yes, `.tcm` zip via `ti_create_aot_module` (`c_api/src/taichi_core_impl.cpp:641-660`) | no; constructor takes a directory path (`llvm_aot_module_loader.h:20-30`) |
| capability check | present but dead (3.2) | none at all |
| SNode trees | hardcoded to 1 (`aot_module_loader_impl.cpp:136-139`, comment: "we only support a single SNodeTree during AOT") | `cache_reader_->get_num_snode_trees()` (`llvm_aot_module_loader.h:48-50`), loop at `taichi_llvm_impl.cpp:114-118` |
| root size | `ti_aot_data_.root_buffer_size` (`aot_module_loader_impl.cpp:106-108`) | `return 0` (`llvm_aot_module_loader.h:40-42`) |

`aot::Module::load` (chain 2 of section 1.1) has **no arm for any LLVM arch**.
The LLVM path bypasses it entirely. The two are dispatched by the virtuals on
the c_api `Runtime` base: `load_aot_module` at
`c_api/src/taichi_core_impl.h:133-140`, marked
`[[deprecated("create_aot_module")]]`, and `create_aot_module` at 142-145
whose base body is `TI_NOT_IMPLEMENTED`. `GfxRuntime` implements
`create_aot_module` (`c_api/src/taichi_gfx_impl.cpp:7-35`); `LlvmRuntime`
overrides `load_aot_module` instead and never implements `create_aot_module`.
Upstream comment at `c_api/src/taichi_core_impl.cpp:629-630`: "Should call
`create_aot_module` directly after all backends adapted to it."

**[V]** Failure shape in `aot::Module::load`: if the arch matches an arm but
that `TI_WITH_X` is undefined, the `#ifdef` body vanishes, control falls out
of the if/else chain and reaches `TI_NOT_IMPLEMENTED` (module_loader.cpp:57)
with no message naming the arch.

### 3.2 No handshake of any kind exists on AOT load

**[V]** `aot::Module` declares three pure virtuals at
`taichi/aot/module_loader.h:84-86`: `arch()`, `version()` and
`get_root_size()`. The first two are the natural compatibility handshake. Both
are unreachable.

`version()` overrides, all three of them:

| impl | site | body |
|---|---|---|
| gfx | `taichi/runtime/gfx/aot_module_loader_impl.cpp:114-116` | `TI_NOT_IMPLEMENTED` |
| dx12 | `taichi/runtime/dx12/aot_module_loader_impl.cpp:67-69` | `TI_NOT_IMPLEMENTED` |
| LLVM | `taichi/runtime/llvm/llvm_aot_module_loader.h:36-38` | `return 0` |

`arch()` overrides:

| impl | site | body |
|---|---|---|
| gfx | `aot_module_loader_impl.cpp:111-113` | `device_api_backend_` |
| dx12 | `dx12/aot_module_loader_impl.cpp:64-66` | `device_api_backend_` |
| LLVM | `llvm_aot_module_loader.h:32-34` | `executor_->get_config().arch` |

**Neither is ever called.** Grep for `version()` across `taichi/aot/`,
`taichi/runtime/gfx/`, `taichi/runtime/llvm/`, `taichi/runtime/dx12/` and
`c_api/src/` returns exactly four lines: the declaration and the three
overrides. A pattern grep for `arch()` invoked on a module object across
`taichi/` and `c_api/` returns nothing.

Even if `arch()` were called it would answer nothing. `device_api_backend_` is
the arch the *caller* passed in at `taichi/aot/module_loader.cpp:31`, and the
LLVM one is the running executor's own arch. Neither is read from the module.
And the module file records no arch at all: the serialised field list is
`TI_IO_DEF(kernels, fields, required_caps, root_buffer_size)`
(`taichi/runtime/gfx/aot_utils.h:23`), mirrored at
`taichi/aot/module_data.h:135`.

**[I]** So an AOT module built for Vulkan can be handed to an OpenGL runtime
and nothing in the load path will object.

This reframes what 6.3 faces on the AOT side. My first pass presented the dead
capability check (3.3) as *the* gap. It is one of three, and the only one that
was wired even half-way. The other two were never wired.

**[V]** The pattern exists one layer down and was not lifted to the module:
the *kernel* level does carry and check an arch, at
`taichi/compilation_manager/kernel_compilation_manager.cpp:225`
(`TI_ASSERT(loaded->arch() == arch)`),
`taichi/runtime/cuda/kernel_launcher.cpp:191` and
`taichi/runtime/amdgpu/kernel_launcher.cpp:154`.

### 3.3 The capability gate for AOT modules exists and is inert

Of the three handshakes in 3.2 this is the one that was wired, and it is dead
at the last step.

The chain exists:

1. **[V]** Producer:
   `taichi/runtime/gfx/aot_module_builder_impl.cpp:22-24` writes the builder's
   `DeviceCapabilityConfig` into `TaichiAotData::required_caps` as
   `{capability name -> uint32 level}`.
2. **[V]** Storage: `taichi/runtime/gfx/aot_utils.h:20`, serialised at line 23
   via `TI_IO_DEF`.
3. **[V]** Written to `metadata.json` at
   `taichi/runtime/gfx/aot_module_builder_impl.cpp:57-61`.
4. **[V]** Read back into `ti_aot_data_` at
   `taichi/runtime/gfx/aot_module_loader_impl.cpp:43-46`.
5. **[V]** Consumer: `c_api/src/taichi_gfx_impl.cpp:18-29` compares the
   module's required caps against the live device's caps and returns
   `TI_ERROR_INCOMPATIBLE_MODULE` on mismatch.

**[V] There is exactly one producer, and the generic struct is dead.** Grep for
`required_caps` across `taichi/` and `c_api/` finds a single write in the whole
tree: `aot_module_builder_impl.cpp:23`, into `gfx::TaichiAotData`. The generic
field `aot::ModuleData::required_caps` (`taichi/aot/module_data.h:125`,
serialised at :135) is **never written by anybody**. Its only subclass anywhere
is `ModuleDataDX12 : public aot::ModuleData`
(`taichi/runtime/dx12/aot_module_builder_impl.h:11`), which adds `dxil_codes`
and populates no caps. (My first pass called `aot::ModuleData` a generic mirror
of the gfx struct. There are two structs; only the gfx one is live, so the
declaration side is thinner than I said.)

**[V] The chain is broken between steps 4 and 5.**
`aot::Module::get_required_caps()` at `taichi/aot/module_loader.h:97-100`
returns a static empty `DeviceCapabilityConfig`. Grep over `taichi/` and
`c_api/` for `get_required_caps` returns exactly two hits: that definition and
the c_api call site at `taichi_gfx_impl.cpp:21`. **No subclass overrides it.** `gfx::AotModuleImpl` overrides `get_graph`, `get_root_size`,
`arch`, `version`, `make_new_kernel`, `make_new_kernel_template` and
`make_new_field` (aot_module_loader_impl.cpp:89-169) but not
`get_required_caps`. So `required_devcaps.devcaps` is always empty at
`taichi_gfx_impl.cpp:22` and the loop body never executes.

**[V]** `str2devcap` (`taichi/rhi/device_capability.cpp:6-13`) is the inverse
of the `to_string` the builder uses at
`aot_module_builder_impl.cpp:23`, but its only callers are
`translate_devcaps` and `make_aot_module_builder`
(`taichi/program/program.cpp:530, 544`). The loader never calls it. That is
precisely the missing link: the strings survive the round trip into a
`std::map<std::string, uint32_t>` and stop there.

#### The `!=` comparison is a contract question, not a defect

**Correction.** My first pass wrote that the comparison at
`c_api/src/taichi_gfx_impl.cpp:25` — `current_version != required_version`,
exact equality — means "a device exceeding the module's requirement would be
rejected" and that such a device "should presumably be accepted". The second
half assumes an ordering the published contract explicitly declines to
promise, which is me supplying an assumption rather than reading one.

**[V]** `c_api/include/taichi/taichi_core.h:409-416`:

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

The same sentence is in the published documentation at
`c_api/docs/taichi/taichi_core.h.md:281`. It is authored prose, not generated:
the `c_api/taichi.json` entry for this structure (:131-143) carries the two
fields and no description. Against that contract, `!=` is the only relation the
type supports, and changing it to `>=` is a change to the public contract
rather than a bug fix.

Two further facts, which cut in opposite directions and which I record because
they are what makes the decision non-obvious.

**[V] The tree already relies on ordering internally, in thirteen places.**
Amended: I cited three sites as examples and gave no count. The full census,
grepped over `taichi/` and `c_api/` including `.mm`, for an ordered operator
applied to a `spirv_version` value — this is the only capability in the
vocabulary that is ever compared with `<`, `>`, `<=` or `>=`; every other one
is read for truth:

| operator | sites |
|---|---|
| `<` | `taichi/codegen/spirv/spirv_ir_builder.cpp:556, 625, 671, 679, 741, 777`; `taichi/rhi/vulkan/vulkan_device_creator.cpp:577` |
| `>` | `taichi/codegen/spirv/spirv_codegen.cpp:1908` |
| `>=` | `taichi/codegen/spirv/spirv_ir_builder.h:424`; `taichi/codegen/spirv/spirv_codegen.cpp:2669, 2671, 2673, 2675` |

Seven, one and five. Thirteen.

**[V]** The four I had not cited are one construct, and it is the strongest
instance in the tree. `KernelCodegen::KernelCodegen`
(`taichi/codegen/spirv/spirv_codegen.cpp:2661`) reads `spirv_version` once from
`params.caps` at `:2666` and walks a graded ladder — `>= 0x10600`, `>= 0x10500`,
`>= 0x10400`, `>= 0x10300`, else — at `:2669-2678`, choosing `spv_target_env`
between `SPV_ENV_VULKAN_1_3`, `_1_2`, `_1_1_SPIRV_1_4`, `_1_1` and `_1_0`. That
value configures the optimizer for the whole module at `:2681`. This is not an
incidental comparison; it is a monotone ladder over the capability level, and
it is the clearest evidence that the engine treats the value as graded while
the C API documentation declines to promise that it is.

(Corrected in the round-three pass: I gave the optimizer line as `:2680` here
and again in the section 6 map. I re-read `spirv_codegen.cpp:2660-2681`.
`:2666` reads `spirv_version` into a local, `:2669-2679` is the ladder, `:2680`
is blank, and `:2681` is
`spirv_opt_ = std::make_unique<spvtools::Optimizer>(target_env);`.)

The single site I did cite from that block, `:2673`, is one rung of it.

**[V] No capability in the tree today would be mishandled by `>=`.** Amended:
my figures were 29 / 10 / 13, totalling 52, and I stated no exclusions in a
revision whose whole remedy was to state the scope actually searched. The
correct tally, grepping `\.set\((taichi::lang::)?DeviceCapability::` across
`taichi/` and `c_api/` including `.mm`, is **54 textual sites**:

| level written | sites | of which not live |
|---|---|---|
| `true` | 30 | 2 compiled out — `vulkan_device_creator.cpp:826`, `c_api/src/taichi_vulkan_impl.cpp:48` (2.4) |
| `1` | 10, all `metal_device.mm` | 2 commented out — `metal_device.mm:1058, 1059` |
| a `spirv_version` value | 14 | 1 is a build target, not a device level — `program.cpp:536` (3.4) |

The `spirv_version` values written are 0x10000, 0x10300, 0x10400 and 0x10500.
Excluding the five above leaves 49 live device-level writes. Every one of them
is a boolean stored as `true` or `1`, or a packed `spirv_version`. For a
boolean stored as 1, `current >= required` is the correct relation — "device
has it, module needs it" passes, "device lacks it, module needs it" fails. For
`spirv_version` it is also correct. The conclusion is unchanged by the
recount; only the arithmetic behind it was wrong.

**[I]** So the objection to `>=` is not about any capability that exists. It is
that the disclaimer reserves the right to introduce an unordered enumeration
later, which `>=` would silently mishandle. That is a real reason to leave
`!=` alone and a real reason the decision belongs to the project owner. Moved
to Escalations as a contract question.

### 3.4 Target capability sets are already an explicit build input

**[V]** `translate_devcaps` at `taichi/program/program.cpp:514-539` parses a
`std::vector<std::string>` of the form `spirv_has_int8` or
`spirv_version=1.3` (comment 515-517), defaulting `spirv_version` to 0x10300
when absent (535-537). `Program::make_aot_module_builder`
(program.cpp:541-557) passes the result into
`program_impl_->make_aot_module_builder(cfg)`.

So the *target* capability set for an AOT module is already specified
explicitly rather than probed from the local device. That is the closest thing
in the tree to "build this module for tier X", and it is the natural
counterpart to a working version of 3.2.

Upstream caveat at program.cpp:545-547: building a Metal AOT module still
requires being on macOS, because the runtime backend and the target AOT
backend are coupled.

### 3.5 `taichi/aot/` is thin

**[V]** Seven files, 836 lines total (183+81+139+125 header, 146+68+94
source). Globbed unconditionally into `TAICHI_CORE_SOURCE` at
`cmake/TaichiCore.cmake:86`, with no `TI_WITH_*` guard, which is why
`module_loader.cpp:3-4` can include the gfx and dx12 loader headers
unconditionally.

---

## 4. Adaptive module loading: what exists to build on

### 4.1 No shared-object plugin mechanism

**[V]** `DynamicLoader` is the only dlopen wrapper. The header actually in
use is `taichi/common/dynamic_loader.h`, implemented in
`taichi/common/dynamic_loader.cpp`; every include in the tree names that one —
`taichi/rhi/cuda/cuda_driver.h:5` and `cuda_driver.cpp:3`,
`taichi/rhi/amdgpu/amdgpu_driver.h:5` and `amdgpu_driver.cpp:3`,
`taichi/rhi/vulkan/vulkan_loader.h:7`. (My first pass anchored this on
`taichi/system/dynamic_loader.h:12-34`, the duplicate copy nothing includes;
see section 7.)

Construction sites, all of them: `taichi/rhi/cuda/cuda_driver.cpp:34, 89, 98,
107`, `taichi/rhi/amdgpu/amdgpu_driver.cpp:33`,
`taichi/rhi/vulkan/vulkan_loader.cpp:98`. Every one loads a **vendor driver
library**. Nothing loads Taichi's own compiled code as a shared object.

### 4.2 But the LLVM backend already loads a per-arch module at run time

**[V]** `TaichiLLVMContext::module_from_file`
(`taichi/runtime/llvm/llvm_context.cpp:358-...`) loads LLVM bitcode from
`fmt::format("{}/{}", runtime_lib_dir(), file)` (361-362). The filename comes
from `get_runtime_fn(Arch)` at `llvm_context.cpp:209-211`:
`fmt::format("runtime_{}.bc", arch_name(arch))`. `runtime_lib_dir()` is
`taichi/util/lang_util.cpp:30-47` — the global `compiled_lib_dir` if set,
otherwise `$TI_LIB_DIR`, error if neither.

Those `.bc` files are produced by the `COMPILE_LLVM_RUNTIME` loop at
`taichi/runtime/llvm/runtime_module/CMakeLists.txt:29-31` and installed at
line 13 there and again at `cmake/TaichiCAPI.cmake:179-184`.

**[V]** This is the one place in the tree where Taichi loads a variant of its
own compiled code at run time from an installed directory, selected by
hardware. Today the selector is `arch_name(arch)` alone — one module per arch,
no tier, no capability.

**Re-argued, framing pass.** My revision closed this subsection with: "For the
LLVM family, which is where the brief's named target cards live, this is where
adaptive module loading plausibly starts, not at `aot::Module::load`." That is
a recommendation resting on the card list, and plan 5.2 now says the card list
defines fidelity classes and nothing else. The sentence is withdrawn as
written. I have not withdrawn the underlying observation, because it does not
need the cards, but it has to be stated as a description of two loci rather
than a starting point for one:

- **[V]** There are two places where a variant of Taichi's own compiled output
  is selected after the build. This one, `get_runtime_fn` +
  `module_from_file`, selecting a `.bc` by arch name, which serves
  `arch_uses_llvm` — **including the CPU backend, which brief section 2 makes a
  first class target, not a fallback**. And the AOT loader,
  `aot::Module::load` (`taichi/aot/module_loader.cpp:31-58`), which serves
  `arch_uses_spirv` and carries `.spv` per task plus a `.tcm` archive readable
  from memory (3.1).
- **[V]** They are in different states. The `.bc` locus has a working
  parameterised production, install and selection chain (5.3.1) and no tier or
  capability in the selector. The AOT locus has a capability channel that is
  built, serialised, deserialised and then never read (3.3), and no arch or
  version handshake at all (3.2).
- **[I]** So item 6.3 has to be answered in both places, and the two need
  different work: the `.bc` locus needs a richer selector on a mechanism that
  works, and the AOT locus needs its existing channel connected. **Nothing here
  ranks them, and nothing in the plan ranks them either.**

**Withdrawn in the round-three pass, and it is the sharpest instance of the
framing error.** The bullet above previously closed: "and the ranking that plan
5.2 does supply runs the other way from my withdrawn sentence: the reference
path is the portable one." That is a statement about what the governing
document says, and **the document does not say it**:
`grep -n 'reference path\|accelerant' PROJECT-PLAN.md` returns nothing, and
plan section 5.2 as corrected on 2026-09-09 states that neither spine is the
vendor spine and that a better-served spine is never evidence that the other
matters less. Plan section 2.2a adds that the two halves of item 6.2 are not
separable. So there is no ranking to cite. Under standing instruction 7 a
verified citation does not verify the claim attached to it; here the claim was
attached to a section that does not contain it, which is the same failure one
level up. The two loci are described above and left unranked.

**[V]** One precedent for varying a bitcode filename by *detected hardware*
rather than by arch: the AMDGPU device libraries at
`taichi/runtime/llvm/llvm_context.cpp:628-655` include
`"oclc_isa_version_" + isa_version + ".bc"`.

Other bitcode loaded the same way: `slim_libdevice.<cuda_major>.bc`
(`llvm_context.cpp:213-218`, installed `cmake/TaichiCore.cmake:429-432`) and
`cuda_runtime-cuda-nvptx64-nvidia-cuda-sm_60.bc`
(`taichi/util/lang_util.cpp:18-28`, loaded `llvm_context.cpp:574-591`), whose
build rule at `runtime_module/CMakeLists.txt:17-26` is commented out at 36-43
so the file is never produced and the link is skipped.

### 4.3 Capability query and override already cross the C boundary

**[V]** `ti_get_runtime_capabilities`
(`c_api/src/taichi_core_impl.cpp:336-...`) reads `Device::get_caps()`.
`ti_set_runtime_capabilities_ext` (317-334) writes them via `set_caps` (331).
`ti_get_available_archs` (171-205) reports which archs the binary can run,
built from six free functions at the top of the same file:
`is_vulkan_available` 13-19, `is_opengl_available` 21-27,
`is_cuda_available` 29-35, `is_x64_available` 37-45, `is_arm64_available`
47-53, `is_metal_available` 55-61. Each is `#ifdef TI_WITH_X` around a real
probe, `false` otherwise; x64 and arm64 are pure preprocessor with no probe.
The list omits gles, dx11, dx12 and amdgpu (179-196).

**[I]** `ti_get_available_archs` is the nearest thing to the plan's
install-time loader examining the system. It answers "which arch" and never
"which tier of that arch"; there is no memory size, compute capability or
device name anywhere in the C API. And see 4.4 — at the header level it does
not reliably answer "which arch" either.

**[V] It is also not free of side effects, which matters because plan section 5
puts it first in the natural sequence.** Its OpenGL probe at
`c_api/src/taichi_core_impl.cpp:193` runs `initialize_opengl(false, true)`
through two default arguments (`taichi/rhi/opengl/opengl_api.h:13`,
`opengl_api.cpp:244-246`), which latches `supported` and `kUseGles` for the
rest of the process (`opengl_api.cpp:238-239`), and its Vulkan probe at `:178`
runs `VulkanLoader::init` (`vulkan_loader.cpp:140-142`), which fires the
`std::call_once` that reads `TI_VISIBLE_DEVICE`. Both consequences are in 2.9
and 2.10; both are ordering-dependent, and enumerating archs first is the order
an install-time loader would naturally take.

### 4.4 The installed package records nothing about how it was built

Brief section 5 puts a loader at install time. What an installed Taichi tells
that loader about itself is squarely in my territory, and my first pass did not
look at it.

**[V]** `c_api/include/taichi/taichi.h` is 29 lines. After `taichi_platform.h`
(:5) and `taichi_core.h` (:7) it gates every per-backend public header on a
`TI_WITH_*` macro:

| line | macro | header |
|---|---|---|
| 9-11 | `TI_WITH_VULKAN` | `taichi_vulkan.h` |
| 13-15 | `TI_WITH_OPENGL` | `taichi_opengl.h` |
| 17-19 | `TI_WITH_CUDA` | `taichi_cuda.h` |
| 21-23 | `TI_WITH_CPU` | `taichi_cpu.h` |
| 25-27 | `TI_WITH_METAL` | `taichi_metal.h` |

These are **consumer-side** macros, and the installed package defines none of
them. `cmake/TaichiConfig.cmake.in` is five lines: `@PACKAGE_INIT@`, an include
of `TaichiTargets.cmake`, and `check_required_components("Runtime")`.
`cmake/TaichiTargets.cmake:56-88` declares one `IMPORTED` `taichi_c_api` target
and sets only `IMPORTED_LOCATION`, `IMPORTED_SONAME` (or `IMPORTED_IMPLIB` on
Windows) and `INTERFACE_INCLUDE_DIRECTORIES`. There is no
`INTERFACE_COMPILE_DEFINITIONS`, no capability data and no tier data anywhere
in the installed CMake package.

**[I]** So a downstream consumer that includes `taichi/taichi.h` gets
`taichi_core.h` and nothing else, unless it defines the macros itself by
guessing what the binary was built with.

**[V] `TI_WITH_CPU` is defined nowhere in the repository.** Grep across all
C++, CMake, Python and JSON sources outside `external/` returns exactly two
lines: `c_api/include/taichi/taichi.h:21` and `:23`, the `#ifdef` and its
`#endif` comment. So `taichi_cpu.h` is unreachable through the umbrella header
in every configuration, even though `cmake/TaichiCAPI.cmake:31-33` installs it
whenever `TI_WITH_LLVM` is on — which is the macro that actually gates the CPU
c_api implementation.

**[I]** This is the fourth re-expression of the toggle list in my territory,
after `cmake/TaichiCore.cmake:1-11`, the `#ifdef` chains of section 1.1, and
`cmake/TaichiCAPI.cmake:31-60`. It is the one facing the outside world, and it
is the concrete form of the install-time question in brief section 5.

---

## 5. Build-time parameterisation (brief 6.1)

### 5.1 Where the constant lives and who uses it

**[V]** Grep over the whole tree excluding `external/` and `modernization/`,
with no file-type filter, re-run for this revision:
`taichi_max_num_snodes` appears at exactly three places —
`taichi/inc/constants.h:12` (the declaration),
`taichi/codegen/llvm/struct_llvm.cpp:266` (the assertion), and
`taichi/runtime/llvm/runtime_module/runtime.cpp:567, 568, 569` (the three
arrays `element_lists`, `node_allocators`, `ambient_elements`).
`kMaxNumSnodeTreesLlvm` at `constants.h:13` and `runtime.cpp:562, 563`.

**None of these is in `taichi/rhi/`, `taichi/aot/`, `c_api/` or `cmake/`.**
6.1 does not touch my territory at its use sites. It touches my territory
entirely at the threading mechanism and the packaging.

### 5.2 A non-boolean CMake value already reaches a generated header and selects a bitcode file. `CUDA_VERSION`.

**This corrects the central premise of my first pass.** I wrote that a numeric
SNode ceiling "would be the first non-boolean project-level build parameter"
and built my first escalation on the idea that only a generated header could
work and that this was untried ground. Both halves are wrong. The mechanism
exists, end to end, and is in use today.

**[V]** The `CUDA_VERSION` chain, every step opened:

| step | site |
|---|---|
| default, overridable by `-DCUDA_VERSION=` on the cmake command line | `cmake/TaichiCore.cmake:98-100`, `if(NOT CUDA_VERSION) set(CUDA_VERSION 10.0) endif()` |
| forced when CUDA is off | `CMakeLists.txt:149-152`, `set(CUDA_VERSION "0.0")` |
| substituted into a generated header | `taichi/common/version.h.in:5`, `#define CUDA_VERSION "@CUDA_VERSION@"`, by the `configure_file` at `CMakeLists.txt:203` |
| exposed to C++ | `taichi/common/core.cpp:87-89`, `get_cuda_version_string()` returns the macro |
| selects a device bitcode file at run time | `taichi/runtime/llvm/llvm_context.cpp:213-218`, `libdevice_path()` builds `slim_libdevice.<major>.bc` |

So a non-boolean value already travels configure → generated header → host C++
→ run-time choice of which bitcode module to load. That is the shape brief 6.1
needs, and it is established practice rather than a novel design.

**[V] The generated headers are gitignored, not checked in.** Neither
`taichi/common/version.h` nor `taichi/common/commit_hash.h` exists in this
checkout; both are produced only by a configure run, and `.gitignore:63-64`
lists them. So the established pattern is "generate into the source tree and
ignore the product", not "bake a value into a tracked file". My first
escalation asked a hygiene question the repository has already answered by
convention.

**[V] `CUDA_VERSION` specifically never reaches the bitcode.** It is read by
host C++ to choose between prebuilt `.bc` files. `runtime.cpp` does not include
`taichi/common/version.h` — grep for `version.h` or `CUDA_VERSION` in
`taichi/runtime/llvm/runtime_module/runtime.cpp` returns nothing, and the only
includers of `taichi/common/version.h` anywhere are
`taichi/common/core.cpp:7`, `taichi/util/offline_cache.h:12` and
`taichi/runtime/llvm/llvm_offline_cache.cpp:14`. That narrow fact stands.

**Correction, twice over.** I then generalised from it, and both halves of the
generalisation were wrong.

1. I wrote that the SNode ceiling "sizes a struct that the host and the bitcode
   must agree on". **The host does not lay out that struct at all.** See 5.3.
2. I wrote that the ceiling therefore "has to be compiled into the bitcode, not
   consulted beside it". The first clause is true and the second does not
   follow from it. The value must be fixed at *some* bitcode compile, which is
   not the same as saying one install may hold only one value. Brief section 5
   permits "custom compilation *or* activating the correct prebuilt binary",
   and my sentence foreclosed the second. See 5.3.2.

What actually survives from this subsection is narrower and is still worth
having: `CUDA_VERSION` is precedent for the configure → generated header → host
C++ → filename selection route, and it is not precedent for reaching the
bitcode compile. It is not, however, the only precedent available. 5.3.1 has
the one that is.

**[V]** Both `configure_file` calls in the whole build are at
`CMakeLists.txt:203-204`, and both write into `${CMAKE_SOURCE_DIR}` rather
than the binary directory. `version.h.in:1-5` shows the form: plain `@VAR@`
substitution into `#define`s.

`taichi/inc/constants.h` is a **checked-in, tracked** header with no `.in`
template, which is the one genuine difference from the `version.h` precedent:
making it generated means removing a tracked file and adding a `.in`.

### 5.3 The critical constraint: the .bc runtime ignores CMAKE_CXX_FLAGS

**[V]** `taichi/runtime/llvm/runtime_module/CMakeLists.txt:3-15`. The bitcode
runtime is built by a bare `add_custom_target` invoking clang directly,
line 8:

```
${CLANG_EXECUTABLE} ${CLANG_OSX_FLAGS} -c runtime.cpp -o "runtime_${rtm_arch}.bc"
  -fno-exceptions -emit-llvm -std=c++17 -D "ARCH_${rtm_arch}" -I ${PROJECT_SOURCE_DIR}
```

That command line does **not** include `${CMAKE_CXX_FLAGS}`.

**Consequence.** Threading the SNode ceiling as a `-D` appended to
`CMAKE_CXX_FLAGS` — the pattern every `TI_WITH_*` uses
(`cmake/TaichiCore.cmake:94-131`) — would reach `taichi_core` but would
**not** reach `runtime.cpp`, which is exactly the file whose lines 567-569
size the three arrays.

**Correction: the failure that would produce is not the one I described.** I
wrote that "host and device would disagree on the runtime struct layout,
silently". The host has no layout for that struct to disagree with. What I got
right is the conclusion, that the value must reach the clang invocation on line
8; what I got wrong is the reason, and the wrong reason sends a reader looking
for an ABI mismatch that cannot occur. The mechanism, verified:

- **[V] `LLVMRuntime` is defined once, inside the bitcode translation unit**, at
  `taichi/runtime/llvm/runtime_module/runtime.cpp:552`. In host code it is a
  forward declaration only — `taichi/program/context.h:11` and
  `taichi/rhi/llvm/llvm_device.h:8`, both `struct LLVMRuntime;` — used as an
  opaque pointer at `context.h:18` and `llvm_device.h:14`. Grep for
  `struct LLVMRuntime` across `taichi/` and `c_api/` returns exactly those two
  declarations plus three lines in `runtime.cpp` (`:136`, `:337`, and the
  definition at `:552`). `LlvmRuntimeExecutor::get_llvm_runtime`
  (`taichi/runtime/llvm/llvm_runtime_executor.cpp:767-769`) is a `static_cast`
  on a `void *`, which needs no layout.
- **[V] `runtime.cpp` is not compiled into the host library.**
  `cmake/TaichiCore.cmake:77-91` globs `"taichi/runtime/*.h"` and
  `"taichi/runtime/*.cpp"` non-recursively, so
  `taichi/runtime/llvm/runtime_module/runtime.cpp` never enters
  `TAICHI_CORE_SOURCE`.
- **[V] The host takes the LLVM type from the loaded bitcode.**
  `TaichiLLVMContext::get_runtime_type` (`taichi/runtime/llvm/llvm_context.cpp:1004-1011`)
  calls `llvm::StructType::getTypeByName` on the runtime module's context, and
  `TaskCodeGenLLVM::get_runtime` (`taichi/codegen/llvm/codegen_llvm.cpp:2694-2698`)
  bit-casts to `get_runtime_type("LLVMRuntime")`.
- **[V] Fields are read through accessors compiled inside the bitcode**, for
  example `runtime_query<void *>("LLVMRuntime_get_element_lists", ...)` at
  `taichi/runtime/llvm/llvm_runtime_executor.cpp:328`, generated by
  `STRUCT_FIELD_ARRAY(LLVMRuntime, element_lists)` at `runtime.cpp:616` and
  `RUNTIME_STRUCT_FIELD_ARRAY` at `:750`.
- **[V] The host's only use of the constant is the assertion** at
  `taichi/codegen/llvm/struct_llvm.cpp:266`,
  `TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);`. Note that the
  plan's own 6.1 correction records that this assertion does not guard those
  arrays: it bounds a per-tree SNode count while the arrays are indexed by the
  process-global `SNode::id`.

**[V] The real failure of a host-only `-D`.** The host-side bound at
`struct_llvm.cpp:266` would rise while the bitcode's arrays stayed at the old
size, and the overflow would happen inside the bitcode, at the writes
`runtime->element_lists[i] =` (`runtime.cpp:1005`),
`runtime->node_allocators[snode_id] =` (`:1029`) and
`runtime->ambient_elements[snode_id] =` (`:1038`). Still silent, still a
defect, and the conclusion is strengthened rather than weakened, because those
arrays exist *only* in the bitcode.

**[V]** `runtime.cpp:25` includes `"taichi/inc/constants.h"`, resolved through
the `-I ${PROJECT_SOURCE_DIR}` on line 8. So a `configure_file`-generated
`constants.h` following the `version.h.in` precedent reaches **both** the host
build and the clang bitcode build, because both resolve the include through
the source tree.

**[V]** Combining this with 5.2: the generated-header route is both an
established practice in this build and the route that reaches the bitcode
compile. The `CMAKE_CXX_FLAGS` `-D` route reaches only the host.

**[V] The `-I` alternative is not a clean substitution for generating into the
source tree.** The clang command at line 8 carries exactly one include
directory, `-I ${PROJECT_SOURCE_DIR}`. If a generated `constants.h` were
emitted into the binary directory and that directory prepended to `-I`, the
tracked `taichi/inc/constants.h` would still be on disk and still visible
through the existing entry, so which of two headers with the same relative
path wins would be decided by include-directory ordering. Generating into the
source tree, as `version.h` does, avoids that because only one file exists.
Recording the asymmetry; not choosing between the routes.

**[V] The bitcode artefacts are already produced into the source tree.**
`runtime_module/CMakeLists.txt:10` sets
`WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"`, so `runtime_<arch>.bc` is
written next to `runtime.cpp`, and :13 installs it from
`${CMAKE_SOURCE_DIR}/taichi/runtime/llvm/runtime_module/`. The in-file TODO at
:9 reads "it's better to avoid polluting the source dir, keep in build". If 6.3
multiplies the number of device modules per arch, the multiplication lands in
the source tree under the mechanism as it stands.

The loop driving the bitcode builds is
`runtime_module/CMakeLists.txt:29`,
`foreach(arch IN LISTS HOST_ARCH CUDA_ARCH DX12_ARCH AMDGPU_ARCH)`.
`HOST_ARCH` is set at `cmake/TaichiCXXFlags.cmake:149`; `CUDA_ARCH`,
`AMDGPU_ARCH` and `DX12_ARCH` at root `CMakeLists.txt:154-164`, each guarded
by its `TI_WITH_*`.

#### 5.3.1 A configure-time value IS already compiled into the bitcode. `-D "ARCH_${rtm_arch}"`. New, and it falsifies my first escalation.

**This is the correction that matters most in this amendment.** My first
escalation ended on the sentence "Nothing in the tree yet compiles a
configure-time value *into* the bitcode, which is what the ceiling requires."
That is false word for word, and the counter-example is on the clang command
line this report quotes verbatim at the head of 5.3, and quoted in my first
pass too. `-D "ARCH_${rtm_arch}"` is a
configure-time value compiled into the bitcode. I quoted it twice across two
passes and never read it.

**[V]** The whole chain, every step opened for this amendment:

| step | site |
|---|---|
| the values are decided at configure time | `HOST_ARCH` at `cmake/TaichiCXXFlags.cmake:149` (`set(HOST_ARCH ${ARCH} CACHE INTERNAL "Host arch")`, from `CMAKE_SYSTEM_PROCESSOR`); `CUDA_ARCH` at `CMakeLists.txt:155`, `AMDGPU_ARCH` at `:159`, `DX12_ARCH` at `:163`, each set only under its `TI_WITH_*` |
| iterated into a per-value build | `runtime_module/CMakeLists.txt:29`, `foreach(arch IN LISTS HOST_ARCH CUDA_ARCH DX12_ARCH AMDGPU_ARCH)`, calling `compile_llvm_runtime(${arch})` at `:30` |
| **compiled into the bitcode** | `runtime_module/CMakeLists.txt:8`, `-D "ARCH_${rtm_arch}"` on the clang line |
| consumed inside the bitcode | 21 lines, all inside the bitcode translation unit: `runtime.cpp:51, 119, 155, 347, 771, 798, 801, 860, 1144, 1156, 1167, 1293, 1343, 1431, 1549, 1633, 1651, 1861, 1865`, plus `node_pointer.h:15` (included at `runtime.cpp:1660`) and `locked_task.h:7` (included at `:1872`) |
| one installed artefact per value | `runtime_${rtm_arch}.bc` at `:8`, installed at `:13` |
| selected at run time by filename | `get_runtime_fn(Arch)` at `taichi/runtime/llvm/llvm_context.cpp:209-211`, `fmt::format("runtime_{}.bc", arch_name(arch))`, loaded by `module_from_file` at `:358-362` |

**[V]** What the 21 consumption sites do is select preprocessor branches:
`#if ARCH_cuda`, `#if ARCH_cuda || ARCH_amdgpu`, `#ifdef ARCH_amdgpu`,
`#if ARCH_x64 || ARCH_arm64`. So the parameter changes which code is in the
module, and every module is a separately compiled and separately installed
artefact of the same source file.

**[I]** This is a closer precedent for items 6.1 and 6.3 together than
`CUDA_VERSION` is. `CUDA_VERSION` selects among vendor-supplied prebuilt
libraries, and the AMDGPU ISA selection at `llvm_context.cpp:628-655` (4.2)
selects among vendor-supplied prebuilts by detected hardware. This one is a
project-owned parameter that **produces** project-built modules, installs them,
and selects among them at run time. It is the shape brief section 5 describes,
already working, on the exact file item 6.1 has to reach.

**[V] One caveat, checked before recording it.** The existing form is a bare
macro name, `-D "ARCH_x64"`, which clang defines to 1. The ceiling is
`constexpr int taichi_max_num_snodes = 1024;` at `taichi/inc/constants.h:12`, a
declared name rather than a macro, so `-D taichi_max_num_snodes=4096` would
macro-expand the declaration itself and fail to compile. Reaching the constant
by this route needs a distinct macro name that `constants.h` reads, which is a
design decision and not mine. Recording the mechanism; not choosing a route.

#### 5.3.2 The prebuilt route my wording foreclosed is open. New.

Brief section 5 permits configuring "either by custom compilation or by
activating the correct prebuilt binary, whichever proves easier". My sentence at
5.2 — the ceiling "has to be compiled into the bitcode, not consulted beside
it" — reads as ruling out the second, and it should not.

**[V]** The pieces of a prebuilt-variant route all exist and are in use:
the per-value production loop (`runtime_module/CMakeLists.txt:29`), the `-D`
that fixes the value inside each artefact (`:8`, 5.3.1), the install
(`:13`, and again `cmake/TaichiCAPI.cmake:179-184`), and the run-time selection
by filename (`llvm_context.cpp:209-211`). Today the parameter that varies is
the arch; nothing in that machinery is specific to the arch being the thing
that varies.

**[V] The one host-side coupling.** Because the host holds no layout for
`LLVMRuntime` (5.3), the only host-side quantity that has to agree with the
loaded bitcode is the assertion bound at
`taichi/codegen/llvm/struct_llvm.cpp:266`, which is compiled into
`taichi_core`. Under brief section 5's model the host binary and the installed
bitcode are configured at the same moment, so that agreement is satisfied by
construction. Under run-time adaptation by a single fixed host binary it is
not, and where the host's bound would then come from is undecided.

I am recording that both routes are open, not choosing between them.
Escalation 1 is rewritten accordingly and escalation 17 is new.

#### 5.3.3 One ceiling per build is what the existing loop already produces, and the filename does not have to change. New.

Added in the round-three pass, against plan section 5.1a, which no earlier pass
of this report had seen. Section 5.1a settles that an install-time
configuration agent sets the SNode field size per configuration, that baking
the constant into per-architecture bitcode at build time is the CORRECT shape
rather than a defect to work around, that **per build already means per device
configuration**, and that **the filename scheme does not need to grow**. It
also settles that a two-card machine gets two binaries and that the system
activates one or both, so no common denominator has to be found.

**This report and its escalations were written without that section, and three
of my open questions are answered by it.** What follows is the positive
statement that the mechanism already satisfies it, which nothing in this
territory has stated. It is a reading of machinery I had already verified, not
new investigation.

**[V] Every bitcode artefact produced by one configure necessarily carries one
value of the constant.** The loop is
`runtime_module/CMakeLists.txt:29-31`,
`foreach(arch IN LISTS HOST_ARCH CUDA_ARCH DX12_ARCH AMDGPU_ARCH)` calling
`compile_llvm_runtime(${arch})`. Inside `COMPILE_LLVM_RUNTIME` the clang line
at `:8` is fixed except for the `${rtm_arch}` substitutions, so the only thing
that varies across iterations is the `ARCH_` token and the output filename.
`runtime.cpp:25` includes `taichi/inc/constants.h`, resolved through the single
`-I ${PROJECT_SOURCE_DIR}` on that same line, so every iteration compiles
against the same header in the same source tree.

**[V] The host agrees with them by construction.** The host's only use of the
constant is the assertion at `taichi/codegen/llvm/struct_llvm.cpp:266` (5.3),
compiled into `taichi_core` in the same configure; `runtime_module/CMakeLists.txt:12`
makes each `generate_llvm_runtime_${rtm_arch}` target a dependency of
`${CORE_LIBRARY_NAME}`, so the two are produced together. Under section 5.1a's
model, where the host binary and the installed bitcode are configured at the
same moment, host and every bitcode variant hold the same ceiling with no
further mechanism.

**[V] The filename carries the arch and nothing else, and nothing needs to
change about that.** `get_runtime_fn` is three lines,
`taichi/runtime/llvm/llvm_context.cpp:209-211`, one format argument,
`arch_name(arch)`. 5.2 records the constraint that threading a second parameter
through it either grows the filename scheme or fixes one ceiling per build.
**Section 5.1a chooses the second, and the existing machinery is already that
second thing.** So the constraint I recorded is not a cost to be paid; it
describes what the build already does.

**What remains open is the sizing rule, not the mechanism**, and plan section
8.1 item 2 records that as closed in principle too: optimise for what is there.
Escalations 4 and 17 are restated to carry forward only what 5.1a leaves open,
and escalation 2 narrows for the same reason.

### 5.4 Every `option()` is boolean, but non-boolean parameters are not

**[V]** Every `option()` call in `cmake/*.cmake`, the root `CMakeLists.txt` and
`c_api/cmake/` is ON/OFF. Non-boolean *cache* entries are
`CMAKE_BUILD_TYPE` (`CMakeLists.txt:41`, `CACHE STRING`), the internal
`HOST_ARCH` (`cmake/TaichiCXXFlags.cmake:149`, `CACHE INTERNAL`) and a
`CACHE PATH` at `c_api/cmake/FindTaichi.cmake:83`.

**Correction.** My first pass concluded from this that "a numeric SNode ceiling
would be the first non-boolean project-level build parameter". That is false.
`CUDA_VERSION` (5.2) is a non-boolean project-level build parameter today; it
is a plain variable rather than a cache entry, which is why a search restricted
to `CACHE` misses it. `HOST_ARCH` is a second, and it does control the build —
it is the first element of the `foreach` at
`runtime_module/CMakeLists.txt:29` that decides which `runtime_<arch>.bc` files
are produced.

What survives is narrower: the `option()` *form* is boolean-only, which matters
for 5.5 but not for whether a numeric parameter can exist.

### 5.5 The build wrapper's option handling

**[V]** `.github/workflows/scripts/ti_build/cmake.py` is the wrapper that
turns options into a build.
- `OPTION_RE` (line 17) scrapes `option(NAME "desc" ON|OFF)` — that exact form
  only — from `CMakeLists.txt` and `cmake/*.cmake` (`collect_options`, called
  at line 147).
- The trailing `# wheel-tag: xx` comment on each option
  (`cmake/TaichiCore.cmake:2-11`, root `CMakeLists.txt:71, 73, 206, 207, 232`)
  feeds `render_wheel_tag` (109-116), which joins the tags of enabled options
  into the wheel filename. This is the existing "one artefact per feature
  combination" mechanism, and it is the closest thing in the repo to the
  brief's install-time "activate the correct prebuilt binary".
- `DEF_RE` (line 18) parses `-DNAME=value` out of the `TAICHI_CMAKE_ARGS`
  environment variable (`parse_initial_args`, 43-46). A numeric `-D` passes
  through this fine.
- **Line 84: `assert not wheel_tag, "Set a non boolean value to an option with
  wheel-tag"`.** A non-boolean option **cannot** carry a wheel tag. An integer
  SNode-ceiling option could not be encoded into the artefact name by this
  mechanism as it stands. Note this is a constraint on *naming the artefact*,
  not on passing the value: `CUDA_VERSION` (5.2) passes through
  `TAICHI_CMAKE_ARGS` and `DEF_RE` without difficulty and is simply absent from
  the wheel tag.

### 5.6 Changing the ceiling invalidates no cache and no LLVM AOT module

**[V]** 5.3 establishes that the host build and the bitcode build could
disagree on the size of the three arrays at `runtime.cpp:567-569`. The same
hazard exists one layer up, at the point where already-compiled artefacts are
reused.

The LLVM offline cache validates its metadata against the Taichi version and
nothing else. `taichi/util/offline_cache.h:95-99`:

```
if (ver[0] != TI_VERSION_MAJOR || ver[1] != TI_VERSION_MINOR ||
    ver[2] != TI_VERSION_PATCH) {
  ...
  return LoadMetadataError::kVersionNotMatched;
}
```

`LlvmAotModule` reuses that reader —
`cache_reader_(LlvmOfflineCacheFileReader::make(module_path))` at
`taichi/runtime/llvm/llvm_aot_module_loader.h:25` — so LLVM AOT modules inherit
the same version-only check.

**[V]** `taichi_max_num_snodes` is not hashed into any key. The kernel key
built at `taichi/analysis/offline_cache_util.cpp:180-196` folds in the compile
config, the device caps, the kernel parameters, returns, body and autodiff
mode. The three-site census in 5.1 confirms the constant appears nowhere in
that path. By contrast, 6.2's related constant *is* coupled:
`offline_cache_util.cpp:110` loops `taichi_max_num_indices` when serialising an
SNode.

**[I]** So a rebuild that raises the SNode ceiling does not move `TI_VERSION`
and does not move any cache key. Kernels and AOT modules compiled against the
old struct layout would be loaded and run against the new one. The cache format
itself belongs to territory 03; the coupling to 6.1's constant is in mine, and
it needs an owner. Escalated.

### 5.7 constants.h values are hand-duplicated across the C boundary

**[V]** `c_api/src/taichi_core_impl.h:121-122`:
`std::array<uint64_t, 32> host_result_buffer_;` with the comment
"32 is a magic number in `taichi/inc/constants.h`". That 32 is
`taichi_result_buffer_entries` (`constants.h:20`), copied as a literal.
Any constants.h value made parameterisable would not propagate to copies of
this kind. I found this one; I did not audit the tree for others.

---

## 6. What section 6.3 would touch in my territory

Enumerated, with references. This is a map, not a proposal.

**Selection and registration**
- `taichi/program/program.cpp:19-43` (guarded includes), `53-161`
  (the constructor dispatch, especially 80-133).
- `taichi/aot/module_loader.cpp:3-4` (includes), `31-58` (the AOT dispatch).
- `c_api/src/taichi_core_impl.cpp:247-309` (`ti_create_runtime`),
  `171-205` (`ti_get_available_archs`), `13-61` (the six availability probes).
- `c_api/src/taichi_core_impl.h:133-145` (`load_aot_module` /
  `create_aot_module` virtuals).
- `c_api/src/taichi_llvm_impl.cpp:78-121` (the LLVM AOT load path).
- `taichi/rhi/arch.cpp:42-93` (the classification predicates),
  `taichi/inc/archs.inc.h`, `c_api/taichi.json:110-123` (the second arch
  numbering).

**Capability description and matching**
- `taichi/inc/rhi_constants.inc.h:8-35` (the capability list; ABI-ordered,
  see 2.6).
- `taichi/rhi/device_capability.{h,cpp}` in full.
- `taichi/rhi/public_device.h:617, 852-857` (`caps_`, get/set).
- `taichi/program/program_impl.h:170-172` and
  `taichi/runtime/program_impls/gfx/gfx_program.cpp:82-85` (the only override).
- `taichi/aot/module_loader.h:97-100` (`get_required_caps`, the broken link),
  and `:84-86` (`arch()` / `version()`, the two handshakes that were never
  wired at all — see 3.2).
- `taichi/runtime/gfx/aot_utils.h:20, 23` (the live `required_caps` storage)
  and `taichi/aot/module_data.h:125, 135` (the dead parallel one).
- `taichi/runtime/gfx/aot_module_builder_impl.cpp:22-24` (the only producer in
  the tree).
- `taichi/runtime/gfx/aot_module_loader_impl.cpp:43-46` (deserialisation),
  `23-190` (the class that would need the override), `111-116` (the
  unreachable `arch()` / `version()`).
- `taichi/runtime/dx12/aot_module_loader_impl.cpp:64-69` and
  `taichi/runtime/llvm/llvm_aot_module_loader.h:32-38` (the other two sets of
  unreachable `arch()` / `version()`).
- `c_api/src/taichi_gfx_impl.cpp:18-29` (the consumer, including the `!=`
  comparison at line 25 — a contract question, see 3.3).
- `taichi/program/program.cpp:514-557` (`translate_devcaps`,
  `make_aot_module_builder`).
- `c_api/src/taichi_core_impl.cpp:317-334, 336-...` (the C capability API).
- `c_api/include/taichi/taichi_core.h:409-416` (the published statement that
  levels are not ordered).
- `taichi/analysis/offline_cache_util.cpp:88-95, 185, 190` (the second consumer
  of the capability ordinals, see 2.6).
- Per-backend detection: `vulkan_device_creator.cpp:475-...` (caps built
  through the whole function, `set_caps` at 876), `opengl_device.cpp:506-531`,
  `dx_device.cpp:557-567`, `metal_device.mm:1017-1070` and the `set_caps` at
  `:1168`, `vulkan_device.cpp:1575-1581`.
- Tier data detected and discarded on `arch_uses_spirv` (2.8):
  `taichi/rhi/vulkan/vulkan_device.cpp:2515-2532` (memory properties used only
  for handle types), `taichi/rhi/vulkan/vulkan_device.h:739` with
  `vulkan_device.cpp:1125-1129` and `:1945` (two fields consumed),
  `taichi/rhi/vulkan/vulkan_device_creator.cpp:203-229` (the device score),
  `:438` and `:512` (device name to debug log only), and
  `taichi/rhi/opengl/opengl_api.cpp:20-21, 231, 234` (workgroup limits nothing
  reads). `taichi/rhi/metal/metal_device.mm:1027-1069` is the counter-example
  in MECHANISM: a hardware-class ladder translated into portable vocabulary.
  Its CONTENT is feature admission — `:1038` and `:1051-1053` gate
  `spirv_has_int64` on Apple family 3 — which plan sections 2.2 and 6.3 mark as
  the pattern to avoid (2.8).
- `c_api/src/taichi_opengl_impl.cpp:4-13` (the wholesale capability
  replacement, 2.9) against `taichi/rhi/opengl/opengl_device.cpp:506-529` (the
  detection it overwrites), with
  `c_api/include/taichi/taichi_opengl.h:33-36` and
  `c_api/src/taichi_opengl_impl.cpp:26` (the public entry point that makes it
  live on GLES) and `taichi/rhi/opengl/opengl_api.cpp:46-59, 239, 252-257` (the
  early return, the override consumption after it, and the second `kUseGles`
  writer).
- Device selection, both channels (2.5, 2.10):
  `c_api/src/taichi_core_impl.cpp:254-255` (the only arch honouring
  `device_index`) and `:270, 277, 283, 289, 297` (the five refusals);
  `taichi/rhi/vulkan/vulkan_loader.cpp:111-113` (`TI_VISIBLE_DEVICE`),
  `:144-146` (the singleton write) and `vulkan_loader.h:14-17, 32` (the
  singleton itself); `taichi/rhi/vulkan/vulkan_device_creator.cpp:236` (the
  `init()` call that consumes the environment after the C API has written the
  argument) and `:442-468` (the consumption and the fallback);
  `taichi/rhi/cuda/cuda_context.cpp:21-22` (the ordinal pin).
- The six sites that gate on `spirv_has_physical_storage_buffer` in codegen and
  runtime, with their enclosing statements:
  `taichi/codegen/spirv/spirv_codegen.cpp:734` (`ExternalPtrStmt`), `:2326`
  (args struct), `:2402` (argpack struct), `:2483` (return struct);
  `taichi/runtime/gfx/runtime.cpp:95-96`;
  `taichi/runtime/program_impls/gfx/gfx_program.h:79-83`. Against the SNode
  path that does not consult it, `spirv_codegen.cpp:351, 359, 468`.
- The four RHI backends with no capability support at all:
  `taichi/rhi/cpu/`, `taichi/rhi/cuda/`, `taichi/rhi/llvm/`,
  `taichi/rhi/amdgpu/`, plus `taichi/rhi/dx12/`. Tier information for CUDA
  currently sits at `taichi/rhi/cuda/cuda_context.{h,cpp}` (see 2.5). The
  boundary they all sit on is `taichi/rhi/arch.cpp:54-57` and `:63-66`
  (`arch_uses_llvm`, `arch_uses_spirv`) — see 2.7.
- The thirteen ordered comparisons on `spirv_version` (3.3), and in particular
  the target-environment ladder at
  `taichi/codegen/spirv/spirv_codegen.cpp:2666-2679`, feeding the optimizer at
  `:2681`.

**Module variants and their installation**
- `taichi/runtime/llvm/llvm_context.cpp:209-211` (`get_runtime_fn`),
  `358-362` (`module_from_file`).
- `taichi/util/lang_util.cpp:30-47` (`runtime_lib_dir`).
- `taichi/runtime/llvm/runtime_module/CMakeLists.txt:3-15, 29-31`
  (production of the per-arch `.bc`), and specifically `:8`, the
  `-D "ARCH_${rtm_arch}"` that compiles a configure-time value into each
  module (5.3.1), with its 21 consumption sites in `runtime.cpp`,
  `node_pointer.h:15` and `locked_task.h:7`.
- `taichi/runtime/llvm/runtime_module/runtime.cpp:552` (the sole definition of
  `LLVMRuntime`), `:567-569` (the three arrays), `:616` and `:750`
  (the in-bitcode field accessors), against
  `taichi/program/context.h:11` and `taichi/rhi/llvm/llvm_device.h:8`
  (the host's forward declarations) and
  `taichi/codegen/llvm/struct_llvm.cpp:266` (the only host-side use of the
  constant). See 5.3.
- `cmake/TaichiCAPI.cmake:179-184`, `cmake/TaichiCore.cmake:429-438`
  (installation of runtime bitcode).

**Build assembly**
- `cmake/TaichiCore.cmake:1-11` (toggles), `35-75` (coercion),
  `94-131` (the `-D` propagation), `134-302` (the link graph).
- `taichi/rhi/CMakeLists.txt:42-105`, and `:17` (the precedence bug, section 7).
- `cmake/TaichiCAPI.cmake:20-64` (c_api source assembly).
- `.github/workflows/scripts/ti_build/cmake.py:17, 84, 109-116, 147`.

**The installed surface**
- `c_api/include/taichi/taichi.h:9-27` (the fourth re-expression of the toggle
  list, gated on macros the install never defines; `TI_WITH_CPU` at `:21`
  exists nowhere else — see 4.4).
- `cmake/TaichiConfig.cmake.in` (5 lines) and `cmake/TaichiTargets.cmake:56-88`
  (the installed package, carrying no build configuration).
- `cmake/TaichiCAPI.cmake:117-195` (`install_taichi_c_api`), `:31-60` (which
  public headers get installed under which toggle).

**Artefact invalidation**
- `taichi/util/offline_cache.h:88-105` (the version-only metadata check).
- `taichi/runtime/llvm/llvm_aot_module_loader.h:25` (LLVM AOT inheriting it).
- `taichi/analysis/offline_cache_util.cpp:110` (`taichi_max_num_indices` in the
  SNode key), `:180-196` (what the kernel key actually folds in).

---

## 7. Upstream defects found in passing

Recorded as observations. I am not proposing changes.

**[V]** `cmake/TaichiCore.cmake:111` writes
`list(APPEND TAIHI_CORE_SOURCE ${TAICHI_AMDGPU_RUNTIME_SOURCE})` — misspelled,
missing the C, so it appends to a variable nobody reads.

**Correction.** My first pass concluded from this that
`taichi/runtime/amdgpu/runtime.cpp` "is never added to `TAICHI_CORE_SOURCE`",
which asserts the file exists and is being dropped. It does not exist.
`taichi/runtime/amdgpu/` contains `CMakeLists.txt`, `jit_amdgpu.cpp`,
`jit_amdgpu.h`, `kernel_launcher.cpp` and `kernel_launcher.h`, and nothing
else; `taichi/runtime/cuda/` has the identical shape. So the glob at :110
matches nothing and the misspelled append at :111 appends an empty list. The
correctly spelled CUDA equivalent at :104-105 also appends nothing.

The typo is therefore dormant, not live: harmless today, and it would stay
silently harmless if that file were ever added. I checked line 105 as a
control and drew a conclusion from the contrast instead of checking the
inputs to both globs.

**[V]** `taichi/rhi/CMakeLists.txt:17` reads

```
if (TI_WITH_OPENGL OR TI_WITH_VULKAN AND NOT ANDROID)
```

CMake binds `AND` tighter than `OR`, so this parses as
`TI_WITH_OPENGL OR (TI_WITH_VULKAN AND NOT ANDROID)`. The `NOT ANDROID` guard
therefore does not cover the OpenGL arm: with OpenGL on and `ANDROID` set,
GLFW is still added as a subdirectory (:29), linked (:30) and put on the public
include path (:31-34). I read this file line by line in my first pass, cited
`:42-105` twice, and did not notice line 17. Whether Android is a target at all
is not settled anywhere in the brief, so this is recorded as an observation
only.

**[V]** `taichi/system/dynamic_loader.h` and `taichi/common/dynamic_loader.h`
both declare the same class and differ; the `common` one additionally declares
`static bool check_lib_loaded(const std::string&)`, which is what
`taichi/rhi/cuda/cuda_driver.cpp:54` calls. Only the `common` copy is ever
included (see 4.1); nothing includes the `system` one.

Minor, same category: `taichi/program/program.cpp:9-10` includes
`opengl_program.h` and `metal_program.h` unconditionally, then again under
`#ifdef` at 28-29 and 40-41. And
`taichi/runtime/llvm/runtime_module/CMakeLists.txt:13` uses `${arch}` (the
outer `foreach` variable) where the command on line 8 uses `${rtm_arch}` (the
function parameter); they coincide only through CMake's parent-scope
visibility inside functions.

---

## 8. Verified versus inferred, and what this revision changed

### 8.1 What changed in this pass

Five corrections, each verified in the source before I accepted it. The first
three are errors adversarial review found in my own work; the last two are
scope I had missed.

1. **Metal does call `set_caps`** (2.2). There are eight sites, not seven. The
   cause was a grep restricted to `*.cpp` and `*.h` and then described as
   exhaustive over the directories. I have removed the word "exhaustive" from
   every grep claim in this report and replaced it with the scope actually
   searched, and I re-ran the other censuses without the file-type filter.
2. **The AMDGPU CMake typo is inert** (7), because
   `taichi/runtime/amdgpu/runtime.cpp` does not exist. I asserted the file was
   being dropped without listing the directory.
3. **My central 6.1 premise was too strong** (5.2, 5.4). `CUDA_VERSION` is
   already a non-boolean CMake value carried into a generated header and used
   to select a bitcode file. What survives, and is sharper than what I wrote,
   is that the precedent covers the "choose between prebuilt modules" half and
   not the "compile the value into the module" half that the SNode ceiling
   needs.
4. **Three new gaps in what I had covered**: no arch or version handshake on
   AOT load (3.2), the ceiling invalidating no cache or AOT module (5.6), and
   the installed package recording nothing about its own build (4.4).
5. **Two reclassifications**: the `!=` comparison is a contract question rather
   than a defect (3.3), and the `required_caps` channel is one live struct
   plus one dead one rather than a generic mirror (3.3).

Plus the citation corrections listed in the revision-pass notes, entry 35.

### 8.1b What changed in the amendment pass

Five claims were put to me. I verified each against source before accepting or
rejecting it. Four held; one is wrong as stated. Adjudication is recorded in the
notes at entries 36-44.

1. **UPHELD, and it is the load-bearing one.**
   `-D "ARCH_${rtm_arch}"` at `runtime_module/CMakeLists.txt:8` is a
   configure-time value compiled into the bitcode, consumed at 21 lines inside
   the bitcode translation unit, producing one installed artefact per value,
   selected at run time by filename. My escalation 1 said no such thing existed.
   Rewritten at 5.3.1 and escalation 1, not trimmed.
2. **UPHELD.** The host never lays out `LLVMRuntime`; it is a forward
   declaration typed from the loaded bitcode and read through in-bitcode
   accessors. My "host and device would disagree on the struct layout" named a
   failure that cannot occur. Corrected at 5.3, where the real failure — an
   out-of-bounds write inside the bitcode — is now given. The related claim that
   my 5.2 wording forecloses the prebuilt route brief section 5 permits also
   holds; 5.3.2 reopens it.
3. **UPHELD.** `ti_set_runtime_capabilities_ext`
   (`c_api/src/taichi_core_impl.cpp:317-334`) installs arbitrary capabilities
   wholesale at `:331`, and I cite that function at 2.2 and 4.3 without ever
   reconciling it with "permanently" and "never set on any path" at 2.4.
   Retracted there.
4. **UPHELD.** I cited three ordered comparisons on `spirv_version` as examples
   and gave no count; there are thirteen, and the four I had not cited are one
   graded ladder selecting the SPIR-V target environment
   (`spirv_codegen.cpp:2666-2678`). Full census now at 3.3. While recounting I
   also corrected my capability-level tally there from 52 to 54, with the five
   non-live sites named, since both adversaries had tested that figure and it
   was a `[V]` claim of mine that did not match the tree.
5. **NOT UPHELD as stated, and nothing was changed on its account.** The claim
   was that neither report states the CUDA path never populates the capability
   system. This report states it at 2.2, lines 221-233, marked `[V]`, and calls
   it "the central gap for brief section 5.2" in bold. What was genuinely
   missing is the consequence for item 6.2, which is now stated in one place at
   the new 2.7: the item divides into two differently shaped halves along
   `arch_uses_spirv` (`taichi/rhi/arch.cpp:63-66`).

Three smaller corrections, all to `[V]` claims of mine: the `TI_ARCH_METAL`
case range at 1.1 (`:295-301`, not `:294-301`), the described scope of the
`spirv_has_atomic_int64` grep at 2.3 (case-sensitive it returns three lines,
not four), and the tally in item 4 above.

### 8.1c What changed in the framing pass

Plan section 5.2 was amended after the amendment pass: the three cards define
fidelity classes, not a vendor target, and section 2 governs. Six claims came
with it. Five held; one was a recommendation of mine that had to go.

**Read this list against 8.1d.** Items 2, 3 and 5 below were carried out
against an intermediate wording of plan section 5.2 that the 2026-09-09
correction withdraws. The list is kept as written, with the parts the current
plan contradicts marked in place, because what happened to it is itself part of
the record: the framing pass was correct against the text it was given.

1. **Withdrawn and re-argued, not deleted.** My 4.2 closed with "For the LLVM
   family, which is where the brief's named target cards live, this is where
   adaptive module loading plausibly starts". That is a recommendation resting
   on the card list. Withdrawn as written. The observation under it survives on
   grounds that do not need the cards — the `.bc` locus is the only run-time
   selection of Taichi's own compiled output, and it serves the CPU-only first
   class target — and is now stated as a description of two loci, the `.bc`
   selector and the AOT loader, with neither ranked.
2. **Six card-dependent statements restated**: 2.2 (the central gap now argued
   from CPU-only being first class), 2.5 twice (the compute-capability clamp
   and the device-0 pin), 4.2, and escalations 10 and 11. Escalation 8 was
   already restated this way in the amendment pass. This item then added: "In
   two of them the direction is worth noting: on device selection the portable
   path is the one better served, so there is no gap in it to record."
   **That closing sentence is withdrawn in the round-three pass** — it is wrong
   against plan section 5.1a, which makes choosing between cards at install
   time the model, and against plan section 5.2, which says a better-served
   spine is never evidence that the other matters less. A gap exists on the
   spine that lacks the selector and is recorded at 2.5 and escalation 10.
3. **The 64-bit machinery re-weighted at 2.4 and 2.7.** The re-weighting itself
   is **withdrawn in the round-three pass**: it ranked the two halves of item
   6.2 by naming one spine the reference path, which plan sections 2.2a and 5.2
   rule out. What was verified alongside it stands, and is recorded so neither
   half is overstated: the six gates sit in `ExternalPtrStmt`, the args struct,
   the argpack struct, the return struct, the ndarray path in the gfx runtime
   and the argument layout string. SNode and root-buffer addressing goes
   through `BufferType::Root` and consults none of them.
4. **New subsection 2.8**, verified and true, and stronger than claimed in one
   place: the OpenGL workgroup globals are not merely absent from the
   capability map, they have no consumer anywhere in the tree.
5. **Metal folded into 2.8** as the model: it reads a vendor hardware-class
   ladder and writes only portable vocabulary, adding no vendor entry to
   `rhi_constants.inc.h`. **Incomplete as written**, and completed in the
   round-three pass: only its MECHANISM is the model. What it publishes is
   feature admission, and below Apple family 3 a device loses
   `spirv_has_int64` outright.
6. **New subsection 2.9**, verified and worse than claimed: the C API does not
   add an unconditional 64-bit assertion, it replaces the whole detected
   capability config, dropping the detected 16-bit entries as well. **Its
   stated scope limit is false and is retracted in the round-three pass**: GLES
   is reachable through `ti_import_opengl_runtime`, so the defect is live
   rather than latent.

### 8.1d What changed in the round-three pass

Seven claims and one instruction were put to me. I verified each against source
before accepting or rejecting it. All seven claims hold. Adjudication is in the
notes at entries 52-59. Nothing here required re-investigating a census; one
item required reading source neither pass of this territory had opened.

1. **The superseded axis, removed at 37 lines.** Enumerated by my own grep and
   listed in the header block above: 20 lines carrying "reference path", 5
   carrying "accelerant", 12 further lines carrying "the portable path" or "the
   vendor path" as the same axis, line 360 of the previous revision counted
   once for carrying two of them. Restated at 2.2, 2.4, 2.5, 2.7, 2.8, 2.9,
   4.2, the section 6 map, 8.1c and escalations 8, 9, 10, 11, 18, 19 and 20.
   `grep -n 'reference path\|accelerant' PROJECT-PLAN.md` returns nothing, so
   no citation of the plan was lost by the change; there was none to lose.
2. **The attribution of a ranking to the plan, withdrawn** at 4.2. This was the
   sharpest instance, because it stated what the governing document says rather
   than deriving a conclusion from source.
3. **The GLES escalation inverted.** My scope caveat at 2.9 and my notes entry
   50 both rest on GLES being unreachable through the C API. It is reachable:
   `ti_import_opengl_runtime` (`c_api/include/taichi/taichi_opengl.h:33-36`,
   body at `c_api/src/taichi_opengl_impl.cpp:21-29`) takes a `use_gles`
   parameter and sets the override at `:26`. Retracted at 2.9 with the chain
   opened step by step, and escalation 20 rewritten. The converse ordering, in
   which an earlier `ti_get_available_archs` silently loses the `use_gles`
   argument, is recorded with it.
4. **`TI_VISIBLE_DEVICE`, new subsection 2.10.** An out-of-process device
   selector at `taichi/rhi/vulkan/vulkan_loader.cpp:111-113`, consumed at
   `vulkan_device_creator.cpp:442-456`, absent from this report and its notes.
   Verified with it: the selector is a member of a process-wide singleton, and
   the environment read sits inside the `std::call_once` that
   `VulkanDeviceCreator` triggers **after** `ti_create_runtime` has written the
   caller's `device_index`. New escalation 21.
5. **The downgrade of multi-device selection, withdrawn** at 2.5, 8.1c item 2
   and escalation 10, against plan section 5.1a. Recorded with it, and neither
   position adopted: whether the refusal on `x64` and `arm64` counts as a gap
   at all, given that `taichi/rhi/cpu/` and `taichi/rhi/llvm/` contain no
   device index for it to name (verified: a grep for `device_index`,
   `device_get_count` and `num_devices` across both returns nothing). Also
   recorded: the CUDA pin is on a driver ordinal, and Taichi reads no
   `CUDA_VISIBLE_DEVICES` anywhere, which is what reconciles the finding with
   the concurrency plan section 5.1a records as observed.
6. **Plan section 5.1a absorbed, new 5.3.3.** One ceiling per build is what the
   existing loop already produces, and the filename scheme does not need to
   grow. Escalations 4 and 17 are restated to carry only what 5.1a leaves open,
   and escalation 2 narrows.
7. **The plan section 2.2 caveat on Metal's content added at 2.8**, with the
   admission gate at `metal_device.mm:1038, 1051-1053` and the consumers that
   take the no-64-bit branch below it. New escalation 23, which records the
   question rather than answering it.
8. **Three citation corrections to `[V]` sentences of mine**: the optimizer
   line is `:2681` not `:2680` (3.3 and the section 6 map); `get_device_score`
   at `vulkan_device_creator.cpp:452` gates admission of an explicitly
   requested device rather than dying at the call site (2.8, new escalation
   22); `kUseGles` has two writers, `opengl_api.cpp:239` and `:254` (2.9).

**One place where an instruction I was given is narrower than the plan, stated
per standing instruction 6.** I was asked to remove the reference-path and
accelerant framing. The plan withdraws the whole axis, and this report also
used "the portable path" against "the vendor path" to mean the same thing, at
12 further lines. I removed those too, because plan section 5.2 names that
exact pair as the wrong axis and section 2.2a states what "vendor" does mean.

### 8.2 Verified

Everything cited in sections 1, 2, 3, 5, 6 and 7 was read at the line given,
except where marked `[I]` below. That includes the `set_caps` census as
re-run, the `get_required_caps`, `spirv_has_physical_storage_buffer`,
`spirv_has_int64`, `spirv_has_atomic_int64` and `taichi_max_num_snodes`
censuses, the `CUDA_VERSION` chain end to end, the `arch()` / `version()`
override and caller census, the capability-level tally in 3.3 as recounted,
and the `TI_WITH_CPU` search.

Added in the amendment pass and read at the line given: the
`-D "ARCH_${rtm_arch}"` chain end to end including all 21 consumption sites
(5.3.1); the `LLVMRuntime` declaration census, the non-recursive host source
glob, `get_runtime_type`, the in-bitcode accessors and the three array write
sites (5.3); `ti_set_runtime_capabilities_ext` and the path from `Device::caps_`
to the twelve consumers (2.4); the thirteen ordered comparisons and the
`spv_target_env` ladder (3.3); the `arch_uses_spirv` / `arch_uses_llvm`
predicates and the negative `set_caps` grep over the five LLVM-spine RHI
directories (2.7).

Added in the framing pass and read at the line given: the Vulkan memory
properties call and its only use, the stored `VkPhysicalDeviceProperties` and
its two consumers, the device-name log sites and `get_device_score` (2.8); the
OpenGL workgroup globals and the negative grep showing they have no consumer
and no header declaration (2.8); `collect_metal_device_caps` in full (2.8);
`OpenglRuntime::OpenglRuntime` against `GLDevice::GLDevice` and the early
return in `initialize_opengl` (2.9); the enclosing functions of all six
`spirv_has_physical_storage_buffer` gates and the three SNode-path visitors
that do not consult it (2.4); and the `device_index` handling for all six archs
in `ti_create_runtime` (2.5, escalation 10).

Added in the round-three pass and read at the line given:
`ti_import_opengl_runtime` and its declaration, `set_gles_override`,
`unset_gles_override`, the override consumption at `opengl_api.cpp:56-59`
against the early return at `:46-54`, the two `kUseGles` writers and
`reset_opengl` in full, the default argument at `opengl_api.h:13` and the
`ti_get_available_archs` probe chain (2.9); `VulkanLoader::init` in full
including the `std::call_once` and the `TI_VISIBLE_DEVICE` read,
`vulkan_loader.h:14-17, 32`, `set_vulkan_visible_device`, the `init()` call at
`vulkan_device_creator.cpp:236` and the selection block at `:428-468`, and the
`VulkanRuntimeOwned` member declaration (2.10); `get_device_score` in full
against both of its call sites (2.8); `collect_metal_device_caps:1027-1053`
against `metal_device.mm:131-135` and `device_capability.cpp:33-39` (2.8);
`spirv_codegen.cpp:2660-2681` re-numbered line by line (3.3); the negative greps
for `CUDA_VISIBLE_DEVICES` over `taichi/`, `c_api/` and `python/` and for
`device_index`, `device_get_count` and `num_devices` over `taichi/rhi/cpu/` and
`taichi/rhi/llvm/` (2.5); `runtime_module/CMakeLists.txt:1-31` re-read whole,
with `runtime.cpp:25` and `struct_llvm.cpp:266` (5.3.3); and
`taichi/util/lang_util.cpp:30-47` for the `TI_LIB_DIR` comparison (2.10).

### 8.3 Inferred

- 2.4, that the `spirv_has_physical_storage_buffer` capability means part
  of the 6.2 machinery exists and is switched off. I verified the code is
  unreachable from inside the core; I did not verify it is correct.
- 2.4, that a host calling `ti_set_runtime_capabilities_ext` before compiling
  would put the twelve consumers on the true branch. Every link in the path is
  verified; that the resulting code runs correctly on a device that may not
  support the feature is not, and I did not test it.
- 2.7, the division of item 6.2 into two halves. Both halves are verified
  facts; that they size to different amounts of work is my reading.
- 5.3.2, that both deployment routes in brief section 5 are open for the
  ceiling. The machinery cited is verified in use; that it would carry a
  different parameter is inference.
- 2.4, that enabling the disabled pointer machinery would not by itself widen
  an SNode address. The gate sites and the SNode path are both verified; that
  nothing else couples them is inference from those two censuses.
- 2.8, that Metal shows the shape a publishing step would take. What Metal does
  is verified line by line; calling its mechanism a model is my reading. That
  its content is admission rather than fidelity is verified at
  `metal_device.mm:1038, 1051-1053`; whether an inherited behaviour falls
  inside plan section 2.2's test is a question I record and do not answer.
- 2.9, that a host taking either of the two sequences gets one of two wrong
  outcomes. Every link in both paths is verified; the composed outcome is read,
  not run.
- 2.10, that the environment overwrites the caller's `device_index` on the
  first Vulkan runtime in a process and not on later ones. The singleton, the
  `call_once`, the environment read and the call ordering are all verified; the
  composed outcome is read, not run.
- 5.3.3, that host and bitcode agree on the ceiling by construction under plan
  section 5.1a's model. The loop, the single include path, the target
  dependency and the host assertion are verified; that the agreement holds is
  the consequence I draw from them, and it depends on the deployment model
  section 5.1a states rather than on anything the build enforces.
- 2.9, that the wholesale replacement is a loss of low-end protection. The
  replacement and the discarded entries are verified. The inference I recorded
  here previously — that a GLES context arises in practice ahead of a C API
  OpenGL runtime — is **no longer what the finding rests on**, because
  `ti_import_opengl_runtime` reaches GLES directly, and that chain is verified
  step by step at 2.9.
- 4.2, that item 6.3 has to be answered at both loci. The state of each locus
  is verified; the consequence is mine.
- 3.2, that a Vulkan module can be handed to an OpenGL runtime unchallenged.
  The absence of every caller is verified; that no other layer catches it is
  inference from that absence.
- 3.3, that the objection to `>=` concerns only hypothetical future
  capabilities. The tally of existing levels is verified; the reading of the
  disclaimer's purpose is mine.
- 4.2, that the per-arch `.bc` load is where LLVM-side adaptive module loading
  would start.
- 4.3 and 4.4, that these are the nearest analogues of the brief's install-time
  loader, and that a downstream consumer must guess the build configuration.
- 5.6, that a ceiling change would load stale artefacts. The version-only check
  and the absent hash coupling are verified; the runtime consequence is
  inference.

### 8.4 Not investigated

Outside my territory: the correctness of SPIR-V and LLVM codegen under a 64-bit
width change (agents 01, 02); the runtime struct and the offline cache format
themselves (agent 03); the Python front end (brief 1.2). Also not investigated,
and flagged by review as a genuine hole in my coverage: the `Extension` system
(`taichi/program/extension.{h,cpp}`, `taichi/inc/extensions.inc.h`), which is a
second capability vocabulary, and the DX12 availability stub
(`taichi/rhi/dx12/dx12_api.cpp`). Both were found by the paired pass. I did not
re-derive them here and I am not restating that agent's findings as my own.

---

## 9. Escalations

Each of these needs a decision I am not authorised to make. Items 1, 8 and 9
were rewritten in the amendment pass because verification changed what the
question is. Items 17 and 18 were new then.

**Round-three pass.** Items 8, 9, 10, 11, 18, 19 and 20 have the withdrawn
spine axis removed; none of their facts changes and item 20's question is
inverted by a live defect. Items 2, 4 and 17 are narrowed against plan section
5.1a, which settles the mechanism half of what they asked. Items 21, 22 and 23
are new.

1. **How the SNode ceiling reaches the bitcode runtime (6.1). Rewritten, twice
   now.** My first pass escalated this as untried ground. My revision answered
   half of that with `CUDA_VERSION` and then asserted that nothing in the tree
   compiles a configure-time value into the bitcode. **That assertion is false**
   (5.3.1), so this is no longer a question about whether a route exists. It is
   a choice among three that do:

   | route | what exists today | what is undecided |
   |---|---|---|
   | generated `constants.h` via `configure_file` | the form is established (`version.h.in:5`, `CMakeLists.txt:203`); `runtime.cpp:25` includes `taichi/inc/constants.h` through the `-I ${PROJECT_SOURCE_DIR}` on `runtime_module/CMakeLists.txt:8`, so it reaches both builds | `taichi/inc/constants.h` is a **tracked** file, so this means deleting it and adding a `.in`. The `-I` alternative is not equivalent (5.3): with the tracked header still on disk, two headers of the same relative path would be visible and include-order would decide. The tracked header has to stop being a header, not merely be shadowed |
   | a `-D` on the clang line at `runtime_module/CMakeLists.txt:8` | already in use for `ARCH_${rtm_arch}` (5.3.1) | `-D` on the `constexpr` name itself would macro-expand the declaration and fail to compile, so this needs a distinct macro name that `constants.h` reads. Also, this route reaches the bitcode only; the host's assertion bound at `struct_llvm.cpp:266` would still need its own path |
   | both, one per side of the boundary | — | whether two mechanisms for one value is acceptable |

   The hygiene question my first pass raised is answered by convention:
   `.gitignore:63-64` shows generated headers are ignored rather than tracked,
   and neither `taichi/common/version.h` nor `commit_hash.h` exists in this
   checkout.

2. **Whether the SNode ceiling should be one value or a per-arch value.
   Narrowed against plan section 5.1a.** The bitcode runtime is built once per
   arch (`runtime_module/CMakeLists.txt:29`) and the assertion bound the host
   holds must match whatever the loaded bitcode was compiled against
   (`struct_llvm.cpp:266`). A per-arch ceiling would mean per-arch host code
   too. **Section 5.1a settles the mechanism half**: per build already means
   per device configuration, so one value per build is sufficient and is what
   the loop already produces (5.3.3). What is left is whether one build should
   ever hold different ceilings for different archs, which is a sizing question
   under plan section 8.1 item 2 rather than a mechanism question, and is not
   mine to settle.

3. **Whether a changed ceiling must invalidate offline caches and LLVM AOT
   modules, and keyed on what (5.6).** `taichi/util/offline_cache.h:95-99`
   keys on `TI_VERSION` alone and
   `taichi/runtime/llvm/llvm_aot_module_loader.h:25` inherits it. A ceiling
   change moves neither `TI_VERSION` nor any hash input, so stale artefacts
   compiled against the old struct layout would load silently. The cache
   format is territory 03; the coupling to 6.1's constant is mine. Somebody
   has to own the seam.

4. **Whether the artefact naming needs to carry the ceiling. CLOSED by plan
   section 5.1a as to the runtime bitcode; a narrower question survives.**
   Section 5.1a states that per build already means per device configuration,
   that a two-card machine gets two binaries, and that **the filename scheme
   does not need to grow**. 5.3.3 states the positive result: one ceiling per
   build is what `runtime_module/CMakeLists.txt:29-31` already produces, with
   `get_runtime_fn` (`llvm_context.cpp:209-211`) unchanged. So I withdraw this
   as a question about `runtime_*.bc`.

   Two facts I recorded under it are not closed by that section and are carried
   forward as observations rather than as a question. The wheel-tag mechanism
   cannot encode a non-boolean option (`ti_build/cmake.py:84`), so if whole
   wheels rather than bitcode variants are the unit an installer chooses
   between, they carry no ceiling in their names; and if item 6.3 multiplies
   device modules per arch, the multiplication lands **in the source tree**
   under `runtime_module/CMakeLists.txt:10` as it stands (5.3).

5. **The `!=` comparison at `c_api/src/taichi_gfx_impl.cpp:25`. Reclassified
   from defect to contract question.** My first pass said a device exceeding a
   module's requirement "should presumably be accepted". The published contract
   at `c_api/include/taichi/taichi_core.h:411-412` states that a higher level
   is not guaranteed compatible with a lower one, so `!=` is the relation the
   documented type supports and `>=` would be a change to the public contract.
   Complicating it in both directions (3.3): the tree already relies on
   ordering internally for `spirv_version` in **thirteen** places, the strongest
   being the graded ladder at `spirv_codegen.cpp:2666-2678` that picks the
   SPIR-V target environment for the whole module, and no capability that
   exists today would be mishandled by `>=`. The decision is whether the
   contract or the code changes. Whether the C ABI is fixed at all in this fork
   is the same question underneath.

6. **The AOT module format has no arch and no version handshake (3.2), not
   merely a dead capability check.** `version()` is never called and aborts on
   two of three paths; `arch()` is never called and returns the caller's own
   answer; `metadata.json` records no arch. Wiring `get_required_caps` fixes
   one third of this. Whether the module format should carry an arch and a
   version at all is a design decision the plan does not record.

7. **`get_required_caps` is never overridden (3.3).** Wiring it would make a
   dormant compatibility check live for the first time. Existing AOT modules
   carry `required_caps` written by `aot_module_builder_impl.cpp:23`, so
   turning the check on would start rejecting modules that load today. Whether
   that is desired is a decision, and it interacts with item 5.

8. **No capability mechanism exists on the whole LLVM spine: CPU, CUDA, AMDGPU
   and DX12 (2.2, 2.7). Rewritten.** The boundary is
   `taichi/rhi/arch.cpp:54-57` and `:63-66`. Per brief section 2 the
   significant half of this is not that the three cards named in 5.2 sit on the
   undescribed spine, which is incidental, but that **CPU-only operation is a
   first class target and `taichi/rhi/cpu/` sits there too**. The spine the
   capability system does not describe is `arch_uses_llvm`, and item 6.3 has
   nothing to select on there today. Recorded per plan section 5.2 as a gap in
   the less-served spine, never as evidence that either spine matters less. The
   capability vocabulary in `rhi_constants.inc.h` is entirely SPIR-V, though
   the file's own header comment (lines 1-7) says CUDA compute capability could
   be listed there. Extending that vocabulary is the kind of creative
   extrapolation I was told to escalate rather than resolve. Two ABI
   constraints attach: new entries must go at the end of the file or the C API
   renumbers (2.6), **and** the same ordinals key the offline cache
   (`offline_cache_util.cpp:88-95`), so a renumbering silently invalidates
   every cached kernel as well. Also note that device memory is not reachable
   through the RHI `Device` base at all (2.5), so a tier-aware selector would
   have to go through `LlvmDevice` or `CUDAContext`.

9. **`spirv_has_physical_storage_buffer` has twelve consumers and no in-tree
   producer (2.4). Rewritten.** Both producers are compiled out by an upstream
   author with the note "until device capability is ready". My earlier wording
   called the false branch permanent; it is not.
   `ti_set_runtime_capabilities_ext` (`c_api/src/taichi_core_impl.cpp:317-334`,
   `set_caps` at `:331`) lets a host install that capability with no validation
   against the device, and `c_api/include/taichi/cpp/taichi.hpp:1156` ships a
   typed setter for it. So there are three answers, not two: revive the
   disabled producers, replace the mechanism, or treat the C API override as
   the intended way in. The third makes the twelve consumers exercisable today
   with no source change, which is how one would find out whether the machinery
   works before deciding. It also means an unvalidated host claim can put
   codegen on a path the device may not support. Which of the three is intended
   is a judgement about upstream intent that I am not making.

   **Weighting withdrawn, round-three pass.** The framing pass added here that
   this machinery is "the primary existing form" of item 6.2 because its
   backends were the reference path. Plan section 2.2a states that the two
   halves of item 6.2 are not separable and that work on either spine is BASE
   work, and plan section 5.2 states that neither spine is the vendor spine, so
   the ranking has no source and is gone. Two verified facts stand in its place
   (2.4): the machinery is real 64-bit pointer code on `arch_uses_spirv`, and
   its scope is ndarray, argpack and return-struct addressing, not SNode
   addressing, which goes through `BufferType::Root` bound buffers instead.
   Enabling it would therefore not by itself widen an SNode address. What is
   still undecided is whether 6.2 on that spine means enabling this, building
   the SNode half beside it, or both.

10. **Device selection through the C API exists on Vulkan and nowhere else
    (2.5, 2.10). Restated twice; the downgrade is withdrawn.**
    `cuda_context.cpp:21-22` pins CUDA driver ordinal 0 and
    `taichi_core_impl.cpp:289-290` rejects any non-zero `device_index`; the
    same rejection is at `:270, 277, 283, 297` for OpenGL, x64, arm64 and
    Metal. Vulkan alone honours it, at `:254-255` through
    `set_vulkan_visible_device`. `TI_ARCH_GLES`, `amdgpu` and `dx12` have no
    case in the switch at all.

    **Withdrawn: "its practical effect is on testing rather than
    architecture".** Plan section 5.1a settles that a machine holding several
    GPUs is the ordinary case, that the install-time agent may need to choose
    between them or use more than one, and that more than one engine instance
    can run across more than one card. Choosing a device is therefore part of
    the settled deployment mechanism, and an install-time agent that cannot
    name a device through the C API on CUDA is blocked on it. That is the
    question: **should any arch but Vulkan be able to name a device through the
    C API, and if so which.**

    Two things I record without choosing between them. First, whether the
    refusal on `x64` and `arm64` is a gap at all: there is no second host CPU
    for a device index to name, and a grep for `device_index`,
    `device_get_count` and `num_devices` across `taichi/rhi/cpu/` and
    `taichi/rhi/llvm/` returns nothing, so on those two archs the refusal may be
    the correct answer rather than a missing feature. On that reading the
    substantive gap is CUDA, plus AMDGPU and DX12, which have no case at all.
    On the other reading the refusal is uniform across the spine and stating it
    enum-wide is what makes the finding stand without reference to the owner's
    cards. The source settles that there is no CPU device index; it does not
    settle whether a refusal with nothing to refuse counts as a gap.
    Second, escalation 21: on Vulkan there are two channels, and they collide.

11. **The compute capability clamp at `cuda_context.cpp:75-77`. Restated.** It
    caps `sm_` at 86, so every newer device is compiled as sm_86 and nothing
    reports that it happened. Plan 5.2 says "Beyond the upper tier, rough
    upwards scaling is expected"; this clamp is where that stops on the CUDA
    backend, which is vendor specific in the sense plan section 2.2a defines.
    My first wording argued the point through which card is sm_86, which plan
    5.2 makes irrelevant, and the framing pass then called CUDA "the vendor
    accelerant", which is the withdrawn axis. Neither is needed: the clamp is a
    fixed ceiling on a detection path that has no way to surface itself, and
    that is true whatever hardware is present. Flagging; not touching.

12. **Two AOT paths, two formats, one deprecated dispatcher (3.1).** The gfx
    path takes a `.tcm` zip in memory and has a dead capability gate; the LLVM
    path takes a directory and has no gate. Any adaptive-module design has to
    decide whether these converge or stay separate. Upstream left a comment
    intending convergence (`taichi_core_impl.cpp:629-630`) and did not do it.

13. **What an installed Taichi should tell its loader (4.4). New.** The public
    umbrella header gates every per-backend header on `TI_WITH_*` macros that
    the installed CMake package never defines, so a consumer must guess the
    build configuration. Brief section 5 puts a loader at install time and this
    is the surface it would read. Whether the answer is a generated header, a
    variable in `TaichiConfig.cmake.in`, `INTERFACE_COMPILE_DEFINITIONS` on the
    imported target, or a C entry point, is not recorded anywhere in the plan.

14. **Whether `TI_WITH_CPU` is a bug to fix or a name to choose (4.4). New.**
    It is referenced at `c_api/include/taichi/taichi.h:21` and defined nowhere
    in the repository. Either the build should define it or the header should
    use `TI_WITH_LLVM`, which is what `cmake/TaichiCAPI.cmake:31-33` actually
    gates `taichi_cpu.h` on. Not mine to settle.

15. **`taichi/rhi/CMakeLists.txt:17` operator precedence (7). New.** The
    `NOT ANDROID` guard does not cover the OpenGL arm. Whether it matters
    depends on whether Android is a target at all, which the brief does not
    settle.

16. **Process disclosure, carried forward.** In my first pass a grep failed to
    exclude `modernization/` and three lines of the paired agent's notes
    appeared in the output. I did not open the file, and I re-derived the one
    overlapping subject independently. Both adversarial reviews examined this
    and judged the contamination immaterial, citing among other things that I
    was wrong about Metal in a way the paired pass was right about. I record
    the finding and their disposition; I am not the one to close it.

17. **Which of plan section 5's two deployment routes item 6.1 takes.
    Narrowed against plan section 5.1a, which answers most of it.** Section 5
    permits "custom compilation or activating the correct prebuilt binary,
    whichever proves easier", and my revision's wording foreclosed the second
    (5.2, corrected). Both remain open as routes. **What section 5.1a settles
    is the half I could not**: baking the constant into per-architecture
    bitcode at build time is the CORRECT shape rather than a defect, no runtime
    mutability of the ceiling is required or wanted, and per build already means
    per device configuration. The prebuilt-variant route has every piece in
    service for a different parameter: production per value
    (`runtime_module/CMakeLists.txt:29`), the value fixed inside each artefact
    (`:8`), installation (`:13`), and selection by filename
    (`llvm_context.cpp:209-211`). Its one host-side coupling is the assertion
    bound at `struct_llvm.cpp:266`, and under section 5.1a's model — host binary
    and installed bitcode configured at the same moment — **that agreement holds
    by construction** (5.3.3). The run-time-adaptation case that would break it
    is the case section 5.1a says is not wanted. What survives as a question is
    only which of the two routes is easier in practice, which is an
    implementation choice rather than an architectural one, and it no longer
    decides escalations 2 and 4, both of which section 5.1a narrows on its own.

18. **Item 6.2 is one work item in the plan and the source shows three shapes
    (2.7). Restated, and the question is narrowed.** On `arch_uses_spirv` a
    large part of the 64-bit machinery exists and is switched off at the
    capability layer. On `arch_uses_llvm`, which carries the CPU-only first
    class target of plan section 2 as well as CUDA, AMDGPU and DX12, there is
    no capability representation to switch. And what exists on the first covers
    argument and return addressing rather than SNode addressing (2.4), so the
    SNode half is unbuilt on both. Three shapes.

    **What I am not asking, corrected.** Earlier versions of this item headed
    it "whether item 6.2 is one work item or two", and the framing pass
    surrounded it with a ranking. **Plan section 2.2a settles that the two
    halves are not separable and that both are BASE work in full**, so a split
    is not on the table and no ranking is available. What remains is a sizing
    observation for the planner: one plan item covers three differently shaped
    pieces of work, and sizing it as one conflates them. I record that; I
    propose nothing.

19. **Both spines detect tier data and neither publishes it (2.8). Restated.**
    On `arch_uses_llvm` the data is kept outside the capability system, on
    `CUDAContext` (2.5). On `arch_uses_spirv` it is discarded: Vulkan reads
    memory properties and uses them only for handle types
    (`vulkan_device.cpp:2515-2532`), keeps a full `VkPhysicalDeviceProperties`
    and consumes two fields (`vulkan_device.h:739`, consumed at
    `vulkan_device.cpp:1125-1129` and `:1945`), logs the device name and
    nothing more (`vulkan_device_creator.cpp:438, 512`), and scores devices by
    integrated-versus-discrete class, keeping the number nowhere
    (`vulkan_device_creator.cpp:203-229`, called `:452, :462`; the score is
    discarded at `:462` and used as an admission gate at `:452`, see 2.8);
    OpenGL fills two file-scope workgroup-limit globals that nothing anywhere
    reads (`opengl_api.cpp:20-21, 231, 234`). Metal is the one backend that
    turns a hardware-class ladder into portable vocabulary
    (`metal_device.mm:1017-1070`), though what it publishes is admission rather
    than fidelity (2.8, escalation 23). The decision is what an install-time
    loader should be able to ask, and whether answering it needs new capability
    entries at all or only the publication of detection that already happens.
    Interacts with escalation 8.

20. **The C API discards OpenGL's GLES protection, and it is live (2.9).
    Rewritten; the question has changed.** `OpenglRuntime`
    (`c_api/src/taichi_opengl_impl.cpp:4-13`) replaces the whole capability
    config its own `GLDevice` just detected, asserting 64-bit integer and float
    support unconditionally at `:9-10` and dropping the detected 16-bit
    entries. The check it overwrites (`opengl_device.cpp:509-513`, comment
    "64bit isn't supported in ES profile") is precisely the low-end protection
    plan sections 2 and 5.3 ask for.

    **My earlier framing of this as latent is retracted.** It rested on GLES
    being unreachable through the C API. `ti_import_opengl_runtime`
    (`c_api/include/taichi/taichi_opengl.h:33-36`) is a public exported
    function taking a `use_gles` parameter, and its body sets the GLES override
    at `c_api/src/taichi_opengl_impl.cpp:26` before calling
    `ti_create_runtime(TI_ARCH_OPENGL, 0)` at `:28`. So a host reaches the
    defect by calling a documented function with a documented argument.

    So the question is no longer whether to close the assertion or wire up
    GLES, because GLES is wired up. **It is what the C API should do when its
    own device has just detected the opposite of what the C API asserts.**
    Three shapes are available and the code does not say which is intended:
    stop replacing and merge instead; keep replacing, on the ground that an
    interop host is presumed to know its own context; or validate the
    replacement against the device. The same question attaches to
    `ti_set_runtime_capabilities_ext` (escalation 9), which does the same thing
    wholesale with no device involved at all.

    Carried with it, and also undecided: which of `use_gles` and an earlier
    `initialize_opengl` should win. As it stands the first
    `initialize_opengl` in the process wins, so the natural install-time
    sequence of enumerating archs first silently loses the GLES request (2.9).

21. **Should the `device_index` argument outrank `TI_VISIBLE_DEVICE`, and
    should the selector be per-runtime? New (2.10).** The environment read at
    `taichi/rhi/vulkan/vulkan_loader.cpp:111-113` sits inside the
    `std::call_once` at `:87-115`, which `VulkanDeviceCreator` triggers at
    `vulkan_device_creator.cpp:236`, after `ti_create_runtime` has written the
    caller's `device_index` at `c_api/src/taichi_core_impl.cpp:254-255`. So the
    environment wins on the first Vulkan runtime in a process and the argument
    wins thereafter, and the state is a plain member of a process-wide
    singleton (`vulkan_loader.h:14-17, 32`). Which input is authoritative is
    recorded nowhere. It matters because plan section 5.1a puts the
    configuration agent outside the process, where an environment variable is
    the natural channel, and makes the multi-GPU machine ordinary. I am not
    choosing between them, and I am not touching the concurrency question
    section 5.1a records as observed and closed.

22. **Is a zero score meant to veto an explicitly requested Vulkan device?
    New (2.8).** `vulkan_device_creator.cpp:452` admits `devices[id]` only if
    `get_device_score` returns non-zero, and otherwise falls through to the
    automatic pick at `:458-468` with no message; the only error log, at
    `:448-451`, covers the out-of-range case. So a device named by
    `TI_VISIBLE_DEVICE` or by the C API `device_index` can be silently replaced
    by a different one. Under plan section 5.1a an install-time agent naming a
    device would expect either that device or a failure. Whether this is a
    defect or a deliberate fallback the code does not say, and I am recording
    it rather than grading it.

23. **Is Metal's admission ladder itself a section 2.2 problem, or inherited
    behaviour to be preserved and worked around? New (2.8).** Below Apple
    family 3 Metal withholds `spirv_has_int64`
    (`metal_device.mm:1038, 1051-1053`), so that hardware cannot do a thing
    other hardware can. Plan section 2.2 presents itself as "a TEST to apply to
    every proposed change"; applying a test written for proposed changes to
    existing upstream code is a step past what the section authorises, and
    nothing in the plan says whether inherited behaviour is in its scope. Plan
    section 6.3 records the mechanism-versus-content distinction as a finding
    without saying what follows from it. Recording the question, not answering
    it.
