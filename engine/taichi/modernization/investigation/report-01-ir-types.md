# Report 01 — Frontend IR and type system

Explore agent 01. Territory: `taichi/ir/`, `taichi/inc/`, `taichi/analysis/`,
`taichi/transforms/`.

**Revision 4 — closing pass.** Both round-three adversaries
(`adversary3-01-1.md`, `adversary3-01-2.md`) judge this report CORRECT with one
arithmetic defect, and NOT COMPLETE. Neither recommends a fourth investigative
round. This revision fixes the arithmetic, corrects one mischaracterisation,
and closes the completeness gap by running the exhaustive
`PrimitiveType::i32` sweep neither adversary's predecessor ran to the end.
**Section 7.6 records what revision 3 got wrong and what this pass changed.**
New section 3.13 enumerates all sixty-four lines the sweep returns, with a
disposition for every one, and records the limit of what that sweep can reach.

Revision 3 was itself an amendment pass against seven claims; section 7.1
records what revision 1 got wrong and section 7.5 what revision 2 got wrong.
Both stand unchanged.

Working notes: `modernization/investigation/notes-01-ir-types.md`, entries
N001–N062. N022–N038 are the revision-2 pass. N039–N050 are the revision-3
amendment pass. **N051–N062 are this closing pass** and record the
adjudication of each round-three claim, the four sweeps run, and the counts
checked mechanically.

Line references in this revision were opened individually and checked for both
content **and attribution**. The first revision claimed re-verification on the
strength of content checks alone; that claim was false and has been struck.
Paths are relative to `/opt/project/taichi/`.

---

## 1. Headline finding

The type system can already express 64-bit integers. `i64` and `u64` exist,
`TypedConstant` carries them, `TypeFactory::get_primitive_int_type` accepts 64
bits, and `IndexExpression::type_check` accepts any integral index — qualified
in revision 4, since that is true of field and array indices and not of texture
coordinates, which are hard-required i32 (sections 6 and 3.14). Nothing needs
adding to
`taichi/ir/type.h` to *represent* a wide index.

What does not exist is any notion of an **index type** or **address type**.
`PointerType` (`taichi/ir/type.h:177-209`) carries a pointee, an
`int addr_space_{0}` tagged `// TODO: make this an enum` (`:207`), and a
bit-pointer flag. It carries no width. Width is written down as a literal at
each construction site.

Four qualifications on that, the first three of which the first revision of this
report got wrong, and all of which change what item 6.2 costs:

**The enforcement is not concentrated in one place.** Ten sites in four files
insert a narrowing cast (section 3.1). Twelve sites abort outright on a non-i32
constant (section 3.2). The i32 coercion pass has **twenty-nine invocation sites
in this territory** — seven in the compilation driver and twenty-two inside
individual passes across fourteen further files (section 5) — so a wider type
does not simply flow through once the pass is changed. Revision 2 said seven,
plus three named others. That was an undercount, and section 5 now gives the
whole set.

**The frontend IR does not form one flat address.** `ScalarPointerLowerer` emits
one `LinearizeStmt` **per SNode level** — `strides` is declared inside the
per-level loop at `taichi/transforms/scalar_pointer_lowerer.cpp:56` and the
statement is pushed at `:81-82`. Each level's index is bounded by that level's
container, not by the structure. A 2^40-cell structure produces a chain of small
indices, not one large one. The places a whole-structure extent *is* flattened
are three, listed in section 4.

**A target lacking 64-bit integers can already say so, inside this territory.**
`taichi/inc/rhi_constants.inc.h:14` declares
`PER_DEVICE_CAPABILITY(spirv_has_int64)`, and SPIR-V codegen already enforces it.
Section 6 sets this out. This directly answers the stub-architecture note in
plan 6.2, and the first revision of this report missed it despite the file being
in my own territory.

**The literal is written in four different spellings, and counting one of them
undercounts the surface.** New in revision 4. `TypedConstant` has an implicit
`int32` constructor (`taichi/ir/type.h:582`), so an i32 index constant can be
built with no `i32` token on the line at all. Sweeping for `PrimitiveType::i32`
returns sixty-four lines in this territory; sweeping for `(int32)`/`int32(`
returns thirty-seven more; `get_data_type<int32>()` returns three more; and the
implicit form leaves seventy-eight candidate lines in sixteen files that this
report does not classify. Four sites survived three rounds of adversarial review
precisely because they use the implicit spelling
(`taichi/transforms/lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477`).
Section 3.13 gives the four commands and their counts, and states precisely
which three are closed and where the fourth's boundary falls. Escalation 16
carries the open class, which the planner has taken off this report and given to
a dedicated pair as its own territory. **Any plan that sizes item 6.2 from a
single grep will undercount it.**

Upstream does record the 32-bit limit in two places, both still accurate:

- `taichi/ir/snode.cpp:95-100` emits `TaichiIndexWarning`: *"SNode index might be
  out of int32 boundary but int64 indexing is not supported yet. Struct fors
  might not work either."* A warning, not an error. The truncation it guards is
  at `:92`, a plain `static_cast<int>`, which **is** silent in every build.
  Qualified in revision 3: both the warning and the truncation are
  **per-container**, not per-structure. The root-to-leaf quantity on the adjacent
  field has no warning at all. Section 4.
- `taichi/transforms/simplify.cpp:189-205` emits, when `config.debug` is set, an
  assertion reading *"The indices provided are too big!"* and substitutes zero
  for a negative linear index.

---

## 2. What section 6 touches in this territory

### 2.1 Item 6.1 — parameterise `taichi_max_num_snodes = 1024`

**Zero uses in my territory.** `grep -rn taichi_max_num_snodes taichi/` gives the
declaration at `taichi/inc/constants.h:12` and use sites only at
`taichi/codegen/llvm/struct_llvm.cpp:266` and
`taichi/runtime/llvm/runtime_module/runtime.cpp:567-569`, exactly as plan 6.1
already records.

**A second, lower ceiling exists that plan 6.1 does not name.**
`taichi/inc/constants.h:13` declares `kMaxNumSnodeTreesLlvm = 512`, used at
`taichi/runtime/llvm/runtime_module/runtime.cpp:562` (`Ptr roots[...]`) and
`:563` (`size_t root_mem_sizes[...]`). Also zero uses in my territory. Raised
here because it bounds SNode *trees* where 1024 bounds SNodes, and nothing in
the plan records it. Escalated (section 8, item 9).

What my territory owns is **SNode identity**:

| Site | Content |
|---|---|
| `taichi/ir/snode.h:88` | `static std::atomic<int> counter;` |
| `taichi/ir/snode.h:89` | `int id{0};` |
| `taichi/ir/snode.cpp:12` | `std::atomic<int> SNode::counter{0};` |
| `taichi/ir/snode.cpp:220` | `id = counter++;` |

Ids are `int` from a process-global atomic, never recycled. Consumers, all
checked for bit-packing and none found: `taichi/ir/ir.cpp:81-85`
(`-1` sentinel), `taichi/analysis/alias_analysis.cpp:154-161` (`-1` sentinel),
`taichi/analysis/same_statements.cpp:140-141`,
`taichi/analysis/offline_cache_util.cpp:101-120`,
`taichi/analysis/gen_offline_cache_key.cpp:540`. `int` holds far more than 1024;
raising the ceiling does not stress any of them.

The one thing that does change here is the **offline cache key**.
`taichi/analysis/offline_cache_util.cpp:110-116` serialises all three
`AxisExtractor` int fields across all twelve axes, `:119`
`physical_index_position`, `:123` `num_cells_per_container`. Changing
`taichi_max_num_indices` or the extractor field widths changes the key.

### 2.2 Item 6.2 — 64-bit addressing

Section 3 is the full inventory. The load-bearing points:

1. **`taichi/transforms/type_check.cpp:154` and `:461` are the only two sites
   that insert a narrowing cast on a *non-mesh* pointer-statement index.** That
   is the true and useful form of the claim. The mesh passes insert eight more
   (section 3.1), and `type_check.cpp:154` is skipped entirely for
   bit-vectorized pointers (`:122-125`).
2. **`taichi/ir/statements.h:1280` — `LinearizeStmt::strides` is
   `std::vector<int>`.** Per SNode level, per section 1.
3. **`taichi/ir/snode.h:37-54` — `AxisExtractor` is three `int` fields**, and
   `taichi/ir/snode.cpp:92` casts an `int64` accumulator down into one of them
   explicitly, with the comment `// casting to int32 in extractors.`
4. **`taichi/ir/statements.h:1415-1416` — range-for bounds are `int32`**, and
   the readback at `taichi/transforms/offload.cpp:109`/`:117` **aborts** rather
   than truncating (section 3.2).
5. **`taichi/transforms/make_cpu_multithreaded_range_for.cpp:61-95` re-synthesises
   the whole parallel range-for in i32 after `type_check` has run**, including
   reading a *dynamic* bound back out of the `size_t`-offset global temporary
   buffer as i32 (`:84`, `:93`). CPU-only is a first-class target under plan
   section 2.
6. **`taichi/transforms/lower_ast.cpp:270-274` builds an external-array
   struct-for extent as a running i32 product with no guard and no assert**
   (section 3.12), as does the dynamic tensor-offset branch at
   `taichi/ir/frontend_ir.cpp:731-740`.
7. **New in revision 4. `taichi/ir/frontend_ir.cpp:1160` — the dynamic-SNode
   append index is born i32 in the frontend, and its device-side counterpart is
   i32 too.** `AllocaStmt(PrimitiveType::i32)` in the `append` branch of
   `SNodeOpExpression::flatten`, passed as the `val` of the
   `SNodeOpStmt(allocate, …)` at `:1161-1162` and returned via the
   `LocalLoadStmt` at `:1169`. On the LLVM path it reaches
   `Ptr Dynamic_allocate(Ptr meta_, Ptr node_, i32 *len)`
   (`taichi/runtime/llvm/runtime_module/node_dynamic.h:61`, writing at `:66`)
   through `taichi/codegen/llvm/codegen_llvm.cpp:1374`. This is a third birth
   site alongside points 1 and 2 above, it caps a dynamic SNode's element count
   at 2^31, and widening it is not a frontend-only change. Section 3.3.

8. **New in revision 3. `taichi/ir/snode.h:41` — `AxisExtractor::num_elements_from_root`
   is a plain `int` accumulated root-to-leaf** (`taichi/ir/snode.cpp:22-23`,
   `:83`) with no guard of any kind, returned by `shape_along_axis`
   (`taichi/ir/snode.cpp:174-177`) and materialised as the i32 bound compared
   against every SNode field index (`taichi/transforms/check_out_of_bound.cpp:123-126`).
   Section 4.

### 2.3 Related constant `taichi_max_num_indices = 12`

Complete in-territory list, from exhaustive grep:

| Site | Use |
|---|---|
| `taichi/inc/constants.h:5` | declaration |
| `taichi/ir/snode.h:28-30` | `Axis` ctor bounds check, `TI_ERROR_UNLESS` |
| `taichi/ir/snode.h:78` | `AxisExtractor extractors[taichi_max_num_indices];` |
| `taichi/ir/snode.h:81` | `int physical_index_position[taichi_max_num_indices]{};` |
| `taichi/ir/snode.cpp:21` | loop propagating `num_elements_from_root` |
| `taichi/ir/snode.cpp:90` | reverse loop computing `acc_shape` |
| `taichi/ir/snode.cpp:105` | dynamic-SNode standalone-dimension check |
| `taichi/analysis/offline_cache_util.cpp:110` | cache-key serialisation loop |
| `taichi/transforms/demote_dense_struct_fors.cpp:19` | `std::array<int, taichi_max_num_indices> total_shape;` |
| `taichi/transforms/demote_dense_struct_fors.cpp:23` | loop over it |
| `taichi/transforms/scalar_pointer_lowerer.cpp:33` | `std::array<int, taichi_max_num_indices> total_shape;` |
| `taichi/transforms/scalar_pointer_lowerer.cpp:36` | loop over it |
| `taichi/transforms/scalar_pointer_lowerer.cpp:40` | `std::array<bool, taichi_max_num_indices> is_first_extraction;` |

Two of these embed fixed-size C arrays **by value in every `SNode`**
(`snode.h:78`, `:81`), so the constant is a per-SNode footprint, not only a
bound — the same property plan 6.1 identifies for `taichi_max_num_snodes`.

It is also **not host-only**. `taichi/runtime/llvm/runtime_module/runtime.cpp:288-290`
declares `struct PhysicalCoordinates { i32 val[taichi_max_num_indices]; };` —
the device-side struct carrying struct-for coordinates, dimensioned by the same
constant and with **i32 slots**. That is outside my territory, and it is the
device-side mirror of `type_check.cpp:467`. It means the frontend's i32 loop
index is not a free choice for the LLVM path. Escalated (section 8, item 3).

---

## 3. Inventory of 32-bit width assumptions in this territory

**Heading amended in revision 4.** Revisions 1 to 3 called this a *complete*
inventory. It is complete on the explicitly-typed spellings, and section 3.13
enumerates the four sweeps that establish that. It is **not** a complete
enumeration of the implicitly-typed `TypedConstant(<host int>)` class, which
section 3.13 measures at seventy-eight candidate lines in sixteen files and does
not classify. The unqualified word has been removed rather than defended.

### 3.1 Cast insertion — a real value truncation. Ten sites, four files.

Counted by me, from `grep -rn "cast_type = PrimitiveType::i32" ir/ transforms/
analysis/` plus the `insert_type_cast_before` calls in `type_check.cpp`. The two
adversaries gave eight and ten; ten is correct.

| Site | What is truncated |
|---|---|
| `taichi/transforms/type_check.cpp:154` | `GlobalPtrStmt` index. Warns first at `:149-152`: *"Field index {} not int32, casting into int32 implicitly"* |
| `taichi/transforms/type_check.cpp:461` | `ExternalPtrStmt` index. **Silent** — no warning |
| `taichi/transforms/demote_mesh_statements.cpp:16` | mesh index, immediately before it becomes a `GlobalPtrStmt` index at `:18` |
| `taichi/transforms/demote_mesh_statements.cpp:54` | loaded mapping value, i.e. the converted index |
| `taichi/transforms/demote_mesh_statements.cpp:131` | same, other conversion direction |
| `taichi/transforms/make_mesh_thread_local.cpp:106` | mesh offset |
| `taichi/transforms/make_mesh_thread_local.cpp:114` | mesh count |
| `taichi/transforms/make_mesh_block_local.cpp:212` | mesh mapping load |
| `taichi/transforms/make_mesh_block_local.cpp:376` | mesh mapping load |
| `taichi/transforms/make_mesh_block_local.cpp:399` | mesh mapping load |

Both `type_check.cpp` sites route through the helper
`insert_type_cast_before` at `taichi/transforms/type_check.cpp:234-244`.

**Exception:** `taichi/transforms/type_check.cpp:122-125` returns from
`visit(GlobalPtrStmt *)` immediately when `stmt->is_bit_vectorized`, before the
cast loop at `:147-156`. A bit-vectorized global pointer's indices are **never**
cast.

Eight of the ten are mesh. Whether mesh is in scope therefore decides whether
this table has two rows or ten. Escalated (section 8, item 1).

### 3.2 Hard aborts on a non-i32 constant — twelve sites

Every `val_int32()` call site in the tree was opened and checked for a type
guard. `taichi/ir/type.cpp:462-465` is
`TI_ASSERT(get_data_type<int32>() == dt); return val_i32;`, and
`taichi/common/logging.h:100-107` expands `TI_ASSERT` unconditionally — **it is
not compiled out in release.**

| Site | Reads | Guarded? |
|---|---|---|
| `taichi/transforms/offload.cpp:109` | range-for `begin_value` | no — aborts |
| `taichi/transforms/offload.cpp:117` | range-for `end_value` | no — aborts |
| `taichi/ir/control_flow_graph.cpp:462` | `MatrixPtrStmt::offset` | `ConstStmt` only (`:456-457`) — aborts |
| `taichi/transforms/scalarize.cpp:162` | `MatrixPtrStmt::offset` | `ConstStmt` only (`:160`) — aborts |
| `taichi/transforms/scalarize.cpp:1097` | `MatrixPtrStmt::offset` | `ConstStmt` only (`:1096`) — aborts |
| `taichi/transforms/scalarize.cpp:1120` | `MatrixPtrStmt::offset` | `ConstStmt` only (`:1117`) — aborts |
| `taichi/transforms/scalarize.cpp:1147` | `MatrixPtrStmt::offset` | `ConstStmt` only (`:1144`) — aborts |
| `taichi/transforms/scalarize.cpp:1231` | `MatrixPtrStmt::offset` | `ConstStmt` only (`:1230`) — aborts |
| `taichi/transforms/scalarize.cpp:1311` | `MatrixPtrStmt::offset` | `ConstStmt` only (`:1307`) — aborts |
| `taichi/transforms/auto_diff.cpp:576` | `MatrixPtrStmt::offset` | no type guard — aborts |
| `taichi/transforms/auto_diff.cpp:776` | `MatrixPtrStmt::offset` | no type guard — aborts |
| `taichi/transforms/auto_diff.cpp:1770` | `MatrixPtrStmt::offset` | no type guard — aborts |
| `taichi/transforms/constant_fold.cpp:115` | folded operands | **guarded** by `:113`, with an explicit i64 branch at `:116-118`. **Not an abort site** |

Both adversaries listed `constant_fold.cpp:115` as an abort site. It is not — it
is correct 64-bit type dispatch. (`taichi/codegen/llvm/codegen_llvm.cpp:1054`,
outside my territory, is likewise guarded at `:1052` with an i64 branch at
`:1058`.)

**Structural point:** ten of the twelve abort sites read a `MatrixPtrStmt`
offset, so they are all downstream of the single hard assert at
`taichi/transforms/type_check.cpp:160-161`. The abort surface is not twelve
independent decisions — it is two, one of which has ten dependents.

Three further hard limits abort on structure rather than on a constant:

| Site | Limit |
|---|---|
| `taichi/transforms/type_check.cpp:160-161` | `TI_ASSERT` that a `MatrixPtrStmt` offset is i32 |
| `taichi/transforms/demote_dense_struct_fors.cpp:29` | `TI_ASSERT(total_n <= std::numeric_limits<int>::max())` — a fully-dense struct-for over more than 2^31 cells aborts the compiler |
| `taichi/ir/snode.h:28-30` | `Axis` ctor errors above `taichi_max_num_indices` |

### 3.3 Hardcoded i32 on statement and expression types — fifty-one sites, thirty-one rows

**This table is not an index count, and must not be read as one.** Its unit is a
hardcoded i32 on a statement or expression type, which is what the heading says.
Some of what it holds is not an index: loop masks, a comparison result, texture
coordinates. Where a row is not an index-width site the row says so. A width
change to addressing does not follow from a row being here; item 6.2 has to
select from this table, not adopt it. The point was put by both round-three
adversaries and it is correct.

**Count history, each figure derived from the enumeration beneath it.** Revision
2 headed the table "eighteen sites" over twenty; revision 3 corrected it to
thirty-three across twenty-two rows after thirteen sites were added; this
revision adds nine rows and eighteen sites, giving **fifty-one sites in
thirty-one rows across twelve files**. The row-by-row site counts are 1, 1, 2,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 3, 5, 3, 1, 1, 4, 3, 2, 1, 1, 1, 4,
1, and they sum to 51. Checked mechanically: the fifty-one `file:line` pairs
written to one file, `wc -l` gives 51, `sort -u | wc -l` gives 51, so there are
no duplicates, and `cut -d: -f1 | sort -u | wc -l` gives 12.

**Eighteen further sites added in revision 4**, in nine rows marked *closing
pass* below. `auto_diff.cpp:1819` and the four `lower_matrix_ptr.cpp` sites were
named by both round-three adversaries; `frontend_ir.cpp:1160` and the two
`scalarize.cpp` offset constants were named by `adversary3-01-2.md`; the four
`lower_ast.cpp` `TypedConstant((int32)…)` tokens were named by
`adversary3-01-1.md`; and `make_mesh_thread_local.cpp:42`,
`check_out_of_bound.cpp:220`, the three `scalarize.cpp` `get_data_type<int32>()`
indices and `ir_builder.cpp:125` were named by nobody and fell out of the four
sweeps this pass ran (section 3.13). Every one was opened at the line given.

**Three sites the sweep found are deliberately NOT in this table.** The texture
coordinate checks at `taichi/ir/frontend_ir.cpp:1216`, `:1233`, `:1249` are out
of scope by planner ruling and are recorded in section 3.14 instead. They are
not counted among the fifty-one, and item 6.2 does not have to change them.

The heading no longer says `ret_type`, because the last two rows are not
`ret_type` assignments: `taichi/transforms/lower_ast.cpp:330` is an `AllocaStmt`
element type and `taichi/ir/type_factory.cpp:204` is a struct **member** type in
`get_ndarray_struct_type`. Both are genuine hardcoded i32 and both belong in the
inventory; neither is a pinned `ret_type`. Verified individually.

| Site | Statement or expression |
|---|---|
| `taichi/transforms/type_check.cpp:114` | `SNodeOpStmt`, default branch (`get_addr` is `u64` at `:107`, `is_active` is `u1` at `:112`) |
| `taichi/transforms/type_check.cpp:119` | `ExternalTensorShapeAlongAxisStmt` |
| `taichi/transforms/type_check.cpp:170`, `:171` | **Amendment pass.** `visit(RangeForStmt *)` (`:169-173`) calls `mark_as_if_const(stmt->begin, PrimitiveType::i32)` and the same on `stmt->end`. Pins both range-for bound `ConstStmt`s. Discussed in section 7.1 in revision 2 but inventoried nowhere |
| `taichi/transforms/type_check.cpp:467` | `LoopIndexStmt` — every loop index value |
| `taichi/transforms/type_check.cpp:471` | `LoopLinearIndexStmt` |
| `taichi/transforms/type_check.cpp:475` | `BlockCornerIndexStmt` |
| `taichi/transforms/type_check.cpp:521` | `LinearizeStmt` |
| `taichi/transforms/type_check.cpp:525` | `IntegerOffsetStmt` (whose *field* is `int64`, `statements.h:1260`) |
| `taichi/transforms/type_check.cpp:567` | `InternalFuncStmt` |
| `taichi/ir/statements.h:1684` | `InternalFuncStmt` ctor default |
| `taichi/ir/statements.h:2064` | `MeshIndexConversionStmt` ctor |
| `taichi/ir/statements.h:2083` | `MeshPatchIndexStmt` ctor |
| `taichi/ir/statements.cpp:146` | `SNodeOpStmt` ctor, `element_type() = PrimitiveType::i32` |
| `taichi/ir/frontend_ir.cpp:151-152` | **`FrontendForStmt::add_loop_var` — every frontend loop variable is born `get_pointer_type(PrimitiveType::i32)`** |
| `taichi/ir/frontend_ir.cpp:1114` | `SNodeOpExpression::type_check`, default branch |
| `taichi/ir/frontend_ir.cpp:1306` | `ExternalTensorShapeAlongAxisExpression` |
| `taichi/ir/frontend_ir.cpp:1358`, `:1362`, `:1392` | mesh expressions |
| `taichi/transforms/lower_ast.cpp:330` | `AllocaStmt(PrimitiveType::i32)` — induction variable of a range-for containing a `break`, lowered to a while loop |
| `taichi/transforms/lower_ast.cpp:168`, `:348`, `:361` | **Amendment pass, relabelled in revision 4. None of the three is an index.** `:168` and `:348` are while-loop mask allocas — `:169` and `:349` are each `new_while->mask = mask.get();`. `:361` is neither: an anonymous `AllocaStmt(PrimitiveType::i32)` moved straight into `insert_before_me` at `:360-361` with nothing bound to it, so its result is never captured; the mask in that block is the `:348` alloca, moved in separately at `:365` and stored to at `:367-368`. Revision 3 called all three "loop-mask allocas", which was wrong for `:361`. With `:330`, the one index, these are all four `PrimitiveType::i32` tokens in the file |
| `taichi/transforms/auto_diff.cpp:587`, `:646`, `:787`, `:846`, `:1857` | **Amendment pass.** Matrix element indices built as `insert_const(PrimitiveType::i32, ...)`, and at `:1857` `insert_const_for_grad(PrimitiveType::i32, ...)` |
| `taichi/transforms/auto_diff.cpp:639`, `:839`, `:1828` | **Amendment pass.** Index `TensorType`s built as `TypeFactory::get_instance().get_tensor_type(tensor_shape, PrimitiveType::i32)` (calls open at `:638`, `:838`, `:1827`) |
| `taichi/ir/type_factory.cpp:204` | **ndarray descriptor shape members**, see 3.7 |
| `taichi/transforms/auto_diff.cpp:1819` | **Closing pass.** `indices_values[i] = insert<ConstStmt>(TypedConstant((int32)i));`, in the loop opened at `:1818` over `num_elements`. The vector becomes the `MatrixInitStmt` at `:1829`, whose `ret_type` is set at `:1830` to the `index_tensor_type` built at `:1827-1828`. A matrix element index, i32 by the `TypedConstant(int32)` constructor at `taichi/ir/type.h:582`. Revision 3 adopted an eight-site group from a round-two adversary; the file's own sweep returns twelve lines, nine index constructions and the three `val_int32()` reads of section 3.2, so the group was short by this one |
| `taichi/transforms/lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477` | **Closing pass.** Four identical `auto const_stmt = std::make_unique<ConstStmt>(TypedConstant(i));`, each in a loop over `num_elements` and each fed as the offset of the `MatrixPtrStmt` built two lines below at `:191-192`, `:234-235`, `:426-427`, `:479-480`. Matrix element indices, i32 by the same constructor. Named by both round-three adversaries and by nobody before them |
| `taichi/transforms/scalarize.cpp:63`, `:114`, `:530` | **Closing pass.** `TypedConstant(get_data_type<int32>(), i)`, each fed as the offset of the `MatrixPtrStmt` built below at `:66-67` and `:116-117`. The `:530` one takes one of two branches: at `:543-544` it is folded into a `BinaryOpStmt` add against an existing offset before the `MatrixPtrStmt` at `:546-547`, or it is the offset directly at `:553-554`. Same class as the `lower_matrix_ptr.cpp` group in a fourth spelling. Found by the sweep in 3.13, named by nobody |
| `taichi/transforms/scalarize.cpp:471`, `:473` | **Closing pass.** `ConstStmt(TypedConstant(PrimitiveType::i32, 0))` and `(…, 1)`, consumed at `:475-478` as the offsets of the two `MatrixPtrStmt`s in the f16 atomic split. Bounded at two elements, so the practical consequence is nil; inventoried because listing this file's six abort sites in 3.2 while omitting its index constructions is the same asymmetry the `auto_diff.cpp` rows were added to close |
| `taichi/ir/frontend_ir.cpp:1160` | **Closing pass. The material one.** `SNodeOpExpression::flatten`, `append` branch: `auto alloca = ctx->push_back<AllocaStmt>(PrimitiveType::i32, dbg_info);`. The alloca is passed as the `val` operand of the `SNodeOpStmt(SNodeOpType::allocate, …)` at `:1161-1162` and is the expression's value via the `LocalLoadStmt` at `:1169`, so it receives the index of the newly appended element of a dynamic SNode. **It has a device-side counterpart at the same width:** `taichi/codegen/llvm/codegen_llvm.cpp:1367-1375` passes it into the runtime call at `:1374`, and the runtime signature is `Ptr Dynamic_allocate(Ptr meta_, Ptr node_, i32 *len)` at `taichi/runtime/llvm/runtime_module/node_dynamic.h:61`, which writes the index through it at `:66`. Same class as the two birth sites below, and the same host/device mirror property as `PhysicalCoordinates` (escalation 3). This report already carried `type_check.cpp:114`, this site's downstream half, and never had the frontend half |
| `taichi/transforms/make_mesh_thread_local.cpp:42` | **Closing pass.** `ConstStmt(TypedConstant(PrimitiveType::i32, 1))`, added at `:43-44` to the `MeshPatchIndexStmt` created at `:39-40` whose own ctor is the i32 pin at `statements.h:2083` already in this table. Mesh patch-index arithmetic. Found by the sweep in 3.13, named by nobody |
| `taichi/transforms/check_out_of_bound.cpp:220` | **Closing pass. Not an index.** `compare->ret_type = PrimitiveType::i32;` on the `cmp_ge` `BinaryOpStmt` built at `:218-219` for the negative-exponent assertion on integer `pow`. A comparison result pinned i32, inside this territory and under this heading. Found by the sweep in 3.13, named by nobody |
| `taichi/transforms/lower_ast.cpp:151`, `:177`, `:333`, `:363` | **Closing pass.** The file's four `TypedConstant((int32)…)` tokens, invisible to the `PrimitiveType::i32` sweep that revision 3 used to claim the file closed. `:151` is the break condition in `visit(FrontendBreakStmt *)`, consumed as the `cond` operand of the `WhileControlStmt` at `:152` (`statements.h:69-79`, `WhileControlStmt(Stmt *mask, Stmt *cond)`); `:177` and `:363` are the `0xFFFFFFFF` mask initialisers stored at `:181-182` and `:367-368`. **`:333` is the index one:** `const_one`, used at `:334-335` to form `begin - 1` and at `:341-342` to form `i + 1` for the induction variable allocated at `:330`. Only `:333` is an index-width site |
| `taichi/ir/ir_builder.cpp:125` | **Closing pass. Not an index.** `IRBuilder::create_break()` builds its `WhileControlStmt` condition as `get_int32(0)`, hardcoded where the same builder offers `get_int64`/`get_uint64` at `ir_builder.h:134`, `:136`. Same class as `lower_ast.cpp:151`. Found by the sweep in 3.13, named by nobody |

`frontend_ir.cpp:151-152` and `lower_ast.cpp:330` are *birth* sites. A loop
induction variable never reaches the casts at `type_check.cpp:154`/`:461` as an
index — it is already i32 when it gets there, and the cast is a no-op. In a
struct-for driven engine every index is loop-derived, so changing
`type_check.cpp` alone widens nothing.

Two qualifications on the rows added in the amendment pass. The eight
`auto_diff.cpp` sites are bounded by matrix element count, so their practical
consequence is small; they are inventoried because listing that file's twelve
abort sites (section 3.2) while omitting its index constructions is an asymmetry
a reader will trip on. And `type_check.cpp:170-171` is the pinning half of the
mechanism section 7.1 item 1 describes: `mark_as_if_const`
(`taichi/transforms/type_check.cpp:40-44`) writes `Stmt::ret_type` only, leaving
`TypedConstant::dt` untouched, which is why a wide range-for bound aborts at
`taichi/transforms/offload.cpp:109`/`:117` rather than truncating.

### 3.4 Range-for iteration space, re-synthesised in i32 after type checking

`taichi/transforms/make_cpu_multithreaded_range_for.cpp`, run at
`taichi/transforms/compile_to_offloads.cpp:199`.

**Gate corrected in revision 3.** Revision 2 said the pass runs "whenever the
arch is CPU". It does not. The gate at
`taichi/transforms/compile_to_offloads.cpp:198` is

```
198:   if (config.make_cpu_multithreading_loop && arch_is_cpu(config.arch)) {
```

`make_cpu_multithreading_loop` is a plain `bool` on `CompileConfig`
(`taichi/program/compile_config.h:44`), defaulted to `true` at
`taichi/program/compile_config.cpp:48`, and exposed read-write to Python at
`taichi/python/export_lang.cpp:224-225`. So **the pass is switchable, and is not
an unconditional property of the CPU path**. That bears directly on escalation 4,
where I ask the planner to rule on whether the CPU path is in scope: the i32
re-synthesis below is on by default but avoidable by configuration, not only by
changing the pass. `CompileConfig` and the Python binding are outside my
territory; I state the gate and go no further.

| Site | Content |
|---|---|
| `:68` | `TypedConstant(PrimitiveType::i32, 1)` |
| `:70` | `TypedConstant(PrimitiveType::i32, 512)` |
| `:72` | `TypedConstant(PrimitiveType::i32, config.cpu_max_num_threads)` |
| `:81` | `TypedConstant(PrimitiveType::i32, offloaded->begin_value)` |
| `:84` | `GlobalTemporaryStmt(offloaded->begin_offset, PrimitiveType::i32)` |
| `:90` | `TypedConstant(PrimitiveType::i32, offloaded->end_value)` |
| `:93` | `GlobalTemporaryStmt(offloaded->end_offset, PrimitiveType::i32)` |

`:84` and `:93` are the sharpest. A *dynamic* range bound, which
`taichi/transforms/offload.cpp:508` and `:522` deliberately routed through a
`std::size_t` global-temporary offset (`taichi/ir/statements.h:1411-1412`), is
read back out of that buffer **reinterpreted as i32**. The 64-bit-clean offset
path is closed again here.

`taichi/transforms/demote_no_access_mesh_fors.cpp:31-34` is a fourth writer of
`begin_value`/`end_value`.

### 3.5 32-bit fields on IR statements and SNode geometry

| Site | Field |
|---|---|
| `taichi/ir/statements.h:1280` | `LinearizeStmt::strides` — `std::vector<int>` |
| `taichi/ir/statements.h:1415-1416` | `OffloadedStmt::begin_value`, `end_value` — `int32` |
| `taichi/ir/statements.h:1439` | `OffloadedStmt::index_offsets` — `std::vector<int>` |
| `taichi/ir/statements.h:376`, `:379` | `ExternalPtrStmt::ndim`, `element_shape` |
| `taichi/ir/statements.h:456` | `MatrixOfGlobalPtrStmt::dynamic_index_stride` — `int` |
| `taichi/ir/statements.h:1370` | `GetChStmt::chid` — `int` |
| `taichi/ir/snode.h:41`, `:45`, `:49` | `AxisExtractor::num_elements_from_root`, `shape`, `acc_shape` |
| `taichi/ir/snode.h:79` | `SNode::index_offsets` — `std::vector<int>` |
| `taichi/ir/snode.h:81` | `SNode::physical_index_position` — `int[12]` |
| `taichi/ir/snode.h:98` | `SNode::chunk_size` — `int` |
| `taichi/ir/snode.h:317` | `shape_along_axis(int i)` returns `int` |
| `taichi/ir/type.h:222-227` | `TensorType::get_num_elements()` — `int` accumulator |
| `taichi/ir/type.h:307-321` | `StructType::get_flattened_num_elements()` — `int` |
| `taichi/ir/type_utils.cpp:30` | **`data_type_size` returns `int`**, and `:47-48` computes a `TensorType`'s size in `int` |
| `taichi/ir/frontend_ir.h:529` | `ExternalTensorExpression::ndim` |
| `taichi/ir/frontend_ir.h:615`, `:617` | `MatrixFieldExpression::element_shape`, `dynamic_index_stride` |
| `taichi/ir/frontend_ir.h:676` | `IndexExpression::ret_shape` |
| `taichi/ir/frontend_ir.h:712` | `RangeAssumptionExpression::low`, `high` |
| `taichi/ir/analysis.h:20-21` | `DiffRange::coeff`, `low`, `high` |
| `taichi/ir/analysis.h:185` | `DiffPtrResult::diff_range` |
| `taichi/ir/scratch_pad.h:33-42` | `BoundRange::low`, `high` |
| `taichi/ir/scratch_pad.h:47-55` | `coefficients`, `pad_size`, `block_size`, `dim` |
| `taichi/analysis/bls_analyzer.h:15-18` | `IndexRange::low`, `high` |

### 3.6 32-bit arithmetic that overflows before any IR type is involved

These are host-side compile-time `int` computations. They cannot be fixed by
changing an IR type.

**Corrected in revision 3.** Revision 2 listed
`taichi/transforms/lower_ast.cpp:270-274` in this table. That was a
categorisation error, caught independently by both adversaries and confirmed
against source: `:270` and `:271` push `ConstStmt`s and `:273` pushes a
`BinaryOpStmt`, so the product is built out of IR statements and overflows in a
register at run time. It is exactly the class of thing a width change *does*
fix, and this report's own section 4 already treated it that way. It has moved
to section 3.12, along with a second IR-level product the adversaries found
missing. Every remaining row below was re-opened and is genuinely a host `int`.

| Site | Expression | What actually overflows |
|---|---|---|
| `taichi/transforms/scalar_pointer_lowerer.cpp:33-39` | `std::array<int, 12> total_shape;` `total_shape[j] *= s->extractors[j].shape;` | **one axis extent**, not the cell count — the array is indexed by axis `j` |
| `taichi/transforms/demote_dense_struct_fors.cpp:19-24` | same accumulator | same — one axis extent |
| `taichi/transforms/demote_dense_struct_fors.cpp:74` | `generate_mod(..., ext.acc_shape * ext.shape)` | 32-bit product formed before the call |
| `taichi/transforms/simplify.cpp:176`, `:187` | `auto stride_product = 1;` (deduced `int`), `stride_product *= stmt->strides[i];` | the per-level stride product during `LinearizeStmt` expansion |
| `taichi/transforms/check_out_of_bound.cpp:54-57` | `int flattened_element` product over `element_shape` | element count |
| `taichi/transforms/check_out_of_bound.cpp:169-173` | `int max_valid_index` product over the tensor shape | element count |
| `taichi/transforms/handle_external_ptr_boundary.cpp:62-67` | same product | element count |
| `taichi/ir/frontend_ir.cpp:742-747` | `int offset = 0; offset = offset * shape[i] + ...` | folded tensor offset |
| `taichi/ir/scratch_pad.h:96-100`, `:131-138`, `:140-147`, `:149-157` | `int` products and `linearized_index` | block-local, small in practice |

Correction from revision 1: I previously described the `total_shape` arrays as
overflowing on a large *cell count*. They do not. They are indexed by axis and
cap a **single axis extent** at 2^31. The accumulator is real; the failure mode
I attributed to it was wrong.

### 3.7 Widths hardcoded rather than derived

| Site | Hardcoded value |
|---|---|
| `taichi/transforms/simplify.cpp:238-239` | `previous_offset->offset * sizeof(int32) * snode->ch.size()` — SNode child element size assumed 4 bytes |
| `taichi/transforms/simplify.cpp:260-261` | `stmt->chid * sizeof(int32) + previous_offset->offset` — child stride assumed 4 bytes |
| `taichi/ir/type_factory.cpp:204` | `shape_members.push_back({PrimitiveType::i32, fmt::format("dim_{}", i)});` |
| `taichi/transforms/make_mesh_block_local.cpp:526` | **Amendment pass.** `mapping_data_type_ = PrimitiveType::i32;`, with the derived form commented out immediately above at `:525` — `// mapping_data_type_ = mapping_snode_->dt.ptr_removed();`. `:527` then derives `mapping_dtype_size_` from the hardcoded type via `data_type_size`. The clearest instance in the territory of a derived width that was deliberately replaced by a literal one, and the comment records that the derived form once existed. Mesh, so escalation 1 governs whether it is in scope |
| `taichi/transforms/utils.cpp:5`, `:14` | `generate_mod(VecStatement*, Stmt *x, int y)` and `generate_div(...)` take **`int y`**, so a stride above 2^31 cannot be expressed at all, independent of any IR type. `:17` emits an explicit `TypedConstant(PrimitiveType::i32, ...)`. Called from `scalar_pointer_lowerer.cpp:73`, `:75` and `demote_dense_struct_fors.cpp:60`, `:63`, `:74`, `:76` — the most-used index-arithmetic helper in the territory |

`type_factory.cpp:204` is not merely a literal: `get_ndarray_struct_type`
(`:198-215`) builds the **host-to-kernel ABI descriptor**, so the i32
per-dimension shape crosses the process boundary.

`taichi/transforms/scalarize.cpp:1121-1122` and `:1148-1149` do the analogous
fold using `data_type_size(...)` — derived. The machinery exists;
`simplify.cpp` does not use it. Note however that `data_type_size` is itself
32-bit (section 3.5), so it is the right *shape* of answer, not a wide one.

### 3.8 Byte offsets: 64-bit clean everywhere except two places

The pattern across the territory is that **byte offsets are already `size_t` and
index/shape/stride values are not**:
`taichi/ir/statements.h:1260` (`int64`), `:1411-1412`, `:1597`, `:1618`, `:1770`
(`std::size_t`); `taichi/ir/snode.h:97` (`int64`), `:99-100` (`size_t`),
`:306-308`; `taichi/ir/type.h:243`, `:257`, `:305`, `:336`.

Two exceptions, both in my territory:

- `taichi/transforms/lower_matrix_ptr.cpp:536-540` — the **dynamic matrix-field
  byte offset** is `stmt->offset * ConstStmt(origin->dynamic_index_stride)` with
  `offset->ret_type = stmt->offset->ret_type` forced at `:540`, which
  `type_check.cpp:160-161` has already asserted is i32. A byte offset computed
  in 32 bits while every other byte offset in the IR is `size_t`.
- `taichi/transforms/make_block_local.cpp:157`, `:334` and
  `taichi/transforms/make_mesh_block_local.cpp:80`, `:118`, `:200`, `:265`,
  `:301`, `:575` — eight explicit narrowings of a `std::size_t` byte offset into
  an i32 `TypedConstant`. Three of the mesh sites use the functional spelling
  `int32(offset_in_bytes)` rather than the cast spelling, which is why revision 1
  found only three of the six.

### 3.9 Other explicit narrowing casts

| Site | Cast |
|---|---|
| `taichi/ir/snode.cpp:92` | `extractors[i].acc_shape = static_cast<int>(acc_shape);` from `int64`. Commented `// casting to int32 in extractors.` **Silent in every build.** `:101` stores the untruncated value into `num_cells_per_container`, so the two disagree above 2^31 |
| `taichi/ir/snode.h:313` | `total_num_elemts *= (int)s->max_num_elements();` — narrowing cast inside an `int64` accumulation. **Expanded in revision 3**, see below |
| `taichi/transforms/demote_dense_struct_fors.cpp:35` | `offloaded->end_value = total_n;` — `int64` into `int32`, guarded only by `:29` |
| `taichi/transforms/demote_dense_struct_fors.cpp:60`, `:63` | `int64 total_n` into `generate_mod`/`generate_div`'s `int y` |
| `taichi/analysis/value_diff.cpp:71-76` | `val_int()` (`int64`) into `DiffRange`'s `int` fields |

**`taichi/ir/snode.h:310-315` — `SNode::get_total_num_elements_towards_root`.
A fourth whole-structure flattening site, with no callers anywhere in the tree.**
Recorded on the planner's ruling, and stated with the no-caller fact up front so
that nobody mistakes it for a live defect.

```
310:   int64 get_total_num_elements_towards_root() const {
311:     int64 total_num_elemts = 1;
312:     for (auto *s = this; s != nullptr; s = s->parent)
313:       total_num_elemts *= (int)s->max_num_elements();
314:     return total_num_elemts;
315:   }
```

The accumulator is `int64` and the walk is root-to-leaf, but every factor is
narrowed to `int` at `:313` before it is multiplied in, so the `int64` type of
the accumulator buys nothing above 2^31 in any single container. It is the same
shape of quantity as the three live sites in section 4 and it has no guard, no
assert and no warning.

`grep -rn "get_total_num_elements_towards_root"` over the whole repository —
`.cpp`, `.h`, `.cc`, `.mm` and `.py`, including `tests/` and the Python bindings
— returns exactly one line, the definition itself at `snode.h:310`. **It cannot
fail today because nothing reaches it.** It is inventoried as latent rather than
dropped, because standing instruction 3 forbids deciding that something is
unnecessary, and because plan section 4.6 says the engine SNode configuration is
composed from the larger data stores for working scope, so new callers are
foreseeable. It is not counted among the three formation sites in section 4,
which are the reached ones.

### 3.10 Analysis passes that recognise only i32 constants

Not truncations. They stop working, which costs optimisation.

| Site | Behaviour |
|---|---|
| `taichi/analysis/value_diff.cpp:81-87` | `ValueDiffLoopIndex::visit(ConstStmt*)` — gates on `is_primitive(PrimitiveTypeID::i32)` at `:82`, reads `val_i32` at `:83`, returns an unrelated `DiffRange()` otherwise |
| `taichi/analysis/value_diff.cpp:142-146` | `FindDirectValueBaseAndOffset::visit(ConstStmt*)` — same gate at `:143`, same read at `:144`, no decomposition otherwise |
| `taichi/transforms/bit_loop_vectorize.cpp:311-325` | `get_constant_value` gates on i32 at `:321`, returns `-1` otherwise. Used at `:180`, `:202` |

Revision 1 named one `value_diff` visitor, gave no line numbers, and attributed
a consumer to the wrong one. There are two independent visitors feeding
different consumers. The consumer set is six:
`taichi/analysis/alias_analysis.cpp:59`, `:142`, `:178`;
`taichi/transforms/lower_access.cpp:237`; `taichi/analysis/bls_analyzer.cpp:50`;
`taichi/transforms/bit_loop_vectorize.cpp:54`.

If index constants become i64, both visitors stop recognising them, index
comparisons degrade to `AliasResult::uncertain`, and the optimisations built on
that stop firing. A performance regression, not a correctness one — but plan 6.4
makes packing density and dispatch throughput the sole criterion.

### 3.11 Bounds checks and print formats

- `taichi/transforms/check_out_of_bound.cpp:123-126` — the per-axis upper bound
  compared against **every SNode field index** is `int size_i =
  snode->shape_along_axis(i);` materialised as an i32 `ConstStmt`. Revision 1
  listed the element-count *products* but not the bound actually compared.
- `taichi/transforms/check_out_of_bound.cpp:91`, `:153`, `:201` — assert message
  formats use `"%d"` per index.
  `taichi/transforms/make_block_local.cpp:292-296` likewise for BLS bounds (the `%d` is at `:295`).
- `taichi/ir/type_utils.cpp:113-137` already has correct formats for i64
  (`"%lld"` at `:134`) and u64 (`:137`), with comments at `:133` — *"Vulkan does
  not support printing 64-bit signed integer"* — and `:136`.

### 3.12 Products built as i32 IR statements, not as host `int`

New in revision 3. These are the opposite case to section 3.6: the accumulator
is a chain of `ConstStmt` and `BinaryOpStmt`, so the overflow happens in a
register at run time and a width change is exactly what fixes it. Both were
misplaced or missing in revision 2.

| Site | What is built | Guard |
|---|---|---|
| `taichi/transforms/lower_ast.cpp:270-274` | external-array struct-for extent. `:270` `ConstStmt(TypedConstant(0))`, `:271` `ConstStmt(TypedConstant(1))`, `:273` `BinaryOpStmt(mul, end, shape[i])` per axis over the `ExternalTensorShapeAlongAxisStmt` values pushed at `:258-259`/`:265-266` | **none. No assert, no warning** |
| `taichi/ir/frontend_ir.cpp:731-740` | dynamic tensor element offset. `:732` `ConstStmt(TypedConstant(0))`, `:735` `ConstStmt(TypedConstant(shape[i]))`, `:736-739` mul then add per axis | **none** |

Both are i32 by the `TypedConstant(int32)` constructor at
`taichi/ir/type.h:582`, and in the `lower_ast` case additionally by
`type_check.cpp:119`, which pins `ExternalTensorShapeAlongAxisStmt` to i32.

`frontend_ir.cpp:731-740` is the **dynamic** branch of the same tensor-offset
computation whose constant-folded branch is at `:742-747`. Revision 2 carried
only the folded branch, in section 3.6, where it correctly belongs — `:742` is
`int offset = 0;`, a host `int`. The dynamic branch is the one that emits IR,
and it was raised in round one by `adversary-01-1.md` and left unanswered by
revision 2. Recorded here as an accepted objection, not a rebutted one.

Bounded by tensor element count in the `frontend_ir` case, so its practical
consequence is small; it is inventoried because the two branches of one
computation should not be split across two categories.


### 3.13 The exhaustive `PrimitiveType::i32` sweep — all sixty-four lines

New in revision 4. `adversary3-01-2.md` charged that neither report had run this
sweep to the end, and that six in-territory sites fall through as a result. I ran
it myself rather than adopting the charge:

```
grep -rn "PrimitiveType::i32" taichi/ir/ taichi/analysis/ taichi/transforms/
```

**It returns exactly sixty-four lines.** Every one is enumerated below with a
disposition, so the completeness claim can be checked against the command rather
than trusted. Fifty-two were already inventoried, five are added to the inventory
in this revision, four are swept and excluded for a stated reason, and three are
recorded but deliberately not inventoried by planner ruling (section 3.14).
52 + 5 + 4 + 3 = 64, counted from the table rather than asserted over it.

| Site | Disposition | Where / why |
|---|---|---|
| `taichi/ir/type_utils.h:40` | swept and excluded | `get_data_type<T>()` template mapping, the C++ `int32` arm. The correct mapping, not an assumption |
| `taichi/ir/statements.h:1684` | inventoried | section 3.3 |
| `taichi/ir/statements.h:2064` | inventoried | section 3.3 |
| `taichi/ir/statements.h:2083` | inventoried | section 3.3 |
| `taichi/ir/type_factory.cpp:204` | inventoried | section 3.3 and 3.7 |
| `taichi/ir/expr.cpp:93` | swept and excluded | `Expr::Expr(int32 x)`, one overload of a pair whose i64 sibling is at `:96-98`. Struck in revision 1, section 7.1 item 5 |
| `taichi/ir/type.h:582` | swept and excluded | the `TypedConstant(int32)` constructor itself. The mechanism cited by 3.3 and 3.12, not a site |
| `taichi/ir/frontend_ir.cpp:152` | inventoried | section 3.3 |
| `taichi/ir/frontend_ir.cpp:1114` | inventoried | section 3.3 |
| `taichi/ir/frontend_ir.cpp:1160` | **added, revision 4** | dynamic-SNode append index |
| `taichi/ir/frontend_ir.cpp:1216` | **recorded, out of scope** | texture coordinate hard-required i32, `kFetchTexel` branch. Planner ruling, section 3.14 |
| `taichi/ir/frontend_ir.cpp:1233` | **recorded, out of scope** | texture coordinate hard-required i32, `kLoad` branch. Planner ruling, section 3.14 |
| `taichi/ir/frontend_ir.cpp:1249` | **recorded, out of scope** | texture coordinate hard-required i32, `kStore` branch. Planner ruling, section 3.14 |
| `taichi/ir/frontend_ir.cpp:1306` | inventoried | section 3.3 |
| `taichi/ir/frontend_ir.cpp:1358` | inventoried | section 3.3 |
| `taichi/ir/frontend_ir.cpp:1362` | inventoried | section 3.3 |
| `taichi/ir/frontend_ir.cpp:1392` | inventoried | section 3.3 |
| `taichi/transforms/utils.cpp:17` | inventoried | section 3.7 |
| `taichi/transforms/make_mesh_thread_local.cpp:42` | **added, revision 4** | mesh patch-index addend |
| `taichi/transforms/make_mesh_thread_local.cpp:106` | inventoried | section 3.1 |
| `taichi/transforms/make_mesh_thread_local.cpp:114` | inventoried | section 3.1 |
| `taichi/ir/statements.cpp:146` | inventoried | section 3.3 |
| `taichi/transforms/type_check.cpp:114` | inventoried | section 3.3 |
| `taichi/transforms/type_check.cpp:119` | inventoried | section 3.3 |
| `taichi/transforms/type_check.cpp:154` | inventoried | section 3.1 |
| `taichi/transforms/type_check.cpp:170` | inventoried | section 3.3 |
| `taichi/transforms/type_check.cpp:171` | inventoried | section 3.3 |
| `taichi/transforms/type_check.cpp:459` | swept and excluded | the `!=` guard condition for the cast emitted at `:461`. Carried by that row |
| `taichi/transforms/type_check.cpp:461` | inventoried | section 3.1 |
| `taichi/transforms/type_check.cpp:467` | inventoried | section 3.3 |
| `taichi/transforms/type_check.cpp:471` | inventoried | section 3.3 |
| `taichi/transforms/type_check.cpp:475` | inventoried | section 3.3 |
| `taichi/transforms/type_check.cpp:521` | inventoried | section 3.3 |
| `taichi/transforms/type_check.cpp:525` | inventoried | section 3.3 |
| `taichi/transforms/type_check.cpp:567` | inventoried | section 3.3 |
| `taichi/transforms/demote_mesh_statements.cpp:16` | inventoried | section 3.1 |
| `taichi/transforms/demote_mesh_statements.cpp:54` | inventoried | section 3.1 |
| `taichi/transforms/demote_mesh_statements.cpp:131` | inventoried | section 3.1 |
| `taichi/transforms/make_mesh_block_local.cpp:212` | inventoried | section 3.1 |
| `taichi/transforms/make_mesh_block_local.cpp:376` | inventoried | section 3.1 |
| `taichi/transforms/make_mesh_block_local.cpp:399` | inventoried | section 3.1 |
| `taichi/transforms/make_mesh_block_local.cpp:526` | inventoried | section 3.7 |
| `taichi/transforms/check_out_of_bound.cpp:220` | **added, revision 4** | cmp_ge result type |
| `taichi/transforms/auto_diff.cpp:587` | inventoried | section 3.3 |
| `taichi/transforms/auto_diff.cpp:639` | inventoried | section 3.3 |
| `taichi/transforms/auto_diff.cpp:646` | inventoried | section 3.3 |
| `taichi/transforms/auto_diff.cpp:787` | inventoried | section 3.3 |
| `taichi/transforms/auto_diff.cpp:839` | inventoried | section 3.3 |
| `taichi/transforms/auto_diff.cpp:846` | inventoried | section 3.3 |
| `taichi/transforms/auto_diff.cpp:1828` | inventoried | section 3.3 |
| `taichi/transforms/auto_diff.cpp:1857` | inventoried | section 3.3 |
| `taichi/transforms/lower_ast.cpp:168` | inventoried | section 3.3 |
| `taichi/transforms/lower_ast.cpp:330` | inventoried | section 3.3 |
| `taichi/transforms/lower_ast.cpp:348` | inventoried | section 3.3 |
| `taichi/transforms/lower_ast.cpp:361` | inventoried | section 3.3 |
| `taichi/transforms/scalarize.cpp:471` | **added, revision 4** | MatrixPtrStmt offset constant |
| `taichi/transforms/scalarize.cpp:473` | **added, revision 4** | MatrixPtrStmt offset constant |
| `taichi/transforms/make_cpu_multithreaded_range_for.cpp:68` | inventoried | section 3.4 |
| `taichi/transforms/make_cpu_multithreaded_range_for.cpp:70` | inventoried | section 3.4 |
| `taichi/transforms/make_cpu_multithreaded_range_for.cpp:72` | inventoried | section 3.4 |
| `taichi/transforms/make_cpu_multithreaded_range_for.cpp:81` | inventoried | section 3.4 |
| `taichi/transforms/make_cpu_multithreaded_range_for.cpp:84` | inventoried | section 3.4 |
| `taichi/transforms/make_cpu_multithreaded_range_for.cpp:90` | inventoried | section 3.4 |
| `taichi/transforms/make_cpu_multithreaded_range_for.cpp:93` | inventoried | section 3.4 |

**Reconciliation with section 3.3.** Five of these sixty-four enter section 3.3,
but section 3.3 gains eighteen sites. The difference is the thirteen sites that
this sweep **cannot** reach, and that is the more important result of running it.
The three texture sites are the fourth category: found by the sweep, on the page,
and out of scope by ruling rather than by my judgement (section 3.14).

**The sweep is per-spelling, and `PrimitiveType::i32` is only one of four
spellings in this territory.** An i32 constant can be built without the token
appearing at all, because `TypedConstant` has an implicit `int32` constructor at
`taichi/ir/type.h:582`. Three companion sweeps, each run to exhaustion:

| Command | Lines | What it adds that the first misses |
|---|---|---|
| `grep -rn "TypedConstant((int32)\|TypedConstant{(int32)\|int32(" taichi/ir/ taichi/analysis/ taichi/transforms/` | 37 | `auto_diff.cpp:1819`; `lower_ast.cpp:151`, `:177`, `:333`, `:363`. The remaining 32 are the `val_int32()` sites of section 3.2 and the byte-offset narrowings of section 3.8, all already inventoried |
| `grep -rn "get_data_type<int32>()" taichi/ir/ taichi/analysis/ taichi/transforms/` | 3 | `scalarize.cpp:63`, `:114`, `:530`, all three of them matrix element indices. The only other hits tree-wide are the two asserts inside `TypedConstant::val_int32`/`val_int64` at `taichi/ir/type.cpp:463`, `:478` |
| `grep -rn "TypedConstant(" taichi/ir/ taichi/analysis/ taichi/transforms/`, minus every call whose first argument is a type | 78 candidate lines in 16 files | `lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477` — and a much larger residue |

**That last row is a gap I am not closing, and I state it rather than paper over
it.** The filter is `TypedConstant(` in territory with the explicitly-typed forms
removed — those whose first argument is `dt`, `dst_type`, `data_type`,
`load_data_type`, `PrimitiveType::…`, a `ret_type`, `get_data_type<int32>()`, or
`…get_element_type()`. It leaves **78 candidate lines across 16 files**, of which
the four `lower_matrix_ptr.cpp` sites are four. The rest are unclassified. Some
are certainly not sites at all: `constant_fold.cpp` contributes five lines that
are macro continuations whose type argument sits on the previous line, and
`ir_builder.cpp` contributes six of the same shape. Others plainly are sites of
the same class — `demote_operations.cpp:99` is `ConstStmt(TypedConstant(i))`
inside a loop over matrix elements, the identical form to
`lower_matrix_ptr.cpp:189`.

**Seventy-eight is a candidate count, not a site count, and I have not derived a
site count from it.** Doing so means opening and classifying every one, which is
a fresh enumeration of a class nobody has scoped, and standing instruction 2
says stop and report rather than improvise. Escalated (section 8, item 16). The
planner has since taken the class off this report and dispatched a dedicated
pair to it, with the instruction that I must not partially classify the residual
in the meantime, because a half-closed enumeration reads as finished.

**Where the boundary falls, stated precisely for whoever picks the class up.**
Three of the four spellings above are closed and can be trusted as closed. The
fourth is untouched apart from five lines, and those five were named by others
before the class was identified, not selected by me out of the residue:

| | Status |
|---|---|
| `PrimitiveType::i32`, 64 lines | **Closed.** Every line dispositioned in the table above |
| `(int32)` / `int32(`, 37 lines | **Closed.** Every line either already inventoried or added in this revision |
| `get_data_type<int32>()`, 3 lines | **Closed.** All three added in this revision |
| implicit `TypedConstant(<host int>)`, 78 candidates | **Open, and deliberately so.** Five of the 78 are accounted for: `lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477`, added because both round-three adversaries named them and the planner directed it, and `demote_operations.cpp:99`, which is quoted below as a worked example and is **not** inventoried. The other 73 are unopened and unclassified |

`demote_operations.cpp:99` is the one line of the residue I opened, and I opened
it to test whether the residue contains real sites rather than to begin
classifying it. It does. It is cited as evidence in escalation 16 and appears in
no inventory table in this report. Nobody should read it as the start of a pass.

**What this means for the word "complete" in this report's section 3 heading.**
The inventory is complete on the explicitly-typed spellings: all sixty-four
`PrimitiveType::i32` lines are dispositioned above, all thirty-seven
`(int32)`/`int32(` lines and all three `get_data_type<int32>()` lines are
accounted for. It is **not** a complete enumeration of the implicitly-typed
`TypedConstant(<host int>)` class, and no revision of this report ever was.
Revision 3 said "complete" without that qualification and revision 4 states the
limit. The heading is amended accordingly.


### 3.14 Found, recorded, and ruled out of scope — the texture coordinate checks

New in revision 4. These three sites are real, they are inside this territory,
and they are **not** in the section 3.3 inventory and not counted anywhere in
this report. They are on the page so that a later reader can see that they were
found and why they were excluded, rather than wondering whether the sweep missed
them.

| Site | Content |
|---|---|
| `taichi/ir/frontend_ir.cpp:1216` | `TextureOpExpression::type_check`, `kFetchTexel` branch |
| `taichi/ir/frontend_ir.cpp:1233` | same, `kLoad` branch |
| `taichi/ir/frontend_ir.cpp:1249` | same, `kStore` branch |

Each loops over `ptr->num_dims` and raises
`ErrorEmitter(TaichiTypeError(), …)` unless `arg_type == PrimitiveType::i32`
exactly. They are not pins on a type — they are hard rejections of any other
width, which is a stronger form than anything else in section 3.3.

**Why they are excluded, and who decided.** Not me. Revision 4 inventoried them
and escalated the scope question, because standing instruction 3 forbids an
agent deciding that something is unnecessary. **The planner ruled them out of
scope**, on two grounds: plan section 1.3 puts rendering entirely out of scope,
and the project owner has since confirmed positively that the foreseen data
carries no graphic payload — particle attachments are field interaction data and
the modelling is purely mathematical. Texture operations are not work this
project will do. Escalation 15 is closed against that ruling.

**One thing the ruling does not touch.** These sites falsify an unqualified
claim this report carried from revision 1 to revision 3, and the claim needed
correcting whichever way the scope question went. Sections 1 and 6 said the
frontend does not restrict the type of an index, citing
`IndexExpression::type_check` checking only `is_integral`. That is true of field
and array indices. It is not true of every index the frontend accepts. Both
sections now carry the qualification. A site being out of scope for the *work*
does not make it out of scope as *evidence* against a claim about the tree.

---

## 4. Where a whole-structure extent is actually formed

Because addressing is hierarchical (section 1), a large structure does not by
itself produce a large value in the frontend IR. Three sites flatten a
whole-structure or whole-array extent, and they behave differently:

| Site | Accumulator | Guard |
|---|---|---|
| `taichi/transforms/demote_dense_struct_fors.cpp:26`, `:35` | `int64 total_n`, stored into an `int32` field | `TI_ASSERT` at `:29` — **aborts** |
| `taichi/transforms/lower_ast.cpp:270-274` | running i32 product of axis extents, built as IR (section 3.12) | **none** — silent overflow |
| `taichi/ir/snode.h:41` — `AxisExtractor::num_elements_from_root` | plain `int`, accumulated root-to-leaf | **none of any kind** — silent overflow |

**Third row corrected in revision 3.** Revision 2 put `taichi/ir/snode.cpp:89-101`
in this slot and called it a whole-structure extent. That was wrong, and this
report's own row text ("truncated per node") contradicted its own heading. Both
adversaries caught it. Verified against source:

- `SNode::insert_children` (`taichi/ir/snode.cpp:14-35`) propagates **only**
  `num_elements_from_root`, at `:22-23`. It never touches `shape` or `acc_shape`.
- `AxisExtractor::shape` and `::acc_shape` therefore start at their declared
  defaults of 1 (`taichi/ir/snode.h:45`, `:49`), and `snode.cpp:84` is a plain
  assignment, `new_node.extractors[ind].shape = sizes[i];` — this node's own axis
  size.
- So the `int64 acc_shape` accumulation at `:89-93` is the product of **one
  node's own** axis shapes, stored at `:101` into `num_cells_per_container`,
  which `taichi/ir/snode.h:93-97` documents as *"Product of the |shape| of all
  the activated axes identified by |extractors|"* with an explicit pointer to
  the cell/container terminology. `SNode::max_num_elements()` (`:306-308`)
  returns it unchanged.

That is a **per-container** extent — precisely the quantity section 1 says is
bounded by the level rather than by the structure. The `static_cast<int>`
truncation at `:92`, its disagreement with the untruncated `:101`, and the
`TaichiIndexWarning` at `:95-100` are all real and all stay in the inventory,
where section 3.9 and section 1 already carry them. What is withdrawn is only
the classification of that site as a whole-structure flattening.

**The site that belongs in the slot is `AxisExtractor::num_elements_from_root`,
and revision 2 had both halves of it without joining them.** Verified:

| Step | Site |
|---|---|
| Declaration, plain `int` | `taichi/ir/snode.h:41` — `int num_elements_from_root{1};` |
| Accumulates root-to-leaf | `taichi/ir/snode.cpp:22-23` — each child's per-axis value is multiplied by its parent's |
| Multiplied by this node's axis size | `taichi/ir/snode.cpp:83` — `new_node.extractors[ind].num_elements_from_root *= sizes[i];` |
| Returned as the per-axis extent | `taichi/ir/snode.cpp:174-177` — `shape_along_axis` returns exactly this field at `:176` |
| Materialised as the i32 bound on every field index | `taichi/transforms/check_out_of_bound.cpp:123-126` — `int size_i = snode->shape_along_axis(i);` at `:123`, `ConstStmt(TypedConstant(upper_bound_i))` at `:125-126` |

There is no assert, no warning and no `int64` shadow anywhere on that path.
Revision 2 cited `snode.h:317` as "`shape_along_axis(int i)` returns `int`" in
section 3.5, and cited `check_out_of_bound.cpp:123-126` as the bound actually
compared in section 3.11, and never joined them. Joined, they say that **the
bound checked against every SNode field index is an unguarded root-to-leaf `int`
product**, and it is the only one of the three formation sites with no guard of
any kind.

Three formation sites, three different failure modes: abort, silent overflow in
IR, and silent overflow in host `int`. Which of these the workload reaches
depends on the engine SNode configuration, which plan 8.1 item 3 records as
unsettled. Escalated (section 8, item 6).

Two qualifications on the count of three, neither of which changes it:

- `SNode::get_total_num_elements_towards_root` (`taichi/ir/snode.h:310-315`)
  also flattens a whole-structure extent, with an `(int)` narrowing per factor
  at `:313` inside an `int64` accumulator. It has **no callers tree-wide**, so
  it fails no way today. Recorded, not counted.
- The whole-structure cell count assembled *from*
  `num_elements_from_root` is formed at the codegen seam, outside this
  territory: `taichi/codegen/spirv/snode_struct_compiler.cpp:115-121`,
  `sn_desc.total_num_cells_from_root *= e.num_elements_from_root;`. That is what
  makes the territory-01 `int` field load-bearing rather than incidental.
  Recorded in section 7.4 for agent 02. Note that the in-tree comment at
  `snode_struct_compiler.cpp:118-120` says the extractors are also set by
  `StructCompiler::infer_snode_properties()`; that function does not exist
  anywhere in the tree, so the comment is stale and `taichi/ir/snode.cpp` is the
  only writer.

A fourth consideration: **`LinearizeStmt` has two independent implementations**
in this territory. `taichi/transforms/simplify.cpp:160-220` expands it into
adds and muls with an `int` stride accumulator;
`taichi/analysis/arithmetic_interpretor.cpp:98-109` evaluates it separately,
accumulating in `int64_t` at `:99`, `:106` and then storing under
`stmt->ret_type` at `:108`, which `type_check.cpp:521` pinned to i32. Both
codegen paths implement it a third and fourth time
(`taichi/codegen/llvm/codegen_llvm.cpp:1736-1744`,
`taichi/codegen/spirv/spirv_codegen.cpp:532-541`, the latter explicitly
`i32_type()`). Any width change has four implementations to keep in step.

`ArithmeticInterpretor` has no callers in `taichi/`. Its only consumers are
`tests/cpp/transforms/scalar_pointer_lowerer_test.cpp` and
`tests/cpp/transforms/make_block_local_test.cpp`, which assert the exact numeric
output of the index lowering that item 6.2 would change.

---

## 5. Pass ordering — the coercion is not a one-shot

**Corrected and completed in revision 3.** Revision 2 ran
`grep -n "irpass::type_check" taichi/transforms/compile_to_offloads.cpp` — a
grep scoped to one file — and reported seven calls plus three others found by
hand. `adversary2-01-2.md` charged that this understates the surface by more
than half. **It is right on the mechanism and right that the report undercounted,
but its own figure is wrong**: it wrote "at least fifteen further re-runs across
fourteen files" over a list that contains twenty-two. Fourteen files is correct.
I re-derived the whole set independently rather than adopting either number.

Two greps are needed, not one, because most in-pass re-runs call
`type_check(root, config)` **unqualified** from inside `namespace irpass` and are
invisible to a search for `irpass::type_check`:

```
grep -rn "irpass::type_check" taichi/
grep -rn "[^_a-zA-Z:>.]type_check(" taichi/ir/ taichi/analysis/ taichi/transforms/
```

**Seven in the compilation driver**, `taichi/transforms/compile_to_offloads.cpp`:
`:57`, `:66`, `:193`, `:200`, `:208`, `:310`, `:391`. Unchanged and re-verified.

**Twenty-two further invocation sites across fourteen files, all in territory.**

*Qualified `irpass::type_check`, five sites in four files:*

| Site | Context |
|---|---|
| `taichi/ir/ir.cpp:566` | `DelayedIRModifier::modify_ir` — the drain point for every queued re-check |
| `taichi/transforms/check_out_of_bound.cpp:248` | end of `run()`, if modified |
| `taichi/transforms/handle_external_ptr_boundary.cpp:100` | end of `run()`, if modified |
| `taichi/transforms/demote_operations.cpp:300`, `:303` | inside the fixed-point loop, and again after it |

*Unqualified `type_check(root, config)` from inside `namespace irpass`, fourteen
sites in **nine** files:*

| Site | Context |
|---|---|
| `taichi/transforms/auto_diff.cpp:2407`, `:2410`, `:2417`, `:2420`, `:2430` | five, around each independent-block rewrite and once at the end |
| `taichi/transforms/demote_atomics.cpp:241` | |
| `taichi/transforms/demote_mesh_statements.cpp:160` | mesh |
| `taichi/transforms/lower_access.cpp:294` | immediately after the `LinearizeStmt`s are emitted |
| `taichi/transforms/make_block_local.cpp:391` | |
| `taichi/transforms/make_mesh_block_local.cpp:681` | mesh |
| `taichi/transforms/make_mesh_thread_local.cpp:165` | mesh |
| `taichi/transforms/make_thread_local.cpp:229` | |
| `taichi/transforms/offload.cpp:766`, `:773` | |

*Queued through `DelayedIRModifier::type_check`
(`taichi/ir/ir.h:617`, `taichi/ir/ir.cpp:533-534`, drained at `ir.cpp:566`),
three sites in one file:*

| Site | Context |
|---|---|
| `taichi/transforms/simplify.cpp:219` | after each `LinearizeStmt` expansion |
| `taichi/transforms/simplify.cpp:344`, `:349` | on a synthesised load and select |

**Twenty-nine invocation sites in total in this territory.** Revision 2 named
four of them (`compile_to_offloads.cpp`'s seven as a group, `simplify.cpp:219`,
`handle_external_ptr_boundary.cpp:100`, and `check_out_of_bound.cpp`'s `run()`
without a line). Nineteen sites in twelve files were missing.

**Sub-count corrected in revision 4, and the whole section reconciled
mechanically.** Revision 3 headed the middle sub-table "fourteen sites in ten
files" over a table listing nine, which made the three sub-counts read
10 + 4 + 1 = 15 against this section's own stated total of fourteen files. Both
round-three adversaries derived nine independently and both are right. The nine
are `auto_diff.cpp`, `demote_atomics.cpp`, `demote_mesh_statements.cpp`,
`lower_access.cpp`, `make_block_local.cpp`, `make_mesh_block_local.cpp`,
`make_mesh_thread_local.cpp`, `make_thread_local.cpp`, `offload.cpp`. This is
the fifth instance in this project of a total contradicting the list beneath it,
and the second by this report, so the arithmetic is now derived rather than
asserted:

| Group | Sites | Files |
|---|---|---|
| Unqualified `type_check(root, config)` | 14 | 9 |
| Qualified `irpass::type_check` | 5 | 4 |
| Queued via `DelayedIRModifier::type_check` | 3 | 1 |
| **Outside the driver** | **22** | **14** |
| `compile_to_offloads.cpp` | 7 | 1 |
| **Total in territory** | **29** | **15** |

Checked mechanically, not read off: the twenty-two `file:line` pairs written to
one file, `wc -l` gives 22, `cut -d: -f1 | sort -u | wc -l` gives 14, and the
three groups' file sets are disjoint, so 9 + 4 + 1 = 14 rather than merely
agreeing with it. Each of the twenty-two lines was then re-printed from source
with `sed -n "${l}p"` to confirm it is a call and not a declaration or a
comment. The fifteen in the last row counts `compile_to_offloads.cpp`, which
contributes none of the twenty-two.

Consequence, unchanged in direction and stronger in degree: it is not sufficient
to change the coercion once and let a wider type flow. The pass re-applies after
every intervening transform, and several transforms (section 3.4) re-synthesise
i32 statements *after* it has run. A planner sizing item 6.2 from "seven" would
undercount the re-application surface by roughly three quarters.

Relevant ordering, all from `compile_to_offloads.cpp`: `:43`
`frontend_type_check` (warns only — `frontend_type_check.cpp:71-77` emits the
same message with no cast); `:44` `lower_ast`; `:57` first `type_check`;
`:72` `lower_matrix_ptr`; `:76` `scalarize`; `:89`
`handle_external_ptr_boundary`; `:124` `check_out_of_bound`; `:139` `offload`;
`:192` `demote_dense_struct_fors`; `:199` `make_cpu_multithreaded_range_for`;
`:232` `make_block_local`; `:262` `lower_access` (emits the `LinearizeStmt`s);
`:278` `full_simplify` (expands them).

---

## 6. What the type system can and cannot express today

**Can, verified:**

- `i64` / `u64` exist — `taichi/inc/data_type.inc.h:7`, `:12`, surfaced by the
  macro at `taichi/ir/type.h:151-155`.
- `TypedConstant` carries 64-bit values — union at `taichi/ir/type.h:556-569`,
  accessors at `:661-677`.
- `TypeFactory::get_primitive_int_type` accepts `bits == 64` —
  `taichi/ir/type_factory.cpp:168-169`.
- `is_integral` / `is_signed` include i64/u64 — `taichi/ir/type_utils.h:107`,
  `:112`, `:123`.
- `data_type_size` derives the size, including i64/u64 —
  `taichi/ir/type_utils.cpp:59`, `:64`.
- Promotion is by bit width, so `i32 op i64` promotes to `i64` —
  `taichi/ir/type_factory.cpp:222-256`, `:265-286`. **Bounded:**
  `to_primitive_type` at `:249-251` opens with
  `if (d->is<PointerType>()) { TI_ERROR("promoted_type got a pointer input."); }`,
  so promotion is unavailable for any operand carrying a pointer type.
- Constant folding handles i64/u64 — `taichi/transforms/constant_fold.cpp:33-47`,
  with the type-dispatched branches at `:113-120`.
- `IRBuilder` offers `get_int64` / `get_uint64` —
  `taichi/ir/ir_builder.h:134`, `:136`; `taichi/ir/ir_builder.cpp:147-151`,
  `:159-163`.
- `IndexExpression::type_check` accepts any integral index —
  `taichi/ir/frontend_ir.cpp:936-947`, which checks only `is_integral` at `:941`.
  **Qualified twice.** First, this holds for an explicit index expression. It
  does not hold for a loop variable, which `taichi/ir/frontend_ir.cpp:151-152`
  pins to i32 in the frontend itself. Second, **new in revision 4**, it is a
  statement about field and array indices and not about every index the frontend
  accepts: `TextureOpExpression::type_check` rejects any texture coordinate that
  is not exactly i32, with a `TaichiTypeError`, in three branches at
  `taichi/ir/frontend_ir.cpp:1216`, `:1233` and `:1249`. Raised by
  `adversary3-01-2.md` and verified. The planner has since ruled those three
  sites **out of scope**, so they are recorded in section 3.14 and are not in
  the section 3.3 inventory — but the qualification on this claim is a statement
  about the tree, not about scope, and it holds either way.
- Byte offsets are already 64-bit clean — section 3.8.
- `SNodeOpType::get_addr` already yields `u64` —
  `taichi/transforms/type_check.cpp:106-107`,
  `taichi/ir/frontend_ir.cpp:1109-1110`.
- **A 64-bit-integer device capability already exists and is already enforced.**
  `taichi/inc/rhi_constants.inc.h:14` declares
  `PER_DEVICE_CAPABILITY(spirv_has_int64)`; the enum is materialised at
  `taichi/rhi/device_capability.h:11-15`; backends set it at
  `taichi/rhi/vulkan/vulkan_device_creator.cpp:632`,
  `taichi/rhi/opengl/opengl_device.cpp:511`,
  `taichi/rhi/metal/metal_device.mm:132`, `:1052`; and SPIR-V codegen enforces
  it at `taichi/codegen/spirv/spirv_ir_builder.cpp:64-66` (emits
  `OpCapability Int64`), `:166-169` (declares `t_int64_`/`t_uint64_` only if
  present), `:311-314` and `:325-328` (`TI_ERROR("Type {} not supported.")` for
  i64 and u64 without it).

  So the accurate statement for plan 6.2's stub note is **not** "a target
  lacking Int64 has nowhere to say so". It is: *the capability is declared and
  enforced at SPIR-V codegen; what does not exist is any link from the type
  layer to it.* Today, widening an index to i64 on a Vulkan target without
  Int64 produces a hard `TI_ERROR`, not a silent degradation.

**Cannot, verified:**

- There is no index type and no address type. `PointerType` carries no width
  (section 1). Pointer interning is keyed on `(element, is_bit_pointer)`
  (`taichi/ir/type_factory.cpp:86-95`), so a width field alone would not
  distinguish two pointer types.
- There is no way for a pass to ask what width an index should be on the
  current target. The capability enum above is consulted only at SPIR-V codegen;
  nothing in `taichi/ir/`, `taichi/analysis/` or `taichi/transforms/` reads it.
- `data_type_size` cannot report a pointer's size.
  `taichi/ir/type_utils.cpp:35` is `t.set_is_pointer(false);` — it **strips** the
  pointer flag and returns the pointee's size rather than refusing. The TODO at
  `:31-34` records that a loud failure on pointers was intended and never
  written, so a caller asking a pointer type's size today silently gets a wrong
  answer.

---

## 7. Verified versus inferred

### 7.1 What revision 1 got wrong, and the correction

Recorded explicitly because the planner read revision 1.

1. **WITHDRAWN — "a 64-bit constant range-for bound is silently truncated to its
   low 32 bits."** False. `mark_as_if_const`
   (`taichi/transforms/type_check.cpp:40-44`) writes `Stmt::ret_type` only;
   `ConstStmt` holds a separate `TypedConstant val` (`statements.h:988`) whose
   `dt` is set once in the constructor (`:993`) and never re-synchronised;
   `val_int32()` (`taichi/ir/type.cpp:462-465`) asserts against
   `TypedConstant::dt` (`taichi/ir/type.h:551`), **not** `ret_type`; and
   `TI_ASSERT` (`taichi/common/logging.h:100-107`) is unconditional. The path
   raises a hard `TI_ERROR` at `offload.cpp:109`/`:117`. Revision 1 also listed
   the assert among its own verified facts, contradicting the inference. The
   silent-truncation claim survives only for `taichi/ir/snode.cpp:92`, a plain
   `static_cast<int>` with no assert.
2. **WITHDRAWN — the mechanism behind "the sparse path truncates silently above
   2^31 cells."** `total_shape` in `scalar_pointer_lowerer.cpp:33-39` is indexed
   by axis and holds a per-axis extent, not a cell count. The conclusion may
   hold by another route; the evidence given did not support it.
3. **CORRECTED — "`type_check.cpp` is the single enforcement point. Nothing else
   in the compiler imposes i32 on an index."** Ten cast sites in four files
   (3.1), twelve abort sites (3.2), eighteen pinned `ret_type`s (3.3), a whole
   re-synthesis pass after type checking (3.4), and seven re-runs of the
   coercion (section 5).
4. **ADDED — `taichi/inc/rhi_constants.inc.h:14`** (section 6). Missed in
   revision 1 despite being in my own territory and read in note N001.
5. **REMOVED — `taichi/ir/expr.cpp:93` as a "default integer literal".** It is
   `Expr::Expr(int32 x)`, one overload of a pair whose i64 sibling is at
   `:96-98`. Not a default and not a narrowing. Listing it inflated the
   inventory.
6. **STRUCK — "All line numbers were read directly and then re-verified in a
   second pass."** The second pass checked line *content* and not line
   *attribution*, so it confirmed that a line said what was quoted without
   confirming it was the intended line. Sixteen references were wrong; all are
   corrected here and tabulated in note N031.

### 7.2 VERIFIED — opened and read

Everything in sections 2, 3, 4, 5 and 6, and every file:line in this report.
Beyond the files listed in revision 1, this pass additionally opened
`taichi/common/logging.h`, `taichi/ir/type.cpp`,
`taichi/ir/control_flow_graph.cpp`, `taichi/transforms/lower_ast.cpp`,
`taichi/transforms/make_cpu_multithreaded_range_for.cpp`,
`taichi/transforms/demote_mesh_statements.cpp`,
`taichi/transforms/make_mesh_thread_local.cpp`,
`taichi/transforms/make_mesh_block_local.cpp`,
`taichi/transforms/lower_matrix_ptr.cpp`, `taichi/transforms/auto_diff.cpp`,
`taichi/analysis/arithmetic_interpretor.cpp`,
`taichi/rhi/device_capability.h`, `taichi/codegen/spirv/spirv_ir_builder.cpp`
and `taichi/runtime/llvm/runtime_module/runtime.cpp` (the last four outside my
territory, opened only to check claims about my own).

Specifically verified by exhaustive grep, not assumed:

- Zero uses of `taichi_max_num_snodes` and `kMaxNumSnodeTreesLlvm` in territory.
- The complete `taichi_max_num_indices` list (2.3).
- All 14 `val_int32()` call sites tree-wide, each opened to check for a type
  guard (3.2). **Both adversaries were wrong here**: `constant_fold.cpp:115` is
  guarded and is not an abort site.
- All 8 `cast_type = PrimitiveType::i32` sites plus the 2 in `type_check.cpp`
  (3.1). **Adversary 1's count of eight was wrong**; its enumeration listed ten.
- The 6 `make_mesh_block_local.cpp` byte-offset narrowings, including the three
  in functional-cast spelling.
- The 7 `irpass::type_check` calls in `compile_to_offloads.cpp`.
- The 2 `value_diff.cpp` i32-only visitors and their 6 consumers.

Added in revision 3, each opened rather than adopted from an adversary:

- `taichi/ir/snode.cpp:14-35`, `:83`, `:84`, `:89-101`, `:174-177` and
  `taichi/ir/snode.h:41`, `:45`, `:49`, `:93-97`, `:306-308`, `:310-315` — the
  full propagation path that settles per-container versus root-to-leaf
  (section 4).
- `taichi/transforms/check_out_of_bound.cpp:123-126`, re-opened to confirm the
  bound is `shape_along_axis`'s return value.
- `taichi/transforms/lower_ast.cpp:250-280` and
  `taichi/ir/frontend_ir.cpp:725-750`, both branches (section 3.12).
- `taichi/transforms/simplify.cpp:218-278`, to fix the visitor span.
- The complete `type_check` invocation set, from two greps, all 29 sites opened
  in context (section 5).
- `taichi/ir/type_factory.cpp:198-215` and `taichi/transforms/lower_ast.cpp:330`,
  to confirm neither is a `ret_type` assignment (section 3.3).
- `taichi/runtime/program_impls/llvm/llvm_program.cpp:105-120`, to establish that
  the `:112` versus `:113` claim is not about this report (section 7.5 item 7).

Added in revision 4. Four sweeps run to exhaustion, with their commands and
counts in section 3.13, and every line each returned opened or dispositioned:

- The sixty-four `PrimitiveType::i32` lines in territory, each assigned a
  disposition in section 3.13 and the table's three-way total checked against
  sixty-four.
- `taichi/transforms/auto_diff.cpp` swept on all three i32 spellings — twelve
  lines, nine index constructions and three `val_int32()` reads — rather than
  adopting an adversary's group of eight.
- `taichi/transforms/lower_matrix_ptr.cpp:185-195`, `:228-238`, `:420-430`,
  `:473-483`, to confirm each `TypedConstant(i)` feeds a `MatrixPtrStmt` offset.
- `taichi/transforms/lower_ast.cpp:143-184` and `:326-370`, to settle which of
  the four allocas is an index and which is anonymous, and to place the file's
  four `TypedConstant((int32)…)` tokens.
- `taichi/transforms/scalarize.cpp:57-69`, `:108-120`, `:460-482`, `:524-560`.
- `taichi/ir/frontend_ir.cpp:1155-1172` and `:1208-1256`.
- `taichi/transforms/make_mesh_thread_local.cpp:30-50` and
  `taichi/transforms/check_out_of_bound.cpp:205-230`, the two sites nobody named.
- `taichi/transforms/demote_operations.cpp:92-104`, to check whether the
  implicit-spelling residue contains real sites. It does; escalation 16.
- `taichi/ir/ir_builder.cpp:118-130` and `taichi/ir/statements.h:69-79`, to
  establish that `create_break()`'s `get_int32(0)` is the `WhileControlStmt`
  **cond** and not the mask.
- Outside territory, opened only to check the device-side half of
  `frontend_ir.cpp:1160`: `taichi/runtime/llvm/runtime_module/node_dynamic.h:55-70`
  and `taichi/codegen/llvm/codegen_llvm.cpp:1360-1378`. Agents 02 and 03 own them.

Counts checked mechanically in this pass, not read off a table: the twenty-two
non-driver `type_check` lines (22 lines, 14 files, groups 9 + 4 + 1 disjoint);
the section 3.3 sites, re-derived after the texture ruling (51 lines, 51 unique,
12 files, and the table's own rows re-extracted and diffed against that list);
the sixty-four-line sweep (52 inventoried + 5 added + 4 excluded + 3 recorded
out of scope = 64).

### 7.3 INFERRED — reasoning from verified code, not observed

None of these were executed or tested.

1. **A dense struct-for over more than 2^31 cells aborts the compiler.** From
   the hard `TI_ASSERT` at `demote_dense_struct_fors.cpp:29` with the `int64`
   accumulation at `:18`, `:26`. Not observed.
2. **An external-array struct-for over more than 2^31 elements overflows
   silently**, since `lower_ast.cpp:270-274` has no assert and no warning on its
   i32 product. By contrast with (1). Not observed.
3. **`extractors[i].acc_shape` and `num_cells_per_container` disagree above
   2^31**, from the adjacent assignments at `snode.cpp:92` and `:101`. Not
   observed.
4. **Widening indices to i64 degrades alias analysis to `uncertain`**, from the
   two i32-only visitors (3.10) and their six consumers. The optimisation loss
   was not measured.
5. **`simplify.cpp:260-261` is unguarded** while `:238-239` is guarded by the
   asserts at `:233`/`:234-235`. Those asserts sit inside
   `visit(SNodeLookupStmt *)` (`:222-249`); `visit(GetChStmt *)` (`:251-273`)
   contains none. A transitive data-flow argument through the
   `IntegerOffsetStmt` at `:255` is plausible and unproven. Both adversaries
   examined this and neither settled it either way.
6. **The offline cache key changes shape if `taichi_max_num_indices` changes**,
   from the loop bound at `offline_cache_util.cpp:110`. The cache versioning
   machinery is outside my territory and was not checked.
7. **Ten of the twelve abort sites are downstream of one decision**, since they
   all read a `MatrixPtrStmt::offset` that `type_check.cpp:160-161` has already
   asserted. From reading each site's guard; I did not trace every producer.

### 7.4 Outside my territory — recorded, not asserted as complete

`taichi/runtime/llvm/runtime_module/runtime.cpp:288-290`
(`PhysicalCoordinates`, i32 slots), `taichi/codegen/spirv/spirv_ir_builder.cpp`
(the Int64 capability enforcement), `taichi/codegen/llvm/codegen_llvm.cpp:1736-1744`
and `taichi/codegen/spirv/spirv_codegen.cpp:532-541` (the two codegen
`LinearizeStmt` implementations), `taichi/codegen/llvm/codegen_llvm.cpp:1054`
(a guarded `val_int32()`), and `tests/cpp/transforms/`. Opened to check claims
about my own territory. Agents 02, 03 and 04 own these.

Added in revision 4: `taichi/runtime/llvm/runtime_module/node_dynamic.h:61`,
`Ptr Dynamic_allocate(Ptr meta_, Ptr node_, i32 *len)`, writing through `len` at
`:66`; and `taichi/codegen/llvm/codegen_llvm.cpp:1367-1375`, which passes the
frontend alloca into that call at `:1374`. Opened only to establish that
`taichi/ir/frontend_ir.cpp:1160` has a device-side counterpart at the same
width, which is what makes it material rather than incidental (section 3.3).
Agents 02 and 03 own them.

Added in revision 3:
`taichi/codegen/spirv/snode_struct_compiler.cpp:115-121` —
`sn_desc.total_num_cells_from_root *= e.num_elements_from_root;`, the genuine
whole-structure cell count, assembled from the territory-01 `int` field named in
section 4. Opened only to establish that the unguarded field is load-bearing at
the seam. Agent 02 owns it.

### 7.5 What revision 2 got wrong, and the correction

This pass adjudicated seven claims put to me by the planner, drawn from
`adversary2-01-1.md` and `adversary2-01-2.md`. Every one was checked against
source before being acted on. **Five were correct and are fixed here. One was
correct on the mechanism but wrong on its own count. One does not belong to this
report.** Full working in notes N039–N046.

1. **CORRECTED — `taichi/ir/snode.cpp:89-101` was misclassified as a
   whole-structure flattening site.** It is a per-container extent. Claim
   verified in full: `insert_children` propagates only `num_elements_from_root`
   (`snode.cpp:22-23`), `shape` is a plain per-node assignment (`:84`), and the
   result is `num_cells_per_container`, documented as such at `snode.h:93-97`.
   The replacement site, `AxisExtractor::num_elements_from_root`, is the
   unguarded root-to-leaf `int` product returned by `shape_along_axis`
   (`snode.cpp:174-177`) and materialised as the i32 bound at
   `check_out_of_bound.cpp:123-126`. Revision 2 cited both halves and never
   joined them. Section 4 rewritten.
2. **CORRECTED — `taichi/transforms/lower_ast.cpp:270-274` was filed under a
   heading asserting host-side compile-time arithmetic.** It is an IR-level i32
   product: `ConstStmt` at `:270-271`, `BinaryOpStmt` at `:273`. Moved to the new
   section 3.12. Every other row of section 3.6 re-opened and confirmed host-side.
3. **CORRECTED — section 3.3 headed twenty sites with a count of eighteen**, and
   two of its rows are not `ret_type` assignments. Heading and preamble fixed.
4. **CORRECTED — the `visit(GetChStmt *)` span.** It is
   `taichi/transforms/simplify.cpp:251-273`: `:270` closes the inner `if`, `:272`
   is `set_done(stmt);`, `:273` closes the visitor. Revision 2 said `:251-270`.
   Both adversaries flagged it and both are right; pass B's `:251-274` is wrong
   in the other direction. The substantive point about `:261` being unguarded on
   its face is unaffected and remains inferred and escalated.
5. **CORRECTED, with the adversary's count adjudicated against.** The in-pass
   `type_check` re-run surface was badly understated, and the stated mechanism —
   unqualified `type_check(root, config)` from inside `namespace irpass`, invisible
   to a search for `irpass::type_check` — is correct for fourteen of the missing
   sites. But `adversary2-01-2.md` section 3.2 writes "at least fifteen further
   re-runs across fourteen files" over a list of twenty-two. I re-derived the set
   independently: twenty-two further sites, fourteen files, twenty-nine
   invocation sites in total. Section 5 rewritten. Fourteen files is right;
   fifteen sites is not.
6. **CORRECTED — `taichi/ir/frontend_ir.cpp:731-740`, the dynamic tensor-offset
   branch, was raised in round one and neither addressed nor answered by
   revision 2.** The claim is accurate. Revision 2 carried only the
   constant-folded branch at `:742-747`, which is correctly filed in section 3.6
   as a host `int`. The dynamic branch emits IR and is now in section 3.12. This
   was an objection I failed to answer, not one I rebutted.
7. **NOT MINE — the `llvm_program.cpp:112` versus `:113` citation.** Verified
   twice. The claim is factually right: `taichi/runtime/program_impls/llvm/llvm_program.cpp:112`
   is `LlvmOfflineCache::FieldCacheData::SNodeCacheData snode_cache_data;`, the
   declaration, and the assignment `snode_cache_data.id = snodes[i]->id;` is at
   `:113`. But the citation is not in this report and never was. It is at
   `modernization/investigation/notes-03-runtime-struct.md:466`, and
   `adversary2-03-1.md:391` and `:497` already raise it against territory 03.
   **I changed nothing.** Recorded here so the planner does not chase it into
   this territory a third time.

Five further items raised by the round-two adversaries fell outside the seven
claims. I verified each and escalated the scoping call rather than making it.
**The planner ruled that completeness inside this territory is the brief, not a
scope question, and directed that all five enter the body.** They now do:

| Item | Where it landed |
|---|---|
| `taichi/transforms/type_check.cpp:170`, `:171` — `mark_as_if_const` on both `RangeForStmt` bounds | Section 3.3, new row |
| `taichi/transforms/lower_ast.cpp:168`, `:348`, `:361` — three further i32 allocas beyond `:330` | Section 3.3, new row |
| `taichi/transforms/auto_diff.cpp:587`, `:639`, `:646`, `:787`, `:839`, `:846`, `:1828`, `:1857` — i32 index constructions | Section 3.3, two new rows |
| `taichi/transforms/make_mesh_block_local.cpp:526` — `mapping_data_type_ = PrimitiveType::i32;` with the derived form commented out at `:525` | Section 3.7, new row |
| The section 3.4 gate is `config.make_cpu_multithreading_loop && arch_is_cpu(config.arch)`, not arch alone | Section 3.4 body, as a correction |

Section 3.3 therefore moves from twenty sites to thirty-three. The gate is a
factual correction to something revision 2 asserted, so it is stated in the body
rather than carried as an escalation; escalation 4 now rests on a correct
description of the gate.

Separately, the planner ruled on
`SNode::get_total_num_elements_towards_root` (`taichi/ir/snode.h:310-315`): a
whole-structure flattening with an `(int)` narrowing per factor at `:313` and no
callers anywhere in the tree. Recorded in section 3.9 as **latent**, with the
no-caller fact stated first, on the grounds that absence of callers today is not
a reason to omit it and that plan section 4.6 makes new callers foreseeable. It
is deliberately not counted among the three reached formation sites in section 4.


### 7.6 What revision 3 got wrong, and the correction

Both round-three adversaries judged revision 3 CORRECT and NOT COMPLETE, and
neither recommended a fourth investigative round. This pass adjudicated the five
claims the planner put to me. **Four were correct and are fixed here. One was
correct on the site and wrong on nothing, and I found the group it named to be
larger than either adversary said.** Every claim was checked against source
before being acted on. Working in notes N051–N062.

1. **CORRECTED — section 5's middle sub-count.** "Fourteen sites in ten files"
   over a table listing nine. Verified independently, mechanically: the fourteen
   unqualified `type_check(root, config)` lines written to a file give
   `cut -d: -f1 | sort -u | wc -l` = 9. The nine are named in section 5. Both
   adversaries derived nine and both are right. Revision 3's own stated total of
   fourteen files required it, since 9 + 4 + 1 = 14 and 10 + 4 + 1 = 15. The
   fifth instance in this project of a total contradicting its list, and the
   second by this report. Section 5 now carries a reconciliation table and the
   commands that check it, so the figure is derived and not asserted.

2. **CORRECTED, and the claim was an undercount of its own.**
   `taichi/transforms/auto_diff.cpp:1819` is real and revision 3 did not have
   it. Verified at source: `indices_values[i] = insert<ConstStmt>(TypedConstant((int32)i));`,
   loop at `:1818`, feeding the `MatrixInitStmt` at `:1829` typed at `:1830`
   from the i32 tensor type at `:1827-1828`. I swept the file myself as
   instructed — `grep -n "PrimitiveType::i32\|(int32)\|int32(" taichi/transforms/auto_diff.cpp`
   returns **twelve lines**: nine index constructions (`:587`, `:639`, `:646`,
   `:787`, `:839`, `:846`, `:1819`, `:1828`, `:1857`) and the three `val_int32()`
   reads already in section 3.2 (`:576`, `:776`, `:1770`). The group of eight
   revision 3 adopted from a round-two adversary was short by exactly one, and
   the claim that the section total should be thirty-four is right **as far as
   it goes** — with the other seventeen additions of this pass, and the three
   texture sites the planner subsequently ruled out of scope, the total is
   fifty-one, derived in section 3.3.

3. **CORRECTED — the `lower_ast.cpp` alloca label, and the table's unit stated
   plainly.** Revision 3 called `:168`, `:348` and `:361` "all loop-mask
   allocas". Wrong for `:361`. Verified: `:169` and `:349` are each
   `new_while->mask = mask.get();`, so `:168` and `:348` are masks; `:361` is
   moved straight into `insert_before_me` at `:360-361` with nothing bound to
   it, and the mask in that block is the `:348` alloca moved in at `:365` and
   stored to at `:367-368`. Both adversaries rule the inclusion defensible under
   this report's own heading, which is hardcoded-i32 and not index-width, and
   both are right — the count is not inflated. The row is relabelled and
   **section 3.3 now opens by stating that the table is not an index count**,
   which is the substance of the objection.

4. **CORRECTED — `taichi/transforms/lower_matrix_ptr.cpp:189`, `:232`, `:424`,
   `:477` were absent entirely.** Verified: four identical
   `std::make_unique<ConstStmt>(TypedConstant(i))` in loops over `num_elements`,
   each feeding the offset of a `MatrixPtrStmt` two lines below. Named by nobody
   before round three. Revision 3 cited this file only at `:536-540` and in
   escalation 5.

5. **CLOSED — the completeness gap, and one thing the claim did not know.** The
   claim that `grep -rn "PrimitiveType::i32"` over the territory returns
   sixty-four lines is exact; I ran it and got sixty-four. All sixty-four are now
   enumerated with a disposition in the new section 3.13. The claim that
   `taichi/ir/frontend_ir.cpp:1160` is the material one is right, and I verified
   the device-side half rather than adopting it:
   `Ptr Dynamic_allocate(Ptr meta_, Ptr node_, i32 *len)` at
   `taichi/runtime/llvm/runtime_module/node_dynamic.h:61`, writing the index
   through `len` at `:66`, reached from
   `taichi/codegen/llvm/codegen_llvm.cpp:1374`.

   **Where the claim stops short.** Six of the sixty-four were said to be in
   neither report. In this report eight of the sixty-four were missing, not six.
   Five of the eight entered the inventory; the three texture sites were ruled
   out of scope by the planner and are recorded in section 3.14 instead.
   The two nobody named are `taichi/transforms/make_mesh_thread_local.cpp:42`
   and `taichi/transforms/check_out_of_bound.cpp:220`, both found by running the
   sweep rather than reading about it. And the sweep itself does not close the
   territory: three companion spellings exist, two of which I closed
   exhaustively and one of which I did not. Section 3.13 gives all four commands
   and their counts, and section 8 item 16 escalates the one that remains open.
   The word "complete" has been removed from the section 3 heading in
   consequence.

**Upheld from round three, disturbed by nothing in this pass.** The qualifier
"above 2^31 in any single container" on `get_total_num_elements_towards_root`
(section 3.9), which both adversaries independently call load-bearing and
correct — stripped of that clause the sentence would be false, because the
`int64` accumulator does buy headroom on the product of already-narrowed
factors. The filing of `make_mesh_block_local.cpp:526` in section 3.7 rather
than 3.1, ruled defensible by both. And citation accuracy, which survived a
sample of roughly seventy cited lines in `adversary3-01-1.md` and a second
independent sample in `adversary3-01-2.md` with no fault found in either. None
of the three was touched.

**One claim in `adversary3-01-1.md` that I did not act on, and why.** Its
section 6 and escalation 1 raise a mixed-width residual at
`taichi/transforms/constant_fold.cpp:115`: the guard at `:113` tests
`lhs->val.dt` only, while `:115` calls the narrowing `val_int32()` on both
operands, so an i32 left with a wider right would abort. I verified the
mechanism — `:65` is `auto dt = lhs->val.dt;` and nothing between `:49` and
`:125` reads `rhs->val.dt` — and both adversaries agree the residual is real and
that whether such a pair is reachable is unproven in both directions. This
report's section 3.2 finding is unaffected: `:115` is guarded 64-bit dispatch
and is not an abort site under today's invariant, which is what revision 3
claimed and what both adversaries uphold. The residual is a consequence of a
width change rather than a defect today, and it belongs in escalations, where it
now is (section 8, item 17). I changed no count.

---

## 8. Escalations

Unresolved. Recorded, not decided.

1. **Is the mesh path in scope?** Eight of the ten cast-insertion sites (3.1)
   and four pinned `ret_type`s (3.3) are mesh. The answer decides whether the
   cast inventory has two rows or ten. Plan section 6 does not say. **Planner.**

2. **What width should an index be, and is it one width?** Plan 6.2 says the
   change must hold end to end but not whether that means i64 everywhere or a
   target-dependent width. Section 6 shows the capability machinery for a
   target-dependent answer already exists at the RHI and SPIR-V layers.
   Connecting it to the type layer is added structure, which standing
   instruction 4 forbids me from proposing. **Planner.**

3. **Does `PhysicalCoordinates` fix the frontend's index width?** If the device
   struct at `taichi/runtime/llvm/runtime_module/runtime.cpp:288-290` keeps i32
   slots, `type_check.cpp:467`'s i32 `LoopIndexStmt` is not a free choice on the
   LLVM path. Spans territories 01, 02 and 03. **Planner.**

4. **Is the CPU path in scope?** `make_cpu_multithreaded_range_for.cpp` (3.4)
   narrows the whole parallel iteration space to i32, including reading a
   dynamic bound back out of a `size_t`-offset buffer as i32. Plan section 2
   makes CPU-only a first-class target, not a fallback. **Planner.**

5. **`MatrixPtrStmt::offset` has two meanings.** The TODO at
   `taichi/ir/statements.h:512-520` says it means *bytes* on some paths and
   *index* on others, disambiguated by `offset_used_as_index()` at `:521-529`.
   The byte-meaning branch is realised at `lower_matrix_ptr.cpp:536-540` and
   computes a byte offset in i32 while every other byte offset in the IR is
   `size_t`. Ten of the twelve abort sites (3.2) read this field. **Planner.**

6. **Which of the three whole-structure extent formation sites does the workload
   reach?** Section 4 shows three sites with three different failure modes:
   `demote_dense_struct_fors.cpp:26`/`:35` aborts, `lower_ast.cpp:270-274`
   overflows silently in IR, and `AxisExtractor::num_elements_from_root`
   (`snode.h:41`) overflows silently in host `int` with no guard of any kind.
   **Sharpened in revision 3:** the third is the bound compared against every
   SNode field index (`check_out_of_bound.cpp:123-126` via
   `snode.cpp:174-177`), so on a dense-SNode workload it is the one reached
   first, and it is the one with no assert and no warning. This is what
   separates a map from a list, and it depends on the engine SNode configuration
   that plan 8.1 item 3 records as unsettled. **Planner.**

7. **The `sizeof(int32)` sites at `simplify.cpp:239` and `:261`.** Hardcoded
   4-byte element strides where `scalarize.cpp:1121-1122` derives the equivalent
   via `data_type_size(...)`. Whether these are dead paths, deliberate, or bugs
   is not mine to decide, and standing instruction 3 forbids me deciding
   anything is unnecessary. **Planner.**

8. **`taichi_max_num_indices` is a footprint on both host and device.**
   `snode.h:78`, `:81` embed `AxisExtractor[12]` and `int[12]` by value in every
   `SNode`; `runtime.cpp:288-290` sizes a device struct by the same constant.
   Plan 6.2 names it as a "related constant" without saying what relation.
   **Planner.**

9. **`kMaxNumSnodeTreesLlvm = 512`.** A second, lower, undocumented ceiling at
   `taichi/inc/constants.h:13`, used at
   `taichi/runtime/llvm/runtime_module/runtime.cpp:562-563`. Plan 6.1 names only
   `taichi_max_num_snodes`. Whether the SNode-tree ceiling is in scope is not
   mine. **Planner.**

10. **Are the C++ tests a constraint or a casualty?**
    `tests/cpp/transforms/scalar_pointer_lowerer_test.cpp` and
    `tests/cpp/transforms/make_block_local_test.cpp` assert the exact numeric
    output of the index lowering item 6.2 would change, and are the sole
    consumers of `ArithmeticInterpretor`. `tests/cpp/ir/` additionally holds
    `ir_type_promotion_test.cpp` and `type_test.cpp`. Nobody has said whether
    they are maintained. **Planner.**

11. **Offline cache invalidation.** Changing `taichi_max_num_indices` or the
    extractor widths changes the key produced at
    `offline_cache_util.cpp:110-124` and `gen_offline_cache_key.cpp:540`.
    **Planner.**

12. **`taichi/program/ndarray.cpp:57` and `:100`** carry the same "int64 indexing
    is not supported yet" warning as `snode.cpp:97` but sit outside my
    territory. **Planner, for assignment.**

13. **RESOLVED by the planner, revision 3. Five adversary items fell outside the
    seven claims this pass was scoped to.** Ruling: completeness inside this
    territory is the brief, not a scope question, so all five enter the
    inventory, marked *amendment pass* for provenance. Done —
    `type_check.cpp:170`, `:171` and `lower_ast.cpp:168`, `:348`, `:361` and the
    eight `auto_diff.cpp` index constructions in section 3.3;
    `make_mesh_block_local.cpp:526` in section 3.7; the
    `config.make_cpu_multithreading_loop` half of the gate at
    `compile_to_offloads.cpp:198` corrected in the body of section 3.4 rather
    than carried here, since it refines an assertion the report already made.
    Landing table in section 7.5. Nothing outstanding.

14. **RESOLVED by the planner, revision 3.
    `SNode::get_total_num_elements_towards_root` (`taichi/ir/snode.h:310-315`)
    narrows every factor with `(int)` at `:313` inside an `int64` accumulator,
    and has no callers tree-wide.** Ruling: record it as latent rather than drop
    it, with the no-caller fact stated plainly, following the precedent set in
    territory 03 and plan section 4.6, under which the engine SNode
    configuration is composed for working scope and new callers are foreseeable.
    Done — section 3.9. It is not counted among the three reached formation
    sites in section 4. Nothing outstanding.

15. **RESOLVED by the planner, revision 4. Texture coordinates are out of
    scope.** I raised `taichi/ir/frontend_ir.cpp:1216`, `:1233`, `:1249` — three
    hard i32 requirements on index arguments, raising `TaichiTypeError` on
    anything else — inventoried them, and escalated the scope call rather than
    making it, per standing instruction 3. **Ruling: out of scope, recorded
    rather than inventoried.** Grounds: plan section 1.3 puts rendering entirely
    out of scope, and the project owner has confirmed positively that the
    foreseen data carries no graphic payload, particle attachments being field
    interaction data under purely mathematical modelling. Texture operations are
    not work this project will do. Done — the row is out of section 3.3, the
    three sites are recorded with the reason in the new section 3.14, and the
    section 3.3 count falls from fifty-four to fifty-one, re-derived rather than
    decremented. The qualification they force on sections 1 and 6 stands
    regardless, because it is a claim about the tree and not about scope.
    Nothing outstanding.

16. **NEW in revision 4. The implicitly-typed i32 constant class is unscoped and
    unenumerated.** `TypedConstant` has an implicit `int32` constructor at
    `taichi/ir/type.h:582`, so an i32 index constant can be built with no `i32`
    token anywhere on the line — which is exactly how
    `lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477` escaped three rounds of
    review. Section 3.13 measures the class at **78 candidate lines across 16
    files** after removing every explicitly-typed form, and does not classify
    them. Some are macro continuations and not sites at all; others plainly are
    sites of the same class, for instance `taichi/transforms/demote_operations.cpp:99`,
    `ConstStmt(TypedConstant(i))` in the loop at `:98` over
    `lhs_tensor_ty->get_num_elements()`, feeding the `MatrixPtrStmt` offsets at
    `:100` and `:101` — the identical form to `lower_matrix_ptr.cpp:189`.
    Classifying all seventy-eight is a fresh enumeration of a class nobody has
    scoped. I stopped and am reporting rather than improvising, per standing
    instruction 2.

    **Standing, and taken off this report by the planner in revision 4.** A
    dedicated pair is being dispatched to enumerate and classify the class as its
    own territory with its own adversarial round. This escalation stays open and
    is the incoming pair's starting point, so it is deliberately left as written
    rather than trimmed. The planner's instruction to me was explicit: **do not
    partially classify the residual**, because a half-closed enumeration reads as
    finished and is worse than an open one. I have not, and section 3.13 now
    states exactly where the boundary falls, so the incoming pair can tell what
    has been touched from what has not.

    The mechanism, stated once and plainly because it is the reusable part:
    `TypedConstant`'s implicit `int32` constructor at `taichi/ir/type.h:582`
    means an i32 index constant can be written with no `i32` token on the line.
    A grep for the type name therefore cannot see this class at all. That is not
    a defect in any one report; it is why four sites
    (`lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477`) survived two explore
    passes and three adversarial rounds unnamed. **The class of miss is the
    finding, not the four instances.**

17. **NEW in revision 4. The mixed-width residual at
    `taichi/transforms/constant_fold.cpp:115`.** The guard at `:113` tests
    `lhs->val.dt` alone (set at `:65`), while `:115` calls the narrowing
    `val_int32()` on both operands, whose `TI_ASSERT` is unconditional. An i32
    left operand with a wider right operand would abort. Nothing between `:49`
    and `:125` reads `rhs->val.dt`; the invariant rests entirely on
    `irpass::type_check` having unified the operands upstream, which the in-tree
    comment at `:64` asserts and does not prove. Both round-three adversaries
    examined it, both call the residual real, and both failed to settle
    reachability: `type_check.cpp`'s `cast()` inserts a `UnaryOpStmt` that would
    normally defeat `ConstantFold`'s `is<ConstStmt>` gate, but `ConstantFold`
    also folds unary casts into fresh `ConstStmt`s, and neither traced the fixed
    point. **INFERRED in both directions.** Not a defect today and not a change
    to section 3.2, which stands. A consequence of a width change to weigh.
    **Planner.**

Items 1 through 12, 16 and 17 remain unresolved and are for the planner or the
project owner. Item 16 is open by design: the planner has given the class to a
dedicated pair as its own territory, and this entry is their starting point.
Items 13, 14 and 15 are recorded as closed rather than deleted, with the ruling
on each, so the adversaries can see how their objections were disposed of.
