# Adversary 04-2 — backend, AOT and build architecture

Judgement on `report-04-backend-build.md` (pass A) and `report-04b-backend-build.md`
(pass B). Every correction below was checked by opening the cited file.

---

## 1. Verdicts

**CORRECT — qualified fail for both, worse for A.**
The file:line citations in both reports are accurate at a high rate. I opened
roughly seventy of them and found eight off-by-one or off-by-range slips, none
material on its own. The errors that matter are not citations; they are
**[V]**-marked general statements that the citation underneath does not support.
Pass A has two of these that would change a design decision (§3.1, §3.2 below).
Pass B has one (§4.1).

**COMPLETE — neither alone; the union is close but still short.**
Each pass has a large blind spot the other covers, which is the method working.
After merging them, five things in territory remain unfound (§6).

**MAJOR REVISION REQUIRED: yes for pass A. Targeted correction for pass B.**
Pass A must withdraw the `Extension::data64` "compile-time blocker" claim and the
`ti_device_api_shared` claim, both marked **[V]**, both false. Pass B must
withdraw the Metal `set_caps` claim and the AMDGPU runtime source claim. Both
reports should absorb §6.

---

## 2. Independence

I judge the two passes genuinely independent, and pass B's disclosure
(`report-04b:720-726`) does not change that.

The evidence is the shape of the blind spots, not the prose. Pass A missed
`spirv_has_physical_storage_buffer` being dead, the third dispatch chain in
`ti_create_runtime`, the two `c_api` `set_caps` producers, the CUDA device-0 pin
and `DynamicLoader`. Pass B missed the entire `Extension` system,
`CompileConfig::fit()` and the `is_dx12_api_available()` stub. Those are large,
non-overlapping, and in both directions. Contamination produces convergence, not
this.

On the specific overlap disclosed — c_api header generation — the two accounts
are cited differently. Pass A cites `misc/taichi_json.py:401` for the JSON open
and gives no `c_api/taichi.json` line numbers (`report-04:381-397`). Pass B cites
`c_api/taichi.json:110-123` and `:125-129` and additionally lists the
`archs.inc.h` ordinals, which pass A does not (`report-04b:75-80`). Both are
correct against source. If pass B had read pass A's notes it gained nothing from
them. I would not discount the paragraph.

---

## 3. Pass A — errors

### 3.1 `Extension::data64` is not a compile-time blocker. [material]

`report-04:274-276` states, marked **[V]**: "on the whole portable SPIR-V spine —
vulkan, opengl, gles, metal, dx11 — 64-bit data is declared unsupported ... That
is a compile-time blocker sitting in front of 6.2 for every non-CUDA GPU path."

The table content is right. `taichi/program/extension.cpp:23-27` gives metal,
gles, vulkan and dx11 empty sets and opengl only `extfunc`. The *blocker* is not.
`Extension::data64` has **no consumer in the C++ core at all**.

Every call to `is_extension_supported` in `taichi/` and `c_api/`:
`taichi/program/program.cpp:149` (`assertion`),
`taichi/codegen/llvm/codegen_llvm.cpp:2726` (`bls`),
`taichi/transforms/compile_to_offloads.cpp:92, 205, 218, 236` (`mesh`),
`:245, 288` (`quant`), plus the pybind export at
`taichi/python/export_lang.cpp:1225`. No call site names `data64`.

The only `data64` consumers anywhere are Python test decorators under
`tests/python/` reaching it through that pybind export. The Python front end is
out of scope by brief 1.2.

Consequence for the plan: pass A's escalation 4 (`report-04:641-647`) is
overstated. There are not two capability systems in conflict at the point where
64-bit data is decided; there is one live system (`DeviceCapability`) and one
declaration that gates nothing in C++.

Separately, `taichi/program/extension.cpp:31` indexes a non-const function-static
`std::unordered_map` with `operator[]`, so a miss mutates shared state from
whatever thread called in. Pass A noted the empty-set default (`report-04:270-272`)
but not that the map is written at runtime.

### 3.2 `ti_device_api_shared` claim. [material]

`report-04:84-85`, marked **[V]**: "This is the only shared-library backend
artefact, and it contains every enabled backend at once."

Both halves fail.

`taichi/rhi/dummy.cpp` is a zero-byte file. The target's sources are
`public_device.h` and that empty file (`taichi/rhi/CMakeLists.txt:114, 120`).
Linking a static archive into a shared object extracts only members that resolve
undefined symbols, and an empty translation unit has none.

It is also not an artefact. Grep over every `CMakeLists.txt` and `*.cmake` in the
tree finds `ti_device_api_shared` at exactly three lines, all in
`taichi/rhi/CMakeLists.txt`: `:114`, `:115`, `:120`. Nothing links it and nothing
installs it. Every real consumer links the static `ti_device_api`:
`cmake/TaichiCore.cmake:146`, `taichi/ui/ggui/CMakeLists.txt:48`,
`taichi/runtime/llvm/CMakeLists.txt:42`,
`taichi/runtime/program_impls/vulkan/CMakeLists.txt:24`.

This matters because pass A offers it as the existing shape of backend
packaging, which feeds 6.3. There is no such shape.

### 3.3 "Each toggle does exactly three things and nothing else."

`report-04:56`, marked **[V]**. False on four counts.
`TI_WITH_CUDA` also drives a `file(GLOB)` and list append at
`cmake/TaichiCore.cmake:104-105`. `TI_WITH_VULKAN` forces `TI_WITH_GGUI` on at
`:62-64`. `TI_WITH_CUDA` with `TI_WITH_CUDA_TOOLKIT` pulls
`find_package(CUDAToolkit REQUIRED)` and a `CUDA::cupti` link at `:278-283`.
`TI_WITH_CUDA`, `TI_WITH_AMDGPU` and `TI_WITH_DX12` set `CUDA_ARCH`,
`AMDGPU_ARCH` and `DX12_ARCH` at `CMakeLists.txt:154-164`, which drive the
bitcode arch loop at `taichi/runtime/llvm/runtime_module/CMakeLists.txt:29`.

### 3.4 `TI_WITH_LLVM` dropped from two enumerations.

`report-04:57-58` lists the `CMAKE_CXX_FLAGS` append sites as
`TaichiCore.cmake:103, 109, 115, 119, 123, 127, 131`. It omits `:94`
(`-DTI_WITH_LLVM`) and `:281` (`-DTI_WITH_CUDA_TOOLKIT`).

`report-04:210` says "Every `#else` is `TI_ERROR(...)`" and lists
`program.cpp:90, 101, 108, 115, 122, 129`. It omits `:94`,
`TI_ERROR("This taichi is not compiled with LLVM")`, which is the `#else` of the
outermost and largest branch. Pass B lists all seven (`report-04b:38-39`).

### 3.5 "No project-defined non-boolean CMake cache variable."

`report-04:137-139`, marked **[V]**. Refuted by `cmake/TaichiCXXFlags.cmake:149`:
`set(HOST_ARCH ${ARCH} CACHE INTERNAL "Host arch")`, where `ARCH` is set to
`"x64"`, `"arm64"` or `"x86"` at `:139, 142, 145`. It is project-defined,
non-boolean, cached, and it is the first element of the bitcode-arch loop at
`taichi/runtime/llvm/runtime_module/CMakeLists.txt:29`. Pass A's narrower
statement about `CACHE STRING` holds; the general one does not. Pass B has this
right (`report-04b:496-500`).

Related: `report-04:130-133` says `HOST_ARCH CUDA_ARCH DX12_ARCH AMDGPU_ARCH`
"are set in root `CMakeLists.txt:154-164`". Only three of the four are. `:155`,
`:159` and `:163` set the arch-specific ones; `HOST_ARCH` is set in
`TaichiCXXFlags.cmake`.

### 3.6 Contradicts itself on 64-bit hardware queries.

`report-04:321-322`, marked **[V]**: "The only hardware query for 64-bit integers
in the entire tree is `vulkan_device_creator.cpp:623-632`."

Pass A's own table three paragraphs earlier (`report-04:307`) records Metal
setting `spirv_has_int64` gated on `feature_64_bit_integer_math`. That flag is
derived from real device queries: `[mtl_device supportsFamily:]` at
`taichi/rhi/metal/metal_device.mm:1027-1036`, reduced to
`feature_64_bit_integer_math = family_apple3` at `:1038`, consumed at `:1051-1053`.

### 3.7 Counting and range slips.

- `report-04:181` — "Thirteen entries" in `taichi/inc/archs.inc.h`. There are
  twelve (`:4-17`), and pass A's own list names twelve. This propagates to
  `report-04:234-235`, where "eight cases against `archs.inc.h`'s thirteen,
  missing js, dx11, dx12, opencl, amdgpu" only balances at twelve: seven real
  `TiArch` values plus `TI_ARCH_RESERVED` (`c_api/include/taichi/taichi_core.h:359-373`),
  and seven plus five missing is twelve.
- `report-04:405` — "`taichi/aot/` is six files, 836 lines", then lists seven.
  Seven files, 836 lines. The error originates in notes entry 1.
- `report-04:157` — `c_api/src/taichi_core_impl.h:120-121`. The comment is at
  `:121`, the array at `:122`. Pass B has it right.
- `report-04:515` — `runtime_lib_dir` at `taichi/util/lang_util.cpp:30-46`. The
  function closes at `:47`.
- `report-04:301` — "Only four backends ever call `set_caps`." True inside
  `taichi/rhi/`, which pass A states as its grep scope, but the report presents
  it as the producer picture (summary point 3, `report-04:24-27`). Two producers
  sit outside that scope: `c_api/src/taichi_opengl_impl.cpp:12` and
  `c_api/src/taichi_vulkan_impl.cpp:53`. Pass A cites
  `taichi_core_impl.cpp:331` separately and never reconciles the two counts.

### 3.8 Gaps in pass A that pass B fills.

- The third hardcoded dispatch chain, `ti_create_runtime`
  (`c_api/src/taichi_core_impl.cpp:247-309`), absent from pass A entirely. Its
  arch coverage differs from both other chains: it has no dx11, dx12, gles or
  amdgpu case.
- `spirv_has_physical_storage_buffer` is dead. Pass A lists it at
  `report-04:290` as 6.2-relevant and never checks it. See §4 below for the
  verification; this is the single most 6.2-relevant fact in the territory and
  pass A does not have it.
- CUDA is pinned to device index 0 (`taichi/rhi/cuda/cuda_context.cpp:21-22`,
  rejected elsewhere at `c_api/src/taichi_core_impl.cpp:289-290`). Directly
  relevant to brief 3.1, which names two cards in this box.
- No dlopen of Taichi's own code. Pass A asserts this in summary point 1
  (`report-04:19-20`) with no citation. Pass B supports it exhaustively
  (`report-04b:373-376`), and the six construction sites it lists are correct:
  `taichi/rhi/cuda/cuda_driver.cpp:34, 89, 98, 107`,
  `taichi/rhi/amdgpu/amdgpu_driver.cpp:33`,
  `taichi/rhi/vulkan/vulkan_loader.cpp:98`.

---

## 4. Pass B — errors

### 4.1 Metal does call `set_caps`. [material]

`report-04b:136-151` claims "`set_caps` is called from exactly seven places
(exhaustive grep over `taichi/` and `c_api/`)" and then, below the table,
"Metal builds its config at `taichi/rhi/metal/metal_device.mm:1044-1070` and
returns it rather than calling `set_caps`."

Wrong on both. `MetalDevice::MetalDevice` at
`taichi/rhi/metal/metal_device.mm:1162-1169` calls
`collect_metal_device_caps(mtl_device)` at `:1167` and `set_caps(std::move(caps))`
at `:1168`. The grep is not exhaustive; there are eight call sites, not seven.
Pass A has this right (`report-04:307`).

The full producer list, which neither report states completely:

| site | scope |
|---|---|
| `taichi/rhi/metal/metal_device.mm:1168` | rhi |
| `taichi/rhi/vulkan/vulkan_device_creator.cpp:876` | rhi |
| `taichi/rhi/vulkan/vulkan_device.cpp:1580` | rhi |
| `taichi/rhi/opengl/opengl_device.cpp:529` | rhi |
| `taichi/rhi/dx/dx_device.cpp:565` | rhi |
| `c_api/src/taichi_opengl_impl.cpp:12` | c_api |
| `c_api/src/taichi_vulkan_impl.cpp:53` | c_api |
| `c_api/src/taichi_core_impl.cpp:331` | c_api, host-supplied |

The declaration is at `taichi/rhi/public_device.h:855`.

### 4.2 The AMDGPU runtime source does not exist.

`report-04b:600-603` says the `TAIHI_CORE_SOURCE` typo at
`cmake/TaichiCore.cmake:111` means "`taichi/runtime/amdgpu/runtime.cpp` is never
added to `TAICHI_CORE_SOURCE`", and that "line 105 spells it correctly for the
CUDA equivalent".

The typo is real. The consequence is not. `taichi/runtime/amdgpu/` contains
`CMakeLists.txt`, `jit_amdgpu.cpp`, `jit_amdgpu.h`, `kernel_launcher.cpp`,
`kernel_launcher.h` and nothing else; `taichi/runtime/cuda/` is the same shape.
Neither `runtime.cpp` exists. The glob at `:110` is empty, so the misspelled
append at `:111` appends nothing, and the correctly spelled append at `:105` also
appends nothing. Pass A got this right (`report-04:604-607`) and verified it by
listing the directories.

### 4.3 Cites the dead copy of `DynamicLoader`.

`report-04b:373-374` defines `DynamicLoader` as
`taichi/system/dynamic_loader.h:12-34`. Nothing in the tree includes that header.
Every include is `taichi/common/dynamic_loader.h`:
`taichi/rhi/cuda/cuda_driver.h:5` and `cuda_driver.cpp:3`,
`taichi/rhi/amdgpu/amdgpu_driver.h:5` and `amdgpu_driver.cpp:3`,
`taichi/rhi/vulkan/vulkan_loader.h:7`. Pass B flags the duplication itself at
`report-04b:605-608`, correctly — the two headers differ only by
`static bool check_lib_loaded(const std::string&)`, which
`taichi/rhi/cuda/cuda_driver.cpp:54` calls — but then uses the unused copy as the
definition. The user list is correct.

### 4.4 `spirv_has_atomic_int64` grep overstated.

`report-04b:181-184`: "no producer and no consumer anywhere in the tree. Grep
returns only the declaration." Grep returns three lines:
`taichi/inc/rhi_constants.inc.h:17`,
`c_api/include/taichi/cpp/taichi.hpp:1121` and
`python/taichi/lang/enums.py:27`. The substantive point — no `set`, no `get` — is
right. Pass A's phrasing at `report-04:339-342` is the accurate one.

### 4.5 Smaller slips.

- `report-04b:99-100` — "Only GGUI uses `target_compile_definitions`
  (TaichiCore.cmake:384)." `taichi/rhi/CMakeLists.txt:28` uses
  `target_compile_definitions(${TAICHI_DEVICE_API} PUBLIC TI_WITH_GLFW)`. True
  within TaichiCore.cmake, not across the build.
- `report-04b:84-87` — "Declared at `cmake/TaichiCore.cmake:1-11`" then lists
  ten. `:1` is `USE_STDCPP`. Eleven `option()` calls in that range. Pass A's count
  is right.
- `report-04b:416` — `ti_get_available_archs` "(171-202)". The function runs
  `c_api/src/taichi_core_impl.cpp:171-205`.
- `report-04b:236-238` — "`cuda_context.cpp:22-23` calls `device_get_count` then
  `device_get(&device_, 0)`". Those are `:21` and `:22`. The finding stands.

### 4.6 Gaps in pass B that pass A fills.

- **The `Extension` system is absent from pass B entirely.** No mention of
  `taichi/program/extension.{h,cpp}` or `taichi/inc/extensions.inc.h`. Pass B's
  §2.3 is headed "64-bit integer capability per backend (brief 6.2)" and does not
  contain the one arch-keyed table in the tree that names 64-bit data. That the
  table turns out to gate nothing (§3.1) is something pass B could not have known
  without looking.
- `CompileConfig::fit()` (`taichi/program/compile_config.cpp:67-76`) against the
  comment at `taichi/program/program.cpp:140`. Pass A's §3.2 and escalation 6 are
  exactly right: `fit()` contains only the debug out-of-bound force at `:68-71`,
  the spirv `demote_dense_struct_fors` at `:72-74`, and
  `disable_offline_cache_if_needed` at `:75`. No arch fallback.
- `is_dx12_api_available()` (`taichi/rhi/dx12/dx12_api.cpp:6-12`) returning
  `true` on `#ifdef TI_WITH_DX12` with no device touched, under a `TI_ASSERT` at
  `taichi/program/program.cpp:87`, with `make_dx12_device()` returning `nullptr`
  at `dx12_api.cpp:14-16`. Brief 5.1 asks specifically for wrongly shaped stubs
  and this is the one in the territory. Pass B does not have it.
- The `ti_device_api` link graph in detail (`taichi/rhi/CMakeLists.txt:7-9, 45,
  51, 57, 81, 86, 90, 95, 100, 104, 108, 111`). All twelve of pass A's line
  numbers here are exact.

---

## 5. Where the two disagree, resolved

| # | Question | A says | B says | Source | Winner |
|---|---|---|---|---|---|
| 1 | Does Metal call `set_caps`? | yes, `metal_device.mm:1168` | no, returns config | `:1167-1168` | **A** |
| 2 | Entries in `archs.inc.h` | thirteen | twelve | `:4-17` = 12 | **B** |
| 3 | Files in `taichi/aot/` | six | seven | 7 files, 836 lines | **B** |
| 4 | Does `taichi/runtime/amdgpu/runtime.cpp` exist? | no, glob empty | implied yes | directory listing | **A** |
| 5 | Where `HOST_ARCH` is set | `CMakeLists.txt:154-164` | `TaichiCXXFlags.cmake:149` | `:149` | **B** |
| 6 | Any project non-boolean cache var? | none | `HOST_ARCH` | `:149` | **B** |
| 7 | `host_result_buffer_` lines | `:120-121` | `:121-122` | `:121-122` | **B** |
| 8 | `set_caps` producer count | four backends (rhi only) | seven, exhaustive | eight sites | **neither** |
| 9 | `spirv_has_atomic_int64` grep | decl + `taichi.hpp:1121` | decl only | 3 hits | **A** |
| 10 | `runtime_lib_dir` extent | `:30-46` | `:30-47` | closes `:47` | **B** |
| 11 | `ti_get_available_archs` extent | `:171-205` | `:171-202` | `:171-205` | **A** |
| 12 | `CMAKE_CXX_FLAGS` append sites | 7, no `:94` | 9, incl. `:94`, `:281` | 9 | **B** |
| 13 | `program.cpp` `#else` arms | 6, no `:94` | 7, incl. `:94` | 7 | **B** |

Nothing on this list is a case of both being wrong in the same direction except
item 8, where the union of the two scopes gives the right answer and neither
report states it.

Where they agree, they are right. I re-verified independently and confirm:
the dead AOT capability gate (`taichi/aot/module_loader.h:97-100` never
overridden; exactly two `get_required_caps` hits, the other being
`c_api/src/taichi_gfx_impl.cpp:21`); the `!=` comparison at
`taichi_gfx_impl.cpp:25`; the `configure_file` precedent writing into
`${CMAKE_SOURCE_DIR}` (`CMakeLists.txt:203-204`); the standalone clang command
and its single `-I ${PROJECT_SOURCE_DIR}` (`runtime_module/CMakeLists.txt:8`);
`runtime.cpp:25` including `taichi/inc/constants.h`; the three arrays at
`runtime.cpp:567-569` and the two at `:562-563`; the exhaustive three-site
footprint of `taichi_max_num_snodes`; the positional enum derivation at
`misc/taichi_json.py:115`; the compute-capability clamp at
`cuda_context.cpp:75-77`; the `${arch}` / `${rtm_arch}` mismatch at
`runtime_module/CMakeLists.txt:13` against `:8`; and pass A's reading of
`.github/workflows/scripts/ti_build/cmake.py`, which I traced through `:49`,
`:57`, `:59-60`, `:84` and `:98` and confirm: an unknown non-boolean name passes
both assertions and renders as `-DNAME=value`.

---

## 6. What both missed

### 6.1 The public C header re-expresses the toggle list, and one of its toggles does not exist

`c_api/include/taichi/taichi.h:9-27` gates every per-backend public header behind
a `TI_WITH_*` macro: `TI_WITH_VULKAN` `:9`, `TI_WITH_OPENGL` `:13`,
`TI_WITH_CUDA` `:17`, `TI_WITH_CPU` `:21`, `TI_WITH_METAL` `:25`.

These are consumer-side macros. The installed package defines none of them:
`cmake/TaichiConfig.cmake.in` is five lines, and
`cmake/TaichiTargets.cmake:56-88` sets only `IMPORTED_LOCATION`,
`IMPORTED_SONAME` and `INTERFACE_INCLUDE_DIRECTORIES`. A downstream consumer that
includes `taichi/taichi.h` therefore gets `taichi_core.h` and nothing else unless
it defines the macros itself, guessing at what the binary was built with.

And `TI_WITH_CPU` is defined nowhere in the repository. Grep across all C++,
CMake and Python sources outside `external/` returns exactly two lines:
`c_api/include/taichi/taichi.h:21` and `:23`. So `taichi_cpu.h` can never be
reached through the umbrella header, even though `cmake/TaichiCAPI.cmake:33`
installs it whenever `TI_WITH_LLVM` is on.

Pass A named three files that re-express the toggle list (`report-04:69-71`).
This is the fourth, it is the one facing the outside world, and it is the
concrete form of pass A's escalation 10 and pass B's install-time-loader
inference at `report-04b:424-427`.

### 6.2 `required_caps` has exactly one producer, and the generic one is unused

Grep for `required_caps` across `taichi/` and `c_api/` returns seven lines. Only
one is a write: `taichi/runtime/gfx/aot_module_builder_impl.cpp:23`.

The generic field `aot::ModuleData::required_caps`
(`taichi/aot/module_data.h:125`, serialised `:135`) is **never written by
anybody**. Its only subclass anywhere is
`ModuleDataDX12 : public aot::ModuleData`
(`taichi/runtime/dx12/aot_module_builder_impl.h:11`, instantiated at
`taichi/runtime/dx12/aot_module_loader_impl.cpp:118`), and the DX12 builder does
not populate it.

Both reports describe a generic AOT capability channel that round-trips —
pass A at `report-04:412-421`, pass B at `report-04b:301-310`. There are two
structs, not one: `gfx::TaichiAotData` (`taichi/runtime/gfx/aot_utils.h:20, 23`),
which is populated and serialised, and `aot::ModuleData`, which is neither. The
declaration side of the capability story is thinner than either report says.

### 6.3 Renumbering the capability enum also invalidates offline cache keys

Both reports raise that inserting a `PER_DEVICE_CAPABILITY` anywhere but the end
of `taichi/inc/rhi_constants.inc.h` renumbers the public C ABI — pass A
escalation 8, pass B escalation 6. There is a second consumer of those numbers.

`taichi/analysis/offline_cache_util.cpp:88-95`,
`get_offline_cache_key_of_device_caps`, serialises `caps.devcaps` directly at
`:92`. That is the raw `std::map<DeviceCapability, uint32_t>`, keyed by the
numeric enum value. Renumbering therefore silently changes offline-cache keys as
well as the C ABI. Neither report has this.

### 6.4 The `.bc` modules are already built into the source tree

Both reports treat the per-arch bitcode runtime as the working adaptive-module
precedent — pass A §5.3, pass B §4.2 — and both escalate whether a generated
`constants.h` should be written into the source tree.

The artefact side already has that property.
`taichi/runtime/llvm/runtime_module/CMakeLists.txt:10` sets
`WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"`, so `runtime_<arch>.bc` is
produced next to `runtime.cpp`, and `:13` installs it from
`${CMAKE_SOURCE_DIR}/taichi/runtime/llvm/runtime_module/`. The in-file TODO at
`:9` says so: "it's better to avoid polluting the source dir, keep in build".

This bears directly on pass A escalation 1 and pass B escalations 1 and 3. If
6.3 multiplies the number of device modules per arch, the multiplication lands in
the source tree under the mechanism as it stands.

### 6.5 The `-I` alternative is not a clean substitution

Pass A escalation 1 (`report-04:623-628`) offers two ways to reach the bitcode
build: generate into `${CMAKE_SOURCE_DIR}`, or extend the `-I` list at
`runtime_module/CMakeLists.txt:8`. The second is not symmetric with the first.

The checked-in `taichi/inc/constants.h` would still exist, and
`-I ${PROJECT_SOURCE_DIR}` would still be on the command line. Two headers of the
same relative path would then both be visible, and which one wins is decided by
`-I` ordering. That is a silently order-dependent build, which is the same class
of failure pass A is trying to avoid. Whichever route is chosen, the source-tree
header has to stop being a header, not merely be shadowed. Neither report says
this.

---

## 7. Escalations

Unresolved. Recorded, not decided.

1. **Pass A escalation 4 needs restating.** With §3.1 established, the question is
   no longer "reconcile two capability systems". It is whether `Extension` is a
   live mechanism in this fork at all, given that the C++ core reads only
   `assertion`, `bls`, `mesh` and `quant` from it, and `data64` is consumed solely
   through the Python binding at `taichi/python/export_lang.cpp:1225`. That is a
   scope decision against brief 1.2.

2. **What an installed Taichi is supposed to tell its loader.** §6.1 makes this
   sharper than either report did. The public umbrella header already assumes the
   consumer knows the build configuration and provides no way to learn it. Brief
   section 5 has a loader configuring the binary at install time. Whether the
   answer is a generated header, a CMake variable in `TaichiConfig.cmake.in`, or a
   C entry point, is a decision the plan does not record.

3. **Whether `TI_WITH_CPU` is a bug to be fixed or a name to be chosen.** It is
   referenced at `c_api/include/taichi/taichi.h:21` and defined nowhere. Either
   the build should define it or the header should use `TI_WITH_LLVM`, which is
   what `cmake/TaichiCAPI.cmake:31-33` actually gates `taichi_cpu.h` on. Not mine
   to settle.

4. **Whether the offline cache key is in scope for 6.2/6.3.** §6.3 couples
   capability renumbering to cache invalidation, and
   `taichi/analysis/offline_cache_util.cpp:110` also loops
   `taichi_max_num_indices`, coupling the cache key to 6.2's related constant.
   The cache format belongs to territory 03; the coupling to the capability enum
   belongs here. Somebody has to own the seam.

5. **The `!=` comparison at `c_api/src/taichi_gfx_impl.cpp:25`.** Both reports
   escalate it and both are right to. I add only that fixing it is not sufficient
   on its own: with `get_required_caps()` never overridden
   (`taichi/aot/module_loader.h:97-100`) and `required_caps` written by exactly
   one producer (§6.2), the comparison operator is the third thing that would have
   to change, not the first.

---

## 8. Summary of what must go back

**Pass A — withdraw or restate:**
`report-04:274-276` (data64 as compile-time blocker, **[V]**, false);
`report-04:84-85` (`ti_device_api_shared`, **[V]**, false);
`report-04:56` ("exactly three things", **[V]**, false);
`report-04:137-139` (no non-boolean cache variable, **[V]**, false);
`report-04:321-322` (only 64-bit hardware query, **[V]**, contradicts its own §4.2);
`report-04:301` (scope of the `set_caps` claim);
`report-04:130-133`, `:181`, `:234-235`, `:405`, `:157`, `:515`, `:57-58`, `:210`
(counts and ranges).

**Pass B — withdraw or restate:**
`report-04b:136-151` (Metal `set_caps`, **[V]**, false, and the grep is not
exhaustive);
`report-04b:600-603` (AMDGPU runtime source consequence);
`report-04b:373-374` (wrong `dynamic_loader.h`);
`report-04b:181-184` (grep overstated);
`report-04b:99-100`, `:84-87`, `:416`, `:236-238` (smaller).

**Both — add:** §6.1 through §6.5.

**Both — keep as is:** the dead AOT capability gate, the bitcode-runtime build
constraint on 6.1, the positional capability ABI, the three-site footprint of
`taichi_max_num_snodes`, and the CUDA compute-capability clamp. Those five are the
load-bearing findings of the territory and both passes got them right.

---

## 9. Divergence from adversary 04-1

`modernization/investigation/adversary-04-1.md` did not exist when I finished, so
there was nothing to compare against. Checked at the point of writing; the
directory contained `adversary-02-1.md` and this file only.
