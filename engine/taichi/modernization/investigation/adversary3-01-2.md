# Adversary 3, pass 2 — territory 01, frontend IR and type system

Round three. Reports under review: `report-01-ir-types.md` (pass A, revision 3)
and `report-01b-ir-types.md` (pass B, amendment + completeness pass). Round-two
objections read: `adversary2-01-1.md`, `adversary2-01-2.md`. Notes read:
`notes-01-ir-types.md`, `notes-01b-ir-types.md`.

Everything below was derived from source in this working tree. Where I state a
count I give the enumeration it comes from, per PROJECT-PLAN section 10 item 7.
Paths are relative to `/opt/project/taichi/`.

**Headline verdict, stated first.**

- **CORRECT: yes, both.** Every load-bearing claim I tested holds at source. Two
  defects remain, one in each report, and neither changes a site or a
  conclusion. Pass A carries one arithmetic error of exactly the kind section
  10 item 7 exists to catch. Pass B carries one loose tally whose underlying
  enumeration is complete.
- **COMPLETE: no, neither — by six sites in three groups, absent from both.**
  Plus two groups absent from A alone. All are inside territory, all are the
  same class of site both reports inventory, and all are reachable by a
  mechanical sweep neither report ran to the end.
- **On the principal task, the two reports do not diverge at the headline.**
  Both say twenty-two further call lines across fourteen files, twenty-nine in
  total. Both are right. The divergence is one level down, in a sub-heading, and
  there pass B is right and pass A is wrong.

---

## 1. The principal task — the `type_check` invocation set, settled from source

### 1.1 The true set, enumerated

Derived from `grep -rn "type_check(" taichi/ir/ taichi/analysis/
taichi/transforms/`, every hit opened, then re-derived mechanically three ways
(by spelling, by file, by distinct-file count).

**Seven in the compilation driver**, `taichi/transforms/compile_to_offloads.cpp`:
`:57`, `:66`, `:193`, `:200`, `:208`, `:310`, `:391`.
`:43` and `:357` are `irpass::frontend_type_check`, a different pass, and are
correctly excluded by both reports.

**Outside the driver, twenty-two call lines in fourteen files.**

*Unqualified `type_check(root, config);` — fourteen lines, __nine__ files:*

| File | Lines |
|---|---|
| `taichi/transforms/auto_diff.cpp` | `:2407`, `:2410`, `:2417`, `:2420`, `:2430` |
| `taichi/transforms/demote_atomics.cpp` | `:241` |
| `taichi/transforms/demote_mesh_statements.cpp` | `:160` |
| `taichi/transforms/lower_access.cpp` | `:294` |
| `taichi/transforms/make_block_local.cpp` | `:391` |
| `taichi/transforms/make_mesh_block_local.cpp` | `:681` |
| `taichi/transforms/make_mesh_thread_local.cpp` | `:165` |
| `taichi/transforms/make_thread_local.cpp` | `:229` |
| `taichi/transforms/offload.cpp` | `:766`, `:773` |

5 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 2 = **14 lines**. Distinct files: **9**.

*Qualified `irpass::type_check(...)` — five lines, four files:*
`taichi/ir/ir.cpp:566`; `taichi/transforms/check_out_of_bound.cpp:248`;
`taichi/transforms/demote_operations.cpp:300`, `:303`;
`taichi/transforms/handle_external_ptr_boundary.cpp:100`. **5 lines, 4 files.**

*Queued via `DelayedIRModifier::type_check` (`taichi/ir/ir.h:617`, defined
`taichi/ir/ir.cpp:533`, drained `taichi/ir/ir.cpp:566`) — three lines, one file:*
`taichi/transforms/simplify.cpp:219`, `:344`, `:349`. **3 lines, 1 file.**

14 + 5 + 3 = **22 lines**. Distinct files 9 + 4 + 1, no overlap = **14 files**.
With the driver's seven, **29 invocation sites in territory**.

### 1.2 Which report's set is right

**Both. They are counting the same thing and they agree on it.** Pass A section
5 and pass B section 3.11 give the same twenty-two lines in the same fourteen
files, with the same three-way split by spelling, and the same twenty-nine
total. I found no line in either that is not in the true set, and no line in the
true set that is missing from either.

Pass B's alternative figure is arithmetically sound: excluding the queueing form
removes three lines and one file, giving **nineteen lines across thirteen
files**, because `simplify.cpp` contributes queued lines and nothing else.

### 1.3 Testing pass B's falsifiable set

Pass B defined its unit and listed its exclusions. I tested each exclusion.

- **`irpass::frontend_type_check`.** Distinct pass, declared
  `taichi/ir/transforms.h:65`, defined `taichi/transforms/frontend_type_check.cpp:200`.
  Correctly excluded.
- **The `Expr` / `Expression` member function.** B lists its call lines as
  `taichi/ir/expr.cpp:50` and `taichi/ir/frontend_ir.cpp:334`, `:375`, `:377`,
  `:788`, `:1041`, `:1130`, `:1838`, `:1847`. My grep returns those nine call
  lines and no others. **Exhaustive and correct.**
- **`Signature::type_check` / `Operation::type_check`.** `taichi/ir/type_system.h:246`,
  `:280`, defined `taichi/ir/type_system.cpp:114`, `:138`, called into at
  `taichi/ir/frontend_ir.cpp:635`. Correctly excluded.
- **Comments naming the pass.** `taichi/ir/statements.h:383`, `:1374`;
  `taichi/ir/frontend_ir.cpp:314`; `taichi/transforms/scalarize.cpp:260`, `:512`,
  `:623`. All six are comments. Correct.
- **Nothing outside the three territory directories.** Verified two independent
  ways. A tree-wide grep for `irpass::type_check(` returns nothing outside
  `taichi/ir/`, `taichi/analysis/`, `taichi/transforms/`. And an unqualified call
  is only possible from inside `namespace irpass`, which
  `grep -rln "namespace irpass" taichi/` shows exists in **no** file outside
  those three directories. **The set is exhaustive tree-wide.**

**One loose tally in pass B, changing no site.** B writes that "four distinct
functions in this tree share the name `type_check`". By declaration there are at
least five and arguably six: `irpass::type_check` (`taichi/ir/transforms.h:67`),
`Expr::type_check` (`taichi/ir/expr.h:110`), the virtual
`Expression::type_check` (`taichi/ir/expression.h:48`) with its overrides,
`Signature::type_check` (`taichi/ir/type_system.h:246`), `Operation::type_check`
(`taichi/ir/type_system.h:280`), and `DelayedIRModifier::type_check`
(`taichi/ir/ir.h:617`). B's own exclusion list names every one of these
positions, so nothing is missed; only the word "four" is wrong. Recorded under
section 10 item 7 because the tally does not reconcile against the list beneath
it.

### 1.4 The one genuine divergence, and pass A is wrong

Pass A section 5 heads its unqualified sub-table:

> *Unqualified `type_check(root, config)` from inside `namespace irpass`,
> **fourteen sites in ten files**:*

The table beneath it lists **nine** files. Pass B says nine. **Nine is correct**,
by the enumeration in 1.1 above, and pass A's own total of fourteen files
requires it: 9 + 4 + 1 = 14, whereas 10 + 4 + 1 = 15 would contradict A's own
headline. So A's total is right and A's sub-heading is wrong.

This is the fifth recorded instance in this project of a stated total
contradicting the list beneath it, and the second by pass A in this territory
after the "eighteen sites" over twenty rows that round two caught.

**Everything else in pass A's section 5 reconciles.** "Ten sites, four files" in
3.1 — verified. "Five sites in four files" — verified. "Three sites in one file"
— verified. "Twenty-nine invocation sites in total" — verified. "Nineteen sites
in twelve files were missing" relative to revision 2's three named non-driver
sites — verified: 22 − 3 = 19, and those nineteen lie in twelve files because
`simplify.cpp` still contributes two of them.

### 1.5 Pass A's section 3.3 count, checked separately

Not the same quantity as the `type_check` surface, and I checked it because A
states it as a heading over a list. A heads section 3.3 "thirty-three sites,
twenty-two rows". Counting the rows: twenty-two. Counting the sites row by row
(three rows carry three sites each, one carries two, one carries five, the rest
one each): 33. **A's heading reconciles against its own table.** The round-two
defect there is fixed.

---

## 2. Attack 1 — `auto_diff.cpp:1819`

**Pass B is right, and pass A does not have it.**

`taichi/transforms/auto_diff.cpp:1819`:

```cpp
        indices_values[i] = insert<ConstStmt>(TypedConstant((int32)i));
```

Inside the loop at `:1818-1820` over `num_elements`, filling `indices_values`,
which becomes the `MatrixInitStmt` at `:1828-1829` typed with the i32 tensor
type built at `:1827-1828`. It is i32 by the `TypedConstant(int32)` constructor
at `taichi/ir/type.h:582`. It is a matrix element index, exactly as B says, and
it differs from `:587`/`:646`/`:787`/`:846` only in spelling — which is why a
grep shaped around `insert_const(PrimitiveType::i32, ...)` misses it.

`grep -n "PrimitiveType::i32\|(int32)\|int32(" taichi/transforms/auto_diff.cpp`
returns exactly twelve lines: `:576`, `:587`, `:639`, `:646`, `:776`, `:787`,
`:839`, `:846`, `:1770`, `:1819`, `:1828`, `:1857`. Nine index constructions and
the three `val_int32()` reads. **B's sweep closes the file; A's does not.**

Pass A's inventory has `:587`, `:639`, `:646`, `:787`, `:839`, `:846`, `:1828`,
`:1857` — eight. `grep -n "1819" report-01-ir-types.md` returns nothing.
A's section 3.3 total of thirty-three should be thirty-four.

---

## 3. Attack 2 — the three `lower_ast.cpp` allocas

**Pass B is right on all three characterisations. Pass A is right to include
them and wrong in how it labels them. A's count is not inflated.**

`grep -n "PrimitiveType::i32" taichi/transforms/lower_ast.cpp` returns exactly
four lines: `:168`, `:330`, `:348`, `:361`. Both reports state this and both are
right.

Read in context:

| Site | What it is | Evidence |
|---|---|---|
| `:168` | While-loop mask | `:169` is `new_while->mask = mask.get();` |
| `:330` | **Index.** Range-for induction variable | `:331` `auto loop_var = fctx.back_stmt();`, `:332` binds it to `stmt->loop_var_ids[0]` |
| `:348` | While-loop mask | `:349` is `new_while->mask = mask.get();` |
| `:361` | **Neither.** Anonymous, result never captured | `stmt->insert_before_me(std::make_unique<AllocaStmt>(PrimitiveType::i32));` at `:360-361`, no binding. The real mask is the one created at `:348` and moved in at `:365` |

So pass B is right that only `:330` is an index, right that `:168` and `:348`
are masks, and right that `:361`'s result is never captured. Its argument that a
width change to indices does not imply one to masks follows.

**Ruling on pass A.** A's section 3.3 is headed "Hardcoded i32 on statement and
expression types", not "index-width sites". A mask alloca is a hardcoded i32
statement type. Under A's own stated unit the three sites belong, and A's count
of thirty-three is **not** inflated by including them. But A's row text reads
"Three further `AllocaStmt(PrimitiveType::i32)`, all loop-mask allocas", and
`:361` is not a mask — the mask in that block is `:348`. That is a
mischaracterisation of one site, and more importantly A gives the reader no way
to separate the one index from the three non-indices in a table whose other
rows are overwhelmingly index and address types. B's treatment is the more
useful one and A should adopt the distinction without dropping the sites.

---

## 4. Attack 3 — `make_mesh_block_local.cpp:526`

**Both treatments are defensible. Neither is a defect.**

Source, `:522-527`:

```cpp
    mapping_snode_ = (offload->mesh->index_mapping
                          .find(std::make_pair(element_type, conv_type))
                          ->second);
    // mapping_data_type_ = mapping_snode_->dt.ptr_removed();
    mapping_data_type_ = PrimitiveType::i32;
    mapping_dtype_size_ = data_type_size(mapping_data_type_);
```

Both reports' description is exact: the derived form is present and commented
out at `:525`, the literal is at `:526`, and `:527` sizes from the literal.

Pass A files it in section 3.7, "Widths hardcoded rather than derived". That is
the sharpest available category for it: the commented-out `:525` is direct
evidence that the derived form once existed and was replaced, which is precisely
what that section is about, and A says so.

Pass B files it in section 3.1, "Index and address values pinned to i32 by a
pass". Also true: `mapping_data_type_` is the element type of a mesh index
mapping, so the site pins the width of an index value.

The site satisfies both units. B's own section 3.5 is titled "Hardcoded widths,
not derived" and would have taken it too; choosing 3.1 is a preference, not an
error. The only cost is cross-report navigation, and both reports name the line
and the `:525` comment, so a reader can find it either way.

---

## 5. Attack 4 — `SNode::get_total_num_elements_towards_root`

**Both halves verified. One qualifier in pass A's wording is load-bearing and A
carries it.**

**No callers, tree-wide.** `grep -rn "get_total_num_elements_towards_root" .`
over the entire repository returns one line of source, `taichi/ir/snode.h:310`,
the definition itself. Every other hit is a file under
`modernization/investigation/`. Nothing in `taichi/`, nothing in `tests/`,
nothing in `python/`. **Verified.**

**The `(int)` narrowing.** Source, `taichi/ir/snode.h:306-315`:

```cpp
  int64 max_num_elements() const {
    return num_cells_per_container;
  }

  int64 get_total_num_elements_towards_root() const {
    int64 total_num_elemts = 1;
    for (auto *s = this; s != nullptr; s = s->parent)
      total_num_elemts *= (int)s->max_num_elements();
    return total_num_elemts;
  }
```

`max_num_elements()` returns `int64` (it returns `num_cells_per_container`,
`int64` at `taichi/ir/snode.h:97`), and every factor is narrowed to `int` at
`:313` before it enters the accumulator. **Verified.**

**The qualifier matters.** The `int64` accumulator is not useless: it holds the
cross-level product without overflowing even when several 32-bit factors are
multiplied together. What it cannot recover is any single container whose count
exceeds 2^31, because that factor is already truncated before it arrives. Pass A
states this correctly — "the `int64` type of the accumulator buys nothing above
2^31 **in any single container**". Stripped of that clause the claim would be
wrong. A carries it; B (escalation 12) states only that the narrowing exists and
declines to judge, which is also defensible.

Both reports record the function as latent rather than dropping it, on different
grounds — A on the planner's ruling and plan section 4.6, B on standing
instruction 3. Both land in the same place.

---

## 6. Attack 5 — `constant_fold.cpp:115`

**Pass B is right. Verified in full, including the absence of any upstream check
inside the function or its caller.**

`taichi/transforms/constant_fold.cpp:64-65`:

```cpp
    // Type check should have been done at this point.
    auto dt = lhs->val.dt;
```

`HANDLE_INTEGRAL_BINARY` at `:111-125`:

```cpp
    if (dt->is_primitive(PrimitiveTypeID::i32)) {                              \
      res = TypedConstant(                                                     \
          dst_type, PREFIX(lhs->val.val_int32() OP_CPP rhs->val.val_int32())); \
    } else if (dt->is_primitive(PrimitiveTypeID::i64)) {                       \
```

The guard at `:113` tests `dt`, which is `lhs->val.dt` and nothing else. Line
`:115` calls `val_int32()` on **both** operands. `TypedConstant::val_int32()`
(`taichi/ir/type.cpp:462-465`) opens with `TI_ASSERT(get_data_type<int32>() == dt)`
against the operand's own `dt`, and `TI_ASSERT` is unconditional
(`taichi/common/logging.h:100-107`). **So an i32 left operand with a wider right
operand aborts at `:115`.**

I looked for a guard that would make the pair impossible and found none:

- `get_scalar_value_to_replace(BinaryOpStmt *, ConstStmt *lhs, ConstStmt *rhs,
  DataType)` at `:48-147` contains no comparison of `lhs->val.dt` against
  `rhs->val.dt`. The only earlier type test is the `pow` check at `:55-62`,
  which reads `rhs->ret_type` and does not constrain width.
- `is_good_type` (`:32-46`) is called only on the **unary** path (`:194`), never
  on the binary one.
- The caller, `visit(BinaryOpStmt *)` at `:149-182`, checks only that both
  operands are `ConstStmt` (`:153`) or both `MatrixInitStmt` (`:161`).

So the invariant rests entirely on `irpass::type_check` having unified the
operand types upstream — which is exactly what the in-tree comment at `:64`
asserts and does not prove. B's characterisation, "a consequence of a width
change to weigh, not a defect today", is the right weight. Both reports agree
`:115` is not an abort site under today's invariant; only B records the
residual, and the residual is real.

---

## 7. Independent citation sampling

I opened and checked, in both reports where both carry them, and against source
in every case:

`taichi/transforms/type_check.cpp` `:114`, `:119`, `:122-125`, `:147-156`,
`:154`, `:160-161`, `:170`, `:171`, `:234`, `:459-462`, `:467`, `:471`, `:475`,
`:521`, `:525`, `:567` — all correct in both.

`taichi/ir/snode.cpp` `:14-35` (only `num_elements_from_root` propagated, at
`:21-23`), `:83`, `:84` (plain assignment, not accumulation), `:89`, `:92`,
`:95-100`, `:101`, `:174-177` (`shape_along_axis` returns
`num_elements_from_root` at `:176`) — all correct in both. The per-container
versus root-to-leaf finding that round two forced onto both reports is right,
and both now state it correctly.

`taichi/transforms/check_out_of_bound.cpp:123-126` — `int size_i =
snode->shape_along_axis(i);` at `:123`, `ConstStmt(TypedConstant(upper_bound_i))`
at `:125-126`. Correct in both. The join both reports now make — the bound
checked against every SNode field index is an unguarded root-to-leaf `int`
product — holds.

`taichi/ir/statements.h` `:456`, `:1260`, `:1280`, `:1370`, `:1415-1416`,
`:1439`, `:1684`, `:1787`, `:2064`, `:2083`; `taichi/ir/snode.h` `:41`, `:45`,
`:49`, `:78`, `:80`, `:81`, `:99`; `taichi/ir/type_factory.cpp:204`;
`taichi/ir/type_utils.cpp:30`, `:35`; `taichi/inc/rhi_constants.inc.h:14`;
`taichi/ir/type.h:530-532`, `:582`; `taichi/transforms/utils.cpp:5`, `:14`,
`:17`; `taichi/transforms/demote_dense_struct_fors.cpp:18`, `:26`, `:29`, `:35`;
`taichi/analysis/arithmetic_interpretor.cpp:98-109` (int64_t at `:99` and `:106`,
stored under `ret_type` at `:108`); `taichi/analysis/offline_cache_util.cpp:110-112`;
`taichi/analysis/value_diff.cpp:81-87`; `taichi/analysis/bls_analyzer.cpp:27`;
`taichi/transforms/simplify.cpp:176`, `:187`, `:222`, `:239`, `:248-249`,
`:251`, `:261`, `:270-274`; `taichi/transforms/lower_ast.cpp:270-274`;
`taichi/ir/frontend_ir.cpp:731-740`;
`taichi/transforms/make_cpu_multithreaded_range_for.cpp:68`, `:70`, `:72`,
`:81`, `:84`, `:90`, `:93`; `taichi/transforms/compile_to_offloads.cpp:198`;
`taichi/transforms/offload.cpp:236-237`, `:244`. **All correct in both reports.**

Two counts I re-derived rather than accepted:

- **Ten cast-insertion sites in four files.**
  `grep -rn "cast_type = PrimitiveType::i32"` over the territory returns eight,
  all mesh: `demote_mesh_statements.cpp:16`, `:54`, `:131`;
  `make_mesh_thread_local.cpp:106`, `:114`;
  `make_mesh_block_local.cpp:212`, `:376`, `:399`. Plus the two
  `insert_type_cast_before` calls at `type_check.cpp:154`, `:461` = **ten sites,
  four files**. Pass A's figure verified.
- **Fourteen `val_int32()` call sites tree-wide, twelve of them aborts.**
  Thirteen in territory (`control_flow_graph.cpp:462`; `offload.cpp:109`,
  `:117`; `constant_fold.cpp:115`; `auto_diff.cpp:576`, `:776`, `:1770`;
  `scalarize.cpp:162`, `:1097`, `:1120`, `:1147`, `:1231`, `:1311`) plus
  `codegen/llvm/codegen_llvm.cpp:1054` outside it. Minus `constant_fold.cpp:115`
  = twelve aborts. **Both reports verified.**
- **Eight byte-offset narrowings.** `make_block_local.cpp:157`, `:334` use
  `TypedConstant((int32)...)`; `make_mesh_block_local.cpp:118`, `:265`, `:301`
  use the functional spelling `int32(...)`; `:80`, `:200`, `:575` use brace-init
  `TypedConstant{(int32)...}`, which is why a naive grep finds only some of
  them. Both reports have all eight. **Pass A's note about the spelling
  difference is correct.**

**I found no citation fault in either report.** That is the third round in which
citation accuracy has held for pass B and the second for pass A since its
revision-3 attribution sweep.

---

## 8. Completeness — what is still missing, from BOTH reports

Both reports head their section 3 "Complete inventory of 32-bit width
assumptions in this territory". I tested that claim with the sweep neither
report ran to the end: `grep -rn "PrimitiveType::i32" taichi/ir/
taichi/analysis/ taichi/transforms/` returns **64 lines**. Pass B closed
`auto_diff.cpp` this way and stopped there; pass A closed `lower_ast.cpp` this
way and stopped there. Six of the sixty-four are inventoried by neither.

### 8.1 `taichi/ir/frontend_ir.cpp:1160` — the dynamic-SNode append index

This is the material one. `SNodeOpExpression::flatten`, `append` branch:

```cpp
  } else if (op_type == SNodeOpType::append) {
    auto alloca = ctx->push_back<AllocaStmt>(PrimitiveType::i32, dbg_info);
    auto addr = ctx->push_back<SNodeOpStmt>(SNodeOpType::allocate, snode, ptr,
                                            alloca, dbg_info);
```

and at `:1169` the expression's value is `ctx->push_back<LocalLoadStmt>(alloca, ...)`.

The alloca receives the index of the newly appended element. I traced it out of
the territory to confirm rather than assume:
`taichi/codegen/llvm/codegen_llvm.cpp:1367-1375` passes `llvm_val[stmt->val]`
straight into the runtime call, and the runtime signature is
`Ptr Dynamic_allocate(Ptr meta_, Ptr node_, i32 *len)` at
`taichi/runtime/llvm/runtime_module/node_dynamic.h:61`.

So this is an SNode element index **born i32 in the frontend, with a matching
i32 in the device runtime** — the same class as `frontend_ir.cpp:151-152` and
`lower_ast.cpp:330`, which both reports flag as load-bearing birth sites, and
with the same device-side-mirror property both reports flag for
`PhysicalCoordinates`. Both reports inventory `type_check.cpp:114`, the
`SNodeOpStmt` default i32 pin that is this site's downstream half, and neither
inventories the frontend half. `grep -n "1160" ` returns nothing in either
report.

### 8.2 `taichi/transforms/scalarize.cpp:471`, `:473` — i32 `MatrixPtrStmt` offsets

```cpp
      auto zero =
          std::make_unique<ConstStmt>(TypedConstant(PrimitiveType::i32, 0));
      auto one =
          std::make_unique<ConstStmt>(TypedConstant(PrimitiveType::i32, 1));
```

consumed at `:475-478` as the offsets of two `MatrixPtrStmt`s in the f16 atomic
split. Same class as the `auto_diff.cpp` index constructions both reports added
in the amendment pass, and it reproduces **exactly the asymmetry both cited as
the reason to add those**: `scalarize.cpp`'s six `val_int32()` abort sites are
inventoried in both reports while its two matrix-index constructions are in
neither. Consequence is as low as the `auto_diff` ones — bounded at two
elements — and by the standard both reports adopted, low consequence is a
qualification to state, not a reason to omit.

### 8.3 `taichi/ir/frontend_ir.cpp:1216`, `:1233`, `:1249` — texture indices hard-required i32

`TextureOpExpression::type_check` rejects any texture coordinate argument that
is not exactly i32, in three branches — `kFetchTexel`, `kLoad`, `kStore`:

```cpp
      if (arg_type != PrimitiveType::i32) {
        ErrorEmitter(TaichiTypeError(), this, fmt::format(... "all "
                     "arguments must be i32", arg_type->to_string()));
      }
```

Lowest weight of the three groups: these are texture coordinates, and plan
section 1.3 makes rendering data irrelevant. But they are a **hard i32
requirement on an index argument in the frontend**, and they qualify a claim
both reports make prominently — pass B headline 3 and pass A section 6, that the
frontend does not restrict the type of an index expression and
`IndexExpression::type_check` checks only `is_integral`. That is true of field
and array indices; it is not true of every index the frontend accepts. Standing
instruction 3 forbids either report deciding these are unnecessary.

### 8.4 Absent from pass A alone

- **`taichi/transforms/auto_diff.cpp:1819`** — section 2 above. B has it.
- **`taichi/transforms/lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477`** —
  `auto const_stmt = std::make_unique<ConstStmt>(TypedConstant(i));`, matrix
  element indices, i32 by the `TypedConstant(int32)` constructor at
  `taichi/ir/type.h:582`. B carries these as a trailing note in its section 3.3.
  Pass A cites `lower_matrix_ptr.cpp` only at `:536-540`
  (`grep -n "lower_matrix_ptr" report-01-ir-types.md` returns `:455`, `:739`,
  `:865`, `:1060` — the byte-offset finding, the pass ordering, the opened-files
  list and escalation 5). Same low-consequence class; same asymmetry.

---

## 9. Whether the round-two objections were met

Checked item by item against `adversary2-01-1.md` section 7 and
`adversary2-01-2.md` section 7.4.

**Pass A, five demanded amendments — all five done.** `lower_ast.cpp:270-274`
moved out of section 3.6 into the new 3.12 as an IR-level product; section 4's
third row replaced with `AxisExtractor::num_elements_from_root` with the join to
`check_out_of_bound.cpp:123-126` stated; the 3.3 heading reconciled and the two
non-`ret_type` rows flagged; the `visit(GetChStmt *)` span corrected to
`simplify.cpp:251-273`; and `frontend_ir.cpp:731-740`,
`type_check.cpp:170-171`, `make_mesh_block_local.cpp:526` all added. The
`make_cpu_multithreading_loop` half of the gate at
`compile_to_offloads.cpp:198`, which adversary2 01-2 raised separately, is
corrected in the body of section 3.4. The optional `auto_diff.cpp` row is added
but short by `:1819`.

**Pass B, three demanded amendments — all three done.**
`constant_fold.cpp:115` reclassified as guarded dispatch with the counts moved
to twelve and fifteen; the 3.4 sub-table rewritten from three sites to four
distinct quantities with scopes and guards; the `visit(GetChStmt *)` span
corrected. B additionally closed `auto_diff.cpp` on its own sweep and found
`:1819`, which neither round-two adversary had.

**The shared gap adversary2 01-2 raised — the `type_check` re-run surface — is
now closed in both, at the correct figures**, and adversary2 01-2's own figure
of "at least fifteen further re-runs" is superseded by the twenty-two both
reports now carry, which section 1.1 confirms.

---

## 10. Verdicts

### 10.1 CORRECT

**Yes, both.** I tried to break the load-bearing structure and could not. The
central finding survives a third round in both reports and in my own reading:
the type system can represent i64 and models no index type and no address type;
`PointerType` (`taichi/ir/type.h:177-209`) carries no width; the 32-bit
assumption sits on the index, shape and stride side while byte offsets are
already `size_t` except at `lower_matrix_ptr.cpp:536-540`; addressing is
hierarchical, so a large structure does not by itself produce a large frontend
value; the SPIR-V Int64 capability is declared inside this territory at
`taichi/inc/rhi_constants.inc.h:14` and enforced at SPIR-V codegen, with no link
from the type layer; and the coercion is re-applied at twenty-nine points, not
seven.

Two defects, neither changing a site or a conclusion:

1. **Pass A:** "fourteen sites in ten files" over a nine-file table, section 5.
   Should read nine. Section 1.4.
2. **Pass B:** "four distinct functions ... share the name `type_check`" where
   the declarations number at least five. The exclusion enumeration beneath it
   is complete, so no site is affected. Section 1.3.

### 10.2 COMPLETE

**No, neither — but the gap is smaller than round two's and is a closure, not an
investigation.**

Missing from **both**, six sites in three groups, all in territory, all the same
class as sites both already inventory:

| Site | Class | Weight |
|---|---|---|
| `taichi/ir/frontend_ir.cpp:1160` | i32 alloca receiving the dynamic-SNode append index; device side is `i32 *len` at `taichi/runtime/llvm/runtime_module/node_dynamic.h:61` | **Material.** Same class as the two birth sites both call load-bearing |
| `taichi/transforms/scalarize.cpp:471`, `:473` | i32 `MatrixPtrStmt` offset constants, consumed `:475-478` | Low, and identical to the `auto_diff` group both added for the sake of symmetry |
| `taichi/ir/frontend_ir.cpp:1216`, `:1233`, `:1249` | Texture coordinate arguments hard-required i32 with `TaichiTypeError` | Low, but it qualifies a headline claim in both about the frontend not restricting index types |

Missing from **pass A** alone: `taichi/transforms/auto_diff.cpp:1819`, and
`taichi/transforms/lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477`.

Pass A's section 3.3 total would move from thirty-three to thirty-four on
`:1819` alone, and further on the shared groups if they are filed there.

### 10.3 Is general consensus reached?

**On substance, yes. On the completeness claim as each report words it, not
quite.** I record the distinction plainly rather than resolving it, since the
arbiter seat is the planner's.

What is settled and I could not shake: every structural finding in section 10.1;
the per-container versus root-to-leaf distinction and its join to the bound at
`check_out_of_bound.cpp:123-126`; the twenty-nine-point coercion surface; twelve
aborts and three structural aborts; ten cast insertions in four files, eight of
them mesh; the three whole-extent formation sites with three distinct failure
modes; and the twelve escalations, which the two reports raise in different
order and identical substance.

What is not: both reports assert a *complete* inventory, and six in-territory
sites falsify that assertion for both. They are line-level additions with no
investigation behind them — I have given every file, line and consuming site
above. My recommendation, which the planner may take or leave: treat section 8
as a closing edit against both reports rather than a fourth adversarial round,
because nothing in it disturbs a finding, an escalation or a count that anyone
has argued over, and because a fourth round would be spent re-reading what three
rounds have already settled.

I did not look for reasons to extend this. I ran one sweep neither report
finished and reported what it returned.

---

## 11. Escalations

Unresolved judgements arising from this pass. Recorded, not decided.

1. **Do the texture-op index checks belong in territory 01's inventory?**
   `taichi/ir/frontend_ir.cpp:1216`, `:1233`, `:1249` are in territory and are
   hard i32 requirements on index arguments. Plan section 1.3 makes rendering
   data irrelevant, which is an argument for excluding them, but standing
   instruction 3 forbids either report deciding so unilaterally, and neither
   report mentions them at all — so the exclusion has not been made, it has been
   missed. **Planner.**

2. **Is `frontend_ir.cpp:1160` in scope for item 6.2?** It is a dynamic-SNode
   element index, i32 on both the frontend and runtime sides. Whether item 6.2's
   width change reaches `ti.append`'s return value depends on the answer to pass
   A's escalation 2 and pass B's escalation 7, which ask what "the address"
   means. **Planner.**

3. **The two reports file `make_mesh_block_local.cpp:526` in different
   sections** (A section 3.7, B section 3.1), both defensibly, per section 4
   above. When the reports are read together a reader may double-count it.
   Presentational, and the planner should know before the joint read. **Planner.**

4. **Whether a fourth adversarial round is warranted** given section 10.3. My
   recommendation is a closing edit rather than a round, but the arbiter seat is
   not mine. **Planner.**

---

## 12. Divergence from `adversary3-01-1.md`

`adversary3-01-1.md` does not exist in
`/opt/project/taichi/modernization/investigation/` at the time of writing.
Recorded as instructed. If it lands later, this section is where the comparison
belongs.

**Appended after `adversary3-01-1.md` landed.** It exists, 633 lines, and I have
read it in full. We were blind to each other while working. We agree on the
principal task and on all five attack items, in every ruling and in most
wording. What follows is only where we differ, and which of us the source
supports.

### 12.1 Where we agree, stated once

The `type_check` set: both of us derived twenty-two lines in fourteen files
outside the driver, twenty-nine in total, with the same three-way split. Both of
us found report A's "fourteen sites in ten files" sub-heading wrong and nine
correct, both by the same reconciliation against A's own total. Both of us find
report B's set falsifiable and surviving the test. Attack 1: `auto_diff.cpp:1819`
is real, B has it, A does not. Attack 2: B's characterisation of the three
`lower_ast.cpp` allocas is right, A's inclusion of them is defensible under A's
own heading, A's label on `:361` is wrong, A's count is not inflated. Attack 3:
both filings defensible. Attack 4: both halves verified, and both of us
independently flagged that the qualifier "in any single container" is
load-bearing and that A carries it. Attack 5: B is right, and the residual is
real. Neither of us found a citation fault in either report.

### 12.2 Where adversary 3 pass 1 is right and I was short

Three groups it has and I did not. I have now opened each and the source
supports it on all three:

- **`taichi/transforms/lower_ast.cpp:151`, `:177`, `:333`, `:363`** — the four
  `TypedConstant((int32)...)` tokens in that file. Report A closes the file on
  `PrimitiveType::i32` tokens only, which is a true statement about a narrower
  sweep than the file needs; report B carries all four in its section 3.9 table.
  Missing from A.
- **`taichi/ir/snode.h:98`** — `int chunk_size{0};`. In report A's section 3.5,
  absent from report B. Verified at source.
- **`taichi/ir/frontend_ir.h:712`** — `int low, high;` on
  `RangeAssumptionExpression`. In report A's section 3.5, absent from report B.
  Verified at source.

Its data-flow probe on attack 5 also goes further than mine. It traces
`visit(BinaryOpStmt *)` at `taichi/transforms/type_check.cpp:290` unifying
operands through `cast()`, and stops short of proving the mixed-width pair
unreachable. I confirm the mechanism: `cast()` inserts an
`insert_type_cast_after` when widths differ, which would make the right operand a
`UnaryOpStmt` and fail `ConstantFold`'s `is<ConstStmt>` gate before `:115`. One
correction of no consequence: `cast()` spans `:282-288`, not `:283-289`; the
visitor line `:290` is right. Its conclusion — inferred in both directions, and
correctly left in B's escalation register — is the right disposition and I adopt
it.

### 12.3 Where I diverge, and the source supports me

**The completeness verdict.** `adversary3-01-1.md` section 9.2 rules "report B
complete on the surface it defines; report A correct but short", and its section
8 records gaps running in both directions but names none that is missing from
both. My section 8 names three groups, six sites, **absent from both reports**:

| Site | Present in report A? | Present in report B? |
|---|---|---|
| `taichi/ir/frontend_ir.cpp:1160` | no | no |
| `taichi/transforms/scalarize.cpp:471`, `:473` | no | no |
| `taichi/ir/frontend_ir.cpp:1216`, `:1233`, `:1249` | no | no |

I verified the absences by grep over both report files before writing this, and
`adversary3-01-1.md` names none of the three either — `grep -n
"1160\|scalarize.cpp:471\|:1216"` over it returns nothing.

The source supports me on the existence and class of all three, and the first is
material rather than marginal. `taichi/ir/frontend_ir.cpp:1160` is
`ctx->push_back<AllocaStmt>(PrimitiveType::i32, dbg_info)` in the `append` branch
of `SNodeOpExpression::flatten`; it receives the appended element's index from
the `SNodeOpStmt(SNodeOpType::allocate, ...)` at `:1161-1162`, and that value is
the expression's result via the `LocalLoadStmt` at `:1169`. The device side
matches it exactly: `Ptr Dynamic_allocate(Ptr meta_, Ptr node_, i32 *len)` at
`taichi/runtime/llvm/runtime_module/node_dynamic.h:61`, reached from
`taichi/codegen/llvm/codegen_llvm.cpp:1367-1375`. That is a dynamic-SNode element
index born i32 in the frontend with a matching i32 in the runtime — the same
class as `frontend_ir.cpp:151-152` and `lower_ast.cpp:330`, which **both** reports
call load-bearing birth sites, and with the same device-side-mirror property both
reports flag for `PhysicalCoordinates`. Both reports already inventory
`type_check.cpp:114`, this site's downstream half, and neither has the frontend
half.

So report B is **not** complete on the surface it defines. B's section 3 is
headed "Complete inventory of 32-bit width assumptions in my territory", and
`frontend_ir.cpp:1160` is a 32-bit width assumption in that territory.

**The reason we differ is method, not judgement.** Adversary 3 pass 1 sampled
citations and checked the five attack items, as did I. I additionally ran
`grep -rn "PrimitiveType::i32" taichi/ir/ taichi/analysis/ taichi/transforms/`
to exhaustion — sixty-four lines — and worked every one. Report B closed
`auto_diff.cpp` that way and report A closed `lower_ast.cpp` that way, but
neither ran it over the whole territory, and six lines fall through. That is a
mechanical check, per plan section 10 item 7, and it is repeatable in one
command.

### 12.4 Net effect on the joint verdict

We do not contradict each other on **CORRECT**: both reports, yes, with A's
"ten files" and B's "four distinct functions" as the two harmless defects, and
adversary 3 pass 1 did not raise the second.

We differ on **COMPLETE**. The union of our two section 8s is the answer, and it
is symmetric:

- Missing from **A** only: `auto_diff.cpp:1819`; `lower_matrix_ptr.cpp:189`,
  `:232`, `:424`, `:477`; `lower_ast.cpp:151`, `:177`, `:333`, `:363`.
- Missing from **B** only: `snode.h:98`; `frontend_ir.h:712`.
- Missing from **both**: `frontend_ir.cpp:1160`; `scalarize.cpp:471`, `:473`;
  `frontend_ir.cpp:1216`, `:1233`, `:1249`.

We reach the same recommendation by different routes, and I hold to mine: **do
not send this territory back for a fourth investigative pass.** Every remaining
item is a named line with a named class. Where adversary 3 pass 1 asks for four
edits to one report, the corrected list is edits to both, and my section 11 item
1 escalates whether the texture-op group belongs in the inventory at all. Which
of the three shared groups the planner requires is the arbiter's call; the two
lower-weight groups are defensibly out of scope on plan section 1.3 grounds, and
`frontend_ir.cpp:1160` is not.
