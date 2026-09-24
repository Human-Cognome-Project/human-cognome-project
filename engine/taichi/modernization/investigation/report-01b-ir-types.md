# Report — explore agent 01B — frontend IR and type system

Territory: `taichi/ir/`, `taichi/inc/`, `taichi/analysis/`, `taichi/transforms/`.
Focus: where index and address types originate and propagate.

Working notes: `modernization/investigation/notes-01b-ir-types.md`
(52 entries; 1-26 are the first pass, 27-38 the revision pass, 39-42 the first
amendment pass, 43-46 the completeness pass, 47-52 the closing pass).

**Revision status.** This report was returned for amendment after adversarial
review. Every substantive change is recorded in notes entries 27-38, including
the four points where the two adversaries disagreed with each other and I went
to the source to adjudicate. Section 7 lists what changed and why.

**Amendment status.** After round two both adversaries judged this report one
tightly scoped pass short of consensus, on three named points. Those three, and
nothing else, were re-opened against the source. Notes entries 39-42 record the
adjudication of each; section 8 lists what changed. Two of the three claims were
correct and are now fixed; the third was correct in mechanism and wrong in its
count, and is fixed to the verified figures rather than the claimed ones.

**Completeness pass.** A second amendment then applied one rule: where a site is
inside this territory and can be confirmed present, it goes in the inventory,
whether or not it was among the claims under adjudication. Five groups were
re-opened on that rule. Notes entries 43-46; section 8.1 lists what changed. One
site was found that no adversary had listed, one line span was wrong and is
corrected, and three groups were already present and are now characterised
correctly rather than flatly.

**Closing pass.** Round three found no citation fault in this report and upheld
it on every contested item, but recorded a completeness gap that neither round
had run to the end: an exhaustive sweep of `PrimitiveType::i32` across the three
territory directories returns sixty-four lines, and this report inventoried
fifty-four of them. All sixty-four are now enumerated and accounted for in
section 3.12; five lines are added to the inventory and five are recorded as
swept and excluded with the reason stated, three of them on a planner ruling
that puts texture operations out of scope. One tally that did not reconcile
against its own list is corrected. Two escalations were raised in this pass and
both were ruled inside it: the texture-op checks are out of scope, and whether
item 6.2 reaches the dynamic-SNode append index is recorded as a condition on
the undecided sizing rule rather than answered. Notes entries 47-52; section 8.2
lists what changed.

Every claim below carries a file:line reference. Section 5 separates what I
VERIFIED by reading code from what I INFERRED.

---

## 1. Headline findings

1. **Upstream states in-tree that 64-bit indexing does not exist.**
   `taichi/ir/snode.cpp:95-100` emits `"SNode index might be out of int32
   boundary but int64 indexing is not supported yet. Struct fors might not work
   either."` It is a **warning**, and the code proceeds with a value truncated
   by a plain `static_cast<int>` at `:92`. That truncation is silent in every
   build configuration.

2. **The i32 enforcement is concentrated only for pointer-statement indices,
   and even there it has an exception.** Two sites insert a value-truncating
   cast on the indices of a pointer statement:
   `taichi/transforms/type_check.cpp:154` (SNode, with a warning) and `:461`
   (external array, silently). But `taichi/transforms/type_check.cpp:122-125`
   returns from `visit(GlobalPtrStmt *)` before that loop whenever
   `stmt->is_bit_vectorized`, so a bit-vectorized global pointer's indices are
   never cast. And beyond those two sites the width is fixed in four other
   ways: eight more `cast_value` truncations in the mesh passes (section 3.3);
   four **birth sites** where an index is created i32 and never reaches a cast
   (`taichi/ir/frontend_ir.cpp:151-152`, `taichi/ir/frontend_ir.cpp:1160`,
   `taichi/transforms/lower_ast.cpp:330`,
   `taichi/transforms/type_check.cpp:467`); one **re-synthesis** site that
   rebuilds the whole CPU iteration space in i32 after type-checking has already
   run (`taichi/transforms/make_cpu_multithreaded_range_for.cpp:68-93`); and
   fifteen hard aborts (section 3.6). The coercion is also not a one-shot:
   `irpass::type_check` runs seven times in
   `taichi/transforms/compile_to_offloads.cpp` (`:57,66,193,200,208,310,391`),
   and **twenty-two further call lines across fourteen other files** re-run it
   from inside individual passes. Fourteen of those twenty-two are unqualified
   `type_check(root, config)` calls made from inside `namespace irpass` itself,
   so a grep for `irpass::type_check` does not see them. Full inventory and
   mechanism in section 3.11.

3. **The frontend does not restrict the type of an index expression, but it does
   fix the type of a loop variable.** `IndexExpression::type_check`
   (`taichi/ir/frontend_ir.cpp:936-947`) requires only `is_integral`, and
   `taichi/transforms/frontend_type_check.cpp:71-78` only warns. So a
   user-supplied i64 index survives the frontend. But
   `FrontendForStmt::add_loop_var` (`taichi/ir/frontend_ir.cpp:149-153`) types
   **every** loop variable as pointer-to-i32 unconditionally. In a struct-for
   driven workload essentially every index is loop-derived, so in practice the
   frontend does pin the width for the case that matters. Both halves are true
   and they must be read together.

   **Qualified in the closing pass.** The first half is true of field and array
   indices; it is not true of every index the frontend accepts.
   `TextureOpExpression::type_check` rejects any texture coordinate argument
   whose type is not exactly `PrimitiveType::i32`, in three branches —
   `kFetchTexel` at `taichi/ir/frontend_ir.cpp:1216`, `kLoad` at `:1233`,
   `kStore` at `:1249` — each raising a `TaichiTypeError` through
   `ErrorEmitter`. **Those three sites are out of scope and are not counted
   among the sites item 6.2 must change**, per the planner's ruling recorded in
   3.12. The qualification stands as a fact about the frontend; it is not work.

4. **`taichi_max_num_snodes` has zero use sites in my territory.** Tree-wide the
   only sites are the declaration (`taichi/inc/constants.h:12`), one assertion
   (`taichi/codegen/llvm/struct_llvm.cpp:266`) and three array declarations in
   one runtime struct (`taichi/runtime/llvm/runtime_module/runtime.cpp:567,568,569`).
   SNode ids are used in `taichi/ir`, `taichi/analysis` and `taichi/transforms`
   only as identity keys, never bit-packed. **Item 6.1 does not touch my
   territory.**

5. **The type system carries no address width, and silently gives a wrong answer
   if asked for one.** `PointerType` (`taichi/ir/type.h:177-209`) has a pointee,
   an `addr_space_` int with a `// TODO: make this an enum`, and an
   `is_bit_pointer_` flag. No width field. `data_type_size`
   (`taichi/ir/type_utils.cpp:30-69`) does **not** refuse a pointer: `:35` calls
   `t.set_is_pointer(false)`, which at `taichi/ir/type.cpp:59-61` replaces the
   type with its pointee, so the function returns the **pointee's** size. The
   TODO at `taichi/ir/type_utils.cpp:31-34` records that a loud failure on
   pointers was intended and never written. Scalar i64/u64 are fully supported;
   addresses are simply not modelled.

6. **The SPIR-V Int64 capability already exists, declared inside my own
   territory, and is already enforced.**
   `taichi/inc/rhi_constants.inc.h:14` declares
   `PER_DEVICE_CAPABILITY(spirv_has_int64)`. It is materialised at
   `taichi/rhi/device_capability.h:11-15` and enforced at
   `taichi/codegen/spirv/spirv_ir_builder.cpp:311-314` and `:325-328`, which
   raise `TI_ERROR("Type {} not supported.")` when an i64 or u64 type is
   requested without it. So widening an index to i64 does **not** silently
   degrade on a Vulkan target lacking Int64 — it is a hard compile error today.
   What is missing is the link from the type layer: nothing in
   `taichi/ir/type.h`, `taichi/ir/type_factory.cpp` or any pass in
   `taichi/transforms/` consults `DeviceCapability`. This corrects the previous
   version of this report, which claimed the capability could not be expressed.

7. **One hard abort already exists at 2^31 cells, and one comparable path has
   none.** `taichi/transforms/demote_dense_struct_fors.cpp:29`
   `TI_ASSERT(total_n <= std::numeric_limits<int>::max());` aborts a dense
   struct-for over more than 2^31 cells. The equivalent flat extent for an
   **external-array** struct-for is built at
   `taichi/transforms/lower_ast.cpp:270-274` as a running i32 product with no
   guard and no assert at all.

8. **Widening indices would silently disable two optimisations.**
   `taichi/analysis/value_diff.cpp:81-87` and `:142-146` are two independent
   visitors that handle only `PrimitiveTypeID::i32` constants and otherwise
   return "unrelated". They feed six consumers:
   `taichi/analysis/alias_analysis.cpp:59,142,178`,
   `taichi/analysis/bls_analyzer.cpp:50`,
   `taichi/transforms/lower_access.cpp:237`,
   `taichi/transforms/bit_loop_vectorize.cpp:54`. The result would be correct
   but slower. Directly relevant to PROJECT-PLAN 6.4.

9. **The existing C++ regression tests assert the exact lowering item 6.2 must
   change.** `taichi/analysis/arithmetic_interpretor.cpp` has no callers in
   `taichi/` at all; its only two consumers are
   `tests/cpp/transforms/scalar_pointer_lowerer_test.cpp` and
   `tests/cpp/transforms/make_block_local_test.cpp`, which pin index types to
   i32 and assert exact numeric linearisation results. See section 3.10.

---

## 2. Where the section 6 work items touch my territory

### 6.1 Parameterise the SNode ceiling — NO TOUCH POINTS

Verified by `grep -rn "taichi_max_num_snodes" taichi/` over the whole tree.
Nothing in `taichi/ir/`, `taichi/analysis/`, `taichi/transforms/`. The only file
of mine involved is `taichi/inc/constants.h:12`, the declaration itself.

Two adjacent facts the plan does not record:

- `taichi/inc/constants.h:13` `constexpr int kMaxNumSnodeTreesLlvm = 512;` is a
  **second, lower, separate ceiling**. It appears at exactly two lines,
  `taichi/runtime/llvm/runtime_module/runtime.cpp:562` (sizing `roots`) and
  `:563` (sizing `root_mem_sizes`), both inside one struct. Section 6.1 does not
  name it. See Escalations.
- SNode ids are `int` (`taichi/ir/snode.h:88-89`, assigned at
  `taichi/ir/snode.cpp:220` from a `std::atomic<int>`). Any plausible raised
  ceiling stays far inside `int` range, so no width problem arises from 6.1 in
  my territory.

### 6.2 64-bit addressing — EXTENSIVE TOUCH POINTS

The complete inventory is section 3. The touch points that must change for a
width change to hold, ordered by how load-bearing they are:

| # | Location | What it is |
|---|---|---|
| 1 | `taichi/transforms/type_check.cpp:147-156` | Forced value-truncating cast of SNode indices to i32, skipped when `is_bit_vectorized` (`:122-125`) |
| 2 | `taichi/transforms/type_check.cpp:457-463` | Same, for external-array indices, silently |
| 3 | `taichi/transforms/type_check.cpp:520-522` | `LinearizeStmt` ret_type pinned i32 — the fused address |
| 4 | `taichi/ir/statements.h:1280` | `LinearizeStmt::strides` is `std::vector<int>` |
| 5 | `taichi/analysis/arithmetic_interpretor.cpp:98-109` | A **second** `LinearizeStmt` implementation; accumulates `int64_t` at `:99,106`, stores under the i32 `ret_type` at `:108` |
| 6 | `taichi/ir/snode.h:41,45,49` | `AxisExtractor` `num_elements_from_root`, `shape`, `acc_shape` all `int`. `num_elements_from_root` is the **root-to-leaf per-axis extent** and is unguarded; see 3.4 |
| 7 | `taichi/ir/snode.cpp:89-100` | `acc_shape` computed in int64, stored truncated to int, warning only. **Per container**, not per structure |
| 8 | `taichi/transforms/demote_dense_struct_fors.cpp:29` | Hard assert at 2^31 cells (dense path) |
| 9 | `taichi/transforms/lower_ast.cpp:270-274` | The external-array struct-for flat extent, an i32 running product, **no guard** |
| 10 | `taichi/ir/statements.h:1415-1416` | `OffloadedStmt::begin_value/end_value` declared `int32` |
| 11 | `taichi/transforms/offload.cpp:109,117` | Read via `val_int32()`, which asserts on non-i32 |
| 12 | `taichi/transforms/make_cpu_multithreaded_range_for.cpp:68-93` | Rebuilds the CPU iteration space in i32 **after** type-checking; `:84,93` re-read the 64-bit-clean dynamic bounds as i32 |
| 13 | `taichi/ir/frontend_ir.cpp:149-153` | Every loop variable born as pointer-to-i32 |
| 14 | `taichi/transforms/lower_ast.cpp:330` | A break-containing range-for's induction variable is an i32 alloca |
| 15 | `taichi/ir/frontend_ir.cpp:1160` | The dynamic-SNode append index: an `AllocaStmt(PrimitiveType::i32)` written by the `SNodeOpStmt(allocate)` at `:1161-1162` and read back at `:1169`. Device-side counterpart is `i32 *len` at `taichi/runtime/llvm/runtime_module/node_dynamic.h:61`. Added in the closing pass; see 3.12 |
| 16 | `taichi/transforms/type_check.cpp:466-476` | `LoopIndexStmt`, `LoopLinearIndexStmt`, `BlockCornerIndexStmt` all i32 |
| 17 | `taichi/transforms/type_check.cpp:159-163` | `MatrixPtrStmt` offset **hard-asserted** i32 |
| 18 | `taichi/transforms/lower_matrix_ptr.cpp:536-540` | A dynamically-indexed matrix-field **byte** offset computed in i32 |
| 19 | `taichi/transforms/check_out_of_bound.cpp:123-126` | The per-axis SNode bound compared against every index, an i32 constant. `shape_along_axis` (`taichi/ir/snode.cpp:174-177`) returns `AxisExtractor::num_elements_from_root` at `:176`, so this bound **is** the unguarded root-to-leaf `int` product of item 6 |
| 20 | `taichi/transforms/scalar_pointer_lowerer.cpp:33,37,56,63-65,78` | Index extraction shape/stride arithmetic in `int` |
| 21 | `taichi/transforms/utils.cpp:5-22` | `generate_mod`/`generate_div` take `int y`, emit i32 constants |
| 22 | `taichi/ir/type_factory.cpp:204` | Ndarray shape descriptor struct hardcodes i32 per dimension, across the host-to-kernel ABI |
| 23 | `taichi/analysis/value_diff.cpp:81-87,142-146` | Two index-difference visitors, i32 constants only |
| 24 | `taichi/ir/type.h:177-209` + `taichi/ir/type_factory.cpp:86-95` | `PointerType` has no width; pointer types interned on (pointee, is_bit_pointer) |
| 25 | The mesh passes | Eight `cast_value`-to-i32 sites, section 3.3 |
| 26 | `tests/cpp/transforms/` | Two tests assert the current lowering exactly, section 3.10 |

### 6.3 Adaptive module loading — the capability layer already exists

**This section replaces an incorrect assessment in the previous version of this
report, which stated that a target's 64-bit integer capability could not be
expressed.** It can, and it is.

`taichi/inc/rhi_constants.inc.h` is in my territory and I did not open it on the
first pass. Read in full (102 lines). Its header comment at `:1-7` states the
design: *"Device capability is a shared intelligence between the runtime
environment and the code generator."* Line 14 declares
`PER_DEVICE_CAPABILITY(spirv_has_int64)`, in a block of SPIR-V capabilities at
`:10-34` that also carries `spirv_has_int8` (`:12`), `spirv_has_int16` (`:13`),
`spirv_has_float64` (`:16`) and `spirv_has_atomic_int64` (`:17`).

The enum is built from that file at `taichi/rhi/device_capability.h:11-15`, with
the query surface (`contains`/`get`/`set` over a
`std::map<DeviceCapability, uint32_t>`) at `:20-30`.

It is enforced, not merely declared:
- `taichi/codegen/spirv/spirv_ir_builder.cpp:64-66` emits
  `spv::CapabilityInt64` only when the capability is present.
- `:166-169` declares `t_int64_`/`t_uint64_` only when it is present.
- `:311-314` and `:325-328` raise `TI_ERROR("Type {} not supported.")` when an
  i64 or u64 type is requested without it.

**The accurate statement of the gap.** The capability is expressible and
enforced at SPIR-V codegen. What does not exist is any link from the type layer
to it. Nothing in `taichi/ir/type.h`, `taichi/ir/type_factory.cpp`, or any pass
in `taichi/transforms/` consults `DeviceCapability`. The only arch-awareness
anywhere in `taichi/ir/` is `data_type_format(DataType dt, Arch arch)`
(`taichi/ir/type_utils.h:5,17`, `taichi/ir/type_utils.cpp:113-154`), which
branches on `Arch::vulkan` at `:137` for a printf specifier and carries in-tree
comments at `:133` and `:136` about Vulkan's 64-bit integer limits.

Consequence for PROJECT-PLAN 6.2's stub note: an i64 index on a Vulkan target
lacking Int64 produces a hard error at `spirv_ir_builder.cpp:312`, not a silent
degradation. That is a better starting point than "the capability cannot be
expressed", and it changes what the stub architecture has to build. Building the
link is added structure, which standing instruction 4 forbids me from proposing.
See Escalations.

---

## 3. Complete inventory of 32-bit width assumptions in my territory

Grouped by kind. Every entry read, not inferred from the grep line alone.

### 3.1 Index and address values pinned to i32 by a pass

| Location | Detail |
|---|---|
| `taichi/transforms/type_check.cpp:148-155` | `GlobalPtrStmt` indices cast to i32 (`TaichiCastWarning`) |
| `taichi/transforms/type_check.cpp:122-125` | **Exception**: that loop is skipped entirely when `stmt->is_bit_vectorized` |
| `taichi/transforms/type_check.cpp:459-462` | `ExternalPtrStmt` indices cast to i32, no diagnostic |
| `taichi/transforms/type_check.cpp:160-161` | `MatrixPtrStmt::offset` asserted i32 |
| `taichi/transforms/type_check.cpp:170-171` | Range-for `begin`/`end` marked i32 |
| `taichi/transforms/type_check.cpp:467` | `LoopIndexStmt` -> i32 |
| `taichi/transforms/type_check.cpp:471` | `LoopLinearIndexStmt` -> i32 |
| `taichi/transforms/type_check.cpp:475` | `BlockCornerIndexStmt` -> i32 |
| `taichi/transforms/type_check.cpp:521` | `LinearizeStmt` -> i32 |
| `taichi/transforms/type_check.cpp:525` | `IntegerOffsetStmt` -> i32 (though its field is `int64`) |
| `taichi/transforms/type_check.cpp:119` | `ExternalTensorShapeAlongAxisStmt` -> i32 |
| `taichi/transforms/type_check.cpp:114` | `SNodeOpStmt` default -> i32 |
| `taichi/transforms/type_check.cpp:567` | `InternalFuncStmt` -> i32 |
| `taichi/ir/frontend_ir.cpp:151-152` | Loop variable -> pointer-to-i32 |
| `taichi/transforms/lower_ast.cpp:330` | Break-containing range-for induction variable -> `AllocaStmt(PrimitiveType::i32)` |
| `taichi/ir/frontend_ir.cpp:1114` | `SNodeOpExpression` default -> i32 |
| `taichi/ir/frontend_ir.cpp:1306` | `ExternalTensorShapeAlongAxisExpression` -> i32 |
| `taichi/ir/statements.cpp:146` | `SNodeOpStmt` ctor `element_type() = i32` |
| `taichi/ir/statements.h:1684` | `InternalFuncStmt` default ret_type i32 |
| `taichi/ir/statements.h:2064` | `MeshIndexConversionStmt` ret_type i32 |
| `taichi/ir/statements.h:2083` | `MeshPatchIndexStmt` ret_type i32 |
| `taichi/ir/frontend_ir.cpp:1358,1362,1392` | `MeshPatchIndexExpression`, `MeshRelationAccessExpression`, `MeshIndexConversionExpression` -> i32 |
| `taichi/transforms/make_mesh_block_local.cpp:526` | `mapping_data_type_ = PrimitiveType::i32;`. The derived alternative is present and **commented out** directly above at `:525`: `// mapping_data_type_ = mapping_snode_->dt.ptr_removed();` |
| `taichi/transforms/auto_diff.cpp:587,646,787,846,1857` | Matrix element index `ConstStmt`s built i32 at construction. `:587,646,787,846` use `insert_const(PrimitiveType::i32, ...)` (helper at `:14`); `:1857` uses `insert_const_for_grad(PrimitiveType::i32, ...)` |
| `taichi/transforms/auto_diff.cpp:1819` | A matrix element index built as `insert<ConstStmt>(TypedConstant((int32)i))` — i32 by the `TypedConstant(int32)` constructor at `taichi/ir/type.h:582` |
| `taichi/transforms/auto_diff.cpp:639,839,1828` | The **index vector's** `TensorType`, built as `get_tensor_type(tensor_shape, PrimitiveType::i32)`. `:1828` types the `MatrixInitStmt` fed by `:1819` |
| `taichi/ir/frontend_ir.cpp:1160` | **Added in the closing pass.** The dynamic-SNode append index. `SNodeOpExpression::flatten`'s `append` branch opens at `:1159` with `auto alloca = ctx->push_back<AllocaStmt>(PrimitiveType::i32, dbg_info);`, which `taichi/ir/statements.h:21-29` types pointer-to-i32; the `SNodeOpStmt(SNodeOpType::allocate, ...)` at `:1161-1162` takes it as `val`, and `:1169` reads it back with a `LocalLoadStmt` as the expression's value. Nothing casts it. See 3.12 |
| `taichi/transforms/scalarize.cpp:471,473` | **Added in the closing pass.** Two `ConstStmt(TypedConstant(PrimitiveType::i32, 0/1))` built inside `visit(AtomicOpStmt *)` (opens `:421`) on the f16 tensor branch, consumed at `:475-478` as the offsets of two `MatrixPtrStmt`s. Same class as the `auto_diff.cpp` group above and bounded at two elements |
| `taichi/transforms/make_mesh_thread_local.cpp:42` | **Added in the closing pass.** `ConstStmt(TypedConstant(PrimitiveType::i32, 1))` in `make_mesh_thread_local_offload` (opens `:13`), added at `:43-44` to the `MeshPatchIndexStmt` created at `:39-40`, whose own `ret_type` is the i32 pinned at `taichi/ir/statements.h:2083`. Mesh patch index arithmetic in i32 |

**Added in the amendment pass: the nine `auto_diff.cpp` index constructions.**
The previous version inventoried that file's three `val_int32()` aborts
(section 3.6) and none of its index constructions, which is an asymmetry a
reader would trip on. All nine are bounded by matrix element count rather than
by structure size, so their consequence is low, but they are index values pinned
i32 at birth in this territory and they belong here. My own sweep,
`grep -n "PrimitiveType::i32\|(int32)\|int32(" taichi/transforms/auto_diff.cpp`,
returns twelve lines: these nine plus the three `val_int32()` reads at `:576`,
`:776`, `:1770`. That closes the file. **`:1819` is mine and was in neither
adversary's list of eight**; it is the same kind of site as `:587` and the
others, differing only in spelling.

The casts inserted at `type_check.cpp:154,461` go through
`insert_type_cast_before` (`taichi/transforms/type_check.cpp:234-244`), which
emits `UnaryOpType::cast_value` — a genuine value truncation, not a
reinterpretation.

### 3.2 Structural metadata declared `int`

| Location | Field |
|---|---|
| `taichi/ir/snode.h:41` | `AxisExtractor::num_elements_from_root` |
| `taichi/ir/snode.h:45` | `AxisExtractor::shape` |
| `taichi/ir/snode.h:49` | `AxisExtractor::acc_shape` |
| `taichi/ir/snode.h:79` | `SNode::index_offsets` (`std::vector<int>`) |
| `taichi/ir/snode.h:80` | `SNode::num_active_indices` |
| `taichi/ir/snode.h:88-89` | `SNode::counter`, `SNode::id` |
| `taichi/ir/snode.h:317` | `int shape_along_axis(int) const`, consumed at `check_out_of_bound.cpp:123` |
| `taichi/ir/snode.h:329-334` | Host read/write accessors take `const std::vector<int> &` index tuples |
| `taichi/ir/statements.h:376` | `ExternalPtrStmt::ndim` |
| `taichi/ir/statements.h:379` | `ExternalPtrStmt::element_shape` (`std::vector<int>`) |
| `taichi/ir/statements.h:456` | `MatrixOfGlobalPtrStmt::dynamic_index_stride` (bytes) |
| `taichi/ir/statements.h:1280` | `LinearizeStmt::strides` (`std::vector<int>`) |
| `taichi/ir/statements.h:1370` | `GetChStmt::chid` |
| `taichi/ir/statements.h:1415-1416` | `OffloadedStmt::begin_value/end_value` (`int32`) |
| `taichi/ir/statements.h:1439` | `OffloadedStmt::index_offsets` |
| `taichi/ir/type.h:222-227` | `TensorType::get_num_elements()` returns `int` |
| `taichi/ir/type.h:229-235,250` | `TensorType` shape is `std::vector<int>` |
| `taichi/ir/type.h:307-321` | `StructType::get_flattened_num_elements()` returns `int` |
| `taichi/ir/type.h:530-532,545` | `QuantArrayType::get_num_elements()` returns `int` |
| `taichi/ir/type_utils.h:13`, `taichi/ir/type_utils.cpp:30` | `data_type_size` **returns `int`**, and `:44-48` computes a `TensorType`'s byte size in `int` |
| `taichi/ir/frontend_ir.h:529` | `ExternalTensorExpression::ndim` |
| `taichi/ir/frontend_ir.h:615,617` | `MatrixFieldExpression::element_shape`, `dynamic_index_stride` |
| `taichi/ir/frontend_ir.h:676` | `IndexExpression::ret_shape` |
| `taichi/ir/analysis.h:20-21` | `DiffRange::coeff`, `low`, `high` |
| `taichi/ir/analysis.h:185` | `DiffPtrResult::diff_range` |

### 3.3 Live narrowing conversions (a value is actually truncated)

**Value-truncating casts inserted into the IR, outside `type_check.cpp`.** These
were previously and wrongly filed under "hardcoded widths". They are live
truncations:

| Location | What is truncated |
|---|---|
| `taichi/transforms/demote_mesh_statements.cpp:15-16` | A mesh index, and the result becomes the sole index of a `GlobalPtrStmt` at `:18` |
| `taichi/transforms/demote_mesh_statements.cpp:52-54` | The loaded mapping value, i.e. the converted index |
| `taichi/transforms/demote_mesh_statements.cpp:130-131` | Same, other conversion direction |
| `taichi/transforms/make_mesh_thread_local.cpp:104-106,112-114` | Mesh offset and count |
| `taichi/transforms/make_mesh_block_local.cpp:210-212,374-376,397-399` | Mesh mapping loads |

**Host-side narrowing:**

| Location | What narrows |
|---|---|
| `taichi/ir/snode.cpp:92` | `acc_shape` int64 -> `static_cast<int>`; the source comment says *"casting to int32 in extractors"* |
| `taichi/ir/snode.h:313` | `total_num_elemts *= (int)s->max_num_elements();` — int64 factor cast to int inside an int64 accumulation |
| `taichi/transforms/demote_dense_struct_fors.cpp:35` | `int64 total_n` -> `int32 end_value` |
| `taichi/transforms/demote_dense_struct_fors.cpp:60,63` | `int64 total_n` passed into `int y` parameters of `generate_mod`/`generate_div` |
| `taichi/analysis/value_diff.cpp:71-76` | `val_int()` (int64) -> `int` low/high of `DiffRange` |
| `taichi/transforms/offload.cpp:236-237,244` | int64 element count -> `int block_dim`. Harmless, capped at 1024 |

**A byte offset computed in 32 bits.** `taichi/transforms/lower_matrix_ptr.cpp:536-540`:
```cpp
auto stride = std::make_unique<ConstStmt>(
    TypedConstant(origin->dynamic_index_stride));
auto offset = std::make_unique<BinaryOpStmt>(
    BinaryOpType::mul, stmt->offset, stride.get());
offset->ret_type = stmt->offset->ret_type;
```
`dynamic_index_stride` is `int` (`taichi/ir/statements.h:456`), and `:540` forces
the product's `ret_type` to `stmt->offset->ret_type`, which
`taichi/transforms/type_check.cpp:160-161` has already hard-asserted to be i32.
So the dynamically-indexed matrix-field **byte** offset is 32-bit while every
other byte offset in the IR is `std::size_t`. This is exactly the inconsistency
PROJECT-PLAN 6.2 warns about. Also `taichi/transforms/lower_matrix_ptr.cpp:189,
232,424,477` build matrix element indices as `ConstStmt(TypedConstant(i))`, i32
by `taichi/ir/type.h:582`.

### 3.4 Silent overflow: `int * int` accumulations with no guard

**Important qualification, corrected in this revision.** Two of these are
indexed **per axis**, not per cell.
`taichi/transforms/scalar_pointer_lowerer.cpp:33` and
`taichi/transforms/demote_dense_struct_fors.cpp:19` both declare
`std::array<int, taichi_max_num_indices> total_shape` — one slot per axis — and
accumulate the extent of each axis separately. A structure of 2^40 cells spread
over four axes of 2^10 each leaves every slot at 1024. **They overflow when a
single axis extent exceeds 2^31, not when the cell count does.** They are real
32-bit accumulators and belong in the inventory, but they are not the
>2^31-cells failure mode. The cell count is carried separately in `int64` at
`demote_dense_struct_fors.cpp:18,26`, and that is what `:29` guards.

| Location | Product |
|---|---|
| `taichi/ir/snode.cpp:22-23` | `num_elements_from_root *= parent's num_elements_from_root` — **per axis, and root-to-leaf**. The load-bearing row; see the sub-table |
| `taichi/ir/snode.cpp:83` | `num_elements_from_root *= sizes[i]` — **per axis, and root-to-leaf** |
| `taichi/transforms/scalar_pointer_lowerer.cpp:37` | `total_shape[j] *= s->extractors[j].shape` — **per axis** |
| `taichi/transforms/demote_dense_struct_fors.cpp:24` | `total_shape[j] *= snode->extractors[j].shape` — **per axis** |
| `taichi/transforms/demote_dense_struct_fors.cpp:74` | `ext.acc_shape * ext.shape` |
| `taichi/transforms/check_out_of_bound.cpp:56` | `flattened_element *= stmt->element_shape[i]` |
| `taichi/transforms/check_out_of_bound.cpp:172` | `max_valid_index *= matrix_shape[i]` |
| `taichi/transforms/handle_external_ptr_boundary.cpp:64` | `max_valid_index *= matrix_shape[i]` |
| `taichi/ir/frontend_ir.cpp:744-745` | Constant-folded tensor offset, `offset = offset * shape[i] + index` in `int` |
| `taichi/ir/frontend_ir.cpp:732,735` | The **dynamic** tensor-offset branch, equally i32: an i32 zero seed and an i32 shape multiplier per axis |
| `taichi/ir/type.h:225` | `num_elements *= shape_[i]` |
| `taichi/ir/type_utils.cpp:44-48` | `TensorType` byte size, `get_num_elements() * data_type_size(element)` in `int` |

**Where an extent spanning more than one level is actually formed.**
**Corrected in this amendment.** The previous version listed three sites under
the heading "whole-structure flat extent" and the third one is not a
whole-structure quantity. Four distinct quantities exist, and they differ in
scope, in width and in what happens on overflow. They must not be conflated:

| Location | Quantity | Scope | Guard |
|---|---|---|---|
| `taichi/transforms/demote_dense_struct_fors.cpp:18,26,29,35` | `total_n`, the product of `num_cells_per_container` root-to-leaf | **Whole structure**, all axes | `int64` accumulator at `:18`, **hard assert** at `:29`, then narrowed into the `int32 end_value` field (`taichi/ir/statements.h:1416`) at `:35` |
| `taichi/transforms/lower_ast.cpp:270-274` | The external-array flat extent | **Whole array**, all axes | i32 running product of `ExternalTensorShapeAlongAxisStmt` values seeded by `ConstStmt(TypedConstant(1))` at `:271`. **No guard, no assert, no warning** |
| `taichi/ir/snode.cpp:21-23,83` | `AxisExtractor::num_elements_from_root` (`taichi/ir/snode.h:41`, `int`) | **Root-to-leaf, one axis.** `insert_children` at `:21-23` multiplies each child's per-axis value by its parent's; `:83` multiplies in this node's own size | **None whatsoever.** No assert, no warning, no `int64` shadow anywhere on the path |
| `taichi/ir/snode.cpp:89-93` | `acc_shape`, stored into `extractors[i].acc_shape` at `:92` and into `num_cells_per_container` at `:101` | **One container, all axes.** Not per structure | `int64` accumulator at `:89`, truncated to `int` at `:92`, **warning only** at `:95-100`. `num_cells_per_container` itself stays `int64` at `:101`, so the field and the extractor disagree above 2^31 |

**Why the fourth row is per-container, verified rather than assumed.**
`SNode::insert_children` (`taichi/ir/snode.cpp:14-34`) propagates **only**
`num_elements_from_root`, at `:21-23`. It does not touch `shape` or `acc_shape`,
which therefore start at the `AxisExtractor` defaults of 1
(`taichi/ir/snode.h:45,49`). `taichi/ir/snode.cpp:84` is a plain assignment,
`new_node.extractors[ind].shape = sizes[i];`, not an accumulation. So the
`acc_shape` loop at `:89-93` multiplies one node's own axis shapes and nothing
else, and `taichi/ir/snode.h:93-97` documents the result it stores as *"Product
of the |shape| of all the activated axes identified by |extractors|"*, with a
pointer to the cell/container terminology. That is the per-level container
extent — the very quantity escalation 7 says is bounded by the level rather than
by the structure. Calling it a whole-structure flattening contradicted this
report's own section 1.

**Why the third row is the load-bearing one, and why it was missed.**
`SNode::shape_along_axis` (`taichi/ir/snode.cpp:174-177`) returns
`extractor.num_elements_from_root` at `:176`. That return value is materialised
as the i32 `ConstStmt` at `taichi/transforms/check_out_of_bound.cpp:123-126`
(`int size_i = snode->shape_along_axis(i);` at `:123`,
`ConstStmt(TypedConstant(upper_bound_i))` at `:126`) and compared against every
SNode field index. The previous version of this report cited `snode.h:317` and
cited `check_out_of_bound.cpp:123-126` and never joined them. **The bound every
index is checked against is an unguarded root-to-leaf `int` product.** The field
has exactly two writers tree-wide, both in `taichi/ir/snode.cpp` and both in
this territory (`:22-23`, `:83`), one in-territory reader (`:176`), one cache-key
serialiser (`taichi/analysis/offline_cache_util.cpp:112`), and one consumer
outside it: `taichi/codegen/spirv/snode_struct_compiler.cpp:115-121` multiplies
it across axes into `sn_desc.total_num_cells_from_root`, a genuine
whole-structure cell count built out of this `int` field. That seam is agent
02's ground and is recorded here only as the reason the field matters.

### 3.5 Hardcoded widths, not derived

| Location | Hardcoded value | Derived alternative that exists |
|---|---|---|
| `taichi/transforms/simplify.cpp:239` | `previous_offset->offset * sizeof(int32) * snode->ch.size()` | `snode->cell_size_bytes` (`taichi/ir/snode.h:99`) |
| `taichi/transforms/simplify.cpp:261` | `stmt->chid * sizeof(int32) + previous_offset->offset` | same |
| `taichi/ir/statements.h:1787` | `sizeof(int32)` as AD-stack header size | — |
| `taichi/transforms/utils.cpp:17` | `TypedConstant(PrimitiveType::i32, bit::log2int(y))` shift constant | — |
| `taichi/transforms/make_block_local.cpp:157,334` | `TypedConstant((int32)bls_offset_in_bytes)` | — |
| `taichi/transforms/make_mesh_block_local.cpp:80,118,200,265,301,575` | `(int32)` / `int32(...)` BLS byte offsets | — |
| `taichi/ir/type_factory.cpp:204` | `{PrimitiveType::i32, "dim_N"}` per ndarray dimension | — |
| `taichi/transforms/check_out_of_bound.cpp:91,153,201` | `"%d"` printf specifier per printed index | `data_type_format` (`taichi/ir/type_utils.cpp:113`) |

**Correction to the previous version of this report.** I previously stated that
both `simplify.cpp` sites are guarded by the child-type assertion. They are not.
Verified by reading the visitor spans: `visit(SNodeLookupStmt *)` runs
`:222-249` and contains the loop at `:232-236` whose asserts at `:233` and
`:234-235` restrict every child SNode to i32 or f32. **That guards the `:239`
site only.** `visit(GetChStmt *)` runs `:251-273` (`:270` closes the inner `if`,
`:272` is `set_done(stmt);`, `:273` closes the visitor, `:274` is blank) and
contains no assertion, so the `:261` site is unguarded on its face. **The span
is corrected in this amendment; the previous version said `:251-274`.** The
substantive point is unchanged. A transitive argument exists — the
`IntegerOffsetStmt` gating the fold at `:255` is normally produced by the
guarded fold at `:238-244` on the same SNode — but that is an argument, not a
guard, and I could not rule out chained `GetChStmt` folds. See Escalations.

Note also that the derived alternative I named, `data_type_size`, is itself
`int`-returning (`taichi/ir/type_utils.h:13`). It is derived, and it is also
32-bit.

`taichi/transforms/bit_loop_vectorize.cpp:74` is the counter-example done
properly: it derives its width with `data_type_bits(load_data_type)`.

### 3.6 Strict-i32 readbacks that abort rather than truncate

`TypedConstant::val_int32()` (`taichi/ir/type.cpp:462-465`) opens with
`TI_ASSERT(get_data_type<int32>() == dt);`, and `TI_ASSERT` is not NDEBUG-gated
(`taichi/common/logging.h:100-107`).

**Thirteen call sites in this territory. Twelve of them fail loudly, in every
build, on a wider constant. Corrected in this amendment: the previous version
said thirteen.**

| Call site | Guarded on the type? |
|---|---|
| `taichi/transforms/offload.cpp:109,117` (2) | **No.** Guarded only on `is<ConstStmt>()` at `:107` and `:115`. Aborts |
| `taichi/transforms/scalarize.cpp:162,1097,1120,1147,1231,1311` (6) | **No.** Each guards on `is<ConstStmt>()` and never on the type. Aborts |
| `taichi/transforms/auto_diff.cpp:576,776,1770` (3) | **No.** Same shape. Aborts |
| `taichi/ir/control_flow_graph.cpp:462` (1) | **No.** `:456-457` asserts `is<ConstStmt>()` only. Aborts |
| `taichi/transforms/constant_fold.cpp:115` (1) | **Yes. Not an abort site.** See below |

`taichi/transforms/constant_fold.cpp:115` sits inside the
`HANDLE_INTEGRAL_BINARY` macro (`:111-125`) behind
`if (dt->is_primitive(PrimitiveTypeID::i32))` at `:113`, with an i64 arm at
`:116-118` calling `val_int()` and a u32/u64 arm at `:119-122`. The `dt` tested
is `lhs->val.dt`, assigned at `:65` under the comment at `:64` *"Type check
should have been done at this point."* — the **`TypedConstant::dt` field**
(`taichi/ir/type.h:551`), which is exactly the field `val_int32()` asserts
against, and not `Stmt::ret_type`, so `mark_as_if_const`
(`taichi/transforms/type_check.cpp:40-44`) cannot desynchronise it. That is
correct 64-bit dispatch, not an abort. An i64 constant takes the `:116` arm.

One residual, recorded because it is real and narrow: `:115` also calls
`rhs->val.val_int32()`, and the guard at `:113` tests only `lhs->val.dt`. A
mixed-width operand pair would abort there. Nothing in that function checks it;
the in-tree comment at `:64` asserts the invariant rather than proving it. That
is a consequence of a width change to weigh, not a defect today.

Three further hard aborts on a wide index:
`taichi/transforms/type_check.cpp:160-161` (`MatrixPtrStmt` offset),
`taichi/ir/snode.h:28-30` (`Axis` count),
`taichi/transforms/demote_dense_struct_fors.cpp:29` (2^31 cells).

**Total: fifteen hard aborts, not sixteen.** Twelve `val_int32()` readbacks plus
these three. Ten of the twelve read a `MatrixPtrStmt` offset, so they are
downstream of the single assert at `taichi/transforms/type_check.cpp:160-161`;
the two exceptions are the `offload.cpp` pair.

### 3.7 Fixed-size arrays sized by `taichi_max_num_indices`

In territory, twelve sites:
`taichi/ir/snode.h:28-30` (`Axis` bounds check), `:78` (`AxisExtractor
extractors[...]`), `:81` (`int physical_index_position[...]`);
`taichi/ir/snode.cpp:21,90,105` (loop bounds);
`taichi/analysis/offline_cache_util.cpp:110`;
`taichi/transforms/demote_dense_struct_fors.cpp:19,23`;
`taichi/transforms/scalar_pointer_lowerer.cpp:33,36,40`.

**Outside my territory but load-bearing on this constant:**
`taichi/runtime/llvm/runtime_module/runtime.cpp:288-290` declares
`struct PhysicalCoordinates { i32 val[taichi_max_num_indices]; };` — a
**device-side** struct with **i32** slots — and `:1012` loops over it. Mirrored
and asserted in codegen at `taichi/codegen/llvm/codegen_llvm.cpp:2356-2376`.
Also `taichi/codegen/llvm/struct_llvm.cpp:171` (refine-coordinates loop bound);
`taichi/program/launch_context_builder.cpp:230,247,269,302,322` (ndarray rank
assertions); `taichi/python/export_lang.cpp:559,1222` (Python bindings, which
PROJECT-PLAN 1.2 puts out of scope but which are still consumers).

Raising `taichi_max_num_indices` is therefore not a host-only change, and the
device-side per-axis coordinate width matches the frontend's i32 exactly.

### 3.8 Already 64-bit clean (does not need touching)

`taichi/ir/statements.h:1260` (`IntegerOffsetStmt::offset`, `int64`);
`taichi/ir/statements.h:1411-1412,1447-1448,1597,1618,1770` (`std::size_t`);
`taichi/ir/snode.h:97,99-100,306-308` (`num_cells_per_container` int64,
`cell_size_bytes`/`offset_bytes_in_parent_cell` size_t, `max_num_elements()`
int64);
`taichi/ir/type.h:243,257,305,336` (element byte offsets are `size_t`);
`taichi/ir/type.h:557,560,568,588,612` (`TypedConstant` union keyed on `uint64
value_bits`, with i64/u64 members);
`taichi/transforms/type_check.cpp:105-107` and `taichi/ir/frontend_ir.cpp:1109-1110`
(`SNodeOpType::get_addr` -> **u64**);
`taichi/transforms/lower_access.cpp:162-169` (the `get_addr` lowering already
emits `cast_bits` to `PrimitiveTypeID::u64`);
`taichi/transforms/alg_simp.cpp:118,132` and
`taichi/transforms/binary_op_simplify.cpp:66` (use `val_as_int64()`);
`taichi/ir/ir_builder.h:134,136` (`get_int64`, `get_uint64` exist);
`taichi/ir/type_factory.cpp:160-177` (`get_primitive_int_type` handles 64).

**The pattern:** byte offsets into buffers are already 64-bit, with the one
exception at `lower_matrix_ptr.cpp:536-540`. Index values, shapes and strides are
32-bit. The 32-bit assumption is concentrated on the index/shape/stride side.

### 3.9 Block-local storage, BLS analysis and mesh (recovered from notes)

Present in my working notes and absent from the previous version of this report.

**`taichi/ir/scratch_pad.h`** — all-`int`:
`:34-38` (`struct BoundRange { int low; int high; int range(); }`),
`:47,51,53` (`std::vector<int> coefficients / pad_size / block_size`),
`:75-76,107-108` (`std::numeric_limits<int>::max()/min()` sentinels),
`:97-98`, `:131-147` (`int pad_size_linear()`, `int block_size_linear()`),
`:149-157` (`int linearized_index(const std::vector<int> &indices)` — an
int-typed linearised address), `:182` (`inline int div_floor(int a, int b)`),
`:229-234` (`generate_address_code`, `int offset = 0`).
Consumers of extractor data: `:106`, `:214`.
My assessment that this is bounded by GPU shared memory rather than by structure
size is INFERRED, not verified — see 5.2.

**`taichi/analysis/bls_analyzer.cpp:27`** — `snode->extractors[j].shape - 1` used
as an index range bound. `:50` consumes `value_diff_loop_index`.

**Mesh, beyond the casts in 3.3 and the `ret_type` pins in 3.1:**
`taichi/transforms/demote_no_access_mesh_fors.cpp:31-34` is a fourth writer of
`OffloadedStmt::begin_value`/`end_value`, alongside
`demote_dense_struct_fors.cpp:32-35`,
`make_cpu_multithreaded_range_for.cpp:138-141` and `offload.cpp:108-117`.
`taichi/transforms/make_mesh_block_local.cpp:434` uses `default_shared_mem_size`;
`:440,444` reserve four bytes each.

**`taichi/transforms/lower_ast.cpp`, other i32 sites**, with the amendment pass
distinguishing what each alloca actually is, because "i32 allocas" ran together
two different kinds of value:

| Location | What it is |
|---|---|
| `taichi/transforms/lower_ast.cpp:330` | **An index.** The break-containing range-for's induction variable, `fctx.push_back<AllocaStmt>(PrimitiveType::i32)`, captured as `loop_var` at `:331` and bound to the loop variable id at `:332`. This is touch point 14 and the one that matters for item 6.2 |
| `taichi/transforms/lower_ast.cpp:168`, `:348` | **Not indices.** While-loop mask allocas: `:169` and `:349` both assign the result to `new_while->mask` |
| `taichi/transforms/lower_ast.cpp:361` | **Not an index, and its result is not captured.** An anonymous `AllocaStmt(PrimitiveType::i32)` inserted before the statement at `:360-361`. The real mask is inserted separately at `:365` and stored to at `:368` |
| `taichi/transforms/lower_ast.cpp:177`, `:363` | `TypedConstant((int32)0xFFFFFFFF)` all-ones mask values, paired with the mask allocas above |
| `taichi/transforms/lower_ast.cpp:151`, `:333` | `TypedConstant((int32)0)` and `TypedConstant((int32)1)` |
| `taichi/transforms/lower_ast.cpp:238` | `snode->physical_index_position[i]` |

Recorded because a width change to indices does not automatically imply one to
loop masks, and the previous flat list did not let a reader tell them apart. All
six rows, nine lines between them, were already in this report; only the
characterisation is new. (**Closing pass:** the unit was left unstated here and
"six locations" read as six lines. It is six rows: `:330`; `:168`, `:348`;
`:361`; `:177`, `:363`; `:151`, `:333`; `:238`.)

### 3.10 The existing C++ test harness

`taichi/analysis/arithmetic_interpretor.cpp` has **no callers anywhere in
`taichi/`**. Its only two consumers are tests of my territory:

**`tests/cpp/transforms/scalar_pointer_lowerer_test.cpp:55-100`.**
`:62` builds the index with `builder.get_int32(loop_index)`.
`:72` `ASSERT_EQ(lowerer.linears.size(), 3);` — asserts one `LinearizeStmt` per
SNode level, i.e. asserts the hierarchical shape of the lowering.
`:77` runs `irpass::type_check`, with the comment *"Set types so that
ArithmeticInterpretor can run correctly"* — the test depends on the i32 pinning.
`:91-99` evaluates each level's `LinearizeStmt` and asserts the exact numeric
result.

**`tests/cpp/transforms/make_block_local_test.cpp:145-188`.**
`:171-176` seeds `LoopIndexStmt` and `BlockCornerIndexStmt` with
`TypedConstant(PrimitiveType::i32, ...)` explicitly.
`:187` reads the result back with `val_int32()`, which asserts `dt == i32`
(`taichi/ir/type.cpp:462-465`). This test breaks under a width change by
assertion, not by a wrong value.
`:155-165` embeds a commented IR dump showing every statement typed `<i32>`.

Not examined in detail, but present and touching the same layer:
`tests/cpp/ir/ir_type_promotion_test.cpp`, `tests/cpp/ir/type_test.cpp`,
`tests/cpp/ir/frontend_type_inference_test.cpp`,
`tests/cpp/transforms/scalarize_test.cpp`,
`tests/cpp/transforms/simplify_test.cpp`.

`tests/` is not named in my territory, but these are the regression harness for
`taichi/transforms/scalar_pointer_lowerer.cpp` and
`taichi/transforms/make_block_local.cpp`, which are. See Escalations.

### 3.11 The `type_check` re-application surface

**New in this amendment.** The previous version named seven `irpass::type_check`
calls in `taichi/transforms/compile_to_offloads.cpp` plus three in-pass re-runs.
The three were right; the surface is far larger, and the reason it looked small
is a namespace one.

**Mechanism.** `type_check` is declared inside `namespace irpass`
(`taichi/ir/transforms.h:67`, the namespace opening at `:29` and closing at
`:210`). A pass that itself lives inside `namespace irpass` therefore calls it
unqualified, as `type_check(root, config);`. `taichi/transforms/make_thread_local.cpp`
is the plain example: its `namespace irpass` block runs `:217-232` and the call
sits at `:229`. **Fourteen of the twenty-two re-run lines below are of this
form, and a grep for `irpass::type_check` does not see any of them.**

**What this set is, stated exactly, so it can be checked against any other
count.** The unit is a **source line that textually invokes the IR pass
`irpass::type_check(IRNode *, const CompileConfig &)`**
(`taichi/ir/transforms.h:67`), or queues it. Not an execution count, not a
per-pass count, not a file count.

INCLUDED: every such line in `taichi/ir/`, `taichi/analysis/` and
`taichi/transforms/` — my territory — **except** the seven in
`taichi/transforms/compile_to_offloads.cpp`, which the report already counts
separately. Both spellings count, qualified `irpass::type_check(...)` and
unqualified `type_check(...)`. A line inside a loop or behind a condition counts
once, as one line.

EXCLUDED, and these are the exclusions a differing count most likely turns on:

- The seven driver calls in `taichi/transforms/compile_to_offloads.cpp`
  (`:57,66,193,200,208,310,391`). Counted separately, not in the twenty-two.
- `irpass::frontend_type_check`, a different pass
  (`taichi/transforms/frontend_type_check.cpp`).
- The `Expr` / expression member function `type_check(const CompileConfig *)` —
  `taichi/ir/expr.cpp:50` and `taichi/ir/frontend_ir.cpp:334,375,377,788,1041,
  1130,1838,1847`. Same name, different function, frontend not IR.
- `TypeSpec` / `Signature::type_check` on argument type vectors
  (`taichi/ir/type_system.h:246,280`, `taichi/ir/type_system.cpp:140`), and the
  `op->type_check(arg_types)` call into it at `taichi/ir/frontend_ir.cpp:635`.
- Comments naming the pass, which is what
  `taichi/ir/statements.h:383,1374`, `taichi/ir/frontend_ir.cpp:314` and
  `taichi/transforms/scalarize.cpp:260,512,623` are.
- Anything outside the three territory directories. Verified empty: a tree-wide
  grep for the pass, minus member-function and comment forms, returns nothing
  under `taichi/program/`, `taichi/runtime/`, `taichi/codegen/`, `taichi/rhi/`
  or `taichi/aot/`.

`DelayedIRModifier::type_check` (`taichi/ir/ir.h:617`) is a fourth distinct
function and is **included**, in its own row, because it queues the IR pass
rather than shadowing it. Excluding it instead would give nineteen lines across
thirteen files; that choice is stated rather than buried.

**Complete in-territory inventory on that definition.** Twenty-two call lines
across fourteen files. Re-derived from
`grep -rn "type_check(" taichi/transforms/ taichi/ir/ taichi/analysis/` with
every hit opened, and the file set cross-checked by a second tree-wide grep.

| Form | Sites |
|---|---|
| Unqualified `type_check(root, config)` — invisible to an `irpass::type_check` grep | `taichi/transforms/auto_diff.cpp:2407,2410,2417,2420,2430`; `taichi/transforms/demote_atomics.cpp:241`; `taichi/transforms/demote_mesh_statements.cpp:160`; `taichi/transforms/lower_access.cpp:294`; `taichi/transforms/make_block_local.cpp:391`; `taichi/transforms/make_mesh_block_local.cpp:681`; `taichi/transforms/make_mesh_thread_local.cpp:165`; `taichi/transforms/make_thread_local.cpp:229`; `taichi/transforms/offload.cpp:766,773` — **14 lines, 9 files** |
| Qualified `irpass::type_check(node, config)` | `taichi/transforms/check_out_of_bound.cpp:248`; `taichi/transforms/demote_operations.cpp:300,303`; `taichi/transforms/handle_external_ptr_boundary.cpp:100`; `taichi/ir/ir.cpp:566` — **5 lines, 4 files** |
| Queued via `DelayedIRModifier::type_check` (`taichi/ir/ir.h:617`, `taichi/ir/ir.cpp:533-534`), flushed through `taichi/ir/ir.cpp:566` | `taichi/transforms/simplify.cpp:219,344,349` — **3 lines, 1 file** |

Three qualifications, so the number is not read as more than it is:

- The three `simplify.cpp` lines do not each execute a type check. They append to
  `to_type_check_` (`taichi/ir/ir.h:603`) and are drained by the single loop at
  `taichi/ir/ir.cpp:565-567`. Counted as three call lines and one execution
  point.
- Several are conditional or repeated. `check_out_of_bound.cpp:248`,
  `demote_operations.cpp:303` and `handle_external_ptr_boundary.cpp:100` sit
  behind `if (modified)`. `demote_operations.cpp:300` sits inside the
  fixed-point loop opened at `:294`, so it runs once per rewrite round.
  `auto_diff.cpp:2407,2410` and `:2420` sit inside a loop over independent
  blocks (`:2403`, `:2418`), so they run once per block, not once per compile.
- Four of the fourteen files are mesh or autodiff passes:
  `taichi/transforms/demote_mesh_statements.cpp`,
  `taichi/transforms/make_mesh_block_local.cpp`,
  `taichi/transforms/make_mesh_thread_local.cpp` and
  `taichi/transforms/auto_diff.cpp`. Escalation 4 already asks whether the mesh
  path is in scope; it covers the first three and not the fourth. Between them
  they hold eight of the twenty-two lines.

Consequence for PROJECT-PLAN 8.1 item 1, stated as a fact and not a proposal: a
width change to any index type is re-applied by `type_check` at every one of
these points, not only at the seven in `compile_to_offloads.cpp`. Anything that
depends on a type surviving a pass has to survive re-derivation here too.

---

### 3.12 The `PrimitiveType::i32` sweep across the territory, closed

**New in the closing pass.** Sections 3.1 and 3.9 each closed one file with a
per-file i32 sweep and stopped there. The same sweep run over the whole
territory is one command:

```
grep -rn "PrimitiveType::i32" taichi/ir/ taichi/analysis/ taichi/transforms/
```

It returns **sixty-four lines**. Every one was opened. Fifty-four were already
inventoried in this report; ten were not. **Five** of the ten are now in the
inventory and **five** are recorded and excluded with the reason stated below,
so all sixty-four are accounted for. 5 + 5 = 10; 54 + 10 = 64.

**The sixty-four, by file, so any other count reconciles line by line:**

| File | Lines | Count |
|---|---|---|
| `taichi/ir/type_utils.h` | `:40` | 1 |
| `taichi/ir/type.h` | `:582` | 1 |
| `taichi/ir/type_factory.cpp` | `:204` | 1 |
| `taichi/ir/expr.cpp` | `:93` | 1 |
| `taichi/ir/statements.h` | `:1684`, `:2064`, `:2083` | 3 |
| `taichi/ir/statements.cpp` | `:146` | 1 |
| `taichi/ir/frontend_ir.cpp` | `:152`, `:1114`, `:1160`, `:1216`, `:1233`, `:1249`, `:1306`, `:1358`, `:1362`, `:1392` | 10 |
| `taichi/transforms/type_check.cpp` | `:114`, `:119`, `:154`, `:170`, `:171`, `:459`, `:461`, `:467`, `:471`, `:475`, `:521`, `:525`, `:567` | 13 |
| `taichi/transforms/utils.cpp` | `:17` | 1 |
| `taichi/transforms/demote_mesh_statements.cpp` | `:16`, `:54`, `:131` | 3 |
| `taichi/transforms/make_mesh_thread_local.cpp` | `:42`, `:106`, `:114` | 3 |
| `taichi/transforms/make_mesh_block_local.cpp` | `:212`, `:376`, `:399`, `:526` | 4 |
| `taichi/transforms/check_out_of_bound.cpp` | `:220` | 1 |
| `taichi/transforms/auto_diff.cpp` | `:587`, `:639`, `:646`, `:787`, `:839`, `:846`, `:1828`, `:1857` | 8 |
| `taichi/transforms/lower_ast.cpp` | `:168`, `:330`, `:348`, `:361` | 4 |
| `taichi/transforms/scalarize.cpp` | `:471`, `:473` | 2 |
| `taichi/transforms/make_cpu_multithreaded_range_for.cpp` | `:68`, `:70`, `:72`, `:81`, `:84`, `:90`, `:93` | 7 |
| **Total** | | **64** |

1 + 1 + 1 + 1 + 3 + 1 + 10 + 13 + 1 + 3 + 3 + 4 + 1 + 8 + 4 + 2 + 7 = 64.
Checked mechanically: the sixty-four lines written to a file, `wc -l` gives 64,
`cut -d: -f1 | sort -u | wc -l` gives 17 files.

**The five added to the inventory in this pass.**

| Line | What it is | Where it now sits |
|---|---|---|
| `taichi/ir/frontend_ir.cpp:1160` | The dynamic-SNode append index, born pointer-to-i32 | 3.1, and touch point 15 |
| `taichi/transforms/scalarize.cpp:471`, `:473` | i32 `MatrixPtrStmt` offset constants, f16 atomic split | 3.1 |
| `taichi/transforms/make_mesh_thread_local.cpp:42` | i32 constant added to a `MeshPatchIndexStmt` | 3.1 |
| `taichi/transforms/check_out_of_bound.cpp:220` | **Not an index.** `compare->ret_type = PrimitiveType::i32;` on the `cmp_ge` guarding a negative `pow` exponent, inside `visit(BinaryOpStmt *)` (opens `:209`), consumed by the `AssertStmt` at `:223-224`. A comparison result carried as i32, the same non-index class as the loop-mask allocas in 3.9 | this row, labelled |

**`taichi/ir/frontend_ir.cpp:1160` in full, because it is the material one.**
`SNodeOpExpression::flatten`, `append` branch:

```cpp
1159:  } else if (op_type == SNodeOpType::append) {
1160:    auto alloca = ctx->push_back<AllocaStmt>(PrimitiveType::i32, dbg_info);
1161:    auto addr = ctx->push_back<SNodeOpStmt>(SNodeOpType::allocate, snode, ptr,
1162:                                            alloca, dbg_info);
```

with `:1169` `ctx->push_back<LocalLoadStmt>(alloca, dbg_info);` supplying the
expression's value. The device side matches the width exactly:
`taichi/codegen/llvm/codegen_llvm.cpp:1367-1374` passes `llvm_val[stmt->val]`
into the `"allocate"` runtime call, whose signature is
`Ptr Dynamic_allocate(Ptr meta_, Ptr node_, i32 *len)` at
`taichi/runtime/llvm/runtime_module/node_dynamic.h:61`, and `:66` writes the
index through it. So this is an SNode element index born i32 in the frontend
with a matching i32 in the device runtime — the same class as
`taichi/ir/frontend_ir.cpp:151-152` and `taichi/transforms/lower_ast.cpp:330`,
which this report already calls load-bearing birth sites, and with the same
device-side-mirror property recorded for `PhysicalCoordinates` in 3.7. Dynamic
SNodes are sparse machinery, which PROJECT-PLAN 4.1 requires, so this is not a
peripheral site. The codegen and runtime halves are agent 02's and agent 03's
ground and are cited here only to fix the width, not claimed as my territory.

**One correction to the round-three finding that named this site.**
`adversary3-01-2.md` section 8.1 states that `taichi/transforms/type_check.cpp:114`
is "this site's downstream half". It is not. `visit(SNodeOpStmt *)`
(`taichi/transforms/type_check.cpp:105-116`) branches on `op_type`, and the
`allocate` op takes the branch at `:108-110`, which sets `ret_type` to
`PrimitiveType::gen` with `set_is_pointer(true)`. The i32 default arm at `:114`
is reached by `SNodeOpType::length` and the remaining ops, not by `allocate`.
The width of the append index is fixed by the alloca at `:1160` and by the
runtime signature, and by nothing in `type_check.cpp`. The site itself, and its
materiality, stand exactly as that adversary stated them; only the attribution
of the downstream half is wrong.

**The five recorded and excluded, with the reason, so each exclusion is
falsifiable.** None of these is dropped from the page. Two are not width
assumptions at all; three are, and are excluded on scope by a planner ruling
rather than by any judgement of mine.

| Line | Why it is not inventoried as a site item 6.2 must change |
|---|---|
| `taichi/ir/type_utils.h:40` | **Not a width assumption.** One arm of the `get_data_type<T>()` template ladder, `std::is_same<T, int32>()` -> `PrimitiveType::i32`. The i64 arm sits directly below at `:42`. It maps a C++ type to its own Taichi type; it pins nothing |
| `taichi/ir/expr.cpp:93` | **Not a width assumption.** One arm of the `Expr::Expr(...)` constructor ladder, `Expr::Expr(int32 x)`. `Expr::Expr(int64 x)` at `:95-97` builds an i64 `ConstExpression`. Same reason |
| `taichi/ir/frontend_ir.cpp:1216`, `:1233`, `:1249` | **A width assumption, and out of scope.** These are real: `TextureOpExpression::type_check` rejects any texture coordinate argument that is not exactly i32 — `if (arg_type != PrimitiveType::i32)` then `ErrorEmitter(TaichiTypeError(), ...)` — in the `kFetchTexel`, `kLoad` and `kStore` branches. Not a pin but a hard rejection, and the only place in the frontend that refuses a wider index outright. **Excluded by planner ruling**, recorded immediately below |

**Planner ruling on the texture-op checks, recorded so nobody re-derives them as
a gap.** PROJECT-PLAN 1.3 puts rendering entirely out of scope, and the project
owner has since confirmed positively that there is no graphic payload in the
foreseen data: particle attachments are strictly field interaction data and the
modelling is purely mathematical. Texture operations are therefore not work this
project will do. `taichi/ir/frontend_ir.cpp:1216`, `:1233` and `:1249` are
**found, read, and deliberately not inventoried** among the sites item 6.2 must
change. They remain on this page with their mechanism so that a later reader can
see that the sweep reached them and why they are out, rather than finding them
absent and reopening the territory. The factual qualification they force on
headline 3 stands; the sites are not work. This ruling closes what was
Escalation 16.

**What this sweep does NOT cover, stated so the completeness claim stays
falsifiable.** It matches the token `PrimitiveType::i32` only. It does not see
the `TypedConstant(int32)` constructor spelling
(`taichi/ir/type.h:582`), which is how
`taichi/transforms/auto_diff.cpp:1819`,
`taichi/transforms/lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477` and
`taichi/transforms/lower_ast.cpp:151`, `:177`, `:333`, `:363` are written; all
nine of those are in this report already, from the per-file sweeps in 3.1, 3.3
and 3.9, which used the wider pattern
`"PrimitiveType::i32\|(int32)\|int32("`. Nor does it see `int`-typed host
arithmetic, which is sections 3.2 and 3.4. The claim this sweep supports is
therefore narrow and exact: **every `PrimitiveType::i32` line in
`taichi/ir/`, `taichi/analysis/` and `taichi/transforms/` is accounted for in
this report.** It is not a claim that no 32-bit assumption remains unlisted in
any other spelling.

---

## 4. Can `taichi/ir/type.h` express 64-bit addressing?

**Scalar 64-bit integers: yes, completely.** `taichi/inc/data_type.inc.h:7,12`
declare i64/u64; `taichi/inc/data_type_with_c_type.inc.h:7,11` map them to C
types; `taichi/ir/type_utils.cpp:59,64` size them; `taichi/ir/type_utils.h:107,
112,123,140,152-153,180-181` classify them; `taichi/ir/type.h:560,568,588,612,
641,664,672` store and construct them;
`taichi/ir/type_factory.cpp:168-169` builds them on request. An i64 index
*value* is fully representable today.

**Type promotion already widens.** `compare_types`
(`taichi/ir/type_factory.cpp:222-246`) ranks integral types by `data_type_bits`
(`:234-237`), so `i32 op i64` promotes to i64 on its own. But that mechanism is
unavailable wherever a pointer is involved: `to_primitive_type`
(`taichi/ir/type_factory.cpp:248-262`) opens with
```cpp
if (d->is<PointerType>()) {
  TI_ERROR("promoted_type got a pointer input.");
}
```
at `:249-251`. Mixed-width arithmetic widens on its own; mixed-width *address*
arithmetic cannot use promotion at all.

**Addresses: not modelled, and the accessor gives a wrong answer.**

- `taichi/ir/type.h:177-209` — `PointerType` holds `Type *pointee_` (`:206`),
  `int addr_space_{0}` with a `// TODO: make this an enum` (`:207`), and
  `bool is_bit_pointer_` (`:208`). **No bit-width field, no accessor.**
- `taichi/ir/type_utils.cpp:30-69` — `data_type_size` does **not** refuse a
  pointer. `:35` is `t.set_is_pointer(false);`, and
  `taichi/ir/type.cpp:59-61` shows that call replaces the type with its pointee.
  There is no `PointerType` branch in the function body, so it falls through to
  the primitive dispatch and returns the **pointee's** size. The TODO at
  `:31-34` records that a loud failure on pointers was intended and never
  written. A caller asking for a pointer's size today gets a wrong answer, not
  an error.
- `taichi/ir/type_factory.h:43` / `taichi/ir/type_factory.cpp:86-95` — pointer
  types are interned on the key `std::make_pair(element, is_bit_pointer)`. Two
  pointers to the same pointee are the same object, so a width field alone would
  not be enough; the interning key would have to change too.

**What it would take (statement of the gap, not a proposal).** For the type
system to express 64-bit addressing, three things that do not exist today would
have to exist: a width on `PointerType`, a corresponding key in the interning
cache at `taichi/ir/type_factory.cpp:89`, and a link from the type layer to the
device-capability layer. On that third point, and unlike the previous version of
this report: the capability layer **already exists** and is already enforced
(section 2, item 6.3). What is missing is only the link. I am not proposing any
of this; PROJECT-PLAN 10.4 forbids me adding abstraction. Recorded under
Escalations as a decision for the planner.

---

## 5. VERIFIED versus INFERRED

### 5.1 VERIFIED — read directly in the source

- Every file:line reference in sections 2 and 3. Each was opened and read in
  context, not taken from a grep line. On the revision pass I re-read every line
  contested by either adversary.
- `taichi_max_num_snodes` appears at one declaration
  (`taichi/inc/constants.h:12`) and four use lines
  (`taichi/codegen/llvm/struct_llvm.cpp:266`;
  `taichi/runtime/llvm/runtime_module/runtime.cpp:567,568,569`), none in my
  territory. `kMaxNumSnodeTreesLlvm` appears at one declaration
  (`taichi/inc/constants.h:13`) and two use lines
  (`taichi/runtime/llvm/runtime_module/runtime.cpp:562,563`), none in my
  territory. Stated as lines rather than counts because "three runtime arrays in
  one struct" is one site or three depending on convention.
- SNode ids are used only as identity keys in `taichi/ir`, `taichi/analysis`,
  `taichi/transforms` — checked each of `taichi/ir/ir.cpp:81-89`,
  `taichi/analysis/same_statements.cpp:140-141`,
  `taichi/analysis/alias_analysis.cpp:154-163`,
  `taichi/analysis/offline_cache_util.cpp:101-120`,
  `taichi/analysis/gen_offline_cache_key.cpp:540`. No bit-packing anywhere.
- `insert_type_cast_before` emits `UnaryOpType::cast_value`
  (`taichi/transforms/type_check.cpp:237-239`), so the index narrowing is a
  value truncation.
- `TypedConstant::val_int32()` asserts on `dt` (`taichi/ir/type.cpp:462-465`),
  and `TI_ASSERT` is not NDEBUG-gated (`taichi/common/logging.h:100-107`), so
  the range-for bound path fails loudly rather than truncating.
- `mark_as_if_const` (`taichi/transforms/type_check.cpp:40-44`) rewrites
  `ret_type` without touching `val.dt`, so those two can disagree.
  `ConstStmt` sets `ret_type = val.dt` only in its constructor
  (`taichi/ir/statements.h:993`), and `type_check.cpp` has no `visit(ConstStmt*)`.
- `taichi/transforms/type_check.cpp:122-125` returns from `visit(GlobalPtrStmt*)`
  before the cast loop when `is_bit_vectorized`.
- `DataType::set_is_pointer(false)` at `taichi/ir/type.cpp:59-61` replaces the
  type with its pointee, so `data_type_size` returns the pointee's size for a
  pointer argument rather than refusing.
- `taichi/inc/rhi_constants.inc.h:14` declares
  `PER_DEVICE_CAPABILITY(spirv_has_int64)`; `taichi/rhi/device_capability.h:11-15`
  materialises it; `taichi/codegen/spirv/spirv_ir_builder.cpp:64-66,166-169,
  311-314,325-328` emit, declare and enforce it.
- `taichi/analysis/arithmetic_interpretor.cpp:98-109` is a second
  `LinearizeStmt` implementation, and `grep -rn "ArithmeticInterpretor"` shows
  its only consumers are the two tests in section 3.10.
- `total_shape` in `taichi/transforms/scalar_pointer_lowerer.cpp:33,36-37` and
  `taichi/transforms/demote_dense_struct_fors.cpp:19,24` is indexed per axis, so
  it overflows on a single axis extent above 2^31, not on a cell count.
- `taichi/transforms/lower_ast.cpp:270-274` builds the external-array struct-for
  extent as an i32 running product with no guard.
- Seven `irpass::type_check` calls in
  `taichi/transforms/compile_to_offloads.cpp` (`:57,66,193,200,208,310,391`),
  and `make_cpu_multithreaded_range_for` is gated at `:198` on
  `config.make_cpu_multithreading_loop && arch_is_cpu(config.arch)`.
- **Amendment pass.** Twenty-two further `type_check` call lines across fourteen
  files, section 3.11, re-derived by `grep -rn "type_check(" taichi/transforms/
  taichi/ir/ taichi/analysis/` and read one at a time. `type_check` is declared
  inside `namespace irpass` (`taichi/ir/transforms.h:29,67,210`), so the
  fourteen unqualified calls are invisible to an `irpass::type_check` grep;
  confirmed against `taichi/transforms/make_thread_local.cpp:217,229,232`.
- **Amendment pass.** `taichi/transforms/constant_fold.cpp:113` guards `:115` on
  `lhs->val.dt` (assigned `:65`) with an i64 arm at `:116-118`, so `:115` is
  correct 64-bit dispatch and not an abort. Each of the other twelve
  `val_int32()` sites re-read: `offload.cpp:107,115` guard on `is<ConstStmt>()`
  only; `control_flow_graph.cpp:456-457` likewise; the six `scalarize.cpp` and
  three `auto_diff.cpp` sites likewise. Ten of the twelve read a `MatrixPtrStmt`
  offset — verified by locating the enclosing visitor for each: five
  `visit(MatrixPtrStmt *)` in `scalarize.cpp` plus `:162`'s
  `stmt->src->as<MatrixPtrStmt>()`, `visit(MatrixPtrStmt *)` at
  `auto_diff.cpp:1770` and the `matrix_ptr_stmt` locals at `:576,776`, and
  `control_flow_graph.cpp:460-462`.
- **Amendment pass.** `SNode::insert_children` (`taichi/ir/snode.cpp:14-34`)
  propagates only `num_elements_from_root` (`:21-23`); `:84` assigns `shape`
  rather than accumulating it; so the `acc_shape` loop at `:89-93` is
  per-container. `shape_along_axis` (`:174-177`) returns
  `num_elements_from_root` at `:176`, materialised as the i32 bound at
  `taichi/transforms/check_out_of_bound.cpp:123,126`.
  `grep -rn "num_elements_from_root" taichi/` returns **seven lines covering six
  sites** — corrected in the closing pass, where the previous wording said "six
  lines" while enumerating six sites, one of which spans two source lines. The
  six sites: the
  declaration (`taichi/ir/snode.h:41`), two writers (`taichi/ir/snode.cpp:22-23,
  83`), one reader (`:176`), the cache-key serialiser
  (`taichi/analysis/offline_cache_util.cpp:112`) and the codegen seam
  (`taichi/codegen/spirv/snode_struct_compiler.cpp:121`). The in-tree comment at
  `snode_struct_compiler.cpp:117-120` says the extractors are also set by
  `StructCompiler::infer_snode_properties()`; that function does not exist
  anywhere in the tree, so the comment is stale and `taichi/ir/snode.cpp` is the
  only writer.
- **Completeness pass.** All twelve i32 tokens in
  `taichi/transforms/auto_diff.cpp` opened: nine index constructions (`:587`,
  `:639`, `:646`, `:787`, `:839`, `:846`, `:1819`, `:1828`, `:1857`) and the
  three `val_int32()` reads already inventoried. `:1819` is
  `TypedConstant((int32)i)` and `:1857` is `insert_const_for_grad`, neither of
  which matches the form the round-two adversaries gave for the group.
- **Completeness pass.** `visit(GetChStmt *)` spans
  `taichi/transforms/simplify.cpp:251-273`: `:255` opens the inner `if`, `:270`
  closes it, `:272` is `set_done(stmt);`, `:273` closes the visitor, `:274` is
  blank. `visit(SNodeLookupStmt *)` spans `:222-249`, with `:248`
  `set_done(stmt);` — that one was already right.
- **Completeness pass.** `taichi/transforms/type_check.cpp:169-172` is
  `visit(RangeForStmt *)`, with `mark_as_if_const` on `begin` at `:170` and on
  `end` at `:171`. `taichi/transforms/make_mesh_block_local.cpp:525` is the
  commented-out derived form of the hardcoded type at `:526`, and `:527` sizes
  from it. `taichi/transforms/lower_ast.cpp:330` is captured as `loop_var` at
  `:331`; `:168` and `:348` are assigned to `new_while->mask` at `:169` and
  `:349`; `:361`'s alloca result is never captured and the real mask is moved in
  at `:365`.
- **Completeness pass, count corrected in the closing pass.** **Six** distinct
  functions in this tree are declared with the name `type_check`, and only
  `irpass::type_check(IRNode *, const CompileConfig &)`
  (`taichi/ir/transforms.h:67`) is the pass. The other five, each named with its
  sites in section 3.11's exclusion list: `Expr::type_check`
  (`taichi/ir/expr.h:110`), the virtual `Expression::type_check`
  (`taichi/ir/expression.h:48`, with twenty-six `override` declarations in
  `taichi/ir/frontend_ir.h`), `Signature::type_check`
  (`taichi/ir/type_system.h:246`), `Operation::type_check`
  (`taichi/ir/type_system.h:280`) and `DelayedIRModifier::type_check`
  (`taichi/ir/ir.h:617`). The previous version said "four", which is the count
  of *families* in the exclusion list — it groups `Expr` with `Expression` and
  `Signature` with `Operation` — and did not reconcile against the list beneath
  it. The exclusion list itself was and is complete, so no site is affected;
  only the tally was wrong. Nothing outside `taichi/ir/`,
  `taichi/analysis/` and `taichi/transforms/` calls the pass.
- **Closing pass.** `grep -rn "PrimitiveType::i32" taichi/ir/ taichi/analysis/
  taichi/transforms/` returns sixty-four lines. All sixty-four opened and
  accounted for in section 3.12, which gives the per-file breakdown that sums to
  sixty-four. Ten were not previously inventoried: eight added, two excluded
  with the reason given.
- **Closing pass.** `taichi/ir/frontend_ir.cpp:1159-1162,1169`: the `append`
  branch's `AllocaStmt(PrimitiveType::i32)`, the `SNodeOpStmt(allocate)` that
  takes it as `val`, and the `LocalLoadStmt` that reads it.
  `taichi/ir/statements.h:21-29` shows `AllocaStmt(DataType)` sets `ret_type` to
  a pointer to that type. `taichi/codegen/llvm/codegen_llvm.cpp:1367-1374`
  passes `llvm_val[stmt->val]` into the `"allocate"` call;
  `taichi/runtime/llvm/runtime_module/node_dynamic.h:61` declares
  `i32 *len` and `:66` writes the index through it.
- **Closing pass.** `taichi/transforms/type_check.cpp:105-116` is
  `visit(SNodeOpStmt *)`: `get_addr` -> u64 at `:107`, **`allocate` -> `gen`
  with `set_is_pointer(true)` at `:109-110`**, `is_active` -> u1 at `:112`,
  default -> i32 at `:114`. So `:114` is not the downstream half of
  `frontend_ir.cpp:1160`, contrary to `adversary3-01-2.md` section 8.1.
- **Closing pass.** `taichi/ir/frontend_ir.cpp:1216`, `:1233`, `:1249` each read
  `if (arg_type != PrimitiveType::i32)` followed by an `ErrorEmitter` with a
  `TaichiTypeError`, in the `kFetchTexel`, `kLoad` and `kStore` branches of
  `TextureOpExpression::type_check`. A rejection, not a cast. Verified and then
  excluded on scope by planner ruling; see 3.12. Recorded as read, not
  inventoried as work.
- **Closing pass.** `taichi/transforms/scalarize.cpp:470-473` builds the two i32
  constants inside `visit(AtomicOpStmt *)` (opens `:421`); `:475-478` consume
  them as `MatrixPtrStmt` offsets.
  `taichi/transforms/make_mesh_thread_local.cpp:39-44` builds the
  `MeshPatchIndexStmt`, the i32 `one` at `:41-42`, and their sum.
  `taichi/transforms/check_out_of_bound.cpp:209-231` is `visit(BinaryOpStmt *)`;
  `:220` types the `cmp_ge` result i32 and `:223-224` feeds it to an
  `AssertStmt`.
- **Closing pass.** `taichi/transforms/lower_matrix_ptr.cpp:189`, `:232`,
  `:424`, `:477` re-read: each is
  `auto const_stmt = std::make_unique<ConstStmt>(TypedConstant(i));`, and
  `grep -n "TypedConstant" taichi/transforms/lower_matrix_ptr.cpp` returns those
  four plus `:537`, the byte-offset site in 3.3. All four were already in this
  report and are unchanged.
- The six `value_diff` consumers, by
  `grep -rn "value_diff_ptr_index\|value_diff_loop_index" taichi/`.
- `to_primitive_type` errors on a pointer operand
  (`taichi/ir/type_factory.cpp:249-251`); `compare_types` ranks by
  `data_type_bits` (`:234-237`).
- Four constants from `taichi/inc/constants.h` are used in my territory:
  `taichi_max_num_indices` (twelve sites, section 3.7),
  `taichi_global_tmp_buffer_size` (`taichi/transforms/offload.cpp:358`),
  `taichi_max_gpu_block_dim` (`taichi/ir/frontend_ir.cpp:132`) and
  `default_shared_mem_size` (`taichi/transforms/make_mesh_block_local.cpp:434`).
  Checked one constant at a time. (The previous version of this report said
  "only three", which silently excluded the constant section 3.7 is about.)
- `get_total_num_elements_towards_root` (`taichi/ir/snode.h:310`) has no callers
  anywhere in the tree, including Python bindings.
- `PhysicalCoordinates` at `taichi/runtime/llvm/runtime_module/runtime.cpp:288-290`
  is sized by `taichi_max_num_indices` with `i32` slots.

### 5.2 INFERRED — reasoning, not directly observed

- **That widening indices would cost throughput.** I traced both
  `value_diff.cpp` visitors to their six consumers and the code path is
  verified. The *consequence* — that `on_loop_tree` at
  `taichi/transforms/lower_access.cpp:237` would go false and activation would
  become conservative — is inference from reading, not measured. I did not build
  or run anything.
- **That `taichi/ir/scratch_pad.h`'s all-`int` arithmetic is not a scale limit.**
  This rests on scratch-pad being block-local storage bounded by GPU shared
  memory (cf. `taichi/inc/constants.h:39` `default_shared_mem_size = 65536`).
  Reasonable but not proven by a code path I read end to end.
- **That the `taichi/transforms/simplify.cpp:261` site is correct today.** The
  assertion at `:233-235` is inside a different visitor. The transitive argument
  (its gating `IntegerOffsetStmt` comes from the guarded fold) is plausible and
  unproven. Escalated rather than asserted.
- **That `int block_dim` narrowing at `taichi/transforms/offload.cpp:236-237` is
  harmless.** Rests on block dims being capped at
  `taichi_max_gpu_block_dim = 1024` (`taichi/inc/constants.h:14`) asserted at
  `taichi/ir/frontend_ir.cpp:132`. I did not check every path that sets
  `block_dim`.
- **That the two tests in section 3.10 would break under a width change.** The
  `val_int32()` call at `make_block_local_test.cpp:187` and the exact-value
  assertions at `scalar_pointer_lowerer_test.cpp:91-99` are verified; that a
  particular width change trips them depends on the change, which is not settled.
- **The list in section 3.8 of things that "do not need touching".** Each entry's
  declared width is verified. Whether each is genuinely sufficient depends on how
  the width change is scoped, which is not settled.

### 5.3 NOT investigated (outside my assignment)

Codegen lowering of `PointerType`, `LinearizeStmt` and `SNodeLookupStmt` beyond
the `spirv_has_int64` enforcement path traced above; the device runtime structs;
anything in `taichi/program/`, `taichi/runtime/`, `taichi/rhi/`, `taichi/aot/`.
Where I touched these it was to establish the boundary of my own territory or to
settle a claim about it, and it is marked as such. Within `tests/` I read the two
files in section 3.10 in full and only listed the others.

---

## 6. Escalations

Items requiring judgement beyond my assignment. Per PROJECT-PLAN 10.2 I have
stopped each thread rather than resolving it.

**1 to 15 are open.** 16 was raised and ruled inside the closing pass and is
closed. 17 was raised as an escalation and returned by the planner as a
condition to state rather than a judgement to make; it is kept in this register
because a reader looking for the open question will look here, but it is a
recorded dependency, not a request for a decision.

1. **`kMaxNumSnodeTreesLlvm = 512` is a second undocumented ceiling.**
   `taichi/inc/constants.h:13`, used at
   `taichi/runtime/llvm/runtime_module/runtime.cpp:562,563`. Section 6.1 names
   only `taichi_max_num_snodes`. Whether the SNode-tree ceiling is in scope for
   6.1 is not mine to decide.

2. **The device-capability layer exists; the link from the type layer does not.**
   `taichi/inc/rhi_constants.inc.h:14` declares `spirv_has_int64` and
   `taichi/codegen/spirv/spirv_ir_builder.cpp:312,326` enforce it with a hard
   `TI_ERROR`. Nothing in the type layer consults it. Whether an index width
   should be target-dependent, and where that decision should live, is a design
   question. Building the link is added structure, which standing instruction 4
   forbids me from proposing. **Planner.**

3. **The type system has no place to express an address width.** `PointerType`
   (`taichi/ir/type.h:177-209`) has no width, and the interning key at
   `taichi/ir/type_factory.cpp:89` would also need changing. Separately,
   `data_type_size` currently returns the *pointee's* size for a pointer
   argument (`taichi/ir/type_utils.cpp:35`, `taichi/ir/type.cpp:59-61`) rather
   than failing, and the in-tree TODO at `taichi/ir/type_utils.cpp:31-34` says a
   loud failure was intended. Whether to make that failure loud is a decision.

4. **Is the mesh path in scope?** Eight value-truncating i32 casts
   (section 3.3), four pinned `ret_type`s (section 3.1) and a fourth writer of
   the range-for bounds (`taichi/transforms/demote_no_access_mesh_fors.cpp:31-34`)
   live in the mesh passes. Nothing in PROJECT-PLAN section 6 says whether mesh
   is in scope. If it is out of scope, the "enforcement is concentrated" claim
   becomes nearly true and should be restated with that exclusion explicit.

5. **Is the CPU multithreaded range-for in scope?**
   `taichi/transforms/make_cpu_multithreaded_range_for.cpp:68-93` narrows the
   entire parallel iteration space to i32, including re-reading the 64-bit-clean
   dynamic bounds out of the global temporary buffer as i32 (`:84,93`). It runs
   on every CPU build (`taichi/transforms/compile_to_offloads.cpp:198-199`), and
   PROJECT-PLAN 2 makes CPU-only a first-class target, not a fallback. Nobody
   has asked whether an i32 CPU iteration space is acceptable.

6. **Are the existing C++ tests a constraint or a casualty?**
   `tests/cpp/transforms/scalar_pointer_lowerer_test.cpp` and
   `tests/cpp/transforms/make_block_local_test.cpp` encode the current i32
   lowering and are the only consumers of
   `taichi/analysis/arithmetic_interpretor.cpp`. Whether they are maintained is
   not stated anywhere I can see.

7. **What does "the address" mean for item 6.2?** `ScalarPointerLowerer` emits
   one `LinearizeStmt` **per SNode level**
   (`taichi/transforms/scalar_pointer_lowerer.cpp:55-56` resets `strides` inside
   the per-level loop; `:82` pushes that level's `LinearizeStmt`), so the
   frontend forms a chain of small per-level indices, not one flat address. A
   structure of 2^40 cells does not by itself produce a 2^40 value anywhere in
   the frontend IR. Whether 6.2 wants a wider per-level index, a wider flattened
   index, or a wider byte address changes which of the inventoried sites matter,
   and it has not been asked.

   **Sharpened in the amendment pass**, because the sub-table in 3.4 now
   separates four quantities that the previous version ran together. They have
   different scopes, different widths and different failure modes, and a
   decision on 6.2 selects among them rather than covering them all:

   | Quantity | Scope | Width today | On overflow |
   |---|---|---|---|
   | `AxisExtractor::acc_shape` / `num_cells_per_container` (`taichi/ir/snode.cpp:92,101`) | One container, all axes | `int` in the extractor, `int64` in the field | Warns, truncates the extractor, leaves the two disagreeing |
   | `AxisExtractor::num_elements_from_root` (`taichi/ir/snode.h:41`, `taichi/ir/snode.cpp:21-23,83`) | Root-to-leaf, one axis | `int` | **Nothing.** Silently wrong, and it is the bound at `taichi/transforms/check_out_of_bound.cpp:126` |
   | `total_n` (`taichi/transforms/demote_dense_struct_fors.cpp:26`) | Whole structure, all axes | `int64`, narrowed to `int32` at `:35` | Hard assert at `:29` |
   | The external-array extent (`taichi/transforms/lower_ast.cpp:270-274`) | Whole array, all axes | i32 IR values | **Nothing** |

   Which one item 6.2 means is not answerable inside this territory.

8. **Two different reactions to the same 2^31 overflow, and two paths with
   neither.** `taichi/ir/snode.cpp:95-100` warns and proceeds with a truncated
   value; `taichi/transforms/demote_dense_struct_fors.cpp:29` hard-asserts;
   `taichi/transforms/lower_ast.cpp:270-274` does neither. **Added in the
   amendment pass:** `AxisExtractor::num_elements_from_root`
   (`taichi/ir/snode.cpp:21-23,83`) does neither either, and it is the one of the
   four that every SNode index is bounds-checked against
   (`taichi/transforms/check_out_of_bound.cpp:123-126`). Which behaviour is
   intended, and which should govern, is a decision.

9. **`MatrixPtrStmt::offset` has two conflicting semantics.** The in-tree TODO at
   `taichi/ir/statements.h:512-520` says `offset` means "number of bytes" on some
   paths and "index" on others, with `offset_used_as_index()` (`:521-529`) as the
   discriminator. The byte-meaning branch is realised at
   `taichi/transforms/lower_matrix_ptr.cpp:536-540`, which computes a byte offset
   in i32 while every other byte offset in the IR is `size_t`. A width change
   would plausibly want different widths for the two meanings.

10. **Does the `simplify.cpp:233-235` assertion transitively cover
    `:261`?** The two sites are in different visitors
    (`:222-249` and `:251-273`). Cannot be settled from the two visitors alone.
    `taichi/ir/snode.h:99` (`cell_size_bytes`) is the derived value that exists.

11. **Widening the extractor fields changes the offline cache key.**
    `taichi/analysis/offline_cache_util.cpp:110-126` serialises
    `num_elements_from_root`, `shape`, `acc_shape`, `physical_index_position`
    and `num_cells_per_container` directly into the cache key. Cache versioning
    and invalidation is outside my assignment.

12. **`get_total_num_elements_towards_root` has no callers** and contains an
    `(int)` narrowing (`taichi/ir/snode.h:310-315`). Whether a narrowing in
    uncalled code needs fixing is a judgement I am forbidden from making
    (PROJECT-PLAN 10.3), and I am not making it either way.

13. **`ret_type` and `val.dt` can disagree on a range-for bound ConstStmt.**
    `taichi/transforms/type_check.cpp:40-44` rewrites `ret_type` without
    touching `val.dt`. Any width change has to pick which one is authoritative.

14. **Section 8.1 item 2 ("replacement for 1024") cannot be sized from my
    territory.** With zero use sites here, my territory offers no evidence about
    a suitable value. It must come from the runtime footprint
    (`taichi/runtime/llvm/runtime_module/runtime.cpp:567-569`) against the 2 GB
    baseline tier in PROJECT-PLAN 5.2 — agent 03's ground.

15. **Raising `taichi_max_num_indices` is not host-only.** It sizes a device
    struct at `taichi/runtime/llvm/runtime_module/runtime.cpp:288-290`, whose
    layout codegen asserts at `taichi/codegen/llvm/codegen_llvm.cpp:2365-2372`.
    Coordinating a change across all three territories is a planner decision.

16. **CLOSED — the texture-op index checks are out of scope.** Raised in the
    closing pass, ruled by the planner in the same pass, and kept here as a
    numbered stub so the trail is not lost.
    `taichi/ir/frontend_ir.cpp:1216`, `:1233`, `:1249` are found, read and
    recorded in 3.12, and deliberately **not** inventoried among the sites item
    6.2 must change. Reason and full mechanism in 3.12. Not open.

17. **NOT AN ESCALATION — a recorded condition.
    `taichi/ir/frontend_ir.cpp:1160` is inventoried as material, and whether
    item 6.2's width change reaches it resolves on a value nobody has set yet.**
    Raised as an escalation in the closing pass; the planner returned it as a
    dependency to state rather than a judgement to make, because it turns on the
    sizing rule that PROJECT-PLAN 8.1 item 2 records as undecided. Stated
    precisely, and not answered:

    **The width change reaches the append index if and only if a single dynamic
    SNode can hold more than 2^31 elements.** Nothing else about item 6.2
    decides it. A raised `taichi_max_num_snodes`, more SNodes, or a wider
    flattened address across the whole structure do not reach this site; only
    one node's own element count does.

    **What bounds that today.** The live element count of a dynamic node is
    `i32 n` at `taichi/runtime/llvm/runtime_module/node_dynamic.h:5`. It cannot
    exceed 2^31 − 1 as declared, and every operation on it is i32 or `int`:
    `atomic_add_i32(&node->n, 1)` at `:65`, `atomic_max_i32(&node->n, i + 1)` at
    `:21`, and the chunk walk's `int chunk_start` at `:22`, `:67` and `:99`. So
    the condition is false today by construction, not by convention, and this
    site is inert until it changes.

    **The fields that would have to move with it.** Device runtime, agent 03's
    ground, cited here to fix the width and not claimed as mine:
    `DynamicNode::n` (`node_dynamic.h:5`); the `i32 *len` out-parameter of
    `Dynamic_allocate` (`:61`, written at `:66`); the two atomics at `:21`,
    `:65`; the `int i` parameters of `Dynamic_activate` (`:16`),
    `Dynamic_is_active` (`:90`) and `Dynamic_lookup_element` (`:95`); the
    `int chunk_start` accumulators and the element address arithmetic
    `(i - chunk_start) * meta->element_size` at `:81` and `:105`; the `i32`
    return of `Dynamic_get_num_elements` (`:116`); and `DynamicMeta::chunk_size`
    (`:11`) with its host mirror `SNode::chunk_size` (`taichi/ir/snode.h:98`).
    In **my** territory, four sites move with it, all already inventoried:
    the alloca at `taichi/ir/frontend_ir.cpp:1160`; the `SNodeOpType::length`
    result pinned i32 at `taichi/transforms/type_check.cpp:114`, which is what
    `ti.length()` returns; the `SNodeOpExpression` default at
    `taichi/ir/frontend_ir.cpp:1114`; and the `SNodeOpStmt` constructor's
    `element_type() = PrimitiveType::i32` at `taichi/ir/statements.cpp:146`.

    **Where the condition already surfaces elsewhere in this report.** A single
    dynamic node above 2^31 elements is the same quantity as the per-container
    extent in the 3.4 sub-table: `acc_shape` is truncated to `int` at
    `taichi/ir/snode.cpp:92` with the warning at `:95-100`, while
    `num_cells_per_container` stays `int64` at `:101`. So the host already warns
    at exactly the point this condition becomes true, and Escalation 8 asks
    which of the four overflow behaviours should govern. The two resolve
    together or not at all.

    **The site stays inventoried as material either way.** Dynamic SNodes are
    part of the sparse machinery PROJECT-PLAN 4.1 requires. It is the condition
    that is open, not the site.

---

## 7. What changed in this revision, and why

Full adjudication in notes entries 27-38. Summary:

| Change | Cause |
|---|---|
| Headline 5 and section 4: `data_type_size` corrected from "refuses pointers" to "returns the pointee's size" | Adversaries disagreed with each other; adjudicated against `taichi/ir/type.cpp:59-61`. Adversary 1 was right, adversary 2's endorsement of my version was wrong and it later conceded. Notes 27 |
| New headline 6 and rewritten section 2 item 6.3: the SPIR-V Int64 capability exists in my own territory | I never opened `taichi/inc/rhi_constants.inc.h`. Verified all six enforcement sites myself. Notes 28 |
| Section 3.5: the `simplify.cpp` guard demoted from fact to one-site-only | Both adversaries; verified the two visitor spans. My section 3.5 had contradicted my own section 5.2. Notes 29 |
| Headline 2 and section 3.1: `is_bit_vectorized` exception added | I had read `type_check.cpp:122-125` and failed to draw the consequence. Notes 30 |
| Headline 2 rewritten from "one loop in one pass" | Verified the alternative enumeration and the seven `type_check` re-runs myself. Notes 31 |
| Section 3.4: `total_shape` qualified as per-axis | Adversary 2's mechanism correction, verified. Adversary 1 conceded the same point. Notes 32 |
| Added: `lower_ast.cpp`, `arithmetic_interpretor.cpp:98-109`, `check_out_of_bound.cpp:123-126`, `lower_matrix_ptr.cpp:536-540`, `type_factory.cpp:248-251` and the promotion rule, `frontend_ir.cpp:731-740` | Shared omissions; each opened and read before accepting. Notes 33 |
| New section 3.10: the C++ test harness | Not investigated on the first pass. Notes 34 |
| Section 5.1: "only three constants" corrected to four; `kMaxNumSnodeTreesLlvm` stated as lines rather than a count | Two internal inconsistencies in my own report. Notes 35 |
| New section 3.9 and additions throughout: `make_cpu_multithreaded_range_for.cpp`, the mesh casts recategorised into 3.3, `scratch_pad.h`, `bls_analyzer.cpp:27`, `demote_no_access_mesh_fors.cpp:31-34`, the mesh statement classes, `analysis.h:185`, the six `value_diff` consumers, the count of 13 `val_int32()` sites (corrected to 12 fail-loudly in the amendment pass, section 8) | Recovered from my own notes, which held them while the report did not. Notes 36 |
| Headline 3 rewritten to reconcile both halves rather than withdraw either | I did not accept the framing that the two statements conflict; I accept that I failed to reconcile them. Notes 37 |
| Escalations 4, 5, 6, 7 added | Follow from the above |

Nothing was removed from the inventory. One claim was withdrawn (that the
capability cannot be expressed), two were corrected in mechanism
(`data_type_size`, the `simplify.cpp` guard), and one was narrowed (the
enforcement concentration).

---

## 8. What changed in the amendment pass, and why

Round two returned three claims against this report. Each was verified against
the source before anything was changed. Full adjudication in notes entries
39-42.

| Claim | Verdict | Change |
|---|---|---|
| `taichi/transforms/constant_fold.cpp:115` is guarded at `:113` on `lhs->val.dt` with an i64 arm at `:116-118`, so it is 64-bit dispatch, not an abort; the counts should be twelve and fifteen | **Correct.** Verified at source. `:65` assigns `dt = lhs->val.dt`; `:113` tests it; `:116-118` handles i64. The other twelve `val_int32()` sites re-read individually and all confirmed unguarded on type | Section 3.6 rewritten as a table separating the twelve aborts from the one guarded site. Thirteen → twelve, sixteen → fifteen, in section 3.6 and headline 2 |
| `taichi/ir/snode.cpp:89-101` is a per-container extent, not a whole-structure one; the site that belongs there is `AxisExtractor::num_elements_from_root`, returned by `shape_along_axis` (`taichi/ir/snode.cpp:174-177`) and materialised at `taichi/transforms/check_out_of_bound.cpp:123-126`; this report cites both halves and never joins them | **Correct on both counts.** Verified: `insert_children` (`:14-34`) propagates only `num_elements_from_root` at `:21-23`; `:84` assigns `shape` rather than accumulating it | Section 3.4 sub-table rewritten from three sites to four distinct quantities with their scopes and guards, and the join stated. Touch-point rows 6, 7 and 18 amended. Escalations 7 and 8 sharpened. (Row 18 is row 19 after the closing pass inserted a row at 15) |
| The `type_check` re-run surface is understated by more than half, because most re-runs are unqualified calls from inside `namespace irpass`; fifteen further sites across fourteen files | **Mechanism correct, count wrong.** The namespace mechanism is real and verified (`taichi/ir/transforms.h:29,67,210`). The figure is not fifteen: the complete set outside `compile_to_offloads.cpp` is **twenty-two call lines across fourteen files**, of which nineteen are further than the three this report already named. "Fourteen files" is the whole set, not the remainder | New section 3.11 with the full inventory, the mechanism, and three qualifications on how the number should be read. Headline 2 updated |

Nothing else was touched. Nothing was removed from the inventory. No source file
was modified. The three items round two upheld — the headline 3 rewrite, the
reading of `data_type_size`, and the absence of citation faults — were left
alone.

### 8.1 The completeness pass

Five groups, re-opened under the rule that a confirmed in-territory site belongs
in the inventory regardless of whether it was under adjudication. Each was
opened at source; none was taken from a characterisation. Notes entries 43-46.

| Item | What I found | Change |
|---|---|---|
| The `taichi/transforms/auto_diff.cpp` index constructions | **Present, and the list I was given was short by one.** My own i32 sweep of the file returns twelve lines: nine index constructions plus the three `val_int32()` reads already inventoried. `:1819` builds a matrix element index as `TypedConstant((int32)i)` and appears in neither adversary's list of eight. `:1857` uses `insert_const_for_grad`, not `insert_const` as characterised | Three rows added to section 3.1, with the low-consequence qualification stated. The file is now closed |
| The `visit(GetChStmt *)` span in `taichi/transforms/simplify.cpp` | **This report was wrong.** The visitor spans `:251-273`: `:270` closes the inner `if`, `:272` is `set_done(stmt);`, `:273` closes the visitor, `:274` is blank. The report said `:251-274` in two places | Corrected in section 3.5 and escalation 10. The substantive point — `:261` unguarded on its face, the transitive argument unproven — is unaffected and stays inferred and escalated |
| `taichi/transforms/type_check.cpp:170-171` | **Present and already inventoried**, section 3.1 row "Range-for `begin`/`end` marked i32". Confirmed at source: `visit(RangeForStmt *)` opens at `:169`, `mark_as_if_const(stmt->begin, PrimitiveType::i32)` at `:170` and the same for `end` at `:171` | None needed |
| `taichi/transforms/make_mesh_block_local.cpp:526` | **Present and already inventoried**, section 3.1. Confirmed, and one thing was missing: the derived alternative sits commented out immediately above at `:525`, `// mapping_data_type_ = mapping_snode_->dt.ptr_removed();` | Row extended with `:525` |
| The i32 allocas at `taichi/transforms/lower_ast.cpp:168`, `:348`, `:361` | **All three present and already inventoried** in section 3.9, but listed flatly as "i32 allocas" alongside `:330`, which is a different kind of value. `:168` and `:348` are while-loop mask allocas, assigned to `new_while->mask` at `:169` and `:349`. `:361` is anonymous and its result is never captured; the real mask is inserted at `:365`. Only `:330` is an index | Section 3.9 list replaced with a table separating index from mask. No site added or removed |

Section 3.11 also gained an explicit definition of what its twenty-two-line
count includes and excludes, so a differing count can be reconciled against it
line by line rather than argued about.

### 8.2 The closing pass

Round three ruled this report correct, found no citation fault in a seventy-line
sample, upheld it on all five contested items, and confirmed its `type_check`
set by independent re-derivation twice. One adversary ruled it complete; the
other did not, on one ground, and that ground is what this pass exists to close.
Notes entries 47-50.

| Item | What I found | Change |
|---|---|---|
| **The completeness gap.** `grep -rn "PrimitiveType::i32"` over the three territory directories returns sixty-four lines; neither report ran it to the end | **Correct, and larger than claimed.** I ran it. Sixty-four lines, all opened. **Ten** were not in this report, not six: the six the adversary named plus `taichi/transforms/make_mesh_thread_local.cpp:42`, `taichi/transforms/check_out_of_bound.cpp:220`, `taichi/ir/type_utils.h:40` and `taichi/ir/expr.cpp:93` | New section 3.12 with the full per-file accounting summing to sixty-four. Five lines added to the inventory, five recorded and excluded with reasons. Three rows added to 3.1 for the index sites, one to 3.12 for the non-index one |
| `taichi/ir/frontend_ir.cpp:1160` is the material one, the same class as the two birth sites this report calls load-bearing, with an `i32 *len` device counterpart | **Correct.** Verified at source and traced out of territory to `taichi/runtime/llvm/runtime_module/node_dynamic.h:61` through `taichi/codegen/llvm/codegen_llvm.cpp:1367-1374` | Added to 3.1, to 3.12 in full, and as touch point 15 (rows 15-25 renumbered to 16-26). Headline 2's birth sites go three to four. New Escalation 17 |
| Its downstream half is `taichi/transforms/type_check.cpp:114` | **Wrong, and I changed nothing on it.** `visit(SNodeOpStmt *)` (`:105-116`) sends `SNodeOpType::allocate` to `:109-110`, `PrimitiveType::gen` with `set_is_pointer(true)`. The i32 default arm at `:114` is reached by `length` and the rest, not by `allocate`. The site's existence and materiality stand; only the attribution is wrong | Recorded in 3.12 and in section 5.1. No inventory change |
| Four further sites of the same class at `taichi/transforms/lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477`, named by nobody before round three | **Already present.** Section 3.3 closing sentence has all four. Re-verified at source: each is `auto const_stmt = std::make_unique<ConstStmt>(TypedConstant(i));`, and the file's `TypedConstant` grep returns those four plus `:537` | None. Re-verification recorded in 5.1 |
| One arithmetic defect | **Found, and it is the "four distinct functions" tally in section 5.1.** Six functions in this tree are declared `type_check`; the exclusion list in 3.11 names all six positions but groups them into four families, and the tally was taken from the families rather than derived from the list. Harmless: no site, count or conclusion depends on it | Corrected to six, with all six named and the source of the old figure stated |
| Second, smaller unit slip found by the same method | `grep -rn "num_elements_from_root" taichi/` returns **seven** lines, not the six the report claimed; six is the site count, and one site spans two source lines. Also in 3.9, "all six locations" was six table rows holding nine lines | Both corrected to state the unit. No site changes |

**Derived, not asserted.** Every total in this report was re-derived from its own
enumeration in this pass and checked mechanically where a grep could do it. The
following reconcile and are unchanged: fifteen hard aborts (twelve `val_int32()`
readbacks plus three), thirteen `val_int32()` call lines in territory, ten of
the twelve aborts reading a `MatrixPtrStmt` offset, eight mesh `cast_value`
sites, twelve `taichi_max_num_indices` sites across thirteen lines, six
`value_diff` consumers, one declaration and four use lines for
`taichi_max_num_snodes`, one declaration and two for `kMaxNumSnodeTreesLlvm`,
nine `auto_diff.cpp` index constructions in a twelve-line file sweep, four
constants from `taichi/inc/constants.h`, and the whole of section 3.11:
twenty-two call lines across fourteen files, split fourteen unqualified in nine
files, five qualified in four, three queued in one, plus seven in the driver for
twenty-nine. That last set was written to a file and counted with `wc -l` and
`cut -d: -f1 | sort -u | wc -l`, giving 22 and 14, and every line printed from
source to confirm it is a call. Nothing in section 3.11 changed.

**The two escalations the closing pass raised were ruled inside it, and both
rulings are on the page.**

| Escalation | Ruling | Change |
|---|---|---|
| 16, whether the texture-op index checks belong in the inventory | **Out of scope.** PROJECT-PLAN 1.3 puts rendering entirely out of scope, and the project owner has confirmed positively that there is no graphic payload in the foreseen data: particle attachments are strictly field interaction data and the modelling is purely mathematical. Texture operations are not work this project will do | `taichi/ir/frontend_ir.cpp:1216`, `:1233`, `:1249` moved out of the 3.1 inventory into 3.12's recorded-and-excluded table, given the same treatment as the two type-ladder arms: on the page, with the mechanism and the reason, not counted among the sites item 6.2 must change. Headline 3's factual qualification stands and now says the sites are not work. Escalation 16 marked closed. Additions to the inventory drop from eight lines to five; exclusions rise from two to five |
| 17, whether item 6.2's width change reaches the append index | **Not answerable, and not to be guessed.** It depends on the sizing rule PROJECT-PLAN 8.1 item 2 records as undecided. Record the condition instead | Escalation 17 rewritten as a recorded condition rather than a question: the width change reaches the append index **if and only if a single dynamic SNode can hold more than 2^31 elements**; what bounds that today (`i32 n` at `taichi/runtime/llvm/runtime_module/node_dynamic.h:5`, and every operation on it i32 or `int`); the device fields that would have to move with it; the four sites in my own territory that move with it; and the join to Escalation 8, since the host already warns at `taichi/ir/snode.cpp:95-100` at exactly the point the condition becomes true. The site stays inventoried as material, since dynamic SNodes are sparse machinery PROJECT-PLAN 4.1 requires |

Nothing was removed from the inventory except the three texture-op lines, which
were moved rather than dropped and are recorded in full with the ruling that
excluded them. No source file was modified. No other
agent's file was touched. The five items round three upheld — the `type_check`
set, the three `lower_ast.cpp` allocas, `auto_diff.cpp:1819`,
`constant_fold.cpp:115`, and citation accuracy — were re-read where cheap and
left exactly as they stood.

