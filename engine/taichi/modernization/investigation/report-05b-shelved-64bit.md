# Report 05B — Why upstream shelved the partial 64-bit work

Explore pass B, territory 05, `shelved-64bit`. Investigation only; nothing in the
tree was modified. Contemporaneous notes: `modernization/investigation/notes-05b-shelved-64bit.md`.

**AMENDED 2026-09-09.** This report has been through adversarial review by
`adversary-05-1.md` and `adversary-05-2.md` and amended in place. Corrections are
marked **[CORRECTED]** at the point of the error, and Part III records the
adjudication of every claim put to the amendment pass, including the places where
an adversary is wrong. The single largest change is in section 1 and section 2:
**the capability is not off, it is un-defaulted**, and the report previously
described a disabled feature throughout.

Repository state: HEAD `ba0e81dce559fb63a5958bf82feb1d00c55c02fe`, branch `master`,
remote `upstream` only. Every local claim below carries a commit hash or a
`path:line`. Every GitHub claim is labelled GITHUB and was read through `gh api`
against `taichi-dev/taichi`. No writes were made to GitHub.

---

## 1. Headline

**The premise of the question needs splitting.** The four items in the brief are
not one shelved project. They are four unrelated artefacts with four different
histories, and only one of them was ever working code that got switched off.

| Item | What it actually is | Why it is in its current state |
|---|---|---|
| `spirv_has_physical_storage_buffer` | Working, shipped feature, live in **eleven** tagged releases | **Un-defaulted**, not disabled. Automatic enablement from the device was removed for **AOT cross-machine portability**, after a **MoltenVK driver bug** forced a macOS carve-out six days earlier. Two live producers remain |
| `use_64bit_pointers` | A stub with a half-written body, the declaration born `false` and never once true; the body's true branch was edited in 2023 | **Never started.** Not shelved |
| `snode.cpp` int64 warning | A detector added alongside a 2021 overflow *fix* | The fix was to warn, not to widen. int64 indexing was **never attempted** |
| `PointerType::addr_space_` | A field added in 2020 with a getter and no setter | **Born dead.** No reader outside the serialisation macro |

The one genuinely shelved piece is the physical storage buffer. And it does not
address SNodes **in live code**; section 2.6 records the dead code that shows the
intent was once otherwise.

### 1.1 [CORRECTED] The feature is un-defaulted, not off

VERIFIED, and this correction governs the whole report. An earlier version of
this document said "zero live producers" at section 2.5 and described a feature
that is switched off. That is wrong. What the `&& false` removed is **automatic
enablement from the device**. Two producers remain live and user-reachable, and
neither consults a device.

**Producer 1, the ahead-of-time module interface.** `Program::make_aot_module_builder`
at `taichi/program/program.cpp:541-556` takes `const std::vector<std::string> &caps`
and hands it to `translate_devcaps` at `:544`. `translate_devcaps` at `:514-538`
calls `str2devcap` at `:530` on each caller-supplied string.
`str2devcap` (`taichi/rhi/device_capability.cpp:6-13`) expands
`taichi/inc/rhi_constants.inc.h`, in which `spirv_has_physical_storage_buffer` is
a valid name at `:28`. The resulting config goes to
`GfxProgramImpl::make_aot_module_builder` (`taichi/runtime/program_impls/gfx/gfx_program.cpp:30-40`),
which passes it through **unexamined**, into `AotModuleBuilderImpl`'s `caps_`
member (`taichi/runtime/gfx/aot_module_builder_impl.cpp:13-27`), and from there
into `compilation_manager_.load_or_compile(config_, caps_, *kernel)` at `:75` and
`:119`. That reaches `spirv::KernelCompiler::compile` at
`taichi/codegen/spirv/kernel_compiler.cpp:25-37`, which assigns `params.caps =
device_caps` at `:37`, which becomes `TaskCodegen::caps_` at
`taichi/codegen/spirv/spirv_codegen.cpp:86` and is read at `:783`, `:2340`,
`:2416` and `:2491`.

**There is no device check at any point in that chain.** I looked for one at each
hop. The contrast is exact: the just-in-time path takes its capabilities from the
device, `GfxProgramImpl::get_device_caps` at `gfx_program.cpp:82-85` returning
`runtime_->get_ti_device()->get_caps()`, and that is the path the `&& false`
actually closes. The ahead-of-time builder does not go through it.

The C++ signature is the producer. The Python binding at
`taichi/python/export_lang.cpp:403` and `python/taichi/aot/module.py:86-110`
(the `caps` argument, reaching `make_aot_module_builder` at `:110`) is one
caller of it, and plan section 1.2 puts that frontend out of scope, but removing
the frontend does not remove the producer.

**Producer 2, the C API runtime.** `ti_set_runtime_capabilities_ext` at
`c_api/src/taichi_core_impl.cpp:317-334` builds a `DeviceCapabilityConfig` from
caller-supplied `TiCapabilityLevelInfo` values in a loop at `:326-330` and calls
`runtime2->get().set_caps(std::move(devcaps))` at `:331`. `set_caps`
(`taichi/rhi/public_device.h:855`) **replaces the device's capability set
wholesale**; it does not merge and it does not validate. A typed convenience
setter ships in the public C++ header at
`c_api/include/taichi/cpp/taichi.hpp:1156-1159`, and the enumerator is public as
`TI_CAPABILITY_SPIRV_HAS_PHYSICAL_STORAGE_BUFFER = 18` at
`c_api/include/taichi/taichi_core.h:399`.

**Consequence, and it is the reason this correction matters more than the
others.** VERIFIED: there is no test anywhere that exercises this capability.
`grep -rn 'physical_storage_buffer\|PHYSICAL_STORAGE_BUFFER\|buffer_device_address\|BufferDeviceAddress\|PhysicalStorageBuffer' tests/ c_api/tests/ benchmarks/ misc/`
returns **zero hits**. So the reachable-today path is a path into branches that
have never been executed by any test and have not been executed by the default
build since 2022-10-30. The risk is not a future decision to re-enable. It is
present.

I record what this does **not** show. It does not show that anybody does this,
that it produces correct SPIR-V, or that the resulting module runs. It shows that
nothing in the chain refuses it.

---

## 2. `spirv_has_physical_storage_buffer` — shelved, and why

### 2.1 It was not unfinished. It shipped.

**[CORRECTED] — the earlier table here was a four-tag spot check presented as a
census, and it started three releases too late.** Both adversaries caught it.
Adversary 1 gave 8 enabled starting at v1.0.0; adversary 2 gave 11 starting at
v0.9.0. **Adversary 2 is right and adversary 1 is three tags short.**

VERIFIED by mechanical enumeration over **all 121 tags** in the repository, not
by sampling and not by ancestry. For each tag the script extracts
`taichi/rhi/vulkan/vulkan_device_creator.cpp`, falling back to the pre-refactor
path `taichi/backends/vulkan/vulkan_device_creator.cpp`, locates the
`caps.set(...)` line, and classifies on the eight lines above it. Handling the
path move is what makes v0.9.0–v1.0.3 visible; a path-fixed sweep shows a false
gap there. The script matches the **capability name**, not the setter, which is
the second thing that matters: v0.9.x writes `ti_device_->set_cap(...)` rather
than `caps.set(...)`, so a setter-shaped grep misses those three tags entirely.
Adversary 1 says `git tag` yields 122; measured here it is **121**.

| Class | Tags | Count |
|---|---|---|
| File absent | everything up to and including v0.8.6 | 92 |
| File present, capability name absent | v0.8.7, v0.8.8, v0.8.9, v0.8.10, v0.8.11 | 5 |
| **ENABLED, unguarded** | v0.9.0, v0.9.1, v0.9.2, v1.0.0, v1.0.1, v1.0.2, v1.0.3, v1.0.4, v1.1.0, v1.1.2, v1.1.3 | **11** |
| Enabled except macOS, `#if !defined(__APPLE__)` | v1.2.0, v1.2.1, v1.2.2 | **3** |
| Un-defaulted, `#if !defined(__APPLE__) && false` | v1.3.0, v1.4.0, v1.4.1, v1.5.0, v1.6.0, v1.7.0, v1.7.1, v1.7.2, v1.7.3, v1.7.4 | **10** |

Reconciliation, per plan section 10.7: 92 + 5 + 11 + 3 + 10 = 121, the full tag
count. The capability appears in the file in 11 + 3 + 10 = **24** tags. There is
no v1.1.1 tag in the repository.

Written Feb 2022 (`7705f688a`, author date 2022-02-08). First tagged release
carrying it is **v0.9.0, 2022-02-22**. Last release carrying it enabled outside
macOS is v1.2.2. Live in production for roughly eight and a half months, then
macOS-excepted from v1.2.0, then un-defaulted from v1.3.0 onward and never
restored through v1.7.4.

### 2.2 Three disabling events, not one

VERIFIED, all from `git log -- taichi/rhi/vulkan/vulkan_device_creator.cpp`.

**Event A, 2022-10-24.** `f677fb1417d906b2c13d1fa308378c4272c28f19`, Yi Xu
(strongoier), "[vulkan] [bug] Stop using the buffer device address feature on
macOS (#6415)". Added `#if !defined(__APPLE__)`.

Cause, GITHUB, issue 6295 comment `1288522905` by strongoier, quoted:

> After bisecting MoltenVK commits, I found that the problem was introduced in
> https://github.com/KhronosGroup/MoltenVK/pull/1638, which enabled support for
> `VK_KHR_buffer_device_address` and `VK_EXT_buffer_device_address` on macOS 12.5.
> To make a healthy release, I'll submit a PR to stop using the feature on macOS.
> We can diagnose further about whether the problem comes from the feature itself
> or the use of the feature in Taichi in the future.

The symptom was silent wrong answers: `from_numpy()` returning all zeroes on
macOS/Vulkan while correct on Metal. Bisected to a **MoltenVK** change, not a
Taichi change. **This is a driver limitation.** It was explicitly left open
whether the fault lay in the feature or in Taichi's use of it, and nobody ever
returned to say.

**Event B, 2022-10-30.** `d20f55dc8a48ebaf016205d8eaa1bd0a0681f55d`, PENGUINLIONG,
"[aot] Disabled physical storage buffer temporarily (#6468)". Changed the guard to
`#if !defined(__APPLE__) && false`, killing it on every platform. **The commit
body is empty.**

Cause, GITHUB, PR 6468 body, quoted in full:

> Disabled for now because of multi-platform compatibility issues. Will be
> recovered when device capability is fully functional.

Approved by ailzhang with no comment. The only other activity on the PR is a
Netlify bot. **There is no design discussion anywhere on this PR.**

**Event C, 2022-11-02.** `12546578a4a954979be73ba0dab8e0c73612b826`, PENGUINLIONG,
"[aot] Disable physical storage buffer in runtime (#6494)". Commented out the
second automatic producer at `c_api/src/taichi_vulkan_impl.cpp:45-51`
(**[CORRECTED]**, the earlier span `:44-52` was one line wide at each end; `:45`
is the explanatory comment, `:46` the `/*` and `:51` the `*/`, and it is a
comment block rather than a preprocessor guard, unlike the Vulkan one). Body:

> The physical storage buffer codegen was disabled but the runtime can still use
> them. This is an attempt to disable psb at runtime. Will bring it back after
> devcap.

A mop-up of event B: the codegen and the imported-runtime path had come to
disagree about the capability.

### 2.3 What "multi-platform compatibility issues" means

INFERRED, but from a tight evidence chain, and I state the inferential step
explicitly.

VERIFIED premises:
- GITHUB. PR 6184, "[aot] Device capability refactorization", PENGUINLIONG, merged
  2022-10-21 (commit `07446dd0c`), nine days before event B. Body: *"The current
  implementation collect capabilities from the device interface but it blocks
  extension of AOT code generation because it depends on the currently attached
  compute device whether to enable a set of feature during compilation. AOT users,
  however, demands an interface to control the availability of DeviceCapability."*
- `c_api/src/taichi_gfx_impl.cpp:18-30`. Loading an AOT module compares each
  required capability against the runtime's and returns
  `TI_ERROR_INCOMPATIBLE_MODULE` on `current_version != required_version` — an
  **inequality, not a floor**. A module built with PSB fails to load on any runtime
  that does not report the identical level.
- Every disabling commit is tagged `[aot]`, and the whole surrounding commit window
  is AOT and C-API work.

INFERRED: the "multi-platform compatibility issues" are AOT modules compiled with
PSB on one machine failing to load on another. Removing the capability from the
device-derived set removes one source of that mismatch. This is the cheapest
possible fix and it matches the tag, the timing and the stated precondition.

**[CORRECTED].** An earlier version of this paragraph said the change "makes
every module portable at a stroke." **That is false, and adversary 1 is right to
call it material.** VERIFIED: the check at `c_api/src/taichi_gfx_impl.cpp:22-29`
loops over **every** entry in the module's required capability set and rejects on
`current_version != required_version` at `:25`. Six further capabilities are
still derived from the device with no guard at all, in
`taichi/rhi/vulkan/vulkan_device_creator.cpp`:

| Capability | Set at | Derived from |
|---|---|---|
| `spirv_has_int16` | `:628` | `device_supported_features.shaderInt16` |
| `spirv_has_int64` | `:632` | `device_supported_features.shaderInt64` |
| `spirv_has_float64` | `:636` | `device_supported_features.shaderFloat64` |
| `spirv_has_subgroup_basic` | `:659` | `subgroup_properties.supportedOperations` |
| `spirv_has_subgroup_vote` | `:663` | same |
| `spirv_has_subgroup_arithmetic` | `:667` | same |
| `spirv_has_subgroup_ballot` | `:671` | same |

Seven rows, seven capabilities, derived from two device queries at `:624` and
`:654`. Any of them differing between build machine and run machine produces the
same `TI_ERROR_INCOMPATIBLE_MODULE`. **Un-defaulting the physical storage buffer
removed one of at least eight sources of mismatch, not all of them.**

This weakens the inference above rather than overturning it: the AOT equality
check does explain why a capability that varies by device is a portability
hazard, but it does not on its own explain why **this** capability was singled
out while `spirv_has_int64` was left alone. The unexplained residue is real and I
am not filling it with a guess. What survives unchanged is that
`spirv_has_int64` bites the same way, which is a live constraint on plan section
6.3 regardless of what happened in 2022.

**This is a deployment-model decision, not a technical objection to 64-bit
addressing.**

### 2.4 Why it was never restored

VERIFIED. The stated precondition — devcap being "fully functional" — was
substantially met. The follow-ups PR 6184 promised all landed: `29749416a` (#6549),
`a0227ca89` (#6618), `fdde622a5` (#6623), `c27a2e474` (#6702), reverted by
`3711e8b18` (#6772) and re-landed as `bed652dc2` (#6773), then `285fe5213` (#7407,
2023-02-21) and `fcd46769e` (#7506).

Capability configuration exists today on both user-facing paths:
`taichi/program/program.cpp:514-538` (`translate_devcaps`, feeding
`make_aot_module_builder` at `:541-556`), and
`c_api/src/taichi_core_impl.cpp:317-334` (`ti_set_runtime_capabilities_ext`), with
a typed setter at `c_api/include/taichi/cpp/taichi.hpp:1156-1160`.

**Nobody lifted the `&& false`.** It is still at
`taichi/rhi/vulkan/vulkan_device_creator.cpp:825` at the fork commit, dated
2025-07-30, and in v1.7.4. Exact shape, VERIFIED: `:825` is
`#if !defined(__APPLE__) && false`, `:826` the single `caps.set`, `:827` the
`#endif`, all inside `if (device_supported_features.shaderInt64)` at `:821`,
itself inside the buffer-device-address feature test at `:813-820`.

VERIFIED: no branch, tag or ref carries a continuation.
`git log --all --not master -S "spirv_has_physical_storage_buffer"` returns only
ghstack scratch refs and release cherry-picks for work that landed in master. A
sweep of all `refs/remotes/upstream/*` for a ref with the capability set and no
`&& false` returns only branches that predate 2022-10-30.

**Classification: shelved for a driver limitation (macOS), then for an AOT
portability problem (globally), then never restored because the project lost
momentum. Not shelved as broken. Not shelved as wrong.**

### 2.5 [REBUILT] The consumer inventory

The earlier list here was rebuilt from scratch rather than patched, because it
had three separate defects: it said "zero live producers", which section 1.1
corrects; it counted twelve while listing bullets that do not correspond to the
twelve; and it omitted three consumers, which both adversaries independently
found and named identically. Method: grep the whole tree for the capability
name, then follow the `bool has_buffer_ptr` value it is converted into, then
check reachability of each hit.

**Tier 1 — direct readers of the capability. Twelve, derived by enumeration.**

| # | Site | What it gates |
|---|---|---|
| 1 | `taichi/rhi/vulkan/vulkan_device.cpp:1772` | buffer usage flag |
| 2 | `taichi/rhi/vulkan/vulkan_device.cpp:1792` | `vkGetBufferDeviceAddressKHR` |
| 3 | `taichi/rhi/vulkan/vulkan_device.cpp:2147` | device-address usage on import |
| 4 | `taichi/rhi/vulkan/vulkan_device.cpp:2509` | VMA device-address allocator bit |
| 5 | `taichi/codegen/spirv/spirv_ir_builder.cpp:73` | `OpCapability PhysicalStorageBufferAddresses`, emitted at `:74-76` |
| 6 | `taichi/codegen/spirv/spirv_ir_builder.cpp:113` | `OpExtension SPV_KHR_physical_storage_buffer` at `:114-116` and `OpMemoryModel AddressingModelPhysicalStorageBuffer64` at `:119-122`, against `AddressingModelLogical` at `:124-126` |
| 7 | `taichi/codegen/spirv/spirv_codegen.cpp:783` | `visit(ExternalPtrStmt *)`, ndarray base address |
| 8 | `taichi/codegen/spirv/spirv_codegen.cpp:2340` | `compile_args_struct` layout |
| 9 | `taichi/codegen/spirv/spirv_codegen.cpp:2416` | `compile_argpack_struct` layout |
| 10 | `taichi/codegen/spirv/spirv_codegen.cpp:2491` | `compile_ret_struct` layout |
| 11 | `taichi/runtime/gfx/runtime.cpp:95-96` | host writes `get_memory_physical_pointer` into the args buffer at `:97-102` |
| 12 | `taichi/runtime/program_impls/gfx/gfx_program.h:80-81` | argument layout string `"1b"` versus `"1-"` at `:82` |

Twelve rows, counted from the rows. Declaration at
`taichi/inc/rhi_constants.inc.h:28`; public C enumerator at
`c_api/include/taichi/taichi_core.h:399`; typed C++ setter at
`c_api/include/taichi/cpp/taichi.hpp:1156-1159`; Python name at
`python/taichi/lang/enums.py:38`. None of the twelve is on the SNode path, which
is the finding section 3 rests on and which survives the rebuild.

**Tier 2 — indirect consumers, reached through `bool has_buffer_ptr`. Three, all
three missing from the earlier list, and all three named identically by both
adversaries.** This is the one place where the two explore passes shared a blind
spot, and it is confirmed twice independently.

1. **`taichi/codegen/spirv/spirv_types.cpp:484-514`, `translate_ti_type`.** The
   function that rows 8, 9 and 10 all call. The pointer-width decision is at
   `:490-497`: a Taichi `PointerType` becomes `IntType(64, unsigned)` at
   `:491-493` under the flag, `IntType(32, unsigned)` at `:494-496` otherwise.
   This is where the decision is actually made; the three call sites only carry
   the boolean.
2. **`taichi/runtime/gfx/runtime.cpp:823` and `:851-859`,
   `GfxRuntime::get_struct_type_with_data_layout_impl`.** The **host** half of the
   same ABI. `has_buffer_ptr = layout[1] == 'b'` at `:823`, then a `PointerType`
   member is sized and aligned `sizeof(uint64_t)` at `:853-854` or
   `sizeof(uint32_t)` at `:857-858`. Row 12 produces the string; this consumes it.
   Its callers are `taichi/program/callable.cpp:159-161` (return struct) and
   `:180-182` (argument struct), plus `taichi/program/argpack.cpp:15`. Argument
   and return structs only, so it does not disturb the separation in section 3,
   but it belongs in the inventory and it is where section 2.7's mismatch bites.
3. **`taichi/codegen/spirv/spirv_ir_builder.cpp:334-352`, `from_taichi_type`.** A
   **second, independent implementation** of the same pointer-width rule, `t_uint64_`
   at `:338-339` against `t_uint32_` at `:340-341`. It is **dead**: `grep -rn
   from_taichi_type taichi/ c_api/` returns exactly three hits, the definition at
   `:334`, its own recursive call at `:347`, and the declaration at
   `spirv_ir_builder.h:343`. No caller. Anyone widening the pointer rule for item
   6.2 has two copies in front of them, one of which compiles and does nothing.

**One removal. `spirv_types.cpp:433-439` was in the earlier list and does not
belong there.** `visit_physical_pointer_type` in the SPIR-V translator emits
`OpTypePointer` with `spv::StorageClassPhysicalStorageBuffer`, but it fires only
on a `PhysicalPointerType` node, and VERIFIED there is no live producer of one.
`grep -rn PhysicalPointerType taichi/` gives exactly two construction sites:
`snode_struct_compiler.cpp:53`, which is dead (section 2.6), and
`spirv_types.cpp:337`, inside the type reducer, which only copies a node that
already exists. Adversary 2 is right on this and adversary 1 passed over it.

**Nothing was ripped out — but two things rotted in place.** `from_taichi_type`
is orphaned and `construct` is entombed (section 2.6). The earlier sentence "the
machinery is intact" was too clean.

### 2.6 [NEW] Dead SNode code that contradicts the intent half of section 3

VERIFIED, and it is the only line in the tree connecting the physical storage
buffer to SNodes. Adversary 2 found it; adversary 1 and both explore passes,
including this one, missed it.

`taichi/codegen/spirv/snode_struct_compiler.cpp:34-63` is a private method
`construct` that walks an SNode tree and builds a `tinyir` type for it. At
`:52-54`:

```cpp
      if (sn->type == SNodeType::pointer) {
        cell_type = ir_module.emplace_back<PhysicalPointerType>(cell_type);
      }
```

`PhysicalPointerType` is declared at `taichi/codegen/spirv/spirv_types.h:71-91`,
fixed at 64 bits unsigned by its own constructor at `:74-77`,
and its SPIR-V translation at `spirv_types.cpp:433-439` emits `OpTypePointer`
with `spv::StorageClassPhysicalStorageBuffer` — the storage class that exists
only under this capability. That is the SNode struct compiler proposing to give
**sparse `pointer` SNode cells physical device addresses**.

It never ran. VERIFIED:
- Its only call site is commented out at `snode_struct_compiler.cpp:16-19`, inside
  `StructCompiler::run`.
- The `CompiledSNodeStructs` fields it would write, `type_factory` and
  `root_type`, are commented out at `snode_struct_compiler.h:48-50` under the
  comment `// TODO: Use the new type compiler`. `grep -rn "root_type"` across
  `taichi/` returns no user outside this file.
- `git log -S "PhysicalPointerType" --all -- taichi/` returns exactly **one**
  commit, `46b5632e5e61036a788013780d0d9b9eacfd7f4f`, Bob Cao, 2022-06-22,
  "[vulkan] Codegen & runtime improvements (#5213)". Reading its diff, the call
  site was **added already commented out**. It was never live for a single
  commit.

**How this bears on section 3.** Section 3's claim that the physical storage
buffer never addressed SNodes is correct about **shipped behaviour** and wrong
about **design intent**. Four months after landing the capability, its author
sketched an SNode type system in which sparse `pointer` nodes carry physical
device addresses, and committed the sketch commented out. That is not a reusable
asset — forty lines that never compiled into anything, against a
`CompiledSNodeStructs` shape that has since moved on — but it is evidence, and
the earlier flat statement suppressed it. Per plan section 10.3 I am not calling
it surplus and I am not calling it an asset. It goes to the planner in
Escalations.

### 2.7 [NEW] A host/device layout mismatch sitting in the un-tested branch

VERIFIED as code, with the reachability question left open. Adversary 1 found
this and it is the sharpest single find in either adversary file.

`taichi/runtime/program_impls/gfx/gfx_program.h` produces both layout strings.
The **argument** string at `:79-83` reads the capability and returns `"1b"` or
`"1-"`. The **return** string at `:75-77` is:

```cpp
  std::string get_kernel_return_data_layout() override {
    return "4-";
  };
```

A literal, with no capability read. Meanwhile `compile_ret_struct` at
`taichi/codegen/spirv/spirv_codegen.cpp:2490-2494` **does** read the capability
at `:2490-2491` and threads it into `translate_ti_type` at `:2494`.

So with the capability on, the shader lays out a `PointerType` member of a return
struct at 64 bits, while the host, through
`get_struct_type_with_data_layout_impl` reading `'-'` out of `"4-"` at
`runtime.cpp:823` and taking the `:857-858` branch, sizes and aligns it at 32.
The consumer is `Callable::finalize_rets` at `taichi/program/callable.cpp:159-161`.
The argument path is self-consistent; the return path is not.

**Reachability: NOT ESTABLISHED, and my hedge is stronger than adversary 1's.**
Adversary 1 leaves it as a frontend question, out of scope under plan section 1.2.
There is a second reason to doubt it, from C++ alone. `finalize_params` at
`callable.cpp:170-177` **synthesises** pointer members deliberately, wrapping
array parameters via `TypeFactory::get_instance().get_pointer_type(...)` at
`:174`. `finalize_rets` at `:150-156` does no such wrapping: a return member is
`rets[i].dt` verbatim at `:155`. So a return struct contains a `PointerType` only
if a declared return dtype is itself one, which nothing in C++ constructs. The
mismatch is real in the code and I could not reach it. Recorded as a latent
defect in the branch section 1.1 shows is user-reachable, not as a live fault.

The branch was added by `88ac098bc` (#8061, 2023-05-23), one of the ten commits
in the 2023 argument-refactor campaign, with no matching change to
`get_kernel_return_data_layout`.

---

## 3. The finding that matters most for item 6.2

VERIFIED, and this is the claim I most want tested adversarially.

**The physical storage buffer never addressed SNodes. It addressed ndarrays.**

`spirv_has_physical_storage_buffer` is consumed in codegen at:
- `spirv_codegen.cpp:783`, inside `visit(ExternalPtrStmt *)` (opens at `:734`) —
  loads a `u64` device address from the args buffer for an **external array /
  ndarray** argument.
- `:2340`, `:2416`, `:2491` — args, argpack and ret **struct layout**, so a
  pointer member is laid out 64-bit wide.
- `:2201`, inside `at_buffer` at `:2194` — the join: a `u64` pointer becomes a
  physical pointer via `OpConvertUToPtr`; anything else is a 32-bit offset shifted
  into a bound storage buffer.

The SNode path uses a different mechanism entirely, `make_pointer`
(`spirv_codegen.cpp:2317-2324`), called at exactly four sites:
- `:355` `visit(GetRootStmt *)` — `make_pointer(0)`
- `:371` `visit(GetChStmt *)`
- `:408` `bitmasked_activation`
- `:506` SNode lookup

`make_pointer` is what `use_64bit_pointers` gates. **SNode root addressing on the
SPIR-V path is a 32-bit byte offset into a bound buffer and always has been.**
Turning the physical storage buffer back on would widen the ndarray **base
address** and would not move the SNode ceiling at all.

### 3.1 [CORRECTED] What the capability actually buys on the ndarray path

An earlier version of this section said restoring it "would widen ndarray
addressing." That overcredits it, and both adversaries caught the same thing.

VERIFIED. In `visit(ExternalPtrStmt *)`, `spirv_codegen.cpp:734-795`, the entire
index computation is `i32`. `linear_offset` is initialised `i32` at `:737`,
accumulated by `mul` and `add` at `:770-771`, and byte-shifted at `:773-777` with
an `i32` result type. Only at `:790` is it `OpSConvert`-ed to `u64` and added to
the `u64` base loaded at `:786-789`.

So the capability delivers a **64-bit base address plus a 32-bit signed byte
offset**. It does not lift a per-array indexing limit. This makes the conclusion
in section 3 stronger, not weaker: the capability buys even less than the earlier
wording credited it with.

### 3.2 [NEW] The qualifier the convergent finding needs

INFERRED on its second half, and I state the inferential step. This is adversary
1's addition, and it qualifies section 3 rather than refuting it. I checked it
and it holds.

VERIFIED, from Taichi source:
- The SNode root is a **descriptor-bound storage buffer**. `get_buffer_value` at
  `spirv_codegen.cpp:2266-2315` binds it through `ir_->buffer_argument(...)` at
  `:2308-2309`; `bitmasked_activation` binds it explicitly at `:401-402`.
- `at_buffer`'s non-`u64` path at `:2212-2218` fetches that bound buffer at
  `:2212`, shifts the byte offset right by `log2(width)` at `:2214-2216`, and
  hands the result to `struct_array_access` at `:2217-2218`. The offset it shifts
  is whatever `make_pointer` produced: a `u32` today.
- **This tree never queries `maxStorageBufferRange`.** `grep -rn
  maxStorageBufferRange taichi/ c_api/` returns **zero hits**. I re-ran it.

INFERRED, from the SPIR-V and Vulkan addressing models rather than from any
Taichi line: a descriptor-bound storage buffer's reach is capped by
`maxStorageBufferRange`, and SPIR-V offers exactly two addressing mechanisms — a
bound buffer with an offset, and a physical device address. Widening
`make_pointer` to `u64` while staying on a bound buffer widens an offset into a
range the offset was never what limited.

**Consequence for item 6.2, and the planner should carry it with the reversal.**
The correct statement is not "the shelved work is irrelevant to item 6.2." It is
**"the shelved work is not reusable as written, and is simultaneously the only
door SPIR-V offers for an SNode root that outgrows a bound buffer."** A bare
reversal would mislead.

Adversary 1 marks the Vulkan half inferred on the ground that Taichi has no
source line on it. **Adversary 2 supplies one, and it is a real upgrade to
adversary 1's argument.** `snode_struct_compiler.cpp:53` (section 2.6) is
precisely that design — sparse `pointer` SNode cells carrying physical device
addresses — applied to SNodes by the author of the mechanism, four months after
he landed it. It never ran and cannot be run, so it does not make the inference
verified. It moves it from "deduced from the specification" to "deduced from the
specification and independently arrived at by the person who wrote the
mechanism."

---

## 4. `use_64bit_pointers` — never started, not shelved

VERIFIED. `taichi/codegen/spirv/spirv_codegen.cpp:82`:
`const bool use_64bit_pointers = false;`

Both `git log -S "use_64bit_pointers" --all` and
`git log -L 82,82:taichi/codegen/spirv/spirv_codegen.cpp` return **one** commit:
`1b34a2d4d16ee2faaf52af926e9476f46325054d`, Bob Cao, author date 2021-10-25,
"[vulkan] Indexed load codegen (#3259)". The **declaration** was born `false` and
has never been edited, four months before the physical storage buffer work
existed. `git log --all --not master -S "use_64bit_pointers"` returns nothing.

**[CORRECTED] — the body is not untouched, and the earlier phrasing "born
`false`, never edited" invited the wrong reading.** Both adversaries flagged it.
VERIFIED: `git log -L 2317,2324:taichi/codegen/spirv/spirv_codegen.cpp` returns
**two** commits, `1b34a2d4d` and `715a04c98`. The second is Bob Cao, 2023-08-15,
"[vulkan] Fix shared memory atomic float operations (#8315)", and its diff on
this file is exactly the one-line change from a cast-widened `u32` to a direct
`u64` immediate:

```
-      return ir_->cast(ir_->u64_type(), ir_->uint_immediate_number(
+      return ir_->uint_immediate_number(ir_->u64_type(), offset);
```

The earlier text described both forms in prose but did not cite the commit that
changed one into the other. It matters to section 9's momentum reading:
**somebody went into unreachable code and corrected it ten months after the
un-defaulting**, which is the only positive signal against the momentum
inference and should not be suppressed. It does not overturn it. See section
13.6.

Its body, `:2317-2324`:

```cpp
  spirv::Value make_pointer(size_t offset) {
    if (use_64bit_pointers) {
      // This is hacky, should check out how to encode uint64 values in spirv
      return ir_->uint_immediate_number(ir_->u64_type(), offset);
    } else {
      return ir_->uint_immediate_number(ir_->u32_type(), uint32_t(offset));
    }
  }
```

In the introducing commit the true branch read
`ir_->cast(ir_->u64_type(), ir_->uint_immediate_number(ir_->u32_type(), uint32_t(offset)))`
— it computed 32 bits and widened, so it could never have carried a wide offset.

VERIFIED: flipping the flag would not produce a working build, for three
independent reasons. Reason 1 stands as written. Reason 2 is **[CORRECTED]**: it
was wrong on four of its five citations and missed the clearest instance. Reason
3 is new.

1. **The `at_buffer` collision.** `at_buffer` (`:2194-2220`) discriminates at
   `:2197` on `ptr_val.stype.dt == PrimitiveType::u64` and, on that branch, emits
   `OpConvertUToPtr` at `:2198-2202` to a
   `spv::StorageClassPhysicalStorageBuffer` pointer — that is, it treats any
   `u64` pointer as an absolute **device address**. But `visit(GetRootStmt *)` at
   `:351-357` supplies `make_pointer(0)` at `:355` — a literal zero, a
   buffer-relative offset. With the flag on that zero becomes a `u64` and
   converts to a null physical pointer. `ptr_to_buffers_[stmt]`, the map entry
   set at `:377` that names the root buffer, is consulted only on the
   fall-through path at `:2212` and is silently never read. The same
   `== PrimitiveType::u64` test recurs in `load_buffer` at `:2227` and
   `store_buffer` at `:2249`. Section 13.5 adjudicates whether this is
   architectural.

2. **Type mixing in `bitmasked_activation`, `:383-435`.** `ptr_dt` is taken from
   `parent_ptr.stype` at `:388` and becomes `u64` under the flag. The earlier
   version of this list cited `:404`, `:411`, `:412`, `:413` and claimed a u32
   was added to `parent_ptr` at `:409`. Re-read line by line:

   | Line | Earlier claim | What the source says |
   |---|---|---|
   | `:398-399` | not cited | **A real defect, and the clearest one.** `OpShiftLeftLogical`, Result Type `ptr_dt`, Base `ir_->const_i32_one_`. At 32 bits today Base and Result agree in width; under the flag Result is 64 and Base is 32. Missed entirely by the earlier version |
   | `:404` | cited as a defect | **Not a defect.** Result Type is `ptr_dt`. The `u32_type()` is on `:405` and is the shift **amount**, which SPIR-V permits to differ in width from Base |
   | `:409` | "adds a u32 to `parent_ptr`" | **Wrong.** `ir_->add(parent_ptr, bitmask_word_ptr)`; under the flag both operands are `ptr_dt`. `bitmask_word_ptr` arrives from `:403-404` as `ptr_dt`, added at `:406-408` to a `make_pointer(...)` result, also `u64`. No mismatch |
   | `:410-412` | `:411`, `:412` cited | **A real defect**, at `:410-412` as one construct: `OpShiftRightLogical`, Result Type hardcoded `ir_->u32_type()`, Base `bitmask_word_ptr` typed `ptr_dt`. The `u32_type()` at `:412` is again the shift amount and is fine on its own |
   | `:413` | cited as a defect | **Off by one.** `:413` is the assignment; the `struct_array_access(ir_->u32_type(), ...)` is at `:414` |

   So the defects are `:398-399` and `:410-412`, both Base/Result width
   mismatches on shift instructions. The rule as SPIR-V enforces it is equal bit
   width and component count between Base and Result Type, saying nothing about
   the Shift operand or about signedness.

   The earlier version also stated the failure mode wrongly. `ir_->add` is
   generated by `DEFINE_BUILDER_BINARY_USIGN_OP` at
   `spirv_ir_builder.cpp:1038-1047`, which opens with
   `TI_ASSERT(a.stype.id == b.stype.id)` at `:1040`. A genuine operand mismatch
   there would trip a **host-side assertion during codegen**, not emit invalid
   SPIR-V.

3. **[NEW] `make_pointer` bypasses the `shaderInt64` guard, silently.** Adversary
   2 found this and it is the one failure mode of the three that produces no
   diagnostic. VERIFIED: `make_pointer` at `:2317-2324` calls `ir_->u64_type()`,
   which at `spirv_ir_builder.h:532-534` is a bare `return t_uint64_;` with no
   guard. `t_uint64_` is assigned only inside
   `if (caps_->get(cap::spirv_has_int64))` at `spirv_ir_builder.cpp:166-169`. The
   `TI_ERROR` guards at `:311-313` and `:325-327` that section 7.1 of this report
   offers as a safety net live in `get_primitive_type(DataType)`, and
   `make_pointer` does not call it. **The safety net does not cover the one path
   item 6.2 has to widen.** On a device without `shaderInt64`, flipping the flag
   yields a default-constructed `SType` and an undeclared type id, with nothing
   said. Under plan section 2.2 that is the worst available shape: correct code
   on a card that has the feature and silently wrong code on one that does not,
   which is hardware changing capability rather than speed.

**Classification: a placeholder whose author recorded, in the comment, that he had
not worked out the encoding. Its value here is as a precise marker of where SNode
address width lives on the SPIR-V path, not as reusable code.**

---

## 5. `snode.cpp` int64 indexing — never attempted

VERIFIED. `taichi/ir/snode.cpp:95-100`, warning text at `:97-99`.

Origin: `cfc4065d3c7614b98584d7f5c0c43c1f4cebd30f`, Ailing, 2021-10-21, "[bug] Fix
silent int overflow in indice calculation. (#3177)". That commit widened
`acc_shape` to `int64` **but kept the store narrow** —
`new_node.extractors[i].acc_shape = static_cast<int>(acc_shape);` with the comment
"casting to int32 in extractors." The fix was to *detect* the overflow, not remove
it. The same commit added a parallel warning to the Metal struct compiler.

Second edit: `b347a21592221527e40d89706192e8b6e43c1a40`, Yi Xu, 2022-10-28, "[bug]
Fix false overflow alarm in struct fors under packed mode (#6457)", fixing issue
6258. It added "Struct fors might not work either" and, in
`taichi/transforms/demote_dense_struct_fors.cpp`, replaced
`TI_ASSERT(total_bits <= 30)` with
`TI_ASSERT(total_n <= std::numeric_limits<int>::max())`, now at `:29`. That is a
hard assert, not a warning, and it has not moved since.

### 5.0 [NEW] The hard stop does not fire on this project's workload, and the binding width is elsewhere

**This is the most important correction in this amendment, because it is the
constraint that actually applies to plan item 6.2 on this project's data.** Both
adversaries reached it independently and neither explore pass had it.

**First: the assert at `demote_dense_struct_fors.cpp:29` is gated, and the gate
excludes sparse trees.** VERIFIED. `convert_to_range_for` at `:13-112` is the
only function containing the assert, and its only caller is `maybe_convert` at
`:114-119`:

```cpp
void maybe_convert(OffloadedStmt *stmt) {
  if ((stmt->task_type == TaskType::struct_for) &&
      stmt->snode->is_path_all_dense) {
    convert_to_range_for(stmt);
  }
}
```

`is_path_all_dense` is declared at `taichi/ir/snode.h:114`, defaulting true, and
is cleared at `taichi/ir/snode.cpp:20`:
`new_ch->is_path_all_dense = (is_path_all_dense && !new_ch->need_activation());`
— so a `pointer`, `bitmasked` or `dynamic` node anywhere on the path clears it
for that node and everything below. `taichi/transforms/offload.cpp:192` reads the
same flag for the same decision. **Plan section 4.1 requires sparsity. On a
sparse tree that assertion never executes.** The 2^31-1 total-cell cap that this
assert enforces is not the ceiling this project will hit.

That is a correction to the report, not a dismissal of the assert. It is a real
hard stop: `TI_ASSERT` at `taichi/common/logging.h:100` expands to
`TI_ASSERT_INFO`, defined at `:101-107` as a plain `if (!x) TI_ERROR(...)` with
no `NDEBUG` gate, so it fires in every build rather than being compiled out. It
is what upstream users actually report hitting: issue 8608's terminating line is
`[demote_dense_struct_fors.cpp:convert_to_range_for@29] Assertion failure: total_n <= std::numeric_limits<int>::max()`.
It is the operative constraint **on all-dense trees**.

**Second: for the sparse case the binding width is three `int` fields that
nobody in either explore pass named.** VERIFIED,
`taichi/ir/snode.h:37-54`, `struct AxisExtractor`:

| Field | Declared | Type |
|---|---|---|
| `num_elements_from_root` | `taichi/ir/snode.h:41` | `int` |
| `shape` | `taichi/ir/snode.h:45` | `int` |
| `acc_shape` | `taichi/ir/snode.h:49` | `int` |

Against which `SNode::num_cells_per_container` at `taichi/ir/snode.h:97` is
**already `int64`**. So the widening is smaller than this report previously
implied: the 64-bit half of the pair is already there and the extractors were
left behind.

The ordering matters and this report previously read it backwards.
`taichi/ir/snode.cpp:89-101`: `acc_shape` is a local `int64` at `:89`, the store
into the 32-bit field at `:92` truncates on **every iteration** of the loop at
`:90-94`, and the check at `:95` runs once, afterwards, on the untruncated local.
**The warning at `:96-100` is a post-hoc detector for damage already done, not a
guard against it.**

**Third, and this refines the claim as it was put to me.** The three fields are
not equally load-bearing, and which one binds depends on the path. Derived by
enumerating every reader of each:

- **`acc_shape` (`:49`) has two consumers: `taichi/codegen/llvm/struct_llvm.cpp:174`
  and `:176`, and `taichi/transforms/demote_dense_struct_fors.cpp:74` and `:76`.**
  That is the LLVM spine and the dense-demotion pass. **Neither is the SPIR-V
  sparse path.**
- **`num_elements_from_root` (`:41`) is the one that binds on the SPIR-V path.**
  Its consumer is `taichi/codegen/spirv/snode_struct_compiler.cpp:115-122`, where
  `sn_desc.total_num_cells_from_root *= e.num_elements_from_root;` at `:121`
  accumulates over all extractors, for **every** SNode type including sparse
  ones. The destination `SNodeDescriptor::total_num_cells_from_root`
  (`snode_struct_compiler.h:28`) is a `size_t`, so it is wide — but every factor
  it multiplies was already truncated to `int`. The field is itself accumulated
  at `snode.cpp:22-23` and `:83` with **no overflow check anywhere**: no warning,
  no assert, unlike `acc_shape` at `:95`. `SNode::shape_along_axis`
  (`snode.cpp:174-177`) returns it as `int` as well.
- **`shape` (`:45`)** feeds `acc_shape`'s accumulation at `snode.cpp:93` and the
  dense demotion at `demote_dense_struct_fors.cpp:24` and `:74`.

So for plan section 4.1's sparse workload on the SPIR-V spine, the load-bearing
32-bit field is `taichi/ir/snode.h:41`, and it is the one with no detector at
all. The claim that all three are the binding width is right that all three are
`int` and right that nobody named them; it is imprecise in putting `:49` on this
project's path.

One further unnamed truncation, VERIFIED, in the dense pass itself:
`demote_dense_struct_fors.cpp:19` declares
`std::array<int, taichi_max_num_indices> total_shape` and `:23-25` multiplies
into it in a loop, with no check, while `total_n` beside it at `:18` is `int64`
and **is** checked at `:29`.

This overlaps territory 01 and I defer to that pair on completeness of the
frontend map. It is recorded here because it is the answer to plan section 8.1.1
for this corner and because it fell between two territories.

### 5.1 GITHUB — upstream's own position, and its silence

- **Issue 8608, "Implement int64 indexing", OPEN**, filed 2024-12-18 by lstrgar,
  label "feature request". Quotes both the `snode.cpp` warning and the
  `demote_dense_struct_fors.cpp:29` assert. Two comments, both users asking for
  movement (bgailleton 2025-06-30, lstrgar 2025-07-14). **No maintainer has ever
  replied.** Still open.
- **Issue 5320, "Add int64 dtype support for RangeFor boundaries", OPEN**, filed
  2022-07-04 by jim19930609, a maintainer. The nearest thing to a design statement
  upstream produced:

  > Ideally, backend's dtype limitation should not bother the frontend design.
  > Unfortunately, we did forbiddened higher precision dtypes in certain cases due
  > to concerns regarding the backends ... We should add a "dtype demotion" pass
  > for each specific backend, and remove all these dtype conversions from the
  > frontend.

  Same author, 2022-07-08: *"PR #5322 just added a warning message for this
  implicit conversion, which did not actually resolve this issue."* Subsequent
  comments are users only, through 2026-05.
- **Issue 8161, "ndrange is limited in size", OPEN**, 2023-06-08. listerily,
  2023-06-09: *"Seems that M*M has overflown int32. There may be some difficulties
  to resolve it since some back-ends do not support int64. We will investigate it
  later."* No further maintainer activity.

**Classification: never started. The recorded maintainer position is that the
frontend narrows to int32 because some backends lack int64, and that the right fix
is a per-backend demotion pass. That design was proposed in July 2022 and never
implemented. Upstream's tracking issue is open and unanswered.**

---

## 6. `PointerType::addr_space_` — born dead

VERIFIED. `taichi/ir/type.h:207`: `int addr_space_{0};  // TODO: make this an enum`,
getter at `:191-193`.

`git log -S "addr_space_" -- taichi/ir/type.h` returns two commits:
- `dcd5d7d35b6e4f4065fd6e00f1229114eb8510df`, Yuanming Hu, 2020-10-12, "[type] Add
  basic implementations of VectorType and PointerType (#1948)" — the file's
  creation. Added the field, the getter, **and no constructor parameter or setter**.
- `48fa799da`, lin-hitonami, 2023-03-06, "[type] Let Type * be serializable" (PR
  #7460) — purely mechanical, adds it to `TI_IO_DEF` at `:203`.

`grep -rn "addr_space" taichi/ c_api/ python/ tests/` returns **seven hits, four
of them inside `type.h` itself** — the getter body at `:191-193`, the
serialisation macro at `:203` and the field at `:207` — and three in
`taichi/codegen/llvm/`, which are the unrelated LLVM parameter. **No call site of
`get_addr_space` outside its own definition.**

**[CORRECTED] on two points, both from adversary 2, both verified.**
- The constructors are at `:179` (default) and `:181-185` (taking
  `Type *pointee, bool is_bit_pointer`), not `:184` and `:186-191` as this report
  previously said. Neither takes the field, and no setter exists anywhere.
- "Zero call sites, ever" is not literally true. `TI_IO_DEF(pointee_,
  addr_space_, is_bit_pointer_)` at `:203` reads it, so it is serialised into the
  offline cache and AOT type records. It is always zero, so nothing observable
  follows, but the absolute claim was wrong.

**Classification: not shelved 64-bit work at all. A placeholder field added
alongside the type system in 2020, never wired to anything, never read by code
that acts on it. Its TODO is about making an int an enum, not about address
width.** The only place an address space is genuinely threaded anywhere in the
tree is `taichi/codegen/llvm/codegen_llvm.cpp:1131-1134` (parameter declared with
default 0 at `codegen_llvm.h:165`, passed to `llvm::PointerType::get(...,
addr_space)` at `:1133`), which is unconnected to this field.

**Ruling on the claim that it should be struck from plan item 6.2: UPHELD.** Both
explore passes, both adversaries and this amendment reach it independently, and
no source or history contradicts it. It is address space, not address width, and
the TODO is the tell: widths are not an enum, spaces are.

One qualification, from adversary 1, recorded without softening the ruling: on
some targets an address space does imply a pointer width, AMDGPU's
`addrspace(3)` local pointers being 32-bit against `addrspace(1)` global pointers
at 64. Space and width are not fully orthogonal on every backend. That is an item
6.3 concern about the LLVM spine, not a reason to keep a dead field under 6.2.

---

## 7. Collateral finding: the 64-bit capability layers already present

VERIFIED. Relevant to the stub-architecture note in plan section 6.2, which says
64-bit integers in SPIR-V are a declared capability and the architecture must be
able to express a target lacking it. Upstream already expresses this, at three
independent layers:

1. **`spirv_has_int64`** — LIVE. Produced without any guard at
   `taichi/rhi/vulkan/vulkan_device_creator.cpp:630-633`, from
   `device_supported_features.shaderInt64`. Consumed at
   `spirv_ir_builder.cpp:64-66` (emits `OpCapability Int64`), `:166-169` (declares
   `t_int64_`/`t_uint64_`), `:311-313` and `:325-327` (hard error if an i64/u64
   type is requested without it). **64-bit scalars already work on the SPIR-V path.
   Only the 64-bit addressing model is un-defaulted.**

   **[CORRECTED].** The `:311-313` and `:325-327` guards are narrower than this
   report presented them. They live in `get_primitive_type(DataType)`, and the
   accessor `u64_type()` at `spirv_ir_builder.h:532-534` is a bare
   `return t_uint64_;` that does not go through it. `make_pointer`
   (`spirv_codegen.cpp:2317-2324`) calls the bare accessor. **So the guard does
   not cover the one path item 6.2 has to widen**, and on a device without
   `shaderInt64` the result is a default-constructed `SType` with no diagnostic.
   Adversary 2 found this; see section 4 reason 3 and escalation 16.3.
2. **`spirv_has_physical_storage_buffer`** — the 64-bit *pointer* layer, off. Note
   it was already conditioned on `shaderInt64` at `vulkan_device_creator.cpp:821`,
   introduced by `06cb1c50db16912973b0f58f938cba6790ad85bb`, Bob Cao, 2022-02-08,
   "[vulkan] Disable buffer device address if int64 is not supported (#4244)".
3. **`Extension::data64`** — the frontend architecture-level flag,
   `taichi/program/extension.cpp:9-28`. Granted to `x64`, `arm64`, `cuda` **only**.
   Not to `vulkan`, `opengl`, `gles`, `metal`, `dx11`, `amdgpu`. The OpenGL grant
   sits commented out at `:29-30`, disabled as collateral of a build refactor
   (`b3446e905a98ebd469e6a18c04d3b4ca957ec922`, Bo Qiao, 2022-04-30, #4887), having
   been added originally by `d9a803a3f` "[OpenGL] 64-bit data type support (#717)".
   **`data64` is advisory, not enforcement**: `is_extension_supported` has no C++
   caller that consults it. **[CORRECTED]** on the surrounding two claims, both
   from adversary 2, both re-verified here by enumeration:

   - The C++ callers of `is_extension_supported` are, in full: `assertion` at
     `taichi/program/program.cpp:149`; `bls` at
     `taichi/codegen/llvm/codegen_llvm.cpp:2726`; `mesh` at
     `taichi/transforms/compile_to_offloads.cpp:92`, `:205`, `:218` and `:236`;
     `quant` at `:245` and `:288`. Eight call sites, four extensions.
     **`sparse` is not among them** — this report previously listed it, and its
     six call sites are all Python (`python/taichi/lang/snode.py:50`, `:76`,
     `:93`; `python/taichi/_snode/fields_builder.py:83`, `:100`, `:122`).
   - "Its only uses in the tree are three Python test gates" is wrong by more
     than an order of magnitude. `grep -rn data64 tests/ python/` returns **53
     hits across 28 files**. The escalation this report builds on the point still
     stands — no C++ code consults `data64` while `spirv_has_int64` says the
     device can do it — but the supporting count was badly wrong.

---

## 8. Searches that came back empty

Recorded so the adversaries do not repeat them.

- No design note on 64-bit addressing, index width or physical storage buffers
  exists in the tree. `docs/design/` holds one file (`llvm_sparse_runtime.md`);
  `docs/rfcs/` holds the RFC process doc, one AOT RFC and a template.
- `grep TODO|FIXME|HACK|XXX` across `taichi/` filtered for 64/addr/pointer/index
  yields nothing bearing on this question beyond `type.h:207` (section 6) and
  `codegen_llvm_quant.cpp:75` (CUDA atomicCAS widths, unrelated).
- GITHUB issue search for "physical storage buffer" returns one issue, 4919, "GGUI
  issues on Intel graphics card". PSB appears there only inside a pasted
  `print_all_cap` trace log. **Nothing in that thread attributes the Intel failure
  to PSB, and I do not treat it as evidence.**
- **No upstream issue, PR or commit anywhere states that the physical storage
  buffer approach was judged wrong on merit.** I looked specifically for this,
  because the brief asked whether the approach itself is wrong. The evidence does
  not support that conclusion, and it does not support the opposite one either.
  Upstream simply stopped.

---

## 9. Verified versus inferred, itemised

**[AMENDED]** Items 1, 7, 8, 9 and 11 below are restated to match the
corrections above. Items 12 and the two INFERRED entries stand as written.

### VERIFIED
1. PSB shipped enabled and unguarded in **eleven** tagged releases, v0.9.0
   through v1.1.3; macOS-excepted in the three v1.2.x tags; un-defaulted in the
   ten tags v1.3.0 through v1.7.4. Full enumeration over all 121 tags in section
   2.1.
2. Three disabling commits: `f677fb141` (2022-10-24, macOS, cites issue 6295),
   `d20f55dc8` (2022-10-30, global, empty commit body), `12546578a` (2022-11-02,
   C-API path).
3. GITHUB: issue 6295's cause was bisected to MoltenVK PR 1638, not to Taichi.
4. GITHUB: PR 6468's body gives the global cause as "multi-platform compatibility
   issues" plus a dependency on devcap.
5. The devcap follow-ups all landed by Feb 2023; the guard was never lifted.
6. No branch, tag or ref carries abandoned continuation work on either mechanism.
7. The `use_64bit_pointers` **declaration** has exactly one commit in its history
   and was never true. Its **body's** true branch has two, the second being
   `715a04c98`, 2023-08-15.
8. Turning `use_64bit_pointers` on would not produce a working build, for the
   three reasons in section 4. Two of them are host-side or SPIR-V validity
   failures; the third is silent.
9. PSB gates the ndarray **base address** and argument, argpack and return struct
   layout; `make_pointer` gates SNode addressing. They are disjoint in live code.
   Section 3.2 gives the qualifier this needs; section 2.6 gives the dead code
   that contradicts the intent half.
10. `PointerType::addr_space_` has no setter and no reader outside the
    serialisation macro at `type.h:203`, since 2020.
11. `spirv_has_int64` is live and unguarded, and its guards do not cover
    `make_pointer`; `Extension::data64` is withheld from every non-LLVM arch and
    is advisory only.
12. GITHUB: issues 8608, 5320 and 8161 are all open, all about int64 indexing, and
    8608 has never received a maintainer reply.

### INFERRED
1. That "multi-platform compatibility issues" in PR 6468 means AOT modules failing
   the strict capability equality check at `c_api/src/taichi_gfx_impl.cpp:18-30`.
   Supported by the `[aot]` tag, the nine-day gap after PR 6184, the surrounding
   commit window and the equality-not-floor check. **Not stated anywhere by
   upstream.** **[WEAKENED]** by section 2.3: seven other device-derived
   capabilities bite the same check, so this explains why PSB is a hazard but not
   why PSB alone was singled out.
2. That the failure to restore PSB is momentum loss rather than a further
   unrecorded technical objection. Supported by the precondition being met, by the
   absence of any retracting statement, and by the upstream commit-rate collapse
   recorded in plan section 2.1. **Argument from silence; weigh it as such.**

### NOT ESTABLISHED
1. Whether the MoltenVK bug was in the driver or in Taichi's use of the feature.
   strongoier explicitly left this open and nobody returned to it.
2. Whether PSB would work correctly on the target hardware in plan section 5.2. No
   evidence either way was found; nothing in this investigation tested it.
3. Whether any non-macOS platform ever actually failed with PSB on. No bug report
   naming PSB as a cause was found on any platform other than macOS.

---

## 10. Escalations

Extended by section 12. Items 1 to 6 below stand as written; section 12 adds
the expiry questions.

Unresolved. Not resolved by assumption, per plan sections 0.3 and 10.2.

1. **The brief's premise needs a ruling.** It says upstream "wrote a substantial
   part of the 64-bit addressing machinery on the portable path and then switched
   it off." That is true of exactly one of the four items, and that item addresses
   ndarrays, not SNodes. Item 6.2 concerns addressing to accompany a raised SNode
   count. **If the planner's interest is SNode addressing, the shelved PSB work is
   not the relevant asset, and there is no shelved work to recover — only a stub
   and a warning.** The planner should say which reading governs before anyone
   sizes item 6.2 from this report.

2. **The AOT capability check is a live design constraint on plan section 5 and item
   6.3.** `c_api/src/taichi_gfx_impl.cpp:22-29` rejects a module on
   `current_version != required_version`, an equality, so a module is incompatible
   with a runtime that is *more* capable as well as one that is less. Plan section
   5 puts configuration at install time and section 6.3 wants adaptive module
   loading. Those two things meet exactly here. Whether that check is acceptable as
   it stands, or must change, is a decision I am not authorised to make.

3. **Reason 1 in section 4 may be recoverable, and I did not pursue it.** The
   `make_pointer(0)` at `spirv_codegen.cpp:355` fails because the root buffer's
   device address is never fetched, whereas the ndarray path at `:783` does fetch
   one out of the args buffer. Whether the same mechanism could supply a root
   address is a design question, not an archaeology question, and it is outside
   this assignment. Flagging it because it bears directly on item 6.2 and on
   whether any of this is reusable.

4. **The macOS question is still formally open upstream** and would reopen if this
   project ever targets Apple hardware. Not in plan section 5.2's tier table, so I
   have not pursued it, but the fault was never located.

5. **`Extension::data64` being advisory rather than enforced is a hazard I am not
   ruling on.** The frontend flag says Vulkan cannot do 64-bit data, while
   `spirv_has_int64` says the device can and the SPIR-V builder will emit it. Those
   two disagree, and no C++ code consults `data64` to reconcile them. Whether that
   is a latent bug or a deliberate soft gate needs a decision.

6. **I did not audit the LLVM path.** Territory 05 is the portable path. If the
   planner wants the 32-bit assumptions in the LLVM backend mapped, that is item
   8.1.1 and belongs to territory 02 or 03.

---

# Part II — Does each reason still hold today?

Added after the brief was extended. Same verified/inferred discipline. Sections 1
to 10 above are unchanged.

## 11. Expiry assessment

### 11.1 Summary table

| # | Reason established in Part I | Kind | Still holds? |
|---|---|---|---|
| 1 | MoltenVK's buffer-device-address support was broken (event A) | ENVIRONMENTAL | **YES**, still open upstream. But on a platform absent from plan section 5.2 |
| 2 | AOT capability equality check forces cross-machine incompatibility (event B) | **ARCHITECTURAL** | **YES** as code. Whether it binds this project is a planner decision, escalated |
| 3 | "Device capability not yet fully functional" (events B and C) | ENVIRONMENTAL, tooling | **EXPIRED** |
| 4 | Vulkan availability of `VK_KHR_buffer_device_address` | ENVIRONMENTAL | **NEVER APPLIED**. Core since Vulkan 1.2, and already handled by the guard |
| 5 | Hardware support on the target tiers | ENVIRONMENTAL | **NOT A BLOCKER**. Verified present on this box, baseline tier included |
| 6 | SPIR-V toolchain maturity | ENVIRONMENTAL | Fixed **upstream**, **not fixed in our checkout** |
| 7 | The SNode root "pointer" is a buffer offset, not a device address, colliding with `at_buffer`'s u64 sniff | **ARCHITECTURAL**, and larger than this report described | **YES**, and it does not expire. Both adversaries now agree. Section 13.5 |
| 8 | `bitmasked_activation` mixes u32 with the pointer width | DESIGN, local and fixable | **YES**, code unchanged. Citations corrected in section 4 |
| 9 | The frontend narrows indices to int32 because some backends lack int64 | **ARCHITECTURAL** | **YES**. The proposed fix was never built |
| 10 | `demote_dense_struct_fors.cpp:29` hard INT_MAX assert | **[QUALIFIED]** ARCHITECTURAL, but gated | **YES** on all-dense trees only. **Does not fire on this project's sparse workload.** Section 5.0 |
| 11 | **[NEW]** The `int` extractor fields at `taichi/ir/snode.h:41`, `:45`, `:49` truncate before any check | **ARCHITECTURAL** | **YES.** `:41` is the one that binds on the SPIR-V sparse path and it has no detector at all. Section 5.0 |
| 12 | **[NEW]** `make_pointer` reaches `u64_type()` past the `shaderInt64` guard | **ARCHITECTURAL** for plan section 2.2 | **YES.** Silent miscompile on a device lacking the feature. Section 4 reason 3 |

**[AMENDED] The architectural findings that matter, and that do not expire, are
7, 9, 11 and 12, with 10 surviving only for all-dense trees.** They are the
answer to the part of the brief asking whether the approach itself is wrong. None
of them says the physical storage buffer approach is wrong. All of them say the
SNode and index path was built 32-bit and was never converted.

Row 7 survives adversarial attack from both sides and comes out **larger** than
this report originally described: not a data-type sniff to be replaced, but a
backend value model with no room to carry the distinction. Section 13.5 has the
adjudication and the four legs I verified. The module-wide addressing model,
which an earlier version of this report treated as an obstacle alongside it, does
**not** survive and is refuted outright in section 13.4. So of the two obstacles
this report called architectural, one is confirmed and hardened and the other is
struck.

### 11.2 The Vulkan 1.2 promotion hypothesis is wrong, and wrong in our favour

VERIFIED. The extended brief asks whether the shelving predates or straddles the
promotion of `VK_KHR_buffer_device_address` into core Vulkan 1.2. It does neither.
Vulkan 1.2 shipped in January 2020 with the extension promoted. The three disabling
commits are October and November 2022, about two years and nine months later.

Further, the code already accepts both routes. At
`taichi/rhi/vulkan/vulkan_device_creator.cpp:812-819`, with the macros defined at
`:714-717` and `:720-721`, the guard is
`CHECK_VERSION(1, 2) || CHECK_EXTENSION(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)`,
then `CHECK_VERSION(1, 3) || buffer_device_address_feature.bufferDeviceAddress`,
then `device_supported_features.shaderInt64`.

**API availability was never the blocker and is not what changed.** This removes a
candidate explanation rather than supplying one.

### 11.3 The target hardware supports it, measured on this box

VERIFIED ON THIS MACHINE, read-only, via `nvidia-smi` and `vulkaninfo`.

Both GPUs run driver 535.309.01 and report Vulkan 1.3.242. Instance version 1.3.275.

| Property | GTX 1070 (Pascal, mid tier) | GTX 750 Ti (Maxwell, baseline tier) | llvmpipe (CPU) |
|---|---|---|---|
| `VK_KHR_buffer_device_address` | present | present | present |
| `VK_EXT_buffer_device_address` | present | present | absent |
| `shaderInt64` | true | true | true |
| `bufferDeviceAddress` | true | true | true |

Every clause of the guard is satisfied on all three devices. **If the `&& false` at
`vulkan_device_creator.cpp:825` were removed, the capability would be set on every
device on this box, including the Maxwell baseline card and the CPU software
renderer.**

Relevant to plan section 2's requirement that CPU-only operation be first class:
`llvmpipe` reports both `shaderInt64` and `bufferDeviceAddress` true. I have
verified the *report*, not the behaviour. Nothing here was executed through Taichi.

### 11.4 The dependency picture splits cleanly in two

VERIFIED. This is the "fixed upstream versus fixed in what we have" distinction the
extended brief asks for, and the answer differs by submodule.

| Submodule | Pinned revision | Date of that revision |
|---|---|---|
| `external/Vulkan-Headers` | `409c16be502e39fe70dd6fe2d9ad4842ef2c9a53` | **2025-04-18**, Vulkan-Docs 1.4.313 |
| `external/volk` | `b87f88292b09bc899b24028984186581a1d24c4e` | 2023-03-24, Vulkan 1.3.245 |
| `external/SPIRV-Headers` | `34d04647d384e0aed037e7a2662a655fc39841bb` | **2022-12-15** |
| `external/SPIRV-Cross` | `c77b09b57c27837dc2d41aa371ed3d236ce9ce47` | **2022-11-21** |
| `external/SPIRV-Tools` | `46ca66e6991f16c89e17ebc9b86995143be2c706` | **2022-11-18** |

`external/Vulkan-Headers/include/vulkan/vulkan_core.h:72,75` gives
`VK_HEADER_VERSION 313` and a complete version of 1.4.313.

The four SPIR-V submodules last moved in
`ee0af32abe2ca6e8ad2de4eea39d2e3ed7bd5c43`, 2023-04-03 (#7676). Comparing
`git ls-tree ee0af32ab external/` against `git ls-tree HEAD external/`, their
gitlinks are identical; only Vulkan-Headers and VulkanMemoryAllocator moved after
that, in `4c2bac69ea237236f418637d8f91b14bfd0c78d3`, 2025-04-25 (#8680).

**The Vulkan API headers we actually have are current. The SPIR-V toolchain we
actually have is from the same weeks as the shelving and has not moved in over
three years.** Any SPIR-V toolchain fix from 2023 onward exists upstream and is
absent from our checkout.

Nothing is missing at the header level: the pinned SPIRV-Headers define
`AddressingModelPhysicalStorageBuffer64` at
`external/SPIRV-Headers/include/spirv/unified1/spirv.hpp:106`,
`StorageClassPhysicalStorageBuffer` at `:229` and
`CapabilityPhysicalStorageBufferAddresses` at `:1062`.

### 11.5 One toolchain fix we lack touches a pass Taichi actually runs

VERIFIED, with a stated limit on what it means.

GITHUB. Searching SPIRV-Tools for merged pull requests mentioning
PhysicalStorageBuffer after our pin date returns eleven: 5291, 5316, 5476, 5539,
5575, 5635, 5977, 6000, 6266, 6275, 6714. Most are **validator** work, which cannot
affect us: Taichi disables the validator at
`taichi/codegen/spirv/spirv_codegen.cpp:2710`,
`spirv_opt_options_.set_run_validator(false);`.

The exception is **SPIRV-Tools PR 5316, "[spirv-opt] Handle OpFunction in GetPtr",
merged 2023-07-17**, fixing SPIRV-Tools issue 5270. Body:

> When using PhysicalStorageBuffer it is possible for a function to return a
> pointer type. This was not being handled correctly in
> `GetLoadedVariablesFromFunctionCall` in the DCE pass.

It changes `source/opt/aggressive_dead_code_elim_pass.cpp` and
`source/opt/mem_pass.cpp`. Taichi registers `spvtools::CreateAggressiveDCEPass()`
at `spirv_codegen.cpp:2690`. Our pin is eight months earlier, so **we do not have
this fix**.

NOT ESTABLISHED: whether Taichi's generated SPIR-V can trigger it. The bug needs a
function returning a pointer, and Taichi runs `CreateInlineExhaustivePass()` at
`:2688` before DCE, which would inline most functions away. **I am recording a gap
in what we have, not asserting a live fault.**

Method note: the SPIRV-Tools submodule checkout is shallow, so its history could
not be searched locally; this came from GitHub.

### 11.6 The macOS blocker is environmental and has NOT expired

GITHUB. MoltenVK issue **2174, "regression bug with vkGetBufferDeviceAddress",
open since 2024-03-07**: *"MoltenVK latest: Support for the buffer device address
extension for my GPU has been removed entirely. v1.2.0 through v1.2.6:
vkBufferDeviceAddress returns 0."* MoltenVK issue **2696**, open, filed 2026-03-06,
reports device-address buffers failing with `VK_ERROR_OUT_OF_DEVICE_MEMORY`.

"Returns 0" is the same class of symptom as Taichi issue 6295's silent zeroes, four
years on.

So this reason is environmental and is still live, on a platform that does not
appear in plan section 5.2. The `#if !defined(__APPLE__)` half of the guard is the
part that still has a reason behind it. The `&& false` half does not.

### 11.7 The AOT portability reason: architectural, and possibly not ours

The one shelving reason that is architectural rather than environmental is the
capability equality check at `c_api/src/taichi_gfx_impl.cpp:22-29`, which rejects an
AOT module unless `current_version == required_version`. That code is unchanged.

INFERRED, and flagged rather than concluded: plan section 5 puts configuration at
**install time**, with the loader examining the system and selecting or compiling
the right binary. That is a different deployment model from AOT modules shipped
prebuilt to unknown machines, which is the scenario the check exists to protect.
If this project never ships a module across a capability boundary, the reason that
caused event B may not apply to it at all.

I am not deciding that. See escalation 12.1.

## 12. Escalations, extended

Section 10 above stands. These are additional and unresolved.

1. **Whether reason 2 binds this project is the single highest-value open
   question, and it is the planner's, not mine.** The global kill was, on the best
   available reading, an AOT cross-machine portability measure. Plan section 5's
   install-time configuration model may be immune to it. If it is, the only
   surviving reason for the `&& false` is the macOS one, which the
   `#if !defined(__APPLE__)` clause already handles on its own. **I am not
   proposing any change. I am reporting that the stated reasons for the global
   guard appear, on this evidence, not to apply to this project's targets, and that
   this needs a decision.**

2. **The SPIR-V submodule pins are three years stale and this is a standing
   exposure, not a one-off.** Vulkan-Headers was bumped to 2025 while SPIRV-Tools,
   SPIRV-Cross, SPIRV-Headers and SPIRV-Reflect were left at late 2022. I found one
   concrete PhysicalStorageBuffer fix in a pass Taichi runs that we do not have
   (11.5), by searching only for that one keyword. I did not audit the other three
   years of SPIR-V toolchain changes, and the gap is certainly wider than the one
   item I found. Whether to bump those pins is a decision with its own blast radius
   and is outside this assignment.

3. **I cannot say whether the physical storage buffer actually works on this
   hardware.** 11.3 verifies that the devices *advertise* every capability the
   guard tests. It does not verify that Taichi's PSB code path produces correct
   results on them. Establishing that requires building with the guard lifted and
   running, which is outside an investigation-only assignment. **Do not read 11.3 as
   "it works." Read it as "nothing in the hardware refuses it."**

4. **Reason 6's real size is unknown.** I checked SPIRV-Tools against one keyword.
   SPIRV-Cross is pinned at 2022-11-21 and I did not assess it at all; Taichi uses
   it on paths I did not examine. If the planner wants the toolchain gap sized
   properly that is a separate assignment.

5. **The llvmpipe result deserves separate attention and I have not pursued it.**
   The CPU software renderer reports `shaderInt64` and `bufferDeviceAddress` true.
   Plan section 2 makes CPU-only a first-class target. Whether the Vulkan-on-CPU
   path is a route this project wants at all, as against the LLVM CPU backend, is
   not something this investigation touched or should decide.

---

# Part III — Amendment pass, 2026-09-09

Sections 1 to 12 above are amended in place, marked **[CORRECTED]**,
**[REBUILT]**, **[NEW]**, **[QUALIFIED]** or **[AMENDED]** at
the point of each change. This part records the adjudication of every claim put
to the amendment pass, including the two places where the two adversaries
contradict each other and the places where an adversary is wrong.

Method: every claim was re-derived from source and `git` at
`ba0e81dce559fb63a5958bf82feb1d00c55c02fe`. No adversary citation was accepted on
its face; each line number below was re-read. Nothing in the tree was modified,
no git state was changed, and nothing was written to GitHub. Where an adversary
is wrong I say so and change nothing.

## 13. Adjudication, claim by claim

### 13.1 The feature is un-defaulted, not off. UPHELD, and it governs the report.

**VERIFIED in full.** The chain is traced hop by hop in section 1.1. The
load-bearing points: `Program::make_aot_module_builder`
(`taichi/program/program.cpp:541-556`) takes a caller-supplied capability string
list; `str2devcap` (`taichi/rhi/device_capability.cpp:6-13`) accepts
`spirv_has_physical_storage_buffer` because it expands
`taichi/inc/rhi_constants.inc.h:28`; `GfxProgramImpl::make_aot_module_builder`
(`taichi/runtime/program_impls/gfx/gfx_program.cpp:30-40`) passes the config
through without examining it; and it arrives at `TaskCodegen::caps_`
(`spirv_codegen.cpp:86`) via `kernel_compiler.cpp:37`. I looked for a device
check at each hop and found none. The contrast that proves the point:
`GfxProgramImpl::get_device_caps` at `gfx_program.cpp:82-85` reads the device,
and that is the path the `&& false` closes. The ahead-of-time builder does not
use it.

The C API half is equally verified: `ti_set_runtime_capabilities_ext`
(`c_api/src/taichi_core_impl.cpp:317-334`) calls `set_caps`
(`taichi/rhi/public_device.h:855`) at `:331`, which replaces the set wholesale
rather than merging or validating.

**Consequence, and I state it as the claim did.** This report, its pair and
adversary 1 all describe a disabled feature. That framing is wrong, and adversary
2 is right that it is the one place all four documents in this territory agreed on
something false. Adversary 1 files "no live producer on any ref after 2022-11-02"
under what it could not break (its section 11 item 5) — the **ref sweep** is
correct and I reproduced it, but it answers the narrower question of whether any
committed source line sets the capability. It does not answer whether a producer
exists. Adversary 1's own section 8 observes that "a caller can declare the
requirement today" and stops one step short.

Joined to the zero test coverage verified in section 1.1, the consequence is a
user-reachable path into branches no test exercises. That is a present exposure,
not a future decision.

### 13.2 Both consumer inventories omit three consumers. UPHELD. Inventory rebuilt.

**VERIFIED.** Section 2.5 is rebuilt from scratch, not patched. The three
omissions are real, and the two adversaries name the same three independently,
which is the shared-blindness signal the working method is looking for:
`spirv_types.cpp:484-514` (`translate_ti_type`, where the width decision is
actually made), `runtime/gfx/runtime.cpp:823` and `:851-859`
(`get_struct_type_with_data_layout_impl`, the host half of the same ABI), and
`spirv_ir_builder.cpp:334-352` (`from_taichi_type`, a second and **dead**
implementation of the same rule — `grep` returns its definition, its own
recursive call at `:347`, and the declaration at `spirv_ir_builder.h:343`, and no
caller).

The rebuild also found one entry that should come **out**:
`spirv_types.cpp:433-439` was listed as a live consumer and is unreachable, for
the reason given in section 2.6. Adversary 2 is right and adversary 1 mentions
the citation in passing without flagging it. And the earlier list said "twelve"
while enumerating bullets that do not correspond to the twelve direct readers —
a heading contradicting its own rows, which plan section 10.7 names as this
project's recurring failure. The rebuilt table is numbered so the count can be
reconciled against the rows.

### 13.3 Dead SNode code contradicting the intent half. UPHELD.

**VERIFIED.** Section 2.6. `snode_struct_compiler.cpp:52-54` wraps a sparse
`pointer` SNode's cell in a `PhysicalPointerType`, the type that translates to
`spv::StorageClassPhysicalStorageBuffer`. Its caller is commented out at `:16-19`,
the result fields at `snode_struct_compiler.h:48-50`, and
`git log -S "PhysicalPointerType" --all -- taichi/` returns one commit,
`46b5632e5e61036a788013780d0d9b9eacfd7f4f` (Bob Cao, 2022-06-22, #5213), whose
diff adds the call site **already commented out**.

**How I weigh it.** It is not an asset: forty lines that never compiled into
anything, against a `CompiledSNodeStructs` shape that has moved on since. It is
not surplus either, and per plan section 10.3 I am not calling it that. Its value
is evidential and it cuts two ways.

- Against this report's section 3: "the physical storage buffer never addressed
  SNodes" is true of shipped behaviour and false of intent. Four months after
  landing the capability its author sketched exactly the opposite. Section 3 has
  been qualified accordingly.
- For adversary 1's section 2.4, which is now this report's section 3.2:
  adversary 1 reasons from the SPIR-V and Vulkan addressing models to the
  conclusion that a large SNode root on Vulkan needs physical addressing, and
  marks it inferred because Taichi has no source line on it. **Taichi has a
  source line on it.** That does not make the inference verified — the code never
  ran — but the person who wrote the mechanism arrived at the same design
  independently, which is worth more than a lone spec deduction.

Adversary 1 and both explore passes, this one included, missed it. Adversary 2
found it alone.

### 13.4 The module-wide addressing model is not an architectural obstacle. UPHELD.

**The factual claim is VERIFIED and the obstacle claim is REFUTED**, and the two
adversaries agree here, having reached it by the same route independently.

The fact: `spirv_ir_builder.cpp:113-127` emits one `OpMemoryModel` per module,
`AddressingModelPhysicalStorageBuffer64` under the capability at `:119-122` and
`AddressingModelLogical` otherwise at `:124-126`. One declaration, no per-pointer
opt-in.

The refutation, VERIFIED from the shipped releases rather than predicted. At
**v1.1.3**, dated 2022-09-20, a release in which the capability was ON and
unguarded per section 2.1:

```
$ git show v1.1.3:taichi/codegen/spirv/spirv_codegen.cpp | grep -n 'use_64bit_pointers\|make_pointer(0)'
74:  const bool use_64bit_pointers = false;
275:    spirv::Value root_val = make_pointer(0);
$ git show v1.1.3:taichi/codegen/spirv/spirv_ir_builder.cpp | grep -n AddressingModel
114:        .add_seq(spv::AddressingModelPhysicalStorageBuffer64,
119:        .add_seq(spv::AddressingModelLogical, spv::MemoryModelGLSL450)
```

So v1.1.3 declares the physical addressing model while `make_pointer` at its
`:2062-2063` still returns a 32-bit offset and SNode roots are still bound-buffer
relative. **Eleven tagged releases shipped mixed addressing under that model**,
per the enumeration in section 2.1, and what eventually broke was a MoltenVK bug
in ndarray reads, not the memory model. Declaring the model makes physical
pointers legal; it does not make any pointer physical.

A second, independent refutation: "every shader in the module" is one shader.
`TaskCodegen` constructs its own `IRBuilder` per instance at
`spirv_codegen.cpp:97` and `run()` finalizes it at `:151`, so there is one module
per offloaded task.

**The addressing model is a declaration, not a constraint.** What remains in this
area is real but environmental: declaring a SPIR-V capability the device did not
enable is a specification violation, so the capability must track actual device
support — which is exactly what the guard at
`vulkan_device_creator.cpp:813-828` already does, testing Vulkan 1.2 or the
extension, then `bufferDeviceAddress`, then `shaderInt64`, before the
`&& false`.

### 13.5 ADVERSARY DIVERGENCE — the `at_buffer` u64 collision. THE DIVERGENCE HAS CLOSED. ADJUDICATED FOR ADVERSARY 1.

**Read against `adversary-05-1.md` at 1822 lines and `adversary-05-2.md` at 1740
lines.** Both files grew while this amendment was in progress, and the growth
matters: the two adversaries exchanged again and **the divergence put to me no
longer exists**. Adversary 2 withdraws its classification in full at its section
14.1; adversary 1 withdraws its justifying sentence at its section 24.6 but holds
its ruling. I re-derived the deciding facts myself rather than accepting the
convergence, because a convergence between two agents is not evidence.

**Ruling: the collision is architectural. Adversary 1 is right, adversary 2's
withdrawal is correct, and an earlier draft of this section ruled the other way
on a ground that does not hold.**

**What is true, and what an earlier draft of this section made too much of.**
VERIFIED, and these were the facts I ruled on:

- `enum class ValueKind` at `taichi/codegen/spirv/spirv_ir_builder.h:69-79`, nine
  members, `kPhysicalPtr` at `:75`. `Value::flag` at `:88`, default `kNormal`.
- It is genuinely read, not decorative. `IRBuilder::load_variable`
  (`spirv_ir_builder.cpp:1270-1285`) asserts the flag is one of three pointer
  kinds at `:1271-1273` and branches on `kPhysicalPtr` at `:1275` to emit `OpLoad`
  with `spv::MemoryAccessAlignedMask` at `:1277-1280`. `store_variable`
  (`:1286-1298`) does the same at `:1290-1294`.
- It survives the name table: `register_value` stores the whole `Value` at
  `:1308`, `query_value` returns it by value at `:1314`.

So adversary 1's original sentence — "the code has no third thing to disambiguate
them" — is false as written, and adversary 1 withdraws it. That much stands.

**What decides it, and I missed the consequence of a fact I had already found.**
The tag is set at `spirv_codegen.cpp:2203` on `at_buffer`'s **output**, after the
branch is chosen, so that `load_variable` and `store_variable` downstream know to
emit an aligned physical access. It is not on the input. **[CORRECTION TO BOTH
ADVERSARIES, which both still carry]: that assignment is at `:2203`; `:2204` is
the `return`.**

The question is therefore not whether a tag exists but whether one is available
at `:2197`. It is not, by any route, and I verified all four legs:

1. `make_pointer` (`:2317-2324`) → `uint_immediate_number` → `get_const`, which
   constructs `new_value(dtype, ValueKind::kConstant)` at
   `spirv_ir_builder.cpp:1502`. Tag `kConstant`.
2. `visit(GetChStmt *)` at `:372` does `ir_->add(input_ptr_val, offset)`. `add` is
   `DEFINE_BUILDER_BINARY_USIGN_OP` (`spirv_ir_builder.cpp:1038-1047`) →
   `make_value` (`spirv_ir_builder.h:289-298`), which constructs `kNormal` at
   `:292` and upgrades to `kVariablePtr` only when `out_type.flag ==
   TypeKind::kPtr` at `:294-295`. An integer byte offset is `kPrimitive`. **Tag
   erased at the first arithmetic op.**
3. **And decisively, the same is true of the live ndarray path.** With the
   capability on, `visit(ExternalPtrStmt *)` loads the genuine device address at
   `:789` via `load_variable`, which itself returns `new_value(res_type,
   ValueKind::kNormal)` at `spirv_ir_builder.cpp:1274`, then adds the converted
   offset at `:790-791` through `make_value` — `kNormal` again. So **the real
   physical device address arrives at `at_buffer` tagged exactly as a root offset
   would be.** `at_buffer` cannot be reading the tag, because the tag is `kNormal`
   on both sides of the decision. The dtype sniff is not a shortcut past an
   available mechanism; it is there because no mechanism is available.
4. Adversary 2 tested the one alternative adversary 1 did not, and I re-checked
   it: `ptr_to_buffers_` cannot serve either. `visit(ExternalPtrStmt *)` sets it
   at `:797-801`, **outside** the capability branch at `:783-795`, so under the
   capability an ndarray pointer carries both a `u64` physical address and a
   `ptr_to_buffers_` entry, exactly as an SNode pointer would.

**Where I went wrong, stated plainly.** I found the `make_value` erasure
independently, before reading either adversary's second exchange, and recorded it
as a correction to adversary 2's *sizing* of the remedy. It is not a sizing
correction. It is the refutation of the ruling, and I did not see that because I
traced the erasure on the SNode path and did not trace it on the ndarray path.
Tracing leg 3 is what closes it.

**Consequence for the remedy, and it is the opposite of "local".** Tagging at
production requires changing `make_value` at `spirv_ir_builder.h:291` — the
single constructor for every SSA value this backend emits. `grep -rn "make_value("
taichi/codegen/spirv/` returns 148 occurrences, 147 of them call sites — plus a
propagation rule for mixed operands across `add`, `mul`, `sub`, the shifts and
the bitwise ops, all of which route through it. That is the backend's value
model. Adversary 2's own summary of the exchange is the right one: **the obstacle
is larger than either adversary originally described.**

**Classification, and the open question underneath it.** Against the operative
test — architectural obstacles do not expire, environmental ones do — the
collision does not expire, and neither does the `make_value` propagation problem
behind it. No driver, toolchain bump or hardware change touches either. It is
architectural on that test, and this report's Part II row 7 is restored to the
architectural column.

Adversary 1's Escalation 13 is nonetheless well taken and I pass it on rather
than resolving it: the two adversaries graded findings against two different
readings of "architectural" — "no external progress fixes it" against "no
decision fixes it either" — and the second reading makes almost nothing
architectural, since any code can be rewritten. The brief's own wording supports
the first. That is the arbiter's word to fix, not mine, and several gradings in
all four documents in this territory are not comparable until it is.

**On the facts nobody disagrees:** flipping the flag does not produce a working
build. Both explore passes, both adversaries and this amendment agree, and it is
now verified five times over. Section 4 gives the three reasons.

One formulation of adversary 2's should carry forward, and adversary 1 says so
too. The two mechanisms are better described as **one route with a type-sniff
switch** than as two disjoint routes: `at_buffer` (`:2194-2220`) is the single
entry point for every buffer access, ndarray and SNode alike, and `:2197` reads
the width of the pointer value as the addressing scheme. That explains in one
sentence why widening the SNode offset is blocked — the width is already spoken
for as a tag — and it is sharper than this report's section 3, which is true but
only shows that the two paths never share a buffer.


### 13.6 The hard stop fires only on all-dense trees, and the sparse binding width is elsewhere. UPHELD, with a refinement.

**VERIFIED in both halves.** Section 5.0 gives the full working. Both adversaries
reached it independently and neither explore pass had it.

Half one: `demote_dense_struct_fors.cpp:29` sits in `convert_to_range_for`
(`:13-112`), whose only caller `maybe_convert` (`:114-119`) requires
`stmt->snode->is_path_all_dense`. That flag is cleared at `snode.cpp:20` by any
node needing activation. Plan section 4.1 requires sparsity, so on this project's
workload the assertion never executes. This report previously presented it as a
second edit to a warning and did not name the gate.

Half two: the three `int` fields at `taichi/ir/snode.h:41`, `:45` and `:49`, in
`struct AxisExtractor` (`:37-54`), against `num_cells_per_container` already
`int64` at `:97`. The truncating store at `snode.cpp:92` runs on every iteration
of the loop at `:90-94`, before the check at `:95` runs once.

**REFINEMENT, and it improves the claim rather than contradicting it.** The three
fields are not equally load-bearing, and the one the claim leads with is not the
one that binds here. Derived by enumerating every reader:

- `acc_shape` (`:49`) is read at `taichi/codegen/llvm/struct_llvm.cpp:174` and
  `:176`, and at `demote_dense_struct_fors.cpp:74` and `:76`. **LLVM spine and
  the dense pass. Not the SPIR-V sparse path.**
- `num_elements_from_root` (`:41`) is read at
  `taichi/codegen/spirv/snode_struct_compiler.cpp:121`, inside the loop at
  `:115-122` that runs for every SNode type including sparse ones. **This is the
  one that binds on the SPIR-V sparse path**, and unlike `acc_shape` it has **no
  overflow detector anywhere**: it is accumulated at `snode.cpp:22-23` and `:83`
  with no warning and no assert.
- `shape` (`:45`) feeds `acc_shape` at `snode.cpp:93` and the dense pass at
  `demote_dense_struct_fors.cpp:24` and `:74`.

So the claim is right that all three are `int` and right that nobody named them.
It is imprecise in putting `:49` on this project's path. For plan item 6.2 on a
sparse tree on the SPIR-V spine, `taichi/ir/snode.h:41` is the field to widen,
and it is the one that fails silently.

This overlaps territory 01. I defer to that pair on completeness of the frontend
map and record it here because it fell between two territories and because plan
section 4.1 makes it the constraint that actually applies.

### 13.7 `PointerType::addr_space_` should be struck from item 6.2. UPHELD.

**VERIFIED**, section 6. Address space, not address width. Four independent
agreements now stand: both explore passes, both adversaries, and this amendment.
No source or history contradicts it. Two corrections to this report's own
citations, both from adversary 2, are folded into section 6: the constructors are
at `type.h:179` and `:181-185`, and "zero call sites, ever" overstated it because
the `TI_IO_DEF` macro at `:203` serialises the field.

### 13.8 ADVERSARY DIVERGENCE — the tag census. ADJUDICATED FOR ADVERSARY 2 BY ENUMERATION.

The divergence: adversary 1 reports reading all tags and gives 8 enabled, 3
macOS-excepted, 10 dead, out of 21 tags containing the capability, from a total
it states as 122. Adversary 2 gives 11 enabled from v0.9.0, 3 macOS-excepted, 10
off.

**Settled by enumeration over every tag. Adversary 2 is right. Adversary 1 is
three tags short, and its total is off by one.** The full classification is the
table in section 2.1; the counts reconcile as 92 + 5 + 11 + 3 + 10 = **121**,
which is the tag count measured here. `git tag | wc -l` returns 121, not 122.
Adversary 1 correctly handles the file's move from `taichi/backends/vulkan/` to
`taichi/rhi/vulkan/`, which is why it caught v1.0.0–v1.0.3, and then stops one
minor version early. **v0.9.0, v0.9.1 and v0.9.2 also ship it enabled and
unguarded**, and v0.9.0, dated 2022-02-22, is a fortnight after `7705f688a` and
is the first release to carry the feature at all.

Adversary 1's charge against this report stands and is upheld: the earlier
four-tag table was a spot check presented as a census, and the headline "live in
four public releases" was the size of the sample rather than of the range. That
is the same defect plan section 10.7 names. Both are corrected. Adversary 1's
disabled row of ten is correct and agrees with adversary 2's and with mine
exactly; the disagreement is confined to the v0.9.x window.

This strengthens section 13.4: eleven tagged releases, not eight, shipped
`AddressingModelPhysicalStorageBuffer64` alongside 32-bit bound-buffer SNode
addressing.

### 13.9 "Disabling made every module portable" is false. UPHELD.

**VERIFIED.** Section 2.3 carries the correction and the enumeration. The check
at `c_api/src/taichi_gfx_impl.cpp:22-29` loops over every required capability and
rejects on inequality at `:25`. Seven further capabilities are set from the
device with no guard, at `vulkan_device_creator.cpp:628`, `:632`, `:636`, `:659`,
`:663`, `:667` and `:671`. `spirv_has_int64` at `:632` is among them, which
matters directly: it is derived from the same `shaderInt64` feature that gates
the physical storage buffer, and it is still device-derived today.

The correction weakens this report's own inference about why PSB was singled out,
and I have marked it as weakened rather than withdrawn in section 9. Adversary 1
found it; adversary 2 verified it and says it did not spot it.

### 13.10 The return-layout host/device mismatch. UPHELD as code; reachability NOT ESTABLISHED, and my hedge is stronger than adversary 1's.

**VERIFIED line for line**, section 2.7. `gfx_program.h:75-77` returns the literal
`"4-"` with no capability read; `spirv_codegen.cpp:2490-2494` reads the
capability and threads it into `translate_ti_type`. With the capability on the
shader lays out a return-struct pointer member at 64 bits and the host, through
`runtime.cpp:823` and the `:857-858` branch, at 32. The argument path is
consistent; the return path is not. `88ac098bc` (#8061, 2023-05-23) added the
codegen half and, verified against its file list, **does not touch
`gfx_program.h`**.

Adversary 1 hedges on whether a kernel return struct can contain a `PointerType`,
calling it a frontend question out of scope under plan section 1.2. That hedge is
right and there is a second, stronger one available from C++ alone, which I add:
`Callable::finalize_params` at `taichi/program/callable.cpp:170-177`
**synthesises** pointer members deliberately, wrapping array parameters through
`get_pointer_type` at `:174`. `finalize_rets` at `:150-156` does no wrapping — a
return member is `rets[i].dt` verbatim at `:155`. So the return struct holds a
`PointerType` only if a declared return dtype is itself one, and nothing in C++
constructs that. **Recorded as a latent defect in a user-reachable but untested
branch, not as a live fault.**

### 13.11 On SPIR-V there is no other mechanism. UPHELD as a qualifier, with the inferential step stated.

**VERIFIED on the Taichi side, INFERRED on the Vulkan side, and I keep the two
apart.** Section 3.2 carries it.

Verified: the SNode root is bound through `ir_->buffer_argument` at
`spirv_codegen.cpp:2308-2309`; `at_buffer`'s non-u64 path at `:2212-2218` shifts
a `u32` offset and hands it to `struct_array_access`; and **this tree never
queries `maxStorageBufferRange`** — `grep -rn maxStorageBufferRange taichi/
c_api/` returns zero hits, re-run here.

Inferred, from the SPIR-V and Vulkan addressing models rather than any Taichi
line: a bound buffer's reach is capped by that limit, and SPIR-V offers a bound
buffer or a physical device address and nothing else. Widening an offset inside a
bound buffer widens something that was not the constraint.

**This qualifies the convergent finding rather than refuting it, and section 3 now
says so.** The planner's reversal to the project owner should carry the
qualifier: the shelved work is not reusable as written **and** is structurally
prerequisite for the version of item 6.2 that outgrows a bound buffer. A bare
"irrelevant" would mislead. Adversary 2's find at `snode_struct_compiler.cpp:53`
(section 13.3) is the Taichi-side evidence adversary 1 says does not exist.

## 14. Corrections made to this report, itemised

Derived by enumerating the edits, so the count reconciles: **fourteen**.

| # | Section | Correction | Source |
|---|---|---|---|
| 1 | 1, 1.1 | "zero live producers" and the disabled framing throughout, replaced with the un-defaulted framing and the two producer chains | adversary 2 |
| 2 | 2.1 | Four-tag spot check replaced with a 121-tag enumeration; 11 / 3 / 10, opening at v0.9.0 | adversary 2, against adversary 1's 8 |
| 3 | 2.2 | c_api producer span `:44-52` corrected to `:45-51`, and named as a comment block rather than a guard | adversary 1 |
| 4 | 2.3 | "makes every module portable at a stroke" struck; seven other device-derived capabilities enumerated | adversary 1 |
| 5 | 2.5 | Consumer inventory rebuilt: twelve direct readers numbered, three indirect consumers added, `spirv_types.cpp:433-439` removed as unreachable | both adversaries |
| 6 | 2.6 | New: the dead SNode physical-pointer code | adversary 2 |
| 7 | 2.7 | New: the return-layout host/device mismatch | adversary 1 |
| 8 | 3.1 | "would widen ndarray addressing" narrowed to the base address only; index arithmetic stays `i32` | both adversaries |
| 9 | 3.2 | New: the bound-buffer qualifier and `maxStorageBufferRange` | adversary 1, evidence upgraded by adversary 2 |
| 10 | 4 | "born false, never edited" qualified; `715a04c98` cited | both adversaries |
| 11 | 4 | Reason 2 rewritten: `:404`, `:409` and `:413` withdrawn, `:398-399` and `:410-412` substituted, failure mode corrected from invalid SPIR-V to a host assertion | both adversaries |
| 12 | 4 | New reason 3: `make_pointer` bypasses the `shaderInt64` guard silently | adversary 2 |
| 13 | 5.0 | New: the `is_path_all_dense` gate and the three `int` extractor fields, refined to name `:41` as the sparse-path binder | both adversaries, refined here |
| 14 | 6, 7 | `PointerType` constructor lines; "zero call sites" qualified; `data64` test-gate count corrected from three to 53 across 28 files; `sparse` removed from the C++ callers of `is_extension_supported` | adversary 2 |

## 15. Where an adversary is wrong, and nothing was changed

Recorded so the planner can see what was tested and rejected.

1. **Adversary 1's original justifying sentence, section 4: "the code has no
   third thing to disambiguate them."** False as written, and adversary 1 has
   withdrawn it. `ValueKind` exists at `spirv_ir_builder.h:69-79`, `Value::flag`
   at `:88`, and it is read at `spirv_ir_builder.cpp:1275` and `:1290`. **But the
   ruling the sentence was offered for survives**, because the tag is downstream
   of the decision and both sides of the decision arrive `kNormal`. An earlier
   draft of this amendment ruled against adversary 1 on the strength of the
   withdrawn sentence and was wrong to. Adjudicated in section 13.5.
2. **Both adversaries cite `spirv_codegen.cpp:2204` for the `kPhysicalPtr`
   assignment.** It is at `:2203`; `:2204` is the return.
3. **Adversary 1, section 7: 122 tags, 21 containing the capability, 8 enabled
   from v1.0.0.** Measured here: 121 tags, 24 containing it, 11 enabled from
   v0.9.0. Adjudicated in section 13.8.
4. **Adversary 2, section 2b: the fix is "a local change to `at_buffer`,
   `load_buffer`, `store_buffer` and the four `make_pointer` call sites."**
   Understated, and adversary 2 has since withdrawn both the estimate and the
   ruling that rested on it. A production-side tag is erased by `make_value`
   (`spirv_ir_builder.h:289-298`) on every arithmetic instruction, including on
   the live ndarray path at `spirv_codegen.cpp:790-791`, so the fix reaches the
   backend's value model. Section 13.5.
5. **The claim as put to me that the sparse binding width is "three `int` fields
   at `snode.h:41`, `:45` and `:49`".** All three are `int` and all three were
   unnamed, but `:49` is read only by the LLVM spine and the dense pass. On the
   SPIR-V sparse path the binder is `:41`. Section 13.6.
6. **Adversary 1's hedge on the return-layout mismatch is right but incomplete.**
   A second, C++-only reason to doubt reachability exists at
   `callable.cpp:150-156` against `:170-177`. Section 13.10.

## 16. Escalations, third pass

Sections 10 and 12 stand. These are additional, and unresolved. Not settled by
assumption, per plan sections 0.3 and 10.2.

1. **The un-defaulted framing changes what escalation 12.1 is asking.** Escalation
   12.1 asked whether the AOT portability reason binds this project, treating
   restoration as a future decision. Section 1.1 shows the capability is
   user-settable today with no device validation, into branches with zero test
   coverage. The question is therefore two questions, and both are the planner's:
   whether that door should be closed, and whether it should instead be the
   supported route. Plan section 5 puts configuration at install time and section
   6.3 wants adaptive module loading; an unvalidated capability override is either
   exactly the mechanism those want or exactly the hazard they must not inherit. I
   am not deciding it.

2. **`snode_struct_compiler.cpp:34-63` needs a disposition before item 6.2 is
   sized.** Forty lines of dead SNode type construction that would give sparse
   `pointer` nodes physical device addresses. Per plan section 10.3 I am calling
   it neither surplus nor an asset. It is the only line in the tree connecting the
   two mechanisms, and section 3.2's qualifier partly rests on it.

3. **The `u64_type()` gap is a plan section 2.2 problem, not a codegen detail.**
   Section 4 reason 3. Whatever widening item 6.2 chooses must route through
   `get_primitive_type` or add an equivalent guard, or the fork ships correct code
   on a device with `shaderInt64` and silently wrong code on one without. Section
   11.3 of this report verifies that both cards on this box and llvmpipe report
   `shaderInt64` true, so the development hardware does not expose it — which
   makes it more dangerous, not less, because it will not show up here.

4. **The sparse ceiling is now a live question and it is not mine.** Section 13.6
   establishes that the assert this territory has been treating as the hard stop
   does not fire on plan section 4.1's workload, and that the SPIR-V sparse path
   truncates through `snode.h:41` with no detector. Where the sparse path's own
   32-bit ceiling actually binds is territory 01, 02 or 03 work. I flag it because
   this territory has twice presented the wrong artefact as the operative
   constraint.

5. **The word "architectural" needs a definition from the arbiter before any of
   these gradings can be compared.** This is adversary 1's Escalation 13 and I
   pass it on rather than resolving it. The two adversaries graded findings
   against two different tests — "no external progress fixes it, only a Taichi
   decision" against "no decision fixes it either" — and the second makes almost
   nothing architectural, since any code can be rewritten. The brief's wording
   supports the first. Gradings in all four documents in this territory, mine
   included, are not comparable until it is fixed.

6. **Of the two obstacles this report called architectural, one is struck and one
   is hardened.** The module-wide addressing model does not survive: section 13.4
   refutes it from eleven shipped releases. The `at_buffer` collision does
   survive, and section 13.5 shows it is larger than any of the four documents in
   this territory originally described, reaching `make_value` at
   `spirv_ir_builder.h:291` rather than three functions. I record it that way
   because an earlier draft of this amendment had it the other way round, on a
   ground I found myself and then drew the wrong conclusion from. The favourable
   half of this territory's picture is real; this half is not, and it should not
   be sized as though it were.
