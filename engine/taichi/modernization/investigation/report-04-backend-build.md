# Report 04 — backend, ahead-of-time and build architecture

Explore agent 04. Territory: `taichi/rhi/`, `taichi/aot/`, `c_api/`, `cmake/` and
the root `CMakeLists.txt`.

Working notes: `modernization/investigation/notes-04-backend-build.md`.
Notes entries 1-18 are the first pass; entries 19-36 are revision 2; entries
37-43 are the amendment pass.

**Revision 2**, after adversarial review by `adversary-04-1.md` and
`adversary-04-2.md`. Five claims are withdrawn and eleven citations corrected;
section 9 lists every one. Nine findings are added. Where the two adversaries
disagreed I re-opened the source and adjudicated myself; that work is in notes
entries 22 and 34 and the outcome is in section 4.4 and escalation 3.

**Revision 3 — amendment pass**, after round-two adversarial review by
`adversary2-04-1.md` and `adversary2-04-2.md`, both of which judge this report
correct and the pair not complete. Seven claims were put to me. I verified each
against source before acting on it; notes entries 37-43 carry the checks, one
per claim. Six claims hold and are absorbed: the surviving gap in section 2.5.1,
the ordered-comparison count in 4.4, the layout mechanism in 2.5, the retraction
of "permanently" in 4.3, the GGUI line number in 2.1 and section 6, and the
missing dlopen citation in summary point 1. One claim is wrong as put and I have
changed nothing for it; section 9.6 says which and why. Section 9.6 lists every
amendment. This pass opened no new line of investigation.

**Revision 4 — framing pass**, after plan section 5.2 was amended (the three
cards are **fidelity** classes, not capability classes and not a vendor target)
and section 2.2 was added (hardware changes speed and granularity, never what the
system can do). Six statements in this report were factually sound but drew their
conclusion from the card list rather than from the source; each is restated in
place, and section 9.7 lists them with what changed. One new finding is added,
section 4.8: **both spines detect tier data and neither publishes it**, which
replaces the reading that tier information is a vendor-path asset. Notes entries
45-51. No finding is withdrawn in this pass and no citation is corrected; what
changes is what the facts are said to mean.

**Revision 5 — amendment pass against the plan dated 2026-09-09**, after
round-three adversarial review by `adversary3-04-1.md` and `adversary3-04-2.md`,
which agree that this report is correct on facts, not complete, and that
consensus is not reached. **I worked against `PROJECT-PLAN.md` as it stands on
2026-09-09**, read in full at the start of this pass, with section 2.2a
(commonised base, vendor branches welcome) and section 5.1a (the install-time
configuration agent sets the SNode field size) in place and with the 2026-09-09
correction to section 5.2. Notes entries 53-60.

Four things change.

1. **A framing this report attributed to plan section 5.2 is withdrawn in full.**
   Revision 4 called the SPIR-V spine "the reference path" and the LLVM spine
   "the accelerant", cited plan section 5.2 for it, and concluded from it that
   one half of item 6.2 is "primary". **The plan contains neither phrase and
   supplies no such ranking.** Section 9.8 lists all seven instances in this
   report and what each now says. The wording traces to an intermediate version
   of section 5.2, quoted verbatim in my own notes at
   `notes-04-backend-build.md:1946-1949`; the current plan withdraws that axis by
   name. No count, census or citation changes; a ranking is removed.
2. **A live defect is recorded and a scope caveat withdrawn as false.** This
   report said GLES is unreachable through the C API. It is reachable, through a
   documented exported function. Section 4.2a and escalation 20.
3. **Two additions neither this report nor any prior file in this territory had:**
   the `TI_VISIBLE_DEVICE` environment selector (3.3a) and `get_device_score`
   (4.8).
4. **Three escalations are closed by plan section 5.1a**, which this report did
   not know existed, and one positive result is stated that nobody has stated
   (2.5.2).

Everything below is marked **[V]** verified by reading the named code, or
**[I]** inferred. Escalations are in section 8.

---

## 1. Summary

Six things determine how much work section 6 is in my territory.

1. **Backend availability is a link-time fact plus a preprocessor define.** No
   registry, no factory, no plugin, no dlopen of Taichi's own code. Selection is
   three hand-written `#ifdef` chains. **[V]** — the dlopen half now cited:
   the only wrapper is `taichi/common/dynamic_loader.cpp:21-31` (`dlopen` at
   `:29`, `LoadLibraryA` at `:27`, `RTLD_NOLOAD` probe at `:16`), and its four
   construction sites all load a **vendor** library, never a Taichi artefact:
   `taichi/rhi/cuda/cuda_driver.cpp:34, 89, 98, 107`,
   `taichi/rhi/amdgpu/amdgpu_driver.cpp:33` and
   `taichi/rhi/vulkan/vulkan_loader.cpp:98` (`libMoltenVK.dylib`, third-party,
   shipped beside the runtime). `taichi/system/dynamic_loader.h` is a dead
   second copy: no `.cpp`, no includer anywhere in the tree. **[V]** (Revision
   3: this was asserted uncited; adversary 04-2 flagged the absence twice.)
2. **The `Arch` vocabulary is build-independent**; only the implementations
   behind it are gated. That split already has the shape 6.3 wants. **[V]**
3. **Capability detection is populated for four backends and empty for the rest,
   including CUDA** — by two independent routes, the RHI device and
   `ProgramImpl::get_device_caps`. **[V]**
4. **A large part of the 64-bit machinery on the SPIR-V spine is already written
   and is switched off**, not absent: `spirv_has_physical_storage_buffer` has two
   producers, both compiled out, and twelve live consumers. **[V]**
5. **There is no handshake of any kind at ahead-of-time module load.** Not the
   capability check (dead), not version (never called), not arch (never called,
   and not recorded in the module). **[V]**
6. **The mechanism 6.1 needs already exists and is in use**: `CUDA_VERSION` is a
   non-boolean CMake value carried into C++ through a generated header and used
   at run time to choose a bitcode file. **[V]**

---

## 2. Build architecture and section 6.1

### 2.1 The toggle mechanism

`cmake/TaichiCore.cmake:1-11` declares eleven options. **[V]**

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

A toggle's **usual** effect is threefold, but it is not the only effect — see
2.2. **[V]**
1. It appends `-DTI_WITH_<X>` to `CMAKE_CXX_FLAGS`. Nine sites:
   `TaichiCore.cmake:94` (LLVM), `:103` (CUDA), `:109` (AMDGPU), `:115` (DX12),
   `:119` (METAL), `:123` (OPENGL), `:127` (DX11), `:131` (VULKAN), `:281`
   (CUDA_TOOLKIT). Four are redundantly repeated at
   `taichi/rhi/CMakeLists.txt:43, 49, 55, 61`. GGUI instead uses
   `target_compile_definitions` at `TaichiCore.cmake:384`, as does
   `TI_WITH_GLFW` at `taichi/rhi/CMakeLists.txt:28`.
2. It gates an `add_subdirectory()` for that backend's rhi/codegen/runtime/
   program_impl.
3. It gates `target_link_libraries()` into `taichi_core` or `ti_device_api`.

Toggles are coerced by host platform before use, not only by the user **[V]**:
`TaichiCore.cmake:35-53` (Apple forces CUDA/OpenGL/AMDGPU off; non-Apple forces
Metal off), `:55-60` (Windows forces AMDGPU off), `:62-64` (Vulkan implies GGUI),
`:66-69` (missing `external/glad/src/gl.c` forces OpenGL off), `:71-75` (no LLVM
forces CUDA, CUDA_TOOLKIT and DX12 off).

### 2.2 A toggle does more than gate compilation

Four cases where a `TI_WITH_*` toggle reaches past the three effects above. **[V]**

- `TaichiCore.cmake:104-105` — `TI_WITH_CUDA` drives a `file(GLOB)` and a
  `list(APPEND)`.
- `TaichiCore.cmake:62-64` — `TI_WITH_VULKAN` forces `TI_WITH_GGUI ON`.
- `TaichiCore.cmake:278-283` — `TI_WITH_CUDA` with `TI_WITH_CUDA_TOOLKIT` runs
  `find_package(CUDAToolkit REQUIRED)` (:279) and links `CUDA::cupti` (:283).
- `CMakeLists.txt:154-164` — `TI_WITH_CUDA` (:155), `TI_WITH_AMDGPU` (:159) and
  `TI_WITH_DX12` (:163) set `CUDA_ARCH`, `AMDGPU_ARCH`, `DX12_ARCH`, which drive
  the bitcode arch loop at `taichi/runtime/llvm/runtime_module/CMakeLists.txt:29`.
  The fourth loop element, `HOST_ARCH`, is set elsewhere — see 2.5.

### 2.3 The RHI aggregation target

`taichi/rhi/CMakeLists.txt:3-4` builds `ti_device_api` as a STATIC library whose
own sources are `arch.cpp`, `device.cpp`, `device_capability.cpp` (:7-9). Each
backend is a separate static lib linked in: `metal_rhi` :45, `opengl_rhi` :51,
`dx_rhi` :57, `vulkan_rhi` :81, `cpu_rhi` :86, `cuda_rhi` :90, `amdgpu_rhi` :95,
`dx12_rhi` :100, `llvm_rhi` :104, plus unconditional `interop_rhi` :108 and
`common_rhi` :111. **[V]**

Its consumers all link the **static** target: `cmake/TaichiCore.cmake:146`,
`taichi/ui/ggui/CMakeLists.txt:48`, `taichi/runtime/llvm/CMakeLists.txt:42`,
`taichi/runtime/program_impls/vulkan/CMakeLists.txt:24`. **[V]**

`taichi/rhi/CMakeLists.txt:114-115, 120` also declares `ti_device_api_shared`.
**It is inert.** Its only compiled source is `taichi/rhi/dummy.cpp`, which is a
**zero-byte file** (`wc -c` → 0); `public_device.h` is a header, not a
translation unit. Linking a static archive into a shared object extracts only
members that resolve undefined symbols, and an empty translation unit has none,
so it contains no backend code. Grep across the tree excluding `external/`
returns `ti_device_api_shared` at exactly those three lines: nothing links it and
nothing installs it. It would export nothing in any case, because
`TaichiCore.cmake:18` sets `CMAKE_CXX_VISIBILITY_PRESET hidden` globally. **[V]**

**There is no shared-library backend artefact in this tree.** (Revision 2: my
first pass claimed the opposite. See section 9.)

### 2.4 Threading a value into `taichi/inc/constants.h` — the mechanism already exists

`taichi/inc/constants.h` is hand-written, not generated:
`taichi_max_num_indices = 12` at `:5`, `taichi_max_num_snodes = 1024` at `:12`,
`kMaxNumSnodeTreesLlvm = 512` at `:13`. **[V]**

**A complete working example of what 6.1 needs is already in the tree, and it is
not the version macros — it is `CUDA_VERSION`.** Verified end to end: **[V]**

| Step | Location |
|---|---|
| Overridable default | `cmake/TaichiCore.cmake:97-100`, `if(NOT CUDA_VERSION) set(CUDA_VERSION 10.0) endif()` |
| Forced when CUDA is off | `CMakeLists.txt:149-152`, set to `"0.0"` |
| Substituted into a header | `taichi/common/version.h.in:5`, `#define CUDA_VERSION "@CUDA_VERSION@"` |
| Generated | `CMakeLists.txt:203`, `configure_file(... ${CMAKE_SOURCE_DIR}/taichi/common/version.h)` |
| Read in C++ | `taichi/common/core.cpp:87-89`, `get_cuda_version_string()` |
| **Chooses a device bitcode file** | `taichi/runtime/llvm/llvm_context.cpp:213-218`, `slim_libdevice.<major>.bc` |

A non-boolean value, set at configure time, carried into C++ through a generated
header, used at run time to pick which device module to load. It reaches CMake
through the same channel any other would: `-DCUDA_VERSION=...` on the command
line, hence `TAICHI_CMAKE_ARGS` (`setup.py:114`) and `DEF_RE` in
`.github/workflows/scripts/ti_build/cmake.py:18`.

`setup.py:144-148` additionally forwards `-DTI_VERSION_MAJOR/_MINOR/_PATCH`,
substituted at `taichi/common/version.h.in:2-4` (the file is 5 lines total).

Note the destination of both: **`${CMAKE_SOURCE_DIR}`, not the binary dir.**

### 2.5 The trap in 6.1

`taichi/runtime/llvm/runtime_module/CMakeLists.txt:8` compiles `runtime.cpp` with
a standalone clang invocation inside `add_custom_target`: **[V]**
```
COMMAND ${CLANG_EXECUTABLE} ${CLANG_OSX_FLAGS} -c runtime.cpp -o "runtime_${rtm_arch}.bc"
        -fno-exceptions -emit-llvm -std=c++17 -D "ARCH_${rtm_arch}" -I ${PROJECT_SOURCE_DIR};
```
Its only include path is `-I ${PROJECT_SOURCE_DIR}`; it inherits no CMake target's
include directories and no `CMAKE_CXX_FLAGS`. `runtime.cpp:25` includes
`taichi/inc/constants.h`, and `runtime.cpp:567-569` declares the three arrays
sized by `taichi_max_num_snodes` that brief 6.1 identifies as the real footprint.

So a generated `constants.h` emitted into the binary directory in the normal
CMake way would leave the device runtime compiling against the stale source-tree
header. **[V]**

**The failure that follows is not a host-versus-device layout disagreement, and
revision 2 said it was.** Adversary 04-1 is right and I have verified it. The
host never lays out `LLVMRuntime`. It is forward-declared and nothing more in
host code — `taichi/program/context.h:11` and `taichi/rhi/llvm/llvm_device.h:8`,
both `struct LLVMRuntime;` — and used only as an opaque pointer
(`context.h:18`, `llvm_device.h:14`). The one definition in the tree is inside
the bitcode translation unit, `runtime.cpp:552`. The host obtains the LLVM type
**by name from the loaded bitcode module**,
`TaichiLLVMContext::get_runtime_type` at
`taichi/runtime/llvm/llvm_context.cpp:1004-1011`
(`llvm::StructType::getTypeByName(..., "struct." + name)`), called for this
struct at `taichi/codegen/llvm/codegen_llvm.cpp:2694-2698`. Fields are read
through accessors compiled **inside** the bitcode —
`STRUCT_FIELD_ARRAY(LLVMRuntime, element_lists)` at `runtime.cpp:616` and
`RUNTIME_STRUCT_FIELD_ARRAY` at `:750` — reached from the host by name, for
example `runtime_query<void *>("LLVMRuntime_get_element_lists", ...)` at
`taichi/runtime/llvm/llvm_runtime_executor.cpp:327-329`. There is no host-side
layout to disagree. **[V]**

The host's only use of `taichi_max_num_snodes` is the assertion at
`taichi/codegen/llvm/struct_llvm.cpp:266`,
`TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);` — which the plan's own
section 6.1 correction records does not guard those arrays, because it bounds a
per-tree count while the arrays are indexed by the process-global `SNode::id`.
**[V]**

So the actual failure of threading the value into the host only is: the
assertion admits more SNodes while the bitcode's arrays stay at the old size,
and the overflow is an out-of-bounds write **inside the bitcode**, at
`runtime.cpp:1005`, `runtime->element_lists[i] = runtime->create<ListManager>(...)`.
The conclusion of this section is unchanged and in fact strengthened — the value
must reach the clang invocation at `runtime_module/CMakeLists.txt:8`, because
the arrays exist only there. **[V]** for the write site and the include
mechanism; **[I]** that the failure is silent rather than caught.

**Adding a directory to that `-I` list does not solve it.** The checked-in
`taichi/inc/constants.h` would still exist and `-I ${PROJECT_SOURCE_DIR}` would
still be on the command line, so two headers of the same relative path would both
be visible and `-I` order would decide the winner. That is the same class of
silent, order-dependent failure the finding is about. Whichever route is chosen,
the source-tree header has to stop being a header, not merely be shadowed. **[V]**
for the two visible headers; **[I]** for the ordering consequence. (Revision 2:
my first pass offered this as a second valid option. It is not one.)

Related, and it settles the hygiene question underneath: **the build already
writes generated artefacts into the source tree**, by design and against its own
TODO. `runtime_module/CMakeLists.txt:10` sets
`WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"`, so `runtime_<arch>.bc` is
produced next to `runtime.cpp`, and `:13` installs it from `${CMAKE_SOURCE_DIR}`.
The TODO at `:9` reads "it's better to avoid polluting the source dir, keep in
build". Generated headers do the same (`CMakeLists.txt:203-204`). **[V]**

### 2.5.1 The clang line already carries a configure-time parameter into the bitcode

This is the gap both round-two adversaries found, and it is in a command line
this report already quotes verbatim at 2.5. I had not read past the `-I`.
Verified end to end. **[V]**

`-D "ARCH_${rtm_arch}"` is not a fixed flag. It is a CMake value, substituted per
iteration, and it is consumed by the preprocessor **inside** the bitcode
translation unit.

| Step | Location |
|---|---|
| Configure-time values | `HOST_ARCH` — `cmake/TaichiCXXFlags.cmake:149`, `CACHE INTERNAL`, from `ARCH` at `:139/:142/:145`. `CUDA_ARCH` — `CMakeLists.txt:154-156`. `AMDGPU_ARCH` — `:158-160`. `DX12_ARCH` — `:162-164`. |
| Iterated into a function parameter | `runtime_module/CMakeLists.txt:29-31`, `foreach(arch IN LISTS HOST_ARCH CUDA_ARCH DX12_ARCH AMDGPU_ARCH)` → `compile_llvm_runtime(${arch})` → `rtm_arch` |
| **Compiled into the bitcode** | `:8`, `-D "ARCH_${rtm_arch}"` |
| Consumed inside the bitcode | **21 sites.** 19 in `runtime.cpp` at `:51, 119, 155, 347, 771, 798, 801, 860, 1144, 1156, 1167, 1293, 1343, 1431, 1549, 1633, 1651, 1861, 1865`; plus `locked_task.h:7` (`#if ARCH_x64 \|\| ARCH_arm64`) and `node_pointer.h:15` (`#if defined(ARCH_cuda)`), both included by `runtime.cpp` at `:1872` and `:1660` and therefore part of the same translation unit |
| One artefact per value | `runtime_${rtm_arch}.bc`, `:8` |
| Installed | `:13` |
| Selected at run time by filename | `get_runtime_fn(Arch)`, `taichi/runtime/llvm/llvm_context.cpp:209-211`, `fmt::format("runtime_{}.bc", arch_name(arch))` |

Three consequences, all bearing on 6.1. **[V]** for each except where marked.

1. **It is the only channel by which a configure-time value reaches the bitcode
   today, and it already exists.** The clang line inherits no `CMAKE_CXX_FLAGS`
   and no target include directories (2.5), so `-D` and `-I` are the whole
   interface. One of the two is already in use for a project-owned parameter.
2. **It narrows escalation 1.** Revision 2 stated that only one route is sound.
   That was reached by considering the `-I` list and the generated-header route
   and never the `-D` list on the same line. Withdrawn; escalation 1 is now a
   choice among three routes.
3. **It is a closer precedent for 6.1 and 6.3 together than `CUDA_VERSION`
   (2.4) or the AMDGPU ISA selection (5.3).** Both of those *select* among
   vendor-supplied prebuilts. This one is a project-owned parameter that
   produces, installs and selects project-built modules, which is the whole
   shape brief section 5 describes.

Two constraints on using it, both checked before recording. **[V]** for the
first; **[I]** for the second.

- **The existing form defines a bare name, not a value.** `taichi/inc/constants.h`
  declares `constexpr int taichi_max_num_snodes = 1024;` at `:12`, so
  `-D taichi_max_num_snodes=4096` would macro-expand the declaration to
  `constexpr int 4096 = 1024;` and fail to compile. Reaching the constant this
  way needs a distinct macro name that `constants.h` reads. That is a design
  decision and is not mine to make.
- **The variant is keyed by filename, and the filename carries only the arch.**
  `get_runtime_fn` formats `runtime_<arch>.bc` and nothing else, so a second
  parameter either folds into that name or fixes one ceiling per build. **Plan
  section 5.1a settles which**, and 2.5.2 states the consequence.

### 2.5.2 One ceiling per build is what the existing build loop already produces, with no filename change. New in revision 5.

Revisions 1 to 4 of this report did not know plan section 5.1a existed; a grep
for "5.1a" over this report and `notes-04-backend-build.md` returned nothing
before this pass. Section 5.1a settles that the install-time configuration agent
sets the SNode field size, that "per build already means per device
configuration", that a two-card machine gets two binaries with one or both
activated, and that **"the filename scheme does not need to grow, and no common
denominator has to be found."**

**The positive result, which nobody in this territory has stated: the machinery
already behaves that way. It is not merely permitted by section 5.1a — it is what
the existing loop does.** Verified in this pass. **[V]**

1. **Every bitcode artefact in one configure carries one ceiling, by
   construction.** The loop at `runtime_module/CMakeLists.txt:29-31` varies one
   thing, `arch`. The clang line at `:8` interpolates `rtm_arch` in exactly two
   places, the output filename and the `-D "ARCH_${rtm_arch}"` token. A second
   `-D` carrying a ceiling would therefore take the **same value on every
   iteration**, because nothing else in the loop varies. Adding it needs no
   change to `get_runtime_fn` (`llvm_context.cpp:209-211`) and no change to the
   filename scheme.
2. **The host agrees with the bitcode by construction too.** The one host-side
   coupling to the constant is the assertion
   `TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes)` at
   `taichi/codegen/llvm/struct_llvm.cpp:266`, compiled into `taichi_core` in the
   **same** configure that runs the loop. Host and every bitcode variant read the
   same configured value. **[V]** for both the assertion line and the single
   configure; **[I]** that the two therefore cannot disagree, which is reasoning
   about the build rather than a build I ran.

So the constraint recorded at the end of 2.5.1 — "either grows the filename
scheme or fixes one ceiling per build" — resolves to the second branch, and the
second branch is what plan section 5.1a asks for. **What remains open is the
sizing rule, which is plan section 8.1 item 2 and not a mechanism question, plus
the choice of route in escalation 1.** Escalation 16 is closed on this ground;
see 9.9.

### 2.6 Constraints on adding a non-boolean build parameter

**[V]** Non-boolean CMake variables that already control this build:
- `CUDA_VERSION` — a plain variable, `TaichiCore.cmake:97-100` (see 2.4).
- `HOST_ARCH` — `cmake/TaichiCXXFlags.cmake:149`,
  `set(HOST_ARCH ${ARCH} CACHE INTERNAL "Host arch")`, where `ARCH` is `"x64"`
  (:139), `"arm64"` (:142) or `"x86"` (:145). It is the first element of the
  bitcode-arch loop at `runtime_module/CMakeLists.txt:29`.

The remaining `CACHE` uses are: `CMAKE_BUILD_TYPE` (`CMakeLists.txt:41`, CMake's
own builtin); `CACHE BOOL` forcings of third-party submodule options at
`cmake/TaichiTests.cmake:7`, `cmake/TaichiCAPITests.cmake:7`,
`taichi/rhi/CMakeLists.txt:18-20, 23`; and `CACHE PATH` at
`c_api/cmake/FindTaichi.cmake:83`, which is for consumers of an install. The 24
`option()` calls across `cmake/`, root `CMakeLists.txt`, `taichi/` and `c_api/`
are all boolean. **[V]**

**[V]** The CI build tooling tolerates a non-boolean.
`.github/workflows/scripts/ti_build/cmake.py:17` `OPTION_RE` matches only
`option(NAME "desc" ON|OFF)` with an optional `# wheel-tag:`, collected from
`CMakeLists.txt` and `cmake/*.cmake` (:147). A name it did not collect yields
`default=None`, `is_bool=False`, `wheel_tag=""` at `:57`; the assertion at `:60`
passes and the one at `:84` ("Set a non boolean value to an option with
wheel-tag") passes on the empty tag. `render()` at `:98` emits `-DNAME=value`.
The parameter simply would not appear in `render_wheel_tag()` (:109-116). This is
how `CUDA_VERSION` already travels.

### 2.7 Constant duplication across the c_api boundary

**[V]** `c_api/src/taichi_core_impl.h:121-122`:
```
  // 32 is a magic number in `taichi/inc/constants.h`.
  std::array<uint64_t, 32> host_result_buffer_;
```
That is `taichi_result_buffer_entries = 32` (`constants.h:20`) copied as a
literal rather than included. Not in 6.1's path, but direct evidence that
constants cross into `c_api/` by duplication.

**[V]** No hardcoded `1024` duplicate of `taichi_max_num_snodes` exists in my
territory. The only file there that includes `constants.h` is
`taichi/rhi/llvm/allocator.h:6`, and it uses none of the three constants (its
members are `std::size_t` and `uint8_t*`, `allocator.h:22-25`).

### 2.8 A CMake operator-precedence bug

`taichi/rhi/CMakeLists.txt:17`: **[V]**
```
if (TI_WITH_OPENGL OR TI_WITH_VULKAN AND NOT ANDROID)
```
CMake binds `NOT` tighter than `AND`, and `AND` tighter than `OR`, so this parses
as `TI_WITH_OPENGL OR (TI_WITH_VULKAN AND (NOT ANDROID))`. The `NOT ANDROID`
guard does not apply to the OpenGL arm: with OpenGL on and `ANDROID` set, GLFW is
still added (`:29`), linked (`:30`) and put on the public include path (`:31-34`).
**[V]** for the text and the precedence rule; **[I]** for the build outcome, which
I did not execute. Recording only. Whether Android is a target is not settled
anywhere in the brief.

---

## 3. Backend selection and registration

### 3.1 The `Arch` vocabulary is build-independent

`taichi/rhi/arch.h:7-12` builds `enum class Arch : int` from
`taichi/inc/archs.inc.h` via `PER_ARCH`. **Twelve** entries (`:4-17`): x64,
arm64, js, cuda, metal, opengl, dx11, dx12, opencl, amdgpu, vulkan, gles. **[V]**

`taichi/rhi/arch.cpp` contains **zero** `#ifdef TI_WITH_*`. `arch_name` (:6-18),
`arch_from_name` (:20-39), `arch_is_cpu` (:42-48), `arch_is_cuda` (:50-52),
`arch_uses_llvm` (:54-57), `arch_is_gpu` (:59-61), `arch_uses_spirv` (:63-66),
`arch_use_host_memory` (:78-80) and `default_simd_width` (:82-93) are pure
functions of the enum. `host_arch()` (:68-76) is the one preprocessor decision,
on `TI_ARCH_x64` / `TI_ARCH_ARM`, else `RHI_NOT_IMPLEMENTED`. **[V]**

Two classifications matter downstream: `arch_uses_llvm` = {x64, arm64, cuda,
dx12, amdgpu} (`arch.cpp:55-56`); `arch_uses_spirv` = {opengl, gles, vulkan,
dx11, metal} (`arch.cpp:64-65`). **[V]**

### 3.2 Three hardcoded selection chains

**Chain 1 — runtime execution.** `taichi/program/program.cpp:80-133`. **[V]**

| Arch | Lines | Guard | Availability probe | Impl |
|---|---|---|---|---|
| x64/arm64/cuda/amdgpu | 80-83 | `TI_WITH_LLVM` | none | `LlvmProgramImpl` |
| dx12 | 84-92 | `TI_WITH_LLVM`+`TI_WITH_DX12` | `directx12::is_dx12_api_available()` :87 | `Dx12ProgramImpl` |
| metal | 96-102 | `TI_WITH_METAL` | `metal::is_metal_api_available()` :98 | `MetalProgramImpl` |
| vulkan | 103-109 | `TI_WITH_VULKAN` | `vulkan::is_vulkan_api_available()` :105 | `VulkanProgramImpl` |
| dx11 | 110-116 | `TI_WITH_DX11` | `directx11::is_dx_api_available()` :112 | `Dx11ProgramImpl` |
| opengl | 117-123 | `TI_WITH_OPENGL` | `opengl::initialize_opengl(false)` :119 | `OpenglProgramImpl` |
| gles | 124-130 | `TI_WITH_OPENGL` | `opengl::initialize_opengl(true)` :126 | `OpenglProgramImpl` |
| other | 131-133 | — | — | `TI_NOT_IMPLEMENTED` |

Seven `#else` arms, each `TI_ERROR("This taichi is not compiled with X")`:
`:90` (DX12), `:94` (LLVM — the outermost and largest branch), `:101` (Metal),
`:108` (Vulkan), `:115` (DX11), `:122` and `:129` (OpenGL). Every probe is a
`TI_ASSERT`, not a graceful fallback. **[V]**

**Chain 2 — ahead-of-time module load.** `taichi/aot/module_loader.cpp:31-58`.
See 5.2. **[V]**

**Chain 3 — C API runtime creation.** `c_api/src/taichi_core_impl.cpp:247-309`, a
`switch (arch)` with `#ifdef`-guarded cases: `TI_ARCH_VULKAN` :252-267,
`TI_ARCH_OPENGL` :268-274, `TI_ARCH_X64` :275-281, `TI_ARCH_ARM64` :282-287,
`TI_ARCH_CUDA` :288-293, `TI_ARCH_METAL` :295-301, `default` →
`TI_CAPI_NOT_SUPPORTED(arch)` :302-305. No gles, dx11, dx12 or amdgpu case. **[V]**

The three chains cover different arch sets. Adding or swapping a backend means
editing all of them. There is no seam a module could be loaded into. **[V]**

`CompileConfig::fit()` (`taichi/program/compile_config.cpp:67-76`) contains **no
arch fallback logic** — only the debug out-of-bound force (:68-71), the spirv
`demote_dense_struct_fors` (:72-74) and `disable_offline_cache_if_needed` (:75) —
despite the comment at `program.cpp:140`, "Must have handled all the arch
fallback logic by this point". **[V]**

Default arch is `host_arch()`, set at `compile_config.cpp:10-11`. **[V]**

### 3.3 Device index: CUDA is pinned to device 0

`taichi/rhi/cuda/cuda_context.cpp:21-22` reads the device count and then ignores
it: **[V]**
```
  driver_.device_get_count(&dev_count_);
  driver_.device_get(&device_, 0);
```
And `ti_create_runtime` rejects any non-zero `device_index` for every arch except
Vulkan: `TI_CAPI_NOT_SUPPORTED_IF_RV(device_index != 0)` at
`c_api/src/taichi_core_impl.cpp:270` (opengl), `:277` (x64), `:283` (arm64),
`:289` (cuda), `:297` (metal). Only Vulkan honours it, through
`set_vulkan_visible_device(std::to_string(device_index))` at `:254-255`. **[V]**

**Framing, restated in revision 4.** Revision 2 ended this section with "Brief
3.1 names two NVIDIA cards in this box. Today CUDA can only see device 0." That
made the finding sound like a CUDA problem raised because of the owner's
hardware. It is not. The device-index refusal at
`c_api/src/taichi_core_impl.cpp:270, 277, 283, 289, 297` covers **opengl, x64,
arm64, cuda and metal alike**, and the one arch that honours a device index is
Vulkan (`:254-255`), which is on the SPIR-V spine. So the tree's only
multi-device support sits on the SPIR-V spine, and the shortfall is on the LLVM
spine. The CUDA pin at `cuda_context.cpp:21-22` is one instance of a refusal that
spans the enum, not the substance of it. **[V]**

**How large the shortfall is, stated with the dispute intact — revision 5.**
Revision 4 wrote "the gap covers four archs — including CPU — cannot express a
second device at all". That is literally true of the refusal sites and is
disputed as a *gap*. `adversary3-04-2.md` §12.5 argues that on `x64` and `arm64`
there is no second host device for a non-zero index to name, so the refusal is
the correct answer rather than a missing feature; I verified its evidence —
`taichi/rhi/cpu/` is three files (`CMakeLists.txt`, `cpu_device.cpp`,
`cpu_device.h`) and a grep for `device_index`, `device_get_count` or
`num_devices` across `taichi/rhi/cpu/` and `taichi/rhi/llvm/` returns nothing,
and `host_arch()` (`taichi/rhi/arch.cpp:68-76`) returns exactly one arch.
`adversary3-04-1.md` claim 7 argues the opposite way, that CPU being covered is
what lets the finding stand enum-wide without the card list. **[V]** for the
evidence on both sides; the reading is a judgement the source does not settle,
and I record both rather than picking. Escalation 21.

What is not in dispute: the substantive shortfall on the LLVM spine is `cuda`,
which has a case and refuses, plus `amdgpu` and `dx12`, which have no
`ti_create_runtime` case at all — verified against the switch at
`c_api/src/taichi_core_impl.cpp:251-305`, whose only cases are vulkan, opengl,
x64, arm64, cuda and metal. **[V]**

**The CUDA pin is an ordinal pin, and that is why it does not contradict plan
section 5.1a. New in revision 5.** Section 5.1a records that the project owner
has run the engine on both cards at once, calls it direct observation, and says
it outranks any code-reading conclusion. Read baldly, "CUDA can only see device
0" reads as a challenge to that. It is not one.
`driver_.device_get(&device_, 0)` (`cuda_context.cpp:22`) pins **ordinal** 0, not
a physical card, and the CUDA driver's own `CUDA_VISIBLE_DEVICES` decides which
physical card is ordinal 0 — outside Taichi, which reads no such variable: I
grepped `CUDA_VISIBLE_DEVICES` across `taichi/`, `c_api/` and `python/` and it
returns nothing. So two processes launched with different values of that driver
variable each get a different card, which is consistent with the observation.
**The severity is one-process-one-card, not one-machine-one-card.** **[V]** for
the pin, the absent grep and the enum-wide refusal; **[I]** for the composed
reconciliation, which I read rather than ran. This qualification is owed to
`adversary3-04-1.md` §5.6 and was absent from this report.

### 3.3a `TI_VISIBLE_DEVICE`: a second device-selection channel, out of process. New in revision 5.

Absent from this report through four revisions, from `notes-04-backend-build.md`,
and from all four prior adversary files in this territory. Both round-three
adversaries found it independently. I verified the whole chain. **[V]**

`taichi/rhi/vulkan/vulkan_loader.cpp:111-113`, inside `VulkanLoader::init`:
```
    const char *id = std::getenv("TI_VISIBLE_DEVICE");
    if (id) {
      set_vulkan_visible_device(id);
    }
```

| Step | Location |
|---|---|
| Environment read | `vulkan_loader.cpp:111-113`, last statement of the `std::call_once` body opened at `:87` |
| Stored on the loader singleton | `set_vulkan_visible_device` at `:144-146`, writing `VulkanLoader::instance().visible_device_id`, a plain `std::string` member at `vulkan_loader.h:32`; `instance()` is a function-local static at `:14-17` |
| Parsed and bounds-checked | `vulkan_device_creator.cpp:442-451`: `std::stoi` at `:445`, range test against `device_count` at `:446`, `RHI_LOG_ERROR` "TI_VISIBLE_DEVICE=%d is not valid, found %d devices available" at `:448-451` |
| Admitted only if scored non-zero | `:452-455`, `else if (get_device_score(devices[id], test_surface))` |
| Otherwise falls through to the automatic pick | `:458-468`, highest score wins |

**Why it matters against the current plan.** Section 5 puts configuration at
install time and section 5.1a puts the configuration agent **outside the
process**, with a multi-GPU machine as the ordinary case and the agent possibly
choosing between cards or using more than one. An environment variable read at
loader init is exactly the shape of channel such an agent can use, with no C API
call and no source change. It is the **second** out-of-process channel in the
tree, alongside `TI_LIB_DIR` in `runtime_lib_dir()`
(`taichi/util/lang_util.cpp:35-45`), which this report already cites for module
location. So device selection on Vulkan has two routes in, and revisions 1 to 4
of this report found one. **[V]** for both channels; **[I]** for the reading that
either is the intended install-time interface, which the code does not say.

**And the two routes collide, order-dependently.** **[V]** for each step; **[I]**
for the composed outcome, which I read rather than ran.

- The selector is **process-global, not per-runtime**: one `std::string` on a
  function-local static, shared by every runtime in the process
  (`vulkan_loader.h:14-17, 32`). Serial creation works only because the value is
  consumed during construction, at `vulkan_device_creator.cpp:442`.
- The environment read sits inside `std::call_once` (`vulkan_loader.cpp:87`), and
  `VulkanDeviceCreator::VulkanDeviceCreator` calls `VulkanLoader::instance().init()`
  at `vulkan_device_creator.cpp:236` — **after** `ti_create_runtime` has already
  written the caller's `device_index` at `c_api/src/taichi_core_impl.cpp:254-255`.
- So on the **first** Vulkan runtime in a process, if `TI_VISIBLE_DEVICE` is set,
  the `call_once` body overwrites the index the caller passed. On the **second
  and later** runtimes the flag has fired, so the caller's argument stands. Which
  input wins depends on whether `init()` ran earlier in the process — for
  instance through `is_vulkan_api_available()`, reached from
  `ti_get_available_archs` at `c_api/src/taichi_core_impl.cpp:179`.

Which of the two inputs is authoritative is recorded nowhere. Escalation 12,
restated.

### 3.4 What the C API exposes

`ti_get_available_archs` (`c_api/src/taichi_core_impl.cpp:171-205`) is the only
backend-enumeration API. It caches into `thread_local std::vector<TiArch>`
(:176) and probes six archs (:178-195). The probes are at `:13-61`: vulkan,
opengl, cuda and metal do real API probing behind their `#ifdef`; x64 (:37-45)
and arm64 (:47-53) are pure preprocessor. **[V]**

Its documented contract, `c_api/include/taichi/taichi_core.h:883-897`: an arch is
available only if "1. The Runtime library is compiled with its support;
2. The current platform is installed with a capable hardware or an emulation
software", and otherwise "a call to `ti_create_runtime` with that arch is
guaranteed failing". It also warns the returned order is undefined. That is
exactly the two-part availability question 6.3 is about, already stated as the C
API's contract. **[V]**

`TiArch` (`taichi_core.h:358-375`) has eight enumerators, one of which is
`TI_ARCH_RESERVED = 0`, so it **names 7 of the 12 archs**. Missing: `js`, `dx11`,
`dx12`, `opencl`, `amdgpu`. Unlike `TiCapability`, `TiArch` is a hand-written case
list in `c_api/taichi.json`, not derived from the `.inc.h`. **[V]**

`ti_get_runtime_capabilities` (`taichi_core_impl.cpp:336-366`) reads
`runtime->get().get_caps()`. `ti_set_runtime_capabilities_ext` (:317-334)
**overrides** the device capability set wholesale at `:331`, with no validation
against the hardware. **[V]**

### 3.5 The installed package records nothing, and the public header assumes it does

`cmake/TaichiConfig.cmake.in` is five lines. `cmake/TaichiTargets.cmake:56-88`
declares one IMPORTED target `taichi_c_api` and sets only `IMPORTED_LOCATION`,
`IMPORTED_SONAME` and `INTERFACE_INCLUDE_DIRECTORIES`. Neither carries
`TI_WITH_*` state, capability data or tier information. Both are hand-written,
per the comment at `cmake/TaichiCAPI.cmake:144-146`. **[V]**

Meanwhile the public umbrella header **assumes the consumer already knows**.
`c_api/include/taichi/taichi.h:1-29` gates each per-backend header on a
consumer-side macro: `TI_WITH_VULKAN` :9, `TI_WITH_OPENGL` :13, `TI_WITH_CUDA`
:17, `TI_WITH_CPU` :21, `TI_WITH_METAL` :25. Since the install defines none of
them, a downstream consumer including `taichi/taichi.h` gets `taichi_core.h` and
nothing else unless it defines the macros itself, guessing at the build
configuration. **[V]**

**This is the fourth re-expression of the toggle list**, after
`cmake/TaichiCore.cmake:1-11`, `taichi/rhi/CMakeLists.txt:42-105` and
`cmake/TaichiCAPI.cmake:31-60`. It is the one facing outward. **[V]**

**And one of its toggles does not exist.** Grep for `TI_WITH_CPU` across the repo
excluding `external/` and `build/` returns `taichi.h:21`, `taichi.h:23`, and
`docs/lang/articles/deployment/tutorial.md:288`, where it is listed under "Other
commonly used CMake options" beside `TI_WITH_OPENGL` and `TI_WITH_CUDA`. It is
neither an `option()` nor a define anywhere in the build. So `taichi_cpu.h` is
unreachable through the umbrella header, although `cmake/TaichiCAPI.cmake:33`
installs it whenever `TI_WITH_LLVM` is on. **[V]**

---

## 4. Capability detection

### 4.1 `Extension` — a per-arch table that gates nothing in C++

`taichi/program/extension.h:17-24`, enum from `taichi/inc/extensions.inc.h` (ten
entries). `taichi/program/extension.cpp:8-33` is a hardcoded
`static std::unordered_map<Arch, std::unordered_set<Extension>>`. **[V]**

The table's contents are as I first reported: `data64` is present for `Arch::x64`
(`extension.cpp:12`), `arm64` (:16) and `cuda` (:20), and absent for `amdgpu`
(:22), `metal` (:23), `opengl` (:24), `gles` (:25), `vulkan` (:26) and `dx11`
(:27). `dx12`, `js` and `opencl` are absent from the map entirely. **[V]**

**But `Extension::data64` is never queried from C++.** Every call to
`is_extension_supported` in `taichi/` and `c_api/`: **[V]**
- `taichi/program/program.cpp:149` — `assertion`
- `taichi/codegen/llvm/codegen_llvm.cpp:2726` — `bls`
- `taichi/transforms/compile_to_offloads.cpp:92, 205, 218, 236` — `mesh`
- `taichi/transforms/compile_to_offloads.cpp:245, 288` — `quant`
- `taichi/python/export_lang.cpp:1225` — the pybind export

**No site passes `data64`.** Its only consumers are Python:
`python/taichi/lang/misc.py:186` and test-skip decorators such as
`tests/python/test_svd.py:8`, `tests/python/test_quant_atomics.py:44`,
`tests/python/test_ad_basics.py:249`. Brief 1.2 puts the Python front end out of
scope. **[V]**

**So the table is not a blocker on 6.2.** It is a declaration that only
out-of-scope code reads. (Revision 2: my first pass called it a compile-time
blocker and marked that verified. It was false. See section 9.)

Two details worth recording, neither proposing anything:
- `extension.cpp:29-30` carries an abandoned attempt to make it dynamic:
  `// if (with_opengl_extension_data64()) / // arch2ext[Arch::opengl].insert(Extension::data64); // TODO: singleton`. **[V]**
- `extension.cpp:31` reads `arch2ext[arch]` — non-const `operator[]` on a
  function-static map — so a lookup on an absent key (dx12, js, opencl)
  **inserts** into shared static state, unsynchronised. **[V]**

### 4.2 `DeviceCapability` — per-device, runtime-populated, and empty on CUDA

`taichi/rhi/device_capability.h:11-15`, enum from
`taichi/inc/rhi_constants.inc.h:8-35`, 25 entries. The 6.2-relevant ones:
`spirv_version` :11, `spirv_has_int64` :14, `spirv_has_atomic_int64` :17,
`spirv_has_physical_storage_buffer` :28. **[V]**

Storage is `DeviceCapabilityConfig`, a `std::map<DeviceCapability, uint32_t>`
(`device_capability.h:20-33`) with `contains`/`get`/`set` at
`device_capability.cpp:29-42`. `get()` on a missing key returns 0
(`device_capability.cpp:38`), so absent silently means "not supported, level 0",
with no distinction between "not probed" and "probed and absent". It lives on the
RHI `Device` base: `taichi/rhi/public_device.h:617`, accessors `:852` and `:855`. **[V]**

**Eight `set_caps` call sites** across `taichi/` and `c_api/`, excluding the
declaration at `public_device.h:855`: **[V]**

| # | Site | Real device query | Sets `spirv_has_int64` |
|---|---|---|---|
| 1 | `taichi/rhi/metal/metal_device.mm:1168` (built by `collect_metal_device_caps`, :1017) | yes | yes, :1052, gated on `feature_64_bit_integer_math` |
| 2 | `taichi/rhi/vulkan/vulkan_device_creator.cpp:876` (built from :476) | yes | yes, :632 |
| 3 | `taichi/rhi/vulkan/vulkan_device.cpp:1580` (built :1578) | no, only `spirv_version=0x10000` :1579 | no |
| 4 | `taichi/rhi/opengl/opengl_device.cpp:529` (built :508) | partly | yes, :511, gated on `!is_gles()` |
| 5 | `taichi/rhi/dx/dx_device.cpp:565` (built :563) | no, only `spirv_version=0x10300` :564 | no |
| 6 | `c_api/src/taichi_opengl_impl.cpp:12` | no | yes, :9, **unconditionally** |
| 7 | `c_api/src/taichi_vulkan_impl.cpp:53` | no | no |
| 8 | `c_api/src/taichi_core_impl.cpp:331` | host-supplied, no validation | caller's choice |

**CUDA, AMDGPU, CPU and DX12 RHI devices never call `set_caps`.** Their `caps_`
stays empty and every `get()` returns 0. **[V]**

That is internally consistent — those archs go through LLVM codegen, which does
not consult `DeviceCapability` — but it means there is no device-capability record
whatsoever on the whole `arch_uses_llvm` spine, `{x64, arm64, cuda, dx12,
amdgpu}` (`taichi/rhi/arch.cpp:54-57`). **[V]** for the absence; **[I]** that
`ti_get_runtime_capabilities` therefore reports zero capabilities for a runtime
on any of them.

(Revision 4: revision 2 ended that sentence "on the CUDA path, which is the path
the section 5.2 target tiers are about". That named one arch out of five because
of the cards in section 5.2, and the cards are incidental test hardware. The
emptiness is not a CUDA fact. It covers `x64` and `arm64` — CPU-only operation,
a first-class target under plan section 2 — as well as `amdgpu` and `dx12`. See
4.7.)

**A second, independent route to the same emptiness.**
`ProgramImpl::get_device_caps()` (`taichi/program/program_impl.h:170-172`)
returns `{}`, with exactly one override, `GfxProgramImpl::get_device_caps`
(`taichi/runtime/program_impls/gfx/gfx_program.cpp:82`, declared
`gfx_program.h:85`). It is surfaced by `Program::get_device_caps`
(`taichi/program/program.h:136-137`) and feeds kernel compilation at
`taichi/aot/graph_data.cpp:36` and six sites in
`taichi/program/snode_rw_accessors_bank.cpp:45, 54, 68, 80, 89, 100`. So the LLVM
archs also compile kernels against an empty capability set through this path. **[V]**

**Two backends query hardware for 64-bit integers**, not one:
- Vulkan, `vulkan_device_creator.cpp:623-632`, via
  `vkGetPhysicalDeviceFeatures` → `device_supported_features.shaderInt64`.
- Metal, `taichi/rhi/metal/metal_device.mm:1027-1036` via
  `[mtl_device supportsFamily:]`, reduced to `feature_64_bit_integer_math` at
  `:1038`, consumed at `:1051-1053`. **[V]**

**The C API discards OpenGL's real detection.** `GLDevice::GLDevice`
(`opengl_device.cpp:506-529`) sets `spirv_has_int64`/`float64` only when
`!is_gles()` (:509-513, comment at :510 "64bit isn't supported in ES profile"),
plus int16/float16 gated on `GLAD_GL_NV_gpu_shader5` (:515-518),
`GLAD_GL_AMD_gpu_shader_int16` (:520-522) and `GLAD_GL_AMD_gpu_shader_half_float`
(:524-526). `OpenglRuntime::OpenglRuntime`
(`c_api/src/taichi_opengl_impl.cpp:4-13`) then builds a fresh config (:8) and
calls `set_caps` again (:12), setting `spirv_has_int64` unconditionally (:9) with
no `is_gles()` guard and dropping the int16/float16 detection entirely. **[V]**

**WITHDRAWN IN REVISION 5, AND IT WAS FALSE.** Revisions 1 to 4 carried this
scope caveat: "`ti_create_runtime` has no `TI_ARCH_GLES` case, and `GLDevice`
here calls `initialize_opengl(false, true)` (`opengl_device.cpp:507`), so a GLES
context is not reachable through this path today." **A GLES context IS reachable
through the C API**, by a different entry point I had not looked for. The
withdrawn sentence is kept visible here because it is cited by two adversary
files and by escalation 20. What survives it: `TI_ARCH_GLES = 7` does exist in
the enum (`taichi_core.h:373`) with no `ti_create_runtime` case, and the
wholesale replacement and the lost int16/float16 detection are real. Section 4.2a
carries the correction. **[V]**

**GLES is named in two of the three dispatch chains and absent from the third.**
Re-verified in revision 4: `Arch::gles` appears at
`taichi/program/program.cpp:124` (chain 1, via `initialize_opengl(true)` at
`:126`) and `taichi/aot/module_loader.cpp:40` (chain 2), and it is classified by
`arch_uses_spirv` at `taichi/rhi/arch.cpp:64`. In the C API it has neither a
`ti_create_runtime` case (chain 3, 3.2) nor a probe in `ti_get_available_archs`,
whose six probes are vulkan, metal, cuda, x64, arm64 and opengl
(`c_api/src/taichi_core_impl.cpp:178-195`). So the arch is nameable and
dispatchable in-tree and unreachable through the C API. **[V]**

**Weighed against plan sections 2 and 5.3, this is not a cosmetic gap.** Those
sections put edge hardware first and make modern capability an opportunity rather
than a prerequisite. The `!is_gles()` guard at `opengl_device.cpp:509-513` exists
precisely to keep a low-end profile from being told it has 64-bit integers —
the comment at `:510` says so, "64bit isn't supported in ES profile". The C API
copy at `c_api/src/taichi_opengl_impl.cpp:8-12` sets `spirv_has_int64` and
`spirv_has_float64` with no such guard. **Revision 4 said this mis-declaration
"cannot fire today". That is withdrawn: it fires today, through a documented
exported function. See 4.2a.** GLES is the profile the edge devices in plan
section 2 would actually present. The lost int16/float16 detection (`:515-526`)
is live now and cuts the other way — real capability present and not declared.
Both directions are the same defect: the C API builds a capability set by
assertion where the RHI builds one by detection. Escalation 20. **[V]** for the
two code paths.

### 4.2a GLES is reachable through the C API, and the mis-declaration is live. New in revision 5.

`adversary3-04-2.md` §5 found this; `adversary3-04-1.md` did not have it. It
falsifies a caveat carried by this report, by `report-04b-backend-build.md`, by
`adversary2-04-1.md` §9.3.3 and by `adversary2-04-2.md:455-457`. **I verified
every step myself before acting on it.** **[V]** for the whole chain except where
marked.

`ti_import_opengl_runtime` is a public, `TI_DLL_EXPORT` function taking a
`use_gles` parameter. Declared at `c_api/include/taichi/taichi_opengl.h:33-36`:
```
// Function `ti_import_opengl_runtime`
TI_DLL_EXPORT TiRuntime TI_API_CALL
ti_import_opengl_runtime(TiOpenglRuntimeInteropInfo *interop_info,
                         bool use_gles);
```

| Step | Location |
|---|---|
| Public entry with `use_gles` | `c_api/include/taichi/taichi_opengl.h:33-36`; body `c_api/src/taichi_opengl_impl.cpp:21-29` |
| Override set from the argument | `taichi_opengl_impl.cpp:26`, `set_gles_override(use_gles)` → `taichi/rhi/opengl/opengl_api.cpp:35-37`, writing the anonymous-namespace static `use_gles_override` at `:32` |
| Runtime created | `taichi_opengl_impl.cpp:28`, `ti_create_runtime(TI_ARCH_OPENGL, 0)` → `new OpenglRuntime` at `c_api/src/taichi_core_impl.cpp:271` |
| Its `GLDevice` member constructed first | `GLDevice::GLDevice`, `taichi/rhi/opengl/opengl_device.cpp:506`, calling `initialize_opengl(false, true)` at `:507` |
| **The override replaces that `false`** | `opengl_api.cpp:56-59`, `if (use_gles_override.has_value()) { use_gles = use_gles_override.value(); unset_gles_override(); }` |
| `kUseGles` set true | `opengl_api.cpp:239`, `kUseGles = use_gles;`, read back by `is_gles()` at `:248-250` |
| Detection correctly withholds 64-bit | `opengl_device.cpp:509-513`, `if (!is_gles())`, comment `:510` "64bit isn't supported in ES profile"; `set_caps` at `:529` |
| The C API asserts it anyway | `taichi_opengl_impl.cpp:8-12`, a fresh three-entry config installed wholesale: `spirv_has_int64` `:9`, `spirv_has_float64` `:10`, `spirv_version` `:11`, `set_caps` `:12` |

So a host calling `ti_import_opengl_runtime(info, true)` gets a runtime whose
device was detected as GLES, correctly refused 64-bit integer and float support,
and then had both asserted over the top, with the detected int16/float16
(`opengl_device.cpp:515-526`) dropped in the same replacement. **The
mis-declaration is live, not latent, and it does not depend on an accident of
ordering.** **[V]** for every row; **[I]** for the composed outcome, which I read
rather than ran.

**The converse case, same override, opposite failure.** The override is consumed
at `opengl_api.cpp:56-59`, which sits **after** the early return at `:46-54`. If
`initialize_opengl` has already run in the process, `supported` has a value
(`:27`), the function returns at `:48` or `:52`, and `use_gles_override` is
neither read nor cleared. `ti_get_available_archs`
(`c_api/src/taichi_core_impl.cpp:171-205`) probes OpenGL at `:193` through
`is_opengl_available` (`:21-27`), which calls `is_opengl_api_available()` with
its default argument `use_gles = false` (`taichi/rhi/opengl/opengl_api.h:13`),
forwarded straight to `initialize_opengl(use_gles, true)` at
`opengl_api.cpp:244-246`. **So a host that enumerates archs before importing a
runtime — the natural install-time-loader sequence under plan section 5 —
locks `kUseGles = false`, and its later `ti_import_opengl_runtime(info, true)`
silently loses the GLES request and runs desktop GL.** `reset_opengl`
(`opengl_api.cpp:252-256`) clears both `supported` and `kUseGles`, and its only
caller is `taichi/runtime/program_impls/opengl/opengl_program.cpp:29`. **[V]**
for each line; **[I]** for the composed outcome.

**Why this is weighted, not filed as a curiosity.** Plan sections 2 and 5.3 put
the low-end profile first and make modern capability an opportunity rather than a
prerequisite. GLES is the profile the low end presents. The RHI detection at
`opengl_device.cpp:509-513` is exactly the protection those sections ask for and
its own comment says so, and the C boundary discards it on a path a host reaches
by calling a documented function with a documented argument. It also engages plan
section 2.2 in the harder direction: the RHI declines to claim a capability the
hardware lacks and the C API then claims it, which is hardware being told it can
do something it cannot.

**`spirv_has_atomic_int64` is declared and never used.** Declared at
`rhi_constants.inc.h:17`. Three hits in the tree:
that declaration, `c_api/include/taichi/cpp/taichi.hpp:1121` (the header-only C++
wrapper) and `python/taichi/lang/enums.py:27`. **No `set` and no `get` anywhere.**
A target with 64-bit integers but not 64-bit atomics can be named, not acted on. **[V]**

### 4.3 The 64-bit machinery that is written and switched off

`spirv_has_physical_storage_buffer` is the buffer-device-address capability — 64-bit
pointers inside shaders. This is the most 6.2-relevant fact in my territory and my
first pass cited only its declaration line. **[V]** throughout.

**Two producers, both compiled out:**
- `taichi/rhi/vulkan/vulkan_device_creator.cpp:826`, inside
  `#if !defined(__APPLE__) && false` opened `:825`, closed `:827`. Comment `:824`:
  "(penguinliong) Temporarily disabled (until device capability is ready)";
  `:822-823` reference taichi issue 6295. Note `:821` guards it on
  `device_supported_features.shaderInt64` — the intended design coupled shader
  64-bit pointers to hardware 64-bit integers.
- `c_api/src/taichi_vulkan_impl.cpp:48`, inside a `/* */` block spanning `:46-51`,
  preceded at `:45` by "(penguinliong) Will bring it back after devcap."

**Twelve live consumers**, all taking the false branch on every in-tree path:
`taichi/rhi/vulkan/vulkan_device.cpp:1772, 1792, 2147, 2509`;
`taichi/codegen/spirv/spirv_ir_builder.cpp:73, 113`;
`taichi/codegen/spirv/spirv_codegen.cpp:783, 2340, 2416, 2491`;
`taichi/runtime/gfx/runtime.cpp:96`;
`taichi/runtime/program_impls/gfx/gfx_program.h:81`.

The sharpest is `gfx_program.h:79-83`:
```
  std::string get_kernel_argument_data_layout() override {
    auto has_buffer_ptr = runtime_->get_ti_device()->get_caps().get(
        DeviceCapability::spirv_has_physical_storage_buffer);
    return "1" + std::string(has_buffer_ptr ? "b" : "-");
  };
```
which always returns `"1-"`.

**The false branch is not permanent, and revision 2 called it that.** Adversary
04-1 is right and I have verified it. `ti_set_runtime_capabilities_ext`
(`c_api/src/taichi_core_impl.cpp:317-334`) builds a `DeviceCapabilityConfig`
from caller-supplied `TiCapabilityLevelInfo` values at `:326-330` and installs
it wholesale at `:331`, `runtime2->get().set_caps(std::move(devcaps))`. That
lands on `Device::caps_` (`taichi/rhi/public_device.h:617`, written through
`set_caps` at `:855`, read through `get_caps` at `:852`) — the same field every
one of the twelve consumers reads, and the same field
`GfxProgramImpl::get_device_caps` returns at
`taichi/runtime/program_impls/gfx/gfx_program.cpp:82-85`. The header-only C++
wrapper even ships a typed setter for this exact capability at
`c_api/include/taichi/cpp/taichi.hpp:1156`. **[V]**

The accurate statement is therefore: **no in-tree producer sets it**, both
producers are compiled out, and the only way it is set today is a host calling
the C API override. **[V]** for the write path; **[I]** that a host calling it
before kernel compilation puts all twelve consumers on the true branch, which is
composition rather than something I ran. This report already lists that function
as `set_caps` site 8 in the 4.2 table and raises it as escalation 11; revision 2
did not reconcile the two. It bears directly on escalation 13, which now has a
third answer.

**So on the SPIR-V spine a substantial part of the 64-bit addressing machinery is
already written and compiled, and is switched off at the capability layer rather
than absent.** That is a materially different starting position for 6.2 than
"64-bit is unsupported there". **[I]** — this is a judgement composed from the
census above, not a reading; the census itself is **[V]**.

**Weighting. Both earlier weightings are withdrawn — revision 5, against the
plan dated 2026-09-09.**

Revisions 2 and 3 treated this machinery as a side axis, on the ground that the
section 5.2 cards are not on this spine. Revision 4 replaced that with the
opposite ranking: it called `{opengl, gles, vulkan, dx11, metal}` the
**reference** path, called the LLVM spine the **accelerant**, and concluded that
written-and-disabled machinery there is the **primary** existing form of item
6.2 — attributing all of it to plan section 5.2.

**Both are withdrawn, and the second is the more serious because it cited the
plan.** The plan as it stands contains neither "reference path" nor
"accelerant". Section 2.2a states the axis precisely and the other way: a vendor
backend is CUDA, AMDGPU, DirectX "or anything else tied to one supplier or one
operating system", while "the two COMPILATION SPINES are not vendors: LLVM
carries CPU-only operation, and SPIR-V carries the portable GPU path", so "work
on either spine is BASE work". Section 5.2 adds that no backend may be privileged
and that where one spine is better served the shortfall is "a gap in the
less-served spine to record, never evidence that it matters less". Section 2.2a
puts "Item 6.2, 64-bit addressing, **in full**" in BASE scope, naming the shelved
SPIR-V path explicitly. **The plan therefore supplies no ranking between the two
spines' halves of item 6.2, and I do not supply one.** The superseded wording
traces to an intermediate section 5.2 quoted in my own notes at
`notes-04-backend-build.md:1946-1949`.

**What the source supports without a ranking, and it is sufficient on its own.**
Item 6.2 exists in two different **states** on the two spines. The difference is
in what is already written, not in what matters more. **[V]** for both censuses,
which are unchanged; **[I]** for the two-state reading.

- On `arch_uses_spirv` (`taichi/rhi/arch.cpp:63-66`) the 64-bit pointer
  machinery is written, compiled, and switched off at the capability layer:
  twelve live consumers and two disabled producers, all enumerated above. Work
  there is finishing and enabling what exists.
- On `arch_uses_llvm` (`:54-57`) there is no capability representation at all to
  switch anything on with (4.2, 4.7). Work there starts from nothing. That spine
  carries `Arch::x64` and `Arch::arm64`, which is CPU-only operation and a
  first-class target under plan section 2.

Both are BASE work under plan section 2.2a. The difference in state is recorded
as a gap on the LLVM spine, per section 5.2, and as nothing else.

**What survives the withdrawal.** The consequence for escalation 13 does not
depend on the withdrawn premise and is now derived from the plan text that does
exist. Revision 2 phrased escalation 13 as "is this in scope", presuming it might
not be. Scope is settled, but by plan section 2.2a putting item 6.2 in full in
BASE scope, not by which spine the machinery sits on. The open question is what
to do with machinery that is already written.

**Two limits on that, and neither may be dropped.** **[V]** for the first from
the sites this report already cites; **[I]** for the second, which is a reading
of plan sections 2.2a and 2.3 rather than of source.

1. **It does not reach SNode addressing.** A separate investigation reports that
   this machinery gates ndarray, argpack and return-struct addressing only, and
   the consumer sites cited above bear that out where I can check them in my own
   territory: `spirv_codegen.cpp:783` sits in
   `visit(ExternalPtrStmt *)` (opens `:734`), the external-tensor and ndarray
   pointer; `:2340` sits in `compile_args_struct()` (opens `:2326`); `:2416` sits
   in `compile_argpack_struct()` (opens `:2401`); `:2491` sits in
   `compile_ret_struct()` (opens `:2483`); and `taichi/runtime/gfx/runtime.cpp:96`
   substitutes a device address for ndarray and unmarked external arrays. None of
   the five is on the SNode or root-buffer path. I have not audited the SNode path
   myself — that is territory 02 and 03 — so I record the corroboration and not a
   verdict. Plan section 2.2a states the same division from the other side: the
   shelved work "covers 64-bit addressing for everything EXCEPT SNodes, so it is
   the other half of the same limit", and "the two halves are not separable". My
   five sites are consistent with that and do not test it.
2. **Neither state is evidence about priority.** Plan section 2.3 states that
   item 6.1 is the project and that the shelved 64-bit work is stub material
   under section 5.1 rather than a path to the ceiling, while section 2.2a puts
   item 6.2 in full in BASE scope. Both hold without tension: this machinery is
   in scope and it is not the leading edge of the project. The overstatement to
   avoid — and the one revision 4 made — is reading "already written" as
   "primary".

### 4.4 Are capability levels ordered? Adjudicated.

The two adversaries disagreed here, so I went to source. **[V]** for both facts.

**Fact 1 — the published contract declines to guarantee ordering.**
`c_api/include/taichi/taichi_core.h:409-412`:
```
// Structure `TiCapabilityLevelInfo` (1.4.0)
//
// An integral device capability level. It currently is not guaranteed that a
// higher level value is compatible with a lower level value.
```

**Fact 2 — the engine already assumes ordering for `spirv_version`.**
**Thirteen** ordered comparisons, not nine. Revision 2 said nine and marked it
**[V]** off a claimed grep; both round-two adversaries found the same four
missing sites independently, and I have re-run it. **[V]**
- `<` — `taichi/codegen/spirv/spirv_ir_builder.cpp:556, 625, 671, 679, 741, 777`
  (all against `0x10300`) and `taichi/rhi/vulkan/vulkan_device_creator.cpp:577`
  (`< 0x10400`).
- `>` — `taichi/codegen/spirv/spirv_codegen.cpp:1908` (`> 0x10300`).
- `>=` — `taichi/codegen/spirv/spirv_ir_builder.h:424` (`>= 0x10400`), and the
  four I missed, `taichi/codegen/spirv/spirv_codegen.cpp:2669, 2671, 2673, 2675`.

Several of these decide which SPIR-V constructs to emit.

**The four I missed are the strongest instance in the tree, not incidental
ones.** `spirv_codegen.cpp:2666-2679`, in `KernelCodegen::KernelCodegen`, reads
`spirv_version` once into a local at `:2666` and walks a graded ladder —
`>= 0x10600` → `SPV_ENV_VULKAN_1_3` (:2669-2670), `>= 0x10500` → `_1_2`
(:2671-2672), `>= 0x10400` → `_1_1_SPIRV_1_4` (:2673-2674), `>= 0x10300` → `_1_1`
(:2675-2676), else `_1_0` (:2677-2678) — selecting the `spv_target_env` handed to
the optimizer at `:2681` for the **whole module**. That is not a construct-level
branch; it is the value being treated as a graded scale end to end. **[V]**

The direction of the error favours my own conclusion, which is why nothing below
inverts. It was still a **[V]** census inside the section written to adjudicate a
dispute, which is the shape revision 2 was sent back for.

**My adjudication.** "Not guaranteed" is weaker than "not ordered": the
documentation leaves the relation undefined rather than fixing it at `!=`. So the
`!=` at `c_api/src/taichi_gfx_impl.cpp:25` is neither a plain bug nor forced by
the documentation. It is a contract decision — and the source shows the contract
is already contradicted internally for the one capability that is genuinely a
version number, while the rest are booleans stored as levels for which an ordered
relation is meaningless. A single relation cannot serve both kinds. Escalation 3.

### 4.5 What the CUDA path detects about the hardware it is running on

(Titled "The section 5.2 target tiers" in revision 2. Retitled in revision 4:
this section describes one backend's detection, and reading it as "the tier
section" made a portable question look like a CUDA one. What the other backends
detect is 4.8; the two must be read together.)

CUDA compute capability is detected at `taichi/rhi/cuda/cuda_context.cpp:29-33`
via `CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR`/`_MINOR`, combined at `:73`,
**clamped at `:75-77`**:
```
  if (compute_capability_ > 86) {
    compute_capability_ = 86;
  }
```
and formatted at `:83` as `mcpu_ = fmt::format("sm_{}", compute_capability_)`. **[V]**

Mapping to the brief's tiers: GTX 750 (Maxwell) is sm_50, GTX 1070 (Pascal)
sm_61, RTX 3060 (Ampere) sm_86, so on this backend the fidelity classes are
observable and the clamp sits exactly on the Upper-tier boundary. **[I]** for the
card-to-number mapping, which is external knowledge; **[V]** for the detection,
the clamp and the format.

Two things this does **not** establish, and revision 2 let the second be read
into it. It does not make CUDA the path the tiers live on — the tiers are
fidelity classes and section 4.8 shows the SPIR-V backends detect their own
equivalents. And under plan section 2.2 a detected class must not decide what the
system *can* do, only how fast and how finely it does it; `compute_capability_`
is fed to `mcpu_` and to an LLVM constant (see the consumer list below), which is
code generation quality, not feature admission. That is the correct shape and it
is worth recording as such.

Also detected: `CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_BLOCK_OPTIN` into
`max_shared_memory_bytes_` (`cuda_context.cpp:79-81`); total and free device
memory (`get_total_memory` `:88-92`, `get_free_memory` `:94-98`), so the Baseline
tier's 2 GB sizing is queryable. **[V]**

Consumers of `get_compute_capability()` outside `cuda_context` itself:
`taichi/codegen/cuda/codegen_cuda.cpp:408`,
`taichi/runtime/llvm/llvm_context.cpp:424` (baked in as an LLVM constant),
`taichi/runtime/llvm/llvm_context.cpp:504`, `taichi/python/export_lang.cpp:1230`.
AMDGPU has a parallel accessor at `taichi/rhi/amdgpu/amdgpu_context.h:85`. **[V]**

**All of this is runtime JIT-time detection, and none of it reaches
`DeviceCapability`, `Extension`, the AOT `required_caps` or the C API.** **[V]**

### 4.6 The capability enum is a positional key in two places

`c_api/include/taichi/*.h` are generated from `c_api/taichi.json` by
`misc/generate_c_api.py` via `misc/taichi_json.py` (`:401` opens the json), then
checked in. The capability enum is declared
`{"name":"capability","type":"enumeration","since":"v1.4.0","inc_cases":"PER_DEVICE_CAPABILITY"}`.
`misc/taichi_json.py:182-183` resolves that through `load_inc_enums()` (:102-116),
which globs `taichi/inc/*.inc.h` (:103), regexes `MACRO(NAME)` (:108) and assigns
**values by position**: `cases[key][case_name] = len(cases[key])` (:115). **[V]**

Counted: 25 `PER_DEVICE_CAPABILITY` entries; `taichi_core.h:380-407` has 25 real
`TI_CAPABILITY_*` values (0-24) plus `TI_CAPABILITY_MAX_ENUM`.
`spirv_has_int64` is fifth (`rhi_constants.inc.h:14`) and
`TI_CAPABILITY_SPIRV_HAS_INT64 = 4` (`taichi_core.h:385`). **[V]**

**There is a second consumer of those numbers.**
`taichi/analysis/offline_cache_util.cpp:88-95`,
`get_offline_cache_key_of_device_caps`, serialises `caps.devcaps` directly at
`:92` — the raw `std::map<DeviceCapability, uint32_t>`, keyed by the numeric enum
— and the result is hashed into the kernel cache key at `:185` and `:190`. **[V]**

So inserting a capability anywhere but the end of `rhi_constants.inc.h` renumbers
the public C ABI **and** silently changes every offline-cache key. **[V]** for
both mechanisms; **[I]** that the C ABI mismatch would not surface until the
checked-in headers are regenerated.

### 4.7 6.2 divides in two along `arch_uses_spirv`, and the halves are shaped differently

Both round-two adversaries observe that neither report states this in one place.
The two component facts are each already in this report; the consequence is not,
and it is a gap in coverage. **[V]** for the components; **[I]** for the split.

`arch_uses_spirv` is `{opengl, gles, vulkan, dx11, metal}`
(`taichi/rhi/arch.cpp:63-66`); `arch_uses_llvm` is `{x64, arm64, cuda, dx12,
amdgpu}` (`:54-57`). **[V]**

- **On the SPIR-V spine**, `{opengl, gles, vulkan, dx11, metal}` —
  `DeviceCapability` is populated from real hardware queries at five RHI sites
  (4.2), and a substantial part of the 64-bit addressing machinery exists and is
  switched off at the capability layer (4.3). Item 6.2 there is finishing and
  enabling what is written, bounded to ndarray, argpack and return-struct
  addressing. **[V]** (Revision 5: this bullet previously opened "the reference
  path under plan section 5.2" and asserted this half was "the primary existing
  form of 6.2". Both are withdrawn — the plan contains no such phrase and
  supplies no such ranking. See 4.3 and 9.8. The bullet's facts are unchanged.)
- **On the LLVM spine**, no RHI device calls `set_caps` at all — verified again
  in this pass, no hit under `taichi/rhi/cuda/`, `taichi/rhi/cpu/`,
  `taichi/rhi/llvm/`, `taichi/rhi/amdgpu/` or `taichi/rhi/dx12/` (4.2) — and
  `ProgramImpl::get_device_caps` returns `{}` for everything but gfx
  (`taichi/program/program_impl.h:170-172`). There is no capability vocabulary
  to switch anything on with. Item 6.2 there starts from nothing. **[V]**

Recording this as a **gap in coverage on the LLVM spine**, which is what plan
section 5.2 asks for: "where one spine is better served than the other, that is a
gap in the less-served spine to record, never evidence that it matters less."
Stating what it is not. It is not a reason to weight 6.2 toward either spine.
Neither spine is the vendor spine — plan section 2.2a, which defines a vendor
backend as one "tied to one supplier or one operating system" and states that
work on either spine is BASE work. The LLVM spine carries `Arch::x64` and
`Arch::arm64`, which is CPU-only operation, a first-class target under plan
section 2, and it carries `amdgpu` and `dx12` alongside `cuda`. So the missing
capability system on that spine is a hole under CPU-only operation just as much
as under CUDA. The
hardware detection that does exist on the CUDA path is JIT-time only and reaches
none of the capability systems (4.5) — and per 4.8 the SPIR-V backends discard
theirs too, so this is not an asymmetry in what is *detected*, only in what each
spine does with it. Escalation 7.

What follows for planning is only that sizing 6.2 as one item conflates two
different amounts of work. Which spine is worked, and in what order, is not
recorded in the plan. Escalation 17.

### 4.8 Both spines detect hardware class. Neither publishes it. New in revision 4.

This is a new finding and it corrects the shape of escalation 7. The reading
carried through revisions 1 to 3 was that the CUDA path detects tier data (4.5)
and the SPIR-V spine does not. That is false. **The SPIR-V backends query their
own hardware-class data and then throw it away**, which means the gap is not
"the SPIR-V spine lacks tier information". It is that **nothing publishes it, on
either spine**. Verified site by site in this pass. **[V]**

**Vulkan — memory properties queried, heap sizes never read.**
`taichi/rhi/vulkan/vulkan_device.cpp:2515-2516` calls
`vkGetPhysicalDeviceMemoryProperties` into a local. The only consumption is the
vector built at `:2518-2519` and the loop at `:2521-2532`, which reads `memoryTypes[i].propertyFlags` for
`VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT` and writes a
`VkExternalMemoryHandleTypeFlags` entry per type, then hands the array to VMA at
`:2534`. `memoryHeaps` and `heapSize` appear **nowhere** in `taichi/rhi/vulkan/`
— grep returns zero hits — so the device memory budget is asked for and
discarded. That is the Baseline tier's 2 GB sizing question, unanswered on the
SPIR-V spine while `cuda_context.cpp:88-98` answers it on the LLVM spine.
**[V]**

**Vulkan — full device properties stored, two fields used.**
`VkPhysicalDeviceProperties vk_device_properties_` is a member
(`vulkan_device.h:739`), filled at `vulkan_device.cpp:1596`, exposed by
`get_vk_physical_device_props()` (`vulkan_device.h:710-712`). Grep over
`taichi/` and `c_api/` returns exactly two reads:
`vulkan_device.cpp:1125-1128`, `limits.maxComputeWorkGroupCount[0..2]`, bounding
`dispatch` and returning `RhiResult::not_supported` past it; and `:1945`,
`limits.timestampPeriod`, converting a profiler duration. Everything else in the
struct — device type, vendor and device id, and the whole rest of `limits` — is
carried and never read. **[V]**

**Vulkan — the device name reaches only a debug log.**
`vulkan_device_creator.cpp:432-440` enumerates every physical device and logs
`properties.deviceName` through `RHI_LOG_DEBUG`; `:504-518` re-queries and logs
name and API version. One field of that second query is used for real:
`apiVersion` at `:523` feeds the `spirv_version` ladder at `:525-533`. So the
Vulkan creator does publish one thing into the capability map, and it is an API
version, not a hardware class. **[V]**

**OpenGL — workgroup limits queried into two globals that nothing reads.**
`taichi/rhi/opengl/opengl_api.cpp:20-21` declares
`int opengl_max_block_dim = 1024;` and `int opengl_max_grid_dim = 1024;`, with
the comment at `:18-19`: "will later be initialized in initialize_opengl, here we
use the minimum value according to OpenGL spec in case glGetIntegerv didn't work
properly". `initialize_opengl` fills them at `:231` and `:234` from
`GL_MAX_COMPUTE_WORK_GROUP_COUNT` and `GL_MAX_COMPUTE_WORK_GROUP_SIZE`. A grep
over `taichi/` and `c_api/` for both names returns **only** those two
declarations, the two `glGetIntegeri_v` calls and the two `TI_TRACE` lines at
`:233` and `:236`. Nothing else in the tree reads either global. They are
write-only, and the hardcoded 1024 fallback is therefore also never consulted.
**[V]** — this is stronger than "detected and not published": on OpenGL the
value is detected and not used at all.

**Vulkan — `get_device_score` makes an explicit hardware-class judgement and
throws the number away. Added in revision 5.** Both round-three adversaries found
this missing from this census, and they are right: it is the strongest instance
of the finding this section exists to make, and it was absent from every revision
of this report. `get_device_score` (`taichi/rhi/vulkan/vulkan_device_creator.cpp:203-229`)
queries `vkGetPhysicalDeviceFeatures` at `:206` and
`vkGetPhysicalDeviceProperties` at `:208`, then composes a `size_t score`:
queue-family completeness × 1000 (`:212-217`), `features.wideLines` × 100
(`:219`), `VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU` × 500 (`:220-222`),
`VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU` × 1000 (`:223-225`), and
`VK_API_VERSION_MINOR(properties.apiVersion)` × 100 (`:226`). Returned at `:228`.
**A discrete-versus-integrated classification is exactly a hardware class**, and
it is graded rather than boolean. **[V]**

Two call sites, and they use it differently. **[V]**

- `:462`, inside the automatic pick at `:458-468`: the score ranks devices and the
  number dies at `:465`.
- `:452`, `else if (get_device_score(devices[id], test_surface))`: the score is a
  **boolean admission gate** on a device the caller explicitly named through
  `device_index` or `TI_VISIBLE_DEVICE` (3.3a). A named device scoring zero is
  dropped and the automatic pick runs instead, with no log on that branch — the
  only error log, at `:448-451`, covers the out-of-range case only.

So on this one site the judgement does not die: what dies is the number, never
the decision. That is a qualification on this section's own thesis and it is
recorded rather than smoothed over. Escalation 22.

One detail recorded because it bears on plan section 1.3, which says not to judge
a backend by its graphics support: `:219` adds 100 for `features.wideLines`, a
rasterisation feature, when scoring devices for a compute workload. Recording,
not proposing. **[V]**

**What follows.** None of the above reaches `DeviceCapability`, `Extension`, the
AOT `required_caps` or the C API — the same sentence 4.5 ends with for CUDA. So
the correct statement of the gap is: **every backend that can ask its hardware
about class asks, and no backend tells anything downstream what it learned.**
There is no publication channel, on either spine. **[V]** for each census;
**[I]** for the composed statement. **One qualification, added in revision 5:**
`get_device_score` at `vulkan_device_creator.cpp:452` does act on its judgement,
by admitting or refusing a named device. It publishes nothing downstream, so the
sentence stands as written about publication, and it is not true that every such
judgement is inert.

Per plan section 5.2 this is recorded as a gap **in the SPIR-V spine, on the one
axis where that spine is the less-served one** — the memory budget, which CUDA
reads (`cuda_context.cpp:88-98`) and Vulkan discards
(`vulkan_device.cpp:2515-2532`) — and equally as a gap that is *not* asymmetric
overall, since the publication channel is missing on both spines. It is not
evidence for preferring either spine, and section 5.2's rule is that a gap is
recorded as a gap and "never evidence that it matters less". Escalation 18.

### 4.9 Metal already expresses hardware classes in the portable vocabulary

The one backend in the tree that does what escalations 7 and 18 are asking for,
and it is on the SPIR-V spine. **[V]**

`collect_metal_device_caps` (`taichi/rhi/metal/metal_device.mm:1017-1069`) reads
a graded hardware family ladder at `:1027-1036` —
`[mtl_device supportsFamily:]` against `MTLGPUFamilyMac2` and
`kMTLGPUFamilyApple3` through `Apple7`, each higher family OR-ed down into the
lower ones so the ladder is monotone — reduces it to five named features at
`:1038-1042` (`feature_64_bit_integer_math`, `feature_floating_point_atomics`,
`feature_quad_scoped_permute_operations`,
`feature_simd_scoped_permute_operations`,
`feature_simd_scoped_reduction_operations`), and writes those into a
`DeviceCapabilityConfig` at `:1044-1068`, returned at `:1069`, installed by `MetalDevice::MetalDevice`
at `:1168`. It is then consumed portably: `metal_device.mm:131-135` reads
`caps.contains(DeviceCapability::spirv_has_int64)` back out to select MSL 2.3.

So the three-stage shape — **detect a hardware class → reduce it to named
features → publish into the portable capability vocabulary → read it back
downstream** — already exists, end to end, on one backend. It is the model
escalations 7 and 18 were reaching for, and it did not have to be invented.

**One caveat, and it is the whole of plan section 2.2.** What Metal's ladder
currently publishes is *feature admission*: below Apple3 there is no
`spirv_has_int64`, so that hardware cannot do a thing other hardware can. Plan
section 2.2 says hardware must change speed and granularity, never what the
system can do, and calls a change that lets some hardware do what other hardware
cannot "a violation, not an optimisation". So the **mechanism** is the model; the
**content** it currently carries is the pattern section 2.2 warns about. Whether
a fidelity class can be expressed in a vocabulary whose entries are all currently
feature bits is escalation 19. I am recording the distinction, not resolving it.

---

## 5. What the ahead-of-time module path can and cannot do

`taichi/aot/` is **seven** files, 836 lines: `graph_data.cpp` 146,
`graph_data.h` 183, `module_builder.cpp` 68, `module_builder.h` 81,
`module_data.h` 139, `module_loader.cpp` 94, `module_loader.h` 125. **[V]**

### 5.1 Can

**Declare a required capability set — on the gfx path only.**
`gfx::TaichiAotData::required_caps` (`taichi/runtime/gfx/aot_utils.h:20`,
serialised `:23`) is populated at build time from the builder's
`DeviceCapabilityConfig` (`taichi/runtime/gfx/aot_module_builder_impl.cpp:22-24`):
```
  for (const auto &pair : caps.to_inner()) {
    ti_aot_data_.required_caps[to_string(pair.first)] = pair.second;
  }
```
**[V]**

There are **two** such structs, not one, and only this one is live. The generic
`aot::ModuleData::required_caps` (`taichi/aot/module_data.h:125`, serialised
`:135`) is **never written by anybody**. Its only subclass in the tree is
`ModuleDataDX12 : public aot::ModuleData`
(`taichi/runtime/dx12/aot_module_builder_impl.h:11`, instantiated at
`taichi/runtime/dx12/aot_module_loader_impl.cpp:118`), and the DX12 builder
populates no caps. **[V]** (Revision 2: my first pass described these as one
generic channel.)

**Be told to target a capability set the build machine does not have.**
`Program::make_aot_module_builder(Arch, const std::vector<std::string> &caps)`
(`taichi/program/program.cpp:541-557`) takes capability *strings* through
`translate_devcaps()` (`:514-539`), which parses `name` or `name=value` (`:517-531`,
`spirv_version=1.3` per the comment at `:515-517`) and defaults `spirv_version`
to `0x10300` when absent (`:534-537`). Validation is only `str2devcap` throwing on
an unknown name (`device_capability.cpp:12`). **[V]**

This is build-time parameterisation of the AOT target, and it already exists.
`str2devcap` (`device_capability.cpp:6-13`) is the exact inverse of the
`to_string` used by the producer at `aot_module_builder_impl.cpp:23`. **[V]**

**Ship a directory of SPIR-V blobs plus JSON.**
`taichi/runtime/gfx/aot_module_loader_impl.cpp` reads `metadata.json` (:34-46),
one `<taskname>.spv` per task with the magic checked against `0x07230203`
(:48-69), then `graphs.json` (:71-84), all through `io::VirtualDir`. **[V]**

### 5.2 Cannot

**Load a module for any LLVM arch through `aot::Module::load`.**
`taichi/aot/module_loader.cpp:31-58`: vulkan :32-35, opengl :36-39, gles :40-43,
dx11 :44-47, metal :52-55 all to `gfx::make_aot_module`; dx12 :48-51 to
`directx12::make_aot_module`; falling through to `TI_NOT_IMPLEMENTED` at `:57`.
**`Arch::x64`, `Arch::arm64`, `Arch::cuda` and `Arch::amdgpu` are absent.** **[V]**

When the arch matches but the `#ifdef` is false, the `return` is preprocessed
away and control reaches `TI_NOT_IMPLEMENTED`, so "built without this backend"
and "this backend has no AOT support" produce the same error. That is
well-defined: `TI_NOT_IMPLEMENTED` is `TI_ERROR("Not supported.")`
(`taichi/common/logging.h:108`) and `TI_ERROR` ends in `TI_UNREACHABLE`
(`:40-44`). **[V]**

The LLVM archs use a separate path: `c_api/src/taichi_llvm_impl.cpp:78-121`
calls `taichi::lang::LLVM::make_aot_module` directly, with two arms, cpu (:82-90)
and cuda (:91-104, guarded `TI_WITH_CUDA`, `TI_ASSERT(config.arch == Arch::cuda)`
at `:93`, `TI_NOT_IMPLEMENTED` at `:103`). AMDGPU and DX12 have no LLVM AOT load
path. **No capability check of any kind on this path.** **[V]**

`LlvmAotModule` (`taichi/runtime/llvm/llvm_aot_module_loader.h:18-92`) reuses the
offline-cache file format (`:25`) plus `graphs.tcb` (`:28-29`), hardcodes
`version()` to 0 (`:36-38`) and `get_root_size()` to 0 (`:40-42`), leaves
`make_new_kernel_template` as `TI_NOT_IMPLEMENTED` (`:74-77`), and does not
override `get_required_caps()`. **[V]**

**Perform any handshake at load. All three are absent.** **[V]**

*Capability.* `c_api/src/taichi_gfx_impl.cpp:18-29` is the only capability gate
anywhere:
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
`get_required_caps()` is **never overridden**. Grep across the repo excluding
`external/` returns exactly two hits: the default body at
`taichi/aot/module_loader.h:97-100` (a function-static empty config) and this
call site. `AotModuleImpl` does not override it and
`aot_module_loader_impl.h` has no caps member. The loop always iterates an empty
map and always passes.

*Version.* `aot::Module::version()` is pure virtual at `module_loader.h:85`.
Three overrides: `taichi/runtime/gfx/aot_module_loader_impl.cpp:114-116` is
`TI_NOT_IMPLEMENTED`, i.e. it aborts; `taichi/runtime/dx12/aot_module_loader_impl.cpp:67-69`
likewise; `taichi/runtime/llvm/llvm_aot_module_loader.h:36-38` returns 0.
**No caller anywhere.**

*Arch.* `aot::Module::arch()` is pure virtual at `module_loader.h:84`. The gfx
override (`aot_module_loader_impl.cpp:111-113`) and the dx12 override
(`dx12/aot_module_loader_impl.cpp:64-66`) both return `device_api_backend_`,
which is the arch handed in **from the runtime** at
`taichi/aot/module_loader.cpp:31`, not read from the module. **No caller
anywhere.** And `metadata.json` records no arch: the serialised list is
`TI_IO_DEF(kernels, fields, required_caps, root_buffer_size)`
(`aot_utils.h:23`), mirrored at `module_data.h:135`.

**Consequence: a module built for Vulkan can be handed to an OpenGL runtime and
nothing will object.** The accurate frame for 6.3 is not "one dead check to wire
up" but "no handshake exists; the capability slot is the only one wired even
half-way". **[V]** for each absence; **[I]** for the composed consequence.

The pattern exists one layer down and was not lifted to the module:
`taichi/compilation_manager/kernel_compilation_manager.cpp:225`
`TI_ASSERT(loaded->arch() == arch)`, plus
`taichi/runtime/cuda/kernel_launcher.cpp:191` and
`taichi/runtime/amdgpu/kernel_launcher.cpp:154`. **[V]**

**Detect that the SNode ceiling changed.** `taichi/util/offline_cache.h:95-100`
rejects a cache only when
`ver[0] != TI_VERSION_MAJOR || ver[1] != TI_VERSION_MINOR || ver[2] != TI_VERSION_PATCH`.
`LlvmAotModule` reuses that reader (`llvm_aot_module_loader.h:25`). A rebuild that
changes `taichi_max_num_snodes` does not move `TI_VERSION`, so kernels compiled
against the old struct layout will be loaded and run against the new one, from
both the offline cache and an LLVM AOT module. **[V]** for the validation code and
the reuse; **[I]** for the consequence.

Note the asymmetry with 4.6: the capability map **is** hashed into the kernel key
(`offline_cache_util.cpp:92, 185, 190`); the SNode ceiling is hashed into nothing.

### 5.3 The adaptive-module patterns that already work

**Keyed on arch.** `taichi/runtime/llvm/llvm_context.cpp:209-211`:
```
std::string get_runtime_fn(Arch arch) {
  return fmt::format("runtime_{}.bc", arch_name(arch));
}
```
Loaded from `runtime_lib_dir()` (`taichi/util/lang_util.cpp:30-47`), which is
`compiled_lib_dir` if set, else the `TI_LIB_DIR` environment variable (`:35-45`).
Produced one per arch by `runtime_module/CMakeLists.txt:29-31`, installed by
`:13` and `cmake/TaichiCAPI.cmake:179-184`. **[V]**

**Keyed on detected hardware.** `taichi/runtime/llvm/llvm_context.cpp:625-655`
composes AMDGPU device-library filenames from the ISA version of the actual
device: `:629` `auto isa_version = AMDGPUContext::get_instance().get_mcpu().substr(3, 4);`
and `:637` `"oclc_isa_version_" + isa_version + ".bc"`, loaded from
`runtime_lib_dir()` at `:642-643`. **[V]**

That second one is the closest existing precedent for what 6.3 asks: the right
module selected for the hardware present, not for the arch name. The `.bc` files
themselves are vendor-supplied (`external/amdgpu_libdevice/*.bc`, installed at
`cmake/TaichiCore.cmake:434-438`), so the precedent is for *selection*, not
*production*. **[V]**

**Keyed on a build-time value.** `CUDA_VERSION` → `slim_libdevice.<major>.bc`,
see 2.4. **[V]**

---

## 6. What section 6.3 would touch in my territory

Listing, not proposing.

**Build gating** — `cmake/TaichiCore.cmake:1-11`, `:25-75`, `:94-132`, `:192-302`,
`:278-283`, `:384`; `taichi/rhi/CMakeLists.txt:17`, `:28`, `:42-105`;
`cmake/TaichiCAPI.cmake:31-60`; root `CMakeLists.txt:142-144, 149-164, 206-221`;
`cmake/TaichiCXXFlags.cmake:139-149`;
`taichi/runtime/llvm/runtime_module/CMakeLists.txt:8-13, 29-31`. **[V]**

**Selection chains** — `taichi/program/program.cpp:19-43, 80-133, 140`;
`taichi/aot/module_loader.cpp:31-58`; `c_api/src/taichi_core_impl.cpp:247-309`;
`c_api/src/taichi_llvm_impl.cpp:78-121`;
`taichi/program/compile_config.cpp:67-76`. **[V]**

**Capability declaration, population and checking** —
`taichi/inc/rhi_constants.inc.h:8-35`; `taichi/rhi/device_capability.{h,cpp}` in
full; `taichi/rhi/public_device.h:617, 852-856`;
`taichi/program/program_impl.h:170-172`;
`taichi/runtime/program_impls/gfx/gfx_program.cpp:82`;
`taichi/program/program.h:136-137`; `taichi/aot/module_loader.h:84-85, 97-100`;
`taichi/aot/module_data.h:125, 135`; `taichi/runtime/gfx/aot_utils.h:20, 23`;
`taichi/runtime/gfx/aot_module_builder_impl.cpp:22-24`;
`c_api/src/taichi_gfx_impl.cpp:18-29`; `taichi/program/program.cpp:514-557`;
`taichi/analysis/offline_cache_util.cpp:88-95, 185, 190`. **[V]**

**Per-backend capability population** — the eight `set_caps` sites in the table
at 4.2, plus the two producers of `spirv_has_physical_storage_buffer` at 4.3. **[V]**

**Per-backend hardware detection that is not published** — added in revision 4,
per 4.8 and 4.9. `taichi/rhi/vulkan/vulkan_device.cpp:2515-2534` (memory
properties, heap sizes unread), `:1596` with `vulkan_device.h:710-712, 739`
(stored device properties) and its two readers `vulkan_device.cpp:1125-1128` and
`:1945`; `taichi/rhi/vulkan/vulkan_device_creator.cpp:432-440, 504-518` (device
name to debug log) and `:523-533` (`apiVersion` → `spirv_version`, the one
published field); `taichi/rhi/opengl/opengl_api.cpp:20-21, 231-236` (two
write-only globals); `taichi/rhi/cuda/cuda_context.cpp:29-33, 73-77, 79-81,
88-98` (per 4.5); `taichi/rhi/metal/metal_device.mm:1017-1069, 1168, 131-135`
(the one complete chain, per 4.9). **[V]**

**Arch classification** — `taichi/inc/archs.inc.h:4-17`; `taichi/rhi/arch.h:7-32`;
`taichi/rhi/arch.cpp:6-93`. **[V]**

**The arch-to-feature table** — `taichi/program/extension.cpp:8-33`;
`taichi/inc/extensions.inc.h:1-10`; `taichi/program/extension.h:17-24`. Inert in
C++ per 4.1. **[V]**

**C API surface** — `c_api/taichi.json` (`arch` and `capability` declarations);
`c_api/include/taichi/taichi.h:1-29`;
`c_api/include/taichi/taichi_core.h:358-375, 380-407, 409-416, 883-899, 926-955`;
`c_api/src/taichi_core_impl.cpp:13-61, 171-205, 247-309, 317-366`;
`c_api/src/taichi_opengl_impl.cpp:4-13`; `c_api/src/taichi_vulkan_impl.cpp:38-53`;
`misc/taichi_json.py:102-116, 182-183`. **[V]**

**Install surface** — `cmake/TaichiConfig.cmake.in`;
`cmake/TaichiTargets.cmake:56-88`; `cmake/TaichiCAPI.cmake:117-195`. **[V]**

**Module naming and location** — `taichi/runtime/llvm/llvm_context.cpp:209-211,
213-218, 625-655`; `taichi/util/lang_util.cpp:30-47`. **[V]**

### 6.1 Stub shape, per brief 5.1

`is_dx12_api_available()` (`taichi/rhi/dx12/dx12_api.cpp:6-12`) returns `true`
whenever `TI_WITH_DX12` is defined and never touches a device, so
`TI_ASSERT(directx12::is_dx12_api_available())` at `taichi/program/program.cpp:87`
is unconditionally true in a DX12 build, and `make_dx12_device()`
(`dx12_api.cpp:14-16`) then returns `nullptr`. The whole DX12 RHI is
`dx12_api.h` 20 lines and `dx12_api.cpp` 26 lines. **[V]**

It reports *build configuration* where the others report *hardware*:
`vulkan::is_vulkan_api_available()`, `opengl::is_opengl_api_available()`,
`metal::is_metal_api_available()` and `taichi::is_cuda_api_available()` all probe
(call sites `c_api/src/taichi_core_impl.cpp:13-61`). **[V]**

`aot::Module::get_required_caps()` has the analogous shape: its default
(`module_loader.h:97-100`) returns an empty config, which reads as "requires
nothing" rather than "has not declared", and that is what makes the check in 5.2
silently pass. **[V]**

Recording only; not proposing changes.

### 6.2 Dead and near-dead build code

Recording, not proposing removal, per standing instruction 3.
- `cmake/TaichiCore.cmake:104` globs `taichi/runtime/cuda/runtime.cpp` and `:110`
  globs `taichi/runtime/amdgpu/runtime.cpp`. **Neither file exists** — verified by
  listing both directories, which hold only `jit_*.cpp/.h`,
  `kernel_launcher.cpp/.h` and `CMakeLists.txt`. Both globs are empty. **[V]**
- `cmake/TaichiCore.cmake:111` appends to `TAIHI_CORE_SOURCE`, a typo for
  `TAICHI_CORE_SOURCE` (correct spelling only at `:77, 105, 137, 340`). Inert
  today because the glob it appends is empty. **[V]**
- `taichi/runtime/llvm/runtime_module/CMakeLists.txt:13` uses `${arch}` inside a
  function whose parameter is `rtm_arch`; it resolves only because the caller's
  `foreach` variable (`:29-31`) is in scope. `:8` correctly uses `${rtm_arch}`. **[V]**
- `taichi/rhi/dummy.cpp` is a zero-byte file and `ti_device_api_shared` has no
  consumer (2.3). **[V]**

---

## 7. Width audit for 6.2

The RHI's own addressing is already 64-bit. `taichi/rhi/public_device.h`: **[V]**

| Thing | Type | Line |
|---|---|---|
| `DeviceAllocationId` | `uint64_t` | 85 |
| `DeviceAllocation::get_ptr(offset)` | `uint64_t` | 92 |
| `DevicePtr::offset` | `uint64_t` | 123 |
| `Device::AllocParams::size` | `uint64_t` | 623 |
| `get_memory_physical_pointer` | `uint64_t` | 635 |
| `kBufferSizeEntireSize` | `size_t` max | 51 |
| buffer bind sizes | `size_t` | 154, 172, 364, 392, 407 |

Genuinely 32-bit, matching the underlying APIs: binding indices (`:152, 161, 170,
179, 188, 199`), `dispatch(x, y, z)` (`:426-428`), `ComputeSize{x,y,z}`
(`:430-434`), image extents and offsets (`:286-306`). **[V]**

`DeviceCapability` is `uint32_t` (`device_capability.h:11`) and levels are
`uint32_t` (`:22`). `spirv_version` is a packed hex integer, e.g. `0x10300`
(`program.cpp:536`, `vulkan_device_creator.cpp:526-532`, `metal_device.mm:1045`,
`dx_device.cpp:564`, `opengl_device.cpp:528`, `vulkan_device.cpp:1579`). **[V]**

Note that the shader-side 64-bit pointer capability is a distinct axis from the
allocation and pointer widths tabulated above, and it is the one that is switched
off (4.3). "Distinct" is not "peripheral": revision 2 wrote "separate axis" in a
context that made it read as a side concern because the section 5.2 cards are not
on that spine. Revision 4 then over-corrected, calling that spine "the reference
path" and this axis "the primary existing form of 6.2". **Both readings are
withdrawn in revision 5**; the plan supplies no ranking between the spines and
section 2.2a puts item 6.2 in full in BASE scope regardless of spine. The
statement that stands is the source one: this axis is switched off on
`arch_uses_spirv` and has no counterpart at all on `arch_uses_llvm`, and it is
bounded, per 4.3, to ndarray, argpack and return-struct addressing rather than
SNode addressing.

---

## 8. Escalations

Unresolved. Each needs a decision that has not been made, **except escalation
16**, which plan section 5.1a closes and which is retained struck through with
its closure recorded. Escalations 21 and 22 are new in revision 5.

1. **How the SNode ceiling reaches the bitcode. Three routes, not one.**
   (Revision 3: revision 2 said "only one route is sound". Withdrawn — it was
   reached without reading the `-D` on the same command line. See 2.5.1.)
   - **Generated `constants.h`.** Requires the source-tree header to stop being
     a header: adding a directory to the `-I` list at
     `runtime_module/CMakeLists.txt:8` leaves two headers of the same relative
     path visible with include order deciding (2.5). Generating into
     `${CMAKE_SOURCE_DIR}` is what `version.h` and the `.bc` artefacts already
     do (`CMakeLists.txt:203`, `runtime_module/CMakeLists.txt:10, 13`).
   - **A `-D` on the same clang line.** The mechanism is already in use and
     already carries a configure-time value into the bitcode, `-D "ARCH_${rtm_arch}"`,
     consumed at 21 sites (2.5.1). It needs a macro name `constants.h` reads,
     because `-D` on the `constexpr` name itself would not compile.
   - **Both**, if the header is generated and the per-variant part is a `-D`.

   Which route is taken is a decision that has not been made.

2. **Is `CUDA_VERSION` the intended precedent for 6.1, and macro or constexpr?**
   It is a working non-boolean value carried into a generated header and used to
   pick a device bitcode file (2.4). One hazard if a generated `constants.h`
   follows it as *macros*: `taichi/rhi/cuda/cupti_toolkit.cpp:263, 502, 725` use
   `#if CUDA_VERSION >= 11040`, an integer comparison against what would be a
   string macro. No collision today, because `taichi/common/version.h` has only
   three includers (`taichi/common/core.cpp:7`, `taichi/util/offline_cache.h:12`,
   `taichi/runtime/llvm/llvm_offline_cache.cpp:14`) and `cupti_toolkit.cpp` is not
   one; its `CUDA_VERSION` is the CUDA Toolkit's own. But `constants.h` is
   included far more widely, so the collision class becomes live.

3. **What relation should gate module compatibility, and per capability?**
   `c_api/src/taichi_gfx_impl.cpp:25` uses `!=`. Per 4.4, the published contract
   at `taichi_core.h:411-412` declines to guarantee ordering while the engine
   already assumes it for `spirv_version` in nine places. `>=` would be right for
   `spirv_version` and meaningless for the booleans stored as levels, which the
   current single-relation loop cannot express. Changing the relation changes the
   published contract.

4. **Does the AOT module need arch and version handshakes at all?** Today none
   exists (5.2): the capability check is dead, `version()` is uncalled and aborts
   on two of three paths, `arch()` is uncalled and returns the caller's own arch,
   and the module records no arch. Wiring the capability check alone would leave
   two of three gaps open — and would immediately start rejecting modules, since
   `required_caps` has exactly one producer and the generic struct is never
   written (5.1).

5. **Must a changed SNode ceiling invalidate offline caches and LLVM AOT
   modules, and keyed on what?** `taichi/util/offline_cache.h:95-100` keys on
   `TI_VERSION` alone and `llvm_aot_module_loader.h:25` inherits it. A ceiling
   change does not move `TI_VERSION`. The cache *format* is territory 03; this
   invalidation seam sits in 6.1.

6. **Is `Extension` a live mechanism in this fork at all?** Per 4.1 the C++ core
   reads only `assertion`, `bls`, `mesh` and `quant` from it, and `data64` reaches
   only the Python binding and test harness, which brief 1.2 puts out of scope.
   That is a scope decision, not a reconciliation.

7. **How should the 5.2 fidelity classes be expressed?** (Restated in revision 4;
   revision 2 asked this as a CUDA question.) Per 4.8 **every** backend that can
   ask its hardware about class does ask, and none of them publishes the answer:
   CUDA detects compute capability and clamps it (`cuda_context.cpp:73-77`);
   Vulkan queries memory properties and reads no heap size
   (`vulkan_device.cpp:2515-2532`), stores full device properties and reads two
   fields (`:1125-1128`, `:1945`); OpenGL fills two globals nothing reads
   (`opengl_api.cpp:20-21, 231-236`). None of it reaches `DeviceCapability`,
   `Extension`, the AOT `required_caps` or the C API. There is no vocabulary for
   a fidelity class on any spine. Two models exist in the tree, one on each
   spine: **Metal** (SPIR-V spine) already runs the whole
   detect → name → publish → consume chain (4.9), and the **AMDGPU ISA-version
   module selection** (5.3, LLVM spine) is the only hardware-keyed module choice.
   That they sit one per spine is a fact about where the machinery is, not a
   reason to prefer either. Which model, and what the vocabulary should contain,
   is not settled anywhere in the plan.

8. **Adding a device capability changes two keys, not one.** Unless appended at
   the end of `rhi_constants.inc.h`, it renumbers the public C ABI
   (`misc/taichi_json.py:115`) **and** every offline-cache key
   (`offline_cache_util.cpp:92`). Whether the C ABI is fixed at all in this fork
   is undecided; the cache key is a separate question with a different owner.

9. **What should an installed Taichi tell its loader?** Per 3.5, the install
   records no configuration, while the public umbrella header
   (`c_api/include/taichi/taichi.h:9-27`) requires the consumer to already know
   it. Brief section 5 has a loader configuring the binary at install time.
   Whether the answer is a generated header, a variable in
   `TaichiConfig.cmake.in`, or a C entry point is not recorded in the plan.

10. **Is `TI_WITH_CPU` a bug or a name to be chosen?** Referenced at
    `c_api/include/taichi/taichi.h:21, 23` and documented as a build option at
    `docs/lang/articles/deployment/tutorial.md:288`, defined nowhere. Either the
    build should define it or the header should use `TI_WITH_LLVM`, which is what
    `cmake/TaichiCAPI.cmake:31-33` actually gates `taichi_cpu.h` on.

11. **`ti_set_runtime_capabilities_ext` overwrites detected capabilities with no
    validation** (`c_api/src/taichi_core_impl.cpp:317-334`). Either the intended
    install-time override hook or a hazard; the code does not say which.

12. **Should any backend but Vulkan be able to see more than device 0?**
    (Restated in revision 4; revision 2 asked this as a CUDA question because of
    the cards in brief 3.1.) Per 3.3 the refusal is enum-wide:
    `ti_create_runtime` rejects a non-zero `device_index` for opengl, x64, arm64,
    cuda and metal (`c_api/src/taichi_core_impl.cpp:270, 277, 283, 289, 297`),
    and Vulkan alone honours it (`:254-255`). CUDA additionally pins device 0 in
    its own driver layer (`cuda_context.cpp:21-22`). So the single existing
    multi-device path is on the SPIR-V spine, and the shortfall is on the LLVM
    spine. Per 3.3 the substantive part of it is `cuda`, `amdgpu` and `dx12`;
    whether the `x64` and `arm64` refusals also count as a gap is disputed and is
    escalation 21.

    **Restated in revision 5, and the question is now larger than it was.** Per
    3.3a there are **two** device-selection channels on Vulkan, not one:
    the C API `device_index` parameter, and the `TI_VISIBLE_DEVICE` environment
    variable read at `taichi/rhi/vulkan/vulkan_loader.cpp:111-113` and consumed
    at `vulkan_device_creator.cpp:442-456`. Plan section 5.1a puts the
    configuration agent **outside the process** and makes a multi-GPU machine the
    ordinary case, so an out-of-process channel is the shape that section
    describes. Three things are undecided and none is mine:
    - Should any backend but Vulkan be able to see more than device 0?
    - Is an environment variable read at loader init the intended install-time
      channel, and if so why does only one backend have one?
    - **Which of the two inputs is authoritative?** Per 3.3a they collide
      order-dependently: the environment wins on the first Vulkan runtime in a
      process and the C API argument wins thereafter, and the selector is a
      member of a process-wide singleton rather than per-runtime.

13. **What is done with the SPIR-V 64-bit pointer path, and does it have to be
    re-enabled to be exercised?** (Restated in revision 5 against the plan dated
    2026-09-09. Revision 2 asked "is it in scope", presuming it might not be
    because the section 5.2 cards are elsewhere. Revision 4 answered that with
    "it is on the reference path", which is **withdrawn** — the plan contains no
    such phrase. **Scope is settled, by plan section 2.2a**, which puts "Item
    6.2, 64-bit addressing, in full" in BASE scope and names the shelved SPIR-V
    path as part of it. So the question is what to do with the machinery, not
    whether it counts, and that follows from the plan's own scope statement
    rather than from any ranking of spines. It is bounded to ndarray, argpack and
    return-struct addressing and does not reach SNode
    addressing.) Per 4.3 `spirv_has_physical_storage_buffer` has
    twelve live consumers and two disabled producers, one of which was already
    coupled to `shaderInt64` (`vulkan_device_creator.cpp:821`). Reviving a
    producer is a decision with a referenced upstream issue behind it (taichi
    6295), not a mechanical change. There is a **third answer** revision 2 missed:
    `ti_set_runtime_capabilities_ext` (`c_api/src/taichi_core_impl.cpp:331`)
    installs any capability set wholesale with no validation, onto the same
    `Device::caps_` all twelve consumers read, and
    `c_api/include/taichi/cpp/taichi.hpp:1156` ships a typed setter for it. So
    the path is reachable today from outside the core with no source change.
    Whether that is a supported way to find out whether the machinery works, an
    install-time override hook, or a hazard to be closed, the code does not say.
    This is the sharp form of escalation 11.

14. **Does dispatch geometry count as "addressing" for 6.2?** `dispatch` and
    `ComputeSize` are `uint32_t` (`public_device.h:426-434`), matching Vulkan and
    CUDA. Allocation and pointer addressing is already 64-bit throughout (7).

15. **Does `taichi/rhi/CMakeLists.txt:17` need fixing, and is Android a target?**
    The precedence bug is real (2.8); whether it matters is a scope decision the
    brief does not record.

16. **CLOSED by plan section 5.1a. Not an open question.** ~~Is 6.1's answer one
    configured binary or N pre-sized bitcode variants?~~ Revisions 1 to 4 of this
    report were written without sight of section 5.1a and asked two things here:
    what the variant filename carries, and where the host-side bound at
    `taichi/codegen/llvm/struct_llvm.cpp:266` comes from. **Section 5.1a answers
    both.** "Per build already means per device configuration"; a two-card
    machine gets two binaries and the system activates one or both; "the filename
    scheme does not need to grow, and no common denominator has to be found."
    And per 2.5.2 the existing build loop already produces exactly that: the loop
    at `runtime_module/CMakeLists.txt:29-31` varies only `arch`, so one configure
    yields one ceiling across every `runtime_*.bc`, and the host assertion at
    `struct_llvm.cpp:266` compiles in the same configure and therefore agrees by
    construction. What remains is the **sizing rule**, which plan section 8.1
    item 2 owns, and the **choice of route**, which is escalation 1. Retained
    here struck through rather than deleted, because two adversary files cite it
    as open. I record the closure; only the planner may enter it.

17. **Is item 6.2 one work item or two?** Per 4.7 it divides along
    `arch_uses_spirv` (`taichi/rhi/arch.cpp:63-66`) into a finish-and-enable half
    on the SPIR-V spine and a start-from-nothing half on the LLVM spine, which
    carries CPU-only operation as well as CUDA. Sizing it as one item conflates
    two different amounts of work. Which spine is worked, and in what order, is
    not settled anywhere in the brief.

18. **Where does detected hardware class get published, and to whom?** Per 4.8
    detection exists on every spine and publication exists on none: no backend's
    hardware-class data reaches `DeviceCapability`, `Extension`, the AOT
    `required_caps` or the C API. Item 6.3 needs the install-time loader to see
    it, and plan section 5 puts that loader outside the process. Whether the
    channel is the capability map (which would need entries that are not feature
    bits, escalation 19), a separate structure, or something the installed
    package records (escalation 9), is not decided. The one field the SPIR-V
    spine discards and the LLVM spine keeps is the memory budget — Vulkan asks
    for memory properties and never reads `heapSize`
    (`vulkan_device.cpp:2515-2532`), while `cuda_context.cpp:88-98` reads total
    and free memory — and the Baseline tier's 2 GB sizing is exactly that
    question. Recorded per plan section 5.2 as a gap in the SPIR-V spine on that
    one axis, not as a reason to prefer either spine.

19. **Can a fidelity class be expressed in the capability vocabulary at all?**
    Per 4.9 Metal already runs the full chain — hardware family ladder
    (`metal_device.mm:1027-1036`) → named features (`:1038-1042`) → published
    caps (`:1044-1069`, installed `:1168`) → read back (`:131-135`). The
    mechanism is the model escalation 7 wants. But what it publishes is feature
    admission: below Apple3 there is no `spirv_has_int64`. Plan section 2.2 says
    hardware must change speed and granularity and never what the system can do,
    and calls the opposite "a violation, not an optimisation". Every entry in
    `taichi/inc/rhi_constants.inc.h:8-35` is today either a feature bit or a
    version number; none is a fidelity level. Whether a fidelity class belongs in
    that enum, in a parallel one, or nowhere in the RHI, is a decision that has
    not been made. Note that `rhi_constants.inc.h:1-7` invites exactly this, and
    names CUDA compute capability as something that "can be listed here".

20. **Should the C API build capability sets by assertion?** Per 4.2 it does, in
    two places, and they fail in opposite directions.
    `c_api/src/taichi_opengl_impl.cpp:8-12` sets `spirv_has_int64` and
    `spirv_has_float64` unconditionally, discarding the `!is_gles()` guard at
    `opengl_device.cpp:509-513` that exists to keep a low-end profile from being
    told it has 64-bit support, and discarding the int16/float16 detection at
    `:515-526` that would report capability the hardware really has.
    `ti_set_runtime_capabilities_ext` (escalation 11) lets a host do the same
    thing wholesale. Plan sections 2 and 5.3 put edge hardware first and make
    modern capability an opportunity rather than a prerequisite, which is the
    interest the discarded guard serves.

    **Restated in revision 5, and the premise it rested on is withdrawn as
    false.** This escalation previously closed: "The over-declaration cannot fire
    today because GLES is unreachable through the C API ... so whether to close
    the assertion, wire up GLES, or both, is one decision with two halves." **It
    is not two halves, because GLES is already wired up.** Per 4.2a,
    `ti_import_opengl_runtime` (`c_api/include/taichi/taichi_opengl.h:33-36`,
    body `c_api/src/taichi_opengl_impl.cpp:21-29`) takes a `use_gles` argument
    and sets the override at `:26`, and the RHI's correct refusal at
    `opengl_device.cpp:509-513` is then overwritten by the assertion at
    `taichi_opengl_impl.cpp:8-12`. The defect is live on a documented public
    entry point.

    The question that remains is sharper and single: **what should the C API do
    when its own device has just detected the opposite of what the C API
    asserts?** Three shapes are available and the code does not say which is
    intended — stop replacing and merge into the detected set; keep replacing,
    on the ground that an interop host is presumed to know its own context; or
    validate the replacement against the device and fail. It now also reaches
    `ti_set_runtime_capabilities_ext` (escalation 11), which does the same thing
    wholesale with no device consulted at all. A second, separable question sits
    beside it: per 4.2a the `use_gles` argument is **silently lost** instead if
    `initialize_opengl` has already run in the process, which the natural
    install-time-loader sequence under plan section 5 causes. So the same
    override produces two opposite wrong outcomes depending on call order.
    Whether that ordering is a bug to fix or an interop contract to document is
    also not recorded.

    Still true and unchanged: `Arch::gles` is named at
    `taichi/program/program.cpp:124` and `taichi/aot/module_loader.cpp:40`, and
    has no `ti_create_runtime` case and no probe in `ti_get_available_archs`. The
    C API reaches GLES **only** through the OpenGL import path, never by naming
    the arch.

21. **Does a refused capability with nothing to address count as a gap?** Per 3.3,
    `ti_create_runtime` refuses `device_index != 0` on `x64` and `arm64`
    (`c_api/src/taichi_core_impl.cpp:277, 283`), and there is no CPU device index
    anywhere for it to name: `taichi/rhi/cpu/` is three files, a grep for
    `device_index`, `device_get_count` or `num_devices` across `taichi/rhi/cpu/`
    and `taichi/rhi/llvm/` returns nothing, and `host_arch()`
    (`taichi/rhi/arch.cpp:68-76`) returns one arch. One reading calls the refusal
    correct rather than missing, so the LLVM-spine shortfall is three archs
    (`cuda`, `amdgpu`, `dx12`). The other calls CPU coverage what lets the
    finding stand enum-wide without reference to the owner's cards, making it
    four. **[V]** for the evidence; the source does not settle the reading. It
    matters only because plan section 5.2 asks that gaps be recorded per spine,
    and the size of this one is three or four depending on the answer. Raised by
    `adversary3-04-2.md` §12.5 and §13 against `adversary3-04-1.md` claim 7; I
    verified both sides and am recording both.

22. **Is `get_device_score` gating an explicitly requested device intended?** Per
    4.8, `vulkan_device_creator.cpp:452` uses the score as a boolean admission
    gate: a device named through `device_index` or `TI_VISIBLE_DEVICE` whose
    score is zero is silently dropped and the automatic pick at `:458-468` runs
    instead, with no log on that branch — the only error log, at `:448-451`,
    covers the out-of-range case only. Under plan section 5.1a an install-time
    agent naming a device would expect that device or a failure, not a silent
    substitution. Whether this is a defect or a deliberate fallback the code does
    not say, and grading it is not mine.


---

## 9. Corrections made in this revision

Every item was re-checked against source by me before being changed. Notes
entries 19-35 carry the checks.

### 9.1 Claims withdrawn — all were marked [V] and were false

| Claim (revision 1) | Status |
|---|---|
| `Extension::data64` is "a compile-time blocker sitting in front of 6.2 for every non-CUDA GPU path" | **False.** No C++ site queries `data64`; the six `is_extension_supported` callers pass `assertion`, `bls`, `mesh`, `quant` only. Replaced by 4.1. |
| `ti_device_api_shared` is "the only shared-library backend artefact, and it contains every enabled backend at once" | **False on both halves.** `dummy.cpp` is 0 bytes; three references, none linking or installing. Replaced by 2.3. |
| "Each toggle does exactly three things and nothing else" | **False.** Four counter-examples. Replaced by 2.1 and 2.2. |
| "There is no project-defined non-boolean CMake cache variable controlling the build of Taichi itself" | **False.** `HOST_ARCH` at `cmake/TaichiCXXFlags.cmake:149`. Replaced by 2.6. |
| "The only hardware query for 64-bit integers in the entire tree is `vulkan_device_creator.cpp:623-632`" | **False**, and it contradicted my own table. Metal queries too (`metal_device.mm:1027-1038`). Replaced by 4.2. |

### 9.2 Downgraded

- The `-I` route for 6.1 was offered as one of two valid options. It is a defect
  in that option, not a variant. Escalation 1.
- `!=` at `taichi_gfx_impl.cpp:25` was called "a second, independent defect". It
  is a contract decision. Adjudicated in 4.4; escalation 3.
- Escalation 4 of revision 1 ("two conflicting capability systems") is restated
  as escalation 6, a scope question.

### 9.3 Citations corrected

| Was | Is |
|---|---|
| `archs.inc.h` has 13 entries | 12 |
| `TiArch` 8 cases against 13 | 8 enumerators incl. `TI_ARCH_RESERVED`, naming 7 of 12 |
| `taichi/aot/` is 6 files | 7 |
| `taichi_core_impl.h:120-121` | `:121-122` |
| `runtime_lib_dir` `:30-46` | `:30-47` |
| `version.h.in:2-6` | the file is 5 lines |
| 7 `-DTI_WITH_*` append sites | 9, adding `:94` and `:281` |
| 6 `#else` `TI_ERROR` arms | 7, adding `:94` |
| `cuda_context.cpp:80-82` | `:79-81` |
| `cuda_context.cpp:88-96` | `:88-92` and `:94-98` |
| `public_device.h:426-433` | `dispatch` `:426-428`, `ComputeSize` `:430-434` |
| `spirv_has_atomic_int64`: 2 hits | 3, adding `python/taichi/lang/enums.py:27` |
| "only four backends call `set_caps`" | 8 sites, 5 in RHI and 3 in the C API |
| `HOST_ARCH` set at `CMakeLists.txt:154-164` | `cmake/TaichiCXXFlags.cmake:149`; only the three arch-specific ones are set in the root file |

### 9.4 Findings added

`spirv_has_physical_storage_buffer` dead (4.3); the third selection chain (3.2);
the CUDA device-0 pin (3.3); `ProgramImpl::get_device_caps` (4.2); the C API
OpenGL capability override (4.2); no arch or version handshake (5.2); the two
`required_caps` structs (5.1); no cache invalidation on a ceiling change (5.2);
capability renumbering also moves the cache key (4.6); the `CUDA_VERSION`
precedent (2.4); the umbrella header and `TI_WITH_CPU` (3.5); the AMDGPU
hardware-keyed module selection (5.3); the `taichi/rhi/CMakeLists.txt:17`
precedence bug (2.8); `extension.cpp:31` mutating static state (4.1).

### 9.5 Adjudications between the two adversaries

- **Metal `set_caps`.** They disagreed about pass B. Source:
  `metal_device.mm:1168` does call it. Adversary 04-1 is right. My original table
  was right on Metal; the sentence above it was not (4.2).
- **Capability level ordering.** Adversary 04-1 said the documentation settles it
  and `!=` is "the only relation the documented type supports". Adversary 04-2
  treated it as a defect worth escalating. **Neither is right.** "Not guaranteed"
  is weaker than "not ordered", and the engine already assumes ordering for
  `spirv_version` in nine places. It is a contract decision with an internal
  contradiction already present. Adjudicated in 4.4, escalation 3.
- **`ti_device_api_shared`.** Adversary 04-1 argued only that it is unused;
  adversary 04-2 additionally refuted the content claim on link semantics.
  04-2's is the stronger argument and I verified it (`dummy.cpp` is 0 bytes).

### 9.6 Amendment pass — adjudication of the seven round-two claims

Seven claims were put to me from `adversary2-04-1.md` and `adversary2-04-2.md`.
I verified each against source before acting. Notes entries 37-43, one per claim.

| # | Claim | Verdict | What changed |
|---|---|---|---|
| 1 | The clang line carries `-D "ARCH_${rtm_arch}"`, a configure-time value compiled into the bitcode, consumed at ~21 sites, one variant per value, selected by filename — narrowing escalation 1 | **Upheld exactly.** 21 sites: 19 in `runtime.cpp`, 2 in headers it includes | New 2.5.1. Escalation 1 rewritten from one route to three |
| 2 | The ordered-comparison census says nine where there are thirteen; the four missed are the `spv_target_env` ladder | **Upheld exactly.** `spirv_codegen.cpp:2669, 2671, 2673, 2675` in the block `:2666-2679` | 4.4 Fact 2 corrected and the ladder described. The adjudication is unchanged |
| 3 | The host-versus-device layout disagreement cannot happen; the host has only an opaque forward declaration typed from the bitcode | **Upheld.** `LLVMRuntime` is defined only at `runtime.cpp:552`; host declares it at `context.h:11` and `llvm_device.h:8` | 2.5 mechanism replaced. The section's conclusion is unchanged and strengthened |
| 4 | "Permanent" is false; `ti_set_runtime_capabilities_ext` installs capabilities wholesale, and this report cites that function elsewhere | **Upheld.** `taichi_core_impl.cpp:326-331` onto `Device::caps_` (`public_device.h:617`) | 4.3 retracted and restated. Escalation 13 gains a third answer |
| 5 | The GGUI compile-definition line is wrong, inherited from a round-one adversary error the paired report refused | **Upheld.** `:383` is `if(TI_WITH_GGUI)`; the call is `:384` | Corrected in 2.1 and in the section 6 map |
| 6 | "No dlopen of Taichi's own code" is marked **[V]** with no citation anywhere | **Upheld.** The claim is true; the citation was absent | Summary point 1 now carries the census |
| 7 | Neither report states that the CUDA path never populates the device capability system while the SPIR-V backends do | **Wrong as put. Nothing changed for it.** This report states it twice already: summary point 3, and 4.2 — "**CUDA, AMDGPU, CPU and DX12 RHI devices never call `set_caps`**", with the consequence spelled out in the paragraph under it and again through `ProgramImpl::get_device_caps`. What neither report states is the *consequence for sizing 6.2*, which is what adversary 04-2 actually claimed (its §3.2, "neither report says in one place that 6.2 splits into two"). That gap is real and is now 4.7 and escalation 17 |

Nothing in this pass was verified by agreement between the two adversary files.
Every figure above was re-derived from source in this pass.

### 9.7 Framing pass — the six statements restated, and what was added

Plan section 5.2 was amended to make the three cards **fidelity** classes and
explicitly not a vendor target, and section 2.2 was added. Six statements in this
report were factually sound and drew their conclusion from the card list. None is
withdrawn as false; each is restated so the conclusion follows from the source.

| # | Where | Was | Is |
|---|---|---|---|
| 1 | 3.3, closing line | "Brief 3.1 names two NVIDIA cards in this box. Today CUDA can only see device 0." | The device-index refusal covers opengl, x64, arm64, cuda and metal alike (`taichi_core_impl.cpp:270, 277, 283, 289, 297`); Vulkan alone honours it (`:254-255`). The one multi-device path in the tree is on the portable spine |
| 2 | 4.2, after the `set_caps` table | "no device-capability record whatsoever on the CUDA path, which is the path the section 5.2 target tiers are about" | The emptiness covers the whole `arch_uses_llvm` set `{x64, arm64, cuda, dx12, amdgpu}` (`arch.cpp:54-57`), including CPU-only operation |
| 3 | 4.5, title and tier paragraph | "The section 5.2 target tiers", and the card mapping presented as making the tiers observable | Retitled to what the section actually covers, one backend's detection. The mapping now says "on this backend", and the section states what it does not establish, against plan 2.2 |
| 4 | 4.3, closing, and 7, closing note | The switched-off 64-bit pointer machinery framed as a side axis, because the cards are on the other spine | **This row's replacement is itself WITHDRAWN in revision 5.** It read: "It is on the reference path, so it is the primary existing form of item 6.2 in this territory — bounded to ndarray, argpack and return-struct addressing, and not raising 6.2's priority against plan 2.3." The plan contains no "reference path" and supplies no ranking. What survives is the bound. See 9.8 |
| 5 | Escalation 7 | "How should the 5.2 tiers be expressed?", asked as a CUDA question | Asked across all spines, with the two in-tree models named, both portable or vendor-neutral in shape |
| 6 | Escalation 12 | "Should CUDA be able to see more than device 0?", justified by the cards | Asked enum-wide, with the refusal sites and the one honouring arch |

Also restated: escalation 13's "is it in scope for 6.2", and the 4.7 sentence
about CUDA-side detection, which 4.8 falsifies as an asymmetry.

**Added in this pass:** section 4.8, the new finding that both spines detect
hardware class and neither publishes it; section 4.9, Metal as the one complete
detect-name-publish-consume chain, with the plan section 2.2 caveat on what it
publishes; the GLES reachability and OpenGL assertion weighing in 4.2; the
hardware-detection block in the section 6 map; and escalations 18, 19 and 20.

**Not changed, deliberately.** No count, citation or census in this report was
altered in the framing pass. Where the source says a thing about one backend, it
still says it about that backend; what changed is the sentence that told the
planner what it meant.

**Superseded by revision 5.** The framing pass above was carried out faithfully
against the instruction it was given, and that instruction quoted a version of
plan section 5.2 that no longer exists. Section 9.8 records what it produced and
what each statement now says. The framing pass's *other* products — sections 4.8,
4.9, the escalation restatements at 7, 12 and 18, and every census — are
unaffected and none is withdrawn.

### 9.8 Revision 5 — the superseded framing, every instance in this report

Plan version worked against: **`PROJECT-PLAN.md` dated 2026-09-09**, read in full
at the start of this pass.

**What happened, so it is not re-derived.** An intermediate version of plan
section 5.2 opposed "the portable path" to "a vendor-specific path" and called
the latter "an optional accelerant, never the baseline". That wording reached me
through the round-two amendment instruction and is quoted verbatim in my own
notes at `notes-04-backend-build.md:1946-1949`. Revision 4 applied it
faithfully. **The plan dated 2026-09-09 withdraws that axis by name** — section
5.2's own CORRECTION paragraph says "That is the wrong axis, it was mine, and an
agent caught it", and section 2.2a restates it positively: a vendor backend is
one "tied to one supplier or one operating system", the two compilation spines
are not vendors, and "work on either spine is BASE work". Under standing
instruction 6 the plan wins and I say so. This report was correct against the
plan it was given and wrong against the plan as it stands.

**Seven instances in this report.** Found by grepping "reference path",
"accelerant", "vendor path" and "portable path" over the whole file; the line
numbers are those of revision 4, as enumerated independently by both round-three
adversaries.

| # | Line (rev 4) | Section | Superseded statement | What it says now |
|---|---|---|---|---|
| 1 | 716 | 4.3, weighting | "`{opengl, gles, vulkan, dx11, metal}` is the **reference** path, and the LLVM spine is where the incidental test hardware happens to sit" | Withdrawn. Neither spine is the vendor spine, per plan 2.2a. The LLVM spine carries CPU-only operation, a first-class target |
| 2 | 718 | 4.3, weighting | "Written-and-disabled machinery on the reference path is therefore the **primary** existing form of item 6.2 ... whatever 6.2 eventually needs on the LLVM spine is the accelerant, not the baseline" | Withdrawn. Item 6.2 exists in two different **states** on the two spines — written-and-switched-off on one, absent on the other — and the plan supplies no ranking between them |
| 3 | 720 | 4.3, weighting | "It is on the reference path, so the question is what to do with it, not whether it counts" | Restated. Scope is settled by plan section 2.2a, which puts item 6.2 in full in BASE scope, not by which spine the machinery sits on |
| 4 | 875 | 4.7, bullet | "On the SPIR-V spine — **the reference path under plan section 5.2** —" and "per 4.3 that is the primary existing form of 6.2" | Withdrawn, and this was the sharpest instance because it **cited the plan for a proposition the plan does not contain**. The bullet now names the spine and its five archs and states the state, not a rank |
| 5 | 1292 | 7, closing note | "that spine is the reference path, so this axis is the primary existing form of 6.2 in this territory" | Withdrawn. Both the revision-2 "side axis" reading and the revision-4 inversion are gone; the source statement and the ndarray/argpack/return-struct bound remain |
| 6 | 1409 | Escalation 13 | "Per 4.3 as restated, it is on the reference path, so scope is not the question" | Restated onto plan section 2.2a's scope statement |
| 7 | 1601 | 9.7, table row 4 | The correction-log row asserting the same ranking | Marked withdrawn in place, with the withdrawn text kept legible |

**The self-contradiction this produced, and it is fixed.** Revision 4 asserted
the ranking at `:716-718` and at `:875`, and then twelve lines below `:875`, in
the same section 4.7, asserted the opposite: "It is not a reason to weight 6.2
toward either spine. The LLVM spine is not 'the vendor path': it carries
`Arch::x64` and `Arch::arm64`, which is CPU-only operation, a first-class
target." Both sentences were revision-4 text and a reader could not reconcile
them. **The second is the one that matches the current plan and it is the one
kept**; the first is withdrawn. Found independently by both round-three
adversaries.

**Nothing factual moved.** No count, census or file:line citation in this report
changed on account of the framing. The two censuses the withdrawn ranking rested
on both stand: twelve live consumers and two disabled producers of
`spirv_has_physical_storage_buffer` on `arch_uses_spirv`, and no `set_caps` call
anywhere under `taichi/rhi/cuda/`, `cpu/`, `llvm/`, `amdgpu/` or `dx12/`. What
was removed is a ranking this report was not entitled to make.

### 9.9 Revision 5 — everything else in this pass

Every item below was verified against source by me before being written. Notes
entries 53-60, one per item.

| # | Item | Verdict | What changed |
|---|---|---|---|
| 1 | The superseded framing, 32 statements across the two reports, seven of them mine | **Upheld.** The plan contains neither phrase; section 2.2a withdraws the axis by name | 9.8, plus 4.3, 4.7, section 7 and escalation 13 |
| 2 | GLES **is** reachable through the C API, so this report's escalation 20 rests on a false premise | **Upheld, and it is a live defect.** `ti_import_opengl_runtime` is exported with a `use_gles` parameter (`taichi_opengl.h:33-36`), sets the override at `taichi_opengl_impl.cpp:26` | Caveat withdrawn in 4.2; new 4.2a; escalation 20 restated |
| 3 | `TI_VISIBLE_DEVICE` is an out-of-process device selector absent from both reports and all four prior adversary files | **Upheld.** `vulkan_loader.cpp:111-113`, consumed at `vulkan_device_creator.cpp:442-456`. I confirmed the absence by grepping this report and my notes | New 3.3a; escalation 12 restated |
| 4 | That selector is a process-global singleton the environment overwrites on the first Vulkan runtime, after `ti_create_runtime` has written the caller's index | **Upheld.** Singleton `vulkan_loader.h:14-17, 32`; `std::call_once` `vulkan_loader.cpp:87`; `init()` called at `vulkan_device_creator.cpp:236`, after `taichi_core_impl.cpp:254-255` | 3.3a, second half; escalation 12 |
| 5 | This report does not know plan section 5.1a exists; three escalations are closed by it, and the positive result is unstated | **Upheld.** A grep for "5.1a" over this report and my notes returned nothing before this pass | New 2.5.2; escalation 16 closed; escalation 12 and 18 reference 5.1a |
| 6 | The detect-and-discard census in 4.8 omits `get_device_score` | **Upheld.** `vulkan_device_creator.cpp:203-229`, called `:452` and `:462`; absent from every revision of this report | Added to 4.8, with the `:452` admission-gate qualification and escalation 22 |
| 7 | Pass A over-reaches by counting `x64` and `arm64` in the device-index gap (`adversary3-04-2.md` §12.5) | **Not settled by the source, and I have not picked.** Its evidence is right — three files under `taichi/rhi/cpu/`, no device index anywhere, one arch from `host_arch()` — and so is `adversary3-04-1.md`'s opposite reading | 3.3 records both positions; new escalation 21 |
| 8 | The CUDA device-0 pin should be reconciled with plan 5.1a's empirical statement (`adversary3-04-1.md` §5.6) | **Upheld.** The pin is on an ordinal; `CUDA_VISIBLE_DEVICES` returns zero hits across `taichi/`, `c_api/` and `python/` | 3.3, new paragraph. Severity restated as one-process-one-card |

**Where an adversary is wrong, and it is one place.** `adversary3-04-1.md` §6
item 1 and §3.4 both state that this report's escalation 13 "opens by asserting
the SPIR-V machinery is the primary existing form of 6.2 on the strength of the
framing". Escalation 13's opening parenthesis did carry the framing and is
restated above. But the escalation's *substance* — twelve consumers, two disabled
producers, the `shaderInt64` coupling at `vulkan_device_creator.cpp:821`, and the
third answer through `ti_set_runtime_capabilities_ext` — never depended on the
ranking and is unchanged. The framing was an opening clause, not the question.

**Two citation slips in `adversary3-04-2.md` §5.3, neither changing anything.**
It cites `is_opengl_api_available` as forwarding at `opengl_api.cpp:243-245`; the
function opens at `:244` and the body runs `:244-246`. It cites `reset_opengl` as
`:252-...`; it is `:252-256`. I used the corrected lines in 4.2a. The finding
itself is right and is the most important thing in this pass.

Nothing else in either adversary file is contradicted by source in my checking;
where I differ from one, the other found it first, and both instances are
recorded at 9.9 rows 7 and 8.

**Not changed, deliberately.** No count, census or citation in this report was
altered in revision 5 except by addition. Nothing was decided that the plan
leaves open, and nothing was struck on the grounds that it looked surplus:
escalation 16 is marked closed **by the plan**, with its text retained, and only
the planner may enter that closure.

---

## 10. Verified versus inferred, consolidated

**Verified by reading code** — everything in sections 2 through 7 carrying a
file:line citation, except the items listed below. In particular, after
re-checking in this pass: the three selection chains and their exact arch
coverage; the eight `set_caps` sites; the twelve consumers and two disabled
producers of `spirv_has_physical_storage_buffer`; that no C++ site queries
`data64`; that `get_required_caps`, `version()` and `arch()` have no live
handshake; that the offline cache validates on `TI_VERSION` only; that the
capability map is hashed into the kernel key; the complete `CUDA_VERSION` thread;
that `dummy.cpp` is 0 bytes and `ti_device_api_shared` has no consumer; that
`HOST_ARCH` is a cached non-boolean driving the bitcode loop; the CMake
precedence text at `taichi/rhi/CMakeLists.txt:17`; the **thirteen** ordered
comparisons on `spirv_version` against the contract at `taichi_core.h:411-412`;
and that `TI_WITH_CPU` is referenced twice and defined nowhere. Added in the
amendment pass and verified the same way: the `-D "ARCH_${rtm_arch}"` chain and
its 21 consumption sites (2.5.1); that `LLVMRuntime` has no host-side layout
(2.5); that `ti_set_runtime_capabilities_ext` writes the same `Device::caps_` the
twelve consumers read (4.3); the four `spv_target_env` comparisons (4.4); the
dlopen census (summary point 1); and `TaichiCore.cmake:384`. Added in the framing
pass and verified the same way: that Vulkan queries memory properties and reads
no heap size, and that `memoryHeaps`/`heapSize` appear nowhere in
`taichi/rhi/vulkan/`; that `vk_device_properties_` has exactly two readers; that
`opengl_max_block_dim` and `opengl_max_grid_dim` are written and never read
outside their own trace lines; that Metal's family ladder is published into the
capability map and read back; that `Arch::gles` appears in two dispatch chains
and neither C API entry point; and that four of the five
`spirv_has_physical_storage_buffer` codegen consumers sit in `ExternalPtrStmt`,
`compile_args_struct`, `compile_argpack_struct` and `compile_ret_struct`. Added in revision 5 and
verified the same way: that `ti_import_opengl_runtime` is an exported function
taking `use_gles` and setting the override, and every row of the chain table in
4.2a; that the override is consumed after `initialize_opengl`'s early return, and
that `is_opengl_api_available`'s default argument is `false`; that
`TI_VISIBLE_DEVICE` is read at `vulkan_loader.cpp:111-113` inside the
`std::call_once` opened at `:87`, stored on a function-local-static singleton
(`vulkan_loader.h:14-17, 32`) and consumed at `vulkan_device_creator.cpp:442-456`;
that `VulkanDeviceCreator` calls `init()` at `:236`, after
`taichi_core_impl.cpp:254-255`; that `get_device_score`
(`vulkan_device_creator.cpp:203-229`) reads device type and API minor version and
is called at `:452` and `:462`, the first as an admission gate; that
`CUDA_VISIBLE_DEVICES` has zero occurrences across `taichi/`, `c_api/` and
`python/`; that `taichi/rhi/cpu/` is three files with no device index; and that
the bitcode loop at `runtime_module/CMakeLists.txt:29-31` varies only `arch`.

**Inferred, not read from this tree:**
- That GTX 750 is sm_50, GTX 1070 is sm_61 and RTX 3060 is sm_86, and that these
  map to the brief's three tiers. External knowledge; the detection, the clamp and
  the format string are verified, the card-to-number mapping is not in the tree.
- That a `taichi_max_num_snodes` raised for the host but not for the bitcode
  fails silently rather than at build time. The include-path mechanism, the
  assertion at `struct_llvm.cpp:266` and the write at `runtime.cpp:1005` are
  verified; the failure mode is reasoning. (Revision 3: revision 2 stated this as
  a host-versus-device *layout* mismatch, which cannot happen. See 2.5.)
- That a host calling `ti_set_runtime_capabilities_ext` before kernel compilation
  puts all twelve `spirv_has_physical_storage_buffer` consumers on the true
  branch. The write path and the read sites are verified; the composition is
  inference (4.3).
- That item 6.2 divides into two differently-shaped halves along
  `arch_uses_spirv`. Both component censuses are verified; the split is a
  judgement (4.7).
- That no backend publishes hardware-class data anywhere downstream. Each
  backend's census is verified individually; the composed statement across all of
  them is inference (4.8).
- ~~That the C API's unconditional `spirv_has_int64` on OpenGL would mis-declare a
  GLES device. Both code paths and the unreachability of GLES through the C API
  are verified; what would happen were GLES wired up is reasoning (4.2).~~
  **WITHDRAWN in revision 5. The unreachability was false.** GLES is reachable
  through `ti_import_opengl_runtime` and the mis-declaration is live (4.2a). What
  remains inferred is only the composed outcome of the two call orders, which I
  read rather than ran.
- That the switched-off pointer machinery does not reach SNode addressing. The
  five consumer sites and their enclosing functions are verified; the wider claim
  comes from a separate investigation and I have not audited the SNode path,
  which is territory 02 and 03 (4.3).
- That the host assertion at `struct_llvm.cpp:266` and every bitcode variant
  cannot disagree on the ceiling, because both are fixed in one configure. The
  assertion line, the loop and the clang line are verified; the conclusion is
  reasoning about the build, not a build I ran (2.5.2).
- That a host calling `ti_import_opengl_runtime(info, true)` gets a GLES device
  with 64-bit support asserted over it, and that a host calling
  `ti_get_available_archs` first loses the GLES request instead. Every step of
  both chains is verified; the two composed outcomes are reasoning (4.2a).
- That `TI_VISIBLE_DEVICE` overrides the caller's `device_index` on the first
  Vulkan runtime in a process and not on later ones. The singleton, the
  `call_once`, the environment read and the call ordering are verified; the
  composed outcome is reasoning (3.3a).
- That the variant filename is a constraint on the N-variant route, since
  `get_runtime_fn` formats only the arch. The format string is verified; that it
  forecloses a second parameter without a change is reasoning (2.5.1).
- That adding a directory to the `-I` list produces an order-dependent build. The
  two visible headers and the existing `-I` are verified; the ordering outcome is
  reasoning about the compiler.
- That `ti_get_runtime_capabilities` reports zero capabilities for a CUDA runtime.
  Both halves are verified; the composition is inference.
- That the C ABI renumbering would not surface until the checked-in headers are
  regenerated.
- That a Vulkan-built module handed to an OpenGL runtime would be accepted. Each
  of the three absent handshakes is verified; the composed consequence is
  inference.
- That kernels compiled against an old SNode layout would be loaded and run
  against a new one. The validation code and the reader reuse are verified; the
  consequence is reasoning.
- The build outcome of the `taichi/rhi/CMakeLists.txt:17` precedence bug on an
  Android configuration. The text and the CMake precedence rule are verified; I
  did not run the build.

**Not investigated, by scope:** SPIR-V and LLVM code generation internals (agent
02); the runtime struct layer and the LLVM offline cache *format* (agent 03); the
frontend IR and type system (agent 01); the Python front end (brief 1.2).
