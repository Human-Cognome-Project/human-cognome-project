# Adversary 2, 04-1 — round two, backend, AOT and build architecture

Judging the revised `report-04-backend-build.md` (pass A) and
`report-04b-backend-build.md` (pass B) against the source, and judging whether my
own round one objections in `adversary-04-1.md` were actually met.

Method: I re-opened every file I cite. I did not accept agreement between the two
passes as evidence for anything. Where the two passes now agree, I re-derived the
fact from source before crediting it. Where a report ruled against me, I went to
source rather than defending my earlier text; on one of those I was wrong, and on
one of my round one corrections I was wrong, and both are recorded below with
their citations.

---

## 1. Verdicts up front

| Question | Verdict |
|---|---|
| Is pass A now **correct**? | Yes, with three corrections. None inverts a finding. One is my own round one error propagated into A. |
| Is pass B now **correct**? | Yes, with three corrections. None inverts a finding. |
| Is pass A now **complete**? | Yes for its own coverage, except the shared gap in section 5. |
| Is pass B now **complete**? | No, by B's own statement, and defensibly so. Two areas remain uncovered. |
| Is the **pair** complete? | No. One gap survives both, and it sits on item 6.1, the item this territory owns the mechanism for. |
| Was every round one objection addressed? | Yes, except two that B declined on scope grounds and disclosed. |
| **General consensus reached?** | **Not yet.** Close. Section 5 says precisely what remains. |

The revisions are serious work. I checked roughly seventy citations across the
two revised reports, including every census either report claims to have re-run,
and the great majority land exactly. Both passes withdrew claims that were false
and neither substituted a softer version of the same claim. The remaining
problems are three corrections, one shared imprecision of mechanism, and one gap.

---

## 2. Every round one objection, and whether it was addressed

### 2.1 My objections to pass A (`adversary-04-1.md` §2, §3)

| Objection | Status |
|---|---|
| `Extension::data64` is not a compile-time blocker | **Addressed.** Withdrawn at report-04 §9.1, replaced by §4.1. I re-verified: `is_extension_supported` has six call sites, `taichi/program/program.cpp:149` (`assertion`), `taichi/codegen/llvm/codegen_llvm.cpp:2726` (`bls`), `taichi/transforms/compile_to_offloads.cpp:92, 205, 218, 236` (`mesh`), `:245, 288` (`quant`), plus the pybind export at `taichi/python/export_lang.cpp:1225`. No site passes `data64`. |
| `ti_device_api_shared` "contains every enabled backend" | **Addressed**, and A adopted the stronger refutation. `taichi/rhi/dummy.cpp` is 0 bytes; `ti_device_api_shared` appears at exactly `taichi/rhi/CMakeLists.txt:114, 115, 120` and nowhere else in the tree. |
| `set_caps` count | **Addressed.** Eight sites, five in the RHI and three in the C API. My grep over `taichi/` and `c_api/` with no file-type filter returns exactly A's table. |
| `archs.inc.h` has 12 entries, `TiArch` names 7 of 12 | **Addressed.** `taichi/inc/archs.inc.h:4-17` is 12 entries. |
| `taichi/aot/` is 7 files | **Addressed.** 7 files, 836 lines; `wc -l` confirms each figure in A §5. |
| `HOST_ARCH` refutes the non-boolean claim | **Addressed** at report-04 §2.6. |
| `runtime_lib_dir` closes at `:47` | **Addressed.** `taichi/util/lang_util.cpp:30-47`. |
| Nine `-D` append sites, seven `#else` arms | **Addressed.** I count nine at `cmake/TaichiCore.cmake:94, 103, 109, 115, 119, 123, 127, 131, 281` and four redundant at `taichi/rhi/CMakeLists.txt:43, 49, 55, 61`. |
| `taichi_core_impl.h:121-122`, `version.h.in` 5 lines, `cuda_context.cpp:79-81`, `:88-92`/`:94-98`, `public_device.h:426-428`/`:430-434` | **All addressed and all five of my line numbers were right.** Re-verified individually. |
| A missed `spirv_has_physical_storage_buffer`, the third selection chain, the c_api producers, `ProgramImpl::get_device_caps`, the CUDA device-0 pin, the AMDGPU ISA selection | **All addressed**, at report-04 §4.3, §3.2, §4.2, §4.2, §3.3, §5.3. |
| S1 no handshake / S2 `CUDA_VERSION` / S3 no cache invalidation / S4 CMake precedence / S5 capability ordering | **All addressed**, at §5.2, §2.4, §5.2, §2.8, §4.4. |

### 2.2 My objections to pass B (`adversary-04-1.md` §4)

| Objection | Status |
|---|---|
| Metal does call `set_caps`; "exhaustive" was false | **Addressed**, with the root cause named (a grep passing `--include=*.cpp --include=*.h` and no `--include=*.mm`). B removed the word "exhaustive" throughout and states the scope searched. |
| B omits the `Extension` system | **Acknowledged, not addressed.** B §8.4 declines to re-derive it and says so. See 4.1. |
| B omits the installed-package surface | **Addressed**, at report-04b §4.4, which is now the better of the two treatments. |
| B omits the DX12 stub | **Acknowledged, not addressed.** Same disclosure. |
| B cites the dead `taichi/system/dynamic_loader.h` | **Addressed** at §4.1. |
| "first non-boolean project-level build parameter" is false | **Addressed** at §5.4, and B went further than the correction required. See 3.3. |
| `arch_is_gpu` classifies js explicitly | **Addressed.** `taichi/rhi/arch.cpp:43` names js; B §1.2 now says so. |
| `get_total_memory` is on `LlvmDevice`, not the RHI base | **Addressed** at §2.5. I re-verified `taichi/rhi/public_device.h` has no such member and `taichi/rhi/llvm/llvm_device.h:34` declares it. |
| `ti_get_available_archs` is `:171-205`; `TiCapability` is `:380-407`; SPIR-V link gate `:295-298`; `cuda_context.cpp:21` and `:22`; `USE_STDCPP` | **All addressed.** |
| GGUI `target_compile_definitions` at `:383` | **Correctly refused.** I was wrong. See 3.3. |

### 2.3 Adversary 04-2's objections

I checked these too, since consensus is joint. Every item in 04-2 §3 and §4 is
now either corrected or, for the two scope declinations, disclosed. All five of
04-2's §6 gaps are now in both reports: the umbrella header and `TI_WITH_CPU`
(A §3.5, B §4.4), the single `required_caps` producer (A §5.1, B §3.3), the
capability ordinals keying the offline cache (A §4.6, B §2.6), the `.bc`
artefacts already landing in the source tree (A §2.5, B §5.3), and the `-I` route
not being a clean substitution (A §2.5, B §5.3).

---

## 3. Where the explore agents ruled against round one — adjudicated

### 3.1 Capability equality. A's adjudication is right on the source. I concede.

**The source question, settled.** The documentation text is exactly as both now
quote it, `c_api/include/taichi/taichi_core.h:409-412`:

```
// Structure `TiCapabilityLevelInfo` (1.4.0)
//
// An integral device capability level. It currently is not guaranteed that a
// higher level value is compatible with a lower level value.
```

A's reading is correct and mine was not. "Not guaranteed" constrains what a
consumer may *rely on*; it does not fix the implementation's relation at `!=`. My
round one sentence — "`!=` is the only relation the documented type supports" —
overstated the text. 04-2's framing, that it is a plain defect, is equally
unsupported, because the disclaimer is real and both reports had to reckon with
it. A's "neither adversary was right" is the correct verdict and I adopt it.

**But A's supporting census is wrong, and it is a claimed grep.** A §4.4 and
notes entry 34 state "**nine** ordered comparisons" and notes entry 34 says "I
grepped every comparison on `spirv_version`". There are **thirteen**:

| Operator | Sites |
|---|---|
| `<` | `taichi/codegen/spirv/spirv_ir_builder.cpp:556, 625, 671, 679, 741, 777`; `taichi/rhi/vulkan/vulkan_device_creator.cpp:577` |
| `>` | `taichi/codegen/spirv/spirv_codegen.cpp:1908` |
| `>=` | `taichi/codegen/spirv/spirv_ir_builder.h:424`; `taichi/codegen/spirv/spirv_codegen.cpp:2669, 2671, 2673, 2675` |

A missed the four-arm ladder at `taichi/codegen/spirv/spirv_codegen.cpp:2666-2676`,
which is the single strongest instance in the tree: it reads `spirv_version` once
at `:2666` and walks `>= 0x10600`, `>= 0x10500`, `>= 0x10400`, `>= 0x10300` down
to a default, selecting `spv_target_env` for the whole module. That is not an
incidental comparison, it is a graded ladder over the value. Pass B cites
`spirv_codegen.cpp:2673`, one of the four A missed, so between them the pair has
the fact and only A's count is wrong.

The direction matters: the error **strengthens** A's own conclusion, so nothing
downstream inverts. But it is a `[V]`-marked census, backed by a stated grep,
inside the section A wrote specifically to adjudicate a dispute. That is the same
shape of error round one sent A back for.

**Which half is which.** The source question is settled: the contract declines to
promise ordering, and the engine assumes ordering for `spirv_version` in thirteen
places. The contract question is open and belongs to the project owner: whether
`c_api/src/taichi_gfx_impl.cpp:25` should keep `!=`, move to `>=`, or become
per-capability. Both reports place it in escalations. Neither resolves it. That
is correct handling.

One fact neither report states and which bears on that decision: the check is
moot while `get_required_caps()` is never overridden, so the relation is not
observable today on any path. Both reports establish the second half; neither
draws the consequence that the relation could be decided later at no cost.

### 3.2 `spirv_has_physical_storage_buffer`. Verified. It does change the shape of 6.2, on one spine only.

**The census holds exactly.** Grep over the tree excluding `external/` returns
two producers, both compiled out:

- `taichi/rhi/vulkan/vulkan_device_creator.cpp:826`, inside
  `#if !defined(__APPLE__) && false` opened at `:825` and closed at `:827`.
  Comment `:824` "(penguinliong) Temporarily disabled (until device capability is
  ready)", issue reference `:822-823`, and the enclosing hardware guard
  `if (device_supported_features.shaderInt64)` at `:821`. Every one of A's line
  numbers here is exact.
- `c_api/src/taichi_vulkan_impl.cpp:48`, inside a `/* */` block spanning `:46-51`,
  preceded at `:45` by "(penguinliong) Will bring it back after devcap."

And twelve consumers: `taichi/rhi/vulkan/vulkan_device.cpp:1772, 1792, 2147, 2509`;
`taichi/codegen/spirv/spirv_ir_builder.cpp:73, 113`;
`taichi/codegen/spirv/spirv_codegen.cpp:783, 2340, 2416, 2491`;
`taichi/runtime/gfx/runtime.cpp:96`;
`taichi/runtime/program_impls/gfx/gfx_program.h:81`.

**Is it "finish and enable" rather than "build"?** On the SPIR-V spine, yes, and
I say so having read what the consumers do rather than counting them:

- `spirv_ir_builder.cpp:73-77` emits `OpCapability PhysicalStorageBufferAddresses`.
- `spirv_ir_builder.cpp:113-120` emits the `SPV_KHR_physical_storage_buffer`
  extension and switches the module's memory model to
  `AddressingModelPhysicalStorageBuffer64`.
- `spirv_codegen.cpp:783-792` loads a `u64` address out of the args buffer and
  does 64-bit pointer arithmetic on it, against the 32-bit path above it.
- `spirv_codegen.cpp:2340` threads `has_buffer_ptr` into `translate_ti_type`, so
  the whole argument type translation branches on it.

That is a working 64-bit-pointer path, not a stub. So for item 6.2 on
vulkan/opengl/gles/dx11/metal the accurate framing is finishing and enabling
existing machinery, and A's summary point 4 and B's §2.4 both say so.

Two qualifications neither report makes, both of which the planner needs:

1. **It is a different axis from the item 6.2 the brief's target hardware needs.**
   Brief 5.2's three tiers are NVIDIA cards, which run the CUDA/LLVM path. None
   of this code is on that path. A's escalation 13 asks whether the SPIR-V
   pointer path is in scope at all, which is the right question, but neither
   report states plainly that the 64-bit work for the named tiers shares nothing
   with it.
2. **The false branch is not permanent.** See 4.1.

### 3.3 B's `CUDA_VERSION` distinction. The fact holds; the reason B gives is wrong.

**B's fact is correct.** `taichi/runtime/llvm/runtime_module/runtime.cpp` includes
`taichi/inc/constants.h` at `:25` and nothing else that could carry a configure
value; it does not include `taichi/common/version.h`, whose only three includers
in the tree are `taichi/common/core.cpp:7`, `taichi/util/offline_cache.h:12` and
`taichi/runtime/llvm/llvm_offline_cache.cpp:14`. `CUDA_VERSION` is read by host
C++ at `taichi/common/core.cpp:87-89` and used at
`taichi/runtime/llvm/llvm_context.cpp:213-218` to build a filename. So the
precedent is selection between prebuilt modules, and nothing in the tree compiles
a configure-time value into a `.bc`. B is right.

**But B's reason for saying the ceiling needs the other half is wrong, and the
same wrong reason is in pass A.** Both say host and device would disagree on the
runtime struct layout:

- report-04 §2.5: "host C++ and device bitcode would disagree on the size of
  `element_lists`, `node_allocators` and `ambient_elements`".
- report-04b §5.3: "Host and device would disagree on the runtime struct layout,
  silently."

The host has no layout for that struct. `LLVMRuntime` is **forward-declared
only** in host code — `taichi/program/context.h:11` and
`taichi/rhi/llvm/llvm_device.h:8`, both `struct LLVMRuntime;` — and is used as an
opaque pointer (`context.h:18`, `llvm_device.h:14`). The host obtains its LLVM
type from the loaded bitcode module at `taichi/runtime/llvm/llvm_context.cpp:1008`
and `taichi/codegen/llvm/codegen_llvm.cpp:2697`, and reads fields through
accessors compiled inside the bitcode, for example
`runtime_query<void *>("LLVMRuntime_get_element_lists", ...)` at
`taichi/runtime/llvm/llvm_runtime_executor.cpp:328`, generated by
`STRUCT_FIELD_ARRAY(LLVMRuntime, element_lists)` at
`taichi/runtime/llvm/runtime_module/runtime.cpp:616` and `:750`.

The host's **only** use of the constant is the assertion at
`taichi/codegen/llvm/struct_llvm.cpp:266` — which the plan's own section 6.1
correction already records does not guard those arrays, because it bounds a
per-tree count while the arrays are indexed by the global `SNode::id`.

So the real failure of a host-only `-D` is: the assertion permits more SNodes
while the bitcode's arrays stay at the old size, and the overflow happens inside
the bitcode at `runtime.cpp:1005`, `runtime->element_lists[i] = ...`. That is
still silent, still a defect, and the conclusion both reports draw — the value
must reach the clang invocation at
`taichi/runtime/llvm/runtime_module/CMakeLists.txt:8` — survives intact and is in
fact strengthened, because the arrays exist *only* in the bitcode. But the stated
mechanism would send a reader looking for an ABI or serialisation mismatch that
does not exist.

**And this reopens the route B closed.** Brief section 5 allows "either by custom
compilation or by activating the correct prebuilt binary". Because the host
carries no struct layout, a set of prebuilt `runtime_<arch>.bc` variants each
compiled with a different ceiling is much closer to reachable than B's framing
implies: the selection half is exactly the `get_runtime_fn` pattern at
`taichi/runtime/llvm/llvm_context.cpp:209-211`, already in use, and the loop that
would have to produce the variants is
`taichi/runtime/llvm/runtime_module/CMakeLists.txt:29`. The one host-side coupling
that would have to move is the bound at `struct_llvm.cpp:266`. Neither report
reaches this, and it is the fork in the road that item 6.1 actually faces.

I am not proposing either route. I am recording that the pair's analysis
forecloses one of the two routes the brief permits, on a mechanism the source
does not support.

### 3.4 `taichi/rhi/CMakeLists.txt:17`. Verified, both reports correct.

The line reads, verbatim:

```
if (TI_WITH_OPENGL OR TI_WITH_VULKAN AND NOT ANDROID)
```

CMake's `if` precedence is parentheses, then unary tests, then comparisons, then
`NOT`, then `AND`, then `OR`, so this parses as
`TI_WITH_OPENGL OR (TI_WITH_VULKAN AND (NOT ANDROID))`. With OpenGL on and
`ANDROID` set, the body still runs: `target_compile_definitions` at `:28`,
`add_subdirectory` for GLFW at `:29`, `target_link_libraries` at `:30` and the
public include path at `:31-34`, closing at `:35`. Every line number in both
reports is exact. Both correctly mark the build outcome as not executed, and both
correctly note that whether Android is a target is unsettled.

### 3.5 The AOT handshake. Verified independently, not by accepting agreement.

**Version.** `aot::Module::version()` is pure virtual at
`taichi/aot/module_loader.h:85`. A grep for `version()` across `taichi/` and
`c_api/` excluding `external/` returns the declaration and exactly three
overrides — `taichi/runtime/gfx/aot_module_loader_impl.cpp:114`,
`taichi/runtime/dx12/aot_module_loader_impl.cpp:67`,
`taichi/runtime/llvm/llvm_aot_module_loader.h:36` — plus unrelated names
(`k_api_version`, `mz_version`, `ti_get_version`, `simplify_nested_conversion`).
**No caller.**

**Arch.** `aot::Module::arch()` is pure virtual at `module_loader.h:84`. A grep
for `->arch()` and `.arch()` across `taichi/` and `c_api/` returns ten sites, and
**every one is on a `CompiledKernelData`, not a module**: `program.cpp:193`,
`codegen/compiled_kernel_data.cpp:140`, `codegen/spirv/compiled_kernel_data.cpp:28`,
`codegen/llvm/compiled_kernel_data.cpp:46`, `runtime/cpu/kernel_launcher.cpp:72`,
`runtime/llvm/llvm_aot_module_builder.cpp:73`, `runtime/llvm/kernel_launcher.cpp:12`,
`runtime/amdgpu/kernel_launcher.cpp:154`, `runtime/cuda/kernel_launcher.cpp:191`,
`compilation_manager/kernel_compilation_manager.cpp:225`. **No caller on a
module.**

**Nothing recorded.** `taichi/runtime/gfx/aot_utils.h:23` is
`TI_IO_DEF(kernels, fields, required_caps, root_buffer_size)`. No arch, no
version.

**Capability.** `get_required_caps` returns exactly two hits across the tree
excluding `external/`: the default body at `taichi/aot/module_loader.h:97-100` and
the call site at `c_api/src/taichi_gfx_impl.cpp:21`. No override.

So the three-part absence is confirmed on independent evidence. Both reports have
it and both frame it correctly.

One citation slip shared by both: they say the gfx `TI_IO_DEF` is "mirrored at
`taichi/aot/module_data.h:135`". It is not a mirror — `module_data.h:135` reads
`TI_IO_DEF(kernels, kernel_tmpls, fields, required_caps, root_buffer_size)`, with
`kernel_tmpls` that the gfx struct lacks. The load-bearing point, that neither
records an arch, holds for both.

---

## 4. Newly wrong or still missing

### 4.1 Both reports: "permanently" and "never on any path" are false. The C API can turn the capability on.

This is the one substantive correction I have against both.

- report-04 §4.3: "**Twelve live consumers**, all permanently taking the false
  branch."
- report-04b §2.4: "is **never set on any path, on any platform**" and "twelve
  live consumers, all of which therefore permanently take the false branch."

`ti_set_runtime_capabilities_ext` (`c_api/src/taichi_core_impl.cpp:317-334`) builds
a `DeviceCapabilityConfig` from caller-supplied `TiCapabilityLevelInfo` values at
`:326-330` and installs it wholesale with `runtime2->get().set_caps(...)` at
`:331`. The C++ wrapper even ships a typed setter for this exact capability at
`c_api/include/taichi/cpp/taichi.hpp:1156`. That set lands on the same
`Device::caps_` (`taichi/rhi/public_device.h:617`) that
`GfxProgramImpl::get_device_caps` reads at
`taichi/runtime/program_impls/gfx/gfx_program.cpp:82-85` and that every one of the
twelve consumers reads. A host that calls it before compiling kernels puts all
twelve on the true branch.

Both reports cite the override elsewhere without reconciling it: A lists it as
`set_caps` site 8 in its §4.2 table and raises it as escalation 11; B lists it in
its §2.2 table and describes it in §4.3. Neither noticed that it falsifies the
absolute in its own §4.3 / §2.4.

This is not pedantry about a word. It bears directly on A's escalation 13 and B's
escalation 9, both of which ask whether 6.2 on the SPIR-V spine means reviving
the disabled producers. There is a third answer: the path can be exercised today
from outside the core, with no source change, which is how one would find out
whether the machinery works before deciding to revive it. Neither report offers
the planner that option because both wrote the branch off as permanent.

The accurate statement is: no in-tree producer sets it, both are compiled out, and
the only way it is set today is a host calling the C API override.

### 4.2 Pass A: `cmake/TaichiCore.cmake:383` is wrong, and it is my error propagated.

report-04 §2.1 says "GGUI instead uses `target_compile_definitions` at
`TaichiCore.cmake:383`", repeated in the §6 build-gating map. Source:

```
383:     if(TI_WITH_GGUI)
384:         target_compile_definitions(${CORE_WITH_PYBIND_LIBRARY_NAME} PRIVATE -DTI_WITH_GGUI)
385:     endif()
```

The call is at `:384`. `:383` is the guard. Pass B had `:384` in round one and
kept it; I "corrected" B to `:383` in `adversary-04-1.md` §2.8 and §4.8, and pass
A adopted my error while pass B correctly did not. The mistake is mine, the
report that carries it is A's, and B was right both times.

I re-checked the other four line corrections I gave A in round one, and all four
are right: `c_api/src/taichi_core_impl.h:121` comment and `:122` array;
`taichi/common/version.h.in` is 5 lines; `taichi/rhi/cuda/cuda_context.cpp:79-81`
shared memory, `:88-92` and `:94-98` for total and free memory;
`taichi/rhi/public_device.h:426-428` `dispatch` and `:430-434` `ComputeSize`.

### 4.3 Pass A: the ordered-comparison count, section 3.1 above.

Nine claimed, thirteen present. Recorded here because it is a correction, not
only an adjudication.

### 4.4 Pass B: the re-run censuses check out, but one replacement claim does not.

The test the planner asked for. B claims at §2.2 that re-running its other censuses
without the `--include` filter changes nothing "since no `.mm` file contains those
symbols". I checked. The tree has four `.mm` files —
`c_api/src/taichi_metal_impl.mm`, `taichi/rhi/metal/metal_device.mm`,
`taichi/ui/ggui/gui_metal.mm`, `taichi/ui/ggui/nswindow_adapter.mm` — and a grep
over all four for `get_required_caps`, `spirv_has_physical_storage_buffer`,
`taichi_max_num_snodes`, `DynamicLoader`, `spirv_has_atomic_int64` and
`required_caps` returns nothing. **B's claim is true and its censuses stand.** I
re-derived each of those censuses myself with no file-type filter and got B's
numbers.

The one that does not hold is B's replacement for the census I objected to.
B §2.3 says the `spirv_has_atomic_int64` grep "returns four lines", listing
`taichi/inc/rhi_constants.inc.h:17`, `c_api/include/taichi/taichi_core.h:388`,
`c_api/include/taichi/cpp/taichi.hpp:1121-1122` and `python/taichi/lang/enums.py:27`.
A case-sensitive grep returns **three**: the enumerator at
`taichi_core.h:388` is `TI_CAPABILITY_SPIRV_HAS_ATOMIC_INT64`, upper case, and
does not match the string B says it greps for. Pass A's §4.2 states three hits and
is right. The substantive point — no `set` and no `get` anywhere — is correct in
both.

### 4.5 Pass B: the capability level tally is one short.

B §3.3 tallies "29 sites pass `true`, 10 pass `1`, and 13 pass a `spirv_version`
value". My count over `taichi/` and `c_api/` including `.mm`: 29 `true`, 10 `1`,
and **14** `spirv_version` sites — `taichi/program/program.cpp:536`,
`taichi/rhi/metal/metal_device.mm:1045`,
`taichi/rhi/vulkan/vulkan_device_creator.cpp:526, 528, 530, 532, 578`,
`taichi/rhi/opengl/opengl_device.cpp:528`, `taichi/rhi/dx/dx_device.cpp:564`,
`taichi/rhi/vulkan/vulkan_device.cpp:1579`, `c_api/src/taichi_opengl_impl.cpp:11`,
`c_api/src/taichi_vulkan_impl.cpp:38, 40, 42`. B's conclusion — no capability
written today would be mishandled by `>=` — is unaffected and I confirm it: every
level in the tree is a boolean stored as `true` or `1`, or a packed
`spirv_version`, and `>=` is the correct relation for both kinds.

### 4.6 Minor, recorded for the file

- report-04 §2.2 cross-references "see 2.5" for where `HOST_ARCH` is set. It is
  in §2.6. Internal reference only.
- report-04b §2.5 gives the shared-memory attribute as `cuda_context.cpp:79-82`;
  the call is `:79-81` and `:82` is blank. A has it right.
- Both call `taichi/aot/module_data.h:135` a mirror of
  `taichi/runtime/gfx/aot_utils.h:23`; the field lists differ by `kernel_tmpls`
  (section 3.5).
- report-04 §4.3 marks the section "**[V]** throughout", which covers the closing
  judgement "a materially different starting position for 6.2". That is a
  conclusion, not a reading. B marks the same conclusion **[I]** at §2.4 and adds
  "I verified the code is unreachable; I did not verify it is correct", which is
  the correct discipline. On a method that runs on the `[V]`/`[I]` split this is
  worth a line, though it is nothing like the round one pattern.

---

## 5. What remains, precisely

Consensus is not reached. Three things stand between here and it. I have kept
this list to what I can cite.

**R1. Both reports must retract "permanently" and "never on any path"** for
`spirv_has_physical_storage_buffer` (report-04 §4.3, report-04b §2.4), because
`c_api/src/taichi_core_impl.cpp:317-334` sets device capabilities wholesale at
`:331` and both reports cite that function elsewhere. The corrected statement is
that no in-tree producer sets it and the C API override is the only way it is set
today. This changes what A's escalation 13 and B's escalation 9 are asking.

**R2. Both reports must correct the mechanism behind the 6.1 threading trap**
(report-04 §2.5, report-04b §5.3). The host does not lay out `LLVMRuntime`: it is
forward-declared at `taichi/program/context.h:11` and
`taichi/rhi/llvm/llvm_device.h:8`, typed from the loaded bitcode at
`taichi/runtime/llvm/llvm_context.cpp:1008`, and read through in-bitcode accessors
(`taichi/runtime/llvm/llvm_runtime_executor.cpp:328`). The host's only use of
`taichi_max_num_snodes` is the assertion at
`taichi/codegen/llvm/struct_llvm.cpp:266`. The conclusion each report draws is
unchanged; the reason given for it is not the reason the source supports.

**R3. The gap that survives both.** Following from R2: because the host carries no
struct layout, the brief's second deployment route — several prebuilt bitcode
variants, selected at install time — is materially closer to reachable than either
report allows, using the existing selection pattern at
`taichi/runtime/llvm/llvm_context.cpp:209-211` and the existing production loop at
`taichi/runtime/llvm/runtime_module/CMakeLists.txt:29`. Pass B forecloses this at
§5.2 and §5.4 on the wrong ground; pass A does not consider it at all. Item 6.1 is
the item this territory owns the mechanism for, and the pair currently presents
one route where the brief permits two.

**Corrections that do not block consensus** but should go in: A's
`cmake/TaichiCore.cmake:383` (correct value `:384`, and the error is mine, 4.2);
A's nine-versus-thirteen ordered comparisons (3.1); B's `spirv_has_atomic_int64`
line count (4.4); B's `spirv_version` tally (4.5); the small items in 4.6.

**Pass B remains incomplete alone**, by its own §8.4: the `Extension` system
(`taichi/program/extension.{h,cpp}`, `taichi/inc/extensions.inc.h`) and the DX12
availability stub (`taichi/rhi/dx12/dx12_api.cpp:6-16`). B's reason — that copying
the paired agent's findings would defeat the point of a blind pair — is sound
under section 9.1, and A covers both correctly, so the **pair** is complete on
these. If the planner's bar is "either report readable unaccompanied", B does not
clear it and says so.

---

## 6. What I tried to break and could not

Everything below I re-derived from source in this round, not from the reports.

- The eight `set_caps` sites, with no file-type filter:
  `taichi/rhi/metal/metal_device.mm:1168`,
  `taichi/rhi/vulkan/vulkan_device_creator.cpp:876`,
  `taichi/rhi/vulkan/vulkan_device.cpp:1580`,
  `taichi/rhi/opengl/opengl_device.cpp:529`, `taichi/rhi/dx/dx_device.cpp:565`,
  `c_api/src/taichi_opengl_impl.cpp:12`, `c_api/src/taichi_vulkan_impl.cpp:53`,
  `c_api/src/taichi_core_impl.cpp:331`, declaration at
  `taichi/rhi/public_device.h:855`. Both tables exact.
- Metal's real hardware queries: `[mtl_device supportsFamily:]` at
  `taichi/rhi/metal/metal_device.mm:1027-1036`,
  `feature_64_bit_integer_math = family_apple3` at `:1038`, consumed at `:1051-1053`,
  and the MSL 2.3 selection at `:132`. Both correct.
- The C API discarding OpenGL's GLES-aware detection:
  `taichi/rhi/opengl/opengl_device.cpp:509-513` under `!is_gles()` with the comment
  at `:510`, plus `:515-518`, `:520-522`, `:524-526`, `:528`, `:529`; then
  `c_api/src/taichi_opengl_impl.cpp:4-13` rebuilding a config and setting
  `spirv_has_int64` unconditionally at `:9`. A's account and A's scope caveat about
  `TI_ARCH_GLES` are both right.
- `taichi_max_num_snodes` at exactly three places and `kMaxNumSnodeTreesLlvm` at
  exactly two, with no file-type filter and no directory exclusions beyond
  `external/` and `modernization/`. Both reports exact.
- The `CUDA_VERSION` chain end to end: `cmake/TaichiCore.cmake:98-100`,
  `CMakeLists.txt:149-152`, `taichi/common/version.h.in:5`, `CMakeLists.txt:203`,
  `taichi/common/core.cpp:87-89`, `taichi/runtime/llvm/llvm_context.cpp:213-218`.
  Both exact. `.gitignore:63-64` does list both generated headers and neither
  exists in this checkout, so B's §5.2 point stands.
- The bitcode build: the single `-I ${PROJECT_SOURCE_DIR}` at
  `taichi/runtime/llvm/runtime_module/CMakeLists.txt:8`, the TODO at `:9`, the
  `WORKING_DIRECTORY` at `:10`, the `${arch}`/`${rtm_arch}` mismatch between `:13`
  and `:8`, the loop at `:29-31`, the commented-out CUDA rule at `:36-43`.
  Both exact.
- Eleven `option()` calls at `cmake/TaichiCore.cmake:1-11` and 24 across the
  build, all boolean. A's count of 24 is exact.
- The offline-cache coupling: `taichi/analysis/offline_cache_util.cpp:88-95`
  serialising `caps.devcaps` at `:92`, called at `:185`, hashed at `:190`, and
  `taichi_max_num_indices` at `:110`. Both exact.
- The version-only cache gate at `taichi/util/offline_cache.h:95-100`, inherited
  by `taichi/runtime/llvm/llvm_aot_module_loader.h:25`. Both correct.
- `required_caps` has exactly one write in the tree,
  `taichi/runtime/gfx/aot_module_builder_impl.cpp:23`; the generic
  `aot::ModuleData::required_caps` at `taichi/aot/module_data.h:125` is written by
  nobody. Both correct.
- `str2devcap` at `taichi/rhi/device_capability.cpp:6-13` is the exact inverse of
  `to_string` at `:15-27`, and `DeviceCapabilityConfig::get` returns 0 for a
  missing key at `:33-38`. Both correct.
- The DX12 stub: `taichi/rhi/dx12/dx12_api.cpp:6-12` returning `true` on the
  preprocessor alone, `make_dx12_device()` returning `nullptr` at `:14-16`, file
  length 26 lines. A exact.
- `ti_create_runtime` rejecting a non-zero device index at
  `c_api/src/taichi_core_impl.cpp:270, 277, 283, 289, 297`, with Vulkan alone
  honouring it at `:254-255`, and `taichi/rhi/cuda/cuda_context.cpp:21-22` pinning
  device 0. Both correct.
- `ti_device_api_shared` at exactly `taichi/rhi/CMakeLists.txt:114, 115, 120`,
  `taichi/rhi/dummy.cpp` 0 bytes. A exact.
- `TI_WITH_CPU` at `c_api/include/taichi/taichi.h:21, 23` and
  `docs/lang/articles/deployment/tutorial.md:288`, defined nowhere. A's version,
  which includes the docs hit, is the more complete of the two.
- `taichi/inc/constants.h` is included in this territory only by
  `taichi/rhi/llvm/allocator.h:6`, and there is no hardcoded `1024` duplicate of
  the ceiling anywhere in it. A's negative claim holds.
- `cupti_toolkit.cpp` uses `#if CUDA_VERSION >= 11040` at `:263`, `:502`, `:725`,
  and does not include `taichi/common/version.h`. A's escalation 2 hazard is
  correctly stated.

---

## 7. Escalations

Unresolved. Each needs a decision the project owner has not made. I resolve none
of them, and I do not restate the escalations already carried in the two reports.

1. **Is the C API capability override the intended way to exercise the disabled
   64-bit pointer path?** Per 4.1, `c_api/src/taichi_core_impl.cpp:331` lets a host
   install any capability set, including
   `spirv_has_physical_storage_buffer`, with no validation against the device, and
   `c_api/include/taichi/cpp/taichi.hpp:1156` ships a typed setter for it. That
   makes the twelve consumers reachable today without touching either disabled
   producer. Whether that is a supported evaluation route, an install-time
   override hook, or a hazard to be closed, the code does not say. This is the
   sharper form of A's escalation 11 and it changes A's escalation 13 and B's
   escalation 9.

2. **Which of the two routes in brief section 5 is item 6.1 taking?** Per 3.3 and
   R3, the host has no `LLVMRuntime` layout, so both routes are open: one ceiling
   compiled into one bitcode set via a generated `taichi/inc/constants.h`, or
   several prebuilt bitcode variants selected at install time through the existing
   `get_runtime_fn` pattern (`taichi/runtime/llvm/llvm_context.cpp:209-211`). The
   second route additionally requires deciding where the host's bound at
   `taichi/codegen/llvm/struct_llvm.cpp:266` comes from, since that is the only
   host-side coupling. The plan records no choice, and the pair's current analysis
   presents only the first.

3. **What relation should gate module compatibility, and per capability?** Per 3.1
   the source question is settled and the contract question is not. The engine
   assumes ordering for `spirv_version` in thirteen places; the published contract
   at `c_api/include/taichi/taichi_core.h:411-412` declines to promise it; and the
   relation at `c_api/src/taichi_gfx_impl.cpp:25` is unobservable today because
   `get_required_caps` is never overridden. That last point means the decision can
   be deferred at no cost, which is worth the planner knowing.

---

## 8. Divergence from adversary2-04-2

`adversary2-04-2.md` appeared after I finished sections 1 through 7, which are
unchanged. I read it and re-opened every file it cites that I had not already
opened. This section records where we diverge and which of us the source
supports.

### 8.1 Where we agree, independently

We reached the same two verdicts by different routes: both reports are now
correct subject to a short list of count-and-grep corrections, the pair is not
yet complete, and consensus is not reached. Neither of us calls for a re-run.

We independently found the same three things and neither of us took them from
the reports:

- **Pass A's "nine ordered comparisons" is thirteen**, and the four missing are
  `taichi/codegen/spirv/spirv_codegen.cpp:2669, 2671, 2673, 2675` inside the
  block at `:2666-2679` that selects `spv_target_env`. 04-2 §5.1, my 3.1. We
  even single out the same block for the same reason.
- **The capability-ordering adjudication.** We both concede that pass A's ruling
  is right and that our round one positions overreached in opposite directions.
  Mine read a prohibition into a non-guarantee; 04-2's accepted the reports'
  defect framing without having seen `c_api/include/taichi/taichi_core.h:411-412`.
- **B's re-run censuses hold.** We both tested rather than accepted, both
  enumerated the same four `.mm` files, and both got zero hits for every symbol
  B claims to have re-checked.

We also both flag B's `spirv_has_atomic_int64` "four lines" as a fresh
imprecision, both flag the tally at report-04b §3.3 as not matching the tree, and
both note that 6.2 divides along `arch_uses_spirv` (`taichi/rhi/arch.cpp:63-66`)
into a finish-what-exists half and a build-from-nothing half, with the brief's
5.2 tiers all on the second.

### 8.2 Where 04-2 is right and I missed it. Four items, each verified before conceding.

**1. The clang line already compiles a configure-time value into the bitcode.
This is the strongest single finding in either round-two file, and it is 04-2's.**

`taichi/runtime/llvm/runtime_module/CMakeLists.txt:8` carries
`-D "ARCH_${rtm_arch}"`. I verified the whole chain myself:

- The values are configure-time: `HOST_ARCH` at `cmake/TaichiCXXFlags.cmake:149`,
  `CUDA_ARCH` at `CMakeLists.txt:155`, `AMDGPU_ARCH` at `:159`, `DX12_ARCH` at
  `:163`.
- They are iterated at `runtime_module/CMakeLists.txt:29`.
- They are consumed **inside the bitcode**: 19 occurrences of `ARCH_` in
  `taichi/runtime/llvm/runtime_module/runtime.cpp`, at exactly the lines 04-2
  lists, plus `locked_task.h:7` and `node_pointer.h:15`. I counted them.
- One artefact per value at `:8`, installed at `:13`, selected at run time by
  `get_runtime_fn` at `taichi/runtime/llvm/llvm_context.cpp:209-211`.

I also confirmed 04-2's negative claim: `-D "ARCH_` appears in the two reports
and two notes files only inside the quoted command block, three times total.
Neither pass discusses it.

This falsifies pass B's "Nothing in the tree yet compiles a configure-time value
*into* the bitcode" (report-04b escalation 1) and narrows pass A's "Only one
route is sound" (report-04 escalation 1). 04-2's caveat is also right: `-D` on
the `constexpr` name itself would produce `constexpr int 4096 = 1024;` and fail
to compile, so the route needs a macro name that `taichi/inc/constants.h` reads.

I did not find this. It is a better version of the territory my own R3 covers,
and I adopt it.

**2. The "not guaranteed" sentence is authored documentation, generated into the
header.** Verified: `c_api/docs/taichi/taichi_core.h.md:281` carries the sentence
under the key `structure.capability_level_info` at `:279`; `misc/taichi_json.py:260-268`
reads `c_api/docs/{name}` into `Documentation.api_refs`; `misc/generate_c_api.py:50-55`
emits those lines as `//` comments, which is how they reach
`c_api/include/taichi/taichi_core.h:411-412`. Pass B has this at its §3.3 and
04-2 traced the direction. I treated the header comment as the primary text. The
consequence 04-2 draws is right and belongs in the escalation: changing the
relation without changing the `.md` leaves the shipped documentation
contradicting the shipped behaviour.

**3. `runtime.cpp` is not in the host source list.** `cmake/TaichiCore.cmake:77-91`
globs `"taichi/runtime/*.h" "taichi/runtime/*.cpp"` non-recursively, so
`taichi/runtime/llvm/runtime_module/runtime.cpp` never enters `TAICHI_CORE_SOURCE`.
Verified. This composes with my 3.3 rather than competing with it, and 04-2 got
there by a route I did not take.

**4. Two small ones.** B's `TI_ARCH_METAL` case range at report-04b §1.1 begins
at `:294`, which is `#endif  // TI_WITH_LLVM`; the `#ifdef TI_WITH_METAL` is at
`:295` and the case at `:296`. Pass A's `:295-301` is right. And
`taichi/inc/rhi_constants.inc.h:1-7` does say that "DirectX shader model, CUDA
compute capability and Vulkan physical device features can be listed here"; pass
B cites it, pass A does not, and it strengthens A's escalation 7. I had not read
those seven lines.

### 8.3 Where I have material 04-2 does not. Three items.

**1. The false branch is not permanent, and neither adversary file but this one
says so.** Per my 4.1, `ti_set_runtime_capabilities_ext`
(`c_api/src/taichi_core_impl.cpp:317-334`) installs a caller-supplied capability
set wholesale at `:331`, onto the same `Device::caps_` at
`taichi/rhi/public_device.h:617` that all twelve
`spirv_has_physical_storage_buffer` consumers read, with a typed setter shipped
at `c_api/include/taichi/cpp/taichi.hpp:1156`. 04-2 §3.2 re-verifies the census
in full, counts the twelve consumers individually, and does not challenge either
report's "permanently" or "never set on any path, on any platform". Both reports
are wrong on that word and both cite the override elsewhere without reconciling
it. This is a live third answer to A's escalation 13 and B's escalation 9: the
path can be exercised today from outside the core with no source change.

**2. The host has no `LLVMRuntime` layout, and this is where 04-2 and I actually
diverge rather than merely differ in coverage.** 04-2 §3.3 reaches the adjacent
fact (item 3 above) and then writes that the constant the host was compiled
against is "guarding the write at `struct_llvm.cpp:266`". It does not guard the
write. The plan's own section 6.1 correction records that
`taichi/codegen/llvm/struct_llvm.cpp:266` bounds a per-tree SNode count while the
arrays are indexed by the process-global `SNode::id`, and I re-read the line to
confirm it is `TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);`.

The rest of my 3.3 stands as material 04-2 lacks: `LLVMRuntime` is
forward-declared only in host code (`taichi/program/context.h:11`,
`taichi/rhi/llvm/llvm_device.h:8`), its LLVM type is taken from the loaded
bitcode (`taichi/runtime/llvm/llvm_context.cpp:1008`,
`taichi/codegen/llvm/codegen_llvm.cpp:2697`), and its fields are read through
accessors compiled inside the bitcode
(`taichi/runtime/llvm/llvm_runtime_executor.cpp:328`, generated by
`STRUCT_FIELD_ARRAY` at `runtime.cpp:616, 750`). So both reports' "host and
device would disagree on the struct layout" is not the mechanism, and the actual
failure is an out-of-bounds write inside the bitcode at `runtime.cpp:1005`.

The source supports me on the assertion, and the plan supports me explicitly.

**3. Pass A's `cmake/TaichiCore.cmake:383` for the GGUI
`target_compile_definitions`, which is at `:384`.** 04-2 does not have it. It is
my own round one error, adopted by A and correctly refused by B, and I have
recorded it as mine at 4.2.

### 8.4 One correction to my own section 4.5

04-2's tally is the precise one and mine undercounted by one. Recounting
`\.set\((taichi::lang::)?DeviceCapability::` across `taichi/` and `c_api/`
including `.mm`: **54 sites, 30 passing `true`, 10 passing `1`, 14 passing a
`spirv_version` value.** My "29 true" in 4.5 was a text-line count that missed
`c_api/src/taichi_vulkan_impl.cpp:48-49`, where the call wraps and `true` sits on
the second line. 04-2's 30/10/14 is right. My "14 spirv_version" stands, and
pass B's 29/10/13 is short on two of the three figures. B's conclusion — that no
capability written today would be mishandled by `>=` — is unaffected and both of
us confirm it independently.

### 8.5 Net, and what consensus now requires

Neither round-two adversary file is complete alone, which is the same signal the
explore pair produces one level down. 04-2 has four items I lack and one of them,
the `-D` mechanism, is the most consequential finding in either file. I have
three it lacks, one of which is a direct correction to it.

Taken together, consensus requires five things, none of them a re-run:

1. **Both reports absorb the `-D "ARCH_${rtm_arch}"` mechanism** (04-2 §4). Pass
   B withdraws "Nothing in the tree yet compiles a configure-time value into the
   bitcode"; pass A withdraws "Only one route is sound". Escalation 1 in each
   becomes a choice among three routes.
2. **Both reports retract "permanently" and "never on any path"** for
   `spirv_has_physical_storage_buffer` (my 4.1).
3. **Both reports correct the mechanism behind the 6.1 threading trap** (my 3.3,
   R2), and neither should describe `struct_llvm.cpp:266` as guarding the array
   write, which 04-2 also does.
4. **Pass A corrects nine to thirteen ordered comparisons** (both of us), and
   corrects `cmake/TaichiCore.cmake:383` to `:384` (mine).
5. **Pass B states the exclusions behind its capability tally and fixes the
   `spirv_has_atomic_int64` grep description and the `TI_ARCH_METAL` range**
   (both of us).

Items 1, 2 and 3 all bear on item 6.1 and on the first escalation of each
report, so they should go back together.

---

## 9. Addendum: the amended section 5.2, and what it changes

The plan's section 5.2 has been amended. I re-read it. The three cards are
capability classes, not a vendor target; their all sitting on one vendor's path
is incidental and must not shape an architectural decision; section 2 governs;
the reference path is the portable one and a vendor path is an optional
accelerant; and where a vendor path is better served than the portable one, that
is a gap in the portable path to be recorded.

This does not overturn a single verified fact in sections 1 through 8 above. It
inverts the weight of several of them, including two of my own conclusions. It
also promotes one item from a footnote to a finding, and it surfaces a gap that
neither report, neither round-one adversary, nor 04-2 nor I had, because all six
documents were reading toward the vendor path.

### 9.1 My own conclusions that leaned on the old wording

**My 3.2, qualification 1, is restated.** I wrote that the switched-off
`spirv_has_physical_storage_buffer` machinery "is a different axis from the item
6.2 the brief's target hardware needs" and that "the 64-bit work for the named
tiers shares nothing with it". The facts hold; the weighting was backwards.
`arch_uses_spirv` is `{opengl, gles, vulkan, dx11, metal}`
(`taichi/rhi/arch.cpp:63-66`), which is the portable path. Real 64-bit pointer
machinery written, compiled and switched off on the reference path is now the
**primary** form of item 6.2, not a side axis. The CUDA/LLVM half is the
accelerant half.

**My 8.1 agreement with 04-2 is restated.** We both observed that 6.2 divides
along `arch_uses_spirv` into a finish-what-exists half and a
build-from-nothing half, and we both noted "the brief's 5.2 tiers all sit on the
second" as though that settled which half matters. It settles nothing. The
division is real; the priority runs the other way.

Nothing else in sections 1 through 8 moves. In particular R1, R2, R3 and the
five-item consensus list in 8.5 are unaffected, since none of them rests on which
vendor the target cards come from.

### 9.2 Statements in the two reports that must now be restated

These are framing corrections, not factual ones. Every underlying citation is
sound. Each of these sentences directs the planner toward the vendor path on the
strength of the old wording.

| Report | Statement | Why it must be restated |
|---|---|---|
| report-04b §2.2 | "**This is the central gap for brief section 5.2.** The target tiers are Maxwell, Pascal and Ampere NVIDIA cards. Those run on the CUDA (LLVM) backend, which has no capability representation at all." | The capability vocabulary is the portable path's own mechanism and it is populated there. That the vendor path lacks one is a gap on the accelerant. Calling it central rests entirely on the old wording. |
| report-04b §4.2 | "For the LLVM family, which is where the brief's named target cards live, this is where adaptive module loading plausibly starts, not at `aot::Module::load`." Marked **[I]**. | This is the one place either report makes a recommendation, and the amendment inverts it. Under section 5.2 as amended the portable AOT path is the reference and the per-arch `.bc` path is the accelerant. B's inference should be withdrawn or re-argued on grounds that do not use the card list. |
| report-04 §4.2 | "there is no device-capability record whatsoever on the CUDA path, **which is the path the section 5.2 target tiers are about**." | The clause after the comma is now false. The fact before it stands. |
| report-04 §5.3 and escalation 7 | The AMDGPU ISA-version selection at `taichi/runtime/llvm/llvm_context.cpp:625-655` offered as "the closest existing precedent for what 6.3 asks" and as a possible model. | It is a vendor-specific mechanism on a vendor-specific path. Under the amendment it is to be recorded as a place the vendor path is better served than the portable one, which is a gap in the portable path, not a model to copy. See 9.3 for what the portable model actually is. |
| report-04 escalation 7, report-04b escalation 8 | Both ask how the 5.2 tiers should be expressed, and both name only vendor-specific candidates (CUDA compute capability, the AMDGPU ISA string, the header comment at `taichi/inc/rhi_constants.inc.h:1-7` suggesting CUDA compute capability be listed as a capability). | The question is now how capability classes are expressed **portably**, with the vendor value as one optional input. Extending the capability vocabulary with a CUDA-specific entry is the shape the amendment warns against. |
| report-04b escalation 11 | The `sm_` clamp at `taichi/rhi/cuda/cuda_context.cpp:75-77` framed as "where upward scaling stops" for the brief's tiers. | Still true as a fact about the CUDA path. It is a limit on the accelerant, not on the project's scaling. |

For completeness, and because consensus is joint: adversary2-04-2 §3.2 concludes
"on the spine the brief actually targets, this finding is inert and 6.2 remains a
build-from-scratch question", and its escalation 3 rests on the same reading.
Both invert under the amendment. Its §4 finding, the `-D` mechanism, is
unaffected and remains the strongest item in either round-two file.

### 9.3 What the amendment surfaces that nobody had

The amendment asks for gaps in the portable path to be recorded as such. Working
that instruction against the source produced one finding I did not have, and
promoted three that were sitting in the reports as caveats.

**9.3.1 The portable path already queries tier data and throws it away.** This is
new. Both reports establish the negative — no memory size reaches the C API,
`get_total_memory` is not on the RHI base — and neither noticed that the data is
in hand on the portable side:

- `taichi/rhi/vulkan/vulkan_device.cpp:2515-2516` calls
  `vkGetPhysicalDeviceMemoryProperties`, then uses the result only to build
  external-memory handle-type flags at `:2518-2531`. The heap sizes are never
  read.
- `taichi/rhi/vulkan/vulkan_device.cpp:1596` stores a full
  `VkPhysicalDeviceProperties` into `vk_device_properties_`
  (`taichi/rhi/vulkan/vulkan_device.h:739`, accessor `:710`). Its only two
  consumers are `vulkan_device.cpp:1125-1130`, which checks
  `limits.maxComputeWorkGroupCount` inside `dispatch`, and `:1945`, which uses
  `limits.timestampPeriod` for profiling.
- `taichi/rhi/vulkan/vulkan_device_creator.cpp:504-518` reads the device name and
  API version; the name is written to a debug log and discarded, and the API
  version becomes `spirv_version` at `:523-533`.
- `taichi/rhi/opengl/opengl_api.cpp:231-236` queries
  `GL_MAX_COMPUTE_WORK_GROUP_COUNT` and `GL_MAX_COMPUTE_WORK_GROUP_SIZE` into two
  file-scope globals declared at `:20-21` with a hardcoded 1024 fallback.

None of it reaches `DeviceCapabilityConfig`, the RHI `Device` base — which
exposes only `arch()` and the capability map at
`taichi/rhi/public_device.h:851-857` — or the C API.

So the accurate statement of the 6.3 gap is not "the portable path has no tier
information while CUDA does". It is that **both paths detect tier information and
neither publishes it**, the CUDA path keeping it in `CUDAContext` and the Vulkan
and OpenGL paths keeping it in backend-private state or discarding it outright.
That is a materially smaller and better-shaped piece of work than inventing a
vendor vocabulary, and it is what the amendment asks to be recorded.

**9.3.2 Metal is the portable-shaped precedent for capability classes, and both
reports have the lines without offering it as one.** `taichi/rhi/metal/metal_device.mm:1027-1036`
issues real `supportsFamily` queries, `:1038-1042` reduces them to five named
feature flags including `feature_64_bit_integer_math`, and `:1044-1053` writes the
result into a `DeviceCapabilityConfig`. That is hardware generation classes
detected at run time and expressed in the portable capability vocabulary, which
is exactly what escalation 7 of pass A and escalation 8 of pass B ask for. Both
reports cite these lines for the narrower point that Metal queries hardware for
64-bit integers. Under the amendment this is the model to look at, in place of
the AMDGPU ISA string.

**9.3.3 GLES is named everywhere and reachable nowhere through the C API.**
`TI_ARCH_GLES = 7` exists at `c_api/include/taichi/taichi_core.h:373`;
`aot::Module::load` has a gles arm at `taichi/aot/module_loader.cpp:40-43`; the
`Program` constructor has one at `taichi/program/program.cpp:124-130`. But
`ti_create_runtime` (`c_api/src/taichi_core_impl.cpp:247-309`) has no gles case,
and the six availability probes at `:13-61` have no gles probe, so
`ti_get_available_archs` can never report it either. GLES is the lowest-end
portable target in the tree. Pass A found this and used it correctly but only as a
scope caveat limiting its own OpenGL claim (report-04 §4.2); pass B lists gles
among the omissions from `ti_get_available_archs` without connecting it to the
missing constructor. Under the amendment it is a gap in the portable path in its
own right.

**9.3.4 Two portable-path defects both reports carry as hygiene questions.**

- The C API OpenGL runtime sets `spirv_has_int64` unconditionally at
  `c_api/src/taichi_opengl_impl.cpp:9`, replacing the caps that
  `GLDevice::GLDevice` had just built, and dropping both the `!is_gles()` guard at
  `taichi/rhi/opengl/opengl_device.cpp:509-513` and the extension-gated int16 and
  float16 detection at `:515-526`. Both reports describe it accurately. Under
  section 5.3, which says modern capability is an opportunity and never a
  prerequisite, this is the C API asserting a capability the low-end portable
  device may not have.
- `TI_WITH_CPU` is referenced at `c_api/include/taichi/taichi.h:21, 23` and
  defined nowhere, so `taichi_cpu.h` is unreachable through the public umbrella
  header. Both reports have it and both frame it as a naming question. Section 2
  makes CPU-only a first-class target, so this is the fallback-of-last-resort path
  being unreachable through the public header.

### 9.4 Effect on my verdicts

**Correct: unchanged.** Both reports remain correct. Nothing in 9.2 is a false
statement about the source; they are conclusions drawn on a premise the plan has
now withdrawn.

**Complete: unchanged in verdict, longer in remedy.** The pair was already
incomplete. It now has 9.3.1 as an additional gap, and 9.3.2 through 9.3.4 as
material both reports hold but file under the wrong heading.

**Consensus: still not reached.** The five items in 8.5 stand, and I add one:

6. **Both reports must be reread against the amended section 5.2 and their
   vendor-path framing restated**, per the table in 9.2. Pass B's §4.2 inference
   about where adaptive module loading starts is the one that has to be withdrawn
   or re-argued rather than reworded, because it is a recommendation and the
   amendment inverts it.

### 9.5 Escalations, revised

My escalation 3 in section 7 is unchanged. Escalations 1 and 2 there are
unchanged. I add two, and I flag one of my earlier ones as now under-specified.

4. **How are capability classes to be expressed portably?** This supersedes the
   vendor-shaped form of pass A's escalation 7 and pass B's escalation 8. The
   material is: Metal already expresses hardware generation classes as
   capabilities (9.3.2); Vulkan and OpenGL detect equivalent data and drop it
   (9.3.1); CUDA detects it and keeps it in `CUDAContext`; and the capability
   vocabulary at `taichi/inc/rhi_constants.inc.h:8-35` is entirely SPIR-V, whose
   own header comment at `:1-7` invites vendor entries. Whether the answer is to
   publish what the portable backends already detect, to add class capabilities in
   the Metal shape, or something else, is the project owner's decision. Adding a
   CUDA-specific entry is the shape the amended 5.2 warns against, and I record
   that without deciding it.

5. **Is the C API's arch surface required to cover the portable archs it names?**
   Per 9.3.3, `TI_ARCH_GLES` is declared and has no constructor and no probe;
   dx11, dx12 and amdgpu have neither. Under the amendment gles is not an
   incidental omission. Whether the proof-of-concept stage in section 5.1 requires
   it now, or only requires the stub to be correctly shaped, is not settled.

Under-specified, flagged rather than rewritten: my escalation 2 in section 7 asks
which of the two routes item 6.1 takes. Both routes I described, and the third
route 04-2 found, are on the LLVM bitcode path, because that is where the SNode
ceiling's arrays live (`taichi/runtime/llvm/runtime_module/runtime.cpp:567-569`).
That is a property of where the constant is used and not a vendor preference, so
the amendment does not disturb it. I record the check rather than leaving the
reader to wonder whether it was made.
