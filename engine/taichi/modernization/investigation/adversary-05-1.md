# Adversary 05-1 — territory `shelved-64bit`

Adversary 1 of 2. Judging `report-05-shelved-64bit.md` (pass A) and
`report-05b-shelved-64bit.md` (pass B), plus their notes.

Repo at `ba0e81dce559fb63a5958bf82feb1d00c55c02fe`, branch `master`. Everything
below was re-derived from source, `git`, and `gh api` against
`taichi-dev/taichi`. Nothing outside this file was modified. No writes to
GitHub.

Method note: I did not accept either report's citations. Every line number,
commit hash, tag and issue in this document was re-read at the fork commit.
Where I say VERIFIED I read the artefact; where I say INFERRED I say what the
inferential step is.

---

## 0. Verdict in one page

**The convergent claim survives.** I attacked it as instructed and it holds.
`spirv_has_physical_storage_buffer` and the SNode addressing path are disjoint
in the source at the fork commit, and I found no consumer that crosses between
them. Both passes are right, and the planner's earlier conclusion to the project
owner does need reversing.

**But the convergent claim is incomplete in a way that matters, and both passes
missed the same thing.** They stop at "restoring the capability delivers nothing
for the SNode ceiling." True. What neither says is that on the SPIR-V path there
is no *other* mechanism. The SNode root is a descriptor-bound storage buffer
addressed by a 32-bit byte offset. Widening that offset to 64 bits inside a
bound buffer buys nothing, because a bound buffer's reach is capped by
`maxStorageBufferRange`, a limit this codebase never queries anywhere. If item
6.2 ever needs a root buffer beyond a bound buffer's range on Vulkan, the
physical-storage-buffer machinery is the only door SPIR-V offers. So the correct
statement is not "the shelved work is irrelevant to 6.2." It is "the shelved
work is not reusable *as it stands*, and it is simultaneously the only
prerequisite for the version of 6.2 that exceeds a bound buffer." Report B's
Escalation 3 gestures at this without stating the constraint. Report A does not
raise it at all. See section 2.4.

**Of the two claimed ARCHITECTURAL obstacles, one is architectural and one is
not.**

| Obstacle | Both reports say | I rule |
|---|---|---|
| Module-wide `AddressingModelPhysicalStorageBuffer64` switch | architectural | **Not architectural.** Factually true, consequence overstated, and refuted by upstream's own shipped releases. Section 3. |
| `at_buffer` u64-means-device-address vs `make_pointer` root-relative-from-zero | architectural | **Architectural. Upheld.** Section 4. Both reports' line-level reasoning contains errors; the conclusion is right anyway. |

**Both reports contain errors.** Neither is disqualifying. The itemised list is
section 10. The three that would mislead the planner if left standing:

1. Report A, §1.8 and Escalation 4: "extended by ten commits through 2023-2024."
   **Zero of those commits are from 2024.** All ten are April–July 2023. Nothing
   has touched `taichi/codegen/spirv/` on this path since August 2023. This
   changes the momentum picture by eighteen months.
2. Report A, §3.2: `demote_dense_struct_fors.cpp:29` is "the operative
   constraint today." It is the operative constraint on **all-dense trees only**
   — the pass is gated on `is_path_all_dense`. The project plan §4.1 requires
   sparsity. On a sparse tree that assertion never runs.
3. Report B, §2.3: disabling the capability "makes every module portable at a
   stroke." False. The same equality check B relies on still bites on
   `spirv_has_int64`, `spirv_has_float64`, `spirv_has_int16` and the subgroup
   capabilities, all still device-derived at `vulkan_device_creator.cpp:626-671`.

**On relative quality.** Pass A is more precise on SPIR-V mechanics, on the ref
sweep and on `use_64bit_pointers`'s edit history. Pass B is materially more
complete on GitHub — it found the open upstream tracking issue for exactly this
question, #8608 "Implement int64 indexing", which A missed entirely, and B's
inference about the AOT capability check is better evidenced than A's about
issue #6368. Neither consumer inventory is exhaustive; both miss the same three
consumers. Section 2.2.

---

## 1. What I did, so the planner can judge the weight

I re-ran the consumer inventory from scratch by grepping the whole tree for both
the capability name and the `has_buffer_ptr` boolean it flows into, then reading
every hit in context rather than trusting either report's excerpt. I read
`at_buffer`, `load_buffer`, `store_buffer`, `make_pointer`,
`bitmasked_activation`, `visit(GetRootStmt*)`, `visit(GetChStmt*)`,
`visit(ExternalPtrStmt*)`, `visit(MatrixPtrStmt*)`,
`visit(GlobalTemporaryStmt*)`, `visit(SNodeLookupStmt*)`, `compile_args_struct`,
`compile_argpack_struct`, `compile_ret_struct`, `translate_ti_type`,
`from_taichi_type` and `get_struct_type_with_data_layout_impl` in full. I
enumerated every assignment into `ptr_to_buffers_`, which is the map that decides
which buffer a pointer resolves against, because if the two paths were going to
touch, that map is where it would show.

I read out the capability guard from every one of the 122 release tags rather
than sampling. I swept all 350 refs for a live producer. I read PR bodies,
review states, comment authors and comment dates through `gh api` for issues
6295, 6368, 5979, 8608, 5320, 8161, 3177 and PRs 4221, 6468, 6494.

I did not compile or run anything. Every claim about what a flag flip would do
is read from source. Both reports say the same about themselves; I am not in a
position to improve on that and I flag it in Escalations.

---

## 2. The convergent claim, attacked

### 2.1 It holds. The separation is clean at the fork commit.

VERIFIED. `ptr_to_buffers_` is assigned in exactly seven places in
`taichi/codegen/spirv/spirv_codegen.cpp`, and they partition cleanly:

| Line | Statement | Buffer |
|---|---|---|
| `:319`, `:326`, `:332` | `MatrixPtrStmt` | inherited from `stmt->origin` |
| `:377` | `GetChStmt`, when the child is a place | `BufferType::Root` |
| `:711` | `GlobalTemporaryStmt` | `BufferType::GlobalTmps` |
| `:798` | `ExternalPtrStmt`, array argument | `BufferType::ExtArr` |
| `:800` | `ExternalPtrStmt`, non-array argument | `BufferType::Args` |

The only `Root` entry is `:377`. The capability is never consulted on the way to
it. The capability's five codegen consumers are `:783` (external array base
address), `:2340` (`compile_args_struct`), `:2416`
(`compile_argpack_struct`), `:2491` (`compile_ret_struct`), and `:2201`
(the `at_buffer` join). Not one of those is on the root path.

Conversely the SNode path runs through `make_pointer` at `:2317-2324`, whose
four call sites are `:355` (`GetRootStmt`, `make_pointer(0)`), `:371`
(`GetChStmt`), `:408` (`bitmasked_activation`) and `:506` (`SNodeLookupStmt`).
`make_pointer` reads `use_64bit_pointers`, never the capability.

I looked specifically for a crossing and there is none. Both reports are
correct. The claim is not an artefact of shared blindness: it is a structural
property of `ptr_to_buffers_`, and I arrived at it from the map rather than from
either report's excerpt.

### 2.2 Neither inventory is exhaustive. Three consumers are missing from both.

Both reports present their consumer lists as complete. Report A's §1.1 list runs
to six bullets. Report B's §2.5 says "Twelve live consumers" and enumerates
them. Both omit the same three:

1. **`taichi/runtime/gfx/runtime.cpp:851-859`**, inside
   `get_struct_type_with_data_layout_impl`. This is the **host-side** half of the
   argument ABI: `has_buffer_ptr = layout[1] == 'b'` at `:823`, then a
   `PointerType` member is sized `sizeof(uint64_t)` or `sizeof(uint32_t)` at
   `:852-858`. Report A cites `gfx_program.h:81-83`, the producer of the `"1b"`
   string, but not the consumer that acts on it. Report B cites neither.
2. **`taichi/codegen/spirv/spirv_types.cpp:484-497`**, `translate_ti_type`, the
   function all three struct-compile sites call. This is where a Taichi
   `PointerType` actually becomes a 64-bit or 32-bit SPIR-V integer. Report B
   cites `spirv_types.cpp:433-439` (`visit_physical_pointer_type`) but not this.
   Report A cites `spirv_types.cpp` not at all.
3. **`taichi/codegen/spirv/spirv_ir_builder.cpp:334-342`**,
   `IRBuilder::from_taichi_type`, a **second, independent implementation** of the
   same pointer-width decision. Missing from both.

None of these changes the verdict — all three are argument-struct and
return-struct layout, still not SNode. But an inventory offered as complete and
missing a second implementation of the load-bearing decision is a defect, and it
is the same defect in both passes. That is the shared-blindness signal the
planner asked me to look for. It appears here and nowhere else.

### 2.3 A live latent defect in the dead branch that neither pass found

VERIFIED, and it is the sharpest available evidence for the "re-enabling means
enabling untested code" risk both reports raise abstractly.

The argument layout string and the return layout string are produced at
`taichi/runtime/program_impls/gfx/gfx_program.h`. Arguments, at `:79-83`, read
the capability and return `"1b"` or `"1-"`. Returns, at `:75-77`, return the
literal `"4-"` unconditionally. Meanwhile `compile_ret_struct` in
`spirv_codegen.cpp:2490-2494` reads the capability and passes it into
`translate_ti_type`.

So with the capability on, the shader would lay out a pointer member of a return
struct as 64 bits while the host, via `get_struct_type_with_data_layout_impl`,
sizes and aligns it as 32. The argument path is consistent; the return path is
not.

The return-struct branch was added by `88ac098bc` (#8061, 2023-05-23) — one of
the ten commits both reports count. I confirmed the diff adds
`bool has_buffer_ptr = caps_->get(...)` and threads it into `translate_ti_type`,
with no matching change to `get_kernel_return_data_layout`.

I am **not** claiming this is reachable today: whether a kernel return struct can
contain a `PointerType` at all is a frontend question I did not resolve, and the
project plan §1.2 puts the Python frontend out of scope. Stated conditionally:
if it can, this is a live host/device ABI mismatch sitting in the dead branch. It
goes in Escalations.

### 2.4 What both passes stop short of, and it changes the conclusion's force

This is my main substantive addition, and it partially cuts against the
convergent claim's practical reading.

Both reports conclude the shelved work is not the asset for item 6.2 and leave
it there. Report A §6.1: "not reusable for SNode addressing width." Report B §3:
"would not move the SNode ceiling at all." Both true.

What follows from the same evidence, and neither states:

- The SNode root is a **descriptor-bound storage buffer**. `get_buffer_value` at
  `spirv_codegen.cpp:2287-2314` binds it via `ir_->buffer_argument(...)` at
  `:2309`. `bitmasked_activation` binds it explicitly at `:401-402`.
- `at_buffer`'s non-u64 path at `:2210-2218` shifts the byte offset right by
  `log2(width)` and hands the result to `struct_array_access`. The offset it
  shifts is whatever `make_pointer` produced: a `u32` today.
- A descriptor-bound storage buffer's reach is capped by Vulkan's
  `maxStorageBufferRange`. **This codebase never queries that limit.** `grep -rn
  "maxStorageBufferRange" taichi/ c_api/` returns nothing.

So on the SPIR-V path there are exactly two addressing mechanisms: a bound
buffer with a 32-bit offset, and a physical device address. Widening
`make_pointer` to `u64` without moving off the bound buffer widens an offset into
a range the offset was not what limited. Getting the SNode root past a bound
buffer's reach on Vulkan requires the physical-storage-buffer mechanism, because
SPIR-V does not offer a third option.

That reframes the shelved work from "irrelevant" to "not reusable as written,
and structurally prerequisite for the large-root version of 6.2." The planner's
reversal to the project owner should carry that qualifier, or the next
decision — whether to restore the capability — will be taken against an
incomplete picture.

I mark the Vulkan-limit half INFERRED: I am reading the SPIR-V and Vulkan
addressing models, not a Taichi source line, because Taichi has no source line on
this. The Taichi-side half — bound buffer, 32-bit offset, limit never
queried — is VERIFIED.

### 2.5 A smaller correction to both: the capability does not widen ndarray *indexing*

Both reports say restoring the capability would widen ndarray addressing.
Report A §6.1: "It addresses ndarray and external-array arguments." Report B §3:
"would widen ndarray addressing."

VERIFIED, and it is less than that. In `visit(ExternalPtrStmt *)`,
`spirv_codegen.cpp:734-795`, the whole index computation is `i32`:
`linear_offset` is initialised `i32` at `:737`, accumulated by `mul`/`add` at
`:770-771`, and byte-shifted at `:773-777` with an `i32` result type. Only at
`:790` is it `OpSConvert`-ed to `u64` and added to the `u64` base.

So the capability gives a 64-bit **base address** plus a 32-bit **signed byte
offset**. It does not lift a per-array indexing limit. Both reports credit it
with more than it delivers, which — usefully for the planner — makes their own
conclusion stronger, not weaker.

---

## 3. Attack 1: the module-wide addressing model. Not architectural.

Report A §1.2 and §6.4; report B §2.5. Both treat the single module-wide
`OpMemoryModel` as a hard obstacle. A states it hardest: "Turning the capability
on changes the SPIR-V validity rules for every shader in the module."

**The factual claim is VERIFIED.** `taichi/codegen/spirv/spirv_ir_builder.cpp:113-127`:
under the capability, `OpExtension SPV_KHR_physical_storage_buffer` plus
`OpMemoryModel AddressingModelPhysicalStorageBuffer64`; otherwise
`AddressingModelLogical`. One declaration, no per-pointer opt-in. The
`OpCapability PhysicalStorageBufferAddresses` is emitted separately at `:73-77`.

**The consequence drawn from it does not hold, for three reasons.**

**First, "every shader in the module" is one shader.** VERIFIED. `TaskCodegen`
constructs its own `IRBuilder` per instance at `spirv_codegen.cpp:97`, and
`run()` at `:130-156` calls `init_header()`, builds a single function named
`"main"` at `:132-133`, and returns `ir_->finalize()` at `:151` as that task's
`spirv_code`. One offloaded task, one module, one entry point. The scope A
describes as alarming is a single compute shader. The real scope of the decision
is program-wide, but that is because `caps_` is program-wide, which is a
different argument from the one either report makes.

**Second, the addressing model is permissive, not exclusive.**
`AddressingModelPhysicalStorageBuffer64` does not invalidate logical pointers.
It adds the `PhysicalStorageBuffer` storage class alongside the existing ones.
Descriptor-bound storage buffers keep working unchanged in the same module.
INFERRED from the SPIR-V and Vulkan specifications, not from a Taichi source
line.

**Third, and this is the part that settles it — upstream's own shipped code
proves the second point.** VERIFIED. In v1.0.0 through v1.1.3 the capability was
on and `use_64bit_pointers` was `false`. That is precisely a module declaring
`AddressingModelPhysicalStorageBuffer64` while addressing the SNode root through
a descriptor-bound buffer with a 32-bit offset. Mixed logical and physical
addressing in one module, shipped in eight public releases, and the thing that
eventually broke was a MoltenVK bug in ndarray reads, not the memory model.

The genuine constraint in this area is different and neither report names it
cleanly: **the device must have `VK_KHR_buffer_device_address` actually enabled,
or `vkCreateShaderModule` rejects the module** for declaring an unmet capability.
That failure shape is real — k-ye's log in issue #6368, which I read in full,
shows it for `AtomicFloat64AddEXT`, `AtomicFloat32AddEXT`, `Int8`, `Int16`,
`Int64`, `Float16`, `Float64` and `SPV_EXT_shader_atomic_float_add`. It is a
device-and-driver fact, not an architectural one. Under the planner's own
distinction it expires. It is also exactly what `vulkan_device_creator.cpp:814-828`
already guards against on the owned-device path, by checking
`CHECK_VERSION(1,2)`, the extension, `bufferDeviceAddress` and `shaderInt64`
before setting anything.

**Ruling: the module-wide switch is real and correctly described, but
misclassified. It is not an architectural obstacle. It is a device-capability
negotiation problem, and upstream shipped working code through it for eight
months.** Both reports should be corrected on this point.

Note also that `PhysicalStorageBufferAddresses` is conspicuously **absent** from
the actual error dump in #6368. Report A §1.6 offers that issue as the likely
referent for "multi-platform compatibility issues" and calls the match "exactly
the failure shape for this capability." It is the failure *class*, but the
capability A is arguing about is not in the list. A marks the inference INFERRED
and "strong but circumstantial"; I judge it weaker than A does. Section 8.

---

## 4. Attack 2: the u64 collision. Architectural. Upheld.

Report A §2.3(a); report B §4 reason 1. Both say flipping `use_64bit_pointers`
collides with `at_buffer` treating a `u64` pointer as an absolute device address
while `make_pointer` produces a root-relative offset from literal zero.

**VERIFIED, and this one is genuinely architectural.**

- `at_buffer`, `spirv_codegen.cpp:2194-2219`. The discriminator is at `:2197`:
  `if (ptr_val.stype.dt == PrimitiveType::u64)`. On that branch it emits
  `OpConvertUToPtr` at `:2199-2203` to a
  `spv::StorageClassPhysicalStorageBuffer` pointer and tags the result
  `ValueKind::kPhysicalPtr` at `:2204`. **The pointer's SPIR-V type is the only
  discriminator. There is no separate tag saying which buffer it belongs to.**
  `ptr_to_buffers_` is consulted only on the fall-through path, at `:2211`.
- The same `== PrimitiveType::u64` test recurs in `load_buffer` at `:2226` and
  `store_buffer` at `:2248`, in both cases to decide the buffer element type.
- `make_pointer`, `:2317-2324`, returns a `u64` immediate under the flag.
- `visit(GetRootStmt *)`, `:351-357`, registers `make_pointer(0)` as the root
  value. Literal zero.

Flipping the flag therefore makes every SNode place pointer `u64`-typed, which
makes `at_buffer` take the physical branch, which converts `0 + offset` into an
absolute device address and dereferences it. `ptr_to_buffers_[stmt]` — the map
entry set at `:377` that names the root buffer — is silently never read.

This is architectural in the strict sense the planner draws. The type carries two
incompatible meanings and the code has no third thing to disambiguate them.
Nothing about hardware, drivers or time changes that. Fixing it means either
giving the root buffer a real device address (report B's Escalation 3) or
introducing a discriminator that is not the scalar type. Both are design work,
not a flag flip.

**Upheld for both reports. This is the finding in this territory that should
survive into the plan.**

### 4.1 Both reports' secondary reasoning on `bitmasked_activation` contains errors

Both offer `bitmasked_activation` as a second independent breakage. It is one,
but neither gets it right.

The function is `spirv_codegen.cpp:383-437`. `ptr_dt` is taken from
`parent_ptr.stype` at `:388` and becomes `u64` under the flag.

**Report A is right in substance and slightly overstated in the rule it
invokes.** A cites `:398-399` (`OpShiftLeftLogical`, result type `ptr_dt`, Base
`ir_->const_i32_one_`) and `:410-412` (`OpShiftRightLogical`, result type
`ir_->u32_type()`, Base `bitmask_word_ptr` typed `ptr_dt`). Both citations are
accurate to the line. A states the rule as "Base and Result must share a type."
That is stricter than the rule actually enforced: SPIR-V validation for the shift
instructions requires Base and Result Type to match in **bit width and component
count**, not in signedness. Under A's stated rule `:398-399` would already be
invalid today, since `const_i32_one_` is signed and `ptr_dt` is `u32` — and it
plainly is not, because this code ships. Under the correct rule `:398-399` is
fine today at 32 bits and breaks at 64. **A's conclusion is right; the rule as
stated would have implied a bug that does not exist.**

A is also right that `:405`'s `u32_type()` shift *amount* is legal, and right
that `SNodeLookupStmt` at `:503-508` would survive because `:504-505` casts the
index to `parent_val.stype` first. I checked both.

**Report B has one citation that is simply wrong.** B lists ":404, :411, :412,
:413" and then adds: "and adds a u32 value to `parent_ptr` at `:409`. `OpIAdd`
requires matching widths."

- `:404` is the `OpShiftLeftLogical` whose result type is `ptr_dt`; the
  `u32_type()` at `:405` is its **shift amount**, which SPIR-V permits to differ
  from Base. Not a defect. A explicitly says so; B lists it as one.
- `:409` is `ir_->add(parent_ptr, bitmask_word_ptr)`. Under the flag
  **both operands are `u64`**: `parent_ptr` is `ptr_dt` by definition, and
  `bitmask_word_ptr` at that point came from `:403-404` with result type `ptr_dt`
  and was added at `:406-408` to a `make_pointer(...)` result, also `u64`. There
  is no width mismatch at `:409`. **B's second reason is wrong.**
- I also checked the mechanism B invokes. `ir_->add` is generated by
  `DEFINE_BUILDER_BINARY_USIGN_OP` at `spirv_ir_builder.cpp:1038-1047`, which
  opens with `TI_ASSERT(a.stype.id == b.stype.id)`. A genuine mismatch there
  would trip a host-side assertion during codegen, not produce invalid SPIR-V.
  B's framing of the failure mode is wrong even where a mismatch existed.
- B's `:411`, `:412` land on the same construct A cites at `:410-412`, and
  B's `:413` is one line short of the `struct_array_access` at `:414`.

**On this sub-point A is materially more reliable than B.** Both reach the right
top-level conclusion — flipping the flag does not produce a working build.

---

## 5. Item 2: `demote_dense_struct_fors.cpp:29` versus the `snode.cpp` warning

Report A §3.2 makes the assertion "the operative constraint today" and demotes
the warning. Report B §5 centres the warning and notes the assertion as a second
edit. **Neither is right, and the resolution is more useful than either.**

**Both artefacts exist as described.** VERIFIED.

- `taichi/ir/snode.cpp:95-100`, `ErrorEmitter(TaichiIndexWarning(), ...)`,
  "SNode index might be out of int32 boundary but int64 indexing is not
  supported yet. Struct fors might not work either."
- `taichi/transforms/demote_dense_struct_fors.cpp:29`,
  `TI_ASSERT(total_n <= std::numeric_limits<int>::max())`.

**The assertion is a real hard stop, not a debug-only one.** VERIFIED, and this
supports A. `TI_ASSERT` at `taichi/common/logging.h:100-107` expands to a plain
`if (!x) TI_ERROR(...)`. There is no `NDEBUG` gate. It fires in every build.
`taichi/common/serialization.h:32` does redefine `TI_ASSERT` to `assert`, but
that is a separate translation-unit-local definition and does not reach this
pass.

**It is confirmed empirically as what users actually hit.** VERIFIED via
GitHub. Issue **#8608 "Implement int64 indexing"**, open, filed 2024-12-18 by
lstrgar, quotes both artefacts in its body, and the one that terminates the run
is:

```
[E 12/18/24 23:27:59.749 2523792] [demote_dense_struct_fors.cpp:convert_to_range_for@29] Assertion failure: total_n <= std::numeric_limits<int>::max()
```

**But A's "operative constraint today" is too broad, and this is the correction
that matters for this project.** VERIFIED. `demote_dense_struct_fors.cpp:114-118`:

```cpp
void maybe_convert(OffloadedStmt *stmt) {
  if ((stmt->task_type == TaskType::struct_for) &&
      stmt->snode->is_path_all_dense) {
    convert_to_range_for(stmt);
  }
}
```

`convert_to_range_for` is the only caller of the assertion, and it runs only when
`is_path_all_dense`. That flag is set at `taichi/ir/snode.cpp:20`,
`new_ch->is_path_all_dense = (is_path_all_dense && !new_ch->need_activation())`,
so a `pointer`, `bitmasked` or `dynamic` node anywhere on the path clears it.
`taichi/transforms/offload.cpp:191-192` reads the same flag for the same
decision.

**Project plan §4.1 states sparsity is required.** On a sparse tree that
assertion never executes. Report A's §3.2 would tell the planner the hard cap is
2^31-1 total cells for this project's workload. It is not: for a sparse tree
neither the assertion nor the demotion pass applies.

**And both reports miss the truncation that is neither of the two.** VERIFIED,
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

The store at `:92` truncates on **every iteration**, before the check at `:95`
runs even once. `SNode::Extractor::acc_shape` is `int` at
`taichi/ir/snode.h:49`, while `num_cells_per_container` is `int64` at
`taichi/ir/snode.h:97`. So the surviving 32-bit field is `acc_shape`, the warning
is a post-hoc detector for damage already done, and `shape_along_axis` at
`snode.cpp:174-177` returns `int` as well.

**Ruling.** The assertion is the hard stop **on all-dense trees**, which is what
upstream's users report hitting and what A correctly identifies. The warning is a
detector, as B says, and it is emitted after the truncation rather than instead
of it. Neither is the operative limit for a sparse tree. For item 6.2's actual
map of 32-bit assumptions, the load-bearing artefact is `snode.h:49`, the `int`
width of `Extractor::acc_shape` — which neither pass names. That belongs to
territory 01, and I defer to that pair on completeness, but it should not be lost
because both passes here stopped at the two visible messages.

---

## 6. Item 3: `PointerType::addr_space_`. Both reports are right.

**Ruling: address space, not address width. It does not belong under item 6.2 as
an asset. Both passes are correct and I could not break either.**

VERIFIED independently:

- `taichi/ir/type.h:207`, `int addr_space_{0};  // TODO: make this an enum`.
  Getter at `:191-193`. Serialised at `:203`.
- Neither constructor takes it. `type.h:179` is the default constructor;
  `type.h:181-185` takes `(Type *pointee, bool is_bit_pointer)` only. No setter
  exists anywhere.
- `grep -rn "get_addr_space\|addr_space_"` across `.cpp`, `.h` and `.mm` in the
  whole tree returns four hits, all four inside `type.h` itself: the getter body,
  the getter's return, the `TI_IO_DEF` and the declaration. **Zero call sites,
  ever.**
- History is exactly two commits: `dcd5d7d35` (#1948, Yuanming Hu, 2020-10-12)
  created it with getter and no setter; `48fa799da` (#7460, lin-hitonami,
  2023-03-06) added it to `TI_IO_DEF`. Both reports have this right.
- The working analogue both cite is real: `int addr_space` declared with default
  `0` at `taichi/codegen/llvm/codegen_llvm.h:165`, used at
  `codegen_llvm.cpp:1131` and passed to `llvm::PointerType::get(..., addr_space)`
  at `:1133`. That is unambiguously an LLVM address space — the
  generic/global/shared/constant selector — not a width.

One qualification I will record without softening the ruling: on some targets an
address space does imply a pointer width. AMDGPU's `addrspace(3)` local pointers
are 32-bit while `addrspace(1)` global pointers are 64-bit. So "space, not width"
is right as a classification of this field and right as a reason to move it out
from under 6.2, but space and width are not fully orthogonal on every backend.
That is an item 6.3 / territory 04 concern, not a reason to keep a dead field
under 6.2. Report A's Escalation 1 asks the planner to move it. I concur.

---

## 7. Item 4: the version ranges. Resolved, and both are incomplete.

I read the guard out of **every** tag rather than sampling. `git tag` yields 122
tags; the capability appears in a `vulkan_device_creator.cpp` in 21 of them. The
file moved from `taichi/backends/vulkan/` to `taichi/rhi/vulkan/` between v1.0.3
and v1.0.4, which is why a path-fixed sweep shows a false gap at v1.0.0–v1.0.3.

| Tags | Guard | State |
|---|---|---|
| v1.0.0, v1.0.1, v1.0.2, v1.0.3, v1.0.4, v1.1.0, v1.1.2, v1.1.3 | none | **ENABLED** (8 tags) |
| v1.2.0, v1.2.1, v1.2.2 | `#if !defined(__APPLE__)` | enabled except macOS (3 tags) |
| v1.3.0, v1.4.0, v1.4.1, v1.5.0, v1.6.0, v1.7.0, v1.7.1, v1.7.2, v1.7.3, v1.7.4 | `#if !defined(__APPLE__) && false` | **DISABLED** (10 tags) |

**Neither report is wrong; both are incomplete, and B's headline miscounts.**

- Report B §2.1 presents a table headed "Capability state read out of each
  release tag" and lists four enabled tags: v1.0.1, v1.0.4, v1.1.0, v1.1.3. It is
  eight. B's headline in §1 says "live in four public releases." That is the
  count of B's sample, not of the releases. B's disabled row lists four of the
  ten. **B's table is a spot-check presented as a full read-out.**
- Report A §1.3 makes no claim about the v1.0/v1.1 line and gives the disabled
  range as "v1.3.0 through v1.7.0". Accurate as far as it goes; v1.7.1 through
  v1.7.4 also ship it dead. A's method — `git merge-base --is-ancestor d20f55dc8
  <tag>` — is sound and I reproduced its answer.

**The eight-months claim is right on both.** `7705f688a` author-dated
2022-02-08; `f677fb141` 2022-10-24; `d20f55dc8` author-dated 2022-10-30 (commit
date 2022-10-29, which is why the two reports differ by a day on nothing).
Roughly eight and a half months.

A's timeline row for `06cb1c50d` gives 2022-02-09 and B gives 2022-02-08. Both
are defensible: author date 2022-02-08, commit date 2022-02-09. No error.

**A is right and B is loose on the two producers' current form.** VERIFIED:
`taichi/rhi/vulkan/vulkan_device_creator.cpp:825-827` is
`#if !defined(__APPLE__) && false` around the single `caps.set` at `:826`;
`c_api/src/taichi_vulkan_impl.cpp:45-51` is a `/* */` comment block, not a
preprocessor guard. A states the distinction and flags it as a correction to the
brief. B gives the span as `:44-52`, two lines wide.

**A's ref sweep reproduces exactly.** I swept all 350 refs myself for a
`vulkan_device_creator.cpp` that sets the capability without `&& false`. Filtered
to tips after 2022-11-02, the only survivors are
`refs/remotes/upstream/bump/v1.2.3` and `refs/remotes/upstream/rc-v1.2.0`, both
dated 2022-11-15, both v1.2-line release branches carrying pre-disable code.
That is precisely A's §1.7 result. B's §2.4 reaches the same conclusion by a
different route. Both verified.

---

## 8. Item 5: "inference from silence". Both reports understate their own case.

The instruction was to test whether the evidence supports the momentum-loss
inference. **It does, and it is stronger than an argument from silence. Both
reports are more cautious than the record requires.**

Report B labels it explicitly: "Argument from silence; weigh it as such."
Report A §1.7 is firmer — "The evidence supports 'momentum lost'" — but rests it
on the precondition being met.

What the record actually contains is not silence. It is **unkept explicit
commitments plus unanswered direct requests**, which is affirmative evidence.
All VERIFIED through `gh api`:

| Artefact | State | The commitment, and what followed |
|---|---|---|
| PR **#6468** | merged 2022-10-30 | Body: "Will be recovered when device capability is fully functional." Approved by ailzhang with an empty review body. Sole issue comment is `netlify[bot]`. |
| PR **#6494** | merged 2022-11-02 | Body: "Will bring it back after devcap." ailzhang APPROVED with body "Thanks!". Sole issue comment is `netlify[bot]`. |
| Issue **#5979** "[AOT] Device capability management for AOT (Tracker)" | **OPEN** since 2022-09-05 | One comment, by PENGUINLIONG, its own author, 2022-09-05T08:01, three minutes after opening. Nothing since. |
| Issue **#6368** | **OPEN** since 2022-10-18 | One comment, k-ye, 2022-10-19. Nothing since. |
| Issue **#8161** "ndrange is limited in size" | **OPEN** since 2023-06-08 | listerily, 2023-06-09: "We will investigate it later." Next and last comment is a user, 2023-09-13. |
| Issue **#8608** "Implement int64 indexing" | **OPEN** since 2024-12-18 | Two comments, both users: bgailleton 2025-06-30 "any news on that side?", lstrgar 2025-07-14 "I would like to bump this... @FantasyVR". **No maintainer has ever replied.** |
| Issue **#5320** | **OPEN** since 2022-07-04 | Last maintainer word jim19930609 2022-07-08. Then lstrgar 2025-09-23, lstrgar 2026-02-13 offering to implement it himself if a maintainer would give guidance, yojeep 2026-05-09. **No maintainer reply in nearly four years.** |

Set against this: the devcap machinery that #6468 named as its precondition
demonstrably landed. I re-read the consumer surface — `spirv_has_physical_storage_buffer`
is declared at `taichi/inc/rhi_constants.inc.h:28`, exposed as
`TI_CAPABILITY_SPIRV_HAS_PHYSICAL_STORAGE_BUFFER = 18` at
`c_api/include/taichi/taichi_core.h:399`, with a typed setter at
`c_api/include/taichi/cpp/taichi.hpp:1156-1159`. A caller can declare the
requirement today. The producer is still `&& false`, three and a half years
later.

**Ruling: not an inference from silence. A stated intention, a met precondition,
an unlifted guard, and four open issues in which users ask directly and
maintainers do not answer.** Report B's self-characterisation is too modest and
should be revised upward; A's is closer but rests on a narrower base than the
record offers. B deserves the credit for finding #8608 and #5320 at all — A
missed both, and #8608 is the upstream tracking issue for the exact question
this territory was assigned.

I record one honest counterweight. `715a04c98` (#8315, Bob Cao, 2023-08-15)
edited the dead true branch of `make_pointer`, and the PR's own changelog
describes it as fixing "a bug in the `make_pointer` function ... that caused
incorrect pointer arithmetic for 64-bit pointers." Someone was reading that code
in August 2023. Weighed low: that changelog is Copilot-generated boilerplate, and
the change is a one-line simplification from a cast-widened `u32` to a direct
`u64` immediate. It does not overturn the ruling, but it is the only positive
signal against it and it should not be suppressed.

---

## 9. Item 6: the ten commits and the test coverage. Verified, with one date wrong.

**The count is right.** VERIFIED. `git log --since=2023-01-01 -S 'has_buffer_ptr'
master` returns exactly ten:

| Commit | Date | Subject |
|---|---|---|
| `7809ebbd5` | 2023-07-13 | [refactor] Refactor code repetition for get argpack layout (#8278) |
| `14e83c484` | 2023-07-10 | [lang] Argpacks stores scalar values only in cpp implementation |
| `cfad91fc8` | 2023-07-10 | [spirv] [ir] Support argpack buffer load for spir-v backends |
| `29cfb5c72` | 2023-07-10 | [lang] Instantiate a runtime ArgPack object |
| `b66279b0b` | 2023-07-10 | [refactor] Enhance argument data structures |
| `32518fc52` | 2023-06-13 | [refactor] Add base class GfxProgramImpl |
| `88ac098bc` | 2023-05-23 | [spirv] [ir] [lang] Support struct object as return value in spir-v (#8061) |
| `748abdbcc` | 2023-04-25 | [Lang] Let kernel argument support matrix nested in a struct |
| `457ada6a6` | 2023-04-19 | [spirv] Support struct as kernel argument |
| `4c33922df` | 2023-04-06 | [gfx] Compile struct type of result and arguments in gfx backends |

**The date range is wrong in report A.** §1.8 says the code "kept being extended
through the 2023-2024 argument-refactor work," and Escalation 4 says "extended by
ten commits through 2023-2024." **None of the ten is from 2024.** The last is
2023-07-13. `git log --since=2024-01-01 -- taichi/codegen/spirv/` returns three
commits in total across 2024 and 2025, and none touches this path: `8ebe32b69`
(2024-06-17, spdlog bump), `4a80bb2dd` (2025-02-26, CI actions), `ba0e81dce`
(2025-07-30, debugprintf string sanitising). Adding `715a04c98` (2023-08-15,
which the `-S has_buffer_ptr` search does not catch because it edits
`make_pointer` rather than `has_buffer_ptr`) moves the true end date to August
2023, not into 2024. **Report A's date range should be corrected to April–August
2023.** Report B makes no such claim and is not liable.

This matters to the planner because it moves the last touch of the dead branch
eighteen months earlier, which strengthens the momentum-loss reading rather than
weakening it.

**"Extended" is generous for two of the ten.** `32518fc52` and `7809ebbd5` are
refactors that moved existing code. `-S` counts commits that change the number of
occurrences of a string, which includes pure relocation. Six of the ten are real
additions; I checked `88ac098bc` in diff and it genuinely adds
`bool has_buffer_ptr = caps_->get(...)` plus a `translate_ti_type` call in
`compile_ret_struct`.

**Zero test coverage: VERIFIED, and I widened the search beyond both reports'.**
`grep -rn 'physical_storage_buffer\|PHYSICAL_STORAGE_BUFFER\|buffer_device_address\|BufferDeviceAddress\|PhysicalStorageBuffer'` over `tests/`, `c_api/`, `misc/`
and `benchmarks/` returns nothing outside the C-API headers and the generated
documentation. The only hit anywhere in `python/` is the enum name at
`python/taichi/lang/enums.py:38`. **There is no test, C++ or Python, that
exercises the capability on or asserts anything about it.**

The precise statement of the risk is narrower than A's §1.8 phrasing and worth
getting right: the `if (has_buffer_ptr)` **true branches** have not executed
since 2022-10-30. The surrounding code executes constantly with the flag false.
So re-enabling means turning on roughly a dozen never-executed branches added
across ten commits, not resurrecting a dormant subsystem wholesale. Section 2.3
gives a concrete example of what is sitting in there.

---

## 10. Itemised errors, by report

### Report A — `report-05-shelved-64bit.md`

| # | Location | Error | Severity |
|---|---|---|---|
| A1 | §1.8, Escalation 4 | "through 2023-2024." No commit is from 2024; the range is April–August 2023. | **Material.** Misdates the momentum loss by 18 months. |
| A2 | §3.2 | `demote_dense_struct_fors.cpp:29` called "the operative constraint today" without qualification. The pass is gated on `is_path_all_dense` (`:114-118`); on a sparse tree it never runs, and plan §4.1 requires sparsity. | **Material.** |
| A3 | §1.2, §6.4 | The module-wide addressing model classified as the hard architectural obstacle. Factually right, consequence overstated, and refuted by upstream's own v1.0–v1.2 releases which shipped mixed logical/physical addressing. | **Material.** Section 3. |
| A4 | §2.3(b) | "Base and Result must share a type" for the shift instructions. The enforced rule is bit width and component count, not signedness. Under A's stated rule `:398-399` would be invalid today, which it is not. Conclusion unaffected. | Minor. |
| A5 | §1.5 | "neither PR has a single line of human review discussion. The only comments on either are from the Netlify bot." PR 6494's approving review by ailzhang carries the body "Thanks!". | Trivial, but it is an absolute claim and it is false. |
| A6 | §1.1 | Consumer inventory presented as complete; omits `gfx/runtime.cpp:851-859`, `spirv_types.cpp:484-497`, `spirv_ir_builder.cpp:334-342`. | Moderate. Section 2.2. |
| A7 | §1.3, §1.6 | GitHub coverage misses issues #8608, #5320 and #8161 entirely, including the open upstream tracking issue titled "Implement int64 indexing" which quotes both artefacts A adjudicates in §3. | **Material completeness gap.** |
| A8 | §1.6 | Offers #6368 as the referent for "multi-platform compatibility issues," calling it "exactly the failure shape for this capability." `PhysicalStorageBufferAddresses` does not appear in that issue's error dump. Correctly labelled INFERRED; I weigh it lower than A does. | Minor, correctly hedged. |
| A9 | §1.3 | Disabled range given as "v1.3.0 through v1.7.0"; v1.7.1–v1.7.4 also ship it dead. Makes no claim about v1.0/v1.1. | Trivial. |
| A10 | §6.1 | "It addresses ndarray and external-array arguments" — the base address only. Index arithmetic stays `i32` at `:737-777`. | Minor; understates A's own conclusion. |

### Report B — `report-05b-shelved-64bit.md`

| # | Location | Error | Severity |
|---|---|---|---|
| B1 | §4, reason 2 | ":409 adds a u32 value to `parent_ptr`. `OpIAdd` requires matching widths." Both operands are `ptr_dt` under the flag. No mismatch. And `ir_->add` guards with `TI_ASSERT(a.stype.id == b.stype.id)` at `spirv_ir_builder.cpp:1040`, so a mismatch would be a host assertion, not invalid SPIR-V. | **Material.** One of B's two stated reasons is wrong. |
| B2 | §4, reason 2 | Cites `:404` as a defect. The `u32_type()` there is at `:405` and is the shift **amount**, which SPIR-V permits to differ from Base. `:413` is one line short of the `struct_array_access` at `:414`. | Minor. |
| B3 | §2.1, §1 | Table headed "read out of each release tag" lists 4 of 8 enabled tags and 4 of 10 disabled; headline says "live in four public releases" when it is eight, plus three macOS-excepted. | **Material.** A sample presented as a census. |
| B4 | §2.3 | "Forcing the capability off everywhere makes every module portable at a stroke." The same equality check at `taichi_gfx_impl.cpp:22-29` still bites on `spirv_has_int64`, `spirv_has_float64`, `spirv_has_int16` and the subgroup capabilities, all device-derived at `vulkan_device_creator.cpp:626-671`. | **Material.** Weakens B's own inference: it explains why PSB is a problem but not why PSB alone was singled out. |
| B5 | §2.5 | "Twelve live consumers" presented as an enumeration; omits `gfx/runtime.cpp:851-859`, `spirv_types.cpp:484-497`, `spirv_ir_builder.cpp:334-342`. | Moderate. Section 2.2. |
| B6 | §2.2, Event C | `c_api/src/taichi_vulkan_impl.cpp:44-52`. The comment block is `:45-51`. | Trivial. |
| B7 | §4 | "Born `false`, never edited." True of the declaration at `:82`; the body's true branch was edited by `715a04c98` (2023-08-15). B describes the original form in prose but never cites the commit that changed it. A does. | Minor. |
| B8 | §9 VERIFIED 8 | "Turning `use_64bit_pointers` on would emit invalid SPIR-V, for the two reasons in section 4." Reason 2 is wrong (B1). The conclusion still holds on reason 1. | Follows from B1. |
| B9 | §2.5, §3 | "would widen ndarray addressing" — the base address only; index arithmetic stays `i32`. | Minor; understates B's own conclusion. |

### Where the two disagree, and who the source supports

| Question | A | B | Source supports |
|---|---|---|---|
| `06cb1c50d` date | 2022-02-09 | 2022-02-08 | Both. Author 02-08, commit 02-09. |
| `use_64bit_pointers` true branch edited since 2021? | Yes, `715a04c98` | "never edited" (of `:82`) | **A.** `715a04c98` edits `:2318-2320`. |
| `bitmasked_activation` at `:409` | not cited | broken | **A.** Both operands are `u64`. |
| `bitmasked_activation` shift amount at `:405` | explicitly fine | cited as a defect | **A.** |
| c_api producer form | `/* */`, `:45-51`, calls it a correction to the brief | `:44-52` | **A.** |
| Meaning of "multi-platform compatibility issues" | issue #6368, imported-device VMA | AOT capability equality check | **B**, on evidence weight: the `[aot]` tags on both disabling PRs, the nine-day gap after #6184, and the verbatim inequality at `taichi_gfx_impl.cpp:25`. Weakened by B4. A's referent does not name the capability in its own error dump. Neither is established. |
| Upstream tracking issues for int64 indexing | not found | #8608, #5320, #8161 | **B.** All three exist, all open, all verified. |
| Release range | v1.2.0–v1.7.0 only | four-tag sample | **Neither.** Section 7 has the full table. |
| Hard stop on int64 indexing | the assertion | the warning | **Split.** Section 5. The assertion, on all-dense trees only; the real truncation is `snode.h:49`. |

---

## 11. What I could not break

Recorded so the planner knows where the reports are solid, not merely
unchallenged.

1. **The disjointness of the two mechanisms.** Attacked from `ptr_to_buffers_`
   rather than from either report's excerpt. Holds.
2. **`use_64bit_pointers` was never true.** `git log --all -S 'use_64bit_pointers = true'`
   returns nothing across all 350 refs. `git log --all -S 'use_64bit_pointers'`
   returns exactly one commit, `1b34a2d4d`. Both reports right.
3. **`PointerType::addr_space_` is dead and is not width.** Section 6.
4. **The `at_buffer` u64 collision.** Section 4. Architectural, upheld.
5. **No live producer on any ref after 2022-11-02.** Reproduced A's sweep
   independently over all 350 refs.
6. **Zero test coverage.** Widened the search and found nothing.
7. **No design document and no re-enablement thread.** I re-ran the docs and
   issue searches and reached the same empty result.
8. **Only the Vulkan RHI ever set the capability.** The full-tree grep returns
   two producers, both Vulkan. Metal, DX11 and OpenGL never set it.

---

## 12. Escalations

Unresolved. Not to be settled by assumption.

1. **The convergent claim needs a qualifier before it reaches the project
   owner.** Reversing the earlier conclusion is correct: the shelved capability
   is not the asset for the SNode ceiling as it stands. But section 2.4 shows the
   reverse is not "irrelevant" — on SPIR-V it is the only mechanism that reaches
   past a descriptor-bound buffer, and `maxStorageBufferRange` is never queried
   anywhere in this tree. I am not authorised to decide how that qualifier is
   worded. Flagging that a bare reversal would mislead.

2. **The two obstacles should be reclassified, and only one survives as
   architectural.** Section 3 and section 4. The planner's own distinction —
   architectural obstacles do not expire, environmental ones do — puts the
   memory-model switch in the expiring column and the u64 collision in the
   permanent one. Both reports classify both as architectural. This is a ruling
   for the arbiter seat, not for me.

3. **Section 2.3's return-layout mismatch is conditional and I could not close
   it.** `gfx_program.h:75-77` hardcodes `"4-"` while `spirv_codegen.cpp:2490-2494`
   reads the capability. Whether a kernel return struct can contain a
   `PointerType` is a frontend question, and plan §1.2 puts the Python frontend
   out of scope. If it can, this is a live ABI mismatch in the dead branch. Needs
   someone with the frontend in scope, or a compile.

4. **`Extractor::acc_shape` at `taichi/ir/snode.h:49` is `int`, and the
   truncation at `snode.cpp:92` precedes the warning at `:95`.** Neither pass
   names it. It overlaps territory 01 and I defer on completeness, but it is the
   32-bit field that actually carries the damage and it should not be lost
   between two territories.

5. **I did not build or run anything.** Every claim about a flag flip, in all
   three documents, is read from source. My corrections to both reports'
   `bitmasked_activation` reasoning are also source-read. If the planner wants
   the flip verified, it needs a compile and a SPIR-V validator run, and that is
   the same escalation both passes raise.

6. **Target-hardware relevance remains untested by all three of us.**
   `vulkan_device_creator.cpp:814-828` requires Vulkan 1.2 or
   `VK_KHR_buffer_device_address`, plus `bufferDeviceAddress`, plus
   `shaderInt64`. Whether the GTX 750 and GTX 1070 in plan §3.1 satisfy that is
   unchecked. Both reports flag it; I add only that it is checkable on the
   development box in minutes with `vulkaninfo`, and that it is a section 2.2
   question — if one card satisfies it and the other does not, restoring the
   capability would let some hardware do something other hardware cannot, which
   is the violation the plan names. That makes it a gate, not a detail.

7. **The macOS root cause was never determined by upstream.** strongoier left it
   explicitly open in issue #6295 comment 1288522905 and nobody returned. Both
   reports flag this. I add that it is the only recorded instance of the
   capability producing wrong answers on any platform, and no bug report naming
   it as a cause exists for any non-macOS platform. Unresolvable from here.

---

## 13. Cross-adversary

Checked for `/opt/project/taichi/modernization/investigation/adversary-05-2.md`.
**It does not exist at the time of writing.** No divergence section can be
written. If adversary 2 lands later, this file should be revisited against it,
particularly on section 3 (whether the module-wide addressing model is
architectural) and section 5 (which of the two int64 artefacts is the operative
stop), since those are the two places where I depart from both reports rather
than from one.

---
---

# Addendum — the expiry assessment

Added after an extension to my brief. Both reports have grown expiry material
since I first judged them: report 05 gained section 7 (`report-05-shelved-64bit.md:411-642`),
report 05b gained sections 11 and 12 (`report-05b-shelved-64bit.md:519-664`).
Sections 1 through 13 above stand unchanged; nothing in the new material
overturns them.

Same constraints. Read-only. Every system query below was a read-only
`vulkaninfo` / `nvidia-smi` invocation on this box, output captured to the
session scratchpad, nothing installed, nothing modified.

---

## 14. Verdict on the addendum material

| Claim in my extended brief | Ruling |
|---|---|
| 1. `vulkaninfo` shows all three devices advertise `shaderInt64` and `bufferDeviceAddress` | **VERIFIED. I re-ran it myself.** And the code path goes further than either report says — see 15.2. |
| 2. The Vulkan 1.2 hypothesis is wrong | **CONFIRMED WRONG.** Record it as such. Both reports right; A's evidence is stronger. Section 16. |
| 3. Three architectural reasons, none saying the approach is wrong | **Two of three upheld as stated. The third is misassembled.** Section 17. |
| 4. Submodule pins; SPIRV-Tools PR 5316 absent from our checkout | **VERIFIED to the commit.** And I can now settle what B left open: **it cannot trigger on Taichi's output.** Section 18. |
| 5. The macOS blocker has NOT expired | **Overstated by B, better calibrated by A.** The only maintainer statement on that thread says the feature works. Section 19. |
| 6. The escalated AOT question | **The reasoning survives its first two steps and has two holes in the third and fourth.** Not deciding. Section 20. |

The single most useful new fact in this addendum is in 15.2, and neither report
has it in the form that matters: **on this box, today, with the `&& false` in
place, Taichi already enables `VK_KHR_buffer_device_address` on the device and
already requests the `bufferDeviceAddress` feature at device creation.** The
guard suppresses only the SPIR-V capability declaration and the VMA flag. The
device-side plumbing is live in every Vulkan run this project makes right now.

---

## 15. Claim 1: the hardware measurement. Verified independently.

### 15.1 I re-ran it. Both reports' numbers are right, and B's table is exact.

MEASURED. `vulkaninfo` (3922 lines, captured to the session scratchpad) and
`nvidia-smi`, both read-only. `nvidia-smi` reports two GPUs, driver 535.309.01.

| | GTX 1070 (GPU0) | GTX 750 Ti (GPU1) | llvmpipe (GPU2) |
|---|---|---|---|
| device `apiVersion` | 1.3.242 | 1.3.242 | **1.4.318** |
| `VK_KHR_buffer_device_address` | present | present | present |
| `VK_EXT_buffer_device_address` | present | present | **absent** |
| `shaderInt64` | true | true | true |
| `bufferDeviceAddress` | true | true | true |

Instance version 1.3.275. Source lines in my capture: `shaderInt64` at 1082 /
2333 / 3485; `bufferDeviceAddress` at 1126 / 2369 / 3879.

**Report B's table is exact, including the detail that `VK_EXT_buffer_device_address`
is absent on llvmpipe** — a detail that costs B nothing, since `06cb1c50d`
dropped the `VK_EXT_` alternative from the guard in February 2022, but which
shows B read the output rather than summarising it.

**Report A's table has one error and one gap.** A gives llvmpipe's `apiVersion`
as "n/a"; it is 1.4.318 and is printed in the output. A omits the `VK_EXT_`
column. Neither affects A's conclusion.

**Both reports' central claim is upheld: every clause of the guard is satisfied
on all three devices, baseline Maxwell card and CPU software renderer included.**

### 15.2 Tracing the code path against those numbers — and the finding both reports understate

The brief asks me to distinguish what a device advertises from what the code
path does. Both reports flag the distinction; A traces the path and gets it
right; neither draws out the consequence.

VERIFIED, `taichi/rhi/vulkan/vulkan_device_creator.cpp`:

- `:523`, `vk_api_version = physical_device_properties.apiVersion`. The **device's**
  version, not the instance's. 1.3.242 on both NVIDIA cards, 1.4.318 on llvmpipe.
- `:720-721`, `CHECK_VERSION(major, minor)` is
  `vk_api_version >= VK_MAKE_API_VERSION(0, major, minor, 0)`.
- `:714-717`, `CHECK_EXTENSION(ext)` searches **`enabled_extensions`**, not the
  advertised list.
- `:710`, the whole block is gated on `vk_caps().physical_device_features2`, set
  at `:369` when the instance offers
  `VK_KHR_get_physical_device_properties2`. Present here.
- `:814-815`, `CHECK_VERSION(1, 2) || CHECK_EXTENSION(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)`.
  Passes on version alone, on all three.
- `:819-820`, `CHECK_VERSION(1, 3) || buffer_device_address_feature.bufferDeviceAddress`.
  **Passes on version alone on all three**, so the queried feature bit is not
  even load-bearing here.
- `:821`, `device_supported_features.shaderInt64`. Passes on all three.
- `:825`, `#if !defined(__APPLE__) && false`. The only false term.

**The consequence neither report states plainly.** The `#if` at `:825` encloses
exactly one line, the `caps.set` at `:826`, closing at `:827`. Everything around
it is live:

- `:595-596` pushes `VK_KHR_buffer_device_address` into `enabled_extensions`
  whenever the device advertises it, in a plain `else if` chain, outside any
  `#if`.
- `:830-831` chains `buffer_device_address_feature` into the `VkDeviceCreateInfo`
  `pNext` list — outside the `#if` (closed at `:827`), outside the `shaderInt64`
  test (closed at `:828`), outside the version test (closed at `:829`).

So Taichi, today, on this box, **creates its logical device with the extension
enabled and the `bufferDeviceAddress` feature requested.** Only the SPIR-V
capability declaration (`spirv_ir_builder.cpp:73-77`, `:113-127`), the VMA
allocator flag (`vulkan_device.cpp:2509`) and the argument ABI
(`gfx_program.h:79-83`) are suppressed.

Report A gets this right at its §7.4 and uses it to dispose of issue #6368 on the
owned-device path. It is worth more than that, and it belongs in the plan rather
than inside a sub-argument: **restoring the capability is not "turning on a
Vulkan feature." The Vulkan feature is already on. It is turning on Taichi's use
of it.** That materially lowers the driver-side risk of the change and raises,
correspondingly, the weight of the untested-code risk in my section 9.

Report B does not trace the path at all and rests on the advertised values.

### 15.3 What the measurement does not establish, and both reports say so

Neither report claims the feature *works*, and both say so explicitly — B's
Escalation 3 ("Do not read 11.3 as 'it works'") and A's closing line to §7.2
("This says the gate passes. It does not say the feature works"). I confirm the
limit applies to me too: I ran a capability query, not Taichi.

I add one item that sharpens the gap. VERIFIED: **Taichi never validates its own
generated SPIR-V.** `spirv_codegen.cpp:2710`,
`spirv_opt_options_.set_run_validator(false);`. The only other SPIRV-Tools entry
point in the file is a `Disassemble` at `:2763`, inside `if constexpr (false)` at
`:2758`. A malformed physical-storage-buffer module would reach the driver
undiagnosed — which is the exact shape of the 2022 macOS symptom, silently wrong
results with no message.

Report A has this at its §7.6 and cites `:2709`; the line is `:2710`. Report B
cites `:2710` correctly in its §11.5. Both are right on substance; this is the
one place A's line numbers are looser than B's.

---

## 16. Claim 2: the Vulkan 1.2 hypothesis. Confirmed wrong.

**Record the hypothesis as wrong. Both reports say so and the source agrees.**

VERIFIED. Vulkan 1.2 promoted `VK_KHR_buffer_device_address` to core in January
2020. The three disabling commits are `f677fb141` (2022-10-24), `d20f55dc8`
(2022-10-30) and `12546578a` (2022-11-02) — two years and nine months later. API
availability cannot have been what changed.

**Report A's evidence is stronger than B's** and it is the version that should
survive. A quotes the guard as it stood in the **original introduction**,
`7705f688a`, February 2022:

```
+    if (CHECK_VERSION(1, 2) ||
+        CHECK_EXTENSION(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) ||
+        CHECK_EXTENSION(VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)) {
```

That settles it beyond timing: the code accepted both the core route and the
extension route from the day the feature was written. B establishes the same
conclusion from the current form at `:812-819` plus the calendar, which is
sufficient but weaker, because the current form is post-`06cb1c50d` and does not
by itself show what the original did.

A's account of `06cb1c50d`'s narrowing — dropping the `VK_EXT_` alternative,
adding `shaderInt64`, adding the `CHECK_VERSION(1, 3) ||` short-circuit — I
checked against the diff and it is accurate.

Both reports correctly note this removes a candidate explanation rather than
supplying one. **This is negative information and it is worth having: the
hypothesis was mine to test and it is dead.**

---

## 17. Claim 3: the three architectural reasons. Two upheld, one misassembled.

### 17.1 "The SNode root pointer is a buffer offset, not a device address" — UPHELD

This is my section 4 above, reached independently and by a different route. It
is architectural in the strict sense, it is the finding in this territory that
should reach the project owner, and both reports have it. Nothing in the new
material weakens it.

### 17.2 "`demote_dense_struct_fors.cpp:29` unchanged since October 2022" — UPHELD as a fact, and my section 5 qualification stands

VERIFIED. `git log -L 29,29:taichi/transforms/demote_dense_struct_fors.cpp`
returns exactly one commit, `b347a2159` (#6457, Yi Xu, 2022-10-28). The line has
not moved since.

But the qualification from my section 5 is untouched by the new material and
both expiry sections repeat the unqualified version. The assertion runs only via
`convert_to_range_for`, whose only caller is `maybe_convert` at
`demote_dense_struct_fors.cpp:114-118`, gated on `stmt->snode->is_path_all_dense`.

I checked one thing I had not: whether sparse trees are even reachable on the
SPIR-V path, since if they were not, the gate would always be open and the
assertion would be unconditional there. **They are reachable in the C++ core.**
`Extension::sparse` is withheld from `Arch::vulkan` at
`taichi/program/extension.cpp:26`, but `is_extension_supported` is consulted for
`sparse` in **Python only** — `python/taichi/lang/snode.py:50,76,93` and
`python/taichi/_snode/fields_builder.py:83,100,122`. The C++ callers of that
function are for `assertion`, `bls`, `mesh` and `quant` only
(`program.cpp:149`, `codegen_llvm.cpp:2726`, `compile_to_offloads.cpp:92,205,218,236,245,288`).
Plan §1.2 puts the Python frontend out of scope. **Nothing in the C++ core stops
a sparse tree reaching the SPIR-V backend**, and `bitmasked_activation` at
`spirv_codegen.cpp:383-437` is the machinery for exactly that.

So the assertion is a hard INT_MAX cap on all-dense struct-fors and does not
apply to the sparse configuration plan §4.1 requires. Both expiry sections state
it without the gate.

### 17.3 "The frontend narrows indices to int32 because some backends lack int64" — MISASSEMBLED

This is B's reason 9 and A's reason I. Upheld as a description of the code.
**Not upheld as assembled, on two counts.**

**Count one: the cited evidence is out of scope.** B rests reason 9 on issue
5320, whose recorded maintainer position B quotes at length. I verified that
quote against the issue body and it is accurate. But the artefact that issue
names — PR **#5322**, "Added warning messages for implicit type conversion for
RangeFor boundaries", `ef9cfaa72`, 2022-07-06 — changes **exactly one file**:

```
 python/taichi/lang/ast/ast_transformer.py | 37 +++++++++++++++++++++++--------
 1 file changed, 28 insertions(+), 9 deletions(-)
```

The narrowing it warns about is still there and still Python:
`boundary_type_cast_warning` at `python/taichi/lang/ast/ast_transformer.py:56-64`,
its two call sites at `:1291-1292` and `:1301`, and the
`ti_ops.cast(..., primitive_types.i32)` at `:1316-1317` and `:1359-1360`.

**Plan §1.2 puts the Python front end explicitly out of scope and not being
updated.** So the RangeFor half of reason 9 is not this project's problem, and
the issue B cites as its evidence is about that half.

The in-scope half is real and is a different artefact: the SNode index
narrowing at `taichi/ir/snode.cpp:92`, `static_cast<int>(acc_shape)`, against the
`int` field at `taichi/ir/snode.h:49`. That is C++ core, in scope, and it is the
one I named in my section 5 and Escalation 4. **Reason 9 should be split.**

**Count two: the stated rationale is environmental, and on this box it has
expired.** "Because some backends lack int64" is a claim about backends. On this
machine `shaderInt64` is true on the Maxwell baseline card, the Pascal card and
the CPU software renderer (section 15.1), and `spirv_has_int64` is produced
unguarded at `vulkan_device_creator.cpp:630-633` and consumed to emit
`OpCapability Int64` at `spirv_ir_builder.cpp:64-66`. The reason has expired.
**The code has not**, because the narrowing at `snode.cpp:92` is unconditional —
it consults no arch, no capability and no config. A backend that has int64 is
narrowed anyway.

That is the accurate classification and it is better than either report's:
**the rationale is environmental and expired; the implementation is
unconditional and therefore behaves architecturally.** B files the whole thing as
ARCHITECTURAL, which is right about the effect and wrong about the reason. A's
reason I, "a deliberate limit reaffirmed twice by maintainers," is closer but
does not test whether the premise still holds.

**"Proposed in 2022 and never built" is VERIFIED.** No per-backend dtype demotion
pass exists. `taichi/transforms/` contains `demote_atomics.cpp`,
`demote_dense_struct_fors.cpp`, `demote_mesh_statements.cpp`,
`demote_no_access_mesh_fors.cpp` and `demote_operations.cpp`.
`demote_operations.cpp` is operation lowering — its own header comment says
"Demote Operations into pieces for backends to deal easier" — and it consults
neither `arch` nor `config` anywhere in the file. Nothing implements
jim19930609's July 2022 proposal.

### 17.4 "None of them says the approach is wrong" — UPHELD

I looked for a contrary statement independently in my section 11 item 7 and
found none, and nothing in the new material changes that. The only recorded
design argument, on PR #3177, went in favour of widening, with ailzhang
overruling k-ye. **Upheld for both reports.**

But the corollary should be stated the way my section 2.4 states it and neither
expiry section does: not judged wrong, not reusable as written, **and
structurally prerequisite** for any SNode addressing that exceeds a bound
buffer's reach.

---

## 18. Claim 4: the submodule pins and SPIRV-Tools PR 5316

### 18.1 The pins. Verified to the commit, and both tables are right.

VERIFIED by resolving every gitlink under `external/` against each submodule's
own history:

| Submodule | Pin | Revision date |
|---|---|---|
| Vulkan-Headers | `409c16be50` | 2025-04-18 |
| VulkanMemoryAllocator | `539c0a8d8e` | 2025-04-14 |
| volk | `b87f88292b` | 2023-03-24 |
| SPIRV-Reflect | `7c9c841fa9` | **2023-02-01** |
| SPIRV-Headers | `34d04647d3` | 2022-12-15 |
| SPIRV-Cross | `c77b09b57c` | 2022-11-21 |
| SPIRV-Tools | `46ca66e699` | 2022-11-18 |

**Report A's table is the more complete of the two** — it includes
VulkanMemoryAllocator and SPIRV-Reflect, which B's §11.4 table omits.

**B's claim about which Taichi commits last moved them is exactly right** and I
verified it: all four SPIR-V submodules last moved in `ee0af32ab` (#7676,
2023-04-03, "[vulkan] Update SPIR-V codegen to emit FP16 consts"); Vulkan-Headers
and VMA last moved in `4c2bac69e` (#8680, 2025-04-25).

**One correction to the restatement in my brief**, which says the four SPIR-V
submodules "sit at November and December 2022." Three do. **SPIRV-Reflect sits at
2023-02-01.** B's Escalation 2 makes the same slip, listing SPIRV-Reflect among
those "left at late 2022." A's table has the right date.

**And a distinction worth keeping that both reports blur.** The gitlinks were
last *touched* in April 2023, not abandoned in 2022. The revisions they point at
are from November 2022 to February 2023. So this is not neglect dating to the
shelving; it is a deliberate pin as of April 2023 that has not been revisited.
Relevant to 18.2: PR 5316 merged 2023-07-17, **three months after Taichi's last
bump**. Nobody skipped a fix that existed; the fix postdates the last look.

B's header-level check is accurate: `VK_HEADER_VERSION 313` at
`external/Vulkan-Headers/include/vulkan/vulkan_core.h:71`, complete version
1.4.313 at `:74`; `AddressingModelPhysicalStorageBuffer64 = 5348`,
`StorageClassPhysicalStorageBuffer = 5349` and
`CapabilityPhysicalStorageBufferAddresses = 5347` at
`external/SPIRV-Headers/include/spirv/unified1/spirv.hpp:106`, `:229`, `:1062`
respectively. All three verified. A's independent check — 64
`PhysicalStorageBuffer` hits in `external/SPIRV-Tools/source/` — also reproduces
exactly.

A's second qualification is sound and B does not make it: **SPIRV-Cross does not
bear on this question.** I re-ran A's grep; `spirv_cross` appears only under
`taichi/rhi/metal/`, `taichi/rhi/dx/` and `taichi/rhi/opengl/`, none of which
ever set the capability. Of the three stale pins only SPIRV-Tools and
SPIRV-Headers can matter here.

### 18.2 PR 5316 — verified, and I can settle what B left open

**The claim is VERIFIED to the letter.** SPIRV-Tools PR **5316**, "[spirv-opt]
Handle OpFunction in GetPtr", merged **2023-07-17T19:16:25Z**, three files
changed: `source/opt/aggressive_dead_code_elim_pass.cpp`,
`source/opt/mem_pass.cpp`, `test/opt/aggressive_dead_code_elim_test.cpp`. Body
verbatim:

> When using PhysicalStorageBuffer it is possible for a function to return a
> pointer type. This was not being handled correctly in
> GetLoadedVariablesFromFunctionCall in the DCE pass.

Our pin predates it by eight months. Taichi registers
`spvtools::CreateAggressiveDCEPass()` at `spirv_codegen.cpp:2690` and
`CreateInlineExhaustivePass()` at `:2688`. All confirmed.

**And I can close one thing B could not, in B's favour.** B recorded the pass
registration but wrote: "NOT ESTABLISHED: whether Taichi's generated SPIR-V can
trigger it." I checked, and it cannot.

VERIFIED, and it is decisive:

- `taichi/codegen/spirv/spirv_ir_builder.h:406-410`, with the comment on the
  function itself: `// NOTE: only support void kernel function, i.e. main`.
  `new_function()` returns a value of type `t_void_func_`.
- `spirv_codegen.cpp:132` calls it exactly once per module, for `main`.
- **`OpFunctionCall` appears nowhere in the Taichi tree.** `grep -rn
  "OpFunctionCall" taichi/` returns nothing. The only `OpFunction` emission is
  `spirv_ir_builder.h:445-447`, for `main` itself.
- The upstream fix is inside `GetLoadedVariablesFromFunctionCall`, which opens
  `assert(inst->opcode() == spv::Op::OpFunctionCall)` at
  `external/SPIRV-Tools/source/opt/aggressive_dead_code_elim_pass.cpp:440`.
- The optimizer is run at one site, `spirv_codegen.cpp:2746`, on
  `task_res.spirv_code` — Taichi's own output. It is never run on foreign SPIR-V.

A Taichi module contains one function, `void main()`, no parameters, no calls.
The bug needs a call returning a pointer. **It cannot trigger.**

**A finding that cuts the other way, and neither report checked it.** B's
concern would have been defused entirely if the optimizer did not run by
default. It does. `params.enable_spv_opt = compile_config.external_optimization_level > 0`
at `taichi/runtime/gfx/runtime.cpp:800` and
`taichi/codegen/spirv/kernel_compiler.cpp:38`, and
`external_optimization_level = 3` by default at
`taichi/program/compile_config.cpp:13`. **All twenty-four registered passes run
on every SPIR-V kernel by default.** So B's line of concern was the right one to
pursue; it simply happens to close on the `OpFunctionCall` fact rather than on
the passes being off.

**Ruling: B's item 6 is verified as an accounting of what our checkout lacks, and
correctly hedged. The specific fault it names cannot reach us.** B's Escalations
2 and 4 — that the wider toolchain gap is unsized and that this was a
single-keyword search — remain entirely valid and I did not narrow them.

---

## 19. Claim 5: the macOS blocker. B overstates it; A is better calibrated.

The three MoltenVK issues exist and are open, exactly as cited. VERIFIED:

| Issue | Opened | State | Comments |
|---|---|---|---|
| 2174 "regression bug with vkGetBufferDeviceAddress" | 2024-03-07 | open | 1 |
| 2278 "GPU address fault error when using descriptor indexing + variable descriptor count" | 2024-07-19 | open | 7 |
| 2696 "VK_ERROR_OUT_OF_DEVICE_MEMORY when using device address buffer with indirect draw/dispatch" | 2026-03-06 | open | 1 |

B's quotation from 2174 is verbatim and accurate.

**But B's heading — "The macOS blocker is environmental and has NOT expired" —
is stronger than the thread supports, and B omits the only maintainer statement
on it.** Read in full, issue 2174 is:

- a report from a **2016 Intel MacBook Pro, AMD Radeon Pro 455, macOS 12.7.3**,
  using **custom builds** of MoltenVK with a swapped-in SPIRV-Cross;
- answered once, by maintainer **billhollings**, 2024-03-12: "`VK_KHR_buffer_device_address`
  and `VK_EXT_buffer_device_address` are supported in the latest MoltenVK, and
  are currently passing about **90% of the CTS tests**. It's possible you are
  encountering an issue, but we'd need more information to determine that.";
- never followed up by the reporter.

An unconfirmed report on eight-year-old Intel Mac hardware, answered by a
maintainer saying the feature works and passes most of conformance, is not
"the blocker has not expired." It is unresolved conformance on one vendor.

**Report A gets this right.** A quotes the same maintainer comment, which B
omits, and grades it "Largely expired, not cleanly" — which is what the thread
says. A also draws the sharper point: since nobody ever established whether the
2022 Taichi fault was MoltenVK's or Taichi's, **the Taichi-side possibility has
never been eliminated.** That matters more than the MoltenVK status, and it is
the hole in claim 6 that I develop in section 20.

**One loose citation in A.** A groups issue **2278** with 2174 and 2696 as
"further open device-address reports". 2278 is a GPU address *fault* arising
from descriptor indexing with variable descriptor count, from the vkd3d test
suite on an M2 MacBook Air. It is not a `vkGetBufferDeviceAddress` report. The
grouping is a stretch; drop it or relabel it.

**A scoping point both reports make too quickly.** Both dismiss macOS as "not a
target tier" (A §7.3) or "a platform absent from plan section 5.2" (B §11.1). The
tier table is indeed silent on Apple. But plan §2 makes **vendor and hardware
agnosticism the deliverable**, §5.2 says the three cards "are not a vendor
target" and are incidental, and §5.2's consequence clause says a portable path
served worse than a vendor path is "a GAP IN THE PORTABLE PATH to be recorded."
Under the plan's own logic macOS is not out of scope; it is merely hardware the
owner cannot test on. A capability that works on NVIDIA and silently returns
wrong data on Apple is precisely a §2.2 event — hardware changing capability, not
speed. I am not deciding this. I am recording that "not in the tier table" is not
the same as "not a concern," and both reports use it as if it were.

---

## 20. Claim 6: the escalated AOT question. Testing the reasoning, not deciding it.

Both reports arrive here and both correctly refuse to decide it. A's §7.7 D and
§7.8.3, B's §11.7 and Escalation 12.1. The chain is:

1. The global kill was an AOT cross-machine portability measure.
2. Enforced by strict equality at `c_api/src/taichi_gfx_impl.cpp:22-29`.
3. Plan §5's install-time configuration model may be immune to that concern.
4. If so, the only surviving reason for the global guard is macOS, which the
   `#if !defined(__APPLE__)` clause already handles on its own.

### 20.1 Step 2 is fully verified, and narrower than either report says

VERIFIED, `c_api/src/taichi_gfx_impl.cpp:18-29`. The loop walks the module's
required capabilities and returns `TI_ERROR_INCOMPATIBLE_MODULE` on
`current_version != required_version`. An inequality. A module is rejected by a
runtime that is *more* capable as well as one that is less.

**And it exists in exactly one place.** `grep -rn "get_required_caps\|required_devcaps\|TI_ERROR_INCOMPATIBLE_MODULE"`
over `taichi/` and `c_api/src/` returns the declaration at
`taichi/aot/module_loader.h:97`, this one consumer at `taichi_gfx_impl.cpp:20-27`,
and the error-string mapping at `taichi_core_impl.cpp:94`. **Nothing on the JIT
path performs any capability comparison.**

That strengthens step 3's premise more than either report claims: the check is
not merely associated with AOT, it is reachable *only* through
`ti_load_aot_module` on the C API. A project that compiles kernels in-process
never executes it.

### 20.2 Step 1 remains INFERRED, and my earlier objection to B stands

Neither report can prove step 1 — upstream never enumerated the "multi-platform
compatibility issues," and both say so. My section 10 objection B4 is unchanged
by the new material: turning off this one capability does not make modules
portable, because the same equality check still bites on `spirv_has_int64`,
`spirv_has_float64`, `spirv_has_int16` and the subgroup capabilities, all
device-derived at `vulkan_device_creator.cpp:626-671`. B's §2.3 "portable at a
stroke" remains an overstatement.

The best answer to my own objection — which neither report offers — is that this
capability is not merely a tag: it changes the **argument ABI**,
`gfx_program.h:79-83`, `"1b"` versus `"1-"`. A module built with it has a
different host/device struct layout, not just a different capability level. That
distinguishes it from `spirv_has_int64` and would explain singling it out. I
offer it as the strongest available reconstruction of step 1 and mark it
INFERRED. Report A independently reaches the ABI point at its §7.7 D and grades
it ARCHITECTURAL, which I agree with; A does not connect it to why this
capability specifically was killed.

### 20.3 Two holes in steps 3 and 4

**Hole one, and it is the serious one. Step 4's enumeration cannot be shown
complete, and one known unknown sits directly inside it.** Step 4 says the only
surviving reason is macOS and that the platform clause contains it. That holds
only if the 2022 fault was MoltenVK's. **Nobody ever established that.**
strongoier's own words, issue 6295 comment 1288522905: "We can diagnose further
about whether the problem comes from the feature itself or the use of the
feature in Taichi in the future." No such diagnosis exists.

If the fault was in Taichi's *use* of buffer device address, it is not
platform-specific, `#if !defined(__APPLE__)` does not contain it, and step 4
fails. Report A notices the open root cause at §7.3 and at its Escalation 3 but
does not carry it into the escalated question. Report B lists it under NOT
ESTABLISHED and likewise does not connect it. **It is the load-bearing unknown
for the decision the project owner is being asked to take, and neither report
puts it there.**

**Hole two: step 3 grants immunity from a concern whose relief buys nothing on
its own.** Both reports' central finding — my section 2 — is that the capability
does not touch SNode addressing. So the escalated question, taken alone, is
whether to lift a guard that changes nothing for item 6.2. Its value is in
ndarray base addressing (and not even ndarray *indexing*, my section 2.5) and as
the prerequisite machinery of my section 2.4. Neither report says this when
framing the escalation, and it changes what the project owner is deciding about.

### 20.4 A §2.2 consequence neither report identifies

Plan §2.2 is a test to apply to every proposed change: a change that lets some
hardware do something other hardware cannot do at all is a violation.

Applying it to a restoration, in two stages:

- **Restoring the capability as it stands passes.** The capability would be set
  where the device advertises it and not otherwise. The SNode path is unchanged
  either way, and the ndarray path works both ways — the `"1-"` branch is the one
  running today. What varies is the argument ABI and the SPIR-V addressing model:
  a build-artifact variant, which is exactly what plan §5.1's install-time
  configuration model is for. Fidelity and packing, not capability.
- **The second stage does not pass, and it is the stage item 6.2 would need.** If
  the physical-storage-buffer mechanism were later used to give the SNode root a
  device address, per my section 2.4, then hardware without
  `bufferDeviceAddress` could not carry a root beyond a bound buffer's reach,
  while hardware with it could. **That is hardware changing capability, which is
  the §2.2 violation by name.**

Section 15.1 shows all three devices on this box clear the gate, so the violation
does not bite here. It would bite on any device that does not — and plan §5.2's
own text says the three cards are incidental and that the portable path is what
correctness is measured against.

I am not deciding this and it is not mine to decide. **I am recording that the
§2.2 test gives different answers to "restore the capability" and to "use the
capability for SNode addressing," and that no document in this territory
currently distinguishes them.** That distinction should be in front of the
project owner alongside the escalated question, because the second stage is the
one that serves item 6.2.

---

## 21. Errors in the addendum material, itemised

### Report 05 (pass A), section 7

| # | Location | Error | Severity |
|---|---|---|---|
| A11 | §7.2 table | llvmpipe `apiVersion` given as "n/a". It is 1.4.318 and appears in the output. | Trivial. |
| A12 | §7.2 | Cites `:813`, `:824`, `:828-829`. The guard `if` is `:814`, the `#if` is `:825`, the `pNext` chain is `:830-831`. `:523`, `:819`, `:821`, `:595-596` are exact. | Trivial; the trace itself is correct and is A's best contribution here. |
| A13 | §7.6 | `set_run_validator(false)` cited at `:2709`; it is `:2710`. | Trivial. |
| A14 | §7.3 | Groups MoltenVK issue 2278 with the device-address reports. 2278 is a descriptor-indexing GPU address fault, not a `vkGetBufferDeviceAddress` report. | Minor. |
| A15 | §7.7 I | "The 32-bit index width" folded into one reason without separating the in-scope C++ artefact (`snode.cpp:92`, `snode.h:49`) from the out-of-scope Python one, and without testing whether the "backends lack int64" premise still holds. | Moderate. Section 17.3. |
| A16 | §7.7 E | The module-wide addressing model still graded ARCHITECTURAL and "permanent". My section 3 stands: upstream shipped mixed logical and physical addressing in one module through eight releases. | **Material**, carried over. |
| A17 | §7.2 | Omits the `VK_EXT_buffer_device_address` column; llvmpipe lacks it. Harmless because `06cb1c50d` dropped that route in Feb 2022, but the table is presented as a capability read-out. | Trivial. |

A's §7.4 is the strongest single piece of new work in either expiry section, and
I verified every line of it. A's §7.6 qualification that SPIRV-Cross is
irrelevant is correct and B does not make it. A's §7.1 quotation of the original
guard is better evidence than B's for claim 2.

### Report 05b (pass B), sections 11 and 12

| # | Location | Error | Severity |
|---|---|---|---|
| B10 | §11.6 heading | "The macOS blocker is environmental and has NOT expired." Overstated. Omits the only maintainer comment on issue 2174, which says the feature works and passes about 90% of CTS, and omits that the reporter is on a 2016 Intel Mac with custom MoltenVK builds. | **Material.** Section 19. |
| B11 | §11.1 reason 9, §11.4 | Reason 9 rests on issue 5320, whose artefact (PR 5322, `ef9cfaa72`) changes only `python/taichi/lang/ast/ast_transformer.py` — out of scope per plan §1.2. The in-scope artefact is `snode.cpp:92` / `snode.h:49`. And the stated rationale is environmental and has expired on this box, while the code is unconditional. | **Material.** Section 17.3. |
| B12 | Escalation 12.2 | Lists SPIRV-Reflect among submodules "left at late 2022". It is pinned at 2023-02-01. | Trivial. |
| B13 | §11.4 table | Omits VulkanMemoryAllocator and SPIRV-Reflect, which A includes. | Minor. |
| B14 | §11.1 reason 10, §11.3 | The `demote_dense_struct_fors.cpp:29` assert presented without the `is_path_all_dense` gate. | **Material**, carried over from my section 5. |
| B15 | §11.5 | "NOT ESTABLISHED: whether Taichi's generated SPIR-V can trigger it." Correctly hedged, and now settled: it cannot. Taichi emits one `void main()` per module and never emits `OpFunctionCall`. | Not an error. Recording that the open item closes. |
| B16 | §11.3 | Rests on advertised values without tracing the code path. Misses that the extension is already enabled and the feature already requested at device creation today. | Moderate. Section 15.2. |

B's §11.4 and §11.5 are the strongest single pieces of new work on B's side: the
submodule accounting is exact to the commit, the PR 5316 identification is
precise, and B correctly refused to assert a fault it had not established.

### Where the two now disagree, and who the source supports

| Question | A | B | Source supports |
|---|---|---|---|
| Vulkan 1.2 hypothesis | wrong, shown from the original Feb 2022 guard | wrong, shown from the current guard plus the calendar | **Both. A's evidence is stronger.** |
| llvmpipe `apiVersion` | "n/a" | not stated | **Neither.** It is 1.4.318. |
| `VK_EXT_buffer_device_address` on llvmpipe | not stated | absent | **B.** |
| Is the extension already enabled today? | yes, `:595-596`, `:828-829` | not addressed | **A**, with the `pNext` line at `:830-831`. |
| SPIRV-Cross relevance | irrelevant; only Metal/DX/OpenGL consume it | not addressed | **A.** |
| SPIRV-Reflect pin date | 2023-02-01 | "late 2022" | **A.** |
| Which Taichi commits last moved the pins | not stated | `ee0af32ab`, `4c2bac69e` | **B**, exactly. |
| SPIRV-Tools PR 5316 | not found | found, hedged | **B.** And it cannot trigger. |
| macOS status | largely expired, not cleanly | has NOT expired | **A.** |
| Validator disabled | `:2709` | `:2710` | **B.** |
| Optimizer runs by default? | not addressed | not addressed | **Neither.** It does: `external_optimization_level = 3`. |

---

## 22. Escalations, addendum

Section 12 above stands. These are additional.

7. **The §2.2 test splits into two different answers and no document says so.**
   Restoring the capability passes the test; using it for SNode addressing does
   not, because hardware lacking `bufferDeviceAddress` could then not carry what
   hardware with it could. Section 20.4. This belongs in front of the project
   owner beside the escalated question, since the second stage is the one that
   serves item 6.2.

8. **The load-bearing unknown for the escalated question is the 2022 macOS root
   cause, and it is unresolvable from here.** Step 4 of the reasoning holds only
   if the fault was MoltenVK's. strongoier explicitly left that open and nobody
   returned. If it was Taichi's use of the feature, the fault is not
   platform-specific and the `#if !defined(__APPLE__)` clause does not contain
   it. Both reports have the fact; neither carries it into the decision.
   Section 20.3.

9. **"Not in the tier table" is being used as "not a concern," for macOS.** Plan
   §2 makes vendor agnosticism the deliverable and §5.2 says the three cards are
   incidental. A capability that works on NVIDIA and silently returns wrong data
   elsewhere is a §2.2 event. Not mine to rule on; flagging that both reports
   dismiss it on a tier-table argument the plan does not support.

10. **Reason 9 should be split before it enters the plan.** The RangeFor
    narrowing is Python and out of scope; the SNode narrowing is C++ and in
    scope; and the rationale behind both is environmental and has expired on this
    box while the code remains unconditional. Section 17.3.

11. **I ran capability queries, not Taichi.** Section 15 establishes that the
    devices advertise everything the guard tests and that the device-side
    plumbing is already live. It does not establish that Taichi's
    physical-storage-buffer code path produces correct results. Both reports say
    the same of themselves. Settling it needs a build with the guard lifted and a
    run, which no one in this territory is authorised to do.

12. **The toolchain gap remains unsized.** I closed the one item B found. B's own
    Escalations 2 and 4 stand untouched: the search was a single keyword against
    one submodule, SPIRV-Cross was not assessed at all, and three years of
    SPIR-V toolchain changes are unaudited. Sizing it is a separate assignment.

---

## 23. Cross-adversary, re-checked

Re-checked for `/opt/project/taichi/modernization/investigation/adversary-05-2.md`
after completing this addendum. **It still does not exist.** No divergence
section can be written.

If adversary 2 lands later, the places where I depart from both reports rather
than from one, and which should be tested first, are: my section 3 (the
module-wide addressing model is not architectural), my section 5 and 17.2 (the
`is_path_all_dense` gate), my section 2.4 (the physical-storage-buffer mechanism
as prerequisite rather than irrelevant), my section 17.3 (reason 9 is
misassembled and half of it is out of scope), my section 18.2 (PR 5316 cannot
trigger), and my section 20.4 (the §2.2 test splits).

---
---

# 24. Divergence from adversary-05-2

`adversary-05-2.md` did not exist when I wrote sections 13 and 23. It landed
between that check and my final one. Sections 13 and 23 are superseded by this
section; everything else above stands except where I explicitly withdraw it
below.

Adversary 2 had already read my file and written a divergence appendix (its
§12). I have re-derived every claim it makes against me. **It is right on four
of five, and I concede those without reservation. On the fifth we disagree, and
the disagreement turns out to be definitional rather than factual.**

---

## 24.1 Conceded: live producers exist. My section 11 item 5 was too broad.

**Adversary 2's §1.3 is correct and it is the most consequential finding any of
the four documents in this territory has produced. I was wrong to file "no live
producer" under what I could not break.**

I verified the whole chain myself rather than taking it:

- `taichi/program/program.cpp:514-539`, `translate_devcaps`, takes an arbitrary
  `std::vector<std::string>`, splits on `=`, and calls `str2devcap(key)` at
  `:530` followed by `cfg.set(devcap, value)` at `:531`. **No device is
  consulted anywhere in the function.**
- `Program::make_aot_module_builder` at `:541-557` passes that config straight
  to `program_impl_->make_aot_module_builder(cfg)` at `:554`.
- `taichi/runtime/gfx/aot_module_builder_impl.cpp:21` stores it as `caps_`, and
  `:75` and `:119` pass it to `compilation_manager_.load_or_compile(config_,
  caps_, *kernel)` — which is what drives SPIR-V codegen.
- `spirv_has_physical_storage_buffer` is a valid key: `str2devcap` is generated
  from `taichi/inc/rhi_constants.inc.h:28`.
- `c_api/src/taichi_core_impl.cpp:317-334`, `ti_set_runtime_capabilities_ext`,
  builds a `DeviceCapabilityConfig` from caller-supplied
  `TiCapabilityLevelInfo` values and calls
  `runtime2->get().set_caps(std::move(devcaps))` at `:331`, replacing the
  device's capability set wholesale. No validation.

**My section 11 item 5 said "No live producer on any ref after 2022-11-02" and
described it as reproducing pass A's sweep.** The sweep is correct and I
reproduced it correctly, but it answers only "does any committed line in
`vulkan_device_creator.cpp` set the capability." I let it stand for the broader
proposition. My section 8 came within one step of the counter-example — "a
caller can now declare the requirement, and the producers are still off" — and
did not take it. **I withdraw the broader reading.**

The corrected statement, which I endorse in adversary 2's words: the capability
is not off, it is **un-defaulted**. It is user-settable, on any machine, with no
device check in the chain, and it drives real codegen when set.

This changes my own section 9 materially. I wrote there that re-enabling means
turning on branches that "have not executed since 2022-10-30." That remains true
of the *default* path and true of CI, since the coverage grep still returns
nothing. It is not true as a statement about reachability. **The untested code is
reachable today by a user who passes one string.** That is a worse risk posture
than either report or my section 9 described, and it bears directly on plan §5's
install-time configuration model and §6.3's adaptive module loading, because the
mechanism those items need already exists and is unguarded.

## 24.2 Conceded: `snode_struct_compiler.cpp:52-54`. I missed it, and it strengthens my own section 2.4.

**Adversary 2's §1.4 is correct.** Verified:

`taichi/codegen/spirv/snode_struct_compiler.cpp:34-63`, function `construct`,
at `:52-54`:

```cpp
      if (sn->type == SNodeType::pointer) {
        cell_type = ir_module.emplace_back<PhysicalPointerType>(cell_type);
      }
```

`PhysicalPointerType` translates at `taichi/codegen/spirv/spirv_types.cpp:433-439`
to `OpTypePointer` with `spv::StorageClassPhysicalStorageBuffer` — the storage
class that exists only under the capability. Its only caller is commented out at
`snode_struct_compiler.cpp:16-19`; the fields it would populate are commented out
at `snode_struct_compiler.h:48-50`.

**This is the single line in the tree connecting the physical storage buffer to
SNode addressing, and all four prior documents concluded no such connection
existed.** I checked it hardest of anything in this addendum because it is the
closest thing to a refutation of the convergent claim.

It does not refute it. The claim as both reports state it is about live code, and
this code has never been live. But it does what adversary 2 says it does for me
personally: **my section 2.4 argued that the physical-storage-buffer mechanism is
structurally prerequisite for an SNode root that outgrows a descriptor-bound
buffer, and I marked the Vulkan half INFERRED on the grounds that "Taichi has no
source line on this." Taichi has a source line on this, and I did not find it.**
The author of the mechanism reached the same design four months after landing it,
applied it to sparse `pointer` nodes specifically, and shipped it commented out.

The inference is still an inference — the code never ran and cannot be run — but
it is no longer mine alone. I accept adversary 2's upgrade to my §2.4 and my
Escalation 1.

Adversary 2 is also right that report B's §2.5 wrongly lists
`spirv_types.cpp:433-439` among the live consumers: with the only real
construction site dead and `spirv_types.cpp:337` merely copying an existing
instance inside the type reducer, that visitor never runs. My section 2.2
mentioned B's citation of those lines without noticing they are unreachable.
**Add that to my error list for B as B17, and to my own.**

## 24.3 Conceded: my tag census is three tags short. Adversary 2's is complete.

**Verified. Adversary 2 is right and my section 7 table is wrong.** I re-ran the
census with a guard classifier over both file paths and the correct totals are
**24 tags: 11 ENABLED, 3 MACOS-EXCEPTED, 10 DISABLED.**

The three I missed are `v0.9.0`, `v0.9.1` and `v0.9.2`, all ENABLED. My error was
mechanical and I can name it: my second pass, which caught the
`taichi/backends/vulkan/` → `taichi/rhi/vulkan/` move, only ran over v1.0.0
through v1.1.0, and my later 350-ref sweep piped through `sort | tail -30`, which
truncated exactly the oldest entries. Adversary 2 also correctly notes that
v0.9.x uses `ti_device_->set_cap(...)` rather than `caps.set(...)`, which is why
a setter-shaped grep misses them.

v0.9.0 is dated 2022-02-22, a fortnight after `7705f688a`. It is the first
release carrying the feature and the right start of the range.

Our disabled rows agree exactly. **The corrected census strengthens the point
both of us make against both reports' "architectural" grading of the addressing
model: eleven tagged releases, not eight, shipped
`AddressingModelPhysicalStorageBuffer64` in the same module as 32-bit
bound-buffer SNode addressing.**

## 24.4 Conceded: `make_pointer` bypasses the `shaderInt64` guard. I missed a third failure mode.

**Adversary 2's §2b finding is correct and I verified every link.**

- `make_pointer` at `spirv_codegen.cpp:2317-2324` calls `ir_->u64_type()`.
- `u64_type()` at `spirv_ir_builder.h:532-534` is `return t_uint64_;`. No guard.
- `t_uint64_` is assigned only inside `if (caps_->get(cap::spirv_has_int64))` at
  `spirv_ir_builder.cpp:166-169`.
- The `TI_ERROR("Type {} not supported.")` guards are at `:311-313` and
  `:325-327`, inside `get_primitive_type(DataType)` — which `make_pointer` never
  calls.

So on a device without `shaderInt64`, flipping the flag yields a
default-constructed `SType` with no declared type id and **no diagnostic**. My
section 4.1 examined both reports' `bitmasked_activation` reasoning line by line
and did not look upstream of it at the type accessor. This is a third failure
mode, it is the only silent one of the three, and adversary 2 is right that under
plan §2.2 it is the worst available shape: correct output on a card with
`shaderInt64`, silently wrong output on one without.

It does not bite on this box — section 15.1 shows `shaderInt64` true on all three
devices — which is precisely why it would not be caught here.

## 24.5 Conceded in part: PR #8315's human summary

Adversary 2 is right that I read only half of that PR body. I weighed
`715a04c98` low because its "fixes a bug in the `make_pointer` function" line is
Copilot-generated. **That grading is correct — I re-read the body and the line
sits under a block headed "Generated by Copilot at 0eec54b".** But the
human-written Brief Summary above it reads:

> With the specific case of shared memory atomic floats, the `at_buffer` command
> will be called with wrong pointer dtype. Causing the codegen attempting to
> generate offset arithmetic with actual pointer type, which is invalid

So the human content of #8315 is about the dtype discriminator, not about the
dead branch. Adversary 2 is right that this makes my dismissal correct on the
changelog and slightly too quick on the commit. It does not move the momentum
ruling, which adversary 2 concedes to me on the strength of the seven artefacts
in my section 8. Adversary 2 explicitly withdraws its implication that report B
was right to hedge as hard as it did; I take that as settled between us.

## 24.6 Not conceded: the `at_buffer` collision. My justification was wrong; the ruling was not.

This is our one substantive disagreement and it deserves the space.

**Adversary 2 is right that my justifying sentence was wrong, and I withdraw
it.** I wrote in section 4: "The type carries two incompatible meanings and the
code has no third thing to disambiguate them." A `ValueKind` field does exist, at
`spirv_ir_builder.h:88`, defaulting to `kNormal`, with nine members at `:69-79`
including `kPhysicalPtr`. It is read: `spirv_ir_builder.cpp:1271-1275` in
`load_variable` and `:1287-1290` in `store_variable` both branch on
`pointer.flag == ValueKind::kPhysicalPtr`. `register_value` at `:1300-1309` and
`query_value` at `:1311-1317` do carry the whole `Value` by value, so the field
survives that round trip. Adversary 2 checked three things and all three check
out.

**But adversary 2's two supporting claims are both wrong, and they are the ones
its conclusion rests on.**

**First: the tag is not "already populated on the physical branch" at the point
of decision.** It is set at `spirv_codegen.cpp:2203` on `at_buffer`'s **output**,
after the decision has been taken, so that `load_variable` and `store_variable`
downstream know to emit a physical access. It is not set on `at_buffer`'s input.
The value `at_buffer` inspects at `:2197` arrives untagged.

**Second, and this is the one that decides it: `make_value` erases the tag on
every arithmetic instruction.** `spirv_ir_builder.h:289-298`:

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

Every SSA value in this backend is born `kNormal` and is upgraded only when the
SPIR-V **type** is a pointer type. An integer byte offset, `u32` or `u64`, has
`out_type.flag == TypeKind::kPrimitive`, so the result is `kNormal` regardless of
what its operands were tagged.

Trace it on both paths:

- `make_pointer` → `uint_immediate_number` → `get_const` →
  `new_value(dtype, ValueKind::kConstant)` at `spirv_ir_builder.cpp:1502`. Tag
  `kConstant`.
- `GetChStmt` at `spirv_codegen.cpp:371-372`, `ir_->add(input_ptr_val, offset)` →
  `DEFINE_BUILDER_BINARY_USIGN_OP` at `:1038-1047` → `make_value(OpIAdd, ...)` →
  **`kNormal`.** Tag gone at the first arithmetic op.
- `SNodeLookupStmt` at `:506-508`, `mul` then `add` → **`kNormal`.**
- And decisively: the **ndarray** path too. `visit(ExternalPtrStmt *)` at
  `:790-792` does `ir_->add(addr, OpSConvert(...))` → **`kNormal`.**

So with the capability on today, the genuine 64-bit device address reaching
`at_buffer` from `:790` is tagged `kNormal`, exactly like a root offset would be.
**`at_buffer` cannot be reading the tag, because the tag is `kNormal` on both
sides of the decision.** That is not an oversight in `at_buffer`; it is why the
dtype sniff is there.

**What this does to adversary 2's remedy.** Adversary 2 describes the fix as
"a local change to `at_buffer`, `load_buffer`, `store_buffer` and the four
`make_pointer` call sites." It is not. Tagging at production only works if the
tag survives to consumption, and it does not. Making it survive means changing
`make_value` at `spirv_ir_builder.h:291` — **the single constructor for every SSA
value the SPIR-V backend emits** — and defining a propagation rule for mixed
operands, since `add`, `mul`, `sub`, the shifts and the bitwise ops all route
through it. That is a change to the backend's value model, not to four call
sites.

**Where that leaves the classification, honestly.** With my justification
withdrawn, the disagreement is definitional, and I want to state both readings
rather than win on wording:

- **My reading, which I take from the planner's brief:** architectural means it
  does not expire — no passage of time, no driver, no toolchain bump, no hardware
  fixes it; only a Taichi decision does. The collision meets that test. So does
  the `make_value` propagation problem behind it.
- **Adversary 2's reading:** architectural means "a property that survives
  independent of time, hardware **or decision**." Under that definition the
  collision is not architectural — but neither is almost anything, since any code
  can be rewritten.

I hold my ruling under the first reading and I do not think adversary 2's
evidence touches it. But I acknowledge the second reading is coherent and that
the difference between them changes how item 6.2 is sized, which is exactly the
kind of thing the brief says goes to the arbiter rather than getting settled
between adversaries. It is Escalation 13.

**On the facts we do not disagree at all:** flipping the flag breaks the build,
verified independently four times now, and adversary 2 says so explicitly.

## 24.7 Where adversary 2 improves on me without disagreeing

Recorded because it is better than what I wrote and should carry forward.

Adversary 2's §1.5 reformulates the convergent claim as **"one route with a
type-sniff switch"** rather than two disjoint routes. That is more accurate than
my section 2.1 and more useful: `at_buffer` at `spirv_codegen.cpp:2194-2219` is
the single entry point for every buffer access, ndarray and SNode alike, and the
branch at `:2197` reads the **width of the pointer value as the addressing
scheme**. My `ptr_to_buffers_` partition is true and shows the two paths never
share a buffer, but adversary 2's formulation explains *why* widening the SNode
offset is blocked, in one sentence. **Adopt it.**

## 24.8 Where we independently converged

Two findings now stand from two blind adversarial passes and should be treated as
settled, as adversary 2 says in its §12.4:

1. **The module-wide addressing model is not an architectural obstacle.** My
   section 3, adversary 2's §2a. Both of us reject both reports on this, by
   different routes: I rested it on the one-module-one-entry-point fact at
   `spirv_codegen.cpp:97/130-156` plus the shipped releases; adversary 2 reached
   the same place. With the corrected census, eleven tagged releases shipped the
   mixed model.
2. **The consumer inventories in both reports are incomplete, and the same three
   consumers are missing from both.** My section 2.2, adversary 2's §1.2.

## 24.9 Net, and what remains open

**Adversary 2's file is the stronger of the two on completeness. Mine is
stronger on the code paths I traced in the addendum** — the device-creator trace
in section 15.2, the SPIRV-Tools PR 5316 closure in section 18.2, the
`is_path_all_dense` gate in sections 5 and 17.2, and the reason-9 scoping in
section 17.3, none of which adversary 2 covers, since its file predates the
expiry extension. Neither of us caught what the other did on the four points in
24.1 through 24.5, and I came off worse on those.

Open between us, for the arbiter:

1. **Whether the `at_buffer` collision is architectural.** Definitional, not
   factual. Section 24.6 states both readings and the source facts each rests on.
   Adversary 2's specific supporting claims about the `ValueKind` tag do not
   survive `make_value` at `spirv_ir_builder.h:291-298`; its conclusion may still
   stand under its own definition.
2. **Live producers.** Not open. Adversary 2 is right, I concede, and it can be
   closed by reading the six files in section 24.1. **This should reach the
   project owner: the capability is un-defaulted, not disabled.**
3. **What `snode_struct_compiler.cpp:52-54` means.** I agree with adversary 2
   that it is neither surplus nor an asset and that it belongs on the planner's
   desk before item 6.2 is sized. Per plan §10.3 neither of us should call it
   either way.

## 24.10 Additions to my error lists

**Against report B:** B17 — §2.5 lists `spirv_types.cpp:433-439`
(`visit_physical_pointer_type`) among live consumers. It is unreachable: the only
real `PhysicalPointerType` construction site is the commented-out
`snode_struct_compiler.cpp:53`. Credit adversary 2.

**Against both reports and against me:** the "zero live producers" claim
(B §2.5), "the producers are still off" (A §1.7), and my own section 11 item 5.
All three are wrong for the same reason. Credit adversary 2.

**Against myself, corrected above:** section 7's tag census (three tags short,
now 24 / 11 / 3 / 10); section 4's justifying sentence (withdrawn); section 2.4's
"Taichi has no source line on this" (it does, `snode_struct_compiler.cpp:53`);
section 4.1's failure-mode list (a third, silent mode exists via
`u64_type()`); section 11 item 5 (withdrawn); section 8's weighting of
`715a04c98` (right on the changelog, too quick on the commit); sections 13 and 23
(superseded — adversary-05-2.md exists).

## 24.11 Escalation 13

**The word "architectural" needs a definition from the arbiter before either
adversary's rulings can be applied.** Adversary 2 and I agree on every source
fact about the `at_buffer` collision and disagree on whether it counts, because
we are using different tests: mine is "no external progress fixes it, only a
Taichi decision," adversary 2's is "no decision fixes it either." The plan's
§2.2 language ("hardware changes speed and granularity, never capability") and
the brief's ("architectural obstacles do not expire and environmental ones do")
support mine, but neither text settles it explicitly. **Both of us have graded
several findings against our own reading of this word, and the gradings are not
comparable until it is fixed.**
