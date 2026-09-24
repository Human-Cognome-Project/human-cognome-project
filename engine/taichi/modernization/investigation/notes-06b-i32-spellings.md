# Notes — agent 06B — enumerating implicit i32 spellings in the IR territory

Contemporaneous. Numbered entries, written as I work. Territory:
`taichi/ir/`, `taichi/analysis/`, `taichi/transforms/`, `taichi/inc/`.
Tree at `ba0e81dce559fb63a5958bf82feb1d00c55c02fe`.

All commands are run from `/opt/project/taichi`.

---

## E001 — Starting point, and the one correction I make to it before starting

Read as instructed: `notes-01-ir-types.md` N055–N062, and
`report-01-ir-types.md` section 3.13.

The mechanism as stated there: `TypedConstant` has an "implicit `int32`
constructor" at `taichi/ir/type.h:582`, so an i32 constant can be built with no
`i32` token on the line.

**Correction to the wording, which matters for how the enumeration is built.**
The constructor is `explicit`:

```
taichi/ir/type.h:582:  explicit TypedConstant(int32 x) : dt(PrimitiveType::i32), val_i32(x) {
```

Verified against the full constructor set, `grep -n "TypedConstant(" taichi/ir/type.h`:

| Line | Signature |
|---|---|
| 572 | `TypedConstant()` |
| 575 | `explicit TypedConstant(DataType dt)` |
| 582 | `explicit TypedConstant(int32 x)` |
| 585 | `explicit TypedConstant(float32 x)` |
| 588 | `explicit TypedConstant(int64 x)` |
| 591 | `explicit TypedConstant(float64 x)` |
| 594 | `explicit TypedConstant(int8 x)` |
| 597 | `explicit TypedConstant(int16 x)` |
| 600 | `explicit TypedConstant(uint1 x)` |
| 603 | `explicit TypedConstant(uint8 x)` |
| 606 | `explicit TypedConstant(uint16 x)` |
| 609 | `explicit TypedConstant(uint32 x)` |
| 612 | `explicit TypedConstant(uint64 x)` |
| 616 | `template <typename T> TypedConstant(DataType dt, const T &value)` — two-arg |

Two consequences, both load-bearing for this sweep:

1. Because it is `explicit`, the token `TypedConstant(` **must** appear at every
   such construction. There is no silent conversion from a bare `int` into a
   `TypedConstant` parameter. So `grep -rn "TypedConstant("` really does bound
   this spelling's occurrences, which is what makes closing it possible at all.
   "Implicit" is the right word for the *width*, not for the *conversion*.
2. Which overload is selected is decided by the **static C++ type of the single
   argument**, not by anything on the line. `TypedConstant(some_int)` is i32.
   `TypedConstant(some_int64)` is i64. `TypedConstant(some_size_t)` is u64.
   Classification therefore means resolving the declared type of the argument
   expression at each site. That is the work, and it cannot be done by grep.

This also means the fourth spelling and the fifth (plain C++ `int` flowing into
an IR width) are **the same mechanism seen from two ends**: the host `int` is
what selects the i32 overload.

---

## E002 — Spelling census. Six, not four, and one of the four was mis-scoped

I did not assume four. I looked for every syntactic route by which a
32-bit-wide integer type or value can be attached to an IR object in this
territory, then ran each to exhaustion.

Method for finding them, stated so it can be attacked:

1. Read the full constructor set of `TypedConstant` (`taichi/ir/type.h:560-660`)
   and of `Expr` (`taichi/ir/expr.h:21-53`, `taichi/ir/expr.cpp:80-110`) — the
   two value-carrying types that select a width from a C++ argument type.
2. Read the full public surface of `TypeFactory` (`taichi/ir/type_factory.h`)
   and of `IRBuilder` (`taichi/ir/ir_builder.h`), looking for anything that
   yields a type or a constant without the caller naming a width.
3. `grep -rn "i32"` over the territory and eliminated by inspection every match
   already covered, to see what token forms remained.

Result — six spellings:

| # | Spelling | Route to i32 |
|---|---|---|
| S1 | `PrimitiveType::i32` | the named singleton, `taichi/ir/type.h` via `taichi/inc/data_type.inc.h` |
| S2 | `PrimitiveTypeID::i32` | the enumerator, fed to `TypeFactory::get_primitive_type` or tested with `Type::is_primitive` |
| S3 | `get_data_type<int32>()` | C++-type-to-Taichi-type template, `taichi/ir/type_utils.h:35-60` |
| S4 | `TypedConstant((int32)x)` / `int32(x)` | explicit cast at the call, selecting the `int32` overload |
| S5 | `TypedConstant(<host int expr>)` | overload selected by the argument's static C++ type. **No i32 token on the line.** |
| S6 | `IRBuilder::get_int32(...)` | a named 32-bit factory; the width is inside the callee |

And two width-bearing routes that are not `i32` *spellings* but are the same
class of assumption, kept separate and enumerated separately:

| # | Route | Why separate |
|---|---|---|
| S7 | a literal bit count into `TypeFactory::get_primitive_int_type(<n>)` | the width is a number, not a type name |
| S8 | plain C++ `int` declaring a field or local that carries an IR index/extent | never becomes a `Type` at all; it is where the host truncates |

**S2 was mis-scoped in the prior work.** Report 01 section 3.13 names four
spellings and `PrimitiveTypeID::i32` is not among them. It is a distinct token
— `grep "PrimitiveType::i32"` does not match `PrimitiveTypeID::i32`, verified
below — and it has 27 occurrences in this territory. Report 01 cites exactly one
of them (`value_diff.cpp:82`, its section 3.10).

**S6 was seen but not named as a spelling.** Report 01's closing pass added
`ir_builder.cpp:125` (`get_int32(0)`) to its section 3.3, so the site is
inventoried; the *spelling* is not, and the sweep that would find its other
callers was never stated.

## E003 — S1: `PrimitiveType::i32`. 64 lines. Reproduced, unchanged.

```
grep -rn "PrimitiveType::i32" taichi/ir/ taichi/analysis/ taichi/transforms/
```
→ 64. Same command over the four-directory territory including `taichi/inc/`:
also 64, so `taichi/inc/` contributes none.

Report 01 section 3.13 dispositions all 64. I re-ran the count only; I did not
re-adjudicate its 64 dispositions, because that is its enumeration and this is
not a re-audit of it. **Not covered by me** — stated in the report's coverage
section.

Note the token distinction that makes S2 a separate set:
```
grep -rn "PrimitiveType::i32" <territory> | grep -c "PrimitiveTypeID::i32"   -> 0
grep -rn "PrimitiveTypeID::i32" <territory> | grep -c "PrimitiveType::i32"   -> 0
```
Neither grep sees the other's lines. Verified, both zero. So S1 and S2 are
disjoint sets, and S1's 64 do not contain any of S2's 27.

## E004 — S3: `get_data_type<int32>()`. The stated command returns 4, not 3.

```
grep -rn "get_data_type<int32>()" taichi/ir/ taichi/analysis/ taichi/transforms/
taichi/ir/type.cpp:463:  TI_ASSERT(get_data_type<int32>() == dt);
taichi/transforms/scalarize.cpp:63:            TypedConstant(get_data_type<int32>(), i));
taichi/transforms/scalarize.cpp:114:            TypedConstant(get_data_type<int32>(), i));
taichi/transforms/scalarize.cpp:530:            TypedConstant(get_data_type<int32>(), i));
```
Four lines. Report 01 section 3.13's third sweep row states **3** against the
same command and the same territory. The fourth, `taichi/ir/type.cpp:463`, is
inside `TypedConstant::val_int32()` and is the assert that the report's own
prose names one sentence later — it calls it a hit "tree-wide" when it is a hit
*in territory*. Benign as a finding, since the site is an assert and not a
width decision; recorded because it is a stated count that its own command
contradicts, which is the failure mode standing instruction 7 names.

Disposition, mine:
- `type.cpp:463` — excluded. Guard inside the accessor, asserts the tag matches
  before reading `val_i32`. Not a place a width is chosen.
- `scalarize.cpp:63`, `:114`, `:530` — real. Matrix element indices. Already in
  report 01 section 3.3.

## E005 — S4: `TypedConstant((int32)…)` / `int32(`. 37 lines. Reproduced.

```
grep -rn "TypedConstant((int32)\|TypedConstant{(int32)\|int32(" taichi/ir/ taichi/analysis/ taichi/transforms/
```
→ 37. Count confirmed.

Of the 139 `TypedConstant(` lines in territory (E006), exactly 10 carry an
explicit `(int32)` or `int32(` cast, and those 10 are the intersection of S4
with S5's candidate set. The other 27 of S4's 37 are `val_int32()` accessor
calls and `int32(` narrowing casts that never reach a `TypedConstant`; report
01 places them in its sections 3.2 and 3.8. **I did not re-adjudicate those 27.**

## E006 — S5: the open class. I closed it by enumerating all 139, not 78.

I did not reproduce report 01's 78-line filter. That filter is defined by
subtraction — "minus every call whose first argument is a type", with the type
forms listed by hand — so its membership depends on a hand-written list of
spellings for the thing being subtracted, which is the same failure the whole
exercise is about. I enumerated the superset instead and classified every row.

```
grep -rn "TypedConstant(" taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/
```
→ **139 lines**, in 26 files. `taichi/inc/` contributes none.

Because the constructor is `explicit` (E001), those 139 lines contain every
`TypedConstant` construction in the territory. Nothing can be built without the
token.

**Classification rule.** For each line I resolved, from source, either the arity
of the call or the declared C++ type of its single argument:

| Class | Definition | Count |
|---|---|---|
| `DECL` | the constructor declarations in `taichi/ir/type.h` themselves | 14 |
| `TYPED2` | two-argument form `TypedConstant(<type expr>, value)`; the width comes from the type argument, not from this line | 52 |
| `TYPED1DT` | one-argument form whose argument is a `DataType`, selecting `TypedConstant(DataType)` at `type.h:575` | 2 |
| `CASTED` | one-argument with an explicit `(int32)` / `int32(` cast — i32, but that is spelling S4, already inventoried | 10 |
| `U1` | one-argument, argument `bool`; `taichi/common/core.h:136` is `using uint1 = bool`, so this is an **exact** match on `TypedConstant(uint1)` and yields `PrimitiveType::u1`, not i32 | 3 |
| `F32` | one-argument, argument declared `float32`; yields f32 | 2 |
| `I32` | one-argument, argument's static type is `int`/`int32`; yields `PrimitiveType::i32` | **56** |

14+52+2+10+3+2+56 = 139. Checked mechanically: the classification file has 139
rows, 139 distinct `file:line` keys, and `diff` against the raw grep output is
empty.

**The `U1` three matter.** `check_out_of_bound.cpp:48`, `:107`, `:180` are
`TypedConstant(true)`. A hand filter for "first argument is not a type" keeps
them, and they read exactly like the i32 rows around them. They are not i32.
`bool` is an exact match for the `uint1` overload and exact match beats the
integral promotion `bool`→`int`. Anyone closing this class by eyeballing the
78-line residue would have counted them.

**The `F32` two matter for the same reason.** `auto_diff.cpp:957`, `:963` are
`TypedConstant(x)` inside `ADTransform::constant(float32 x, …)` at
`auto_diff.cpp:953`. The line is indistinguishable from an i32 row; the
parameter declaration eight lines up is what decides it.

## E007 — S5 classification, resolved argument by argument

The 56 `I32` rows were not assigned by pattern. For each I opened the argument's
declaration. The ones that are not a bare `0`/`1` literal:

| Site | Argument | Declared where | Type |
|---|---|---|---|
| `frontend_ir.cpp:663` | `offsets[i]` | `frontend_ir.cpp:658` `const std::vector<int> &offsets` | `int` |
| `frontend_ir.cpp:735` | `shape[i]` | `frontend_ir.cpp:722` `const std::vector<int> &shape` | `int` |
| `frontend_ir.cpp:747` | `offset` | `frontend_ir.cpp:742` `int offset = 0;` | `int` |
| `utils.cpp:7`, `:10`, `:20` | `y - 1`, `y` | `utils.cpp:5`, `:14` `int y` | `int` |
| `lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477` | `i` | loop `for (int i = …)` | `int` |
| `lower_matrix_ptr.cpp:537` | `origin->dynamic_index_stride` | `statements.h:456` `int dynamic_index_stride{0}` | `int` |
| `check_out_of_bound.cpp:74` | `flattened_element` | `check_out_of_bound.cpp:54` `int flattened_element = 1;` | `int` |
| `check_out_of_bound.cpp:126` | `upper_bound_i` | `:124` `int upper_bound_i = size_i;` | `int` |
| `check_out_of_bound.cpp:142` | `offset_i` | `:115` `int offset_i = …` | `int` |
| `check_out_of_bound.cpp:186` | `max_valid_index` | `:169` `int max_valid_index = 1;` | `int` |
| `simplify.cpp:178` | `stride_product` | `:176` `auto stride_product = 1;` — deduced `int` | `int` |
| `demote_dense_struct_fors.cpp:79` | `total_shape[p]` | `:19` `std::array<int, taichi_max_num_indices> total_shape;` | `int` |
| `bit_loop_vectorize.cpp:76` | `vectorization_width` | `= data_type_bits(...)`, `type_utils.h:19` returns `int` | `int` |
| `handle_external_ptr_boundary.cpp:78` | `max_valid_index` | `:62` `int max_valid_index = 1;` | `int` |
| `lower_ast.cpp:241` | `offsets[i]` | `lower_ast.cpp:208` `std::vector<int> offsets;` | `int` |
| `make_mesh_block_local.cpp:83`, `:204`, `:578` | `mapping_dtype_size_` | `make_mesh_block_local.h:69` `int mapping_dtype_size_{0}` | `int` |
| `make_mesh_block_local.cpp:116`, `:268`, `:304` | `dtype_size` | `= data_type_size(...)`, `type_utils.h:13` returns `int` | `int` |
| `make_block_local.cpp:145` | `loop_offset` | `:140` `int loop_offset = 0;` | `int` |
| `make_block_local.cpp:152`, `:328` | `dtype_size` | `:62` `= data_type_size(...)` | `int` |
| `make_block_local.cpp:165` | `bls_num_elements` | `:76` `= pad.second.pad_size_linear()`, `scratch_pad.h:131` returns `int` | `int` |
| `make_block_local.cpp:183` | `pad.second.pad_size[i]` | `scratch_pad.h:51` `std::vector<int> pad_size` | `int` |
| `make_block_local.cpp:194`, `:285` | `pad.second.bounds[i].low` | `scratch_pad.h:34` `int low{0}` | `int` |
| `make_block_local.cpp:202`, `:277` | `pad.second.coefficients[i]` | `scratch_pad.h:47` `std::vector<int> coefficients` | `int` |
| `make_block_local.cpp:303` | `bls_axis_size` | `:290-291` `= pad.second.bounds[i].high - pad.second.bounds[i].low` | `int` |
| `make_block_local.cpp:315` | `bls_strides[i]` | `:79` `std::vector<int> bls_strides(dim)` | `int` |
| `demote_operations.cpp:99` | `i` | loop `for (int i = …)` | `int` |

Every remaining `I32` row is a literal `0` or `1`, which is `int` by the
standard and therefore an exact match on `TypedConstant(int32)`.

## E008 — Status of the 56 against report 01, computed not eyeballed

I extracted every `file:line` citation in `report-01-ir-types.md` — full paths
and the report's back-reference shorthand (`` `:1160` `` attributed to the last
full path seen) — and split range citations from exact ones. Script and both
sets are reproducible; the counts below are its output, not a hand tally.

| Status | Meaning | Count |
|---|---|---|
| A | the exact line is named in report 01 | 9 |
| B | the line falls inside a line-range report 01 cites, but is not itself named | 7 |
| C | the line appears nowhere in report 01 | **40** |

9 + 7 + 40 = 56, asserted in the script.

**A (9):** `frontend_ir.cpp:732`, `:735`; `lower_matrix_ptr.cpp:189`, `:232`,
`:424`, `:477`; `lower_ast.cpp:270`, `:271`; `demote_operations.cpp:99` — and
the last of those report 01 states explicitly is **not** inventoried, it is
quoted as a worked example of the class it left open.

**B (7):** `frontend_ir.cpp:747` (inside `:742-747`, filed as a *host* `int`
product in 3.6, not as the IR constant it becomes at `:747`);
`lower_matrix_ptr.cpp:537` (inside `:536-540`); `check_out_of_bound.cpp:126`
(inside `:123-126`), `:217` (inside `:205-230` in the notes and `:218-219` in
3.13's `:220` row); `simplify.cpp:175`, `:178`, `:193` (inside `:160-220` and
`:189-205`, cited for what the pass does, and `:176`/`:187` cited in 3.6 as the
*host* accumulator — the IR constant at `:178` is a different object from the
host `int` at `:176`).

**C (40):** listed in the report.

## E009 — S2: `PrimitiveTypeID::i32`. 27 lines, all dispositioned.

```
grep -rn "PrimitiveTypeID::i32" taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/
```
→ 27. Disjoint from S1's 64, verified both ways (E003).

Classification rule: a line is an **exhaustive-dispatch arm** if it is one arm
of an if/else chain that has an arm for every primitive type in
`taichi/inc/data_type.inc.h`, or one disjunct of a set predicate over all of
them. Such a line encodes no assumption; deleting the i32 arm would break i32
and nothing else. Everything not an exhaustive arm I opened individually.

| Class | Count |
|---|---|
| exhaustive-dispatch arm | 18 |
| width→type mapping, correct by construction | 2 |
| definition of a 32-bit factory (S6) | 1 |
| already inventoried by report 01 | 4 |
| **real, and not in report 01** | **2** |

18+2+1+4+2 = 27.

The two: `taichi/ir/type.cpp:238` and `taichi/transforms/simplify.cpp:234`.
Both written up in the report.

## E010 — S6: `IRBuilder::get_int32`. Closed, and empty apart from one call.

```
grep -rn "get_int32\|get_uint32" taichi/ --include=*.cpp --include=*.h
```
Six hits, all in `taichi/ir/ir_builder.h` and `taichi/ir/ir_builder.cpp`.
Declarations at `ir_builder.h:133-136`, definitions at `ir_builder.cpp:141-163`,
and **one call in the whole of `taichi/`**: `ir_builder.cpp:125`, inside
`IRBuilder::create_break()`. Report 01 already inventoried that site in its
section 3.3, from the S1 sweep. Nothing further falls out of naming S6 as a
spelling; I record it because the enumeration has to show it was run.

## E011 — S7: literal bit widths into the width-taking factories

Membership rule: a call, in territory, to `TypeFactory::get_primitive_int_type`
or `get_primitive_real_type` — the only two APIs that take a width as a number
— whose width argument is a literal rather than a parameter.

```
grep -rn "get_primitive_int_type\|get_primitive_real_type" taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/
```
Six hits. Three are the declarations and definitions in `type_factory.{h,cpp}`.
The other three are calls:

| Call | Width argument | Verdict |
|---|---|---|
| `taichi/ir/type_utils.h:205` | `max_num_bits`, the `BitStructTypeBuilder` constructor parameter | derived, not a site |
| `taichi/ir/snode.cpp:150` | `bits`, from the caller | derived, not a site |
| `taichi/ir/frontend_ir.cpp:265-266` | **literal `32`** | **real site, not in report 01** |

Also swept, same class, hardcoded byte widths:
```
grep -rn "sizeof(int32)\|sizeof(int)\|sizeof(i32)" taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/
```
→ 5 lines. `simplify.cpp:239`, `:261` are report 01's section 3.7.
`scalarize.cpp:1113`, `:1140` are inside `/* … */` documentation comments
(verified by reading `:1105-1145`) and are excluded as comments.
`taichi/ir/statements.h:1787` is real and **not in report 01**.

## E012 — CORRECTION to E009. The S2 breakdown I wrote there does not add up to its own rows.

I wrote 18 / 2 / 1 / 4 / 2 in E009 before building the row-by-row table. When I
built it, the classes came out differently. The E009 figures are wrong and this
entry supersedes them. Recorded rather than edited, because the point of these
notes is the sequence.

Final S2 table, one row per grep hit, 27 rows:

| # | Line | Class |
|---|---|---|
| 1 | `taichi/ir/type_utils.h:71` | MAP — `get_primitive_data_type<T>()`, the `std::is_same<T,int32>` arm |
| 2 | `taichi/ir/type_utils.h:106` | EXH — `is_integral` |
| 3 | `taichi/ir/type_utils.h:122` | EXH — `is_signed` |
| 4 | `taichi/ir/type_utils.h:137` | EXH — `to_unsigned` |
| 5 | `taichi/ir/type_utils.h:150` | EXH — `get_max_value` |
| 6 | `taichi/ir/type_utils.h:178` | EXH — `get_min_value` |
| 7 | `taichi/ir/type_factory.cpp:167` | MAP — `get_primitive_int_type`, the `bits == 32` arm |
| 8 | `taichi/ir/type_utils.cpp:126` | EXH — printf format table |
| 9 | `taichi/ir/type.cpp:238` | **NEW** |
| 10 | `taichi/ir/type.cpp:400` | EXH — `TypedConstant::stringify` |
| 11 | `taichi/ir/type.cpp:434` | EXH — `equal_type_and_value` |
| 12 | `taichi/ir/type.cpp:524` | EXH — `val_int` |
| 13 | `taichi/ir/type.h:621` | EXH — the two-argument template constructor |
| 14 | `taichi/ir/ir_builder.cpp:143` | S6DEF — body of `IRBuilder::get_int32` |
| 15 | `taichi/analysis/value_diff.cpp:82` | INV — report 01 section 3.10, `:81-87` |
| 16 | `taichi/analysis/value_diff.cpp:143` | INV — report 01 section 3.10, `:142-146` |
| 17 | `taichi/transforms/type_check.cpp:148` | GUARD — the `if` for the cast at `:154`; report 01 excludes the twin guard `:459` by exactly this rule |
| 18 | `taichi/transforms/type_check.cpp:161` | INV — report 01 section 3.2, `:160-161` |
| 19 | `taichi/transforms/frontend_type_check.cpp:72` | INV — report 01 cites `:71-77` |
| 20 | `taichi/transforms/constant_fold.cpp:37` | INV — report 01 cites `:33-47` |
| 21 | `taichi/transforms/constant_fold.cpp:77` | EXH — binary macro, i32/i64 arm |
| 22 | `taichi/transforms/constant_fold.cpp:113` | INV — report 01 section 3.2 and its N061 |
| 23 | `taichi/transforms/constant_fold.cpp:204` | EXH — unary macro |
| 24 | `taichi/transforms/constant_fold.cpp:233` | EXH — unary macro |
| 25 | `taichi/transforms/constant_fold.cpp:253` | EXH — cast folding |
| 26 | `taichi/transforms/simplify.cpp:234` | **NEW** |
| 27 | `taichi/transforms/bit_loop_vectorize.cpp:321` | INV — report 01 cites `:311-325` |

MAP 2, EXH 14, S6DEF 1, INV 7, GUARD 1, NEW 2. 2+14+1+7+1+2 = 27.

## E013 — S8: plain C++ `int`. Scoped honestly, because it cannot be closed by grep.

The brief asks for "plain C++ `int` where it flows into an IR type or an index".
The literal class is not enumerable: `int` appears many thousands of times in
this territory and most occurrences are loop counters, ids and thread counts.
Reporting a number over it would be a quantifier no citation can test, which is
standing instruction 6.

What IS closable, and what I closed:

**(a) The host-`int`-to-IR-i32 crossings.** These are exactly S5's 56 `I32`
rows. That is the complete set of points in this territory where a host `int`
becomes an `i32` IR constant, because `TypedConstant` is `explicit` and those
are all its single-`int`-argument constructions. Already enumerated.

**(b) `int`-typed data members and header-inline locals.** Membership rule: a
line in a territory header declaring `int`, `int32`, `std::vector<int>`,
`std::vector<int32>` or `std::array<int,…>` followed by a name.

```
grep -rnE '^[[:space:]]+(int|int32|std::vector<int>|std::vector<int32>|std::array<int,[^>]*>)[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*(\{|=|;|\[)' \
  taichi/ir/*.h taichi/analysis/*.h taichi/transforms/*.h taichi/inc/*.h
```
→ **109 lines.** I opened each and split them:

- **40 carry a width-bearing quantity** — an index, extent, offset, stride,
  element count, byte size or bit width that scales with the problem.
- **69 do not** — thread and block counts (`num_cpu_threads`, `block_dim`,
  `grid_dim`), object ids (`Stmt::id`, `SNode::id`, `Identifier::id`,
  `snode_tree_id_`, `id_in_bit_struct`, `instance_id`, `id_counter_`), axis
  numbers bounded by `taichi_max_num_indices = 12` (`Axis::value`,
  `num_active_indices`, `LoopIndexStmt::index`, `BlockCornerIndexStmt::index`,
  `axis`), argument ids and depths, struct-member index paths
  (`GetElementStmt::index`, `GetElementExpression::index`), texture parameters
  (`num_dims`, `dimensions`, `lod`), bit-struct member bookkeeping bounded by
  the physical type's own width, insertion positions, `addr_space_`,
  `path_length_`, `final_node`, `unique_accessed`, `depth`, and the
  `int32 val_i32` union member at `type.h:558`, which is the mechanism itself.

40 + 69 = 109.

Of the 40, report 01 sections 3.5 and 3.6 already carry 33. **Seven are not in
report 01 at any line:**

| Site | Field | Why it is width-bearing |
|---|---|---|
| `taichi/ir/type.h:250` | `TensorType::shape_` — `std::vector<int>` | the tensor's own shape; `get_num_elements()` at `:222-227`, which report 01 does carry, is the product **of these** |
| `taichi/ir/type.h:372` | `QuantIntType::num_bits_{32}` | **the default is 32**, and the comment at `:369-370` says so: *"for now we can uniformly use i32 as the 'compute_type'"* |
| `taichi/ir/type.h:545` | `QuantArrayType::num_elements_` — `int` | element count of a quant array |
| `taichi/ir/statements.h:1063` | `StructForStmt::index_offsets` — `std::vector<int>` | report 01 carries the `OffloadedStmt` twin at `:1439` and not this one; it is the field written at `lower_ast.cpp:234` and read at `:239-241` |
| `taichi/ir/mesh.h:77` | `MeshLocalRelation::num_patches{0}` — `int` | mesh patch count |
| `taichi/transforms/make_mesh_block_local.h:69` | `mapping_dtype_size_{0}` — `int` | the byte size fed to `TypedConstant` at `make_mesh_block_local.cpp:83`, `:204`, `:578` |
| `taichi/ir/scratch_pad.h:232` | `int offset = 0` in `generate_address_code` | host address product — **but see below** |

`scratch_pad.h:232` is qualified, not asserted. `generate_address_code` is
declared at `scratch_pad.h:229` and, by
`grep -rn "generate_address_code" taichi/`, is **called from nowhere in
`taichi/`** — one hit, the definition. Its `offset` is also never read after the
loop that computes it. Dead on two counts. Listed so it is not silently
dropped, and labelled.

## E014 — What this sweep does not cover. Stated before writing the report.

1. **The 64 S1 lines are not re-adjudicated.** I reproduced the count and the
   disjointness from S2. Report 01 section 3.13's dispositions of those 64 are
   its work, not mine, and I did not re-open them.
2. **27 of S4's 37 lines are not re-adjudicated** — the `val_int32()` calls and
   `int32(` narrowings that never reach a `TypedConstant`. Report 01 sections
   3.2 and 3.8 hold them. I checked only that 10 of the 37 are `TypedConstant`
   lines and reconcile with S5.
3. **Territory only.** `taichi/codegen/`, `taichi/runtime/`, `taichi/program/`,
   `taichi/rhi/`, `taichi/python/` are out. Where a width crosses the seam I say
   so and stop.
4. **`taichi/inc/` contributes nothing to S1–S6.** Verified: all four
   `TypedConstant`/`PrimitiveType`/`PrimitiveTypeID` sweeps return the same
   counts with and without it. Its width content is the `constexpr int`
   constants in `constants.h`, which are section 6.1's territory, not 6.2's.
5. **No reachability claim.** I classify what a line means, not whether a
   particular pass runs. The one exception is `scratch_pad.h:232`, where the
   callee has no callers at all and I say so.
6. **`u1` and `f32` overload selection is by declared C++ type.** I resolved
   those from source. I did not compile the tree to confirm overload resolution;
   the argument is the standard's, from the declarations quoted.
7. **S8 is bounded to headers.** `int` locals inside `.cpp` files that carry
   index arithmetic are not enumerated. Report 01 section 3.6 has nine such
   rows; I neither extended nor closed that list, except where an S5 argument
   forced me to open the declaration (E007).

## E015 — CORRECTION to E013. The S8(b) regex leaks, and I found the leak by testing it.

After writing E013 I checked `mesh.h:77` to name the enclosing class and found
`MeshMapping<int> num_elements{}` on the very next line — an `int`-parameterised
member my regex does not match, sitting immediately beside one it does. So the
109 was a count of a regex, not of a class. Two supplementary sweeps:

```
grep -rnE '^[[:space:]]+[A-Za-z_][A-Za-z0-9_:]*<[^>]*\bint\b[^>]*>[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*(\{|=|;)' \
  taichi/ir/*.h taichi/analysis/*.h taichi/transforms/*.h taichi/inc/*.h \
  | grep -vE 'std::vector<int>|std::vector<int32>|std::array<int,'
```
→ 2: `taichi/ir/mesh.h:78` (`MeshMapping<int> num_elements{}`, width-bearing)
and `taichi/ir/statements.h:757` (`std::unordered_set<int> covers`, SNode ids,
not width-bearing).

```
grep -rnE '^[[:space:]]+(int|int32|std::vector<int>|MeshMapping<int>|std::array<int,[^>]*>|[A-Za-z_][A-Za-z0-9_:]*<[^>]*\bint\b[^>]*>)$' \
  taichi/ir/*.h taichi/analysis/*.h taichi/transforms/*.h taichi/inc/*.h
```
→ 1: `taichi/ir/mesh.h:79`, the type of `patch_max_element_num` declared on
`:80`. The only member in the territory's headers whose type and name are on
different lines. Width-bearing.

**Corrected S8(b) figures: 112 declarations, 42 width-bearing, 70 not.**
109 + 2 + 1 = 112; 40 + 2 = 42; 69 + 1 = 70; 42 + 70 = 112.

Report 01 sections 3.5 and 3.6 carry 33 of the 42, enumerated in the report.
**Nine are new**, the seven of E013 plus `mesh.h:78` and `mesh.h:79-80`.
33 + 9 = 42.

Also corrected from E013: `mesh.h:77` is `Mesh::num_patches`, not
`MeshLocalRelation::num_patches`. `MeshLocalRelation` ends at `mesh.h:68`;
`class Mesh` opens at `:70`.

I am leaving the leak visible rather than presenting the corrected regex as if
it had been the first one. The lesson is the same one the whole task is about:
a set defined by a pattern is only as closed as the pattern, and the only way to
know is to open something and check what sits next to it.

## E016 — Verification pass over the finished report

Every number in `report-06b-i32-spellings.md` re-derived from its own list, per
standing instruction 7.

| Claim | Check | Result |
|---|---|---|
| S1 = 64, S2 = 27, S3 = 4, S4 = 37, S5 = 139 | re-ran each grep | all match |
| `taichi/inc/` contributes nothing | each grep run with and without it | identical counts, all five |
| S1 and S2 disjoint | each grep piped through `grep -c` for the other | 0 and 0 |
| S5 partition 14+52+2+10+3+2+56 | summed the classification file's class column | 139 |
| classification covers the grep exactly | `diff` of key column against raw grep | empty |
| A+B+C = 56 | asserted inside the extraction script | passes |
| section 4 table has 40 rows | `awk` row count over the table | 40 |
| per-file counts over C | `uniq -c` on the C list | 12+8+6+4+3+2+2+1+1+1 = 40 |
| S2 partition 2+14+1+7+1+2 | counted from the E012 table's class column | 27 |
| S8(b) 42 + 70 | counted from the three sweeps | 112 |
| the "33 already carried" list | counted the backticked citations in that paragraph | 33 |

**Three errors this pass caught and fixed in the report:**

1. `make_mesh_block_local.cpp` contributes **8** of the forty, not 7, so the two
   block-local files carry **20**, exactly half, not 19 and not "more than
   half". Three sentences corrected. This is the fifth or sixth instance in this
   project of a stated total disagreeing with its own list, and it was mine.
2. The quoted `frexp` block was numbered `262-266`; the lines are `263-267`.
   The citation `frontend_ir.cpp:265-266` in the prose was right; only the quote
   block was shifted.
3. `demote_operations.cpp:134` was described as a "division-by-zero guard". It
   is not. It is the zero of `demote_ifloordiv`'s sign correction, compared
   against both operands at `:147-151` and `:155-156` and broadcast into a
   `MatrixInitStmt` at `:138-141` for tensor operands. Reworded against source.

Line-range citations spot-checked against source and correct:
`check_out_of_bound.cpp` `:60-61`, `:77-78`, `:143-144`, `:187-188`;
`utils.cpp:5`, `:8`, `:11`, `:14`, `:21`;
`make_block_local.cpp:163-165`;
`handle_external_ptr_boundary.cpp:40-41`;
`lower_ast.cpp:242-243`;
`bit_loop_vectorize.cpp:79-80`;
`type.cpp:232-241`; `simplify.cpp:232-239`; `statements.h:1770`, `:1778`,
`:1782`, `:1786-1788`; `type.h:369-370`, `:372`.

No source file was modified. No other agent's file was modified. The only files
I opened under `modernization/investigation/` are the two named as inputs and
the two I wrote.

---

# Handover pass — entries E017 onwards

## E017 — What arrived, and what of it was already done

The planner forwarded detail from the agent that discovered the class, after
that agent finished. I read `notes-01-ir-types.md` N065, which I had not seen;
my inputs were N055–N062 and report section 3.13. N065 states the mechanism and
hands over five points.

Already covered by E001–E016, and not redone:

- **N065 point 1**, three spellings closed, check rather than re-run. Done in
  E003–E005; I re-ran the counts only and found one of them, `get_data_type<int32>()`,
  stated as 3 against a command returning 4.
- **N065 point 2 and 3**, the 78-candidate filter and its over-collection. E006
  bypassed the filter entirely by enumerating the 139-line superset. Resolved
  mechanically in E019 below.
- **N065 point 5**, `demote_operations.cpp:98-101` as the shape to look for. It
  is row `demote_operations.cpp:99` in my S5 table, status A, and the planner
  confirms it appears in no inventory table in report 01. Its sibling
  `demote_operations.cpp:134`, in the same file and the same class, is one of my
  forty.

**Not covered, and the reason this pass exists: N065 point 4.** Whether other
constructors in the territory have width-selecting overloads of the same kind.
I did not ask that question. E018 asks it.

**Boundary confirmation from the planner, folded in.** Of the 78, five were
accounted for: the four `lower_matrix_ptr.cpp` sites, added on instruction, and
`demote_operations.cpp:99`, cited as evidence and inventoried nowhere. The other
73 were unopened and unsampled. That is consistent with what I computed
independently in E008: of my 56, exactly 9 are named exactly in report 01, and
those 9 are the four `lower_matrix_ptr.cpp` sites, `demote_operations.cpp:99`,
and four more (`frontend_ir.cpp:732`, `:735`, `lower_ast.cpp:270`, `:271`) that
report 01 names in its section 3.12 for the *products they build* rather than as
constants of this class. So the planner's five and my nine agree once that
distinction is made, and neither number contradicts the other.

## E018 — N065 point 4 answered. A second type has it. `Expr`.

Question: does any other type in this territory select a 32-bit width from the
C++ type of a constructor argument, the way `TypedConstant` does?

Method — three sweeps, each stated so the answer can be attacked:

**(a) Every constructor-shaped declaration taking a bare 32-bit integer.**
```
grep -rnE '\b[A-Z][A-Za-z0-9_]*\((const )?(int32|uint32)( |&| &)[a-zA-Z_]+\)' \
  taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/ --include=*.h --include=*.cpp
```
→ 4 lines, naming **two** types: `Expr` (`expr.h:30`, `expr.cpp:92`) and
`TypedConstant` (`type.h:582`, `:609`).

**(b) Every constructor-shaped declaration taking a bare 64-bit integer**, to
find the *sibling* that makes an overload set width-selecting rather than merely
integer-taking.
```
grep -rnE '\b[A-Z][A-Za-z0-9_]*\((const )?(int64|uint64|long|std::size_t|size_t)( |&| &)[a-zA-Z_]+\)' <territory> --include=*.h --include=*.cpp
```
→ 4 lines, the same two types: `Expr` (`expr.h:32`, `expr.cpp:96`) and
`TypedConstant` (`type.h:588`, `:612`).

**(c) Constructor-shaped declarations taking plain `int`**, in case one is
spelled `int` rather than `int32`.
```
grep -rnE '\b[A-Z][A-Za-z0-9_]*\((const )?int( |&| &)[a-zA-Z_]+\)' <territory> --include=*.h --include=*.cpp
```
→ 3 lines: `BitStructTypeBuilder(int max_num_bits)` (`type_utils.h:203`),
`Axis(int value)` (`snode.h:27`), `ReplaceLocalVarWithStacks(int ad_stack_size)`
(`auto_diff.cpp:705`). **None has an `int64` sibling**, so none is a
width-selecting overload set. `BitStructTypeBuilder`'s parameter is the width
argument of the S7 factory call at `type_utils.h:205`, already dispositioned as
derived. `Axis::value` is an axis number and `ad_stack_size` a stack depth.

**(d) Silent conversions.** Rerunning (a)+(b)+(c) and filtering out lines
carrying `explicit`:
→ 2 lines, `expr.cpp:92` and `expr.cpp:96`, which are the out-of-line
*definitions* whose `explicit` sits on the declarations at `expr.h:30`, `:32`.
So **there is no non-explicit narrowing constructor in this territory.** Every
such construction must spell the type's name, which is what makes both classes
closable.

**Conclusion: exactly two, and the second one is real.**

`taichi/ir/expr.h:26-36` declares six `explicit` single-argument constructors and
`taichi/ir/expr.cpp:84-106` defines them. The i32 one:

```
expr.h:30:   explicit Expr(int32 x);
expr.cpp:92: Expr::Expr(int32 x) : Expr() {
expr.cpp:93:   expr = std::make_shared<ConstExpression>(PrimitiveType::i32, x);
```

Identical mechanism, one level earlier in the pipeline: `Expr(<host int>)` builds
a `ConstExpression` pinned to `PrimitiveType::i32` with no `i32` token on the
calling line. Report 01 section 3.13 sweeps `expr.cpp:93` and excludes it as
"one overload of a pair whose i64 sibling is at `:96-98`" — the right call for
the *constructor*, but it means the **call sites** were never swept, exactly as
`type.h:582` being excluded left `TypedConstant`'s call sites unswept.

I am calling this **S9**.

## E019 — N065 point 3 resolved mechanically. The over-collection is real; the direction is not.

N065 says the filter over-collects because macro continuation lines carry their
type argument **on the previous line**, five in `constant_fold.cpp` and six in
`ir_builder.cpp`.

I tested every one of my 52 `TYPED2` rows for where its type argument sits:
same line, following line, or neither.

| Position of the type argument | Rows |
|---|---|
| same line | 41 |
| **following** line | 11 |
| previous line | **0** |
| neither | 0 |

41 + 11 = 52. The eleven are `ir_builder.cpp:142`, `:148`, `:154`, `:160`,
`:166`, `:172` and `constant_fold.cpp:79`, `:83`, `:114`, `:117`, `:121` —
**six and five, exactly the files and counts N065 gives.** The identification was
right to the row. Only the direction word is wrong: `:79` is
`res = TypedConstant(` and `:80` begins `dst_type,`.

This matters only for anyone writing the line-spanning parse N065 says it did
not write. It would have to look forward, not back. My classification resolves
arity from source instead and is unaffected either way.

## E020 — S9 enumerated and closed

```
grep -rnE '(^|[^A-Za-z0-9_])Expr\(' taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/ --include=*.h --include=*.cpp
```
→ **35 lines.** Classified:

| Class | Rule | Count |
|---|---|---|
| DECL | the six `explicit` constructors' declarations and definitions, plus `Expr()`, the copy and move constructors, the `shared_ptr` and `Identifier` overloads | 17 |
| SHARED | `Expr(std::make_shared<…>)` or `Expr(std::shared_ptr<Expression>(…))` — the expression-node form, no width | 14 |
| TEMPLATE | `expr.h:90` `Expr(std::make_shared<T>(…))` inside `Expr::make`, and `expr.h:131` `return Expr(val)` inside `template <typename T> Expr value(const T &val)` | 2 |
| **I32** | one argument, static type `int` | **3** |

17 + 14 + 2 + 3 = 35. Two more, `expr.cpp:132` and `snode.cpp:204`, are counted
under SHARED: `expr.cpp:132` is `auto ret = Expr(` continued onto `:133-134`
with a `std::make_shared` argument, and `snode.cpp:204` is
`Expr(snode_to_fields_->at(this))`, a lookup returning an `Expr`, so it selects
the copy constructor. 17 + 12 + 2 + 2 + 2 = 35 either way; I fold them into
SHARED and say so rather than adding a class of two.

**The three I32 sites**, both lines in `frontend_ir.cpp`, inside
`expand_tensor_or_scalar`, the lambda that expands a matrix-typed `Expr` into
per-element index expressions:

```
1835: for (int i = 0; i < shape[0]; i++) {
1836:   auto ind = Expr(std::make_shared<IndexExpression>(
1837:       id_expr, ExprGroup(Expr(i)), expr->dbg_info));
...
1843: for (int i = 0; i < shape[0]; i++) {
1844:   for (int j = 0; j < shape[1]; j++) {
1845:     auto ind = Expr(std::make_shared<IndexExpression>(
1846:         id_expr, ExprGroup(Expr(i), Expr(j)), expr->dbg_info));
```

`i` at `:1835` and `:1843` and `j` at `:1844` are all `int`. `:1846` carries two
sites on one line, which is why three sites sit on two lines.

Each becomes an `ExprGroup` element, i.e. a **frontend index** of an
`IndexExpression`. Same class as the `MatrixPtrStmt` offsets of
`lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477`, one stage earlier.
`grep -nE "frontend_ir\.cpp:18[0-9][0-9]"` over `report-01-ir-types.md` returns
nothing, so **neither line appears anywhere in report 01.**

**Two closing checks on S9, both negative, both worth stating.**

1. `taichi::lang::value<T>` at `expr.h:129-132` is the one place that could
   launder a host type into `Expr` through a template.
   `grep -rn "value<int32>\|value<int64>\|lang::value<" taichi/` returns nothing.
   It has no callers anywhere in `taichi/`. Dead template, no sites.
2. If any operator on `Expr` took a raw `int`, every `expr + 1` in the territory
   would be a hidden i32 site.
   `grep -rnE 'operator[-+*/%<>=!&|]+\([^)]*\b(int|int32|int64|float32|float64)\b' taichi/ir/*.h taichi/ir/expr.cpp`
   returns nothing. Every operator takes `Expr`, and because the constructors are
   `explicit`, `expr + 1` does not compile. No sites by that route.

## E021 — CORRECTION to E020. I wrote the S9 table before building it, again.

Same mistake as E009, in the same session, after writing a paragraph in E015
about exactly this. I wrote 17 / 14 / 2 / 3 from memory of the grep output
instead of from a classification file. Built the file; the columns are
different, and one of them was not even a count of the same thing — I had put a
**site** count in a column of **line** counts, because `:1846` carries two.

Final S9 table, 35 rows, one per grep hit, diffed against the grep output and
identical:

| Class | Lines |
|---|---|
| DECL — the constructor declarations at `expr.h:21`, `:26`, `:28`, `:30`, `:32`, `:34`, `:36`, `:38`, `:42`, `:47`, `:53` and the definitions at `expr.cpp:84`, `:88`, `:92`, `:96`, `:100`, `:104`, `:108` | 18 |
| SHARED — `Expr(std::make_shared<…>)`, `Expr(std::shared_ptr<Expression>(…))`, and `snode.cpp:204`, which selects the copy constructor | 13 |
| TEMPLATE — `expr.h:90` inside `Expr::make`, `expr.h:131` inside `value<T>` | 2 |
| **I32** — `frontend_ir.cpp:1837`, `:1846` | **2 lines** |

18 + 13 + 2 + 2 = 35.

The two I32 lines carry **three sites**, confirmed mechanically:
`sed -n '1837p;1846p' taichi/ir/frontend_ir.cpp | grep -oE '[^A-Za-z0-9_]Expr\(' | wc -l` → 3.

So: **S9 contributes 2 lines and 3 sites, none of them in report 01.** The
figure "3" in E020 was right for sites and wrong as a member of a line-count
column. Recorded rather than edited.

That is now three arithmetic slips of mine in this task — E009's class
breakdown, E016's per-file count, and this one — every one of them from writing
a number next to a list instead of out of it, and every one caught only by
building the list. It is the failure plan section 10 item 7 names, and knowing
the rule did not stop me committing it. The only thing that caught it each time
was the mechanical check.

## E022 — Second verification pass, over the handover additions

Re-ran every command the report quotes and compared each against the number
printed beside it in section 10. Nine sweeps, all matching after two fixes.

**Two more of my own counts were wrong, both caught by running the block rather
than reading it.**

1. **S6 is 5 hits, not 6.** The report said six, and described them as the four
   declarations at `ir_builder.h:133-136` plus definitions and one call. But the
   grep pattern is `get_int32\|get_uint32`, which does not match `get_int64` or
   `get_uint64`. The five are `ir_builder.h:133`, `:135`,
   `ir_builder.cpp:141`, `:153`, `:125`. I had described the whole four-function
   family while quoting a two-function grep. Corrected, and I added the fact
   that `get_int64` and `get_uint64` — `ir_builder.h:134`, `:136`,
   `ir_builder.cpp:147`, `:159` — have **zero** callers anywhere in `taichi/`,
   verified by `grep -rn "get_int64\|get_uint64" taichi/`. So the builder offers
   a 64-bit constant factory that nothing uses, next to a 32-bit one used once.
2. **S7's factory sweep is 7 hits, not 6.** Four are declarations and
   definitions in `type_factory.{h,cpp}` (`type_factory.h:19`, `:21`,
   `type_factory.cpp:160`, `:179`), not three. The three calls and their
   dispositions are unchanged, so `frontend_ir.cpp:265-266` remains the one
   literal-width site.

Still holding after the additions, re-checked:

| Claim | Result |
|---|---|
| section 4 table rows | 40 |
| section 7a site table rows | 2 lines, 3 sites |
| the "33 already carried" list | 33 backticked citations |
| S9 partition 18+13+2+2 | 35, and `diff` against the raw grep is empty |
| S5 partition 14+52+2+10+3+2+56 | 139 |
| `TYPED2` continuation positions 41+11+0+0 | 52 |
| additions outside S5: 2+1+1+9+3 | 16 |
| no stale "six spellings" / "thirteen sites" / "19 sites" / "more than half" | none found |

That is five arithmetic slips of mine across this task. Every one was a number
written beside a list rather than out of it, and every one was caught by a
mechanical check and by nothing else. I have left all five visible in these
notes.
