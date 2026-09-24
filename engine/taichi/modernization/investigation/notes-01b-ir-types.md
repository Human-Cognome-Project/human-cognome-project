# Notes — explore agent 01B — frontend IR and type system

Contemporaneous working notes. Appended as I go. Territory: `taichi/ir/`,
`taichi/inc/`, `taichi/analysis/`, `taichi/transforms/`.

---

## 1. Starting point: `taichi/inc/constants.h`

Read in full. The whole file is 78 lines. Relevant declarations:

- `taichi/inc/constants.h:5` — `constexpr int taichi_max_num_indices = 12;`
- `taichi/inc/constants.h:12` — `constexpr int taichi_max_num_snodes = 1024;`
- `taichi/inc/constants.h:13` — `constexpr int kMaxNumSnodeTreesLlvm = 512;`
- `taichi/inc/constants.h:7` — `taichi_max_num_args = 8` (comment says legacy,
  opengl only)
- `taichi/inc/constants.h:10-11` — `taichi_max_num_args_total = 64`,
  `taichi_max_num_args_extra = 32`
- `taichi/inc/constants.h:16` — `taichi_global_tmp_buffer_size = 1024*1024`
  (std::size_t)
- `taichi/inc/constants.h:29` — `taichi_listgen_max_element_size = 1024`

First observation, unverified yet: all of these are `int`, not `size_t`, except
the ones explicitly declared `std::size_t`. `taichi_max_num_snodes` being `int`
means any expression mixing it with an index promotes to `int` (32-bit) unless
the other operand is wider. Need to check every use site in my territory.

Note `kMaxNumSnodeTreesLlvm = 512` is a *separate* ceiling from
`taichi_max_num_snodes` and is not mentioned in PROJECT-PLAN section 6.1. Flag
for Escalations if it turns out to be load-bearing in my territory.

Next: enumerate the territory and find all uses.

---

## 2. Use sites of the two named constants

`grep -rn "taichi_max_num_indices" taichi/ir taichi/inc taichi/analysis taichi/transforms`:

- `taichi/ir/snode.cpp:21` — loop bound in `insert_children`
- `taichi/ir/snode.cpp:90` — loop bound in `create_node`, extractor inference
- `taichi/ir/snode.cpp:105` — loop bound, dynamic SNode validation
- `taichi/ir/snode.h:28-30` — `Axis` ctor bounds check
- `taichi/ir/snode.h:78` — `AxisExtractor extractors[taichi_max_num_indices];`
  (fixed C array member)
- `taichi/ir/snode.h:81` — `int physical_index_position[taichi_max_num_indices]{};`
  (fixed C array member)
- `taichi/analysis/offline_cache_util.cpp:110` — loop bound, hash key emission
- `taichi/transforms/demote_dense_struct_fors.cpp:19` —
  `std::array<int, taichi_max_num_indices> total_shape;`
- `taichi/transforms/demote_dense_struct_fors.cpp:23` — loop bound
- `taichi/transforms/scalar_pointer_lowerer.cpp:33` —
  `std::array<int, taichi_max_num_indices> total_shape;`
- `taichi/transforms/scalar_pointer_lowerer.cpp:36` — loop bound
- `taichi/transforms/scalar_pointer_lowerer.cpp:40` —
  `std::array<bool, taichi_max_num_indices> is_first_extraction;`

SURPRISE (worth flagging): `grep -rn "taichi_max_num_snodes" taichi/` over the
WHOLE repo returns only three files:

- `taichi/inc/constants.h:12` (the declaration)
- `taichi/codegen/llvm/struct_llvm.cpp:266` (the assertion)
- `taichi/runtime/llvm/runtime_module/runtime.cpp:567-569` (the three arrays)

i.e. **`taichi_max_num_snodes` has ZERO use sites in my territory.** Section 6.1
of the plan touches nothing in `taichi/ir/`, `taichi/inc/` (beyond the
declaration itself), `taichi/analysis/` or `taichi/transforms/`. Verified by
grep across all of `taichi/`.

`kMaxNumSnodeTreesLlvm = 512` (`taichi/inc/constants.h:13`) likewise has exactly
one use site, `taichi/runtime/llvm/runtime_module/runtime.cpp:562-563`. Also
outside my territory. Recording it because it is a second, lower, undocumented
ceiling that section 6.1 does not name. -> Escalation.

## 3. `taichi/ir/snode.h` — the SNode record itself

Read in full (359 lines). Width-relevant fields:

- `taichi/ir/snode.h:41` — `int num_elements_from_root{1};`
- `taichi/ir/snode.h:45` — `int shape{1};`
- `taichi/ir/snode.h:49` — `int acc_shape{1};`
  All three are the per-axis extractor metadata, **32-bit**. `acc_shape` and
  `num_elements_from_root` are *accumulated products*, so they are exactly the
  quantities that overflow first as the structure grows.
- `taichi/ir/snode.h:79` — `std::vector<int> index_offsets;`
- `taichi/ir/snode.h:80` — `int num_active_indices{0};`
- `taichi/ir/snode.h:88-89` — `static std::atomic<int> counter; int id{0};`
  SNode ids are `int`. Relevant to 6.1 only in the sense that a raised SNode
  ceiling stays far inside `int` range; not a truncation risk.
- `taichi/ir/snode.h:97` — `int64 num_cells_per_container{1};` — this one IS
  already 64-bit.
- `taichi/ir/snode.h:99-100` — `std::size_t cell_size_bytes`,
  `std::size_t offset_bytes_in_parent_cell` — already 64-bit on LP64.
- `taichi/ir/snode.h:306-308` — `int64 max_num_elements()` returns
  `num_cells_per_container`, 64-bit.
- `taichi/ir/snode.h:310-315` — `get_total_num_elements_towards_root()`:
  ```
  int64 total_num_elemts = 1;
  for (auto *s = this; s != nullptr; s = s->parent)
    total_num_elemts *= (int)s->max_num_elements();
  ```
  **The accumulator is int64 but each factor is explicitly cast down to `int`.**
  A 64-bit-clean value is truncated on the way into a 64-bit accumulation. This
  is a live narrowing, not merely a declaration width. VERIFIED by reading.
- `taichi/ir/snode.h:317` — `int shape_along_axis(int i) const;` returns int.
- `taichi/ir/snode.h:329-334` — read/write accessors take
  `const std::vector<int> &i` for the index tuple. Index tuple is int32.

## 4. `taichi/ir/snode.cpp` — where the 32-bit limit is *stated in the source*

`taichi/ir/snode.cpp:89-100`, in `SNode::create_node`:

```cpp
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

This is upstream's own admission, in-tree, that **int64 indexing is not
supported**. It is a warning, not an error: the code proceeds and stores a
truncated `acc_shape`. This is the single most load-bearing line for item 6.2 in
my territory. VERIFIED.

Also `taichi/ir/snode.cpp:22-23` (`insert_children`):
`new_ch->extractors[i].num_elements_from_root *= extractors[i].num_elements_from_root;`
— int*int, silent wraparound, no guard at all. VERIFIED.

And `taichi/ir/snode.cpp:83` — `new_node.extractors[ind].num_elements_from_root *= sizes[i];`
same, `sizes` is `std::vector<int>`.

`taichi/ir/snode.cpp:174-177` — `shape_along_axis` returns
`extractor.num_elements_from_root`, i.e. hands out the 32-bit accumulated
product.

Next: the type system, `taichi/ir/type.h`.

---

## 5. The type system: can `taichi/ir/type.h` express 64-bit addressing?

Read `taichi/ir/type.h` in full (790 lines), plus `taichi/inc/data_type.inc.h`,
`taichi/inc/data_type_with_c_type.inc.h`, `taichi/inc/type_kind.inc.h`,
`taichi/ir/type_utils.h`, and the head of `taichi/ir/type_utils.cpp`.

**Answer, split into two halves.**

(a) *Scalar 64-bit integers: yes, fully present.*
- `taichi/inc/data_type.inc.h:7` `PER_TYPE(i64)`, `:12` `PER_TYPE(u64)`.
- `taichi/inc/data_type_with_c_type.inc.h:7,11` map them to `int64`/`uint64`.
- `taichi/ir/type_utils.cpp:59,64` `data_type_size` returns `sizeof(int64)` /
  `sizeof(uint64)`.
- `taichi/ir/type_utils.h:107,112,123,140,152-153,180-181` — `is_integral`,
  `is_signed`, `to_unsigned`, `get_max_value`, `get_min_value` all handle
  i64/u64.
- `taichi/ir/type.h:560,568,612,623,641,664,672` — `TypedConstant` has
  `val_i64`/`val_u64` storage and constructors, and the union is keyed on
  `uint64 value_bits` (`taichi/ir/type.h:557`), so constants are already
  64-bit-wide storage.
So an i64 index *value* is representable. Nothing in the type system forbids it.

(b) *Pointers/addresses: the type system carries NO width at all.*
- `taichi/ir/type.h:177-209` `PointerType` holds exactly three things: a
  `Type *pointee_` (`:206`), `int addr_space_{0}` with a `// TODO: make this an
  enum` (`:207`), and `bool is_bit_pointer_` (`:208`). **There is no bit-width
  field, and no accessor for one.** Address width is not a property the type
  system models.
- `taichi/ir/type_utils.cpp:30-35` — `data_type_size` opens with
  `t.set_is_pointer(false);` and a TODO reading "2. Support pointer types here."
  So the type system cannot even *report* the size of a pointer. VERIFIED.

Consequence (VERIFIED from the code, not inferred): the width of an address is
decided entirely downstream of the type system, by whichever backend lowers a
`PointerType`. `type.h` neither expresses nor constrains it. So for item 6.2,
`type.h` does not *block* 64-bit addressing, but it also provides no place to
*declare* it, and no way for a pass in `taichi/transforms/` to ask "how wide is
an address on this target?". That question has no answer inside my territory.

The plan's note in 6.2 about SPIR-V Int64 being an optional capability lands
exactly here: the natural place to express "this target's address width is 32"
vs "64" would be `PointerType`, and there is no such field.
-> Escalation (design decision, not mine to make).

Other 32-bit-typed things noticed in `type.h` while reading, recorded for the
inventory:
- `taichi/ir/type.h:222-227` `TensorType::get_num_elements()` — `int`
  accumulator over `int` shape.
- `taichi/ir/type.h:229-235,250` `TensorType` shape is `std::vector<int>`.
- `taichi/ir/type.h:307-321` `StructType::get_flattened_num_elements()` — `int`.
- `taichi/ir/type.h:530-532,545` `QuantArrayType::get_num_elements()` — `int`.
- `taichi/ir/type.h:243,257,305,336` — element *byte offsets* are `size_t`, so
  byte offsets inside a type are already 64-bit-clean. It is *element counts*
  that are `int`, not byte offsets.

DEAD END recorded: `taichi/inc/address_mode.inc.h` sounds relevant to addressing
but is not. It is `repeat`/`mirrored_repeat`/`clamp_to_edge`, i.e. texture
sampler wrap modes. Nothing to do with memory addressing. Do not chase it.

Next: the pointer-producing statements in `taichi/ir/statements.h`.

---

## 6. The address-producing statements in `taichi/ir/statements.h`

Read the relevant class definitions.

**`ExternalPtrStmt`** — `taichi/ir/statements.h:369-407`
- `:373` `std::vector<Stmt *> indices;` — indices are IR values, so their width
  is whatever `ret_type` they carry. Not fixed by the class.
- `:376` `int ndim;`
- `:379` `std::vector<int> element_shape;` — **32-bit shape**.

**`GlobalPtrStmt`** — `taichi/ir/statements.h:417-441`
- `:419-420` `SNode *snode; std::vector<Stmt *> indices;`
- No width declared here either; width lives in the index statements' ret_type.

**`MatrixOfGlobalPtrStmt`** — `taichi/ir/statements.h:450-482`
- `:456` `int dynamic_index_stride{0};` — **32-bit stride, in bytes**. This is a
  hardcoded 32-bit quantity on an address computation.

**`MatrixPtrStmt`** — `taichi/ir/statements.h:505-553`
- `:512-520` carries an explicit TODO admitting `offset` is sometimes bytes and
  sometimes an index. Recorded as a hazard for any width change: the two
  semantics would need different widths. Not mine to resolve. -> Escalation.
- `:531-536` `std::vector<int> get_origin_shape()` — 32-bit.

**`IntegerOffsetStmt`** — `taichi/ir/statements.h:1257-1272`
- `:1260` `int64 offset;` — **already 64-bit.** Good.

**`LinearizeStmt`** — `taichi/ir/statements.h:1277-1295`
- `:1280` `std::vector<int> strides;` — **the linearisation strides are 32-bit.**
  This is the statement whose whole documented purpose (`:1274-1276`, "All
  indices of an address fused together") is address linearisation, and its
  stride vector is `int`. Direct hit for item 6.2. VERIFIED.

**`SNodeLookupStmt`** — `taichi/ir/statements.h:1333-1361`
- `:1335-1338` `SNode *snode; Stmt *input_snode; Stmt *input_index; bool activate;`
- Width is carried by `input_index`'s ret_type. Nothing hardcoded in the class.

**`GetChStmt`** — `taichi/ir/statements.h:1366-1398`
- `:1370` `int chid;` — a child ordinal, not an address. Fine.

**`OffloadedStmt`** — `taichi/ir/statements.h:1403-1490`
- `:1411-1412` `std::size_t begin_offset{0}; std::size_t end_offset{0};` — 64-bit.
- `:1415-1416` **`int32 begin_value{0}; int32 end_value{0};`** — explicitly
  spelled `int32`, not `int`. These are the constant range-for bounds. A
  range-for cannot express a loop bound past 2^31 in the IR at all. VERIFIED.
- `:1439` `std::vector<int> index_offsets;`
- `:1447-1448` `std::size_t tls_size`, `bls_size` — 64-bit.

**`LoopIndexStmt`** — `taichi/ir/statements.h:1495-1502`
- `:1498` `int index;` — this is *which* index, an axis ordinal, not a value.

**`GlobalTemporaryStmt`** — `taichi/ir/statements.h:1595-1611`, `:1597`
`std::size_t offset;` — 64-bit.
**`ThreadLocalPtrStmt`** — `:1616-1632`, `:1618` `std::size_t offset;` — 64-bit.
**`BlockLocalPtrStmt`** — `:1637-1652`, `:1639` `Stmt *offset;` — IR value.

Pattern so far: *byte offsets* into buffers are already `size_t`/`int64`.
*Index values, shapes and strides* are `int`. The 32-bit assumption is
concentrated in the index/shape/stride side, not the byte-offset side.

Next: where the ret_type of these statements is actually assigned, i.e.
`taichi/ir/statements.cpp` and `taichi/transforms/type_check.cpp`.

---

## 7. `taichi/transforms/type_check.cpp` — THE enforcement point

This is the densest concentration of hardcoded 32-bit in the whole territory.
Every one of these is a literal `PrimitiveType::i32` written into a ret_type or
asserted on an operand. All VERIFIED by reading.

**Forced narrowing of indices (active truncation, not just declaration):**
- `taichi/transforms/type_check.cpp:147-156` — `visit(GlobalPtrStmt *)`:
  ```cpp
  for (int i = 0; i < stmt->indices.size(); i++) {
    if (!stmt->indices[i]->ret_type->is_primitive(PrimitiveTypeID::i32)) {
      ErrorEmitter(TaichiCastWarning(), stmt,
          fmt::format("Field index {} not int32, casting into int32 implicitly", i));
      stmt->indices[i] =
          insert_type_cast_before(stmt, stmt->indices[i], PrimitiveType::i32);
    }
  }
  ```
  **An i64 SNode index is compiled away to i32 here, with a warning.** This is
  the single hardest blocker for 6.2 in my territory. Even if a user produced an
  i64 index, the compiler inserts a cast down to i32 before the address is
  formed.
- `taichi/transforms/type_check.cpp:457-463` — `visit(ExternalPtrStmt *)`: same
  forced cast to i32 for every external-array index, here with no warning at all,
  just a silent `insert_type_cast_before(..., PrimitiveType::i32)`.
- `taichi/transforms/type_check.cpp:159-163` — `visit(MatrixPtrStmt *)`:
  `TI_ASSERT(stmt->offset->ret_type.get_element_type()->is_primitive(PrimitiveTypeID::i32));`
  A hard assert, not a cast. An i64 matrix offset trips an assertion.

**Hardcoded i32 result types on address arithmetic:**
- `taichi/transforms/type_check.cpp:520-522` — `visit(LinearizeStmt *)`:
  `stmt->ret_type = PrimitiveType::i32;`
  The fused linearised address is typed i32. Direct hit for 6.2.
- `taichi/transforms/type_check.cpp:524-526` — `visit(IntegerOffsetStmt *)`:
  `stmt->ret_type = PrimitiveType::i32;`
  NOTE THE INCONSISTENCY: the statement's own `offset` field is declared `int64`
  at `taichi/ir/statements.h:1260`, but its result type is i32. A 64-bit offset
  constant is carried in the IR node and then typed as 32-bit.
- `taichi/transforms/type_check.cpp:466-468` — `visit(LoopIndexStmt *)`: i32.
- `taichi/transforms/type_check.cpp:470-472` — `visit(LoopLinearIndexStmt *)`:
  i32. This is the *linear* loop index, i.e. exactly the flattened offset in a
  struct-for. Typed i32.
- `taichi/transforms/type_check.cpp:474-476` — `visit(BlockCornerIndexStmt *)`: i32.
- `taichi/transforms/type_check.cpp:118-120` —
  `visit(ExternalTensorShapeAlongAxisStmt *)`: i32. External array shapes are
  reported as i32.
- `taichi/transforms/type_check.cpp:169-171` — `visit(RangeForStmt *)`:
  `mark_as_if_const(stmt->begin, PrimitiveType::i32);` and same for `end`.
  Range-for bounds are i32. Consistent with `OffloadedStmt::begin_value/end_value`
  being `int32` at `taichi/ir/statements.h:1415-1416`.
- `taichi/transforms/type_check.cpp:113-115` — `visit(SNodeOpStmt *)` default
  branch: i32. (Includes `append`, `length` etc.)
- `taichi/transforms/type_check.cpp:565-568` — `visit(InternalFuncStmt *)`: i32.

**Counter-example worth noting:** `taichi/transforms/type_check.cpp:105-107` —
`SNodeOpType::get_addr` yields `PrimitiveType::u64`. So a *materialised* address
value is already 64-bit when it is handed to the user. The 32-bit-ness is in the
*computation* of addresses, not in the representation of a finished one.

**Pointer types are produced without any width:**
- `:478-481` `GetRootStmt` -> `get_pointer_type(PrimitiveType::gen)`
- `:483-495` `SNodeLookupStmt` -> `get_pointer_type(...)`
- `:497-514` `GetChStmt` -> `get_pointer_type(...)`
All go through `TypeFactory::get_pointer_type`, which (per section 5) has no
width parameter.

Next: `taichi/transforms/scalar_pointer_lowerer.cpp` and
`taichi/transforms/lower_access.cpp`, which are where GlobalPtrStmt actually
becomes SNodeLookupStmt/GetChStmt.

---

## 8. `taichi/transforms/scalar_pointer_lowerer.cpp` — index extraction

Read in full (88 lines). This is the pass that turns a set of user indices into
the GetRoot -> Linearize -> SNodeLookup -> GetCh chain.

- `:33` `std::array<int, taichi_max_num_indices> total_shape;` — **32-bit**.
- `:37` `total_shape[j] *= s->extractors[j].shape;` — int*int accumulation of the
  total shape along each axis across the whole SNode path. Silent overflow, no
  guard here at all.
- `:56` `std::vector<int> strides;` — 32-bit, fed straight into `LinearizeStmt`
  at `:82`, matching `LinearizeStmt::strides` being `std::vector<int>`.
- `:63-65` `const int prev = total_shape[k]; total_shape[k] /= ...; const int next = total_shape[k];`
  — the divisors/moduli used to peel indices apart are `int`.
- `:73` `generate_mod(lowered_, indices_[k_], prev)` and `:75`
  `generate_div(lowered_, extracted, next)` — both take `int`.
- `:78` `strides.push_back(snode->extractors[k].shape);` — int.

`taichi/transforms/utils.cpp` (the helpers used above), read in full:
- `:5` `Stmt *generate_mod(VecStatement *stmts, Stmt *x, int y)` — divisor is
  `int`.
- `:7` `TypedConstant(y - 1)` — resolves to `TypedConstant(int32)`
  (`taichi/ir/type.h:582`), so the mask constant is typed **i32**.
- `:10` `TypedConstant(y)` — same, i32.
- `:14` `Stmt *generate_div(VecStatement *stmts, Stmt *x, int y)` — `int`.
- `:16-17` `TypedConstant(PrimitiveType::i32, bit::log2int(y))` — **explicitly
  i32**, the shift amount for the power-of-two fast path.
- `:20` `TypedConstant(y)` — i32.
- `taichi/transforms/utils.h:8-9` — the declared signatures, `int y`.

So every divisor, modulus and shift used to decompose an index into per-level
coordinates is an i32 constant. VERIFIED.

## 9. `taichi/transforms/demote_dense_struct_fors.cpp` — a HARD 32-bit ceiling

Read in full (140 lines).

- `:18` `int64 total_n = 1;` — accumulator is 64-bit.
- `:19` `std::array<int, taichi_max_num_indices> total_shape;` — 32-bit.
- `:24` `total_shape[j] *= snode->extractors[j].shape;` — int*int.
- `:26` `total_n *= snode->num_cells_per_container;` — int64 *= int64, clean.
- `:29` **`TI_ASSERT(total_n <= std::numeric_limits<int>::max());`**
  This is a **hard assertion**, not a warning. A dense struct-for over more than
  2^31 cells aborts at compile time. Contrast with `taichi/ir/snode.cpp:95-100`
  which only warns. So the frontend has two different reactions to the same
  overflow: a warning when the SNode is built, a hard abort when a dense
  struct-for over it is demoted. VERIFIED. -> This is the concrete, immediate
  ceiling on structure size for the dense path.
- `:35` `offloaded->end_value = total_n;` — assigns `int64` into the `int32`
  field at `taichi/ir/statements.h:1416`. Implicit narrowing. Only safe because
  of the assert on `:29`.
- `:60,63` `generate_mod(&body_header, extracted, total_n)` /
  `generate_div(&body_header, extracted, total_n)` — passes an `int64 total_n`
  into an `int y` parameter. Narrowing at the call site.
- `:74` `generate_mod(&body_header, index, ext.acc_shape * ext.shape)` — int*int,
  and `acc_shape` is the already-truncated value from
  `taichi/ir/snode.cpp:92`.
- `:76` `generate_div(&body_header, index, ext.acc_shape)`.
- `:79` `TypedConstant(total_shape[p])` — i32 multiplier constant.

Next: `check_out_of_bound.cpp`, `lower_access.cpp`,
`handle_external_ptr_boundary.cpp`, `offload.cpp`.

---

## 10. `lower_access.cpp`, `check_out_of_bound.cpp`, `handle_external_ptr_boundary.cpp`

**`taichi/transforms/lower_access.cpp`** — read `PtrLowererImpl::handle_snode_at_level`
at `:227-281`. It is width-NEUTRAL: it takes the `LinearizeStmt *linearized`
produced by `ScalarPointerLowerer` and threads it into `SNodeOpStmt` (`:257`)
and `SNodeLookupStmt` (`:268`) unmodified. `:270` `int chid` is a child ordinal.
No width assumption originates here; it inherits whatever `LinearizeStmt`
carries (i32, per `taichi/transforms/type_check.cpp:521`). Recording this
explicitly because it is a place I expected a problem and did not find one.

**`taichi/transforms/check_out_of_bound.cpp`** — read `:40-230`.
- `:54-56` `int flattened_element = 1; flattened_element *= stmt->element_shape[i];`
  — int accumulation.
- `:74` `TypedConstant(flattened_element)` — i32 upper bound constant.
- `:123-126` `int size_i = snode->shape_along_axis(i); int upper_bound_i = size_i;`
  then `TypedConstant(upper_bound_i)` — the SNode bounds check constant is i32,
  and its source `shape_along_axis` returns `int`
  (`taichi/ir/snode.h:317`, `taichi/ir/snode.cpp:174-177`).
- `:115` `int offset_i = has_offset ? snode->index_offsets[i] : 0;` — i32.
- `:170-175` `int max_valid_index = 1; max_valid_index *= matrix_shape[i];` — i32.
- `:91`, `:153`, `:201` — the assertion message format strings hardcode `"%d"`
  for every printed index. If index values widened to 64-bit these format
  specifiers would be wrong at runtime. Recording as a downstream consequence
  inside my territory. VERIFIED.
- `:220` `compare->ret_type = PrimitiveType::i32;` — pow guard, not addressing.

**`taichi/transforms/handle_external_ptr_boundary.cpp`** — read in full (114
lines).
- `:30,39,71` `TypedConstant(0)` / `TypedConstant(1)` — i32 constants used as
  clamp bounds against index values.
- `:62-67` `int max_valid_index` — i32.
Whole pass clamps indices using i32 constants.

## 11. `TypeFactory` — the pointer type has nowhere to put a width

`taichi/ir/type_factory.h:43`:
`Type *get_pointer_type(Type *element, bool is_bit_pointer = false);`

`taichi/ir/type_factory.cpp:86-95` — pointer types are **interned on the key
`std::make_pair(element, is_bit_pointer)`**. Two pointers to the same pointee
are the same object. There is no width in the key and no width in `PointerType`
(section 5). So even if a width field were added to `PointerType`, the interning
cache at `taichi/ir/type_factory.cpp:89` would have to change too or the two
widths would alias. VERIFIED by reading. -> Escalation, design decision.

`taichi/ir/type_factory.cpp:160-177` `get_primitive_int_type(int bits, bool is_signed)`
already handles `bits == 64` (`:168-169`), so requesting a 64-bit primitive is
supported today.

`taichi/ir/type_factory.cpp:198-215` `get_ndarray_struct_type`:
- `:204` `shape_members.push_back({PrimitiveType::i32, fmt::format("dim_{}", i)});`
  **The ndarray shape descriptor struct hardcodes i32 per dimension.** This is
  the type of the shape metadata handed across the ABI for external arrays. An
  ndarray dimension larger than 2^31 cannot be described. VERIFIED.
- `:209` `get_pointer_type(dt->get_compute_type())` for `data_ptr` — no width.

Other hardcoded i32 ret_types found by
`grep -rn "PrimitiveType::i32\|PrimitiveTypeID::i32" taichi/ir taichi/analysis taichi/transforms taichi/inc`:
- `taichi/ir/statements.h:1684` `InternalFuncStmt` default ret_type.
- `taichi/ir/statements.h:2064` `MeshIndexConversionStmt` ret_type — mesh index.
- `taichi/ir/statements.h:2083` `MeshPatchIndexStmt` ret_type.
- `taichi/ir/statements.cpp:146` `SNodeOpStmt` ctor sets `element_type() = PrimitiveType::i32;`
- `taichi/ir/type_utils.h:40` — `get_data_type<int32>()`, definitional, fine.
- `taichi/ir/type.cpp:238` — quant compute type defaults to i32/u32.
- `taichi/transforms/demote_mesh_statements.cpp:16,54,131` — mesh index casts to i32.
- `taichi/transforms/make_mesh_thread_local.cpp:42,106,114` — mesh offsets cast to i32.
- `taichi/transforms/make_mesh_block_local.cpp:212,376,399,526` — mesh mapping
  data type pinned to i32 (`:526` `mapping_data_type_ = PrimitiveType::i32;`).
- `taichi/transforms/make_cpu_multithreaded_range_for.cpp:68-93` — the CPU
  range-for splitter builds every one of its bounds/step constants as
  `TypedConstant(PrimitiveType::i32, ...)`, including reading
  `offloaded->begin_value`/`end_value` (`:81,:90`) and building
  `GlobalTemporaryStmt(offset, PrimitiveType::i32)` (`:84,:93`). The parallel
  range-for iteration space is i32 end to end.
- `taichi/transforms/scalarize.cpp:471,473` — i32 loop constants.
- `taichi/transforms/auto_diff.cpp:587,639,646,787,839,846,1828,1857` — i32
  index constants for tensor element access in autodiff.

---

## 12. `taichi/ir/frontend_ir.cpp` — where index widths ORIGINATE

This answers the "where do index and address types originate" part of my brief
most directly.

**Loop variables are born as pointer-to-i32.**
`taichi/ir/frontend_ir.cpp:149-153`:
```cpp
void FrontendForStmt::add_loop_var(const Expr &loop_var) {
  loop_var_ids.push_back(loop_var.cast<IdExpression>()->id);
  loop_var.expr->ret_type =
      TypeFactory::get_instance().get_pointer_type(PrimitiveType::i32);
}
```
Every `ti.ndrange`/struct-for/range-for loop variable is typed i32 at birth.
This is the origin point. VERIFIED.

**The frontend itself does NOT force i32 on indices.** Notable nuance.
`taichi/ir/frontend_ir.cpp:936-947` — `IndexExpression::type_check` only checks
`is_integral(expr_type)` and errors if not integral. It accepts i64. Likewise
`taichi/transforms/frontend_type_check.cpp:71-78` only *warns* ("Field index {}
not int32, casting into int32 implicitly") and does not modify anything.
The actual truncation happens later, in
`taichi/transforms/type_check.cpp:147-156`. So the width narrowing is a
mid-level IR pass decision, not a frontend language restriction.
This is a genuinely useful separation for item 6.2 and I want it on record.

**Index statement construction:**
- `taichi/ir/frontend_ir.cpp:656-669` `make_index_stmts` — `const std::vector<int> &offsets`
  and `TypedConstant(offsets[i])` at `:663`, so the index offset applied to a
  user coordinate is an i32 constant.
- `taichi/ir/frontend_ir.cpp:671-676` `make_field_access` -> `GlobalPtrStmt`.
- `taichi/ir/frontend_ir.cpp:678-691` `make_matrix_field_access` ->
  `MatrixOfGlobalPtrStmt`, passing `matrix_field.dynamic_index_stride`
  (an `int`, `taichi/ir/frontend_ir.h:617`).
- `taichi/ir/frontend_ir.cpp:693-717` `make_ndarray_access` -> `ExternalPtrStmt`,
  with `expr->dt.get_shape()` (a `std::vector<int>`) as `element_shape`.
- `taichi/ir/frontend_ir.cpp:719-750` `make_tensor_access_single_element` —
  builds the flattened tensor offset:
  - `:732` `TypedConstant(0)` seed — i32.
  - `:735` `TypedConstant(shape[i])` — i32 multiplier.
  - `:742-747` constant-index path: `int offset = 0; offset = offset * shape[i] + ...;`
    then `TypedConstant(offset)` — **the whole flattened tensor offset is
    computed in `int` and emitted as an i32 constant.** VERIFIED.
- `taichi/ir/frontend_ir.h:615` `std::vector<int> element_shape;` (MatrixFieldExpression)
- `taichi/ir/frontend_ir.h:617` `int dynamic_index_stride{0};`
- `taichi/ir/frontend_ir.h:529` `int ndim;` (ExternalTensorExpression)
- `taichi/ir/frontend_ir.h:676` `std::vector<int> ret_shape;` (IndexExpression)

**Other frontend i32 pins (recorded, mostly not addressing):**
- `taichi/ir/frontend_ir.cpp:1108-1115` `SNodeOpExpression::type_check` — mirrors
  `type_check.cpp:105-115`; `get_addr` -> **u64**, everything else -> i32.
- `taichi/ir/frontend_ir.cpp:1306` `ExternalTensorShapeAlongAxisExpression` -> i32.
  So a queried external array dimension is i32 at the frontend too.
- `taichi/ir/frontend_ir.cpp:1358,1362,1392` — mesh patch index, mesh relation
  access, mesh index conversion, all i32.
- `taichi/ir/frontend_ir.cpp:1216,1233,1249` — texture fetch_texel/load/store
  require i32 index args. NOT RELEVANT: PROJECT-PLAN 1.3 puts rendering
  explicitly out of scope. Recording only so the inventory is complete.
- `taichi/ir/expr.cpp:93` — `ConstExpression(PrimitiveType::i32, x)` for the
  int literal constructor.

---

## 13. Complete map of `AxisExtractor` field consumers (repo-wide)

`grep -rn "acc_shape\|num_elements_from_root\|extractors\[" taichi/` minus
`taichi/python`. Producers and consumers, so the boundary of my territory is
visible:

IN territory (producers):
- `taichi/ir/snode.cpp:22-23,82-84,89-101,175-176` — sets everything.
- `taichi/ir/snode.h:41,45,49,78` — declarations.

IN territory (consumers):
- `taichi/ir/scratch_pad.h:106,214` — `.shape`
- `taichi/analysis/bls_analyzer.cpp:27` — `.shape - 1` as a bound
- `taichi/analysis/offline_cache_util.cpp:111-115` — serialises all four fields
  into the offline cache key. NOTE: widening these fields changes the cache key
  layout. -> Escalation (cache invalidation is outside my assignment).
- `taichi/transforms/demote_dense_struct_fors.cpp:24,67,74,76`
- `taichi/transforms/scalar_pointer_lowerer.cpp:37,60,64,78`

OUT of territory (the seam — for agent 02):
- `taichi/codegen/spirv/snode_struct_compiler.cpp:121`
  `sn_desc.total_num_cells_from_root *= e.num_elements_from_root;`
- `taichi/codegen/llvm/struct_llvm.cpp:173-176,184` — turns
  `acc_shape * shape` and `acc_shape` into LLVM constants for the
  refine-coordinates function. `tlctx_->get_constant(int)` -> i32 constant.

So the truncated `acc_shape` written at `taichi/ir/snode.cpp:92` is consumed by
BOTH codegen paths. The truncation at that one line propagates into generated
code on every backend.

## 14. Range-for bounds: a hard assert, not a truncation

`grep -rn "begin_value\|end_value\|begin_offset\|end_offset" taichi/ir taichi/analysis taichi/transforms`:

- `taichi/ir/statements.h:1415-1416` — `int32 begin_value{0}; int32 end_value{0};`
- `taichi/ir/statements.cpp:392-397` — copied verbatim in `OffloadedStmt::clone`.
- `taichi/transforms/offload.cpp:107-117`:
  ```cpp
  if (auto val = s->begin->cast<ConstStmt>()) { ... offloaded->begin_value = val->val.val_int32(); }
  if (auto val = s->end->cast<ConstStmt>())   { ... offloaded->end_value   = val->val.val_int32(); }
  ```
  `TypedConstant::val_int32()` at `taichi/ir/type.cpp:462-465` begins with
  `TI_ASSERT(get_data_type<int32>() == dt);`. So an i64 range bound does not
  silently truncate here — **it trips an assertion**. VERIFIED by reading both
  sites. This is a hard block, and a loud one.
- `taichi/transforms/demote_dense_struct_fors.cpp:34-35` — assigns `int64 total_n`
  into `int32 end_value`, guarded by the assert at `:29`.
- `taichi/transforms/demote_no_access_mesh_fors.cpp:33-34` — same fields, mesh path.
- `taichi/transforms/make_cpu_multithreaded_range_for.cpp:79-93,138-141` — reads
  them back and rebuilds i32 constants; sets its own bounds from
  `config.cpu_max_num_threads`.
- `taichi/transforms/ir_printer.cpp:687-698` — printing only.
- `taichi/transforms/offload.cpp:501-522` — the non-const path uses
  `begin_offset`/`end_offset`, which ARE `std::size_t`. So a *dynamic* range
  bound is passed through a 64-bit global-temporary offset, while a *constant*
  one is pinned to i32. Asymmetry recorded.

## 15. `taichi/analysis/value_diff.cpp` — silent loss of analysis on i64

- `taichi/ir/analysis.h:15-53` `class DiffRange` — `int coeff; int low, high;`
  (`:20-21`). 32-bit.
- `taichi/analysis/value_diff.cpp:71-76` — reads `val.val_int()` (returns
  `int64`, `taichi/ir/type.cpp:522`) and stores into the `int` low/high of
  `DiffRange`. Narrowing.
- `taichi/analysis/value_diff.cpp:81-87` — `visit(ConstStmt *)`:
  **only handles `PrimitiveTypeID::i32`**; anything else yields a default
  (unrelated) `DiffRange()`.
- `taichi/analysis/value_diff.cpp:142-146` — `FindDirectValueBaseAndOffset`, same,
  i32 only. Its `ret_type` is `std::tuple<bool, Stmt *, int>` (`:131`).

Consequence, VERIFIED by tracing the call site: `value_diff_loop_index` is used
at `taichi/transforms/lower_access.cpp:237` to decide `on_loop_tree`, i.e.
whether a struct-for access can skip activation. If indices became i64, these
visitors would fall through to the unrelated branch, `on_loop_tree` would go
false, and **every access would be conservatively activated**. That is
correctness-preserving but a serious throughput regression, which matters
directly under PROJECT-PLAN 6.4. Recording prominently.

## 16. `taichi/ir/scratch_pad.h` — all-int, but bounded by shared memory

- `:34-38` `struct BoundRange { int low{0}; int high{0}; int range() ... }`
- `:47,51,53` `std::vector<int> coefficients / pad_size / block_size`
- `:75-76,107-108` `std::numeric_limits<int>::max()/min()` sentinels
- `:97-98,131-147` `int size`, `int pad_size_linear()`, `int block_size_linear()`
- `:149-156` `int linearized_index(const std::vector<int> &indices)` — an
  int-typed linearised address.
- `:182` `inline int div_floor(int a, int b)`
- `:229-234` `generate_address_code(SNode *, const std::vector<int> &indices)`,
  `int offset = 0;`

Assessment: this is block-local storage (BLS), sized against GPU shared memory
(tens of KB, cf. `taichi/inc/constants.h:39` `default_shared_mem_size = 65536`).
32-bit here is not a scale limit. Recording it for inventory completeness but
NOT flagging it as a blocker. I am explicitly not proposing anything be changed
here.

---

## 17. Hardcoded, non-derived widths (the "hardcoded rather than derived" ask)

Found by `grep -rn "int32\b" taichi/ir taichi/inc taichi/analysis taichi/transforms`
and following up.

**`taichi/transforms/simplify.cpp:222-273` — `sizeof(int32)` as the cell stride.**
```cpp
// SNodeLookupStmt, :232-239
for (int i = 0; i < (int)snode->ch.size(); i++) {
  TI_ASSERT(snode->ch[i]->type == SNodeType::place);
  TI_ASSERT(snode->ch[i]->dt->is_primitive(PrimitiveTypeID::i32) ||
            snode->ch[i]->dt->is_primitive(PrimitiveTypeID::f32));
}
auto offset_stmt = Stmt::make<IntegerOffsetStmt>(
    stmt, previous_offset->offset * sizeof(int32) * (snode->ch.size()));
```
and `:260-261` for `GetChStmt`:
`stmt->chid * sizeof(int32) + previous_offset->offset`.

The byte stride of an SNode cell is **hardcoded as 4** rather than derived from
`data_type_size(snode->ch[i]->dt)` or from `snode->cell_size_bytes`
(`taichi/ir/snode.h:99`, which exists and is `std::size_t`). The assertion at
`:233-235` is what keeps this correct: it restricts the fold to SNodes whose
children are all i32 or f32. So this is a *hardcoded* width guarded by a
narrowing assertion. VERIFIED. Exactly the "hardcoded rather than derived"
category my brief asks for. I am NOT proposing a change; recording only.

**`taichi/ir/statements.h:1787` — `sizeof(int32)` as the AD-stack header size:**
`return sizeof(int32) + entry_size_in_bytes() * max_size;`
The stack's element-count header is hardcoded 4 bytes. Everything else there is
`std::size_t` (`:1770,1778,1782,1786`).

**BLS byte offsets narrowed to int32 on the way into constants:**
- `taichi/transforms/make_block_local.cpp:157,334` —
  `TypedConstant((int32)bls_offset_in_bytes)`
- `taichi/transforms/make_mesh_block_local.cpp:80,118,200,265,301,575` — same
  pattern, `(int32)mapping_bls_offset_in_bytes_` / `int32(offset_in_bytes)`.
Bounded by shared-memory size, so not a scale limit, but the cast is explicit.

**`MatrixPtrStmt::offset` is read back as strict i32 in five passes.** All of
these call `val.val_int32()`, which asserts `dt == i32`
(`taichi/ir/type.cpp:462-465`):
- `taichi/transforms/scalarize.cpp:162,1097,1120,1147,1231,1311`
- `taichi/transforms/auto_diff.cpp:576,776,1770`
These are consistent with the hard assert at `taichi/transforms/type_check.cpp:160`.

**`taichi/transforms/lower_ast.cpp:177,363`** — `TypedConstant((int32)0xFFFFFFFF)`
as an all-ones loop mask. A 32-bit mask literal. Used for the vectorisation mask
(`:168` `AllocaStmt(PrimitiveType::i32)`), not for addressing.

**`taichi/ir/control_flow_graph.cpp:462`** — `->val.val_int32();` in CFG
reaching-value analysis. Same strict-i32 assertion exposure.

**`taichi/ir/ir_builder.h:133,135` / `taichi/ir/ir_builder.cpp:141-155`** —
`get_int32(int32)` / `get_uint32(uint32)`; `taichi/ir/ir_builder.cpp:143` builds
`get_primitive_type(PrimitiveTypeID::i32)`. There is no `get_int64` convenience
in the builder. Checked: `grep -n "get_int64" taichi/ir/ir_builder.h` — see next
entry.

## 18. Other constants from `taichi/inc/constants.h` used in my territory

Checked each constant individually. Only three appear in my territory:
- `taichi_global_tmp_buffer_size` — `taichi/transforms/offload.cpp:358`
  `TI_ASSERT(global_offset_ < taichi_global_tmp_buffer_size);` inside
  `allocate_global` (`:344-360`). A hard 1 MB cap on all global temporaries in a
  kernel. `global_offset_` is `std::size_t` and `data_type_size` returns `int`
  (`taichi/ir/type_utils.h:13`). Recording; not part of 6.1 or 6.2 as written.
- `taichi_max_gpu_block_dim` — `taichi/ir/frontend_ir.cpp:132`
  `TI_ASSERT(block_dim <= taichi_max_gpu_block_dim);`
- `default_shared_mem_size` — `taichi/transforms/make_mesh_block_local.cpp:434`

All other constants in `taichi/inc/constants.h` (`taichi_max_num_args`,
`taichi_max_num_args_total`, `taichi_max_num_args_extra`,
`taichi_listgen_max_element_size`, `taichi_max_num_mem_requests`,
`taichi_result_buffer_*`, `taichi_max_num_ret_value`, `taichi_page_size`,
`taichi_error_message_max_*`, `cuda_dynamic_shared_array_threshold_bytes`)
have **zero** use sites in `taichi/ir`, `taichi/analysis`, `taichi/transforms`.
VERIFIED by per-constant grep.

## 19. Analysis directory sweep

`grep -rn "val_int32\|val_i32\|int32\|numeric_limits" taichi/analysis/`:
only `taichi/analysis/value_diff.cpp:83,144` (covered in entry 15) and
`taichi/analysis/gather_uniquely_accessed_pointers.cpp:65` (a `uint32_t` cast of
a decoration enum, not addressing).

`taichi/analysis/alias_analysis.cpp` — reasons about pointer identity, not
widths. `:59-60` calls `value_diff_ptr_index` on `MatrixPtrStmt::offset`, which
routes into the i32-only `FindDirectValueBaseAndOffset`
(`taichi/analysis/value_diff.cpp:142-146`, `:189-200`). So alias analysis also
degrades to "uncertain" on non-i32 offsets. Same class of throughput regression
as entry 15. VERIFIED by following the call chain.

---

## 20. Corrections and things that ARE already 64-bit

Correcting my own entry 17: `IRBuilder` **does** have 64-bit constant helpers.
`taichi/ir/ir_builder.h:134,136` declare `get_int64(int64)` and
`get_uint64(uint64)`, implemented at `taichi/ir/ir_builder.cpp:147,159`. Only
`taichi/ir/ir_builder.cpp:143` (inside `get_int32`) pins i32. So the builder is
not a blocker.

Places in my territory that are ALREADY 64-bit-clean, recorded so the report can
say what does NOT need touching:
- `taichi/ir/statements.h:1260` `IntegerOffsetStmt::offset` is `int64`.
- `taichi/ir/statements.h:1411-1412,1447-1448,1597,1618,1770` — `std::size_t`
  byte offsets/sizes.
- `taichi/ir/snode.h:97,99-100,306` — `num_cells_per_container` (`int64`),
  `cell_size_bytes`, `offset_bytes_in_parent_cell` (`std::size_t`),
  `max_num_elements()` (`int64`).
- `taichi/transforms/type_check.cpp:105-107` and
  `taichi/ir/frontend_ir.cpp:1109-1110` — `SNodeOpType::get_addr` yields u64.
- `taichi/transforms/lower_access.cpp:162-169` — the `get_addr` lowering already
  emits `UnaryOpType::cast_bits` to `PrimitiveTypeID::u64` (`:166-167`). So an
  address-as-value is materialised at 64 bits today.
- `taichi/transforms/demote_dense_struct_fors.cpp:18,26` — int64 accumulator.
- `taichi/transforms/offload.cpp:218-221,233,237` — uses `int64`
  `max_num_elements()`; note it narrows into `int block_dim`
  (`taichi/ir/statements.h:1418`) at `:236-237,244`, harmless since block dims
  are capped at `taichi_max_gpu_block_dim = 1024`.
- `taichi/transforms/alg_simp.cpp:118,132` and
  `taichi/transforms/binary_op_simplify.cpp:66` — already use `val_as_int64()`
  and 64-bit masks, so strength reduction is width-agnostic.
- `taichi/ir/type.h:557` — `TypedConstant`'s union is keyed on `uint64
  value_bits`, so no storage change is needed to hold a 64-bit index constant.

## 21. `get_total_num_elements_towards_root` has no callers

`grep -rn "get_total_num_elements_towards_root" .` across the whole tree returns
exactly one hit: the definition at `taichi/ir/snode.h:310`. No call sites
anywhere, including Python bindings. Recording the fact only. Per standing
instruction 3 in PROJECT-PLAN section 10 I make no recommendation about it. Its
`(int)` narrowing at `:313` is therefore currently inert. -> Escalation, because
"currently inert" is a judgement about whether it needs fixing, which is not
mine.

## 22. Seam with codegen (agent 02's territory) — for the adversaries

The frontend hands these 32-bit things across the boundary:
- `snode->extractors[i].acc_shape` / `.shape` (int) ->
  `taichi/codegen/llvm/struct_llvm.cpp:173-176,184`
- `snode->extractors[i].num_elements_from_root` (int) ->
  `taichi/codegen/spirv/snode_struct_compiler.cpp:121`
- `LinearizeStmt` with i32 ret_type and `std::vector<int> strides`
- `SNodeLookupStmt::input_index`, i32
- `GlobalPtrStmt::indices`, forcibly cast to i32
- `OffloadedStmt::begin_value/end_value`, `int32`
- `snode->max_num_elements()` (int64) -> `taichi/codegen/llvm/codegen_llvm.cpp:280-281`
  `tlctx->get_constant(...)`, and `taichi/codegen/llvm/struct_llvm.cpp:72,99,106,108`.
  NOTE: this one is int64 on the frontend side; what codegen does with it is
  agent 02's to determine.

---

## 23. IMPORTANT: I nearly got `taichi_max_num_indices` wrong

I was about to conclude that `taichi_max_num_indices` is host-side-only
compile-time metadata, on the basis that `extractors[]` and
`physical_index_position[]` never appear under `taichi/runtime/`
(verified by grep). **That conclusion is wrong.**

A repo-wide grep including all directories turns up:
`taichi/runtime/llvm/runtime_module/runtime.cpp:288-290`:
```cpp
struct PhysicalCoordinates {
  i32 val[taichi_max_num_indices];
};
```
and `taichi/runtime/llvm/runtime_module/runtime.cpp:1012`
`for (int i = 0; i < taichi_max_num_indices; i++)`.

So `taichi_max_num_indices` **does** size a device-side struct, and each slot in
it is `i32`. The struct is mirrored in codegen at
`taichi/codegen/llvm/codegen_llvm.cpp:2356-2376`, which asserts the LLVM struct
shape matches and GEPs into it for `BlockCornerIndexStmt`.

This is agent 03's and agent 02's territory to own, but it changes what I can
say about my own: raising `taichi_max_num_indices` is NOT free, and the per-axis
device coordinate storage is hardcoded i32, matching
`taichi/transforms/type_check.cpp:474-476`
(`BlockCornerIndexStmt` -> i32) and `:466-468` (`LoopIndexStmt` -> i32) on my
side. The frontend i32 and the runtime i32 are the same decision seen from two
ends. Recording this as the single most important cross-territory correspondence
I found.

Full list of `taichi_max_num_indices` sites outside my territory, for
completeness of the seam:
- `taichi/program/launch_context_builder.cpp:230,247,269,302,322` — asserts an
  ndarray's `shape.size() <= taichi_max_num_indices`.
- `taichi/python/export_lang.cpp:559,1222` — Python bindings (out of scope per
  PROJECT-PLAN 1.2, recorded only).
- `taichi/codegen/llvm/struct_llvm.cpp:171` — refine-coordinates loop bound.
- `taichi/runtime/llvm/runtime_module/runtime.cpp:289,1012`.

## 24. `data_type_format` already knows about a Vulkan 64-bit quirk

`taichi/ir/type_utils.cpp:113-154`. Note `:130-137`:
```cpp
} else if (dt->is_primitive(PrimitiveTypeID::i64)) {
  // Vulkan does not support printing 64-bit signed integer
  return "%lld";
} else if (dt->is_primitive(PrimitiveTypeID::u64)) {
  // Vulkan requires %lu to print 64-bit unsigned integer
  return arch == Arch::vulkan ? "%lu" : "%llu";
}
```
This is the only place in my territory where `Arch` reaches the type layer at
all (`taichi/ir/type_utils.h:5` includes `taichi/rhi/arch.h`, `:17`
`data_type_format(DataType dt, Arch arch = Arch::x64)`). It is a printf-format
concern, not a width concern, but it is a standing in-tree acknowledgement that
64-bit integer support is target-dependent on the Vulkan/SPIR-V path — which is
exactly the caveat PROJECT-PLAN 6.2 raises. Recording the correspondence; the
capability question itself is agent 02's.

## 25. Passes checked and found width-neutral (negative results)

Recording these so the adversaries know they were looked at, not skipped.
- `taichi/transforms/lower_access.cpp` — threads `LinearizeStmt` through
  unchanged (entry 10).
- `taichi/transforms/alg_simp.cpp:118,132` — uses `val_as_int64()`, width-agnostic.
- `taichi/transforms/binary_op_simplify.cpp:66` — `int64` mask, width-agnostic.
- `taichi/transforms/bit_loop_vectorize.cpp:74` — uses
  `data_type_bits(load_data_type)`, i.e. **derived**, not hardcoded. Its `:180,202,311-312`
  `int32 get_constant_value` is for quant shift amounts, bounded by the physical
  type's bit count.
- `taichi/analysis/verify.cpp` — no width literals at all (grep returned nothing).
- `taichi/analysis/gen_offline_cache_key.cpp:540` — `static_cast<std::size_t>(snode->id)`,
  identity only.
- `taichi/analysis/same_statements.cpp:140-141`, `taichi/analysis/alias_analysis.cpp:154-163`,
  `taichi/ir/ir.cpp:81-89` — SNode `id` used purely as an identity/equality key.
  **No bit-packing of SNode ids anywhere in my territory.** This is why raising
  `taichi_max_num_snodes` has no consequence in `taichi/ir`, `taichi/analysis`
  or `taichi/transforms`. VERIFIED by grep + reading each site.
- `taichi/ir/frontend_ir.h:1174` `TI_ASSERT((v % 32 == 0) || bit::is_power_of_two(v));`
  — block dim warp alignment, not addressing.
- `taichi/transforms/make_mesh_block_local.cpp:440,444` `available_bytes -= 4;`
  — reserving 4 bytes of shared memory, mesh path.

---

## 26. Final check: the cast is a VALUE cast, and one ret_type/val.dt divergence

`taichi/transforms/type_check.cpp:234-244` `insert_type_cast_before` emits
`UnaryOpType::cast_value` (`:238`), not `cast_bits`. So the index narrowing at
`:154` and `:461` is a genuine value truncation, not a reinterpretation.
VERIFIED.

`taichi/transforms/type_check.cpp:40-44`:
```cpp
static void mark_as_if_const(Stmt *stmt, DataType t) {
  if (stmt->is<ConstStmt>()) {
    stmt->ret_type = t;
  }
}
```
Used at `:170-171` for range-for `begin`/`end`. It overwrites `ret_type` to i32
**without touching `stmt->val.dt`**. So after this pass a range-for bound
constant can have `ret_type == i32` while `val.dt == i64`. `offload.cpp:109/117`
then reads `val.val_int32()`, which asserts on `val.dt`
(`taichi/ir/type.cpp:463`), so the mismatch surfaces as an assertion rather than
silent corruption. Recording the divergence because it is a place where two
notions of a statement's type disagree, which any width change has to reckon
with. VERIFIED.

Notes end. Writing the report now.

---
---

# REVISION PASS (entries 27 onward)

Report returned for amendment. Both adversary files read in full
(`adversary-01-1.md`, 736 lines; `adversary-01-2.md`, 598 lines). Where they
disagree I went to the source. Adjudications below, then the report edits.

## 27. ADJUDICATED: `data_type_size` does NOT refuse pointers. I was wrong.

**The dispute.** Adversary 1 section 4.1 says my description is wrong: `:35`
strips the pointer flag and returns the *pointee's* size. Adversary 2 section 6
originally endorsed my version ("`type_utils.cpp:30-35` `data_type_size`
refusing pointers. B's section 4 is a precise and correct statement of the
gap"), then conceded to adversary 1 in its section 9.2 item 3.

**Adjudicated against the source. ADVERSARY 1 IS RIGHT.**

`taichi/ir/type_utils.cpp:30` — the parameter is `DataType t`, **by value**.
`taichi/ir/type_utils.cpp:35` — `t.set_is_pointer(false);`
`taichi/ir/type.cpp:55-62`:
```cpp
void DataType::set_is_pointer(bool is_ptr) {
  if (is_ptr && !ptr_->is<PointerType>()) {
    ptr_ = TypeFactory::get_instance().get_pointer_type(ptr_);
  }
  if (!is_ptr && ptr_->is<PointerType>()) {
    ptr_ = ptr_->cast<PointerType>()->get_pointee_type();
  }
}
```
`:59-61` **replaces `ptr_` with the pointee**. There is no `PointerType` branch
anywhere in `data_type_size` (`taichi/ir/type_utils.cpp:36-69`), so control
falls straight into the primitive/tensor dispatch on the *pointee*.

So `data_type_size(some_pointer_type)` returns the size of what it points at,
silently. It does not refuse, does not error, does not return a sentinel. The
TODO at `taichi/ir/type_utils.cpp:31-34` records that a "loud failure on
pointers" was intended and never written.

My conclusion (the type system cannot report a pointer's size) survives. My
description of the mechanism was wrong, and the difference is material: a caller
today gets a **wrong answer**, not an error. Report section 4 and headline 5 both
corrected.

**Second, related fact neither of my earlier passes stated:**
`taichi/ir/type_utils.h:13` and `taichi/ir/type_utils.cpp:30` —
`data_type_size` **returns `int`**. And `taichi/ir/type_utils.cpp:44-48`
computes a `TensorType`'s size as `get_num_elements() * data_type_size(element)`
in `int`. My report held this function up as the derived alternative to
`simplify.cpp`'s hardcoded `sizeof(int32)`. It is derived, and it is also
32-bit. Adversary 2 section 5.5 is right. Adding to the inventory.

## 28. ADJUDICATED: the `spirv_has_int64` capability EXISTS, inside my territory

I claimed in report section 4 that "a target that lacks Int64 has nowhere in the
current type system to say so", and in section 6.3 that "there is no mechanism
in my territory for a pass to ask what a target's ... 64-bit integer capability
is". **Both statements are refuted by a file in my own territory that I did not
open.** Adversary 1 section 5.1 found it; adversary 2 conceded the point in its
section 9.2 item 1. I verified every link myself.

`taichi/inc/rhi_constants.inc.h` — read in full (102 lines). Line 14:
```
PER_DEVICE_CAPABILITY(spirv_has_int64)
```
sitting in a block of SPIR-V capabilities at `:10-34`, alongside
`spirv_has_int8` (`:12`), `spirv_has_int16` (`:13`), `spirv_has_float64`
(`:16`) and `spirv_has_atomic_int64` (`:17`). The file header comment
(`:1-7`) states the intent plainly: *"Device capability is a shared intelligence
between the runtime environment and the code generator."*

Materialised at `taichi/rhi/device_capability.h:11-15`:
```cpp
enum class DeviceCapability : uint32_t {
#define PER_DEVICE_CAPABILITY(name) name,
#include "taichi/inc/rhi_constants.inc.h"
#undef PER_DEVICE_CAPABILITY
};
```
with the query surface at `taichi/rhi/device_capability.h:20-30`
(`DeviceCapabilityConfig::contains/get/set`, a `std::map<DeviceCapability,
uint32_t>`).

Enforced at SPIR-V codegen, all four sites read by me:
- `taichi/codegen/spirv/spirv_ir_builder.cpp:64-66` — emits
  `spv::CapabilityInt64` only if the capability is present.
- `taichi/codegen/spirv/spirv_ir_builder.cpp:166-169` — declares `t_int64_` and
  `t_uint64_` **only** if the capability is present.
- `taichi/codegen/spirv/spirv_ir_builder.cpp:311-314`:
  ```cpp
  } else if (dt->is_primitive(PrimitiveTypeID::i64)) {
    if (!caps_->get(cap::spirv_has_int64))
      TI_ERROR("Type {} not supported.", dt->to_string());
    return t_int64_;
  ```
- `taichi/codegen/spirv/spirv_ir_builder.cpp:325-328` — the same for u64.

**The corrected statement.** The capability is declared (in my territory, at
`taichi/inc/rhi_constants.inc.h:14`) and hard-enforced (in agent 02's, at
`taichi/codegen/spirv/spirv_ir_builder.cpp:312,326`). What does not exist is
any link from the **type layer** to it: nothing in `taichi/ir/type.h`,
`taichi/ir/type_factory.cpp` or any pass in `taichi/transforms/` consults
`DeviceCapability`. So widening an index to i64 today does not silently degrade
on a Vulkan target lacking Int64 — it produces a hard `TI_ERROR` at
`spirv_ir_builder.cpp:312`.

This is a materially better starting point for PROJECT-PLAN 6.2's stub note than
what I reported, and it changes escalation 2 from "the capability cannot be
expressed" to "the capability exists and is enforced; only the link from the type
layer is absent". Report sections 1, 4, 2 (6.3) and Escalation 2 all corrected.

I record how I missed it: I read `taichi/inc/constants.h`,
`data_type.inc.h`, `data_type_with_c_type.inc.h`, `type_kind.inc.h`,
`address_mode.inc.h`, `snodes.inc.h` and `statements.inc.h`, and I checked
`address_mode.inc.h` specifically *because* its name suggested addressing. I
never listed the directory's remaining `.inc.h` files against my focus. The name
`rhi_constants.inc.h` reads as backend plumbing; it is not.

## 29. ADJUDICATED: the `simplify.cpp` guard covers ONE site, not two

**The dispute.** My report section 3.5 stated as fact: "The two `simplify.cpp`
sites are guarded by `taichi/transforms/simplify.cpp:233-235`". Both adversaries
say the paired agent's literal reading is right and I overstated. Adversary 1
section 2.5 also points out I contradicted my own section 5.2, which marked the
same claim INFERRED.

**Adjudicated against the source. BOTH ADVERSARIES ARE RIGHT. I overstated.**

Verified the visitor spans by direct line read:
- `taichi/transforms/simplify.cpp:222` — `void visit(SNodeLookupStmt *stmt) override {`
- `:232-236` — the `for` loop containing the two `TI_ASSERT`s at `:233` and `:234-235`
- `:238-239` — the first `sizeof(int32)` site
- `:249` — closing `}` of the `SNodeLookupStmt` visitor
- `:251` — `void visit(GetChStmt *stmt) override {`
- `:260-261` — the second `sizeof(int32)` site
- `:273-274` — `set_done(stmt);` and closing `}`

**The assert is inside `visit(SNodeLookupStmt *)` only. `visit(GetChStmt *)`
contains no assert.** There is a plausible transitive argument (the
`IntegerOffsetStmt` that gates the `GetChStmt` fold at `:255` is normally the
one produced by the guarded fold at `:238-244` on the same SNode) but that is
an argument, not a guard, and I cannot rule out chained `GetChStmt` folds.
Neither adversary proved it either way and neither could I. Report section 3.5
demoted to match my own section 5.2 hedge, and the `:260-261` site is now marked
unguarded.

Adversary 1 declines to score whether the assert spans `:233-235` or `:233-237`.
For the record: `:232` opens the loop, `:233` is one `TI_ASSERT`, `:234-235` is a
second, `:236` closes the loop. My `:233-235` covers both asserts; the paired
agent's `:233-237` is a range over the same guard plus the loop close. Neither
is wrong; I keep mine and note the loop bounds explicitly.

## 30. ADJUDICATED: "every `GlobalPtrStmt` index" is wrong — there is an exception

Adversary 1 section 5.5 and adversary 2 section 9.2 item 2 both flag this and
neither report stated it. Verified:

`taichi/transforms/type_check.cpp:122-125`:
```cpp
void visit(GlobalPtrStmt *stmt) override {
  if (stmt->is_bit_vectorized) {
    return;
  }
```
The early return precedes the cast loop at `:147-156`. **A bit-vectorized global
pointer's indices are never cast to i32.** My headline 2 said "every
`GlobalPtrStmt` index". Corrected.

(I did read `:122-125` — it is quoted in my own notes entry 7's surrounding
context — and did not draw the consequence. Recording that as my error, not a
missing read.)

## 31. ADJUDICATED: my "single enforcement point" headline is too broad

Both adversaries reject it, in the same terms, and adversary 1 section 6
enumerates the alternative. I accept the correction and verified the
enumeration myself:

- **Cast insertion on a pointer statement's index — 2 sites**, both in
  `taichi/transforms/type_check.cpp` (`:154` SNode with a warning, `:461`
  external array silently), via the helper at `:234-244`, with the
  `is_bit_vectorized` exception at `:123-125`. This narrow claim survives.
- **Cast insertion on an index elsewhere — 8 more sites**, all in the mesh
  passes: `demote_mesh_statements.cpp:15-16,52-54,130-131`;
  `make_mesh_thread_local.cpp:104-106,112-114`;
  `make_mesh_block_local.cpp:210-212,374-376,397-399`.
- **Birth sites** where an index is created i32 and never passes a cast at all:
  `frontend_ir.cpp:151-152`, `lower_ast.cpp:330`, `type_check.cpp:467`.
- **Re-synthesis after `type_check` has already run**:
  `make_cpu_multithreaded_range_for.cpp:68-93`.
- **Hard aborts** rather than casts: `type_check.cpp:160-161`,
  `snode.h:28-30`, `demote_dense_struct_fors.cpp:29`, plus the thirteen
  `val_int32()` call sites.

I also verified adversary 1's and adversary 2's shared point about pass
ordering, which I had not investigated at all:
`grep -c "irpass::type_check" taichi/transforms/compile_to_offloads.cpp` = **7**
(`:57, 66, 193, 200, 208, 310, 391`), plus in-pass re-runs at
`taichi/transforms/lower_access.cpp:294`,
`taichi/transforms/handle_external_ptr_boundary.cpp:100` and
`taichi/transforms/simplify.cpp:219`. The i32 coercion is re-applied after
almost every transform. It is not a one-shot, and changing the pass once would
not let a wider type flow through.

## 32. ADJUDICATED: `total_shape` is per-AXIS, not a cell count

Adversary 2 section 3 corrects the framing of my own section 3.4 rows for
`scalar_pointer_lowerer.cpp:37` and `demote_dense_struct_fors.cpp:24`.
Adversary 1 section 10.1 concedes the point and says its own section 6 has the
same weakness.

**Adjudicated. ADVERSARY 2 IS RIGHT.** Verified:
`taichi/transforms/scalar_pointer_lowerer.cpp:33` declares
`std::array<int, taichi_max_num_indices> total_shape;` — one slot per **axis**.
`:36-37` accumulates `total_shape[j] *= s->extractors[j].shape;` over the
root-to-leaf path, per axis. Same shape at
`taichi/transforms/demote_dense_struct_fors.cpp:19,24`.

So a structure of 2^40 cells spread over four axes of 2^10 each leaves every
`total_shape[j]` at 1024. These accumulators overflow only when a **single axis
extent** exceeds 2^31, not when the cell count does. Both are genuine 32-bit
accumulators and both belong in the inventory, but neither is the >2^31-cells
failure mode. My section 3.4 listed them without that qualification. Corrected.

The cell count is carried separately, in `int64`, at
`demote_dense_struct_fors.cpp:18,26`, and that is what `:29` guards.

## 33. NEW, verified myself: sites I missed that the adversaries named

Each opened and read before accepting.

**`taichi/transforms/lower_ast.cpp` — absent from my report entirely.**
- `:270-274` — the external-array struct-for extent:
  ```cpp
  Stmt *begin = fctx.push_back<ConstStmt>(TypedConstant(0));
  Stmt *end = fctx.push_back<ConstStmt>(TypedConstant(1));
  for (int i = 0; i < (int)shape.size(); i++) {
    end = fctx.push_back<BinaryOpStmt>(BinaryOpType::mul, end, shape[i]);
  }
  ```
  `TypedConstant(1)` resolves to the `int32` constructor
  (`taichi/ir/type.h:582`), and each `shape[i]` is an
  `ExternalTensorShapeAlongAxisStmt` pinned to i32 by
  `taichi/transforms/type_check.cpp:119`. **This is a whole-structure flat
  extent formed in 32 bits, with no guard and no assert** — unlike
  `demote_dense_struct_fors.cpp:29`, which does assert. Adversary 2 section 5.1
  is right that this is exactly what item 6.2 is about. Best of the items I
  missed.
- `:281-285` — the resulting loop index is then decomposed back into per-axis
  coordinates with `BinaryOpType::mod` against the same i32 shapes.
- `:330` — `fctx.push_back<AllocaStmt>(PrimitiveType::i32);` — a range-for
  containing a `break` is lowered to a while loop whose induction variable is an
  i32 alloca. A third loop-variable birth site.
- `:333` — `TypedConstant((int32)1)` as the increment.
- Already in my notes entry 17 but not my report: `:151,177,348,361,363`.

**`taichi/analysis/arithmetic_interpretor.cpp:98-109` — a SECOND
`LinearizeStmt` implementation.** Verified:
```cpp
void visit(LinearizeStmt *stmt) override {
  int64_t val = 0;
  for (int i = 0; i < (int)stmt->inputs.size(); ++i) {
    auto idx_opt = context_.maybe_get(stmt->inputs[i]);
    if (!idx_opt) { failed_ = true; return; }
    val = (val * stmt->strides[i]) + idx_opt.value().val_int();
  }
  insert_to_ctx(stmt, stmt->ret_type, val);
}
```
`:99` accumulates in `int64_t`; `:106` multiplies by `stmt->strides[i]`, which is
`int` (`taichi/ir/statements.h:1280`); `:108` stores the result under
`stmt->ret_type`, which `taichi/transforms/type_check.cpp:521` pinned to i32.
Any width change has **two** implementations of `LinearizeStmt` semantics to
keep in step. This is squarely in my territory (`taichi/analysis/`) and I missed
it — my notes entry 19 swept `taichi/analysis/` with a grep for
`val_int32|val_i32|int32|numeric_limits`, and this file uses `int64_t` and
`val_int()`, so the grep could not hit it. My sweep was too narrow.

**`taichi/transforms/check_out_of_bound.cpp:123-126` — the SNode bounds
ceiling.** I had these exact lines in notes entry 10 but classified them under
"i32 constants" and listed only the *products* (`:56`, `:172`) in my report's
section 3.4. Adversary 2 section 5.2 is right that the bound actually compared
against the index is the material one:
```cpp
int size_i = snode->shape_along_axis(i);
int upper_bound_i = size_i;
auto upper_bound = new_stmts.push_back<ConstStmt>(TypedConstant(upper_bound_i));
```
`SNode::shape_along_axis` returns `int` (`taichi/ir/snode.h:317`,
`taichi/ir/snode.cpp:174-177`, returning the 32-bit
`extractor.num_elements_from_root`). I listed that signature in section 3.2
without connecting it to its consumer. Now connected.

**`taichi/transforms/lower_matrix_ptr.cpp:536-540` — a byte offset in i32.**
Verified:
```cpp
auto stride = std::make_unique<ConstStmt>(
    TypedConstant(origin->dynamic_index_stride));
auto offset = std::make_unique<BinaryOpStmt>(
    BinaryOpType::mul, stmt->offset, stride.get());
offset->ret_type = stmt->offset->ret_type;
```
`dynamic_index_stride` is `int` (`taichi/ir/statements.h:456`), and `:540`
forces the product's `ret_type` to `stmt->offset->ret_type`, which
`taichi/transforms/type_check.cpp:160-161` has already hard-asserted to be i32.
So the dynamically-indexed matrix-field **byte** offset is computed in 32 bits,
while every other byte offset in the IR is `std::size_t`. That is precisely the
inconsistency PROJECT-PLAN 6.2 warns about. Absent from my report; adding.

**`taichi/ir/type_factory.cpp:248-251` — `promoted_type` refuses pointers.**
Verified:
```cpp
static DataType to_primitive_type(DataType d) {
  if (d->is<PointerType>()) {
    TI_ERROR("promoted_type got a pointer input.");
  }
```
And the promotion rule itself, `compare_types` at
`taichi/ir/type_factory.cpp:222-246`, ranks integral types by
`data_type_bits` (`:234-237`), so `i32 op i64` does promote to i64. I never
discussed promotion at all; both adversaries note the omission. Adding both the
rule and its pointer restriction.

**`taichi/ir/frontend_ir.cpp:731-740` — the DYNAMIC tensor-index branch.**
My report cited only the constant-folded branch at `:744-745`. The dynamic
branch is equally i32: `:732` seeds with `ConstStmt(TypedConstant(0))` and `:735`
emits each shape factor as `ConstStmt(TypedConstant(shape[i]))`, both i32 by
`taichi/ir/type.h:582`. I had `:732,735` in notes entry 12 and cited only
`:742-747` in the report. Recovering.

## 34. NEW, verified myself: the existing C++ test harness

Neither report, and only adversary 2, mentions tests. Verified from scratch.

`grep -rn "ArithmeticInterpretor" --include=*.cpp --include=*.h .` — the class
has **no callers anywhere in `taichi/`**. Its only two consumers are tests:

**`tests/cpp/transforms/scalar_pointer_lowerer_test.cpp:55-100`.** Read in full.
- `:62` builds the index with `builder.get_int32(loop_index)`.
- `:72` `ASSERT_EQ(lowerer.linears.size(), 3);` — asserts one `LinearizeStmt`
  per SNode level (root, pointer, dense), i.e. it asserts the hierarchical
  structure of the lowering.
- `:77` runs `irpass::type_check(block.get(), cfg_);` with the comment
  *"Set types so that ArithmeticInterpretor can run correctly"* — the test
  depends on the i32 pinning.
- `:91-99` evaluates each level's `LinearizeStmt` and asserts the exact
  numeric result (`EXPECT_EQ(res_opt.value().val_int(), i)` and `... , j`).

**`tests/cpp/transforms/make_block_local_test.cpp:145-188`.** Read in full.
- `:171-176` seeds `LoopIndexStmt` and `BlockCornerIndexStmt` with
  `TypedConstant(PrimitiveType::i32, ...)` explicitly.
- `:187` reads the result back with `val_int32()`, which asserts `dt == i32`
  (`taichi/ir/type.cpp:462-465`). **This test breaks on a width change by
  assertion, not by a wrong value.**
- `:155-165` embeds a commented IR dump showing every statement typed `<i32>`.

Also present, unexamined in detail: `tests/cpp/ir/ir_type_promotion_test.cpp`,
`tests/cpp/ir/type_test.cpp`, `tests/cpp/ir/frontend_type_inference_test.cpp`,
`tests/cpp/transforms/scalarize_test.cpp`, `tests/cpp/transforms/simplify_test.cpp`.

This is material and I should have looked. `tests/` is not named in my
territory, but the tests for `taichi/transforms/scalar_pointer_lowerer.cpp` are
tests *of* my territory. Adding to the report and escalating whether they are a
constraint or a casualty.

## 35. ADJUDICATED: two internal inconsistencies in my own report

Both adversaries caught these. Both correct.

1. **"Only three constants from `constants.h` are used in my territory"**
   (my section 5.1) contradicts my own section 3.7, which inventories twelve
   `taichi_max_num_indices` sites. **Four** constants are used:
   `taichi_max_num_indices` (12 sites), `taichi_global_tmp_buffer_size`
   (`taichi/transforms/offload.cpp:358`), `taichi_max_gpu_block_dim`
   (`taichi/ir/frontend_ir.cpp:132`) and `default_shared_mem_size`
   (`taichi/transforms/make_mesh_block_local.cpp:434`). And now a fifth file in
   `taichi/inc/` matters: `rhi_constants.inc.h:14` (entry 28). My wording
   silently excluded the constant my whole section 3.7 is about. Corrected.

2. **`kMaxNumSnodeTreesLlvm` use count.** My section 5.1 says "one use site";
   my Escalation 1 cites `runtime.cpp:562-563`, which is two lines.
   Adversary 1 section 2.8 calls this an internal inconsistency; adversary 2
   section 9.4 item 2 says counting consecutive array declarations in one struct
   as one site is a convention, not an error, and would not hold it against me.
   **I side with adversary 1 on the wording and adversary 2 on the substance:**
   the fact is two declarations (`taichi/runtime/llvm/runtime_module/runtime.cpp:562`
   sizes `roots`, `:563` sizes `root_mem_sizes`) inside one struct. I will state
   the lines rather than a count, which removes the ambiguity entirely. Same
   treatment for `taichi_max_num_snodes`: three consecutive declarations at
   `runtime.cpp:567-569` plus the assertion at `struct_llvm.cpp:266`.

## 36. Recovered from my own notes, dropped from my report

Per the amendment instruction, sweeping notes 1-26 for anything the report does
not carry:

- **`make_cpu_multithreaded_range_for.cpp`** — notes entry 11, last bullet. My
  notes say "The parallel range-for iteration space is i32 end to end" and the
  report drops the file entirely. Both adversaries flag it as the clearest case
  of the reporting step losing the investigation's result. Re-verified:
  `:68,70,72` i32 constants; `:81,90` read `begin_value`/`end_value`;
  **`:84,93` `GlobalTemporaryStmt(offloaded->begin_offset, PrimitiveType::i32)`**
  — the dynamic bound, which `taichi/transforms/offload.cpp:508,522` routed
  through a `std::size_t` offset, is read back out of the buffer as i32;
  `:138-141` sets its own bounds. Live on every CPU build:
  `taichi/transforms/compile_to_offloads.cpp:198-199`,
  `if (config.make_cpu_multithreading_loop && arch_is_cpu(config.arch))`.
  CPU-only is a first-class target under PROJECT-PLAN 2. Recovering.
- **The mesh cast sites** — notes entry 11. Report filed them under "hardcoded
  widths", which miscategorises them. `taichi/transforms/demote_mesh_statements.cpp:15-18`
  emits a `cast_value` to i32 and feeds the result directly as the sole index of
  a `GlobalPtrStmt` at `:18`. That is a live truncation on an index. Moving to
  section 3.3 and raising mesh scope as an escalation.
- **`taichi/ir/scratch_pad.h`** — notes entry 16, a full inventory
  (`:34-38,47,51,53,75-76,97-98,107-108,131-147,149-157,182,229-234`). Report
  omits it entirely. Recovering, with my assessment that it is bounded by shared
  memory kept as INFERRED.
- **`taichi/analysis/bls_analyzer.cpp:27`** — notes entry 13
  (`snode->extractors[j].shape - 1` as a bound). Report omits.
- **`taichi/transforms/demote_no_access_mesh_fors.cpp:31-34`** — notes entry 14,
  a fourth writer of `begin_value`/`end_value`. Report omits.
- **Mesh statement classes and expressions** — notes entries 11 and 12:
  `taichi/ir/statements.h:2064,2083`;
  `taichi/ir/frontend_ir.cpp:1358,1362,1392`. Report omits.
- **`taichi/ir/analysis.h:15-53` `DiffRange`** — notes entry 15. Report has
  `:20-21` in section 3.2 but not the class span or `DiffPtrResult`. Adding
  `taichi/ir/analysis.h:185` (`DiffPtrResult::diff_range`), which adversary 1
  section 5.8 credits to the paired agent and which I verified.
- **`taichi/transforms/lower_ast.cpp:151,177,348,361,363`** — notes entry 17.
  Report has only `:177,363`.
- **The `value_diff` consumer set.** My report named two consumers. Verified the
  complete set by `grep -rn "value_diff_ptr_index\|value_diff_loop_index" taichi/`:
  six — `taichi/analysis/alias_analysis.cpp:59,142,178`,
  `taichi/analysis/bls_analyzer.cpp:50`,
  `taichi/transforms/lower_access.cpp:237`,
  `taichi/transforms/bit_loop_vectorize.cpp:54`. Both adversaries say my two were
  incomplete; adversary 1's set of six matches mine exactly. Corrected.
- **The `val_int32()` count.** Both adversaries confirm my list is the complete
  set. Adversary 1 says "twelve", adversary 2 counts thirteen and says
  adversary 1 miscounted its own quotation of my list. **Adversary 2 is right:**
  `offload.cpp:109,117` (2) + `scalarize.cpp:162,1097,1120,1147,1231,1311` (6) +
  `auto_diff.cpp:576,776,1770` (3) + `control_flow_graph.cpp:462` (1) +
  `constant_fold.cpp:115` (1) = **13**. My report gave the list without a count,
  so no correction is needed to the list; I will state the count as 13 to
  foreclose the ambiguity.

## 37. Where I do NOT accept an adversary point

**Adversary 2 section 6, Pass B defect 3**, says my headline 3 ("the frontend
does not forbid i64 indices") is "true for an explicit `IndexExpression` and
false for a loop variable, which B's own touch point 10 pins to i32 in the
frontend. B states both and reconciles neither."

I accept that I did not reconcile them, and I will. But the two statements are
not in conflict, and the reconciliation is worth stating precisely rather than
retracting either half. Verified:
- `taichi/ir/frontend_ir.cpp:936-947` — an index *expression* is checked only
  for `is_integral`. A user-supplied i64 index passes the frontend.
- `taichi/ir/frontend_ir.cpp:149-153` — a *loop variable* is assigned
  `get_pointer_type(PrimitiveType::i32)` unconditionally.

So: the frontend does not restrict the **type of an index expression**, and it
does fix the **type of a loop variable**. Since a struct-for driven workload
derives essentially every index from a loop variable, the practical effect is
that the frontend does pin the width for the case that matters. That is the
reconciliation, and adversary 2's section 1.1 makes the same operational point
("In a struct-for driven engine every index is loop-derived"). Headline 3
rewritten to say both halves and their consequence, not withdrawn.

**Adversary 1 section 2.9** calls my `taichi_max_num_snodes` phrasing "loose"
for saying three sites where the grep gives four lines. Adversary 2 section 9.4
item 2 says the range notation is a convention and PROJECT-PLAN 6.1 itself uses
it. I agree with adversary 2 that this is not an error, and with adversary 1
that it is ambiguous. Resolved by giving lines instead of counts (entry 35).

## 38. Report edits applied

Revising `report-01b-ir-types.md` in place. Nothing removed from the inventory;
everything below is a correction or an addition.

1. Headline 2 — narrowed to "pointer statement indices", `is_bit_vectorized`
   exception stated, mesh/CPU/birth sites named, pass re-run count added.
2. Headline 3 — reconciled per entry 37.
3. Headline 5 — `data_type_size` mechanism corrected per entry 27.
4. New headline — `spirv_has_int64` per entry 28.
5. New headline — the C++ tests per entry 34.
6. Section 2, table of 6.2 touch points — extended with the recovered and new
   sites.
7. Section 2, item 6.3 — rewritten per entry 28.
8. Section 3.1 — `is_bit_vectorized` exception, mesh `ret_type` pins.
9. Section 3.3 — mesh casts moved in from 3.5; `lower_matrix_ptr.cpp:536-540`
   added.
10. Section 3.4 — per-axis qualification per entry 32; `lower_ast.cpp:270-274`
    added as the unguarded whole-structure extent.
11. Section 3.5 — simplify guard demoted per entry 29; `data_type_size` returns
    `int` noted.
12. Section 3.6 — count stated as 13.
13. New section 3.9 — scratch-pad, BLS and mesh inventories recovered from notes.
14. New section 3.10 — the C++ test harness.
15. Section 4 — rewritten per entries 27 and 28; promotion rule added.
16. Section 5.1/5.2 — the two internal inconsistencies fixed per entry 35;
    new VERIFIED items; `total_shape` correction moved from VERIFIED framing.
17. Escalations — 2 rewritten, and new entries for mesh scope, the CPU path,
    the tests, and what "the address" means.

---

# AMENDMENT PASS (entries 39-42)

Round two returned three claims. I opened each against the source before
changing anything. Both round-two adversaries agreed this report was one tightly
scoped pass from consensus; these four entries are that pass.

## 39. ADJUDICATED: `constant_fold.cpp:115` is NOT an abort site. I was wrong,
## and I was wrong in a specific way I want on the record.

**The claim.** Both round-two adversaries say my section 3.6 wrongly lists
`taichi/transforms/constant_fold.cpp:115` among the `val_int32()` sites that
fail loudly, that it is guarded at `:113` on `lhs->val.dt` with an i64 branch at
`:116-118`, and that my derived counts of thirteen and sixteen should be twelve
and fifteen. Adversary 2 adds that I "took a round-one adversary's count on
trust rather than opening the site."

**The claim is correct, including that last part. Opened:**

```
64:    // Type check should have been done at this point.
65:    auto dt = lhs->val.dt;
...
111: #define HANDLE_INTEGRAL_BINARY(OP_TYPE, PREFIX, OP_CPP)
113:     if (dt->is_primitive(PrimitiveTypeID::i32)) {
115:         dst_type, PREFIX(lhs->val.val_int32() OP_CPP rhs->val.val_int32()));
116:     } else if (dt->is_primitive(PrimitiveTypeID::i64)) {
118:         dst_type, PREFIX(lhs->val.val_int() OP_CPP rhs->val.val_int()));
119:     } else if (dt->is_primitive(PrimitiveTypeID::u32) ||
120:                dt->is_primitive(PrimitiveTypeID::u64)) {
```

`dt` is `lhs->val.dt` — the `TypedConstant::dt` field (`taichi/ir/type.h:551`),
which is precisely the field `TypedConstant::val_int32()` asserts against
(`taichi/ir/type.cpp:462-465`). It is not `Stmt::ret_type`, so the
`mark_as_if_const` divergence I recorded in entry 26 cannot reach it. An i64
constant takes the `:116` arm. This is correct 64-bit dispatch.

One line-number correction to adversary 1, which said `:64` is
`auto dt = lhs->val.dt;`. `:64` is the comment; `:65` is the assignment.
Adversary 2 has `:65` and is right.

**I re-read the other twelve rather than accept "twelve" on trust in turn.**
None of them guards on the type:

- `taichi/transforms/offload.cpp:107,115` — `if (auto val = s->begin->cast<ConstStmt>())`
  and the same for `end`. Statement class only.
- `taichi/ir/control_flow_graph.cpp:456-457` — `TI_ASSERT(load_src->is<MatrixPtrStmt>() && ...offset->is<ConstStmt>());`
  Statement class only; the read is at `:460-462`.
- `taichi/transforms/scalarize.cpp:160` guards `:162` on
  `is<ConstStmt>()`; `:1096`, `:1117`, `:1144`, `:1230`, `:1307` likewise.
- `taichi/transforms/auto_diff.cpp:576`, `:776`, `:1770` likewise.

So twelve abort, plus the three structural aborts already in the report
(`type_check.cpp:160-161`, `snode.h:28-30`,
`demote_dense_struct_fors.cpp:29`) = **fifteen**, not sixteen.

**I also checked the "ten of the twelve are downstream of one assert" claim
rather than repeat it.** Located the enclosing visitor for each site with
`awk '/void visit\(/ && NR<=L {v=$0} END{print v}'`:
`scalarize.cpp:1097,1120,1147,1231,1311` are all inside
`void visit(MatrixPtrStmt *stmt)`; `scalarize.cpp:162` reads
`stmt->src->as<MatrixPtrStmt>()->offset`; `auto_diff.cpp:1770` is inside
`void visit(MatrixPtrStmt *stmt)` and `:576`, `:776` read a `matrix_ptr_stmt`
local; `control_flow_graph.cpp:460-462` reads
`load_src->as<MatrixPtrStmt>()->offset`. That is ten. The two exceptions are the
`offload.cpp` pair, which read a range-for bound. Confirmed, not adopted.

**One nuance I am adding that neither adversary put in its recommendation.**
Line `:115` also calls `rhs->val.val_int32()`, and the guard at `:113` tests
only `lhs->val.dt`. A mixed-width operand pair would abort there. The in-tree
comment at `:64` asserts the invariant that type checking has unified the
operands; it does not prove it. Adversary 1 spotted this and said it deserved a
clause; adversary 2 says it saw it and did not write it down. It is in the
report now, framed as a consequence of a width change to weigh, not as a defect
today.

**How I got it wrong, plainly.** Entry 36 shows it. Round-one adversary 1 said
twelve and round-one adversary 2 said thirteen and said adversary 1 had
miscounted its own quotation of my list. I adjudicated the *arithmetic* — I
added up 2 + 6 + 3 + 1 + 1 and got 13 — and wrote "Adversary 2 is right". I
never opened `constant_fold.cpp`. My own report's section 3.6 asserts that every
site on the list "fails loudly", which is a claim about behaviour, and I settled
it by counting entries in a list. Adversary 1's twelve was right for a reason
neither of us had, and I overruled it with a correct sum against a wrong
question. That is the mistake, and it is the same mistake — accepting a number
in place of a read — that entry 35 caught elsewhere in my own file.

**Report edits.** Section 3.6 rewritten as a table separating the twelve from
the one. Headline 2: "sixteen hard aborts" → "fifteen". Section 5.1: new
verified item. Section 7's historical row annotated so the superseded 13 is not
read as current.

## 40. ADJUDICATED: `snode.cpp:89-101` is per-container, and the site that
## belongs in that slot was in neither report

**The claim.** `taichi/ir/snode.cpp:89-101` accumulates one node's own axis
shapes, so it is a per-container extent, not a whole-structure one. The site
that belongs in my sub-table is `AxisExtractor::num_elements_from_root`,
returned by `shape_along_axis` at `taichi/ir/snode.cpp:174-177` and materialised
as the i32 bound at `taichi/transforms/check_out_of_bound.cpp:123-126`. My
report cites both halves and never joins them.

**Correct on every part. Verified rather than accepted:**

- `SNode::insert_children` (`:14-34`) has exactly one propagation loop, at
  `:21-23`, and it propagates **only** `num_elements_from_root`. It does not
  touch `shape` or `acc_shape`.
- Those two therefore start at the `AxisExtractor` defaults of 1
  (`taichi/ir/snode.h:45,49`).
- `:84` is `new_node.extractors[ind].shape = sizes[i];` — a plain assignment of
  this node's own axis size, not an accumulation.
- So the loop at `:89-93` multiplies one node's own axis shapes and nothing
  else. `:101` stores the result into `num_cells_per_container`, documented at
  `taichi/ir/snode.h:93-97` as "Product of the |shape| of all the activated axes
  identified by |extractors|", with a pointer to the cell/container terminology.

That is a per-level container extent. Calling it a whole-structure flattening
contradicted my own headline about hierarchical addressing and my own escalation
7, which says exactly that this quantity is bounded by the level.

**The join.** `SNode::shape_along_axis` is at `:174-177` and its body is
`return extractor.num_elements_from_root;` at `:176`.
`taichi/transforms/check_out_of_bound.cpp:123` is
`int size_i = snode->shape_along_axis(i);` and `:126` is
`new_stmts.push_back<ConstStmt>(TypedConstant(upper_bound_i))`. My report cited
`snode.h:317` in section 3.2 as "returns `int`, consumed at
`check_out_of_bound.cpp:123`" and cited `check_out_of_bound.cpp:123-126` in the
touch-point table as "an i32 constant from `shape_along_axis`" and never said
what that value **is**. It is the unguarded root-to-leaf `int` product. That is
the finding, and I did not have it.

**Scope discipline on the replacement.** I am not adopting the adversaries'
phrasing that `num_elements_from_root` is "the root-to-leaf extent" belonging in
a slot headed whole-structure. It is per-axis and root-to-leaf, which is a third
scope, distinct from both per-container and whole-structure. Substituting it
into a table headed "whole-structure" would repeat the category error in the
opposite direction. So the sub-table is rewritten by quantity, with four rows
and an explicit scope column, and the heading no longer claims whole-structure
for all of them.

**Completeness of the field, checked myself.** `grep -rn "num_elements_from_root"
taichi/` returns exactly six lines: `taichi/ir/snode.h:41` (declaration),
`taichi/ir/snode.cpp:22-23` and `:83` (the only two writers, both mine),
`taichi/ir/snode.cpp:176` (the only in-territory reader),
`taichi/analysis/offline_cache_util.cpp:112` (cache key, already escalation 11),
and `taichi/codegen/spirv/snode_struct_compiler.cpp:121`
(`sn_desc.total_num_cells_from_root *= e.num_elements_from_root;`, the seam,
agent 02's ground). The in-tree comment at `snode_struct_compiler.cpp:117-120`
says the extractors are also set by `StructCompiler::infer_snode_properties()`;
`grep -rn "infer_snode_properties" taichi/` returns only that comment, so the
function does not exist and the comment is stale. `taichi/ir/snode.cpp` is the
sole writer. Recorded because it is what makes the two-writer claim safe.

**Report edits.** Section 3.4 sub-table rewritten from three rows to four
quantities with scope and guard columns, plus two short paragraphs giving the
per-container proof and the join. Section 3.4 main table: `snode.cpp:22-23` and
`:83` marked per-axis and root-to-leaf, which they were not. Touch-point rows 6,
7 and 18 amended. Escalation 7 given a four-quantity table; escalation 8 given
the fourth path.

## 41. ADJUDICATED: the `type_check` re-run mechanism is real; the claimed
## count is not. I verified both and used my own numbers.

**The claim.** My in-pass `type_check` re-run surface is understated by more
than half, because most re-runs call `type_check(root, config)` unqualified from
inside `namespace irpass` and are invisible to the grep I ran. Fifteen further
sites across fourteen files.

**Mechanism: correct, and verified.** `type_check` is declared at
`taichi/ir/transforms.h:67`, inside the `namespace irpass` block that opens at
`:29` and closes at `:210`. A pass that itself sits inside `namespace irpass`
calls it unqualified. `taichi/transforms/make_thread_local.cpp` is the clean
example: `namespace irpass {` at `:217`, the call at `:229`,
`}  // namespace irpass` at `:232`. My entry 36 recorded
`grep -c "irpass::type_check" taichi/transforms/compile_to_offloads.cpp` = 7 —
a grep scoped to one file for a qualified spelling. It could not have found any
of these.

**Count: not correct, and I am not adopting it.** I re-derived the set with
`grep -rn "type_check(" taichi/transforms/ taichi/ir/ taichi/analysis/` and read
each hit. Outside `compile_to_offloads.cpp`, in territory:

- Unqualified `type_check(root, config)` — **14 lines, 9 files**:
  `auto_diff.cpp:2407,2410,2417,2420,2430`; `demote_atomics.cpp:241`;
  `demote_mesh_statements.cpp:160`; `lower_access.cpp:294`;
  `make_block_local.cpp:391`; `make_mesh_block_local.cpp:681`;
  `make_mesh_thread_local.cpp:165`; `make_thread_local.cpp:229`;
  `offload.cpp:766,773`.
- Qualified `irpass::type_check(node, config)` — **5 lines, 4 files**:
  `check_out_of_bound.cpp:248`; `demote_operations.cpp:300,303`;
  `handle_external_ptr_boundary.cpp:100`; `ir.cpp:566`.
- Queued through `DelayedIRModifier::type_check` (`taichi/ir/ir.h:617`,
  `taichi/ir/ir.cpp:533-534`, drained at `:565-567`) — **3 lines, 1 file**:
  `simplify.cpp:219,344,349`.

**Twenty-two call lines across fourteen files.** The claimed "fifteen further
sites across fourteen files" does not reconcile with the claimant's own
enumeration, which lists the same twenty-two lines. Fourteen is the count of
files in the *whole* set, not the remainder; and the remainder after the three
my report already named (`lower_access.cpp:294`,
`handle_external_ptr_boundary.cpp:100`, `simplify.cpp:219`) is nineteen lines
across eleven further files, not fifteen. The mechanism claim is what matters
and it stands; the figure is the claimant's arithmetic, not the source's, and I
have used the source's.

**Three qualifications I am adding, so the number is not inflated in the other
direction.** The report rests a structural conclusion on this, so it should not
overstate either way.

- The three `simplify.cpp` lines are queueing sites, not execution sites. They
  append to `to_type_check_` (`taichi/ir/ir.h:603`) and are drained by the
  single loop at `taichi/ir/ir.cpp:565-567`. Three call lines, one execution
  point.
- Conditionality, read rather than assumed: `check_out_of_bound.cpp:248`,
  `demote_operations.cpp:303` and `handle_external_ptr_boundary.cpp:100` sit
  behind `if (modified)`; `demote_operations.cpp:300` sits inside the
  fixed-point `while (true)` opened at `:294`, so it runs per rewrite round, not
  once. One adversary listed `:300` and `:303` together as both behind
  `if (modified)`; `:300` is not, and I checked.
- `auto_diff.cpp:2407,2410` sit inside the loop at `:2403` and `:2420` inside
  the loop at `:2418`, so they run once per independent block. `:2417` is
  outside the loop and `:2430` is at function end.

**Report edits.** New section 3.11 with the mechanism, the full inventory in
three forms, and the three qualifications. Headline 2 rewritten from "plus
in-pass re-runs at [three sites]" to the twenty-two figure with the namespace
reason. Section 5.1: new verified item.

## 42. Report edits applied in the amendment pass

Confined to `report-01b-ir-types.md`. No source file touched, no other agent's
file touched.

1. Header — notes count 38 → 42; new "Amendment status" paragraph.
2. Headline 2 — "sixteen hard aborts" → "fifteen"; the re-run sentence replaced
   with the twenty-two-line figure and the namespace mechanism, pointing to 3.11.
3. Section 2, item 6.2 touch-point table — rows 6, 7 and 18 amended so the
   root-to-leaf field, the per-container truncation and the out-of-bound bound
   say what they actually are and are joined to each other.
4. Section 3.4 main table — `snode.cpp:22-23` and `:83` marked per-axis and
   root-to-leaf.
5. Section 3.4 sub-table — rewritten from three "whole-structure" sites to four
   quantities with scope and guard columns, plus the per-container proof and the
   `shape_along_axis` join.
6. Section 3.6 — rewritten as a guarded/unguarded table; twelve aborts, fifteen
   total; the `rhs->val.val_int32()` nuance added.
7. New section 3.11 — the `type_check` re-application surface.
8. Section 5.1 — four new VERIFIED items covering this pass.
9. Escalations 7 and 8 — sharpened with the four quantities and the fourth
   unguarded path. No new escalation opened, and none resolved.
10. Section 7 — one historical row annotated so its superseded count is not read
    as current.
11. New section 8 — what changed in this pass and why, with the verdict on each
    of the three claims.

**Left alone deliberately**, as instructed and because round two upheld them:
headline 3 and its two-object reconciliation; the `data_type_size` reading in
headline 5 and section 4; every citation, round two having found no citation
fault in this file.

**Not done, and why.** Both adversaries offered, as optional and non-blocking, a
row for the `auto_diff.cpp` i32 index constructions (`:587`, `:639`, `:646`,
`:787`, `:839`, `:846`, `:1828`, `:1857`) and a correction of the
`visit(GetChStmt *)` span in section 3.5 and escalation 10 from `:251-274` to
`:251-273`. Neither is among the three claims I was sent to adjudicate. I have
not opened either, and I am not asserting anything about them. They are the
planner's to dispatch if wanted.

---

# COMPLETENESS PASS (entries 43-46)

The planner ruled that where a site is inside my territory and I can confirm it
present, completeness is the brief and not a scope question. Five groups
re-opened on that rule. I was told explicitly not to take any of them from an
adversary's characterisation or the planner's, and not to converge on the paired
report's figures. Everything below was opened myself.

## 43. `auto_diff.cpp` index constructions — present, and the list I was handed
## was short by one

**What I was pointed at.** Eight sites: `:587`, `:639`, `:646`, `:787`, `:839`,
`:846`, `:1828`, `:1857`.

**All eight confirmed present.** Opened each with `sed -n`:

- `:587`, `:646`, `:787`, `:846` — `insert_const(PrimitiveType::i32, stmt, i, true)`,
  the helper declared at `:14`. Matrix element index constants.
- `:1857` — `insert_const_for_grad(PrimitiveType::i32, stmt, i)`. A **different
  helper**. Adversary 1 wrote the whole group as
  "`insert_const(PrimitiveType::i32, ...)`"; that is not what `:1857` says.
- `:639`, `:839`, `:1828` — `get_tensor_type(tensor_shape, PrimitiveType::i32)`,
  the index vector's element type.

**A ninth site, which is mine.** I did not stop at confirming the eight. My own
sweep, `grep -n "PrimitiveType::i32\|(int32)\|int32(" taichi/transforms/auto_diff.cpp`,
returns twelve lines. Nine are index constructions; three are the `val_int32()`
reads at `:576`, `:776`, `:1770` already in section 3.6. The ninth construction
is

```
1819:        indices_values[i] = insert<ConstStmt>(TypedConstant((int32)i));
```

i32 by the `TypedConstant(int32)` constructor at `taichi/ir/type.h:582`. It is
the same kind of site as `:587` — a matrix element index built i32 at
construction — differing only in spelling, which is presumably why a grep keyed
to `PrimitiveType::i32` missed it. It feeds `indices_matrix_init_stmt` at
`:1829`, whose `ret_type` is set at `:1830` from the i32 tensor type built at
`:1828`. Neither adversary has it.

Nine constructions plus three reads is twelve, and twelve is every i32 token in
the file. The file is closed.

**Consequence, stated honestly.** Low. These are matrix element indices, bounded
by element count, not by structure size. The reason they belong is the one
adversary 1 gave and I accept: inventorying a file's aborts and not its index
constructions is an asymmetry, and my section 3.6 already inventories that
file's three aborts.

**Report edit.** Three rows in section 3.1, grouped by construction form, with
the low-consequence qualification and the `:1819` provenance stated.

## 44. `visit(GetChStmt *)` span — my report was wrong, and by one line

Both round-two adversaries said my `:251-274` was wrong and gave `:251-273`. I
did not take that; I opened the file.

```
251:  void visit(GetChStmt *stmt) override {
...
269:      return;
270:    }
271:
272:    set_done(stmt);
273:  }
274:  (blank)
275:  void visit(WhileControlStmt *stmt) override {
```

`:270` closes the inner `if` opened at `:255`. `:272` is `set_done(stmt);`.
`:273` closes the visitor. `:274` is blank. **The visitor spans `:251-273` and my
report said `:251-274`, in section 3.5 and again in escalation 10.** Corrected in
both.

I also re-checked the companion span I gave for `visit(SNodeLookupStmt *)`,
`:222-249`: `:222` opens, `:248` is `set_done(stmt);`, `:249` closes. That one
was right.

Nothing substantive moves. The asserts at `:233` and `:234-235` are still inside
the `SNodeLookupStmt` visitor and still guard `:239` only; `:261` is still
unguarded on its face; the transitive argument through the `IntegerOffsetStmt`
at `:255` is still plausible and still unproven. It stays inferred in 5.2 and
escalated at 10.

## 45. The three further sites — all already present. Two needed sharpening,
## one needed nothing.

I checked each against my own report before checking it against the source, so
that I would not add a duplicate row.

**`taichi/transforms/type_check.cpp:170-171` — already inventoried, correct as
written.** Section 3.1 carries the row "Range-for `begin`/`end` marked i32".
Source confirms: `void visit(RangeForStmt *stmt) override {` at `:169`,
`mark_as_if_const(stmt->begin, PrimitiveType::i32);` at `:170`,
`mark_as_if_const(stmt->end, PrimitiveType::i32);` at `:171`,
`stmt->body->accept(this);` at `:172`. The `mark_as_if_const` mechanism is
already in section 5.1 and entry 26. **No change.** Both adversaries listed this
as a pass A omission, not a pass B one, and that is right.

**`taichi/transforms/make_mesh_block_local.cpp:526` — already inventoried, and I
found something at `:525` that was not.**

```
525:    // mapping_data_type_ = mapping_snode_->dt.ptr_removed();
526:    mapping_data_type_ = PrimitiveType::i32;
527:    mapping_dtype_size_ = data_type_size(mapping_data_type_);
```

The derived alternative is sitting there commented out. That is the same shape
as the `simplify.cpp` sites in section 3.5, where I list a hardcoded width beside
a derived alternative that exists. Here the derived form is not merely available,
it is the line that was replaced. Row extended with `:525`. `:527` then sizes
from the hardcoded type, which is why the mesh mapping loads at `:210-212,
374-376, 397-399` truncate as they do.

**`taichi/transforms/lower_ast.cpp:168`, `:348`, `:361` — all three already
inventoried in section 3.9, and all three were mischaracterised by my own flat
listing.** I had them in one run as "`:168,330,348,361` (i32 allocas)". Opened:

- `:330` `fctx.push_back<AllocaStmt>(PrimitiveType::i32);` — captured as
  `loop_var` at `:331` and bound to `stmt->loop_var_ids[0]` at `:332`. **An
  index.** This is touch point 14.
- `:168` and `:348` — `auto mask = std::make_unique<AllocaStmt>(PrimitiveType::i32);`
  with `new_while->mask = mask.get();` on the next line in both cases (`:169`,
  `:349`). **Loop masks, not indices.**
- `:361` — `stmt->insert_before_me(std::make_unique<AllocaStmt>(PrimitiveType::i32));`
  at `:360-361`. The result is not captured at all. The actual mask is moved in
  separately at `:365` and stored to at `:368` with the all-ones constant built
  at `:362-363`. **Anonymous, and not an index.**

Grouping an induction variable with three loop masks under one label was my
error, not an omission. A width change to indices does not imply one to masks,
and my flat list gave a reader no way to tell which was which. Section 3.9's run
replaced with a table that says what each one is. No site added, none removed.

## 46. Report edits applied in the completeness pass, and the `type_check` set
## made checkable

Confined to `report-01b-ir-types.md`. No source file touched, no other agent's
file touched.

1. Section 3.1 — three rows added for the nine `auto_diff.cpp` index
   constructions; the `make_mesh_block_local.cpp:526` row extended with the
   commented-out derived alternative at `:525`.
2. Section 3.5 and escalation 10 — `visit(GetChStmt *)` span `:251-274` →
   `:251-273`, with the closing lines named so it can be checked.
3. Section 3.9 — the flat `lower_ast.cpp` i32 run replaced by a table separating
   the induction variable at `:330` from the masks at `:168`, `:348`, `:361`.
4. Section 3.11 — an explicit definition of the counted set, below.
5. Header and new section 8.1 — the pass recorded.

**On the `type_check` count discrepancy.** The planner tells me my figure and
the paired report's do not reconcile, will not tell me theirs, and does not want
me moving toward it. Correct instruction, and I have not looked. What I have
done instead is make my own set falsifiable, because a count nobody can
reconstruct is worth nothing in an adversarial loop.

The unit is now stated in the report as **a source line that textually invokes
the IR pass `irpass::type_check(IRNode *, const CompileConfig &)`
(`taichi/ir/transforms.h:67`), or queues it**. Not executions, not passes, not
files. Included: every such line in `taichi/ir/`, `taichi/analysis/`,
`taichi/transforms/`, both spellings, except the seven in
`compile_to_offloads.cpp` which are counted separately. A line inside a loop or
behind a condition counts once.

The exclusions are listed explicitly in the report, because that is where a
differing count will turn out to live. Four distinct functions share the name
`type_check` in this tree and only one of them is the pass:

- `irpass::type_check(IRNode *, const CompileConfig &)` — the pass. Counted.
- `irpass::frontend_type_check` — a different pass. Excluded.
- `Expr` / expression member `type_check(const CompileConfig *)` —
  `taichi/ir/expr.cpp:50`, `taichi/ir/frontend_ir.cpp:334,375,377,788,1041,
  1130,1838,1847`. Frontend, not IR. Excluded.
- `TypeSpec` / `Signature::type_check(const std::vector<DataType> &)` —
  `taichi/ir/type_system.h:246,280`, `taichi/ir/type_system.cpp:140`, reached
  from `taichi/ir/frontend_ir.cpp:635`. Excluded.

Plus comments naming the pass (`taichi/ir/statements.h:383,1374`,
`taichi/ir/frontend_ir.cpp:314`, `taichi/transforms/scalarize.cpp:260,512,623`),
which a `grep -l` will hit and which are not calls.

`DelayedIRModifier::type_check` (`taichi/ir/ir.h:617`,
`taichi/ir/ir.cpp:533-534`) is the fourth function. I **include** it, in its own
row, because unlike the other three it queues the pass itself, draining through
`taichi/ir/ir.cpp:565-567`. That is a judgement about the boundary rather than a
fact, so the report now states the alternative too: excluding it gives nineteen
lines across thirteen files instead of twenty-two across fourteen. Anyone
reconciling against another count can see immediately whether that is the
difference.

I also cross-checked the file set by a second route rather than trusting one
grep. `grep -rln` for the pass across all of `taichi/` returns seventeen files;
three of those (`taichi/ir/frontend_ir.cpp`, `taichi/ir/statements.h`,
`taichi/transforms/scalarize.cpp`) match only on comments, and
`compile_to_offloads.cpp` is counted separately, leaving thirteen files with
direct calls. `taichi/transforms/simplify.cpp` does not appear in that grep at
all, because it calls the queueing form; adding it gives the fourteen. Nothing
outside `taichi/ir/`, `taichi/analysis/` and `taichi/transforms/` calls the pass,
so no part of my count depends on a territory boundary judgement.

My figures are unchanged by this pass: twenty-two lines across fourteen files
outside `compile_to_offloads.cpp`, of which fourteen are the unqualified form.
I have not adjusted toward anything.

---

## 47. CLOSING PASS: I ran the sweep myself, and it returns sixty-four

I was handed the claim that `grep -rn "PrimitiveType::i32"` over
`taichi/ir/ taichi/analysis/ taichi/transforms/` returns sixty-four lines and
that six of them are in neither report. I did not take either half on trust.

The command, exactly:

```
grep -rn "PrimitiveType::i32" taichi/ir/ taichi/analysis/ taichi/transforms/
```

Sixty-four lines. `wc -l` on the captured output gives 64;
`cut -d: -f1 | sort -u | wc -l` gives 17 files. I then checked each of the
sixty-four against my own report mechanically — for each line I searched the
report for the `:NNN` token and read every hit in context, rather than trusting
a substring match, because my report cites grouped lines
(`frontend_ir.cpp:1358,1362,1392`) and spans (`type_check.cpp:147-156`,
`make_cpu_multithreaded_range_for.cpp:68-93`) that a naive per-line grep scores
as missing when they are present.

**Result: ten uncovered, not six.** The claim understated it, and it understated
it in a direction that matters less than the one it got right. The six named:

- `taichi/ir/frontend_ir.cpp:1160`
- `taichi/transforms/scalarize.cpp:471`, `:473`
- `taichi/ir/frontend_ir.cpp:1216`, `:1233`, `:1249`

Four more that neither adversary named:

- `taichi/transforms/make_mesh_thread_local.cpp:42`
- `taichi/transforms/check_out_of_bound.cpp:220`
- `taichi/ir/type_utils.h:40`
- `taichi/ir/expr.cpp:93`

I opened all ten. Eight are real sites and go in. Two are not:
`type_utils.h:40` is the `std::is_same<T, int32>()` arm of the
`get_data_type<T>()` ladder, with the i64 arm two lines below at `:42`, and
`expr.cpp:93` is `Expr::Expr(int32 x)` with `Expr::Expr(int64 x)` at `:95-97`.
Both map a C++ type to its own Taichi type. Neither pins anything. I am not
deciding they are unnecessary — standing instruction 3 — I am recording that
they are not width assumptions, with the reason and the adjacent i64 arm, so
anyone can overturn the exclusion in one look. They are in the section 3.12
table either way, in the excluded column rather than dropped from the page.

The per-file accounting sums to 64. I wrote it into 3.12 as a table so the next
person does not have to re-run the sweep to check my arithmetic against my own
list.

**What the sweep does not cover, which I have now said in the report.** It
matches one spelling. `TypedConstant(int32)` sites are invisible to it: that is
how `auto_diff.cpp:1819`, `lower_matrix_ptr.cpp:189,232,424,477` and
`lower_ast.cpp:151,177,333,363` are written, and all nine were already in my
report from the wider per-file pattern
`"PrimitiveType::i32\|(int32)\|int32("`. I would have preferred not to state a
"complete inventory" claim that a single grep could falsify, and the honest fix
is to state exactly what the sweep does prove and what it does not, rather than
to keep the broader word. Section 3.12 does that.

## 48. `frontend_ir.cpp:1160` is the material one, and the adversary got its
## downstream half wrong

The site is real and it is the class I said matters. Read at source:

```
1159:  } else if (op_type == SNodeOpType::append) {
1160:    auto alloca = ctx->push_back<AllocaStmt>(PrimitiveType::i32, dbg_info);
1161:    auto addr = ctx->push_back<SNodeOpStmt>(SNodeOpType::allocate, snode, ptr,
1162:                                            alloca, dbg_info);
...
1169:    ctx->push_back<LocalLoadStmt>(alloca, dbg_info);
```

`AllocaStmt(DataType)` at `taichi/ir/statements.h:21-29` sets `ret_type` to a
pointer to the type given, so this is a pointer-to-i32 local, the same shape as
the loop variable at `frontend_ir.cpp:151-152`. I traced it out of territory to
fix the width rather than assume it: `codegen_llvm.cpp:1367-1374` passes
`llvm_val[stmt->val]` into the `"allocate"` runtime call, and the runtime
signature is `Ptr Dynamic_allocate(Ptr meta_, Ptr node_, i32 *len)` at
`node_dynamic.h:61`, with `:66` `*len = i;`. Frontend i32, device i32, no cast
anywhere between them.

**Where `adversary3-01-2.md` is wrong.** Its section 8.1 says "Both reports
already inventory `type_check.cpp:114`, the `SNodeOpStmt` default i32 pin that
is this site's downstream half". It is not. `visit(SNodeOpStmt *)` runs
`taichi/transforms/type_check.cpp:105-116` and branches on `op_type`:

```
107:      stmt->ret_type = PrimitiveType::u64;            // get_addr
109-110:  stmt->ret_type = PrimitiveType::gen; set_is_pointer(true);   // allocate
112:      stmt->ret_type = PrimitiveType::u1;             // is_active
114:      stmt->ret_type = PrimitiveType::i32;            // everything else
```

`allocate` takes `:109-110`. The i32 default at `:114` is reached by `length`
and the remaining ops. The append index's width is fixed by the alloca at
`:1160` and by the runtime signature and by nothing in `type_check.cpp`, and
`codegen_llvm.cpp:1371-1372` asserts the `gen` pointer, which confirms it from
the other side.

I have changed nothing on account of that error. The site goes in exactly as the
adversary said it should; only the mechanism sentence is mine rather than
adopted. This is the second time in this territory that adopting an adversary's
characterisation rather than re-deriving it would have put a wrong sentence in
the report, and both times the correction came from opening the file.

Filed as touch point 15, which pushed rows 15-25 to 16-26. I checked every
in-report cross-reference to a row number afterwards: `lower_ast.cpp:330` is
still touch point 14 and section 3.9 still says so; row 19's text still refers
to "item 6" meaning row 6, unchanged; the section 8 change log's "touch-point
rows 6, 7 and 18" is a record of what the amendment pass did and I annotated it
rather than rewriting it.

## 49. The arithmetic, derived from the enumerations rather than checked by eye

I was told this report carries one harmless arithmetic defect and to find it by
deriving every stated total from its own enumeration. I did that for every total
I could find, mechanically where a grep would settle it. Two did not reconcile,
not one.

**The one that is a genuine tally failure.** Section 5.1 said "Four distinct
functions in this tree are named `type_check`". Enumerated from source by
`grep -rnE "^[^/]*\b[A-Za-z_:<>*& ]+ type_check\("`:

| Declaration | Site |
|---|---|
| `irpass::type_check` | `taichi/ir/transforms.h:67` |
| `Expr::type_check` | `taichi/ir/expr.h:110` |
| `Expression::type_check`, virtual | `taichi/ir/expression.h:48`, with 26 `override`s in `taichi/ir/frontend_ir.h` |
| `Signature::type_check` | `taichi/ir/type_system.h:246` |
| `Operation::type_check` | `taichi/ir/type_system.h:280` |
| `DelayedIRModifier::type_check` | `taichi/ir/ir.h:617` |

Six. The 26 override count is `grep -c "void type_check(const CompileConfig *config) override" taichi/ir/frontend_ir.h`, and no other header carries one.

Where "four" came from: my own exclusion list in 3.11 groups `Expr` with
`Expression` and `Signature` with `Operation`, so it has four *families*. I took
the tally off the shape of my list instead of off the thing the list is about.
That is exactly the failure plan section 10 item 7 names, and it is mine.
Harmless — nothing depends on it, the exclusion list itself names all six
positions and is complete, and both round-three adversaries checked the
exclusions independently and found them exhaustive. Corrected to six with all
six named and the origin of the old number stated, so the correction is not
mistaken later for a change of substance.

**The second one, a unit slip, found by the same method.** Section 5.1 said
`grep -rn "num_elements_from_root" taichi/` "returns exactly six lines". It
returns **seven**. Six sites; the writer at `snode.cpp:22-23` spans two source
lines. `adversary3-01-1.md` calls this "loose, not wrong", and as a statement
about sites it is right — but it is written as a claim about what a command
prints, and that claim is false when you run the command. Corrected to "seven
lines covering six sites". Nothing else in that bullet changes, and the six
sites are the same six.

**A third, in section 3.9, same shape.** "All six locations were already in this
report" over a table whose six rows hold nine line numbers. Reworded to say six
rows and to list them, so the unit is on the page.

**Everything else reconciles.** I re-derived, and where a grep could check it I
checked it: fifteen hard aborts = twelve plus three; thirteen `val_int32()` call
lines in territory out of fourteen tree-wide, twelve aborting; ten of the twelve
on a `MatrixPtrStmt` offset = 5 + 1 + 1 + 2 + 1; eight mesh `cast_value` sites
(`grep -c "cast_type = PrimitiveType::i32"` gives 8); twelve
`taichi_max_num_indices` sites over thirteen grep lines, the `snode.h:28-30`
error message spanning two; six `value_diff` consumers; `taichi_max_num_snodes`
one declaration and four use lines; `kMaxNumSnodeTreesLlvm` one and two; nine
`auto_diff.cpp` index constructions inside a twelve-line file sweep; four
constants from `taichi/inc/constants.h`; eight of the twenty-two `type_check`
lines in mesh or autodiff files (5 + 1 + 1 + 1); nineteen lines across thirteen
files as the stated alternative (22 − 3, 14 − 1).

Section 3.11 I re-derived from scratch rather than re-reading: the twenty-two
lines written to a file, `wc -l` gives 22, `cut -d: -f1 | sort -u | wc -l` gives
14, and each line printed from source with `sed -n "${l}p"` to confirm it is a
call and not a declaration or a comment. Fourteen unqualified `type_check(root,
config);`, five `irpass::type_check(...)`, three `modifier.type_check(...)`. It
is unchanged, and I have not touched a word of it.

## 50. Report edits applied in the closing pass, and what I did not touch

Applied to `report-01b-ir-types.md` only:

1. Header: notes count 46 to 50; new **Closing pass** paragraph naming the gap,
   the sweep and section 3.12.
2. Headline 2: three birth sites to four, adding `frontend_ir.cpp:1160`.
3. Headline 3: qualification added. The claim that the frontend does not
   restrict an index expression's type is true of field and array indices and
   false of texture coordinates, which `frontend_ir.cpp:1216,1233,1249` reject
   outright unless they are exactly i32. The claim's two halves are otherwise
   untouched; this is a third half, not a retraction.
4. Section 2, the 6.2 touch-point table: new row 15 for `frontend_ir.cpp:1160`,
   rows 15-25 renumbered 16-26.
5. Section 3.1: four rows added — `frontend_ir.cpp:1160`;
   `frontend_ir.cpp:1216,1233,1249`; `scalarize.cpp:471,473`;
   `make_mesh_thread_local.cpp:42`.
6. Section 3.9: "all six locations" to "all six rows, nine lines between them",
   with the rows listed.
7. New section 3.12: the sweep, the per-file accounting summing to sixty-four,
   the eight additions, the two exclusions with reasons, the full reading of
   `frontend_ir.cpp:1160`, the correction to the adversary's downstream-half
   attribution, and an explicit statement of what the sweep does and does not
   prove.
8. Section 5.1: "four distinct functions" to six with all six named; "six lines"
   to "seven lines covering six sites"; six new VERIFIED bullets for this pass,
   including the re-verification of `lower_matrix_ptr.cpp:189,232,424,477`.
9. Escalations 16 and 17 added: whether the texture-op group is in scope, and
   whether the append index is in scope for item 6.2. Both are scope questions I
   am forbidden to settle, and both sites are inventoried regardless.
10. New section 8.2 recording all of the above against the claims that prompted
    it, including the one claim that was wrong and the one that was already
    satisfied.

**Not touched, deliberately.** Section 3.11 in any part. Section 3.6 and its
twelve-and-three arithmetic. The `lower_ast.cpp` alloca table's three
characterisations. `auto_diff.cpp:1819`. `constant_fold.cpp:115` and its
mixed-width residual. Every citation in the seventy-line sample. The escalation
register 1-15. No source file was edited. No other agent's file was read for
material or written to.

**Unresolved and left in Escalations rather than decided.** Whether the
texture-op checks belong in territory 01 at all given PROJECT-PLAN 1.3, and
whether item 6.2's width change reaches `ti.append`'s return value. Both are the
planner's.

---

## 51. RULING APPLIED: texture-op index checks are out of scope

The planner ruled Escalation 16 in the same pass that raised it. Recorded here
with the reasoning, because a ruling I only apply and do not record is a gap the
next reader will re-open.

**The ruling.** PROJECT-PLAN 1.3 puts rendering entirely out of scope, and the
project owner has since confirmed positively — not by inference from 1.3, but
directly — that there is no graphic payload in the foreseen data: particle
attachments are strictly field interaction data, and the modelling is purely
mathematical. Texture operations are therefore not work this project will do.

**The treatment I was told to give them, and did.** Exactly what I gave
`type_utils.h:40` and `expr.cpp:93`: on the page, with the mechanism and the
reason, not dropped and not counted as work. So
`taichi/ir/frontend_ir.cpp:1216`, `:1233`, `:1249` came out of the section 3.1
inventory and went into section 3.12's recorded-and-excluded table, with the
ruling written out beneath it. Anyone who runs the sweep will hit all three,
find them on the page, and see why they are out.

**The distinction the table now draws, which matters.** The two type-ladder arms
are excluded because they are **not width assumptions**: each maps a C++ type to
its own Taichi type with the i64 arm two lines away. The three texture sites are
excluded on **scope**: they *are* width assumptions — hard rejections of any
index argument that is not exactly i32, the only outright refusal of a wider
index in the frontend — and they are out because the work does not touch them.
Filing both kinds under one word would be wrong, so the column is headed "why it
is not inventoried as a site item 6.2 must change" and each row says which
ground it stands on.

**What I did not weaken.** Headline 3's qualification stays. The claim that the
frontend does not restrict an index expression's type is false as a flat
statement, and it is false because of these three lines; deleting the
qualification along with the inventory rows would put a wrong sentence back in
the report to tidy the bookkeeping. The headline now carries the fact and says
the sites are not work, which is both true things at once.

**Counts moved with it.** Section 3.12's split goes from eight added and two
excluded to five added and five excluded. 5 + 5 = 10 uncovered, 54 + 10 = 64.
I re-derived that rather than adjusting it by subtraction.

## 52. RULING APPLIED: the append index is a condition, not an answer

The planner returned Escalation 17 with the shape of the answer rather than the
answer: it cannot be settled, by me or by the planner, because it depends on a
sizing rule the owner has not set, and PROJECT-PLAN 8.1 item 2 records that
replacement for the 1024 ceiling as undecided. So state the condition
precisely, name what bounds it today and what would have to move with it, and
stop.

**The condition, as I have written it.** Item 6.2's width change reaches the
append index **if and only if a single dynamic SNode can hold more than 2^31
elements**. Nothing else about 6.2 decides it — not a raised
`taichi_max_num_snodes`, not more SNodes, not a wider flattened address across
the whole structure. Only one node's own element count.

**What bounds it today, read at source rather than asserted.**
`taichi/runtime/llvm/runtime_module/node_dynamic.h`:

```
  3: struct DynamicNode {
  4:   i32 lock;
  5:   i32 n;
  6:   Ptr ptr;
  7: };
 11:   int chunk_size;                                  // DynamicMeta
 16: void Dynamic_activate(Ptr meta_, Ptr node_, int i) {
 21:   atomic_max_i32(&node->n, i + 1);
 61: Ptr Dynamic_allocate(Ptr meta_, Ptr node_, i32 *len) {
 65:   auto i = atomic_add_i32(&node->n, 1);
 66:   *len = i;
 81:              (i - chunk_start) * meta->element_size;
 90: u1 Dynamic_is_active(Ptr meta_, Ptr node_, int i) {
 95: Ptr Dynamic_lookup_element(Ptr meta_, Ptr node_, int i) {
116: i32 Dynamic_get_num_elements(Ptr meta_, Ptr node_) {
```

The live count is `i32 n` at `:5`. It cannot exceed 2^31 − 1 as declared, and
every operation on it is i32 or `int`: both atomics, all three `int i`
parameters, the `int chunk_start` accumulators at `:22`, `:67`, `:99`, and the
element address arithmetic at `:81` and `:105`. **The condition is false today
by construction, not by convention.** That is worth stating plainly: the site is
inert until the declaration changes, which is why it is a dependency rather than
a defect.

**Territory boundary, kept honest.** All of the above is
`taichi/runtime/llvm/`, which is agent 03's ground, and the call path through
`taichi/codegen/llvm/codegen_llvm.cpp:1367-1374` is agent 02's. I cite them to
fix the width and I flag them as theirs. What is mine is the four sites that
move with the condition, all already inventoried: the alloca at
`taichi/ir/frontend_ir.cpp:1160`; the `SNodeOpType::length` result pinned i32 at
`taichi/transforms/type_check.cpp:114`, which is what `ti.length()` returns; the
`SNodeOpExpression` default at `taichi/ir/frontend_ir.cpp:1114`; and
`element_type() = PrimitiveType::i32` in the `SNodeOpStmt` constructor at
`taichi/ir/statements.cpp:146`.

**The join I found while writing it, which I had not seen before.** A single
dynamic node above 2^31 elements is the *same quantity* as the per-container
extent in my 3.4 sub-table. `acc_shape` is truncated to `int` at
`taichi/ir/snode.cpp:92` with the warning at `:95-100`, while
`num_cells_per_container` stays `int64` at `:101`. So the host already emits a
warning at exactly the point this condition becomes true, and Escalation 8 asks
which of the four overflow behaviours should govern. The two resolve together or
not at all, and I have said so in the report rather than leaving a reader to
notice it.

**Why this is better than a guess, which is the planner's point and I agree with
it.** Answered either way, the entry would have to be re-opened when the sizing
rule lands. Written as a condition, it resolves itself: whoever sets the rule
reads one sentence and knows whether this site is in or out, and the field list
is already assembled for them.

**Where it now lives.** Escalation 17, rewritten from a question to a recorded
condition and labelled as not an escalation, with a note in the register's
preamble saying 1-15 are open, 16 is closed and 17 is a dependency. The site
itself stays inventoried as material — dynamic SNodes are sparse machinery
PROJECT-PLAN 4.1 requires — and touch point 15 is unchanged. It is the
condition that is open, not the site.
