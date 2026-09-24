# Adversary 01-1 — frontend IR and type system

Judging `report-01-ir-types.md` (pass A) against `report-01b-ir-types.md`
(pass B), and both against the source at `/opt/project/taichi/`.

Every file:line below was opened. Where I correct a report, I give the line I
read.

---

## 0. Verdicts

**CORRECT: qualified yes, with one substantive analytical error in pass A and
one overstated guard claim in pass B.**

Pass A contains a wrong causal chain (section 3 item 1 below) that inverts the
failure mode of a 64-bit range-for bound, and sixteen line-number errors. Pass B
is materially more accurate on line numbers and on the assert/readback surface,
and states one guard as fact that the source does not support.

**COMPLETE: no, for both.** Both reports drop findings that are present in
their own contemporaneous notes, and both miss a device-capability enum that
sits inside their own declared territory and answers the plan's 6.2 stub
question directly.

**MAJOR REVISION REQUIRED: yes, but narrowly scoped.** The inventories are
broadly sound and I could not break the central structural finding. What must
be revised is (a) pass A's range-for truncation claim, (b) both reports'
headline claim that the i32 enforcement is a single point, and (c) the
omission of `taichi/inc/rhi_constants.inc.h` from both. Everything else is
correction, not rework.

---

## 1. The central structural finding survives

Both reports converge on this and I could not break it:

- `i64`/`u64` exist and are fully supported as scalar values.
  `taichi/inc/data_type.inc.h:7,12`; `taichi/ir/type.h:151-155` (macro
  expansion of `PER_TYPE`); `taichi/ir/type_utils.cpp:59,64` (sizing);
  `taichi/ir/type_utils.h:107,112,123` (classification);
  `taichi/ir/type_factory.cpp:168-169` (`get_primitive_int_type(64, ...)`).
  Verified.
- There is no index type and no address type. `PointerType`
  (`taichi/ir/type.h:177-209`) carries `Type *pointee_` (`:206`),
  `int addr_space_{0}` with `// TODO: make this an enum` (`:207`), and
  `bool is_bit_pointer_` (`:208`). No width. Verified.
- The frontend does not forbid a wide index. `IndexExpression::type_check`
  (`taichi/ir/frontend_ir.cpp:936-947`) checks only `is_integral(expr_type)`
  at `:941`. Verified.
- Byte offsets are already 64-bit-clean; index/shape/stride values are not.
  `taichi/ir/statements.h:1260` (`IntegerOffsetStmt::offset`, `int64`),
  `:1411-1412`, `:1447-1448`, `:1597`, `:1618`, `:1770` (`std::size_t`);
  `taichi/ir/snode.h:97` (`num_cells_per_container`, `int64`), `:99-100`
  (`size_t`), `:306-308` (`max_num_elements()` returns `int64`);
  `taichi/ir/type.h:243,257,305,336` (element offsets `size_t`). Verified.
  Pass B's phrasing of this as "the pattern" (its section 3.8) is the single
  most useful sentence in either report.

---

## 2. Every place the two reports disagree, resolved against the source

### 2.1 `type_check.cpp` line numbers for `SNodeOpStmt` and `ExternalTensorShapeAlongAxisStmt`

A's section 3.2 gives `:119` for `SNodeOpStmt` (default branch) and `:124` for
`ExternalTensorShapeAlongAxisStmt`. B's section 3.1 gives `:114` and `:119`.

**B is right.** `taichi/transforms/type_check.cpp:114` is
`stmt->ret_type = PrimitiveType::i32;` inside the `else` of
`visit(SNodeOpStmt *)` (`:105-116`). `:119` is the body of
`visit(ExternalTensorShapeAlongAxisStmt *)` (`:118-120`). A is uniformly +5 on
both.

### 2.2 `mark_as_if_const`

A: `type_check.cpp:39-43`. B: `:40-44`. **B is right**; the function occupies
`:40-44`, with `:39` blank.

### 2.3 `MatrixPtrStmt` offset-semantics TODO

A: TODO at `statements.h:512-521`, `offset_used_as_index()` at `:522-530`.
B: `:512-520` and `:521-529`. **B is right.** The comment block ends at `:520`;
`bool offset_used_as_index() const {` begins at `:521` and closes at `:529`.

### 2.4 `IndexExpression::ret_shape`

A: `frontend_ir.h:675`. B: `:676`. **B is right.** `:675` is
`std::vector<ExprGroup> indices_group;`, `:676` is `std::vector<int> ret_shape;`.

### 2.5 The `simplify.cpp` `sizeof(int32)` guard

B's section 3.5 states as fact: "The two `simplify.cpp` sites are guarded by
`taichi/transforms/simplify.cpp:233-235`, which asserts every child SNode is
i32 or f32." A's section 5.2 item 6 says the `:238-239` site is guarded and the
`:260-261` site is not, and marks it INFERRED.

**A's framing is the defensible one; B overstates.** The assertion at
`simplify.cpp:233-236` lives inside `visit(SNodeLookupStmt *)` (`:222-249`).
The second hardcoded site is at `:260-261`, inside `visit(GetChStmt *)`
(`:251-270`), which contains no such assertion. There is a plausible
transitive argument — the `IntegerOffsetStmt` that gates the `GetChStmt` fold
at `:255` is normally produced by the `SNodeLookupStmt` fold at `:238-244` on
the same SNode — but that is an argument, not a guard, and chained `GetChStmt`
folds are not obviously covered by it. B contradicts itself here: its own
section 5.2 marks the same claim INFERRED with "I did not verify that no other
code path reaches those lines with a different child type." The fact statement
in 3.5 must be demoted to match the hedge in 5.2.

### 2.6 What `value_diff.cpp` restricts to i32

A attributes the i32-only `ConstStmt` handling to
`FindDirectValueBaseAndOffset::visit(ConstStmt*)` alone. B cites both
`value_diff.cpp:81-87` and `:142-146`.

**B is right and A is incomplete.** There are two independent i32-only
`ConstStmt` visitors: `:81-87` in `ValueDiffLoopIndex` and `:142-146` in
`FindDirectValueBaseAndOffset`. Both gate on
`stmt->val.dt->is_primitive(PrimitiveTypeID::i32)` and both read
`stmt->val.val_i32`. They feed different consumers, so naming only one
understates the optimisation surface.

Neither report enumerates the consumers correctly. The complete set is
`taichi/analysis/alias_analysis.cpp:59`, `:142`, `:178`;
`taichi/transforms/lower_access.cpp:237`; `taichi/analysis/bls_analyzer.cpp:50`;
`taichi/transforms/bit_loop_vectorize.cpp:54`. A gives three of six, B gives two
of six.

### 2.7 `make_mesh_block_local.cpp` byte-offset casts

A lists `:80`, `:200`, `:575`. B lists `:80,118,200,265,301,575`.

**B is right.** `:118`, `:265` and `:301` use the function-style spelling
`TypedConstant(int32(offset_in_bytes))` rather than `(int32)`, which is why a
naive grep for `(int32)` misses them. I confirmed all six by reading.

### 2.8 `kMaxNumSnodeTreesLlvm` use count

B's section 5.1 says "one use site"; B's Escalation 1 says
`runtime.cpp:562-563`, which is two. **Two.** `taichi/inc/constants.h:13`
declares it; `taichi/runtime/llvm/runtime_module/runtime.cpp:562` and `:563`
size `roots` and `root_mem_sizes`. B's escalation text is right, its
verification list is wrong. A does not raise the constant at all, which I judge
a worse omission than B's internal inconsistency: `kMaxNumSnodeTreesLlvm = 512`
is a second, lower, undocumented ceiling and plan 6.1 does not name it.

### 2.9 `taichi_max_num_snodes` count

A: "three sites outside my territory". B: "the only three sites are the
declaration, the assertion and the three runtime arrays". **Both are loose.**
Exhaustive grep across the tree excluding `build/` gives one declaration
(`taichi/inc/constants.h:12`) and four use sites
(`taichi/codegen/llvm/struct_llvm.cpp:266`,
`taichi/runtime/llvm/runtime_module/runtime.cpp:567`, `:568`, `:569`). The
substantive claim — zero uses in territory 01 — is correct in both.

---

## 3. Errors in pass A

### 3.1 The range-for truncation claim is wrong (substantive)

A's section 5.2 item 1 asserts, as its lead inference: "A 64-bit constant
range-for bound is silently truncated to its low 32 bits." A's notes
(`notes-01-ir-types.md:370-389`) make the chain explicit: "The assert passes
only *because* `mark_as_if_const` already retagged the constant to i32."

**That is false. The assert fires.**

- `TypeCheck::mark_as_if_const` (`taichi/transforms/type_check.cpp:40-44`)
  assigns `stmt->ret_type = t`. It does not touch `stmt->val`.
- `ConstStmt` holds `TypedConstant val` (`taichi/ir/statements.h:988`) and sets
  `ret_type = val.dt` only in its constructor (`:993`). Nothing re-synchronises
  them afterwards; `type_check.cpp` has no `visit(ConstStmt *)`.
- `TypedConstant::val_int32()` (`taichi/ir/type.cpp:462-465`) asserts
  `get_data_type<int32>() == dt`, where `dt` is `TypedConstant::dt`
  (`taichi/ir/type.h:551`), **not** `Stmt::ret_type`.
- `TI_ASSERT` is unconditional. `taichi/common/logging.h:100-107` expands it to
  an `if (!x) TI_ERROR(...)` with no `NDEBUG` guard. It is not compiled out in
  a release build.

So `taichi/transforms/offload.cpp:109` and `:117` raise a hard error on an i64
range-for bound. **B has this right**: its headline and its section 3.6 both
say `val_int32()` "fails loudly, not silently", and its section 5.1 states
correctly that `mark_as_if_const` "rewrites `ret_type` without touching
`val.dt`, so those two can disagree".

A's report also contradicts itself: section 5.1 lists "`val_int32()` asserts the
type is exactly i32 — `taichi/ir/type.cpp:462-465`" among its VERIFIED facts,
immediately before section 5.2 infers a silent truncation that requires the
assert not to fire.

This is not cosmetic. A silent truncation and a hard compile error demand
opposite treatment under plan 6.2, and A's is the first inference the planner
would read.

### 3.2 The `val_int32()` readback surface is missing entirely

A names only `offload.cpp:109,117`. B's section 3.6 enumerates the full set,
and it matches my grep exactly: `taichi/transforms/offload.cpp:109,117`;
`taichi/transforms/scalarize.cpp:162,1097,1120,1147,1231,1311`;
`taichi/transforms/auto_diff.cpp:576,776,1770`;
`taichi/ir/control_flow_graph.cpp:462`;
`taichi/transforms/constant_fold.cpp:115`. Twelve sites that abort on a
non-i32 constant, eleven of which A does not mention. Given that A's own
headline is about where the width is enforced, this is a real gap.

### 3.3 Remaining line-number errors in A

Each verified by reading:

| A says | Actual |
|---|---|
| `type.h:546-561` for the `TypedConstant` union (section 5.2) | `:556-569`; A's own section 3.1 says `:556-570`, so A contradicts itself |
| `type.h:177-208` for `PointerType` | class spans `:177-209` |
| `type.h:303`, `:335` for `size_t` element offsets | `:305`, `:336` |
| `frontend_ir.h:713` for `RangeAssumptionExpression::low/high` | `:712` |
| `type_utils.cpp:112-155` for `data_type_format` | begins `:113` |
| `type_utils.cpp:131` for the Vulkan comment, `:132` `%lld`, `:136` `%lu`/`%llu` | `:133`, `:134`, `:137` |
| `check_out_of_bound.cpp:199` for a `"%d"` | `:201` |
| `scratch_pad.h:97-100`, `:130-137`, `:139-146`, `:148-156` | `:96-100`, `:131-138`, `:140-147`, `:149-157` |
| `ir_builder.cpp:159-162` for `get_uint64` | `:159-163` |

A's report opens by claiming "All line numbers were read directly and then
re-verified in a second pass (N019)". Sixteen errors is not consistent with
that claim, and the claim itself should be struck.

### 3.4 A's Escalation 7 cites an inventory that does not exist

A's Escalation 7 says the mesh passes are "inventoried in section 3.2 and note
N011". Section 3.2 contains no entry for
`taichi/transforms/demote_mesh_statements.cpp`,
`make_mesh_thread_local.cpp` or the `make_mesh_block_local.cpp` cast sites. The
inventory is in A's notes (`notes-01-ir-types.md:528-539`) and was dropped from
the report.

---

## 4. Errors in pass B

### 4.1 `data_type_size` does not refuse pointers

B's headline 5 and section 4 say "`data_type_size` explicitly refuses pointers
(`taichi/ir/type_utils.cpp:30-35`)". It does not refuse. `:35` is
`t.set_is_pointer(false);` — it clears the pointer flag and then sizes the
pointee. The TODO immediately above (`:31-34`) records that a loud failure on
pointers was intended and never implemented. B's conclusion ("the type system
cannot report a pointer's size") is correct; its description of the mechanism
is not, and the difference matters: today a caller asking for the size of a
pointer type silently gets the pointee's size back.

### 4.2 The simplify guard, above (2.5).

### 4.3 The `kMaxNumSnodeTreesLlvm` count, above (2.8).

### 4.4 B's out-of-territory list for `taichi_max_num_indices` is partial

B's section 3.7 correctly flags
`taichi/runtime/llvm/runtime_module/runtime.cpp:288-290`,
`taichi/codegen/llvm/codegen_llvm.cpp:2356-2376` and
`taichi/program/launch_context_builder.cpp:230,247,269,302,322`. It omits
`taichi/codegen/llvm/struct_llvm.cpp:171`,
`taichi/runtime/llvm/runtime_module/runtime.cpp:1012`, and
`taichi/python/export_lang.cpp:559,1222` (the last exposes
`get_max_num_indices` to the Python layer, which plan 1.2 puts out of scope but
which is still a consumer of the constant). Having chosen to cross the
territory line, B should have crossed it completely.

Set against this, B's in-territory list for the same constant is exactly
complete, and so is A's. I checked both against
`grep -rn taichi_max_num_indices taichi/`.

---

## 5. What BOTH reports missed

This is the section that matters. Divergence between the two is small; their
shared blind spots are not.

### 5.1 `taichi/inc/rhi_constants.inc.h:14` — the SPIR-V Int64 capability already exists, in territory 01

Plan 6.2 says: "64 bit integers in SPIR-V are a declared capability, not a
given. The architecture must be able to express a target that lacks it."

Both reports answer that this cannot be expressed. B's section 4: "A target
that lacks Int64 has nowhere in the current type system to say so." B's section
6.3: "There is no mechanism in my territory for a pass to ask what a target's
address width or 64-bit integer capability is." A's Escalation 1 makes the same
assumption.

**Both are wrong about their own territory.**
`taichi/inc/rhi_constants.inc.h:14` declares
`PER_DEVICE_CAPABILITY(spirv_has_int64)`. `taichi/inc/` is territory 01 by both
agents' own stated scope. The enum is materialised at
`taichi/rhi/device_capability.h:12-14`, populated by the backends at
`taichi/rhi/vulkan/vulkan_device_creator.cpp:632`,
`taichi/rhi/opengl/opengl_device.cpp:511` and
`taichi/rhi/metal/metal_device.mm:132,1052`, and consumed at
`taichi/codegen/spirv/spirv_ir_builder.cpp:64` (emits `OpCapability Int64`),
`:166` (declares `t_int64_`/`t_uint64_` only if present), and `:312`, `:326`
(`TI_ERROR("Type {} not supported.")` if an i64 or u64 type is requested
without it).

The accurate statement is not "the capability cannot be expressed". It is:
**the capability is already declared and already enforced at SPIR-V codegen;
what does not exist is any link from the type layer to it.** Under the current
code, widening an index to i64 does not silently degrade on a Vulkan target
without Int64 — it produces a hard `TI_ERROR` at
`spirv_ir_builder.cpp:311-313`. That is a materially different starting point
for plan 6.2's stub-architecture requirement, and neither report gives it to
the planner.

Both agents did find the adjacent, weaker signal (the Vulkan printf comment at
`taichi/ir/type_utils.cpp:133,136-137`) and stopped there.

### 5.2 `make_cpu_multithreaded_range_for.cpp` — a second, independent i32 pin on range-for bounds

Neither report names this file. Both notes do.

`taichi/transforms/make_cpu_multithreaded_range_for.cpp:61-95` rebuilds the
entire CPU range-for iteration space in i32: `:68` (constant 1), `:70` (512),
`:72` (`cpu_max_num_threads`), `:81` (`offloaded->begin_value`), `:90`
(`end_value`). Critically, `:83-84` and `:92-93` read the **dynamic**, non-constant
bounds out of the global temporary buffer as
`GlobalTemporaryStmt(offset, PrimitiveType::i32)`. The buffer *offset* is
`std::size_t` (`taichi/ir/statements.h:1411-1412`), which both reports cite as
64-bit-clean; the *value read from it* is pinned i32 here. The pass runs at
`taichi/transforms/compile_to_offloads.cpp:199`.

A's own notes (`notes-01-ir-types.md:396-403`) call this "a second,
independent 32-bit narrowing on the dynamic path" and then A's report asserts
"Nothing else in the compiler imposes i32 on an index." B's notes
(`notes-01b-ir-types.md:480-485`) say "The parallel range-for iteration space
is i32 end to end" and B's report drops it. Both agents found the counterexample
to their own headline and neither carried it forward.

### 5.3 The mesh path inserts value-truncating i32 casts outside `type_check.cpp`

`grep -rn "UnaryOpType::cast_value" taichi/ir/ taichi/transforms/ taichi/analysis/`
returns exactly two families that pin to i32:

- `taichi/transforms/type_check.cpp:234-244` / `:246-256` (the helpers behind
  `:154` and `:461`), and
- the mesh path: `taichi/transforms/demote_mesh_statements.cpp:15-16` (mesh
  index cast to i32 *before* a `GlobalPtrStmt` is formed at `:18`), `:52-54`,
  `:130-131`; `taichi/transforms/make_mesh_thread_local.cpp:104-106`,
  `:112-114`; `taichi/transforms/make_mesh_block_local.cpp:210-212`,
  `:374-376`, `:397-399`.

Six mesh cast sites, none in either report. A's notes have them
(`notes-01-ir-types.md:528-539`); B's notes have them
(`notes-01b-ir-types.md:476-479`). Both dropped.

### 5.4 `lower_matrix_ptr.cpp` is absent from both reports

`taichi/transforms/lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477` build
matrix element indices as `ConstStmt(TypedConstant(i))`, which is i32 by the
`TypedConstant(int32)` constructor at `taichi/ir/type.h:582`. More
significantly, `:536-541` computes a **dynamic-index byte offset** as
`stmt->offset * TypedConstant(origin->dynamic_index_stride)` and then forces
`offset->ret_type = stmt->offset->ret_type` (`:540`), which
`type_check.cpp:160-161` has already hard-asserted to be i32. The byte offset
for a dynamically indexed matrix field is therefore computed in 32 bits even
though every other byte offset in the IR is `size_t`. This is exactly the class
of inconsistency plan 6.2 warns about, and it is in neither report. A's notes
have it at `notes-01-ir-types.md:595-603`; B has it nowhere.

### 5.5 The `GlobalPtrStmt` index cast has an exception neither report states

A's report: "Every SNode index (`:148-156`) ... is cast to `PrimitiveType::i32`."
B's headline 2: "inserts a value-truncating cast to i32 on every `GlobalPtrStmt`
index."

`taichi/transforms/type_check.cpp:123-125` returns from `visit(GlobalPtrStmt *)`
immediately when `stmt->is_bit_vectorized`, before the loop at `:147-156`. A
bit-vectorized global pointer's indices are never cast. A's notes reference
`type_check.cpp:123-125` (`notes-01-ir-types.md:526`); neither report states the
exception.

### 5.6 `promoted_type` cannot take a pointer operand

A's section 4 leans on automatic promotion: "Type promotion is by bit width, so
`i32 op i64` promotes to `i64` — `taichi/ir/type_factory.cpp:222-256`,
`:265-286`. Mixed-width index arithmetic widens on its own." I verified the
promotion rule at `:222-246` and `:265-270` — A is right about it, and B does
not discuss promotion at all.

But `to_primitive_type` (`taichi/ir/type_factory.cpp:248-262`) opens with
`if (d->is<PointerType>()) { TI_ERROR("promoted_type got a pointer input."); }`
at `:249-251`. Promotion is unavailable for anything carrying a pointer type.
Neither report records this, and it bounds the "widens on its own" claim.

### 5.7 The dynamic tensor-index path is i32, not only the folded one

Both reports cite `taichi/ir/frontend_ir.cpp:742-747` (A) / `:744-745` (B) — the
constant-folded branch of `make_tensor_access_single_element`. The
**dynamic** branch at `:731-740` is equally i32: `:732` seeds the offset with
`ConstStmt(TypedConstant(0))` and `:735` emits each shape factor as
`ConstStmt(TypedConstant(shape[i]))`, both i32 by `type.h:582`. The whole
flattened tensor offset is built in i32 on both branches, not just the folded
one.

### 5.8 Minor items each report has and the other lacks

Not misses so much as asymmetric coverage, recorded so the planner can merge:

- A only: `taichi/ir/statements.h:1370` (`GetChStmt::chid`);
  `taichi/ir/analysis.h:185` (`DiffPtrResult::diff_range`);
  `taichi/analysis/bls_analyzer.h:15-18` (`IndexRange`); the `scratch_pad.h`
  arithmetic sites; the promotion rule; the `scalarize.cpp:1121-1122,1148-1149`
  contrast showing `data_type_size(...)` used properly.
- B only: `taichi/ir/frontend_ir.cpp:149-153` (every loop variable is born as
  pointer-to-i32 — verified at `:151-152`, and this is the origin point A's
  headline is looking for); `taichi/transforms/utils.cpp:5-22`
  (`generate_mod`/`generate_div` take `int y` and emit i32 constants — the
  single most-called index-arithmetic helper, used from
  `scalar_pointer_lowerer.cpp:73,75` and `demote_dense_struct_fors.cpp:60,63,74,76`);
  `taichi/ir/type_factory.cpp:86-95` (pointer interning key is
  `(element, is_bit_pointer)`, so a width field alone would not suffice);
  `taichi/ir/snode.cpp:22-23,83` (`num_elements_from_root` int products);
  `taichi/transforms/offload.cpp:236-237,244` (int64 element count narrowed into
  `int block_dim`); `taichi/ir/type.h:530-532,545` (`QuantArrayType`);
  `taichi/ir/snode.h:317,329-334` (`shape_along_axis`, and the host accessors
  taking `std::vector<int>` index tuples); `taichi/runtime/.../runtime.cpp:288-290`
  (`PhysicalCoordinates { i32 val[taichi_max_num_indices]; }`, the device-side
  mirror of the frontend's i32 index — A never encountered this at all);
  the observation that `get_total_num_elements_towards_root`
  (`taichi/ir/snode.h:310-315`) has no callers tree-wide, which I confirmed.

On raw coverage B is the stronger report. On the two places A is stronger — the
promotion rule and the `simplify.cpp` guard asymmetry — A is right and B is
wrong or silent.

---

## 6. Testing the "enforcement is concentrated" claim (assignment item 4)

Both passes claim it. A: "`taichi/transforms/type_check.cpp` is the single
enforcement point... Nothing else in the compiler imposes i32 on an index."
B: "The hard enforcement point is one loop in one pass."

**The claim is false as stated, and defensible only in a much narrower form.**

I searched three ways: every `PrimitiveType::i32` token in the four directories
(64 hits — A's count is right, though A's report classifies only about a
quarter of them); every `UnaryOpType::cast_value` construction site; and every
`val_int32()` call site.

What the source actually shows:

**Cast insertion (a real value truncation) — 8 sites in 4 files, not 2 in 1.**
`type_check.cpp:154` and `:461` (via the helper at `:234-244`), plus
`demote_mesh_statements.cpp:15-16`, `:52-54`, `:130-131`;
`make_mesh_thread_local.cpp:104-106`, `:112-114`;
`make_mesh_block_local.cpp:210-212`, `:374-376`, `:397-399`. And the
`type_check.cpp:154` site is skipped entirely for bit-vectorized pointers
(`:123-125`).

**Hard aborts on a non-i32 index — 15 sites.** `type_check.cpp:160-161`
(`MatrixPtrStmt`); `snode.h:28-30` (axis count);
`demote_dense_struct_fors.cpp:29` (2^31 cells); and the twelve `val_int32()`
call sites listed in 3.2 above, all of which route through the unconditional
`TI_ASSERT` at `taichi/ir/type.cpp:463`.

**`ret_type` pinned to i32 with no cast — 18 sites.**
`type_check.cpp:114,119,467,471,475,521,525,567`;
`statements.h:1684,2064,2083`; `statements.cpp:146`;
`frontend_ir.cpp:152,1114,1306,1358,1362,1392`.

**i32 constants forming index, stride or offset arithmetic — at least 9 files.**
`utils.cpp:7,10,17,20`; `simplify.cpp:175,178`;
`demote_dense_struct_fors.cpp:49,79`;
`make_cpu_multithreaded_range_for.cpp:68,70,72,81,84,90,93`;
`lower_matrix_ptr.cpp:189,232,424,477,537`;
`scalarize.cpp:63,114,471,473,530`; `frontend_ir.cpp:732,735,747`;
`make_block_local.cpp:152,157,285,298,328,334`;
`make_mesh_block_local.cpp:80,118,200,265,301,575`.

**Host-side `int` arithmetic that overflows before any IR type exists.** Both
reports cover this adequately and A's framing of it (section 4, closing
paragraph) is the correct one: this class cannot be fixed by changing a type.
Verified sites: `snode.cpp:22-23,83,92`;
`scalar_pointer_lowerer.cpp:33-39,56,63-65,78`;
`demote_dense_struct_fors.cpp:19-24,74`; `type.h:222-227,307-321,530-532`;
`frontend_ir.cpp:742-747`; `check_out_of_bound.cpp:54-57,169-173`;
`handle_external_ptr_boundary.cpp:62-67`; `scratch_pad.h:96-100,131-157`.

**One further point neither report carried into its report.** A's notes
(`notes-01-ir-types.md:769-776`) record that `irpass::type_check` is re-run
after almost every transform — `compile_to_offloads.cpp:57,66,193,200,208,310,391`,
plus `simplify.cpp:219` after each `LinearizeStmt` expansion — and conclude
"the i32 coercion is re-applied at many points; it is not a one-shot." That is
correct and I verified the call sites. It bears directly on any attempt to
widen: it is not sufficient to change the pass once and let a wider type flow,
because the pass runs again after every intervening transform.

**The defensible form of the claim** is: *for non-mesh SNode and external-array
index operands, `type_check.cpp:154` and `:461` are the only two places a
narrowing cast is inserted.* That is true, it is useful, and it is what both
reports should say. What they do say is broader than the source supports.

---

## 7. Assignment item 2 — completeness judgement

Item 6.1 is correctly assessed by both: zero touch points in territory 01,
verified independently by me. Both correctly identify that what the territory
owns is SNode identity (`snode.h:88-89`, `snode.cpp:12,220`), that ids are
plain `int` assigned from a process-global atomic and never recycled, and that
no consumer bit-packs them. I checked each consumer they name —
`ir.cpp:81-85`, `same_statements.cpp:140-141`, `alias_analysis.cpp:154-161`,
`offline_cache_util.cpp:101-120`, `gen_offline_cache_key.cpp:540` — and found
no packing. Both are right that raising 1024 does not stress this.

Item 6.2 is where both are incomplete, per section 5.

Item 6.3 is where both are wrong, per 5.1. A does not address 6.3 at all; B
addresses it and reaches an incorrect conclusion.

The offline cache observation is sound in both and independently verified:
`offline_cache_util.cpp:110-116` serialises all three `AxisExtractor` int fields
across all twelve axes, plus `physical_index_position` (`:119`) and
`num_cells_per_container` (`:123`). Changing either the constant or the field
widths changes the key.

---

## 8. Escalations

Unresolved. Recorded, not decided.

1. **The mesh path's scope.** Six value-truncating i32 casts and four pinned
   `ret_type`s live in the mesh passes. Nothing in plan section 6 says whether
   mesh is in scope. Both agents raised this; I am not resolving it. If mesh is
   out of scope, both reports' "single enforcement point" claim becomes nearly
   true and should be restated with that exclusion made explicit. If mesh is in
   scope, section 5.3 above is a required addition to both.

2. **`spirv_has_int64` and the type layer.** The capability exists
   (`taichi/inc/rhi_constants.inc.h:14`) and is enforced
   (`taichi/codegen/spirv/spirv_ir_builder.cpp:312,326`). Nothing connects it to
   the type layer, and connecting it is added structure, which standing
   instruction 4 forbids me from proposing. Whether an index width should be
   target-dependent is a planner decision, and it is now a better-informed one
   than either report allows.

3. **The `simplify.cpp:260-261` `sizeof(int32)` site.** Whether the
   `SNodeLookupStmt` assertion at `:233-236` transitively covers the
   `GetChStmt` fold at `:255-268` cannot be settled from the two visitors. Both
   agents stopped here; so do I. `taichi/ir/snode.h:99`
   (`cell_size_bytes`) is the derived value that exists.

4. **`MatrixPtrStmt::offset` dual semantics.** Both agents escalated this
   (`statements.h:512-520`, `:521-529`). I add one fact neither supplied: the
   byte-meaning branch is realised at `lower_matrix_ptr.cpp:536-541`, and it
   computes a byte offset in i32 while every other byte offset in the IR is
   `size_t`. That sharpens the question but does not answer it.

5. **`taichi_max_num_indices` as footprint.** Both agents raised it. I confirm
   it is not host-only: `runtime.cpp:288-290` sizes a device-side struct with
   i32 slots, mirrored and asserted at `codegen_llvm.cpp:2356-2376`, and it
   also drives `struct_llvm.cpp:171`, `runtime.cpp:1012` and
   `export_lang.cpp:559`. Coordination across territories is a planner call.

6. **`kMaxNumSnodeTreesLlvm = 512`.** B raised it; A did not. Plan 6.1 names
   only `taichi_max_num_snodes`. Whether the SNode-tree ceiling is in scope is
   not mine.

---

## 9. Required revisions, in priority order

1. Pass A: strike section 5.2 item 1 and replace it with B's finding. The
   failure is a hard `TI_ERROR` at `offload.cpp:109/117`, because
   `mark_as_if_const` (`type_check.cpp:40-44`) changes `ret_type` while
   `val_int32()` (`type.cpp:462-465`) checks `val.dt`, and `TI_ASSERT`
   (`logging.h:100-107`) is not compiled out.
2. Both: restate the headline. `type_check.cpp:154` and `:461` are the only
   cast sites *for non-mesh SNode and external-array indices*; they are not the
   only enforcement in the compiler.
3. Both: add `taichi/inc/rhi_constants.inc.h:14` and the SPIR-V Int64
   enforcement path, and withdraw the claim that the capability cannot be
   expressed.
4. Both: promote from notes to report —
   `make_cpu_multithreaded_range_for.cpp:68-93`, the mesh cast sites, and
   (A) `lower_matrix_ptr.cpp:536-541`.
5. Pass A: correct the sixteen line references in section 3 above, and strike
   the "re-verified in a second pass" claim.
6. Pass A: add B's `val_int32()` call-site inventory (section 3.6 of B).
7. Pass B: demote the section 3.5 simplify-guard statement to match its own
   section 5.2 hedge, and correct the `data_type_size` description.
8. Both: note the `is_bit_vectorized` exception at `type_check.cpp:123-125`.

Neither report needs to be rewritten. Both need these amendments before the
planner uses them to size item 6.2.

---

## 10. Divergence from adversary 01-2

`adversary-01-2.md` exists and I have read it. We reach the same three verdicts
independently: correct with defects, not complete, major revision for pass A and
minor for pass B. We agree on every one of the head-to-head adjudications we both
attempted, including the two that matter most — that pass A's silent-truncation
inference is false because `mark_as_if_const` (`type_check.cpp:40-44`) writes
`ret_type` while `val_int32()` (`type.cpp:462-465`) consults `TypedConstant::dt`
(`type.h:551`), and that the "single enforcement point" headline does not
survive contact with the source. We also independently identified the same two
dropped-from-notes omissions: `make_cpu_multithreaded_range_for.cpp:68-93` and
the mesh cast sites.

Where we differ, with the source consulted for each.

### 10.1 Adversary 2 is right and I was imprecise

**`total_shape` is per-axis, not a cell count.** Adversary 2's section 3
corrects pass A's inference item 3 on a point I did not catch and that also
weakens my own section 6. `taichi/transforms/scalar_pointer_lowerer.cpp:33`
declares `std::array<int, taichi_max_num_indices> total_shape` and `:36-37`
accumulates `total_shape[j] *= s->extractors[j].shape` **per axis**. A structure
of 2^40 cells spread over four axes of 2^10 leaves every element at 1024.
Overflow requires a single axis extent above 2^31, not a cell count above it. I
listed `scalar_pointer_lowerer.cpp:33-39` and
`demote_dense_struct_fors.cpp:19-24` in my section 6 under host-side arithmetic
that overflows without stating that qualification. Adversary 2's correction
stands and should be applied to both reports and to my section 6.

**Three sites I missed, all verified:**

- `taichi/transforms/lower_ast.cpp:270-273` — the external-array struct-for
  extent is built as a running i32 product of `ExternalTensorShapeAlongAxisStmt`
  values seeded by `ConstStmt(TypedConstant(1))` at `:271`, with no guard and no
  assert. Neither report names `lower_ast.cpp`; nor did I.
- `taichi/transforms/lower_ast.cpp:330` — a range-for containing a `break` is
  lowered to a while loop whose induction variable is
  `AllocaStmt(PrimitiveType::i32)`. A third i32 birth site for a loop variable.
- `taichi/analysis/arithmetic_interpretor.cpp:98-109` — a **second, independent
  implementation of `LinearizeStmt`**, accumulating in `int64_t` at `:99,106`
  and then storing under `stmt->ret_type` at `:108`, which `type_check.cpp:521`
  pinned to i32. This is the best single find in either adversarial file. Any
  width change has two `LinearizeStmt` implementations to keep in step.
  Its only consumers are `tests/cpp/transforms/scalar_pointer_lowerer_test.cpp:79-90`
  and `tests/cpp/transforms/make_block_local_test.cpp:147-184`, which I
  confirmed by grep — so adversary 2's section 5.4 point about the existing C++
  regression harness is also sound, and I did not look at tests at all.

**Two smaller corrections I did not make.** Pass A's listing of
`taichi/ir/expr.cpp:93` as a "default integer literal" is wrong: it is
`Expr::Expr(int32 x)`, one overload of a set that includes `Expr::Expr(int64 x)`
at `:96`. And pass B's "only three constants from `constants.h` are used in my
territory" (its section 5.1) does contradict its own section 3.7. I verified the
underlying fact — exactly three constants beyond `taichi_max_num_indices` are
used in territory 01: `taichi_max_gpu_block_dim` in `taichi/ir/frontend_ir.cpp`,
`taichi_global_tmp_buffer_size` in `taichi/transforms/offload.cpp`,
`default_shared_mem_size` in `taichi/transforms/make_mesh_block_local.cpp` — so
this is a scoping slip in B's wording, not a factual error about the code.
Adversary 2's framing slightly overstates it; the substance is right.

### 10.2 I am right and adversary 2 is wrong

**`data_type_size` does not refuse pointers.** Adversary 2's section 6 lists
under "Verified good, in both": "`type_utils.cpp:30-35` `data_type_size`
refusing pointers. B's section 4 is a precise and correct statement of the gap."
It is not. `taichi/ir/type_utils.cpp:35` is `t.set_is_pointer(false);` — the
function strips the pointer flag and returns the **pointee's** size. The TODO at
`:31-34` records that a loud failure on pointers was intended and never
implemented. Both B and adversary 2 endorse a description the source
contradicts. The downstream conclusion (the type system cannot report a
pointer's size) is unaffected; the mechanism is not what either says, and the
difference is that a caller today gets a wrong answer rather than an error.

**The SPIR-V Int64 capability already exists, inside territory 01.** Adversary 2
does not mention `taichi/inc/rhi_constants.inc.h` anywhere, and its section 7
escalation list carries B's escalation 2 forward unchallenged. My section 5.1
stands: `taichi/inc/rhi_constants.inc.h:14` declares
`PER_DEVICE_CAPABILITY(spirv_has_int64)`, materialised at
`taichi/rhi/device_capability.h:12-14`, set by the backends at
`taichi/rhi/vulkan/vulkan_device_creator.cpp:632`,
`taichi/rhi/opengl/opengl_device.cpp:511`,
`taichi/rhi/metal/metal_device.mm:132,1052`, and enforced at
`taichi/codegen/spirv/spirv_ir_builder.cpp:64,166,312,326` — where requesting an
i64 or u64 type without it produces `TI_ERROR("Type {} not supported.")`. Both
reports and adversary 2 conclude that a target lacking Int64 has nowhere to say
so. The declaration is in the explorers' own territory; what is missing is the
link from the type layer to it. This bears directly on plan 6.2's stub note and
on plan 6.3, and it is the largest thing all four of us but one missed.

**Three inventory sites adversary 2 does not have.** All verified in my section
5: `taichi/transforms/lower_matrix_ptr.cpp:536-541` (a dynamic matrix-field byte
offset computed in i32 while every other byte offset in the IR is `size_t`);
`taichi/ir/type_factory.cpp:249-251` (`promoted_type` hard-errors on a pointer
operand, which bounds pass A's "widens on its own" claim); and
`taichi/ir/frontend_ir.cpp:732,735` (the dynamic tensor-index branch is i32,
not only the folded branch at `:742-747` that both reports cite).

**The `is_bit_vectorized` exception.** `taichi/transforms/type_check.cpp:123-125`
returns from `visit(GlobalPtrStmt *)` before the cast loop at `:147-156`, so a
bit-vectorized global pointer's indices are never cast. Adversary 2 does not
raise this, and it is a direct counterexample to both reports' word "every".

**The `value_diff` consumer set.** Adversary 2's table row 9 maps the two
visitors to one consumer each. The complete set is six:
`taichi/analysis/alias_analysis.cpp:59,142,178`,
`taichi/transforms/lower_access.cpp:237`, `taichi/analysis/bls_analyzer.cpp:50`,
`taichi/transforms/bit_loop_vectorize.cpp:54`. Verified by
`grep -rn "value_diff_ptr_index\|value_diff_loop_index" taichi/`.

### 10.3 Where we differ in emphasis, not in fact

Adversary 2's section 1.4 says "**Exactly one** site aborts instead:
`type_check.cpp:160-161`". Read in its scope — aborts on a *pointer statement's
index* — that is correct. Read as a global count it is not: my section 6
enumerates fifteen abort sites, twelve of them the `val_int32()` calls adversary
2 itself credits B with finding. The sentence needs its scope made explicit
before the planner reads it.

On pass A's `simplify.cpp` guard (adversary 2's table row 7) we reach the same
resolution by the same reasoning: the assert at `simplify.cpp:233-236` is inside
`visit(SNodeLookupStmt *)` (`:222-249`) and `visit(GetChStmt *)` (`:251-270`)
has none, so A is literally right; B's transitive data-flow argument is
plausible and unproven, and B contradicts its own section 5.2 by stating it as
fact. Neither of us proved it either way. On row 6 (whether the assert spans
`:233-235` or `:233-237`) I decline to score: `:233` and `:234-235` are two
asserts inside a loop opening at `:232` and closing at `:236`, so both citations
are ranges over the same guard.

### 10.4 Net

Adversary 2's analysis is stronger than mine on the frontend birth sites, on the
`total_shape` mechanism, and on the second `LinearizeStmt` implementation. Mine
is stronger on the device-capability question, on the narrowing sites in
`lower_matrix_ptr.cpp` and the dynamic tensor path, and on the completeness of
the consumer and abort-site sets. Neither of us contradicts the other on any
verdict. The union of the two "what both missed" sections, not either alone, is
what should go back with the reports.
