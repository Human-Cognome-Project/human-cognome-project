# Adversary 3, pass 1 — territory 04, backend and build architecture

Round three. Reviewing `report-04-backend-build.md` (revision 4) and
`report-04b-backend-build.md` (revision 4, framing pass), against
`adversary2-04-1.md`, `adversary2-04-2.md`, `notes-04-backend-build.md` and
`notes-04b-backend-build.md`, and against `PROJECT-PLAN.md` as it stands on
2026-09-09 with the section 5.2 correction and section 5.1a in place.

Every figure below was re-derived from source in this pass. Where I state a
count I give the enumeration it came from, per standing instruction 8. Nothing
below is verified by agreement between the two reports.

---

## 0. Verdicts

**CORRECT — with one exception that is not a citation error and not a count
error.** Every one of the eight claims put to me is verified true, at the lines
given, with two minor citation slips recorded in section 4. The censuses are
sound; I re-ran all of them mechanically and they reconcile.

The exception is section 3. **Both reports privilege one compilation spine over
the other, in language taken verbatim from a version of plan section 5.2 that
no longer exists and that the current correction explicitly contradicts.** It
appears 32 times across the two files. It is not a leftover phrase: it is
load-bearing on four escalations and on the weighting of item 6.2 in both
reports. Answering the lead's question directly — **yes, a spine-privileging
conclusion survives the framing correction.** It is inverted relative to the
round-one error, which is why the framing pass did not catch it, but it is the
same class of error and the current plan rules it out in terms.

**NOT COMPLETE.** Six gaps, in section 5. The largest is a device-selection
mechanism neither report found, which bears directly on the deployment model
section 5.1a settles. Two more are escalations the plan has since closed and
both reports still carry as open, because neither report is aware section 5.1a
exists.

**Consensus is therefore NOT reached.** What remains is in section 6. None of it
requires re-investigation. Five of the six items are restatements against the
current plan text; one is a small new census.

---

## 1. The eight claims, verified

### Claim 1 — the `-D` on the bitcode clang line. UPHELD, exactly.

`taichi/runtime/llvm/runtime_module/CMakeLists.txt:8` reads, in full:

```
COMMAND ${CLANG_EXECUTABLE} ${CLANG_OSX_FLAGS} -c runtime.cpp -o "runtime_${rtm_arch}.bc" -fno-exceptions -emit-llvm -std=c++17 -D "ARCH_${rtm_arch}" -I ${PROJECT_SOURCE_DIR};
```

`rtm_arch` is the function parameter of `COMPILE_LLVM_RUNTIME`, declared `:3`,
called from the loop at `:29-31`,
`foreach(arch IN LISTS HOST_ARCH CUDA_ARCH DX12_ARCH AMDGPU_ARCH)`. One
artefact per value, `:8`; installed `:13`; selected at run time by
`get_runtime_fn`, `taichi/runtime/llvm/llvm_context.cpp:209-211`.

**21 consumption sites, derived from my own enumeration, not from either
report's list.** `grep -n 'ARCH_' taichi/runtime/llvm/runtime_module/runtime.cpp`
returns exactly 19 lines: `:51, 119, 155, 347, 771, 798, 801, 860, 1144, 1156,
1167, 1293, 1343, 1431, 1549, 1633, 1651, 1861, 1865`. `grep -rn 'ARCH_'` over
`taichi/runtime/llvm/runtime_module/*.h` returns exactly 2:
`node_pointer.h:15` (`#if defined(ARCH_cuda)`) and `locked_task.h:7`
(`#if ARCH_x64 || ARCH_arm64`). Both are included by `runtime.cpp`, at `:1660`
and `:1872` respectively, so both are in the same translation unit. 19 + 2 = 21.

Both reports state 21 and both list the same 19 line numbers. Correct.

The falsification the lead describes is real and both reports record it against
themselves: report B's escalation 1 said "nothing in the tree yet compiles a
configure-time value into the bitcode" and is now rewritten at 5.3.1; report A's
escalation 1 said "only one route is sound" and is now three routes. Neither
trimmed the retraction, which is the right handling.

### Claim 2 — the runtime filename carries the arch and nothing else. UPHELD.

`llvm_context.cpp:209-211` is three lines and the whole of it is
`return fmt::format("runtime_{}.bc", arch_name(arch));`. `arch_name`
(`taichi/rhi/arch.cpp:6-18`) is an x-macro switch over `archs.inc.h` returning
the stringified enumerator. No second parameter, no second format argument.

**And section 5.1a rules one ceiling per build sufficient — which the existing
machinery already delivers with no change to the filename scheme.** Neither
report states this positively, and it is the closing answer to the escalation
both of them still leave open. The mechanics: within a single configure, the
`-D` at `:8` takes the same value on every iteration of the loop at `:29`
except the `ARCH_` token itself, because the loop varies only `arch`. So all
`runtime_*.bc` produced by one build necessarily carry one ceiling. The host's
only coupling to the constant, the assertion at
`taichi/codegen/llvm/struct_llvm.cpp:266`, is compiled once into `taichi_core`
in the same configure. Host and every bitcode variant therefore agree by
construction. Section 5.1a's "per build already means per device configuration"
is not merely permitted by this machinery — it is what the machinery does now.

Section 6 of this file records this as something the reports should say rather
than as a defect in them; it was not knowable to either, because neither was
given section 5.1a.

### Claim 3 — the host never lays out `LLVMRuntime`. UPHELD.

`grep -rn 'struct LLVMRuntime' taichi/ c_api/` returns exactly five lines:
`taichi/program/context.h:11` and `taichi/rhi/llvm/llvm_device.h:8`, both
`struct LLVMRuntime;` and both in host code; and
`taichi/runtime/llvm/runtime_module/runtime.cpp:136`, `:337` and `:552`, the
last being the sole definition. I opened `:552` and confirmed the three arrays
at `:567-569` (`element_lists`, `node_allocators`, `ambient_elements`, all
`[taichi_max_num_snodes]`) and the two at `:562-563` sized by
`kMaxNumSnodeTreesLlvm`.

So the host-versus-device layout disagreement both reports described in their
earlier revisions cannot happen, and both have retracted it in place with the
reason given rather than deleted. Report A at 2.5, report B at 5.3. Both
correctly note that the conclusion is strengthened, not weakened: the arrays
exist only inside the bitcode, so the value must reach that clang line.

### Claim 4 — the disabled 64-bit branch is not permanent. UPHELD.

Two producers, both dead. `taichi/rhi/vulkan/vulkan_device_creator.cpp:826` sits
inside `#if !defined(__APPLE__) && false` — I read the guard, and the `&& false`
is present, so the branch is dead on every platform, not only Apple. The comment
at `:824` is "(penguinliong) Temporarily disabled (until device capability is
ready)" and `:822-823` reference taichi issue 6295.
`c_api/src/taichi_vulkan_impl.cpp:48` is inside a `/* */` block spanning `:46-51`.

**Twelve live consumers, from my own enumeration.**
`grep -rn 'spirv_has_physical_storage_buffer' taichi/ c_api/` returns 16 lines.
Removing the declaration (`taichi/inc/rhi_constants.inc.h:28`), the two dead
producers above, and the header-only C++ wrapper's typed setter
(`c_api/include/taichi/cpp/taichi.hpp:1156`) leaves 12:
`taichi/rhi/vulkan/vulkan_device.cpp:1772, 1792, 2147, 2509` (4);
`taichi/codegen/spirv/spirv_ir_builder.cpp:73, 113` (2);
`taichi/codegen/spirv/spirv_codegen.cpp:783, 2340, 2416, 2491` (4);
`taichi/runtime/gfx/runtime.cpp:96` (1);
`taichi/runtime/program_impls/gfx/gfx_program.h:81` (1). 4+2+4+1+1 = 12.

The C API entry point installs any set with no validation. I read
`ti_set_runtime_capabilities_ext` at `c_api/src/taichi_core_impl.cpp:317-334`
in full: it loops the caller's `TiCapabilityLevelInfo` array at `:326-330`,
casts `cap_level_info.capability` straight to `DeviceCapability` at `:328`, and
calls `runtime2->get().set_caps(std::move(devcaps))` at `:331`. There is no
device query, no range check on the cast, and no comparison against anything.
That lands on `Device::caps_`, `taichi/rhi/public_device.h:617`, written by
`set_caps` at `:855` and read by `get_caps` at `:852` — the same field all
twelve consumers read.

Both reports have retracted "permanently" and both kept the retracted wording
visible. Report B's 2.4 does this most legibly, heading the section with the
corrected statement and keeping the withdrawn one below it.

### Claim 5 — both spines detect tier data and neither publishes it. UPHELD, and stronger than report A states.

Verified site by site.

- **Vulkan memory properties.** `taichi/rhi/vulkan/vulkan_device.cpp:2515-2516`
  calls `vkGetPhysicalDeviceMemoryProperties` into a local. I read `:2518-2534`:
  the only reads are `properties.memoryTypeCount` (`:2518-2519`, sizing a
  vector) and `properties.memoryTypes[i].propertyFlags` (`:2522`), tested for
  `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT` to write an external-memory handle-type
  flag per type. `grep -rn 'memoryHeap\|heapSize' taichi/rhi/` returns **zero
  lines** across the whole RHI, not only the Vulkan directory. Heap sizes are
  never read anywhere. Confirmed.
- **Stored device properties, two consumers.** `grep -rn
  'vk_device_properties_\|get_vk_physical_device_props' taichi/ c_api/` returns
  six lines: the accessor `vulkan_device.h:710-711`, the member `:739`, the
  fill at `vulkan_device.cpp:1596`, and exactly two reads —
  `vulkan_device.cpp:1125` (`maxComputeWorkGroupCount[0..2]`, bounding
  `dispatch`, returning `RhiResult::not_supported` past it) and `:1945`
  (`limits.timestampPeriod`). Two consumers, confirmed.
- **Device name to debug logs.** `vulkan_device_creator.cpp:438` and `:512`, both
  inside `RHI_LOG_DEBUG` blocks. Confirmed.
- **OpenGL, two write-only globals.** `grep -rn
  'opengl_max_block_dim\|opengl_max_grid_dim' taichi/ c_api/ python/` returns
  exactly six lines: the two definitions at `opengl_api.cpp:20-21`, the two
  `glGetIntegeri_v` calls at `:231` and `:234`, and the two `TI_TRACE` lines at
  `:233` and `:236`. No header declares either name and nothing consumes either.
  Both reports are right that this is stronger than "detected and not
  published": the values are detected and used for nothing at all, and the
  hardcoded 1024 fallback documented at `:18-19` is therefore also never
  consulted.

**One item is in report B and missing from report A.** `get_device_score`,
`taichi/rhi/vulkan/vulkan_device_creator.cpp:203-229`, reads
`properties.deviceType` and adds 500 for
`VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU` (`:220-222`) and 1000 for
`VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU` (`:223-225`), plus 100 per API minor
version (`:226`). Called at `:452` and `:462`. This is the most tier-shaped
judgement made anywhere on the SPIR-V spine — a discrete-versus-integrated
classification — and report A's section 4.8 census does not contain it. That is
a gap in report A on the exact finding 4.8 exists to make.

**And report B slightly understates it.** Report B says "the score is a local
that dies there". At `:462-465` that is true, it ranks and the number dies. At
`:452` it does not: `else if (get_device_score(devices[id], test_surface))`
uses the score as a **boolean admission gate** on an explicitly requested
device. A device the user asked for whose score is zero is silently dropped and
the automatic pick runs instead, with no log on that branch. So the score does
decide which device is used; what dies is the numeric value, never the
decision. Small, but it is a `[V]` sentence in a section about things being
thrown away, and one of the two call sites does not throw it away.

### Claim 6 — Metal is the one complete chain, and it publishes admission. UPHELD.

I read `collect_metal_device_caps`, `taichi/rhi/metal/metal_device.mm:1017-1069`,
in full. The ladder is at `:1027-1036`: `MTLGPUFamilyMac2` and Apple 3 through
7, each rung OR-ed into the one below it (`family_apple6` folds in
`family_apple7`, and so on down), so it is monotone. Five named features at
`:1038-1042`. Written into a `DeviceCapabilityConfig` at `:1044-1068`, returned
`:1069`, installed by `MetalDevice::MetalDevice` at `:1168`. Read back at
`:131-132`, `caps.contains(DeviceCapability::spirv_has_int64)`, selecting MSL
2.3 at `:134`. Detect, reduce, publish, consume — the only complete chain in the
tree.

**And the content is feature admission.** `:1051-1053` is
`if (feature_64_bit_integer_math) { caps.set(DeviceCapability::spirv_has_int64, 1); }`,
and `feature_64_bit_integer_math = family_apple3` at `:1038`. Below Apple3 the
capability is absent, `get()` returns 0 (`device_capability.cpp:38`), and the
four consumers at `spirv_ir_builder.cpp:64, 166, 312, 326` take the
no-64-bit branch. Hardware generation decides what the system can express.
That is the shape section 2.2 calls a violation rather than an optimisation.

**Section 6.3's finding is correct, with one qualification the plan's wording
does not carry.** I read `taichi/inc/rhi_constants.inc.h` in full. Lines 10-34
are 25 `PER_DEVICE_CAPABILITY` entries: `reserved`, `spirv_version`, and 23
`spirv_has_*` feature bits. So "entirely feature bits and version numbers" is
exactly right. The qualification: the plan says the vocabulary "has no way to
say that a device does a thing faster, or at finer granularity". The *storage*
does — the level is `uint32_t` (`device_capability.h:22`) and `spirv_version`
is already read as a graded scale in thirteen places (claim 8). What is absent
is any *entry whose meaning is fidelity*, not any means of expressing a level.
Report A's escalation 19 states this correctly — "Every entry ... is today
either a feature bit or a version number; none is a fidelity level" — and is
more precise than the plan's paraphrase of it.

Note also that the file's own header comment invites the missing entry:
`:1-7` says "DirectX shader model, CUDA compute capability and Vulkan physical
device features **can be listed here**", while excluding API and GLFW versions.
Both reports cite this. It means the vocabulary's author anticipated a graded
hardware-class entry and none was ever added.

**Report B does not carry the section 2.2 caveat.** Its 2.8 calls Metal "the
exception, and it is the model", records that `spirv_has_int64` is set under
`feature_64_bit_integer_math`, and draws only the positive lesson: "detect
whatever the platform offers, express the consequence in portable terms". It
never observes that what Metal publishes is admission and that below Apple3 a
device loses a capability outright. Report A does, at 4.9 and escalation 19,
and escalates rather than resolving. That asymmetry is a gap in report B.

### Claim 7 — multi-device selection, and the amendment agent's flag. UPHELD on fact; the flag was HALF right.

**The fact.** `grep -n 'TI_CAPI_NOT_SUPPORTED_IF_RV(device_index' c_api/src/taichi_core_impl.cpp`
returns exactly five lines: `:270` (opengl), `:277` (x64), `:283` (arm64),
`:289` (cuda), `:297` (metal). Vulkan alone consumes the parameter, at `:254-255`,
`set_vulkan_visible_device(std::to_string(device_index))`. `TI_ARCH_GLES` has no
case at all in the switch (`:251-305`), so of the six archs with cases, one
honours and five refuse. Confirmed. `taichi/rhi/cuda/cuda_context.cpp:21-22`
additionally reads the count and then passes a literal 0:
`driver_.device_get_count(&dev_count_); driver_.device_get(&device_, 0);`.

**The flag.** The amendment agent's move was to stop arguing this through the
owner's two cards and state it enum-wide. That half is right, and both reports
now do it: report A 3.3 and escalation 12, report B 2.5 and escalation 10.
The refusal genuinely covers CPU, which is a first-class target, so the finding
does not need the card list and is stronger without it.

**But report B went one step too far, and that step is wrong against section
5.1a.** Report B 2.5 closes: "Practical consequence for testing, not for
architecture: a box with two cards can exercise both only through the portable
path", and its 8.1c item 2 records "on device selection the portable path is the
one better served, so there is no gap in it to record". Section 5.1a settles
that a machine holding several GPUs is the ordinary case, that the install-time
agent "may need to choose between them, or to use more than one", and that
"more than one instance of the engine can run, especially across more than one
card". Choosing a device is therefore part of the deployment mechanism the plan
has settled, not a testing convenience. An install-time agent that cannot name
a device through the C API on CUDA or on CPU is blocked on an architectural
requirement. Report B's downgrade is an error; report A does not make it and
asks the question straight at escalation 12.

**Neither report reconciles the pin with the plan's empirical statement, and
one of them should.** Section 5.1a records that the owner has run the engine on
both cards at once, calls it direct observation, and says it outranks any
code-reading conclusion. Read baldly, "CUDA can only see device 0" contradicts
that. It does not actually contradict it, for a reason neither report states:
`driver_.device_get(&device_, 0)` pins an **ordinal**, not a physical device,
and the CUDA driver's own `CUDA_VISIBLE_DEVICES` remaps which physical card is
ordinal 0 — outside Taichi, which reads no such variable
(`grep -rn 'CUDA_VISIBLE_DEVICES' taichi/ c_api/ python/` returns nothing). So
two processes with different values of that driver variable each get a
different card, which is consistent with the observation. The finding is real
and its severity is one-process-one-card, not one-machine-one-card. Stating it
without that distinction invites the planner to weigh a code reading against an
observation the plan says outranks it.

### Claim 8 — the ordering census and the CMake precedence. UPHELD, both.

**Thirteen, from my own enumeration.** I grepped `spirv_version` across
`taichi/` and `c_api/` including `.mm`, then discarded matches where the only
angle bracket was the `->` in `caps_->get`. What survives:

| operator | sites | count |
|---|---|---|
| `<` | `spirv_ir_builder.cpp:556, 625, 671, 679, 741, 777`; `vulkan_device_creator.cpp:577` | 7 |
| `>` | `spirv_codegen.cpp:1908` | 1 |
| `>=` | `spirv_ir_builder.h:424`; `spirv_codegen.cpp:2669, 2671, 2673, 2675` | 5 |

7 + 1 + 5 = 13. I also checked whether any capability other than
`spirv_version` is ever compared ordinally: grepping `DeviceCapability::` and
`cap::` for a comparison operator, excluding `->`, returns one line,
`vulkan_device_creator.cpp:577`, which is itself a `spirv_version` site. So
`spirv_version` is the only ordinally-compared capability in the tree. Both
reports state thirteen and both are right; report B additionally states the
exclusivity and is right about that too.

The ladder both reports call the strongest instance: I numbered it line by
line. `spirv_version` is read into a local at **2666**; the ladder runs
**2669-2678**, closing at 2679; the optimizer is constructed at **2681**.

**The CMake precedence.** `taichi/rhi/CMakeLists.txt:17` reads, verbatim:
`if (TI_WITH_OPENGL OR TI_WITH_VULKAN AND NOT ANDROID)`. CMake's `if()`
precedence is unary tests, then binary tests, then `NOT`, then `AND`, then `OR`,
so this parses as `TI_WITH_OPENGL OR (TI_WITH_VULKAN AND (NOT ANDROID))`. The
guarded block runs to `endif()` at `:35` and contains the GLFW subdirectory add
(`:29`), link (`:30`) and public include path (`:31-34`), plus
`target_compile_definitions(... TI_WITH_GLFW)` at `:28`. So with OpenGL on and
`ANDROID` set the guard does not apply. Report A states the precedence in full
(`NOT` tighter than `AND` tighter than `OR`); report B states the half it
needs. Both are correct and both mark the build outcome as not executed, which
is the right handling under standing instruction 6.

---

## 2. Other censuses I re-derived, all sound

- **`set_caps`, eight sites.** `grep -rn 'set_caps(' taichi/ c_api/` returns
  nine lines; removing the declaration at `public_device.h:855` leaves eight:
  `metal_device.mm:1168`, `vulkan_device.cpp:1580`,
  `vulkan_device_creator.cpp:876`, `opengl_device.cpp:529`,
  `dx_device.cpp:565` (five in the RHI), plus
  `taichi_core_impl.cpp:331`, `taichi_opengl_impl.cpp:12`,
  `taichi_vulkan_impl.cpp:53` (three in the C API). 5 + 3 = 8.
  `grep -rn 'set_caps'` over `taichi/rhi/cpu/`, `cuda/`, `llvm/`, `amdgpu/` and
  `dx12/` returns nothing. Both reports correct.
- **`get_required_caps`, two hits.** The default body at
  `taichi/aot/module_loader.h:97` and the sole call site at
  `c_api/src/taichi_gfx_impl.cpp:21`. No override anywhere. Both reports correct;
  the capability gate is dead.
- **The AOT dispatch.** `taichi/aot/module_loader.cpp:31-58`, read in full:
  vulkan, opengl, gles, dx11 and metal to `gfx::make_aot_module`, dx12 to
  `directx12::make_aot_module`, `TI_NOT_IMPLEMENTED` at `:57`. No `Arch::x64`,
  `arm64`, `cuda` or `amdgpu` arm. Both reports correct.
- **`Extension`, and no `data64` consumer.** `grep -rn 'is_extension_supported'`
  returns eleven lines: the definition (`extension.cpp:8`), the declaration
  (`extension.h:24`), the pybind export (`export_lang.cpp:1225`), and eight real
  call sites — `program.cpp:149` (assertion), `codegen_llvm.cpp:2726` (bls),
  `compile_to_offloads.cpp:92, 205, 218, 236` (mesh) and `:245, 288` (quant).
  No site passes `data64`. Report A's 4.1 is exact. Report B does not cover the
  `Extension` system at all and says so in its 8.4.
- **`extension.cpp:31`** is `const auto &exts = arch2ext[arch];` — non-const
  `operator[]` on a function-static map, so a lookup on an absent key inserts.
  The commented-out dynamic attempt is at `:29-30`. Report A correct.
- **`dummy.cpp` is zero bytes** (`wc -c` → 0) and `ti_device_api_shared` appears
  at exactly three lines, `taichi/rhi/CMakeLists.txt:114, 115, 120`, none of
  which is a link from anything else or an install. Both reports correct.
- **`TI_WITH_CPU`** appears at `c_api/include/taichi/taichi.h:21` and `:23`, and
  at `docs/lang/articles/deployment/tutorial.md:288`. Defined nowhere. Report A
  gives all three, report B the two source lines under a stated narrower scope.
  Both correct.
- **The published ordering contract**, `c_api/include/taichi/taichi_core.h:409-412`,
  reads exactly as both reports quote it. Both correct.
- **`archs.inc.h`** carries twelve `PER_ARCH` entries at `:4-17`;
  `arch_uses_llvm` is `{x64, arm64, cuda, dx12, amdgpu}` (`arch.cpp:54-57`) and
  `arch_uses_spirv` is `{opengl, gles, vulkan, dx11, metal}` (`:63-66`). Both
  reports correct.

---

## 3. What is newly wrong: both reports privilege a spine, and the plan forbids it

This is the finding of this round and it is the direct answer to the lead's
question about vendor-leaning conclusions.

### 3.1 The language

Both reports call the SPIR-V spine **"the reference path"** and the LLVM spine
**"the vendor path"** or **"the accelerant"**.

Report A, six instances of "reference path" and one of "accelerant":
`report-04-backend-build.md:716, 718, 720, 875, 1292, 1409, 1601`.

Report B, twenty instances of "reference path" and five of "accelerant":
`report-04b-backend-build.md:42, 48, 359, 360, 379, 417, 430, 500, 501, 515,
522, 530, 621, 640, 956, 1433, 1656, 1868, 1871, 1875, 1883, 1893, 1953, 1959,
1989`.

### 3.2 Why it is wrong

`grep -n 'reference path\|accelerant' PROJECT-PLAN.md` returns **nothing**.
Neither phrase is in the plan. What the plan says, at `PROJECT-PLAN.md:414-431`:

- "There are two compilation spines and **neither one is the vendor spine**"
  (`:416-417`).
- The LLVM spine "carries `x64` and `arm64`, which is CPU-only operation and a
  FIRST CLASS target under section 2" (`:420-421`).
- "Vendor lock is a property of individual backends, not of a spine. CUDA is
  vendor locked. **The LLVM spine is not**, because CPU-only operation runs
  through it. Treating the LLVM spine as 'the vendor half' would deprioritise
  the no-GPU target, which section 2 puts first" (`:424-426`).
- "**no backend may be privileged** such that capability differs across
  targets ... Where one spine is better served than the other, that is a gap in
  the less-served spine to record, **never evidence that it matters less**"
  (`:428-431`).

Calling one spine the reference and the other the accelerant is precisely
"treating the LLVM spine as the vendor half". The correction names that move and
rules it out.

### 3.3 Where it came from, which is not the reports' fault

`notes-04-backend-build.md:1946-1949` records the instruction the amendment
agent was working from, quoting it: "Plan section 5.2 as amended says the
reference path is the portable one and a vendor path is 'an optional
accelerant, never the baseline'". `adversary2-04-1.md:753-754` carries the same
wording. So an intermediate version of section 5.2 did say this, both amendment
agents applied it faithfully, and the current 2026-09-09 correction replaced it
with the neither-spine formulation. The reports were correct against the plan
they were given and are wrong against the plan as it stands.

That matters for how it is fixed: this is a restatement, not a re-investigation.
No census changes. Section 6 lists what has to move.

### 3.4 Why it is not cosmetic

Four places where the superseded framing is load-bearing.

1. **The weighting of item 6.2.** Report A 4.3 (`:716-718`): "Written-and-disabled
   machinery on the reference path is therefore the **primary** existing form of
   item 6.2 in this territory, and whatever 6.2 eventually needs on the LLVM
   spine is the accelerant, not the baseline." Report B 2.4 (`:359-360`) says the
   same in the same words. Under the current plan neither half of 6.2 is
   primary; there is one work item with two shapes and no ranking. Both reports
   assert a ranking the plan does not supply.
2. **Report B claims the plan supplies that ranking.** `report-04b:956`: "the
   ranking that plan 5.2 does supply runs the other way from my withdrawn
   sentence: the reference path is the portable one." That is a statement about
   what the plan says, and the plan does not say it. It is the sharpest instance
   because it attributes the conclusion to the plan rather than deriving it.
3. **Report A contradicts itself.** Its 4.7 bullet at `:875` reads "On the
   SPIR-V spine — the reference path under plan section 5.2". Twelve lines
   further down, the same section says: "It is not a reason to weight 6.2 toward
   either spine. **The LLVM spine is not 'the vendor path'**: it carries
   `Arch::x64` and `Arch::arm64`, which is CPU-only operation, a first-class
   target." Both sentences are in section 4.7. One of them is the current plan
   and the other is the superseded one. A reader cannot reconcile them.
4. **It reaches the escalations.** Report A escalation 13 (`:1409`) and report B
   escalation 9 (`:1868-1875`) both open by asserting the SPIR-V machinery is
   "the primary existing form" of 6.2 on the strength of the framing. Report B
   escalation 20 (`:1989`) closes with "and it is on the reference path", which
   is offered as weight. Report B escalation 11 (`:1893`) calls the CUDA clamp a
   ceiling on "the vendor accelerant".

### 3.5 What survives the correction unchanged

To be explicit about what I am **not** objecting to. The underlying facts in
every one of those passages are verified and stand:

- The 64-bit pointer machinery is written, compiled and switched off on
  `arch_uses_spirv`, twelve consumers, two dead producers.
- No capability representation of any kind exists on `arch_uses_llvm`, and that
  spine carries `taichi/rhi/cpu/`.
- Item 6.2 divides into halves of different shape along that predicate.
- Multi-device selection exists on Vulkan and nowhere else.

All of that is a correct application of the current plan's actual rule: record
the gap in the less-served spine. The error is the extra step of naming one
spine the reference and ranking the work by which spine it sits on.

---

## 4. Two citation errors, minor

1. **Report B, the optimizer line.** `report-04b` section 3.3 says the graded
   ladder's `target_env` "configures the optimizer for the whole module at
   `:2680`", and repeats `:2680` in the section 6 map. I numbered
   `spirv_codegen.cpp` line by line: `:2679` is the ladder's closing brace,
   `:2680` is blank, and `:2681` is
   `spirv_opt_ = std::make_unique<spvtools::Optimizer>(target_env);`. Report A
   gives `:2681` and is right. Two occurrences to correct in B.
2. **Report B, `get_device_score`.** "the score is a local that dies there" is
   true of the call at `:462` and false of the call at `:452`, where the score
   gates whether an explicitly named device is accepted. Section 1, claim 5
   above.

Neither changes a conclusion. I record them because both sit in `[V]` sentences.

---

## 5. What is still missing

### 5.1 `TI_VISIBLE_DEVICE`. Missing from both reports, both notes files, and every prior adversary file.

`taichi/rhi/vulkan/vulkan_loader.cpp:111-113`:

```
    const char *id = std::getenv("TI_VISIBLE_DEVICE");
    if (id) {
      set_vulkan_visible_device(id);
    }
```

It sits at the end of `VulkanLoader::init`, inside the `std::call_once` block.
The value is stored on the loader singleton (`vulkan_loader.h:32`,
`visible_device_id`) and consumed by
`vulkan_device_creator.cpp:442-456`, which parses it with `std::stoi` at `:445`,
range-checks against `device_count` at `:446` and logs
`"TI_VISIBLE_DEVICE=%d is not valid, found %d devices available"` at `:449` on
failure, then admits `devices[id]` at `:452-455` only if `get_device_score`
returns non-zero, falling through to the automatic pick at `:458-465` otherwise.

`grep -rn 'TI_VISIBLE_DEVICE'` over both reports, both notes files and all four
prior adversary files returns nothing. It is new.

**Why it matters, and it matters more than the C API parameter both reports do
cover.** Section 5.1a settles that configuration happens at install time, by an
agent outside the process, and that a multi-GPU machine is the ordinary case.
An environment variable read at loader init is exactly the shape of channel an
external install-time agent can use, with no C API call and no source change.
It is the second such channel in the tree, alongside `TI_LIB_DIR` in
`runtime_lib_dir()` (`taichi/util/lang_util.cpp:35-45`), which both reports do
cite for module location. Both reports discuss device selection solely as the
C API `device_index` parameter and conclude the gap is that four archs cannot
express a second device. The gap is real, but the picture is incomplete: on
Vulkan there are **two** routes in and both reports found one.

It also sharpens report A's escalation 12 and report B's escalation 10. The
question is not only "should any backend but Vulkan see more than device 0" but
"is an environment variable read at loader init the intended install-time
channel, and if so why does only one backend have one".

### 5.2 Report A omits `get_device_score` from its 4.8 census

Section 1, claim 5. Report B has it; report A's "detected and discarded" census
is incomplete without it, and it is the strongest instance on that spine.

### 5.3 Report B omits the section 2.2 caveat on Metal

Section 1, claim 6. Report B's 2.8 presents Metal as the model without noting
that its content is feature admission and that below Apple3 a device loses
`spirv_has_int64` outright. Plan section 6.3 now records that distinction as a
finding; report B does not carry it, so one half of the pair does not test it.

### 5.4 Neither report knows section 5.1a exists

`grep -n '5\.1a'` over both reports and both notes files returns nothing. Two
consequences.

- **Escalations the plan has closed are still open in both reports.**
  Report A escalation 16 asks whether 6.1's answer is one configured binary or N
  pre-sized variants, and names "what the variant filename carries" as one of two
  things to decide. Report B escalation 4 asks "whether the artefact naming needs
  to carry the ceiling" and escalation 17 asks which of section 5's two
  deployment routes 6.1 takes. Section 5.1a answers all three: "Per build
  already means per device configuration. A two-card machine gets two binaries,
  and the system activates one or both. **The filename scheme does not need to
  grow**, and no common denominator has to be found." What remains open is the
  sizing rule, which is section 8.1 item 2 and not a mechanism question.
- **The positive result in claim 2 above is unstated.** One ceiling per build is
  not merely permitted; it is what the existing loop produces, and the host
  assertion agrees with it by construction under install-time configuration.
  Report A gets within one step of this at escalation 16 ("Under install-time
  configuration the host and the chosen variant are fixed at the same moment"),
  and report B at 5.3.2 ("satisfied by construction if the host binary is
  configured at the same moment as the bitcode"). Neither closes it, because
  neither had the section that authorises closing it.

### 5.5 Report B's downgrade of multi-device selection

Section 1, claim 7. "Practical consequence for testing, not for architecture"
at `report-04b:430`, and "there is no gap in it to record" at its 8.1c item 2,
are wrong against section 5.1a.

### 5.6 Neither report reconciles the CUDA device-0 pin with the plan's empirical statement

Section 1, claim 7. The pin is on an ordinal, and the plan records a direct
observation that both cards run concurrently, and says that observation outranks
code reading. The two are consistent, but neither report says why, and the
finding as written invites the planner to weigh them against each other.

---

## 6. What remains before consensus

None of this needs new investigation. Five restatements and one small census.

1. **Both reports.** Remove the reference-path / accelerant framing at the 32
   lines enumerated in 3.1, and restate each conclusion on the current section
   5.2: neither spine is the vendor spine, no backend is privileged, and a gap
   in one spine is recorded as a gap and never as a ranking. Four escalations
   are affected — report A 13, report B 9, 11 and 20 — plus report A section 4.7,
   which currently contradicts itself, and report B `:956`, which attributes a
   ranking to the plan that the plan does not contain.
2. **Both reports.** Record `TI_VISIBLE_DEVICE`
   (`taichi/rhi/vulkan/vulkan_loader.cpp:111-113`, consumed at
   `vulkan_device_creator.cpp:442-456`) and fold it into the device-selection
   escalation. It is an out-of-process configuration channel and section 5.1a
   puts the configuration agent out of process.
3. **Both reports.** Restate the ceiling-mechanism escalations against section
   5.1a: report A 16, report B 4 and 17. The filename question is settled; state
   the positive result in claim 2 above and carry forward only the sizing rule,
   which belongs to section 8.1 item 2.
4. **Report A.** Add `get_device_score` (`vulkan_device_creator.cpp:203-229`,
   called `:452, :462`) to the 4.8 census.
5. **Report B.** Add the section 2.2 caveat on Metal's content to 2.8, matching
   report A 4.9 and plan section 6.3. Correct `:2680` to `:2681` in two places.
   Correct "a local that dies there" for the `:452` call site. Withdraw
   "practical consequence for testing, not for architecture" at 2.5 against
   section 5.1a.
6. **Both reports.** State that the CUDA pin is an ordinal pin, that Taichi
   reads no `CUDA_VISIBLE_DEVICES` (verified: zero hits across `taichi/`,
   `c_api/` and `python/`), and that the driver's own remapping is what makes
   the code consistent with the concurrency the plan records as observed.

---

## 7. Escalations

Judgements I am not authorised to make.

1. **Whether item 6.3's fidelity vocabulary should extend
   `rhi_constants.inc.h` or sit beside it.** Both reports escalate this
   (A-19, B-8, B-19) and both are right to. I add one constraint neither states
   as sharply as it deserves: the file's own header comment at `:1-7` invites
   CUDA compute capability as an entry while excluding API versions, so the
   author's intended boundary is "properties of the on-device program" and a
   fidelity level may or may not fall inside it. That is a reading of intent,
   not of behaviour, and it is not mine to settle.

2. **Whether Metal's existing ladder is itself a section 2.2 violation to be
   fixed, or upstream behaviour to be preserved and worked around.** Section 2.2
   presents itself as "a TEST to apply to every proposed change"
   (`PROJECT-PLAN.md:112`). Applying a test written for proposed changes to
   existing upstream code is a step past what the section authorises, and
   nothing in the plan says whether inherited behaviour is in its scope. Report
   A's 4.9 records the distinction and declines to resolve it, which I judge
   correct. Recording that the question is open, not answering it.

3. **Whether the amendment passes should be re-run or the reports amended in
   place.** Both reports' framing passes were correct against the plan text they
   were given and are wrong against the plan as it stands, and the same is true
   of `adversary2-04-1.md`, which issued the instruction. Whether the planner
   wants the superseded framing struck, or kept visible with its retraction the
   way the other corrections in both reports are kept, is a decision about the
   record rather than about the source. Both reports have so far kept every
   withdrawn claim legible below its replacement, which has worked well; I would
   note only that this is the first case where the plan changed under a correct
   report rather than the report being wrong, and that distinction is worth
   preserving in whatever wording is chosen.

4. **Whether `get_device_score` gating an explicitly requested Vulkan device at
   `vulkan_device_creator.cpp:452` is intended.** A device named by
   `TI_VISIBLE_DEVICE` or by the C API `device_index` is silently discarded if
   its score is zero, and the automatic pick runs instead with no message on
   that branch — the only error log, at `:449`, covers the out-of-range case
   only. Under section 5.1a an install-time agent naming a device would expect
   either that device or a failure, not a silent substitution. Whether this is a
   defect or deliberate fallback the code does not say, and I am recording it
   rather than grading it.
