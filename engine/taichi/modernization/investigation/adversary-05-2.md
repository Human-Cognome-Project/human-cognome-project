# Adversary 05-2 — territory `shelved-64bit`

Adversary 2 of 2. Judging `report-05-shelved-64bit.md` (pass A) and
`report-05b-shelved-64bit.md` (pass B), written blind to each other, plus their
notes files.

Everything below was re-derived from source and `git` at fork baseline
`ba0e81dce559fb63a5958bf82feb1d00c55c02fe`, or from `gh api` reads against
`taichi-dev/taichi`. No file in the tree was modified. No git state was altered.
No writes to GitHub.

Verdicts are one of **UPHELD**, **UPHELD WITH CORRECTION**, **REJECTED**, or
**NOT ESTABLISHED**.

---

## 0. Verdict summary

| # | Claim under attack | Verdict |
|---|---|---|
| 1 | PSB never touched SNode addressing; consumers are ndarray/argpack/arg/ret; SNode runs through `make_pointer` | **UPHELD for live code, with three corrections.** Both consumer inventories are incomplete. Both are wrong that there are zero live producers. And both missed dead-but-present SNode code that contradicts the *intent* half of the claim. |
| 2a | The module-wide `AddressingModelPhysicalStorageBuffer64` switch is an ARCHITECTURAL obstacle | **REJECTED.** Upstream shipped mixed addressing under that model for eight months. It is a declaration, not a constraint on other pointers. |
| 2b | `at_buffer` treats u64 as an absolute device address, colliding with `make_pointer`'s root-relative zero | **UPHELD**, verified line by line. But it is not architectural: the discriminator is a data-type sniff, and the tagging mechanism that would replace it already exists in the same file. Both reports also missed a third, silent failure mode. |
| 3 | Hard stop is the assert at `demote_dense_struct_fors.cpp:29`, not the `snode.cpp` warning | **A UPHELD; the framing of the dispute is wrong.** B names the assert too. Both miss the mechanism that actually corrupts data. |
| 4 | `PointerType::addr_space_` is address SPACE not WIDTH, and is misclassified under item 6.2 | **UPHELD.** Ruling given in §5. |
| 5 | Version ranges when PSB was live | **BOTH WRONG.** Resolved against every tag in §6. The true range starts at v0.9.0, three releases earlier than B's table. |
| 6 | Momentum loss inferred from silence | **UPHELD AS INFERENCE, and it is the weaker of the two available readings.** Positive counter-evidence exists that neither report weighed. |
| 7 | Ten commits extended the dead branch through 2023-2024 with zero test coverage | **Count and zero-coverage UPHELD. The date range is wrong.** Nothing after 2023-07-13. |

Neither report should be sent back for major revision. Both are substantially
right on the question they were asked. But the planner must not act on the
"zero live producers" claim, the "architectural" classification of the addressing
model, or B's `data64` finding, all of which are wrong.

---

## 1. The convergent claim: my own exhaustive consumer inventory

I ran the census independently rather than checking theirs. Method: grep the
whole tree for the capability name and for `has_buffer_ptr`, then follow every
value that the capability is threaded into, then check reachability of each hit.

### 1.1 Every direct reader of the capability

Twelve `get(...)` sites, and I agree with B's count of twelve:

| Site | What it gates |
|---|---|
| `taichi/rhi/vulkan/vulkan_device.cpp:1772` | buffer usage flag |
| `taichi/rhi/vulkan/vulkan_device.cpp:1792` | `vkGetBufferDeviceAddressKHR` |
| `taichi/rhi/vulkan/vulkan_device.cpp:2147` | device-address usage on import |
| `taichi/rhi/vulkan/vulkan_device.cpp:2509` | `VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT` |
| `taichi/codegen/spirv/spirv_ir_builder.cpp:73` | `OpCapability PhysicalStorageBufferAddresses` |
| `taichi/codegen/spirv/spirv_ir_builder.cpp:113` | `OpExtension` + `OpMemoryModel` |
| `taichi/codegen/spirv/spirv_codegen.cpp:783` | `visit(ExternalPtrStmt *)`, ndarray base address |
| `taichi/codegen/spirv/spirv_codegen.cpp:2340` | args struct layout |
| `taichi/codegen/spirv/spirv_codegen.cpp:2416` | argpack struct layout |
| `taichi/codegen/spirv/spirv_codegen.cpp:2491` | ret struct layout |
| `taichi/runtime/gfx/runtime.cpp:96` | host writes `get_memory_physical_pointer` into args |
| `taichi/runtime/program_impls/gfx/gfx_program.h:81` | layout string `"1b"` vs `"1-"` |

None of the twelve is on the SNode path. **That half of the convergent claim is
correct**, and I confirm it independently.

### 1.2 Three indirect consumers, all missed by both reports

The capability is converted into a `bool has_buffer_ptr` and threaded onward.
Following that thread finds three more sites neither report lists:

1. **`taichi/codegen/spirv/spirv_types.cpp:484-497`**, `translate_ti_type`. This
   is the function that `:2340`, `:2416` and `:2491` call, and it is where a
   Taichi `PointerType` actually becomes `IntType(64)` at `:491-493` or
   `IntType(32)` at `:494-496`. A cites the three call sites but never the
   function that does the work; B cites neither. Report 02 has this line; the 05
   pair does not.
2. **`taichi/runtime/gfx/runtime.cpp:823` and `:851-859`**,
   `GfxRuntime::get_struct_type_with_data_layout_impl`. This decodes the `'b'`
   out of the layout string that `gfx_program.h:81` produced and sizes a
   `PointerType` member as 8 bytes at `:853` or 4 at `:857`. **This is the host
   side of the same ABI**, and it is the site that makes A's own §6.5 point about
   an ABI variant concrete. Both reports cite `runtime.cpp:96` and stop. I
   confirmed its callers are `taichi/program/callable.cpp:161` and `:182` and
   `taichi/program/program.cpp:494`, i.e. args and ret structs only — so it does
   not disturb the separation, but it belongs in the inventory.
3. **`taichi/codegen/spirv/spirv_ir_builder.cpp:334-342`**, `from_taichi_type`,
   a second and independent implementation of the same pointer-width rule. It is
   **dead**: `grep -rn from_taichi_type taichi/` returns only its definition, its
   own recursive call at `:347`, and the declaration at
   `spirv_ir_builder.h:343`. B says "nothing was ripped out"; this is a case
   where something was left in and rotted. It matters for item 6.2 because
   anyone widening the pointer rule has two copies to change, one of which
   compiles and does nothing.

### 1.3 The claim that there are zero live producers is WRONG. This is the most consequential error in either report.

B §2.5: "Twelve live consumers, **zero live producers**." A §1.7: "the producers
are still off." Both are false.

The two *compiled-in* producers are indeed dead:
`taichi/rhi/vulkan/vulkan_device_creator.cpp:826` inside `#if !defined(__APPLE__)
&& false` at `:825-827`, and `c_api/src/taichi_vulkan_impl.cpp:45-52` commented
out. Both reports have that right, including A's correction that the second is a
`/* */` comment and not a preprocessor guard.

But there are **two live, user-reachable producers** that bypass both:

- **The Python AOT path.** `python/taichi/aot/module.py:86-110` passes a `caps`
  string list to `make_aot_module_builder`, which reaches
  `taichi/program/program.cpp:513-537` `translate_devcaps`, which calls
  `str2devcap` at `:530` on an arbitrary string. `spirv_has_physical_storage_buffer`
  is a valid name in that table (`taichi/inc/rhi_constants.inc.h:28`,
  `python/taichi/lang/enums.py:38`). The resulting config reaches
  `taichi/runtime/gfx/aot_module_builder_impl.cpp:21`, and from there
  `load_or_compile(config_, caps_, *kernel)` at `:75` and `:119`. **A user can
  build an AOT module with PSB codegen today, on any machine, with no device
  check anywhere in that chain.**
- **The C-API runtime path.** `c_api/src/taichi_core_impl.cpp:317-334`
  `ti_set_runtime_capabilities_ext` builds a `DeviceCapabilityConfig` from
  caller-supplied values and calls `runtime2->get().set_caps(std::move(devcaps))`
  at `:331`, replacing the device's capability set wholesale. The typed setter
  `c_api/include/taichi/cpp/taichi.hpp:1156-1159` exists precisely to make this
  convenient.

B's notes-05b lines 370-372 found both of these paths. The report converted them
into "capability configuration exists today", which reads as machinery-in-waiting
rather than as a live producer. That inversion changes the risk picture: A's
escalation 4 says re-enabling the capability means enabling two years of
never-executed code, and treats that as a future decision. It is not. That code
is reachable by a user today. The `&& false` did not turn the feature off; it
turned off *automatic* enablement.

### 1.4 The intent half of the claim is contradicted by dead code both reports missed

Both reports assert flatly that PSB "never touched the SNode root buffer" (A §1.1)
and "never addressed SNodes. It addressed ndarrays" (B §3). True of live code.
Not true of what is in the tree.

`taichi/codegen/spirv/snode_struct_compiler.cpp:34-62` is a function `construct`
that walks an SNode tree and builds a `tinyir` type for it. At `:52-54`:

```cpp
      if (sn->type == SNodeType::pointer) {
        cell_type = ir_module.emplace_back<PhysicalPointerType>(cell_type);
      }
```

`PhysicalPointerType` is the type whose SPIR-V translation, at
`taichi/codegen/spirv/spirv_types.cpp:433-439`, emits `OpTypePointer` with
`spv::StorageClassPhysicalStorageBuffer` — the storage class that exists only
under the PSB capability. That is the SNode struct compiler proposing to give
`pointer` SNode cells physical device addresses.

It is unreachable. Its only caller is commented out at
`snode_struct_compiler.cpp:16-19`, and the `CompiledSNodeStructs` fields it would
write are commented out at `snode_struct_compiler.h:49-50`. `git log -S
"PhysicalPointerType"` returns exactly one commit, `46b5632e5` "[vulkan] Codegen
& runtime improvements (#5213)", Bob Cao, 2022-06-22 — and the diff shows the
call site was **added already commented out**. It was never live.

Two consequences.

- **B's §2.5 inventory is wrong to list `spirv_types.cpp:433-439` among the live
  consumers.** `PhysicalPointerType` has exactly two construction sites:
  `snode_struct_compiler.cpp:53`, dead, and `spirv_types.cpp:337`, which only
  copies an existing one inside the type reducer. No live producer means the
  visitor at `:433` never runs. B's own count of twelve happens to exclude it;
  the prose does not.
- **The reports' shared conclusion needs a qualifier the planner will care
  about.** "PSB never addressed SNodes" is right about shipped behaviour and
  wrong about design intent. Four months after landing PSB, its author sketched
  an SNode type system in which sparse `pointer` nodes carry physical addresses,
  and shipped the sketch commented out. That is not a reusable asset — it is
  forty lines that never compiled into anything — but it is the only evidence in
  the entire territory that anyone connected PSB to SNodes, and both reports
  concluded no such connection exists.

### 1.5 The separation is clean, but it is not two disjoint routes. It is one route with a type-sniff switch.

Both reports describe two mechanisms. B §3 gets closer, calling `at_buffer` "the
join". The precise shape matters for item 6.2 and neither report states it
plainly:

`at_buffer` at `spirv_codegen.cpp:2194-2219` is the **single** entry point for
every buffer access, ndarray and SNode alike. The branch at `:2197` is
`if (ptr_val.stype.dt == PrimitiveType::u64)`. There is no capability test. The
same test recurs in `load_buffer` at `:2227` and `store_buffer` at `:2249`.

So the two paths are not separate code. They are one function with a
discriminator that reads *the width of the pointer value* as *which addressing
scheme applies*. That is why widening SNode offsets is blocked: the width is
already spoken for as a tag. It is a much sharper statement of the problem than
"they are disjoint", and it is why §2b below is a real finding while §2a is not.

---

## 2. Attack 1: the two obstacles both reports call ARCHITECTURAL

The brief is right that this distinction matters most, because an architectural
obstacle does not expire and an environmental one does. My rulings differ from
both reports on the first and agree on the substance of the second while
rejecting its classification.

### 2a. The whole-module addressing model. VERIFIED AS A FACT, REJECTED AS AN OBSTACLE.

The fact is correct. `taichi/codegen/spirv/spirv_ir_builder.cpp:113-127` emits
exactly one `OpMemoryModel` per module: `AddressingModelPhysicalStorageBuffer64`
under the capability at `:120-123`, `AddressingModelLogical` otherwise at
`:125-126`. One declaration, no per-pointer opt-in. A §1.2 and B §2.5 both
report this accurately.

The inference drawn from it does not hold. A §1.2 says "Turning the capability on
changes the SPIR-V validity rules for every shader in the module", and A §6.4
promotes it to "the one hard, verified obstacle that is not merely absent work".

Two independent refutations.

**Refutation one, from the shipped releases.** I read v1.1.3, a release where the
capability was ON (§6 below):

```
$ git show v1.1.3:taichi/codegen/spirv/spirv_codegen.cpp | grep -n 'use_64bit_pointers\|make_pointer(0)'
74:  const bool use_64bit_pointers = false;
275:    spirv::Value root_val = make_pointer(0);
$ git show v1.1.3:taichi/codegen/spirv/spirv_ir_builder.cpp | grep -n AddressingModel
114:        .add_seq(spv::AddressingModelPhysicalStorageBuffer64,
119:        .add_seq(spv::AddressingModelLogical, spv::MemoryModelGLSL450)
```

In v1.1.3, with `AddressingModelPhysicalStorageBuffer64` declared, `make_pointer`
still returned a 32-bit offset and SNode roots were still bound-buffer relative.
**Upstream shipped mixed addressing under that model, in production, for eight
months.** Declaring the model does not force any pointer to become physical; it
only makes physical pointers legal. The all-or-nothing property is a property of
the declaration, not of the pointers. Whatever blocks item 6.2, this does not.

**Refutation two, from module granularity.** A's phrase "every shader in the
module" implies a module holding several shaders. It does not.
`spirv_codegen.cpp:97` constructs a fresh `IRBuilder` per `TaskCodegen` and
`:151` calls `finalize()` on it, so there is one module per offloaded task. The
blast radius of the declaration is one kernel task, not a program.

What is left after those two refutations is the genuine constraint, which both
reports also state and which is **environmental, not architectural**: declaring
a SPIR-V capability the device did not enable is a spec violation, so the
capability must track real device support. A §1.6 supports this with issue #6368;
see §7.4 below for a correction to that section.

**Ruling: reclassify. The addressing-model switch is not an obstacle to item 6.2
and must not be carried into the plan as one.**

### 2b. The `at_buffer` u64 collision. UPHELD ON FACTS. REJECTED AS ARCHITECTURAL. AND BOTH REPORTS MISSED A THIRD FAILURE MODE.

Verified line by line, and A's account is the accurate one.

- `spirv_codegen.cpp:2197` tests `ptr_val.stype.dt == PrimitiveType::u64`, and
  `:2198-2203` emits `OpConvertUToPtr` to a
  `spv::StorageClassPhysicalStorageBuffer` pointer, tagging the result
  `ValueKind::kPhysicalPtr` at `:2204`.
- `visit(GetRootStmt *)` at `:351-357` registers `make_pointer(0)` at `:355` — a
  literal zero, root-buffer relative.
- With the flag on, that zero becomes a `u64` and every SNode access converts a
  null literal to a device address. A §2.3(a) and B §4.1 both have this right.

**Type mixing in `bitmasked_activation`, `:383-415`. A is right; B is wrong on
three of its four citations.**

A cites `:398-399` (`OpShiftLeftLogical`, Result `ptr_dt`, Base
`ir_->const_i32_one_`), `:410-412` (`OpShiftRightLogical`, Result hardcoded
`ir_->u32_type()`, Base `ptr_dt`-typed) and `:414`
(`struct_array_access(ir_->u32_type(), ...)`). All three read correctly against
the file. A also correctly exempts `:405`, where `u32_type()` is the *shift
amount*: SPIR-V constrains Base and Result to equal bit width and says nothing
about the Shift operand's width. A's exemption is the mark of someone who checked
the rule rather than pattern-matching on `u32_type()`.

B cites `:404`, `:411`, `:412`, `:413` and claims a u32 is added to `parent_ptr`
at `:409`.
- `:404` names `ptr_dt` as the Result type. The `u32_type()` on the following
  line is the legal shift amount. Not a defect.
- `:412` is likewise a shift amount. Not a defect.
- `:413` is an assignment; the `u32_type()` is on `:414`. Off by one.
- `:409` is `ir_->add(parent_ptr, bitmask_word_ptr)`. With the flag on,
  `bitmask_word_ptr` arrives from `:406-408` as `ptr_dt` added to
  `make_pointer(...)`, which is also u64. **Both operands are u64. There is no
  width mismatch at `:409`.** B's claim is wrong.
- B misses `:398-399` entirely, which is the clearest of the three and the only
  one A and I both independently landed on.

B's §4 reason 2 is therefore one correct citation out of five, and it misses the
strongest one. A's §2.3(b) is correct throughout.

**The third failure mode, missed by both.** `make_pointer` at `:2317-2324` calls
`ir_->u64_type()`. That accessor, `spirv_ir_builder.h:532-534`, is a bare
`return t_uint64_;` with no guard. `t_uint64_` is only ever assigned inside
`if (caps_->get(cap::spirv_has_int64))` at `spirv_ir_builder.cpp:166-169`. On a
device without `shaderInt64`, flipping the flag yields a default-constructed
`SType` and an undeclared type id, with no diagnostic.

This directly undercuts B's §7.1, which offers `spirv_ir_builder.cpp:311-313` and
`:325-327` as a safety net that "hard errors if an i64/u64 type is requested
without it". Those guards live in `get_primitive_type(DataType)`. `make_pointer`
does not go through it. **The safety net B points at does not cover the one path
item 6.2 has to widen.** Under plan §2.2 that is the shape that matters: a silent
miscompile on weaker hardware and correct code on stronger, which is hardware
changing capability, not speed.

**Ruling on classification.** The collision is real and I uphold it as a blocker
on flipping the flag. It is not architectural. The discriminator is a
data-type sniff inside one function, and the mechanism that would replace it is
already present in the same file: `spirv::Value` carries a `ValueKind flag` field
(`spirv_ir_builder.h:88`), and `at_buffer` already sets
`ValueKind::kPhysicalPtr` at `:2204`. Tagging at production instead of sniffing
at consumption is a local change to three functions, not a redesign. Calling it
architectural overstates its permanence, and the plan's §6.4 test — justify an
addition against removing something — is easier to satisfy here than either
report implies.

**Neither obstacle survives as architectural. That is a material change to the
picture both reports hand the planner, and it cuts in the project's favour.**

---

## 3. Attack 2: the hard stop on int64 indexing

The brief frames this as A saying the assert and B centring the warning. That
framing does not survive reading B: B §5 names the same assert, quotes the same
replacement of `TI_ASSERT(total_bits <= 30)`, and says "That is a hard assert,
not a warning". There is no real dispute between the reports.

**A is right on the substance.** `taichi/transforms/demote_dense_struct_fors.cpp:29`
is `TI_ASSERT(total_n <= std::numeric_limits<int>::max())`, inside
`convert_to_range_for` which starts at `:13`. `TI_ASSERT` is Taichi's own macro
(`taichi/common/logging.h:100-104`), which throws in release builds; it is not
`assert` and is not compiled out. `taichi/ir/snode.cpp:95-100` is
`ErrorEmitter(TaichiIndexWarning(), ...)`, and `TaichiIndexWarning`
(`taichi/common/exceptions.h:114-117`) derives from `TaichiWarning`. Warning
versus throw. A's reading is correct.

Two qualifications neither report supplies.

**The assert is conditional.** It fires only when `demote_dense_struct_fors` runs
and only for an all-dense struct-for. The flag defaults true at
`taichi/program/compile_config.cpp:18` and is forced true for SPIR-V archs at
`:73-75`, and the pass runs at `taichi/transforms/compile_to_offloads.cpp:191-196`
— so it is on by default everywhere, but a user can set it false, and a sparse
tree does not reach it (`taichi/transforms/offload.cpp:192` requires
`is_path_all_dense`). **For this project that qualification bites**, because plan
§4.1 says sparsity is required. The hard stop A names may not be the one this
fork hits.

**Both reports miss the mechanism that actually corrupts data.** Both quote the
`static_cast<int>` at `snode.cpp:92` and both stop at the comment. Neither reads
the declaration it casts into. `SNode::AxisExtractor::acc_shape` is `int` at
`taichi/ir/snode.h:49`; `shape` is `int` at `:45`;
`num_elements_from_root` is `int` at `:41`. Meanwhile
`SNode::num_cells_per_container` is already `int64` at `:97`.

So the ordering is: `snode.cpp:92` silently truncates into a 32-bit field, `:95`
notices and warns, and `demote_dense_struct_fors.cpp:29` throws later and only
sometimes. The truncation is first, is silent, and is the thing item 6.2 has to
widen. Neither report names `snode.h:41,45,49` at all, and that is the
concrete answer to plan §8.1.1 for this corner. The nearby fact that
`num_cells_per_container` is *already* `int64` is what ailzhang's 2021 argument
about consistency was pointing at, and it makes the widening smaller than either
report suggests.

**Ruling: the hard stop is the assert, conditionally. The binding width is the
three `int` fields at `taichi/ir/snode.h:41,45,49`. Both reports stopped one
line short.**

---

## 4. Attack 3: ruling on `PointerType::addr_space_`

**UPHELD. It is address space, not address width, and it does not belong under
item 6.2.**

Grounds, verified independently:

- `taichi/ir/type.h:207`, `int addr_space_{0};  // TODO: make this an enum`. The
  TODO is about turning an int into an enum, which is a statement about a small
  fixed set of named values. Address *widths* are not an enum; address *spaces*
  are — generic, global, shared, constant, local.
- No writer. Both constructors, `type.h:179` and `:181-185`, omit it. `grep -rn
  "addr_space_\|get_addr_space" taichi/ c_api/ python/ tests/` returns four hits
  and all four are in `type.h`: the getter at `:191-193`, the serialisation macro
  at `:203`, and the field at `:207`. Zero call sites of `get_addr_space` outside
  its own body.
- The working analogue is `int addr_space` at
  `taichi/codegen/llvm/codegen_llvm.h:165`, consumed by
  `TaskCodeGenLLVM::cast_pointer` at `codegen_llvm.cpp:1128-1133`, feeding
  `llvm::PointerType::get(get_runtime_type(dest_ty_name), addr_space)` at
  `:1132`. That is LLVM's address-space parameter, which selects a memory space
  and has nothing to do with pointer width. Field and parameter are unconnected.
- `dcd5d7d35` (#1948, Yuanming Hu, 2020-10-12) created the field with the getter
  and no setter. It has never been read.

Both reports reach this ruling. A's line citations are right throughout. B's are
right for the getter and the field but wrong for the constructors, which B places
at `:184` and `:186-191`; they are at `:179` and `:181-185`.

One nuance neither raises, and it is the only reason the field is not entirely
inert: it appears in `TI_IO_DEF(pointee_, addr_space_, is_bit_pointer_)` at
`type.h:203`, so it is serialised into the offline cache and AOT type records. It
is always zero, so nothing observable follows, but "zero call sites, ever"
(B §6) is not literally true — the serialiser touches it.

**Recommendation to the planner: strike `PointerType::addr_space_` from item 6.2.
Both explore passes, and this adversary, independently reach that conclusion, and
no source or history contradicts it.**

---

## 5. Attack 4: the version ranges. Both reports are wrong.

Resolved by reading the file out of every tag in the repository rather than by
ancestry tests. Script: for each tag, extract
`taichi/rhi/vulkan/vulkan_device_creator.cpp` (falling back to the pre-refactor
path `taichi/backends/vulkan/`), find the `caps.set(...)` line, and classify by
the guard four lines above it.

| Tags | State |
|---|---|
| v0.8.7 – v0.8.11 | capability name absent from the file |
| **v0.9.0, v0.9.1, v0.9.2, v1.0.0, v1.0.1, v1.0.2, v1.0.3, v1.0.4, v1.1.0, v1.1.2, v1.1.3** | **ON, unguarded** |
| v1.2.0, v1.2.1, v1.2.2 | ON except macOS |
| v1.3.0, v1.4.0, v1.4.1, v1.5.0, v1.6.0, v1.7.0, v1.7.1, v1.7.2, v1.7.3, v1.7.4 | OFF globally |

Eleven tags shipped it fully on, not four. The live window opens at **v0.9.0**,
dated 2022-02-22 — a fortnight after `7705f688a` on 2022-02-08.

- **B §2.1 is wrong.** Its table lists v1.0.1, v1.0.4, v1.1.0, v1.1.3 as the
  enabled set. It omits v0.9.0, v0.9.1, v0.9.2, v1.0.0, v1.0.2, v1.0.3 and
  v1.1.2. Sampling four tags and presenting the result as a table of the range is
  the error. It also omits v1.4.1, v1.5.0, v1.6.0, v1.7.1, v1.7.2 and v1.7.3 from
  the disabled set. There is no v1.1.1 tag in the repository.
- **A §1.3 is thin rather than wrong.** "v1.2.0/v1.2.1/v1.2.2 shipped it live
  (macOS excepted); v1.3.0 through v1.7.0 ship it dead" is accurate for the tags
  it names, but it names no start point and stops at v1.7.0 when v1.7.4 exists
  and is also dead.

The "eight months" figure both reports use is right: 2022-02-08 to the macOS
carve-out on 2022-10-24 is eight and a half months, to the global disable on
2022-10-30 slightly more.

**Ruling: neither range is usable as written. The correct statement is that the
capability was unconditionally on across eleven tagged releases from v0.9.0 to
v1.1.3, macOS-excepted across three from v1.2.0 to v1.2.2, and off across ten
from v1.3.0 to v1.7.4.** This strengthens both reports' shared conclusion that
the feature was working production code, and it strengthens my §2a refutation,
since all eleven of those releases shipped mixed addressing.

---

## 6. Attack 5: momentum loss from silence

Both reports label this an inference. B does so explicitly and well
(§9 INFERRED 2: "Argument from silence; weigh it as such"). A states it as
VERIFIED in §1.7 — "The evidence supports 'momentum lost' as the reason it stayed
off" — which is a stronger word than the evidence carries.

**What is verified and supports it:**

- The stated precondition was met. B's chain of devcap follow-ups all check out
  against `git`: `29749416a` #6549 2022-11-15, `a0227ca89` #6618 2022-11-16,
  `fdde622a5` #6623 2022-11-16, `c27a2e474` #6702 2022-11-30, reverted
  `3711e8b18` #6772 2022-11-30, re-landed `bed652dc2` #6773 2022-12-07,
  `285fe5213` #7407 2023-02-21, `fcd46769e` #7506 2023-03-11. Eight hashes, eight
  correct.
- No ref anywhere re-enables it. I ran my own sweep over all 229 remote refs plus
  all tags. Only `upstream/rc-v1.2.0`, `upstream/bump/v1.2.3` and `v1.2.2` carry
  a setter without `&& false`, and all three carry the macOS guard, i.e. they are
  pre-global-disable code. Both reports' sweeps agree, and mine confirms them.
- Commit-rate collapse on master: 1841 in 2022, 910 in 2023, 28 in 2024, 37 in
  2025. Matches plan §2.1.
- `#5979 [AOT] Device capability management for AOT (Tracker)` is open, has one
  comment, by PENGUINLIONG on the day he opened it. A §1.7 states this exactly
  right.

**What cuts against it, and neither report weighed:**

`715a04c98`, "[vulkan] Fix shared memory atomic float operations (#8315)", Bob
Cao, **2023-08-15** — ten months after the disable — rewrote the dead
`use_64bit_pointers` true branch, replacing a compute-in-u32-then-widen with a
direct u64 immediate. The commit message says it fixes a bug in `make_pointer`.
Somebody went back into unreachable code and corrected it. That is not the
signature of abandonment; it is the signature of somebody keeping a branch alive
against a later return.

A found this commit and cited it accurately in §2.1, then did not connect it to
its own momentum argument in §1.7. B did not find it at all, and its §4 VERIFIED
item 7 — "`use_64bit_pointers` has exactly one commit in its entire history" — is
true of the *flag* and invites the false reading that the *code* is untouched
since 2021. B's §4 quotes the current body correctly, so the error is one of
framing, not of fact.

PENGUINLIONG's last commit on master is 2023-09-28 (#8275, AOT multi-device).
The people involved kept working for roughly a year after the disable and did not
restore it.

**Ruling: momentum loss is the most probable reading and I do not reject it, but
it is an inference and A should not have labelled it VERIFIED. The honest
statement is that the precondition was met, nobody lifted the guard, and one
author was still maintaining the adjacent dead code as late as August 2023.** For
the planner's purposes the distinction is small: either way, no upstream
statement anywhere says the approach is wrong, and both reports are right that
none exists.

---

## 7. Attack 6: ten commits, no coverage

**Count: correct. Zero coverage: correct. Date range: wrong.**

`git log --since=2023-01-01 -S 'has_buffer_ptr' --date=short master` returns
exactly ten commits, matching A §1.8:

```
7809ebbd5 2023-07-13  [refactor] Refactor code repetition for get argpack layout (#8278)
14e83c484 2023-07-10  [lang] Argpacks stores scalar values only ...
cfad91fc8 2023-07-10  [spirv] [ir] Support argpack buffer load for spir-v backends
29cfb5c72 2023-07-10  [lang] Instantiate a runtime ArgPack object ...
b66279b0b 2023-07-10  [refactor] Enhance argument data structures ...
32518fc52 2023-06-13  [refactor] Add base class GfxProgramImpl
88ac098bc 2023-05-23  [spirv] [ir] [lang] Support struct object as return value in spir-v (#8061)
748abdbcc 2023-04-25  [Lang] Let kernel argument support matrix nested in a struct
457ada6a6 2023-04-19  [spirv] Support struct as kernel argument
4c33922df 2023-04-06  [gfx] Compile struct type of result and arguments in gfx backends
```

A's two named examples, `88ac098bc` (#8061, 2023-05-23, struct return values) and
`cfad91fc8` (2023-07-10, argpack buffer load), are both in the list and both
described correctly.

**The date range is wrong.** A §1.8 says the code "kept being extended through
the 2023-2024 argument-refactor work", and escalation 4 says "extended by ten
commits through 2023-2024". The newest of the ten is 2023-07-13. There is nothing
in 2024. Widening the search to `-S 'spirv_has_physical_storage_buffer'` adds
nothing after 2023-07-10 either; the last capability-touching commit of any kind
is `4a8709294`, a documentation change, 2023-02-14. **Every one of the ten falls
in a fifteen-week window, April to July 2023, and all ten are argument, argpack
and return-struct refactors.** That is a tighter and more useful fact than A's
version: it is one refactor campaign sweeping through, not two years of drift.

**Zero test coverage: confirmed.** `grep -rn
'physical_storage_buffer\|PHYSICAL_STORAGE_BUFFER\|buffer_device_address' tests/
c_api/tests/` returns nothing. No test names the capability, and since no
producer sets it automatically, no test can reach the branch.

**Bearing on the cost of re-enabling.** A's framing — "enabling roughly two years
of never-executed code" — overstates the drift and understates the exposure. The
accurate version: the untested branch has stood for **34 months** since
2022-10-30 and was last modified 2023-07-13, so it has had 25 months of settling.
But because of §1.3, that branch is not sealed off pending a decision. Any user
who names the capability in `ti.aot.Module(caps=[...])` compiles it today.

---

## 8. Consolidated error list

Errors that change a conclusion are marked **MATERIAL**.

### Report 05 (pass A)

1. **MATERIAL.** §1.7 "the producers are still off". Two live producers exist
   (§1.3 above).
2. **MATERIAL.** §1.2 and §6.4, the module-wide addressing model as a hard
   obstacle. Refuted by v1.1.3 and by per-task module granularity (§2a).
3. **MATERIAL.** §1.1 and §0, "It never touched the SNode root buffer" stated
   without qualification. True of live code; contradicted as to intent by
   `snode_struct_compiler.cpp:52-54` (§1.4).
4. **MATERIAL, attribution.** §3.1 reverses the PR #3177 exchange. A writes that
   k-ye proposed enforcing the int fit and that "ailzhang overruled that: 'I think
   it's still better to expand this to 64-bits.'" I read the thread: that
   sentence is the closing line of **k-ye's own comment**, dated 2021-10-14, in
   which k-ye quotes the enforce-the-fit proposal and then rejects it. ailzhang's
   only review comment on that PR
   (`repos/taichi-dev/taichi/pulls/3177/comments`, id 733333856) is about LLVM
   versus C++ casting and says nothing about widening. ailzhang never wrote the
   quoted sentence. A calls this "the clearest evidence of intent in the whole
   territory", so getting the speaker backwards matters — though the substance
   survives: a maintainer did argue for widening in October 2021 and it never
   happened.
5. **MATERIAL, attribution.** §1.6 says issue #6368 was "opened 2022-10-18 by
   PENGUINLIONG". It was opened by **k-ye**. A then cites "k-ye's comment", which
   is right — the sole comment is k-ye's — so the section has one person in two
   roles. Separately, the validation error k-ye pasted names
   **`AtomicFloat64AddEXT`**, not `PhysicalStorageBufferAddresses`. A's "That is
   exactly the failure shape for this capability" is a class-similarity argument,
   correctly flagged INFERRED, but readers should know the evidence is about a
   different capability.
6. §1.5 "neither PR has a single line of human review discussion. The only
   comments on either are from the Netlify bot." PR #6494 carries an APPROVED
   review by ailzhang with the body "Thanks!"; PR #6468 carries an APPROVED review
   by ailzhang with an empty body. The issue-comment threads are indeed
   bot-only. The claim as written is false; the point it serves — no design
   discussion — stands.
7. §1.8 and escalation 4, "through 2023-2024". Nothing after 2023-07-13 (§7).
8. §1.7 labels momentum loss as supported when it is an inference (§6).
9. §4.1 cites `codegen_llvm.cpp:1131` for the parameter and `:1133` for
   `llvm::PointerType::get`. The function spans `:1128-1133`; the
   `PointerType::get` call is at `:1132` and `:1133` is the closing brace.
10. §1.1's consumer list omits `spirv_types.cpp:484-497` and
    `runtime.cpp:823,851-859` (§1.2).
11. §3.1 does not note that issue #6758 is **closed**.

A's per-line accuracy inside `spirv_codegen.cpp` is otherwise the best in either
report: `:355`, `:371`, `:408`, `:506`, `:2194`, `:2197`, `:2199`, `:2227`,
`:2249`, `:398-399`, `:405`, `:410-412`, `:414` all check out, and the `:405`
exemption is correct SPIR-V.

### Report 05B (pass B)

1. **MATERIAL.** §2.5 "Twelve live consumers, **zero live producers**." (§1.3).
2. **MATERIAL.** §2.5 lists `spirv_types.cpp:433-439` among live consumers. It is
   unreachable; its only non-copy producer is dead code (§1.4).
3. **MATERIAL.** §3 "never addressed SNodes", same qualifier as A's item 3 (§1.4).
4. **MATERIAL.** §7.3, "`data64` … its only uses in the tree are three Python test
   gates." `grep -rn data64 tests/ python/` returns **more than fifty** uses
   across at least twenty-five test files — `test_svd.py`, `test_ad_basics.py`,
   `test_pow.py`, `test_types.py`, `test_ndarray.py` and others. B builds
   escalation 5 on this count. The escalation's underlying point — that no C++
   code consults `data64` while `spirv_has_int64` says the device can do it — is
   sound, but the supporting count is wrong by more than an order of magnitude.
5. **MATERIAL.** §7.1 offers `spirv_ir_builder.cpp:311-313` and `:325-327` as the
   guard preventing a u64 request without `shaderInt64`. `make_pointer` reaches
   `u64_type()` (`spirv_ir_builder.h:532-534`) directly and bypasses it (§2b).
6. §4 reason 2: `:404`, `:412` and `:409` are not defects, `:413` is off by one,
   and the strongest instance at `:398-399` is missed (§2b).
7. §2.1's tag table is a four-tag sample presented as a range, and it starts three
   releases too late (§5).
8. §6 places the `PointerType` constructors at `:184` and `:186-191`; they are at
   `:179` and `:181-185`.
9. §6 "Zero call sites, ever" — the serialisation macro at `type.h:203` reads it.
10. §7.3 lists `sparse` among the extensions with C++ callers of
    `is_extension_supported`. The C++ callers are `assertion`
    (`program.cpp:149`), `bls` (`codegen_llvm.cpp:2726`), `mesh` and `quant`
    (`compile_to_offloads.cpp:92,205,218,236,245,288`). `sparse` is consulted only
    from Python.
11. §4 does not mention `715a04c98`, and "born false, never edited" invites the
    reading that the true branch is untouched since 2021 (§6).
12. §2.5's consumer list omits `spirv_types.cpp:484-497` and
    `runtime.cpp:823,851-859` (§1.2).

B's strengths, verified: all eight devcap follow-up hashes in §2.4 are correct;
the `c_api/src/taichi_gfx_impl.cpp:22-29` equality-not-floor reading is correct
and is the sharpest single observation in either report; the GitHub issue survey
in §5.1 is accurate on every issue I checked — #8608 open with two comments from
lstrgar and bgailleton and no maintainer reply, #5320 open, #8161 with listerily's
2023-06-09 comment quoted verbatim; the `Extension::data64` grant table at
`extension.cpp:9-28` and the commented-out OpenGL grant at `:29-30` are correct;
§8's refusal to treat issue #4919 as evidence is the right call.

---

## 9. Where the two reports disagree, and who the source supports

| Question | A | B | Source supports |
|---|---|---|---|
| Why disabled globally | Precaution pending devcap rebuild, cause = "multi-platform compatibility issues", referent inferred as #6368, a host-integration problem | AOT cross-machine portability, inferred from the `[aot]` tags and the equality check at `taichi_gfx_impl.cpp:22-29` | **B, on the stronger chain.** B's evidence is inside the repository and mechanical: every disabling commit is tagged `[aot]`, and a module compiled with PSB genuinely cannot load on a runtime reporting a different level. A's #6368 is an open issue about `create_vma_allocator` on imported devices whose only comment concerns a different capability. Neither is proven; B's is better supported. Both should be recorded as INFERRED, which both do. |
| Tag range | thin, no start | sampled, starts too late | **Neither. See §5.** |
| `bitmasked_activation` breakage | correct on all three sites | one of five citations correct | **A.** |
| The hard int64 stop | assert at `demote_dense_struct_fors.cpp:29` | names both | **A on substance; both miss `snode.h:41,45,49`.** |
| PR #3177 intent exchange | quoted, speaker reversed | not covered | **Neither. k-ye wrote the widening argument.** |
| `use_64bit_pointers` history | one commit for the flag, plus `715a04c98` on the branch | one commit, "never edited" | **A.** |
| Machinery integrity | not claimed | "Nothing was ripped out" | **Neither.** `from_taichi_type` is orphaned and `construct` is entombed. |

---

## 10. Escalations

Not settled by assumption. For the planner.

1. **The "zero live producers" claim must not enter the plan.** A user can build
   a PSB AOT module today via `ti.aot.Module(caps=["spirv_has_physical_storage_buffer"])`
   with no device validation anywhere between `python/taichi/aot/module.py:110`
   and `aot_module_builder_impl.cpp:75`. Whether that is a bug to be closed or a
   door to be kept open bears directly on item 6.3. It is a decision, not a
   finding, and I am not making it.

2. **Both "architectural" obstacles fail as architectural.** §2a is refuted
   outright; §2b is a real blocker but a local one. I am ruling on the
   classification because the brief asked me to. I am not ruling on whether the
   local change should be made — the value tagging in §2b touches `at_buffer`,
   `load_buffer` and `store_buffer`, and under plan §6.4 that needs the planner's
   judgement, not mine.

3. **The `u64_type()` gap is a plan §2.2 problem, not a codegen detail.**
   `make_pointer` can request an undeclared u64 type on a device without
   `shaderInt64`, silently. Whatever widening item 6.2 chooses must route through
   `get_primitive_type` or add an equivalent guard, or the fork ships correct
   code on a 1070 and silently wrong code on a device lacking the feature. That is
   hardware changing capability, which §2.2 forbids. I have not checked whether
   the GTX 750 or 1070 expose `shaderInt64`; that gates the question and neither
   report checked it either (A escalation 6, B not-established 2).

4. **The sparse qualification on the hard stop is unresolved.** The assert at
   `demote_dense_struct_fors.cpp:29` fires only for all-dense struct-fors
   (`offload.cpp:192`). Plan §4.1 requires sparsity. Whether the sparse path has
   its own 32-bit ceiling, and where, is not answered by either report and is not
   in my brief. It belongs to territory 02 or 03 and I flag it because A presents
   the assert as *the* operative constraint.

5. **`snode_struct_compiler.cpp:34-62` needs a disposition.** It is forty lines of
   dead SNode type construction that would give sparse `pointer` nodes physical
   device addresses. Per plan §10.3 I am not calling it unnecessary and I am not
   calling it an asset. It is the single piece of evidence in this territory
   connecting PSB to SNodes, and both explore passes missed it, so the planner
   should decide what it means before item 6.2 is sized.

6. **The PR #3177 attribution should be corrected before the reports are read
   together.** The widening argument was k-ye's. A attributes it to ailzhang and
   builds "the clearest evidence of intent in the whole territory" on it. The
   conclusion survives; the sentence does not.

7. **I compiled and ran nothing.** Every claim about what flipping
   `use_64bit_pointers` would do, including the `u64_type()` gap in item 3 above,
   is read from source. My §2a refutation is different in kind: it rests on
   shipped release artefacts rather than on prediction, and I regard it as settled
   rather than inferred.

8. **The macOS root cause is still unlocated** and both reports say so. I add
   nothing. It is out of scope for plan §5.2's tier table.

---

## 11. Divergence from adversary-05-1

`/opt/project/taichi/modernization/investigation/adversary-05-1.md` did not exist
when I finished, so there is nothing to compare against and nothing to append.

---

# 12. Appendix: divergence from adversary 05-1

`adversary-05-1.md` did not exist when §11 above was written. It landed
afterwards. I read it in full and re-checked every point where we differ. §11 is
left as written; this section supersedes it.

We worked independently and the overlap is substantial. Where we converge on
something both explore passes missed, that is worth more than either of us saying
it alone. Where we diverge, I say which way the source falls and I checked it
again before writing.

## 12.1 Independent convergence

Reached separately, same conclusion, and in three cases the same evidence:

- **The module-wide addressing model is not an architectural obstacle.**
  Adversary 1 §3 and my §2a. We both refuted it from the shipped releases, and we
  both independently noticed that "every shader in the module" is one shader
  because `spirv_codegen.cpp:97` builds an `IRBuilder` per task and `:151`
  finalizes it. Two adversaries arriving at the same refutation by the same route
  should settle this. **Both reports must be corrected.**
- **The three missing consumers.** Adversary 1 §2.2 and my §1.2 name the same
  three: `runtime.cpp:851-859`, `spirv_types.cpp:484-497`,
  `spirv_ir_builder.cpp:334-342`. Neither of us saw the other's list. That is the
  shared-blindness signal the brief asked for, and it is now confirmed twice.
- **The date range on the ten commits.** Both of us found nothing in 2024;
  report A's "2023-2024" is wrong. Adversary 1 went further and checked
  `git log --since=2024-01-01 -- taichi/codegen/spirv/`, finding three unrelated
  commits. I confirm that result.
- **`Extractor::acc_shape` at `taichi/ir/snode.h:49` is the load-bearing 32-bit
  field**, and the truncation at `snode.cpp:92` precedes the warning at `:95`.
  Both of us found this and neither explore pass did.
- **The `demote_dense_struct_fors` assert is gated on `is_path_all_dense`**, so
  it does not fire on a sparse tree, which plan §4.1 requires. Both of us.
- **`PointerType::addr_space_` should leave item 6.2.** Both of us, plus both
  explore passes. Four independent agreements. Treat as settled.
- **Report B's `:409` claim is wrong and report A's `:398-399` is right.** Both
  of us re-read the function and reached the same verdict on every citation.
  Adversary 1 adds the mechanism I did not check: `ir_->add` is
  `DEFINE_BUILDER_BINARY_USIGN_OP` at `spirv_ir_builder.cpp:1038-1047` and opens
  with `TI_ASSERT(a.stype.id == b.stype.id)` at `:1040`, so a genuine mismatch
  would be a host-side assertion during codegen, not invalid SPIR-V. I verified
  that and it is correct. It sharpens the point.

## 12.2 Findings adversary 1 has that I do not, all of which I verified

I checked each rather than accepting it.

1. **§2.3, the return-layout ABI mismatch. Correct and it is the best single
   find in either adversary file.** `gfx_program.h:75-77` returns the literal
   `"4-"` for the kernel return layout with no capability read, while
   `compile_ret_struct` at `spirv_codegen.cpp:2490-2494` does read the capability
   and threads it into `translate_ti_type`. With the capability on, the shader
   would lay out a return-struct pointer member at 64 bits and the host at 32.
   Verified line for line. Adversary 1's hedge — that whether a return struct can
   hold a `PointerType` is a frontend question, and plan §1.2 puts the frontend
   out of scope — is the right hedge.
2. **§2.5, the capability does not widen ndarray indexing.** Verified.
   In `visit(ExternalPtrStmt *)` the offset is `i32` throughout:
   `spirv_codegen.cpp:737` initialises it, `:770-771` accumulate,
   `:773-775` shift with an `i32` result type, and only `:790` widens via
   `OpSConvert` before adding to the `u64` base. So the capability buys a 64-bit
   base plus a 32-bit signed byte offset. Both explore passes overcredit it. I
   missed this and it is a real correction.
3. **B4, "makes every module portable at a stroke" is false.** Verified. The same
   equality check at `taichi_gfx_impl.cpp:22-29` still bites on other
   device-derived capabilities, set unguarded at
   `vulkan_device_creator.cpp:628` (`spirv_has_int16`), `:632`
   (`spirv_has_int64`), `:636` (`spirv_has_float64`) and `:659`, `:663`, `:667`,
   `:671` (the four subgroup capabilities). Turning PSB off does not make AOT
   modules portable; it removes one of at least seven sources of mismatch. This
   genuinely weakens report B's own inference and I did not spot it.
4. **§2.4, `maxStorageBufferRange` is never queried.** Verified:
   `grep -rn maxStorageBufferRange taichi/ c_api/` returns nothing. I have a
   qualification, in §12.3 item 4.

## 12.3 Where we diverge

### 1. The `at_buffer` u64 collision: architectural or not. **I say not. The source is on my side.**

This is our substantive disagreement. Adversary 1 §4 upholds it as architectural
and rests the ruling on one sentence: "The type carries two incompatible meanings
and the code has no third thing to disambiguate them."

**There is a third thing, and adversary 1 quotes the line that sets it.**
`spirv::Value` carries a `ValueKind flag` member at
`taichi/codegen/spirv/spirv_ir_builder.h:88`, defaulting to `ValueKind::kNormal`.
`at_buffer` already assigns `paddr_ptr.flag = ValueKind::kPhysicalPtr` at
`spirv_codegen.cpp:2204` — adversary 1 cites exactly that line, two paragraphs
above the sentence denying such a mechanism exists.

I checked that the flag survives the round trip, because a per-value tag is only
a discriminator if it does. `IRBuilder::register_value` at
`spirv_ir_builder.cpp:1300-1309` stores the whole `Value` into `value_name_tbl_`
at `:1308`, and `query_value` at `:1311-1317` returns it by value at `:1314`. The
flag rides along. `new_value` at `spirv_ir_builder.h:520-526` sets it at
construction, and `:295` already uses the same field to mark
`ValueKind::kVariablePtr`. **The codebase already discriminates pointer kinds by
a tag rather than by scalar type in two places; `at_buffer` is the one that does
not.**

So the correct statement is that `at_buffer` sniffs the data type as a shortcut
where a tag already exists and is already populated on the physical branch.
Tagging at production instead of sniffing at consumption is a local change to
`at_buffer`, `load_buffer`, `store_buffer` and the four `make_pointer` call
sites. That is design work, and I agree with adversary 1 that it is not a flag
flip. It is not a property that survives independent of time, hardware or
decision, which is what the planner's "architectural" means.

Upstream's own record supports reading it as a known-fragile shortcut rather than
a structural commitment. The human-written summary of PR **#8315**, Bob Cao,
2023-08-15 — the commit adversary 1 and I both discuss for other reasons — is:

> With the specific case of shared memory atomic floats, the `at_buffer` command
> will be called with wrong pointer dtype. Causing the codegen attempting to
> generate offset arithmetic with actual pointer type, which is invalid

That is a bug report about the dtype discriminator itself, filed by the author of
the mechanism, fixed locally in one function. A design commitment that cannot be
disambiguated does not get fixed that way.

**I do not dispute that flipping the flag breaks the build. Adversary 1, both
explore passes and I all agree on that and it is verified four times over. I
dispute the classification only, and the classification is what the brief says
matters, because architectural obstacles do not expire.** By my ruling neither of
the two claimed obstacles is architectural, and item 6.2 on the SPIR-V path is
gated by device capability negotiation and by local design work, not by anything
permanent. That is a materially more favourable picture than either report or
adversary 1 hands the planner, and I am stating it knowing that agreeable
findings are the ones that most need checking. I checked it three ways: the flag
field, its propagation, and upstream's own treatment of the same discriminator.

### 2. Live producers. **Adversary 1 repeats both reports' error, and files it under what it could not break.**

Adversary 1 §11 item 5: "No live producer on any ref after 2022-11-02.
Reproduced A's sweep independently over all 350 refs." §8: "A caller can declare
the requirement today. The producer is still `&& false`, three and a half years
later."

The ref sweep is correct and I reproduced it too. But it answers a narrower
question than the sentence claims. It shows no *committed source line* sets the
capability. It does not show no producer exists, and §8's own observation — that
a caller can declare the requirement — is one step from the counter-example
without taking it.

My §1.3 has the chain in full. Two live paths:

- `python/taichi/aot/module.py:110` → `Program::make_aot_module_builder` →
  `translate_devcaps` at `taichi/program/program.cpp:513-537`, which calls
  `str2devcap` at `:530` on an arbitrary user string →
  `aot_module_builder_impl.cpp:21`, then `load_or_compile(config_, caps_, *kernel)`
  at `:75` and `:119`. The capability name is valid input
  (`taichi/inc/rhi_constants.inc.h:28`). **PSB codegen, on demand, on any
  machine, with no device check in the chain.**
- `c_api/src/taichi_core_impl.cpp:317-334`, `ti_set_runtime_capabilities_ext`,
  which calls `runtime2->get().set_caps(std::move(devcaps))` at `:331` and
  replaces the device's capability set wholesale.

This is the one place where all four documents in this territory agree on
something false. Both explore passes, and now both adversaries, describe a
feature that is off. It is not off; it is un-defaulted. For a project whose plan
§5 puts configuration at install time and whose §6.3 wants adaptive module
loading, the difference between "disabled" and "not enabled by default, but
user-settable with no validation" is the whole question.

### 3. The tag census. **Adversary 1's is three tags short. Mine is complete.**

Adversary 1 §7 reports 21 tags containing the capability and gives 8 enabled,
starting at v1.0.0. It explicitly handles the file's move from
`taichi/backends/vulkan/` to `taichi/rhi/vulkan/`, which is why it caught
v1.0.0–v1.0.3 where a path-fixed sweep would not. It then stops.

**v0.9.0, v0.9.1 and v0.9.2 also ship it enabled.** I read each out explicitly:

```
$ git show v0.9.0:taichi/backends/vulkan/vulkan_device_creator.cpp | grep -B6 -n spirv_has_physical_storage_buffer
665-        if (device_supported_features.shaderInt64) {
666-          ti_device_->set_cap(
667:              DeviceCapability::spirv_has_physical_storage_buffer, true);
```

Same at v0.9.1 `:677-678` and v0.9.2 `:677-678`, in all three cases with no
`__APPLE__` and no `&& false`. v0.9.0 is dated 2022-02-22, a fortnight after
`7705f688a`, so it is the first release to carry the feature and the natural
start of the range.

The correct totals are **24 tags, 11 enabled, 3 macOS-excepted, 10 disabled**,
against adversary 1's 21 / 8 / 3 / 10. Adversary 1's disabled row is right and
mine agrees with it exactly. Our two censuses differ only in the v0.9.x window.

This strengthens the refutation both of us make in §12.1: eleven tagged releases,
not eight, shipped `AddressingModelPhysicalStorageBuffer64` alongside 32-bit
bound-buffer SNode addressing.

### 4. `snode_struct_compiler.cpp:52-54`. **Neither adversary 1 nor either explore pass found it, and it is the missing evidence for adversary 1's own §2.4.**

My §1.4. `taichi/codegen/spirv/snode_struct_compiler.cpp:34-62` builds a type for
an SNode tree and at `:52-54` wraps a `pointer` SNode's cell in a
`PhysicalPointerType` — the type that translates to
`spv::StorageClassPhysicalStorageBuffer` at `spirv_types.cpp:433-439`. Its caller
is commented out at `:16-19`, its result fields are commented out at
`snode_struct_compiler.h:49-50`, and `git log -S PhysicalPointerType` returns one
commit, `46b5632e5` (#5213, Bob Cao, 2022-06-22), whose diff adds the call site
already commented out.

Two things follow, and the second is the one adversary 1 will want.

First, **B's §2.5 wrongly lists `spirv_types.cpp:433-439` as a live consumer.**
`PhysicalPointerType` has two construction sites: this dead one, and
`spirv_types.cpp:337` inside the type reducer, which only copies an existing
instance. No live producer, so the visitor never runs. Adversary 1 §2.2 mentions
B's citation of `:433-439` in passing and does not flag it as dead.

Second, and more useful: **adversary 1's §2.4 argues that PSB is structurally
prerequisite for any SNode root that outgrows a descriptor-bound buffer, and
marks the Vulkan half INFERRED because "Taichi has no source line on this."
Taichi has a source line on this.** It is `snode_struct_compiler.cpp:53`. Four
months after landing PSB, its author sketched exactly the design adversary 1
reasons its way to from the SPIR-V spec, applied it to sparse `pointer` nodes,
and shipped it commented out. That does not make the argument verified — the code
never ran and cannot be run — but it moves adversary 1's inference from
"deduced from the spec" to "deduced from the spec and independently arrived at by
the person who wrote the mechanism." I regard that as a meaningful upgrade to
adversary 1's §2.4 and to its Escalation 1.

It also puts a qualifier on the convergent claim that neither adversary stated:
"PSB never touched SNode addressing" is true of live code and false of intent.

### 5. `make_pointer` bypasses the `shaderInt64` guard. **Neither of us reported this; I did, adversary 1 did not, and it bears on adversary 1's §4.**

My §2b. `make_pointer` at `spirv_codegen.cpp:2317-2324` calls `ir_->u64_type()`,
which at `spirv_ir_builder.h:532-534` is a bare `return t_uint64_;`. `t_uint64_`
is assigned only inside `if (caps_->get(cap::spirv_has_int64))` at
`spirv_ir_builder.cpp:166-169`. The `TI_ERROR` guards at `:311-313` and `:325-327`
live in `get_primitive_type(DataType)`, which `make_pointer` does not call.

So on a device without `shaderInt64`, flipping the flag yields a
default-constructed `SType`, an undeclared type id, and no diagnostic. This is a
third failure mode alongside the two both reports give, it is the only silent one
of the three, and under plan §2.2 it is the worst shape available: correct output
on one card, silently wrong output on a weaker one.

### 6. Momentum. **Adversary 1 upgrades it from inference to affirmative evidence. I partly concede and hold one reservation.**

Adversary 1 §8 argues the record is not silence but unkept explicit commitments
plus unanswered direct requests, and tabulates seven artefacts. I verified the
ones I had not already checked: issue #5320's comments are `jim19930609`
2022-07-08, then `lstrgar` 2025-09-23, `lstrgar` 2026-02-13, `yojeep` 2026-05-09
— nearly four years with no maintainer reply. Issue #8608's two comments are both
non-maintainers. That is affirmative and adversary 1 is right that it is stronger
than an argument from silence. **I move my §6 ruling toward adversary 1's and
withdraw the implication that report B was right to hedge as hard as it did.**

My reservation is narrower than adversary 1's treatment of it. Adversary 1
weighs `715a04c98` low on the ground that its "fixes a bug in the `make_pointer`
function" line is Copilot-generated boilerplate. **That is correct** — I read the
PR body and the line sits under a `copilot:walkthrough` block headed "Generated
by Copilot at 0eec54b". But the *human-written* Brief Summary above it is quoted
in §12.3 item 1 and is about `at_buffer` receiving the wrong pointer dtype. So
the human content of #8315 is about the discriminator, not about the dead branch.
That makes adversary 1's dismissal right on the changelog and slightly too quick
on the commit: somebody was thinking about this exact mechanism in August 2023.
It does not overturn the momentum ruling. It does mean the last engaged look at
this code is later than a pure momentum reading implies.

## 12.4 Net

We agree on more than we differ on, and the two places both explore passes are
wrong — the module-wide addressing model, and the three missing consumers — are
now found twice independently. The planner should treat those as settled.

Three things remain open between us, and I have stated my grounds for each:

1. **Whether the `at_buffer` collision is architectural.** Adversary 1 says yes;
   I say no, on the `ValueKind` field at `spirv_ir_builder.h:88`, its propagation
   through `register_value`/`query_value`, and its existing use at
   `spirv_codegen.cpp:2204`. This is the arbiter's call and it changes how item
   6.2 is sized.
2. **Whether live producers exist.** I say yes, with the chain in §1.3, and no
   document in this territory has yet said so. This one is not a matter of
   judgement; it is a code path, and it can be closed by reading the six files I
   cite.
3. **What `snode_struct_compiler.cpp:52-54` means.** Per plan §10.3 I am not
   calling it surplus and not calling it an asset. It is the only line in the
   tree connecting PSB to SNodes and it should be on the planner's desk before
   item 6.2 is sized.

---

# 13. Attack on the expiry material (report 05 §7, report 05b Part II)

Added after a second extension to the brief. Sections 1 to 12 stand unchanged.

Both reports grew: report 05b gained sections 11 and 12, report 05 gained section
7 and rewrote its escalations. Both were judged. I re-measured the hardware
myself rather than accepting either account, and I re-read every code and
submodule citation.

## 13.0 Verdicts

| # | Claim | Verdict |
|---|---|---|
| 1 | GTX 750 Ti, GTX 1070 and llvmpipe all report `shaderInt64` and `bufferDeviceAddress` true, extension present | **UPHELD, re-measured independently.** B's table is exact to the row, including llvmpipe lacking the EXT alias. The code-path distinction needs sharpening: the guard passes on API version alone and never reads the feature bit on this box. |
| 2 | The Vulkan 1.2 hypothesis is wrong; availability was never the blocker | **UPHELD.** The lead's hypothesis is wrong and should be recorded as such. A's evidence is the stronger of the two. |
| 3 | Three reasons are architectural and none says the approach is wrong | **TWO UPHELD, ONE UPHELD BUT MISLOCATED.** And I found the two C++ lines that actually do the index narrowing, which no document in this territory names. |
| 4 | Submodules stale; SPIRV-Tools PR 5316 missing from our checkout | **PINS UPHELD EXACTLY. The reachability question resolves in the NEGATIVE.** Taichi's SPIR-V cannot trigger that bug. |
| 5 | The macOS blocker has NOT expired | **OVERSTATED.** A's "largely expired, not cleanly" is the accurate reading. B's is not supported by the thread it cites. |
| 6 | The AOT equality check may not bind this project's install-time model | **THE REASONING HOLDS, and the check is narrower than either report says.** Two conditions it must carry are stated in §13.6. Not decided. |

Neither new section needs to go back. Both are better work than Part I in
sourcing discipline. B's §11.3 is the single most useful new fact in the
territory. A's §7.4 is the single sharpest new inference.

## 13.1 The empirical claim. Re-measured. Upheld.

I ran `nvidia-smi` and `vulkaninfo` read-only. Nothing was executed through
Taichi and nothing was built.

Driver 535.309.01, instance version 1.3.275. Three physical devices.

| Device | apiVersion | `shaderInt64` | `bufferDeviceAddress` | `VK_KHR_buffer_device_address` | `VK_EXT_buffer_device_address` |
|---|---|---|---|---|---|
| GTX 1070 (Pascal, plan Mid) | 1.3.242 | true | true | present | present |
| GTX 750 Ti (Maxwell, plan Baseline) | 1.3.242 | true | true | present | present |
| llvmpipe, Mesa 25.2.8, LLVM 20.1.2 (CPU) | 1.4.318 | true | true | present | **absent** |

I checked the enclosing structures rather than grepping for the names.
`shaderInt64` sits in `VkPhysicalDeviceFeatures` on all three.
`bufferDeviceAddress` sits in `VkPhysicalDeviceVulkan12Features` on all three,
and additionally in `VkPhysicalDeviceBufferDeviceAddressFeatures` on the two
NVIDIA cards. llvmpipe reports `bufferDeviceAddressCaptureReplay = false` and
`bufferDeviceAddressMultiDevice = false`, which the guard does not test.

**B's §11.3 table is correct in every cell, including the one asymmetry — that
llvmpipe carries the KHR extension but not the EXT alias.** That is the kind of
detail that distinguishes a real measurement from a plausible one. A's §7.2 table
is correct too, except that it records llvmpipe's `apiVersion` as "n/a" when it is
1.4.318, the highest of the three.

**The distinction the brief asked for, sharpened.** Both reports say every clause
of the guard is satisfied. True. But the guard is not evaluated the way either
describes.

`vk_api_version` is the *physical device's* `apiVersion`
(`vulkan_device_creator.cpp:523`), and `CHECK_VERSION` at `:720-721` compares
against it. So at `:819-820`:

```cpp
      if (CHECK_VERSION(1, 3) ||
          buffer_device_address_feature.bufferDeviceAddress) {
```

all three devices are at 1.3 or above, so the left operand is true and **the
feature bit fetched at `:816-817` is never consulted on this box**. A's §7.2 says
"`:819` passes on both terms". Both terms do independently hold, as my table
shows, so A's conclusion is right — but the code only ever evaluates the first.
A device at 1.3+ that reported `bufferDeviceAddress = false` would pass this gate
anyway. Whether Vulkan 1.3 makes that feature mandatory is a specification
question I am not equipped to settle from here, and I flag it rather than rule.

**Advertised versus done, stated precisely.** What the measurement establishes is
that the gate would pass. What it does not establish is that anything works, and
B's escalation 3 says so in the right words: "Do not read 11.3 as 'it works.'
Read it as 'nothing in the hardware refuses it.'" I endorse that wording
unchanged. Report A's §7.2 closes with "This says the gate passes. It does not
say the feature works", which is the same discipline.

**One measured fact neither report drew out.** Under plan §2.2, the interesting
result is not that the cards pass. It is that **all three pass, including the CPU
software renderer**. If the baseline Maxwell card and llvmpipe both satisfy the
gate, then restoring the capability would not create a class of hardware that can
do something another class cannot, which is the §2.2 violation the plan forbids.
That is the strongest single argument in favour of the capability anywhere in this
territory, and neither report makes it. B gets closest, in escalation 5, but
frames llvmpipe as an open question rather than as the §2.2 answer.

## 13.2 The Vulkan 1.2 hypothesis. The lead's premise is wrong.

**UPHELD, and A's evidence is better than B's.**

B §11.2 argues from dates: Vulkan 1.2 shipped January 2020, the shelving is
October 2022. Sound but circumstantial.

A §7.1 argues from the original commit, and that is decisive. `7705f688a`,
2022-02-08, introduced the guard already reading

```
+    if (CHECK_VERSION(1, 2) ||
+        CHECK_EXTENSION(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) ||
+        CHECK_EXTENSION(VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)) {
```

I verified the diff. `06cb1c50d` (#4244) narrowed it a day later, dropping the
EXT alternative and adding the `shaderInt64` requirement. Current form at
`vulkan_device_creator.cpp:814-821`.

So the code accepted both the core version and the standalone extension from the
first line it ever had, two years after 1.2 shipped and eight months before the
shelving. **Core promotion cannot have expired a blocker that never existed.**
The lead's hypothesis should be recorded as disproved, and A is right to have
raised it as escalation 2 rather than quietly working around it.

B's citation of the guard as `:812-819` is loose at both ends — `:812` is blank
and `:813` is a comment; the two conditions are at `:814-815` and `:819-820`. B's
macro citations at `:714-717` and `:720-721` are exact. A's `:813` is off by one
for the same reason; A's `:819`, `:821` are exact and its `:824` should be `:825`.

## 13.3 The three architectural reasons

### Reason 7 / A's F. The SNode root pointer is a buffer offset, not a device address. **UPHELD.**

Verified in §1.5 and §2b above and again here. `visit(GetRootStmt *)` at
`spirv_codegen.cpp:351-357` registers `make_pointer(0)`; `at_buffer` at `:2197`
reads a `u64` as an absolute device address. Nothing has changed since 2021.

I maintain my §2b ruling that this is a real blocker but a local one, on the
`ValueKind` field at `spirv_ir_builder.h:88`. That disagreement is with adversary
1, recorded in §12.3, and it is not affected by the new material.

**A new fact that bears directly on it, and on adversary 1's §2.4 and B's
escalation 3.** Both ask whether the root buffer's device address could be
supplied the way the ndarray base is. It already would be. With the capability
on, `vulkan_device.cpp:1772-1780` adds
`VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR` to **every** storage buffer, the
SNode root included, and `:1792-1798` immediately calls
`vkGetBufferDeviceAddressKHR` and stores the result in `alloc.addr`.
`VulkanDevice::get_memory_physical_pointer` at `:1862-1864` returns exactly that
field. **The root buffer's device address is fetched and retained already; what is
missing is plumbing it into the shader, not obtaining it.** Neither report,
neither adversary. It makes B's escalation 3 a smaller question than B thought.

### Reason 9. The frontend narrows indices to int32; the proposed per-backend demotion pass was never built. **UPHELD as to fact, MISLOCATED as to place, and its stated justification has itself partly expired.**

Three separate things, and both reports run them together.

**The pass was never built. Verified.** `taichi/transforms/` contains
`demote_atomics`, `demote_dense_struct_fors`, `demote_mesh_statements`,
`demote_no_access_mesh_fors` and `demote_operations`. I read
`demote_operations.cpp:1-30`: it decomposes operations such as `pow` into
pieces, and has nothing to do with data types. `taichi/ir/transforms.h` declares
no dtype-demotion pass. jim19930609's proposal in issue #5320 — "We should add a
'dtype demotion' pass for each specific backend" — is unimplemented, and I
verified the quote verbatim against the issue body. B's §5.1 rendering is exact.

**The narrowing is real, and it lives in two C++ lines that no document in this
territory names.** B's evidence for reason 9 is a maintainer's issue comment, and
the concrete site B alludes to — RangeFor boundary casting — is in
`python/taichi/lang/ast/ast_transformer.py:1294-1304`, the Python frontend, which
plan §1.2 places out of scope. That would make reason 9 unactionable for this
project as B states it. It is not, because the narrowing also happens in the C++
IR, unconditionally and for every backend:

- `taichi/transforms/type_check.cpp:466-468` sets
  `LoopIndexStmt::ret_type = PrimitiveType::i32`. No capability test, no arch
  test.
- `:470-472` does the same for `LoopLinearIndexStmt`, `:474-476` for
  `BlockCornerIndexStmt`.
- `:457-463`, inside `visit(ExternalPtrStmt *)`, force-casts **every** array index
  down to `i32` via `insert_type_cast_before` whenever it is not already `i32`.

That last one is the sharpest. It also independently confirms adversary 1's §2.5
finding at a higher level: the ndarray index is narrowed in the type checker,
before codegen runs, so no codegen change alone could widen it. **These four sites,
plus `taichi/ir/snode.h:41,45,49` from §3 above, are the in-scope C++ answer to
plan §8.1.1 for the index half of item 6.2.** They belong to territory 01 and I
defer on completeness, but reason 9 should carry them rather than an issue link.

**The stated justification has partly expired, which neither report notices.**
Reason 9 is that the frontend narrows "because some backends lack int64". On this
box that premise is false for the Vulkan backend: `spirv_has_int64` is set
unguarded at `vulkan_device_creator.cpp:630-633`, and all three devices report
`shaderInt64` true (§13.1). So the *code* is architectural and unchanged, while
the *reason given for it* is environmental and has expired on this project's
targets. Both reports file reason 9 wholly under ARCHITECTURAL. It is better
described as architectural code standing on an expired environmental
justification, which is a considerably more favourable position for item 6.2.

### Reason 10. The `demote_dense_struct_fors.cpp:29` assert, unchanged since October 2022. **UPHELD.**

`git log -L 29,29:taichi/transforms/demote_dense_struct_fors.cpp` returns exactly
one commit: `b347a2159`, 2022-10-28, #6457. Unchanged for three years and ten
months. Both reports right.

My §3 qualification stands and neither new section addresses it: the assert fires
only for all-dense struct-fors (`demote_dense_struct_fors.cpp:114-118`, gated on
`is_path_all_dense`), and plan §4.1 requires sparsity. Adversary 1 §5 reaches the
same qualification. **Two adversaries independently. Report A's §3.2 "the
operative constraint today" should be narrowed before the reports are read
together.**

### On the claim that none of the three says the approach is wrong

**UPHELD, and it is the correct reading.** I looked for a contrary statement in
Part I and Part II of both reports, in the issue threads, and in the commits.
There is none. All three reasons say the same thing: the SNode and index path was
built 32-bit and nobody converted it. That is a statement about work not done,
not about a design judged wrong. B's §11.1 wording — "None of them says the
physical storage buffer approach is wrong" — is exactly right and is the single
most load-bearing sentence in the new material.

## 13.4 Submodules and SPIRV-Tools PR 5316

### The pins. Verified exactly, and one paraphrase is wrong.

I read the gitlinks out of `HEAD` and the commit date out of each checked-out
submodule.

| Submodule | Pin | Revision date |
|---|---|---|
| SPIRV-Tools | `46ca66e6991f16c89e17ebc9b86995143be2c706` | 2022-11-18 |
| SPIRV-Cross | `c77b09b57c27837dc2d41aa371ed3d236ce9ce47` | 2022-11-21 |
| SPIRV-Headers | `34d04647d384e0aed037e7a2662a655fc39841bb` | 2022-12-15 |
| SPIRV-Reflect | `7c9c841fa9f40c09d334d5db6629ba318e46efaf` | **2023-02-01** |
| volk | `b87f88292b09bc899b24028984186581a1d24c4e` | 2023-03-24 |
| VulkanMemoryAllocator | `539c0a8d8e3733c9f25ea9a184c85c77504f1653` | 2025-04-14 |
| Vulkan-Headers | `409c16be502e39fe70dd6fe2d9ad4842ef2c9a53` | 2025-04-18 |

Every hash and every date in both reports' tables is correct. A's table is the
more complete of the two: it includes SPIRV-Reflect and VulkanMemoryAllocator,
which B's omits.

**The paraphrase "four SPIR-V submodules sit at November and December 2022" is
wrong.** Three do. SPIRV-Reflect is 2023-02-01. B's prose is defensible because it
says the four *gitlinks last moved together* in `ee0af32ab`, which I verified:
`git ls-tree ee0af32ab external/` and `git ls-tree HEAD external/` give identical
values for all four, and `git log -- external/SPIRV-Tools` shows `ee0af32ab`
(2023-04-03, #7676) as the most recent commit to touch it. The 2025 bump
`4c2bac69e` (#8680, 2025-04-25) moved only Vulkan-Headers and
VulkanMemoryAllocator. So the shape of B's claim is right; the "November and
December 2022" gloss is not.

Header content verified: `external/Vulkan-Headers/include/vulkan/vulkan_core.h:71`
gives `VK_HEADER_VERSION 313` and `:74` the 1.4 complete version — B cites `:72,75`,
off by one on both. `external/SPIRV-Headers/.../spirv.hpp:106`, `:229` and `:1062`
carry the three PSB constants exactly as B says.

### PR 5316. **The gap is real. The reachability question resolves in the negative.**

B §11.5 records this as NOT ESTABLISHED and asks whether Taichi's SPIR-V can
trigger it. I resolved it, and the answer is no.

The gap is real. I read the pinned source:
`external/SPIRV-Tools/source/opt/aggressive_dead_code_elim_pass.cpp:438-448` is
the pre-fix `GetLoadedVariablesFromFunctionCall`, without the guard PR 5316 adds.
`spirv_codegen.cpp:2690` does register `CreateAggressiveDCEPass()`, and `:2710`
does set `set_run_validator(false)` — B's line, correct; A cites `:2709`, off by
one, in both §7.6 and escalation 5.

I then read the PR's own diff and its regression test. The bug needs, in one
module, an `OpTypeFunction` whose **return type** is a `PhysicalStorageBuffer`
pointer, and an `OpFunctionCall` reaching it — the test case is
`%13 = OpTypeFunction %11` with `%11 = OpTypePointer PhysicalStorageBuffer %5`,
and the pre-fix `IsPtr` walks the `OpFunction` definition and wrongly reports its
return type as a pointer.

Taichi cannot emit that shape:

- `spirv_ir_builder.cpp:180-183` declares exactly one function type,
  `t_void_func_`, as `OpTypeFunction` returning `t_void_`. It is the only
  `OpTypeFunction` in the whole SPIR-V codegen.
- `start_function` at `spirv_ir_builder.h:443-451` emits `OpFunction` with return
  type `t_void_` and function type `t_void_func_`. `spirv_codegen.cpp:1938`,
  `:1994` and `:2135` are its only call sites, all passing the same
  `kernel_function_`.
- `grep -rn 'OpFunctionCall' taichi/codegen/spirv/ taichi/rhi/` returns **nothing**.
  Taichi emits one void function per module and never calls a function.

With no `OpFunctionCall` and no pointer-returning function type,
`GetLoadedVariablesFromFunctionCall` is unreachable and `MemPass::IsPtr` is never
handed an `OpFunction` definition. **PR 5316 cannot fire on Taichi output.**
(`CreateInlineExhaustivePass()` at `:2688` running before DCE is a second reason,
but it is redundant: there is nothing to inline.)

So B's §11.5 headline, "One toolchain fix we lack touches a pass Taichi actually
runs", is true of the pass and false of the fix. B was careful to record the
limit rather than assert a fault, and that caution is vindicated — but the
concrete example B offers for the stale-pin risk does not survive. **B's
escalation 2, that the pins are a standing exposure of unknown size, is
unaffected and I agree with it.** One keyword search found one candidate and the
candidate does not reach us; that says nothing about the other three years.

A's two qualifications in §7.6 both check out and are worth keeping: SPIRV-Cross
is used only by the Metal, DX and OpenGL RHIs and does not bear on this question,
and the pinned SPIRV-Tools does know the feature. A's further observation that
`spirv_version` is set to `0x10500` for *both* the 1.3 and the 1.2 branches at
`vulkan_device_creator.cpp:525-528`, so a 1.3 device still compiles under
`SPV_ENV_VULKAN_1_2` at `spirv_codegen.cpp:2671-2672`, is correct and is new
information about the toolchain envelope.

## 13.5 macOS. **B overstates it. A is right.**

B §11.6 concludes the blocker "has NOT expired". I read both issues it cites.

MoltenVK **2174**, open since 2024-03-07, one comment. The reporter is on a 2016
Intel MacBook Pro with an AMD Radeon Pro 455, macOS 12.7.3, custom builds, Xcode
13.4.1. The maintainer, billhollings, replied 2024-03-12:

> `VK_KHR_buffer_device_address` and `VK_EXT_buffer_device_address` are supported
> in the latest MoltenVK, and are currently passing about 90% of the CTS tests.
> It's possible you are encountering an issue, but we'd need more information to
> determine that.

Nobody replied. The thread is unresolved, not confirmed. B quotes the reporter's
"support has been removed entirely for my GPU" without quoting the maintainer
disputing the premise, and without noting the eight-year-old Intel hardware.
MoltenVK **2696**, open since 2026-03-06, is one comment old and reports
`VK_ERROR_OUT_OF_DEVICE_MEMORY` with indirect draw and dispatch, a different
failure.

**A §7.3 has this right**: "the specific 2022 breakage is no longer the live
complaint, but the feature was still short of full conformance on that vendor as
recently as 2024, by its own maintainer's account." That is what the evidence
supports. A's verdict "largely expired, not cleanly" is the accurate one and
B's is not.

One correction to A. A lists MoltenVK **2278** (2024-07-19) as a "further open
device-address report". Its title is "GPU address fault error when using
descriptor indexing + variable descriptor count" and the body attributes the
fault to variable descriptor counts, not to buffer device address. It is not
evidence for this proposition and should be dropped from A's list.

Where both are right, and it is the part that matters for this project: macOS is
absent from plan §5.2, and the `#if !defined(__APPLE__)` clause is the half of
the guard with a live reason behind it. Both say so. **A's further point that the
Taichi-side possibility has never been eliminated is the one that should survive
into the plan**, because it is the only recorded instance of this capability
producing wrong answers anywhere, and nobody ever established whose fault it was.

## 13.6 The escalated question. Testing the reasoning, not deciding it.

The claim: the global kill was an AOT cross-machine portability measure enforced
by `c_api/src/taichi_gfx_impl.cpp:22-29`; plan §5's install-time model may be
immune; if so the only surviving reason is macOS, which the platform clause
already handles alone.

**The check is narrower than either report says, and I verified how narrow.**
`grep -rn 'get_required_caps\|required_caps' taichi/ c_api/` returns the field at
`taichi/aot/module_data.h:125` and `taichi/runtime/gfx/aot_utils.h:20`, the
serialisation at both, the writer at
`taichi/runtime/gfx/aot_module_builder_impl.cpp:23`, the virtual getter at
`taichi/aot/module_loader.h:97`, and **exactly one reader**:
`c_api/src/taichi_gfx_impl.cpp:21`. `TI_ERROR_INCOMPATIBLE_MODULE` is raised in
one place, `:26`.

So this is not "the AOT path". It is `ti_load_aot_module` in the C API, one
function. Nothing in `taichi/` enforces it; the JIT path compiles against the live
device capabilities and never consults `required_caps` at all. If this project
does not load AOT modules through the C API, the check never executes.

**The reasoning holds, and I can state its condition precisely.** Plan §5 has the
loader examine the system at install time and either compile for it or select a
prebuilt binary. In the compile-on-target case the capabilities come from
`vulkan_device_creator` on the machine that will run the code, so
`required == current` by construction and the check is a tautology. The concern
that produced event B genuinely does not arise.

**Two conditions the reasoning has to carry, and neither report states them.**

1. **The concern does not vanish; the responsibility moves.** The check is the
   only thing in the tree that would catch a binary built with a capability the
   target device lacks. In the select-a-prebuilt case, plan §5's loader becomes
   the thing that must do the matching. If it does not, the failure mode is a
   SPIR-V module declaring an unmet capability, which is
   `VUID-VkShaderModuleCreateInfo-pCode-01091` — the exact shape k-ye documented
   in issue #6368. Immunity to the check is not immunity to the problem.
2. **The check bites on far more than this capability, so removing this one
   capability from the equation changes little.** Adversary 1's B4 objection is
   correct and I verified it: `spirv_has_int16` at
   `vulkan_device_creator.cpp:628`, `spirv_has_int64` at `:632`,
   `spirv_has_float64` at `:636` and the four subgroup capabilities at `:659`,
   `:663`, `:667`, `:671` are all set unguarded from the live device. An AOT
   module already carries seven or more device-derived values through an equality
   check. **Whether the check binds this project is therefore a question about
   the whole capability set, not about physical storage buffers.** If it does not
   bind, that is a general result; if it does, disabling one capability never
   fixed it.

**On the second half of the claim, that macOS is then the only survivor.** A's
§7.4 is the load-bearing evidence and I verified all of it. The other candidate
survivor is #6368's imported-device problem, and it does not apply to the guard
in question, for two independent reasons:

- **The owned-device path already enables the extension and the feature.**
  `vulkan_device_creator.cpp:595-596` pushes `VK_KHR_buffer_device_address` into
  `enabled_extensions` whenever the device advertises it, outside any `#if`. And
  `:830-831` chains `buffer_device_address_feature` — already filled with
  `bufferDeviceAddress = true` by the query at `:816-817` — into
  `VkDeviceCreateInfo`'s `pNext`, also outside the `#if`, which encloses only the
  single `caps.set` at `:826`. The logical device created at `:860` therefore has
  the extension enabled and the feature on **today, on this box, with the
  capability off**. A device that enabled the extension cannot produce the
  unmet-capability failure #6368 describes.
- **The two producers are independently disabled.** The owned-device producer is
  `vulkan_device_creator.cpp:826` behind `&& false`. The imported-device producer
  is `c_api/src/taichi_vulkan_impl.cpp:45-51`, a `/* */` comment block in a
  different file, disabled by a different commit (`12546578a`, #6494). **Lifting
  the `&& false` would not touch the imported path at all**, so #6368's concern
  stays sealed off by its own separate guard. Neither report says this cleanly and
  it is what makes the lead's reasoning work.

I also checked the initialisation ordering, because #6368 is fundamentally an
ordering complaint. On the owned path, `create_logical_device` ends with
`set_caps` at `:876` and `init_vulkan_structs` is called afterwards at `:278`,
which calls `create_vma_allocator` at `vulkan_device.cpp:1592`. On the imported
path, `taichi_vulkan_impl.cpp:53` calls `set_caps` before `:54` calls
`init_vulkan_structs`. **Both orderings are correct**, so
`vulkan_device.cpp:2509` sees the final capability set. Lifting `&& false` is safe
in that respect.

**But my §1.3 live-producer path is not**, and this closes a loop between my
earlier finding and A's #6368 inference. `ti_set_runtime_capabilities_ext`
(`c_api/src/taichi_core_impl.cpp:331`) calls `set_caps` on an already-constructed
runtime, long after `create_vma_allocator` has run. A user who turns the
capability on that way gets `vulkan_device.cpp:1772-1780` adding
`VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR` to buffers and `:1792-1798`
calling `vkGetBufferDeviceAddressKHR`, against a VMA allocator that was created
**without** `VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT` because `:2509` read
the capabilities before they existed. That is the VMA-does-not-respect-device-
capabilities failure #6368 is about, reachable today, from a public C-API entry
point. Read from source; I did not run it.

**I am not deciding the escalated question.** The reasoning survives the test. Its
conditions are the two above, and it should reach the project owner with them
attached rather than as a bare "the concern may not apply".

## 13.7 Errors in the new material

### Report A, section 7 and revised escalations

1. §7.6 and escalation 5: `set_run_validator(false)` is at
   `spirv_codegen.cpp:2710`, not `:2709`.
2. §7.2 records llvmpipe's `apiVersion` as "n/a". It is 1.4.318.
3. §7.2: the guard's first condition is at `:814`, not `:813` (a comment); the
   `#if` is at `:825`, not `:824`.
4. §7.3 cites MoltenVK issue 2278 as a device-address report. It is a
   descriptor-indexing fault. Drop it.
5. §7.7 reason E repeats the module-wide-addressing-model claim as "holds
   permanently, inherent to the SPIR-V spec". Refuted in my §2a and adversary 1's
   §3, on the same evidence, independently. This is the one place where the new
   section re-asserts something two adversaries have separately disproved.
6. §7.0 table row I and §7.7 reason I still call
   `demote_dense_struct_fors.cpp:29` the constraint without the
   `is_path_all_dense` qualifier. Carried over from §3.2.
7. Escalation 6 still says "ten commits through 2023-2024". Uncorrected from §1.8.

A's §7.4 is the strongest new work in either report and I could not break any part
of it.

### Report 05b, Part II

1. §11.5's headline overstates what §11.5's own body carefully hedges, and the
   candidate fix cannot reach us (§13.4).
2. §11.6's "has NOT expired" is not supported by the thread it cites (§13.5).
3. §11.4's "November and December 2022" excludes SPIRV-Reflect at 2023-02-01,
   and the table omits SPIRV-Reflect and VulkanMemoryAllocator entirely.
4. `vulkan_core.h` line citations `:72,75` should be `:71,74`.
5. §11.2's guard span `:812-819` starts on a blank line and a comment.
6. §11.1 row 9 files reason 9 wholly under ARCHITECTURAL without noting that its
   stated justification is environmental and has expired on this project's
   targets (§13.3), and locates the narrowing in evidence that points at the
   Python frontend rather than at
   `taichi/transforms/type_check.cpp:457-463,466-476`.

B's §11.3 is exact in every cell and is the best-sourced new fact in the
territory.

## 13.8 Escalations, additional

Added to §10 and §12.4. Not resolved by assumption.

1. **Plan §2.2 is satisfied by the measurement, and that is a finding the planner
   should have explicitly.** All three devices on this box, including the Maxwell
   baseline and the CPU software renderer, pass every clause of the guard. So
   restoring the capability would not create a capability split across the target
   set. Neither report states this and it is the §2.2 test applied to the actual
   question.

2. **The live-producer path can produce the #6368 failure today.** §13.6, last
   paragraph. `ti_set_runtime_capabilities_ext` sets capabilities after the VMA
   allocator is built. This is a defect, not a design question, and it is
   reachable from a public C-API entry point. I am not proposing a change; per
   plan §10.3 I am not calling anything surplus. Flagging that it exists and that
   no document in this territory has noticed it.

3. **Whether Vulkan 1.3 makes `bufferDeviceAddress` mandatory** decides whether
   `vulkan_device_creator.cpp:819`'s `CHECK_VERSION(1, 3) ||` short-circuit is
   sound or is a latent hole. I could not settle it from the repository and it is
   a Vulkan specification question, not a Taichi one.

4. **The index-narrowing sites at `taichi/transforms/type_check.cpp:457-463`,
   `:466-468`, `:470-472` and `:474-476`** are the in-scope C++ half of reason 9
   and appear in no report or adversary file in this territory. They overlap
   territory 01. Flagging so they are not lost between assignments, as
   `snode.h:49` nearly was.

5. **I built and ran nothing.** §13.1 is `vulkaninfo` and `nvidia-smi` output,
   read-only, and says what the devices advertise. Every other claim in section 13
   is read from source, from git, or from GitHub. The one thing that would settle
   most of this territory — building with the guard lifted and running on the 750
   Ti — remains undone by all four documents.

---

# 14. Second divergence pass against adversary 05-1

Adversary 1's file grew an addendum (its sections 14 to 24) covering the same
expiry material, and a section 24 answering my section 12. I read both and
re-derived every claim it makes against me. Sections 1 to 13 stand except where I
withdraw something below.

## 14.1 Conceded in full: the `at_buffer` collision IS architectural. I withdraw §2b's classification.

This was our one substantive disagreement and adversary 1 has won it on the
source. I checked its counter-argument line by line rather than accepting it.

Adversary 1 concedes that the `ValueKind` field exists, is read, and survives
`register_value`/`query_value` — my three checks all hold. It then shows that my
two supporting claims are wrong, and it is right on both.

**The tag is not set at the decision point.** `spirv_codegen.cpp:2204` sets
`kPhysicalPtr` on `at_buffer`'s **output**, so that `load_variable`
(`spirv_ir_builder.cpp:1270-1284`, branching at `:1275`) and `store_variable`
(`:1286-1298`, branching at `:1290`) know to emit an aligned physical access.
The value `at_buffer` inspects at `:2197` arrives untagged. I read the flag's
existing role as available-at-the-decision when it is in fact downstream-of-it.

**And the tag cannot reach the decision point, because arithmetic erases it.**
`spirv_ir_builder.h:289-298`:

```cpp
  Value make_value(spv::Op op, const SType &out_type, Args &&...args) {
    Value val = new_value(out_type, ValueKind::kNormal);
    make_inst(op, out_type, val, std::forward<Args>(args)...);
    if (out_type.flag == TypeKind::kPtr) {
      val.flag = ValueKind::kVariablePtr;
    }
    return val;
  }
```

Every SSA value is born `kNormal` and is upgraded only when the SPIR-V **type**
is a pointer type. A byte offset, `u32` or `u64`, is `TypeKind::kPrimitive`, so
the result is `kNormal` whatever the operands carried. I traced it on both sides
and adversary 1's trace is exact: `make_pointer` yields `kConstant` via
`get_const` at `spirv_ir_builder.cpp:1502`; `GetChStmt`'s
`ir_->add` at `spirv_codegen.cpp:372` routes through
`DEFINE_BUILDER_BINARY_USIGN_OP` at `spirv_ir_builder.cpp:1038-1047` into
`make_value` and comes out `kNormal`; and the genuine device address on the
ndarray path, `ir_->add(addr, OpSConvert(...))` at `spirv_codegen.cpp:790`, comes
out `kNormal` too. **Both sides of the decision are `kNormal`. `at_buffer` cannot
be reading the tag, which is why the dtype sniff is there.**

I then tested the one alternative adversary 1 did not: whether `ptr_to_buffers_`
could serve as the discriminator, since `at_buffer` already consults it at
`:2211`. It cannot. `visit(ExternalPtrStmt *)` sets `ptr_to_buffers_[stmt]` at
`:797-801` **outside** the capability branch at `:783-795`, so under the
capability an ndarray pointer carries both a u64 physical address and a
`ptr_to_buffers_` entry, exactly like an SNode pointer. There is no existing
discriminator at the decision point by any route.

**So my remedy estimate was wrong.** Tagging at production requires changing
`make_value` at `spirv_ir_builder.h:291`, the single constructor for every SSA
value the SPIR-V backend emits, plus a propagation rule for mixed operands across
`add`, `mul`, `sub`, the shifts and the bitwise ops. That is the backend's value
model, not three functions and four call sites.

**And with the remedy estimate goes the classification.** My objection to
"architectural" rested entirely on the obstacle being local. It is not. Against
the planner's operative test — architectural obstacles do not expire,
environmental ones do — the collision does not expire, and neither does the
`make_value` propagation problem behind it. Adversary 1 offers the disagreement
as definitional and gives me the more generous reading. I decline it. **The
ruling in my §2b, in my §12.3 item 1 and in my §12.4 item 1 is withdrawn. The
`at_buffer` collision is architectural. Both explore passes and adversary 1 had
it right and I did not.**

Two things survive the withdrawal and I keep them, because neither depended on
the remedy estimate:

- **§2a stands.** The module-wide addressing model is still not architectural,
  refuted by eleven shipped releases and by per-task module granularity.
  Adversary 1 reaches the same verdict independently in its §3. Nothing in its
  addendum reopens it.
- **§2b's third failure mode stands.** `make_pointer` reaching `u64_type()`
  around the `shaderInt64` guard (§2b, and adversary 1 concedes it at its §24.4)
  is unaffected.

The net effect of the exchange is that the obstacle is **larger** than either of
us originally described: not a dtype sniff to be replaced, but a value model with
no room to carry the distinction.

## 14.2 Where the expiry work converges

Independently, without either of us seeing the other's addendum:

- **The hardware measurement.** Same three devices, same values, same asymmetry
  on llvmpipe's missing EXT alias. Two independent `vulkaninfo` runs agreeing to
  the cell should settle B's §11.3.
- **The guard passes on API version alone.** Both of us traced
  `vulkan_device_creator.cpp:523`, `:720-721`, `:814-815`, `:819-820`, `:821`,
  `:825` and found that `CHECK_VERSION(1, 3)` short-circuits the feature bit.
- **The Vulkan feature is already enabled today.** Both of us landed on
  `:595-596` and `:830-831` sitting outside the `#if`. Adversary 1 states the
  consequence better than I did: restoring the capability is not turning on a
  Vulkan feature, it is turning on Taichi's use of one that is already on.
- **PR 5316 cannot trigger.** Same conclusion, complementary evidence. Adversary
  1 has the `// NOTE: only support void kernel function` comment at
  `spirv_ir_builder.h:406-410` and the default
  `external_optimization_level = 3`, which I did not check; I have the sole
  `OpTypeFunction` declaration at `spirv_ir_builder.cpp:180-183` and the PR's own
  regression test requiring a pointer-returning function type. Two routes, one
  answer. **B's open item closes in the negative.**
- **The macOS claim is overstated in B and correctly calibrated in A.**
- **Reason 9's rationale is environmental and expired while its implementation is
  unconditional.** Both of us reached this split independently.
- **The AOT equality check has exactly one consumer**, `taichi_gfx_impl.cpp`.
- **The `is_path_all_dense` gate** on the struct-for assert, for the third and
  fourth time across four documents.

## 14.3 Where I add to adversary 1's expiry work

1. **The C++ index-narrowing sites.** Adversary 1's §17.3 concludes that the
   in-scope half of reason 9 is `snode.cpp:92` against `snode.h:49`, and that the
   RangeFor half is Python and therefore out of scope under plan §1.2. The first
   is right and the second is right, but the in-scope half is incomplete.
   `taichi/transforms/type_check.cpp:457-463` force-casts **every** array index
   to `i32` in a C++ IR pass, and `:466-468`, `:470-472` and `:474-476` hardcode
   `i32` for `LoopIndexStmt`, `LoopLinearIndexStmt` and `BlockCornerIndexStmt`.
   No arch test, no capability test, every backend. That is loop and array
   indexing narrowed in the C++ core, not the frontend, and it is where reason 9
   actually lives for this project. It also confirms adversary 1's own §2.5 at a
   higher level: the ndarray index is narrowed before codegen runs, so no codegen
   change could widen it.
2. **The live-producer path can produce issue #6368's failure today.**
   §13.6. `ti_set_runtime_capabilities_ext` calls `set_caps` after
   `create_vma_allocator` has already run, so buffers would gain
   `VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR` against a VMA allocator built
   without `VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT`. Adversary 1 concedes
   the live-producer finding at its §24.1 but does not carry it into the ordering
   question. Both device-creation paths order `set_caps` correctly; only the
   runtime setter does not.
3. **The root buffer's device address is already fetched.** §13.3. With the
   capability on, `vulkan_device.cpp:1772-1780` and `:1792-1798` obtain and store
   a device address for every storage buffer including the SNode root, retrievable
   via `get_memory_physical_pointer` at `:1862-1864`. This makes adversary 1's own
   §2.4 and B's escalation 3 a smaller question than either states: the address is
   available, only the plumbing into the shader is missing.
4. **The v0.9.x tags**, conceded by adversary 1 at its §24.3.

## 14.4 Where I correct adversary 1

Its §17.4 and its §10 comparison table both state that PR #3177 shows "ailzhang
overruling k-ye" on widening to 64 bits. **That is report A's inverted
attribution and adversary 1 has adopted it.** I read the thread: the sentence "I
think it's still better to expand this to 64-bits. It is less limiting, and is
consistent with `num_cells_per_container`" is the closing line of **k-ye's own
comment**, dated 2021-10-14, in which k-ye quotes the enforce-the-int-fit
proposal and rejects it. ailzhang's only review comment on that PR
(`repos/taichi-dev/taichi/pulls/3177/comments`, id 733333856) concerns LLVM
versus C++ casting. ailzhang never wrote it. My §8 item A4 has the full working.
The substance survives — a maintainer argued for widening in October 2021 and it
never happened — but the error is now in two documents and should be corrected
before they are read together.

## 14.5 Two things adversary 1 has that I did not, both correct

1. **§20.3 hole one.** Step 4 of the escalated question — that macOS is the only
   survivor and the platform clause contains it — holds only if the 2022 fault
   was MoltenVK's. Nobody established that; strongoier explicitly left it open. If
   the fault was in Taichi's *use* of buffer device address, it is not
   platform-specific and `#if !defined(__APPLE__)` does not contain it. That is
   the load-bearing unknown for the decision and I did not connect it to the
   escalation either. **It should go to the project owner with the question.**
2. **§20.4, the §2.2 test splits.** Restoring the capability passes plan §2.2,
   because the SNode path is unchanged and what varies is a build-artifact ABI
   variant. Using the capability to give the SNode root a device address does not
   pass, because hardware lacking `bufferDeviceAddress` could then not carry a
   root that capable hardware could. My §13.8 item 1 makes the first half of that
   argument and stops. Adversary 1's second half is correct and is the sharper
   point: **the §2.2 answer differs between "restore the capability" and "use it
   for SNode addressing," and only the second serves item 6.2.**

## 14.6 Net after both passes

Settled between the two adversaries, and I regard these as closed:

- The convergent claim holds for live code, with the `snode_struct_compiler.cpp:52-54`
  qualifier on intent.
- The module-wide addressing model is **not** an architectural obstacle.
- The `at_buffer` u64 collision **is** one, and is larger than first described.
- Live producers exist; the capability is un-defaulted, not off.
- PR 5316 cannot reach us.
- The hardware gate passes on all three devices on this box, baseline card and
  CPU renderer included.
- Reason 9's rationale has expired while its implementation has not.
- The struct-for assert does not fire on sparse trees.

Open, and for the arbiter or the project owner:

1. The escalated AOT question, with adversary 1's §20.3 hole one attached and my
   §13.6 two conditions.
2. What `snode_struct_compiler.cpp:52-54` means for item 6.2.
3. The two defects neither of us is authorised to act on: the VMA ordering on the
   runtime capability setter (§13.6, §14.3 item 2) and `make_pointer` bypassing
   the `shaderInt64` guard (§2b).
4. Whether the SPIR-V submodule pins move. Unsized by either of us; B's
   escalation 2 stands.
