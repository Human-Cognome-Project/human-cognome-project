# Notes 05 (pass A) — Why upstream shelved the partial 64-bit work

Contemporaneous. Appended as I go. Repo at `ba0e81dce` (fork baseline).
Sources marked **[LOCAL]** (this git repo) or **[GH]** (github.com/taichi-dev/taichi via `gh`).

---

## 1. Confirming the four starting points [LOCAL]

- `taichi/rhi/vulkan/vulkan_device_creator.cpp:820-827` — the capability producer,
  dead code. Verbatim:

```
        if (device_supported_features.shaderInt64) {
// Temporarily disable it on macOS:
// https://github.com/taichi-dev/taichi/issues/6295
// (penguinliong) Temporarily disabled (until device capability is ready).
#if !defined(__APPLE__) && false
          caps.set(DeviceCapability::spirv_has_physical_storage_buffer, true);
#endif
        }
```

  Two separate disabling acts stacked on top of each other: a macOS-only
  exclusion citing issue 6295, and a later blanket `&& false` attributed to
  penguinliong citing "until device capability is ready". The `&& false` makes
  the `!defined(__APPLE__)` clause moot — the whole guard is unconditionally
  false on every platform.
- `taichi/codegen/spirv/spirv_codegen.cpp:82` — `const bool use_64bit_pointers = false;`
  a non-static const member of `TaskCodegen`, never assigned elsewhere.
- `taichi/ir/snode.cpp:95-100` — int32 truncation in extractor `acc_shape`,
  with `TaichiIndexWarning` text "int64 indexing is not supported yet. Struct
  fors might not work either."
- `taichi/ir/type.h` — `PointerType` `addr_space_` (to confirm below).

Next: git archaeology on each.

---

## 2. Timeline of the physical-storage-buffer capability [LOCAL]

`git log -S 'spirv_has_physical_storage_buffer' --all` plus
`git log -L 815,830:taichi/rhi/vulkan/vulkan_device_creator.cpp`.

Commits touching the producer line, oldest first:

| Commit | PR | Date | Author | Effect |
|---|---|---|---|---|
| `7705f688a` | #4221 | 2022-02-08 | Bob Cao | **Introduces** buffer device address / physical pointers. 14 files, +592/-219. |
| `06cb1c50d` | #4244 | 2022-02 | — | Gate on `shaderInt64`. |
| `07446dd0c` | #6184 | 2022-09/10 | — | Device capability refactorisation; moved the set call. |
| `f677fb141` | #6415 | 2022-10-24 | Yi Xu | **Disable on macOS**, `#if !defined(__APPLE__)`, citing issue 6295 comment 1288522905. |
| `d20f55dc8` | #6468 | 2022-10-30 | PENGUINLIONG | **Disable everywhere**: appends `&& false` plus the comment "(penguinliong) Temporarily disabled (until device capability is ready)." Commit message is the title only, no body. |
| `12546578a` | #6494 | 2022-11-02 | PENGUINLIONG | **Disables the second producer**, in `c_api/src/taichi_vulkan_impl.cpp`, by commenting out the block. Message: "The physical storage buffer codegen was disabled but the runtime can still use them. This is an attempt to disable psb at runtime. Will bring it back after devcap." |

Key reading of the pair #6468 / #6494: the codegen-side capability was switched
off first, which left the runtime still handing out physical addresses; #6494
closed that gap. Both by the same author, three days apart. Both say
"temporarily" / "will bring it back", and both name the same blocker: the
in-flight device-capability ("devcap") rework. Nothing in either message says
the feature was found broken or wrong.

Note: the second producer is NOT the one described in my brief as compiled out
by preprocessor. It is at `c_api/src/taichi_vulkan_impl.cpp:45-51`, commented
out with `/* ... */`, not `#if`. Same net effect.

## 3. Origin of `use_64bit_pointers` [LOCAL]

`git log -S 'use_64bit_pointers' --all` returns exactly ONE commit:
`1b34a2d4d` "[vulkan] Indexed load codegen (#3259)", Bob Cao, 2021-10-25 —
three and a half months BEFORE physical storage buffers existed (#4221).

It was born as `const bool use_64bit_pointers = false;` in the same commit that
created `make_pointer()`. Verbatim from that diff:

```
+  spirv::Value make_pointer(size_t offset) {
+    if (use_64bit_pointers) {
+      // This is hacky, should check out how to encode uint64 values in spirv
+      return ir_->cast(ir_->u64_type(), ir_->uint_immediate_number(
+                                            ir_->u32_type(), uint32_t(offset)));
+    } else {
+      return ir_->uint_immediate_number(ir_->u32_type(), uint32_t(offset));
+    }
+  }
```

**This is important: `use_64bit_pointers` was NEVER true and was never
"switched off".** It has been a hardcoded `false` for its entire life. There is
no disabling commit because there was no enabling one. It is dead scaffolding
left by the author as a marker, admitted as "hacky" and incomplete in the same
line it was written.

Only two references exist in the tree today
(`grep -rn use_64bit_pointers taichi/`):
`spirv_codegen.cpp:82` (the definition) and `spirv_codegen.cpp:2318` (the sole
consumer, `make_pointer`). Current form of the true branch has been cleaned up
slightly — it now emits a genuine u64 immediate rather than a u32-then-cast:

```
  spirv::Value make_pointer(size_t offset) {
    if (use_64bit_pointers) {
      // This is hacky, should check out how to encode uint64 values in spirv
      return ir_->uint_immediate_number(ir_->u64_type(), offset);
    } else {
      return ir_->uint_immediate_number(ir_->u32_type(), uint32_t(offset));
    }
  }
```

So `use_64bit_pointers` and `spirv_has_physical_storage_buffer` are **two
separate things with separate histories**, not one feature. `make_pointer` is
about the width of a root-buffer *offset* constant; the capability is about
whether SPIR-V may hold real device addresses. `make_pointer` does not consult
the capability at all.

## 4. Why the macOS disable happened — root cause [GH]

Issue 6295 "The method from_numpy() not working on Mac when using Vulkan
backend", opened 2022-10-11 by Linyou, closed 2022-10-29.

- Comment 1285502939 (strongoier / Yi Xu, 2022-10-20): bisected to MoltenVK
  version, not to a Taichi commit. Wrong results with MoltenVK >= 1.1.11 on
  macOS >= 12.5, correct with 1.1.10.
- Comment 1286332889 (bobcao3): speculated relaxed synchronisation.
- Comment 1286478458 (strongoier): `ti.sync()` does not help; suspects an
  address alignment issue. Minimal repro is a one-element ndarray read
  returning 0 instead of 6.
- Comment 1288522905 (strongoier, 2022-10-24) — **the comment cited in the
  source**: "After bisecting MoltenVK commits, I found that the problem was
  introduced in https://github.com/KhronosGroup/MoltenVK/pull/1638, which
  enabled support for `VK_KHR_buffer_device_address` and
  `VK_EXT_buffer_device_address` on macOS 12.5. To make a healthy release, I'll
  submit a PR to stop using the feature on macOS. **We can diagnose further
  about whether the problem comes from the feature itself or the use of the
  feature in Taichi in the future.**"

So the macOS disable is category "found broken" — silently wrong data — but the
root cause was **never determined**. Upstream explicitly left open whether the
fault was MoltenVK's or Taichi's own use of buffer device address. No follow-up
diagnosis appears in the issue; it was closed by the workaround PR #6415.

## 5. Why the blanket disable happened [GH + LOCAL]

PR #6468 body, verbatim [GH]:

> Disabled for now because of multi-platform compatibility issues. Will be
> recovered when device capability is fully functional.

PR #6494 body, verbatim [GH]:

> The physical storage buffer codegen was disabled but the runtime can still
> use them. This is an attempt to disable psb at runtime. Will bring it back
> after devcap.

Neither PR has any review discussion. #6468 has one comment, from the Netlify
bot; #6494 likewise. No maintainer objected, no maintainer asked why. That is
itself evidence: this was not a debated architectural retreat, it was a
housekeeping switch flipped during the AOT device-capability rework.

Two distinct stated reasons, and they are NOT the same as the macOS one:
1. "multi-platform compatibility issues" — plural, unspecified. macOS is the
   only one I can evidence.
2. Blocked on "devcap", the device-capability mechanism, which at that moment
   was mid-refactor (`07446dd0c` #6184 landed Sept/Oct 2022; C-API device
   capability work followed in #6702, reverted in #6772, re-landed in #6773).

The pairing matters. The capability system is how an AOT module declares "I need
physical storage buffers" so a consuming runtime can refuse or accept it. Until
that negotiation worked, shipping shaders that assume 64-bit device addresses
made AOT modules silently non-portable. Turning the capability off is the
conservative move while the negotiation layer is rebuilt.

## 6. Release impact [LOCAL]

`git merge-base --is-ancestor d20f55dc8 <tag>`:

| Tag | Physical storage buffer |
|---|---|
| v1.2.0, v1.2.1, v1.2.2 | live (off on macOS only) |
| v1.3.0 onward, incl. v1.7.0 | dead everywhere |

So the feature shipped enabled in real releases for roughly eight months
(#4221 merged 2022-02-09, disabled 2022-10-30). It is not vapourware — it
worked well enough to ship on Windows and Linux across v1.0-v1.2.

## 7. Was it ever re-enabled anywhere? [LOCAL]

- `git log --all -S 'use_64bit_pointers = true'` → **empty**. It has never been
  true on any ref, ever.
- Scanned every remote branch (`git branch -r`, 230+ refs) for a
  `vulkan_device_creator.cpp` that sets the capability without the `&& false`
  guard, filtered to branches whose tip postdates 2022-11-02. Only two hits,
  `upstream/bump/v1.2.3` and `upstream/rc-v1.2.0`, and both are v1.2-line
  release branches carrying the older pre-disable code (verified: they hold the
  `#if !defined(__APPLE__)` form). **No branch, anywhere in upstream, re-enables
  it.** No abandoned re-enablement work exists.

## 8. `addr_space_` is not part of this at all [LOCAL]

`taichi/ir/type.h:207`, `int addr_space_{0};  // TODO: make this an enum`.

- Introduced `dcd5d7d35` "[type] Add basic implementations of VectorType and
  PointerType (#1948)", Yuanming Hu, 2020-10-12 — **sixteen months before** the
  physical-storage-buffer work, and a year before `use_64bit_pointers`.
- Verified dead: `grep -rn get_addr_space /opt/project/taichi/` returns exactly
  one hit, the accessor's own definition at `type.h:191`. Neither constructor
  (`type.h:179`, `type.h:181`) takes or sets it. The only other reference is
  the serialisation macro `TI_IO_DEF(pointee_, addr_space_, is_bit_pointer_)`
  at `type.h:203`.
- It has been `{0}` and unread for its entire five-year life. Never enabled,
  never disabled, no shelving event.
- **It is about address SPACE, not address WIDTH.** The LLVM-side analogue is
  `codegen_llvm.cpp:1131`'s `int addr_space` parameter feeding
  `llvm::PointerType::get(..., addr_space)`, which selects generic/global/shared
  in the LLVM sense. Reading it as 64-bit-addressing machinery would be an
  error. Flagging for the planner: the brief lists it under the 64-bit heading;
  the evidence does not support that classification.

## 9. The int32 index ceiling — a separate, older, still-live story [LOCAL + GH]

Not a shelved 64-bit feature; a deliberate, documented, still-current design
limit.

- `cfc4065d3` "[bug] Fix silent int overflow in indice calculation. (#3177)",
  ailzhang, 2021-10-21. Widened `acc_shape` to `int64` but kept
  `extractors[i].acc_shape` as int32 with an explicit
  `static_cast<int>(acc_shape)` and the comment "casting to int32 in
  extractors." Added the warning.
- PR #3177 body [GH]: "Btw I didn't fix all the implicit conversions from in64
  to int32 (but tried to make the conversions more explicit). IIUC we don't
  support int64 indexing yet so it should be good enough to do the explicit
  conversion for now and revisit this later."
- Reviewer k-ye [GH] proposed the opposite fix — enforce the value fits in int
  rather than widen — arguing "a shape not fit in `int` can hardly fit in
  memory". ailzhang answered: "I think it's still better to expand this to
  64-bits. It is less limiting, and is consistent with `num_cells_per_container`."
  **The widening direction was argued for and won, then not carried through.**
  That is the clearest single piece of evidence on intent: 64-bit indexing was
  agreed to be the right direction in Oct 2021 and simply never done.
- `b347a2159` "[bug] Fix false overflow alarm in struct fors under packed mode
  (#6457)", Yi Xu, 2022-10-28, appended "Struct fors might not work either." to
  the warning and, in the same commit, **replaced** the old
  `TI_ASSERT(total_bits <= 30)` in
  `taichi/transforms/demote_dense_struct_fors.cpp` with
  `TI_ASSERT(total_n <= std::numeric_limits<int>::max())`, now at line 29.
  So struct-for over a dense tree is HARD CAPPED at 2^31-1 total cells by an
  assertion, not a warning.
- `96e795770` "[Lang] Warn users if ndarray size is out of int32 boundary
  (#6846)", Yi Xu, 2022-12-13, replicated the same warning for ndarrays in
  `taichi/program/ndarray.cpp`.
- Issue #6758 [GH]: user hit SIGSEGV on a 48000x48000 f64 ndarray. Maintainer
  strongoier, 2022-11-30: "Taichi currently uses i32 for indexing so this
  behavior is known." Resolution was to add a warning, not to fix the width.

## 10. What the capability actually buys, read from the code [LOCAL]

This is the finding that reframes the question. Traced all twelve live consumers.

**It is about ARGUMENT and NDARRAY pointers, not about the SNode root buffer.**

- Origin issue for #4221 is #3807 "[RFC] Support ndarray for vulkan backend"
  [GH]. PR #4221's own body: "Codegen is modified so that ext arrays and
  ndarrays can be addressed directly instead of using bindings."
  The motive was descriptor-binding pressure and aliasing, **not** address
  range.
- `spirv_codegen.cpp:783-795`: when the cap is on, an array argument's address
  is loaded as a `u64` out of the args buffer and the linear offset added to it.
  When off, the linear offset alone is the value and a descriptor binding is
  used.
- `spirv_codegen.cpp:2340`, `:2416`, `:2491`: `has_buffer_ptr` is threaded into
  `translate_ti_type` for the args struct, argpack struct and ret struct — it
  changes the SPIR-V type of an ndarray field between a u64 pointer and an
  index.
- `taichi/runtime/gfx/runtime.cpp:96-102`: host side writes
  `device_->get_memory_physical_pointer(...)` into the args buffer, only when
  the cap is on.
- `taichi/rhi/vulkan/vulkan_device.cpp:1772` and `:1792`: adds
  `VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR` and calls
  `vkGetBufferDeviceAddressKHR` for every allocation, only when the cap is on.
- `taichi/runtime/program_impls/gfx/gfx_program.h:81-83`: the **kernel argument
  data layout string** is `"1b"` with the cap and `"1-"` without.

That last one is the crux for AOT: **the host/device argument ABI differs
depending on the capability**. An AOT module compiled with physical storage
buffers cannot be consumed by a runtime that lacks them, and vice versa, and in
Oct 2022 the mechanism to record and check that requirement — devcap — was
mid-rebuild. This corroborates PR #6468's "multi-platform compatibility issues"
directly from the code, without needing to take the PR body on trust.

## 11. Enabling the capability is a whole-module switch [LOCAL]

`taichi/codegen/spirv/spirv_ir_builder.cpp:73-77` and `:113-128`:

- adds `OpCapability PhysicalStorageBufferAddresses`
- adds `OpExtension SPV_KHR_physical_storage_buffer`
- and, critically, changes the module's `OpMemoryModel` from
  `AddressingModelLogical` to `AddressingModelPhysicalStorageBuffer64`.

The addressing model is a single module-wide declaration. There is no
per-pointer opt-in. So the capability is all-or-nothing for a compiled shader,
which is why a driver bug anywhere in the physical-address path (the MoltenVK
case) takes out every kernel rather than one feature.

## 12. Why `use_64bit_pointers = true` cannot simply be flipped [LOCAL]

`make_pointer` is called at four sites, all on the SNode root path:
`spirv_codegen.cpp:355` (`GetRootStmt`), `:371` (`GetChStmt`), `:408`
(`bitmasked_activation`), `:506` (`SNodeLookupStmt`).

Two concrete, verifiable breakages if the flag became true:

(a) **Semantic collision with `at_buffer`.** `at_buffer`
(`spirv_codegen.cpp`, the `spirv::Value at_buffer(const Stmt *ptr, DataType dt)`
overload) branches on `ptr_val.stype.dt == PrimitiveType::u64` and, when it
matches, emits `OpConvertUToPtr` to a `StorageClassPhysicalStorageBuffer`
pointer — i.e. it treats a u64 as an **absolute device address**. But
`make_pointer` produces a **root-buffer-relative byte offset**, starting at
literal `0` for the root (`:355`). Flipping the flag would therefore make every
SNode access dereference address `0 + offset`. The two u64 meanings in this file
are incompatible.

(b) **Type mixing in `bitmasked_activation`** (`:383-415`). `ptr_dt` is taken
from `parent_ptr.stype` at `:388`, so it would become u64. Re-read with exact
line numbers; two genuine SPIR-V rule violations, not three:
  - `:398-399` `OpShiftLeftLogical` with Result type `ptr_dt` (u64) but Base
    `ir_->const_i32_one_` (i32). Base and Result must share a type.
  - `:410-412` `OpShiftRightLogical` with Result type hardcoded `ir_->u32_type()`
    but Base `bitmask_word_ptr` is `ptr_dt`-typed. Same rule.
  - `:414` `struct_array_access(ir_->u32_type(), buffer, bitmask_word_ptr)`
    indexes with that value.
  Correcting my own earlier note: `:405`'s `u32_type()` is the shift AMOUNT,
  which SPIR-V permits to differ in width from Base. Not a defect.

By contrast `SNodeLookupStmt` at `:503-508` **would** survive: it casts the
index to `parent_val.stype` before multiplying.

So the true branch is not "written but untested". It is written against a
different meaning of u64 than the rest of the file settled on, and parts of the
root path were never converted at all. Consistent with it having been a
placeholder from day one (see note 3).

## 13. No design document exists [LOCAL]

- `grep -rniE '64[- ]?bit|int64|i64|u64' docs/**/*.md` — nothing about
  addressing width as a design topic. Hits are the type-system reference,
  `docs/design/llvm_sparse_runtime.md` (which documents 64-bit *locks and cell
  pointers in the LLVM sparse runtime*, an unrelated and already-64-bit thing),
  and `docs/lang/articles/basic/sparse.md`.
- `grep -rniE '(TODO|FIXME|XXX|HACK).{0,120}(64[- ]?bit|int64|i64|u64|physical (storage|pointer)|buffer device address|addr_space|address space)' taichi/`
  returns exactly two hits in the whole tree:
  `taichi/codegen/llvm/codegen_llvm_quant.cpp:75` (unrelated, atomicCAS widths)
  and `taichi/codegen/spirv/spirv_codegen.cpp:2319` (the "This is hacky" line).
- There is no RFC, no design note, no tracking issue for 64-bit addressing.
  Searched GitHub for "physical storage buffer" and "buffer device address"
  across issues [GH]: three hits total, none of them a re-enablement or design
  thread. **The absence of a plan is itself the finding.**

## 14. The stated precondition, "devcap", is a still-open tracker [GH]

Issue **#5979 "[AOT] Device capability management for AOT (Tracker)"**, opened
2022-09-05 by PENGUINLIONG, **still OPEN** as of today. Body: "This issue tracks
the implementation of the device capability management mechanism to allow the
users to compile AOT modules based on their intended deployment environment."
Its single comment (PENGUINLIONG, 2022-09-05) says #5976 temporarily enabled
Vulkan extensions "as fully as possible to tolerate extension usage. This hack
should be removed as soon as the mechanism is ready."

So the thing #6468 and #6494 said they were waiting for has one comment, from
its own author, and was never closed.

**But** the mechanism is at least partly there today [LOCAL]:
`TI_CAPABILITY_SPIRV_HAS_PHYSICAL_STORAGE_BUFFER = 18` is in the public C header
`c_api/include/taichi/taichi_core.h:399`, with a setter at
`c_api/include/taichi/cpp/taichi.hpp:1156-1159`, and the capability is enumerated
at `taichi/inc/rhi_constants.inc.h:28`. So a caller *can* express the
requirement. The producers are still switched off anyway.

## 15. The concrete "multi-platform compatibility issue", found [GH]

Issue **#6368 "Vulkan `create_vma_allocator()` needs to respect the device
capabilities"**, opened 2022-10-18 by PENGUINLIONG, **still OPEN**. Body:

> For the above snippet to run, the `VkDevice device_` has to enable all the
> extensions exactly as Taichi requires. Otherwise the following
> `vmaCreateAllocator()` will crash. However, this is not possible when Taichi
> Vulkan runtime accepts an external VkDevice. I think we need a mechanism to
> specify 1) which extensions are enabled, and 2) pick the set of VK function
> handlers according to 1.

k-ye's comment on it shows the symptom class: `VUID-VkShaderModuleCreateInfo-pCode-01091`
validation errors — "The SPIR-V Capability (X) was declared, but none of the
requirements were met to use it."

This lines up exactly with physical storage buffer:
- `taichi/rhi/vulkan/vulkan_device.cpp:2509` sets
  `VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT` on the cap.
- `taichi/codegen/spirv/spirv_ir_builder.cpp:113-117` declares
  `OpCapability PhysicalStorageBufferAddresses`, which is exactly the kind of
  declaration VUID-01091 rejects when the device did not enable
  `VK_KHR_buffer_device_address`.
- The **imported-device** path is `c_api/src/taichi_vulkan_impl.cpp` — the same
  file where the second producer was commented out in #6494.

Timing: #6368 opened 2022-10-18, twelve days before #6468 disabled the cap.
That is, to my reading, the "multi-platform compatibility issues" the PR body
refers to, and it is a *host integration* problem, not a GPU-hardware one.
Marking this as strong but circumstantial: no upstream text names #6368 as the
reason for #6468.

## 16. The dead branch has been maintained, not rotted [LOCAL]

`git log --since=2023-01-01 -S 'has_buffer_ptr' master`:
`7809ebbd5`, `14e83c484`, `cfad91fc8` (2023-07-10), `29cfb5c72`, `b66279b0b`,
`32518fc52`, `88ac098bc` (#8061, 2023-05-23), `748abdbcc`, `457ada6a6`,
`4c33922df`.

Contributors kept extending the `has_buffer_ptr == true` code through the
argpack and struct-argument work of 2023-2024. Nobody deleted it, nobody
declared it wrong.

Two readings, and I cannot separate them from the evidence:
(a) it was still regarded as a feature to restore; or
(b) it was simply mechanically carried along by refactors that touched
    `translate_ti_type`.
Either way there is **no CI coverage**: `grep -rn physical_storage_buffer tests/
c_api/tests/` returns nothing, so every line added to that branch since
2022-10-30 has never been executed. Flagging as a risk, not a conclusion.

Also verified [LOCAL]: only the Vulkan RHI ever sets this capability. Metal
(`metal_device.mm:1045-1067`), DX11 (`dx_device.cpp:564`) and OpenGL
(`opengl_device.cpp:511-516`) do not set it at all. So it is Vulkan-only even in
principle.

---

# Extension to brief (received mid-work): does each reason still hold today?

Sort each into ARCHITECTURAL (does not expire) or ENVIRONMENTAL (may have
expired). Same [LOCAL]/[GH] discipline, plus a new tag **[MEASURED]** for
things I ran on this box.

## 17. The Vulkan 1.2 promotion premise does not apply — VERIFIED [LOCAL]

The brief suggests the promotion of `VK_KHR_buffer_device_address` into core
Vulkan 1.2 may have changed the availability picture since the shelving. It did
not, because **the code already accounted for it before the shelving**.

`git show 7705f688a -- taichi/backends/vulkan/vulkan_device_creator.cpp`, the
original February 2022 introduction, verbatim:

```
+    // Buffer Device Address
+    if (CHECK_VERSION(1, 2) ||
+        CHECK_EXTENSION(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) ||
+        CHECK_EXTENSION(VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)) {
```

Vulkan 1.2 shipped January 2020, two years before #4221 and nearly three before
the shelving. `CHECK_VERSION(1, 2)` was in the very first version of this code.
So core promotion is not a change that post-dates the shelving and cannot have
expired anything.

One day later, `06cb1c50d` (#4244) narrowed and strengthened the gate: dropped
the `VK_EXT_` alternative, added the `shaderInt64` requirement, and added a
`CHECK_VERSION(1, 3) ||` short-circuit ahead of the feature-bit test. Current
form at `taichi/rhi/vulkan/vulkan_device_creator.cpp:813-821`.

## 18. The gate passes on BOTH target GPUs today — [MEASURED]

Ran `vulkaninfo` on this box (read-only query; nothing modified). Full dump kept
at the session scratchpad. `nvidia-smi` reports driver 535.309.01.

| Device | apiVersion | `shaderInt64` | `bufferDeviceAddress` (Vulkan12Features) | `VK_KHR_buffer_device_address` |
|---|---|---|---|---|
| GTX 1070 (Pascal, plan tier "Mid") | 1.3.242 | true | true | present |
| GTX 750 Ti (Maxwell, plan tier "Baseline") | 1.3.242 | true | true | present |
| llvmpipe / lavapipe (Mesa, LLVM 20.1.2, CPU) | — | true | true | present |

Line references into the dump: GTX 1070 `shaderInt64` at 1082,
`VkPhysicalDeviceVulkan12Features` block at 1376 with `bufferDeviceAddress` at
1416, `VK_KHR_buffer_device_address` at 714. GTX 750 Ti: 2333, 2596, 2636, 2008.
llvmpipe: 3485, 3839, 3879, 3321.

Now trace Taichi's gate against these numbers [LOCAL]:
- `vulkan_device_creator.cpp:523` takes `vk_api_version` from
  `physical_device_properties.apiVersion` → 1.3.242 here.
- `:813-814` `CHECK_VERSION(1, 2) || CHECK_EXTENSION(...)` → **true**.
- `:819-820` `CHECK_VERSION(1, 3) || buffer_device_address_feature.bufferDeviceAddress`
  → **true** (both terms).
- `:821` `device_supported_features.shaderInt64` → **true**.
- `:824` `#if !defined(__APPLE__) && false` → **false**. This is the only thing
  stopping it.

Also `:304` and `taichi/rhi/vulkan/vulkan_utils.h:26-28`: Taichi requests
`VK_API_VERSION_1_3` at instance creation, so nothing on the Taichi side caps
the negotiated version below 1.2.

**Conclusion, narrow:** on this box, on both hardware tiers named in plan
section 5.2, every runtime precondition the code tests is already satisfied.
The hardware/driver availability blocker, whatever it was in 2022, does not
exist here now. ENVIRONMENTAL, and expired for this hardware set.

I am NOT claiming the feature works, only that the gate passes. See note 21.

## 19. Submodule pins — the fork is stuck at the shelving vintage [LOCAL]

`git submodule status` plus `git -C external/<m> log -1`:

| Submodule | Pin | Date |
|---|---|---|
| Vulkan-Headers | `409c16b` | 2025-04-18 (Vulkan-Docs 1.4.313) |
| VulkanMemoryAllocator | `539c0a8` | 2025-04-14 |
| volk | `b87f882` | 2023-03-24 (Vulkan 1.3.245) |
| SPIRV-Reflect | `7c9c841` | 2023-02-01 |
| **SPIRV-Headers** | `34d0464` | **2022-12-15** |
| **SPIRV-Cross** | `c77b09b` | **2022-11-21** |
| **SPIRV-Tools** | `46ca66e` | **2022-11-18** |

This is exactly the distinction the extended brief asked for. The Vulkan API
surface we compile against is current (2025). **The SPIR-V toolchain is frozen
within three weeks of the shelving commit** `d20f55dc8` (2022-10-30). Anything
fixed in SPIRV-Tools or SPIRV-Headers since November 2022 is fixed upstream and
**not fixed in what we have**.

## 20. But SPIRV-Cross is irrelevant to this question — VERIFIED [LOCAL]

`grep -rln 'spirv_cross\|spirv_glsl\|spirv_msl' taichi/` returns only
`taichi/rhi/metal/`, `taichi/rhi/dx/` and `taichi/rhi/opengl/` files. SPIRV-Cross
is the SPIR-V-to-MSL/HLSL/GLSL translator for those three backends, and none of
them sets `spirv_has_physical_storage_buffer` (note 16). The Vulkan path
consumes SPIR-V directly.

So of the three stale pins, SPIRV-Cross does not bear on physical storage
buffers at all. SPIRV-Headers and SPIRV-Tools do.

## 21. What our pinned SPIRV-Tools does and does not tell us [LOCAL]

Taichi runs the optimizer over every generated module and, notably, **never
validates**:
- `spirv_codegen.cpp:2666-2680` picks `spv_target_env` from the `spirv_version`
  cap. On our devices `vulkan_device_creator.cpp:525-527` sets that cap to
  `0x10500` (SPIR-V 1.5), giving `SPV_ENV_VULKAN_1_2`.
- `:2681` constructs `spvtools::Optimizer`, `:2685-2707` registers the pass list.
- `:2709` `spirv_opt_options_.set_run_validator(false);`
- `grep -n 'Validate' spirv_codegen.cpp` → **no hits**. The only other
  SpirvTools use is `Disassemble` at `:2763`, inside an `if constexpr (false)`.

So nothing in the pipeline would catch a malformed physical-storage-buffer
module before it reaches the driver. That is consistent with the 2022 macOS
symptom being silently wrong results rather than an error.

The pinned SPIRV-Tools is not *blind* to the feature: `grep -r
PhysicalStorageBuffer external/SPIRV-Tools/source/` gives 64 hits across
`val/validate_capability.cpp:158`, `val/validate_conversion.cpp:267-274`,
`val/validate_memory.cpp`, `val/validate_atomics.cpp`, and an
`opt/inst_buff_addr_check_pass.cpp`. SPIR-V 1.5 made
`SPV_KHR_physical_storage_buffer` core, and our target env is Vulkan 1.2 /
SPIR-V 1.5, so the vintage is at least nominally in range.

**What I cannot determine:** whether the twenty-odd optimizer passes Taichi
registers handle `AddressingModelPhysicalStorageBuffer64` modules correctly at
this 2022-11-18 pin. Grepping for the symbol proves awareness, not correctness.
Determining it requires building and running. Escalating.

## 22. MoltenVK today — mostly but not fully recovered [GH]

Relevant only to macOS, which is not in plan section 5.2's target tiers, but the
brief asks about driver maturity across vendors.

- MoltenVK issue **2174** "regression bug with vkGetBufferDeviceAddress", opened
  2024-03-07, **still open**. Reporter could not get buffer device address
  working on MoltenVK 1.2.0 through 1.2.7; `vkGetBufferDeviceAddress` returned 0
  on 1.2.0-1.2.6.
- Maintainer **billhollings**, 2024-03-12, on that issue: "`VK_KHR_buffer_device_address`
  and `VK_EXT_buffer_device_address` are supported in the latest MoltenVK, and
  are currently passing about **90% of the CTS tests**."
- Further open reports touching device addresses: **2696** (2026-03-06,
  `VK_ERROR_OUT_OF_DEVICE_MEMORY` with device address buffers), **2278**
  (2024-07-19, GPU address fault with descriptor indexing).

Reading: the specific 2022 breakage is not the live issue any more, but MoltenVK
buffer device address was still short of full conformance as recently as 2024,
by its own maintainer's account. ENVIRONMENTAL, substantially expired, not
cleanly expired. I found no evidence anyone ever established whether the 2022
fault was MoltenVK's or Taichi's, so I cannot say the *Taichi-side* possibility
was ever eliminated.

## 23. Issue #6368's complaint is STILL unaddressed in our tree — VERIFIED [LOCAL]

The imported-device blocker I identified in note 15. Checked whether the C-API
gained the mechanism #6368 asked for ("we need a mechanism to specify 1) which
extensions are enabled").

`c_api/include/taichi/taichi_vulkan.h:26-47`, `TiVulkanRuntimeInteropInfo`,
carries: `get_instance_proc_addr`, `api_version`, `instance`,
`physical_device`, `device`, `compute_queue`, `compute_queue_family_index`,
`graphics_queue`, `graphics_queue_family_index`.

**No field for enabled extensions or enabled device features.** The caller still
cannot tell Taichi what its imported `VkDevice` turned on. Blocker unexpired on
that path.

## 24. But the OWNED device path never had that problem — VERIFIED [LOCAL]

Critical scoping distinction I nearly missed.

`vulkan_device_creator.cpp:595-596`: `VK_KHR_buffer_device_address` is pushed
into `enabled_extensions` whenever the physical device advertises it —
**unconditionally, outside any `#if`**. And `:828-829`,
`*pNextEnd = &buffer_device_address_feature; pNextEnd = &buffer_device_address_feature.pNext;`
chains the feature struct into `VkDeviceCreateInfo` — **also outside the
`#if !defined(__APPLE__) && false` block**, which encloses only the
`caps.set(...)` line at `:826`.

So on the owned path, today, on this box: Taichi already creates a logical
device with `VK_KHR_buffer_device_address` enabled and `bufferDeviceAddress`
requested. The `&& false` suppresses **only the declaration of the SPIR-V
capability**, not the device plumbing.

Consequence: when Taichi creates its own device it knows exactly what it
enabled, so the #6368 class of failure (declaring a SPIR-V capability the device
did not enable) cannot arise there. That failure mode is confined to the C-API
imported-device path.

`VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT` at
`taichi/rhi/vulkan/vulkan_device.cpp:2509` IS gated on the capability, so it
would follow the flag automatically rather than needing separate work.

---

# SECOND ADVERSARIAL ROUND — amendment pass, 2026-09-09

Entries 25 to 35. One per claim in the amendment brief. Each records what I
checked, the command or file read, and the verdict. Every claim was treated as a
claim and re-derived; nothing was accepted from either adversary file. Repo at
`ba0e81dce`, read-only throughout. No source file touched, no git state changed,
no writes to GitHub.

## 25. CLAIM 1 — the feature is un-defaulted, not off. UPHELD. [LOCAL]

The biggest one, and it reverses my own framing.

Traced the ahead-of-time chain end to end by reading each file, not by grepping
for reassurance:

1. `Program::make_aot_module_builder(Arch, const std::vector<std::string> &)` is
   declared at `taichi/program/program.h:224`, defined at
   `taichi/program/program.cpp:541-557`. **It is a C++ API**, not only a Python
   one. `grep -rn make_aot_module_builder` shows C++ callers at
   `cpp_examples/aot_save.cpp:20` and
   `tests/cpp/aot/dx12/aot_save_load_test.cpp:30`, plus the Python binding at
   `taichi/python/export_lang.cpp:403` and the Python caller at
   `python/taichi/aot/module.py:110`. This matters because plan section 1.2 puts
   the Python frontend out of scope; the C++ interface is in scope and carries
   the same parameter.
2. `translate_devcaps`, `taichi/program/program.cpp:514-539`. Read in full.
   `DeviceCapabilityConfig cfg{}` at `:517` starts **empty**. The loop calls
   `str2devcap(key)` at `:530` and `cfg.set(devcap, value)` at `:531`. The only
   thing it adds unbidden is `spirv_version = 0x10300` at `:535-537`. **No merge
   with any device, no comparison against one.**
3. `str2devcap`, `taichi/rhi/device_capability.cpp:6-13`. Generated by
   `#include "taichi/inc/rhi_constants.inc.h"` over `PER_DEVICE_CAPABILITY`, so
   `"spirv_has_physical_storage_buffer"` (`rhi_constants.inc.h:28`) is a valid
   input string. Unknown names hit `TI_ERROR`; known names are accepted
   unconditionally.
4. `taichi/program/program.cpp:554` calls `program_impl_->make_aot_module_builder(cfg)`.
   `GfxProgramImpl::make_aot_module_builder`,
   `taichi/runtime/program_impls/gfx/gfx_program.cpp:30-41`, forwards `caps` on
   both branches, at `:34` and `:39`.
5. `taichi/runtime/gfx/aot_module_builder_impl.cpp:21` — `caps_(caps)`.
6. `grep -n load_or_compile taichi/runtime/gfx/aot_module_builder_impl.cpp` →
   `:75` and `:119`, both `compilation_manager_.load_or_compile(config_, caps_, *kernel)`.
7. `taichi/compilation_manager/kernel_compilation_manager.cpp:71-82` — forwards
   `caps` to `compile_and_cache_kernel`.
8. `taichi/codegen/spirv/kernel_compiler.cpp:37` — `params.caps = device_caps;`.
9. `taichi/codegen/spirv/spirv_codegen.cpp:86` — `caps_(params.caps)`. That is
   the object read at `:783`, `:2340`, `:2416`, `:2491` and by
   `spirv_ir_builder.cpp:73` and `:113`.

**No device check appears at any of the nine steps.** Verified by reading each,
not by absence of a grep hit.

C API path: `c_api/src/taichi_core_impl.cpp:317-334`,
`ti_set_runtime_capabilities_ext`. Loop at `:324-330` builds a
`DeviceCapabilityConfig` from caller values; `:331` calls
`runtime2->get().set_caps(std::move(devcaps))`. Read `Device::set_caps` at
`taichi/rhi/public_device.h:855-857`: it is `caps_ = std::move(caps);` —
**wholesale replacement, not a merge**, no validation.

**Consequence I found that neither adversary states.** Because `translate_devcaps`
adds nothing unbidden, a caps list naming only `spirv_has_physical_storage_buffer`
leaves `spirv_has_int64` false. Read
`spirv_ir_builder.cpp:155-176`: `t_uint64_` is declared **only** inside
`if (caps_->get(cap::spirv_has_int64))` at `:166-169`. Read
`spirv_ir_builder.h:532-534`: `u64_type()` is `return t_uint64_;`, bare. Read
`spirv_ir_builder.cpp:300-331`: the `TI_ERROR` guards live in
`get_primitive_type(DataType)` — `:311-313` for i64, `:325-328` for u64 — which
`u64_type()` does not go through. And `visit(ExternalPtrStmt*)` calls
`ir_->u64_type()` at `spirv_codegen.cpp:787`, `:789` and `:790` on the
capability branch. So that combination yields a default-constructed `SType` and
an undeclared type id, silently, on a live path. Adversary 2 found this gap only
for `make_pointer` under a flag flip; it is also reachable today without
touching any flag.

**Verdict: UPHELD.** My section 1.7 sentence "the producers are still off" is
false and is withdrawn. Adversary 1 section 11 item 5 repeats the same error and
files it under what it could not break. Adversary 2 section 1.3 is right, and
its phrase "un-defaulted, not off" is the accurate one. Report amended at 0, 1.3,
1.7, new 1.10, 7.4, 7.5, 7.8 item 4, escalations 1 and 2.

## 26. CLAIM 2 — three consumers missing from both inventories. UPHELD. [LOCAL]

Rebuilt rather than patched, as instructed.

`grep -rn "spirv_has_physical_storage_buffer\|PHYSICAL_STORAGE_BUFFER"` over
`*.cpp *.h *.hpp *.mm *.py *.inc.h`, excluding `build/`. Then
`grep -rn "has_buffer_ptr"` over `*.cpp *.h`.

Direct readers: twelve, enumerated in report section 1.1a Group A. Count derived
from the rows, per plan section 10 item 7. Matches both adversaries.

The three indirect consumers, each read in context:

- `taichi/codegen/spirv/spirv_types.cpp:484-497`. `translate_ti_type(tinyir::Block&,
  DataType, bool has_buffer_ptr)`. Read it: `PointerType` becomes
  `IntType(64, unsigned)` at `:491-493` under the flag, `IntType(32, unsigned)`
  at `:494-496` otherwise; struct members recurse at `:509`. This is the function
  `:2340`, `:2416` and `:2491` all call. **It is where the width decision
  actually happens** and I cited none of it.
- `taichi/runtime/gfx/runtime.cpp:817` onward,
  `GfxRuntime::get_struct_type_with_data_layout_impl`. `has_buffer_ptr = layout[1] == 'b'`
  at `:823`; `PointerType` member sized `sizeof(uint64_t)` at `:853`,
  `sizeof(uint32_t)` at `:857`. **Host side of the same ABI.** I cited
  `gfx_program.h`, the producer of the layout string, and never the consumer that
  acts on it.
- `taichi/codegen/spirv/spirv_ir_builder.cpp:334-342`, `IRBuilder::from_taichi_type`.
  Second implementation of the same rule: `t_uint64_` at `:339`, `t_uint32_` at
  `:341`. Checked reachability: `grep -rn from_taichi_type` returns the
  definition at `:334`, its own recursive call at `:347`, and the declaration at
  `spirv_ir_builder.h:343`. **Dead.** Matters for item 6.2 because a widening
  touches two copies, one of which compiles and does nothing.

Also confirmed adversary 1 section 2.5, which I had not: in
`visit(ExternalPtrStmt*)` the index arithmetic is `i32` throughout — `:737`
initialises, `:770-771` accumulate, `:773-777` shift with `i32` result type — and
only `:790` widens by `OpSConvert` before adding to the `u64` base. So the
capability gives a 64-bit base plus a 32-bit signed offset, not wider indexing. I
overcredited it. Report section 1.1b.

**Verdict: UPHELD.** Both adversaries name the same three, independently, which
is the shared-blindness signal the method asks for. Report section 1.1a replaces
the old six-bullet list with fifteen enumerated sites in three groups.

## 27. CLAIM 3 — dead SNode code contradicting the intent half. UPHELD. [LOCAL]

Read `taichi/codegen/spirv/snode_struct_compiler.cpp` in full.

`StructCompiler::construct` at `:34-62`. At `:52-54`:
`if (sn->type == SNodeType::pointer) { cell_type = ir_module.emplace_back<PhysicalPointerType>(cell_type); }`.

Followed `PhysicalPointerType` to its SPIR-V translation:
`spirv_types.cpp:433-439`, `visit_physical_pointer_type`, emits
`spv::OpTypePointer` with `spv::StorageClassPhysicalStorageBuffer` at
`:435-436`. That storage class exists only under this capability. So the SNode
struct compiler proposes physical device addresses for sparse `pointer` cells.

Reachability, checked three ways:
- `grep -n "construct(" snode_struct_compiler.cpp` → `:18` (inside the `/* */`
  block at `:16-19`), the definition at `:34`, the recursive call at `:44`. The
  only non-recursive caller is commented out.
- `snode_struct_compiler.h:49-50` — `type_factory` and `root_type` commented out
  under "TODO: Use the new type compiler".
- `git log --all --oneline -S "PhysicalPointerType"` → **exactly one commit**,
  `46b5632e5` "[vulkan] Codegen & runtime improvements (#5213)", Bob Cao,
  2022-06-22. Read the diff with `git show 46b5632e5 -- taichi/codegen/spirv/snode_struct_compiler.cpp`:
  the `/* result.root_type = construct(...) */` block is added as `+` lines
  **already inside the comment**. It never ran.
- `grep -rn PhysicalPointerType` → construction sites are
  `snode_struct_compiler.cpp:53` (dead) and `spirv_types.cpp:337` (type reducer,
  copies an existing instance only). No live producer, so the visitor at `:433`
  never runs. Report B lists `:433-439` among live consumers; it is not one.

**Verdict: UPHELD, and weighed as follows.** It does not overturn my behaviour
claim: no code that runs connects the capability to the SNode root. It does
falsify the stronger reading — that nobody ever connected them — because the
author of the capability sketched exactly that design four months later and
shipped it commented out. Report section 1.11 states the narrowed claim
verbatim. Per plan section 10 item 3 I am not calling it surplus and not calling
it an asset; escalation 3.

Side effect worth recording: it also strengthens claim 11. Adversary 1 marks its
`maxStorageBufferRange` argument INFERRED because "Taichi has no source line on
this." `snode_struct_compiler.cpp:53` is that source line, reached independently
by the mechanism's author.

## 28. CLAIM 4 — the module-wide addressing model is not an architectural obstacle. UPHELD. [LOCAL]

Both adversaries refuted my section 1.2 consequence by the same two routes and I
checked all three grounds directly.

Ground 1, module granularity. `spirv_codegen.cpp:97` constructs an `IRBuilder`
per `TaskCodegen`; `run()` at `:130-156` calls `init_header()`, builds one
function `"main"`, and returns `ir_->finalize()` at `:151`. One offloaded task,
one module. My phrase "every shader in the module" implies several; there is one.

Ground 2, permissiveness. `AddressingModelPhysicalStorageBuffer64` adds the
`PhysicalStorageBuffer` storage class; it does not invalidate logical pointers.
INFERRED from the SPIR-V and Vulkan specs. I have no Taichi source line for this
and say so.

Ground 3, the shipped releases. **This is the decisive one and I verified it
directly rather than taking adversary 2's excerpt.**

```
$ git show v1.1.3:taichi/codegen/spirv/spirv_ir_builder.cpp | grep -n AddressingModel
114:        .add_seq(spv::AddressingModelPhysicalStorageBuffer64,
119:        .add_seq(spv::AddressingModelLogical, spv::MemoryModelGLSL450)
$ git show v1.1.3:taichi/codegen/spirv/spirv_codegen.cpp | grep -n 'use_64bit_pointers\|make_pointer(0)'
74:  const bool use_64bit_pointers = false;
275:    spirv::Value root_val = make_pointer(0);
1805:      auto container_ptr = make_pointer(0);
2063:    if (use_64bit_pointers) {
```

And the v1.1.3 `make_pointer` body, read out: the false branch returns
`ir_->uint_immediate_number(ir_->u32_type(), uint32_t(offset))`. So in v1.1.3 the
module declared the physical addressing model while the SNode root was a 32-bit
offset into a descriptor-bound buffer. Per note 32 that configuration shipped in
**eleven** tagged releases plus three more with only macOS excepted.

**Verdict: UPHELD. The addressing model is a declaration, not a constraint.** My
section 1.2 sentence is withdrawn and row E of the section 7.0 table changes from
"ARCHITECTURAL, holds permanently" to "withdrawn, not an obstacle". What survives
in that area is the environmental requirement that a declared SPIR-V capability
must be one the device enabled — which per note 25 the AOT path has no mechanism
to ensure.

## 29. CLAIM 5, ADVERSARY DIVERGENCE — the `at_buffer` u64 collision. ADVERSARY 2 ON THE FACT. [LOCAL]

Adversary 1 section 4 rules it architectural and surviving, resting on: "The type
carries two incompatible meanings and the code has no third thing to disambiguate
them." Adversary 2 section 12.3 item 1 says the tag that would replace the sniff
already exists and is already set on the physical branch. Both cannot be right.

Read the mechanism rather than either account.

- `taichi/codegen/spirv/spirv_ir_builder.h:69-79` — `enum class ValueKind`, with
  `kNormal, kConstant, kVectorPtr, kStructArrayPtr, kVariablePtr, kPhysicalPtr,
  kTexture, kFunction, kExtInst`.
- `:81-92` — `struct Value { uint32_t id; SType stype; ValueKind flag{kNormal}; }`.
  The tag is at `:88`.
- `spirv_codegen.cpp:2203` — `paddr_ptr.flag = ValueKind::kPhysicalPtr;`. Set on
  the physical branch of `at_buffer`. Adversary 1 cites this exact line (as
  `:2204`, one off) two paragraphs above denying such a mechanism exists.
- **The tag is already load-bearing.** `IRBuilder::load_variable`,
  `spirv_ir_builder.cpp:1270-1284`: asserts `flag` is one of
  `kVariablePtr / kStructArrayPtr / kPhysicalPtr` at `:1271-1273`, then branches
  on `kPhysicalPtr` at `:1275` to emit an aligned `OpLoad`.
  `store_variable` at `:1286-1298` does the same at `:1288` and `:1290`. So
  pointer kind is *already* discriminated by tag rather than by scalar type at
  load and store.
- **It survives the value table.** `register_value` at `:1299-1309` stores the
  whole `Value` at `:1308`; `query_value` at `:1311-1317` returns it by value at
  `:1314`. `new_value` at `spirv_ir_builder.h:520-526` sets it at construction,
  and `:295` already marks `kVariablePtr`.

**Adversary 1's stated ground is factually wrong.** There is a third thing.

**One precision adversary 2 does not state, and it changes the sizing.** The tag
at `:2203` is on the pointer `at_buffer` *produces*, after the `:2197` branch is
taken. It is not on the address value `at_buffer` *consumes*. To replace the
sniff, the tag must be populated at the production sites: the four `make_pointer`
call sites (`:355`, `:371`, `:408`, `:506`) and the `ExternalPtrStmt` base path
(`:790-792`). `make_pointer` currently returns `ir_->uint_immediate_number(...)`,
producing a `kConstant`. So the mechanism exists and is proven in-file, and is
not yet wired where it would be needed.

**Verdict, and the part that is mine.** The collision is real and blocks a flag
flip — all four documents agree and I re-verified `:2197`, `:2203`, `:2227`,
`:2249`, `:355`, `:377`, `:2212`. It is not ENVIRONMENTAL: nothing external fixes
it. But it is not ARCHITECTURAL under my own section 7 definition ("does not
expire"), because the replacement mechanism is present in the same file and
already used elsewhere in it. **Adversary 2's classification stands; adversary
1's ground for the opposite does not.**

Corroborating, and I read the diff myself rather than the PR summary alone:
`git show 715a04c98 -- taichi/codegen/spirv/spirv_codegen.cpp` adds the
`TI_ERROR_IF(!is_integral(ptr_val.stype.dt), "at_buffer failed, ...")` diagnostic
now at `spirv_codegen.cpp:2207-2210`. The mechanism's own author filed a bug
against the dtype discriminator and fixed it locally in one function. A design
commitment that cannot be disambiguated does not get patched that way.

I also confirmed both adversaries on the `bitmasked_activation` sub-point by
reading `:383-416`: `ptr_dt` at `:388`; `:398-399` OpShiftLeftLogical result
`ptr_dt` base `const_i32_one_`; `:405` `u32_type()` as shift *amount*, legal;
`:410-412` OpShiftRightLogical result `u32_type()` base `ptr_dt`; `:414`
`struct_array_access(ir_->u32_type(), ...)`; `:409` `ir_->add(parent_ptr,
bitmask_word_ptr)` with both operands `ptr_dt`, **not** a defect. My three
citations are right, report B's `:409` claim is wrong, and my stated *rule*
("Base and Result must share a type") was too strict — the enforced rule is bit
width and component count, not signedness. Corrected in report section 2.3(b).

Report amended at 7.0 row F, 7.7 item F, 2.3(a), new 2.3(c), 7.8 item 3.

## 30. CLAIM 6 — the assertion is gated on all-dense; sparse width is `snode.h:41,45,49`. UPHELD. This is the most important correction. [LOCAL]

Both adversaries found this independently and both are right.

Read `taichi/transforms/demote_dense_struct_fors.cpp` in full.

- `convert_to_range_for` at `:13`. The assertion is at `:29`,
  `TI_ASSERT(total_n <= std::numeric_limits<int>::max());`.
- `maybe_convert` at `:114-119` is the **only** caller:
  `if ((stmt->task_type == TaskType::struct_for) && stmt->snode->is_path_all_dense)`.
  The gate is at `:116`.
- `is_path_all_dense` declared `taichi/ir/snode.h:114`; set at
  `taichi/ir/snode.cpp:20`,
  `new_ch->is_path_all_dense = (is_path_all_dense && !new_ch->need_activation());`.
- `SNode::need_activation` returns true for `pointer`, `hash`, `bitmasked`,
  `dynamic`. Any of the four on the path clears the flag.
- `taichi/transforms/offload.cpp:191-192` reads the same flag for the same
  decision.

The assertion is nonetheless a genuine hard stop where it fires:
`taichi/common/logging.h:100-107`, `TI_ASSERT` is `if (!x) TI_ERROR(...)`, no
`NDEBUG` gate. Issue #8608 quotes the exact runtime error naming
`demote_dense_struct_fors.cpp:convert_to_range_for@29`.

**Plan section 4.1 requires sparsity, so for this project the assertion never
executes.** My section 3.2 called it "the operative constraint today" without
qualification. That would have told the planner the cap for this workload is
2^31-1 total cells. It is not.

The width that does bind, read out of `taichi/ir/snode.h:34-53`:

- `:41` `int num_elements_from_root{1};`
- `:45` `int shape{1};`
- `:49` `int acc_shape{1};`

The brief's line numbers are exact. For contrast `:97` is
`int64 num_cells_per_container{1};` — already widened.

Ordering, read out of `taichi/ir/snode.cpp:89-101`: `int64 acc_shape = 1;` at
`:89`; the truncating store `new_node.extractors[i].acc_shape =
static_cast<int>(acc_shape);` at `:92`, **inside the loop, on every iteration**;
the check `if (acc_shape > std::numeric_limits<int>::max())` at `:95`; the
warning at `:96-99`; `new_node.num_cells_per_container = acc_shape;` at `:101`
taking the untruncated value. So the warning is a post-hoc detector for damage
already done. `shape_along_axis` at `:174-177` returns `int` as well.

**Verdict: UPHELD.** Report section 3.2 rewritten, 7.0 row I and 7.7 item I
relocated, escalation 5 added. Flagged as territory 01 overlap; I defer on
completeness but it must not fall between two territories.

## 31. CLAIM 7 — strike `addr_space_` from item 6.2. UPHELD. [LOCAL]

Re-verified rather than assumed, since four agreements is exactly the condition
under which nobody checks again.

- `taichi/ir/type.h:207`, `int addr_space_{0};  // TODO: make this an enum`.
- Getter at `:191-193`. `TI_IO_DEF(pointee_, addr_space_, is_bit_pointer_)` at
  `:203`. Constructors at `:179` (default) and `:181-185` (`Type*, bool`) —
  **neither takes it, and no setter exists**.
- `grep -rn "addr_space"` over `*.cpp *.h *.mm` excluding `build/`: three hits in
  `taichi/ir/type.h` (`:191`, `:192`, `:203`, `:207` — getter body, return,
  serialiser, field) and three in `taichi/codegen/llvm/` which are the unrelated
  LLVM parameter. Everything else is `external/SPIRV-Cross/spirv_msl.cpp`, Metal
  address-space keyword emission, nothing to do with this field. **Zero call
  sites of `get_addr_space`.**
- LLVM analogue read out: `codegen_llvm.h:165` declares `int addr_space = 0` as a
  default parameter of `cast_pointer`; `codegen_llvm.cpp:1129-1134` is the
  function, `:1131` the parameter, `:1133` the
  `llvm::PointerType::get(get_runtime_type(dest_ty_name), addr_space)`. Adversary
  2 puts the call at `:1132`; I read it at `:1133`. Minor and it changes nothing.
- Adversary 2's nuance accepted: "zero call sites, ever" is not literally true
  because the `TI_IO_DEF` at `:203` serialises it into offline cache and AOT type
  records. Always zero, so nothing observable follows. Recorded in report 4.1.
- Adversary 1's qualification accepted and recorded without softening: on AMDGPU
  `addrspace(3)` implies 32-bit and `addrspace(1)` 64-bit, so space and width are
  not orthogonal on every backend. That is item 6.3 territory, not a reason to
  keep a dead field under 6.2.

**Verdict: UPHELD.** Four independent agreements — both explore passes, both
adversaries — with nothing contradicting. Report section 4.2 states the
recommendation and stresses that it is a classification change, not a removal.
Plan section 10 item 3 forbids removal and nothing here argues for it.

## 32. CLAIM 8, ADVERSARY DIVERGENCE — the tag census. ADVERSARY 2. [LOCAL]

Adversary 1 section 7: 122 tags, capability in 21 of them, 8 enabled starting at
v1.0.0, 3 macOS-excepted, 10 off. Adversary 2 section 5: 11 enabled starting at
v0.9.0, 3, 10. Settled by enumeration.

Method: for every tag from `git tag | sort -V`, try
`taichi/rhi/vulkan/vulkan_device_creator.cpp` then
`taichi/backends/vulkan/vulkan_device_creator.cpp`; locate the
`spirv_has_physical_storage_buffer` line; classify by the preprocessor guard in
the eight lines above it. Full output kept.

Result:

- `git tag | wc -l` → **121**, not 122. Adversary 1 is one over.
- Capability absent though the file exists: v0.8.7, v0.8.8, v0.8.9, v0.8.10,
  v0.8.11. **5 tags.**
- ON with no preprocessor guard: v0.9.0, v0.9.1, v0.9.2, v1.0.0, v1.0.1, v1.0.2,
  v1.0.3, v1.0.4, v1.1.0, v1.1.2, v1.1.3. **11 tags.**
- `#if !defined(__APPLE__)`: v1.2.0, v1.2.1, v1.2.2. **3 tags.**
- `#if !defined(__APPLE__) && false`: v1.3.0, v1.4.0, v1.4.1, v1.5.0, v1.6.0,
  v1.7.0, v1.7.1, v1.7.2, v1.7.3, v1.7.4. **10 tags.**

11 + 3 + 10 = 24 tags contain the capability, against adversary 1's 21. No
v1.1.1 tag exists. Both adversaries' disabled rows are right and agree with mine
exactly; the divergence is entirely the v0.9.x window, which adversary 1 missed
even though it handled the `backends/` → `rhi/` path move that would otherwise
explain a gap.

Spot-read the v0.9.0 file to be sure it is a real setter and not a leftover:

```
$ git show v0.9.0:taichi/backends/vulkan/vulkan_device_creator.cpp | sed -n '657,672p'
      if (CHECK_VERSION(1, 3) ||
          buffer_device_address_feature.bufferDeviceAddress) {
        if (device_supported_features.shaderInt64) {
          ti_device_->set_cap(
              DeviceCapability::spirv_has_physical_storage_buffer, true);
```

Same shape at v1.1.3 `:736-738`. Note the precision: "unguarded" means no
*preprocessor* guard. All eleven still sit under the runtime
`device_supported_features.shaderInt64` check. Recorded that way in the report so
nobody reads it as "no device check".

**Verdict: adversary 2. Eleven, three, ten, across 24 of 121 tags.** My old
section 1.3 was thin rather than wrong: accurate for the tags it named, no start
point, and stopped at v1.7.0 when v1.7.4 also ships it off. Replaced by the full
table at report section 1.3a.

## 33. CLAIM 9 — "ten commits through 2023 and 2024" is wrong. UPHELD. [LOCAL]

```
$ git log --since=2023-01-01 -S 'has_buffer_ptr' --format="%h %ad %s" --date=short master
```

Returns exactly ten, enumerated in report section 1.8. Newest `7809ebbd5`
2023-07-13, oldest `4c33922df` 2023-04-06. **98 days, fourteen weeks. Nothing in
2024.** The brief says "roughly fifteen weeks"; measured it is fourteen, and I
state the two dates so the figure is reconcilable.

`git log --since=2024-01-01 --format="%h %ad %s" --date=short -- taichi/codegen/spirv/`
returns three commits in total: `8ebe32b69` (2024-06-17, spdlog bump),
`4a80bb2dd` (2025-02-26, CI actions), `ba0e81dce` (2025-07-30, debugprintf
sanitising). None touches this path.

On "nothing since August 2023": precise. `git log --since=2023-08-01 -- taichi/codegen/spirv/spirv_codegen.cpp`
returns `715a04c98` (2023-08-15), `ac49f79f4` (2023-10-31, clz instruction) and
`ba0e81dce` (2025-07-30). The file has been touched after August 2023; **this
path** has not. `715a04c98` is the last edit to it — verified by reading its diff,
which changes `make_pointer` at `:2318-2320` and adds the `at_buffer` dtype
diagnostic. Stated that way in the report rather than as a blanket claim, because
plan section 10 item 6 warns that a verified citation does not verify a
quantifier attached to it.

Zero test coverage re-verified over `tests/`, `c_api/`, `misc/` and `benchmarks/`
with the widened term list. Nothing outside the C-API headers, generated docs and
`python/taichi/lang/enums.py:38`.

**Verdict: UPHELD.** My section 1.8 and old escalation 6 said "2023-2024" and
"roughly two years of never-executed code". Corrected to April–August 2023.
Note this strengthens the momentum reading: the last touch moves eighteen months
earlier.

## 34. CLAIM 10 — `gfx_program.h:75-77` versus `spirv_codegen.cpp:2490-2494`. UPHELD, conditionally. [LOCAL]

Read all four sites.

- `taichi/runtime/program_impls/gfx/gfx_program.h:75-77`:
  `std::string get_kernel_return_data_layout() override { return "4-"; };`
  **No capability read.**
- `:79-83`: `get_kernel_argument_data_layout()` reads the capability at `:80-81`
  and returns `"1b"` or `"1-"` at `:82`.
- `taichi/codegen/spirv/spirv_codegen.cpp:2483-2500`, `compile_ret_struct`:
  `bool has_buffer_ptr = caps_->get(DeviceCapability::spirv_has_physical_storage_buffer);`
  at `:2490-2491`, threaded into `translate_ti_type(blk, element.type,
  has_buffer_ptr)` at `:2494`.
- `taichi/codegen/spirv/spirv_types.cpp:491-496`: `PointerType` → 64-bit under the
  flag, 32-bit otherwise.
- Host consumer `taichi/runtime/gfx/runtime.cpp:823`, `:851-859`: with `"4-"`,
  `layout[1] != 'b'`, so a `PointerType` member is always sized 4 bytes at
  `:857`.

So with the capability on, the shader lays out a return-struct pointer at 64 bits
and the host at 32.

Callers confirmed: `taichi/program/callable.cpp:159` uses the return layout,
`:180` the argument layout; `taichi/program/argpack.cpp:15` the argument layout.
The LLVM program impl has its own overrides at `llvm_program.h:217` and `:221`,
so this is a gfx-only asymmetry.

Provenance confirmed: added by `88ac098bc` (#8061, 2023-05-23), row 7 of the ten
in note 33, with no matching change to `get_kernel_return_data_layout`.

**Verdict: UPHELD, conditionally.** I did not resolve whether a kernel return
struct can contain a `PointerType`; that is a frontend question and plan section
1.2 puts the Python frontend out of scope. Adversary 1's hedge is the right one
and I keep it. If it can, this is a live host and device ABI mismatch sitting in
a zero-coverage branch that note 25 shows is reachable. Report section 1.12,
escalation 4.

## 35. CLAIM 11 — on SPIR-V there is no other mechanism. UPHELD. [LOCAL] + INFERRED

Adversary 1 section 2.4. Checked each link.

- Root buffer binding: `get_buffer_value` at `spirv_codegen.cpp:2287-2315`; the
  general path calls `ir_->buffer_argument(type, 0, binding, buffer_instance_name(buffer))`
  at `:2309`. Read `IRBuilder::buffer_argument`, `spirv_ir_builder.cpp:734-763`:
  storage class is `StorageClassStorageBuffer` at `:744`
  (`StorageClassUniform` below SPIR-V 1.3 at `:742`), it emits an `OpVariable` at
  `:755-757`, and tags the value `ValueKind::kStructArrayPtr` at `:754`.
  **Descriptor-bound. VERIFIED.** `bitmasked_activation` binds it explicitly at
  `:401-402`.
- Offset width: `at_buffer`'s non-u64 path at `:2212-2218` shifts `ptr_val` right
  by `log2(width)` and hands it to `struct_array_access`. `ptr_val` is whatever
  `make_pointer` produced — `u32` today (`:2322`).
- `grep -rn "maxStorageBufferRange" taichi/ c_api/` → **zero hits**. The limit is
  never queried anywhere in the tree. VERIFIED.

The remaining step — that a descriptor-bound storage buffer's reach is capped by
`maxStorageBufferRange`, and that SPIR-V offers no third addressing mechanism —
is read from the SPIR-V and Vulkan specifications, not from a Taichi line. **I
mark that half INFERRED**, as adversary 1 does.

Adversary 2 supplies the one Taichi artefact that bears on it:
`snode_struct_compiler.cpp:53` (note 27) is the mechanism's own author applying
physical addresses to sparse `pointer` SNode cells. That does not verify the
inference — the code never ran — but it moves it from "deduced from the spec" to
"deduced from the spec and independently arrived at by the person who wrote the
mechanism". Recorded in report section 6 item 6.

**Verdict: UPHELD, and it qualifies the convergent finding rather than refuting
it.** The correct statement is: the shelved work is not reusable as written, and
is simultaneously the only door SPIR-V offers past a bound buffer's reach, hence
structurally prerequisite for a root that outgrows one.

**One scope point that is mine and neither adversary states.** This is a
SPIR-V-spine statement. Plan section 5.2 as corrected puts the LLVM spine first
because CPU-only operation runs through it, and nothing here says anything about
LLVM addressing. Recorded in the report so the qualifier is not over-read.
Report section 6 item 6, escalation 12.

---

## 36. Method note for this pass

Every claim in the brief was treated as a claim. Nine were upheld outright, two
were adversary divergences and were settled by enumeration or by reading the
mechanism. **No claim was found wrong.** Two adversary *grounds* were found wrong
and I changed nothing on their account beyond recording them: adversary 1's "no
third thing to disambiguate them" (note 29) and adversary 1's eight-tag census
(note 32). One adversary 2 phrase was corrected in place rather than rejected
(note 29, on where the `kPhysicalPtr` tag is set).

Counts in the amended report are derived from their own enumerations, per plan
section 10 item 7: fifteen consumer sites in three groups (section 1.1a), 11/3/10
across 24 tags of 121 (section 1.3a), ten commits with dates (section 1.8), nine
verdict rows (section 7.0), eleven ledger rows (section 9).

I compiled nothing and ran nothing. The only executions in this pass were `git`,
`grep` and `sed` reads. No source file, no other agent's file, and no git state
was touched; `git status --porcelain` shows only the untracked `modernization/`
directory it showed at the start.

---

# THIRD ADVERSARIAL ROUND — amendment pass, 2026-09-09

Entries 37 to 43. Against `adversary2-05-1.md` and `adversary2-05-2.md`. Repo at
`ba0e81dce`, read-only. No source file touched, no other agent's file touched, no
git state changed.

**Plan version worked against**, per standing instruction 6:
`modernization/PROJECT-PLAN.md`, "Last updated: 2026-09-09", 60988 bytes, md5
`73106c01879a06b83ca5ccb6fdc891d9`. Re-read at the start and end of this pass.

## 37. THE RULING I HAVE TO MOVE ON — `make_value` erases the tag. I was wrong. [LOCAL]

Entry 29 of the second pass adjudicated a divergence between the two round-one
adversaries and ruled for adversary 2 on the fact and against the ARCHITECTURAL
label. **The label ruling is withdrawn.** Both round-two adversaries settled it
against me independently and I re-derived the deciding fact rather than accepting
either.

Read `IRBuilder::make_value` directly,
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

`new_value(out_type, ValueKind::kNormal)` at `:292`; the only override at
`:294-296` keys on `out_type.flag`. **No operand's flag is read anywhere in the
body.** `SType::flag` defaults to `TypeKind::kPrimitive` at
`spirv_ir_builder.h:59`, so an integer byte offset comes out `kNormal` whatever
its operands carried.

Then the eraser on the SNode path. `ir_->add` is
`DEFINE_BUILDER_BINARY_USIGN_OP(add, Add)`, macro at
`spirv_ir_builder.cpp:1038-1047`, instantiated at `:1062`, body
`make_value(spv::OpI##_Op, a.stype, a, b)`. So `spirv_codegen.cpp:372`,
`ir_->add(input_ptr_val, offset)` inside `visit(GetChStmt *)`, returns `kNormal`.

And it is unavoidable, which is the part I never checked. `grep -rn
"ptr_to_buffers_" taichi/ tests/ cpp_examples/` returns ten hits, all in
`spirv_codegen.cpp`. The only SNode write is `:377`, inside `visit(GetChStmt *)`
and guarded by `out_snode->is_place()` at `:375` — which is five lines *below* the
add at `:372`. **So every SNode place pointer that reaches `at_buffer` has passed
through that add.** `SNodeLookupStmt` at `:504-510` adds a `mul` at `:507` and an
`add` at `:508` on top.

The ndarray side likewise: `load_variable` at `spirv_ir_builder.cpp:1270-1285`
opens `Value ret = new_value(res_type, ValueKind::kNormal);` at `:1274`, above the
`kPhysicalPtr` branch at `:1275`, so it applies to both arms. The device address
loaded at `spirv_codegen.cpp:789` and added at `:790-791` registers at `:792` as
`kNormal`.

**Both sides of `:2197` therefore arrive identically tagged.** My second-pass
remedy — populate the tag at the four `make_pointer` call sites and the
`ExternalPtrStmt` base path — is necessary and insufficient, and my own stated
precision was the seed of the error. "The tag sits on the output, not the input"
is exactly true; "so populate the input" does not follow, because nothing survives
the trip.

**On who was right.** Round-one adversary 1's SENTENCE ("no third thing to
disambiguate them") was wrong and I was right to say so. Round-one adversary 1's
RULING (architectural) was right and I was wrong to overturn it. I ruled against
the adversary with the correct answer on the strength of a defective sentence,
and never traced the constructor. Entry 29 shows the gap in my own words: it
enumerates the tag, `:2203`, `load_variable` and `register_value` and stops.

**One correction back at the adversaries, minor and not load-bearing.** Both say
the values "arrive untagged". They arrive tagged non-discriminatingly, and not
uniformly: `GlobalTemporaryStmt` registers `kConstant` at `spirv_codegen.cpp:710`,
and `make_pointer`'s own result is `kConstant` via
`new_value(dtype, ValueKind::kConstant)` at `spirv_ir_builder.cpp:1502`.
Round-two adversary 2 records this itself at its section 2.4. It changes nothing:
`kConstant` discriminates a device address from a buffer offset no better than
`kNormal`.

Report amended at 7.7 item F (rewritten), 7.0 row F, 7.8 item 3, 2.3(a), section
0, ledger row 5 marked superseded.

## 38. GRADING UNDER THE PLAN'S RULE — kind first, radius separately. UPHELD, and it moves a second row. [LOCAL]

The plan now supplies the test I graded without: *if every external thing were
ideal today, would the obstacle still be there?* And in terms: "Architectural does
not mean hard, and environmental does not mean easy. Grade the KIND first, then
state the BLAST RADIUS separately and concretely, in files and call sites."

My section 7.7 item F withheld the label "because the replacement mechanism is
present in the same file and already load-bearing elsewhere in it." That is a
cost sentence doing a kind's job, which the rule bars. Both adversaries name the
conflation and both are right. Note the symmetry they also record: report B
awarded the label partly because the fix looked large. Same error, opposite sign.
The rule settles it without reference to either agent's sizing.

Applied to F: the overloading of pointer width as an addressing-scheme tag lives
at `spirv_codegen.cpp:2197`, `:2227` and `:2249` — three sites, enumerated by
`grep -n "stype.dt == PrimitiveType::u64"`, a count neither report gave. It is a
representation decision inside this codebase. No ideal driver, specification or
submodule pin removes it. **ARCHITECTURAL.** My own preceding sentence already
held that answer.

**Row C moves too, and this one is mine to accept rather than defend.**
Round-two adversary 1 regrades "AOT device-capability negotiation not ready" from
ENVIRONMENTAL to ARCHITECTURAL. It was unwritten Taichi code; no ideal external
thing writes it, and it expired because somebody changed the codebase, which is
exactly how the rule says architectural obstacles go away. I checked the rule
against the row and it is right. Low stakes — both reports agree the obstacle is
discharged — but a wrong kind next to a right conclusion should not enter the
plan.

**Row B examined and left standing.** Vulkan offers no query for the extensions an
imported device has *enabled*, so an ideal external specification would remove it.
ENVIRONMENTAL. Adversary 1 reached the same grade by the same route and called it
defensible and contestable. I concur and change nothing.

Recount, derived from the rows of section 7.0 and not carried from the old text:
nine rows, four ARCHITECTURAL (C, D, F, I), one withdrawn as not an obstacle (E),
four ENVIRONMENTAL (A, B, G, H). Four plus one plus four is nine. My previous
"two permanent plus one bounded" is withdrawn: the permanent count did not fall
from four, its membership changed.

## 39. THE DISCRIMINATOR DOES EXIST — and this one cuts my way. [LOCAL]

The second point the lead put to me, and it is the one place the third round moves
in this report's favour. I verified it from source rather than adopting territory
07's result.

**The inherited sentence.** Report B's leg 4, taken from `adversary-05-2.md`
section 14.1: "There is no existing discriminator at the decision point by any
route." Both round-two adversaries observed, independently, that the argument
tests whether `ptr_to_buffers_` holds an ENTRY on both sides and never what the
entry's VALUE is.

**Enumerated.** `grep -rn "ptr_to_buffers_" taichi/ tests/ cpp_examples/` returns
ten hits, all in `taichi/codegen/spirv/spirv_codegen.cpp`: seven writes (`:319`,
`:326`, `:332`, `:377`, `:711`, `:798`, `:800`), one `count` at `:376`, one read
at `:2212`, one declaration at `:2635`. Counting rule: one row per grep hit,
classified by whether it assigns.

The two sides of the `:2197` decision hold **different values**:
`BufferInfo(BufferType::Root, root)` at `:377` for an SNode place, against
`{BufferType::ExtArr, arg_id}` at `:798` for an array argument. The map is keyed
on `const Stmt *` and propagated explicitly at `:319`, `:326`, `:332`, so
**arithmetic cannot erase it** — which is the exact property the `Value` flag
lacks, per entry 37. The map is already read at `:2212`, one statement below the
predicate in dispute.

**And the evidence for the inherited sentence was manufactured by the test that
produced it.** This is territory 07's finding and I re-derived every leg:

- `AllocaStmt` is not among the seven writers. Its visitor is at
  `spirv_codegen.cpp:284` and writes no entry.
- So `ptr_to_buffers_[stmt->origin]` at `:319` is `operator[]` on an absent key,
  which default-inserts a value-initialised `BufferInfo`.
- `BufferInfo`, `taichi/codegen/spirv/kernel_utils.h:33-45`: `BufferType type;` at
  `:34` with no default member initialiser, `std::vector<int> root_id{-1}` at
  `:35`, `BufferInfo() = default;` at `:37`. `BufferType` at `:23-31` has seven
  members with `Root` first at `:24`. So the fabricated entry is `{Root, {-1}}`,
  a root buffer with root id −1 that no writer intended.
- It is unobservable today only by accident of ordering.
  `IRBuilder::get_pointer_type` (`spirv_ir_builder.cpp:407-424`) never assigns
  `SType::dt` — I read the whole body — and `DataType::DataType()` is
  `ptr_(PrimitiveType::unknown.ptr_)` at `taichi/ir/type.cpp:20`. So that value's
  `stype.dt` is `unknown`, the u64 test at `:2197` is false, and
  `TI_ERROR_IF(!is_integral(ptr_val.stype.dt), ...)` at `:2207-2210` fires. **That
  guard sits between the predicate at `:2197` and the map read at `:2212`.**

So a rule consulting the map at `:2197` moves the shared-array `MatrixPtrStmt`
from a loud compile error onto the fabricated path. The fabrication has to be
fixed first. Territory 07 says so and I agree.

**Blast radius, with the rule stated per standing instruction 9.** Counting rule:
every source line that must be edited, or whose behaviour changes, for a
map-plus-capability discriminator to replace the width sniff; one row per line.

| Class | Sites | Edit? |
|---|---|---|
| Width sniff | `spirv_codegen.cpp:2197`, `:2227`, `:2249` | yes, 3 |
| Fabricating propagation writes | `:319`, `:326`, `:332` | yes, 3 |
| The `{Root, {-1}}` default | `kernel_utils.h:34` | yes, 1 |
| Map read already present | `:2212` | no |
| Unguarded `at_buffer` calls | `:1617`, `:1626`, `:1681` | no |

Eleven rows: **11 sites in 2 files, 7 requiring an edit.** Three plus three plus
one is seven. Verified the last row myself: `grep -n "at_buffer(" spirv_codegen.cpp`
returns nine hits, and `:1621`, `:1630`, `:1633` carry the `dest_is_ptr` guard
computed at `:1612` while `:1617`, `:1626`, `:1681` do not.

Reconciles with the neighbours rather than contradicting them: territory 07a's
"eight sites in two files" counts one predicate and omits the map read; 07b's
"six lines in one file" counts only the edits inside `spirv_codegen.cpp`, leaving
out `kernel_utils.h:34`. Three counting rules, one finding. *One bookkeeping note,
offered without prejudice: 07a's own bullet list has nine items under a heading
that says eight, which reads as eight only if the already-present map read is not
counted as a site. That is the failure standing instruction 9 warns about and I
flag it rather than adopting either number.*

**What I take from this and what I do not.** My second-pass instinct that the
remedy was small was closer to right than the consensus that overruled me: not
147 call sites and a value-model change, but seven edits in two files. But the
route that makes it small is the buffer map, which I never considered, and not
the `ValueKind` tag, which I argued for and which genuinely cannot supply it.
**Being roughly right about the cost does not license having been wrong about the
kind**, and under the plan's rule the cost was never what the label answered. I
record the vindication narrowly and the correction in full.

## 40. PLAN VERSION, AND A NUMBERING CORRECTION AGAINST THREE DOCUMENTS [LOCAL]

Standing instruction 6 requires the dated version. Worked against
`modernization/PROJECT-PLAN.md`, "Last updated: 2026-09-09", 60988 bytes, md5
`73106c01879a06b83ca5ccb6fdc891d9`, re-read at start and end.

**Three changes bear on this territory** and are recorded in report section 0a:
section 10's new grading definition; section 2.2a putting item 6.2 in scope as
BASE work in full, "the two halves are not separable"; and section 8.2 item 0
recording this territory's own question as resolved by territory 07, with measured
`maxStorageBufferRange` values and a blast radius of 11 sites in 2 files, 7
requiring an edit.

**The numbering.** The lead's brief, `adversary2-05-1.md` section 2 and
`adversary2-05-2.md` section 1 all cite the grading rule as **section 10 item 7**.
Enumerated from the file, the numbered list runs: 1 do the task, 2 escalate
uncertainty, 3 nothing is unnecessary, 4 no abstraction, 5 paths and lines, 6
living document and state the dated version, 7 a verified citation does not verify
the claim, **8 grading an obstacle**, 9 derive counts from enumeration, then a
stray second "6." for "nothing you conclude enters the plan". So the grading rule
is **item 8**. Item 6 was inserted ahead of the others and pushed the list down;
the territory 06 and 07 documents, written later, cite item 8 and are right. The
plan's own list ends with a duplicated "6.", which is a numbering defect in the
document rather than in anyone's citation of it. I cite the rule by content
throughout, because the number is demonstrably not stable — which is the point
item 6 exists to make.

**A tension inside the plan, escalated not resolved.** Section 2.2a: item 6.2 in
full is BASE, the shelved SPIR-V path included, "the two halves are not
separable", "raising the object count while addresses stay 32-bit moves the wall
rather than removing it." Section 2.3: the shelved work is "stub material under
section 5.1 rather than a path to the ceiling." As priority against scope these
reconcile; on substance they do not. Standing instruction 6 says the plan wins
where it contradicts an instruction. It does not say which half wins where the
plan contradicts itself. Report escalation 17.

## 41. THE `u64_type()` HAZARD HAS A SECOND ENTRANCE. UPHELD. [LOCAL]

Both round-two adversaries found it independently; verified here.

Entry 25 traced the hazard to `spirv_codegen.cpp:787`, `:789`, `:790`. There is a
second and earlier route. `translate_ti_type`, `spirv_types.cpp:484-497`, maps a
Taichi `PointerType` to `IntType(64, is_signed=false)` at `:491-493` under the
capability. That tinyir type is lowered by `Translate2Spirv::visit_int_type` at
`spirv_types.cpp:393-419`, read in full: the unsigned 64-bit arm at `:414-415`
calls `spir_builder_->u64_type()`, the signed 64-bit arm at `:402-403` calls
`i64_type()`, and both accessors (`spirv_ir_builder.h:529-531` and `:532-534`)
are bare returns of members assigned only under `spirv_has_int64` at
`spirv_ir_builder.cpp:166-169`. `:418` then stores `ir_node_2_spv_value[type] =
vt.id`, that is, zero.

`translate_ti_type` is called from `compile_args_struct` (`spirv_codegen.cpp:2340`),
`compile_argpack_struct` (`:2416`) and `compile_ret_struct` (`:2491`), all of
which run before any statement is visited. So the argument-struct layout produces
the undeclared type first and the `ExternalPtrStmt` body second. **Five live
sites in two files, not three.**

**One bound, from round-two adversary 1, verified and accepted.** No
device-derived capability set can produce the pairing:
`taichi/rhi/vulkan/vulkan_device_creator.cpp:632` sets `spirv_has_int64` under
`if (device_supported_features.shaderInt64)`, and the physical-storage-buffer
setter at `:826` sits inside the same `shaderInt64` test opened at `:821`. On the
just-in-time path the two are locked together. The hazard needs a hand-written
list naming one capability and omitting its dependency, so it is a trap for a
caller on the unvalidated path rather than a fault on ordinary use. Both that and
"most serious silent finding in the territory" are true, and the record carries
both. Report section 1.10 item 2 and escalation 2.

## 42. VALIDATOR LINE OFF BY ONE. UPHELD. [LOCAL]

`grep -n "set_run_validator\|Disassemble" taichi/codegen/spirv/spirv_codegen.cpp`
returns `2710: spirv_opt_options_.set_run_validator(false);` and
`2763: spirv_tools_->Disassemble(...)`. This report cited `:2709` in section 7.6
and escalation 10. Both round-two adversaries caught it. Corrected in both places.
Immaterial to every conclusion it supports.

## 43. WHICH RELEASE WINDOW "ELEVEN" MEANS. UPHELD. [LOCAL]

Both adversaries note that neither report says. From entry 32's enumeration:
**eleven** is the count of tags carrying the capability with **no preprocessor
guard**, so it is the window live on every platform including macOS. The three
`#if !defined(__APPLE__)` tags compile the setter on Linux and Windows, so the
live window on non-Apple builds is **fourteen**, v0.9.0 through v1.2.2. Eleven
plus three is fourteen.

Both figures matter and for different arguments. Fourteen is the number that
bears on this fork, whose target tiers are all non-Apple. Eleven is the number
that bears on the refutation in report section 7.7 item E, which needs only that
mixed addressing shipped at all. Report section 1.3a now states both and says
which is which.

---

## 44. Method note for this pass

Seven claims were put to me. **Six are upheld against this report and one in its
favour.** Two rulings from the second pass are withdrawn — the ARCHITECTURAL
label on reason F, and the four-or-five-site remedy — and both are marked as
withdrawn in the report rather than deleted, so the record shows the movement.

The error that produced them is worth naming exactly, because it is the same
class the plan keeps recording. Entry 29 checked the `ValueKind` tag three ways:
that it exists, that it propagates through the value table, and that it has
existing consumers. All three hold. It never checked the fourth thing, whether
the tag reaches the decision point, and that is the one that decides. My own
second-pass text said the finding was "the more agreeable" one and was "checked
three ways" — and the three ways were all ways of confirming the mechanism
existed, none of them a way of testing whether it was available where it was
needed. A verified citation does not verify the claim attached to it, and neither
does a verified mechanism.

Two adversary statements are corrected in place rather than adopted, both minor:
the shared "both sides arrive untagged" (entry 37) and the citation of the grading
rule as section 10 item 7 when it is item 8 (entry 40). One neighbouring
bookkeeping defect is flagged without adoption: territory 07a's eight-site heading
against a nine-item list (entry 39).

Counts in the amended report are derived from their own enumerations with the
generating rule stated, per standing instruction 9: nine verdict rows in four
grades (section 7.0), three width-sniff sites, seven `ptr_to_buffers_` writers,
eleven blast-radius sites of which seven are edits (section 7.7a), five live
`u64_type()` sites in two files (section 1.10), eleven and fourteen release
windows (section 1.3a), seven third-pass ledger rows (section 9a).

**What my searches could and could not see.** The tag-erasure result rests on
reading one function body, `make_value`, and on the claim that every SNode path
into `at_buffer` crosses an arithmetic instruction. That second claim is a
reachability statement, and a grep cannot certify it: I established it from the
placement of the only SNode write to `ptr_to_buffers_` at `spirv_codegen.cpp:377`
relative to the add at `:372` within one visitor, which is sound for that visitor
and is an argument about pass structure rather than an execution. I compiled
nothing and ran nothing in this pass; the only executions were `git`, `grep`,
`sed` and `md5sum` reads.
