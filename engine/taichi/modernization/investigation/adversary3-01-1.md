# Adversary 3, pass 1 — territory 01, frontend IR and type system

Round three. Adversary 1 of 2. Scope: whether `report-01-ir-types.md` (pass A)
and `report-01b-ir-types.md` (pass B) are now CORRECT and COMPLETE.

Read in full before writing: `modernization/PROJECT-PLAN.md`, both reports, both
round-two objections (`adversary2-01-1.md`, `adversary2-01-2.md`), both notes
files' amendment and completeness entries.

Every figure below is derived from my own enumeration against source, not
adopted from either report. Paths are relative to `/opt/project/taichi/`.

---

## 0. Verdict in one paragraph

Both reports are **CORRECT**. I found no false load-bearing claim in either, and
no citation fault in a sample of roughly seventy cited lines opened individually
across both files. On **COMPLETENESS** the answer splits. The disputed
`type_check` count is not a divergence at all: both reports have arrived at the
same true set, and I re-derived it independently and got the same set again.
Report A carries one wrong file sub-count inside that section, which contradicts
its own stated total — the fifth instance of the pattern plan section 10 item 7
was written for. Report B is right on every one of the five attack items. Report
A's index-construction inventory is short by five sites, four of which neither
adversary nor either report has previously named. Report B is short by two
structural `int` fields that report A has. **General consensus is reached on
substance and is one four-line edit away on report A's arithmetic.** Detail
below; nothing here needs a fourth round of investigation.

---

## 1. The principal task: the `type_check` invocation set, settled from source

### 1.1 The two reports are not counting different things

Report A, section 5: "Twenty-two further invocation sites across fourteen
files", plus seven in `taichi/transforms/compile_to_offloads.cpp`, "twenty-nine
invocation sites in total".

Report B, section 3.11: "Twenty-two call lines across fourteen files" outside
the driver, fourteen of them unqualified, plus the same seven driver calls named
separately.

**These are the same set, stated with a different total line drawn.** A folds
the seven driver calls into a grand total of twenty-nine; B keeps them separate
and reports twenty-two. Neither report has seen the other's figures and both
landed on twenty-two outside the driver and fourteen files. There is no
divergence on the set to settle. What there is, is one wrong sub-count in A.

### 1.2 My own derivation

Two greps, because the pass is declared inside `namespace irpass`
(`taichi/ir/transforms.h:67`) and passes that themselves live in that namespace
call it unqualified:

```
grep -rn "irpass::type_check(" taichi/
grep -rn "type_check(" taichi/ir/ taichi/analysis/ taichi/transforms/
```

The second returns 112 hits. Every one was classified by opening it. The pass
`irpass::type_check(IRNode *, const CompileConfig &)` is invoked or queued at
these lines and no others:

**Driver, `taichi/transforms/compile_to_offloads.cpp`, seven lines:** `:57`,
`:66`, `:193`, `:200`, `:208`, `:310`, `:391`.

**Outside the driver, twenty-two lines across fourteen files:**

| Form | Lines | Files |
|---|---|---|
| Qualified `irpass::type_check(node, config)` — `taichi/ir/ir.cpp:566`; `taichi/transforms/check_out_of_bound.cpp:248`; `taichi/transforms/demote_operations.cpp:300`, `:303`; `taichi/transforms/handle_external_ptr_boundary.cpp:100` | 5 | 4 |
| Unqualified `type_check(root, config)` — `taichi/transforms/auto_diff.cpp:2407`, `:2410`, `:2417`, `:2420`, `:2430`; `taichi/transforms/demote_atomics.cpp:241`; `taichi/transforms/demote_mesh_statements.cpp:160`; `taichi/transforms/lower_access.cpp:294`; `taichi/transforms/make_block_local.cpp:391`; `taichi/transforms/make_mesh_block_local.cpp:681`; `taichi/transforms/make_mesh_thread_local.cpp:165`; `taichi/transforms/make_thread_local.cpp:229`; `taichi/transforms/offload.cpp:766`, `:773` | 14 | **9** |
| Queued via `DelayedIRModifier::type_check` (`taichi/ir/ir.h:617`, defined `taichi/ir/ir.cpp:533-534`, drained `taichi/ir/ir.cpp:566`) — `taichi/transforms/simplify.cpp:219`, `:344`, `:349` | 3 | 1 |
| **Total outside driver** | **22** | **14** |

Checked mechanically: the 22 lines written to a file, `wc -l` gives 22,
`cut -d: -f1 | sort -u | wc -l` gives 14, and each line re-printed from source to
confirm it is a call and not a declaration, definition or comment. The three file
groups are 4, 9 and 1, which are disjoint and sum to 14.

**Both reports' figure of twenty-two lines across fourteen files is correct.**

### 1.3 The one real defect: report A's middle sub-count

Report A, section 5, writes:

- "*Qualified `irpass::type_check`, five sites in four files*" — correct.
- "*Unqualified `type_check(root, config)` from inside `namespace irpass`,
  **fourteen sites in ten files***" — the line list under it is correct and holds
  fourteen lines, but those lines are in **nine** files, not ten. The nine are
  `auto_diff.cpp`, `demote_atomics.cpp`, `demote_mesh_statements.cpp`,
  `lower_access.cpp`, `make_block_local.cpp`, `make_mesh_block_local.cpp`,
  `make_mesh_thread_local.cpp`, `make_thread_local.cpp`, `offload.cpp`.
- "*Queued through `DelayedIRModifier::type_check` ... three sites in one file*"
  — correct.

4 + 10 + 1 = 15, against A's own stated total of fourteen files two paragraphs
above. The total is right; the sub-count is wrong. This is precisely the failure
plan section 10 item 7 names, and it is the fifth occurrence in this project.
**Fix: "ten files" becomes "nine files".** One word.

A's closing sentence in the same section, "Nineteen sites in twelve files were
missing", is *correct* and I checked it separately: 22 minus the 3 revision 2
named leaves 19, and those 19 fall in 12 files once `simplify.cpp` is counted as
holding two of the missing three. No change needed there.

### 1.4 Report B's set is falsifiable, and it survives the test

B stated its unit, inclusions and exclusions explicitly and invited the test. I
ran it.

**Unit.** "A source line that textually invokes the IR pass
`irpass::type_check(IRNode *, const CompileConfig &)`, or queues it." Verified
usable: every one of my 22 lines matches it, and applying it to the 112 raw hits
partitions them without ambiguity.

**Exclusions, each checked:**

- Driver's seven, counted separately. Confirmed at the seven lines above.
- `irpass::frontend_type_check`, a different pass. Confirmed distinct.
- `Expr` / expression member `type_check(const CompileConfig *)`. B lists
  `taichi/ir/expr.cpp:50` and `taichi/ir/frontend_ir.cpp:334`, `:375`, `:377`,
  `:788`, `:1041`, `:1130`, `:1838`, `:1847`. **All nine present, all member
  calls.** Not the pass.
- `Signature` / `Operation::type_check` on argument type vectors:
  `taichi/ir/type_system.h:246`, `:280`, `taichi/ir/type_system.cpp:140`, and
  the call at `taichi/ir/frontend_ir.cpp:635`. All present, all a different
  function taking `std::vector<DataType>`.
- Comments naming the pass: `taichi/ir/statements.h:383`, `:1374`,
  `taichi/ir/frontend_ir.cpp:314`, `taichi/transforms/scalarize.cpp:260`, `:512`,
  `:623`. All six are comment lines. Confirmed.
- "Anything outside the three territory directories. Verified empty." **I
  re-ran this myself.** A tree-wide grep returns seven hits outside
  `taichi/ir/`, `taichi/analysis/`, `taichi/transforms/`:
  `taichi/python/export_lang.cpp:1151`, `:1156`, `:1161` and
  `taichi/program/program.cpp:281`, `:307`, `:313`, `:317`. **Every one is the
  `Expr`/expression member function** (`args.back()->type_check(nullptr)`,
  `expr.type_check(&this->compile_config())`), not the pass. B's claim holds.

**B's stated alternative** — nineteen lines across thirteen files if the queueing
form is excluded — is arithmetically and factually right: 22 − 3 = 19, and
dropping `simplify.cpp`, whose only contribution is the three queued lines,
leaves 13.

**One loose piece of wording in B, not a fault in the count.** B says "four
distinct functions in this tree share the name `type_check`, while only one is
the pass." Grouped by family that is right, and the exclusion list enumerates
exactly four families. Counted strictly as functions it is more: `Signature::type_check`
(`taichi/ir/type_system.cpp:114`) and `Operation::type_check`
(`:138`) are two, and `Expr::type_check` (`taichi/ir/expr.cpp:49`) and the
virtual `Expression::type_check` (`taichi/ir/expression.h:48`) are two more. The
exclusion list is complete either way; only the headline number is loose.

### 1.5 Ruling on the principal task

**Both reports' set is right and it is the same set.** Report B's is the better
statement of it, because every sub-count in B is correct and A's middle
sub-count is not. B's definition-and-exclusions apparatus is what made the
comparison possible in ten minutes rather than an afternoon, and it should be
kept in the report rather than trimmed.

---

## 2. Attack 1 — `auto_diff.cpp:1819`

**Report B is right. Report A does not have it. Report A's inventory is short by
one site here.**

Verified. `grep -n "PrimitiveType::i32\|(int32)\|int32(" taichi/transforms/auto_diff.cpp`
returns exactly twelve lines and closes the file:

- Nine index constructions: `:587`, `:639`, `:646`, `:787`, `:839`, `:846`,
  **`:1819`**, `:1828`, `:1857`.
- Three `val_int32()` reads already in both inventories: `:576`, `:776`, `:1770`.

`taichi/transforms/auto_diff.cpp:1819` reads
`indices_values[i] = insert<ConstStmt>(TypedConstant((int32)i));` inside the
loop opened at `:1817` over `num_elements`. The vector it fills becomes
`indices_matrix_init_stmt` at `:1829`, whose `ret_type` is set at `:1830` to the
`index_tensor_type` built at `:1827-1828` as
`get_tensor_type(tensor_shape, PrimitiveType::i32)`. So it is a matrix element
index, i32 by the `TypedConstant(int32)` constructor at `taichi/ir/type.h:582`,
and B's account of it — including that `:1828` types the `MatrixInitStmt` fed by
`:1819` — is exact.

It appears in neither round-two adversary's list: `adversary2-01-1.md` section 7
gives the group as eight (`:587`, `:639`, `:646`, `:787`, `:839`, `:846`,
`:1828`, `:1857`), and `adversary2-01-2.md` section 7.2 gives the identical
eight. Report A adopted that eight verbatim into its section 3.3. B swept the
file itself and found the ninth. That is the difference between adopting an
adversary's list and re-deriving it, and it is the behaviour the plan asks for.

Consequence for report A's arithmetic: its section 3.3 total of thirty-three
sites in twenty-two rows is internally consistent as written — I re-added the
rows and got 33 — but it should be **thirty-four**, because the `auto_diff.cpp`
`insert_const` row is five sites and should be six, or a new row is needed.

**Four further sites of the same class that report A also omits, and that
neither round-two adversary named.** Found while checking this item:

| Site | Content |
|---|---|
| `taichi/transforms/lower_matrix_ptr.cpp:189` | `auto const_stmt = std::make_unique<ConstStmt>(TypedConstant(i));`, fed as the offset of the `MatrixPtrStmt` built at `:191-192` |
| `taichi/transforms/lower_matrix_ptr.cpp:232` | same form |
| `taichi/transforms/lower_matrix_ptr.cpp:424` | same form |
| `taichi/transforms/lower_matrix_ptr.cpp:477` | same form |

These are matrix element indices pinned i32 at birth by the same
`TypedConstant(int32)` constructor, in the same territory, and they are the same
class as the `auto_diff.cpp` group whose omission report A's own preamble calls
"an asymmetry a reader will trip on". **Report B has all four** (section 3.3,
closing sentence). Report A cites `lower_matrix_ptr.cpp` only at `:536-540` and
in escalation 5; the four index constructions are absent from it entirely.

So report A's index-construction inventory is short by five sites, not one.

---

## 3. Attack 2 — the three `lower_ast.cpp` allocas

**Report B is right on all three, line for line. Report A's characterisation of
`:361` is wrong.** Verified against source:

| Site | What it is | Evidence |
|---|---|---|
| `:330` | **An index.** `fctx.push_back<AllocaStmt>(PrimitiveType::i32);` | `:331` `auto loop_var = fctx.back_stmt();`, `:332` `stmt->parent->local_var_to_stmt[stmt->loop_var_ids[0]] = loop_var;` |
| `:168` | **A while-loop mask.** | `:169` `new_while->mask = mask.get();` |
| `:348` | **A while-loop mask.** | `:349` `new_while->mask = mask.get();` |
| `:361` | **Anonymous, result never captured.** `stmt->insert_before_me(std::make_unique<AllocaStmt>(PrimitiveType::i32));` — the temporary is moved straight into `insert_before_me` with no local bound to it | the real mask is the `:348` alloca, moved in separately at `:365` `stmt->insert_before_me(std::move(mask));`, and stored to at `:367-368` |

Report A's section 3.3 row reads: "Three further `AllocaStmt(PrimitiveType::i32)`,
**all loop-mask allocas**." That is right for `:168` and `:348` and **wrong for
`:361`**, which is not the mask; the mask arrives at `:365`.

A's parenthetical in the same row — "With `:330` these are all four
`PrimitiveType::i32` tokens in the file" — is **correct**. The token appears at
`:168`, `:330`, `:348`, `:361` and nowhere else.

### Ruling on whether report A is inflating its site count

**No, but its table cannot be read as an index-width count, and A does not say
so.** Two things are true at once and both must be stated:

1. A's section 3.3 is headed "Hardcoded i32 on statement and expression types".
   A while-loop mask alloca genuinely *is* a hardcoded i32 on a statement type.
   Under A's own stated heading the three rows belong there, and plan standing
   instruction 3 forbids removing them on the grounds that they look surplus.
   So this is not inflation of the inventory.
2. But the report exists to size item 6.2, which is a width change to
   **addressing**. B's point stands untouched: a width change to indices does
   not imply one to loop masks, and A's table gives a reader no marker to
   separate the two. Three of A's thirty-three sites are not index-width sites,
   and the number is presented flat.

**Ruling:** report B's treatment is the correct one and report A should adopt
its shape — keep all three rows, label `:168` and `:348` as masks, label `:361`
as an anonymous alloca whose result is never captured, and state that only
`:330` is an index. B's own sentence, "a width change to indices does not
automatically imply one to loop masks, and the previous flat list did not let a
reader tell them apart", is the right words for it. This is a labelling edit,
not a count change.

---

## 4. Attack 3 — `make_mesh_block_local.cpp:526`, filed two different ways

**Both treatments are defensible. Neither is a fault.** Source:

```
525:    // mapping_data_type_ = mapping_snode_->dt.ptr_removed();
526:    mapping_data_type_ = PrimitiveType::i32;
527:    mapping_dtype_size_ = data_type_size(mapping_data_type_);
```

Report A files it in section 3.7, "Widths hardcoded rather than derived", and
calls it "the clearest instance in the territory of a derived width that was
deliberately replaced by a literal one". Report B files it in section 3.1,
"Index and address values pinned to i32 by a pass", with the row extended to
name `:525`.

Both are right about the facts and both readings of the site are sound. A's
placement is the sharper of the two, because `:525` is a commented-out derived
form and `:527` then derives the *size* from the hardcoded type, which is
exactly what "hardcoded rather than derived" names. B's placement is sound
because `mapping_data_type_` is the element type of the mesh index mapping, so
the values it types are indices, which is what B's 3.1 collects.

The site is in both reports with its full mechanism, including `:525`, which is
what matters. A reader of either report can find it. **No action.**

---

## 5. Attack 4 — `SNode::get_total_num_elements_towards_root`

**Both halves of report A's claim verified.**

**No callers tree-wide.** `grep -rn "get_total_num_elements_towards_root" .`
over the whole repository returns exactly one line under `taichi/`:
`taichi/ir/snode.h:310`, the definition. Every other hit is a
`modernization/investigation/` document. Report B states the same thing
independently in its section 5.1.

**Every factor narrowed before multiplying.** Source at
`taichi/ir/snode.h:310-315`:

```
310:  int64 get_total_num_elements_towards_root() const {
311:    int64 total_num_elemts = 1;
312:    for (auto *s = this; s != nullptr; s = s->parent)
313:      total_num_elemts *= (int)s->max_num_elements();
314:    return total_num_elemts;
315:  }
```

`max_num_elements()` is declared `int64` at `taichi/ir/snode.h:306-308` and
returns `num_cells_per_container`, itself `int64` at `:97`. So `:313` narrows a
genuine `int64` to `int` on every iteration before it enters the accumulator.
Confirmed.

**One qualification, and report A already has it right.** The unqualified
sentence "the int64 accumulator buys nothing" would be wrong: the accumulator
does buy headroom on the *product* of several already-narrowed factors, which is
where a root-to-leaf walk over four levels of 2^20 would otherwise overflow.
What it buys nothing against is a **single container** above 2^31, because that
factor is truncated before it arrives. Report A's actual wording is "the `int64`
type of the accumulator buys nothing **above 2^31 in any single container**"
(section 3.9). That is the correct, qualified form. No correction needed to the
report; I note it only so the qualifier is not dropped when the reports are read
together.

Report A's handling of the site — recorded as latent, no-caller fact stated
first, deliberately not counted among the three reached formation sites in its
section 4 — is the right disposition under plan standing instruction 3. Report B
carries it as escalation 12 and explicitly declines to judge it. Both are
consistent with the standing instructions and with each other.

---

## 6. Attack 5 — `constant_fold.cpp:115` and the right operand

**Report B is right. Verified exactly as stated.**

`taichi/transforms/constant_fold.cpp:65`, inside
`get_scalar_value_to_replace(BinaryOpStmt *, ConstStmt *lhs, ConstStmt *rhs, DataType dst_type)`
(opened at `:49-52`), under the comment at `:64` *"Type check should have been
done at this point."*:

```
65:    auto dt = lhs->val.dt;
```

The `HANDLE_INTEGRAL_BINARY` macro at `:111-125` then tests that single `dt`:

```
113:    if (dt->is_primitive(PrimitiveTypeID::i32)) {                              \
114:      res = TypedConstant(                                                     \
115:          dst_type, PREFIX(lhs->val.val_int32() OP_CPP rhs->val.val_int32())); \
```

**Line 115 calls the narrowing accessor on both operands; line 113 tests only
the left.** Nothing between `:49` and `:125` reads `rhs->val.dt`. The only other
use of `rhs` before the switch is the `pow` special case at `:56-62`, which
calls `rhs->val.val_int()`, the wide accessor, and does not check `dt` either.
So a pair whose `val.dt` fields differ — left i32, right i64 — reaches
`rhs->val.val_int32()`, whose `TI_ASSERT(get_data_type<int32>() == dt)`
(`taichi/ir/type.cpp:462-465`) is not NDEBUG-gated
(`taichi/common/logging.h:100-107`) and aborts.

**Report A is not wrong here, it is less complete.** A's section 3.2 correctly
overturns both round-one adversaries by establishing that `:115` is guarded and
is correct 64-bit dispatch, with the i64 arm at `:116-118`. That holds for the
same-width case, which is the case the comment at `:64` asserts. A does not
carry the mixed-width residual.

**B's hedge is the right one and I could not close it either.** B writes that
this is "a consequence of a width change to weigh, not a defect today", and that
"the in-tree comment at `:64` asserts the invariant rather than proving it". I
tried to prove it and stopped short. `visit(BinaryOpStmt *)` at
`taichi/transforms/type_check.cpp:290` unifies operands by inserting a
`cast_value` `UnaryOpStmt` through `cast()` at `:283-289`, which would make the
right operand a `UnaryOpStmt` rather than a `ConstStmt` and so fail
`ConstantFold`'s own `is<ConstStmt>` gate before `:115` is reached. But
`ConstantFold` also folds unary casts into fresh `ConstStmt`s, and whether the
`dt` of every such fold necessarily matches the left operand's is a data-flow
argument over the pass's fixed point that I did not trace end to end. **INFERRED,
not verified, in both directions.** It belongs in escalations, and B has it in
the right register.

---

## 7. Independent citation sampling

Roughly seventy cited lines opened individually across both reports, chosen to
span every section of each and weighted toward claims that carry a count.
**No citation fault found in either report.** What I confirmed at source:

- **Cast insertion, ten sites in four files.** `grep -rn "cast_type = PrimitiveType::i32" taichi/`
  returns eight, all mesh: `demote_mesh_statements.cpp:16`, `:54`, `:131`;
  `make_mesh_thread_local.cpp:106`, `:114`; `make_mesh_block_local.cpp:212`,
  `:376`, `:399`. Plus `type_check.cpp:154` and `:461` through
  `insert_type_cast_before` (`:234-244`, emitting `UnaryOpType::cast_value` at
  `:237-239`). Ten, four files. Both reports right.
- **`val_int32()` sites.** Fourteen call lines tree-wide, thirteen in territory,
  twelve aborting. `codegen_llvm.cpp:1054` is the fourteenth and is outside
  territory. Both reports right; A's "twelve sites" heading over a
  thirteen-row table is consistent because the thirteenth row is explicitly
  marked not an abort site.
- **`type_check.cpp`** `:114`, `:119`, `:122-125` (the `is_bit_vectorized`
  early return), `:149-156`, `:160-161`, `:169-173` (`mark_as_if_const` on both
  bounds), `:459-461`, `:466-476`, `:519-526`, `:567`, `:40-44`, `:234-244`.
  All correct in both.
- **`snode.h`/`snode.cpp`** `:41`, `:45`, `:49`, `:78`, `:79`, `:80`, `:81`,
  `:88-89`, `:93-97`, `:98`, `:306-308`, `:310-315`, `:317`; `snode.cpp:22-23`
  (child `*=` parent), `:83`, `:84` (plain assignment, not accumulation),
  `:89-93`, `:95-100`, `:101`, `:174-177`, `:220`. The per-container versus
  root-to-leaf finding both reports now carry is correct in every step.
- **`num_elements_from_root` reach.** `grep -rn` returns the declaration
  (`snode.h:41`), two writers (`snode.cpp:22-23`, `:83`), one reader
  (`:176`), the cache-key serialiser (`offline_cache_util.cpp:112`) and the
  codegen seam (`snode_struct_compiler.cpp:121`). Both reports right. B says
  "six lines" where the grep prints seven, because the write at `:22-23` spans
  two source lines; six sites is the right reading and the wording is loose, not
  wrong.
- **`simplify.cpp` visitor spans.** `visit(SNodeLookupStmt *)` `:222-249` with
  `:248` `set_done(stmt);`; `visit(GetChStmt *)` `:251-273` with `:270` closing
  the inner `if`, `:272` `set_done(stmt);`, `:273` closing the visitor, `:274`
  blank. Both reports have corrected to `:251-273` and both are now right.
- **`make_cpu_multithreaded_range_for.cpp`** `:68`, `:70`, `:72`, `:81`, `:84`,
  `:90`, `:93` — every one exactly as A's table states, including the two
  `GlobalTemporaryStmt(..., PrimitiveType::i32)` readbacks of the dynamic bounds.
  The gate at `compile_to_offloads.cpp:198` is
  `config.make_cpu_multithreading_loop && arch_is_cpu(config.arch)`. Both right.
- **BLS byte-offset narrowings, eight sites.** `make_block_local.cpp:157`,
  `:334` in `(int32)` spelling; `make_mesh_block_local.cpp:80`, `:200`, `:575`
  in `(int32)` spelling and `:118`, `:265`, `:301` in `int32(...)` functional
  spelling. Both reports right, including A's note that three are functional.
- **`utils.cpp`** `:5` and `:14` both take `int y`; `:17` emits
  `TypedConstant(PrimitiveType::i32, bit::log2int(y))`. Both right.
- **Type layer.** `type.h:177-209` `PointerType` with `addr_space_` and its TODO
  at `:207` and no width field; `:582` `TypedConstant(int32)`;
  `type_factory.cpp:86-95` interning keyed at `:89` on
  `std::make_pair(element, is_bit_pointer)`; `:204` the ndarray i32 shape
  members; `:249-251` `to_primitive_type`'s pointer `TI_ERROR`;
  `type_utils.cpp:30` `int data_type_size(DataType t)`, `:31-34` the TODO,
  `:35` `t.set_is_pointer(false)`. Both right, including both reports'
  corrected reading that `data_type_size` returns the *pointee's* size rather
  than refusing.
- **`statements.h`** `:376`, `:379`, `:456`, `:1260` (`int64 offset`), `:1280`,
  `:1370`, `:1415-1416`, `:1439`, `:1684`, `:1787`, `:2064`, `:2083`. All right.
- **`frontend_ir.cpp`** `:149-153` (loop var born pointer-to-i32), `:936-947`
  (`is_integral` only, at `:941`), `:1109-1114` (`get_addr` → u64,
  `is_active` → u1, default → i32), `:1306`, `:1357-1362`, `:1391-1392`,
  `:731-740` (dynamic branch, IR statements) and `:742-747` (folded branch, host
  `int`). Both right, including the split of the two branches across two
  categories that both reports now make.
- **`lower_ast.cpp:270-274`**: `:270` `ConstStmt(TypedConstant(0))`, `:271`
  `ConstStmt(TypedConstant(1))`, `:273` `BinaryOpStmt(mul, end, shape[i])`. IR
  statements, not host `int`. Both right.
- **`demote_dense_struct_fors.cpp`** `:18` `int64 total_n`, `:19` the per-axis
  array, `:24` the per-axis product, `:26` `total_n *= num_cells_per_container`,
  `:29` the hard assert, `:35` the narrowing into `end_value`. Both right,
  including the per-axis versus cell-count distinction both now draw.
- **`check_out_of_bound.cpp:123`** `int size_i = snode->shape_along_axis(i);`
  and `:125-126` the `ConstStmt(TypedConstant(upper_bound_i))`. Both right.
- **`lower_matrix_ptr.cpp:536-540`**, including `:540`
  `offset->ret_type = stmt->offset->ret_type;`. Both right.
- **`offline_cache_util.cpp:110-124`**: `:112`, `:113`, `:114` the three
  extractor `int` fields, `:115` `active`, `:119` `physical_index_position`,
  `:123` `num_cells_per_container`. Report A's description is right.
- **B-only rows checked and confirmed:** `snode.h:80` `int num_active_indices{0}`;
  `type.h:530-532`, `:545` `QuantArrayType::get_num_elements()` returning `int`
  with `int num_elements_` at `:545`; `statements.h:1447-1448` `std::size_t`;
  `offload.cpp:236-237`, `:244` the `int64` element count into `int block_dim`;
  `bls_analyzer.cpp:27` `snode->extractors[j].shape - 1` as the range bound.
- **A-only rows checked and confirmed:** `snode.h:98` `int chunk_size{0}`;
  `frontend_ir.h:712` `int low, high;` on `RangeAssumptionExpression`;
  `analysis.h:20-21` and `:185`; `bls_analyzer.h:15-18`.

---

## 8. Completeness, judged symmetrically

Both reports head their inventory "complete". Neither is a strict enumeration of
every `int` in the territory, and the gaps run in **both** directions. I record
them symmetrically rather than treating one report as the failing one.

**Report A is missing, and report B has:**

| Site | Class |
|---|---|
| `taichi/transforms/auto_diff.cpp:1819` | i32 matrix element index, `TypedConstant((int32)i)` |
| `taichi/transforms/lower_matrix_ptr.cpp:189`, `:232`, `:424`, `:477` | i32 matrix element indices, `ConstStmt(TypedConstant(i))` |
| `taichi/transforms/lower_ast.cpp:151`, `:177`, `:333`, `:363` | `TypedConstant((int32)...)` mask values and loop constants; low consequence, but they are the other four i32 tokens in a file A claims to have closed |

**Report B is missing, and report A has:**

| Site | Class |
|---|---|
| `taichi/ir/snode.h:98` | `int chunk_size{0}` — a 32-bit SNode geometry field |
| `taichi/ir/frontend_ir.h:712` | `RangeAssumptionExpression::low`, `high` — `int` |

B also treats `taichi/ir/scratch_pad.h` more fully than A (B section 3.9 gives
eleven line spans, A gives three rows), while A gives the `scratch_pad.h` `int`
products a row in its section 3.6 that B does not.

**Weighting.** A's five missing index constructions matter more than B's two
missing structural fields, because index constructions are the exact class item
6.2 turns on, and because A's own preamble to the `auto_diff.cpp` rows says that
inventorying a file's aborts while omitting its index constructions "is an
asymmetry a reader will trip on" — the same asymmetry now stands for
`lower_matrix_ptr.cpp`. B's two are ordinary `int` fields in a table of
twenty-odd such fields, and neither is load-bearing on any conclusion.

---

## 9. Verdicts

### 9.1 CORRECT — both, yes

I attacked five specific claims and sampled seventy citations. **I found no
false load-bearing claim in either report.** Every structural finding both
reports rest on survives:

- The type system can represent i64 but models no index type and no address
  type; `PointerType` carries no width and is interned on
  `(pointee, is_bit_pointer)`.
- The 32-bit assumption sits on the index, shape and stride side; byte offsets
  are already `size_t`, with `lower_matrix_ptr.cpp:536-540` the one exception.
- Addressing is hierarchical: one `LinearizeStmt` per SNode level, so a large
  structure does not by itself produce a large value in the frontend IR.
- The coercion is not a one-shot: twenty-nine invocation lines, twenty-two of
  them outside the driver, most invisible to a grep for `irpass::type_check`.
- `AxisExtractor::num_elements_from_root` is an unguarded root-to-leaf `int`
  product, returned by `shape_along_axis` and materialised as the bound checked
  against every SNode field index.
- The `spirv_has_int64` device capability exists and is enforced at SPIR-V
  codegen; what is missing is only the link from the type layer.

The two errors round two upheld against report A — the `snode.cpp:89-101`
misclassification and the `lower_ast.cpp:270-274` misfiling — are both fixed and
both fixes verify. The one error round two upheld against report B —
`constant_fold.cpp:115` as an abort site — is fixed, and B has gone further than
the correction required by finding the right-operand residual.

### 9.2 COMPLETE — report B yes; report A not quite

**Report B: complete on the surface it defines.** Its `type_check` set is right
in every sub-count. Its `auto_diff.cpp` sweep found a site both round-two
adversaries and report A missed. Its `lower_ast.cpp` characterisation is exact.
Its `constant_fold.cpp` residual is real. Its two missing structural `int`
fields change nothing.

**Report A: correct but short.** Four edits, all mechanical, none requiring new
investigation:

1. **Section 5**: "fourteen sites in **ten** files" → "fourteen sites in
   **nine** files". The nine are named in my section 1.3. The stated total of
   fourteen files is already right; only the sub-count contradicts it.
2. **Section 3.3**: add `taichi/transforms/auto_diff.cpp:1819`
   (`TypedConstant((int32)i)`, feeding the `MatrixInitStmt` typed at `:1828`).
   The `auto_diff.cpp` group becomes nine, and the section total becomes
   thirty-four sites.
3. **Section 3.3**: add `taichi/transforms/lower_matrix_ptr.cpp:189`, `:232`,
   `:424`, `:477`, the four `ConstStmt(TypedConstant(i))` matrix element
   indices. With these the section total becomes **thirty-eight sites in
   twenty-four rows**, if they are added as one row and `auto_diff.cpp:1819` as
   another; A should re-derive rather than take my arithmetic.
4. **Section 3.3**, `lower_ast.cpp` row: `:361` is not a mask alloca. It is
   anonymous, its result is never captured, and the real mask is the `:348`
   alloca moved in at `:365`. Label `:168` and `:348` as masks, `:361` as
   anonymous, and state that of the four only `:330` is an index.

**These are the whole of what remains.** They are edits to two sections of one
report. Nothing in them touches a conclusion, an escalation, or the structural
findings, and nothing in them requires reopening the territory.

### 9.3 Is general consensus reached?

**Yes, on substance, and I will say so plainly rather than manufacture a fourth
round.** The two reports, worked blind, converge on the same set at every point
that carries a conclusion, including the count that was supposed to be the
divergence. Where they differ they differ at the margins of two independent
enumerations of a large surface, in both directions, and both sets of marginal
items are now named with file and line in this document. Round two's objections
were all met, and both reports met them by re-deriving rather than adopting,
which is why B found a site nobody had.

The four edits in 9.2 are the arbiter's call to require or waive. My
recommendation: **require edits 1 and 4** (a count that contradicts its own
total, and a factual mischaracterisation), **require edits 2 and 3** as well,
since the sites are named and the cost is four lines, and **do not send the
territory back for another investigative pass.** There is nothing left here that
investigation would find.

---

## 10. Escalations

Unresolved judgement. Recorded, not decided. I add no new escalation of my own;
all of these are already carried by one or both reports and I am confirming that
they remain open rather than closing any.

1. **Can a mixed-width `ConstStmt` pair actually reach
   `constant_fold.cpp:115`?** Report B says the invariant at `:64` is asserted
   rather than proven, and I could not prove it either. `type_check.cpp:283-289`
   inserts a `cast_value` `UnaryOpStmt` to unify binary operands, which would
   normally defeat `ConstantFold`'s `is<ConstStmt>` gate, but `ConstantFold`
   also folds unary casts into fresh `ConstStmt`s and I did not trace the fixed
   point. **INFERRED in both directions. Planner.**
2. **Mesh scope.** Eight of the ten cast sites, four pinned `ret_type`s, and
   eight of the twenty-two `type_check` re-run lines are mesh. Unchanged from
   rounds one and two; both reports escalate it. **Planner.**
3. **What "the address" means for item 6.2.** Four distinct quantities are in
   play with four different widths and four different failure modes, set out in
   B's escalation 7 sub-table and A's section 4. Which one 6.2 means selects
   which inventoried sites matter. **Planner.**
4. **CPU path scope**, **`PhysicalCoordinates`**, **`MatrixPtrStmt::offset`
   dual semantics**, **`simplify.cpp:261`**, **the C++ tests**, **offline cache
   invalidation**, **`kMaxNumSnodeTreesLlvm = 512`**,
   **`taichi_max_num_indices` as a device footprint**, **the type-layer to
   device-capability link**. All carried correctly by both reports, all still
   open. I add nothing to any of them.
5. **Whether the four edits in 9.2 are required or waived.** Arbiter's seat, per
   plan section 9.1. I have stated a recommendation and stopped.

---

## 11. Divergence from adversary3-01-2

`adversary3-01-2.md` does not exist at the time of writing. Recorded as
instructed.
