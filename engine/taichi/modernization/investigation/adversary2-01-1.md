# Adversary2 01-1 — round two, frontend IR and type system

Judging the revised `report-01-ir-types.md` (pass A) and `report-01b-ir-types.md`
(pass B) against the source at `/opt/project/taichi/`, against my round-one file
`adversary-01-1.md`, and against `adversary-01-2.md`.

Every file:line below was opened with `awk 'NR>=a && NR<=b'` or `sed -n`, so the
line numbers I give are attribution-checked, not content-checked. Paths are
relative to `/opt/project/taichi/`.

---

## 0. Verdicts

**CORRECT — pass A: yes, with three defects, one of which is a real
categorisation error. Pass B: no, on one headline count, otherwise yes.**

Pass A's substantive round-one errors are gone and its adjudications against me
are right. What remains is (a) `taichi/transforms/lower_ast.cpp:270-274` filed
under a section heading that asserts the opposite of what the code is, (b) a
section heading that counts eighteen over a table of twenty, (c) one visitor
span off by three lines.

Pass B still states that `taichi/transforms/constant_fold.cpp:115` aborts on a
wider constant. It does not. That is a false statement about the source in
pass B's headline 2 and section 3.6, and pass A proved it.

**COMPLETE — neither, but the remaining gap is small and specific.** Both
reports now name the site where a per-container extent is truncated and call it
a whole-structure extent. The genuine root-to-leaf extent
(`AxisExtractor::num_elements_from_root`) is formed unguarded in `int` inside
territory 01, feeds the very bound both reports already cite at
`taichi/transforms/check_out_of_bound.cpp:123-126`, and is in neither report.
Pass A additionally still omits the dynamic tensor-offset branch that I raised
in round one.

**CONSENSUS: not reached. One more pass, tightly scoped.** Section 7 lists what
remains. Nothing there is rework; all of it is amendment.

---

## 1. Where the explore agents ruled against round one

The lead named five. I adjudicated each against source. **The explore agents win
four and a half of the five.**

### 1.1 The cast-insertion count is ten, not eight. Pass A is right; I was wrong.

```
grep -rn "cast_type = PrimitiveType::i32" taichi/ir/ taichi/transforms/ taichi/analysis/
```

returns exactly eight lines, all mesh:

| File | Lines |
|---|---|
| `taichi/transforms/demote_mesh_statements.cpp` | `:16`, `:54`, `:131` |
| `taichi/transforms/make_mesh_thread_local.cpp` | `:106`, `:114` |
| `taichi/transforms/make_mesh_block_local.cpp` | `:212`, `:376`, `:399` |

Plus the two `insert_type_cast_before` calls at
`taichi/transforms/type_check.cpp:154` and `:461`, the helper being at
`:234-244`. Total ten.

My round-one section 6 wrote the headline "8 sites in 4 files" and then
enumerated ten under it, and my section 5.3 wrote "Six mesh cast sites" over a
list of eight. Both were arithmetic slips in my own file. Pass A's N024 caught
both and said so. The mesh count is eight. Pass A's ten is correct and both
revised reports now carry it (A section 3.1, B headline 2 and section 3.3).

### 1.2 `constant_fold.cpp:115` is guarded. Pass A is right; both round-one adversaries were wrong.

This one matters more than the count, because it goes to the ret_type/val.dt
divergence both reports build on.

`taichi/transforms/constant_fold.cpp:64` is `auto dt = lhs->val.dt;` — the guard
reads the **`TypedConstant::dt` field**, which is exactly the field
`TypedConstant::val_int32()` asserts against at `taichi/ir/type.cpp:462-465`. It
is not `Stmt::ret_type`, so `mark_as_if_const` cannot desynchronise it. Inside
the `HANDLE_INTEGRAL_BINARY` macro:

```
113:    if (dt->is_primitive(PrimitiveTypeID::i32)) {
114:      res = TypedConstant(
115:          dst_type, PREFIX(lhs->val.val_int32() OP_CPP rhs->val.val_int32()));
116:    } else if (dt->is_primitive(PrimitiveTypeID::i64)) {
117:      res = TypedConstant(
118:          dst_type, PREFIX(lhs->val.val_int() OP_CPP rhs->val.val_int()));
```

That is correct 64-bit dispatch. It cannot abort on an i64 constant, because an
i64 constant takes the `:116` arm. I withdraw my round-one section 3.2 listing
of it as an abort site, and so should `adversary-01-2.md`, whose section 6
called the thirteen-site list "**exactly** the complete set" of aborts.

Twelve aborts, not thirteen. Pass A's section 3.2 is right.

**One nuance neither pass records.** Line `:115` calls
`rhs->val.val_int32()` as well, and the guard tests only `lhs->val.dt`. That is
safe only because operand types have been unified upstream, which the in-tree
comment at `:63` (*"Type check should have been done at this point."*) asserts
rather than proves. It does not change the verdict; it is worth one clause in
pass A's row.

Pass A's parenthetical about `taichi/codegen/llvm/codegen_llvm.cpp:1054` being
guarded at `:1052` with an i64 branch at `:1058` is also correct, and correctly
flagged as outside territory.

### 1.3 Pass B's headline 3 rewrite is coherent. `adversary-01-2.md` should accept it.

The charge was that pass B asserted both "the frontend does not restrict the
index type" and "the frontend pins every loop variable to i32" without
reconciling them. The rewrite reconciles them by scoping to two different
objects, and says so in terms. Both halves verified:

- `IndexExpression::type_check`, `taichi/ir/frontend_ir.cpp:936-947`, tests only
  `is_integral(expr_type)` at `:941`. An explicit i64 index survives.
- `FrontendForStmt::add_loop_var`, `taichi/ir/frontend_ir.cpp:149-153`, sets
  `loop_var.expr->ret_type = get_pointer_type(PrimitiveType::i32)` at `:151-152`
  unconditionally. A loop variable does not.
- `taichi/transforms/frontend_type_check.cpp:71-78` warns without casting.

These are not in tension. An expression the user writes and a variable the
compiler synthesises are different things, and pass B now names which claim
applies to which. Rewriting rather than withdrawing was the right call.

### 1.4 `data_type_size` strips the pointer flag. Both explore agents are right; `adversary-01-2.md` was wrong and has conceded.

`taichi/ir/type_utils.cpp:30` opens `int data_type_size(DataType t)`, `:31-34`
is the TODO recording that a loud failure on pointers was intended, `:35` is
`t.set_is_pointer(false);`. `DataType::set_is_pointer`
(`taichi/ir/type.cpp:55-61`) at `:59-61` does
`ptr_ = ptr_->cast<PointerType>()->get_pointee_type();`. There is no
`PointerType` branch anywhere in the body of `data_type_size`, so a pointer
argument falls through to the primitive dispatch at `:56-66` and returns the
pointee's size. A caller gets a wrong answer, not an error.

Both revised reports now say this (A section 6 "Cannot"; B headline 5 and
section 4). Correct.

### 1.5 The three flattening sites. Two hold. The third does not, in both reports.

This is the one place the explore agents' new framing does not survive.

**Site 1 — `taichi/transforms/demote_dense_struct_fors.cpp`. Holds.**
`:18` `int64 total_n = 1;`; `:21-28` walks root-to-leaf; `:26`
`total_n *= snode->num_cells_per_container;`; `:29`
`TI_ASSERT(total_n <= std::numeric_limits<int>::max());`; `:35`
`offloaded->end_value = total_n;` into the `int32` field at
`taichi/ir/statements.h:1416`. A genuine whole-structure cell count, and it
aborts. Both reports right.

**Site 2 — `taichi/transforms/lower_ast.cpp:270-274`. Holds as a site, but
pass A files it under a heading that contradicts it.**
`:270` seeds `begin` with `ConstStmt(TypedConstant(0))`, `:271` seeds `end` with
`ConstStmt(TypedConstant(1))`, `:272-274` multiplies in each
`ExternalTensorShapeAlongAxisStmt` produced at `:258-259`/`:265-266`. No assert,
no warning. Correct.

But pass A's section 3.6 is headed *"32-bit arithmetic that overflows before any
IR type is involved"* and opens *"These are host-side compile-time `int`
computations. They cannot be fixed by changing an IR type."* — and
`lower_ast.cpp:270-274` is a row in that table. It is neither host-side nor
compile-time. It is a chain of `BinaryOpStmt` mul over
`ExternalTensorShapeAlongAxisStmt` values that `type_check.cpp:119` pins to i32;
it overflows in a register at run time, and it is precisely the class of thing a
width change does fix. Pass A's own section 4 and section 7.3 item 2 treat it
correctly, so section 3.6 contradicts them. Pass B files it in section 3.4 with
no host-side claim and is clean.

I checked the rest of pass A's 3.6 table and every other row is genuinely
host-side: `scalar_pointer_lowerer.cpp:33-39`,
`demote_dense_struct_fors.cpp:19-24`, `:74`, `simplify.cpp:176`/`:187`
(`auto stride_product = 1;` deduced `int`, `stride_product *= stmt->strides[i]`),
`check_out_of_bound.cpp:54-57` (`int flattened_element`) and `:169-173`
(`int max_valid_index`), `handle_external_ptr_boundary.cpp:62-67`,
`frontend_ir.cpp:742-747` (`int offset = 0;`), `scratch_pad.h`. The `lower_ast`
row is the only misfiled one.

**Site 3 — `taichi/ir/snode.cpp:89-101`. Does not hold. This is a per-container
extent, not a whole-structure one, and both reports say otherwise.**

`taichi/ir/snode.cpp:84` is `new_node.extractors[ind].shape = sizes[i];` — a
plain assignment of *this node's own* axis size. `SNode::insert_children`
(`:14-34`) propagates only `num_elements_from_root` (`:21-23`); `shape` and
`acc_shape` start at the `AxisExtractor` defaults (`taichi/ir/snode.h:45`,
`:49`). So the accumulation at `:89-93`:

```
89:   int64 acc_shape = 1;
90:   for (int i = taichi_max_num_indices - 1; i >= 0; i--) {
91:     // casting to int32 in extractors.
92:     new_node.extractors[i].acc_shape = static_cast<int>(acc_shape);
93:     acc_shape *= new_node.extractors[i].shape;
```

is the product of one node's own axis shapes. It is stored at `:101` into
`num_cells_per_container`, which `taichi/ir/snode.h:94-97` documents as
*"Product of the |shape| of all the activated axes identified by |extractors|"*
with a pointer to the cell/container terminology. `SNode::max_num_elements()`
(`snode.h:306-308`) just returns it.

That is the per-level container extent — the exact quantity both reports'
section 1 / escalation 7 says is bounded by the level's container rather than by
the structure. Pass A's own row text says "truncated per node", which
contradicts pass A's own section heading. Pass B's sub-table has the same
mismatch.

The warn-then-truncate behaviour at `:92` and `:95-100` is real and belongs in
the inventory. What is wrong is calling it a whole-structure flattening.

**And the site that does belong in that slot is in neither report.** See 2.1.

---

## 2. What is still missing, in both

### 2.1 `num_elements_from_root` — the root-to-leaf extent, unguarded, in territory

This is the substantive completeness gap.

- `taichi/ir/snode.h:41` — `int num_elements_from_root{1};`
- `taichi/ir/snode.cpp:21-23` — `insert_children` multiplies each child's
  per-axis value by its parent's, so it accumulates **root-to-leaf**.
- `taichi/ir/snode.cpp:83` — `new_node.extractors[ind].num_elements_from_root *= sizes[i];`
- No assert, no warning, no `int64` shadow anywhere on this path.

Three consequences, none stated in either report:

1. `SNode::shape_along_axis` (`taichi/ir/snode.cpp:174-177`) **returns this
   field** — `return extractor.num_elements_from_root;` at `:176`. Both reports
   cite `snode.h:317` as "returns `int`" and both cite
   `taichi/transforms/check_out_of_bound.cpp:123-126` as the per-axis bound
   compared against every SNode index. Neither joins them. The i32 bound
   materialised at `:126` is this unguarded root-to-leaf product.
2. `taichi/codegen/spirv/snode_struct_compiler.cpp:115-121` computes
   `sn_desc.total_num_cells_from_root *= e.num_elements_from_root;` — a genuine
   whole-structure cell count built from the territory-01 `int` field. Outside
   territory, but it is the seam fact that makes the field load-bearing.
3. `SNode::get_total_num_elements_towards_root` (`taichi/ir/snode.h:310-315`) is
   the whole-structure count with an `(int)` narrowing at `:313`. Pass B
   escalates it as uncalled (escalation 12); pass A does not mention it at all.

Pass A's section 4 says "Three sites flatten a whole-structure or whole-array
extent". With site 3 struck and this one added it is still three, but they are
not the same three, and the third one has no guard of any kind rather than a
warning.

### 2.2 `taichi/transforms/auto_diff.cpp` — eight i32 index constructions, in neither inventory

Both reports inventory `auto_diff.cpp:576`, `:776`, `:1770` as abort sites.
Neither inventories the i32 index *constructions* in the same file:
`:587`, `:646`, `:787`, `:846`, `:1857` build matrix element indices as
`insert_const(PrimitiveType::i32, ...)`, and `:639`, `:839`, `:1828` build an
index `TensorType` as `get_tensor_type(tensor_shape, PrimitiveType::i32)`.

Bounded by matrix element count, so low consequence. Recorded because it is the
last of the 64 `PrimitiveType::i32` tokens in the territory that neither report
classifies, and because inventorying a file's aborts but not its index
constructions is an asymmetry a reader will trip on.

For completeness on that sweep: `taichi/transforms/check_out_of_bound.cpp:220`
(`compare->ret_type = PrimitiveType::i32`) is a `pow` assertion's comparison
result, not an index, and `taichi/ir/type_utils.h:40` is the
`get_primitive_data_type<int32>()` template arm. Both benign. That closes all 64.

### 2.3 Pass A only

- **`taichi/ir/frontend_ir.cpp:731-740`, the dynamic tensor-offset branch.** I
  raised this in round one (my section 5.7 and 10.2). Pass A's section 3.6 still
  lists only the constant-folded branch at `:742-747`. Verified: `:731` is
  `if (needs_dynamic_index) {`, `:732` seeds the offset with
  `ConstStmt(TypedConstant(0))`, `:735` emits each shape factor as
  `ConstStmt(TypedConstant(shape[i]))`, both i32 by the `TypedConstant(int32)`
  constructor at `taichi/ir/type.h:582`. Pass B added it (section 3.4 row
  `frontend_ir.cpp:732,735`). Pass A's notes N022–N038 do not mention it. This
  objection was neither addressed nor answered.
- **`taichi/transforms/type_check.cpp:170-171`** — `mark_as_if_const(stmt->begin,
  PrimitiveType::i32)` and `(stmt->end, ...)` on `RangeForStmt`. Pass A discusses
  the mechanism in section 7.1 but the site appears in none of its inventory
  tables. Pass B has it (section 3.1).
- **`taichi/transforms/make_mesh_block_local.cpp:526`** —
  `mapping_data_type_ = PrimitiveType::i32;`. Pass B has it (section 3.1); pass A
  does not.
- **`taichi/transforms/lower_ast.cpp:168`, `:348`, `:361`** — three further i32
  allocas. Pass B has them (section 3.9); pass A carries only `:330`.

### 2.4 Pass B only

Nothing missing that I found. Pass B's inventory is the more complete of the two
after revision, as it was before.

---

## 3. Newly wrong

### 3.1 Pass B — `constant_fold.cpp:115` still listed as an abort

Section 3.6: *"**Thirteen call sites** therefore fail loudly, in every build, on
a wider constant"*, with `taichi/transforms/constant_fold.cpp:115` as the
thirteenth. Headline 2: *"sixteen hard aborts (section 3.6)"*.

Both numbers are wrong by one, and the characterisation of `:115` is wrong
outright, per section 1.2 above. Twelve `val_int32()` aborts plus the three
structural aborts pass B already names (`type_check.cpp:160-161`,
`snode.h:28-30`, `demote_dense_struct_fors.cpp:29`) is fifteen.

Pass B could not have known, being blind to pass A. It is still a false
statement about the source in a headline, and it must be corrected.

### 3.2 Pass A — section 3.3 heading counts eighteen over a table of twenty

The heading reads *"Hardcoded `ret_type = PrimitiveType::i32` — eighteen
sites"*. Pass A's own note N024 enumerates exactly eighteen: `type_check.cpp`
`:114,119,467,471,475,521,525,567`; `statements.h:1684,2064,2083`;
`statements.cpp:146`; `frontend_ir.cpp:152,1114,1306,1358,1362,1392`.

The report's table then adds two more rows — `lower_ast.cpp:330` and
`type_factory.cpp:204` — without moving the count, and the
`frontend_ir.cpp:1358, :1362, :1392` row holds three sites in one row. Twenty
sites under an "eighteen" heading.

Both added rows are also categorically different from the rest:
`lower_ast.cpp:330` is `fctx.push_back<AllocaStmt>(PrimitiveType::i32)` (an
alloca's element type) and `type_factory.cpp:204` is
`shape_members.push_back({PrimitiveType::i32, ...})` (a struct member type).
Neither is a `ret_type` assignment. Verified all twenty lines individually.

This is the same headline-versus-enumeration mismatch pass A correctly scored me
on in 1.1. One sentence fixes it.

### 3.3 Both — the `GetChStmt` visitor span

`taichi/transforms/simplify.cpp`:

```
251:   void visit(GetChStmt *stmt) override {
...
270:     }
271:
272:     set_done(stmt);
273:   }
```

The visitor runs `:251-273`. `:270` closes the inner `if`; `:274` is blank.

Pass A section 7.3 item 5 says `:251-270`. Pass B section 3.5 and escalation 10
say `:251-274`. Both are wrong, in opposite directions, on the same claim. The
companion span both give for `visit(SNodeLookupStmt *)`, `:222-249`, is correct,
as are the asserts at `:233` and `:234-235`, the `:239` site and the `:261` site.

The substantive point — `:261` is unguarded on its face, the transitive argument
through the `IntegerOffsetStmt` at `:255` is plausible and unproven — is right in
both, and both correctly mark it inferred and escalate it. Only the span is
wrong.

---

## 4. Round-one objections: addressed, or merely acknowledged

I went through every objection in `adversary-01-1.md` section 9 and
`adversary-01-2.md` sections 6 and 8, and checked the revised text against the
source rather than against the notes.

### Pass A — addressed in substance

| Objection | Status |
|---|---|
| Silent range-for truncation is false | **Addressed.** Section 7.1 item 1 withdraws it; section 3.2 rows for `offload.cpp:109`/`:117` now read "no — aborts". Chain re-verified: `type_check.cpp:40-44` writes `ret_type`; `statements.h:988` holds `TypedConstant val`, `:993` sets `ret_type = val.dt` in the constructor only; `type.cpp:462-465` asserts against `TypedConstant::dt` (`type.h:551`); `logging.h:100-107` expands `TI_ASSERT` unconditionally. |
| "Single enforcement point" | **Addressed.** Replaced by ten cast sites, twelve aborts, the pinned-`ret_type` table, section 3.4 re-synthesis and section 5's seven re-runs, with the narrow defensible form stated at 2.2 item 1. |
| `rhi_constants.inc.h:14` | **Addressed, and verified independently.** `:14` `PER_DEVICE_CAPABILITY(spirv_has_int64)`; enum at `device_capability.h:11-15`; `spirv_ir_builder.cpp:64-66` emits `CapabilityInt64`, `:166-169` declares `t_int64_`/`t_uint64_`, `:311-314` and `:325-328` raise `TI_ERROR("Type {} not supported.")`. All six read. |
| `make_cpu_multithreaded_range_for.cpp` | **Addressed.** Section 3.4 table verified line by line: `:68`, `:70`, `:72`, `:81`, `:84`, `:90`, `:93`. Gate at `compile_to_offloads.cpp:198`, pass at `:199`. |
| Mesh casts | **Addressed.** Section 3.1, all eight confirmed by grep. |
| `lower_matrix_ptr.cpp:536-540` | **Addressed.** Section 3.8; `:536-537` builds the stride const, `:538-539` the mul, `:540` forces `offset->ret_type = stmt->offset->ret_type`. |
| Sixteen line errors, struck re-verification claim | **Addressed.** I sampled forty-plus references across `type_check.cpp`, `statements.h`, `statements.cpp`, `frontend_ir.cpp`, `type.h`, `type.cpp`, `type_factory.cpp`, `type_utils.cpp`, `snode.h`, `snode.cpp`, `simplify.cpp`, `utils.cpp`, `value_diff.cpp`, `offload.cpp`, `scalar_pointer_lowerer.cpp`, `check_out_of_bound.cpp`, `make_block_local.cpp`, `logging.h`, `rhi_constants.inc.h`, `spirv_ir_builder.cpp`. Two errors found, both in section 3. That is a different order of accuracy from revision 1. |
| `val_int32()` inventory | **Addressed and improved.** Fourteen tree-wide call sites confirmed by grep; thirteen in territory; twelve abort. |
| `is_bit_vectorized` exception | **Addressed.** `type_check.cpp:122-125` returns before the loop at `:147-156`. |
| `expr.cpp:93` removed | **Addressed.** |
| `total_shape` per-axis | **Addressed.** `scalar_pointer_lowerer.cpp:33-39` verified: `:37` `total_shape[j] *= s->extractors[j].shape` indexed by axis `j` over the root-to-leaf `snodes_` loop. |
| `check_out_of_bound.cpp:123-126` | **Addressed.** Section 3.11. |
| `arithmetic_interpretor.cpp:98-109` | **Addressed.** `:99` and `:106` `int64_t`, `:108` stores under `stmt->ret_type`. |
| C++ tests | **Addressed.** Section 4 and escalation 10. |
| `promoted_type` pointer error | **Addressed.** `type_factory.cpp:249-251` verified. |
| `value_diff` two visitors, six consumers | **Addressed.** `:81-87` gate at `:82` read at `:83`; `:142-146` gate at `:143` read at `:144`. |
| `data_type_size` returns `int` | **Addressed.** Section 3.5, with `:47-48` for the `TensorType` path. |
| `kMaxNumSnodeTreesLlvm` | **Addressed.** Section 2.1 and escalation 9. |
| Dynamic tensor branch `frontend_ir.cpp:731-740` | **NOT addressed and not answered.** See 2.3. |

### Pass B — addressed in substance

| Objection | Status |
|---|---|
| `data_type_size` "refuses pointers" | **Addressed.** Headline 5 and section 4 now give the mechanism, with `type.cpp:59-61`. |
| `simplify.cpp` guard stated as fact | **Addressed.** Section 3.5 demotes it to one site and section 5.2 keeps it inferred. The two no longer contradict each other. Span off by one, per 3.3. |
| "Only three constants" | **Addressed.** Section 5.1 now names four with their sites. |
| Mesh casts filed as "hardcoded widths" | **Addressed.** Moved to section 3.3 as live truncations. |
| Headline 3 unreconciled | **Addressed.** See 1.3. |
| No `scratch_pad.h` / `analysis.h:185` / `bls_analyzer.h` / mesh scope | **Addressed.** Section 3.9, section 3.2, escalation 4. |
| Partial out-of-territory `taichi_max_num_indices` list | **Addressed.** Section 3.7 now carries `struct_llvm.cpp:171`, `runtime.cpp:1012` and `export_lang.cpp:559,1222`. |
| `kMaxNumSnodeTreesLlvm` internal count inconsistency | **Addressed.** Stated as lines, with the convention explained. |
| `is_bit_vectorized`, `rhi_constants.inc.h`, `total_shape`, `lower_ast.cpp`, `arithmetic_interpretor.cpp`, `check_out_of_bound.cpp:123-126`, `lower_matrix_ptr.cpp`, `promoted_type`, dynamic tensor branch, tests | **All addressed.** |
| Abort count | **Not addressed** — could not have been. See 3.1. |

Nothing in either revision is a prose concession over an unchanged analysis. Both
agents went back to the source, and in three places pass A went back and beat
both adversaries. That is the loop working.

---

## 5. Independent citation sample

Beyond the claims above, opened and confirmed exactly as cited by one or both
reports:

- `type_check.cpp` — `:114` (`SNodeOpStmt` default i32, with `get_addr` u64 at
  `:107` and `is_active` u1 at `:112`), `:119`, `:122-125`, `:148-155` (warning
  text at `:149-152`), `:154`, `:159-163`, `:170-171`, `:459`, `:461`, `:467`,
  `:471`, `:475`, `:521`, `:525`, `:567`. All thirteen `PrimitiveType::i32`
  tokens in the file accounted for.
- `statements.h` — `:456` `int dynamic_index_stride{0}`, `:988`, `:993`,
  `:1260` `int64 offset`, `:1280` `std::vector<int> strides`, `:1370` `int chid`,
  `:1415-1416` `int32 begin_value`/`end_value`, `:1684`, `:2064`, `:2083`.
- `type.h` — `:177-209` `PointerType` with `pointee_` at `:206`, `addr_space_`
  and its TODO at `:207`, `is_bit_pointer_` at `:208`, and no width field;
  `:551` `DataType dt`; `:556-569` the union; `:582` `TypedConstant(int32 x)`.
- `type_factory.cpp` — `:86-95` interning with the key at `:89`; `:168-169` the
  64-bit arm; `:222-246` `compare_types` ranking by `data_type_bits` at
  `:234-237`; `:248-262` `to_primitive_type` with the pointer error at `:249-251`.
- `snode.h` — `:28-30` `Axis` bound check, `:41`, `:45`, `:49`, `:78`, `:81`,
  `:88-89`, `:97`, `:98`, `:99-100`, `:306-308`, `:310-315` with the `(int)`
  narrowing at `:313`, `:317`, `:329-334`.
- `snode.cpp` — `:21-23`, `:83`, `:84`, `:89-93`, `:95-100`, `:101`, `:174-177`,
  `:220`.
- `utils.cpp` — `:5` and `:14` both take `int y`; `:17`
  `TypedConstant(PrimitiveType::i32, bit::log2int(y))`; `:7`, `:10`, `:20` emit
  `TypedConstant(y)`.
- `simplify.cpp` — `:160-220` the `LinearizeStmt` expansion, `:175` sum seed,
  `:176` `auto stride_product = 1;`, `:178` stride const, `:187` the product,
  `:189-205` the debug assert and select, `:222-249`, `:232-236`, `:239`,
  `:251-273`, `:261`.
- `make_block_local.cpp` — `:157`, `:334` `TypedConstant((int32)bls_offset_in_bytes)`;
  `:292-296` the BLS bound message with `%d` at `:295`. Pass A's N037
  self-correction here is right.
- `make_mesh_block_local.cpp` — `:80`, `:118`, `:200`, `:265`, `:301`, `:575`,
  three in `(int32)` spelling and three in `int32(...)` spelling, exactly as both
  reports now say.
- `offload.cpp` — `:107-109`, `:115-117`, guarded on `ConstStmt` only.
- `control_flow_graph.cpp` — `:456-457` guard, `:460-462` the read.
- `scalarize.cpp:158-162`; `auto_diff.cpp:575-576`, `:775-776`, `:1770`. All read
  a `MatrixPtrStmt` or matrix offset, which supports pass A's structural point
  that ten of the twelve aborts are downstream of the single assert at
  `type_check.cpp:160-161`. Pass A's note N025 says eleven of twelve; the report
  says ten, and ten is the correct figure (twelve minus the two in `offload.cpp`).
  The report is right and the note is not; no action needed on the report.
- `demote_no_access_mesh_fors.cpp:31-35` — a fourth writer of the bounds, with
  `end_value` spanning `:34-35`;
  `make_cpu_multithreaded_range_for.cpp:138-141` — a third.
- `compile_to_offloads.cpp` — seven `irpass::type_check` calls at `:57`, `:66`,
  `:193`, `:200`, `:208`, `:310`, `:391`.

No citation fault found other than the two in section 3.

---

## 6. Escalations

Unresolved. Recorded, not decided. All of these are already in one or both
reports' escalation lists and I am not adding a decision to any of them.

1. **Mesh scope.** Eight of the ten cast sites are mesh. Unchanged from round
   one; both reports escalate it; nothing in plan section 6 answers it.
2. **`spirv_has_int64` and the type layer.** Both reports now have the finding
   and both correctly stop short of proposing the link, per standing instruction
   4.
3. **What "the address" means for item 6.2.** Both reports escalate it. My
   section 1.5 and 2.1 sharpen it rather than settle it: the quantities in play
   are the per-level linearised index, the per-container cell count
   (`num_cells_per_container`), the root-to-leaf per-axis extent
   (`num_elements_from_root`) and the whole-structure cell count
   (`total_num_cells_from_root` at the codegen seam). They have different widths,
   different guards and different failure modes today. Which one item 6.2 means
   is a planner call.
4. **CPU path scope**, **`PhysicalCoordinates`**, **`MatrixPtrStmt::offset` dual
   semantics**, **`simplify.cpp:261`**, **the C++ tests**, **offline cache
   invalidation**, **`kMaxNumSnodeTreesLlvm`**, **`taichi_max_num_indices` as
   device footprint**. All carried correctly by both reports. No addition from me.

---

## 7. What remains, scoped for one pass

Nothing here requires new investigation. Every item names the lines.

**Pass A — five amendments.**

1. Move `taichi/transforms/lower_ast.cpp:270-274` out of section 3.6, or restate
   3.6's preamble. It is an IR-level i32 product, not a host-side compile-time
   `int` computation, and section 4 and 7.3 item 2 already treat it that way.
2. Section 4: strike `taichi/ir/snode.cpp:89-101` as a whole-structure extent —
   it is `num_cells_per_container`, per container, per `snode.h:94-97` — and put
   `AxisExtractor::num_elements_from_root` (`snode.h:41`, formed at
   `snode.cpp:21-23` and `:83`, returned by `shape_along_axis` at
   `snode.cpp:174-177`, consumed at `check_out_of_bound.cpp:123-126` and at the
   seam `taichi/codegen/spirv/snode_struct_compiler.cpp:115-121`) in its place.
   Keep the `:92` truncation and its warning in the inventory where it belongs.
3. Section 3.3: reconcile the "eighteen sites" heading with the twenty sites in
   the table, and note that `lower_ast.cpp:330` and `type_factory.cpp:204` are
   not `ret_type` assignments.
4. Section 7.3 item 5: `visit(GetChStmt *)` spans `simplify.cpp:251-273`.
5. Add `taichi/ir/frontend_ir.cpp:731-740` (dynamic tensor-offset branch, `:732`
   and `:735`), `taichi/transforms/type_check.cpp:170-171`, and
   `taichi/transforms/make_mesh_block_local.cpp:526`.

**Pass B — three amendments.**

1. Section 3.6 and headline 2: `taichi/transforms/constant_fold.cpp:115` is
   guarded by `:113` against `lhs->val.dt` (set at `:64`) with an i64 arm at
   `:116-118`. Twelve `val_int32()` aborts, not thirteen; fifteen hard aborts,
   not sixteen.
2. Section 3.4 sub-table: same correction as pass A item 2 — `snode.cpp:92` is a
   per-container truncation, and `num_elements_from_root` is the missing
   root-to-leaf site.
3. Section 3.5 and escalation 10: `visit(GetChStmt *)` spans
   `simplify.cpp:251-273`.

**Both, optional but cheap.** One row for the `auto_diff.cpp` i32 index
constructions (`:587`, `:639`, `:646`, `:787`, `:839`, `:846`, `:1828`, `:1857`),
so the 64-token sweep is visibly closed.

Consensus is one pass away. The central structural finding — the type system can
represent i64 but models no index type and no address type, `PointerType`
carries no width, and the 32-bit assumption sits on the index/shape/stride side
while byte offsets are already `size_t` except at `lower_matrix_ptr.cpp:536-540`
— survived round one and survives round two. I tried to break it twice and
could not.

---

## 8. Divergence from adversary2-01-2

`adversary2-01-2.md` does not exist at the time of writing. Recorded as
instructed.
