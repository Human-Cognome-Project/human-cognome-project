# Notes 05B — why upstream shelved the 64-bit work

Contemporaneous. Appended as I work. Read-only investigation.

## 1. Baseline

- HEAD `ba0e81dce559fb63a5958bf82feb1d00c55c02fe`, branch `master`, worktree clean apart from untracked `modernization/`.
- Remote: `upstream` only.

## 2. First look: the disabled producer site

`taichi/rhi/vulkan/vulkan_device_creator.cpp:812-830`, verbatim:

```cpp
    // Buffer Device Address
    if (CHECK_VERSION(1, 2) ||
        CHECK_EXTENSION(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)) {
      features2.pNext = &buffer_device_address_feature;
      vkGetPhysicalDeviceFeatures2KHR(physical_device_, &features2);

      if (CHECK_VERSION(1, 3) ||
          buffer_device_address_feature.bufferDeviceAddress) {
        if (device_supported_features.shaderInt64) {
// Temporarily disable it on macOS:
// https://github.com/taichi-dev/taichi/issues/6295
// (penguinliong) Temporarily disabled (until device capability is ready).
#if !defined(__APPLE__) && false
          caps.set(DeviceCapability::spirv_has_physical_storage_buffer, true);
#endif
        }
      }
```

Two distinct disablings stacked in one guard: a macOS carve-out citing issue 6295, and a later global `&& false` attributed to penguinliong citing "device capability is ready". Two events, not one. Need both commits.

## 3. VERIFIED — the two disabling commits on the Vulkan producer

Both found with `git log -- taichi/rhi/vulkan/vulkan_device_creator.cpp`.

**Event A — macOS carve-out.** `f677fb1417d906b2c13d1fa308378c4272c28f19`, Yi Xu
(strongoier), 2022-10-24, "[vulkan] [bug] Stop using the buffer device address
feature on macOS (#6415)". Diff adds `#if !defined(__APPLE__)` around the single
`caps.set(...)` line. Commit body cites
`https://github.com/taichi-dev/taichi/issues/6295#issuecomment-1288522905`.

**Event B — global kill.** `d20f55dc8a48ebaf016205d8eaa1bd0a0681f55d`,
PENGUINLIONG, 2022-10-30, "[aot] Disabled physical storage buffer temporarily
(#6468)". Diff is one line: `#if !defined(__APPLE__)` -> `#if
!defined(__APPLE__) && false`, plus the comment "(penguinliong) Temporarily
disabled (until device capability is ready)." **Commit body is empty.** No
rationale in the repository beyond that comment.

Six days apart. Distinct authors, distinct causes.

## 4. GITHUB — what issue 6295 actually says

Fetched via `gh api repos/taichi-dev/taichi/issues/6295`. Title: "The method
from_numpy() not working on Mac when using Vulkan backend". Opened 2022-10-11 by
Linyou, closed 2022-10-29. Symptom: on macOS + Vulkan (MoltenVK), ndarray reads
returned all zeroes; the same code was correct on Metal. Silent wrong answers,
not a crash.

Key comment, `1288522905`, strongoier, 2022-10-24 (the one the commit cites),
quoted in full:

> After bisecting MoltenVK commits, I found that the problem was introduced in
> https://github.com/KhronosGroup/MoltenVK/pull/1638, which enabled support for
> `VK_KHR_buffer_device_address` and `VK_EXT_buffer_device_address` on macOS
> 12.5. To make a healthy release, I'll submit a PR to stop using the feature on
> macOS. We can diagnose further about whether the problem comes from the
> feature itself or the use of the feature in Taichi in the future.

Earlier comments (strongoier 1285502939) establish the matrix: MoltenVK 1.1.10
correct, MoltenVK 1.1.11 wrong, independent of the Taichi commit. bobcao3
(1286332889) guessed at synchronisation; strongoier (1286478458) showed `ti.sync()`
did not help and suspected address alignment.

**Reading of event A: a driver/platform limitation, specifically MoltenVK's new
and evidently broken buffer-device-address support. NOT a judgement that the
approach is wrong.** The comment explicitly leaves open whether the fault is in
the feature or in Taichi's use of it, and nobody in the thread ever came back to
say.

## 5. VERIFIED — the second producer site

`c_api/src/taichi_vulkan_impl.cpp:44-52`. Not `#if`-ed out; commented out with
`/* */`:

```cpp
  // (penguinliong) Will bring it back after devcap.
  /*
  if (api_version > VK_API_VERSION_1_0) {
    caps.set(taichi::lang::DeviceCapability::spirv_has_physical_storage_buffer,
             true);
  }
  */
```

Same author as event B, same stated reason shape ("devcap" = device capability
work). Need the commit.

## 6. VERIFIED — commit that disabled the C-API producer

`12546578a4a954979be73ba0dab8e0c73612b826`, PENGUINLIONG, 2022-11-02,
"[aot] Disable physical storage buffer in runtime (#6494)". Body:

> The physical storage buffer codegen was disabled but the runtime can still use
> them. This is an attempt to disable psb at runtime. Will bring it back after
> devcap.

So event C is a *follow-up* to event B: after the shader-side capability was
forced off, the imported-runtime path in the C API was still advertising it, and
the two disagreed. Note the wording "This is an attempt to disable psb at
runtime" — tentative.

## 7. GITHUB — the rationale for the global kill (event B)

The commit body for `d20f55dc8` is empty, but PR 6468's body is not. Fetched via
`gh api repos/taichi-dev/taichi/pulls/6468`:

> Disabled for now because of multi-platform compatibility issues. Will be
> recovered when device capability is fully functional.

Approved by ailzhang with no comment text. Merged 2022-10-30T04:07:34Z. The only
other comment on the PR is a Netlify deploy bot. **There is no design discussion
on either disabling PR.** Same for 6494 (ailzhang APPROVED, body "Thanks!").

So event B's stated cause is (i) multi-platform compatibility and (ii) a
dependency on the "device capability" (devcap) subsystem being finished. Neither
is a claim that 64-bit addressing is the wrong approach.

## 8. VERIFIED — where the machinery came from

`git log --all --grep` for physical storage / buffer device address gives exactly
six commits, in order:

| Commit | Date | Author | Subject |
|---|---|---|---|
| `7705f688ac99ee6736dbd5bbda9e9c81b9d06c9b` | 2022-02-08 | Bob Cao | [vulkan] Add buffer device address (physical pointers) support & other improvements (#4221) |
| `06cb1c50db16912973b0f58f938cba6790ad85bb` | 2022-02-08 | Bob Cao | [vulkan] Disable buffer device address if int64 is not supported (#4244) |
| `8936fa60c9b477dc9524a1c0fac4d5aac4fbb444` | 2022-10-14 | PENGUINLIONG | [aot] Fixed buffer device address import (#6326) |
| `f677fb1417d906b2c13d1fa308378c4272c28f19` | 2022-10-24 | strongoier | macOS carve-out (#6415) |
| `d20f55dc8a48ebaf016205d8eaa1bd0a0681f55d` | 2022-10-30 | PENGUINLIONG | global kill (#6468) |
| `12546578a4a954979be73ba0dab8e0c73612b826` | 2022-11-02 | PENGUINLIONG | C-API kill (#6494) |

`7705f688a` is large: 14 files, `spirv_codegen.cpp` +415/-... . Its message is a
squashed list of branch commits with no design prose. One line of it is
"Do not use bitcast if we can use physical pointers" — that is the point of the
feature: without PSB the codegen has to bitcast a 32-bit offset into a buffer
index, with PSB it can carry a real 64-bit address.

`06cb1c50d` is important for section 6.2's stub note: it makes PSB conditional on
`device_supported_features.shaderInt64`, i.e. 64-bit integers in SPIR-V are
already treated upstream as a capability that can be absent. The `if
(device_supported_features.shaderInt64)` at `vulkan_device_creator.cpp:822` is
that commit's residue.

## 9. VERIFIED — `use_64bit_pointers` was never enabled, ever

`taichi/codegen/spirv/spirv_codegen.cpp:82`: `const bool use_64bit_pointers = false;`
Sole consumer at `:2318`, inside `make_pointer(size_t offset)`, `:2317-2324`:

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

`git log -S "use_64bit_pointers" --all` returns **one** commit:
`1b34a2d4d16ee2faaf52af926e9476f46325054d`, Bob Cao, 2021-10-25, "[vulkan]
Indexed load codegen (#3259)". `git log -L 82,82:taichi/codegen/spirv/spirv_codegen.cpp`
likewise returns only that commit. It was **born `false` and never touched**.

Original form in that commit was
`ir_->cast(ir_->u64_type(), ir_->uint_immediate_number(ir_->u32_type(), uint32_t(offset)))`
— i.e. it computed a 32-bit value and widened it, which would not have carried a
real 64-bit offset anyway. So the 64-bit branch as written was never correct.

**Reading: this is not shelved work. It is a placeholder that was never
started.** The comment "should check out how to encode uint64 values in spirv" is
an author noting they had not worked out the encoding, dated four months *before*
the physical-storage-buffer work landed. It is unrelated to events A/B/C.

## 10. VERIFIED — `PointerType::addr_space_` was born dead

`taichi/ir/type.h:181-208`. `git log -S "addr_space_" -- taichi/ir/type.h` gives
two commits only:

- `dcd5d7d35b6e4f4065fd6e00f1229114eb8510df`, Yuanming Hu, 2020-10-12, "[type] Add
  basic implementations of VectorType and PointerType (#1948)". This is the file's
  creation commit. It added, in one go, the private field
  `int addr_space_{0};  // TODO: make this an enum`, the getter
  `get_addr_space()`, **and no constructor parameter or setter for it**.
- `48fa799dabd...` (`48fa799da`), lin-hitonami, 2023-03-06, "[type] Let Type * be
  serializable", PR #7460. Purely mechanical: adds `addr_space_` to the
  `TI_IO_DEF` serialisation list. Does not make it settable.

`grep -rn "get_addr_space" taichi/ c_api/ python/ tests/` returns **one hit, the
definition itself at `taichi/ir/type.h:191`. Zero call sites.** Both constructors
(`type.h:184` and `type.h:186-191`) leave it at its default 0.

**Reading: this is not shelved 64-bit work at all. It is a placeholder field
added in 2020 alongside the type system, never wired to anything, never read.**
The TODO is about turning an int into an enum, not about address width.

## 11. VERIFIED — the snode.cpp warning, and the real ceiling it guards

`taichi/ir/snode.cpp:95-100` (current text at :97-99):

```cpp
  if (acc_shape > std::numeric_limits<int>::max()) {
    ErrorEmitter(
        TaichiIndexWarning(), &dbg_info,
        "SNode index might be out of int32 boundary but int64 indexing is not "
        "supported yet. Struct fors might not work either.");
  }
```

Origin: `cfc4065d3c7614b98584d7f5c0c43c1f4cebd30f`, Ailing, 2021-10-21, "[bug] Fix
silent int overflow in indice calculation. (#3177)". That commit widened
`acc_shape` from `int` to `int64` **but kept the store narrow**:
`new_node.extractors[i].acc_shape = static_cast<int>(acc_shape);` with the comment
"casting to int32 in extractors." So the fix was to *detect* the overflow and warn,
not to remove it. Same commit added an equivalent warning to the Metal struct
compiler ("int64 is not supported on metal backend") and widened locals in
`demote_dense_struct_fors.cpp` and `struct_opengl.cpp`.

Second edit to the text: `b347a21592221527e40d89706192e8b6e43c1a40`, Yi Xu,
2022-10-28, "[bug] Fix false overflow alarm in struct fors under packed mode
(#6457)", fixing issue 6258. That commit added the "Struct fors might not work
either" sentence and, in `demote_dense_struct_fors.cpp`, replaced
`TI_ASSERT(total_bits <= 30)` with
`TI_ASSERT(total_n <= std::numeric_limits<int>::max())`. Present at
`taichi/transforms/demote_dense_struct_fors.cpp:29`.

So the struct-for ceiling moved from 2^30 to 2^31-1 in Oct 2022 and has not moved
since. It is a hard assert, not a warning.

## 12. GITHUB — the int64-indexing thread is open and unanswered

Found by `gh api search/issues`, `repo:taichi-dev/taichi is:issue "int32 boundary"`
and `... int64 indexing`.

- **8608, "Implement int64 indexing", OPEN, filed 2024-12-18 by lstrgar**, label
  "feature request". Body quotes both the `snode.cpp` warning and the
  `demote_dense_struct_fors.cpp:29` assert verbatim, and links 5320 and 6258.
  Two comments, both users asking for movement (bgailleton 2025-06-30 "any news";
  lstrgar 2025-07-14 "I would like to bump this. Is this an easy fix?").
  **No maintainer has ever replied.** Still open at the time of this fetch.
- **5320, "Add int64 dtype support for RangeFor boundaries", OPEN, filed
  2022-07-04 by jim19930609** (a maintainer). Body is the closest thing to a
  design statement upstream produced:

  > Ideally, backend's dtype limitation should not bother the frontend design.
  > Unfortunately, we did forbiddened higher precision dtypes in certain cases due
  > to concerns regarding the backends ... We should add a "dtype demotion" pass
  > for each specific backend, and remove all these dtype conversions from the
  > frontend.

  jim19930609 follow-up 2022-07-08: "PR #5322 just added a warning message for
  this implicit conversion, which did not actually resolve this issue."
  Later comments are users only, through 2026-05.
- **8161, "ndrange is limited in size", OPEN, filed 2023-06-08.** listerily,
  2023-06-09: "Seems that M*M has overflown int32. There may be some difficulties
  to resolve it since some back-ends do not support int64. We will investigate it
  later." No further maintainer activity.

**Reading: the index-width half of the 64-bit story was never shelved after being
tried. It was never started.** The recorded maintainer position is that the
frontend narrows to int32 *because some backends lack int64*, and that the correct
fix is a per-backend demotion pass. That design was proposed in July 2022 and
never implemented. Upstream's own tracking issue is open, unanswered, and now
carries user bumps up to 2026.

## 13. VERIFIED — PSB shipped enabled for three releases, then was switched off for good

Checked the capability's state in each release tag by
`git show <tag>:taichi/rhi/vulkan/vulkan_device_creator.cpp` (falling back to the
pre-rename `taichi/backends/vulkan/...` path for old tags):

| Tag | State |
|---|---|
| v1.0.1 | ENABLED |
| v1.0.4 | ENABLED |
| v1.1.0 | ENABLED |
| v1.1.3 | ENABLED |
| v1.2.0 | disabled on macOS only |
| v1.2.1 | disabled on macOS only |
| v1.2.2 | disabled on macOS only |
| v1.3.0 | DISABLED globally |
| v1.4.0 | DISABLED globally |
| v1.7.0 | DISABLED globally |
| v1.7.4 | DISABLED globally |

**This is the single most important fact in this investigation.** The
shader-side 64-bit pointer path is not unfinished speculative code. It was
written in Feb 2022, shipped live in four public releases across roughly eight
months, and was then switched off in Oct 2022. Everything from v1.3.0 to the last
release v1.7.4 has it off.

## 14. VERIFIED — no abandoned 64-bit branch exists

`git log --all --not master -S "use_64bit_pointers"` returns **nothing**.

`git log --all --not master -S "spirv_has_physical_storage_buffer"` returns only
ghstack scratch refs (`gh/<user>/<n>/{base,head,orig}`) and release/cherry-pick
branches for work that landed in master anyway.

I swept every `refs/remotes/upstream/*` ref for one where the capability is set
without the `&& false`. All ~40 hits are branches that predate 2022-10-30, i.e.
they are simply old, not alternative attempts. No branch, tag or ref carries a
continuation of the 64-bit work.

## 15. GITHUB — what "devcap" was, and why PSB was blocked on it

PR 6184, "[aot] Device capability refactorization", PENGUINLIONG, merged
2022-10-21 (commit `07446dd0c`), nine days before the kill. Body, quoted:

> This PR refactorized `DeviceCapability` control logics in Taichi.
> `DeviceCapability` controls the compilation of kernels and the execution
> behavior on device. The current implementation collect capabilities from the
> device interface but **it blocks extension of AOT code generation because it
> depends on the currently attached compute device whether to enable a set of
> feature during compilation.** AOT users, however, demands an interface to
> control the availability of `DeviceCapability`.
> ...
> Python and C-API users will be able to configure `DeviceCapability`s in future
> PRs.

(Emphasis mine.) This is the shape of the problem. PSB was auto-detected from the
attached device. Under AOT the compile machine and the run machine differ, so a
capability probed at compile time is a lie at run time. The C-API loader enforces
this strictly: `c_api/src/taichi_gfx_impl.cpp:18-30` compares each required cap of
a loaded AOT module against the runtime's caps and returns
`TI_ERROR_INCOMPATIBLE_MODULE` on **any** mismatch, including a runtime that has
*more* capability than required (`current_version != required_version`, not `<`).

So a module built with PSB on a machine that had it would refuse to load anywhere
that did not, and — because the check is inequality, not a floor — also anywhere
that did not report exactly the same level. Forcing PSB off everywhere makes every
module portable. **That is a coherent reading of "multi-platform compatibility
issues" in PR 6468's body.** It is an AOT-portability decision, not a claim that
64-bit addressing is wrong.

Surrounding evidence that this was the live concern in exactly that window:
`38b8ef76e` "[bug] Fix that the cgraph doesn't respect the caps set by
ti.aot.Module() (#6520)", merged 2022-11-04, two days after the C-API kill.

## 16. VERIFIED — devcap did land, and PSB was still not restored

The devcap follow-ups the 6184 body promised did happen:

| Commit | Date | Subject |
|---|---|---|
| `29749416a` | 2022-11 | [aot] C-API device capability query (#6549) |
| `a0227ca89` | 2022-11 | [aot] Test for AOT device capability (#6618) |
| `fdde622a5` | 2022-11 | [aot] Yet another device capability test (#6623) |
| `c27a2e474` | 2022-12 | [aot] C-API Device capability improvements (#6702) |
| `3711e8b18` | 2022-12 | [aot] Revert C-API Device capability improvements (#6772) |
| `bed652dc2` | 2022-12 | [aot] C-API Device capability improvements (#6773) |
| `285fe5213` | 2023-02-21 | [aot] Simplify device capability assignment (#7407) |
| `fcd46769e` | 2023-03 | [metal] Choose the proper msl version according to the device capability (#7506) |

User-facing capability configuration exists today on both paths:
- Python: `Program::make_aot_module_builder` -> `translate_devcaps`,
  `taichi/program/program.cpp:514-538, 541-556`.
- C API: `ti_set_runtime_capabilities_ext`, `c_api/src/taichi_core_impl.cpp:317-334`,
  and the fluent setter `spirv_has_physical_storage_buffer` at
  `c_api/include/taichi/cpp/taichi.hpp:1156-1160`.

**Nobody ever went back and lifted the `&& false`.** The stated precondition was
substantially met by Feb 2023; the guard is still in the tree at the fork commit
`ba0e81dce`, dated 2025-07-30, and in the last release v1.7.4.

**Reading: after the AOT-portability cause was addressed, the restoration simply
never happened. That is momentum loss, not a further technical objection.** I have
found no upstream statement retracting the feature on merit.

## 17. VERIFIED — the JIT path has no way to turn PSB on

Even though the caps *config* is now settable, on the ordinary (non-AOT) Vulkan
path the caps come from `VulkanDeviceCreator::create_logical_device`, which is the
site with `&& false`. `grep -rn "physical_storage_buffer" taichi/ c_api/ python/`
gives exactly one `caps.set(...)` in `taichi/`, at
`taichi/rhi/vulkan/vulkan_device_creator.cpp:826`, and it is inside the dead
`#if`. Every other hit is a *reader*:

- `taichi/rhi/vulkan/vulkan_device.cpp:1772, 1792, 2147, 2509`
- `taichi/codegen/spirv/spirv_ir_builder.cpp:73, 113` (declares
  `CapabilityPhysicalStorageBufferAddresses`, `SPV_KHR_physical_storage_buffer`,
  and switches the SPIR-V addressing model to
  `AddressingModelPhysicalStorageBuffer64` at :120)
- `taichi/codegen/spirv/spirv_types.cpp:436`
- `taichi/codegen/spirv/spirv_codegen.cpp:783, 2201, 2340, 2416, 2491`
- `taichi/runtime/gfx/runtime.cpp:96`
- `taichi/runtime/program_impls/gfx/gfx_program.h:81`
- `taichi/inc/rhi_constants.inc.h:28` (the capability's declaration)

That is twelve live consumer sites against zero live producers. The consuming code
was never removed; it is simply unreachable in a normal build.

## 18. VERIFIED — the two mechanisms address DIFFERENT things

This is the finding I would most want an adversary to attack, so it is stated
carefully with call sites.

`use_64bit_pointers` gates `make_pointer(size_t offset)`
(`spirv_codegen.cpp:2317-2324`). `make_pointer` is called at exactly four sites,
all on the **SNode / field** path:

- `:355` `visit(GetRootStmt *)` — `spirv::Value root_val = make_pointer(0);`
- `:371` `visit(GetChStmt *)` — `make_pointer(desc.mem_offset_in_parent_cell)`
- `:408` `bitmasked_activation` — `make_pointer(desc.cell_stride * desc.snode->num_cells_per_container)`
- `:506` (inside the SNode lookup path) — `make_pointer(desc.cell_stride)`

`spirv_has_physical_storage_buffer` gates, in the codegen:

- `:783`, inside `visit(ExternalPtrStmt *)` (the visitor opens at `:734`) — loads a
  `u64` device address out of the args buffer and adds the linear offset. This is
  the **ndarray / external array** path, the host-numpy-array path, not SNodes.
- `:2340`, `:2416`, `:2491` — passes `has_buffer_ptr` to `translate_ti_type` when
  laying out the args, argpack and ret structs, so that a `PhysicalPointerType`
  member is laid out as a 64-bit pointer (`spirv_types.cpp:433-439`,
  `visit_physical_pointer_type`, emitting
  `OpTypePointer ... StorageClassPhysicalStorageBuffer`).
- `:2201`, inside `at_buffer(const Stmt *ptr, DataType dt)` at `:2194` — **the
  join point**: if the incoming pointer value's type is `u64`, emit
  `OpConvertUToPtr` to a `PhysicalStorageBuffer` pointer; otherwise shift the
  32-bit offset into a `StorageBuffer` array index.

**So the SNode root address is a 32-bit byte offset into a bound storage buffer,
and always has been. The physical-storage-buffer work never touched it.** PSB
gave 64-bit device addresses to ndarray arguments only.

## 19. VERIFIED — flipping `use_64bit_pointers` would not compile correct SPIR-V

Two independent reasons, from the code as it stands:

1. `at_buffer` (`:2194-2205`) dispatches on `ptr_val.stype.dt == PrimitiveType::u64`
   and, if so, treats the value as a **device address** via `OpConvertUToPtr`. But
   `visit(GetRootStmt *)` at `:355` produces `make_pointer(0)` — the literal zero.
   With `use_64bit_pointers = true` that zero becomes a `u64` and would be
   converted to a null physical pointer. The root buffer's actual device address is
   never fetched anywhere on this path. That is precisely what the author's comment
   "This is hacky, should check out how to encode uint64 values in spirv" is
   flagging.
2. `bitmasked_activation` (`:381-415`) mixes `ptr_dt` (which would become u64) with
   hardcoded `ir_->u32_type()` at `:404`, `:411`, `:412`, `:413`, and adds
   `parent_ptr` to a u32 value at `:409`. SPIR-V `OpIAdd` requires matching operand
   widths, so this would be invalid.

**Reading: `use_64bit_pointers` is a stub with a half-written body, not a working
implementation someone turned off.** Its value to this project is as a marker of
where the SNode addressing width lives on the SPIR-V path, not as reusable code.

## 20. VERIFIED — 64-bit scalars already work on the SPIR-V path; 64-bit pointers do not

`spirv_has_int64` is a **live** capability, produced at
`taichi/rhi/vulkan/vulkan_device_creator.cpp:630-633`:

```cpp
  if (device_supported_features.shaderInt64) {
    device_features.shaderInt64 = true;
    caps.set(DeviceCapability::spirv_has_int64, true);
  }
```

No `#if`, no `&& false`. Consumed at `spirv_ir_builder.cpp:64-66` (emits
`OpCapability Int64`), `:166-169` (declares `t_int64_` / `t_uint64_`), and
`:311-313`, `:325-327` (errors if an i64/u64 type is requested without it).

So the SPIR-V backend can already emit and manipulate 64-bit integers when the
device reports `shaderInt64`. Only the 64-bit *addressing model*
(`AddressingModelPhysicalStorageBuffer64`, `spirv_ir_builder.cpp:113-125`) is off.

Separately and at a higher level, `Extension::data64` — the frontend's
architecture-level 64-bit-data flag — is granted at
`taichi/program/extension.cpp:9-28` to `x64`, `arm64` and `cuda` **only**. Not to
`vulkan`, `opengl`, `gles`, `metal`, `dx11` or `amdgpu`. The OpenGL grant is
commented out at `:29-30`:

```cpp
  // if (with_opengl_extension_data64())
  // arch2ext[Arch::opengl].insert(Extension::data64); // TODO: singleton
```

That commenting-out happened in `b3446e905a98ebd469e6a18c04d3b4ca957ec922`, Bo
Qiao, 2022-04-30, "[Build] [refactor] Define Cmake OpenGL runtime target (#4887)",
i.e. as collateral of a build refactor, not as a considered removal. OpenGL 64-bit
data types had been added in `d9a803a3f`, "[OpenGL] 64-bit data type support
(#717)".

`is_extension_supported` has no C++ caller that consults `data64`
(`grep -rn "is_extension_supported"` shows callers for `assertion`, `bls`, `mesh`,
`quant`, `sparse` only). `data64` reaches only Python, and in the tree its only
uses are three test gates (`tests/python/test_svd.py:8`,
`test_floor_dtype_argument.py:43`, `test_quant_atomics.py:44`). **So `data64` is
advisory today, not enforcement.**

## 21. Searches that came back empty (recorded so they are not repeated)

- `docs/design/` holds one file, `llvm_sparse_runtime.md`; `docs/rfcs/` holds the
  RFC process doc, one AOT RFC and a template. **No design note on 64-bit
  addressing, index width, or physical storage buffers exists in the tree.**
- `grep TODO|FIXME|HACK|XXX` across `taichi/` filtered for 64/addr/pointer/index
  yields nothing bearing on this question except `type.h:207` (the addr_space TODO,
  see entry 10) and `codegen_llvm_quant.cpp:75` (CUDA atomicCAS widths, unrelated).
- GitHub issue search for "physical storage buffer" returns one issue, 4919, "GGUI
  issues on Intel graphics card". PSB appears there only inside a pasted
  `print_all_cap` trace log; **nothing in that thread attributes the Intel failure
  to PSB.** I am not treating it as evidence either way.
- No upstream issue, PR or commit anywhere states that the physical-storage-buffer
  approach was judged wrong on merit.

## 22. Consumer count checked

`grep -rn "get(DeviceCapability::spirv_has_physical_storage_buffer)\|get(cap::spirv_has_physical_storage_buffer)" taichi/`
returns 10. Two further reads are split across lines and missed by that pattern:
`taichi/runtime/gfx/runtime.cpp:95-96` and
`taichi/runtime/program_impls/gfx/gfx_program.h:80-81`. **Twelve live consumers,
zero live producers.** This matches the count in the brief.

## 23. Note on the LLVM path

Out of scope for this question, which is about the portable path, so I did not
pursue it. Flagged only because `taichi/codegen/llvm/codegen_llvm.cpp:1131-1134`
takes an `int addr_space` parameter (declared with default 0 at
`codegen_llvm.h:165`) and is the only place in the tree where an address space is
actually threaded anywhere. It is unconnected to `PointerType::addr_space_`.

---

# EXTENSION — does each reason still hold today? (added after brief extension)

## 24. VERIFIED — the Vulkan 1.2 promotion hypothesis is wrong, and in our favour

The extended brief suggests the shelving may predate or straddle the promotion of
`VK_KHR_buffer_device_address` into core Vulkan 1.2. It does not. Vulkan 1.2 was
released 2020-01, and the extension was promoted into it at that point. The three
disabling commits are from **October and November 2022**, roughly two years and
nine months later.

Better: the code already handles the promotion correctly. At
`taichi/rhi/vulkan/vulkan_device_creator.cpp:812-819`, using the macros at `:714`
and `:720-721`:

```cpp
    if (CHECK_VERSION(1, 2) ||
        CHECK_EXTENSION(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)) {
      ...
      if (CHECK_VERSION(1, 3) ||
          buffer_device_address_feature.bufferDeviceAddress) {
        if (device_supported_features.shaderInt64) {
```

`CHECK_VERSION(major, minor)` is `vk_api_version >= VK_MAKE_API_VERSION(0, major, minor, 0)`;
`CHECK_EXTENSION(ext)` searches `enabled_extensions`. So core-1.2 availability and
the standalone extension are both already accepted.

**Availability was never the blocker.** The promotion had already happened when
upstream switched the feature off. Nothing about the API-availability picture has
changed since in a way that bears on this.

## 25. VERIFIED ON THIS BOX — the target hardware supports everything the guard asks for

Ran `vulkaninfo` and `nvidia-smi` read-only on the development/edge box described
in plan section 3.1.

```
NVIDIA GeForce GTX 1070,   driver 535.309.01
NVIDIA GeForce GTX 750 Ti, driver 535.309.01
```

`vulkaninfo` reports, for **both** NVIDIA devices:

| Property | GTX 1070 | GTX 750 Ti | llvmpipe (CPU) |
|---|---|---|---|
| `apiVersion` | 1.3.242 | 1.3.242 | 1.4.318 |
| `VK_KHR_buffer_device_address` | present | present | present |
| `VK_EXT_buffer_device_address` | present | present | absent |
| `shaderInt64` | true | true | true |
| `bufferDeviceAddress` | true | true | true |
| `bufferDeviceAddressCaptureReplay` | true | true | false |
| `bufferDeviceAddressMultiDevice` | true | true | false |

Instance version 1.3.275.

Every clause of the guard at `vulkan_device_creator.cpp:812-822` is satisfied on
all three devices, **including the Maxwell GTX 750 Ti, which is the baseline tier
in plan section 5.2, and including `llvmpipe`, the CPU software renderer**. If the
`&& false` at `:825` were removed, the capability would be set on this box on every
device present, GPU and CPU alike.

Note for plan section 2's "no GPU at all" first-class target: llvmpipe reports both
`shaderInt64` and `bufferDeviceAddress` true. I have verified the report, not the
behaviour.

## 26. VERIFIED — the submodule picture splits in two

`git ls-tree HEAD external/` and `git -C external/<d> log -1`:

| Submodule | Pinned revision | Date of that revision |
|---|---|---|
| `external/Vulkan-Headers` | `409c16be502e39fe70dd6fe2d9ad4842ef2c9a53` | 2025-04-18, "Update for Vulkan-Docs 1.4.313" |
| `external/volk` | `b87f88292b09bc899b24028984186581a1d24c4e` | 2023-03-24, "Update to 1.3.245" |
| `external/SPIRV-Headers` | `34d04647d384e0aed037e7a2662a655fc39841bb` | **2022-12-15** |
| `external/SPIRV-Cross` | `c77b09b57c27837dc2d41aa371ed3d236ce9ce47` | **2022-11-21** |
| `external/SPIRV-Tools` | `46ca66e6991f16c89e17ebc9b86995143be2c706` | **2022-11-18** |
| `external/SPIRV-Reflect` | `7c9c841fa9f40c09d334d5db6629ba318e46efaf` | (bumped with the above) |

`external/Vulkan-Headers/include/vulkan/vulkan_core.h:72,75`: `VK_HEADER_VERSION 313`,
`VK_HEADER_VERSION_COMPLETE` = 1.4.313.

The last commit to touch the four SPIR-V submodules is
`ee0af32abe2ca6e8ad2de4eea39d2e3ed7bd5c43`, 2023-04-03, "[vulkan] Update SPIR-V
codegen to emit FP16 consts (#7676)". Comparing `git ls-tree ee0af32ab external/`
with `git ls-tree HEAD external/` shows the SPIR-V four are **byte-identical**;
only `Vulkan-Headers` and `VulkanMemoryAllocator` moved after that, via
`4c2bac69ea237236f418637d8f91b14bfd0c78d3`, 2025-04-25, "[Build] Update VMA and
Vulkan-Headers to support Vulkan 1.3 (#8680)".

**So: the Vulkan API headers we actually have are current (April 2025). The SPIR-V
toolchain we actually have is from November and December 2022, contemporaneous with
the shelving itself, and has not moved in over three years.**

The pinned SPIRV-Headers do carry the needed enums —
`external/SPIRV-Headers/include/spirv/unified1/spirv.hpp:106` (`AddressingModelPhysicalStorageBuffer64 = 5348`),
`:229` (`StorageClassPhysicalStorageBuffer = 5349`),
`:1062` (`CapabilityPhysicalStorageBufferAddresses = 5347`). Nothing is missing at
the header level.

## 27. VERIFIED — one SPIRV-Tools fix we do NOT have touches a pass Taichi runs

Searched SPIRV-Tools for merged pull requests mentioning PhysicalStorageBuffer
after our pin date: `gh api search/issues`,
`repo:KhronosGroup/SPIRV-Tools is:pull-request PhysicalStorageBuffer merged:>2022-11-18`.
Eleven results, 5291, 5316, 5476, 5539, 5575, 5635, 5977, 6000, 6266, 6275, 6714.

Most are **validator** changes. Taichi disables the validator:
`spirv_codegen.cpp:2710`, `spirv_opt_options_.set_run_validator(false);`. Those
therefore do not bear on us.

The one that does: **SPIRV-Tools PR 5316, "[spirv-opt] Handle OpFunction in
GetPtr", merged 2023-07-17.** Body:

> When using PhysicalStorageBuffer it is possible for a function to return a
> pointer type. This was not being handled correctly in
> `GetLoadedVariablesFromFunctionCall` in the DCE pass.
> Fixes https://github.com/KhronosGroup/SPIRV-Tools/issues/5270.

Files changed: `source/opt/aggressive_dead_code_elim_pass.cpp`,
`source/opt/mem_pass.cpp`, plus its test.

Taichi registers `spvtools::CreateAggressiveDCEPass()` at
`taichi/codegen/spirv/spirv_codegen.cpp:2690`. **We are pinned eight months before
that fix, so our copy does not have it.**

Whether Taichi's generated SPIR-V can actually trigger it is NOT established. It
requires a function returning a pointer, and Taichi runs
`CreateInlineExhaustivePass()` at `:2688` before DCE, which would inline most
functions away. I am recording the gap, not claiming a live fault.

The SPIRV-Tools submodule checkout appears to be shallow (`git -C
external/SPIRV-Tools log --all` returns a single commit), so I could not search its
history locally and used GitHub for this.

## 28. GITHUB — the macOS blocker has NOT expired

- MoltenVK issue **2174, "regression bug with vkGetBufferDeviceAddress", OPEN**,
  filed 2024-03-07. Reporter states: *"MoltenVK latest: Support for the buffer
  device address extension for my GPU has been removed entirely. v1.2.0 through
  v1.2.6: vkBufferDeviceAddress returns 0. v1.1.11 (with normal SPIRV-Cross):
  vkBufferDeviceAddress is segfaulting."*
- MoltenVK issue **2696, "VK_ERROR_OUT_OF_DEVICE_MEMORY when using device address
  buffer with indirect draw/dispatch on MoltenVK", OPEN**, filed 2026-03-06.

Note "vkBufferDeviceAddress returns 0" is the same class of symptom as Taichi issue
6295 (silent zeroes) four years earlier.

**The macOS blocker is environmental but has not expired.** It is, however, on a
platform absent from plan section 5.2's tier table.

## 29. Sorting each reason

| # | Reason | Kind | Still holds? |
|---|---|---|---|
| 1 | MoltenVK buffer-device-address broken (event A) | ENVIRONMENTAL | YES, still open upstream. Irrelevant to plan 5.2 tiers |
| 2 | AOT capability equality check / cross-machine portability (event B) | **ARCHITECTURAL** in upstream's AOT model | Code unchanged; applicability to this project is a planner call |
| 3 | "devcap not yet functional" (events B and C) | ENVIRONMENTAL, tooling | **EXPIRED.** Devcap landed by Feb 2023 |
| 4 | Vulkan API availability of buffer_device_address | ENVIRONMENTAL | **NEVER APPLIED.** Core since 1.2 (2020), already handled by the guard |
| 5 | Hardware support on target tiers | ENVIRONMENTAL | **NOT A BLOCKER.** Verified present on GTX 750 Ti, GTX 1070 and llvmpipe on this box |
| 6 | SPIR-V toolchain maturity | ENVIRONMENTAL | Fixed upstream, **NOT fixed in our checkout**. See entry 27 |
| 7 | `use_64bit_pointers` root pointer is a buffer offset, not a device address | **ARCHITECTURAL** | YES. Nothing has changed. See entry 19 |
| 8 | `bitmasked_activation` mixes u32 and pointer width | ARCHITECTURAL, but local and fixable | YES, code unchanged |
| 9 | Frontend narrows indices to int32 because some backends lack int64 | **ARCHITECTURAL** | YES. Proposed fix (per-backend demotion pass) never built |
| 10 | `demote_dense_struct_fors.cpp:29` INT_MAX assert | **ARCHITECTURAL** | YES, unchanged since Oct 2022 |

---

# Amendment pass, 2026-09-09. Entries 30 onward.

Contemporaneous, written as each claim was checked. Repo at
`ba0e81dce559fb63a5958bf82feb1d00c55c02fe`. Read-only throughout: no source file
touched, no git state changed, no writes to GitHub. Read both
`adversary-05-1.md` and `adversary-05-2.md` in full before starting, then
re-derived every claim rather than accepting either.

## 30. CLAIM 1 — un-defaulted, not off. VERIFIED. This is the big one.

Traced the ahead-of-time chain hop by hop, looking for a device check at each:

- `taichi/program/program.cpp:541-556` `Program::make_aot_module_builder(Arch, const std::vector<std::string> &caps)`.
- `:544` calls `translate_devcaps(caps)`, defined `:514-538`.
- `:530` `str2devcap(key)` on an arbitrary caller string.
- `taichi/rhi/device_capability.cpp:6-13` `str2devcap` expands
  `taichi/inc/rhi_constants.inc.h`, where `spirv_has_physical_storage_buffer` is
  line 28 (grepped, confirmed :28). So it is a valid input name.
- `:554` `program_impl_->make_aot_module_builder(cfg)`.
- `taichi/runtime/program_impls/gfx/gfx_program.cpp:30-40` passes `caps` straight
  into `gfx::AotModuleBuilderImpl`. **No examination of any kind.**
- `taichi/runtime/gfx/aot_module_builder_impl.cpp:13-27` stores it as `caps_`.
- `:75` and `:119` `compilation_manager_.load_or_compile(config_, caps_, *kernel)`.
- `taichi/codegen/spirv/kernel_compiler.cpp:25-37`, `params.caps = device_caps` at `:37`.
- `taichi/codegen/spirv/spirv_codegen.cpp:86` `caps_(params.caps)`; read at `:783`,
  `:2340`, `:2416`, `:2491`.

NO DEVICE CHECK ANYWHERE. Confirmed the contrast:
`GfxProgramImpl::get_device_caps` at `gfx_program.cpp:82-85` returns
`runtime_->get_ti_device()->get_caps()` — that IS the device path, and it is what
the `&& false` closes. The AOT builder does not go through it.

C API half: `c_api/src/taichi_core_impl.cpp:317-334`, loop at `:326-330`,
`runtime2->get().set_caps(std::move(devcaps))` at `:331`. `set_caps` at
`taichi/rhi/public_device.h:855` — checked, it replaces, does not merge, does not
validate. Typed setter `c_api/include/taichi/cpp/taichi.hpp:1156-1159`, public
enum `c_api/include/taichi/taichi_core.h:399` = 18.

Python is one caller (`export_lang.cpp:403`, `python/taichi/aot/module.py:110`,
`caps` param documented at `:91`), but the C++ signature is the producer, so plan
§1.2 does not remove it.

Test coverage re-checked and widened:
`grep -rn 'physical_storage_buffer\|PHYSICAL_STORAGE_BUFFER\|buffer_device_address\|BufferDeviceAddress\|PhysicalStorageBuffer' tests/ c_api/tests/ benchmarks/ misc/`
→ **0 hits**. So the reachable path leads into branches nothing tests.

My report said "zero live producers." Wrong. Report §2.5 rewritten and a new §1.1
added. Adversary 1 also has this wrong and files the ref sweep under what it could
not break — the sweep is right about committed source lines, wrong as an answer to
"does a producer exist".

## 31. CLAIM 2 — rebuild the consumer inventory. DONE FROM SCRATCH.

Whole-tree grep for the capability name, then followed `has_buffer_ptr`.

Direct readers, twelve, counted off the grep not off memory:
`vulkan_device.cpp:1772,1792,2147,2509`; `spirv_ir_builder.cpp:73,113`;
`spirv_codegen.cpp:783,2340,2416,2491`; `runtime/gfx/runtime.cpp:96`;
`gfx_program.h:81`. 4+2+4+1+1 = 12.

My old list had twelve as a number but bullets that do not enumerate to twelve,
and included `spirv_codegen.cpp:2201` (which reads no capability — it is the u64
type sniff) and `spirv_types.cpp:433-439`. Both wrong entries.

`has_buffer_ptr` grep gives the three indirect ones both adversaries name:
- `spirv_types.cpp:484-514` `translate_ti_type`, decision at `:490-497`
  (IntType 64 at `:492-493`, IntType 32 at `:495-496`). Read it. This is where
  the width is actually chosen; the three call sites only carry the bool.
- `runtime/gfx/runtime.cpp:823` (`layout[1] == 'b'`) and `:851-859` (PointerType
  branch, 8 bytes at `:853`, 4 at `:857`). Callers traced:
  `callable.cpp:159-161`, `:180-182`, `argpack.cpp:15`.
- `spirv_ir_builder.cpp:334-352` `from_taichi_type`, pointer branch `:337-342`.
  `grep -rn from_taichi_type taichi/ c_api/` → 3 hits: definition `:334`,
  self-recursion `:347`, declaration `spirv_ir_builder.h:343`. **DEAD.**

Removal: `spirv_types.cpp:433-439` is unreachable, see entry 32.

## 32. CLAIM 3 — dead SNode code. VERIFIED. Adversary 2 alone found this.

`taichi/codegen/spirv/snode_struct_compiler.cpp`:
- `:34-63` private `construct(tinyir::Block&, SNode*)`.
- `:52-54` `if (sn->type == SNodeType::pointer) { cell_type = ir_module.emplace_back<PhysicalPointerType>(cell_type); }`
- Only call site `:18`, inside a `/* */` at `:16-19`.
- `snode_struct_compiler.h:48-50` — `type_factory` and `root_type` commented out
  under `// TODO: Use the new type compiler`. `grep -rn root_type taichi/` finds
  no user outside this file.
- `PhysicalPointerType` declared `spirv_types.h:71-91`, constructor fixes 64 bits
  unsigned at `:74-77`. Its SPIR-V translation at `spirv_types.cpp:433-439` emits
  `OpTypePointer` + `StorageClassPhysicalStorageBuffer`.
- Construction sites, whole tree: `snode_struct_compiler.cpp:53` (dead) and
  `spirv_types.cpp:337` (type reducer, copies an existing node). No live producer
  → the `:433` visitor never runs → my §2.5 listing it as live was wrong.
- `git log -S "PhysicalPointerType" --all -- taichi/` → **one** commit,
  `46b5632e5e61036a788013780d0d9b9eacfd7f4f`, Bob Cao, 2022-06-22, #5213. Read its
  diff: the call site is added ALREADY COMMENTED OUT. Never live for one commit.

Weighing: not an asset (never compiled into anything, `CompiledSNodeStructs` has
moved on), not surplus (plan §10.3). Evidential both ways — it falsifies the
intent half of my §3, and it supplies the Taichi source line adversary 1 says does
not exist for its own bound-buffer argument. Escalated.

## 33. CLAIM 4 + CLAIM 8 — the tag census, by enumeration.

Wrote a loop over `git tag`, extracting `taichi/rhi/vulkan/vulkan_device_creator.cpp`
with fallback to `taichi/backends/vulkan/`, locating the `caps.set` line and
classifying on the 8 lines above it. Full output kept.

- total tags: **121** (`git tag | wc -l`). Adversary 1 says 122. Measured 121.
- NO_FILE: **92** (counted mechanically with a second loop, not by subtraction).
- file present, capability absent: v0.8.7–v0.8.11 = **5**
- ON unguarded: v0.9.0, v0.9.1, v0.9.2, v1.0.0, v1.0.1, v1.0.2, v1.0.3, v1.0.4,
  v1.1.0, v1.1.2, v1.1.3 = **11**
- `#if !defined(__APPLE__)`: v1.2.0, v1.2.1, v1.2.2 = **3**
- `#if !defined(__APPLE__) && false`: v1.3.0, v1.4.0, v1.4.1, v1.5.0, v1.6.0,
  v1.7.0, v1.7.1, v1.7.2, v1.7.3, v1.7.4 = **10**

92+5+11+3+10 = 121. Reconciles. Per plan §10.7 the count comes from the list.

ADJUDICATION of the divergence: **adversary 2 correct, adversary 1 three tags
short** (misses v0.9.x) **and one tag over on the total**. Adversary 1 does handle
the backends→rhi path move, which is why it got v1.0.0–v1.0.3.

My own four-tag table was a spot check presented as a census. Charge upheld
against me. Rewritten.

Claim 4 second half, v1.1.3 (2022-09-20, capability ON per the census):
`git show v1.1.3:.../spirv_codegen.cpp` → `:74 const bool use_64bit_pointers = false;`,
`:275 make_pointer(0)`, `make_pointer` body at `:2062-2070` returning u32 on the
false branch. `git show v1.1.3:.../spirv_ir_builder.cpp` → `:114`
`AddressingModelPhysicalStorageBuffer64`, `:119` `AddressingModelLogical`.
**Mixed addressing shipped.** Eleven tags, not eight. The addressing model is a
declaration, not a constraint. Claim 4 UPHELD.

Second refutation checked independently: `spirv_codegen.cpp:97` builds an
`IRBuilder` per `TaskCodegen`; `run()` at `:130-156` finalizes at `:151`. One
module per offloaded task, so "every shader in the module" is one shader.

## 34. CLAIM 5 — the at_buffer divergence. ADJUDICATED FOR ADVERSARY 2.

Adversary 1: architectural, "no third thing to disambiguate".
Adversary 2: not architectural, the tag already exists and is already set.

Checked four ways:
1. `spirv_ir_builder.h:69-79` `enum class ValueKind`, `kPhysicalPtr` at `:75`.
   `struct Value` at `:82-93`, `ValueKind flag{ValueKind::kNormal}` at `:88`.
2. `spirv_codegen.cpp:2203` `paddr_ptr.flag = ValueKind::kPhysicalPtr;`.
   **NOTE: both adversaries cite `:2204`. That is the `return`.** Off by one, both.
3. **The check neither adversary ran.** Is the flag ever READ?
   `grep -rn "\.flag\b\|kPhysicalPtr" taichi/codegen/spirv/` →
   `spirv_ir_builder.cpp:1271-1273` asserts the flag is one of three pointer
   kinds, `:1275` branches on `kPhysicalPtr` to emit `OpLoad` with
   `MemoryAccessAlignedMask` + explicit alignment at `:1277-1280` instead of the
   plain form at `:1282`. `store_variable` at `:1286-1298` does the same at
   `:1290-1294`. **The tag is already load-bearing on exactly the branch in
   dispute.** This settles it: adversary 1's stated ground is false.
4. Round trip: `register_value` `:1300-1309` stores the whole Value at `:1308`;
   `query_value` `:1311-1317` returns by value at `:1314`. Flag survives.

BUT — adversary 2's sizing is understated, and this is mine:
`GetChStmt` at `:372` does `ir_->add(input_ptr_val, offset)`. `add` is
`DEFINE_BUILDER_BINARY_USIGN_OP` at `spirv_ir_builder.cpp:1038-1047` → `make_value`
→ `spirv_ir_builder.h:291-298`, which constructs with `kNormal` at `:292` and
upgrades to `kVariablePtr` only if `out_type.flag == TypeKind::kPtr` at `:294-295`.
A u64 integer is not `kPtr`. **The tag is dropped on every pointer add.** Same at
`bitmasked_activation:406-409`. So tagging at production also needs the arithmetic
builders changed. Still design work, not permanence — ruling stands, sizing
corrected.

Corroboration for "known-fragile shortcut": PR #8315's human summary describes
`at_buffer` being "called with wrong pointer dtype" as a bug, fixed locally.

## 35. CLAIM 6 — the sparse gate and the extractor fields. VERIFIED, with a refinement.

Gate: `demote_dense_struct_fors.cpp:13-112` `convert_to_range_for` (assert at
`:29`); only caller `maybe_convert` at `:114-119`, requiring
`stmt->snode->is_path_all_dense`. Flag declared `snode.h:114` default true,
cleared at `snode.cpp:20`. Same flag read at `offload.cpp:192`. Plan §4.1 requires
sparsity → **the assert never runs on this project's trees.** Claim UPHELD.

`TI_ASSERT` is real: `taichi/common/logging.h:100` → `TI_ASSERT_INFO` at
`:101-107`, a plain `if (!x) TI_ERROR(...)`, no NDEBUG gate. So it is a hard stop
where it does run, which is what issue 8608 reports hitting.

Fields: `snode.h:37-54` `struct AxisExtractor` —
`:41 int num_elements_from_root{1}`, `:45 int shape{1}`, `:49 int acc_shape{1}`.
`:97 int64 num_cells_per_container{1}` — ALREADY WIDE. Truncation at
`snode.cpp:92` runs every iteration of `:90-94`; the check at `:95` runs once
afterwards on the untruncated local. Warning is a post-hoc detector. UPHELD.

**REFINEMENT, mine, from enumerating readers of each field:**
- `acc_shape` `:49` → `struct_llvm.cpp:174,176` and
  `demote_dense_struct_fors.cpp:74,76`. LLVM spine + dense pass. **NOT the SPIR-V
  sparse path.**
- `num_elements_from_root` `:41` → `snode_struct_compiler.cpp:121`, in the loop
  `:115-122`, which runs for EVERY SNode type including sparse. Destination
  `SNodeDescriptor::total_num_cells_from_root` is `size_t`
  (`snode_struct_compiler.h:28`) but every factor was already truncated.
  Accumulated at `snode.cpp:22-23` and `:83` **with no check of any kind** — no
  warning, no assert, unlike acc_shape. `shape_along_axis` `snode.cpp:174-177`
  returns it as `int`.
- `shape` `:45` → `snode.cpp:93`, `demote_dense_struct_fors.cpp:24,74`.

So on the SPIR-V sparse path the binder is `:41`, not `:49`, and `:41` is the one
with no detector. Claim right in substance, imprecise on which field.

Extra, unnamed by anyone: `demote_dense_struct_fors.cpp:19` declares
`std::array<int, taichi_max_num_indices> total_shape` and `:23-25` multiplies into
it unchecked, while `total_n` at `:18` is int64 and IS checked at `:29`.

## 36. CLAIM 7 — addr_space_. UPHELD.

`type.h:207 int addr_space_{0};  // TODO: make this an enum`. Getter `:191-193`.
`TI_IO_DEF(pointee_, addr_space_, is_bit_pointer_)` at `:203`. Constructors
`:179` and `:181-185` — **my report said `:184` and `:186-191`, wrong**, adversary
2 right. `grep -rn addr_space taichi/ c_api/ python/ tests/` → 7 hits, 4 in
`type.h`, 3 in `codegen_llvm.{cpp,h}` (the unrelated LLVM parameter,
`codegen_llvm.h:165` default 0, `codegen_llvm.cpp:1131-1133`). No call site of
`get_addr_space` outside its own body. "Zero call sites ever" overstated — the
serialiser reads it. Ruling: strike from 6.2. Four independent agreements.

## 37. CLAIM 9 — "every module portable" is false. VERIFIED.

`c_api/src/taichi_gfx_impl.cpp:22-29`: loops over ALL required caps, rejects on
`current_version != required_version` at `:25`. Read the loop.

Other device-derived, unguarded, in `vulkan_device_creator.cpp` (read `:620-673`):
`:628` int16, `:632` int64, `:636` float64, `:659` subgroup_basic, `:663`
subgroup_vote, `:667` subgroup_arithmetic, `:671` subgroup_ballot. Seven, from two
device queries at `:624` and `:654`. Counted off the lines.

So un-defaulting PSB removed one of at least eight mismatch sources. My claim was
wrong. It weakens my own §2.3 inference: explains why PSB is a hazard, not why PSB
alone. I have NOT filled the residue with a guess. Adversary 1 found this;
adversary 2 verified and concedes it missed it.

## 38. CLAIM 10 — the return-layout mismatch. VERIFIED as code.

`gfx_program.h:75-77` `get_kernel_return_data_layout()` → literal `"4-"`, no
capability read. `:79-83` argument layout DOES read it. `spirv_codegen.cpp:2490-2491`
reads it for `compile_ret_struct` and threads into `translate_ti_type` at `:2494`.
Host side takes `runtime.cpp:823` `'-'` → `:857-858`, 4 bytes. Shader would use 8.
Real asymmetry.

`git show --stat 88ac098bc` + `--name-only`: touches the spirv codegen, **does not
touch `gfx_program.h`**. Diff shows `+bool has_buffer_ptr =` and
`+translate_ti_type(blk, element.type, has_buffer_ptr)`. Confirmed.

Reachability: adversary 1 hedges on the frontend. Second hedge, mine, from C++
only — `callable.cpp:170-177` `finalize_params` SYNTHESISES pointer members via
`get_pointer_type` at `:174`; `:150-156` `finalize_rets` does not, a ret member is
`rets[i].dt` verbatim at `:155`. So a ret struct holds a PointerType only if a
declared return dtype is one. Nothing in C++ constructs that. Latent, not live.

## 39. CLAIM 11 — no other mechanism on SPIR-V. UPHELD as a qualifier.

Verified Taichi-side: root bound via `ir_->buffer_argument` at
`spirv_codegen.cpp:2308-2309` inside `get_buffer_value` `:2266-2315`;
`bitmasked_activation` binds at `:401-402`; `at_buffer` non-u64 path `:2212-2218`
shifts a u32 and hands to `struct_array_access`.
`grep -rn maxStorageBufferRange taichi/ c_api/` → **0**. Re-ran it myself.

Vulkan-side INFERRED, and I keep it labelled: bound buffer capped by
`maxStorageBufferRange`, SPIR-V offers bound-buffer-plus-offset or physical
address and nothing else. Adversary 2's `snode_struct_compiler.cpp:53` upgrades
this from spec-deduction to spec-deduction-plus-the-author-agreeing.

Report §3 qualified accordingly. The planner's reversal must carry it or it
misleads.

## 40. Side-checks on my own errors that the adversaries raised and I verified.

- `c_api/src/taichi_vulkan_impl.cpp`: comment `:45`, `/*` `:46`, `*/` `:51`. My
  `:44-52` was one wide at each end. Adversary 1 right.
- ExternalPtrStmt index arithmetic: `:737` init i32, `:770-771` mul/add,
  `:773-777` shift with i32 result, `:790` `OpSConvert` to u64 then add to the u64
  base. So the capability = 64-bit base + 32-bit signed offset. "Would widen
  ndarray addressing" overcredits it. Both adversaries right; makes my own
  conclusion stronger.
- `bitmasked_activation` re-read `:383-435` line by line:
  `:398-399` shift, Result `ptr_dt`, Base `const_i32_one_` — **real defect, I
  missed it entirely**. `:404` Result `ptr_dt`, `:405` u32 shift AMOUNT — legal,
  I wrongly cited it. `:409` `add(parent_ptr, bitmask_word_ptr)`, both `ptr_dt`
  under the flag — **my claim is wrong**. `:410-412` Result hardcoded u32, Base
  `ptr_dt` — real defect. `:413` is the assignment, `struct_array_access` is
  `:414` — off by one. And `ir_->add` asserts `a.stype.id == b.stype.id`
  (`spirv_ir_builder.cpp:1040`), so a mismatch there is a HOST assertion, not
  invalid SPIR-V — my failure mode was wrong too. Reason 2 rewritten.
- `use_64bit_pointers`: `git log --all -S` → 1 commit for the flag
  (`1b34a2d4d`, author 2021-10-25). `git log -L 2317,2324:...spirv_codegen.cpp` →
  **2**, adding `715a04c98` (2023-08-15). Diff is the cast-widen → direct u64
  immediate one-liner. My "never edited" invited the wrong reading. Corrected.
- `make_pointer` → `ir_->u64_type()` → `spirv_ir_builder.h:532-534`, bare
  `return t_uint64_;`. `t_uint64_` assigned only inside
  `if (caps_->get(cap::spirv_has_int64))` at `spirv_ir_builder.cpp:166-169`. The
  guards I cited in §7.1 (`:311-313`, `:325-327`) are inside
  `get_primitive_type(DataType)` and are NOT on this path. **My §7.1 safety-net
  claim was wrong.** Adversary 2 right. New reason 3 in §4.
- `data64`: `grep -rn data64 tests/ python/` → **53 hits, 28 files**. My "three
  Python test gates" was wrong by more than 10x. Adversary 2 right.
- C++ callers of `is_extension_supported`: `program.cpp:149` assertion,
  `codegen_llvm.cpp:2726` bls, `compile_to_offloads.cpp:92,205,218,236` mesh,
  `:245,288` quant. **`sparse` is NOT among them** — its 6 call sites are Python
  (`snode.py:50,76,93`, `fields_builder.py:83,100,122`). My list was wrong.

## 41. What I did NOT change, and why.

- Hardware measurements (§11.3) — upheld, reproduced independently by a second
  agent, untouched.
- Vulkan 1.2 promotion premise wrong (§11.2) — upheld, untouched.
- Issue 8608 as the upstream tracking issue (§5.1) — upheld, and both adversaries
  credit it as the thing pass A missed entirely. Untouched.
- Adversary 1's claim that the at_buffer collision is architectural — REJECTED,
  with the source, and I changed nothing to accommodate it beyond recording the
  rejection.
- Adversary 1's tag census — REJECTED as three short, by enumeration.
- The claim as put to me that `:49` is a sparse-path binder — REFINED, not
  accepted as given.
- Adversary 2's sizing of the at_buffer fix — REJECTED as understated, with the
  `make_value` mechanism that shows why.

Nothing decided to be unnecessary. No abstraction added. Everything unresolved is
in §16 of the report.

## 42. Both adversary files grew mid-amendment. Re-read. My claim-5 ruling reverses.

Caught this on a mtime check before finishing: `adversary-05-1.md` and
`adversary-05-2.md` were 807 and 1019 lines when I read them at the start of this
pass; they are now **1822 and 1740**, last written 09:53 and 09:55. So my
adjudication of claim 5 was against stale files. Re-read both.

**The divergence put to me has closed, and it closed against the ruling I had
written.** Adversary 2 §14.1 withdraws its classification in full. Adversary 1
§24.6 withdraws its justifying sentence but holds its ruling. Both now say
architectural.

I did not take the convergence as evidence. Two agents agreeing is not a check.
Re-derived the deciding facts:

- Everything I verified in entry 34 still holds: `ValueKind` exists
  (`spirv_ir_builder.h:69-79`), `Value::flag` at `:88`, it is READ at
  `spirv_ir_builder.cpp:1275` and `:1290`. Adversary 1's original sentence is
  false and it withdraws it. That part of my entry 34 stands.
- **What I missed.** The tag at `spirv_codegen.cpp:2203` is on `at_buffer`'s
  OUTPUT. The question is whether one is available at the INPUT, `:2197`. I
  checked the SNode side of that in entry 34 and found `make_value` erases it —
  and then filed that as a *sizing* correction to adversary 2 rather than seeing
  it as the refutation of my own ruling. **I did not trace the ndarray side.**
- Traced it now. `:789` `load_variable(addr_ptr, u64_type())` →
  `spirv_ir_builder.cpp:1274` `new_value(res_type, ValueKind::kNormal)`. `:790-791`
  `ir_->add(addr, OpSConvert(...))` → `make_value` → kNormal (u64 is
  `TypeKind::kPrimitive`, not `kPtr`, so the `:294-295` upgrade does not fire).
  **The genuine physical device address arrives at `at_buffer` tagged kNormal,
  identically to a root offset.** So `at_buffer` cannot be reading the tag on
  either side. The dtype sniff is not a shortcut past an available mechanism.
  There is no available mechanism.
- Fourth leg, adversary 2's own check, re-verified: `ptr_to_buffers_` cannot
  serve either. `:797-801` sets it OUTSIDE the capability branch `:783-795`, so
  under the capability an ndarray pointer has both a u64 address and a
  `ptr_to_buffers_` entry.
- `make_pointer` side: `uint_immediate_number` → `get_const` →
  `new_value(dtype, ValueKind::kConstant)` at `:1502`. Confirmed.
- Remedy size: `grep -rn "make_value(" taichi/codegen/spirv/` → 148 occurrences,
  147 call sites. That is the single SSA constructor for the whole backend. Not
  three functions.

**REVERSED. Ruling is now for adversary 1: the collision is architectural.**
Rewrote report §13.5 wholesale, restored Part II row 7 to the architectural
column with a note that it is LARGER than the report originally described,
rewrote §15 items 1 and 4, and replaced escalation §16.5.

I am recording the reversal rather than quietly rewriting, because the error is
instructive: I had the deciding fact (`make_value` erasure) in hand and drew the
wrong conclusion from it by tracing only one of the two paths through the
decision point. Plan §10.6 is about exactly this — the citation was right, the
sentence it supported was not tested.

Standing correction to BOTH adversaries that neither has fixed: they both still
cite `:2204` for the `kPhysicalPtr` assignment. It is `:2203`. `:2204` is the
`return`.

## 43. Cross-checked the rest of the grown material against my other rulings.

Read adversary 1 §§24.1-24.11 and adversary 2 §14. Everywhere else they now
agree with what I had already concluded independently, so no other ruling moves:

- a1 §24.1 concedes live producers. Matches my entry 30. a1 also says this
  "should reach the project owner: the capability is un-defaulted, not disabled",
  which is the framing I put in report §1.1.
- a1 §24.2 concedes `snode_struct_compiler.cpp:52-54` and agrees it strengthens
  its own §2.4 bound-buffer argument. Matches my entries 32 and 39.
- a1 §24.3 concedes the tag census, giving 24 / 11 / 3 / 10. Matches my entry 33
  exactly. It does not restate a total tag count; mine is 121 by measurement.
- a1 §24.4 concedes the `u64_type()` third failure mode. Matches my entry 40.
- a1 §24.10 adds "B17" against my report — `spirv_types.cpp:433-439` listed as a
  live consumer, unreachable. I had already removed it in entry 31 from the
  rebuild, before reading this.
- a2 §14.1 keeps its §2a (addressing model not architectural) and its third
  failure mode. Both match my entries 33 and 40.
- a1 §24.7 and a2 both recommend adopting "one route with a type-sniff switch"
  over "two disjoint routes". Adopted at the end of report §13.5.

Nothing in the new material touches claims 1, 2, 3, 4, 6, 7, 8, 9, 10 or 11 in a
way that changes my adjudication. The three upheld items I was told not to
disturb — the hardware measurements, the Vulkan 1.2 premise, issue 8608 — are
untouched, and a2 §14.2 reports an independent `vulkaninfo` run agreeing with
mine to the cell.

Files still growing while I write. Recording the versions I adjudicated against:
`adversary-05-1.md` at 1822 lines, `adversary-05-2.md` at 1740 lines.
