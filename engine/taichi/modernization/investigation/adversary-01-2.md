# Adversary 01-2 — frontend IR and type system

Judging `report-01-ir-types.md` (pass A) and `report-01b-ir-types.md` (pass B)
against the source. Every file:line below was opened. Paths relative to
`/opt/project/taichi/`.

---

## 0. Verdicts, stated first

| Question | Verdict |
|---|---|
| Is the work CORRECT? | **Pass B: yes, with three defects.** **Pass A: no.** A's headline claim and one of its inferences are contradicted by the source, and one is contradicted by A's own notes. |
| Is the work COMPLETE? | **Neither.** Both reports drop material their own notes contain, and both share four omissions neither noticed. |
| Is major revision required? | **Yes for pass A.** Its section 1 headline and section 5.2 items 1 and 3 must be withdrawn or rewritten. **Minor revision for pass B.** |

Both reports are competent and the great majority of their file:line references
survive checking. The failures are concentrated in the load-bearing claims —
which is where they matter.

---

## 1. The central failure: "a single enforcement point" is false

Pass A, section 1 and section 2.2 item 1:

> `taichi/transforms/type_check.cpp` is the single enforcement point. […]
> **Nothing else in the compiler imposes i32 on an index.** If a 64-bit index is
> to survive, this file decides it.

Pass B, headline 2, softer but the same shape:

> The hard enforcement point is one loop in one pass.

Both are wrong, and the way they are wrong changes what item 6.2 costs.

### 1.1 An index that never passes through those casts

`taichi/transforms/type_check.cpp:154` and `:461` cast the indices *of a pointer
statement*. A loop induction variable never reaches them as an index — it is
born i32 somewhere else, and the cast is a no-op on it. Three birth sites, all
inside the territory, none in `type_check.cpp`:

| Site | What it does |
|---|---|
| `taichi/ir/frontend_ir.cpp:151-152` | `FrontendForStmt::add_loop_var` sets every loop variable's `ret_type` to `get_pointer_type(PrimitiveType::i32)` |
| `taichi/transforms/lower_ast.cpp:330` | a range-for that contains a `break` is lowered to a while loop whose induction variable is `AllocaStmt(PrimitiveType::i32)` |
| `taichi/transforms/type_check.cpp:467` | `LoopIndexStmt` (this one *is* in `type_check.cpp`) |

Pass B caught `frontend_ir.cpp:149-153` (touch point 10). Pass A missed it
entirely, which is what lets A's headline stand unchallenged inside A's own
report. Neither caught `lower_ast.cpp:330`.

This is not pedantry. In a struct-for driven engine every index is loop-derived.
Changing `type_check.cpp` alone produces a 64-bit index for exactly zero of them.

### 1.2 A second, independent narrowing on the CPU path

`taichi/transforms/make_cpu_multithreaded_range_for.cpp` re-synthesises the
entire parallel range-for in i32, after `type_check` has already run:

```
:68   TypedConstant(PrimitiveType::i32, 1)
:70   TypedConstant(PrimitiveType::i32, 512)
:72   TypedConstant(PrimitiveType::i32, config.cpu_max_num_threads)
:81   TypedConstant(PrimitiveType::i32, offloaded->begin_value)
:84   GlobalTemporaryStmt(offloaded->begin_offset, PrimitiveType::i32)
:90   TypedConstant(PrimitiveType::i32, offloaded->end_value)
:93   GlobalTemporaryStmt(offloaded->end_offset, PrimitiveType::i32)
```

Line `:84` and `:93` are the sharpest: a *dynamic* range bound, which
`taichi/transforms/offload.cpp:508,522` deliberately routed through a
`std::size_t` global-temporary offset (`taichi/ir/statements.h:1411-1412`), is
read back out of that buffer *reinterpreted as i32*. The 64-bit-clean path both
reports celebrate is closed again here.

The pass is live: `taichi/transforms/compile_to_offloads.cpp:197-199` runs it
whenever `arch_is_cpu(config.arch)`. CPU-only is a first-class target under
PROJECT-PLAN 2, not a fallback.

**Neither report names this file.** Both agents found it during exploration:
`notes-01-ir-types.md:396-403` records it and calls it *"a second, independent
32-bit narrowing on the dynamic path"*; `notes-01b-ir-types.md:480-487` records
it and calls the iteration space *"i32 end to end"*. Both then dropped it from
their reports, and pass A published a headline its own notes falsify.

### 1.3 A third set of value-truncating casts

`insert_type_cast_before` at `type_check.cpp:154,461` is not the only place a
`UnaryOpType::cast_value` to i32 is inserted on an index:

| Site | What is truncated |
|---|---|
| `taichi/transforms/demote_mesh_statements.cpp:16` | mesh index, immediately before it becomes a `GlobalPtrStmt` index at `:18` |
| `taichi/transforms/demote_mesh_statements.cpp:54` | the loaded mapping value, i.e. the converted index |
| `taichi/transforms/demote_mesh_statements.cpp:131` | same, other conversion direction |
| `taichi/transforms/make_mesh_thread_local.cpp:106`, `:114` | mesh offset and count |
| `taichi/transforms/make_mesh_block_local.cpp:212`, `:376`, `:399` | mesh mapping loads |

Again both agents had these in their notes (`notes-01-ir-types.md:529-540`,
`notes-01b-ir-types.md:476-479`) and neither carried the mesh *casts* into a
report inventory. Pass A escalated mesh scope (escalation 7) without listing the
casts; pass B listed the mesh files only under "hardcoded widths", which
mischaracterises `:16` — that is a live truncation, not a hardcoded constant.

### 1.4 What is actually concentrated, stated precisely

The defensible version of the claim, which neither report made:

- **Exactly two** sites insert a value-truncating cast on a *pointer statement's
  index*: `type_check.cpp:154` (SNode, warns) and `:461` (external array,
  silent). Verified by reading every one of the 64 `PrimitiveType::i32`
  occurrences in `taichi/ir/`, `taichi/inc/`, `taichi/analysis/`,
  `taichi/transforms/`.
- **Exactly one** site aborts instead: `type_check.cpp:160-161`, the
  `MatrixPtrStmt::offset` assert.
- Everything else is a *birth site* or a *re-synthesis site*, and there are at
  least eleven of those (section 1.1 to 1.3 above, plus `type_check.cpp:119,
  467, 471, 475, 521, 525`).

Pass A's count of 64 `PrimitiveType::i32` occurrences is correct; I reproduced
it exactly. The classification built on it is not.

---

## 2. Pass A's inferred item 1 is false; pass B is right

This is a direct, resolvable contradiction between the two reports.

**Pass A, section 5.2 item 1:**
> A 64-bit constant range-for bound is silently truncated to its low 32 bits.

**Pass B, section 5.1:**
> `TypedConstant::val_int32()` asserts on `dt` (`taichi/ir/type.cpp:462-465`), so
> the range-for bound path fails loudly rather than truncating.

**Pass B is right.** The chain:

- `taichi/ir/statements.h:993` — `ConstStmt`'s constructor sets
  `ret_type = val.dt`, so the two start in agreement.
- `taichi/transforms/type_check.cpp:40-44` — `mark_as_if_const` writes
  `stmt->ret_type` only. `val.dt` is untouched. They now disagree.
- `taichi/transforms/offload.cpp:109,117` — read `val->val.val_int32()`.
- `taichi/ir/type.cpp:462-465` — `val_int32()` opens
  `TI_ASSERT(get_data_type<int32>() == dt)`, and `dt` here is `TypedConstant::dt`
  (`taichi/ir/type.h:551`), i.e. the untouched `val.dt`, **not** the retagged
  `ret_type`.

A's inference turns on `mark_as_if_const` retagging the value — but the retag
lands on the field `val_int32()` does not consult. The assert fires.

A's report hedges this with "In a release build a structure larger than 2^31
cells compiles and silently produces truncated addresses" (section 1). That
hedge does not save item 5.2 item 1, because `TI_ASSERT` is **not**
NDEBUG-gated. `taichi/common/logging.h:100-107` expands it unconditionally to
`TI_ERROR`. There is no build configuration in which this path truncates
silently.

A's escalation-adjacent claim survives for a *different* path: `snode.cpp:92` is
a plain C++ `static_cast<int>` and does truncate silently in every build. A
conflated the two.

---

## 3. Pass A's inferred item 3 has the wrong mechanism

**Pass A, section 5.2 item 3:**
> A sparse (not all-dense) path over more than 2^31 cells truncates silently […]
> because `scalar_pointer_lowerer.cpp:33-39` has no equivalent assert and its
> `total_shape` is a plain `int` array.

`total_shape` is indexed **by axis** (`taichi/transforms/scalar_pointer_lowerer.cpp:33`
declares `std::array<int, taichi_max_num_indices>`; `:36-37` multiplies
`total_shape[j] *= s->extractors[j].shape` over the root-to-leaf path). It holds
the per-axis extent, not the cell count. A structure with 2^40 cells spread over
four axes of 2^10 each leaves every `total_shape[j]` at 1024. The array does not
overflow, and A's stated mechanism does not fire.

The same correction applies to A's section 3.4 row for
`demote_dense_struct_fors.cpp:19-24` and to B's section 3.4 rows for
`scalar_pointer_lowerer.cpp:37` and `demote_dense_struct_fors.cpp:24`. All three
describe a real 32-bit accumulator; none of them is the >2^31-cells failure. In
`demote_dense_struct_fors.cpp` the cell count is carried separately, in `int64`,
at `:18` and `:26`, and that is what `:29` guards.

The conclusion A drew may still be true. The evidence A gave does not support it.
See section 5.1 for what neither report established.

---

## 4. Where the two reports disagree, adjudicated

| # | Subject | A says | B says | Source | Right |
|---|---|---|---|---|---|
| 1 | `SNodeOpStmt` default i32 | `type_check.cpp:119` | `:114` | assignment is at `:114` | **B** |
| 2 | `ExternalTensorShapeAlongAxisStmt` | `:124` | `:119` | assignment is at `:119` | **B** |
| 3 | `mark_as_if_const` | `:39-43` | `:40-44` | declared at `:40`, closes `:44` | **B** |
| 4 | `TypedConstant` union | `:556-570` (§3.1) and `:546-561` (§5.2) | `:557,560,568` | union spans `type.h:556-569` | **B**; A contradicts itself |
| 5 | `MatrixPtrStmt` TODO / discriminator | `:512-521` / `:522-530` | `:512-520` / `:521-529` | TODO `512-520`, fn `521-529` | **B** |
| 6 | `simplify.cpp` child-type assert | `:233-237` | `:233-235` | asserts at `233-235` | **B** |
| 7 | `simplify.cpp:261` guarded? | unguarded, no assert in view | guarded by `:233-235` | the assert is inside the `SNodeLookupStmt` visitor (`:222-249`); the `GetChStmt` visitor (`:251-265`) has none | **A literally.** B's claim holds only by data flow — the `IntegerOffsetStmt` `GetChStmt` consumes is produced by the guarded visitor — and B marked it inferred. Neither proved it. |
| 8 | 64-bit const range-for bound | silently truncated | asserts | see section 2 | **B** |
| 9 | `value_diff` i32-only visitors | one visitor, `FindDirectValueBaseAndOffset`, no line numbers | two visitors: `ValueDiffLoopIndex::visit(ConstStmt*)` at `:81-87` feeding `lower_access.cpp:237`, and `FindDirectValueBaseAndOffset::visit(ConstStmt*)` at `:142-146` feeding `alias_analysis.cpp:59-60` | both visitors exist at exactly those lines | **B.** A attributed the `lower_access` consequence to the wrong visitor and gave no line numbers, in violation of standing instruction 5 |
| 10 | `(int32)` casts in `make_mesh_block_local.cpp` | `:80, :200, :575` | `:80, :118, :200, :265, :301, :575` | `:118, :265, :301` use the functional form `int32(offset_in_bytes)`; all six are real | **B**; A's list is incomplete |
| 11 | `kMaxNumSnodeTreesLlvm` | noted as unused in territory, nothing more | flagged as a **second, lower, separate ceiling** at `constants.h:13`, used at `runtime.cpp:562-563`, and escalated | both lines exist; PROJECT-PLAN 6.1 names only `taichi_max_num_snodes` | **B**; a genuine addition to the plan's record |
| 12 | `check_out_of_bound` printf sites | `:91, :151-155, :199` | `:91, :153, :201` | `%d` appended at `91`, `153`, `201` | **B** |
| 13 | Constants from `constants.h` used in territory | not enumerated | "only three" | four: `taichi_max_num_indices` has 13 lines / 12 sites | **A by omission.** B's §5.1 contradicts B's own §3.7 |
| 14 | Scratch-pad and BLS analyser widths | inventoried (`scratch_pad.h:33-42,47-55`; `bls_analyzer.h:15-18`) | not inventoried; dismissed as inferred non-limit | all lines correct | **A**; B has a hole here |
| 15 | Mesh statement classes | `statements.h:2064, 2083`; mesh expressions `frontend_ir.cpp:1358,1362,1392`; escalated as scope question | absent from report | all lines correct | **A**; B never raises mesh scope |

Score on the contested points: B is right on eleven, A on three, one unresolved.

A's `taichi_max_num_indices` inventory (§2.3) is complete and correct — I
reproduced the full 13-line grep and A's table accounts for every site. B's §3.7
list is equally complete.

---

## 5. What BOTH missed

### 5.1 Where a whole-structure linear index is actually formed — unanswered

This is the question item 6.2 exists to answer in this territory, and neither
report answers it.

`ScalarPointerLowerer` emits **one `LinearizeStmt` per SNode level**
(`taichi/transforms/scalar_pointer_lowerer.cpp:55-56` resets `strides` inside the
per-level loop; `:82` pushes the level's own `LinearizeStmt`). Each level's
linear index is therefore bounded by that level's container size, not by the
size of the structure. The address is hierarchical: linearize, `SNodeLookupStmt`,
`GetChStmt`, repeat. `type_check.cpp:483-496` and `:497-515` confirm the pointer
statements carry only a `PointerType`, with no integer width at all.

So a structure of 2^40 cells does not, by itself, produce a 2^40 value anywhere
in the frontend IR. It produces a chain of small ones. The places where a
flattened whole-structure extent *is* formed are few and both reports have the
pieces without assembling them:

- `taichi/transforms/demote_dense_struct_fors.cpp:26,35` — `total_n` over the
  whole path, into an `int32` field, guarded by `:29`.
- `taichi/transforms/lower_ast.cpp:270-273` — the external-array struct-for
  extent, built as a running i32 product of `ExternalTensorShapeAlongAxisStmt`
  values (each pinned i32 by `type_check.cpp:119`). **No guard, no assert.**
  Neither report mentions this file.
- `taichi/ir/snode.cpp:92` — `acc_shape` truncated per node, warned at `:95-100`.

Until someone states which of these the workload actually hits, the "map of 32
bit assumptions" (PROJECT-PLAN 8.1 item 1) is a list, not a map. Both reports
delivered the list.

### 5.2 The SNode bounds check ceiling

`taichi/transforms/check_out_of_bound.cpp:123-126`:

```
int size_i = snode->shape_along_axis(i);
int upper_bound_i = size_i;
auto upper_bound = new_stmts.push_back<ConstStmt>(TypedConstant(upper_bound_i));
```

The per-axis upper bound for every field access is an `int` from
`SNode::shape_along_axis` (`taichi/ir/snode.h:317`, returns `int`), materialised
as an i32 `ConstStmt`. Both reports listed the *products* at `:54-57` and
`:170-173`; neither listed the bound that is actually compared against the index
on the SNode path. B listed `snode.h:317` as a signature without connecting it.

### 5.3 A second implementation of `LinearizeStmt`, in the analysis layer

`taichi/analysis/arithmetic_interpretor.cpp:98-109` evaluates `LinearizeStmt`
independently of `simplify.cpp`. It accumulates in `int64_t` at `:99,106` and
then stores the result under `stmt->ret_type` at `:108` — which
`type_check.cpp:521` pinned to i32. Neither report mentions this file. Any width
change has two implementations of the same statement to keep in step, not one.

### 5.4 The existing C++ test harness

`ArithmeticInterpretor` has no callers in `taichi/` at all. Its only consumers
are `tests/cpp/transforms/scalar_pointer_lowerer_test.cpp` and
`tests/cpp/transforms/make_block_local_test.cpp`, which assert the exact numeric
output of the index-lowering item 6.2 must change
(`scalar_pointer_lowerer_test.cpp:62` builds the index with
`builder.get_int32(loop_index)`; `:91-98` checks the linearised results).
`tests/cpp/ir/` additionally holds `ir_type_promotion_test.cpp` and
`type_test.cpp`.

Neither report mentions any test. For an assignment whose output sizes a width
change, the existing regression harness for that exact code path is not optional
context.

### 5.5 Smaller shared gaps

- `taichi/ir/type_utils.cpp:30` — `data_type_size` **returns `int`**, and at
  `:44-48` computes a `TensorType`'s size as `get_num_elements() *
  data_type_size(element)` in `int`. Both reports hold this function up as the
  correct, derived alternative to the hardcoded `sizeof(int32)` in
  `simplify.cpp`. It is derived, and it is also 32-bit.
- `taichi/ir/type_factory.cpp:198-215` — `get_ndarray_struct_type` is the
  host-to-kernel ABI struct, not merely a hardcoded literal. Both flag `:204`;
  neither says the i32 per-dimension shape crosses the process boundary.
- `taichi/transforms/utils.cpp:5,14` — `generate_mod` / `generate_div` take
  `int y`, so a stride above 2^31 cannot be expressed at all, independent of the
  IR type. B lists the file (touch point 14); A reaches it only indirectly
  through `demote_dense_struct_fors.cpp:60,63`.
- `taichi/transforms/demote_no_access_mesh_fors.cpp:31-34` — a fourth writer of
  `begin_value`/`end_value`. In B's notes only.

---

## 6. Additional defects, per report

### Pass A

1. Section 3.2 lists `taichi/ir/expr.cpp:93` as a "hardcoded
   `ret_type = PrimitiveType::i32`" and calls it "default integer literal in the
   frontend". It is `Expr::Expr(int32 x)`, one overload in a set that includes
   `Expr::Expr(int64 x)` at `:96`. It is not a default and not a narrowing.
   Listing it inflates the inventory.
2. Section 5.1 asserts "All line numbers were read directly and then re-verified
   in a second pass (N019)." Items 1 to 6 and 9, 10, 12 in the table above are
   line-number errors that a second pass would have caught. The +5 offset on
   `type_check.cpp:119/124` reproduces identically in
   `notes-01-ir-types.md:417` and `:526`, which both give `type_check.cpp:123-125`
   for the same visitor, so the error is in the original reading and the claimed
   re-verification did not happen.
3. Section 3.8 gives no line numbers at all for the `value_diff.cpp` visitor.
   Standing instruction 5 requires them.
4. A's pass-ordering note N017 is the strongest single piece of work in either
   pair of files — it establishes that the i32 coercion is re-applied at seven
   `irpass::type_check` calls in `compile_to_offloads.cpp` plus several
   in-pass invocations, and states outright "it is not a one-shot". I confirmed
   the seven. **None of it reached A's report**, which instead claims a single
   enforcement point. This is the clearest instance of the reporting step losing
   the investigation's best result.

### Pass B

1. Section 5.1's "only three constants from `constants.h` are used in my
   territory" is wrong and contradicts B's own section 3.7. Four are used;
   `taichi_max_num_indices` has 12 sites.
2. Section 3.5 files the mesh casts under "hardcoded widths, not derived". Three
   of them (`demote_mesh_statements.cpp:16,54,131`) are live value truncations
   and belong in section 3.3.
3. Headline 3 — "The frontend does NOT forbid i64 indices […] The narrowing is a
   mid-level IR decision, not a language restriction" — is true for an explicit
   `IndexExpression` (`frontend_ir.cpp:936-947` checks only `is_integral`, which
   I verified) and false for a loop variable, which B's own touch point 10
   (`frontend_ir.cpp:151-152`) pins to i32 in the frontend. B states both and
   reconciles neither.
4. No inventory of `taichi/ir/scratch_pad.h`, `taichi/ir/analysis.h:185`, or
   `taichi/analysis/bls_analyzer.h`, and no mention of mesh statement classes or
   mesh scope as an open question.

### Verified good, in both

I tried to break these and could not. All confirmed by direct reading:

- `taichi_max_num_snodes` and `kMaxNumSnodeTreesLlvm` have zero uses in the
  territory; tree-wide the only sites are `constants.h:12-13`,
  `struct_llvm.cpp:266`, `runtime.cpp:562-563,567-569`. Both reports right, and
  the plan's section 6.1 record is right.
- The 12 `taichi_max_num_indices` sites. Both complete.
- `snode.cpp:89-101` — `int64 acc_shape`, truncated at `:92`, warned at
  `:95-100`, stored untruncated into `num_cells_per_container` at `:101`. Both
  right, including the disagreement A flags in 5.2 item 5.
- `snode.h:41,45,49,78,79,81,88-89,97-100,313,317`. Both right.
- `statements.h:376,379,456,993,1260,1280,1370,1411-1412,1415-1416,1439,1684,1787,2064,2083`.
  Both right where cited.
- `type.h:177-209` `PointerType` with no width, `addr_space_` TODO at `:207`;
  `type_factory.cpp:86-95` interning on `(element, is_bit_pointer)`;
  `type_utils.cpp:30-35` `data_type_size` refusing pointers. B's section 4 is a
  precise and correct statement of the gap. A reaches the same conclusion with
  less evidence.
- `type_factory.cpp:160-177` `get_primitive_int_type` handling 64;
  `ir_builder.h:133-136` including `get_int64`/`get_uint64`;
  `type_utils.cpp:113-137` including the two Vulkan 64-bit comments at `:133`
  and `:136`. Both right.
- B's section 3.6 list of `val_int32()` call sites — `offload.cpp:109,117`;
  `scalarize.cpp:162,1097,1120,1147,1231,1311`;
  `auto_diff.cpp:576,776,1770`; `control_flow_graph.cpp:462`;
  `constant_fold.cpp:115` — is **exactly** the complete set. A has two of the
  fourteen.
- B's `PhysicalCoordinates` finding: `runtime.cpp:288-290` declares
  `i32 val[taichi_max_num_indices]`, and `codegen_llvm.cpp:2356-2376` asserts
  the LLVM struct matches. This is the device-side mirror of the frontend's i32
  loop index and is the single most consequential fact in either report for item
  6.2. **A missed it completely.**
- `frontend_type_check.cpp:71-77` warns without casting. Both right.
- `offline_cache_util.cpp:110-126` serialises the extractors and geometry;
  `gen_offline_cache_key.cpp:540` emits the SNode id. Both right; B's range is
  the accurate one.

---

## 7. Escalations

Unresolved. Recorded, not decided.

1. **Both reports' escalation lists are sound and should go forward**, but A's
   escalation 7 (mesh scope) and B's escalations 1 (`kMaxNumSnodeTreesLlvm`) and
   9 (`taichi_max_num_indices` is not host-only) each appear in only one report.
   All three need the planner.

2. **Whether the CPU path is in scope for item 6.2.**
   `make_cpu_multithreaded_range_for.cpp` narrows the iteration space to i32 on
   the arch PROJECT-PLAN 2 calls a first-class target. Neither report raised it,
   so nobody has asked whether widening it is in scope or whether an i32 CPU
   iteration space is acceptable. **Planner.**

3. **Whether `PhysicalCoordinates` fixes the frontend's index width.** If the
   device struct at `runtime.cpp:288-290` keeps `i32` slots, then
   `type_check.cpp:467`'s `LoopIndexStmt` i32 is not a free choice for the LLVM
   path. This spans territories 01, 02 and 03 and no single agent owns it.
   **Planner.**

4. **Whether the existing C++ tests in `tests/cpp/transforms/` and
   `tests/cpp/ir/` are a constraint or a casualty.** They encode the current
   i32 lowering. Neither report says whether they are maintained. **Planner.**

5. **What "the address" means for item 6.2.** The frontend forms hierarchical
   per-level indices, not one flat address (section 5.1). Whether item 6.2 wants
   a wider per-level index, a wider flattened index, or a wider byte address is a
   design question that changes which of the inventoried sites matter, and it has
   not been asked. **Planner.**

---

## 8. Recommendation

Pass B goes forward with the four corrections in section 6. Its inventory is
accurate, its verified/inferred split is honest, and its two unique findings
(`PhysicalCoordinates`, the complete `val_int32()` set) are the most valuable
material either agent produced.

Pass A requires major revision. Its inventory sections 3.3 to 3.6 are good and
in places better than B's, but section 1, section 2.2 item 1, and section 5.2
items 1 and 3 are wrong, and the report's own notes (N017) contain the
correction. A should be sent back with B's notes and with sections 1, 2 and 5 of
this analysis attached.

Both reports need section 5 of this analysis regardless of which is revised.

---

## 9. Divergence from adversary 01-1

`adversary-01-1.md` landed after I finished section 8. I have read it and
re-checked every point on which we differ. Where it corrects me I say so.

### 9.1 Where we agree, independently

We reached the same conclusion by separate routes on the load-bearing items,
which raises confidence in all of them:

- Pass A's section 5.2 item 1 is false; the i64 range-for bound raises a hard
  `TI_ERROR`, not a silent truncation. Same chain: `type_check.cpp:40-44` →
  `statements.h:993` → `type.cpp:462-465` → `logging.h:100-107`. Adversary 1
  adds that A's own section 5.1 lists the assert among its VERIFIED facts,
  which is a sharper way to put it than mine.
- The "single enforcement point" headline is false in both reports, and
  `make_cpu_multithreaded_range_for.cpp:68-93` plus the six mesh cast sites are
  in both agents' notes and neither report. We found this independently and
  cite the same note lines.
- `simplify.cpp:260-261` is not guarded by the assert in the
  `SNodeLookupStmt` visitor; A's literal reading is right, B overstates, and B
  contradicts its own section 5.2 hedge.
- B is right on the disputed `type_check.cpp` line numbers, on
  `mark_as_if_const`, on the `MatrixPtrStmt` TODO range, on the two
  `value_diff.cpp` visitors, and on the six `make_mesh_block_local.cpp` casts.
- `kMaxNumSnodeTreesLlvm` is a second undocumented ceiling; B raised it, A did
  not; B's own use count is internally inconsistent.
- Item 6.1 genuinely does not touch this territory. Both of us verified it.

### 9.2 Where adversary 01-1 is right and I was wrong or silent

Four corrections to my own analysis. I checked each against the source before
conceding.

1. **`taichi/inc/rhi_constants.inc.h:14` declares `PER_DEVICE_CAPABILITY(spirv_has_int64)`.**
   I missed this and so did both reports. It is decisive:
   `taichi/codegen/spirv/spirv_ir_builder.cpp:311-313` and `:325-327` raise
   `TI_ERROR("Type {} not supported.")` when an i64 or u64 type is requested and
   the capability is absent. Verified by reading both. `taichi/inc/` is inside
   territory 01 by both agents' own scope statement, so this is a miss inside
   scope, not a seam observation. It also falsifies B's section 4 claim that "a
   target that lacks Int64 has nowhere in the current type system to say so" —
   the capability is declared and enforced; what is missing is only the link from
   the type layer to it. This is the strongest single finding in either
   adversarial file and it is adversary 01-1's.

2. **`taichi/transforms/type_check.cpp:123-125` — `visit(GlobalPtrStmt *)`
   returns early when `stmt->is_bit_vectorized`, before the cast loop at
   `:147-156`.** I read this code and did not flag the exception. Both reports
   say "every" `GlobalPtrStmt` index is cast. It is not.

3. **`taichi/ir/type_utils.cpp:30-35` does not "refuse" pointers.** I listed
   B's phrasing under "verified good". Adversary 1 is right: `:35` is
   `t.set_is_pointer(false);`, which clears the flag and then sizes the pointee.
   The TODO at `:31-34` records that a loud failure was intended and never
   written. B's conclusion holds; B's mechanism does not, and the difference
   matters because a caller asking a pointer's size today silently gets the
   pointee's.

4. **`taichi/transforms/lower_matrix_ptr.cpp:536-540`** computes the
   dynamic-index byte offset as `stmt->offset * TypedConstant(origin->dynamic_index_stride)`
   and forces `offset->ret_type = stmt->offset->ret_type`, which
   `type_check.cpp:160-161` has already asserted to be i32. Verified. A byte
   offset in i32 while every other byte offset in the IR is `size_t`. I saw the
   file in a grep and did not pursue it. Neither report has it.

Adversary 01-1 is also right that A's `type_utils.cpp` citations are off — the
Vulkan comment is at `:133`, `%lld` at `:134`, the u64 comment at `:136` — which
I had wrongly filed under "verified good, in both". B's `:130-137` is the correct
range. Adversary 1's `promoted_type` finding (`type_factory.cpp:249-251` errors
on a pointer operand) is also verified and bounds A's "widens on its own" claim.

### 9.3 Where I am right and adversary 01-1 is silent

Six items absent from `adversary-01-1.md`. Each verified above in sections 3
and 5.

1. **Pass A's section 5.2 item 3 has the wrong mechanism** (my section 3).
   `total_shape` in `scalar_pointer_lowerer.cpp:33-37` and
   `demote_dense_struct_fors.cpp:19-24` is indexed **by axis**, so it does not
   overflow on a >2^31-cell structure spread over several axes. Adversary 1
   repeats both accumulators in its section 6 list without noticing that A built
   a failure-mode inference on top of one of them. A's conclusion may hold; its
   stated evidence does not.

2. **`taichi/transforms/lower_ast.cpp` is in neither report and in neither
   adversarial file.** Three sites: `:330` pins the induction variable of a
   break-containing range-for to `AllocaStmt(PrimitiveType::i32)`; `:270-273`
   builds the external-array struct-for extent as a running i32 product of
   `ExternalTensorShapeAlongAxisStmt` values with no guard and no assert. That
   second one is a whole-structure extent formed in 32 bits, which is exactly
   what item 6.2 is about.

3. **`taichi/transforms/check_out_of_bound.cpp:123-126`** — the per-axis upper
   bound for every SNode field access is an `int` from
   `SNode::shape_along_axis` (`snode.h:317`) materialised as an i32 `ConstStmt`.
   Both reports listed the products at `:54-57` and `:169-173`; nobody listed
   the bound actually compared against the index.

4. **`taichi/analysis/arithmetic_interpretor.cpp:98-109`** is a second,
   independent implementation of `LinearizeStmt` semantics, accumulating in
   `int64_t` at `:99,106` and storing under the i32 `stmt->ret_type` at `:108`.
   Two implementations of the same statement must move together under a width
   change.

5. **The existing C++ test harness.** `ArithmeticInterpretor` has no callers in
   `taichi/`; its only consumers are
   `tests/cpp/transforms/scalar_pointer_lowerer_test.cpp` and
   `tests/cpp/transforms/make_block_local_test.cpp`, which assert the exact
   numeric output of the index lowering item 6.2 must change
   (`scalar_pointer_lowerer_test.cpp:62,91-98`). `tests/cpp/ir/` holds
   `ir_type_promotion_test.cpp` and `type_test.cpp`. No report and no other
   adversary mentions a test.

6. **`taichi/ir/type_utils.cpp:30` — `data_type_size` returns `int`**, and at
   `:44-48` computes a `TensorType`'s size in `int`. Both reports hold it up as
   the correct derived alternative to `simplify.cpp`'s hardcoded `sizeof(int32)`.
   It is derived, and it is also 32-bit. Also: B's section 5.1 "only three
   constants from `constants.h` are used in my territory" is wrong and
   contradicts B's own section 3.7 — four are used, `taichi_max_num_indices`
   having 12 sites. Adversary 1 does not record either.

### 9.4 Two places where the source supports me over adversary 01-1

1. **The `val_int32()` call-site count.** Adversary 01-1 section 3.2 says
   "Twelve sites ... eleven of which A does not mention", while quoting B's list
   verbatim. That list is thirteen call sites: `offload.cpp:109,117` (2),
   `scalarize.cpp:162,1097,1120,1147,1231,1311` (6),
   `auto_diff.cpp:576,776,1770` (3), `control_flow_graph.cpp:462` (1),
   `constant_fold.cpp:115` (1). My grep over the four directories returns exactly
   those thirteen plus the definition at `type.cpp:462`. The substantive point —
   that B's list is complete and A has two of it — is right in both our files;
   the count is not.

2. **`taichi_max_num_snodes` "looseness".** Adversary 01-1 section 2.9 marks
   both reports "loose" for saying three sites where the grep gives four lines.
   `runtime.cpp:567-569` is three consecutive array declarations in one struct,
   and both reports cite the range `:567-569` explicitly. Counting that as one
   site is a convention, not an error, and PROJECT-PLAN 6.1 itself uses the same
   range notation. I would not hold this against either report.

### 9.5 Net effect on my verdicts

Unchanged in direction, sharper in degree. Adversary 01-1's `spirv_has_int64`
finding makes the incompleteness verdict worse for both reports, not better: B
wrote a whole subsection (its 6.3) asserting the capability is inexpressible,
about a file inside its own declared territory. Combined with my section 5, the
territory has at least ten in-scope sites that neither pass reported, four of
which sit in files neither pass opened.

Both adversarial files agree that pass A requires major revision and pass B
requires amendment. Neither of us could break the central structural finding —
that the type system can represent i64 but models no index or address type — and
I take that finding as sound.
