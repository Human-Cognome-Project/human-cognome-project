# Notes 01 — Frontend IR and type system

Contemporaneous working notes. Explore agent 01.
Territory: `taichi/ir/`, `taichi/inc/`, `taichi/analysis/`, `taichi/transforms/`.
Focus: origin and propagation of index and address types; 32-bit width assumptions.

---

## N001 — Orientation

Read `modernization/PROJECT-PLAN.md` in full. Governing items for me:
- 6.1 parameterise `taichi_max_num_snodes = 1024`.
- 6.2 64-bit addressing end to end; related `taichi_max_num_indices = 12`.
- 8.1(1) map of 32-bit assumptions across frontend IR — that is my deliverable.

Read `taichi/inc/constants.h` in full (78 lines of constants + helpers). Relevant
declarations, verbatim line numbers:

- `taichi/inc/constants.h:5` — `constexpr int taichi_max_num_indices = 12;`
- `taichi/inc/constants.h:7` — `taichi_max_num_args = 8` (comment: legacy, opengl only)
- `taichi/inc/constants.h:10` — `taichi_max_num_args_total = 64`
- `taichi/inc/constants.h:11` — `taichi_max_num_args_extra = 32`
- `taichi/inc/constants.h:12` — `taichi_max_num_snodes = 1024;`
- `taichi/inc/constants.h:13` — `kMaxNumSnodeTreesLlvm = 512;`
- `taichi/inc/constants.h:17` — `taichi_max_num_mem_requests = 1024 * 64`
- `taichi/inc/constants.h:24` — `taichi_listgen_max_element_size = 1024`

Note: both 6.1's constant and `kMaxNumSnodeTreesLlvm` are int; the PLAN cites
`constants.h:12` for the snode ceiling, which matches exactly.

Surprise already: `taichi_max_num_snodes` is `int`, and the plan (6.1) says it
sizes runtime arrays. Need to check whether anything in *my* territory indexes
by snode id and assumes it fits in some narrow width.

---

## N002 — Type system capability (`taichi/ir/type.h`, 790 lines)

Read in full. What exists:

- `PrimitiveTypeID` generated from `taichi/inc/data_type.inc.h`, which lists
  `i64` (line 7) and `u64` (line 12). So **64-bit integer primitive types
  already exist in the type system**. `taichi/ir/type.h:11-15`.
- `PrimitiveType` static `DataType` members are generated for every entry, so
  `PrimitiveType::i64` / `PrimitiveType::u64` are available handles.
  `taichi/ir/type.h:151-155`.
- `PointerType` at `taichi/ir/type.h:177-208`. It holds `Type *pointee_`,
  `int addr_space_{0}  // TODO: make this an enum` (line 206), and
  `bool is_bit_pointer_`. **There is no address width on the pointer type.**
  A `PointerType` is not parameterised by pointer size; width lives entirely in
  whatever integer type the offset arithmetic uses.
- `TensorType` shape is `std::vector<int>`; `get_num_elements()` accumulates
  into an `int` (`taichi/ir/type.h:222-227`). That is a 32-bit element-count
  assumption inside the type system itself.
- `StructType::get_flattened_num_elements()` returns `int`
  (`taichi/ir/type.h:307-321`).
- Offsets in `AbstractDictionaryMember::offset` are `size_t`
  (`taichi/ir/type.h:257`), and `get_element_offset` returns `size_t`
  (`taichi/ir/type.h:243`, `:305`, `:336`). So *byte* offsets within an
  aggregate type are already 64-bit-capable on a 64-bit host.
- `TypedConstant` (`taichi/ir/type.h:549-570`) has a union covering i64/u64,
  and accessors `val_int64()`, `val_uint64()`, `val_as_int64()`
  (`taichi/ir/type.h:661-677`). Constants can therefore already carry 64-bit
  values.

**Preliminary conclusion (to verify against use sites):** the *type system* can
already express 64-bit integers. What it cannot express is a distinguished
"index type" or "address type" — there is no such concept. Index width is
decided at each construction site by whichever `PrimitiveType::i32` literal the
code writes down. That is the thing to inventory.

Dead end noted: I looked for a pointer-width or address-space-width field on
`PointerType`; there is none beyond `addr_space_` which is an unused-ish int
tag (TODO comment says it should be an enum).

---

## N003 — The address/index statements in `taichi/ir/statements.h`

Read the pointer-forming and index-forming statement classes.

- `ExternalPtrStmt` — `taichi/ir/statements.h:369-405`. Fields: `Stmt *base_ptr`,
  `std::vector<Stmt *> indices`, `int ndim` (line 375),
  `std::vector<int> element_shape` (line 378). Indices are `Stmt*` so their width
  is whatever `ret_type` those statements carry. `element_shape` and `ndim` are
  `int`.
- `GlobalPtrStmt` — `taichi/ir/statements.h:417-439`. `SNode *snode`,
  `std::vector<Stmt *> indices`. Again indices are statements, width deferred.
- `MatrixOfGlobalPtrStmt` — `:450-479`. `int dynamic_index_stride{0}` (line 456)
  — a **stride in an int**.
- `MatrixPtrStmt` — `:505-548`. `Stmt *origin`, `Stmt *offset`. Long TODO comment
  at `:512-521` saying `offset` means *bytes* in some paths and *index* in
  others, disambiguated by `offset_used_as_index()` at `:522-530`. Flagging this:
  a width change has to respect both meanings.
- `IntegerOffsetStmt` — `:1257-1272`. `Stmt *input; int64 offset;`. **Already
  int64.** Marked `// TODO: remove this` at `:1256`.
- `LinearizeStmt` — `:1277-1294`. `std::vector<Stmt *> inputs;`
  **`std::vector<int> strides;`** (line 1280). This is *the* index-fusing
  statement — "All indices of an address fused together" (doc comment `:1274`).
  Its strides are 32-bit ints. **This is a hard 32-bit assumption on the central
  linearisation statement.**
- `GetRootStmt` — `:1299-1326`.
- `SNodeLookupStmt` — `:1333-1362`. `SNode *snode; Stmt *input_snode;
  Stmt *input_index; bool activate;`. Index is a `Stmt*`, width deferred to its
  `ret_type`.
- `GetChStmt` — `:1366-1393`. `int chid` (line 1370) — child id within an SNode.

Loop machinery:
- `OffloadedStmt` — `:1395-...`. `std::size_t begin_offset{0}; std::size_t
  end_offset{0};` (`:1404-1405`) but **`int32 begin_value{0}; int32
  end_value{0};`** (`:1408-1409`). So a *constant* range-for bound is stored as
  int32 in the IR, while the *dynamic* offsets into the global tmp buffer are
  size_t. `std::vector<int> index_offsets;` at `:1439`.
- `LoopIndexStmt` — `:1494-1529`. `Stmt *loop; int index;` — `index` here is
  *which* index (the axis), not a value. Its `ret_type` is what matters.
- `LoopLinearIndexStmt` — `:1535-1550`, `GlobalThreadIndexStmt` — `:1556+`.

Surprise: `IntegerOffsetStmt::offset` is already `int64` while `LinearizeStmt`
strides are `int`. Inconsistent widths already present in the IR.

---

## N004 — `taichi/transforms/type_check.cpp` — THE central 32-bit enforcement point

This is where index width is *decided and enforced*, not merely assumed. Read
the whole file (700 lines). Every relevant site:

Index coercion (forces a cast to i32):
- `GlobalPtrStmt` — `taichi/transforms/type_check.cpp:147-156`. Loops over
  `stmt->indices`; if an index's `ret_type` is not `PrimitiveTypeID::i32` it
  emits `TaichiCastWarning` "Field index {} not int32, casting into int32
  implicitly" and calls `insert_type_cast_before(..., PrimitiveType::i32)`
  (line 154). **Any 64-bit SNode index the frontend produces is truncated
  here.**
- `ExternalPtrStmt` — `:457-463`. `TI_ASSERT(is_integral(...))` then, if the
  index is not exactly `PrimitiveType::i32`, inserts a cast to i32 (line 461).
  Same truncation for ndarray/external-array indices.
- `MatrixPtrStmt` — `:159-163`. `TI_ASSERT(stmt->offset->ret_type
  .get_element_type()->is_primitive(PrimitiveTypeID::i32));` — a hard assert,
  not a cast. A 64-bit matrix offset would **crash the compiler**, not silently
  truncate.
- `RangeForStmt` — `:169-173`. `mark_as_if_const(stmt->begin,
  PrimitiveType::i32); mark_as_if_const(stmt->end, PrimitiveType::i32);` —
  range-for bounds pinned to i32.

Hardcoded i32 `ret_type` assignments (the statement's own result width):
- `SNodeOpStmt` default branch — `:119` (`get_addr` is the exception at `:107`,
  which is `PrimitiveType::u64`).
- `ExternalTensorShapeAlongAxisStmt` — `:124`.
- `LoopIndexStmt` — `:466-468`. **`stmt->ret_type = PrimitiveType::i32;`** This
  is the origin of every struct-for / range-for index value in the IR.
- `LoopLinearIndexStmt` — `:470-472`.
- `BlockCornerIndexStmt` — `:474-476`.
- `LinearizeStmt` — `:519-521`. **The fused linear address is i32.**
- `IntegerOffsetStmt` — `:523-525`. Note the *field* `offset` is `int64`
  (`taichi/ir/statements.h:1260`) but the statement's `ret_type` is forced i32
  here. Direct internal inconsistency.
- `InternalFuncStmt` — `:565-568` (with a TODO about return type spec).

Pointer-producing sites that are width-neutral (they build a `PointerType`
around a pointee, no integer width involved):
- `GetRootStmt` — `:477-481` → `get_pointer_type(PrimitiveType::gen)`.
- `SNodeLookupStmt` — `:483-497` → `get_pointer_type(PrimitiveType::gen)` or,
  for `quant_array`, a bit pointer to the element type.
- `GetChStmt` — `:499-517`.

Note the asymmetry that surprised me: `SNodeOpType::get_addr` already yields
`u64` at `:106-108`. So the codebase already admits that a *raw address* is
64-bit, while every *computed* address (Linearize, IntegerOffset) is i32.

---

## N005 — `taichi/ir/snode.h` / `snode.cpp` — the SNode geometry, all 32-bit

`AxisExtractor` — `taichi/ir/snode.h:37-54`. All three shape fields are `int`:
- `int num_elements_from_root{1}` — line 41
- `int shape{1}` — line 45
- `int acc_shape{1}` — line 49

`SNode` — `taichi/ir/snode.h:59-357`:
- `AxisExtractor extractors[taichi_max_num_indices];` — line 78. **Fixed-size
  array dimensioned by `taichi_max_num_indices` (12).**
- `int physical_index_position[taichi_max_num_indices]{};` — line 81. Same.
- `std::vector<int> index_offsets;` — line 79.
- `int num_active_indices{0};` — line 80.
- `static std::atomic<int> counter; int id{0};` — lines 88-89. **SNode ids are
  int, assigned from a global atomic counter** (`snode.cpp:12`, `:220`). This is
  the id that item 6.1's `taichi_max_num_snodes` bounds.
- `int64 num_cells_per_container{1};` — line 97. **Already int64.**
- `std::size_t cell_size_bytes{0}; std::size_t offset_bytes_in_parent_cell{0};`
  — lines 99-100. Byte sizes already size_t.
- `int chunk_size{0}` — line 98.
- `Axis` ctor bounds-checks against `taichi_max_num_indices` —
  `taichi/ir/snode.h:27-31`.
- `int64 max_num_elements()` — `:306-308`, returns `num_cells_per_container`.
- `get_total_num_elements_towards_root()` — `:310-315`. Declared `int64`, but
  the body is `total_num_elemts *= (int)s->max_num_elements();` (line 313).
  **An explicit narrowing cast to `int` inside an int64 accumulation.** Each
  level's cell count is truncated to 32 bits before multiplying.
- `int shape_along_axis(int i)` — `:317`, defined `snode.cpp:174-177`, returns
  `extractor.num_elements_from_root` which is `int`.

**`taichi/ir/snode.cpp:89-100` — the smoking gun for item 6.2:**

```
  int64 acc_shape = 1;
  for (int i = taichi_max_num_indices - 1; i >= 0; i--) {
    // casting to int32 in extractors.
    new_node.extractors[i].acc_shape = static_cast<int>(acc_shape);
    acc_shape *= new_node.extractors[i].shape;
  }
  if (acc_shape > std::numeric_limits<int>::max()) {
    ErrorEmitter(
        TaichiIndexWarning(), &dbg_info,
        "SNode index might be out of int32 boundary but int64 indexing is not "
        "supported yet. Struct fors might not work either.");
  }
```

The accumulator is int64, the stored field is int, the cast is explicit and
commented, and the code *emits a warning saying int64 indexing is unsupported*.
Upstream knows. It is a warning, not an error, so today a large structure
compiles and silently produces truncated `acc_shape` values.

`new_node.num_cells_per_container = acc_shape;` (`snode.cpp:101`) keeps the
untruncated int64. So `num_cells_per_container` and `extractors[i].acc_shape`
disagree above 2^31.

Other `taichi_max_num_indices` loops in this file: `snode.cpp:21-24`
(`insert_children` propagating `num_elements_from_root`), `snode.cpp:90`,
`snode.cpp:105`.

---

## N006 — Index-lowering passes

### `taichi/transforms/scalar_pointer_lowerer.cpp` — the SNode access lowerer

This is where a `GlobalPtrStmt` becomes `GetRootStmt` + per-level
`LinearizeStmt` + `handle_snode_at_level`.

- `:33-34` — `std::array<int, taichi_max_num_indices> total_shape; total_shape
  .fill(1);` **32-bit accumulator array, dimensioned by
  `taichi_max_num_indices`.**
- `:35-39` — `total_shape[j] *= s->extractors[j].shape;` accumulated over every
  SNode on the root-to-leaf path, in `int`. Overflows silently past 2^31.
- `:40` — `std::array<bool, taichi_max_num_indices> is_first_extraction;`
- `:56` — `std::vector<int> strides;` fed to `LinearizeStmt`.
- `:63-65` — `const int prev = total_shape[k]; total_shape[k] /=
  snode->extractors[k].shape; const int next = total_shape[k];` — all int.
- `:73` — `generate_mod(lowered_, indices_[k_], prev)`
- `:75` — `generate_div(lowered_, extracted, next)`
- `:78` — `strides.push_back(snode->extractors[k].shape);`
- `:81-82` — `LinearizeStmt(lowered_indices, strides)`.

### `taichi/transforms/utils.cpp` — the divisor/mask constant generators

- `generate_mod(VecStatement*, Stmt *x, int y)` — `taichi/transforms/utils.cpp:5-12`,
  declared `taichi/transforms/utils.h:8`. **Takes `int y`.** Power-of-two path
  emits `ConstStmt(TypedConstant(y - 1))` — the `int32` ctor of `TypedConstant`
  (`taichi/ir/type.h:592`), so an **i32 mask**. Non-power-of-two path emits
  `TypedConstant(y)`, also i32, and a `BinaryOpType::mod`.
- `generate_div(VecStatement*, Stmt *x, int y)` — `taichi/transforms/utils.cpp:14-22`,
  declared `taichi/transforms/utils.h:9`. Power-of-two path emits
  `TypedConstant(PrimitiveType::i32, bit::log2int(y))` — **explicit i32** at
  `:17` — and a `bit_shr`. Non-power-of-two emits `TypedConstant(y)` and a `div`.

So both helpers hardcode i32 in their signature *and* in the constants they
emit. Every index extraction in both lowering paths funnels through these two
functions.

### `taichi/transforms/demote_dense_struct_fors.cpp`

- `:18` — `int64 total_n = 1;`
- `:19-20` — `std::array<int, taichi_max_num_indices> total_shape;` — 32-bit
  again.
- `:24` — `total_shape[j] *= snode->extractors[j].shape;`
- `:26` — `total_n *= snode->num_cells_per_container;` (int64, correct)
- `:29` — **`TI_ASSERT(total_n <= std::numeric_limits<int>::max());`** A hard
  assert. A fully-dense struct-for over more than 2^31 cells **aborts the
  compiler** rather than truncating.
- `:35` — `offloaded->end_value = total_n;` — assigns `int64` into the `int32`
  field `OffloadedStmt::end_value` (`taichi/ir/statements.h:1416`). The assert
  at `:29` is what stops this being a silent truncation.
- `:60` — `generate_mod(&body_header, extracted, total_n)` — passes an `int64`
  into an `int` parameter. Implicit narrowing, again guarded only by `:29`.
- `:62-63` — `total_n /= snode->num_cells_per_container;` then
  `generate_div(&body_header, extracted, total_n)` — same narrowing.
- `:74` — `generate_mod(&body_header, index, ext.acc_shape * ext.shape)` —
  `int * int`, **32-bit multiply that can overflow before the call**.
- `:76` — `generate_div(&body_header, index, ext.acc_shape)`
- `:77-79` — `total_shape[p] /= ext.shape;` then
  `ConstStmt(TypedConstant(total_shape[p]))` — i32 multiplier.
- `:49` — `ConstStmt(TypedConstant(0))` for the initial loop vars — i32 zero,
  so the accumulated loop variable is i32 throughout.

Surprise: this pass hard-asserts at 2^31 while `scalar_pointer_lowerer.cpp`
silently overflows its `total_shape` array. Two lowering paths, two different
failure modes for the same overflow.

---

## N007 — Frontend expression layer (`taichi/ir/frontend_ir.h` / `.cpp`)

Where a user index becomes IR. The frontend is *more* permissive than the IR
type checker.

- `IndexExpression::type_check` — `taichi/ir/frontend_ir.cpp:870-948`. The index
  check at `:938-947` only requires `is_integral(expr_type)`; an i64 index
  passes the frontend cleanly. It is `type_check.cpp:147-156` / `:457-463` that
  later truncates it. So **there is no frontend barrier; the barrier is the IR
  type-check pass.** Worth stating plainly because it means the frontend needs
  no change to *accept* 64-bit indices.
- `field_validation` — `:858-869`, checks `snode->num_active_indices` against
  index count only.
- `make_index_stmts` — `:657-670`. Applies `snode->index_offsets` as
  `ConstStmt(TypedConstant(offsets[i]))` at `:663` — `offsets` is
  `std::vector<int>` (`taichi/ir/snode.h:79`), so an **i32 const**, then
  `BinaryOpType::sub` against the user index. Width of the result follows binary
  op promotion.
- `make_field_access` — `:672-677` → constructs `GlobalPtrStmt`.
- `make_matrix_field_access` — `:679-693` → `MatrixOfGlobalPtrStmt`, passing
  `matrix_field.dynamic_index_stride` which is `int`
  (`taichi/ir/frontend_ir.h:616`).
- `make_ndarray_access` — `:695-717` → `ExternalPtrStmt`, with
  `expr->dt.get_shape()` (`std::vector<int>`) as element shape.
- `make_tensor_access_single_element` — `:719-749`.
  - Dynamic path `:730-741`: `ConstStmt(TypedConstant(0))` seed at `:731` (i32),
    `ConstStmt(TypedConstant(shape[i]))` at `:735` (i32), then mul/add. **The
    whole tensor offset chain is i32.**
  - Constant path `:742-748`: `int offset = 0; offset = offset * shape[i] +
    indices[i]...val_int();` — accumulated in a plain `int` at `:744-746`, then
    `ConstStmt(TypedConstant(offset))` at `:747`, i32.
  - Result feeds `MatrixPtrStmt` at `:748`, which `type_check.cpp:160-161`
    hard-asserts is i32.
- `ExternalTensorExpression` — `taichi/ir/frontend_ir.h:526-582`. `int ndim`
  (line 529), `std::vector<int> arg_id` (line 530).
- `MatrixFieldExpression` — `taichi/ir/frontend_ir.h:612-638`.
  `std::vector<int> element_shape` (line 615), `int dynamic_index_stride{0}`
  (line 617).
- `IndexExpression` — `taichi/ir/frontend_ir.h:667-707`.
  `std::vector<int> ret_shape` (line 675).
- `RangeAssumptionExpression` — `taichi/ir/frontend_ir.h:709-727`,
  `int low, high` (line 713).
- `SNodeOpExpression::type_check` — `frontend_ir.cpp:1108-1132`. Mirrors the IR
  pass: `get_addr` → `u64` (`:1110`), `is_active` → `u1` (`:1112`), else `i32`
  (`:1114`).
- `SNodeOpExpression::flatten` — `:1134-1176`. `append` allocates an
  `AllocaStmt(PrimitiveType::i32)` at `:1160` to receive the allocated index of
  a dynamic node. **Dynamic-SNode append returns an i32 index.**
- `ExternalTensorShapeAlongAxisExpression` — `frontend_ir.cpp:1306` sets
  `ret_type = PrimitiveType::i32`. Array extents reported to the kernel as i32.
- `MeshPatchIndexExpression::type_check` — `:1357-1359`, i32.
- `MeshRelationAccessExpression` — `:1362`, i32.
- `MeshIndexConversionExpression` — `:1392`, i32.
- `frontend_ir.cpp:152` — `get_pointer_type(PrimitiveType::i32)` (in the
  external-func / arg handling region; noted for completeness).
- `frontend_ir.cpp:1216`, `:1233`, `:1249` — `if (arg_type != PrimitiveType::i32)`
  guards in the texture-op path.
- `taichi/ir/expr.cpp:93` — `ConstExpression(PrimitiveType::i32, x)`: the
  default int literal in the frontend is i32.

---

## N008 — Range-for bounds: `type_check.cpp` + `offload.cpp`

Chased the range-for bound path because `OffloadedStmt::begin_value/end_value`
are `int32` (`taichi/ir/statements.h:1415-1416`).

- `TypeCheck::mark_as_if_const(Stmt *stmt, DataType t)` —
  `taichi/transforms/type_check.cpp:39-43`:
  ```
  if (stmt->is<ConstStmt>()) {
    stmt->ret_type = t;
  }
  ```
  It **relabels the `ret_type` of a `ConstStmt` without touching the stored
  value bits**. `TypedConstant` is a union (`taichi/ir/type.h:556-570`), so
  retagging an i64 constant as i32 makes subsequent reads take `val_i32`, i.e.
  the low 32 bits. Called for `RangeForStmt` begin and end at
  `type_check.cpp:170-171` with `PrimitiveType::i32`.
- `taichi/transforms/offload.cpp:107-118` then reads those constants with
  `val->val.val_int32()` (`:109` for begin, `:117` for end) into the int32
  `begin_value` / `end_value`.
- `val_int32()` — `taichi/ir/type.cpp:462-465` — asserts
  `get_data_type<int32>() == dt` and returns `val_i32`. The assert passes only
  *because* `mark_as_if_const` already retagged the constant to i32.

So: a 64-bit constant range-for bound is silently re-tagged and truncated to its
low 32 bits. Verified by reading all three sites; I did not run it.

- Non-constant bounds take the other path: `offload.cpp:501-525` stores
  `begin_offset` / `end_offset` as `std::size_t` offsets into the global tmp
  buffer (`taichi/ir/statements.h:1411-1412`). The *offset* is 64-bit-safe; the
  *value* stored at that offset is whatever width the producing statement had.
- `taichi/transforms/make_cpu_multithreaded_range_for.cpp:68-93` rebuilds the
  range on CPU. Every constant there is explicitly `PrimitiveType::i32`:
  `:68` (1), `:70` (512), `:72` (`cpu_max_num_threads`), `:81`
  (`offloaded->begin_value`), `:84` (`GlobalTemporaryStmt(begin_offset,
  PrimitiveType::i32)`), `:90` (`end_value`), `:93` (`GlobalTemporaryStmt(
  end_offset, PrimitiveType::i32)`). **The dynamic bound is read out of the
  global temporary buffer as i32.** That is a second, independent 32-bit
  narrowing on the dynamic path.

---

## N009 — `taichi/transforms/check_out_of_bound.cpp` — bounds checks are i32

Every bound constant in this pass is an i32 `TypedConstant`, and the comparison
is against the (already-truncated) index.

`ExternalPtrStmt` — `:43-98`:
- `:47` `ConstStmt(TypedConstant(0))` — i32 zero.
- `:54-57` `int flattened_element = 1; flattened_element *= stmt->element_shape[i];`
  — **32-bit product of the element shape.**
- `:68-70` upper bound from `ExternalTensorShapeAlongAxisStmt`, whose ret_type is
  forced i32 at `type_check.cpp:123-125`.
- `:74` `ConstStmt(TypedConstant(flattened_element))` — i32.
- `:91` message uses `"%d"` per index — a **printf format assuming 32-bit int**
  for the reported index values. Same at `:151-155` for GlobalPtrStmt and `:199`
  for MatrixPtrStmt.

`GlobalPtrStmt` — `:100-160`:
- `:123` `int size_i = snode->shape_along_axis(i);` — `shape_along_axis` returns
  `int` (`taichi/ir/snode.cpp:174-177`, from `num_elements_from_root`).
- `:124-126` `int upper_bound_i = size_i;` → `ConstStmt(TypedConstant(...))`, i32.
- `:115` `int offset_i = has_offset ? snode->index_offsets[i] : 0;` and `:142`
  `ConstStmt(TypedConstant(offset_i))` — i32.

`MatrixPtrStmt` — `:165-205`:
- `:169-173` `int max_valid_index = 1; max_valid_index *= matrix_shape[i];`
  — 32-bit product over the tensor shape.
- `:182` `ConstStmt(TypedConstant(max_valid_index))` — i32.

`BinaryOpStmt` pow guard — `:207-230`, `:220` sets `compare->ret_type =
PrimitiveType::i32`.

Consequence I want to flag rather than resolve: if index width is widened, the
bounds check would compare a 64-bit index against a 32-bit bound constant, and
the assert message format strings (`%d`) would also need attention. Recording,
not proposing.

---

## N010 — Analysis layer

### `taichi/ir/analysis.h` — `DiffRange`

`taichi/ir/analysis.h:15-51`. Fields `int coeff; int low, high;`
(`:20-21`). `taichi/analysis/value_diff.cpp:71-76` assigns
`range_for->begin->as<ConstStmt>()->val.val_int()` (which returns `int64`,
`taichi/ir/type.cpp:522`) into these `int` fields. **int64 → int narrowing in
the loop-index difference analysis.** Consumed by
`taichi/transforms/lower_access.cpp:236-249` to decide `on_loop_tree`, and by
the BLS / scratch-pad passes.

Arithmetic on `DiffRange` at `taichi/analysis/value_diff.cpp:10-37` is all int:
`operator+` `:10-13`, `operator-` `:15-18`, `operator*` `:20-31`,
`operator<<` `:33-37`.

### `taichi/ir/scratch_pad.h` — block-local scratch pad geometry

All int:
- `BoundRange { int low{0}; int high{0}; }` — `:33-42`.
- `std::vector<int> coefficients; std::vector<BoundRange> bounds;
  std::vector<int> pad_size; std::vector<int> block_size;` — `:47-53`.
- `int dim;` — `:55`.
- `std::vector<std::pair<std::vector<int>, AccessFlag>> accesses;` — `:60`.
- `access(const std::vector<int> &coeffs, const std::vector<int> &indices, ...)`
  — `:81-94`.
- `finalize()` — `:96-118`, `int size = 1; size *= pad_size[i];` at `:97-100`.
- `pad_size_linear()` — `:130-137`, int product.
- `block_size_linear()` — `:139-146`, int product.
- `linearized_index(const std::vector<int> &indices)` — `:148-156`, returns
  `int`. **Another linearisation, entirely 32-bit.**
- `extract_offset(std::string var, int d)` — `:158-165`, builds the divisor as
  `int div`.
- `div_floor(int a, int b)` — `:180-182`.
- `ScratchPads::generate_address_code` — `:225-238`, `int offset = 0;`.

Note: block-local scratch pads are per-block, so the values are small in
practice. Recording the widths for completeness; I am not judging whether they
matter.

### `taichi/analysis/offline_cache_util.cpp`

`:110-115` loops `for (int i = 0; i < taichi_max_num_indices; ++i)` and
serialises each `extractor.num_elements_from_root`, `.shape`, `.acc_shape`,
`.active` into the offline cache key. `:118` serialises
`snode->physical_index_position`, `:120` `snode->id`,
`:123` `snode->num_cells_per_container`. **The cache key is sensitive to both
`taichi_max_num_indices` and the width of these fields** — changing either
invalidates cache keys and, if the loop bound changes, changes the key length.

---

## N011 — Block-local storage passes

### `taichi/transforms/make_block_local.cpp`

- `:57` — `std::size_t bls_offset_in_bytes = 0;` (accumulated in size_t) but
  narrowed at **`:157`** and **`:328`**:
  `ConstStmt(TypedConstant((int32)bls_offset_in_bytes))`. **Explicit `(int32)`
  cast of a size_t byte offset into an i32 constant.** Two sites.
- `:73` — `const auto dim = (int)pad.second.pad_size.size();`
- `:76` — `const auto bls_num_elements = pad.second.pad_size_linear();` (int,
  from `scratch_pad.h:130`)
- `:78-88` — `std::vector<int> block_strides(dim); std::vector<int>
  bls_strides(dim);` and the stride products at `:86-87` — all int.
- `:141-142` — `int loop_offset = 0; const int block_dim = offload->block_dim;`
- `:144-145`, `:152`, `:167`, `:183`, `:193`, `:201`, `:274`, `:283`, `:298`,
  `:303`, `:317`, `:322` — every `ConstStmt(TypedConstant(...))` here takes an
  `int` and so emits an i32 constant. Covers pad sizes, bounds lows,
  coefficients, dtype_size, bls_strides and the bls_num_elements guard.

BLS offsets are per-block and small, but they are computed against the *global*
index (`:265-275`, `inc = global_indices[i] - block_corner - bounds[i].low`),
so the operand widths still interact with the global index width.

### `taichi/transforms/handle_external_ptr_boundary.cpp`

- `:30`, `:39`, `:71` — i32 `TypedConstant(0)` / `(1)`.
- `:62-67` — `int max_valid_index = 1; max_valid_index *= matrix_shape[i];` —
  32-bit product; `:78` emits it as an i32 const.
- `:36-38` — clamp upper bound from `ExternalTensorShapeAlongAxisStmt`, i32 by
  `type_check.cpp:123-125`.

### `taichi/transforms/make_mesh_block_local.cpp`, `make_mesh_thread_local.cpp`,
### `demote_mesh_statements.cpp`

Mesh path, all explicitly i32:
- `make_mesh_block_local.cpp:212`, `:376`, `:399` — `cast_type =
  PrimitiveType::i32`; `:526` — `mapping_data_type_ = PrimitiveType::i32`.
- `make_mesh_thread_local.cpp:42` — `TypedConstant(PrimitiveType::i32, 1)`;
  `:106`, `:114` — `cast_type = PrimitiveType::i32`.
- `demote_mesh_statements.cpp:16`, `:54`, `:131` — `cast_type =
  PrimitiveType::i32`.
- `taichi/ir/statements.h:2064` (`MeshIndexConversionStmt`) and `:2083`
  (`MeshPatchIndexStmt`) set `ret_type = PrimitiveType::i32` in the ctor.

Mesh is not obviously in the path of section 6, but it is inside my territory
and it is index arithmetic, so it is inventoried. Whether mesh matters is not
mine to decide — see Escalations.

---

## N012 — `taichi/transforms/simplify.cpp` — where LinearizeStmt actually becomes arithmetic

This is the second-most important file after `type_check.cpp`, and I nearly
missed it. `LinearizeStmt` is not lowered by codegen alone; `simplify` expands
it into adds and muls.

`visit(LinearizeStmt *)` — `taichi/transforms/simplify.cpp:160-220`:
- `:161-172` — offset push-forward through `IntegerOffsetStmt`.
- `:175` — `auto sum = Stmt::make<ConstStmt>(TypedConstant(0));` — **i32 zero
  seeds the accumulator.**
- `:176` — **`auto stride_product = 1;`** — `auto` deduces `int`. This is the
  running stride.
- `:178` — `Stmt::make<ConstStmt>(TypedConstant(stride_product))` — i32 stride
  constant.
- `:179-186` — `mul` then `add` into `sum`.
- `:187` — **`stride_product *= stmt->strides[i];`** — 32-bit accumulation over
  the SNode path strides. Overflows silently.
- `:189-215` — debug-only overflow guard. Verbatim comment at `:189-190`:
  *"Compare the result with 0 to make sure no overflow occurs under Debug
  Mode."* It emits `cmp_ge sum, 0`, an `AssertStmt` with the message
  **"The indices provided are too big!"** (`:198`), and a `select` that
  substitutes 0 for a negative sum so the kernel does not fault (`:200-205`
  comment explains this). **A second explicit in-source admission that the
  linear index is 32-bit and overflows.** Note this guard only exists when
  `config.debug` is true (`:191`); in release the overflow is entirely silent.

`visit(SNodeLookupStmt *)` — `:222-249`:
- `:233-238` — asserts every child of the SNode is `i32` or `f32`.
- `:238-239` — **`IntegerOffsetStmt(stmt, previous_offset->offset *
  sizeof(int32) * (snode->ch.size()))`**. The element size is **hardcoded as
  `sizeof(int32)`**, i.e. 4 bytes, not derived from the SNode's actual dtype.

`visit(GetChStmt *)` — `:251-274`:
- `:260-261` — **`IntegerOffsetStmt(stmt, stmt->chid * sizeof(int32) +
  previous_offset->offset)`**. Again `sizeof(int32)` hardcoded as the child
  stride.

`visit(IntegerOffsetStmt *)` — `:129-134`, folds a zero offset away.

These two `sizeof(int32)` sites are the clearest examples of a width that is
**hardcoded rather than derived** in my territory. They are guarded by the
i32/f32 assert at `:233-237` in the SNodeLookup case, but `GetChStmt` at `:261`
has no such guard in view.

---

## N013 — Remaining passes swept

### `taichi/transforms/lower_matrix_ptr.cpp`
- `:189`, `:232`, `:424`, `:477` — `ConstStmt(TypedConstant(i))` where `i` is an
  `int` element index. i32 matrix element indices, consistent with the
  `MatrixPtrStmt` i32 assert at `type_check.cpp:160-161`.
- `:178`, `:221`, `:302`, `:334`, `:411`, `:464` — `int num_elements =
  ...->get_num_elements();` — `TensorType::get_num_elements()` returns `int`
  (`taichi/ir/type.h:222-227`).
- `:536-540` — `ConstStmt(TypedConstant(origin->dynamic_index_stride))` (i32,
  from `MatrixOfGlobalPtrStmt::dynamic_index_stride`, `taichi/ir/statements.h:456`),
  multiplied into the offset. `:540` copies `stmt->offset->ret_type` onto the
  product, so width follows the offset.

### `taichi/transforms/scalarize.cpp`
- `:471`, `:473` — explicit `TypedConstant(PrimitiveType::i32, 0/1)`.
- `:1119-1122` and `:1146-1149` — folds `MatrixPtrStmt` into
  `GlobalTemporaryStmt` / `ThreadLocalPtrStmt`. Reads the offset with
  `val_int32()` (`:1119`, `:1146`) but computes the byte size with
  **`data_type_size(...)`** — i.e. *derived*, not hardcoded. Good contrast with
  `simplify.cpp:239` / `:261`. `GlobalTemporaryStmt::offset` and
  `ThreadLocalPtrStmt::offset` are both `std::size_t`
  (`taichi/ir/statements.h:1597`, `:1618`), so the resulting `new_offset` is
  64-bit; only the element index is i32.

### `taichi/transforms/constant_fold.cpp` — a positive finding
`is_good_type` at `:33-47` explicitly admits `i64` (`:38`) and `u64` (`:42`).
The fold macros dispatch on `i32` vs everything-integral, using `val_int()`
(int64) for the general case (`:80`, `:118`, `:206`, `:235`, `:255`) and
`val_int32()` only for the i32-specific branch (`:115`). **Constant folding is
already 64-bit correct.**

### `taichi/ir/ir_builder.{h,cpp}` — a positive finding
`get_int32` / `get_int64` / `get_uint32` / `get_uint64` all exist
(`taichi/ir/ir_builder.h:133-136`, `taichi/ir/ir_builder.cpp:141-162`).
`create_global_ptr` (`:440-444`) and `create_external_ptr` (`:446-452`) take
`std::vector<Stmt *>` indices with no width constraint of their own. The builder
imposes no i32; the constraint is entirely in `type_check.cpp`.

### `taichi/transforms/make_mesh_block_local.cpp`
Three more `(int32)` narrowings of a size_t byte offset:
`:80`, `:200`, `:575` — `TypedConstant{(int32)mapping_bls_offset_in_bytes_}`.

### Statement offset field widths (`taichi/ir/statements.h`)
- `GlobalTemporaryStmt::offset` — `std::size_t`, `:1597`.
- `ThreadLocalPtrStmt::offset` — `std::size_t`, `:1618`.
- `BlockLocalPtrStmt::offset` — `Stmt *`, `:1639` (width follows the statement).
- `AdStackAllocaStmt::max_size` — `std::size_t`, `:1769`;
  `size_in_bytes()` at `:1786-1788` is `sizeof(int32) + entry_size_in_bytes() *
  max_size` — the `sizeof(int32)` there is the stack's own size counter, not an
  index; noting it so a reviewer does not mistake it for one.
- `ArgLoadStmt::arg_depth` — `int`, `:201`.

---

## N014 — Type utilities and factory

### `taichi/ir/type_utils.{h,cpp}` — mostly width-agnostic
- `data_type_size(DataType)` — `taichi/ir/type_utils.cpp:30-70`. Derives size
  from the type via `sizeof(...)`, including `REGISTER_DATA_TYPE(i64, int64)`
  at `:59` and `(u64, uint64)` at `:64`. **Derived, not hardcoded.** It returns
  `int`, which is fine for element sizes.
- `data_type_bits` — `taichi/ir/type_utils.h:19-21`.
- `is_integral` — `taichi/ir/type_utils.h:103-113`, includes `i64` (`:107`) and
  `u64` (`:112`).
- `is_signed` — `:115-124`, includes `i64` (`:123`).
- `get_data_type<T>()` — `:28-...`, has `int64 → PrimitiveType::i64` at `:40-41`
  and `uint64 → u64`.
- `data_type_format` — `taichi/ir/type_utils.cpp:112-155`. Has `%lld` for i64
  (`:132`) and `%lu`/`%llu` for u64 (`:136`), with a comment at `:131` noting
  **"Vulkan does not support printing 64-bit signed integer"**. Relevant to the
  SPIR-V int64 capability note in plan section 6.2.
- `data_type_shape` — `:21-28`, returns `std::vector<int>`.
- `align_up(size_t, size_t)` — `taichi/ir/type_utils.h:23-25`, 64-bit clean.

### `taichi/ir/type_factory.cpp`
- `get_primitive_int_type(int bits, bool is_signed)` — `:160-177`. **Already
  supports `bits == 64`** (`:168-169`). Errors on anything else (`:171`).
- `get_ndarray_struct_type(DataType dt, int ndim, bool needs_grad)` —
  `:198-214`. **The ndarray shape struct members are hardcoded
  `PrimitiveType::i32`** at `:204`:
  `shape_members.push_back({PrimitiveType::i32, fmt::format("dim_{}", i)});`
  This is the type of the shape fields that the runtime hands to the kernel and
  that `ExternalTensorShapeAlongAxisStmt` reads. **A hardcoded 32-bit width in
  the type system's own construction of the ndarray descriptor.**
- `get_rwtexture_struct_type()` — `:216-218`, calls the above with ndim 3.
- `compare_types` / `promoted_primitive_type` / `promoted_type` — `:221-286`.
  Promotion is by `data_type_bits` (`:231`, `:235-236`), so **i32 + i64
  correctly promotes to i64**. This matters: mixed-width index arithmetic will
  promote up on its own; the truncation happens only at the explicit cast sites
  in `type_check.cpp`.

---

## N015 — Alias analysis is keyed on i32 constants

`taichi/analysis/value_diff.cpp`, class `FindDirectValueBaseAndOffset`:
- `using ret_type = std::tuple<bool, Stmt *, int>;` — the offset component is
  `int`.
- `visit(ConstStmt *stmt)`: **`if (stmt->val.dt->is_primitive(
  PrimitiveTypeID::i32)) { result = std::make_tuple(true, nullptr,
  stmt->val.val_i32); }`** — it recognises *only* i32 constants. An i64 constant
  falls through to `false` (no decomposition found).
- `DiffPtrResult` — `taichi/ir/analysis.h:180-193`, `int diff_range{0}` at
  `:185`, `make_certain(int diff)` at `:187`.
- `value_diff_ptr_index` — `taichi/analysis/value_diff.cpp` (end of file), builds
  the certain/uncertain answer from the above.

Consumers: `taichi/analysis/alias_analysis.cpp:141-149` (ExternalPtrStmt index
comparison) and `:175-...` (GlobalPtrStmt index comparison), plus
`:100-113` for GlobalTemporaryStmt / ThreadLocalPtrStmt offset equality and
`:115-123` for BlockLocalPtrStmt.

**Consequence worth flagging (not resolving):** if index constants become i64,
this analysis stops recognising them, every index comparison becomes
`AliasResult::uncertain`, and the optimisations that depend on it (dead store
elimination, CSE, load elimination via `taichi/ir/control_flow_graph.cpp`) stop
firing. That is a *performance* regression, not a correctness one, but plan
section 6.4 says packing density and dispatch throughput are the sole criterion,
so it is material. Recorded; not proposing a fix.

Also `taichi/analysis/same_statements.cpp:140-141` compares
`GlobalPtrStmt::snode->id`, and `taichi/analysis/alias_analysis.cpp:154-161`
uses `snode->id` / `output_snode->id` as the disambiguator, returning `int` with
`-1` as the sentinel. `taichi/ir/ir.cpp:81-85` `StmtFieldSNode::get_snode_id`
does the same with `-1`. **SNode ids are `int` with `-1` reserved throughout my
territory.** Relevant to item 6.1: raising `taichi_max_num_snodes` does not by
itself break these, since `int` holds far more than 1024, but the `-1` sentinel
convention is worth noting.

---

## N016 — Sweep of remaining passes

- `taichi/transforms/alg_simp.cpp` — width-agnostic. `get_log2rhs` at `:114-146`
  reads via `val_as_int64()` (`:118`, `:132`) and rebuilds the constant with
  `stmt->lhs->ret_type` / `scalar_stmt->ret_type`, i.e. **derived from the
  operand type**. Positive finding.
- `taichi/transforms/bit_loop_vectorize.cpp:311-325` —
  `int32 get_constant_value(Stmt *stmt)` only recognises
  `PrimitiveTypeID::i32` constants (`:321`), returning `-1` otherwise. Used at
  `:180` and `:202`. Same shape of problem as `value_diff.cpp`: an i64 constant
  is invisible to it. Also `:76` `TypedConstant(vectorization_width)` (i32).
- `taichi/transforms/demote_operations.cpp:99` `TypedConstant(i)` and `:134`
  `TypedConstant(0)` — i32.
- `taichi/analysis/bls_analyzer.{h,cpp}` — `IndexRange { int low; int high; }`
  (`bls_analyzer.h:15-18`); `generate_block_indices` at `bls_analyzer.cpp:23-29`
  uses `snode->extractors[j].shape - 1` (int); `record_access` at `:31-77` uses
  `std::vector<int> coeffs` (`:44`), `std::vector<int> index(num_indices, 0)`
  (`:63`), and reads `diff.low/high/coeff` from `DiffRange` (all int).
- `taichi/analysis/gather_uniquely_accessed_pointers.cpp` — `std::vector<int>
  loop_indices` at `:132`, `:160`; keyed maps on `std::vector<int>` at `:198`,
  `:332`, `:430`. These are *axis* indices, not index values.
- `taichi/analysis/verify.cpp` — no width-relevant content; grep for
  `i32|int32|Linearize|GlobalPtr|indices` returned nothing.
- `taichi/transforms/loop_invariant_code_motion.cpp`,
  `extract_constant.cpp`, `determine_ad_stack_size.cpp`,
  `cache_loop_invariant_global_vars.cpp`, `flag_access.cpp` — no i32/TypedConstant
  hits. Clean.
- `taichi/ir/ir.h:400-402` — `Stmt::instance_id` and `Stmt::id` are `int`, from a
  global atomic (`:400`). `taichi/ir/ir.h:83` — `Identifier::id` is `int`. These
  are compiler-internal counters, not addresses.
- `taichi/transforms/optimize_bit_struct_stores.cpp:37` uses
  `output_snode->id_in_bit_struct` (`int`, `taichi/ir/snode.h:110`).

---

## N017 — Pass ordering (`taichi/transforms/compile_to_offloads.cpp`)

Ordering matters because the truncation sites fire early and repeatedly.

- `:43` `frontend_type_check` — `taichi/transforms/frontend_type_check.cpp:71-78`
  visits `FrontendSNodeOpStmt` and warns "Field index {} not int32, casting into
  int32 implicitly" if an index is not i32. **It only warns here; it does not
  cast.** The cast is `type_check.cpp:154`.
- `:44` `lower_ast`
- `:57` `irpass::type_check` — **first place indices are actually cast to i32.**
- `:66`, `:193`, `:200`, `:208` — `type_check` runs again after
  `bit_loop_vectorize`, `demote_dense_struct_fors`,
  `make_cpu_multithreaded_range_for` and `demote_no_access_mesh_fors`. Also
  invoked from `check_out_of_bound.cpp:...run()` and
  `handle_external_ptr_boundary.cpp:100`, and from `simplify.cpp:219`
  (`modifier.type_check(stmt->parent, config)` after LinearizeStmt expansion).
  **So the i32 coercion is re-applied at many points; it is not a one-shot.**
- `:72` `lower_matrix_ptr`, `:76` `scalarize`
- `:82`, `:111`, `:116`, `:133`, `:147`, `:224`, `:258`, `:278` `full_simplify` —
  each one runs the `LinearizeStmt` expansion in `simplify.cpp:160-220`.
- `:89` `handle_external_ptr_boundary`
- `:124` `check_out_of_bound`
- `:139` `offload` — where `begin_value`/`end_value` are read as int32.
- `:192` `demote_dense_struct_fors` — the `TI_ASSERT(total_n <= INT_MAX)`.
- `:199` `make_cpu_multithreaded_range_for`
- `:232` `make_block_local`
- `:262` `lower_access` — where `ScalarPointerLowerer` runs and emits the
  `LinearizeStmt`s.

Note the interleaving: `lower_access` (`:262`) emits `LinearizeStmt`s *after*
several `full_simplify` rounds, and `:278` runs `full_simplify` again, which is
what expands them. Verified by reading the file; I did not instrument a run.

## N018 — Seam with agent 02 (codegen), recorded for cross-checking only

`LinearizeStmt` is **not** always eliminated by `simplify`; both codegen paths
implement it directly:
- `taichi/codegen/llvm/codegen_llvm.cpp:1736-1744` — `tlctx->get_constant(0)`
  seed, `tlctx->get_constant(stmt->strides[i])` for each stride (the strides are
  the `std::vector<int>` from `taichi/ir/statements.h:1280`).
- `taichi/codegen/spirv/spirv_codegen.cpp:532-541` — `ir_->const_i32_zero_` seed
  and `ir_->int_immediate_number(ir_->i32_type(), stmt->strides[i])`. **SPIR-V
  linearisation is explicitly i32.**
- `taichi/codegen/llvm/codegen_llvm.cpp:1746` —
  `visit(IntegerOffsetStmt *stmt){TI_NOT_IMPLEMENTED}`. IntegerOffsetStmt never
  reaches LLVM codegen.

This is agent 02's territory. I record it only because it shows the width chosen
in `statements.h` propagates verbatim into both backends.

---

## N019 — Line-number verification pass

Re-checked every line number I had written by re-reading the exact lines. Three
corrections to my earlier entries; the corrected values are what go in the
report.

- `PointerType` private members are at `taichi/ir/type.h:206-208`, not `:204-207`:
  `:206` `Type *pointee_{nullptr};`, **`:207` `int addr_space_{0};  // TODO:
  make this an enum`**, `:208` `bool is_bit_pointer_{false};`.
- `OffloadedStmt` starts at `taichi/ir/statements.h:1403`. The range fields are
  **`:1411` `std::size_t begin_offset{0};`, `:1412` `std::size_t end_offset{0};`,
  `:1415` `int32 begin_value{0};`, `:1416` `int32 end_value{0};`** — not
  `:1404-1409` as I first wrote in N003/N008.
- `ExternalPtrStmt::ndim` is `taichi/ir/statements.h:376`, `element_shape` is
  `:379`.
- `type_check.cpp` visitor entry lines: `SNodeOpStmt` `:105`,
  `ExternalTensorShapeAlongAxisStmt` `:118`, `GlobalPtrStmt` `:122`,
  `MatrixPtrStmt` `:159`, `RangeForStmt` `:169`, `ExternalPtrStmt` `:444`,
  `LoopIndexStmt` `:466`, `LoopLinearIndexStmt` `:470`, `BlockCornerIndexStmt`
  `:474`, `LinearizeStmt` `:520`, `IntegerOffsetStmt` `:524`, `InternalFuncStmt`
  `:565`. The `PrimitiveType::i32` assignment lines I cited (`:119`, `:154`,
  `:161`, `:170`, `:171`, `:459`, `:461`, `:467`, `:471`, `:475`, `:521`,
  `:525`) all re-verified correct.
- `taichi/ir/type.h:222-227` `TensorType::get_num_elements()` — re-verified,
  `int num_elements = 1;` accumulation.
- `taichi/ir/snode.h:41/45/49/78/79/81/97/313` — all re-verified verbatim.

## N020 — Cross-reference: the same warning outside my territory

`grep -rn TaichiIndexWarning taichi/` gives exactly three emission sites:
- `taichi/ir/snode.cpp:97` — mine, "SNode index might be out of int32 boundary
  but int64 indexing is not supported yet. Struct fors might not work either."
- `taichi/program/ndarray.cpp:57` and `taichi/program/ndarray.cpp:100` — outside
  my territory, "Ndarray index might be out of int32 boundary but int64 indexing
  is not supported yet." Both guard
  `total_num_scalar > std::numeric_limits<int>::max()` where `total_num_scalar`
  is accumulated with `1LL` (`ndarray.cpp:52-55`, `:94-96`).

Recording the two out-of-territory sites so the adversaries and agent 03/04 can
join them up. I did not investigate `taichi/program/` further.

---

## N021 — Second line-number verification pass (corrections applied)

Re-grepped every `taichi/ir/type.h` and `taichi/ir/statements.h` citation
against the actual file. Corrections applied to both this file and the report:

| Was | Is | Content |
|---|---|---|
| `type.h:146-149` | `taichi/ir/type.h:151-155` | `PrimitiveType` class open + `#define PER_TYPE(x) static DataType x;` |
| `type.h:546-561` / `:546-573` | `taichi/ir/type.h:556-570` (union), class opens `:549` | `TypedConstant` union |
| `type.h:661-672` | `taichi/ir/type.h:661-677` | `val_int32()` … `val_cast_to_float64()` |
| `type.h:170-208` | `taichi/ir/type.h:177-208` | `PointerType` class |
| `type.h:255` | `taichi/ir/type.h:257` | `AbstractDictionaryMember::offset` (`size_t`) |
| `type.h:305-319` | `taichi/ir/type.h:307-321` | `StructType::get_flattened_num_elements()` |
| `type.h:243/:303/:335` | `taichi/ir/type.h:243`, `:305`, `:336` | the three `size_t get_element_offset` |
| `type.h:583` | `taichi/ir/type.h:592` | `TypedConstant(int32 x)` ctor |
| `statements.h:1404-1405` | `taichi/ir/statements.h:1411-1412` | `begin_offset` / `end_offset` (`size_t`) |
| `statements.h:1408-1409` | `taichi/ir/statements.h:1415-1416` | `begin_value` / `end_value` (`int32`) |
| `statements.h:1769` | `taichi/ir/statements.h:1770` | `AdStackAllocaStmt::max_size` |
| `frontend_ir.cpp:938-947` | `taichi/ir/frontend_ir.cpp:936-947` | `IndexExpression::type_check` integral check |
| `type_factory.cpp:229-244` | `taichi/ir/type_factory.cpp:222-256` | `compare_types` |

Re-confirmed unchanged: `type.h:206-208` (PointerType privates),
`type.h:222-227` (`TensorType::get_num_elements`), `statements.h:376`, `:379`,
`:456`, `:1260`, `:1280`, `:1370`, `:1439`, `:1597`, `:1618`, and every
`type_check.cpp` line cited.

Also confirmed by grep: `class TypedConstant` opens at `taichi/ir/type.h:549`;
`class TI_DLL_EXPORT PrimitiveType` at `:151`; `PointerType` at `:177`;
`TensorType` at `:211`.

---
---

# REVISION PASS — entries N022 onwards

Triggered by `adversary-01-1.md` and `adversary-01-2.md`, both returning MAJOR
REVISION. Read both in full. Where they disagree I went to the source and
adjudicated. Every adjudication below was done by opening the file, not by
picking the more confident adversary.

## N022 — My load-bearing inference was FALSE. Adjudicated against source.

Both adversaries say my report's section 5.2 item 1 ("a 64-bit constant
range-for bound is silently truncated to its low 32 bits") is wrong. I checked
the whole chain myself:

- `taichi/transforms/type_check.cpp:40-44` — `mark_as_if_const` body is
  `if (stmt->is<ConstStmt>()) { stmt->ret_type = t; }`. It writes
  **`Stmt::ret_type`** and nothing else.
- `taichi/ir/statements.h:988` — `ConstStmt` holds `TypedConstant val;`
  `:993` — the constructor sets `ret_type = val.dt;` once. There is no
  `visit(ConstStmt *)` in `type_check.cpp` to re-synchronise them. After
  `mark_as_if_const` runs, `ret_type` and `val.dt` **disagree**.
- `taichi/ir/type.cpp:462-465` — `int32 &TypedConstant::val_int32() {
  TI_ASSERT(get_data_type<int32>() == dt); return val_i32; }`. The `dt` here is
  **`TypedConstant::dt`** (`taichi/ir/type.h:551`), i.e. the field
  `mark_as_if_const` did **not** touch.
- `taichi/common/logging.h:100-107` — `TI_ASSERT(x)` expands to
  `TI_ASSERT_INFO((x), ...)`, which is `{ bool ___ret___ = static_cast<bool>(x);
  if (!___ret___) { TI_ERROR(__VA_ARGS__); } }`. **No `NDEBUG` guard. It is not
  compiled out in release.**

**Verdict: the adversaries are right and I was wrong.** An i64 constant
range-for bound raises a hard `TI_ERROR` at `taichi/transforms/offload.cpp:109`
or `:117`. There is no build in which it truncates silently.

Worse, my report listed "`val_int32()` asserts the type is exactly i32 —
`taichi/ir/type.cpp:462-465`" under VERIFIED, immediately before inferring a
truncation that requires that assert not to fire. I had the disproof in my own
report and did not apply it.

Root cause of my error, recorded so it is not repeated: I reasoned about
`TypedConstant` being a union and concluded that retagging changes how the bits
are read. That is true of the union *members*, but `val_int32()` does not
dispatch on `ret_type` — it guards on `dt`. I never opened `type.cpp:462-465`
when forming the inference; I opened it later, to source a different claim, and
did not go back.

The silent-truncation claim survives only for a *different* site:
`taichi/ir/snode.cpp:92` is a plain `static_cast<int>` with no assert, and does
truncate silently in every build. I conflated the two paths.

## N023 — `taichi/inc/rhi_constants.inc.h:14`. My worst miss, and it is in my own territory.

Adversary 1 found it; adversary 2 concedes it missed it too. Verified myself:

- `taichi/inc/rhi_constants.inc.h:14` — `PER_DEVICE_CAPABILITY(spirv_has_int64)`.
  **`taichi/inc/` is my assigned territory. I read this file in N001 and
  transcribed its contents into my notes, then never connected it to plan 6.2.**
- `taichi/rhi/device_capability.h:11-15` — the enum is materialised by expanding
  that same `.inc.h`.
- Set by backends: `taichi/rhi/vulkan/vulkan_device_creator.cpp:632`,
  `taichi/rhi/opengl/opengl_device.cpp:511`,
  `taichi/rhi/metal/metal_device.mm:132`, `:1052`.
- Enforced at SPIR-V codegen, all four verified by reading:
  - `taichi/codegen/spirv/spirv_ir_builder.cpp:64-66` — emits
    `OpCapability Int64` only if present.
  - `:166-169` — declares `t_int64_` / `t_uint64_` only if present.
  - `:311-314` — `if (!caps_->get(cap::spirv_has_int64)) TI_ERROR("Type {} not
    supported.", ...)` for `i64`.
  - `:325-328` — same for `u64`.

**Adjudication: the plan's 6.2 note is already half-answered by my own
territory.** The accurate statement is not "a target lacking Int64 has nowhere
to declare it". It is: *the capability is already declared and already enforced
at SPIR-V codegen; what does not exist is any link from the type layer to it.*
Under current code an i64 index on a Vulkan target without Int64 produces a hard
`TI_ERROR` at `spirv_ir_builder.cpp:312-313`, not a silent degradation.

## N024 — "Single enforcement point" is false. My own count, not either adversary's.

I re-ran the searches myself rather than adopting either count.

**Cast-insertion sites (a real value truncation).** Searched
`grep -rn "cast_type = PrimitiveType::i32" ir/ transforms/ analysis/` plus the
`insert_type_cast_before` calls in `type_check.cpp`:

| File | Lines | Count |
|---|---|---|
| `taichi/transforms/type_check.cpp` | `:154` (SNode index, warns first), `:461` (external-array index, silent) | 2 |
| `taichi/transforms/demote_mesh_statements.cpp` | `:16`, `:54`, `:131` | 3 |
| `taichi/transforms/make_mesh_thread_local.cpp` | `:106`, `:114` | 2 |
| `taichi/transforms/make_mesh_block_local.cpp` | `:212`, `:376`, `:399` | 3 |

**Total: 10 cast-insertion sites in 4 files.** The helper both `type_check.cpp`
sites go through is `insert_type_cast_before` at
`taichi/transforms/type_check.cpp:234-244`.

Adjudication note: adversary 1 says "8 sites across 4 files" but then lists ten;
adversary 2's table lists eight mesh sites, which with the two in
`type_check.cpp` gives ten. **My count is 10.** Adversary 1's headline number is
wrong; its enumeration is right. The team lead's brief repeats adversary 1's "8"
and says "six mesh sites were missed" — the mesh count is eight, not six.

**`ret_type` pinned to i32 with no cast — 18 sites**, verified individually:
`taichi/transforms/type_check.cpp:114`, `:119`, `:467`, `:471`, `:475`, `:521`,
`:525`, `:567`; `taichi/ir/statements.h:1684`, `:2064`, `:2083`;
`taichi/ir/statements.cpp:146`; `taichi/ir/frontend_ir.cpp:152`, `:1114`,
`:1306`, `:1358`, `:1362`, `:1392`.

**The defensible form of my headline**, which is what goes in the revised
report: *for non-mesh SNode and external-array index operands,
`type_check.cpp:154` and `:461` are the only two sites that insert a narrowing
cast.* Everything broader than that is false.

## N025 — `val_int32()` abort surface. Both adversaries got the count wrong; I adjudicated.

`grep -rn "val_int32()" taichi/` minus the declaration and definition gives
**14 call sites tree-wide, 13 inside my territory**:

| File | Lines |
|---|---|
| `taichi/ir/control_flow_graph.cpp` | `:462` |
| `taichi/transforms/offload.cpp` | `:109`, `:117` |
| `taichi/transforms/constant_fold.cpp` | `:115` |
| `taichi/transforms/scalarize.cpp` | `:162`, `:1097`, `:1120`, `:1147`, `:1231`, `:1311` |
| `taichi/transforms/auto_diff.cpp` | `:576`, `:776`, `:1770` |
| `taichi/codegen/llvm/codegen_llvm.cpp` (outside territory) | `:1054` |

Adversary 1 says twelve. Adversary 2 says thirteen and calls the list "exactly
the complete set" of abort sites. **Both are wrong about the abort count, and I
found the reason neither did.** I opened each site to check for a type guard:

- **`taichi/transforms/constant_fold.cpp:115` is guarded.** It sits inside
  `if (dt->is_primitive(PrimitiveTypeID::i32))` at `:113`, with an explicit
  `else if (dt->is_primitive(PrimitiveTypeID::i64))` branch at `:116-118`.
  **It cannot abort. It is correct 64-bit dispatch, not a 32-bit assumption.**
- **`taichi/codegen/llvm/codegen_llvm.cpp:1054` is guarded** by
  `else if (val.dt->is_primitive(PrimitiveTypeID::i32))` at `:1052`, with an i64
  branch at `:1058`. Also not an abort. (Outside territory; noted for the seam.)
- The other twelve guard only that the operand *is a `ConstStmt`*, never that it
  is i32. Verified individually: `control_flow_graph.cpp:456-457`;
  `scalarize.cpp:160`, `:1096`, `:1117`, `:1144`, `:1230`, `:1307`;
  `auto_diff.cpp` (`:575-576`, `:776`, `:1770` — no type guard in the
  surrounding block). These are genuine abort sites.

**So: 13 in-territory call sites, of which 12 abort on a non-i32 constant and 1
is correct type dispatch.**

Structural point neither adversary made: **11 of the 12 abort sites read a
`MatrixPtrStmt::offset` or a matrix element index.** They are all downstream of
the single hard assert at `taichi/transforms/type_check.cpp:160-161`. Only
`offload.cpp:109` and `:117` read something else (range-for bounds). That means
the abort surface is not twelve independent decisions — it is two decisions, one
of which has eleven dependents.

## N026 — My inference 3 had the wrong mechanism. Adversary 2 is right.

I claimed the sparse path truncates silently above 2^31 cells "because
`scalar_pointer_lowerer.cpp:33-39` has no equivalent assert and its
`total_shape` is a plain `int` array." Verified the code:

`taichi/transforms/scalar_pointer_lowerer.cpp:33` declares
`std::array<int, taichi_max_num_indices> total_shape;` and `:36-37` does
`total_shape[j] *= s->extractors[j].shape;` — the index is `j`, **the axis**.
Each element holds the total extent of *one axis* across the root-to-leaf path,
not a cell count. A structure of 2^40 cells spread over four axes of 2^10 leaves
every element at 1024.

Same for `taichi/transforms/demote_dense_struct_fors.cpp:19-24`, where the cell
count is carried separately in `int64 total_n` (`:18`, `:26`) and that is what
`:29` guards.

**Adjudication: adversary 2 is right, adversary 1 repeated my error.** The
32-bit accumulator is real, but it caps **a single axis extent** at 2^31, not
the cell count. My stated failure mode does not fire. The conclusion may still
hold by some other route; the evidence I gave does not support it. The revised
report states the constraint the code actually imposes and drops the inference.

## N027 — `LinearizeStmt` is per-level, not whole-structure. Adversary 2's section 5.1.

Verified at `taichi/transforms/scalar_pointer_lowerer.cpp:48-85`:
`std::vector<int> strides;` is declared at **`:56`, inside the per-level loop
opened at `:48`**, and `:81-82` pushes one `LinearizeStmt` per level. Each
level's linear index is bounded by that level's container size.

**This materially changes my framing.** I called `LinearizeStmt` "the fused
linear address" and treated its i32 strides as the whole-structure address
width. It is not. The address is hierarchical: linearize, `SNodeLookupStmt`,
`GetChStmt`, repeat. A 2^40-cell structure produces a chain of small indices,
not one large one.

The places a *flattened whole-structure* extent is formed are few, and I now
have all three:
- `taichi/transforms/demote_dense_struct_fors.cpp:26`, `:35` — `total_n` across
  the whole path into an `int32` field, guarded by `:29`.
- `taichi/transforms/lower_ast.cpp:270-274` — external-array struct-for extent
  as a running i32 product. **No guard, no assert.** (N028.)
- `taichi/ir/snode.cpp:92` — `acc_shape` truncated per node, warned at `:95-100`.

## N028 — `taichi/transforms/lower_ast.cpp`: a file I never opened for widths.

Both adversaries name it; adversary 1 credits adversary 2 with finding it.
Verified myself:

- `:270-274` — the external-array struct-for iteration extent:
  `Stmt *begin = ConstStmt(TypedConstant(0));` (`:270`),
  `Stmt *end = ConstStmt(TypedConstant(1));` (`:271`), then
  `end = BinaryOpStmt(mul, end, shape[i])` looped over the axes (`:272-274`),
  where each `shape[i]` is an `ExternalTensorShapeAlongAxisStmt` pinned to i32
  by `type_check.cpp:119`. **A whole-array element count built entirely in i32,
  with no assert and no warning anywhere on the path.** Contrast
  `demote_dense_struct_fors.cpp:29`, which does guard.
- `:330` — `fctx.push_back<AllocaStmt>(PrimitiveType::i32);` is the induction
  variable of a range-for containing a `break`, lowered to a while loop. A loop
  variable born i32 outside `type_check.cpp`. `:333` then emits
  `TypedConstant((int32)1)`.

My original notes listed `lower_ast.cpp:168`, `:330`, `:348`, `:361` under a
grep dump in N-series entry for `PrimitiveType::i32` sites and I never went back
to read them. That is the same failure mode as N023: the grep found it, I did
not open it.

## N029 — `taichi/analysis/arithmetic_interpretor.cpp`: a second `LinearizeStmt` implementation.

Adversary 2's find. Verified at `:98-109`:
```
void visit(LinearizeStmt *stmt) override {
  int64_t val = 0;
  ...
    val = (val * stmt->strides[i]) + idx_opt.value().val_int();
  ...
  insert_to_ctx(stmt, stmt->ret_type, val);
}
```
Accumulates in `int64_t` (`:99`, `:106`) and then stores under
`stmt->ret_type` (`:108`), which `type_check.cpp:521` pinned to i32.

**Two independent implementations of `LinearizeStmt` semantics exist** — this
one and the expansion in `taichi/transforms/simplify.cpp:160-220` — plus the two
codegen visitors I already recorded in N018. Four places, not one.

Consumers, verified by `grep -rln ArithmeticInterpretor taichi/ tests/`: the
class has **no callers in `taichi/` at all**. Its only consumers are
`tests/cpp/transforms/scalar_pointer_lowerer_test.cpp` and
`tests/cpp/transforms/make_block_local_test.cpp`. Those tests assert the exact
numeric output of the index lowering that item 6.2 would change. I did not look
at tests at any point in my first pass; that was a gap.

## N030 — `is_bit_vectorized` exception to "every index is cast".

Verified `taichi/transforms/type_check.cpp:122-125`:
```
void visit(GlobalPtrStmt *stmt) override {
  if (stmt->is_bit_vectorized) {
    return;
  }
```
The early return precedes the cast loop at `:147-156`. **A bit-vectorized global
pointer's indices are never cast.** My notes recorded the early return in N004
("if (stmt->is_bit_vectorized) { return; }" appears in the code I quoted) and my
report then said "every SNode index". Both adversaries are right.

## N031 — Line-number adjudication, opened and read individually.

The adversaries agree on most of these. I opened every one rather than adopting
their table. All corrections below are against my published report.

| My report | Correct | Content, as read |
|---|---|---|
| `type_check.cpp:119` (`SNodeOpStmt` default) | **`:114`** | `:114` is inside the `else` of `visit(SNodeOpStmt *)` (`:105-116`) |
| `type_check.cpp:124` (`ExternalTensorShapeAlongAxisStmt`) | **`:119`** | visitor spans `:118-120` |
| `type_check.cpp:39-43` (`mark_as_if_const`) | **`:40-44`** | `:39` is blank |
| `type.h:556-570` / `:546-561` (union) | **`:556-569`** | `:569` is `};`. My report gave two different ranges in two sections |
| `type.h:177-208` (`PointerType`) | **`:177-209`** | class closes at `:209` |
| `type.h:303`, `:335` (element offsets) | **`:305`, `:336`** | already corrected in N021; the report text still carried one stale pair |
| `statements.h:512-521` / `:522-530` (TODO / `offset_used_as_index`) | **`:512-520`** / **`:521-529`** | comment closes `:520`; function opens `:521`, closes `:529` |
| `frontend_ir.h:675` (`ret_shape`) | **`:676`** | `:675` is `indices_group` |
| `frontend_ir.h:713` (`low, high`) | **`:712`** | `:713` is blank |
| `type_utils.cpp:112-155` (`data_type_format`) | **begins `:113`** | |
| `type_utils.cpp:131`/`:132`/`:136` (Vulkan comment, `%lld`, u64) | **`:133`/`:134`/`:136-137`** | `:133` comment, `:134` `"%lld"`, `:136` comment, `:137` format |
| `check_out_of_bound.cpp:151-155`, `:199` (`"%d"`) | **`:153`, `:201`** | `:91` was right |
| `scratch_pad.h:97-100` (`finalize`) | **`:96-100`** | |
| `scratch_pad.h:130-137` (`pad_size_linear`) | **`:131-138`** | |
| `scratch_pad.h:139-146` (`block_size_linear`) | **`:140-147`** | |
| `scratch_pad.h:148-156` (`linearized_index`) | **`:149-157`** | |
| `ir_builder.cpp:159-162` (`get_uint64`) | **`:159-163`** | |
| `simplify.cpp:233-237` (child-type assert) | **loop `:232-236`, asserts at `:233` and `:234-235`** | both my range and adversary 2's `:233-235` describe the same guard; the loop is `:232-236` |
| `make_mesh_block_local.cpp:80`, `:200`, `:575` | **`:80`, `:118`, `:200`, `:265`, `:301`, `:575`** | `:118`, `:265`, `:301` use `int32(...)` functional form, which my `(int32)` grep missed |
| `value_diff.cpp` — one visitor, no line numbers | **two visitors: `:81-87` and `:142-146`** | standing instruction 5 requires line numbers; I gave none |

**On my "re-verified in a second pass" claim.** It has to go. N019/N021 did
verify line *content* — I ran `awk NR==119` and saw
`stmt->ret_type = PrimitiveType::i32;`, which is true — but never verified the
*attribution*. Line 119 does say that; it is just a different visitor from the
one I attributed it to. Verifying that a line says what you quoted is not
verifying that it is the line you meant. The claim is struck from the report.

## N032 — `taichi/ir/expr.cpp:93` was wrongly inventoried. Adversary 2 right.

Verified `taichi/ir/expr.cpp:92-98`:
```
Expr::Expr(int32 x) : Expr() {
  expr = std::make_shared<ConstExpression>(PrimitiveType::i32, x);
}

Expr::Expr(int64 x) : Expr() {
  expr = std::make_shared<ConstExpression>(PrimitiveType::i64, x);
}
```
It is one overload of a pair, with the i64 overload immediately below at
`:96-98`. **It is not a "default integer literal" and not a narrowing.** Listing
it inflated my inventory. Removed from the report.

## N033 — Items I adjudicated where the adversaries disagreed with each other.

1. **`data_type_size` "refuses pointers"** — adversary 1 says no, adversary 2
   endorsed report B's claim that it does. **Adversary 1 is right.**
   `taichi/ir/type_utils.cpp:35` is `t.set_is_pointer(false);` — it *strips* the
   flag and returns the pointee's size. The TODO at `:31-34` records that a loud
   failure on pointers was intended and never written. A caller asking a pointer
   type's size today silently gets the pointee's. This was report B's claim, not
   mine, but it touches my section 4 so I record the adjudication.
   Additionally, and neither adversary's point: **`data_type_size` returns
   `int`** (`:30`) and computes a `TensorType`'s size as
   `get_num_elements() * data_type_size(element)` in `int` (`:47-48`). I held it
   up as the correct derived alternative to `simplify.cpp`'s hardcoded
   `sizeof(int32)`. It is derived, and it is also 32-bit. Both things are true.

2. **`simplify.cpp:260-261` guarded or not** — adversary 1 backs my literal
   reading, adversary 2 scores it "A literally right". I re-read both visitors:
   the asserts at `:233` and `:234-235` are inside
   `visit(SNodeLookupStmt *)` (`:222-249`); `visit(GetChStmt *)` (`:251-270`)
   contains none. **My original framing stands** and stays marked INFERRED,
   because a transitive data-flow argument through the `IntegerOffsetStmt` at
   `:255` is plausible and unproven either way.

3. **`val_int32()` count** — resolved myself in N025. Neither adversary's number
   is right about aborts.

4. **Cast-site count** — resolved myself in N024. Ten, not eight.

5. **`kMaxNumSnodeTreesLlvm` use count** — adversary 1 says adversary B was
   internally inconsistent (one site vs two). Verified:
   `grep -rn kMaxNumSnodeTreesLlvm taichi/` gives
   `taichi/inc/constants.h:13` (declaration),
   `taichi/runtime/llvm/runtime_module/runtime.cpp:562` (`Ptr roots[...]`) and
   `:563` (`size_t root_mem_sizes[...]`). **Two use sites, both outside my
   territory.** It is a second, lower ceiling (512) that plan 6.1 does not name.
   I did not raise it at all in my first pass; that was an omission.

6. **`taichi_max_num_snodes` site count** — adversary 1 calls both reports
   "loose" for saying three; adversary 2 defends the range notation. **I side
   with adversary 2.** `runtime.cpp:567-569` is three consecutive array
   declarations in one struct, and PROJECT-PLAN 6.1 itself cites the range
   `567-569`. Counting it as one site is the plan's own convention. The
   substantive claim — zero uses in my territory — is right either way.

## N034 — Findings I had in these notes and dropped from the report.

Both adversaries make this charge and both are right. Listing them so the
revision is auditable:

| In my notes | Note entry | Dropped from report |
|---|---|---|
| `make_cpu_multithreaded_range_for.cpp:68-93`, called "a second, independent 32-bit narrowing on the dynamic path" | N008 | yes — while the report claimed "nothing else imposes i32" |
| The eight mesh cast sites | N011 | yes — report escalated mesh scope but listed no casts |
| `lower_matrix_ptr.cpp:536-540` | N013 | yes |
| `generate_mod` / `generate_div` taking `int y` | N006 | partially — in section 3.4 rows but not named as the shared helper |
| Pass ordering: `irpass::type_check` re-run seven times, "not a one-shot" | N017 | yes — this is the direct contradiction of my own headline |

Verified the pass-ordering count myself:
`grep -n "irpass::type_check" taichi/transforms/compile_to_offloads.cpp` gives
`:57`, `:66`, `:193`, `:200`, `:208`, `:310`, `:391` — **seven calls**, plus the
in-pass invocations at `taichi/transforms/simplify.cpp:219`,
`taichi/transforms/handle_external_ptr_boundary.cpp:100` and inside
`check_out_of_bound.cpp`'s `run()`.

## N035 — Seam facts I did not have, now verified (outside territory, recorded for the planner).

- `taichi/runtime/llvm/runtime_module/runtime.cpp:288-290` —
  `struct PhysicalCoordinates { i32 val[taichi_max_num_indices]; };`. The
  device-side struct that carries struct-for coordinates is **i32 slots
  dimensioned by `taichi_max_num_indices`**. This is the device-side mirror of
  `type_check.cpp:467`'s i32 `LoopIndexStmt`, and it means the frontend's choice
  is not free for the LLVM path. I missed this entirely; adversary 2 credits
  report B with it. Not my territory, but it constrains my territory.
- `taichi/transforms/lower_matrix_ptr.cpp:536-540` — the dynamic matrix-field
  byte offset is `stmt->offset * ConstStmt(dynamic_index_stride)` with
  `offset->ret_type = stmt->offset->ret_type` forced at `:540`, which
  `type_check.cpp:160-161` has already asserted is i32. **A byte offset computed
  in i32 while every other byte offset in the IR is `size_t`.** In my territory,
  in my notes (N013), absent from my report.
- `taichi/ir/type_factory.cpp:249-251` — `to_primitive_type` opens with
  `if (d->is<PointerType>()) { TI_ERROR("promoted_type got a pointer input."); }`.
  This bounds my section 4 claim that mixed-width arithmetic "widens on its
  own": promotion is unavailable for any operand carrying a pointer type.
- `taichi/transforms/check_out_of_bound.cpp:123-126` — the per-axis upper bound
  compared against every SNode index is `int size_i =
  snode->shape_along_axis(i);` materialised as an i32 `ConstStmt`. I listed the
  *products* at `:54-57` and `:169-173` but not the bound actually compared
  against the index.
- `taichi/ir/frontend_ir.cpp:151-152` — `add_loop_var` sets every loop
  variable's `ret_type` to `get_pointer_type(PrimitiveType::i32)`. I had
  `frontend_ir.cpp:152` in my report's section 3.2 as an unexplained row; it is
  in fact **the birth site of every frontend loop variable**, which is exactly
  what my headline was looking for and mislabelled.

## N036 — What I still cannot settle, and am not settling.

- Whether the mesh path is in scope. Unchanged from my first pass. Now sharper:
  eight of the ten cast-insertion sites are mesh, so the scope answer decides
  whether my corrected headline reads "two sites" or "ten".
- Whether `simplify.cpp:260-261` is transitively guarded (N033 item 2).
- Which of the three whole-structure-extent formation sites (N027) the workload
  actually reaches. Adversary 2 is right that without this the deliverable is a
  list, not a map. I cannot answer it without the engine SNode configuration,
  which PROJECT-PLAN 8.1 item 3 records as unsettled.
- Whether the C++ tests in `tests/cpp/` are a constraint or a casualty (N029).

## N037 — Post-revision verification of the revised report

Ran a mechanical check over every ``file:line`` reference in the revised
`report-01-ir-types.md`: 62 distinct files referenced, **zero references beyond
end-of-file**. Then hand-checked attribution (not just content) on 37 of the
load-bearing ones by `sed -n "${line}p"`. All confirmed, including every
reference that changed in this revision:

`type_check.cpp:114/119/154/461/467/521`; `frontend_ir.cpp:149/151/152`;
`frontend_ir.h:676/712`; `lower_ast.cpp:330`;
`make_cpu_multithreaded_range_for.cpp:84/93`;
`demote_no_access_mesh_fors.cpp:31/32/34`;
`check_out_of_bound.cpp:153/201`; `type_utils.cpp:134`;
`value_diff.cpp:82/143`; `alias_analysis.cpp:59/142/178`;
`lower_access.cpp:237`; `bls_analyzer.cpp:50`; `bit_loop_vectorize.cpp:54`;
`type_factory.cpp:86/95`; `offload.cpp:508/522`; `statements.h:988/993`.

One correction found by this check and applied: I had cited
`make_block_local.cpp:293-298` for the BLS bound-check format string. The
`fmt::format` call is `:292-296` and the `"%d"` is at `:295`; `:298` is the
`ConstStmt(TypedConstant(0))` that follows. Corrected in the report to
`:292-296` with `:295` named.

This is the check N019/N021 should have been. The difference: those passes
confirmed a line *said* what I quoted; this pass confirmed it was the line I
*meant*. The report's claim of re-verification is now scoped to what was
actually done, and the blanket claim from revision 1 is struck.

## N038 — Summary of what changed in the report

Withdrawn: the silent range-for truncation inference (N022); the `total_shape`
overflow mechanism (N026); the "single enforcement point" headline (N024);
`expr.cpp:93` from the inventory (N032); the blanket re-verification claim
(N031).

Added: `rhi_constants.inc.h:14` and the SPIR-V Int64 enforcement chain (N023);
the ten-site cast table and the `is_bit_vectorized` exception (N024, N030); the
twelve-site abort table with the `constant_fold.cpp:115` guard both adversaries
missed (N025); `make_cpu_multithreaded_range_for.cpp` (N034);
`lower_ast.cpp:270-274` and `:330` (N028); the second `LinearizeStmt`
implementation and the test harness (N029); `lower_matrix_ptr.cpp:536-540`
(N035); `check_out_of_bound.cpp:123-126` (N035);
`type_factory.cpp:249-251` bounding the promotion claim (N035); the
`generate_mod`/`generate_div` `int y` signature (N034); `kMaxNumSnodeTreesLlvm`
(N033); `PhysicalCoordinates` as a seam fact (N035); the pass-ordering count
(N034); the hierarchical-addressing correction and the three whole-structure
extent formation sites (N027).

Corrected: sixteen line references (N031), plus the one found in N037.

Escalations grew from 9 to 12: added the CPU path, the test harness, and
`kMaxNumSnodeTreesLlvm`.

Three places where I found an adversary wrong and said so in the report, each
adjudicated against source rather than by preferring one: the cast-site count is
ten not eight (N024); `constant_fold.cpp:115` is guarded and is not an abort
site, contra both (N025); `data_type_size` strips rather than refuses pointers,
contra adversary 2's endorsement (N033).

---

## N039 — Amendment pass. Scope, and the rule I worked under.

Round two returned. `adversary2-01-1.md` and `adversary2-01-2.md` both judge
revision 2 CORRECT on every load-bearing claim and NOT COMPLETE, and both say
consensus is one tightly scoped pass away.

The planner handed me seven CLAIMS, not seven instructions, and said explicitly
that none had been verified and that unverified figures had twice been passed
into briefs and turned out wrong. So the rule for this pass: **open the source
for every claim before touching a line of the report, and where an adversary is
wrong, say so and change nothing.**

Working method, same as N031: `awk 'NR>=a && NR<=b {printf "%d: %s\n", NR, $0}'`
so that every line number printed is the line number checked. No content-only
checks. That is what went wrong in revision 1 and I am not repeating it.

Result across the seven: five correct and acted on, one correct on mechanism but
wrong on its own arithmetic, one factually correct but about a different
territory's file.

## N040 — Claim 1. The third flattening site. ADVERSARY RIGHT. Report was wrong.

Claim: `snode.cpp:89-101` is a per-container extent, not a whole-structure one;
the report's own row text contradicts its heading; the site that belongs there
is `AxisExtractor::num_elements_from_root`; the report cites both halves and
never joins them.

All four parts verified. This is the substantive finding of the pass.

The question is whether `acc_shape` at `snode.cpp:89-93` crosses SNode levels.
It does not, and the proof is in what `insert_children` propagates:

```
14: SNode &SNode::insert_children(SNodeType t) {
21:   for (int i = 0; i < taichi_max_num_indices; i++) {
22:     new_ch->extractors[i].num_elements_from_root *=
23:         extractors[i].num_elements_from_root;
24:   }
```

That loop is the ONLY cross-level propagation in the function. `shape` and
`acc_shape` are never touched, so they sit at their declared defaults of 1
(`snode.h:45`, `:49`). Then in `create_node`:

```
83:     new_node.extractors[ind].num_elements_from_root *= sizes[i];
84:     new_node.extractors[ind].shape = sizes[i];
```

`:83` is a compound multiply — accumulating on top of what the parent gave it.
`:84` is a PLAIN ASSIGNMENT of this node's own axis size. That single character
of difference is the whole thing, and I read past it in both previous passes.

So `acc_shape` at `:89-93` is the product of one node's own axis shapes, and
`:101` stores it into `num_cells_per_container`, which `snode.h:93-97` documents
as "Product of the |shape| of all the activated axes identified by |extractors|"
with a pointer to the cell/container terminology. Per container.

I wrote "truncated per node" in my own row text at the same time as writing
"whole-structure" in the heading above it. Both adversaries caught the
contradiction. They are right and I had the evidence in front of me.

**The replacement.** `num_elements_from_root` is the root-to-leaf quantity, and
it is a plain `int` (`snode.h:41`) with no assert, no warning and no `int64`
shadow anywhere on its path. Verified the chain end to end:

```
snode.h:41       int num_elements_from_root{1};
snode.cpp:22-23  child *= parent            (root-to-leaf accumulation)
snode.cpp:83     *= sizes[i]                (this node's axis size)
snode.cpp:176    return extractor.num_elements_from_root;   <- shape_along_axis
check_out_of_bound.cpp:123  int size_i = snode->shape_along_axis(i);
check_out_of_bound.cpp:125-126  ConstStmt(TypedConstant(upper_bound_i))
```

I had `snode.h:317` in section 3.5 ("`shape_along_axis(int i)` returns `int`")
and I had `check_out_of_bound.cpp:123-126` in section 3.11 ("the bound actually
compared"). I never opened `shape_along_axis`'s definition, so I never joined
them. Joined, they say the bound checked against every SNode field index is an
unguarded root-to-leaf `int` product. That is a better finding than anything I
produced in revision 2 and it is adversary2 01-1's.

Seam fact, confirmed and recorded for agent 02 rather than claimed:
`taichi/codegen/spirv/snode_struct_compiler.cpp:115-121` builds
`sn_desc.total_num_cells_from_root *= e.num_elements_from_root;`. That is what
makes the field load-bearing rather than incidental.

One thing I checked that neither adversary did. The comment at
`snode_struct_compiler.cpp:118-120` says the extractors are set in two places,
the second being `StructCompiler::infer_snode_properties()`. If that were true
it might recompute the field somewhere I had not looked.
`grep -rn "infer_snode_properties" taichi/` returns exactly one hit: that
comment. The function does not exist. The comment is stale and `taichi/ir/snode.cpp`
is the only writer, which makes the claim tighter, not weaker.

**Action:** section 4 rewritten, third row replaced. The `:92` truncation, its
disagreement with `:101`, and the `TaichiIndexWarning` all stay in the inventory
where sections 1 and 3.9 already carry them. Only the classification is
withdrawn. Escalation 6 sharpened; escalation 14 added for
`get_total_num_elements_towards_root`, which also flattens a whole structure,
narrows with `(int)` at `snode.h:313`, and has no callers.

## N041 — Claim 2. `lower_ast.cpp:270-274` misfiled. ADVERSARY RIGHT.

```
270:       Stmt *begin = fctx.push_back<ConstStmt>(TypedConstant(0));
271:       Stmt *end = fctx.push_back<ConstStmt>(TypedConstant(1));
272:       for (int i = 0; i < (int)shape.size(); i++) {
273:         end = fctx.push_back<BinaryOpStmt>(BinaryOpType::mul, end, shape[i]);
274:       }
```

`push_back<ConstStmt>` and `push_back<BinaryOpStmt>` emit IR. Nothing here is a
host `int`. My section 3.6 preamble says "These are host-side compile-time `int`
computations. They cannot be fixed by changing an IR type" — which is the exact
opposite of what this site is, and my own section 4 and section 7.3 item 2
already treated it as an IR-level formation site. The report contradicted itself
across two sections.

i32 by two independent routes: `TypedConstant(int32)` at `type.h:582`, and
`type_check.cpp:119` pinning `ExternalTensorShapeAlongAxisStmt`.

I re-opened every remaining row of 3.6 rather than assuming the adversaries'
sweep was complete: `scalar_pointer_lowerer.cpp:33-39`,
`demote_dense_struct_fors.cpp:19-24` and `:74`, `simplify.cpp:176`/`:187`,
`check_out_of_bound.cpp:54-57` and `:169-173`,
`handle_external_ptr_boundary.cpp:62-67`, `frontend_ir.cpp:742-747`,
`scratch_pad.h`. Every one is a plain host `int`. The `lower_ast` row was the
only misfiled one, exactly as both adversaries said.

**Action:** new section 3.12 for IR-level i32 products, row moved into it,
preamble of 3.6 amended to record the correction rather than silently drop it.

## N042 — Claim 3. Twenty sites under a heading of eighteen. ADVERSARY RIGHT.

Counted the table row by row. Eighteen rows. The
`frontend_ir.cpp:1358, :1362, :1392` row carries three sites, so twenty sites.

Why it happened: N024 fixed the count at eighteen and was correct at the time.
Two rows were then added to the report — `lower_ast.cpp:330` and
`type_factory.cpp:204` — and the heading was not moved. This is the same
headline-versus-enumeration error I correctly charged adversary 1 with in N024,
committed in my own file two sections later. Noted, because that is the class of
error this loop keeps producing on both sides.

The second half of the claim also holds. Verified both added rows:

```
lower_ast.cpp:330      fctx.push_back<AllocaStmt>(PrimitiveType::i32);
type_factory.cpp:204   shape_members.push_back({PrimitiveType::i32, fmt::format("dim_{}", i)});
```

An alloca element type and a struct member type. Neither is a `ret_type`
assignment. Both are real hardcoded i32 and both belong in the inventory, so the
fix is the heading, not the rows.

**Action:** heading changed to "Hardcoded i32 on statement and expression types
— twenty sites, eighteen rows", with a preamble saying what was wrong and which
two rows are not `ret_type`.

## N043 — Claim 4. The `GetChStmt` visitor span. ADVERSARY RIGHT, both of them.

```
251:   void visit(GetChStmt *stmt) override {
270:     }
271:
272:     set_done(stmt);
273:   }
274:
```

`:270` closes the inner `if`, `:272` is `set_done`, `:273` closes the visitor.
The span is `:251-273`. I said `:251-270`; pass B said `:251-274`. Both wrong,
in opposite directions, on the same claim — which is a slightly uncomfortable
coincidence and worth recording as such.

Checked the companion span too, since if one is wrong the other might be:
`visit(SNodeLookupStmt *)` is `:222-249`, correct in both reports, as are the
asserts at `:233`, `:234-235`, the `:239` site and the `:261` site.

The substantive point is untouched. `:261` is
`stmt->chid * sizeof(int32) + previous_offset->offset` with no assert in the
visitor, the transitive argument through the `IntegerOffsetStmt` at `:255` is
plausible and unproven, and it stays inferred (7.3 item 5) and escalated
(item 7). N036 said I cannot settle it. I still cannot, and I am not going to.

**Action:** one number changed in section 7.3 item 5.

## N044 — Claim 5. The `type_check` re-run surface. MECHANISM RIGHT, COUNT WRONG.

This is the claim the planner told me to verify independently, and it is the one
where doing so mattered.

The mechanism is right and it is my error. Revision 2 ran
`grep -n "irpass::type_check" taichi/transforms/compile_to_offloads.cpp` — a grep
scoped to a single file — and then reported the seven hits plus three sites found
by hand. Two greps are needed:

```
grep -rn "irpass::type_check" taichi/
grep -rn "[^_a-zA-Z:>.]type_check(" taichi/ir/ taichi/analysis/ taichi/transforms/
```

The second is the one that matters: most in-pass re-runs sit inside
`namespace irpass` and call `type_check(root, config)` unqualified, so no search
for the qualified name finds them at all.

Full derivation, opened in context rather than taken from the adversary's list:

Qualified `irpass::type_check` outside the driver, 5 sites / 4 files:
  ir.cpp:566 (DelayedIRModifier::modify_ir, the drain point)
  check_out_of_bound.cpp:248, handle_external_ptr_boundary.cpp:100
  demote_operations.cpp:300 (inside the fixed-point loop), :303 (after it)

Unqualified `type_check(root, config)`, 14 sites / 10 files:
  auto_diff.cpp:2407, :2410, :2417, :2420, :2430
  demote_atomics.cpp:241, demote_mesh_statements.cpp:160
  lower_access.cpp:294, make_block_local.cpp:391
  make_mesh_block_local.cpp:681, make_mesh_thread_local.cpp:165
  make_thread_local.cpp:229, offload.cpp:766, :773

Queued via DelayedIRModifier::type_check (ir.h:617, ir.cpp:533-534), 3 / 1:
  simplify.cpp:219, :344, :349

22 further sites, 14 files. Plus the driver's 7 = 29 invocation sites.

**Where the adversary is wrong.** `adversary2-01-2.md` section 3.2 writes "at
least fifteen further re-runs across fourteen files" and then prints a list
containing twenty-two. Fourteen files is right; fifteen is not, and its own list
contradicts it. Had I adopted the number instead of deriving it, I would have
passed a wrong figure into the report — which is precisely the failure mode the
planner warned about. I have said so in the report rather than quietly using 22.

`lower_access.cpp:294` deserves singling out: it re-runs `type_check` immediately
after `lower_access` emits the `LinearizeStmt`s, so it is directly in the path of
anything item 6.2 changes. Pass B found it; I did not.

Direction of the conclusion is unchanged and its force is increased. Seven to
twenty-nine is roughly a fourfold undercount.

**Action:** section 5 rewritten with the full set in three tables by call
mechanism. Section 1 headline changed from "re-run seven times" to twenty-nine
invocation sites, with the undercount stated rather than hidden.

## N045 — Claim 6. The dynamic tensor-offset branch. ADVERSARY RIGHT.

```
730:   Stmt *offset_stmt = nullptr;
731:   if (needs_dynamic_index) {
732:     offset_stmt = ctx->push_back<ConstStmt>(TypedConstant(0));
735:       Stmt *shape_stmt = ctx->push_back<ConstStmt>(TypedConstant(shape[i]));
736-739:  mul then add, per axis
741:   } else {
742:     int offset = 0;
747:     offset_stmt = ctx->push_back<ConstStmt>(TypedConstant(offset));
748:   }
```

Two branches of one computation. Revision 2 carried only `:742-747`, in section
3.6, where it is correctly filed — `:742` is a host `int`. The dynamic branch at
`:731-740` emits IR and was missing.

Both `TypedConstant(0)` and `TypedConstant(shape[i])` take the `int32`
constructor at `type.h:582`, so the whole flattened offset is built in i32.

This was raised in round one by `adversary-01-1.md` (its section 5.7 and 10.2)
and revision 2 neither fixed it nor argued against it. Checking my own
N022–N038, it appears nowhere. It is an objection I failed to answer, not one I
adjudicated, and the report now says so in those terms. Bounded by tensor
element count, so the consequence is small; the reason to fix it is that
splitting two branches of one computation across two categories is exactly how a
reader gets misled.

**Action:** added to the new section 3.12 alongside the `lower_ast` row.

## N046 — Claim 7. `llvm_program.cpp:112` vs `:113`. NOT THIS REPORT.

The planner flagged that this might belong elsewhere. It does.

Facts first, since the claim is checkable and I checked it:

```
112:     LlvmOfflineCache::FieldCacheData::SNodeCacheData snode_cache_data;
113:     snode_cache_data.id = snodes[i]->id;
```

`:112` is the declaration, `:113` the assignment. The claim is right on the
source.

But `grep -rn "llvm_program" modernization/investigation/` shows the citation is
at `notes-03-runtime-struct.md:466`, in territory 03's notes, and
`adversary2-03-1.md:391` and `:497` already raise it there.
`report-01-ir-types.md` has never mentioned `llvm_program.cpp` in any revision.

**Action: none.** Changed nothing. Recorded in report section 7.5 item 7 with the
correct owner, so it does not get chased into this territory a third time.

This is the second time in this project that an unverified item has been routed
to the wrong file. Verifying ownership before acting cost one grep.

## N047 — Items outside the seven claims. Verified, not inventoried, escalated.

The round-two adversaries raised more than the seven claims I was given. My
brief says not to open new lines of investigation beyond the claims and the gaps
they name, and not to decide anything is unnecessary. Leaving them silently out
would let the report imply the adversary lists are now closed. So: each was
opened, none was inventoried, and the disposition is the planner's.

```
type_check.cpp:170-171        mark_as_if_const(stmt->begin/end, PrimitiveType::i32)
                              inside visit(RangeForStmt *) at :169-173. Present.
                              Discussed in my 7.1 but in no inventory table.
make_mesh_block_local.cpp:526 mapping_data_type_ = PrimitiveType::i32;
                              with the derived line commented out at :525.
lower_ast.cpp:168,:348,:361   three further AllocaStmt(PrimitiveType::i32)
                              beyond the :330 I carry. grep confirms exactly four
                              i32 tokens in the file.
auto_diff.cpp:587,639,646,    insert_const(PrimitiveType::i32,...) and
  787,839,846,1828,1857       get_tensor_type(shape, PrimitiveType::i32).
                              Bounded by matrix element count.
compile_to_offloads.cpp:198   if (config.make_cpu_multithreading_loop &&
                                  arch_is_cpu(config.arch))
                              My 3.4 says "whenever the arch is CPU" and drops the
                              flag. The pass is switchable. Bears on escalation 4,
                              where I ask the planner to rule on the CPU path.
```

All present at the lines given. Recorded at the end of report section 7.5 and
escalated as item 13. I am not deciding whether they enter the tables.

## N048 — What this pass did not change, and why.

Recorded so the next reader does not assume silence means agreement.

- The central structural finding is untouched. Both round-two adversaries say
  they tried to break it and could not. Nothing in this pass bears on it.
- Sections 3.1, 3.2, 3.5, 3.7, 3.8, 3.9, 3.10, 3.11 and 6 are unchanged. No
  claim was made against them and I did not go looking.
- The `constant_fold.cpp:115` adjudication stands. Both adversaries have now
  withdrawn against it. Adversary2 01-1 adds one nuance I did not have: `:115`
  calls `rhs->val.val_int32()` while the guard at `:113` tests only
  `lhs->val.dt`, safe only because operand types are unified upstream, which the
  comment at `:63` asserts rather than proves. Not a claim I was given and not an
  error in the report, so no change. Recorded here because it is true and cheap
  to lose.
- `simplify.cpp:261` remains inferred and escalated. Three agents and three
  adversaries have now failed to settle it. That is a signal about the question,
  not about the agents.
- Escalations 1 through 12 are unchanged in substance. 6 is sharpened by N040.
  13 and 14 are new.

Count of things I got wrong across all three revisions, since somebody should
keep it: revision 1, five substantive errors plus sixteen bad line numbers.
Revision 2, four categorisation or count errors and one unanswered round-one
objection, all fixed here. Revision 3 changes no load-bearing claim, which is
what a report converging looks like rather than a report being rewritten.

## N049 — Escalation 13 ruled: all five items enter the inventory.

The planner ruled against my scoping caution, and the reasoning is right. My
original brief required a complete inventory of 32-bit width assumptions in this
territory. All five items are inside the territory and I had already opened each
one. In that case completeness is not a scope question, it is the brief, and I
was treating "do not open new lines of investigation" as though it also meant
"do not record what you already verified". Those are different things.

Where each landed, and why there rather than somewhere else:

`type_check.cpp:170`, `:171` → section 3.3. Two sites, one row. It is
`visit(RangeForStmt *)` at `:169-173` calling `mark_as_if_const` on `begin` and
on `end`. I discussed the mechanism in 7.1 item 1 in revision 2 while never
inventorying the site, which is the same joined-halves failure as N040 in
miniature: the mechanism in one section, the site in none. Added a sentence
after the table tying it back to 7.1, since `mark_as_if_const` writing `ret_type`
only is exactly why a wide bound aborts at `offload.cpp:109`/`:117` rather than
truncating.

`lower_ast.cpp:168`, `:348`, `:361` → section 3.3, one row alongside the `:330`
I already carried. `grep -n "PrimitiveType::i32" taichi/transforms/lower_ast.cpp`
returns exactly these four, so the file is now closed for i32 tokens. All three
new ones are loop-mask allocas.

`auto_diff.cpp` eight sites → section 3.3, two rows split by construction form,
because they are two different things: five `insert_const(PrimitiveType::i32,
...)` at `:587`, `:646`, `:787`, `:846` and `insert_const_for_grad(...)` at
`:1857`, and three `get_tensor_type(tensor_shape, PrimitiveType::i32)` at `:639`,
`:839`, `:1828` whose calls open at `:638`, `:838`, `:1827`. I checked the
opening lines rather than citing the continuation lines alone, since a bare
`tensor_shape, PrimitiveType::i32);` is not self-evidently a `get_tensor_type`
call. Bounded by matrix element count; said so in the report rather than letting
eight rows imply eight problems.

`make_mesh_block_local.cpp:526` → section 3.7, not 3.3. This is the right home
and I want the reason on record. 3.7 is "widths hardcoded rather than derived",
and `:525` is `// mapping_data_type_ = mapping_snode_->dt.ptr_removed();` —
the derived form, commented out, sitting one line above the literal that
replaced it. Then `:527` derives `mapping_dtype_size_` from the hardcoded type.
It is the clearest instance in the territory of a derived width deliberately
swapped for a literal, and filing it under 3.3 with the ordinary pinned types
would have lost that.

Section 3.3 goes from twenty sites to thirty-three, in twenty-two rows. Heading
updated to match, which is the third time this pass a count and an enumeration
have had to be reconciled — mine in N042, the adversary's in N044, and now this
one. I am stating counts as "N sites, M rows" from here so the two cannot drift
apart again.

**The CPU gate**, ruled into the body rather than into an escalation, and the
planner is right that it is a factual refinement rather than a new question.
Revision 2 said the pass runs "whenever the arch is CPU". Verified myself rather
than adopting the adversary's citation:

```
compile_to_offloads.cpp:198  if (config.make_cpu_multithreading_loop && arch_is_cpu(config.arch)) {
compile_config.h:44          bool make_cpu_multithreading_loop;
compile_config.cpp:48        make_cpu_multithreading_loop = true;
export_lang.cpp:224-225      .def_readwrite("make_cpu_multithreading_loop", ...)
```

So: on by default, plain `bool`, read-write from Python. The pass is switchable.
That changes what escalation 4 is asking. Revision 2 put it to the planner as
"the CPU path narrows the whole parallel iteration space to i32"; it is more
precisely "the CPU path does this by default and can be configured not to". I
have corrected the description and left the escalation standing, because whether
the CPU path is in scope is still not mine, and because turning the pass off is
a behaviour change I am not going to propose. Standing instruction 4.

## N050 — Escalation 14 ruled: record the uncalled flattening as latent.

`SNode::get_total_num_elements_towards_root`, `taichi/ir/snode.h:310-315`.

I re-ran the caller search wider than before, because "no callers" is the whole
basis of the ruling and I did not want to be asserting it off a narrow grep:

```
grep -rn "get_total_num_elements_towards_root" . \
  --include=*.cpp --include=*.h --include=*.py --include=*.cc --include=*.mm
```

Whole repository, tests and Python bindings included. One hit: the definition at
`snode.h:310`. Confirmed.

The planner's reasoning is the part worth recording. The precedent is territory
03, where a leak with a single out-of-scope caller was kept as latent rather than
dropped, on the grounds that this fork will add its own paths; and plan section
4.6 says the engine SNode configuration is composed from the larger data stores
for working scope, so a whole-structure element count acquiring a caller is
foreseeable rather than hypothetical. Absence of callers today is a fact about
today.

Filed in 3.9 with the no-caller fact in the first sentence, quoted in full, and
explicitly NOT counted among section 4's three formation sites, which are the
reached ones. That distinction matters: if it went into the section 4 table the
report would say four sites flatten a whole structure and three of them are
live, which invites exactly the misreading the planner asked me to prevent.

One mechanical point I added and neither adversary stated. The accumulator at
`:311` is `int64` and the walk is root-to-leaf, but `:313` narrows every factor
with `(int)` **before** multiplying it in. So the `int64` accumulator buys
nothing above 2^31 in any single container — the narrowing is per factor, not on
the result. It reads as a wide computation and is not one.

Both escalations 13 and 14 are marked RESOLVED in the report rather than deleted,
with the ruling recorded, so the adversaries can see how their objections were
disposed of rather than finding them silently gone. Escalations 1 through 12 are
untouched and still open.

Nothing in this pass changed a load-bearing claim. Section 3.3 got thirteen more
sites, 3.7 one, 3.9 one latent site, and 3.4 one corrected gate. The structural
finding is where it has been since revision 2.

---

# Revision 4 — closing pass. Entries N051–N062.

Round three landed. `adversary3-01-1.md` and `adversary3-01-2.md` both judge this
report CORRECT with one arithmetic defect and NOT COMPLETE, and neither
recommends a fourth investigative round. Planner's instruction: fix what is
listed, close the gaps, stop. Five claims put to me. Every one is a claim until
I have checked it at source.

## N051 — Claim 1. "Fourteen sites in ten files" over a nine-file table. ADVERSARIES RIGHT.

Both adversaries derived nine independently. I did not adopt either. Wrote the
fourteen unqualified `type_check(root, config)` lines to a file and ran the
arithmetic rather than eyeballing the table:

```
wc -l                       -> 14
cut -d: -f1 | sort -u | wc -l -> 9
```

The nine: `auto_diff.cpp` (5 lines), `demote_atomics.cpp`,
`demote_mesh_statements.cpp`, `lower_access.cpp`, `make_block_local.cpp`,
`make_mesh_block_local.cpp`, `make_mesh_thread_local.cpp`,
`make_thread_local.cpp`, `offload.cpp` (2 lines). 5+1+1+1+1+1+1+1+2 = 14.

Then re-printed each of the twenty-two non-driver lines with `sed -n "${l}p"` to
confirm every one is a call and not a declaration or comment. Qualified group: 5
lines, 4 files. Queued group: 3 lines, 1 file. File sets disjoint, so 9+4+1 = 14
files, and that is a derivation rather than a coincidence with the stated total.

Ten was wrong. It made the section read 10+4+1 = 15 against its own headline of
fourteen. This is the fifth instance in the project and the second by this
report — revision 2's "eighteen sites" over twenty rows was the first. Fixed,
and section 5 now carries the reconciliation table plus the commands, so the
next reader checks it rather than trusting it.

## N052 — Claim 2. `auto_diff.cpp:1819`. ADVERSARIES RIGHT, AND THE CLAIM ITSELF WAS SHORT BY NOTHING.

Swept the file myself as instructed rather than adopting the twelve-line figure:

```
grep -n "PrimitiveType::i32\|(int32)\|int32(" taichi/transforms/auto_diff.cpp
```

Twelve lines, exactly. Nine index constructions: `:587`, `:639`, `:646`, `:787`,
`:839`, `:846`, `:1819`, `:1828`, `:1857`. Three `val_int32()` reads already in
section 3.2: `:576`, `:776`, `:1770`. 9 + 3 = 12.

`:1819` opened: `indices_values[i] = insert<ConstStmt>(TypedConstant((int32)i));`
in the loop at `:1818` over `num_elements`. `indices_values` becomes the
`MatrixInitStmt` at `:1829`, `ret_type` set at `:1830` to `index_tensor_type`
built at `:1827-1828` from `get_tensor_type(tensor_shape, PrimitiveType::i32)`.
A matrix element index. i32 by the constructor at `type.h:582`.

Why revision 3 missed it: it adopted a group of eight from a round-two adversary
instead of sweeping the file. The eight are all `insert_const(PrimitiveType::i32,
…)` or `get_tensor_type(…, PrimitiveType::i32)`; `:1819` is the one member of the
group written in the `TypedConstant((int32)…)` spelling. This is the lesson of
the whole pass in miniature — a grep shaped around one spelling is not a sweep.

The claim that the section total becomes thirty-four is right for that one
addition. With the other twenty it is fifty-four; see N057.

## N053 — Claim 3. The `lower_ast.cpp` alloca label. ADVERSARIES RIGHT ON `:361`.

`grep -n "PrimitiveType::i32" taichi/transforms/lower_ast.cpp` -> exactly four:
`:168`, `:330`, `:348`, `:361`. Revision 3's parenthetical that these are all
four tokens in the file is correct. Opened each in context:

- `:168` mask. `:169` is `new_while->mask = mask.get();`.
- `:330` index. `:331` `auto loop_var = fctx.back_stmt();`, `:332` binds it to
  `stmt->loop_var_ids[0]`.
- `:348` mask. `:349` is `new_while->mask = mask.get();`.
- `:361` neither. `stmt->insert_before_me(std::make_unique<AllocaStmt>(...))`
  spanning `:360-361`, the temporary moved straight in, nothing bound to it. The
  mask in that block is the `:348` alloca, moved in at `:365`, stored to at
  `:367-368`.

So revision 3's "all loop-mask allocas" is wrong for one of three. Relabelled.

The count question is separate and both adversaries got it right in the same
way: section 3.3 is headed "Hardcoded i32 on statement and expression types",
not "index-width sites", and a mask alloca is a hardcoded i32 on a statement
type. Under the report's own unit the three belong, standing instruction 3
forbids removing them, and the count is not inflated. What was missing is any
marker letting a reader separate the one index from the three non-indices. Fixed
by opening 3.3 with a paragraph saying plainly that the table is not an index
count, and by labelling every non-index row. That is the substance of the
objection and it costs no sites.

## N054 — Claim 4. `lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477`. ADVERSARIES RIGHT, ABSENT ENTIRELY.

Opened all four. Identical shape each time:

```
188:  for (int i = 0; i < num_elements; i++) {
189:    auto const_stmt = std::make_unique<ConstStmt>(TypedConstant(i));
191:    auto matrix_ptr_stmt =
192:        std::make_unique<MatrixPtrStmt>(alloca_stmt_ptr, const_stmt.get());
```

and the same at `:232`/`:234-235`, `:424`/`:426-427`, `:477`/`:479-480`. Matrix
element indices, i32 by `type.h:582`.

Named by nobody before round three, across two explore passes and two prior
adversarial rounds. The reason is the spelling: `TypedConstant(i)` where `i` is
a host `int`. No `i32` token, no `(int32)` cast. Neither of the two greps this
report had been using could ever have found them. That is what drove N058.

Revision 3 cited this file only at `:536-540` and in escalation 5.

## N055 — Claim 5. The sixty-four-line sweep. RAN IT. CLAIM RIGHT, AND SHORT BY TWO.

```
grep -rn "PrimitiveType::i32" taichi/ir/ taichi/analysis/ taichi/transforms/
```

Sixty-four lines. Figure confirmed exactly.

Dispositioned every one into a table (report section 3.13): 52 already
inventoried, 8 added here, 4 swept and excluded with a reason. 52+8+4 = 64,
generated from the enumeration rather than tallied by hand.

The four excluded, each with the reason stated in the table rather than dropped
silently:

- `type_utils.h:40` — the `int32` arm of `get_data_type<T>()`. That is the
  correct mapping from a C++ type to a Taichi type. Calling it a width
  assumption would be wrong.
- `expr.cpp:93` — `Expr::Expr(int32 x)`, whose i64 sibling is at `:96-98`.
  Already struck from this report in revision 1, section 7.1 item 5.
- `type.h:582` — the `TypedConstant(int32)` constructor itself. It is the
  mechanism the inventory cites, not a site in it.
- `type_check.cpp:459` — the `!=` guard condition for the cast emitted at
  `:461`. Carried by that row already.

**The claim said six of the sixty-four are in neither report. For this report it
is eight.** The two nobody named:

- `make_mesh_thread_local.cpp:42` — `ConstStmt(TypedConstant(PrimitiveType::i32, 1))`,
  added at `:43-44` to the `MeshPatchIndexStmt` created at `:39-40`, whose own
  ctor is the i32 pin at `statements.h:2083` this report already carried. Mesh
  patch-index arithmetic.
- `check_out_of_bound.cpp:220` — `compare->ret_type = PrimitiveType::i32;` on the
  `cmp_ge` built at `:218-219` for the negative-exponent assertion on integer
  `pow`. Not an index; a comparison result. Labelled as such in the row.

Both fell out of actually running the sweep. Neither adversary named them,
including the one that ran the same command — which suggests it read the sixty-
four for what it expected to find rather than dispositioning all of them.

## N056 — `frontend_ir.cpp:1160`, the material one. Verified on both sides.

Frontend half, `SNodeOpExpression::flatten`, `append` branch:

```
1160:  auto alloca = ctx->push_back<AllocaStmt>(PrimitiveType::i32, dbg_info);
1161:  auto addr = ctx->push_back<SNodeOpStmt>(SNodeOpType::allocate, snode, ptr,
1162:                                          alloca, dbg_info);
1169:  ctx->push_back<LocalLoadStmt>(alloca, dbg_info);
```

`alloca` is the statement's `val` operand and is the expression's result.

Device half — I opened this rather than adopting it, because the whole weight of
the site is that both sides are i32:

```
node_dynamic.h:61:  Ptr Dynamic_allocate(Ptr meta_, Ptr node_, i32 *len) {
node_dynamic.h:65:    auto i = atomic_add_i32(&node->n, 1);
node_dynamic.h:66:    *len = i;
```

reached from `codegen_llvm.cpp:1367-1375`, `visit(SNodeOpStmt *)`, which passes
`llvm_val[stmt->val]` into the runtime call at `:1374`.

So a dynamic SNode's element index is i32 at birth in the frontend and i32 in
the runtime signature. Same class as `frontend_ir.cpp:151-152` and
`lower_ast.cpp:330`, which this report already calls load-bearing birth sites,
and the same host/device mirror property as `PhysicalCoordinates` in escalation
3. This report already carried `type_check.cpp:114`, the downstream half of the
same statement, and never had the frontend half. Promoted into section 2.2 as
load-bearing point 7, not just filed in 3.3.

## N057 — Section 3.3 recounted from its own enumeration.

Not tallied by reading the table. Wrote all fifty-four `file:line` pairs to a
file and ran the checks:

```
wc -l                        -> 54
sort -u | wc -l              -> 54   (no duplicates)
cut -d: -f1 | sort -u | wc -l -> 12
```

Row-by-row site counts, in table order: 1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,
5,3,1,1,4,3,2,1,3,1,1,4,1. Thirty-two rows, summing to 54. Existing 22 rows held
33; the ten new rows hold 21; 33+21 = 54.

The twenty-one additions, with who named each:

| Sites | Named by |
|---|---|
| `auto_diff.cpp:1819` | both round-three adversaries |
| `lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477` | both |
| `frontend_ir.cpp:1160` | adversary3-01-2 |
| `frontend_ir.cpp:1216`, `:1233`, `:1249` | adversary3-01-2 |
| `scalarize.cpp:471`, `:473` | adversary3-01-2 |
| `lower_ast.cpp:151`, `:177`, `:333`, `:363` | adversary3-01-1 |
| `scalarize.cpp:63`, `:114`, `:530` | nobody — sweep |
| `make_mesh_thread_local.cpp:42` | nobody — sweep |
| `check_out_of_bound.cpp:220` | nobody — sweep |
| `ir_builder.cpp:125` | nobody — sweep |

1+4+1+3+2+4+3+1+1+1 = 21.

## N058 — The finding that matters most from this pass: four spellings, not one.

This was not asked for and it is not a new line of investigation. It is what
running the item-5 sweep to the end forced.

`TypedConstant` has an implicit `int32` constructor at `type.h:582`. So an i32
constant can be built with no `i32` token on the line. Four spellings exist in
this territory and I ran each to exhaustion:

| Spelling | Command | Lines |
|---|---|---|
| explicit token | `grep -rn "PrimitiveType::i32" <territory>` | 64 |
| cast / functional | `grep -rn "TypedConstant((int32)\|TypedConstant{(int32)\|int32(" <territory>` | 37 |
| via `get_data_type` | `grep -rn "get_data_type<int32>()" <territory>` | 3 |
| implicit | `grep -rn "TypedConstant(" <territory>` minus explicitly-typed first arguments | 78 candidates |

Second sweep: 37 lines. Everything in it is either already inventoried (the
`val_int32()` aborts of 3.2, the byte-offset narrowings of 3.8) or is
`auto_diff.cpp:1819` and `lower_ast.cpp:151`, `:177`, `:333`, `:363`. Closed.

Third sweep: 3 lines, `scalarize.cpp:63`, `:114`, `:530`, all
`TypedConstant(get_data_type<int32>(), i)` feeding `MatrixPtrStmt` offsets at
`:66-67`, `:116-117`, and `:543-547`/`:553-554`. Same class as the
`lower_matrix_ptr.cpp` four in a fourth spelling. The only other hits tree-wide
are the two asserts inside `val_int32`/`val_int64` at `type.cpp:463`, `:478`.
Closed.

Fourth sweep: this is the one I am NOT closing. Filter is `TypedConstant(` in
territory minus every call whose first argument is a type (`dt`, `dst_type`,
`data_type`, `load_data_type`, `PrimitiveType::…`, a `ret_type`,
`get_data_type<int32>()`, `…get_element_type()`). Leaves 78 candidate lines in
16 files. Some are certainly not sites — `constant_fold.cpp` contributes five
macro continuation lines whose type argument is on the previous line, and
`ir_builder.cpp` six of the same shape. Others plainly are. I opened one at
random to test that:

```
demote_operations.cpp:98:  for (int i = 0; i < lhs_tensor_ty->get_num_elements(); i++) {
demote_operations.cpp:99:    auto idx = Stmt::make<ConstStmt>(TypedConstant(i));
demote_operations.cpp:100:   auto lhs_i = Stmt::make<MatrixPtrStmt>(lhs_ptr, idx.get());
demote_operations.cpp:101:   auto rhs_i = Stmt::make<MatrixPtrStmt>(rhs_ptr, idx.get());
```

Identical in form to `lower_matrix_ptr.cpp:189`. So the residue contains real
sites of the class item 6.2 turns on.

**Seventy-eight is a candidate count. I have not derived a site count from it
and I am not going to guess one.** Classifying all seventy-eight means opening
every line and ruling on each, which is a fresh enumeration of a class nobody
has scoped, on a pass whose brief is explicitly closing rather than
investigating. Standing instruction 2 says stop and report. Escalated as item
16, with the command, the count, the filter and the worked example, so the
planner can scope it in one read rather than rediscovering it.

## N059 — The consequence for the word "complete".

Sections 3 of revisions 1 through 3 were headed "Complete inventory of 32-bit
width assumptions in this territory". After N058 that heading cannot stand as
written, and defending it would be exactly the failure standing instruction 6
names — a quantifier over the tree that no citation tests.

Amended to "Inventory", with a paragraph under it stating precisely what is
complete (the three explicitly-typed spellings, all sixty-four plus thirty-seven
plus three lines dispositioned) and what is not (the implicit class, measured
and escalated, not enumerated).

I would rather the report say what it can defend than carry a word two
adversaries have now falsified.

## N060 — The texture branches, and the claim they qualify.

`frontend_ir.cpp:1216`, `:1233`, `:1249`, in `TextureOpExpression::type_check`,
branches `kFetchTexel`, `kLoad`, `kStore`. Each loops over `ptr->num_dims` and
raises `ErrorEmitter(TaichiTypeError(), …)` unless `arg_type == PrimitiveType::i32`
exactly. Not a pin — a hard rejection.

Plan section 1.3 says rendering data is completely irrelevant, so there is a
real argument these are out of scope. That is a scope ruling and standing
instruction 3 forbids me making it, so they go in the inventory labelled, and
the ruling is escalated as item 15.

Independent of scope, they falsify an unqualified claim this report has carried
since revision 1 and repeats in its headline: that
`IndexExpression::type_check` accepts any integral index and the frontend does
not restrict index types. True of field and array indices. Not true of every
index the frontend accepts. Both section 1 and section 6 now carry the
qualification. That correction stands whichever way the scope ruling goes.

## N061 — `constant_fold.cpp:115`. Adversary right on the residual; I changed no count.

`adversary3-01-1.md` section 6 and `adversary3-01-2.md` section 6 both raise it.
Verified the mechanism at source: `:65` is `auto dt = lhs->val.dt;`, `:113` tests
that single `dt`, `:115` calls `val_int32()` on **both** operands, and nothing
between `:49` and `:125` reads `rhs->val.dt`. `is_good_type` (`:32-46`) is called
only on the unary path at `:194`. The caller at `:149-182` checks only that both
operands are `ConstStmt`. So the invariant rests entirely on the in-tree comment
at `:64`, which asserts it and does not prove it.

Reachability is unsettled and both adversaries say so explicitly, in both
directions. This report's section 3.2 finding is untouched: `:115` is guarded
64-bit dispatch and is not an abort site under today's invariant, which is what
revision 3 claimed against both round-one adversaries and what both round-three
adversaries uphold. The residual is a consequence of a width change, not a
defect today. Escalation 17. Twelve abort sites remains twelve.

## N062 — What this pass did not change, and what it deliberately left alone.

Upheld by both adversaries and not touched:

- The qualifier "above 2^31 in any single container" on
  `get_total_num_elements_towards_root` (section 3.9). Both call it load-bearing
  and correct; stripped of the clause the sentence would be false, because the
  `int64` accumulator does buy headroom on the product of already-narrowed
  factors. It was mine, from N050, and it survives.
- `make_mesh_block_local.cpp:526` filed in 3.7 rather than 3.1. Both rule both
  filings defensible; one calls 3.7 the sharper. Left where it is.
- Citation accuracy. Roughly seventy cited lines sampled by one adversary and a
  second independent sample by the other, no fault found in either. Nothing to
  fix.
- Sections 3.1, 3.2, 3.4 to 3.12, 4, 5's site set, 6's positive findings, 7.1,
  7.3, 7.5, and escalations 1 to 14. Untouched.

Section 4's three formation sites stand at three. None of the twenty-one
additions is a whole-structure extent; they are per-element and per-container
quantities. The structural finding is where it has been since revision 2.

No source file was touched. No other agent's file was touched.

---

# Revision 4, second part — planner rulings. Entries N063–N066.

Three rulings came back on the closing pass. None needed the project owner.

## N063 — Ruling: texture coordinates out of scope. Recorded, not inventoried.

I had inventoried `frontend_ir.cpp:1216`, `:1233`, `:1249` in section 3.3 and
escalated the scope call as item 15, because standing instruction 3 forbids an
agent deciding something is unnecessary. Ruling: out of scope.

Two grounds, and the second is new information I did not have:

1. Plan section 1.3 puts rendering entirely out of scope. I had this.
2. **The project owner has since confirmed positively that the foreseen data
   carries no graphic payload.** Particle attachments are strictly field
   interaction data and the modelling is purely mathematical. So this is not
   "rendering is deprioritised", it is "there is no texture work in this
   project". That is a positive confirmation rather than an inference from
   silence, and it is what makes the exclusion safe.

Actioned as the planner specified — same treatment as my four swept-and-excluded
sites, on the page with the reason, not dropped and not counted:

- Row removed from section 3.3. Count re-derived, not decremented: wrote the
  remaining fifty-one `file:line` pairs to a file, `wc -l` 51, `sort -u | wc -l`
  51, `cut -d: -f1 | sort -u | wc -l` 12; then re-extracted the table's own rows
  from the report text and diffed the two sets. They match. 31 rows, 51 sites.
- New section 3.14 records all three with content, mechanism, and the ruling
  including who made it and on what grounds.
- Section 3.13's ledger disposition for the three changed from "added, revision
  4" to "recorded, out of scope", and the ledger totals from 52+8+4=64 to
  52+5+4+3=64. Still generated from the table.
- Escalation 15 marked RESOLVED with the ruling recorded, not deleted, matching
  how 13 and 14 were closed.

**What I did NOT let the ruling take with it.** These three sites falsify an
unqualified claim this report carried from revision 1 to revision 3 — that the
frontend does not restrict the type of an index, citing
`IndexExpression::type_check` checking only `is_integral`. True of field and
array indices, false of every index the frontend accepts. Sections 1 and 6 keep
the qualification, and section 3.14 says explicitly why: a site being out of
scope for the *work* does not make it out of scope as *evidence* against a claim
about the tree. Deleting the qualification along with the row would have
restored a false sentence. Flagging that here because it is the kind of thing a
scope ruling silently eats if nobody watches it.

## N064 — Ruling: the implicit-spelling class is off this report. Boundary made explicit.

A dedicated pair is being dispatched to enumerate and classify the 78-line
residue as its own territory with its own adversarial round. Escalation 16 stays
open and stays as written, as the incoming pair's starting point. Explicit
instruction: **do not partially classify the residual**, because a half-closed
enumeration reads as finished and is worse than an open one.

I have not, and the risk was that it would *look* as though I had, because five
of the seventy-eight are already accounted for in this report. So section 3.13
now carries a status table making the boundary unambiguous:

| Spelling | Status |
|---|---|
| `PrimitiveType::i32`, 64 lines | closed, all dispositioned |
| `(int32)` / `int32(`, 37 lines | closed |
| `get_data_type<int32>()`, 3 lines | closed |
| implicit `TypedConstant(<host int>)`, 78 candidates | **open**, 5 accounted for, 73 unopened |

The five, and why each is not me starting the pass:

- `lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477` — named by both
  round-three adversaries before the class was identified, and added on the
  planner's explicit instruction in item 4 of the closing brief. Not selected by
  me out of the residue.
- `demote_operations.cpp:99` — the single line I opened, and I opened it to test
  whether the residue contains real sites at all. It does. It is quoted as
  evidence in escalation 16 and appears in **no inventory table**. Section 3.13
  now says in as many words that nobody should read it as the start of a pass.

The other 73 are unopened. I have not sampled them, counted them by class, or
formed a view on what fraction are real sites.

## N065 — For the incoming pair: the mechanism, stated once and plainly.

The planner asked that this be plain enough to start from rather than
rediscover. It is the whole of it:

**`TypedConstant` has an implicit `int32` constructor at `taichi/ir/type.h:582`:**

```
582:  explicit TypedConstant(int32 x) : dt(PrimitiveType::i32), val_i32(x) {
```

So `ConstStmt(TypedConstant(i))`, where `i` is a host `int`, produces an i32
constant with **no `i32` token anywhere on the line**. A grep for the type name
cannot see it. Neither can a grep for `(int32)`.

That is why `lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477` survived two
explore passes and three adversarial rounds unnamed by anybody, in a file both
reports cite for other reasons. Nobody was careless. Every sweep anyone ran was
shaped around a spelling, and this class has none.

**The class of miss is the finding, not the four instances.** Concretely, for
the incoming pair:

1. Four spellings exist. Three are closed by this report and the commands and
   counts are in section 3.13; do not re-run them, check them.
2. The fourth is `grep -rn "TypedConstant(" <territory>` minus every call whose
   first argument is a type. My filter removed first arguments of `dt`,
   `dst_type`, `data_type`, `load_data_type`, `PrimitiveType::…`, a `ret_type`,
   `get_data_type<int32>()`, and `…get_element_type()`. 78 candidates, 16 files.
3. The filter is mine and is not authoritative. It certainly over-collects:
   `constant_fold.cpp` contributes five macro continuation lines whose type
   argument sits on the previous line, and `ir_builder.cpp` six of the same
   shape. A line-spanning parse would cut those; I did not write one.
4. It may also under-collect. I only ever tested it against `TypedConstant`.
   Whether other constructors in this territory have implicit narrowing
   overloads is a question I did not ask and the incoming pair should.
5. `demote_operations.cpp:98-101` is a worked example of a real site in the
   residue, quoted in escalation 16. It is the shape to look for: a
   `ConstStmt(TypedConstant(i))` inside a loop over an element count, feeding a
   `MatrixPtrStmt` offset.

The reusable lesson, which is not about this class: **a count derived from one
grep is a count of one spelling, not of a phenomenon.** Plan section 10 item 7
asks that counts be derived from their enumeration. This pass adds the corollary
— check that the enumeration can see the whole class before trusting the count
it produces.

## N066 — Ruling: `constant_fold.cpp:115` stays escalated. No action.

Both round-three adversaries reached the same place independently: the residual
is real, and whether a mixed-width pair can reach the site is inference in both
directions. It changes no count. Escalation 17 stands as written. Section 3.2's
twelve abort sites is unchanged, and `:115` remains classified as guarded 64-bit
dispatch, which is what revision 3 established against both round-one
adversaries. Nothing done.

Still true after all three rulings: no source file touched, no other agent's file
touched. Section 4's three formation sites stand at three.
