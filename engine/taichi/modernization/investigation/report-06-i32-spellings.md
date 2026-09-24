# Report 06 — the complete set of spellings by which a 32-bit width enters the IR

Agent 06A. Territory: `taichi/ir/`, `taichi/analysis/`, `taichi/transforms/`,
`taichi/inc/`. Contemporaneous notes: `notes-06-i32-spellings.md`, entries N01 to
N21.

Investigation only. No source file was modified. No other agent's file was read
or modified, other than the two named as inputs in my brief.

Throughout, `$T` means:

```
T="taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/"
```

and every command is run from `/opt/project/taichi`.

---

## 1. Headline

**Report 01 named four spellings. There are nine.** Two of the four it named
were already closed correctly, one was closed with a count that is one short, and
the fourth — the open one — turns out to be a different set from the one it
measured. Four spellings were never named at all, and the ninth was found by
answering the question `notes-01-ir-types.md` N065 item 4 hands to this pass:
whether any constructor other than `TypedConstant` narrows the same way. One
does.

| # | Spelling | How a 32-bit width enters | Lines returned | Real sites | New vs report 01 |
|---|---|---|---|---|---|
| A | `PrimitiveType::i32` | the `DataType` singleton, written out | 64 | 60 | 0 — verified exactly right |
| B | `PrimitiveTypeID::i32` | the enumerator, passed to `get_primitive_type` or a predicate | 27 | 5 | 2 |
| C | `get_data_type<int32>()` | the C++-type-to-Taichi-type template | 4 | 4 | 0, but report 01's count is 3 for a command that returns 4 |
| D | `TypedConstant(<host int>)` and `TypedConstant{<host int>}` | one-argument construction; the argument's C++ type selects `explicit TypedConstant(int32)` at `type.h:582` | 80 one-argument calls | 73 | **46** |
| E | a bare `DataType i32` object built by a file-local macro | `PRIM(i32)` at `taichi/ir/type_system.cpp:161-165` | 37 | 7 index-bearing of 35 uses | **all of them — the file is absent from report 01** |
| F | a literal bit count, or a member defaulting to 32 | `get_primitive_int_type(32, …)`; `int num_bits_{32}` | 11 | 2 | **2** |
| G | plain C++ `int` declared on an IR class | the value is an index, extent, stride or offset before any Taichi type exists | 102 header declarations | 47 address-bearing | partially covered by report 01 sections 3.5 and 3.6 |
| H | `PrimitiveType::u32` | the unsigned sibling of A | 4 | 1 | **1** |
| I | `Expr(<host int>)` | one-argument construction; the argument's C++ type selects `explicit Expr(int32)`, whose body is `make_shared<ConstExpression>(PrimitiveType::i32, x)` at `expr.cpp:93` | 39 `Expr(` lines | 2 lines, 3 constructions | **2** |

Counts in the "Lines returned" column are what the stated command returns and
are reproduced in section 3. The "Real sites" column is derived from the
per-spelling classification tables, never asserted over them.

**The single most consequential correction.** Report 01 describes the mechanism
as an *implicit* `int32` constructor. `taichi/ir/type.h:582` is
`explicit TypedConstant(int32 x)`. It is not implicit and never fires on a
conversion. What it does is win overload resolution for a host `int` argument, so
`TypedConstant(i)` pins `PrimitiveType::i32` with no `i32` token on the line.
The size of the class is unaffected; the definition of the class is not, and it
is the definition that makes the sweep reproducible.

**The second correction, which is also `notes-01-ir-types.md` N065 item 3
answered.** That note warns that its filter over-collects, because macro
continuation lines carry their type argument on the previous line and no
line-spanning parse was written. Report 01's filter for the open class is a
blacklist of identifier names for the first argument. Mine counts top-level commas after
balancing brackets across lines. Under mine, the eleven "macro continuation"
lines report 01 sets aside are not members of the class at all — they are
two-argument calls whose type argument sits on the previous physical line — and
seven brace-initialised constructions that report 01 could not see are members.
78 against 80, arrived at from different populations.

---

## 2. What defines membership, and what is excluded

A **real 32-bit width site** is a line at which a 32-bit type or a 32-bit value
is fixed and then reaches an IR type, an IR constant, an index, an offset, a
stride or an extent. That is the test applied on every row below.

Four things are excluded throughout, each with the reason stated on the row
rather than dropped:

1. **The mapping from a C++ type to a Taichi type.** `type_utils.h:40`, `:50`,
   `:71` map `int32`/`uint32` to `i32`/`u32`. That is correct, not an
   assumption. Report 01 excludes `:40` for this reason and I extend the same
   reason to its two siblings.
2. **The constructor declarations themselves.** `type.h:582` and its overloads
   at `:575-612` and `:616` are the mechanism, not sites. Report 01 excludes
   `:582` and I extend it to the whole overload set.
3. **`if / else if` dispatch arms that enumerate the whole type set.** A line
   like `else if (dt->is_primitive(PrimitiveTypeID::i32))` inside a chain that
   also handles i8, i16, i64, u8, u16, u32, u64, f32 and f64 asserts nothing
   about width. Every such line is listed and marked.
4. **Non-integer overloads.** `TypedConstant(true)` selects `uint1`, which is
   `bool` at `taichi/common/core.h:136`. `TypedConstant(x)` for a `float32 x`
   selects `float32`. `TypedConstant(dt)` for a `DataType dt` selects the
   one-argument `DataType` overload at `type.h:575` and zero-initialises a
   constant of the operand's own type.

---

## 3. The eight spellings, enumerated

### 3.1 Spelling A — `PrimitiveType::i32`

```
grep -rn "PrimitiveType::i32" $T          -> 64
grep -rn "PrimitiveType::i32" taichi/inc/ ->  0
```

`taichi/inc/` contributes nothing, which is why my four-directory territory and
report 01's three-directory one return the same 64.

**I verified report 01's section 3.13 table mechanically rather than adopting
it:**

```
awk 'NR>=650 && NR<=765 && /^\| `taichi\//' modernization/investigation/report-01-ir-types.md \
  | grep -oE 'taichi/[a-z_/]+\.(cpp|h):[0-9]+' | sort > r13.txt
grep -rn "PrimitiveType::i32" taichi/ir/ taichi/analysis/ taichi/transforms/ \
  | cut -d: -f1,2 | sort > actual.txt
diff r13.txt actual.txt
```

64 table rows, 64 grep hits, `diff` empty. **Report 01's spelling-A enumeration
is exactly right, its four exclusions are sound, and I adopt its dispositions
unchanged.** 64 lines less its 4 exclusions gives 60 real sites. I add nothing
and I remove nothing.

### 3.2 Spelling B — `PrimitiveTypeID::i32`

```
grep -rn "PrimitiveTypeID::i32" $T   -> 27
```

This is a distinct spelling. `grep "PrimitiveType::i32"` cannot match it, because
`ID` sits between. Verified disjoint:

```
comm -12 <(grep -rn "PrimitiveType::i32" $T | sort) \
         <(grep -rn "PrimitiveTypeID::i32" $T | sort) | wc -l    -> 0
```

**Report 01 never ran this spelling.** All 27 dispositioned:

| Disposition | Count | Lines |
|---|---|---|
| Construction or default that fixes a 32-bit width | 3 | `taichi/ir/type_factory.cpp:167`, `taichi/ir/type.cpp:238`, `taichi/ir/ir_builder.cpp:143` |
| Predicate test gating behaviour on i32-ness, already carried by report 01 | 5 | `taichi/transforms/type_check.cpp:148`, `:161`, `taichi/transforms/bit_loop_vectorize.cpp:321`, `taichi/analysis/value_diff.cpp:82`, `:143` |
| Predicate test, **not in report 01** | 2 | `taichi/transforms/frontend_type_check.cpp:72`, `taichi/transforms/simplify.cpp:234` |
| Type-set membership test listing every integer type — unrelated | 3 | `taichi/ir/type_utils.h:106`, `:122`, `taichi/transforms/constant_fold.cpp:37` |
| Dispatch arm over the whole type enumeration — unrelated | 13 | `taichi/ir/type_utils.h:137`, `:150`, `:178`, `taichi/ir/type_utils.cpp:126`, `taichi/ir/type.cpp:400`, `:434`, `:524`, `taichi/ir/type.h:621`, `taichi/transforms/constant_fold.cpp:77`, `:113`, `:204`, `:233`, `:253` |
| C++-type-to-Taichi-type mapping — excluded, exclusion 1 | 1 | `taichi/ir/type_utils.h:71` |

3 + 5 + 2 + 3 + 13 + 1 = 27.

The five real sites, with what each fixes:

- `taichi/ir/type_factory.cpp:167` — the `bits == 32` arm of
  `TypeFactory::get_primitive_int_type(int bits, bool is_signed)`. Every caller
  that asks for 32 bits lands here.
- `taichi/ir/type.cpp:238` — `QuantIntType`'s constructor: when `compute_type`
  is null the compute type **defaults to i32**, or u32 if unsigned. A typed
  helper defaulting to 32 bits.
- `taichi/ir/ir_builder.cpp:143` — the body of `IRBuilder::get_int32`. Its one
  in-territory caller is `ir_builder.cpp:125`, which report 01 carries.
- `taichi/transforms/frontend_type_check.cpp:72` — **new.** The frontend's own
  index check: any field index whose `ret_type` is not i32 raises a
  `TaichiCastWarning` reading "Field index {} not int32, casting into int32
  implicitly". Report 01 cites `frontend_type_check.cpp:71-77` in its
  pass-ordering section but has no inventory row for it. It is a second, earlier
  i32 gate on field indices, ahead of `type_check.cpp:148`.
- `taichi/transforms/simplify.cpp:234` — **new.**
  `TI_ASSERT(snode->ch[i]->dt->is_primitive(PrimitiveTypeID::i32) || … ::f32)`.
  This is the guard that makes the `sizeof(int32)` child stride at `:239` and
  `:261` correct. Report 01 carries both `sizeof(int32)` sites and not the
  assert that pins the child type to four bytes in the first place.

### 3.3 Spelling C — `get_data_type<int32>()`

```
grep -rn "get_data_type<int32>()" $T   -> 4
```

| Line | Disposition |
|---|---|
| `taichi/ir/type.cpp:463` | `TI_ASSERT(get_data_type<int32>() == dt);` inside `TypedConstant::val_int32()`. Real; already the basis of report 01's twelve-abort-site finding in its section 3.2 |
| `taichi/transforms/scalarize.cpp:63` | `TypedConstant(get_data_type<int32>(), i)` — matrix element index |
| `taichi/transforms/scalarize.cpp:114` | same |
| `taichi/transforms/scalarize.cpp:530` | same |

**Report 01 states 3 for this command and says the `type.cpp` hits are "the only
other hits tree-wide".** `taichi/ir/type.cpp` is inside `taichi/ir/`, which is
inside that report's own stated territory, so the command it prints returns 4.
`type.cpp:478` is `get_data_type<int64>()` and never matched at all. This is a
counting error rather than a missed site — the site is inventoried under a
different heading — but standing instruction 7 requires a figure to reconcile
against its own command, and this one does not.

### 3.4 Spelling D — the one report 01 left open

Two forms, and report 01 measured only the first:

```
grep -rn "TypedConstant(" $T   -> 139
grep -rn "TypedConstant{" $T   ->   7
```

The brace form was found by enumerating the follow-character of every occurrence
of the identifier:

```
grep -rno "TypedConstant." $T | sed 's/.*TypedConstant//' | sort | uniq -c | sort -rn
    139 (
     22 <space>
     19 :
     12 >
      7 {
```

The 22 space-following lines were each opened: they are the class definition,
field and parameter declarations, one format string, and local variable
declarations that take a `dst_type` argument or a copy. None is an untyped
integer construction. The 19 `:` lines are qualified names and the 12 `>` lines
are template arguments.

**Population: 146 constructions.** Split by arity, using a bracket-balancing
extractor that reads a call across line breaks and counts top-level commas:

| | Count |
|---|---|
| one-argument, paren | 86, of which 13 are the constructor declarations at `type.h:572-612` |
| two-argument, paren | 53, of which 1 is the declaration at `type.h:616` |
| one-argument, brace | 7 |

86 + 53 + 7 = 146; declarations 13 + 1 = 14; **call sites 132**; one-argument
call sites (86 − 13) + 7 = **80**; two-argument call sites 53 − 1 = **52**;
80 + 52 = 132.

The 52 two-argument calls are explicitly typed by construction. Thirteen name a
32-bit type and therefore belong to spellings A, B or C rather than to D
(`ir_builder.cpp:142`; `utils.cpp:17`; `make_mesh_thread_local.cpp:42`;
`make_cpu_multithreaded_range_for.cpp:68`, `:70`, `:72`, `:81`, `:90`;
`scalarize.cpp:471`, `:473`, `:63`, `:114`, `:530`). Five name a non-i32 fixed
type (`ir_builder.cpp:148`, `:154`, `:160`, `:166`, `:172`). The remaining 34
take a type derived from an operand and carry no width of their own.
13 + 5 + 34 = 52.

**So spelling D is exactly the 80 one-argument calls.** Every one was opened and
the declaration of its argument read, because the overload chosen depends on the
argument's C++ type and nothing else.

| Ruling | Count |
|---|---|
| Real 32-bit width site — selects `TypedConstant(int32)` | 73 |
| Excluded — `bool` argument, selects `uint1` | 3 |
| Excluded — `float32` argument, selects `float32` | 2 |
| Excluded — `DataType` argument, selects `TypedConstant(DataType)` | 2 |
| **Total** | **80** |

The full table follows. It has 80 rows, no duplicate `file:line`
(`cut -f1 … | sort | uniq -d` is empty), and its key set diffs clean against the
union of the two commands above minus the 13 declarations.

| Site | Argument | Argument C++ type | Overload | Ruling | What the width feeds |
|---|---|---|---|---|---|
| `taichi/ir/frontend_ir.cpp:663` | `offsets[i]` | std::vector<int> (frontend_ir.cpp:663 param) | i32 | **real i32 site — NEW** | field-index offset subtracted from each index in make_index_stmts |
| `taichi/ir/frontend_ir.cpp:732` | `0` | int literal | i32 | real i32 site — named in report 01 | accumulator seed for the dynamic tensor-element offset, feeds MatrixPtrStmt at :748 |
| `taichi/ir/frontend_ir.cpp:735` | `shape[i]` | std::vector<int> (:722 param) | i32 | real i32 site — named in report 01 | per-axis multiplier in the dynamic tensor-element offset |
| `taichi/ir/frontend_ir.cpp:747` | `offset` | int (:741) | i32 | real i32 site — inside a range report 01 cites, not named | static tensor-element offset, feeds MatrixPtrStmt at :748 |
| `taichi/transforms/utils.cpp:7` | `y - 1` | int (param :5) | i32 | **real i32 site — NEW** | bit_and mask in generate_mod |
| `taichi/transforms/utils.cpp:10` | `y` | int (param :5) | i32 | **real i32 site — NEW** | divisor in generate_mod |
| `taichi/transforms/utils.cpp:20` | `y` | int (param :14) | i32 | **real i32 site — NEW** | divisor in generate_div |
| `taichi/transforms/lower_matrix_ptr.cpp:189` | `i` | int loop var | i32 | real i32 site — named in report 01 | MatrixPtrStmt element offset |
| `taichi/transforms/lower_matrix_ptr.cpp:232` | `i` | int loop var | i32 | real i32 site — named in report 01 | MatrixPtrStmt element offset |
| `taichi/transforms/lower_matrix_ptr.cpp:424` | `i` | int loop var | i32 | real i32 site — named in report 01 | MatrixPtrStmt element offset |
| `taichi/transforms/lower_matrix_ptr.cpp:477` | `i` | int loop var | i32 | real i32 site — named in report 01 | MatrixPtrStmt element offset |
| `taichi/transforms/lower_matrix_ptr.cpp:537` | `origin->dynamic_index_stride` | int (statements.h:456) | i32 | real i32 site — inside a range report 01 cites, not named | stride multiplied into a MatrixPtrStmt offset for dynamically indexed matrix fields |
| `taichi/transforms/check_out_of_bound.cpp:47` | `0` | int literal | i32 | **real i32 site — NEW** | lower bound compared against every ExternalPtrStmt index |
| `taichi/transforms/check_out_of_bound.cpp:48` | `true` | bool -> uint1 overload | u1 | excluded | EXCLUDED: boolean accumulator, not a 32-bit width |
| `taichi/transforms/check_out_of_bound.cpp:74` | `flattened_element` | int (:54) | i32 | **real i32 site — NEW** | upper bound for element-shape axes of an ndarray |
| `taichi/transforms/check_out_of_bound.cpp:106` | `0` | int literal | i32 | **real i32 site — NEW** | lower bound compared against every GlobalPtrStmt index |
| `taichi/transforms/check_out_of_bound.cpp:107` | `true` | bool -> uint1 overload | u1 | excluded | EXCLUDED: boolean accumulator |
| `taichi/transforms/check_out_of_bound.cpp:126` | `upper_bound_i` | int (:125, from snode->shape_along_axis) | i32 | real i32 site — inside a range report 01 cites, not named | per-axis field extent as the bounds-check upper bound |
| `taichi/transforms/check_out_of_bound.cpp:142` | `offset_i` | int (:115, from snode->index_offsets) | i32 | **real i32 site — NEW** | index offset added to the reported index |
| `taichi/transforms/check_out_of_bound.cpp:179` | `0` | int literal | i32 | **real i32 site — NEW** | lower bound compared against a MatrixPtrStmt offset |
| `taichi/transforms/check_out_of_bound.cpp:180` | `true` | bool -> uint1 overload | u1 | excluded | EXCLUDED: boolean accumulator |
| `taichi/transforms/check_out_of_bound.cpp:186` | `max_valid_index` | int (:170) | i32 | **real i32 site — NEW** | product of the matrix shape, bounds-check upper bound |
| `taichi/transforms/check_out_of_bound.cpp:217` | `0` | int literal | i32 | real i32 site — inside a range report 01 cites, not named | NOT AN INDEX: comparison operand for the negative-exponent pow assertion |
| `taichi/transforms/simplify.cpp:175` | `0` | int literal | i32 | **real i32 site — NEW** | accumulator seed for the LinearizeStmt expansion into add/mul |
| `taichi/transforms/simplify.cpp:178` | `stride_product` | int (:176, auto = 1) | i32 | **real i32 site — NEW** | per-level stride constant in the LinearizeStmt expansion |
| `taichi/transforms/simplify.cpp:193` | `0` | int literal | i32 | real i32 site — inside a range report 01 cites, not named | zero compared against the linearized sum in the debug overflow assert |
| `taichi/transforms/demote_dense_struct_fors.cpp:49` | `0` | int literal | i32 | **real i32 site — NEW** | initial value of every reconstructed struct-for loop variable |
| `taichi/transforms/demote_dense_struct_fors.cpp:79` | `total_shape[p]` | std::array<int, taichi_max_num_indices> (:19) | i32 | **real i32 site — NEW** | per-axis multiplier rebuilding a loop index from the linear index |
| `taichi/transforms/bit_loop_vectorize.cpp:76` | `vectorization_width` | int (:74, data_type_bits) | i32 | **real i32 site — NEW** | offset added to a GlobalPtrStmt index for the vectorized load |
| `taichi/transforms/bit_loop_vectorize.cpp:188` | `0` | int literal | i32 | **real i32 site — NEW** | NOT AN INDEX: dummy zero added to lhs to mark a statement bit-vectorized |
| `taichi/transforms/alg_simp.cpp:71` | `rhs->ret_type` | DataType -> TypedConstant(DataType) overload | derived | excluded | EXCLUDED: zero-initialised constant of an operand-derived type |
| `taichi/transforms/alg_simp.cpp:92` | `scalar_stmt->ret_type` | DataType -> TypedConstant(DataType) overload | derived | excluded | EXCLUDED: zero-initialised constant of an operand-derived type |
| `taichi/transforms/demote_operations.cpp:99` | `i` | int loop var | i32 | real i32 site — named in report 01 | MatrixPtrStmt element offset for the per-element pow demotion |
| `taichi/transforms/demote_operations.cpp:134` | `0` | int literal | i32 | **real i32 site — NEW** | NOT AN INDEX: zero compared against operands in the ifloordiv demotion |
| `taichi/transforms/handle_external_ptr_boundary.cpp:30` | `0` | int literal | i32 | **real i32 site — NEW** | clamp lower bound for every ExternalPtrStmt index |
| `taichi/transforms/handle_external_ptr_boundary.cpp:39` | `1` | int literal | i32 | **real i32 site — NEW** | subtracted from the axis shape to give the clamp upper bound |
| `taichi/transforms/handle_external_ptr_boundary.cpp:71` | `0` | int literal | i32 | **real i32 site — NEW** | clamp lower bound for a MatrixPtrStmt offset |
| `taichi/transforms/handle_external_ptr_boundary.cpp:78` | `max_valid_index` | int (:63) | i32 | **real i32 site — NEW** | product of the matrix shape, clamp upper bound |
| `taichi/transforms/make_mesh_block_local.cpp:83` | `mapping_dtype_size_` | int (make_mesh_block_local.h:69) | i32 | **real i32 site — NEW** | byte stride multiplied into a BLS mapping index |
| `taichi/transforms/make_mesh_block_local.cpp:116` | `dtype_size` | int (:96, data_type_size) | i32 | **real i32 site — NEW** | byte stride multiplied into a BLS attribute index |
| `taichi/transforms/make_mesh_block_local.cpp:118` | `int32(offset_in_bytes)` | explicit narrowing of the map value at :97 | i32 | real i32 site — named in report 01 | BLS base byte offset |
| `taichi/transforms/make_mesh_block_local.cpp:166` | `1` | int literal | i32 | **real i32 site — NEW** | block dimension on x64/arm64 |
| `taichi/transforms/make_mesh_block_local.cpp:204` | `mapping_dtype_size_` | int (make_mesh_block_local.h:69) | i32 | **real i32 site — NEW** | byte stride multiplied into a BLS mapping index |
| `taichi/transforms/make_mesh_block_local.cpp:265` | `int32(offset_in_bytes)` | explicit narrowing of the map value at :249 | i32 | real i32 site — named in report 01 | BLS base byte offset |
| `taichi/transforms/make_mesh_block_local.cpp:268` | `dtype_size` | int (:227) | i32 | **real i32 site — NEW** | byte stride multiplied into a BLS attribute index |
| `taichi/transforms/make_mesh_block_local.cpp:301` | `int32(offset_in_bytes)` | explicit narrowing of the map value at :298 | i32 | real i32 site — named in report 01 | BLS base byte offset |
| `taichi/transforms/make_mesh_block_local.cpp:304` | `dtype_size` | int (:297) | i32 | **real i32 site — NEW** | byte stride multiplied into a BLS attribute index |
| `taichi/transforms/make_mesh_block_local.cpp:327` | `0` | int literal | i32 | **real i32 site — NEW** | thread index on x64/arm64, stands in for LoopLinearIndexStmt |
| `taichi/transforms/make_mesh_block_local.cpp:578` | `mapping_dtype_size_` | int (make_mesh_block_local.h:69) | i32 | **real i32 site — NEW** | byte stride multiplied into a BLS mapping index |
| `taichi/transforms/lower_ast.cpp:151` | `(int32)0` | explicit cast | i32 | real i32 site — named in report 01 | loop-mask zero for a bit-vectorized struct-for |
| `taichi/transforms/lower_ast.cpp:177` | `(int32)0xFFFFFFFF` | explicit cast | i32 | real i32 site — named in report 01 | loop-mask all-ones for a bit-vectorized range-for |
| `taichi/transforms/lower_ast.cpp:241` | `offsets[i]` | std::vector<int> (snode.h:79 index_offsets) | i32 | **real i32 site — NEW** | index offset added to a struct-for LoopIndexStmt |
| `taichi/transforms/lower_ast.cpp:270` | `0` | int literal | i32 | real i32 site — named in report 01 | range-for begin over an external array |
| `taichi/transforms/lower_ast.cpp:271` | `1` | int literal | i32 | real i32 site — named in report 01 | seed of the shape product that becomes the range-for end |
| `taichi/transforms/lower_ast.cpp:333` | `(int32)1` | explicit cast | i32 | real i32 site — named in report 01 | loop-mask one |
| `taichi/transforms/lower_ast.cpp:363` | `(int32)0xFFFFFFFF` | explicit cast | i32 | real i32 site — named in report 01 | loop-mask all-ones |
| `taichi/transforms/auto_diff.cpp:957` | `x` | float32 (param :953) | f32 | excluded | EXCLUDED: selects the float32 overload, not an integer width |
| `taichi/transforms/auto_diff.cpp:963` | `x` | float32 (param :953) | f32 | excluded | EXCLUDED: selects the float32 overload, not an integer width |
| `taichi/transforms/auto_diff.cpp:1819` | `(int32)i` | explicit cast of a size_t loop var | i32 | real i32 site — named in report 01 | MatrixPtrStmt element offset |
| `taichi/transforms/make_block_local.cpp:145` | `loop_offset` | int (:140) | i32 | **real i32 site — NEW** | block-stride loop offset added to the thread index |
| `taichi/transforms/make_block_local.cpp:152` | `dtype_size` | int (:62, data_type_size) | i32 | **real i32 site — NEW** | byte stride multiplied into a BLS element index |
| `taichi/transforms/make_block_local.cpp:157` | `(int32)bls_offset_in_bytes` | explicit narrowing of std::size_t (:57) | i32 | real i32 site — named in report 01 | BLS base byte offset |
| `taichi/transforms/make_block_local.cpp:165` | `bls_num_elements` | int (:76, pad_size_linear) | i32 | **real i32 site — NEW** | BLS element count bound for the block-stride loop |
| `taichi/transforms/make_block_local.cpp:183` | `pad.second.pad_size[i]` | std::vector<int> (scratch_pad.h:51) | i32 | **real i32 site — NEW** | per-axis pad extent used as a mod/div divisor |
| `taichi/transforms/make_block_local.cpp:194` | `pad.second.bounds[i].low` | int (scratch_pad.h:34) | i32 | **real i32 site — NEW** | per-axis BLS lower bound added to a coordinate |
| `taichi/transforms/make_block_local.cpp:202` | `pad.second.coefficients[i]` | std::vector<int> (scratch_pad.h:47) | i32 | **real i32 site — NEW** | per-axis block coefficient multiplied into a global coordinate |
| `taichi/transforms/make_block_local.cpp:277` | `pad.second.coefficients[i]` | std::vector<int> (scratch_pad.h:47) | i32 | **real i32 site — NEW** | per-axis block coefficient multiplied into a global coordinate |
| `taichi/transforms/make_block_local.cpp:285` | `pad.second.bounds[i].low` | int (scratch_pad.h:34) | i32 | **real i32 site — NEW** | per-axis BLS lower bound added to a coordinate |
| `taichi/transforms/make_block_local.cpp:298` | `0` | int literal | i32 | **real i32 site — NEW** | lower bound in the BLS bounds assertion |
| `taichi/transforms/make_block_local.cpp:303` | `bls_axis_size` | int (:290) | i32 | **real i32 site — NEW** | upper bound in the BLS bounds assertion |
| `taichi/transforms/make_block_local.cpp:315` | `bls_strides[i]` | std::vector<int> (:79) | i32 | **real i32 site — NEW** | per-axis BLS stride multiplied into a BLS element offset |
| `taichi/transforms/make_block_local.cpp:328` | `dtype_size` | int (:62) | i32 | **real i32 site — NEW** | byte stride multiplied into a BLS element offset |
| `taichi/transforms/make_block_local.cpp:334` | `(int32)bls_offset_in_bytes` | explicit narrowing of std::size_t (:57) | i32 | real i32 site — named in report 01 | BLS base byte offset |
| `taichi/transforms/demote_mesh_statements.cpp:86` | `?: yielding 6 or from_order + 1` | int (mesh.h:48 element_order) | i32 | **real i32 site — NEW** | fixed size of a high-to-low mesh relation |
| `taichi/transforms/demote_mesh_statements.cpp:94` | `?: yielding 6 or from_order + 1` | int (mesh.h:48 element_order) | i32 | **real i32 site — NEW** | fixed size of a high-to-low mesh relation, multiplied into a relation-table index |
| `taichi/transforms/demote_mesh_statements.cpp:123` | `1` | int literal | i32 | **real i32 site — NEW** | one added to a relation-offset table index |
| `taichi/transforms/make_mesh_block_local.cpp:80` | `(int32)mapping_bls_offset_in_bytes_` | explicit narrowing of std::size_t | i32 | real i32 site — named in report 01 | BLS mapping base byte offset |
| `taichi/transforms/make_mesh_block_local.cpp:169` | `offload_->block_dim` | int (statements.h:1418) | i32 | **real i32 site — NEW** | block dimension on GPU targets |
| `taichi/transforms/make_mesh_block_local.cpp:200` | `(int32)mapping_bls_offset_in_bytes_` | explicit narrowing of std::size_t | i32 | real i32 site — named in report 01 | BLS mapping base byte offset |
| `taichi/transforms/make_mesh_block_local.cpp:575` | `(int32)mapping_bls_offset_in_bytes_` | explicit narrowing of std::size_t | i32 | real i32 site — named in report 01 | BLS mapping base byte offset |

**Where report 01 stands on the 73 real sites.** Computed by script against the
report text, then hand-checked for false matches:

| Tier | Meaning | Count |
|---|---|---|
| A | named in report 01 as `file:line`, or as a `:NNN` shorthand on a line that also names the file | 22 |
| B | falls inside a line range report 01 cites, but is not named as a site | 5 |
| C | **absent from report 01 entirely** | **46** |

22 + 5 + 46 = 73.

Two script matches were false and were moved from A to C by hand:
`make_block_local.cpp:165` matched report 01's `make_mesh_thread_local.cpp:165`,
and `make_mesh_block_local.cpp:204` matched its `type_factory.cpp:204` — in both
cases a different file with the same line number. One tier-A site,
`demote_operations.cpp:99`, is cited by report 01 **and explicitly excluded from
its inventory**, which that report states plainly.

The 46 new sites by file, derived from the tier file, not counted by hand:

| File | New sites |
|---|---|
| `taichi/transforms/make_block_local.cpp` | 12 |
| `taichi/transforms/make_mesh_block_local.cpp` | 9 |
| `taichi/transforms/check_out_of_bound.cpp` | 6 |
| `taichi/transforms/handle_external_ptr_boundary.cpp` | 4 |
| `taichi/transforms/utils.cpp` | 3 |
| `taichi/transforms/demote_mesh_statements.cpp` | 3 |
| `taichi/transforms/simplify.cpp` | 2 |
| `taichi/transforms/demote_dense_struct_fors.cpp` | 2 |
| `taichi/transforms/bit_loop_vectorize.cpp` | 2 |
| `taichi/transforms/lower_ast.cpp` | 1 |
| `taichi/transforms/demote_operations.cpp` | 1 |
| `taichi/ir/frontend_ir.cpp` | 1 |

12+9+6+4+3+3+2+2+2+1+1+1 = 46, twelve files.

### 3.5 Spelling E — a bare `DataType i32` built by a file-local macro

`taichi/ir/type_system.cpp` is third by volume of `i32` tokens in the territory
and **appears nowhere in report 01 or its notes**:
`grep -n "type_system" report-01-ir-types.md notes-01-ir-types.md` returns
nothing.

```
161:#define PRIM(dt) \
162:  DataType dt =  \
163:      TypeFactory::get_instance().get_primitive_type(PrimitiveTypeID::dt);
165:PRIM(i32)
171:DataType i32_void = i32;
```

Inside that anonymous namespace the bare token `i32` **is a `DataType` object**,
so every use of it in the internal-operation signature table declares a 32-bit
type. It contains no `PrimitiveType::i32`, no `TypedConstant`, no `(int32)` and
no `get_data_type<int32>`, so all four of report 01's sweeps pass over it.

```
grep -n "\bi32\b\|i32_void" taichi/ir/type_system.cpp   -> 37
```

Two definitions (`:165`, `:171`) and 35 use lines. Classified:

| Class | Lines |
|---|---|
| Return type i32 on an op that produces a **thread or lane index** | `:307` `linear_thread_idx`, `:369` `localInvocationId`, `:370` `vkGlobalThreadIdx`, `:376` `subgroupInvocationId` |
| i32 **argument** that is a sparse-matrix row or column index | `:302` `insert_triplet_##dt(u64, i32, i32, dt)` |
| SPIR-V subgroup query returning i32 | `:373` `subgroupElect`, `:375` `subgroupSize` |
| CUDA warp intrinsic, i32 as the hardware lane mask or shuffle datum | `:325`, `:327`, `:332`, `:333`, `:334`, `:339`, `:340`, `:341`, `:342`, `:343`, `:347`, `:348` |
| `i32_void` as the return type of a context-taking void internal — a placeholder, not a width | `:302`, `:308`, `:309`, `:310`, `:311`, `:312`, `:313`, `:314`, `:315`, `:331`, `:335`, `:350`, `:367`, `:368`, `:371`, `:372` |
| Test-only internal | `:316` |

`:302` is listed twice because that one line carries both an `i32_void` return
and two i32 arguments. Distinct lines: 4 + 1 + 2 + 12 + 16 + 1 = 36 entries over
**35 distinct lines**, the difference being `:302`. 35 = 37 − 2 definitions.

**Seven of the 35 fix a 32-bit index or lane quantity** — the four thread and
lane indices, the sparse-matrix triplet line, and the two subgroup queries. The
four thread-index returns are the ones that matter for item 6.2: a global thread
index declared i32 caps the launch index space at 2^31 inside the type system
itself, and it does so on both compilation spines, `linear_thread_idx` on the
LLVM side and `vkGlobalThreadIdx` on the SPIR-V side.

Whether these are in scope depends on a ruling I am not entitled to make: the
declarations are in my territory, the consumers are in territory 02. Escalated.

### 3.6 Spelling F — a literal bit count, or a member defaulting to 32

Found by asking which lines carry a bare `32` as a **width** rather than as part
of a type name:

```
grep -rn "\b32\b" $T | grep -v "int32\|uint32\|i32\|u32\|f32\|float32\|//"   -> 11
```

| Line | Disposition |
|---|---|
| `taichi/ir/frontend_ir.cpp:266` | **real 32-bit width site, new.** `get_primitive_int_type(32, /*is_signed=*/true)` builds the exponent member of the struct type returned by `frexp`. A literal bit count. Invisible to every sweep in report 01 and absent from it |
| `taichi/ir/type.h:372` | **real 32-bit default, new.** `int num_bits_{32};` on `QuantIntType`, with the runtime half at `type.cpp:236-240` resolving a null compute type to i32. The comment at `:369-370` states the assumption in the source: "for now we can uniformly use i32 as the compute_type" |
| `taichi/ir/type_factory.cpp:166` | the `bits == 32` arm of `get_primitive_int_type` — the selector, counted under spelling B at `:167` |
| `taichi/ir/type_factory.cpp:183` | the `bits == 32` arm of `get_primitive_real_type` — float, not integer |
| `taichi/ir/frontend_ir.h:1174` | `TI_ASSERT((v % 32 == 0) || …)` on `block_dim` for cuda, vulkan and amdgpu. Launch granularity, not a data width |
| `taichi/inc/cuda_kernel_utils.inc.h:8` | `int warp_size() { return 32; }`, a host-side stub. Hardware warp width |
| `taichi/analysis/build_cfg.cpp:25` | `((std::size_t)key.in_parallel_for << 32)`, a hash-key packing shift. Unrelated |
| `taichi/transforms/alg_simp.cpp:13` | `max_weaken_exponent = 32`, a strength-reduction budget. Unrelated |
| `taichi/inc/constants.h:11` | `taichi_max_num_args_extra = 32`. Argument count |
| `taichi/inc/constants.h:19` | `taichi_error_message_max_num_arguments = 32`. Unrelated |
| `taichi/inc/constants.h:20` | `taichi_result_buffer_entries = 32`. Unrelated |

Eleven rows, eleven lines, **two real sites**, neither in report 01.

### 3.7 Spelling H — `PrimitiveType::u32`

```
grep -rn "PrimitiveType::u32" $T   -> 4
```

| Line | Disposition |
|---|---|
| `taichi/ir/type_utils.h:50` | the `uint32` arm of `get_data_type<T>()`. Excluded, exclusion 1 |
| `taichi/ir/type_utils.h:138` | the i32 arm of `to_unsigned()`. Dispatch arm |
| `taichi/ir/type.h:609` | the `TypedConstant(uint32)` constructor. The mechanism, exclusion 2 |
| `taichi/transforms/make_mesh_thread_local.cpp:26` | **real 32-bit address width, new.** `auto data_type = PrimitiveType::u32;  // unt32_t type address`, the element type of the mesh offset tables built by this pass, whose byte stride is taken from it at `:27`. Report 01's only rows for this file are `:42`, `:106` and `:114` |

`taichi/ir/type.cpp:238` spells its unsigned arm `PrimitiveTypeID::u32` and is
therefore counted under spelling B, not here — the same disjointness established
in section 3.2, working the other way.

### 3.8 Spelling G — plain C++ `int` on an IR class

This is the spelling with no natural boundary, so I state the boundary I chose
and what it leaves out, instead of implying a sweep I did not run.

**Membership closed:** every `int`, `std::vector<int>` or `std::array<int,N>`
declaration on its own line in a **header** of the territory.

```
grep -rnE '^[[:space:]]*(mutable[[:space:]]+)?(const[[:space:]]+)?(int|std::vector<int>|std::array<int,[^>]*>)[[:space:]]+[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*(\{[^}]*\})?[[:space:]]*(=[^;]*)?;' \
  taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/ --include=*.h
```

**102 lines across 16 files.** Every one classified:

| Ruling | Count |
|---|---|
| ADDR — participates in computing an address, index, extent, stride or bit offset | 47 |
| NONADDR — an IR quantity that is not addressing: ids, launch configuration, argument paths, texture parameters | 46 |
| LOCAL — the regex caught a local variable inside an inline method, not a member | 7 |
| PARAM — the regex caught a function parameter with a default, not a member | 2 |
| **Total** | **102** |

47 + 46 + 7 + 2 = 102, and the 102 `file:line` keys diff clean against the
command's own output.

The load-bearing ADDR rows, on my reading: `snode.h:41` `num_elements_from_root`,
`:45` `shape`, `:49` `acc_shape`, `:80` `num_active_indices` and `:89` `id`;
`statements.h:1280` `std::vector<int> strides` on `LinearizeStmt`;
`statements.h:456` and `frontend_ir.h:617` `dynamic_index_stride`;
`type.h:223` and `:250` on `TensorType`. Those are the quantities that decide
how large a structure can be described before any Taichi type is chosen.

| Declaration | Ruling | What it holds |
|---|---|---|
| `taichi/ir/mesh.h:77` `int num_patches{0}` | ADDR | mesh patch count |
| `taichi/ir/ir_builder.h:15` `int position{0}` | NONADDR | IRBuilder insertion cursor |
| `taichi/ir/ir_builder.h:79` `int location_` | NONADDR | LoopGuard insertion cursor |
| `taichi/ir/ir_builder.h:92` `int location_` | NONADDR | IfGuard insertion cursor |
| `taichi/ir/ir_builder.h:114` `int block_dim = 0)` | PARAM | function parameter, not a member; caught by the regex through the trailing semicolon |
| `taichi/ir/ir_builder.h:119` `int block_dim = 0)` | PARAM | function parameter, not a member |
| `taichi/ir/type_utils.h:253` `int old_num_members = member_types_.size()` | LOCAL | local inside an inline method |
| `taichi/ir/type_utils.h:278` `std::vector<int> member_bit_offsets_` | ADDR | bit offsets inside a bit struct |
| `taichi/ir/type_utils.h:279` `int member_total_bits_{0}` | ADDR | accumulated bit width of a bit struct |
| `taichi/ir/type_utils.h:280` `std::vector<int> member_exponents_` | NONADDR | member indices for shared exponents |
| `taichi/ir/type_utils.h:283` `int current_shared_exponent_{-1}` | NONADDR | member index |
| `taichi/ir/control_flow_graph.h:114` `const int start_node = 0` | NONADDR | CFG node index |
| `taichi/ir/control_flow_graph.h:115` `int final_node{0}` | NONADDR | CFG node index |
| `taichi/ir/type_system.h:140` `const int position_` | NONADDR | type-variable position |
| `taichi/ir/type_system.h:199` `const int occurrence_` | NONADDR | type-inference occurrence counter |
| `taichi/ir/ir.h:83` `int id{0}` | NONADDR | Identifier id |
| `taichi/ir/ir.h:401` `int instance_id` | NONADDR | Stmt instance id |
| `taichi/ir/ir.h:402` `int id` | NONADDR | Stmt id |
| `taichi/ir/snode.h:23` `int value` | ADDR | Axis number, bounded by taichi_max_num_indices at :28-30 |
| `taichi/ir/snode.h:41` `int num_elements_from_root{1}` | ADDR | element count from root |
| `taichi/ir/snode.h:45` `int shape{1}` | ADDR | per-axis extractor extent |
| `taichi/ir/snode.h:49` `int acc_shape{1}` | ADDR | accumulated extent used as an index divisor |
| `taichi/ir/snode.h:79` `std::vector<int> index_offsets` | ADDR | per-axis index offsets |
| `taichi/ir/snode.h:80` `int num_active_indices{0}` | ADDR | number of active axes |
| `taichi/ir/snode.h:89` `int id{0}` | ADDR | SNode id, the quantity taichi_max_num_snodes bounds |
| `taichi/ir/snode.h:90` `int depth{0}` | NONADDR | tree depth |
| `taichi/ir/snode.h:98` `int chunk_size{0}` | ADDR | dynamic SNode chunk size |
| `taichi/ir/snode.h:110` `int id_in_bit_struct{-1}` | NONADDR | child index inside a bit struct |
| `taichi/ir/snode.h:353` `int snode_tree_id_{0}` | NONADDR | tree id, the quantity kMaxNumSnodeTreesLlvm bounds |
| `taichi/analysis/mesh_bls_analyzer.h:25` `int unique_accessed` | NONADDR | analysis counter |
| `taichi/ir/type.h:207` `int addr_space_{0}` | NONADDR | pointer address-space tag |
| `taichi/ir/type.h:223` `int num_elements = 1` | ADDR | TensorType element count |
| `taichi/ir/type.h:250` `std::vector<int> shape_` | ADDR | TensorType shape |
| `taichi/ir/type.h:308` `int num = 0` | LOCAL | local inside get_flattened_num_elements() |
| `taichi/ir/type.h:372` `int num_bits_{32}` | ADDR | QuantIntType bit width, DEFAULTING TO 32 |
| `taichi/ir/type.h:494` `std::vector<int> member_bit_offsets_` | ADDR | bit offsets inside a bit struct |
| `taichi/ir/type.h:495` `std::vector<int> member_exponents_` | NONADDR | member indices for shared exponents |
| `taichi/ir/type.h:545` `int num_elements_` | ADDR | QuantArrayType element count |
| `taichi/ir/type.h:546` `int element_num_bits_` | ADDR | QuantArrayType element bit width |
| `taichi/transforms/make_mesh_block_local.h:69` `int mapping_dtype_size_{0}` | ADDR | byte stride for BLS mapping |
| `taichi/ir/statements.h:185` `std::vector<int> arg_id` | NONADDR | kernel argument identifier path |
| `taichi/ir/statements.h:201` `int arg_depth` | NONADDR | argument nesting depth |
| `taichi/ir/statements.h:376` `int ndim` | ADDR | ExternalPtrStmt dimension count |
| `taichi/ir/statements.h:379` `std::vector<int> element_shape` | ADDR | ExternalPtrStmt element shape |
| `taichi/ir/statements.h:456` `int dynamic_index_stride{0}` | ADDR | GlobalPtrStmt dynamic index stride |
| `taichi/ir/statements.h:597` `int axis` | ADDR | ExternalTensorShapeAlongAxisStmt axis number |
| `taichi/ir/statements.h:598` `std::vector<int> arg_id` | NONADDR | argument identifier path |
| `taichi/ir/statements.h:614` `std::vector<int> arg_id` | NONADDR | argument identifier path |
| `taichi/ir/statements.h:1019` `int num_cpu_threads` | NONADDR | launch configuration |
| `taichi/ir/statements.h:1020` `int block_dim` | NONADDR | launch configuration |
| `taichi/ir/statements.h:1063` `std::vector<int> index_offsets` | ADDR | StructForStmt index offsets |
| `taichi/ir/statements.h:1065` `int num_cpu_threads` | NONADDR | launch configuration |
| `taichi/ir/statements.h:1066` `int block_dim` | NONADDR | launch configuration |
| `taichi/ir/statements.h:1098` `int num_cpu_threads` | NONADDR | launch configuration |
| `taichi/ir/statements.h:1099` `int block_dim` | NONADDR | launch configuration |
| `taichi/ir/statements.h:1187` `std::vector<int> index` | ADDR | GetElementStmt struct member path |
| `taichi/ir/statements.h:1280` `std::vector<int> strides` | ADDR | LinearizeStmt per-axis strides |
| `taichi/ir/statements.h:1370` `int chid` | ADDR | GetChStmt child id |
| `taichi/ir/statements.h:1417` `int grid_dim{1}` | NONADDR | launch configuration |
| `taichi/ir/statements.h:1418` `int block_dim{1}` | NONADDR | launch configuration |
| `taichi/ir/statements.h:1421` `int num_cpu_threads{1}` | NONADDR | launch configuration |
| `taichi/ir/statements.h:1439` `std::vector<int> index_offsets` | ADDR | OffloadedStmt index offsets |
| `taichi/ir/statements.h:1498` `int index` | ADDR | LoopIndexStmt axis number |
| `taichi/ir/statements.h:1577` `int index` | ADDR | BlockCornerIndexStmt axis number |
| `taichi/ir/statements.h:1700` `int dimensions{2}` | NONADDR | texture dimensionality |
| `taichi/ir/statements.h:1705` `int lod{0}` | NONADDR | texture level of detail |
| `taichi/ir/statements.h:1956` `std::vector<int> ch_ids` | ADDR | bit-struct child ids being stored |
| `taichi/ir/analysis.h:20` `int coeff` | ADDR | DiffRange index coefficient |
| `taichi/ir/analysis.h:185` `int diff_range{0}` | ADDR | index difference range |
| `taichi/analysis/bls_analyzer.h:16` `int low{0}` | ADDR | BLS access bound |
| `taichi/analysis/bls_analyzer.h:17` `int high{0}` | ADDR | BLS access bound |
| `taichi/ir/scratch_pad.h:34` `int low{0}` | ADDR | BLS bound |
| `taichi/ir/scratch_pad.h:35` `int high{0}` | ADDR | BLS bound |
| `taichi/ir/scratch_pad.h:47` `std::vector<int> coefficients` | ADDR | per-axis block coefficients |
| `taichi/ir/scratch_pad.h:51` `std::vector<int> pad_size` | ADDR | per-axis pad extents |
| `taichi/ir/scratch_pad.h:53` `std::vector<int> block_size` | ADDR | per-axis block extents |
| `taichi/ir/scratch_pad.h:55` `int dim` | ADDR | dimension count |
| `taichi/ir/scratch_pad.h:97` `int size = 1` | LOCAL | local inside an inline method |
| `taichi/ir/scratch_pad.h:133` `int s = 1` | LOCAL | local inside pad_size_linear() |
| `taichi/ir/scratch_pad.h:142` `int s = 1` | LOCAL | local inside an inline method |
| `taichi/ir/scratch_pad.h:150` `int ret = 0` | LOCAL | local inside an inline method |
| `taichi/ir/scratch_pad.h:232` `int offset = 0` | LOCAL | local inside an inline method |
| `taichi/transforms/scalar_pointer_lowerer.h:72` `int path_length_{0}` | ADDR | SNode path length |
| `taichi/ir/frontend_ir.h:21` `int num_cpu_threads{0}` | NONADDR | launch configuration |
| `taichi/ir/frontend_ir.h:24` `int block_dim{0}` | NONADDR | launch configuration |
| `taichi/ir/frontend_ir.h:206` `int num_cpu_threads` | NONADDR | launch configuration |
| `taichi/ir/frontend_ir.h:209` `int block_dim` | NONADDR | launch configuration |
| `taichi/ir/frontend_ir.h:339` `const std::vector<int> arg_id` | NONADDR | argument identifier path |
| `taichi/ir/frontend_ir.h:349` `int arg_depth` | NONADDR | argument nesting depth |
| `taichi/ir/frontend_ir.h:380` `const std::vector<int> arg_id` | NONADDR | argument identifier path |
| `taichi/ir/frontend_ir.h:381` `int num_dims` | NONADDR | texture dimensionality |
| `taichi/ir/frontend_ir.h:383` `int arg_depth` | NONADDR | argument nesting depth |
| `taichi/ir/frontend_ir.h:387` `int lod{0}` | NONADDR | texture level of detail |
| `taichi/ir/frontend_ir.h:529` `int ndim` | ADDR | ExternalTensorExpression dimension count |
| `taichi/ir/frontend_ir.h:530` `std::vector<int> arg_id` | NONADDR | argument identifier path |
| `taichi/ir/frontend_ir.h:533` `int arg_depth` | NONADDR | argument nesting depth |
| `taichi/ir/frontend_ir.h:615` `std::vector<int> element_shape` | ADDR | external tensor element shape |
| `taichi/ir/frontend_ir.h:617` `int dynamic_index_stride{0}` | ADDR | dynamic index stride |
| `taichi/ir/frontend_ir.h:676` `std::vector<int> ret_shape` | ADDR | returned tensor shape |
| `taichi/ir/frontend_ir.h:851` `int axis` | ADDR | ExternalTensorShapeAlongAxisExpression axis number |
| `taichi/ir/frontend_ir.h:912` `std::vector<int> index` | ADDR | GetElementExpression struct member path |
| `taichi/ir/frontend_ir.h:1038` `int id_counter_{0}` | NONADDR | identifier counter |

### 3.9 Spelling I — `Expr(<host int>)`, and the closure of the narrowing question

`notes-01-ir-types.md` N065 item 4 hands this pass an open question: the filter
that found the `TypedConstant` class was only ever tested against
`TypedConstant`, and whether other constructors in this territory narrow the same
way was never asked. It is asked here, and answered as a closed question rather
than by guessing what to grep for.

**The closure argument.** A construct that pins a Taichi 32-bit type must name
that type inside its own body, and there are only so many ways to name it — all
of them enumerated in sections 3.1 to 3.7: 64 + 27 + 4 + 37 + 11 = 143 lines.
So the question is finite: **which of those 143 sit inside a construct keyed on a
C++ argument type or template parameter, rather than on a type the caller wrote?**

| Line | Construct | Keyed on |
|---|---|---|
| `taichi/ir/type_utils.h:40` | `get_data_type<T>()`, the `int32` arm | template parameter |
| `taichi/ir/type_utils.h:71` | `get_primitive_data_type<T>()`, the `int32` arm | template parameter |
| `taichi/ir/type.h:582` | `explicit TypedConstant(int32 x)` | argument type — spelling D |
| `taichi/ir/expr.cpp:93` | `Expr::Expr(int32 x)` body | argument type — **spelling I** |
| `taichi/ir/ir_builder.cpp:143` | `IRBuilder::get_int32` body | nothing; the name states the width |
| `taichi/ir/type_factory.cpp:167` | `get_primitive_int_type` | the runtime value of `bits` |
| `taichi/ir/type.cpp:238` | `QuantIntType` constructor | `compute_type == nullptr` |
| `taichi/ir/type.h:372` | `int num_bits_{32}` | nothing; a member default |
| all remaining lines of A, B, C, E, F | dispatch arms, predicate tests, written-out types | the caller's written type |

One further construct forwards into `TypedConstant` without naming a type at all,
so it is not among the 143:

```
taichi/ir/frontend_ir.h:832-835
  template <typename T>
  explicit ConstExpression(const T &x) : val(x) {
    ret_type = val.dt;
  }
```

**Answer: the complete set of narrowing entry points in this territory is three,
not one** — `TypedConstant(int32)` at `type.h:582`, `Expr(int32)` at
`expr.h:30` / `expr.cpp:92-94`, and the `ConstExpression(const T&)` forwarder at
`frontend_ir.h:833`. All three are `explicit`, so none fires on a conversion and
all three require direct construction.

**Entry point 2, enumerated.**

```
grep -rnE '(^|[^A-Za-z0-9_:>])Expr\(' $T   -> 39
```

| Disposition | Lines |
|---|---|
| Constructor definitions, `expr.cpp:84-108` | 7 |
| Constructor declarations, `expr.h:21-53` | 10 |
| `Expr::make` at `expr.h:90` and the helper returning `Expr(val)` at `expr.h:131` | 2 |
| Defaulted parameter at `frontend_ir.h:115` | 1 |
| Construction from a `shared_ptr<Expression>`, an `Identifier` or an SNode lookup | 17 |
| **Construction from a host `int`** | **2** |

7 + 10 + 2 + 1 + 17 + 2 = 39.

The two:

```
1835:          for (int i = 0; i < shape[0]; i++) {
1836:            auto ind = Expr(std::make_shared<IndexExpression>(
1837:                id_expr, ExprGroup(Expr(i)), expr->dbg_info));

1843:          for (int i = 0; i < shape[0]; i++) {
1844:            for (int j = 0; j < shape[1]; j++) {
1845:              auto ind = Expr(std::make_shared<IndexExpression>(
1846:                  id_expr, ExprGroup(Expr(i), Expr(j)), expr->dbg_info));
```

`taichi/ir/frontend_ir.cpp:1837` carries one construction, `:1846` carries two:
**three constructions on two lines.** Both sit in `ASTBuilder::expand_exprs`,
which begins at `:1795`. `i` and `j` are host `int` loop variables declared at
`:1835`, `:1843` and `:1844`, bounded by `tensor_type->get_shape()`, itself
`std::vector<int>` at `taichi/ir/type.h:250`.

What the width feeds: each `Expr(i)` becomes a `ConstExpression` of
`PrimitiveType::i32`, is wrapped in an `ExprGroup`, and becomes the index of an
`IndexExpression` on a tensor-typed variable. **Frontend tensor element
addressing** — the same class as the `MatrixPtrStmt` offsets of spelling D, one
layer earlier in the pipeline.

Report 01 saw `expr.cpp:93` in its spelling-A sweep and excluded it correctly, as
the mechanism rather than a site. What was never then asked is what calls it.
Neither `frontend_ir.cpp:1837` nor `:1846` appears anywhere in report 01.

**Entry point 3, enumerated and empty here.**

```
grep -rn "ConstExpression" $T   -> 18
```

All 18 dispositioned: 6 are `make_shared<ConstExpression>(PrimitiveType::…, x)`
inside `Expr`'s own constructors, every one the **two**-argument, explicitly
typed form; 3 are the class and its two constructor declarations; 2 are `is<>` /
`cast<>` tests; 4 are the class's own `type_check`, `flatten` and printer; 2 are
the offline-cache key emitter; 1 is the registration macro. 6+3+2+4+2+1 = 18.
**Zero call sites of the one-argument form in this territory.** The forwarder
would be a site if used and is not used here; any user is outside my four
directories, and the Python front end is out of scope by plan section 1.2.

**Four further templates that route a C++ type into a Taichi type** —
`value<T>` at `expr.h:131` (which itself calls `Expr(val)`), `cast<T>` at `:118`,
`bit_cast<T>` at `:125` and `expr_rand<T>` at `:138` — have **no in-territory
instantiations**:

```
grep -rn "value<" $T | grep -v expr.h      -> 0
grep -rn "bit_cast<" $T | grep -v expr.h   -> 0
grep -rn "expr_rand<" $T | grep -v expr.h  -> 0
grep -rn "cast<int" $T | grep -v expr.h    -> 1
```

The single `cast<int` hit is `taichi/ir/snode.cpp:92`,
`static_cast<int>(acc_shape)` — a plain C++ narrowing, not this template. Report
01 already carries it at its sections 3.9 and 7, together with the `int64`
accumulator and the `TaichiIndexWarning` at `:95-100`. Nothing new there.


---

## 4. Every real site not already in report 01

Sixty site lines. The brief asks for those not in report 01's section 3.13; I
give the stricter and more useful set — those not anywhere in report 01 at all,
determined by the tier procedure in section 3.4 and by direct search for the
other spellings.

| Spelling | New site lines |
|---|---|
| D — one-argument `TypedConstant` | 46 |
| E — bare `DataType i32` in `type_system.cpp` | 7 |
| B — `PrimitiveTypeID::i32` | 2 |
| F — literal bit count or 32-bit default | 2 |
| I — `Expr(<host int>)` | 2 |
| H — `PrimitiveType::u32` | 1 |
| **Total** | **60** |

46 + 7 + 2 + 2 + 2 + 1 = 60, and the table below has 60 rows. **The unit is a
line, not a construction.** `frontend_ir.cpp:1846` carries two constructions on
one line, so the sixty lines hold sixty-one constructions.

| Site | Spelling | What the width feeds |
|---|---|---|
| `taichi/ir/frontend_ir.cpp:663` | D | field-index offset subtracted from each index in make_index_stmts |
| `taichi/transforms/utils.cpp:7` | D | bit_and mask in generate_mod |
| `taichi/transforms/utils.cpp:10` | D | divisor in generate_mod |
| `taichi/transforms/utils.cpp:20` | D | divisor in generate_div |
| `taichi/transforms/check_out_of_bound.cpp:47` | D | lower bound compared against every ExternalPtrStmt index |
| `taichi/transforms/check_out_of_bound.cpp:74` | D | upper bound for element-shape axes of an ndarray |
| `taichi/transforms/check_out_of_bound.cpp:106` | D | lower bound compared against every GlobalPtrStmt index |
| `taichi/transforms/check_out_of_bound.cpp:142` | D | index offset added to the reported index |
| `taichi/transforms/check_out_of_bound.cpp:179` | D | lower bound compared against a MatrixPtrStmt offset |
| `taichi/transforms/check_out_of_bound.cpp:186` | D | product of the matrix shape, bounds-check upper bound |
| `taichi/transforms/simplify.cpp:175` | D | accumulator seed for the LinearizeStmt expansion into add/mul |
| `taichi/transforms/simplify.cpp:178` | D | per-level stride constant in the LinearizeStmt expansion |
| `taichi/transforms/demote_dense_struct_fors.cpp:49` | D | initial value of every reconstructed struct-for loop variable |
| `taichi/transforms/demote_dense_struct_fors.cpp:79` | D | per-axis multiplier rebuilding a loop index from the linear index |
| `taichi/transforms/bit_loop_vectorize.cpp:76` | D | offset added to a GlobalPtrStmt index for the vectorized load |
| `taichi/transforms/bit_loop_vectorize.cpp:188` | D | NOT AN INDEX: dummy zero added to lhs to mark a statement bit-vectorized |
| `taichi/transforms/demote_operations.cpp:134` | D | NOT AN INDEX: zero compared against operands in the ifloordiv demotion |
| `taichi/transforms/handle_external_ptr_boundary.cpp:30` | D | clamp lower bound for every ExternalPtrStmt index |
| `taichi/transforms/handle_external_ptr_boundary.cpp:39` | D | subtracted from the axis shape to give the clamp upper bound |
| `taichi/transforms/handle_external_ptr_boundary.cpp:71` | D | clamp lower bound for a MatrixPtrStmt offset |
| `taichi/transforms/handle_external_ptr_boundary.cpp:78` | D | product of the matrix shape, clamp upper bound |
| `taichi/transforms/make_mesh_block_local.cpp:83` | D | byte stride multiplied into a BLS mapping index |
| `taichi/transforms/make_mesh_block_local.cpp:116` | D | byte stride multiplied into a BLS attribute index |
| `taichi/transforms/make_mesh_block_local.cpp:166` | D | block dimension on x64/arm64 |
| `taichi/transforms/make_mesh_block_local.cpp:204` | D | byte stride multiplied into a BLS mapping index |
| `taichi/transforms/make_mesh_block_local.cpp:268` | D | byte stride multiplied into a BLS attribute index |
| `taichi/transforms/make_mesh_block_local.cpp:304` | D | byte stride multiplied into a BLS attribute index |
| `taichi/transforms/make_mesh_block_local.cpp:327` | D | thread index on x64/arm64, stands in for LoopLinearIndexStmt |
| `taichi/transforms/make_mesh_block_local.cpp:578` | D | byte stride multiplied into a BLS mapping index |
| `taichi/transforms/lower_ast.cpp:241` | D | index offset added to a struct-for LoopIndexStmt |
| `taichi/transforms/make_block_local.cpp:145` | D | block-stride loop offset added to the thread index |
| `taichi/transforms/make_block_local.cpp:152` | D | byte stride multiplied into a BLS element index |
| `taichi/transforms/make_block_local.cpp:165` | D | BLS element count bound for the block-stride loop |
| `taichi/transforms/make_block_local.cpp:183` | D | per-axis pad extent used as a mod/div divisor |
| `taichi/transforms/make_block_local.cpp:194` | D | per-axis BLS lower bound added to a coordinate |
| `taichi/transforms/make_block_local.cpp:202` | D | per-axis block coefficient multiplied into a global coordinate |
| `taichi/transforms/make_block_local.cpp:277` | D | per-axis block coefficient multiplied into a global coordinate |
| `taichi/transforms/make_block_local.cpp:285` | D | per-axis BLS lower bound added to a coordinate |
| `taichi/transforms/make_block_local.cpp:298` | D | lower bound in the BLS bounds assertion |
| `taichi/transforms/make_block_local.cpp:303` | D | upper bound in the BLS bounds assertion |
| `taichi/transforms/make_block_local.cpp:315` | D | per-axis BLS stride multiplied into a BLS element offset |
| `taichi/transforms/make_block_local.cpp:328` | D | byte stride multiplied into a BLS element offset |
| `taichi/transforms/demote_mesh_statements.cpp:86` | D | fixed size of a high-to-low mesh relation |
| `taichi/transforms/demote_mesh_statements.cpp:94` | D | fixed size of a high-to-low mesh relation, multiplied into a relation-table index |
| `taichi/transforms/demote_mesh_statements.cpp:123` | D | one added to a relation-offset table index |
| `taichi/transforms/make_mesh_block_local.cpp:169` | D | block dimension on GPU targets |
| `taichi/transforms/frontend_type_check.cpp:72` | B | frontend gate: any field index whose ret_type is not i32 raises a cast warning at :73-77 |
| `taichi/transforms/simplify.cpp:234` | B | assert pinning every child of an offset-pushed SNode to i32 or f32, which is what makes the sizeof(int32) child stride at :239 and :261 correct |
| `taichi/ir/frontend_ir.cpp:266` | F | literal bit count 32 building the exponent member of the struct type frexp returns |
| `taichi/ir/type.h:372` | F | QuantIntType bit width defaulting to 32, with the null-compute-type resolution to i32 at type.cpp:236-240 |
| `taichi/transforms/make_mesh_thread_local.cpp:26` | H | element type of the mesh offset tables this pass builds; its byte stride is taken from this type at :27 |
| `taichi/ir/type_system.cpp:302` | E | i32 row and column arguments of insert_triplet_f32/f64, the sparse-matrix triplet internal |
| `taichi/ir/type_system.cpp:307` | E | return type of linear_thread_idx, the LLVM-spine linear thread index |
| `taichi/ir/type_system.cpp:369` | E | return type of localInvocationId |
| `taichi/ir/type_system.cpp:370` | E | return type of vkGlobalThreadIdx, the SPIR-V-spine global thread index |
| `taichi/ir/type_system.cpp:373` | E | return type of subgroupElect |
| `taichi/ir/type_system.cpp:375` | E | return type of subgroupSize |
| `taichi/ir/type_system.cpp:376` | E | return type of subgroupInvocationId |
| `taichi/ir/frontend_ir.cpp:1837` | I | `ExprGroup(Expr(i))` — a host `int` loop variable becomes an i32 `ConstExpression` and then the index of an `IndexExpression` on a tensor-typed variable, in `ASTBuilder::expand_exprs` |
| `taichi/ir/frontend_ir.cpp:1846` | I | `ExprGroup(Expr(i), Expr(j))` — the two-dimensional arm of the same expansion. **Two constructions on this line** |

### 4.1 The six that change how item 6.2 should be sized

Stated as my reading, not as fact established by the enumeration.

1. **`taichi/transforms/simplify.cpp:175` and `:178`.** Inside
   `visit(LinearizeStmt *)`, where a multi-axis SNode coordinate becomes one
   linear address. The accumulator seed and every per-level stride constant are
   i32. Report 01 section 3.6 has the host-`int` half of this loop
   (`:176` `auto stride_product = 1;`, `:187` `stride_product *= stmt->strides[i]`)
   and does not have the IR half — the `ConstStmt`s those host values are poured
   into. Both halves have to move together, or the pass emits an i32 constant
   into a wider computation.
2. **`taichi/transforms/utils.cpp:7`, `:10`, `:20`.** `generate_mod` and
   `generate_div` are, in report 01's own words, "the most-used index-arithmetic
   helper in the territory". Report 01 records that they take `int y` and that
   `:17` emits an explicit i32 constant. It does not record that the other three
   constants in those two functions are i32 by overload. The two functions hold
   four i32 constants; report 01 carries one.
3. **`taichi/ir/type_system.cpp:307` and `:370`.** The linear thread index on the
   LLVM spine and the global thread index on the SPIR-V spine, both declared i32
   in the internal-op signature table. This is a launch-index-space cap sitting
   in the type system, on both spines, and no revision of report 01 has it.
4. **`taichi/transforms/simplify.cpp:234`.** The assert that every child of an
   offset-pushed SNode is i32 or f32. It is the precondition for the four-byte
   child stride report 01 already carries. A width change that puts an i64 place
   under such an SNode aborts here, before the arithmetic is ever reached.
5. **`taichi/ir/type.h:372` with `taichi/ir/type.cpp:236-240`.** The quantised
   type system's compute type defaults to 32 bits, and says so in a source
   comment. Sparsity is required by plan section 4.1, quantised types are part
   of that machinery, and nothing in report 01 records that this default exists.
6. **`taichi/ir/frontend_ir.cpp:1837` and `:1846`.** A second narrowing entry
   point, `Expr(int32)`, reached from `ASTBuilder::expand_exprs`. These are
   frontend tensor element indices, formed before any statement IR exists, so
   they sit one layer upstream of every `MatrixPtrStmt` offset in spelling D.
   They were found only by asking the question `notes-01-ir-types.md` N065 item
   4 handed to this pass.

---

## 5. Verified versus inferred

### 5.1 VERIFIED — commands run, files opened and read

- All eight `grep` counts in section 1, each re-run at the moment of writing.
- The disjointness of spellings A and B, by `comm`.
- The identity of report 01's section 3.13 table with the spelling-A command
  output, by `diff` on sorted key lists: 64 rows, 64 hits, no difference.
- The four-line result of `get_data_type<int32>()` against report 01's stated 3.
- The follow-character census of the `TypedConstant` identifier, which is what
  produced the brace form.
- The arity split 86 / 53 / 7 and the derived call-site count of 132.
- **All 80 one-argument `TypedConstant` call sites individually opened**, and for
  each, the declaration of the argument located and read. The C++ type in
  column 3 of the section 3.4 table is a declaration I read, not an inference
  from the identifier's name. The `int` types behind
  `pad.second.pad_size[i]`, `bounds[i].low` and `coefficients[i]` come from
  `taichi/ir/scratch_pad.h:34`, `:47`, `:51`; behind `dtype_size` from
  `taichi/ir/type_utils.h:13`; behind `dynamic_index_stride` from
  `taichi/ir/statements.h:456`; behind `block_dim` from
  `taichi/ir/statements.h:1418`; behind `index_offsets` from
  `taichi/ir/snode.h:79`; behind `element_order` from `taichi/ir/mesh.h:48`.
- `uint1 = bool` at `taichi/common/core.h:136` and `int32 = int32_t` at `:146`,
  which is what makes the three `TypedConstant(true)` exclusions correct.
- `taichi/ir/type.h:582` is `explicit`, read at source.
- The absence of `type_system` from report 01 and its notes, by `grep`.
- The 102-line spelling-G enumeration and its classification, diffed against the
  command output.
- The 39-line `Expr(` enumeration, all 39 dispositioned, and the 18-line
  `ConstExpression` enumeration, all 18 dispositioned.
- `Expr::Expr(int32 x)` at `taichi/ir/expr.cpp:92-94` and its declaration at
  `taichi/ir/expr.h:30`, read at source; `explicit`, like `type.h:582`.
- The `ConstExpression(const T &x)` forwarding constructor at
  `taichi/ir/frontend_ir.h:832-835`, read at source, and its zero in-territory
  call sites.
- The zero in-territory instantiations of `value<T>`, `bit_cast<T>` and
  `expr_rand<T>`, and that the single `cast<int` hit is `snode.cpp:92`'s
  `static_cast`.

### 5.2 INFERRED — reasoning from verified code, not observed at runtime

1. **That `TypedConstant(i)` for a host `int i` selects the `int32` overload.**
   This is overload resolution on an exact match against
   `explicit TypedConstant(int32 x)` with `int32 = int32_t = int`, and the other
   integer overloads requiring a conversion. I did not compile a probe to confirm
   it. It is the premise the whole of spelling D rests on and I mark it as
   inferred rather than pretend otherwise.
2. **That `TypedConstant(size_t)` would be ambiguous rather than silently i32.**
   Offered as the explanation for why `make_block_local.cpp:157` and `:334` carry
   an explicit `(int32)` cast where sibling lines do not. Consistent with the
   code; not verified by compilation.
3. **That the seven index-bearing rows of spelling E reach codegen as i32.** The
   signature table is what `InternalFuncStmt` type-checks against; the consumers
   are in territory 02 and I did not follow them.
4. **That the closure argument in section 3.9 is exhaustive.** It rests on the
   premise that a construct pinning a Taichi 32-bit type must name that type
   within its own body, so enumerating spellings A, B, C, E and F enumerates
   every possible pinning site. I believe it and I acted on it, but it is an
   argument about C++ rather than an observation, and a construct that obtained
   the type indirectly — through a stored `DataType` member set elsewhere, for
   instance — would slip past it.
5. **The ADDR / NONADDR split in spelling G.** Each ruling is a judgement about
   what a field is for, made from its name, its declaring class and its uses in
   the same header. Forty-seven rows of judgement is forty-seven places to
   disagree, and an adversary should treat the split as the softest thing in this
   report.

### 5.3 What I did NOT do

I did not re-derive report 01's sections 3.1 through 3.12. Where a site of mine
is already in one of them, I say so and leave the row alone.

---

## 6. What this sweep does not cover, and why

1. **`int` locals and `int` parameters in `.cpp` files.** Spelling G closes
   header declarations only. `check_out_of_bound.cpp:170`'s
   `int max_valid_index`, `simplify.cpp:176`'s `stride_product` and
   `demote_dense_struct_fors.cpp:19`'s `total_shape` live there, and report 01
   sections 3.5 and 3.6 hold a partial set of them. Neither report closes it.
   "An `int` local that reaches an index" has no textual signature, so closing it
   means reading every function in the territory. **This is the largest surface
   left open by this pass.**
2. **Silent narrowing on assignment.** `TypedConstant((int32)x)` is visible;
   `int x = some_size_t;` is not. I have no filter for it and did not attempt
   one.
3. **Anything outside the four directories.** Codegen, runtime, program and the
   backends are other territories. Where a spelling in mine is consumed outside
   it, I say so and escalate rather than following it.
4. **`i8`, `i16`, `u8`, `u16` and the quantised widths.** Out of scope by the
   brief, which asks about 32-bit width. Noted because the dispatch chains I
   marked "unrelated" enumerate all of them and an adversary will see those lines
   in my tables.
5. **Runtime behaviour.** Nothing here was executed. Every claim is a claim about
   source text.
6. **Callers of the `ConstExpression(const T&)` forwarder outside my four
   directories.** Section 3.9 establishes it has none inside them. I did not
   look outside, and "out of scope by plan section 1.2" is not the same as "not
   a caller".
7. **Verification of the closure argument in section 3.9 by anything stronger
   than reading.** It is an argument about how C++ must name a type, not an
   observation. Section 5.2 item 4 marks it inferred.
8. **Whether any of these sites SHOULD change.** Standing instruction 3 forbids
   me deciding anything is unnecessary, and I have proposed no removal, no
   cleanup and no abstraction.

---

## 7. Escalations

1. **Spelling E is in my territory and its consumers are not.**
   `taichi/ir/type_system.cpp` declares i32 returns for `linear_thread_idx`,
   `localInvocationId`, `vkGlobalThreadIdx`, `subgroupElect`, `subgroupSize` and
   `subgroupInvocationId`, and i32 arguments for the sparse-matrix triplet
   internal. The file is inside `taichi/ir/`. The consumers are codegen. Whether
   territory 02 owns these or I do is a scope ruling and I have not made it.
2. **Report 01's `get_data_type<int32>()` count is 3 for a command that returns
   4.** The missing line, `taichi/ir/type.cpp:463`, is inventoried elsewhere in
   that report, so no site is lost. The figure still does not reconcile against
   the command printed above it. Whether report 01 is amended is the planner's
   call.
3. **The word "implicit" in report 01's mechanism statement is wrong as C++.**
   `type.h:582` is `explicit`. The class it describes is real and the size is
   unaffected, but the sentence as written would lead a reader to look for
   conversions that cannot happen.
4. **The eleven "macro continuation" lines report 01 sets aside do not exist as
   members of the class.** Under a bracket-balancing filter they are
   two-argument, explicitly-typed calls. This is not a correction to a count
   report 01 published — it published 78 candidates, not a site count — but it
   means the residue the planner was told about is not the residue I closed.
5. **The mesh question decides how much of this matters, again.** Of my 46 new
   spelling-D sites, 12 are in `make_block_local.cpp`, 9 in
   `make_mesh_block_local.cpp` and 3 in `demote_mesh_statements.cpp`. Report 01
   already escalated whether mesh is in scope. Twenty-four of my 46 turn on the
   same ruling, and `make_block_local.cpp` is block-local storage rather than
   mesh, so the boundary matters.
6. **Texture again.** Spelling G marks `statements.h:1700`, `:1705` and
   `frontend_ir.h:381`, `:387` NONADDR because they are texture parameters. Plan
   section 1.3 says rendering data is completely irrelevant. That is the same
   scope ruling report 01 escalated as its item 15 and I have not pre-empted it.
7. **The quantised type system has an undocumented 32-bit default.**
   `type.h:372` plus `type.cpp:236-240`. Sparsity is required by plan section
   4.1. Whether quantised types are part of the machinery this project needs is
   not something I can determine from the IR.
8. **Spelling G's ADDR / NONADDR split is judgement, not measurement.** Section
   5.2 item 4 says so. If the planner needs a defensible count of address-bearing
   `int` members rather than my reading of one, the split needs a second opinion.
9. **`notes-01-ir-types.md` N065 item 4 is answered, and the answer is yes.**
   A second narrowing entry point exists, `Expr(int32)`, with two call sites in
   this territory, and a third exists with none. The list of narrowing entry
   points is now closed at three by the argument in section 3.9, and that
   argument is marked inferred in section 5.2 item 4. If the planner wants it
   verified rather than argued, the way to do it is a compiler-assisted pass,
   not another grep.
10. **The `ConstExpression(const T&)` forwarder has no callers in this
    territory but is not dead.** `taichi/ir/frontend_ir.h:833`. Its users, if
    any, are outside my four directories. The Python front end is out of scope
    by plan section 1.2, but "out of scope" is not the same as "not a caller",
    and I did not look.
11. **My own arithmetic failed once during this pass**, at notes entry N16, where
   I wrote "twelve lines" over an eleven-row table. It is corrected at N19 and
   the corrected figure is what section 3.6 carries. Recorded here because the
   plan asks for these to be visible rather than tidied away.
