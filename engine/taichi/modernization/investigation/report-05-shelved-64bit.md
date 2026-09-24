# Report 05 (pass A) — Why upstream shelved the partial 64-bit work

Territory: `shelved-64bit`. Repo at fork baseline `ba0e81dce`.
Contemporaneous notes: `modernization/investigation/notes-05-shelved-64bit.md`.

Sources are tagged **[LOCAL]** (this git repository) or **[GH]**
(github.com/taichi-dev/taichi, read via `gh`, read-only).

**Plan version worked against**, per standing instruction 6:
`modernization/PROJECT-PLAN.md`, **"Last updated: 2026-09-09"**, 60988 bytes,
md5 `73106c01879a06b83ca5ccb6fdc891d9`. Re-read in full at the start and end of
this pass. It has changed substantially since the version the earlier passes of
this report worked against, and three of those changes bear on this territory.
They are set out in section 0a.

**AMENDED 2026-09-09, second adversarial round**, against `adversary-05-1.md`
and `adversary-05-2.md`. Marked **[AMEND N]**, adjudicated in notes 25 to 35.

**AMENDED AGAIN 2026-09-09, third adversarial round**, against
`adversary2-05-1.md` and `adversary2-05-2.md`. Marked **[AMEND3 N]**,
adjudicated in notes 37 to 43. Every claim was re-derived from source before
being accepted; where an adversary is wrong I say so with lines.

**The largest change in the second pass was [AMEND 1].** The feature is not
disabled. It is un-defaulted, and two live producers reach the untested branch
today.

**The largest change in the third pass is [AMEND3 1], and it corrects this
report rather than an adversary.** The previous pass ruled the `at_buffer`
pointer-width collision blocking but NOT architectural, on the ground that a
`ValueKind` tag could replace the width sniff if populated at the production
sites. **That ruling is withdrawn.** The four assertions it rested on are each
true and I re-verified all four, but the inference fails: `make_value` at
`taichi/codegen/spirv/spirv_ir_builder.h:290-298` computes the result tag from
the SPIR-V result type alone and never reads an operand tag, so every arithmetic
instruction resets it. The `ir_->add` at `spirv_codegen.cpp:372` destroys any tag
a production site wrote, and every SNode place pointer that reaches `at_buffer`
has passed through it. Both round-two adversaries settled this against me
independently and the source is on their side. Section 7.7 item F.

---

## 0a. [AMEND3 0] What changed in the plan, and what it does to this report

Standing instruction 6 requires this and the changes are not cosmetic.

**1. Section 10 now DEFINES architectural against environmental, and the
definition governs this territory.** The test is: *if every external thing were
ideal today, would the obstacle still be there?* ENVIRONMENTAL if it exists
because of something outside this codebase; ARCHITECTURAL if it exists because of
a decision inside it about how something is represented or structured. And, in
terms: **"Architectural does not mean hard, and environmental does not mean easy.
Grade the KIND first, then state the BLAST RADIUS separately and concretely, in
files and call sites."**

That rule did not exist when section 7 of this report was written. Section 7
graded against a definition of its own, and item F withheld the label because the
remedy looked small. **That is a blast-radius argument used as a kind argument,
which the rule now forbids.** Section 7.7 item F is re-graded accordingly.

*A numbering correction, recorded because three documents cite it wrongly and the
plan warns its own numbers move.* The lead's brief, `adversary2-05-1.md` section
2 and `adversary2-05-2.md` section 1 all cite this rule as **section 10 item 7**.
In the version named above it is **item 8**. Item 7 is the verified-citation
rule, item 6 is the living-document rule, and item 9 is the enumeration rule. An
item was inserted ahead of them and pushed the list down; the territory 06 and 07
documents, written later, cite item 8 and are right. I cite the rule by content
as well as by number throughout, because the number is not stable.

**2. Section 2.2a now puts item 6.2 in scope as BASE work, in full.** Verbatim:
"Item 6.2, 64-bit addressing, in full. This includes the shelved SPIR-V path and
the `at_buffer` pointer-width collision. The shelved work covers 64-bit
addressing for everything EXCEPT SNodes, so it is the other half of the same
limit. Raising the object count while addresses stay 32-bit moves the wall rather
than removing it. **The two halves are not separable.**"

This does not falsify anything in section 1.1a: no code that runs still connects
the capability to the SNode root buffer, and that remains the finding. It does
change what follows from it. Section 0's line that the shelved work "would not
deliver project item 6.2" was a statement about reusability and it stands as
that; it must not be read as saying the shelved work is outside item 6.2, because
the plan now says the opposite. Section 6 is amended.

**An unresolved tension inside the plan, escalated rather than resolved.**
Section 2.3 still reads: "including the shelved 64-bit addressing work, which is
stub material under section 5.1 rather than a path to the ceiling." Section 2.2a
says the two halves are not separable and that raising the count alone moves the
wall. Read as priority against scope the two are compatible; read on substance
they are not, because one says the shelved work is not a path to the ceiling and
the other says the ceiling is not reachable without it. I am not authorised to
pick. Escalation 17.

**3. Section 8.2 item 0 now records this territory's own question as RESOLVED by
territory 07**, including a measured `maxStorageBufferRange` table and a blast
radius of "11 sites in 2 files, 7 requiring an edit". Both bear on section 6 item
6 and on section 7.7 item F, and both are folded in below.

---

## 0. Summary answer

The brief treats four items as one shelved 64-bit programme. **They are four
unrelated things with four unrelated histories**, and only one of them was ever
automatically enabled.

| Piece | What actually happened | Category |
|---|---|---|
| `spirv_has_physical_storage_buffer` | Shipped and worked for eight months, then un-defaulted in two steps in Oct/Nov 2022. **Still reachable on demand.** | Automatic enablement withdrawn pending an AOT capability-negotiation rebuild that never finished. Not removed, not gated behind any device check. |
| `use_64bit_pointers` | Never true, on any branch, ever. Placeholder from birth in Oct 2021 | Never started |
| `snode.cpp` int32 index warning | Deliberate, current, documented design limit. Widening was argued for in 2021 and never done | Never started; momentum lost |
| `PointerType::addr_space_` | Dead field since Oct 2020, about address *space* not address *width* | Not part of this at all |

**The single most important finding is that the physical-storage-buffer work
does not address SNode addressing width.** It exists to pass ndarray and
external-array base addresses to shaders as pointers instead of descriptor
bindings. It never touched the SNode root buffer in any code that runs. So even
fully restored, it would not deliver project item 6.2.

**[AMEND 1] The second most important finding, which this report previously got
backwards: the capability is not off. It is un-defaulted.** A caller can compile
a physical-storage-buffer module today, on any machine, through the C++
ahead-of-time module interface with an explicit capability list, with no device
check anywhere in the chain. Section 1.10. The consequence is that the
zero-coverage branch of section 1.8 is not a dormant subsystem awaiting a
decision. It is a live path.

**[AMEND 3] A third finding, which qualifies the "never touched SNodes" claim
above without overturning it.** Forty lines of dead SNode type-construction code
in the tree would give sparse `pointer` SNode cells physical device addresses.
It was committed already commented out and never ran. It is the only artefact in
the territory connecting the capability to SNodes, and it contradicts the intent
half of the convergent claim while leaving the behaviour half intact.
Section 1.11.

**On whether the blockers have expired** (section 7): the environmental ones
largely have. Measured on this box, both target GPUs and the CPU software
rasteriser satisfy every runtime precondition the code tests. **[AMEND 4] The
module-wide addressing model is not an obstacle at all**: upstream shipped eleven
tagged releases under it with mixed addressing, fourteen on non-Apple builds.
**[AMEND3 1] Of the reasons that remain, four are architectural under the plan's
grading rule and four are environmental**, section 7.0. The permanent count did
not fall; its membership changed.

One caveat cuts the other way: our SPIR-V toolchain submodules are pinned within
three weeks of the shelving commit, so fixes made upstream since November 2022
are **not** present in what we have.

---

## 1. `spirv_has_physical_storage_buffer`

### 1.1 What it is for — VERIFIED [LOCAL] + [GH]

Origin PR **#4221** `7705f688a`, Bob Cao, 2022-02-08, "[vulkan] Add buffer
device address (physical pointers) support & other improvements". Its stated
motive [GH, PR body]: "Codegen is modified so that ext arrays and ndarrays can
be addressed directly instead of using bindings. This also removes an shifting
because the pointers are byte addressed. This also means aliasing is not needed
if buffer device address is supported." Its parent issue is **#3807 "[RFC]
Support ndarray for vulkan backend"** [GH].

### 1.1a [AMEND 2] The consumer inventory, rebuilt from scratch — VERIFIED [LOCAL]

Both adversaries state that this report's previous list omitted three consumers.
**Both are right.** The list was rebuilt from scratch rather than patched, by
grepping the whole tree for the capability name, then following the
`bool has_buffer_ptr` value it is converted into, then reading every hit.

Every count below is derived from the rows beneath it, per plan section 10
item 7.

**Group A. Direct readers of `DeviceCapability::spirv_has_physical_storage_buffer`.**

| # | Site | What it gates |
|---|---|---|
| 1 | `taichi/rhi/vulkan/vulkan_device.cpp:1772` | `VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR` on buffer creation |
| 2 | `taichi/rhi/vulkan/vulkan_device.cpp:1792` | `vkGetBufferDeviceAddressKHR` on an owned allocation |
| 3 | `taichi/rhi/vulkan/vulkan_device.cpp:2147` | `vkGetBufferDeviceAddress` on an imported allocation |
| 4 | `taichi/rhi/vulkan/vulkan_device.cpp:2509` | `VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT` |
| 5 | `taichi/codegen/spirv/spirv_ir_builder.cpp:73` | `OpCapability PhysicalStorageBufferAddresses` |
| 6 | `taichi/codegen/spirv/spirv_ir_builder.cpp:113` | `OpExtension SPV_KHR_physical_storage_buffer` and the `OpMemoryModel` choice |
| 7 | `taichi/codegen/spirv/spirv_codegen.cpp:783` | `visit(ExternalPtrStmt*)`, array base address as `u64` |
| 8 | `taichi/codegen/spirv/spirv_codegen.cpp:2340` | `compile_args_struct` |
| 9 | `taichi/codegen/spirv/spirv_codegen.cpp:2416` | `compile_argpack_struct` |
| 10 | `taichi/codegen/spirv/spirv_codegen.cpp:2491` | `compile_ret_struct` |
| 11 | `taichi/runtime/gfx/runtime.cpp:96` | host writes `get_memory_physical_pointer(...)` into the args buffer |
| 12 | `taichi/runtime/program_impls/gfx/gfx_program.h:81` | argument layout string `"1b"` versus `"1-"` |

Twelve rows, twelve direct readers. This matches the count both adversaries
reached independently.

**Group B. Indirect consumers, reached through the `has_buffer_ptr` boolean.
All three were missing from this report's previous list.**

| # | Site | What it does |
|---|---|---|
| 13 | `taichi/codegen/spirv/spirv_types.cpp:484-497` | `translate_ti_type`. The function sites 8, 9 and 10 all call. A Taichi `PointerType` becomes `IntType(64)` at `:491-493` or `IntType(32)` at `:494-496`. **This is where the width decision is actually made.** |
| 14 | `taichi/runtime/gfx/runtime.cpp:823` and `:851-859` | `get_struct_type_with_data_layout_impl`. Decodes `layout[1] == 'b'` at `:823` and sizes a `PointerType` member `sizeof(uint64_t)` at `:853` or `sizeof(uint32_t)` at `:857`. **This is the host half of the same ABI**, and it is what makes section 7.7 item D concrete. Callers: `taichi/program/callable.cpp:161` and `:182`, `taichi/program/program.cpp:494`. |
| 15 | `taichi/codegen/spirv/spirv_ir_builder.cpp:334-342` | `IRBuilder::from_taichi_type`. A **second, independent implementation** of the same pointer-width rule, `t_uint64_` at `:339` or `t_uint32_` at `:341`. It is **dead**: `grep -rn from_taichi_type` returns its definition at `:334`, its own recursive call at `:347`, and the declaration at `spirv_ir_builder.h:343`, and nothing else. |

Three rows, three indirect consumers. Group A plus Group B is fifteen sites.

Site 15 matters for item 6.2 beyond bookkeeping: anyone widening the pointer
rule has **two** copies to change, and one of them compiles and does nothing.

**Group C. The declaration and exposure surface.** Not consumers, listed so the
inventory is closed.

- `taichi/inc/rhi_constants.inc.h:28` — the enumeration entry.
- `c_api/include/taichi/taichi_core.h:399` — `TI_CAPABILITY_SPIRV_HAS_PHYSICAL_STORAGE_BUFFER = 18`.
- `c_api/include/taichi/cpp/taichi.hpp:1156-1159` — typed C++ setter.
- `python/taichi/lang/enums.py:38` — the name as a Python string constant.

**Nothing in Group A or Group B touches the SNode root buffer.** VERIFIED. The
only `ptr_to_buffers_` assignment naming `BufferType::Root` is
`spirv_codegen.cpp:377`, inside `visit(GetChStmt*)`, and no capability is read
on the way to it. The SNode path runs through `make_pointer`
(`spirv_codegen.cpp:2317-2324`), whose four call sites are `:355`
(`GetRootStmt`), `:371` (`GetChStmt`), `:408` (`bitmasked_activation`) and
`:506` (`SNodeLookupStmt`), and which reads `use_64bit_pointers`, never the
capability.

The RHI plumbing sites 1 to 4 add `VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR`,
call `vkGetBufferDeviceAddressKHR`, and set the VMA device-address flag. The
codegen sites 5 and 6 emit `OpCapability PhysicalStorageBufferAddresses`,
`OpExtension SPV_KHR_physical_storage_buffer`, and switch the module's
`OpMemoryModel` from `AddressingModelLogical` to
`AddressingModelPhysicalStorageBuffer64`.

### 1.1b [AMEND 2] The capability does not widen array *indexing* — VERIFIED [LOCAL]

Correction to this report's own previous claim that the capability "addresses
ndarray and external-array arguments." It delivers less than that, and both
adversaries caught it.

In `visit(ExternalPtrStmt*)`, `spirv_codegen.cpp:734-800`, the whole index
computation is `i32`. `linear_offset` is initialised `i32` at `:737`,
accumulated by `mul` and `add` at `:770-771`, and byte-shifted at `:773-777`
with an `i32` result type. Only at `:790` is it `OpSConvert`-ed to `u64` and
added to the `u64` base loaded at `:789`.

So the capability buys a 64-bit **base address** plus a 32-bit **signed byte
offset**. It does not lift a per-array indexing limit. This makes the
conclusion in section 6 stronger, not weaker.

### 1.2 It is a whole-module switch — VERIFIED [LOCAL], consequence CORRECTED

`spirv_ir_builder.cpp:113-127`. The addressing model is one module-wide
declaration with no per-pointer opt-in.

**[AMEND 4] The consequence this report previously drew from that fact is
wrong, and it was wrong in a way that mattered.** See section 7.7 item E for
the full adjudication. In short: the declaration is permissive, not exclusive;
a module here is one offloaded task, not a program; and upstream shipped eleven
tagged releases in which the physical addressing model was declared while SNode
roots were 32-bit offsets into descriptor-bound buffers. The sentence "turning
the capability on changes the SPIR-V validity rules for every shader in the
module" is withdrawn.

### 1.3 Timeline of the change — VERIFIED [LOCAL]

| Commit | PR | Date | Author | Effect |
|---|---|---|---|---|
| `7705f688a` | #4221 | 2022-02-08 | Bob Cao | Introduced |
| `06cb1c50d` | #4244 | 2022-02-08 author, 2022-02-09 commit | Bob Cao | Gated on `shaderInt64` |
| `07446dd0c` | #6184 | 2022-10 | — | Device capability refactor moved the call |
| `f677fb141` | #6415 | 2022-10-24 | Yi Xu | Excluded macOS, `#if !defined(__APPLE__)` |
| `d20f55dc8` | #6468 | 2022-10-30 | PENGUINLIONG | Un-defaulted the automatic producer, `&& false` |
| `12546578a` | #6494 | 2022-11-02 | PENGUINLIONG | Commented out the second automatic producer in `c_api/src/taichi_vulkan_impl.cpp` |

**[AMEND 1] Current state of the two automatic producers.** Both are
compiled-out, and neither is a device-independent statement that the feature is
off.

- `taichi/rhi/vulkan/vulkan_device_creator.cpp:826` — the `caps.set(...)` call,
  inside `#if !defined(__APPLE__) && false` at `:825`, `#endif` at `:827`. The
  surrounding runtime conditions at `:814-821` still evaluate; only the setter
  is compiled out.
- `c_api/src/taichi_vulkan_impl.cpp:48-49` — the setter, inside a `/* */`
  comment block spanning `:46-51`, with the explanatory line at `:45`. This is
  a comment, not a preprocessor guard. (Correction to the brief, which described
  both as preprocessor-guarded.)

**Neither of these is the whole picture. See section 1.10.**

### 1.3a [AMEND 8] The release census, enumerated — VERIFIED [LOCAL]

The two adversaries diverge here. Adversary 1 reports eight tags shipped
enabled, starting at v1.0.0. Adversary 2 reports eleven, starting at v0.9.0.
**Adversary 2 is right and adversary 1 is three tags short.**

Settled by reading `vulkan_device_creator.cpp` out of every tag in the
repository, falling back to the pre-refactor path
`taichi/backends/vulkan/vulkan_device_creator.cpp`, and classifying by the
preprocessor guard immediately above the `caps.set` line. The repository holds
**121 tags**, not 122. Twenty-four of them contain the capability.

| State | Tags | Count |
|---|---|---|
| File present, capability absent | v0.8.7, v0.8.8, v0.8.9, v0.8.10, v0.8.11 | 5 |
| **ON, no preprocessor guard** | v0.9.0, v0.9.1, v0.9.2, v1.0.0, v1.0.1, v1.0.2, v1.0.3, v1.0.4, v1.1.0, v1.1.2, v1.1.3 | **11** |
| ON except macOS | v1.2.0, v1.2.1, v1.2.2 | 3 |
| OFF by `&& false` | v1.3.0, v1.4.0, v1.4.1, v1.5.0, v1.6.0, v1.7.0, v1.7.1, v1.7.2, v1.7.3, v1.7.4 | 10 |

Eleven plus three plus ten is twenty-four, which is the count of tags containing
the capability. There is no v1.1.1 tag in the repository. The file moved from
`taichi/backends/vulkan/` to `taichi/rhi/vulkan/` between v1.0.3 and v1.0.4,
which is why a path-fixed sweep shows a false gap at v1.0.0 to v1.0.3 and why
adversary 1's sweep, which handled that move, still missed v0.9.x.

In all eleven unguarded tags the setter sits under a runtime device check,
`if (device_supported_features.shaderInt64)`, at `v0.9.0:665` and `v1.1.3:736`.
"Unguarded" here means no preprocessor guard, not no device check.

**[AMEND3 7] Which window "eleven" means, because neither this report nor its
pair said.** Both round-two adversaries asked for it. **Eleven** is the count of
tags carrying the capability with no preprocessor guard, so it is the window that
was live on *every* platform including macOS. On Linux and Windows builds the
`#if !defined(__APPLE__)` tags compile the setter too, so the live window there is
**fourteen**: v0.9.0 through v1.2.2. Eleven plus three is fourteen. The plan's
target tiers are all non-Apple, so fourteen is the figure that bears on this fork;
eleven is the figure that bears on the argument in 7.7 item E, which needs only
that mixed addressing shipped at all.

**The eight-month figure stands.** `7705f688a` author-dated 2022-02-08 to
`f677fb141` on 2022-10-24 is eight and a half months. v0.9.0, the first release
carrying the feature, is dated 2022-02-22.

### 1.4 Reason one: found broken on macOS — VERIFIED [GH]

Issue **#6295**, "The method from_numpy() not working on Mac when using Vulkan
backend", 2022-10-11 to 2022-10-29. Silently wrong data: an ndarray read
returned 0 instead of 6, no error, no validation message. `ti.sync()` did not
help.

The cited comment, 1288522905, strongoier, 2022-10-24, verbatim:

> After bisecting MoltenVK commits, I found that the problem was introduced in
> https://github.com/KhronosGroup/MoltenVK/pull/1638, which enabled support for
> `VK_KHR_buffer_device_address` and `VK_EXT_buffer_device_address` on macOS
> 12.5. To make a healthy release, I'll submit a PR to stop using the feature on
> macOS. We can diagnose further about whether the problem comes from the
> feature itself or the use of the feature in Taichi in the future.

Two things are VERIFIED here and both matter:
1. This is a **driver-level** trigger. The bisect landed on a MoltenVK release,
   not a Taichi commit.
2. **The root cause was never established.** Upstream explicitly left open
   whether the fault was MoltenVK's or Taichi's own use of buffer device
   address. No follow-up diagnosis exists in the issue, and I found none
   elsewhere.

Category: **found broken**, cause undetermined, platform-specific.

### 1.5 Reason two: the blanket withdrawal — VERIFIED [GH] as to text

PR **#6468** body, verbatim: "Disabled for now because of multi-platform
compatibility issues. Will be recovered when device capability is fully
functional."

PR **#6494** body, verbatim: "The physical storage buffer codegen was disabled
but the runtime can still use them. This is an attempt to disable psb at
runtime. Will bring it back after devcap."

**Correction from both adversaries, accepted.** The previous version of this
section said "neither PR has a single line of human review discussion. The only
comments on either are from the Netlify bot." The *issue-comment* threads are
indeed bot-only, but PR #6494 carries an APPROVED review by ailzhang with the
body "Thanks!", and PR #6468 an APPROVED review by ailzhang with an empty body.
The claim as written was false. The point it served, that no design discussion
exists, stands.

Category as stated by upstream: **blocked on an unfinished mechanism**, not
found broken, not judged wrong.

### 1.6 What the "multi-platform compatibility issues" were — INFERRED [GH]

Upstream never enumerated them. My reading, from two still-open issues:

- **#6368** "Vulkan `create_vma_allocator()` needs to respect the device
  capabilities", opened 2022-10-18, **still open**, twelve days before #6468.
  Body: when Taichi's Vulkan runtime accepts an **external VkDevice**, it cannot
  know which extensions that device enabled, and `vmaCreateAllocator()` will
  crash. k-ye's comment shows the shader-side symptom class:
  `VUID-VkShaderModuleCreateInfo-pCode-01091`, "The SPIR-V Capability (X) was
  declared, but none of the requirements were met to use it."
- The imported-device path is `c_api/src/taichi_vulkan_impl.cpp`, the very file
  whose automatic producer #6494 commented out.

**Two corrections from adversary 2, both accepted.** The previous version said
#6368 was "opened 2022-10-18 by PENGUINLIONG". It was opened by **k-ye**, who is
also the author of its sole comment. And the validation error k-ye pasted names
`AtomicFloat64AddEXT`, `AtomicFloat32AddEXT`, `Int8`, `Int16`, `Int64`,
`Float16` and `Float64` — **not `PhysicalStorageBufferAddresses`**. So calling
it "exactly the failure shape for this capability" overstated it. It is the
failure *class*; the capability under discussion does not appear in the dump.

**INFERRED, and weaker than previously stated.** No upstream text names #6368 as
the reason for #6468. Adversary 2 offers a better-evidenced alternative: every
disabling PR is tagged `[aot]`, and the AOT capability check at
`c_api/src/taichi_gfx_impl.cpp:22-29` is an equality test rather than a floor,
so a module compiled with the capability cannot load on a runtime reporting a
different set. That reading is inside the repository and mechanical. I judge it
better supported than mine. **Neither is established.**

### 1.7 The stated precondition was met, and it still was not restored — VERIFIED

- Issue **#5979 "[AOT] Device capability management for AOT (Tracker)"**, opened
  2022-09-05 by PENGUINLIONG, **still OPEN** [GH]. One comment, by its own
  author, on the day it was opened.
- Yet the capability-negotiation machinery visibly exists today [LOCAL]:
  `TI_CAPABILITY_SPIRV_HAS_PHYSICAL_STORAGE_BUFFER = 18` at
  `c_api/include/taichi/taichi_core.h:399`, setter at
  `c_api/include/taichi/cpp/taichi.hpp:1156-1159`, enumerated at
  `taichi/inc/rhi_constants.inc.h:28`, and the C-API device-capability work
  landed in #6773 (`bed652dc2`, merged 2022-12-07) after being reverted once in
  #6772.
- No branch anywhere restores the automatic producer. VERIFIED by scanning all
  remote refs for a `vulkan_device_creator.cpp` setting the capability without
  `&& false`, filtered to tips after 2022-11-02: only `upstream/rc-v1.2.0` and
  `upstream/bump/v1.2.3`, both v1.2-line release branches carrying the older
  pre-change code. Both adversaries reproduced this independently.

**[AMEND 1] The sentence "the producers are still off" is withdrawn.** It was
true of the two automatic producers and false of the feature. Section 1.10.

**Momentum loss.** Adversary 1 argues, and adversary 2 partly concedes, that
this is stronger than an argument from silence: it is unkept explicit
commitments plus unanswered direct requests. Issue #8608 "Implement int64
indexing", open since 2024-12-18, has two comments, both from non-maintainers,
and no maintainer has ever replied. Issue #5320 has had no maintainer reply in
nearly four years. Issue #8161 "ndrange is limited in size" drew "We will
investigate it later" and nothing since. That is affirmative evidence, and I
accept the upgrade. It remains an inference about motive; what is VERIFIED is
that the precondition was met and the guard was never lifted.

### 1.8 The dead branch was maintained afterwards — VERIFIED [LOCAL], DATES CORRECTED

**[AMEND 9] The previous version said "ten commits through 2023 and 2024". No
commit is from 2024. Both adversaries caught this and both are right.**

`git log --since=2023-01-01 -S 'has_buffer_ptr' master` returns exactly ten,
enumerated so the count can be reconciled:

| # | Commit | Date | Subject |
|---|---|---|---|
| 1 | `7809ebbd5` | 2023-07-13 | [refactor] Refactor code repetition for get argpack layout (#8278) |
| 2 | `14e83c484` | 2023-07-10 | [lang] Argpacks stores scalar values only in cpp implementation |
| 3 | `cfad91fc8` | 2023-07-10 | [spirv] [ir] Support argpack buffer load for spir-v backends |
| 4 | `29cfb5c72` | 2023-07-10 | [lang] Instantiate a runtime ArgPack object |
| 5 | `b66279b0b` | 2023-07-10 | [refactor] Enhance argument data structures |
| 6 | `32518fc52` | 2023-06-13 | [refactor] Add base class GfxProgramImpl |
| 7 | `88ac098bc` | 2023-05-23 | [spirv] [ir] [lang] Support struct object as return value in spir-v (#8061) |
| 8 | `748abdbcc` | 2023-04-25 | [Lang] Let kernel argument support matrix nested in a struct |
| 9 | `457ada6a6` | 2023-04-19 | [spirv] Support struct as kernel argument |
| 10 | `4c33922df` | 2023-04-06 | [gfx] Compile struct type of result and arguments in gfx backends |

Ten rows, ten commits. **All ten fall between 2023-04-06 and 2023-07-13, a span
of 98 days, fourteen weeks.** This is one argument-and-return-struct refactor
campaign sweeping through, not two years of drift.

Two of the ten, `32518fc52` and `7809ebbd5`, are relocations rather than
extensions; `-S` counts any change in occurrence count, including pure moves.

**The last edit to this path is `715a04c98`** (#8315, Bob Cao, 2023-08-15),
which the `-S has_buffer_ptr` search does not catch because it edits
`make_pointer` and `at_buffer` rather than `has_buffer_ptr`. `spirv_codegen.cpp`
has two commits after it — `ac49f79f4` (2023-10-31, clz instruction) and
`ba0e81dce` (2025-07-30, debugprintf sanitising) — and neither touches this
path. `git log --since=2024-01-01 -- taichi/codegen/spirv/` returns three
commits in total, none on it. **Correct range: April to August 2023.**

This moves the last touch eighteen months earlier than previously stated, which
strengthens the momentum reading rather than weakening it.

**Risk, VERIFIED:** there is no test coverage. A grep over `tests/`, `c_api/`,
`misc/` and `benchmarks/` for `physical_storage_buffer`,
`PHYSICAL_STORAGE_BUFFER`, `buffer_device_address`, `BufferDeviceAddress` and
`PhysicalStorageBuffer` returns nothing outside the C-API headers and generated
documentation. The only hit in `python/` is the enum name at
`python/taichi/lang/enums.py:38`. **No test, C++ or Python, exercises the
capability on or asserts anything about it.**

Stated precisely: the `if (has_buffer_ptr)` **true branches** have not executed
in CI since 2022-10-30. The surrounding code executes constantly with the flag
false. Re-enabling means turning on roughly a dozen never-executed branches
added across ten commits, not resurrecting a dormant subsystem wholesale.
Section 1.12 gives a concrete example of what is sitting in there.

### 1.9 Vulkan-only among the RHI backends — VERIFIED [LOCAL]

Only the Vulkan RHI ever set this capability automatically. Metal
(`metal_device.mm:1045-1067`), DX11 (`dx_device.cpp:564-565`) and OpenGL
(`opengl_device.cpp:511-516`) do not set it at all.

### 1.10 [AMEND 1] The feature is un-defaulted, not off. Two live producers — VERIFIED [LOCAL]

**This is the correction that changes the framing of this report, its pair, and
adversary 1. All four documents describe a feature that is disabled. It is not.**

The ref sweep in section 1.7 is correct and answers a narrower question than it
appeared to. It shows that no committed source line sets the capability
automatically. It does not show that no producer exists. Two do, and both are
caller-reachable today with no device validation anywhere in the chain.

**Producer one: the ahead-of-time module interface.** This is a C++ API, not
only a Python one. `Program::make_aot_module_builder(Arch, const
std::vector<std::string> &caps)` is declared at `taichi/program/program.h:224`
and defined at `taichi/program/program.cpp:541-557`. `cpp_examples/aot_save.cpp:20`
calls it directly from C++. The chain, every link verified by reading:

1. Caller supplies an arbitrary list of capability name strings.
   `python/taichi/aot/module.py:110` is one caller; `cpp_examples/aot_save.cpp:20`
   and `tests/cpp/aot/dx12/aot_save_load_test.cpp:30` are C++ callers.
2. `taichi/program/program.cpp:544` calls `translate_devcaps(caps)`, defined at
   `:514-539`. It starts from an **empty** `DeviceCapabilityConfig cfg{}` at
   `:517`, calls `str2devcap(key)` on each user string at `:530`, and sets it at
   `:531`. The only value it adds on its own is a `spirv_version` default of
   `0x10300` at `:535-537`.
3. `str2devcap`, `taichi/rhi/device_capability.cpp:6-13`, is generated from
   `taichi/inc/rhi_constants.inc.h`, so
   `"spirv_has_physical_storage_buffer"` at `:28` is valid input. Unknown names
   raise `TI_ERROR`; known ones are accepted unconditionally.
4. `taichi/program/program.cpp:554` passes the config to
   `GfxProgramImpl::make_aot_module_builder`,
   `taichi/runtime/program_impls/gfx/gfx_program.cpp:30-41`, which forwards it
   into `gfx::AotModuleBuilderImpl` on both branches, at `:34` and `:39`.
5. `taichi/runtime/gfx/aot_module_builder_impl.cpp:21` stores it as `caps_`.
6. `:75` and `:119` call
   `compilation_manager_.load_or_compile(config_, caps_, *kernel)`.
7. `taichi/compilation_manager/kernel_compilation_manager.cpp:71-82` forwards
   `caps` into `compile_and_cache_kernel`.
8. `taichi/codegen/spirv/kernel_compiler.cpp:37` sets `params.caps = device_caps`.
9. `taichi/codegen/spirv/spirv_codegen.cpp:86` binds it as `caps_`, which is the
   object every one of the twelve direct readers in section 1.1a consults.

**There is no device check anywhere in that chain.** The config the codegen sees
is exactly what the caller named. The physical device's own capabilities are
never consulted, never merged and never compared.

**Producer two: the C API can replace the device capability set wholesale.**
`ti_set_runtime_capabilities_ext`, `c_api/src/taichi_core_impl.cpp:317-334`,
builds a `DeviceCapabilityConfig` from caller-supplied
`TiCapabilityLevelInfo` values in the loop at `:324-330` and calls
`runtime2->get().set_caps(std::move(devcaps))` at `:331`.
`Device::set_caps`, `taichi/rhi/public_device.h:855-857`, is
`caps_ = std::move(caps)` — a wholesale replacement, not a merge, with no
validation against what the device actually supports. The typed convenience
setter at `c_api/include/taichi/cpp/taichi.hpp:1156-1159` exists to make this
easy.

**Three consequences, stated narrowly.**

1. The zero-coverage risk in section 1.8 is not a future decision. It is a
   present exposure. Anyone who names the capability reaches roughly a dozen
   branches that have never executed.
2. **A capability list naming the capability alone produces an undeclared-type
   hazard, reachable today.** `translate_devcaps` sets nothing the caller did
   not name, so `spirv_has_int64` stays false.
   `spirv_ir_builder.cpp:166-169` declares `t_uint64_` **only** under
   `spirv_has_int64`, and `IRBuilder::u64_type()`,
   `spirv_ir_builder.h:532-534`, is a bare `return t_uint64_;` with no guard.
   Meanwhile `visit(ExternalPtrStmt*)` calls `ir_->u64_type()` at
   `spirv_codegen.cpp:787`, `:789` and `:790` on the capability branch. The
   result is a default-constructed `SType` and an undeclared type id, with no
   diagnostic. The `TI_ERROR` guards at `spirv_ir_builder.cpp:311-313` and
   `:325-328` live in `get_primitive_type(DataType)`, which this path does not
   call. Taichi also never validates its generated SPIR-V (section 7.6), so
   nothing catches it.

   **[AMEND3 5] There is a second entrance, and it fires earlier.** Both
   round-two adversaries found it; verified here. `translate_ti_type`
   (`spirv_types.cpp:484-497`) maps a Taichi `PointerType` to
   `IntType(64, is_signed=false)` at `:491-493` under the capability. That tinyir
   type is lowered by `Translate2Spirv::visit_int_type` at
   `spirv_types.cpp:393-419`, whose unsigned 64-bit arm at `:414-415` calls the
   same unguarded `u64_type()`, and whose signed 64-bit arm at `:402-403` calls
   the equally unguarded `i64_type()` (`spirv_ir_builder.h:529-531`). It then
   stores `ir_node_2_spv_value[type] = vt.id` at `:418`, that is, zero.
   `translate_ti_type` is called by `compile_args_struct` (`:2340`),
   `compile_argpack_struct` (`:2416`) and `compile_ret_struct` (`:2491`), all of
   which run **before any statement is visited**. So a kernel taking an ndarray
   argument produces an undeclared 64-bit type in the argument-struct layout
   first, and in the `ExternalPtrStmt` body second. Five live sites in two files,
   not the three this report named: `spirv_codegen.cpp:787`, `:789`, `:790` and
   `spirv_types.cpp:403`, `:415`.

   **One bound on the exposure, from round-two adversary 1, verified and
   accepted.** No device-derived capability set can produce this pairing. In
   `taichi/rhi/vulkan/vulkan_device_creator.cpp`, `spirv_has_int64` is set at
   `:632` under `if (device_supported_features.shaderInt64)`, and the
   physical-storage-buffer setter at `:826` sits inside the same `shaderInt64`
   test at `:821`. On the just-in-time path the two capabilities are locked
   together. The hazard requires a hand-written capability list that names one
   capability and omits its dependency, so it is a trap for a caller on the
   unvalidated path rather than a fault that fires on ordinary use. Both
   statements are true and both belong in the record.
3. For a project whose plan section 5 puts configuration at install time and
   whose item 6.3 wants adaptive module loading, the difference between
   "disabled" and "not enabled by default, caller-settable, unvalidated" is the
   whole question. Whether that door should be closed or kept open is a
   decision, not a finding. Escalations.

Credit where due: adversary 2 found this and stated it in full. Adversary 1
reproduced the ref sweep and filed "no live producer" under what it could not
break, one step from the counter-example without taking it. Adversary 1 is
wrong on this point, and so was this report.

### 1.11 [AMEND 3] Dead SNode code that contradicts the intent half of the claim — VERIFIED [LOCAL]

Both this report and its pair state flatly that the capability never touched the
SNode root buffer. **That is true of every line that runs. It is not true of
everything in the tree, and adversary 2 found the exception.** Adversary 1 did
not, and neither explore pass did.

`taichi/codegen/spirv/snode_struct_compiler.cpp:34-62` defines
`StructCompiler::construct`, which walks an SNode tree and builds a `tinyir`
type for it. At `:52-54`:

```cpp
      if (sn->type == SNodeType::pointer) {
        cell_type = ir_module.emplace_back<PhysicalPointerType>(cell_type);
      }
```

`PhysicalPointerType` is declared at `taichi/codegen/spirv/spirv_types.h:71-88`,
and its SPIR-V translation, `visit_physical_pointer_type` at
`spirv_types.cpp:433-439`, emits `OpTypePointer` with
`spv::StorageClassPhysicalStorageBuffer` at `:435-436` — the storage class that
exists only under this capability. **That is the SNode struct compiler proposing
to give sparse `pointer` SNode cells physical device addresses.**

It is unreachable, and the evidence is unambiguous:

- Its only non-recursive caller is commented out at
  `snode_struct_compiler.cpp:16-19`. The recursive call at `:44` cannot
  bootstrap it.
- The `CompiledSNodeStructs` fields it would write are commented out at
  `snode_struct_compiler.h:49-50`, under the comment "TODO: Use the new type
  compiler".
- `git log --all -S "PhysicalPointerType"` returns **exactly one commit**,
  `46b5632e5` "[vulkan] Codegen & runtime improvements (#5213)", Bob Cao,
  2022-06-22. I read the diff: **the call site was added already commented out**,
  along with `construct` itself. It never ran.
- `PhysicalPointerType` has two construction sites in the whole tree:
  `snode_struct_compiler.cpp:53`, dead, and `spirv_types.cpp:337`, inside the
  type reducer, which only copies an existing instance. With no live producer,
  the visitor at `:433` never runs. Report B's inventory lists
  `spirv_types.cpp:433-439` among live consumers; it is not one.

**How I weigh it.** It does not overturn the convergent claim. Forty lines that
never compiled into anything are not a reusable asset, and section 6's
conclusion about behaviour is untouched. But it is the **only** artefact in this
territory connecting the capability to SNodes, and it was written four months
after the capability landed, by the same author. So the correct statement is
narrower than either explore pass gave:

> The physical-storage-buffer work never touched SNode addressing **in any code
> that runs**. It is false that nobody ever connected the two: the author of the
> mechanism sketched exactly that design for sparse `pointer` nodes in June 2022
> and shipped the sketch commented out.

That also upgrades the standing of the argument in section 6 item 6, which
otherwise rests on the SPIR-V and Vulkan specifications rather than on anything
in this tree. Per plan section 10 item 3 I am not calling this surplus and not
calling it an asset. It goes to Escalations for disposition.

### 1.12 [AMEND 10] A host and device layout mismatch inside the untested branch — VERIFIED [LOCAL]

Adversary 1's finding, checked line by line and confirmed. Adversary 2 verified
it independently and calls it the best single find in either adversary file. I
agree, and it is the sharpest available evidence for the abstract risk in
section 1.8.

Two layout strings are produced in `taichi/runtime/program_impls/gfx/gfx_program.h`:

- **Arguments**, `:79-83`. Reads the capability at `:80-81` and returns `"1b"`
  or `"1-"` at `:82`.
- **Returns**, `:75-77`. Returns the literal `"4-"` at `:76`,
  **unconditionally, with no capability read at all.**

The host consumes both through
`GfxRuntime::get_struct_type_with_data_layout_impl`,
`taichi/runtime/gfx/runtime.cpp:817` onward, which decodes `layout[1] == 'b'` at
`:823` and sizes a `PointerType` member as `sizeof(uint64_t)` at `:853` or
`sizeof(uint32_t)` at `:857`. With `"4-"` the host always picks 32 bits.

Meanwhile the device side, `compile_ret_struct` at
`spirv_codegen.cpp:2483-2500`, **does** read the capability at `:2490-2491` and
threads it into `translate_ti_type` at `:2494`, which yields `IntType(64)` for a
`PointerType` when the flag is set (`spirv_types.cpp:491-493`).

So with the capability on, the shader lays out a return-struct pointer member at
64 bits while the host sizes and aligns it at 32. **The argument path is
consistent; the return path is not.**

The return-struct branch was added by `88ac098bc` (#8061, 2023-05-23), row 7 of
the ten in section 1.8. The diff adds
`bool has_buffer_ptr = caps_->get(...)` and the `translate_ti_type` call, with
no matching change to `get_kernel_return_data_layout`.

**Stated conditionally, and the hedge is adversary 1's and is the right one.**
Whether a kernel return struct can contain a `PointerType` at all is a frontend
question, and plan section 1.2 puts the Python frontend out of scope. If it can,
this is a live host and device ABI mismatch sitting in a branch with zero test
coverage, reachable through the producer in section 1.10. Escalations.

---

## 2. `use_64bit_pointers`

### 2.1 It was never switched off, because it was never on — VERIFIED [LOCAL]

`git log -S 'use_64bit_pointers' --all` returns **exactly one commit**:
`1b34a2d4d` "[vulkan] Indexed load codegen (#3259)", Bob Cao, 2021-10-25 —
three and a half months *before* physical storage buffers existed.

It was introduced as `const bool use_64bit_pointers = false;` in the same commit
that created `make_pointer()`, with the comment "This is hacky, should check out
how to encode uint64 values in spirv" written on the same line as the code it
describes.

`git log --all -S 'use_64bit_pointers = true'` returns **nothing**. It has never
been true on any ref in the repository's history.

Only two references exist today: the definition at
`taichi/codegen/spirv/spirv_codegen.cpp:82` and the sole consumer at `:2318`.
The true branch has been touched once since, by `715a04c98` (#8315, Bob Cao,
2023-08-15), which replaced a u32-then-cast with a direct u64 immediate at
`:2320`. Both adversaries confirm that report B's "born false, never edited" is
misleading on this point and that this report has it right.

**Correction to the brief:** the comment does not say the work "was never
finished". It says the encoding approach is hacky and unexamined. The
distinction matters: nothing was ever attempted.

**[AMEND3] A sharper way to put it, from round-two adversary 2, verified.**
`git show v1.1.3:taichi/codegen/spirv/spirv_codegen.cpp` line 74 is
`const bool use_64bit_pointers = false;`, identical to `spirv_codegen.cpp:82`
today. So the u64 arm of `make_pointer` has **never executed in any tagged
release, on any platform, including the eleven where the capability itself was
live**. This report and its pair both frame the collision as something that would
surface on a flag flip. It is more than that: the arm is code that has never run,
which bears on how much of this territory is *shelved* work as against work that
was *never written*. Recorded, not resolved.

### 2.2 It governs exactly the thing item 6.2 needs — VERIFIED [LOCAL]

`make_pointer` is called at four sites, all on the SNode root path:
`spirv_codegen.cpp:355` (`GetRootStmt`), `:371` (`GetChStmt`), `:408`
(`bitmasked_activation`), `:506` (`SNodeLookupStmt`). This is root-buffer
offset arithmetic — the width that item 6.2 is about.

### 2.3 It cannot be flipped — VERIFIED [LOCAL], three concrete breakages

**(a) The u64 meaning collides.** `at_buffer` at `spirv_codegen.cpp:2194`
branches at `:2197` on `ptr_val.stype.dt == PrimitiveType::u64` and emits
`OpConvertUToPtr` at `:2198-2202` to a
`spv::StorageClassPhysicalStorageBuffer` pointer, tagging the result
`ValueKind::kPhysicalPtr` at `:2203`. The same test appears in `load_buffer` at
`:2227` and `store_buffer` at `:2249`. Throughout this file, **a u64 pointer
value means an absolute device address.**

But `make_pointer` produces a **root-buffer-relative byte offset**, beginning at
literal `0` for the root (`:355`). Setting `use_64bit_pointers = true` would
make every SNode access take the physical-address path and dereference
`0 + offset`, and `ptr_to_buffers_[stmt]`, set at `:377`, would be silently
never read, because it is consulted only on the fall-through path at `:2212`.

Both round-one adversaries verify this and both uphold it as a real blocker.
**[AMEND3 1] The classification is settled: ARCHITECTURAL**, section 7.7 item F.
And a further breakage belongs here, because it is what makes the collision
irreducible rather than merely awkward: the `ValueKind` tag on `spirv::Value`
cannot be pressed into service as an alternative discriminator, because
`make_value` at `spirv_ir_builder.h:290-298` derives the result tag from the
result type alone and never reads an operand tag, so the `ir_->add` at `:372`
erases it on every SNode path into `at_buffer`. Full derivation in 7.7 item F.

**(b) Type mixing in `bitmasked_activation`** (`:383-437`). `ptr_dt` is taken
from the parent pointer at `:388` and would become u64, but the body then mixes
widths in two places SPIR-V does not permit:

- `:398-399` — `OpShiftLeftLogical` with result type `ptr_dt` (u64) and Base
  `ir_->const_i32_one_` (i32).
- `:410-412` — `OpShiftRightLogical` with result type hardcoded
  `ir_->u32_type()` while Base `bitmask_word_ptr` is `ptr_dt`-typed (u64).
- `:414` — `struct_array_access(ir_->u32_type(), buffer, bitmask_word_ptr)`
  then indexes with that value.

Both adversaries re-read every one of these citations and confirm all three, and
both confirm that `:405`'s `u32_type()` shift *amount* is legal, because SPIR-V
constrains only Base and Result Type. Both also confirm that `:409`,
`ir_->add(parent_ptr, bitmask_word_ptr)`, is **not** a defect: under the flag
both operands are `ptr_dt`.

**One correction to this report's own statement of the rule, from adversary 1.**
The previous version said "Base and Result must share a type." The rule SPIR-V
actually enforces on the shift instructions is matching **bit width and
component count**, not signedness. Under the stricter rule as stated, `:398-399`
would already be invalid today, since `const_i32_one_` is signed and `ptr_dt` is
`u32`, and it plainly is not, because this code ships. Under the correct rule
`:398-399` is fine at 32 bits and breaks at 64. The conclusion is unaffected.

For contrast, `SNodeLookupStmt` at `:503-508` *would* survive, because it casts
the index to `parent_val.stype` before multiplying. Both adversaries checked
this and agree.

**(c) [AMEND 5, from adversary 2] `make_pointer` bypasses the `shaderInt64`
guard, silently.** `make_pointer` at `:2317-2324` calls `ir_->u64_type()`. That
accessor, `spirv_ir_builder.h:532-534`, is a bare `return t_uint64_;` with no
guard. `t_uint64_` is assigned only inside
`if (caps_->get(cap::spirv_has_int64))` at `spirv_ir_builder.cpp:166-169`. The
`TI_ERROR` guards at `:311-313` and `:325-328` live in
`get_primitive_type(DataType)`, which `make_pointer` does not call.

On a device without `shaderInt64`, flipping the flag yields a
default-constructed `SType` and an undeclared type id, **with no diagnostic**.
This is the only silent one of the three failure modes, and under plan section
2.2 it is the worst available shape: correct output on stronger hardware,
silently wrong output on weaker. Section 1.10 item 2 shows the same gap is
reachable today through a different door, without touching this flag.

**INFERRED:** that a build with the flag flipped would fail SPIR-V validation
and/or produce wrong addresses. I did not compile or run anything; this is read
from the source. Both adversaries say the same of themselves.

### 2.4 Category

**Never started.** Not shelved because broken, not shelved for hardware, not
judged wrong. A marker left in place by an author who noticed the width would
one day matter and did not pursue it. There is no discussion, no issue, no
design note anywhere referencing this variable.

---

## 3. The int32 index ceiling

### 3.1 History — VERIFIED [LOCAL] + [GH]

- `cfc4065d3` "[bug] Fix silent int overflow in indice calculation (#3177)",
  ailzhang, 2021-10-21. Widened `acc_shape` to `int64` but kept
  `extractors[i].acc_shape` int32 behind an explicit `static_cast<int>` with the
  comment "casting to int32 in extractors.", and added the warning. Now at
  `taichi/ir/snode.cpp:89-100`.
- PR #3177 body [GH]: "I didn't fix all the implicit conversions from in64 to
  int32 (but tried to make the conversions more explicit). IIUC we don't support
  int64 indexing yet so it should be good enough to do the explicit conversion
  for now and revisit this later."
- **ATTRIBUTION CORRECTED, from adversary 2.** The previous version of this
  section said k-ye proposed enforcing that the value fit in `int` and that
  **ailzhang overruled that** with "I think it's still better to expand this to
  64-bits. It is less limiting, and is consistent with `num_cells_per_container`."
  Adversary 2 read the thread and reports that this sentence is the closing line
  of **k-ye's own comment**, dated 2021-10-14, in which k-ye quotes the
  enforce-the-fit proposal and then rejects it; ailzhang's only review comment on
  that PR is about LLVM versus C++ casting. **I accept the correction. The
  speaker was reversed.** The substance survives — a maintainer argued for
  widening in October 2021 and it was never carried out — but this report called
  it "the clearest evidence of intent in the whole territory", so getting the
  speaker wrong matters and the sentence is corrected here.

- `b347a2159` "[bug] Fix false overflow alarm in struct fors under packed mode
  (#6457)", Yi Xu, 2022-10-28. Appended "Struct fors might not work either." to
  the warning **and**, in the same commit, replaced
  `TI_ASSERT(total_bits <= 30)` with
  `TI_ASSERT(total_n <= std::numeric_limits<int>::max())` in
  `taichi/transforms/demote_dense_struct_fors.cpp`, now at line 29.
- `96e795770` "[Lang] Warn users if ndarray size is out of int32 boundary
  (#6846)", Yi Xu, 2022-12-13. Same warning replicated for ndarrays at
  `taichi/program/ndarray.cpp:53-59` and `:96-102`.
- Issue **#6758** [GH], now **closed**: a user hit SIGSEGV on a 48000x48000 f64
  ndarray. Maintainer strongoier, 2022-11-30: "Taichi currently uses i32 for
  indexing so this behavior is known." The resolution was a warning, not a fix.
- Issue **#8608 "Implement int64 indexing"** [GH], **open** since 2024-12-18,
  filed by lstrgar. It quotes both the warning and the assertion, and the
  artefact that terminates the run is the assertion. Two comments, both from
  non-maintainers; no maintainer has ever replied. This report missed the issue
  entirely on the first pass; report B found it and both adversaries verified
  it. It is the upstream tracking issue for exactly the question this territory
  was assigned.

### 3.2 [AMEND 6] The operative constraint. This section was wrong, and this is the most important correction in the set.

The previous version of this section named
`taichi/transforms/demote_dense_struct_fors.cpp:29`,
`TI_ASSERT(total_n <= std::numeric_limits<int>::max())`, as "the operative
constraint today" and said struct-for is "hard capped at 2^31-1 total cells".
**Both adversaries independently found the gate that makes that false for this
project, and both are right.**

**The assertion is real and is a hard stop where it fires.** VERIFIED.
`TI_ASSERT` at `taichi/common/logging.h:100-107` expands to a plain
`if (!x) TI_ERROR(...)`. There is no `NDEBUG` gate; it throws in every build.
It is what upstream's users actually hit — issue #8608 quotes the exact error
line naming `demote_dense_struct_fors.cpp:convert_to_range_for@29`.

**But it only ever runs on an all-dense tree.** VERIFIED,
`demote_dense_struct_fors.cpp:114-119`:

```cpp
void maybe_convert(OffloadedStmt *stmt) {
  if ((stmt->task_type == TaskType::struct_for) &&
      stmt->snode->is_path_all_dense) {
    convert_to_range_for(stmt);
  }
}
```

`convert_to_range_for` at `:13` is the only function containing the assertion,
and `maybe_convert` at `:114` is its only caller. `is_path_all_dense` is
declared at `taichi/ir/snode.h:114` and set at `taichi/ir/snode.cpp:20`,
`new_ch->is_path_all_dense = (is_path_all_dense && !new_ch->need_activation())`.
`SNode::need_activation` returns true for `pointer`, `hash`, `bitmasked` and
`dynamic`, so any one of those four anywhere on the path clears the flag.
`taichi/transforms/offload.cpp:191-192` reads the same flag for the same
decision.

**Plan section 4.1 requires sparsity.** On a sparse tree the pass does not run,
the assertion never executes, and the cap this report named does not apply to
this project's workload.

**The constraint that does apply, which nobody in this territory named.**
VERIFIED, `taichi/ir/snode.h:37-53`, `struct AxisExtractor`:

| Line | Field | Type |
|---|---|---|
| `taichi/ir/snode.h:41` | `num_elements_from_root` | `int` |
| `taichi/ir/snode.h:45` | `shape` | `int` |
| `taichi/ir/snode.h:49` | `acc_shape` | `int` |

Three fields, all 32-bit, and the binding width for a sparse tree. For contrast
`SNode::num_cells_per_container` at `taichi/ir/snode.h:97` is **already
`int64`**.

The ordering matters and both adversaries reconstructed it the same way.
`taichi/ir/snode.cpp:89-101`:

```cpp
  int64 acc_shape = 1;
  for (int i = taichi_max_num_indices - 1; i >= 0; i--) {
    // casting to int32 in extractors.
    new_node.extractors[i].acc_shape = static_cast<int>(acc_shape);
    acc_shape *= new_node.extractors[i].shape;
  }
  if (acc_shape > std::numeric_limits<int>::max()) {
```

The store at `:92` truncates into a 32-bit field on **every iteration**, before
the check at `:95` runs even once. So the warning at `:95-100` is a post-hoc
detector for damage already done, not a guard against it.
`num_cells_per_container` is assigned the untruncated `int64` at `:101`, and
`shape_along_axis` at `snode.cpp:174-177` returns `int`.

**Ruling.** The assertion is the hard stop on all-dense trees and is what
upstream's users report hitting. For a sparse tree it never fires, and the
binding width is the three `int` fields at `snode.h:41`, `:45` and `:49`. That
the sibling `num_cells_per_container` is already `int64` is what the October
2021 consistency argument was pointing at, and it makes the widening smaller
than this report previously implied.

This overlaps territory 01 (`ir-types`) and I defer to that pair on
completeness. It is recorded here because both explore passes in this territory
stopped at the two visible messages, and because it is the answer to plan
section 8.1 item 1 for this corner.

### 3.3 Category

**Never started; deliberate and current.** No 64-bit indexing work was begun and
abandoned. The 32-bit width is a known, documented, maintained limit that
upstream chose to paper over with warnings twice (2021, 2022) rather than fix.
Nothing here suggests the approach is wrong; it suggests nobody paid the cost.

---

## 4. `PointerType::addr_space_`

### 4.1 It is not 64-bit machinery — VERIFIED [LOCAL]

`taichi/ir/type.h:207`, `int addr_space_{0};  // TODO: make this an enum`.

- Introduced by `dcd5d7d35` "[type] Add basic implementations of VectorType and
  PointerType (#1948)", Yuanming Hu, 2020-10-12. Sixteen months before physical
  storage buffers, a year before `use_64bit_pointers`. `48fa799da` (#7460,
  lin-hitonami, 2023-03-06) later added it to `TI_IO_DEF`. Two commits total.
- Dead on arrival and dead now. `grep -rn "addr_space"` over the tree, excluding
  `external/`, returns three hits in `taichi/ir/type.h` — the getter body at
  `:191-193`, the `TI_IO_DEF` at `:203`, the field at `:207` — and three in the
  LLVM codegen that are a different thing (below). **Zero call sites of
  `get_addr_space` anywhere.** Neither constructor takes or sets it: `type.h:179`
  is the default constructor, `:181-185` takes `(Type *pointee, bool
  is_bit_pointer)` only. No setter exists.
- **One nuance, from adversary 2, accepted.** "Zero call sites, ever" is not
  literally true: the serialisation macro at `type.h:203` reads it, so the field
  is written into the offline cache and AOT type records. It is always zero, so
  nothing observable follows.
- It is address **space** (the LLVM generic/global/shared selector), not address
  **width**. The working analogue is the `int addr_space` parameter declared with
  default `0` at `taichi/codegen/llvm/codegen_llvm.h:165`, used in
  `TaskCodeGenLLVM::cast_pointer` at `codegen_llvm.cpp:1129-1134`, feeding
  `llvm::PointerType::get(get_runtime_type(dest_ty_name), addr_space)` at
  `:1133`. That is LLVM's memory-space selector and has nothing to do with width.
- **One qualification, from adversary 1, recorded without softening the
  ruling.** On some targets an address space does imply a pointer width: AMDGPU
  `addrspace(3)` local pointers are 32-bit while `addrspace(1)` global pointers
  are 64-bit. So space and width are not fully orthogonal on every backend. That
  is an item 6.3 and territory 04 concern, not a reason to keep a dead field
  under item 6.2.

### 4.2 [AMEND 7] Category, and the recommendation

**Not part of the 64-bit story.** No shelving event exists because no work
existed.

**Recommendation: strike `PointerType::addr_space_` from item 6.2.** Four
independent agreements now exist and no source or history contradicts any of
them: this report, report B, adversary 1 section 6, and adversary 2 section 4.
Both adversaries state the recommendation in the same terms. Treat as settled.

Note the distinction the plan requires: striking it from item 6.2 is a
classification change, not a removal. Nobody is proposing to delete the field.
Plan section 10 item 3 forbids that and nothing here argues for it.

---

## 5. Absence of a plan — VERIFIED [LOCAL] + [GH]

- No design document. `grep -rniE '64[- ]?bit|int64|i64|u64' docs/**/*.md` finds
  only the type-system reference, `docs/design/llvm_sparse_runtime.md` (which
  documents 64-bit locks and cell pointers in the LLVM sparse runtime, an
  unrelated and already-64-bit thing), and `docs/lang/articles/basic/sparse.md`.
- No TODOs. A tree-wide grep for TODO/FIXME/XXX/HACK within 120 characters of
  any 64-bit, physical-storage, buffer-device-address or address-space term
  returns exactly two hits: `taichi/codegen/llvm/codegen_llvm_quant.cpp:75`
  (unrelated, atomicCAS widths) and `spirv_codegen.cpp:2319` (the "This is
  hacky" line).
- No re-enablement thread. GitHub issue search for "physical storage buffer"
  returns one unrelated hit; "buffer device address" returns three, none a design
  or re-enablement thread. Both adversaries re-ran these and reached the same
  empty result.
- **Qualification.** There *is* an open upstream tracking issue for the
  underlying requirement, #8608 "Implement int64 indexing", filed 2024-12-18 and
  never answered by a maintainer. It is not a design document and proposes no
  approach, but the previous version of this section left the impression that no
  upstream artefact tracks the question at all, and that is too strong.

**There was never a 64-bit addressing programme to shelve.** There were four
independent, small, local decisions.

---

## 6. What this means for project item 6.2

Stated narrowly, without extrapolation.

1. The physical-storage-buffer work is **not** reusable for SNode addressing
   width as it stands, because no code that runs addresses the SNode root buffer
   through it. It addresses ndarray and external-array argument base addresses,
   and only the base address: the index arithmetic stays `i32`
   (section 1.1b). VERIFIED, sections 1.1a and 1.1b.
2. **[AMEND 3]** That statement is about behaviour. As a statement about intent
   it needs the qualifier in section 1.11: the author of the mechanism sketched
   physical addresses for sparse `pointer` SNode cells in June 2022 and shipped
   the sketch commented out.
3. The one piece that *does* sit on the SNode root path, `use_64bit_pointers`,
   is an empty placeholder, and its true branch is inconsistent with the
   surrounding code's meaning of u64. VERIFIED, section 2.3.
4. Nothing found indicates the 64-bit *approach* was judged wrong. The one
   recorded design argument, on PR #3177, went in favour of widening.
5. **[AMEND 4]** The module-wide SPIR-V addressing model is **not** an
   architectural obstacle. Section 7.7 item E. This report previously called it
   the hard obstacle and both adversaries refuted it by the same two routes.
6. **[AMEND 11] On SPIR-V there is no other mechanism, and this qualifies
   conclusion 1 rather than refuting it.** Adversary 1 raised it, adversary 2
   verified it and supplied the missing source line. Checked independently and
   upheld:
   - The SNode root is a **descriptor-bound storage buffer**. `get_buffer_value`
     at `spirv_codegen.cpp:2287-2315` binds it through
     `ir_->buffer_argument(type, 0, binding, ...)` at `:2309`;
     `IRBuilder::buffer_argument` at `spirv_ir_builder.cpp:734-763` creates an
     `OpVariable` in `StorageClassStorageBuffer` (or `StorageClassUniform` below
     SPIR-V 1.3) at `:754-757`, tagged `ValueKind::kStructArrayPtr`.
     `bitmasked_activation` binds it explicitly at `:401-402`.
   - `at_buffer`'s non-u64 path at `:2212-2218` shifts the byte offset right by
     `log2(width)` and hands the result to `struct_array_access`. The offset it
     shifts is whatever `make_pointer` produced: a `u32` today.
   - A descriptor-bound storage buffer's reach is capped by Vulkan's
     `maxStorageBufferRange`. **This tree never queries that limit anywhere.**
     `grep -rn maxStorageBufferRange taichi/ c_api/` returns zero hits. VERIFIED.
   - Therefore SPIR-V offers exactly two addressing mechanisms here: a bound
     buffer with an offset, and a physical device address. Widening
     `make_pointer` to `u64` without moving off the bound buffer widens an offset
     into a range the offset was not what limited.

   **The Taichi-side half is VERIFIED. The Vulkan-limit half is INFERRED**, read
   from the SPIR-V and Vulkan addressing models rather than from a Taichi source
   line — with the one exception in section 1.11, which is the tree's own
   commented-out expression of the same design, reached independently by the
   author of the mechanism.

   So the correct statement is not "the shelved work is irrelevant to item 6.2".
   It is: **not reusable as written, and simultaneously the only door SPIR-V
   offers past a bound buffer's reach, hence structurally prerequisite for a
   root that outgrows one.** The reversal that goes to the project owner should
   carry that qualifier or the next decision will be taken against an incomplete
   picture.

   **[AMEND3] The limit has since been measured, and it splits the targets.**
   This report's statement that the tree never queries `maxStorageBufferRange`
   stands and was re-verified: `grep -rn maxStorageBufferRange taichi/ c_api/`
   still returns zero. But plan section 8.2 now records the values, obtained
   through the Vulkan loader on the development box: 4294967295 on the GTX 1070
   and on the GTX 750 Ti, and 134217728 on llvmpipe. So on both cards a
   descriptor-bound buffer already reaches within 2 MiB of the 32-bit offset's
   own span and a device address buys nothing, while **on the CPU software
   renderer a bound buffer caps at 128 MiB and a device address, needing no
   descriptor, lifts one tree to 2 GiB.** The plan also records that the field
   reporting the limit is a `uint32_t` in the bundled Vulkan headers, so no
   Vulkan device can ever advertise more and the ceiling will not lift with newer
   silicon.

   That inverts the practical weight of this item without changing its logic. The
   inference held that physical addressing is the only door past a bound buffer;
   the measurement says the door matters least on the GPUs and most on the
   **CPU-only path**, which plan section 5.2 makes a first class target. I record
   it and draw no further conclusion.

   **Scope.** This is a SPIR-V-spine statement. The LLVM spine, which plan
   section 5.2 puts first because CPU-only operation runs through it, is a
   different question and belongs to territories 02 and 03.

8. **[AMEND3] Plan section 2.2a now places this whole path in scope as BASE
   work**, and says the shelved work "covers 64-bit addressing for everything
   EXCEPT SNodes, so it is the other half of the same limit... The two halves are
   not separable." Nothing in conclusion 1 is falsified: no code that runs
   connects the capability to the SNode root buffer. What changes is what follows
   from it. Conclusion 1 is a statement about **reusability of existing code**,
   and it must not be read as putting the shelved work outside item 6.2, because
   the plan now says the opposite. See section 0a for a tension this creates with
   plan section 2.3, which I escalate rather than resolve.
7. Restoring the capability changes the **host and device argument ABI**
   (`gfx_program.h:79-83`, `"1b"` versus `"1-"`, consumed at
   `taichi/runtime/gfx/runtime.cpp:823` and `:851-859`). Any AOT or install-time
   binary-selection scheme has to treat it as an ABI variant, not a flag. And
   per section 1.12, the return half of that ABI is currently inconsistent with
   itself.

---

## 7. Does each reason still hold today?

Every reason established above is sorted **ARCHITECTURAL** (conflicts with
something structural; does not expire) or **ENVIRONMENTAL** (drivers,
extensions, tooling, hardware of the day; expires).

A third tag, **[MEASURED]**, marks facts obtained by running read-only queries
on this box: `nvidia-smi` and `vulkaninfo`. Nothing was modified.

### 7.0 The verdict table

**[AMEND 4] Two rows changed in the second pass. E is no longer an obstacle at
all. [AMEND3 1] Two more change now, under the definition the plan added after
this table was written: F becomes ARCHITECTURAL, and C is regraded on the same
rule. Both regrades are against this report.**

The grade answers one question only — *if every external thing were ideal today,
would the obstacle still be there?* — and the cost of the remedy is stated
separately, in section 7.7.

| # | Reason | Kind | Still holds? |
|---|---|---|---|
| A | macOS / MoltenVK wrong results (#6295) | ENVIRONMENTAL | Largely expired, not cleanly; and macOS is not a target tier |
| B | Imported `VkDevice` cannot report its enabled extensions (#6368) | ENVIRONMENTAL | **Still holds**, verbatim, on the C-API import path only |
| C | AOT device-capability negotiation not ready ("devcap", #5979) | **ARCHITECTURAL** [AMEND3 2] | Expired in substance; the mechanism exists and ships in the public C header. **Regraded:** it was unwritten Taichi code. No ideal driver, specification or dependency writes it, and it went away because somebody changed the codebase, which is how the rule says architectural obstacles go. Adversary 1 raised this and is right |
| D | Argument ABI differs with the capability (`"1b"` vs `"1-"`) | ARCHITECTURAL | **Holds permanently.** Not a bug, a design consequence |
| E | Whole-module SPIR-V addressing model switch | **RECLASSIFIED. Not an obstacle.** | **Withdrawn.** Refuted by eleven shipped releases and by module granularity. See 7.7 |
| F | `use_64bit_pointers` collides with `at_buffer`'s meaning of u64 | **ARCHITECTURAL** [AMEND3 1] | **Holds.** `spirv_codegen.cpp` represents "device address against buffer-relative offset" by the pointer's scalar width, at `:2197`, `:2227` and `:2249`. That is a representation decision inside this codebase. No driver, hardware generation, toolchain pin or specification revision removes it. Blast radius stated separately in 7.7. See 7.7 |
| G | Hardware lacks `bufferDeviceAddress` / `shaderInt64` | ENVIRONMENTAL | **Expired on both target tiers**, measured |
| H | SPIR-V toolchain vintage | ENVIRONMENTAL | Expired upstream, **NOT expired in our checkout** |
| I | 32-bit index width in the IR and struct layer | ARCHITECTURAL | **Holds.** Nobody else's problem to fix. Located correctly at `snode.h:41,45,49` per section 3.2 |

Nine rows. Counting rule: one row per lettered reason, graded by the plan's
architectural-against-environmental test. **Four ARCHITECTURAL (C, D, F, I), one
withdrawn as not an obstacle (E), four ENVIRONMENTAL (A, B, G, H).** Four plus one
plus four is nine.

Row B was examined again and left ENVIRONMENTAL. Vulkan offers no query for the
extensions an imported device has *enabled*, so an ideal external specification
would remove the obstacle. Adversary 1 reached the same grade by the same route
and flagged it as defensible and contestable. I concur and do not change it.

### 7.1 The Vulkan 1.2 promotion premise does not apply — VERIFIED [LOCAL]

The brief suggests the promotion of `VK_KHR_buffer_device_address` into core
Vulkan 1.2 may have changed the availability picture. It did not, because the
code accounted for it from the start.

`7705f688a`, February 2022, the original introduction, verbatim:

```
+    if (CHECK_VERSION(1, 2) ||
+        CHECK_EXTENSION(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) ||
+        CHECK_EXTENSION(VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)) {
```

Vulkan 1.2 shipped January 2020, two years before that commit and nearly three
before the change. `06cb1c50d` (#4244) then narrowed it a day later, dropping
the `VK_EXT_` alternative, adding the `shaderInt64` requirement and a
`CHECK_VERSION(1, 3) ||` short-circuit. Current form at
`taichi/rhi/vulkan/vulkan_device_creator.cpp:813-821`.

**Core promotion post-dates nothing and expired nothing.** Reported because the
premise was offered to me and the evidence contradicts it.

### 7.2 Reason G, hardware availability — EXPIRED, [MEASURED]

`vulkaninfo` and `nvidia-smi` on this box. NVIDIA driver 535.309.01.

| Device | apiVersion | `shaderInt64` | `bufferDeviceAddress` | `VK_KHR_buffer_device_address` |
|---|---|---|---|---|
| GTX 1070, Pascal, plan tier Mid | 1.3.242 | true | true | present |
| GTX 750 Ti, Maxwell, plan tier Baseline | 1.3.242 | true | true | present |
| llvmpipe / lavapipe, Mesa on CPU | n/a | true | true | present |

Tracing Taichi's gate against those numbers [LOCAL]:
`vulkan_device_creator.cpp:523` takes the version from the physical device
(1.3.242); `:814` passes; `:819-821` pass on all terms; and `:825`
`#if !defined(__APPLE__) && false` is the only false term.

**Both hardware tiers named in plan section 5.2 satisfy every runtime
precondition the code tests**, as does the CPU software rasteriser. Adversary 1
flags this as a plan section 2.2 gate: if one card satisfied it and the other did
not, restoring the capability would let some hardware do something other hardware
cannot. Measured here, all three satisfy it, so that violation does not arise on
this box. It would still arise on any device lacking `shaderInt64`, per section
2.3(c) and section 1.10 item 2.

This says the gate passes. It does not say the feature works. See 7.6.

### 7.3 Reason A, MoltenVK — LARGELY EXPIRED [GH]

Relevant only to macOS, which is not among the plan's target tiers, but included
because the brief asks about vendor driver maturity.

- MoltenVK issue **2174**, "regression bug with vkGetBufferDeviceAddress",
  opened 2024-03-07, **still open**. `vkGetBufferDeviceAddress` returned 0 on
  MoltenVK 1.2.0 through 1.2.6 for the reporter.
- Maintainer **billhollings**, 2024-03-12: "`VK_KHR_buffer_device_address` and
  `VK_EXT_buffer_device_address` are supported in the latest MoltenVK, and are
  currently passing about **90% of the CTS tests**."
- Further open device-address reports: **2696** (2026-03-06), **2278**
  (2024-07-19).

The specific 2022 breakage is no longer the live complaint, but the feature was
still short of full conformance on that vendor as recently as 2024, by its own
maintainer's account. And since nobody ever established whether the 2022 fault
was MoltenVK's or Taichi's own (section 1.4), **the Taichi-side possibility has
never been eliminated**. Environmental, substantially expired, not cleanly.

### 7.4 Reason B, the imported device — STILL HOLDS, but narrowly — VERIFIED [LOCAL]

Issue #6368 asked for "a mechanism to specify 1) which extensions are enabled".
Checking whether the C-API gained one:

`c_api/include/taichi/taichi_vulkan.h:26-47`, `TiVulkanRuntimeInteropInfo`,
carries `get_instance_proc_addr`, `api_version`, `instance`, `physical_device`,
`device`, the two queues and their family indices. **No field for enabled
extensions or device features.** The blocker is unexpired, verbatim, on that
path.

**But it never applied to the owned-device path**, and this scoping matters more
than the blocker itself:

- `vulkan_device_creator.cpp:595-596` pushes `VK_KHR_buffer_device_address` into
  `enabled_extensions` whenever the device advertises it — unconditionally,
  outside any `#if`.
- `:830-831` chains `buffer_device_address_feature` into `VkDeviceCreateInfo` —
  also outside the `#if`, which encloses only the `caps.set(...)` line at `:826`.

So when Taichi creates its own device, today, on this box, it **already enables
the extension and requests the feature**. The `&& false` suppresses only the
SPIR-V capability declaration. A device that enabled the extension cannot
produce the `VUID-VkShaderModuleCreateInfo-pCode-01091` failure that #6368
describes.

`VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT` at `vulkan_device.cpp:2509` is
gated on the same capability, so it would follow the flag rather than needing
separate work.

**[AMEND 1] Scoping qualification.** All of the above concerns the device Taichi
creates. Section 1.10 shows the AOT compilation path never consults a device at
all, so neither the owned-path reassurance nor the imported-path blocker applies
there.

### 7.5 Reason C, devcap — EXPIRED IN SUBSTANCE — VERIFIED [LOCAL] + [GH]

Tracker **#5979** is still open [GH], with one comment, from its own author, on
the day it was opened. But the mechanism it tracked is visibly present:
`TI_CAPABILITY_SPIRV_HAS_PHYSICAL_STORAGE_BUFFER = 18` at
`c_api/include/taichi/taichi_core.h:399`, its setter at
`c_api/include/taichi/cpp/taichi.hpp:1156-1159`, the enumeration at
`taichi/inc/rhi_constants.inc.h:28`, and the C-API device-capability work landed
in `bed652dc2` (#6773) on 2022-12-07, five weeks after the change.

The stated precondition for restoration was substantially met within weeks and
the automatic producer was never restored. That is what makes "momentum lost"
the operative explanation for the last three and a half years, distinct from the
reasons it was withdrawn in the first place.

**[AMEND 1] And note what "the mechanism exists" actually means.** It is not
machinery-in-waiting. `ti_set_runtime_capabilities_ext`
(`c_api/src/taichi_core_impl.cpp:317-334`) is the mechanism, it is shipped, and
it replaces the device's capability set wholesale with no validation. Section
1.10.

### 7.6 Reason H, toolchain vintage — EXPIRED UPSTREAM, NOT IN OUR COPY — VERIFIED [LOCAL]

| Submodule | Pin | Date |
|---|---|---|
| Vulkan-Headers | `409c16b` | 2025-04-18 (Vulkan-Docs 1.4.313) |
| VulkanMemoryAllocator | `539c0a8` | 2025-04-14 |
| volk | `b87f882` | 2023-03-24 |
| SPIRV-Reflect | `7c9c841` | 2023-02-01 |
| **SPIRV-Headers** | `34d0464` | **2022-12-15** |
| **SPIRV-Cross** | `c77b09b` | **2022-11-21** |
| **SPIRV-Tools** | `46ca66e` | **2022-11-18** |

The Vulkan API surface we compile against is current. **The SPIR-V toolchain is
frozen within three weeks of the withdrawal commit.** Anything fixed in
SPIRV-Tools since 2022-11-18 is fixed upstream and absent here.

Two qualifications, both verified:

1. **SPIRV-Cross does not bear on this question.**
   `grep -rln 'spirv_cross\|spirv_glsl\|spirv_msl' taichi/` returns only
   `taichi/rhi/metal/`, `taichi/rhi/dx/` and `taichi/rhi/opengl/` files, and none
   of those backends sets the capability (section 1.9). The Vulkan path consumes
   SPIR-V directly. Of the three stale pins, only SPIRV-Tools and SPIRV-Headers
   matter.
2. **The pinned SPIRV-Tools is not blind to the feature.** `grep -r
   PhysicalStorageBuffer external/SPIRV-Tools/source/` gives 64 hits, including
   `val/validate_capability.cpp:158`, `val/validate_conversion.cpp:267-274`,
   `val/validate_memory.cpp`, `val/validate_atomics.cpp` and
   `opt/inst_buff_addr_check_pass.cpp`. SPIR-V 1.5 made the extension core and
   our target env is `SPV_ENV_VULKAN_1_2` (`spirv_codegen.cpp:2670-2680`, driven
   by the `spirv_version` cap `0x10500` set at
   `vulkan_device_creator.cpp:525-527`), so the vintage is nominally in range.

**And a finding that compounds the risk:** Taichi never validates its generated
SPIR-V. `spirv_codegen.cpp:2710` sets `spirv_opt_options_.set_run_validator(false)`,
and `Validate` appears nowhere in the file; the only other SpirvTools call is a
`Disassemble` at `:2763` inside `if constexpr (false)`. *[AMEND3 6] Earlier
versions of this report cited `:2709`. Both round-two adversaries caught the
off-by-one; the line is `:2710`, confirmed by grep.* A malformed
physical-storage-buffer module would go straight to the driver, which is exactly
the shape of the 2022 macOS symptom: silently wrong results, no diagnostic. It
is also what would swallow the undeclared-type hazard in section 1.10 item 2.

Whether the twenty-odd optimizer passes Taichi registers handle
`AddressingModelPhysicalStorageBuffer64` correctly at this pin is not something
grepping can settle. Escalating.

### 7.7 The non-expiring reasons, reclassified

**[AMEND 4] Two of the four reasons this report previously listed here do not
survive as stated. Both adversaries attacked E and both refuted it by the same
two routes, independently. On F they diverge, and I adjudicate below.**

**D. The argument ABI is a capability-dependent variant. ARCHITECTURAL, upheld.**
`taichi/runtime/program_impls/gfx/gfx_program.h:79-83` returns `"1b"` with the
capability and `"1-"` without, and the host acts on it at
`taichi/runtime/gfx/runtime.cpp:823` and `:851-859`. The host and device argument
layout differs. This is not a defect to be fixed; it is what the feature *is*.
Any install-time binary-selection scheme, which plan section 5 makes the
deployment model, has to treat the capability as an ABI variant of the compiled
artifact, not as a runtime flag. VERIFIED. Section 1.12 adds that the *return*
half of the same ABI is currently inconsistent with itself, which is a defect
rather than a design consequence.

**E. The module-wide SPIR-V addressing model. WITHDRAWN. Not an obstacle.**

The fact is unchanged and correct. `spirv_ir_builder.cpp:113-127` emits exactly
one `OpMemoryModel` per module: `AddressingModelPhysicalStorageBuffer64` under
the capability at `:120-123`, `AddressingModelLogical` otherwise at `:125-126`.
One declaration, no per-pointer opt-in. `OpCapability
PhysicalStorageBufferAddresses` is emitted separately at `:73-77`.

The consequence this report drew from it does not hold. Three grounds, all
checked:

1. **"Every shader in the module" is one shader.** VERIFIED. `TaskCodegen`
   constructs its own `IRBuilder` per instance at `spirv_codegen.cpp:97`, and
   `run()` at `:130-156` calls `init_header()`, builds a single function named
   `"main"`, and returns `ir_->finalize()` at `:151` as that task's `spirv_code`.
   One offloaded task, one module, one entry point. The blast radius is one
   kernel task. Both adversaries found this independently, by the same route.
2. **The addressing model is permissive, not exclusive.**
   `AddressingModelPhysicalStorageBuffer64` does not invalidate logical pointers;
   it adds the `PhysicalStorageBuffer` storage class alongside the existing ones.
   Descriptor-bound storage buffers keep working unchanged in the same module.
   INFERRED from the SPIR-V and Vulkan specifications.
3. **[AMEND 4] Upstream's own shipped code settles it, and this is the decisive
   ground.** VERIFIED directly, not taken from either adversary. In v1.1.3,
   `spirv_ir_builder.cpp:114` emits `AddressingModelPhysicalStorageBuffer64`
   under the capability, while `spirv_codegen.cpp:74` reads
   `const bool use_64bit_pointers = false;` and `make_pointer` returns
   `ir_->uint_immediate_number(ir_->u32_type(), uint32_t(offset))`, registered as
   the root value by `make_pointer(0)` at `:275`. That is a module declaring the
   physical addressing model while addressing the SNode root through a
   descriptor-bound buffer with a 32-bit offset. Per section 1.3a this shipped in
   **eleven tagged releases**, v0.9.0 through v1.1.3, plus three more with only
   macOS excepted. Mixed addressing under that model is not hypothetical; it is
   what upstream shipped for eight months, and what eventually broke was a
   MoltenVK bug in ndarray reads, not the memory model.

**The addressing model is a declaration, not a constraint. It is withdrawn as an
obstacle and must not be carried into the plan as one.** What remains in this
area is real and is ENVIRONMENTAL, not architectural: declaring a SPIR-V
capability the device did not enable is a spec violation, so the capability must
track real device support. That is the failure class in issue #6368, and per
section 1.10 the AOT path has no device check at all to make it track anything.

**F. `use_64bit_pointers` collides with the file's own meaning of u64.
ARCHITECTURAL. [AMEND3 1] The previous pass withheld that label and was wrong.**

The second pass adjudicated a divergence between the two round-one adversaries
and ruled the collision blocking but not architectural. **Both round-two
adversaries settled that against this report, independently, and the source is on
their side. The ruling is withdrawn and the label restored.**

**What was right, and stays.** Four assertions about the `ValueKind` tag, each
re-verified in this pass:

- `enum class ValueKind` at `taichi/codegen/spirv/spirv_ir_builder.h:69-79`, nine
  members, `kPhysicalPtr` among them; `ValueKind flag{ValueKind::kNormal}` on
  `struct Value` at `:88`.
- The tag is set on the physical branch: `paddr_ptr.flag = ValueKind::kPhysicalPtr;`
  at `spirv_codegen.cpp:2203`, with the `return` at `:2204`.
- It survives the name table: `register_value` at `spirv_ir_builder.cpp:1299-1309`
  stores the whole `Value` at `:1308`; `query_value` at `:1311-1317` returns it by
  value at `:1314`.
- It already drives load and store: `load_variable` at `:1270-1285` asserts one of
  three pointer kinds at `:1271-1273` and branches on `kPhysicalPtr` at `:1275`;
  `store_variable` at `:1286-1298` does the same at `:1290`.

All four hold. Round-two adversary 1 confirms all four; round-two adversary 2
confirms all four. Nothing in this list is retracted.

**What was wrong: the inference, and the fact that refutes it.** VERIFIED here,
not taken from either adversary. `IRBuilder::make_value`,
`taichi/codegen/spirv/spirv_ir_builder.h:290-298`:

```cpp
  template <typename... Args>
  Value make_value(spv::Op op, const SType &out_type, Args &&...args) {
    Value val = new_value(out_type, ValueKind::kNormal);
    make_inst(op, out_type, val, std::forward<Args>(args)...);
    if (out_type.flag == TypeKind::kPtr) {
      val.flag = ValueKind::kVariablePtr;
    }
    return val;
  }
```

The result's tag is a function of the SPIR-V **result type** alone. **No operand's
tag is read.** `SType::flag` defaults to `TypeKind::kPrimitive` at
`spirv_ir_builder.h:59`, so an integer byte offset of any width comes out
`kNormal` whatever its operands carried.

`ir_->add` is `DEFINE_BUILDER_BINARY_USIGN_OP(add, Add)`, defined at
`spirv_ir_builder.cpp:1038-1047` and instantiated at `:1062`, expanding to
`make_value(spv::OpIAdd, a.stype, a, b)`. So the add is an eraser.

The SNode path, traced instruction by instruction:

| Step | Site | Operation | Resulting tag |
|---|---|---|---|
| 1 | `spirv_codegen.cpp:355` | `make_pointer(0)` → `uint_immediate_number` → `new_value(dtype, ValueKind::kConstant)` at `spirv_ir_builder.cpp:1502` | `kConstant` |
| 2 | `:356` | `register_value` | `kConstant`, survives |
| 3 | `:372` | `ir_->add(input_ptr_val, offset)` | **`kNormal`. Destroyed** |
| 4 | `:373` | `register_value` | `kNormal` |
| 5 | `:377` | `ptr_to_buffers_[stmt] = BufferInfo(BufferType::Root, root)` | map entry, not a tag |
| 6 | `:2195` | `at_buffer` does `query_value` | `kNormal` |

Six steps. Step 3 is decisive and it is not avoidable: `ptr_to_buffers_` is
written for an SNode only at `:377`, inside `visit(GetChStmt *)` and only when
`out_snode->is_place()` at `:375`. **Every SNode place pointer that ever reaches
`at_buffer` has passed through the add at `:372` at least once.**
`SNodeLookupStmt` at `:504-510` compounds it with a `mul` at `:507` and an `add`
at `:508`.

The ndarray side arrives the same way. `load_variable` returns
`new_value(res_type, ValueKind::kNormal)` at `spirv_ir_builder.cpp:1274`, above
the `kPhysicalPtr` branch and applying to both arms, so the genuine physical
device address loaded at `spirv_codegen.cpp:789` and added at `:790-791` is
registered at `:792` as `kNormal`.

**So both sides of the decision at `:2197` arrive identically tagged**, and the
previous pass's remedy — populate the tag at the four `make_pointer` call sites
and the `ExternalPtrStmt` base path — does not work. It is necessary and
insufficient. **This report's own stated precision, that the tag sits on
`at_buffer`'s output rather than its input, was the seed of the error:** it is
exactly true, and the step it invited, that populating the input therefore closes
the gap, is false because nothing survives the trip.

*One qualification on the adversaries' shared wording, from source.* Round-two
adversary 2 says both sides "arrive untagged". They arrive tagged
non-discriminatingly, and not identically across the whole input set:
`GlobalTemporaryStmt` registers a `kConstant` at `spirv_codegen.cpp:710`. That
does not disturb the conclusion — `kConstant` separates a device address from a
buffer offset no better than `kNormal` does — and adversary 2 records it itself.

**Who was right about what, stated plainly because the second pass got this
backwards.** Round-one adversary 1's *sentence*, "the code has no third thing to
disambiguate them", was wrong, and this report was right to say so. Round-one
adversary 1's *ruling*, that the collision is architectural, was right, and this
report was wrong to overturn it. The second pass ruled against the adversary who
had the correct answer on the strength of a defective sentence, and never traced
`make_value`. Notes entry 29 shows the omission: it enumerates the tag, `:2203`,
`load_variable` and `register_value`, and never reaches the constructor.

**Grading, under the plan's rule rather than this report's old one.** *If every
external thing were ideal today, would the obstacle still be there?* Yes.
`spirv_codegen.cpp` represents the addressing scheme of a pointer by that
pointer's scalar width, at `:2197`, `:2227` and `:2249` — three sites, enumerated
by `grep -n "stype.dt == PrimitiveType::u64" taichi/codegen/spirv/spirv_codegen.cpp`,
which neither this report nor its pair previously counted. That is a decision
inside this codebase about how something is represented. No driver, hardware
generation, submodule pin or specification revision removes it. **ARCHITECTURAL.**

The previous pass's own preceding sentence already contained that answer — "It is
not ENVIRONMENTAL: no driver, extension or hardware generation will fix it,
because it is a decision internal to Taichi" — and then withheld the label on the
size of the remedy. The rule now bars that step in terms.

### 7.7a [AMEND3 3] Blast radius for reason F, stated separately

The plan requires the radius stated apart from the kind, concretely, in files and
call sites. Two figures were in circulation and neither is usable: this report's
old five production sites, which is an insufficient remedy, and report B's 147,
which is `grep -rn "make_value(" taichi/codegen/spirv/` at 148 hits less the
definition at `spirv_ir_builder.h:291`. I reproduced 147 and it is arithmetically
right, but it counts call sites whose *behaviour* would change, not sites to edit.
A regression surface is not an edit surface.

**And the remedy those figures price is not the only one available.** This is the
second point where the third round moves in this report's favour rather than
against it, and it comes from territory 07, which was convened on the question.

Round-two adversary 1 section 3.4 and round-two adversary 2 section 3.4 both
observed, independently, that report B's load-bearing sentence — "there is no
existing discriminator at the decision point by any route" — was reached by
testing whether `ptr_to_buffers_` holds an *entry* on both sides, and never what
the entry's *value* is. Verified here. `grep -rn "ptr_to_buffers_"` over
`taichi/`, `tests/` and `cpp_examples/` returns ten hits, all in
`spirv_codegen.cpp`: seven writes, one `count` at `:376`, one read at `:2212`, one
declaration at `:2635`. The two sides of the `:2197` decision hold **different
values**: `BufferInfo(BufferType::Root, root)` at `:377` against
`{BufferType::ExtArr, arg_id}` at `:798`. The map is keyed on `const Stmt *` and
propagated explicitly at `:319`, `:326` and `:332`, so **arithmetic cannot erase
it** — which is precisely the property the `Value` tag lacks.

**Territory 07 has since settled it, and the plan section 8.2 now records the
result.** A discriminator does exist at that point: the buffer-type map crossed
with the compile-constant `caps_->get(...)`, both already in scope at that line.
The capability flag is required alongside the map, because `BufferType::ExtArr`
means a physical address on a device with the capability and a bound-buffer offset
without it.

**And the inherited sentence was worse than merely unproven. Its evidence was
manufactured by the test that produced it.** VERIFIED here rather than accepted:
`AllocaStmt` is not among the seven writers, so the right-hand side
`ptr_to_buffers_[stmt->origin]` at `:319` is `operator[]` on a key that is never
present, which **default-inserts** a value-initialised `BufferInfo`.
`BufferInfo` at `taichi/codegen/spirv/kernel_utils.h:33-45` declares
`BufferType type;` at `:34` with no default member initialiser and
`BufferInfo() = default;` at `:37`, and `BufferType::Root` is the first
enumerator at `:24`, so the fabricated entry is `{Root, {-1}}` — a root buffer
with root id −1 that no writer intended. It is unobservable today only because
`get_pointer_type` (`spirv_ir_builder.cpp:407-424`) never assigns `SType::dt`
and `DataType::DataType()` is `ptr_(PrimitiveType::unknown.ptr_)`
(`taichi/ir/type.cpp:20`), so the `TI_ERROR_IF(!is_integral(ptr_val.stype.dt), ...)`
at `spirv_codegen.cpp:2207-2210` fires first. **That guard sits between the
predicate at `:2197` and the map read at `:2212`**, so any rule consulting the map
at `:2197` moves the shared-array `MatrixPtrStmt` from a loud compile error onto
the fabricated path. Fix the fabrication first.

**The radius, with the rule that generates it.** Counting rule: every source line
that must be edited, or whose behaviour changes, for a map-plus-capability
discriminator to replace the width sniff. One row per line.

| Class | Sites | File | Edit? |
|---|---|---|---|
| The width sniff | `spirv_codegen.cpp:2197`, `:2227`, `:2249` | `spirv_codegen.cpp` | yes, 3 |
| Fabricating propagation writes to make honest first | `:319`, `:326`, `:332` | `spirv_codegen.cpp` | yes, 3 |
| The default that yields `{Root, {-1}}` | `kernel_utils.h:34` | `kernel_utils.h` | yes, 1 |
| The map read already present | `:2212` | `spirv_codegen.cpp` | no |
| Unguarded `at_buffer` calls whose behaviour changes if the guard order moves | `:1617`, `:1626`, `:1681` | `spirv_codegen.cpp` | no |

Eleven rows, **11 sites in 2 files, 7 of them requiring an edit**, three plus
three plus one. That is the figure the plan now carries at section 8.2, and I
derive it here independently rather than adopting it. The three unguarded atomic
calls are real: `grep -n "at_buffer(" spirv_codegen.cpp` returns nine hits, and
`:1621`, `:1630` and `:1633` carry the `dest_is_ptr` guard computed at `:1612`
while `:1617`, `:1626` and `:1681` do not.

This reconciles with the two territory 07 figures rather than contradicting them:
07a's eight counts one predicate rather than three and omits the map read; 07b's
six counts only the sites requiring an edit in `spirv_codegen.cpp`, leaving out
`kernel_utils.h:34`. All three are the same finding under three counting rules,
and I state mine so it can be checked.

**So the second pass's instinct that the remedy was small was closer to right
than the consensus that overruled it — and its stated reason for that instinct
was still wrong.** The remedy is not four or five sites, and it is not a change to
the backend's value model reaching 147 call sites. It is seven edits in two files,
by a route through the buffer map that this report never considered and that the
`ValueKind` route it did consider genuinely cannot supply. **Being roughly right
about the cost does not license having been wrong about the kind**, and under the
plan's rule the cost was never the question the label answered.

**Two limits on the cheap route, recorded so it is not read as settled work.**
Territory 07 could not establish whether the non-array `ExternalPtrStmt` branch at
`:800`, which writes `BufferType::Args`, is reachable at `at_buffer`, and the
fabricated `{Root, {-1}}` entry has to be fixed before the map can be consulted
above the `!is_integral` guard. Neither is mine to resolve. Escalation 18.

**I. The 32-bit index width. ARCHITECTURAL, upheld, and relocated.** Section 3.
A deliberate limit reaffirmed twice by maintainers. Nothing external will fix it.
The hard assertion at `taichi/transforms/demote_dense_struct_fors.cpp:29` applies
to all-dense trees only; for the sparse trees plan section 4.1 requires, the
binding width is the three `int` fields at `taichi/ir/snode.h:41`, `:45` and
`:49`. VERIFIED.

### 7.8 What this changes about the answer to item 6.2

Stated narrowly.

1. **The environmental case against physical storage buffers has largely
   expired**, and on the plan's own target hardware it has expired completely and
   measurably (7.2). The one unexpired environmental blocker (7.4) applies only
   to the C-API imported-device path, and per section 1.10 no device check exists
   on the AOT compilation path at all.
2. **That does not make the machinery useful for item 6.2 as it stands**, because
   of section 1.1a: it addresses ndarray and external-array argument base
   addresses, not the SNode root buffer, and it does not even widen array
   indexing (1.1b). **But see section 6 item 6:** on SPIR-V it is nevertheless
   the only mechanism that reaches past a descriptor-bound buffer, so it is
   structurally prerequisite for a root that outgrows one.
3. **[AMEND3 1] CORRECTED. The previous version of this item said the obstacle
   count "went from four permanent to two permanent plus one bounded" and that
   item 6.2 on the SPIR-V path is gated "by bounded internal design work, not by
   any permanent property". Both rested on the withdrawn ruling in 7.7 item F and
   both are withdrawn with it.** Under the plan's grading rule the count is: **four
   ARCHITECTURAL (C, D, F, I), one withdrawn as not an obstacle (E), four
   ENVIRONMENTAL (A, B, G, H)** — nine rows, section 7.0. The permanent-obstacle
   count went from four to four, by a different four: E left and C arrived, and F
   stayed. The picture is not more favourable than the first pass handed the
   planner; it is differently shaped. What *is* more favourable, and separately so,
   is the cost of reason F: 11 sites in 2 files, 7 requiring an edit, section 7.7a.
   The kind and the cost are now stated apart, which is what the rule asks for.
4. **[AMEND 1] The live-path finding is now the most consequential thing in this
   territory after the disjointness result.** The capability is reachable today
   with no device check, and the code it reaches has never executed. That is a
   present exposure and it bears directly on item 6.3 and on plan section 5's
   install-time model.
5. **Nothing found suggests the 64-bit approach itself was judged wrong.** The
   only recorded design argument, on PR #3177, went in favour of widening.

---

## 8. Escalations

Unresolved. Not to be settled by assumption.

1. **[AMEND 1] The "producers are off" framing must not enter the plan.** A
   caller can compile a physical-storage-buffer module today through
   `Program::make_aot_module_builder(arch, {"spirv_has_physical_storage_buffer"})`
   with no device validation anywhere between the caller and
   `spirv_codegen.cpp:86`. The C API can additionally replace a runtime's whole
   capability set through `ti_set_runtime_capabilities_ext`
   (`c_api/src/taichi_core_impl.cpp:331`, landing in
   `taichi/rhi/public_device.h:855-857`) with no validation. Whether that is a
   door to be closed or one to be kept open bears directly on item 6.3 and on
   plan section 5. **It is a decision, not a finding, and I am not making it.**
   The chain is six files and can be closed by reading them.

2. **[AMEND 1] A capability list naming the capability alone produces an
   undeclared u64 type today, silently.** Section 1.10 item 2:
   `translate_devcaps` adds nothing the caller did not name, so
   `spirv_has_int64` stays false, `t_uint64_` is never declared
   (`spirv_ir_builder.cpp:166-169`), and `ir_->u64_type()`
   (`spirv_ir_builder.h:532-534`) has no guard, yet
   `spirv_codegen.cpp:787`, `:789` and `:790` call it. Taichi never validates its
   SPIR-V (7.6), so nothing catches it. Under plan section 2.2 this is the worst
   available shape — correct on one device, silently wrong on another. Whether
   this is a bug to fix or a constraint to document is the planner's call.
   **[AMEND3 5] Extended:** there are five live sites in two files, not three.
   Beyond `spirv_codegen.cpp:787`, `:789` and `:790`, the argument, argpack and
   return struct translations reach the same unguarded accessors through
   `spirv_types.cpp:415` (`u64_type()`) and `:403` (`i64_type()`), and those fire
   before any statement is visited. Bounded by the fact that no device-derived
   capability set can produce the pairing: `vulkan_device_creator.cpp:632` and
   `:821-826` both sit under `shaderInt64`. It is a trap on the caller-named path
   only. Round-two adversary 1 supplies both the extension and the bound.

3. **[AMEND 3] `snode_struct_compiler.cpp:34-62` needs a disposition.** Forty
   lines of dead SNode type construction that would give sparse `pointer` nodes
   physical device addresses, committed already commented out by `46b5632e5`
   (#5213, 2022-06-22). Per plan section 10 item 3 I am not calling it
   unnecessary and I am not calling it an asset. It is the only artefact in the
   territory connecting the capability to SNodes, and it should be on the
   planner's desk before item 6.2 is sized.

4. **[AMEND 10] The return-layout mismatch is conditional and I could not close
   it.** `gfx_program.h:75-77` hardcodes `"4-"` while
   `spirv_codegen.cpp:2490-2494` reads the capability for the same struct. If a
   kernel return struct can contain a `PointerType`, this is a live host and
   device ABI mismatch inside a branch with zero coverage. Whether it can is a
   frontend question and plan section 1.2 puts the Python frontend out of scope.
   Needs someone with the frontend in scope, or a compile.

5. **[AMEND 6] The operative int32 constraint for a sparse tree is not where
   this report first put it.** The assertion at
   `demote_dense_struct_fors.cpp:29` is gated on `is_path_all_dense`
   (`:114-119`) and never fires on the sparse trees plan section 4.1 requires.
   The binding width is `taichi/ir/snode.h:41`, `:45` and `:49`, three `int`
   fields, with the truncation at `snode.cpp:92` preceding the warning at `:95`.
   This overlaps territory 01 and I defer on completeness, but it is the field
   set that actually carries the damage and it must not be lost between two
   territories. **Whether the sparse path has a further 32-bit ceiling elsewhere
   is not answered by anyone in this territory.**

6. **[AMEND 7] `PointerType::addr_space_` should leave item 6.2.** Four
   independent agreements: both explore passes and both adversaries. Section 4.2.
   This is a classification change, not a removal; nobody proposes deleting the
   field. The planner owns the plan edit.

7. **The Vulkan 1.2 promotion premise in the extended brief is contradicted by
   the evidence.** Section 7.1. `CHECK_VERSION(1, 2)` was in the original
   February 2022 code, two years after Vulkan 1.2 shipped and eight months before
   the withdrawal. Core promotion expired nothing.

8. **"Multi-platform compatibility issues" is unresolved, and my referent is the
   weaker of the two on offer.** PR #6468 names no platform. Section 1.6 offers
   issue #6368 and marks it INFERRED; adversary 2's AOT-portability reading,
   resting on the `[aot]` tags and the equality check at
   `c_api/src/taichi_gfx_impl.cpp:22-29`, is better supported and is also marked
   INFERRED by its author. **Adversary 1 weakens that reading usefully**: the
   same equality check still bites on `spirv_has_int16`, `spirv_has_int64`,
   `spirv_has_float64` and the four subgroup capabilities, all set from the
   device at `vulkan_device_creator.cpp:628`, `:632`, `:636`, `:659`, `:663`,
   `:667` and `:671`, so disabling this one capability removes one of at least
   seven sources of mismatch rather than making modules portable. Neither
   referent is established. The remaining avenue is the Taichi Discord and forum
   of October 2022, outside my brief.

9. **The macOS root cause was never determined by upstream.** It remains open
   whether MoltenVK 1.1.11 was faulty or Taichi's use of buffer device address
   was. MoltenVK's own maintainer put the feature at about 90 percent of CTS in
   2024, so the vendor side is improved but not clean, and **the Taichi-side
   possibility has never been eliminated by anyone.** It is the only recorded
   instance of the capability producing wrong answers on any platform, and no bug
   report naming it as a cause exists for any non-macOS platform.

10. **Our SPIR-V toolchain is pinned at the withdrawal vintage.** Section 7.6.
    SPIRV-Tools `46ca66e` (2022-11-18) and SPIRV-Headers `34d0464` (2022-12-15).
    Whether the optimizer passes Taichi registers handle
    `AddressingModelPhysicalStorageBuffer64` correctly at that pin cannot be
    settled by reading. Compounding it: Taichi never validates its generated
    SPIR-V, disabled at `spirv_codegen.cpp:2710`. Whether to move those pins is a
    decision, and it is the planner's.

11. **Zero CI coverage since 2022-10-30, and the branch is reachable.** Section
    1.8 gives the enumeration and section 1.10 the reachability. Ten commits over
    fourteen weeks in 2023 extended code that has never executed under test.
    Section 1.12 gives one concrete defect already sitting in it. I flag the
    risk; I do not judge it.

12. **[AMEND 11] The reversal to the project owner needs a qualifier.**
    Reversing the earlier conclusion is correct: the capability is not the asset
    for the SNode ceiling as it stands. But section 6 item 6 shows the reverse is
    not "irrelevant" — on SPIR-V it is the only mechanism that reaches past a
    descriptor-bound buffer, and `maxStorageBufferRange` is still never queried
    anywhere in this tree. A bare reversal would mislead. I am not authorised to
    word the qualifier. **[AMEND3] Partly overtaken:** plan section 8.2 now
    carries the measured limits and they put the payoff on the CPU-only path
    rather than on either card. The qualifier is therefore narrower and sharper
    than when I first raised it, not weaker.

13. **I did not build or compile anything.** Read-only `nvidia-smi` and
    `vulkaninfo` only. Every claim about what would happen if
    `use_64bit_pointers` were flipped (section 2.3) is read from source, and so
    is every correction to the adversaries' reasoning about it. Both adversaries
    say the same of themselves. Section 7.2 establishes only that the capability
    *gate* passes on this hardware, not that the feature works. If the planner
    wants the flip verified it needs a compile and a SPIR-V validator run.

14. **RESOLVED, recorded.** An earlier version escalated whether the target GPUs
    expose `VK_KHR_buffer_device_address` and `shaderInt64`. Measured in section
    7.2: both do, as does the CPU software rasteriser. No longer open.

15. **Scope boundary.** Sections 1.1a, 3.2, 6 and 7.8 make claims about where
    32-bit width lives that overlap territories 01 (`ir-types`) and 02
    (`codegen`), and section 7.7 item D touches territory 04 (`backend-build`) on
    the ABI-variant question. Section 6 item 6 is a SPIR-V-spine statement and
    says nothing about the LLVM spine, which plan section 5.2 puts first. I
    report what I found in service of the "why" question and defer to those pairs
    on completeness.

16. **One decision is explicitly the planner's.** Section 7.7 item D establishes
    that the capability is a host and device ABI variant. Whether that fits plan
    section 5's install-time configuration model well or multiplies the build
    matrix unacceptably is a judgement about the deployment design. I state the
    constraint and stop.

17. **[AMEND3] Two sections of the plan are in tension and I am not picking.**
    Section 2.2a: item 6.2 in full is BASE work, the shelved SPIR-V path and the
    `at_buffer` collision included, and "the two halves are not separable", with
    "raising the object count while addresses stay 32-bit moves the wall rather
    than removing it." Section 2.3: the shelved 64-bit addressing work is "stub
    material under section 5.1 rather than a path to the ceiling." As priority
    against scope they reconcile. On substance they do not: one says the shelved
    work is not a path to the ceiling, the other that the ceiling is not reachable
    without it. Standing instruction 6 says the plan wins where it contradicts an
    instruction; it does not say which half wins where the plan contradicts
    itself. Section 0a. Only the planner can settle it.

18. **[AMEND3 3] The cheap remedy for reason F has two open conditions.** Section
    7.7a prices a map-plus-capability discriminator at 11 sites in 2 files, 7
    requiring an edit, and that figure now sits in plan section 8.2. Territory 07
    left two things unresolved and I did not close either: whether the non-array
    `ExternalPtrStmt` branch at `spirv_codegen.cpp:800`, which writes
    `BufferType::Args`, is reachable at `at_buffer`; and the fabricated
    `{Root, {-1}}` entry produced by `operator[]` at `:319`, `:326` and `:332`,
    which must be made honest before the map can be read above the
    `!is_integral` guard at `:2207-2210`. Neither changes the KIND — the
    representation decision is architectural whichever internal replacement is
    chosen — and both change the cost.

19. **[AMEND3 2] A grading question one level down, raised by round-two adversary
    2 and I agree it is open.** The plan now defines architectural against
    environmental and requires the blast radius stated "concretely, in files and
    call sites". It does not say whether that means sites to EDIT or sites whose
    BEHAVIOUR changes. Report B's 147 and my 7 edits are honest answers to
    different readings of one instruction and differ by a factor of twenty-one.
    Section 7.7a states both and labels which is which, but only the planner can
    fix the reading. This is the same failure the grading definition was written
    to end, recurring one level down.

---

## 9. Amendment ledger

What this pass changed, so the planner can see the delta without a diff. Every
row is adjudicated in the notes at the entry given.

| Claim | Verdict | Where in this report | Notes entry |
|---|---|---|---|
| 1. Feature is un-defaulted, not off; two live producers | **UPHELD.** This report was wrong, and so was adversary 1 | 0, 1.3, 1.7, **1.10**, 7.4, 7.5 | 25 |
| 2. Both consumer inventories omit three consumers | **UPHELD.** Inventory rebuilt from scratch | 1.1a, 1.1b | 26 |
| 3. Dead SNode code contradicts the intent half | **UPHELD** as to intent; behaviour claim intact | 1.11, 6 item 2 | 27 |
| 4. Module-wide addressing model is not an obstacle | **UPHELD.** Eleven releases verified tag by tag | 1.2, 7.0 row E, 7.7 item E | 28 |
| 5. `at_buffer` collision: adversary divergence | **SUPERSEDED by [AMEND3 1].** Second-pass verdict was "blocker upheld, ARCHITECTURAL label withdrawn". Third pass restores ARCHITECTURAL and withdraws the remedy | 7.7 item F, 7.7a, 2.3(a), 2.3(c) | 29, superseded by 37 |
| 6. Assertion fires only on all-dense; sparse width is `snode.h:41,45,49` | **UPHELD.** The most important correction in the set | 3.2, 7.0 row I, 7.7 item I | 30 |
| 7. Strike `addr_space_` from item 6.2 | **UPHELD.** Four independent agreements | 4.2 | 31 |
| 8. Tag census: adversary divergence | **Adversary 2. Eleven / three / ten, across 24 tags of 121** | 1.3a | 32 |
| 9. "Ten commits through 2023 and 2024" is wrong | **UPHELD.** All ten in a 98-day window; last touch 2023-08-15 | 1.8 | 33 |
| 10. `gfx_program.h:75-77` versus `spirv_codegen.cpp:2490-2494` | **UPHELD**, conditionally | 1.12 | 34 |
| 11. On SPIR-V there is no other mechanism | **UPHELD.** Taichi half verified, Vulkan half inferred | 6 item 6 | 35 |

Eleven rows, eleven claims, one per claim in the second-pass brief. Nine upheld
outright, two adversary divergences adjudicated. **No claim in that set was found
wrong.** Row 5's adjudication has since been overturned; see the third-pass ledger
below.

Two adversary *grounds* were found wrong and are recorded as such rather than
acted on:

- Adversary 1 section 4, "the code has no third thing to disambiguate them."
  Refuted by `ValueKind` at `spirv_ir_builder.h:88`, which adversary 1 itself
  cites in the same section. Notes 29.
- Adversary 1 section 7, the eight-tag census starting at v1.0.0. It is eleven,
  starting at v0.9.0. Notes 32.

One adversary 2 phrase is over-tight and is corrected in place rather than
rejected: "already set on the physical branch" is true of the pointer `at_buffer`
produces, not of the address value it consumes, and the difference is where the
work would go. Section 7.7 item F.

## 9a. [AMEND3] Third-pass amendment ledger

| Claim put to me | Verdict | Where | Notes |
|---|---|---|---|
| 1. The `ValueKind` remedy fails; `make_value` resets the tag from the result type alone | **UPHELD against this report.** Ruling withdrawn, ARCHITECTURAL restored | 7.7 item F, 7.0 row F, 7.8 item 3, 2.3(a) | 37 |
| 2. Grade the kind, then state the radius separately | **UPHELD**, and it regrades row C as well as row F | 0a, 7.0, 7.7 item F, 7.7a | 38 |
| 3. A discriminator DOES exist; the inherited sentence rests on a lookup that inserts | **UPHELD, and it cuts in this report's favour.** Radius is 11 sites in 2 files, 7 edits | 7.7a | 39 |
| 4. State the dated plan version | **DONE**, with a numbering correction against three documents | header, 0a | 40 |
| 5. The unguarded `u64` hazard has a second entrance | **UPHELD**, two further live sites | 1.10, escalation 2 | 41 |
| 6. Off-by-one on the validator line | **UPHELD**, `:2710` not `:2709` | 7.6, escalation 10 | 42 |
| 7. "Eleven releases" needs saying which window | **UPHELD**, 11 unguarded, 14 live on non-Apple builds | 1.3a | 43 |

Seven rows, seven claims. Six upheld against this report, one upheld in its
favour. **Two rulings in section 7.7 item F are withdrawn** and are marked as
withdrawn rather than deleted.

Two adversary statements are corrected in place rather than adopted, both minor
and neither load-bearing: the shared phrase "both sides arrive untagged" (7.7 item
F, `GlobalTemporaryStmt` arrives `kConstant`), and the citation of the grading
rule as plan section 10 item 7 when it is item 8 (section 0a).

Nothing in this pass removed, bypassed or declared unnecessary any code, and no
source file, no other agent's file and no git state was touched.
