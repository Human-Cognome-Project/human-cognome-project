# Report 06B — the spellings by which a 32-bit width enters the IR

Agent 06B. Territory: `taichi/ir/`, `taichi/analysis/`, `taichi/transforms/`,
`taichi/inc/`. Tree at `ba0e81dce559fb63a5958bf82feb1d00c55c02fe`.
Contemporaneous notes: `modernization/investigation/notes-06b-i32-spellings.md`.

Serves plan item 6.2 and open question 8.1.1. Investigation only. No source file
was modified. No other agent's file was modified or read beyond the two named as
inputs.

**Everything in sections 1 to 7 is VERIFIED** — every count is the output of a
command given beside it, and every classification was made by opening the line
and, where the width depends on it, the declaration of the argument. Section 8
is INFERRED and is labelled as such line by line. Section 9 is Escalations.

---

## 1. Answer in one page

There are **seven spellings** by which a 32-bit width reaches an IR object in
this territory, not four, plus **two further routes** that carry the same
assumption without being `i32` spellings at all.

S9 was added after a handover from the agent that found this class. Its notes
entry N065 point 4 asks whether any type other than `TypedConstant` has a
width-selecting constructor overload. One does.

| # | Spelling | Command | Hits | Closed? |
|---|---|---|---|---|
| S1 | `PrimitiveType::i32` | `grep -rn "PrimitiveType::i32" <T>` | 64 | by report 01 section 3.13, not re-adjudicated here |
| S2 | `PrimitiveTypeID::i32` | `grep -rn "PrimitiveTypeID::i32" <T>` | 27 | **yes, here.** Was not among report 01's four |
| S3 | `get_data_type<int32>()` | `grep -rn "get_data_type<int32>()" <T>` | **4**, not 3 | yes, here |
| S4 | `TypedConstant((int32)…)` / `int32(` | `grep -rn "TypedConstant((int32)\|TypedConstant{(int32)\|int32(" <T>` | 37 | 10 reconciled here; the other 27 belong to report 01 sections 3.2 and 3.8 |
| S5 | `TypedConstant(<host int expr>)` | `grep -rn "TypedConstant(" <T>` | 139 | **yes, here.** This is the class report 01 left open at 78 candidates |
| S6 | `IRBuilder::get_int32(…)` | `grep -rn "get_int32\|get_uint32" taichi/` | 5, one of them a call | yes, here |
| S9 | `Expr(<host int expr>)` | `grep -rnE '(^\|[^A-Za-z0-9_])Expr\(' <T>` | 35 | **yes, here.** The same mechanism on a second type. In no prior report |

`<T>` is `taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/` throughout.
`taichi/inc/` contributes zero hits to every one of the seven; verified by
running each command with and without it and comparing counts.

| # | Route, not a spelling | Command | Hits | Closed? |
|---|---|---|---|---|
| S7 | a literal bit count into a width-taking factory | `grep -rn "get_primitive_int_type\|get_primitive_real_type" <T>` plus the `sizeof(int32)` sweep | 6 + 5 | yes, here |
| S8 | plain C++ `int` declaring an index/extent carrier | see section 7 | 112 declarations in headers | **partially.** The header subset is closed; `.cpp` locals are not, and section 8.2 says why |

**The headline number.** The open class, S5, resolves to **56 real i32 sites**.
Report 01 names 9 of them exactly and 7 more only inside line ranges cited for
other reasons. **Forty appear nowhere in report 01.** They are listed in
section 4.

**Three of the 78 candidates report 01 left open are not i32 at all**, and no
amount of reading the lines would show it. `check_out_of_bound.cpp:48`, `:107`,
`:180` are `TypedConstant(true)`. `taichi/common/core.h:136` is
`using uint1 = bool`, so `bool` is an *exact* match for the `uint1` overload and
beats the `bool`→`int` integral promotion. They produce `PrimitiveType::u1`.
Two more, `auto_diff.cpp:957` and `:963`, are `TypedConstant(x)` where `x` is
declared `float32` in the enclosing signature at `auto_diff.cpp:953`.

**Beyond S5, this sweep adds sixteen sites across four other routes**: two under
S2, one under S7's factory arm, one under S7's `sizeof` arm, nine `int`
declarations under S8, and three under S9. They are in sections 5, 6, 7 and 7a.
2 + 1 + 1 + 9 + 3 = 16.

**Grand total this report adds to report 01's inventory: 56 sites.** Forty from
S5, sixteen from the rest. That the two halves are equal is a coincidence of
counting, not a structure.

---

## 2. The mechanism, corrected

Report 01 and its notes call `taichi/ir/type.h:582` an "implicit `int32`
constructor". The constructor is `explicit`:

```
taichi/ir/type.h:582:  explicit TypedConstant(int32 x) : dt(PrimitiveType::i32), val_i32(x) {
```

Every single-argument constructor in the set is `explicit`
(`type.h:575`, `:582`, `:585`, `:588`, `:591`, `:594`, `:597`, `:600`, `:603`,
`:606`, `:609`, `:612`); only the default at `:572` and the two-argument
template at `:616` are not. The same is true of `Expr`: `taichi/ir/expr.h:26-36`
declares six `explicit` single-argument constructors, and `expr.cpp:92-94` is
the `int32` one.

Two consequences, and both are what make the class closable:

1. **No silent conversion.** A bare `int` cannot become a `TypedConstant` by
   passing through a parameter. The token `TypedConstant(` appears at every
   construction, so `grep -rn "TypedConstant("` bounds the class exactly.
   *Implicit* describes the width, never the conversion.
2. **The width is chosen by the argument's declared C++ type**, at a place that
   may be many lines away. `TypedConstant(x)` is i32 if `x` is `int`, i64 if
   `int64`, u64 if `size_t`, u1 if `bool`, f32 if `float32`. This is why the
   class cannot be classified by reading the lines, and why report 01's 78 was a
   candidate count rather than a site count.

This is also why S5 and S8 are the same mechanism from two ends: the host `int`
is what selects the i32 overload.

**And it is why the mechanism had to be looked for on other types.** `Expr` has
the identical shape — `expr.h:30` `explicit Expr(int32 x)`, `expr.cpp:92-94`
building a `ConstExpression` pinned to `PrimitiveType::i32` — one stage earlier
in the pipeline. Section 7a enumerates it. Report 01 section 3.13 sweeps
`expr.cpp:93` and correctly excludes it as one overload of a pair, which is the
right call for the constructor and is exactly what left its call sites unswept,
the same way excluding `type.h:582` left `TypedConstant`'s call sites unswept.

---

## 3. S5 closed. All 139, partitioned.

```
grep -rn "TypedConstant(" taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/
```
→ **139 lines in 26 files.**

I did not reproduce report 01's 78-line filter. That filter is defined by
subtracting a hand-written list of type spellings from the superset, which
reintroduces the exact failure the exercise exists to fix. I classified the
superset instead.

| Class | Membership rule | Count |
|---|---|---|
| `DECL` | the constructor declarations in `taichi/ir/type.h` themselves — mechanism, not sites | 14 |
| `TYPED2` | two-argument `TypedConstant(<type expr>, value)`; the width comes from the type argument | 52 |
| `TYPED1DT` | one argument, and it is a `DataType`, selecting `type.h:575` | 2 |
| `CASTED` | one argument with an explicit `(int32)` / `int32(` cast — i32, but that is spelling S4 | 10 |
| `U1` | one argument, declared `bool`; exact match on the `uint1` overload → `PrimitiveType::u1` | 3 |
| `F32` | one argument, declared `float32` → `PrimitiveType::f32` | 2 |
| `I32` | one argument, static type `int`/`int32` → `PrimitiveType::i32` | **56** |

14 + 52 + 2 + 10 + 3 + 2 + 56 = **139**.

Checked mechanically, not by hand: the classification file has 139 rows, 139
distinct `file:line` keys, and `diff` of its key column against the raw grep
output is empty.

**The two rules by which `TYPED2` is not a site.** A two-argument call takes its
width from the first argument. Where that argument is `PrimitiveType::i32` or
`get_data_type<int32>()` the line is a site *of S1 or S3* and report 01 already
holds it — `utils.cpp:17`, `make_mesh_thread_local.cpp:42`,
`make_cpu_multithreaded_range_for.cpp:68`, `:70`, `:72`, `:81`, `:90`,
`scalarize.cpp:63`, `:114`, `:471`, `:473`, `:530`. Where it is a runtime
`DataType` (`dst_type`, `stmt->ret_type`, `data_type`, `load_data_type`,
`dtype.ptr_removed().get_element_type()`), the width is inherited and the line
decides nothing. The 17 `constant_fold.cpp` rows are all of the second kind.

**The continuation lines, resolved mechanically rather than asserted.** Report 01
section 3.13 and its notes entry N065 both say the filter over-collects because
some macro continuation lines carry their type argument on the **previous** line,
five in `constant_fold.cpp` and six in `ir_builder.cpp`. I tested all 52
`TYPED2` rows for where the type argument actually sits:

| Position of the type argument | Rows |
|---|---|
| same line | 41 |
| **following** line | 11 |
| previous line | **0** |
| neither | 0 |

41 + 11 = 52. The eleven are `ir_builder.cpp:142`, `:148`, `:154`, `:160`,
`:166`, `:172` and `constant_fold.cpp:79`, `:83`, `:114`, `:117`, `:121` — six
and five, **exactly the files and counts report 01 gives**. The identification
was right to the row; only the direction is wrong. `:79` is
`res = TypedConstant(` and `:80` begins `dst_type,`.

This matters only to anyone writing the line-spanning parse report 01 says it
did not write: it has to look forward, not back. My classification resolves
arity from source and is unaffected either way.

**The `U1` and `F32` five are the trap.** They survive any filter that keeps a
line when its first argument is not a type, and they read on the page exactly
like the i32 rows they sit among. Classifying this class by eye produces 61
sites where there are 56.

---

## 4. The forty S5 sites that appear nowhere in report 01

Status was computed, not eyeballed. I extracted every `file:line` citation in
`report-01-ir-types.md`, including its back-reference shorthand
(`` `:1160` `` attributed to the last full path seen on the line), and separated
exact citations from line-range ones.

| Status | Meaning | Count |
|---|---|---|
| A | the exact line is named in report 01 | 9 |
| B | falls inside a range report 01 cites for another reason, not itself named | 7 |
| C | appears nowhere in report 01 | **40** |

9 + 7 + 40 = 56, asserted by the script rather than added by me.

**A (9).** `frontend_ir.cpp:732`, `:735`; `lower_matrix_ptr.cpp:189`, `:232`,
`:424`, `:477`; `lower_ast.cpp:270`, `:271`; `demote_operations.cpp:99` — and
report 01 states of the last that it is quoted as a worked example and is **not**
inventoried.

**B (7).** `frontend_ir.cpp:747`; `lower_matrix_ptr.cpp:537`;
`check_out_of_bound.cpp:126`, `:217`; `simplify.cpp:175`, `:178`, `:193`.
Two of these are worth separating out, because the range that covers them is
about a different object. `frontend_ir.cpp:747` sits inside `:742-747`, which
report 01 section 3.6 files as a **host** `int` product; `:747` is where that
host `int` becomes an IR i32 constant, which is a different and fixable thing.
The same split applies to `simplify.cpp:178` against section 3.6's `:176`.

**C (40).** What each width feeds:

| Site | What the width feeds |
|---|---|
| `taichi/ir/frontend_ir.cpp:663` | per-axis index offset in `make_index_stmts`, subtracted from a field index at `:664`; the result becomes a `GlobalPtrStmt` index |
| `taichi/transforms/utils.cpp:7` | `y - 1` mask in `generate_mod`, `bit_and` at `:8`. The most-used index-arithmetic helper in the territory |
| `taichi/transforms/utils.cpp:10` | general divisor in `generate_mod`, `mod` at `:11` |
| `taichi/transforms/utils.cpp:20` | general divisor in `generate_div`, `div` at `:21` |
| `taichi/transforms/check_out_of_bound.cpp:47` | zero lower bound, `ExternalPtrStmt` index check, `cmp_ge` at `:60-61` |
| `taichi/transforms/check_out_of_bound.cpp:74` | flattened element-shape extent as upper bound, `cmp_lt` at `:77-78` |
| `taichi/transforms/check_out_of_bound.cpp:106` | zero lower bound, `GlobalPtrStmt` index check |
| `taichi/transforms/check_out_of_bound.cpp:142` | SNode index offset added back for the error message, `add` at `:143-144` |
| `taichi/transforms/check_out_of_bound.cpp:179` | zero lower bound, `MatrixPtrStmt` offset check |
| `taichi/transforms/check_out_of_bound.cpp:186` | maximum valid matrix element index, `cmp_le` at `:187-188` |
| `taichi/transforms/demote_dense_struct_fors.cpp:49` | initial value of every reconstructed per-axis loop variable |
| `taichi/transforms/demote_dense_struct_fors.cpp:79` | per-axis multiplier in loop-index reconstruction, `mul` at `:80-81` |
| `taichi/transforms/bit_loop_vectorize.cpp:76` | vectorization width, the bit count of the load type, added to or subtracted from the bit-loop index at `:79-80` |
| `taichi/transforms/bit_loop_vectorize.cpp:188` | dummy zero for the `lhs + 0` that marks a statement vectorized |
| `taichi/transforms/handle_external_ptr_boundary.cpp:30` | zero lower clamp bound for external-array indices |
| `taichi/transforms/handle_external_ptr_boundary.cpp:39` | the `1` forming `shape - 1`, the upper clamp bound, `sub` at `:40-41` |
| `taichi/transforms/handle_external_ptr_boundary.cpp:71` | zero lower clamp for a `MatrixPtrStmt` offset |
| `taichi/transforms/handle_external_ptr_boundary.cpp:78` | maximum valid index, upper clamp, `min` at `:79-80` |
| `taichi/transforms/lower_ast.cpp:241` | struct-for index offset added to a `LoopIndexStmt` at `:242-243` |
| `taichi/transforms/make_mesh_block_local.cpp:83` | mapping dtype byte size × mesh index → block-local byte offset |
| `taichi/transforms/make_mesh_block_local.cpp:116` | dtype byte size × local index → block-local byte offset |
| `taichi/transforms/make_mesh_block_local.cpp:166` | block dimension `1` on `x64`/`arm64` |
| `taichi/transforms/make_mesh_block_local.cpp:204` | mapping dtype byte size × index → byte offset |
| `taichi/transforms/make_mesh_block_local.cpp:268` | dtype byte size × index → byte offset |
| `taichi/transforms/make_mesh_block_local.cpp:304` | dtype byte size × index → byte offset |
| `taichi/transforms/make_mesh_block_local.cpp:327` | thread index `0` on `x64`/`arm64` |
| `taichi/transforms/make_mesh_block_local.cpp:578` | mapping dtype byte size × index → byte offset |
| `taichi/transforms/make_block_local.cpp:145` | `loop_offset` of the block-stride BLS prologue loop, added to the thread index at `:147-148` |
| `taichi/transforms/make_block_local.cpp:152` | dtype byte size × element id → BLS byte offset |
| `taichi/transforms/make_block_local.cpp:165` | `bls_num_elements`, the guard bound of the block-stride loop, `cmp_lt` at `:163-165` |
| `taichi/transforms/make_block_local.cpp:183` | per-axis pad size, the `mod` and `div` divisor decomposing a BLS element id at `:185-188` |
| `taichi/transforms/make_block_local.cpp:194` | pad lower bound added to a BLS coordinate to recover the global index |
| `taichi/transforms/make_block_local.cpp:202` | pad coefficient multiplying the block corner |
| `taichi/transforms/make_block_local.cpp:277` | the same coefficient, the second of the two sites |
| `taichi/transforms/make_block_local.cpp:285` | pad lower bound subtracted |
| `taichi/transforms/make_block_local.cpp:298` | zero lower bound in the debug-mode BLS bound assertion |
| `taichi/transforms/make_block_local.cpp:303` | BLS axis size, the upper bound of that assertion |
| `taichi/transforms/make_block_local.cpp:315` | per-axis BLS stride multiplier |
| `taichi/transforms/make_block_local.cpp:328` | dtype byte size converting a BLS element offset to bytes |
| `taichi/transforms/demote_operations.cpp:134` | the zero of `demote_ifloordiv`'s sign correction, compared against both operands at `:147-151` and `:155-156`, and broadcast to a `MatrixInitStmt` at `:138-141` when the left operand is a `TensorType` |

Row count of that table: 40, matching status C.

**Two files carry exactly half of it.** `make_block_local.cpp` contributes 12
and `make_mesh_block_local.cpp` 8, all block-local-storage offset arithmetic.
Per-file counts over the forty, derived from the list rather than read off the
table: 12, 8, 6 (`check_out_of_bound.cpp`), 4
(`handle_external_ptr_boundary.cpp`), 3 (`utils.cpp`), 2
(`demote_dense_struct_fors.cpp`), 2 (`bit_loop_vectorize.cpp`), 1
(`lower_ast.cpp`), 1 (`demote_operations.cpp`), 1 (`frontend_ir.cpp`).
They sum to 40.
Neither file appears in report 01's section 3.3 at all. `check_out_of_bound.cpp`
contributes 6 and `handle_external_ptr_boundary.cpp` 4, all bounds checking.

Argument declarations for every non-literal case are in notes entry E007. The
recurring ones: `data_type_size` returns `int` (`taichi/ir/type_utils.h:13`),
`data_type_bits` returns `int` (`:19`), `ScratchPad::pad_size_linear` returns
`int` (`taichi/ir/scratch_pad.h:131`), and `ScratchPad::coefficients`,
`pad_size` and `BoundRange::low`/`high` are `int`
(`scratch_pad.h:47`, `:51`, `:34`, `:35`).

---

## 5. S2, the spelling nobody named

`PrimitiveTypeID::i32` is a distinct token. `grep "PrimitiveType::i32"` does not
match it, and `grep "PrimitiveTypeID::i32"` does not match the other; both
directions verified at zero. Report 01 section 3.13 names four spellings and
this is not among them, though report 01 does cite four of its 27 lines for
other reasons.

```
grep -rn "PrimitiveTypeID::i32" taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/
```
→ **27 lines.** Full row-by-row disposition is notes entry E012.

| Class | Rule | Count |
|---|---|---|
| MAP | a width-to-type or C++-type-to-Taichi-type mapping arm, correct by construction | 2 |
| EXH | one arm of a dispatch that has an arm for every primitive type | 14 |
| S6DEF | the body of `IRBuilder::get_int32` | 1 |
| INV | already cited by report 01 | 7 |
| GUARD | the `if` condition for a cast report 01 already carries | 1 |
| **NEW** | **real, and in neither report** | **2** |

2 + 14 + 1 + 7 + 1 + 2 = 27.

The `GUARD` row is `type_check.cpp:148`, the condition for the cast at `:154`.
Report 01 section 3.13 excludes its twin `:459` on precisely this ground, so
excluding `:148` is its own rule applied consistently, not mine.

### The two new sites

**`taichi/ir/type.cpp:238` — `QuantIntType` defaults its compute type to i32.**

```
232: QuantIntType::QuantIntType(int num_bits, bool is_signed, Type *compute_type)
237:   if (compute_type == nullptr) {
238:     auto type_id = is_signed ? PrimitiveTypeID::i32 : PrimitiveTypeID::u32;
239:     this->compute_type_ =
240:         TypeFactory::get_instance().get_primitive_type(type_id);
241:   }
```

This is exactly what the brief asks for under "any typed helper or factory that
defaults to 32 bits". Every `QuantIntType` constructed without an explicit
compute type computes in 32 bits. It pairs with `taichi/ir/type.h:372`,
`int num_bits_{32}`, whose neighbouring comment at `:369-370` states the
intention outright: *"TODO(type): for now we can uniformly use i32 as the
'compute_type'. It may be a good idea to make 'compute_type' also
customizable."* The in-tree comment is the strongest evidence in this territory
that a 32-bit default was a deliberate placeholder.

**`taichi/transforms/simplify.cpp:234` — SNode children asserted 32-bit wide.**

```
232: for (int i = 0; i < (int)snode->ch.size(); i++) {
233:   TI_ASSERT(snode->ch[i]->type == SNodeType::place);
234:   TI_ASSERT(snode->ch[i]->dt->is_primitive(PrimitiveTypeID::i32) ||
235:             snode->ch[i]->dt->is_primitive(PrimitiveTypeID::f32));
236: }
238: auto offset_stmt = Stmt::make<IntegerOffsetStmt>(
239:     stmt, previous_offset->offset * sizeof(int32) * (snode->ch.size()));
```

Report 01 section 3.7 carries `:238-239`, the `sizeof(int32)` byte stride, and
does not carry `:234`. The two are one mechanism: `:234` is the **assertion that
makes `:239` sound**, and it aborts rather than degrading if an SNode child is
any width other than 32 bits. Changing the stride at `:239` without changing
`:234` swaps a wrong answer for an abort; changing `:234` without `:239` swaps
an abort for a wrong answer. They must move together.

---

## 6. S6 and S7

**S6, `IRBuilder::get_int32`.** Closed and nearly empty.
`grep -rn "get_int32\|get_uint32" taichi/ --include=*.cpp --include=*.h` returns
**five** hits, all inside `taichi/ir/ir_builder.h` and `taichi/ir/ir_builder.cpp`:
declarations at `ir_builder.h:133`, `:135`, definitions at `ir_builder.cpp:141`,
`:153`, and **one call anywhere in `taichi/`**, `ir_builder.cpp:125` inside
`IRBuilder::create_break()`. The 64-bit siblings `get_int64` and `get_uint64`
are declared at `ir_builder.h:134`, `:136` and defined at `ir_builder.cpp:147`,
`:159`; they are outside this grep by construction, and both are uncalled too. Report 01 already inventoried that call in its
section 3.3. Naming S6 as a spelling adds no site; it is recorded so the
enumeration shows the sweep was run and came back empty.

**S7, a literal bit count into a width-taking factory.** Membership rule: a call
in territory to `TypeFactory::get_primitive_int_type` or
`get_primitive_real_type` — the only two APIs taking a width as a number —
whose width argument is a literal.

```
grep -rn "get_primitive_int_type\|get_primitive_real_type" taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/
```
→ **7** hits. Four are the declarations and definitions in
`type_factory.{h,cpp}` — `type_factory.h:19`, `:21`, `type_factory.cpp:160`,
`:179`. Of the three calls:

| Call | Width argument | Verdict |
|---|---|---|
| `taichi/ir/type_utils.h:205` | `max_num_bits`, the `BitStructTypeBuilder` constructor parameter | derived, not a site |
| `taichi/ir/snode.cpp:150` | `bits`, from the caller | derived, not a site |
| `taichi/ir/frontend_ir.cpp:265-266` | **literal `32`** | **new site** |

`frontend_ir.cpp:265-266` is inside `UnaryOpExpression::type_check`, the
`UnaryOpType::frexp` branch:

```
263: elements.push_back({operand_primitive_type, "mantissa", 0});
264: elements.push_back(
265:     {taichi::lang::TypeFactory::get_instance().get_primitive_int_type(
266:          32, /*is_signed=*/true),
267:      "exponent", (size_t)data_type_size(operand_primitive_type)});
```

The mantissa member is derived from the operand's own type on the line above;
the exponent member is pinned at 32 bits. Not an index, and it does not scale
with the SNode count, so its weight for item 6.2 is low. It is here because it
is a real width literal in territory, in a spelling neither report ran, and
because the asymmetry between the two adjacent lines is the point.

**S7, second arm — hardcoded byte widths.**
```
grep -rn "sizeof(int32)\|sizeof(int)\|sizeof(i32)" taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/
```
→ 5 lines. `simplify.cpp:239`, `:261` are report 01 section 3.7.
`scalarize.cpp:1113`, `:1140` are inside `/* … */` documentation comments —
verified by reading `:1105-1145` — and are excluded as comments, not code.

The fifth is **new**: `taichi/ir/statements.h:1787`.

```
1786: std::size_t size_in_bytes() const {
1787:   return sizeof(int32) + entry_size_in_bytes() * max_size;
1788: }
```

`AdStackAllocaStmt::size_in_bytes`. Every other quantity in that class is
`std::size_t` — `max_size` at `:1770`, `element_size_in_bytes` at `:1778`,
`entry_size_in_bytes` at `:1782` — and the four-byte header, the autodiff
stack's own top-of-stack counter, is the one hardcoded term. Autodiff, so
whether it is in scope is a ruling, not my call; escalated.

---

## 7. S8, plain C++ `int`, closed on headers only

The literal class in the brief — "plain C++ `int` where it flows into an IR type
or an index" — is not enumerable. `int` occurs many thousands of times in this
territory and most occurrences are loop counters, ids and thread counts. Any
number stated over it would be a quantifier no citation can test, which
standing instruction 6 forbids. Two closable subsets instead.

**(a) The host-`int`-to-IR-i32 crossings are exactly S5's 56 rows.** That set is
complete for this territory, because `TypedConstant` is `explicit` and those are
all its single-`int`-argument constructions. Nothing further to do.

**(b) `int`-typed declarations in territory headers.** Three sweeps, because the
first one leaked and I found the leak by opening a line rather than by trusting
the pattern. All three are in notes entries E013 and E015.

Membership: a line in a territory header declaring `int`, `int32`, or a
container parameterised on `int`, followed by a name.

**112 declarations. 42 carry a width-bearing quantity; 70 do not.** 42 + 70 = 112.

Not width-bearing, with the reason: thread and block counts
(`num_cpu_threads`, `block_dim`, `grid_dim`), object ids (`Stmt::id`,
`Stmt::instance_id`, `SNode::id`, `Identifier::id`, `snode_tree_id_`,
`id_in_bit_struct`, `id_counter_`, and `statements.h:757`
`std::unordered_set<int> covers`, which holds SNode ids), axis numbers bounded
by `taichi_max_num_indices = 12` (`Axis::value`, `num_active_indices`,
`LoopIndexStmt::index`, `BlockCornerIndexStmt::index`, `axis`), argument ids and
depths, struct-member index paths (`GetElementStmt::index`,
`GetElementExpression::index`), texture parameters (`num_dims`, `dimensions`,
`lod`), bit-struct member bookkeeping bounded by the physical type's own width,
insertion positions, `addr_space_`, `path_length_`, `final_node`,
`unique_accessed`, `depth`, and `type.h:558` `int32 val_i32`, which is the
mechanism itself.

Report 01 sections 3.5 and 3.6 already carry **33** of the 42:
`analysis.h:20`, `:185`; `bls_analyzer.h:16`, `:17`; `scratch_pad.h:34`, `:35`,
`:47`, `:51`, `:53`, `:97`, `:133`, `:142`, `:150`; `snode.h:41`, `:45`, `:49`,
`:79`, `:81`, `:98`; `frontend_ir.h:529`, `:615`, `:617`, `:676`;
`statements.h:376`, `:379`, `:456`, `:1280`, `:1370`, `:1415`, `:1416`,
`:1439`; `type.h:223`, `:308`. That list has 33 entries. 33 + 9 = 42.

**The nine that are new:**

| Site | Declaration | Why it is width-bearing |
|---|---|---|
| `taichi/ir/type.h:250` | `std::vector<int> shape_` — `TensorType` | the tensor's own shape. `get_num_elements()` at `:222-227`, which report 01 carries, is the product **of these**; the factors were never inventoried |
| `taichi/ir/type.h:372` | `int num_bits_{32}` — `QuantIntType` | the default *is* 32, and the comment at `:369-370` says so. Pairs with `type.cpp:238` in section 5 |
| `taichi/ir/type.h:545` | `int num_elements_` — `QuantArrayType` | element count of a quantised array |
| `taichi/ir/statements.h:1063` | `std::vector<int> index_offsets` — `StructForStmt` | report 01 carries the `OffloadedStmt` twin at `:1439` and not this one. It is the field written at `lower_ast.cpp:234` and read at `:239-241`, which is S5 site `lower_ast.cpp:241` |
| `taichi/ir/mesh.h:77` | `int num_patches{0}` — `Mesh` | mesh patch count |
| `taichi/ir/mesh.h:78` | `MeshMapping<int> num_elements{}` | per-element-type mesh element counts |
| `taichi/ir/mesh.h:79-80` | `MeshMapping<int> patch_max_element_num{}` | maximum elements per patch |
| `taichi/transforms/make_mesh_block_local.h:69` | `int mapping_dtype_size_{0}` | the byte size fed to `TypedConstant` at `make_mesh_block_local.cpp:83`, `:204`, `:578` — three of section 4's forty |
| `taichi/ir/scratch_pad.h:232` | `int offset = 0` in `generate_address_code` | host address product — **qualified, see below** |

`scratch_pad.h:232` is listed with its qualification rather than dropped.
`generate_address_code` is defined at `scratch_pad.h:229` and
`grep -rn "generate_address_code" taichi/` returns exactly one hit, that
definition. It has no callers anywhere in `taichi/`, and `offset` is never read
after the loop that computes it. Dead on two counts. Recorded because standing
instruction 3 says not to decide something is unnecessary, and because "dead
today" is a reachability claim that a future caller falsifies.

Four of the mesh rows above are mesh machinery. Report 01 escalation 1 already
asks whether mesh is in scope; the same ruling governs these.

---

## 7a. S9 — the same mechanism on a second type

Added on handover. Report 01's notes entry N065 point 4 says its author only
ever tested the filter against `TypedConstant` and never asked whether other
constructors in this territory narrow the same way. Asked here, and answered:
**one does.**

### How I established that the answer is "exactly two, and no more"

A type qualifies if a single-argument constructor selects the width from the
argument's C++ type — that is, if it has both a 32-bit and a 64-bit
single-argument overload. Three sweeps over the territory's `.h` and `.cpp`:

| Sweep | Command | Result |
|---|---|---|
| 32-bit constructor-shaped declarations | `grep -rnE '\b[A-Z][A-Za-z0-9_]*\((const )?(int32\|uint32)( \|&\| &)[a-zA-Z_]+\)' <T>` | 4 lines, two types: `Expr` and `TypedConstant` |
| the 64-bit siblings that make an overload set width-selecting | `grep -rnE '\b[A-Z][A-Za-z0-9_]*\((const )?(int64\|uint64\|long\|std::size_t\|size_t)( \|&\| &)[a-zA-Z_]+\)' <T>` | 4 lines, the same two types |
| constructors spelled `int` rather than `int32` | `grep -rnE '\b[A-Z][A-Za-z0-9_]*\((const )?int( \|&\| &)[a-zA-Z_]+\)' <T>` | 3 lines, **none with an `int64` sibling** |

The third sweep's three are `BitStructTypeBuilder(int max_num_bits)`
(`taichi/ir/type_utils.h:203`), `Axis(int value)` (`taichi/ir/snode.h:27`) and
`ReplaceLocalVarWithStacks(int ad_stack_size)`
(`taichi/transforms/auto_diff.cpp:705`). None is a width-selecting overload set.
`BitStructTypeBuilder`'s parameter is the width argument of the S7 factory call
at `type_utils.h:205`, already dispositioned as derived; the other two carry an
axis number and a stack depth.

**No silent conversions exist.** Rerunning all three sweeps and filtering out
lines carrying `explicit` leaves two hits, `expr.cpp:92` and `:96`, which are the
out-of-line definitions whose `explicit` sits on the declarations at `expr.h:30`
and `:32`. So every construction of either type must spell that type's name,
which is what makes both classes closable at all.

### S9 enumerated

```
grep -rnE '(^|[^A-Za-z0-9_])Expr\(' taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/ --include=*.h --include=*.cpp
```
→ **35 lines.**

| Class | Rule | Lines |
|---|---|---|
| DECL | the eleven declarations at `expr.h:21`, `:26`, `:28`, `:30`, `:32`, `:34`, `:36`, `:38`, `:42`, `:47`, `:53` and the seven definitions at `expr.cpp:84`, `:88`, `:92`, `:96`, `:100`, `:104`, `:108` | 18 |
| SHARED | `Expr(std::make_shared<…>)` or `Expr(std::shared_ptr<Expression>(…))` — the expression-node form, no width — plus `snode.cpp:204`, which selects the copy constructor | 13 |
| TEMPLATE | `expr.h:90` inside `Expr::make`, `expr.h:131` inside `value<T>`; the width, if any, comes from the call site | 2 |
| **I32** | one argument, static type `int` | **2** |

18 + 13 + 2 + 2 = 35. Diffed against the raw grep output: identical.

**Two lines, three sites.** `frontend_ir.cpp:1846` carries two.
Counted mechanically:
`sed -n '1837p;1846p' taichi/ir/frontend_ir.cpp | grep -oE '[^A-Za-z0-9_]Expr\(' | wc -l` → 3.

| Site | What the width feeds |
|---|---|
| `taichi/ir/frontend_ir.cpp:1837` | `Expr(i)`, `int i` at `:1835`. The single index of an `ExprGroup` passed to an `IndexExpression` at `:1836-1837`, expanding a one-dimensional matrix-typed `Expr` into per-element index expressions |
| `taichi/ir/frontend_ir.cpp:1846` | `Expr(i)` and `Expr(j)`, `int i` at `:1843` and `int j` at `:1844`. The two indices of the `ExprGroup` for the two-dimensional case at `:1845-1846` |

Both are inside `expand_tensor_or_scalar`. Each argument becomes a **frontend
index** of an `IndexExpression` — the same class as the `MatrixPtrStmt` offsets
at `lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477` that report 01 added on
instruction, one stage earlier in the pipeline.

`grep -nE "frontend_ir\.cpp:18[0-9][0-9]" report-01-ir-types.md` returns
nothing. **Neither line appears anywhere in report 01.**

### Two routes into S9 that are closed and empty

Stated because a negative result from a sweep that was actually run is worth
more than a sweep nobody thought to run.

1. **`taichi::lang::value<T>`** at `expr.h:129-132` is `return Expr(val);` — the
   one place a host type could reach an `Expr` through a template.
   `grep -rn "value<int32>\|value<int64>\|lang::value<" taichi/` returns
   nothing. It has no callers anywhere in `taichi/`. Dead template, no sites.
2. **Operators.** If any operator on `Expr` took a raw `int`, every `expr + 1`
   in the territory would be a hidden i32 site.
   `grep -rnE 'operator[-+*/%<>=!&|]+\([^)]*\b(int|int32|int64|float32|float64)\b' taichi/ir/*.h taichi/ir/expr.cpp`
   returns nothing. Every operator takes `Expr`, and because the constructors are
   `explicit`, `expr + 1` does not compile. No sites by that route.

---

## 8. Inferred, and marked as such

Nothing in sections 1 to 7 depends on anything below.

1. **The forty new S5 sites divide into three weights.** *Inferred.* Roughly a
   third are `0`/`1` sentinels in bounds checks and clamps, where the value is
   small forever but the *statement type* still fixes the comparison's width
   against a possibly-64-bit index. Roughly a third are byte-size multipliers
   from `data_type_size`, which is `int` and bounded by the element size, so the
   constant itself is small but the product it forms is an offset. The rest —
   `utils.cpp:7`, `:10`, `:20`, `demote_dense_struct_fors.cpp:79`,
   `check_out_of_bound.cpp:74`, `:186`, `handle_external_ptr_boundary.cpp:78`,
   `make_block_local.cpp:165`, `:183`, `:303`, `:315` — carry extents and
   strides that scale with the field, and are the ones that matter for 6.2. I
   have not measured any of them and this is a reading, not a result.
2. **`utils.cpp` is the highest-leverage single file in the S5 set.** *Inferred.*
   Its three sites are inside `generate_mod` and `generate_div`, whose `int y`
   parameter report 01 section 3.7 already flags. Callers are
   `scalar_pointer_lowerer.cpp:73`, `:75` and `demote_dense_struct_fors.cpp:60`,
   `:63`, `:74`, `:76`, per report 01. Six call sites in the index-lowering path
   through one helper whose parameter, its constant, and its emitted statement
   are all 32-bit. I verified the three constants; the leverage claim is mine.
3. **The block-local files are a coherent block, not scattered misses.**
   *Inferred.* All 20 sites in `make_block_local.cpp` and
   `make_mesh_block_local.cpp` are offsets into a shared-memory scratchpad whose
   size is bounded by `default_shared_mem_size = 65536`
   (`taichi/inc/constants.h`). If that bound holds, none of the 20 can overflow
   32 bits regardless of the SNode count, and they are cheap to leave alone. I
   have not verified that the bound is enforced on every path, and if it is not,
   the reading collapses. This is the single largest group in section 4 and its
   weight turns entirely on a fact I did not establish.
4. **Report 01's 78 probably contains all 56 plus the 10 `CASTED` and some of
   the 52 `TYPED2`.** *Inferred.* I did not reconstruct its filter, so I cannot
   state the overlap. 56 + 10 = 66, and the residue between 66 and 78 would be
   two-argument calls whose type argument its hand list did not name. Offered
   only so a reviewer can see the two numbers are consistent, not as a result.

---

## 9. Escalations

1. **The word "complete" on S1.** I reproduced report 01's 64 and did not
   re-adjudicate its dispositions of them. If the planner wants S1 independently
   re-closed, that is a separate pass and I did not do it.
2. **Mesh scope, again.** Twenty of the forty new S5 sites are in
   `make_block_local.cpp` and `make_mesh_block_local.cpp`, and four of the nine
   new S8 declarations are mesh fields. Report 01 escalation 1 already asks
   whether mesh is in scope. That ruling now governs about half of what this
   report adds, so it is worth more than it was.
3. **Autodiff scope.** `statements.h:1787` and the two `auto_diff.cpp` `F32`
   rows are autodiff machinery. No ruling exists on autodiff scope that I can
   see. Section 1.3 of the plan does not obviously cover it either way.
4. **`type.h:372` plus `type.cpp:238` is a decision, not an oversight.** The
   in-tree comment says a 32-bit compute type was chosen "for now" and should be
   made customizable. That is upstream stating the intent item 6.2 has. It sits
   in the quantised-type machinery, which nothing in the plan mentions. Whether
   quantised types are in scope needs a ruling before anyone decides what to do
   with it.
5. **`simplify.cpp:234` and `:239` must move together**, per section 5. Report
   01 carries one and not the other. Flagged so the pair is not split when
   someone works from that report's section 3.7.
6. **`scratch_pad.h:232` is in an uncalled function.** I recorded it and
   labelled it rather than dropping it, per standing instruction 3. If the
   planner wants uncalled code excluded as a class, that is a rule I would
   rather be given than invent.
7. **S8 in `.cpp` locals is open.** Section 7 closes headers only. Report 01
   section 3.6 has nine `.cpp` rows of this class and does not claim to be
   complete on it either. Closing it means the same argument-declaration work as
   section 3, over a much larger set, and nobody has scoped it. I am not
   estimating a size for it, because report 01's 78 is what an estimate over an
   unclassified set looks like.
8. **Two count corrections to report 01, both benign, both stated so the
   arithmetic reconciles.** Its section 3.13 gives 3 for the
   `get_data_type<int32>()` sweep where the stated command returns 4, the fourth
   being `taichi/ir/type.cpp:463`, which its own prose calls a hit "tree-wide"
   when it is in territory. And its description of the `constant_fold.cpp` macro
   rows puts the type argument on the previous line where it is on the following
   line, though its identification of which eleven rows they are is right to the
   row (section 3). Neither changes a disposition.
9. **`frontend_ir.cpp:1837` and `:1846` need a filing decision.** They are
   frontend indices of an `IndexExpression`, the same class report 01 files in
   its section 3.3, but they arrive through a spelling that report does not
   have. Whether they enter that report's inventory or stand as this one's
   addition is the planner's call, not mine.
10. **The S9 sweep found no third type, and I want that recorded as a result
   rather than as silence.** Section 7a states the three commands and their
   outputs. If a reviewer believes a width-selecting constructor exists that
   those three sweeps cannot see, the sweeps are the thing to attack, and I
   would rather be shown the gap than have the negative treated as proven by my
   saying so. The one form they would miss by construction is a constructor
   whose parameter is a typedef spelled neither `int`, `int32`, `int64`,
   `uint32`, `uint64`, `long`, `size_t` nor `std::size_t`. I did not enumerate
   the territory's typedefs to close that.

---

## 10. Reproducing this report

```
cd /opt/project/taichi
T="taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/"

grep -rn "PrimitiveType::i32" $T            | wc -l    # 64   (S1)
grep -rn "PrimitiveTypeID::i32" $T          | wc -l    # 27   (S2)
grep -rn "get_data_type<int32>()" $T        | wc -l    #  4   (S3)
grep -rn "TypedConstant((int32)\|TypedConstant{(int32)\|int32(" $T | wc -l   # 37 (S4)
grep -rn "TypedConstant(" $T                | wc -l    # 139  (S5 superset)
grep -rn "get_int32\|get_uint32" taichi/ --include=*.cpp --include=*.h | wc -l   # 5 (S6)
grep -rn "get_primitive_int_type\|get_primitive_real_type" $T | wc -l        # 7 (S7)
grep -rn "sizeof(int32)\|sizeof(int)\|sizeof(i32)" $T | wc -l                # 5 (S7)
grep -rnE '(^|[^A-Za-z0-9_])Expr\(' $T --include=*.h --include=*.cpp | wc -l  # 35 (S9)

# S9: exactly two types in the territory have a width-selecting constructor.
grep -rnE '\b[A-Z][A-Za-z0-9_]*\((const )?(int32|uint32)( |&| &)[a-zA-Z_]+\)' $T --include=*.h --include=*.cpp
grep -rnE '\b[A-Z][A-Za-z0-9_]*\((const )?(int64|uint64|long|std::size_t|size_t)( |&| &)[a-zA-Z_]+\)' $T --include=*.h --include=*.cpp
grep -rnE '\b[A-Z][A-Za-z0-9_]*\((const )?int( |&| &)[a-zA-Z_]+\)' $T --include=*.h --include=*.cpp
# first two return Expr and TypedConstant and nothing else; the third returns
# three types, none of which has an int64 sibling.

# S1 and S2 are disjoint, both directions:
grep -rn "PrimitiveType::i32" $T   | grep -c "PrimitiveTypeID::i32"   # 0
grep -rn "PrimitiveTypeID::i32" $T | grep -c "PrimitiveType::i32"     # 0

# taichi/inc/ contributes nothing to any of them: rerun each without it, same counts.
```

S8(b), the three header sweeps, are quoted in full in notes entries E013 and
E015.

The S5 classification of all 139 rows, the S2 classification of all 27, and the
A/B/C status computation against report 01 are all row-by-row in the notes.
Every count in this report is the length of a list that appears in one of the
two files.
