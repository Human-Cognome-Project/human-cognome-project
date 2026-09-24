# Adversary 3, 04-2 — round three, backend, AOT and build architecture

Judging `report-04-backend-build.md` (pass A, revision 4) and
`report-04b-backend-build.md` (pass B, framing pass) against source, against
`adversary2-04-1.md` and `adversary2-04-2.md`, and against plan section 5.2 as
corrected on 2026-09-09.

Every file:line below I opened or grepped myself in this round. Every count in
this document is derived from an enumeration I ran and reproduced in place, per
standing instruction 8. Where I confirm a report I say what I ran. Where I
correct one, the correction carries its own citation.

---

## 1. Verdicts up front

| Question | Verdict |
|---|---|
| Is pass A **correct**? | **No.** One defect, section 4. Every source census I re-derived is exact. The defect is a claim about the governing document, not about the tree, and it carries a priority conclusion. |
| Is pass B **correct**? | **No.** The same defect, more widely spread — it is in the table that carries B's central consequence and in four escalations. Plus one live factual error, section 5, and one citation off by one, section 7.1. |
| Is pass A **complete**? | **No.** Section 5 and section 6 are missing from it, and section 4.8 of pass A omits the strongest instance of its own finding (section 7.2). |
| Is pass B **complete**? | **No.** Section 5 and section 6 are missing from it, and B still declines three areas by its own statement at `report-04b:1748-1753`. |
| Is the **pair** complete? | **No.** Two findings are absent from both, sections 5 and 6. Section 5 is a live defect on the path plan sections 2 and 5.3 care most about. |
| **General consensus reached?** | **Not yet.** Section 9 states precisely what remains. It is a correction pass, not a re-run. |

**On the framing correction specifically, which is what I was asked to test: it
did not land.** Both reports inverted the old CUDA-leaning bias into a
SPIR-V-leaning one and attributed the inversion to plan section 5.2. The plan's
current text withdraws that axis by name. Section 4 below.

The work is otherwise serious. I re-derived roughly forty-five censuses and
citations and found no false `[V]` count in either report. The eight claims I was
asked to verify all hold, one of them with a qualification I state in 3.7.

---

## 2. The eight claims, verified

### 2.1 Claim 1 — the clang line already carries a configure-time define. **Upheld exactly.**

`taichi/runtime/llvm/runtime_module/CMakeLists.txt:8`, read whole:

```
COMMAND ${CLANG_EXECUTABLE} ${CLANG_OSX_FLAGS} -c runtime.cpp -o "runtime_${rtm_arch}.bc" -fno-exceptions -emit-llvm -std=c++17 -D "ARCH_${rtm_arch}" -I ${PROJECT_SOURCE_DIR};
```

My own enumeration of the consumption sites, `grep -n "ARCH_"` over the bitcode
translation unit and the two headers it includes:

| file | lines | count |
|---|---|---|
| `runtime.cpp` | 51, 119, 155, 347, 771, 798, 801, 860, 1144, 1156, 1167, 1293, 1343, 1431, 1549, 1633, 1651, 1861, 1865 | 19 |
| `locked_task.h` | 7 (`#if ARCH_x64 \|\| ARCH_arm64`) | 1 |
| `node_pointer.h` | 15 (`#if defined(ARCH_cuda)`) | 1 |
| | | **21** |

Nineteen plus one plus one is twenty-one. Both reports state 21 and both list the
same nineteen line numbers. Exact.

The rest of the chain, each step opened: values decided at configure time
(`cmake/TaichiCXXFlags.cmake:149` for `HOST_ARCH`; root `CMakeLists.txt:155, 159,
163` for `CUDA_ARCH`, `AMDGPU_ARCH`, `DX12_ARCH`); iterated at
`runtime_module/CMakeLists.txt:29-31`; one artefact per value,
`runtime_${rtm_arch}.bc` at `:8`; installed at `:13`; selected at run time by
filename at `taichi/runtime/llvm/llvm_context.cpp:209-211`. Upheld.

The claim that this falsified one report's central escalation is also upheld:
pass B's first escalation said "Nothing in the tree yet compiles a configure-time
value *into* the bitcode", and B has withdrawn it in terms at
`report-04b:1222-1229`.

### 2.2 Claim 2 — the runtime filename encodes the architecture and nothing else. **Upheld.**

`taichi/runtime/llvm/llvm_context.cpp:209-211`, read directly:

```
std::string get_runtime_fn(Arch arch) {
  return fmt::format("runtime_{}.bc", arch_name(arch));
}
```

One format argument, `arch_name(arch)`. So a second parameter either folds into
that name or fixes one ceiling per build, as both reports state.

Plan section 5.1a settles which applies: "Per build already means per device
configuration", and "The filename scheme does not need to grow". Neither report
records that as settled — both still carry it as open, pass A at escalation 16
(`report-04:1433-1445`) and pass B at escalations 4 and 17
(`report-04b:1798-1805, 1931-1943`). That is not an error; the plan section
postdates the amendment brief they answered and no agent may enter a plan
decision itself. I record it so the planner can close those three escalations
rather than re-dispatch them. Both reports' underlying mechanism statements are
compatible with 5.1a and need no change.

### 2.3 Claim 3 — the host never lays out the runtime struct. **Upheld exactly.**

`grep -rn "struct LLVMRuntime" taichi/ c_api/` returns five lines and no more:

| site | form |
|---|---|
| `taichi/rhi/llvm/llvm_device.h:8` | `struct LLVMRuntime;` — forward declaration |
| `taichi/program/context.h:11` | `struct LLVMRuntime;` — forward declaration |
| `taichi/runtime/llvm/runtime_module/runtime.cpp:136` | forward declaration inside the bitcode unit |
| `taichi/runtime/llvm/runtime_module/runtime.cpp:337` | forward declaration inside the bitcode unit |
| `taichi/runtime/llvm/runtime_module/runtime.cpp:552` | `struct LLVMRuntime {` — the only definition |

Two host declarations, three lines inside the bitcode translation unit, one
definition, and it is in the bitcode. I read `runtime.cpp:552-572` and confirmed
the three arrays sized by `taichi_max_num_snodes` at `:567-569`. The host-versus-
device layout disagreement both reports originally described cannot happen.
Upheld, and both reports have withdrawn it correctly — pass A at
`report-04:201-218`, pass B at `report-04b:1136-1173`.

### 2.4 Claim 4 — the disabled 64-bit branch is not permanent. **Upheld exactly.**

`ti_set_runtime_capabilities_ext` read whole at
`c_api/src/taichi_core_impl.cpp:317-334`. It builds a `DeviceCapabilityConfig`
from caller-supplied values in the loop at `:326-330`, casting the raw
`uint32` straight to `DeviceCapability` at `:329`, and installs it wholesale at
`:331` with `runtime2->get().set_caps(std::move(devcaps))`. There is no
comparison against the device anywhere in the function.

My own census of `spirv_has_physical_storage_buffer` over `taichi/` and `c_api/`
including `.mm` and `.hpp` returns sixteen lines, which decompose as:

| kind | sites | count |
|---|---|---|
| declaration | `taichi/inc/rhi_constants.inc.h:28` | 1 |
| producers, both compiled out | `taichi/rhi/vulkan/vulkan_device_creator.cpp:826`; `c_api/src/taichi_vulkan_impl.cpp:48` | 2 |
| typed setter in the header-only wrapper | `c_api/include/taichi/cpp/taichi.hpp:1156` | 1 |
| live consumers | `vulkan_device.cpp:1772, 1792, 2147, 2509`; `spirv_codegen.cpp:783, 2340, 2416, 2491`; `spirv_ir_builder.cpp:73, 113`; `runtime/gfx/runtime.cpp:96`; `gfx_program.h:81` | **12** |

Four plus four plus two plus one plus one is twelve. Both reports state twelve
and both list the same twelve. Exact.

I read the guard at `vulkan_device_creator.cpp:820-828` directly: `:821`
`if (device_supported_features.shaderInt64) {`, `:822-823` the issue-6295
comment, `:824` the author's note, `:825` `#if !defined(__APPLE__) && false`,
`:826` the set, `:827` `#endif`. Every line number in both reports is right.

### 2.5 Claim 5 — both spines detect tier data and neither publishes it. **Upheld, with one omission in pass A.**

Verified site by site:

- **`memoryHeaps` / `heapSize` appear nowhere in `taichi/rhi/vulkan/`.** My grep
  returns zero lines. The one `vkGetPhysicalDeviceMemoryProperties` call outside
  the VMA function-pointer table is `vulkan_device.cpp:2516`, and the loop that
  follows reads `memoryTypeCount` and `memoryTypes[i].propertyFlags` only.
  Upheld.
- **Two consumers of the stored device properties.** My grep for
  `vk_device_properties_` and `get_vk_physical_device_props` over `taichi/` and
  `c_api/` returns five lines: the accessor at `vulkan_device.h:710-711`, the
  member at `:739`, the fill at `vulkan_device.cpp:1596`, and exactly two reads —
  `:1125` through the accessor, and `:1945` direct. Upheld.
- **Device name to debug logs.** `vulkan_device_creator.cpp:432-440` enumerates
  every physical device and writes `properties.deviceName` through
  `RHI_DEBUG_SNPRINTF` into `RHI_LOG_DEBUG`. Read directly. Upheld.
- **OpenGL writes two file-scope globals that nothing reads.** My grep for
  `opengl_max_block_dim` and `opengl_max_grid_dim` over `taichi/`, `c_api/` and
  `python/` returns exactly six lines: the two definitions at
  `opengl_api.cpp:20-21`, the two `glGetIntegeri_v` calls at `:231` and `:234`,
  and the two `TI_TRACE` lines at `:233` and `:236`. No header declares them.
  Upheld, and pass A is right that this is stronger than "detected and not
  published": on OpenGL the values are detected and not used at all.

**The omission.** Pass A's section 4.8 is the section that composes the finding,
and it does not have the strongest Vulkan instance. `get_device_score`
(`taichi/rhi/vulkan/vulkan_device_creator.cpp:203-229`) reads
`properties.deviceType` and adds 500 for
`VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU` at `:220-222` and 1000 for
`VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU` at `:223-225`, plus 100 per API minor
version at `:226`. It is called at `:452` and `:462`, and the score is a local
that dies at the call site. That is a hardware-class judgement, made and
discarded, and it is the closest thing on that backend to what section 4.8 says
does not exist. Pass B has it (`report-04b:556-563`); pass A does not, anywhere.
Pair-complete, A-incomplete.

One detail neither report has, recorded because it bears on plan section 1.3:
`get_device_score` also adds 100 for `features.wideLines`
(`vulkan_device_creator.cpp:219`), a rasterisation feature, when scoring devices
for a compute workload. Minor; recording, not proposing.

### 2.6 Claim 6 — Metal is the one complete chain, and publishes admission. **Upheld exactly.**

`collect_metal_device_caps` read whole at
`taichi/rhi/metal/metal_device.mm:1017-1070`. The ladder at `:1027-1036` queries
`[mtl_device supportsFamily:]` for `MTLGPUFamilyMac2` and Apple families 3
through 7, each higher family OR-ed into the lower ones so the ladder is
monotone. It reduces to five named features at `:1038-1042`. It writes
`spirv_version` and four capabilities unconditionally at `:1044-1049`,
`spirv_has_int64` under `feature_64_bit_integer_math` at `:1051-1053`, two more
under the permute features at `:1063-1064`, and one under the reduction feature
at `:1067`. Installed at `:1168` inside `MetalDevice::MetalDevice` (`:1162-1169`).
Read back at `:131-135`, `caps.contains(DeviceCapability::spirv_has_int64)`
selecting MSL 2.3.

**The caveat is right and it is the whole of plan section 2.2.** Below Apple3
there is no `spirv_has_int64`, so that hardware cannot do a thing other hardware
can. That is feature admission, which section 2.2 calls a violation rather than
an optimisation.

**Corrected after reading `adversary3-04-1.md`.** I first wrote here that both
reports state the distinction. That is wrong as to pass B. Pass A carries it at
`report-04:992-1000` ("the **mechanism** is the model; the **content** it
currently carries is the pattern section 2.2 warns about"). Pass B's section 2.8
(`report-04b:582-601`) presents Metal as the model, says it "writes **only**
portable vocabulary", and nowhere records that what it writes is admission or
that below Apple3 a device loses `spirv_has_int64` outright. Pass B does note the
`feature_64_bit_integer_math` gate in its 2.3 table row, but not the section 2.2
consequence. So only one half of the pair tests the finding plan section 6.3 now
records. Adversary 3 pass 1 found this and I did not. See section 12.4.

I checked the vocabulary claim underneath, because the plan's own section 6.3
finding rests on it. `taichi/inc/rhi_constants.inc.h:10-34` is 25 entries,
`reserved` plus 24 `spirv_*`: one version number, twenty-three feature bits.
Nothing in it can say "faster" or "finer". The finding recorded in plan section
6.3 holds against my own reading of the file.

Worth adding, because it sharpens rather than softens that finding: the file's
own header comment at `:1-7` invites `CUDA compute capability` and
`DirectX shader model` as entries. Both are admission-shaped and one is
vendor-specific, so the invitation points at exactly the shape section 2.2
warns against. Pass B cites the comment (escalation 8, `report-04b:1842-1844`);
pass A does not have it.

### 2.7 Claim 7 — multi-device selection exists only on Vulkan. **Upheld. The amendment agent's flag was half right.**

I read `ti_create_runtime` whole at `c_api/src/taichi_core_impl.cpp:247-309`. My
own enumeration of the switch:

| case | line | device_index |
|---|---|---|
| `TI_ARCH_VULKAN` | 253 | honoured — `set_vulkan_visible_device(std::to_string(device_index))` at `:254-255` |
| `TI_ARCH_OPENGL` | 269 | rejected at `:270` |
| `TI_ARCH_X64` | 275 | rejected at `:277` |
| `TI_ARCH_ARM64` | 282 | rejected at `:283` |
| `TI_ARCH_CUDA` | 288 | rejected at `:289` |
| `TI_ARCH_METAL` | 295 | rejected at `:297` |
| `default` | 302 | `TI_CAPI_NOT_SUPPORTED(arch)` |

One honoured, five rejected. Both reports' line numbers are exact. CUDA
additionally pins device 0 in its own driver layer,
`taichi/rhi/cuda/cuda_context.cpp:21-22`.

I traced the Vulkan path rather than taking it on the call: `set_vulkan_visible_device`
writes `VulkanLoader::instance().visible_device_id`
(`taichi/rhi/vulkan/vulkan_loader.cpp:144-146`), and
`vulkan_device_creator.cpp:442-459` reads it, `std::stoi`s it, bounds-checks it
against `device_count`, and takes `devices[id]` if `get_device_score` passes,
falling back at `:458-468` to the highest-scoring device. It is a real selector.

**The judgement I was asked for.** The amendment agent's flag was **half right**.

Right that the direction is reversed. Every other finding in this territory has
the SPIR-V spine better covered on capabilities; this one has it better covered
on device selection, and noticing that is the opposite of applying a rule
mechanically.

Wrong in what it concluded. Pass B's amendment note at `report-04b:1653-1654`
reads "on device selection the portable path is the one better served, so there
is no gap in it to record." Plan section 5.2 says: "Where one spine is better
served than the other, that is a gap in the less-served spine to record, never
evidence that it matters less." So a gap exists and must be recorded — on the
LLVM spine, where CUDA and AMDGPU sit. Concluding "no gap to record" only
follows if the rule is read as protecting one named path, which is the axis the
correction withdraws. B reached the right observation through the wrong rule and
then dropped the recordable half.

Pass A applies the rule the other way and over-reaches. `report-04:442-445` and
escalation 12 (`:1403-1404`) say the gap "covers four archs including CPU" and
that four archs "cannot express a second device at all". For `x64` and `arm64`
there is no second host CPU to express; `host_arch()`
(`taichi/rhi/arch.cpp:68-76`) returns one arch and the CPU path has no device
index. Including CPU in the gap manufactures symmetry. The substantive gap is
CUDA, which pins device 0 in its own driver layer, plus AMDGPU and DX12, which
have no C API case at all.

Neither report has the sharper fact, which is section 6 below.

### 2.8 Claim 8 — the ordering census of thirteen, and the CMake precedence condition. **Both upheld exactly.**

My own enumeration. I grepped `spirv_version` across `taichi/` and `c_api/`
including `.mm`, then kept only ordered comparisons:

| operator | sites | count |
|---|---|---|
| `<` | `spirv_ir_builder.cpp:556, 625, 671, 679, 741, 777`; `vulkan_device_creator.cpp:577` | 7 |
| `>` | `spirv_codegen.cpp:1908` | 1 |
| `>=` | `spirv_ir_builder.h:424`; `spirv_codegen.cpp:2669, 2671, 2673, 2675` | 5 |
| | | **13** |

Seven plus one plus five is thirteen. Both reports state thirteen and both list
the same thirteen.

I also tested B's stronger claim that `spirv_version` is the only capability ever
compared with an ordered operator. My grep for any `DeviceCapability::` reference
adjacent to `<`, `>`, `<=` or `>=` excluding `spirv_version` returns nothing.
B's claim holds.

I read `spirv_codegen.cpp:2661-2681` whole. `spirv_version` is read into a local
at `:2666`; the ladder is `:2669-2679`; the optimizer is constructed at `:2681`.
The four `>=` rungs select `spv_target_env` for the whole module. Both reports
are right that this is the strongest instance in the tree.

**The CMake precedence condition.** `taichi/rhi/CMakeLists.txt:17` read directly:

```
if (TI_WITH_OPENGL OR TI_WITH_VULKAN AND NOT ANDROID)
```

CMake binds `NOT` tighter than `AND`, and `AND` tighter than `OR`, so this is
`TI_WITH_OPENGL OR (TI_WITH_VULKAN AND (NOT ANDROID))`. The guarded body runs
`:18-35` and includes `target_compile_definitions` at `:28`, `add_subdirectory`
of GLFW at `:29`, `target_link_libraries` at `:30` and
`target_include_directories` at `:31-34`. The `NOT ANDROID` guard does not reach
the OpenGL arm. Both reports exact, and both correctly mark the build outcome as
reasoning rather than executed.

---

## 3. Other censuses I re-derived and could not break

Recorded so the planner can see which figures survived a third independent run.

- `taichi/rhi/dummy.cpp` is 0 bytes (`wc -c`). `ti_device_api_shared` appears at
  exactly `taichi/rhi/CMakeLists.txt:114, 115, 120` and nowhere else outside
  `modernization/`. Nothing links or installs it.
- `DynamicLoader` construction sites, my own enumeration: `cuda_driver.cpp:34,
  89, 98, 107` (4), `amdgpu_driver.cpp:33` (1), `vulkan_loader.cpp:98` (1).
  Six, every one a vendor library. Nothing includes
  `taichi/system/dynamic_loader.h` anywhere in the tree. Pass A's summary point
  1 is now cited and the citation is right.
- `set_caps`: nine grep hits, one declaration at `public_device.h:855` and eight
  call sites — `metal_device.mm:1168`, `vulkan_device.cpp:1580`,
  `vulkan_device_creator.cpp:876`, `dx_device.cpp:565`, `opengl_device.cpp:529`
  (five RHI), `taichi_opengl_impl.cpp:12`, `taichi_core_impl.cpp:331`,
  `taichi_vulkan_impl.cpp:53` (three C API). Five plus three is eight. Both
  reports' tables match row for row.
- `taichi/aot/` is seven files, 836 lines, by `wc -l`: 146, 183, 68, 81, 139, 94,
  125. Sums to 836.
- 25 `PER_DEVICE_CAPABILITY` entries at `rhi_constants.inc.h:10-34`, which is
  25 lines inclusive.
- `ti_get_available_archs` (`taichi_core_impl.cpp:171-205`) probes six archs, in
  order vulkan, metal, cuda, x64, arm64, opengl, caching into a `thread_local`
  vector at `:176`. No gles, dx11, dx12 or amdgpu probe.

**One count both round-two adversaries got wrong against their own lists.**
`is_extension_supported` has nine call sites, by my enumeration:
`program.cpp:149`, `codegen_llvm.cpp:2726`, `compile_to_offloads.cpp:92, 205,
218, 236, 245, 288`, `export_lang.cpp:1225` — one plus one plus six plus one.
`adversary2-04-1.md:39` says "six call sites" and then lists nine.
`adversary2-04-2.md:39` says "8 call sites" and then lists nine. Pass A
(`report-04:517-522`) lists the nine and states no total, which is right.
Recording it because both adversary files are part of the record a reader may
carry a number out of, and because it is the exact failure standing instruction 8
names.

---

## 4. The framing correction did not land. Both reports.

This is what I was asked to test specifically, and it is the reason neither
report is correct as it stands.

### 4.1 What the plan now says

Plan section 5.2, `PROJECT-PLAN.md:414-431`, corrected 2026-09-09:

> An earlier version of this section opposed "the portable path" to "a
> vendor-specific path". That is the wrong axis, it was mine, and an agent caught
> it. There are two compilation spines and **neither one is the vendor spine**.

and

> Vendor lock is a property of individual backends, not of a spine. CUDA is
> vendor locked. The LLVM spine is not, because CPU-only operation runs through
> it. Treating the LLVM spine as "the vendor half" would deprioritise the no-GPU
> target, which section 2 puts first.

I grepped the whole plan for "reference": it occurs twice, at `:82` and `:497`,
in unrelated sentences. **The plan does not contain the phrase "reference path"
and never has in its current text.**

### 4.2 What both reports say instead

Pass A, `report-04:714-718`, in revision-4 framing text:

> That inverted plan section 5.2: `{opengl, gles, vulkan, dx11, metal}` is the
> **reference** path, and the LLVM spine is where the incidental test hardware
> happens to sit.

and `report-04:875`, heading a bullet in section 4.7:

> **On the SPIR-V spine — the reference path under plan section 5.2** —

and `report-04:1292`, closing section 7:

> that spine is the reference path, so this axis is the primary existing form of
> 6.2 in this territory

Pass B, `report-04b:42`, in the framing-pass header:

> section 2 governs and the reference path is the portable one

and `report-04b:359-360`:

> These backends are the reference path, so **machinery written and disabled on
> them is the primary existing form of item 6.2**, and the vendor path is the
> accelerant.

and the table row that carries B's central consequence, `report-04b:501`:

> | `arch_uses_llvm`, the accelerant plus the CPU-only baseline |

B additionally uses "the vendor path" or "the vendor accelerant" as a live axis
at `:417`, `:528`, `:1893` and `:1958`, and titles escalation 10 at `:1878`
"Device selection exists on the portable path and not on the vendor one."

### 4.3 Why this is a defect and not a wording preference

Three reasons, in ascending order of weight.

1. **It is attributed to the plan and the plan does not say it.** Pass A's
   `:875` says "the reference path under plan section 5.2". That is a citation of
   the governing document for a proposition the document does not contain. Under
   plan rule 0.1 nothing is entered on inference or extrapolation, and standing
   instruction 6 says a verified citation does not verify the claim attached to
   it. This is the same class of error, one level up: a cited section that does
   not support the sentence citing it.

2. **It privileges a spine, which the correction exists to forbid.** The
   correction's operative sentence is that no backend may be privileged such that
   capability differs across targets, and that where one spine is better served
   the shortfall is a gap in the other **to record, never evidence that it matters
   less**. Naming one spine "the reference" and the other "the accelerant" is
   exactly a statement that one matters less. B's `:501` table row calls the spine
   carrying `x64` and `arm64` "the accelerant plus the CPU-only baseline", which
   demotes the first-class target of plan section 2 to a rider on a vendor
   accelerant.

3. **It carries a priority conclusion into the escalations.** Both reports
   conclude from it that the switched-off SPIR-V pointer machinery is "the
   primary existing form of item 6.2" — pass A at `:716-718` and `:1292`, pass B
   at `:359-360`, `:515-519` and escalation 9 at `:1868-1876`. That is a ranking
   of work, derived from a premise the plan has withdrawn, sitting in the
   escalations the planner will read as open questions.

### 4.4 Where it came from, so it is not re-derived

`adversary2-04-1.md:748-755` opens its addendum with: "the reference path is the
portable one and a vendor path is an optional accelerant". That paragraph is a
paraphrase of the **intermediate** wording of section 5.2, the one the 2026-09-09
correction withdraws by name. Its §9.2 table then instructs both reports to
restate six statements on that basis, and both amendment passes carried it out —
pass A's section 9.7 table (`report-04:1596-1606`) and pass B's section 8.1c
(`report-04b:1635-1672`) both cite that table as their input.

So this is not two agents independently drifting. It is one withdrawn premise
propagated through one adversary file into two amendment passes. The reports did
the work they were asked to do; the instruction was stale by the time they did
it.

### 4.5 One internal contradiction this produced, in pass A

Both of these are revision-4 framing text in the same report.

`report-04:716-717`: "the LLVM spine is where the incidental test hardware
happens to sit."

`report-04:890-893`: "The LLVM spine is not 'the vendor path': it carries
`Arch::x64` and `Arch::arm64`, which is CPU-only operation, a first-class target
under plan section 2."

The second is right and matches the corrected plan. The first is the withdrawn
axis. They cannot both stand. Section 4.7 of pass A is the sentence to keep.

### 4.6 What the reports should say instead, stated as facts rather than ranking

Nothing factual changes. The two censuses both reports carry are correct and
neither is withdrawn:

- The capability system describes `arch_uses_spirv` and does not describe
  `arch_uses_llvm` (`taichi/rhi/arch.cpp:54-57, 63-66`). That is a gap on the
  LLVM spine, which carries CPU-only operation, a first-class target.
- Written-and-disabled 64-bit pointer machinery exists on the SPIR-V spine and
  nothing equivalent exists on the LLVM spine. That is a difference in the state
  of the two halves of item 6.2, not a ranking of them.

Both are recordable under the corrected rule without either spine being called
the reference. Pass B's own section 2.7 body (`report-04b:503-513`) already
argues it that way and is the model; it is the "Ordering" paragraph appended at
`:515-523` that reintroduces the ranking.

---

## 5. Newly wrong, and it is live: GLES **is** reachable through the C API

Both reports, both round-two adversaries, and pass A's escalation 20 all rest on
a scope caveat that is false. This survived two adversarial rounds and one
amendment pass in each report.

### 5.1 What the reports claim

Pass A, `report-04:613-618`:

> Scope caveat, checked before overstating: `ti_create_runtime` has no
> `TI_ARCH_GLES` case ... so a GLES context is not reachable through this path
> today.

and `report-04:636-637`: "Today that mis-declaration cannot fire, because the
only reachable context is desktop GL"; and escalation 20 at `:1493-1496`: "The
over-declaration cannot fire today because GLES is unreachable through the C
API".

Pass B, `report-04b:631-635`: "There is also no `TI_ARCH_GLES` case in
`ti_create_runtime` ... so the C API cannot ask for GLES directly. The defect is
therefore latent rather than always live, and what makes it latent is an accident
of ordering rather than a check."

`adversary2-04-1.md:875-884` states it as its own finding 9.3.3, "GLES is named
everywhere and reachable nowhere through the C API".
`adversary2-04-2.md:455-457` endorses pass A's caveat as "correct and correctly
limits the claim".

### 5.2 The entry point that falsifies it

`ti_import_opengl_runtime` is a public, exported C API function taking a
`use_gles` parameter. Declared at `c_api/include/taichi/taichi_opengl.h:33-36`:

```
// Function `ti_import_opengl_runtime`
TI_DLL_EXPORT TiRuntime TI_API_CALL
ti_import_opengl_runtime(TiOpenglRuntimeInteropInfo *interop_info,
                         bool use_gles);
```

Its body is `c_api/src/taichi_opengl_impl.cpp:21-29`. I read the whole file.

```
  taichi::lang::opengl::imported_process_address = interop_info->get_proc_addr;
  taichi::lang::opengl::set_gles_override(use_gles);          // :26
  TI_CAPI_TRY_CATCH_END();
  return ti_create_runtime(TI_ARCH_OPENGL, 0);                // :28
}
```

The chain, every step opened:

| step | site |
|---|---|
| public entry, `use_gles` parameter | `c_api/include/taichi/taichi_opengl.h:33-36` |
| override set | `c_api/src/taichi_opengl_impl.cpp:26` → `set_gles_override` at `taichi/rhi/opengl/opengl_api.cpp:35-37`, writing the file-static `use_gles_override` declared at `:31-33` |
| runtime created | `taichi_opengl_impl.cpp:28`, `ti_create_runtime(TI_ARCH_OPENGL, 0)` → `new OpenglRuntime` (`taichi_core_impl.cpp:271`) |
| its `GLDevice` member constructed | `taichi_opengl_impl.h:8-14`; `GLDevice::GLDevice` at `opengl_device.cpp:506`, calling `initialize_opengl(false, true)` at `:507` |
| **the override replaces the argument** | `opengl_api.cpp:56-59`, `if (use_gles_override.has_value()) { use_gles = use_gles_override.value(); unset_gles_override(); }` |
| `kUseGles` set true | `opengl_api.cpp:239`, `kUseGles = use_gles;` |
| detection correctly withholds 64-bit | `opengl_device.cpp:509-513`, `if (!is_gles())`, comment at `:510` "64bit isn't supported in ES profile" |
| the C API asserts it anyway | `taichi_opengl_impl.cpp:8-12` builds a fresh three-entry config and installs it wholesale, `spirv_has_int64` at `:9`, `spirv_has_float64` at `:10` |

So a host calling `ti_import_opengl_runtime(info, true)` gets a runtime whose
device was detected as GLES, correctly withheld 64-bit integer and float support,
and then had `spirv_has_int64` and `spirv_has_float64` asserted over the top,
with the detected `spirv_has_int16` and `spirv_has_float16`
(`opengl_device.cpp:515-526`) dropped in the same replacement.

**The mis-declaration is live through a documented public entry point.** It is
not latent, it does not depend on an accident of ordering, and the C API can ask
for GLES directly.

### 5.3 The second half of the same defect

The override is consumed at `opengl_api.cpp:56-59`, which sits **after** the
early return at `:46-54`. If `initialize_opengl` has already run in the process,
`supported` has a value, the function returns before `:56`, and
`use_gles_override` is never read or cleared.

`ti_get_available_archs` (`taichi_core_impl.cpp:171-205`) probes OpenGL at `:193`
through `is_opengl_available` (`:21-27`), which calls
`is_opengl_api_available()` with its default argument. The default is
`use_gles = false` (`taichi/rhi/opengl/opengl_api.h:13`), and
`is_opengl_api_available` forwards straight to `initialize_opengl(use_gles, true)`
(`opengl_api.cpp:243-245`).

So a host that enumerates archs before importing a runtime — the natural
install-time-loader sequence in plan section 5 — locks `kUseGles = false`, and
its later `ti_import_opengl_runtime(info, true)` **silently loses its GLES
request** and runs desktop GL.

Either way one of two documented behaviours is wrong, and which one fires depends
on whether an unrelated API call was made earlier in the process.

`reset_opengl` (`opengl_api.cpp:252-...`) clears both `supported` and `kUseGles`,
and its only caller in the tree is
`taichi/runtime/program_impls/opengl/opengl_program.cpp:29`. Neither report
mentions it.

### 5.4 Why this matters against the plan

Plan sections 2 and 5.3 put edge hardware first and make modern capability an
opportunity rather than a prerequisite. GLES is the profile the low end presents.
The detection that exists at `opengl_device.cpp:509-513` is precisely the
protection those sections ask for, its comment says so, and the C boundary
discards it on a path a host reaches by calling a documented function with a
documented argument.

It also engages plan section 2.2 directly and in the harder direction. The RHI
correctly declines to claim a capability the hardware lacks; the C API then
claims it. A kernel compiled against that claim on a device without 64-bit
integer support is hardware being told it can do something it cannot.

**[V]** for every line in the table at 5.2 and for the early return and default
argument at 5.3. **[I]** only for the composed consequence, that a host taking
either sequence gets one of the two wrong outcomes — I read the paths, I did not
run them.

Both reports must withdraw the caveat. Pass A's escalation 20 and pass B's
escalation 20 both need restating: the question is no longer "close the assertion,
wire up GLES, or both", because GLES is wired up. It is what the C API should do
when its own device has just detected the opposite of what the C API asserts.

---

## 6. Still missing from both: the one working device selector is process-global and environment-overridable

Neither report, neither round-two adversary, and neither notes file contains the
strings `TI_VISIBLE_DEVICE`, `visible_device_id` or `call_once`. I grepped all
files in `modernization/investigation/`. This is new.

Claim 7 establishes that Vulkan is the only arch honouring `device_index`. That
selector has two properties neither report records.

**It is process-global, not per-runtime.** `set_vulkan_visible_device`
(`taichi/rhi/vulkan/vulkan_loader.cpp:144-146`) writes
`VulkanLoader::instance().visible_device_id`. `VulkanLoader::instance()` is a
function-local static (`vulkan_loader.h:14-17`) and `visible_device_id` is a
plain member at `:32`. Every runtime in the process shares one selector. Serial
creation works because the value is consumed during construction, at
`vulkan_device_creator.cpp:442`.

**The environment can override the API argument, order-dependently.**
`VulkanLoader::init` (`vulkan_loader.cpp:86-117`) wraps its body in
`std::call_once` at `:87` and, at `:111-114`, reads `TI_VISIBLE_DEVICE` from the
environment and calls `set_vulkan_visible_device(id)` if it is set.
`VulkanDeviceCreator::VulkanDeviceCreator` calls `VulkanLoader::instance().init()`
at `vulkan_device_creator.cpp:236`, which is **after**
`ti_create_runtime` has already written the caller's `device_index` at
`taichi_core_impl.cpp:254-255`.

So on the first Vulkan runtime created in a process, if `TI_VISIBLE_DEVICE` is
set in the environment, the `call_once` body overwrites the `device_index` the
caller passed. On the second and later runtimes the `call_once` has already
fired, so the caller's argument stands. Which of the two wins depends on whether
`init()` had run earlier — for instance through `is_vulkan_api_available()`,
reached from `ti_get_available_archs` at `taichi_core_impl.cpp:179`.

**[V]** for the singleton, the `call_once`, the environment read and the call
ordering. **[I]** for the composed outcome, which I read rather than ran.

**Scope, stated so this is not misread.** Plan section 5.1a records that
concurrent operation across two cards is confirmed empirically by the project
owner, that this outranks any code-reading conclusion, and that no agent needs to
investigate it. **I am not investigating or contradicting that.** This finding is
about the `device_index` parameter of one C API function being silently
overridable, which is a property of the selector and not a claim about
concurrency. It bears on plan section 5.1a only in that 5.1a makes the two-card
case ordinary rather than exceptional, so the one API that can address a second
card is worth having correct.

---

## 7. Smaller corrections

None of these inverts a finding.

### 7.1 Pass B: the optimizer line is off by one, twice

`report-04b:833` says the `spv_target_env` value "configures the optimizer for
the whole module at `:2680`", and the section 6 map at `:1461-1462` says
"feeding the optimizer at `:2680`". I read `spirv_codegen.cpp:2660-2681`:
`:2666` reads `spirv_version`, `:2669-2679` is the ladder, `:2680` is blank, and
`:2681` is `spirv_opt_ = std::make_unique<spvtools::Optimizer>(target_env);`.
Pass A has `:2681` (`report-04:778`) and is right.
`adversary2-04-2.md:144` says `:2682` and is also wrong.

### 7.2 Pass A: section 4.8 is missing `get_device_score`

Covered at 2.5 above. Pass A's section 4.8 composes the statement that every
backend able to ask its hardware about class does ask, and omits the one Vulkan
site that makes an explicit class judgement. Pass B has it. This is a gap in A's
own new section, not in the pair.

### 7.3 Pass B: `kUseGles` has two writers, not one

`report-04b:627` says "`kUseGles` is set once at `:239`". There is a second write
at `opengl_api.cpp:254`, inside `reset_opengl`, whose caller is
`taichi/runtime/program_impls/opengl/opengl_program.cpp:29`. The in-file comment
at `opengl_api.cpp:23` says "set at most once in initialize_opengl below", which
is true of `initialize_opengl` and not of the variable. Minor, but it is part of
the ordering argument B builds escalation 20 on, and section 5 above supersedes
that argument anyway.

### 7.4 Both reports carry escalations plan section 5.1a has already settled

Covered at 2.2. Pass A escalation 16, pass B escalations 4 and 17. Not an error
by either agent; recording it so the planner closes them rather than dispatching
them again.

---

## 8. What I tried to break and could not

Beyond section 3: the two `required_caps` structs and which is live; the
`get_required_caps` chain broken between deserialisation and use; the absence of
any `arch()` or `version()` caller; `taichi_max_num_snodes` at exactly three
places, none of them in `taichi/rhi/`, `taichi/aot/`, `c_api/` or `cmake/`; the
`CUDA_VERSION` chain end to end; the `-I ${PROJECT_SOURCE_DIR}` being the clang
line's only include path; the `WORKING_DIRECTORY` at
`runtime_module/CMakeLists.txt:10` and the `${arch}` versus `${rtm_arch}` slip at
`:13`; `TI_WITH_CPU` referenced twice and defined nowhere; the positional
capability numbering at `misc/taichi_json.py:115` and its second consumer at
`offline_cache_util.cpp:92`. Every one lands exactly as both reports state.

---

## 9. What remains for consensus

Five items. Two are corrections both reports must make, two are absorptions, one
is for the planner.

1. **Both reports: withdraw "the reference path" and "the vendor path" as an
   axis, and the ranking derived from it.** Pass A at `:714-721`, `:875`,
   `:1292`, `:1409` and the correction-log row at `:1601`; pass B at `:42`, `:48`, `:359-360`, `:417`, `:500-501`,
   `:515-523`, `:528-530`, `:640`, `:1653-1656`, `:1868-1876`, `:1878`,
   `:1893`, `:1953-1959`, `:1989`. Pass A must additionally resolve the
   contradiction between its `:716-717` and its `:890-893` in favour of the
   latter. Neither report loses a fact by this; both lose a ranking neither is
   entitled to make. Section 4.

2. **Both reports: withdraw the GLES scope caveat and restate escalation 20.**
   Pass A at `:613-618`, `:636-637` and `:1493-1496`; pass B at `:623-635` and
   `:1983-1987`. `ti_import_opengl_runtime`
   (`c_api/include/taichi/taichi_opengl.h:33-36`,
   `c_api/src/taichi_opengl_impl.cpp:21-29`) takes a `use_gles` parameter and
   sets the override at `:26`. The defect is live. Section 5.

3. **Both reports: absorb section 6**, the process-global and
   environment-overridable Vulkan device selector, into whichever of pass A's
   escalation 12 or pass B's escalation 10 they keep.

4. **Pass A: add `get_device_score`** (`vulkan_device_creator.cpp:203-229`,
   called `:452, :462`) to section 4.8, which currently composes a statement its
   own census does not fully support. Section 2.5.

5. **Pass B: correct the optimizer line** from `:2680` to `:2681` in two places,
   and the `kUseGles` writer count. Sections 7.1 and 7.3.

Items 4 and 5 are edits. Items 1 and 2 are a withdrawal and a restatement each,
against lines both reports already cite. Item 3 is one new paragraph. None of
this is a re-run.

**For the planner, not for the reports.** Plan section 5.1a settles what pass A's
escalation 16 and pass B's escalations 4 and 17 ask. They can be closed rather
than re-dispatched, and no agent may close them.

---

## 10. Escalations

Unresolved, and not decided here. I am not carrying forward the escalations
already raised by pass A (20) and pass B (20) or by the two round-two
adversaries; those stand except where sections 4, 5 and 9 above say otherwise.

1. **What should the C API do when its own device has detected the opposite of
   what the C API asserts?** Per section 5, `OpenglRuntime::OpenglRuntime`
   (`c_api/src/taichi_opengl_impl.cpp:4-13`) replaces the whole capability config
   that `GLDevice::GLDevice` (`opengl_device.cpp:506-529`) has just detected, and
   the GLES path that makes the replacement wrong is reachable through a
   documented public entry point. Three shapes are available and the code does not
   say which is intended: stop replacing and merge instead; keep replacing because
   an interop host is presumed to know its own context; or validate the
   replacement against the device. This is the sharp form of pass A's escalation
   20 and pass B's escalation 20, and it now also touches
   `ti_set_runtime_capabilities_ext` (pass A's escalation 11), which does the same
   thing wholesale with no device.

2. **Should the `device_index` argument outrank `TI_VISIBLE_DEVICE`, and should
   the selector be per-runtime?** Per section 6, the environment wins on the first
   Vulkan runtime in a process and the argument wins thereafter, and the state is
   a member of a process-wide singleton. Plan section 5.1a makes the two-card case
   ordinary. Which of the two inputs is authoritative is not recorded anywhere,
   and I am not choosing.

3. **How is a fidelity class to be expressed, given a vocabulary that cannot
   hold one?** Recorded in plan section 6.3 as a finding and confirmed against
   source at 2.6 above: all 25 entries at `rhi_constants.inc.h:10-34` are one
   version number and twenty-three feature bits. Metal's chain
   (`metal_device.mm:1017-1069, 1168, 131-135`) is the only complete
   detect-publish-consume path and what it publishes is admission. The file's own
   header comment at `:1-7` invites CUDA compute capability and DirectX shader
   model, both admission-shaped and one vendor-specific. So the one invitation the
   source offers points at the shape plan section 2.2 forbids. Whether a fidelity
   level belongs in that enum, in a parallel structure, or outside the RHI is a
   decision that has not been made. This restates pass A's escalation 19 and pass
   B's escalation 19 without the spine ranking; the substance is theirs.

4. **Whether the ranking withdrawn in section 4 needs replacing with anything.**
   Both reports currently tell the planner which half of item 6.2 is primary.
   With the premise withdrawn, item 6.2 is recorded as two halves in different
   states and nothing orders them. Whether the plan wants an order, and on what
   ground, is the planner's and the owner's, not an agent's. Pass A's escalation
   17 and pass B's escalation 18 already ask this; they should ask it without
   answering it in the surrounding prose.

---

## 11. Divergence from adversary3-04-1 — where we agree

`adversary3-04-1.md` did not exist when I wrote sections 1 to 10; I checked by
`ls` before writing and again after, and it landed between the two checks. Nothing
below was written with sight of it, and I have changed nothing in sections 1 to 10
on account of it except the correction marked in 2.6 and the two concessions in
section 12, each of which says what it concedes and why.

We converged independently on the shape of the round.

| Point | Pass 1 | This file | Agreement |
|---|---|---|---|
| All eight claims upheld | §1 | §2 | Same eight verdicts |
| `-D "ARCH_"`, 21 sites | claim 1 | 2.1 | Same 21, same nineteen `runtime.cpp` lines |
| 12 consumers, 2 disabled producers | claim 4 | 2.4 | Identical decomposition |
| 13 ordered comparisons, 7/1/5 | claim 8 | 2.8 | Identical split |
| 8 `set_caps` sites, 5 RHI 3 C API | §2 | §3 | Identical |
| **Both reports privilege a spine** | §3 | §4 | Same finding, same cause, same remedy |
| `TI_VISIBLE_DEVICE` missing from everything | §5.1 | §6 | Both new, both independent |
| Pass A omits `get_device_score` | §5.2 | 2.5, 7.2 | Same |
| Pass B's optimizer line is `:2681` | §4.1 | 7.1 | Same |
| Neither report knows section 5.1a exists | §5.4 | 2.2, 7.4 | Same, same escalations to close |
| Claim 7's flag was half right | claim 7 | 2.7 | Same verdict, different reason — see 12.5 |
| Not correct, not complete, no consensus | §0 | §1 | Same three verdicts |

Two independent agents reaching the spine-privileging finding and the
`TI_VISIBLE_DEVICE` gap by different routes is the strongest signal in this round.
Neither of us took it from the other.

**Their count, checked mechanically rather than accepted.** Pass 1 states 32
instances of the superseded framing. I ran
`grep -no "reference path"` and `grep -no "accelerant"` over both files: pass A
gives 6 plus 1 on lines 716, 718, 720, 875, 1292, 1409, 1601; pass B gives 20 plus
5 on lines 42, 48, 359, 360, 379, 417, 430, 500, 501, 515, 522, 530, 621, 640, 956,
1433, 1656, 1868, 1871, 1875, 1883, 1893, 1953, 1959, 1989. Seven plus twenty-five
is thirty-two, and their enumeration reconciles to their total. Their list is more
complete than the one in my section 9 item 1, which I derived from a phrase grep
without totalling; **theirs should be used for the fix.**

---

## 12. Divergence from adversary3-04-1 — where we differ

### 12.1 I have one finding they do not, and it is a live defect: GLES is reachable through the C API

Section 5 above. `grep -n "ti_import_opengl_runtime\|set_gles_override\|use_gles"`
over `adversary3-04-1.md` returns **nothing**. Their section 5 gap list does not
contain it, and their claim-6 discussion of the OpenGL capability replacement does
not question the reachability caveat both reports rest on.

**The source supports me, and it is not a matter of reading.**
`ti_import_opengl_runtime` is declared at
`c_api/include/taichi/taichi_opengl.h:33-36` with a `bool use_gles` parameter, and
its body calls `taichi::lang::opengl::set_gles_override(use_gles)` at
`c_api/src/taichi_opengl_impl.cpp:26` before `ti_create_runtime(TI_ARCH_OPENGL, 0)`
at `:28`. `initialize_opengl` consumes that override at
`taichi/rhi/opengl/opengl_api.cpp:56-59`, replacing the `use_gles=false` that
`GLDevice::GLDevice` passes at `opengl_device.cpp:507`. So pass A's "GLES is
unreachable through the C API" (`report-04:613-618, 636-637, 1493-1496`) and pass
B's "the C API cannot ask for GLES directly ... latent rather than always live"
(`report-04b:631-635, 1983-1987`) are both false.

This matters for the joint verdict on completeness. Pass 1's six gaps are five
restatements and one census; on that basis it says "None of it requires
re-investigation." That is right for their six and wrong for this one. This is a
live defect on the low-end profile plan sections 2 and 5.3 put first, reached by a
documented public function, and it inverts an escalation in each report rather than
rewording one. It also survived both round-two adversaries: `adversary2-04-1.md`
raised the unreachability as its own finding 9.3.3 and `adversary2-04-2.md:455-457`
endorsed pass A's caveat as correct. Four documents carry it.

### 12.2 They have a `get_device_score` point I missed, and they are right

Pass 1 §4.2: pass B's "the score is a local that dies there" is true of the call at
`vulkan_device_creator.cpp:462` and false of the call at `:452`, where
`else if (get_device_score(devices[id], test_surface))` gates whether the
explicitly named device is accepted at all.

I read `:442-459` and confirmed the structure, then wrote at 2.5 that pass B's
citation is precise. It is — B's line numbers `:203-229`, `:220-225`, `:226`,
`:452`, `:462` are all right — but I did not test the sentence those citations
support, which is exactly the failure standing instruction 6 names. **The source
supports pass 1.** Conceded. B has one `[V]` sentence to narrow.

### 12.3 They have the CUDA reconciliation and I do not

Pass 1 §5.6 and claim 7: `driver_.device_get(&device_, 0)`
(`taichi/rhi/cuda/cuda_context.cpp:22`) pins an **ordinal**, and the CUDA driver's
own `CUDA_VISIBLE_DEVICES` remaps which physical card is ordinal 0, outside
Taichi. I re-ran their negative grep: `CUDA_VISIBLE_DEVICES` over `taichi/`,
`c_api/` and `python/` returns nothing. Their reconciliation holds, and it
correctly restates the severity as one-process-one-card rather than
one-machine-one-card, which is what keeps the finding from reading as a challenge
to the empirical statement plan section 5.1a says outranks code reading.

I did not have this. **The source supports pass 1.** Conceded, and it is the
better handling of the constraint I flagged at 6 about not contradicting 5.1a.

### 12.4 They have the Metal gap in pass B and I did not

Pass 1 §5.3. Conceded and corrected in place at 2.6 above, with the lines. My
original sentence said both reports state the section 2.2 caveat; only pass A
does.

### 12.5 We reach the same verdict on claim 7 by opposite reasoning, and we are both partly right

Both of us say the amendment agent's flag was half right, and both of us say pass
B's "no gap to record" and "practical consequence for testing" are wrong. On that
we agree, and pass 1 argues it better than I did, from plan section 5.1a's "may
need to choose between them, or to use more than one" rather than from the framing
rule. I adopt their argument over mine.

**We disagree about pass A.** Pass 1 says "The refusal genuinely covers CPU, which
is a first-class target, so the finding does not need the card list and is stronger
without it." I said at 2.7 that pass A over-reaches by counting `x64` and `arm64`
among archs with a gap.

Both halves of that are true and they are about different things, so I narrow
rather than withdraw:

- **Pass 1 is right** that stating the refusal enum-wide is stronger than stating
  it through the card list, and that CPU being covered is what makes it stand
  without the cards. I agree and I did not say it.
- **I hold the narrower point.** Calling the CPU refusal a *gap* is over-reach.
  `taichi/rhi/cpu/` is `CMakeLists.txt`, `cpu_device.cpp` and `cpu_device.h`, and a
  grep for `device_index`, `device_get_count` or `num_devices` across
  `taichi/rhi/cpu/` and `taichi/rhi/llvm/` returns nothing. There is no second host
  device in the model for `device_index != 0` to name, so on `x64` and `arm64` the
  refusal is the correct answer rather than a missing feature. Pass A's
  "cannot express a second device at all" (`report-04:443-445`, escalation 12 at
  `:1403-1404`) is true and vacuous for those two archs. The substantive gap is
  CUDA, plus AMDGPU and DX12, which have no C API case at all.

The source underdetermines this: it settles that there is no CPU device index and
leaves open whether an absent capability with nothing to address counts as a gap.
That is a judgement, and I record both positions rather than claim mine wins.
Escalation 5 below.

### 12.6 Where I go further than pass 1 on a shared finding

On `TI_VISIBLE_DEVICE` we found the same variable and drew different consequences,
and the two are complementary rather than competing. Pass 1 §5.1 reads it as a
second install-time channel alongside `TI_LIB_DIR`, which is a good reading I did
not have. My section 6 has the ordering: the environment read sits inside the
`std::call_once` at `vulkan_loader.cpp:87-115`, and
`VulkanDeviceCreator::VulkanDeviceCreator` calls `init()` at
`vulkan_device_creator.cpp:236` **after** `ti_create_runtime` has written the
caller's `device_index` at `taichi_core_impl.cpp:254-255`. So on the first Vulkan
runtime in a process the environment silently overrides the API argument, and on
later ones it does not. Pass 1 does not have that, and it turns the finding from
"a second channel exists" into "the two channels collide, order-dependently". Both
belong in the absorption at section 9 item 3.

Similarly on the C API: my 5.3 has the converse case, where an earlier
`ti_get_available_archs` locks `kUseGles` false and the `use_gles` argument is
silently lost. Pass 1 has neither half.

Two smaller items are mine alone and neither is load-bearing: pass B's `kUseGles`
having two writers (7.3), and both round-two adversary files miscounting
`is_extension_supported` against their own lists (section 3).

### 12.7 Net effect on the joint remedy

Nothing in pass 1 changes my verdicts and nothing in mine changes theirs. The two
files agree that both reports are not correct, that the pair is not complete, and
that consensus is not reached.

The joint list for the planner is pass 1's section 6 plus my section 9, with three
adjustments:

1. **Use pass 1's 32-line enumeration** for the framing fix, not my phrase list.
2. **Add my section 5 to the remedy**, and note that it is the one item on either
   list that is not a restatement. Pass 1's "none of it requires re-investigation"
   holds for its own six items and not for this one.
3. **Absorb both halves of `TI_VISIBLE_DEVICE`** — pass 1's install-time-channel
   reading and my ordering collision.

On the concessions: pass 1 is right on `get_device_score` at `:452`, on the
`CUDA_VISIBLE_DEVICES` reconciliation, and on pass B's missing Metal caveat. I have
recorded all three above and corrected 2.6 in place.

---

## 13. Escalation added after reading adversary3-04-1

5. **Does a refused capability with nothing to address count as a gap?** Per
   12.5, `ti_create_runtime` refuses `device_index != 0` on `x64` and `arm64`
   (`c_api/src/taichi_core_impl.cpp:277, 283`), and there is no CPU device index
   anywhere in `taichi/rhi/cpu/` or `taichi/rhi/llvm/` for it to name. Pass A
   records this among a four-arch gap; I read the refusal as correct rather than
   missing; adversary 3 pass 1 reads CPU coverage as what makes the finding stand
   without the card list. All three readings are consistent with the source, which
   does not settle it. It matters only because plan section 5.2 asks that gaps be
   recorded per spine, and the size of the LLVM-spine gap here is either three
   archs or four depending on the answer. Not mine to decide.
