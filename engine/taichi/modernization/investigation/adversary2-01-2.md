# Adversary 01-2 — ROUND TWO — frontend IR and type system

Judging revision 2 of `report-01-ir-types.md` (pass A) and
`report-01b-ir-types.md` (pass B) against the source, against my round-one
objections in `adversary-01-2.md`, and against the round-one objections in
`adversary-01-1.md`.

Every file:line below was opened in this round. Paths are relative to
`/opt/project/taichi/`.

---

## 0. Verdicts, stated first

| Question | Verdict |
|---|---|
| Is pass A now CORRECT? | **Yes on every load-bearing claim.** Three residual defects, all presentational or categorial, none of which changes a conclusion. |
| Is pass B now CORRECT? | **No — one factual error remains.** B still classifies `taichi/transforms/constant_fold.cpp:115` as a hard abort, and two derived counts ("thirteen call sites fail loudly", "sixteen hard aborts") are wrong as a result. Pass A opened the site; pass B took my round-one count on trust. |
| Are they now COMPLETE? | **No, but narrowly.** Two shared gaps: the in-pass `type_check` re-run surface is understated by roughly an order of magnitude (section 3.2), and `AxisExtractor::num_elements_from_root` — the unguarded root-to-leaf extent that feeds the bound both reports already cite — is in neither (section 7.2). Every missed site is inside territory 01. Pass A additionally still omits `taichi/ir/frontend_ir.cpp:731-740`. |
| Is general consensus reached? | **Not yet. Three items, section 6.** All are one-line-to-one-paragraph fixes against lines I give below. No further investigation is required. |

The revision is substantial and honest. Every one of my round-one objections
was met with new source work rather than a prose concession, and both agents
adjudicated *against* me in places where they were right to. The remaining
items are small enough that I would expect one more pass to close them.

---

## 1. Round-one objections: addressed, or merely acknowledged?

A concession that does not change the analysis is not a fix. I checked each of
my round-one points against the revised text and then against the source.

### 1.1 My objections to pass A

| Round-one objection | Status | Evidence |
|---|---|---|
| §1 "single enforcement point" is false | **Addressed.** Not conceded in prose — replaced. A's headline now reads "Ten sites in four files insert a narrowing cast … Twelve sites abort … re-run seven times", and section 7.1 item 3 records the withdrawal. | Report §1, §3.1, §3.2, §5; note N024 |
| §1.1 birth sites (`frontend_ir.cpp:151-152`, `lower_ast.cpp:330`) | **Addressed.** Both now in §3.3 with the consequence spelled out: "A loop induction variable never reaches the casts … the cast is a no-op." Verified `frontend_ir.cpp:151-152` and `lower_ast.cpp:330` directly. | Report §3.3 |
| §1.2 `make_cpu_multithreaded_range_for.cpp` dropped from report | **Addressed.** Now §3.4 in full, all seven lines, plus escalation 4. I re-read `:68,70,72,81,84,90,93` — every citation exact. | Report §3.4 |
| §1.3 the mesh cast sites | **Addressed.** Now §3.1, all eight, and A corrected my implicit framing by counting them properly. | Report §3.1 |
| §2 the silent-truncation inference | **Withdrawn outright**, with the full chain restated. §7.1 item 1. | Report §7.1 |
| §3 the `total_shape` mechanism | **Withdrawn outright.** §3.6 now carries an explicit "Correction from revision 1" paragraph saying the array is per-axis and the stated failure mode does not fire. | Report §3.6, §7.1 item 2 |
| §4 fifteen contested line numbers | **Addressed.** I re-derived every one. A now gives `:114`, `:119`, `:40-44`, `type.h:556-569`, `statements.h:512-520`/`:521-529`, `simplify.cpp:233-235`, both `value_diff` visitors with lines, and all six `make_mesh_block_local` casts. All correct. | Report §3.3, §3.10, §6, §8 |
| §5.1 addressing is hierarchical | **Addressed, and better than I put it.** A adopted it into the headline and built §4 on it. See section 2.5 below. | Report §1, §4 |
| §5.2 `check_out_of_bound.cpp:123-126` | **Addressed.** §3.11, with the point I made — that nobody had listed the bound actually compared — stated explicitly. | Report §3.11 |
| §5.3 second `LinearizeStmt` implementation | **Addressed and extended.** A found two more, in both codegen paths. Verified `codegen_llvm.cpp:1736-1744` and `spirv_codegen.cpp:532-541`, the latter explicitly `i32_type()`. | Report §4 |
| §5.4 the C++ test harness | **Addressed.** §4 closing paragraph and escalation 10. | Report §4, §8 |
| §5.5 `data_type_size` returns `int`; ndarray ABI; `utils.cpp` `int y`; `demote_no_access_mesh_fors` | **All four addressed.** §3.5, §3.7, §3.4. | Report §3.4, §3.5, §3.7 |
| §6 A-1 `expr.cpp:93` inflates the inventory | **Addressed — removed**, §7.1 item 5. |
| §6 A-2 the false re-verification claim | **Addressed — struck**, and the reason given (content checked, attribution not). §7.1 item 6. |
| §6 A-3 no line numbers for `value_diff` | **Addressed.** §3.10 gives both visitors and all six consumers. |
| §6 A-4 note N017 never reached the report | **Addressed.** It is now section 5 of the report. |

Nothing on pass A's list is a bare acknowledgement.

### 1.2 My objections to pass B

| Round-one objection | Status | Evidence |
|---|---|---|
| B-1 "only three constants from `constants.h`" contradicts B's own §3.7 | **Addressed.** §5.1 now names four and says explicitly that the previous wording "silently excluded the constant section 3.7 is about". I verified all four: `taichi_max_num_indices` (12 sites), `taichi_global_tmp_buffer_size` at `offload.cpp:358`, `taichi_max_gpu_block_dim` at `frontend_ir.cpp:132`, `default_shared_mem_size` at `make_mesh_block_local.cpp:434`. | Report §5.1 |
| B-2 mesh casts filed under "hardcoded widths" | **Addressed.** Moved into a new §3.3 headed "Live narrowing conversions", with an explicit note that they "were previously and wrongly filed under hardcoded widths". | Report §3.3 |
| B-3 headline 3 unreconciled | **Addressed by reconciliation, not withdrawal.** B ruled against me and was right. Section 2.3 below. | Report headline 3; notes 37 |
| B-4 no `scratch_pad.h`, `analysis.h:185`, `bls_analyzer.h`, no mesh scope question | **Addressed.** New §3.9 carries the scratch-pad inventory; `analysis.h:185` is in §3.2; mesh scope is escalation 4. I verified every `scratch_pad.h` line B gives — `:34-38`, `:47,51,53`, `:75-76`, `:97-98`, `:107-108`, `:131-147`, `:149-157`, `:182`, `:229-234`, `:106`, `:214`. All exact. | Report §3.2, §3.9, §6 |
| The `simplify.cpp` guard overstatement | **Addressed.** §3.5 demotes it to the `:239` site only and carries the correction in the body. |
| `data_type_size` "refuses pointers" | **Addressed.** Corrected in headline 5 and §4. Section 2.4 below. |
| B's partial out-of-territory `taichi_max_num_indices` list (adversary 1's objection) | **Addressed.** §3.7 now adds `struct_llvm.cpp:171`, `runtime.cpp:1012`, `export_lang.cpp:559,1222`. I re-ran the tree-wide grep; B's list is now exactly complete. |
| `kMaxNumSnodeTreesLlvm` count inconsistency | **Addressed** by stating lines rather than a count, with the reason given. |

One of B's items is a bare acknowledgement rather than a fix, and it is the
`constant_fold` count. See section 3.1.

---

## 2. The five places the explore agents ruled against round one

### 2.1 Cast-insertion count: ten, not eight. Mesh: eight, not six. **Pass A is right.**

```
$ grep -rn "cast_type = PrimitiveType::i32" taichi/ir/ taichi/inc/ taichi/analysis/ taichi/transforms/
taichi/transforms/demote_mesh_statements.cpp:16
taichi/transforms/demote_mesh_statements.cpp:54
taichi/transforms/demote_mesh_statements.cpp:131
taichi/transforms/make_mesh_thread_local.cpp:106
taichi/transforms/make_mesh_thread_local.cpp:114
taichi/transforms/make_mesh_block_local.cpp:212
taichi/transforms/make_mesh_block_local.cpp:376
taichi/transforms/make_mesh_block_local.cpp:399
```

Eight mesh sites, not six. Plus `taichi/transforms/type_check.cpp:154` and
`:461`, both routed through `insert_type_cast_before`
(`taichi/transforms/type_check.cpp:234-244`), which is the only other producer
of a `UnaryOpType::cast_value` pinned to i32 in the territory — confirmed by
`grep -rn "UnaryOpType::cast_value" taichi/ir/ taichi/analysis/ taichi/transforms/`,
whose only other in-territory constructors are `insert_type_cast_after`
(`type_check.cpp:250`, never called with `PrimitiveType::i32` — its five callers
are `:30`, `:286`, `:366`, `:371`, `:394`, `:398`, all passing a computed type),
`alg_simp.cpp:18` and `ir_builder.cpp:194`.

**Total ten.** Adversary 1's headline "8 sites in 4 files" is contradicted by
adversary 1's own enumeration on the same line, which lists ten; and its "Six
mesh cast sites" is contradicted by its own list of eight. Pass A caught both
and is right on both. I did not state a count in round one, so I am not
corrected here, but the enumeration in my §1.3 is the eight mesh sites and I
should have said so.

Pass B independently reaches the same numbers: headline 2 says "eight more
`cast_value` truncations in the mesh passes" on top of the two. The two reports
agree.

### 2.2 `constant_fold.cpp:115` is not an abort site. **Pass A is right; I was wrong; pass B is still wrong.**

`taichi/transforms/constant_fold.cpp:111-125` is the `HANDLE_INTEGRAL_BINARY`
macro:

```
113:    if (dt->is_primitive(PrimitiveTypeID::i32)) {                              \
114:      res = TypedConstant(                                                     \
115:          dst_type, PREFIX(lhs->val.val_int32() OP_CPP rhs->val.val_int32())); \
116:    } else if (dt->is_primitive(PrimitiveTypeID::i64)) {                       \
117:      res = TypedConstant(                                                     \
118:          dst_type, PREFIX(lhs->val.val_int() OP_CPP rhs->val.val_int()));     \
```

and `dt` is `lhs->val.dt` (`:65`) — precisely the field
`TypedConstant::val_int32()` asserts against (`taichi/ir/type.cpp:462-465`,
checking `TypedConstant::dt` at `taichi/ir/type.h:551`). The i32 branch cannot
be entered with a non-i32 `dt`. It is correct 64-bit dispatch.

I listed this site in round one under "B's section 3.6 list of `val_int32()`
call sites … is **exactly** the complete set" and treated the whole set as
aborts. That was wrong. The list of *call sites* is complete; the
classification as aborts is not. Pass A opened each site and found it; that is
the single best piece of work in either revision.

A's parallel finding at `taichi/codegen/llvm/codegen_llvm.cpp:1052-1060` also
holds: the same guarded shape, i32 branch at `:1052-1054`, i64 branch at
`:1058-1060`.

A's resulting figure of twelve aborts is right. I verified each of the twelve
guards individually: `control_flow_graph.cpp:456-457`, `scalarize.cpp:160`,
`:1096`, `:1117`, `:1144`, `:1230`, `:1307`, and `auto_diff.cpp:575-576`,
`:776`, `:1770` all test `is<ConstStmt>()` and never the type;
`offload.cpp:109`, `:117` have no guard at all.

A's structural point is also right and is new: ten of the twelve read a
`MatrixPtrStmt::offset`, so they are all downstream of the single assert at
`taichi/transforms/type_check.cpp:160-161`. (A's own note N025 says "11 of the
12"; the report says ten, and ten is the correct arithmetic — 1 + 6 + 3.)

**Pass B did not make this correction.** Section 3.6 still reads "**Thirteen
call sites** therefore fail loudly, in every build, on a wider constant" with
`constant_fold.cpp:115` in the list, and headline 2 still says "sixteen hard
aborts". Notes entry 36 shows why: B adjudicated only the *count* of the list,
adopted my thirteen, and never opened the site. This is the one place in either
revision where a round-one objection produced an accepted number instead of a
read. It is the residual correctness defect.

### 2.3 Pass B's headline 3 rewrite. **Coherent. B ruled against me correctly.**

I charged in round one that B "states both and reconciles neither". The rewrite
does reconcile them, and the reconciliation is sound because the two statements
are about different objects:

- `IndexExpression::type_check` (`taichi/ir/frontend_ir.cpp:936-947`) tests only
  `is_integral(expr_type)` at `:941`. Verified. An explicit i64 index survives
  the frontend.
- `FrontendForStmt::add_loop_var` (`taichi/ir/frontend_ir.cpp:149-153`) assigns
  `get_pointer_type(PrimitiveType::i32)` at `:151-152` unconditionally.
  Verified. A loop variable does not.
- `taichi/transforms/frontend_type_check.cpp:71-78` warns and does not cast.
  Verified.

There is no contradiction to withdraw — there is a scope split that was
unstated. B now states it and says which half dominates in practice: "In a
struct-for driven workload essentially every index is loop-derived, so in
practice the frontend does pin the width for the case that matters." That is the
same operational point I made in my own §1.1, so my objection was to the
presentation and B has fixed the presentation. Asserting both halves was the
right call; withdrawing either would have lost information.

### 2.4 `data_type_size` strips the pointer flag. **Both reports right; I was wrong.**

```
taichi/ir/type_utils.cpp:30  int data_type_size(DataType t) {
taichi/ir/type_utils.cpp:31-34  // TODO: … setting a loud failure on pointers …
taichi/ir/type_utils.cpp:35  t.set_is_pointer(false);
taichi/ir/type.cpp:59-61     if (!is_ptr && ptr_->is<PointerType>()) {
                               ptr_ = ptr_->cast<PointerType>()->get_pointee_type(); }
```

There is no `PointerType` branch in the function body, so a pointer argument
falls through to the primitive dispatch and returns the pointee's size. I
endorsed "refuses pointers" in round one under "Verified good, in both"; that
was wrong, and I conceded it in my own §9.2 item 3 after reading adversary 1.
Both revisions now state the mechanism correctly — A at §6 "Cannot, verified"
and B at headline 5 and §4 — and B additionally cites `taichi/ir/type.cpp:59-61`
as the reason, which is the citation that actually proves it. B's treatment is
the better of the two.

### 2.5 The three whole-structure flattening sites. **All three verified; the framing is right.**

Addressing is hierarchical: `strides` is declared inside the per-level loop at
`taichi/transforms/scalar_pointer_lowerer.cpp:56` (the loop opens at `:48`) and
each level's own `LinearizeStmt` is pushed at `:81-82`. Verified. A 2^40-cell
structure produces a chain of small indices, not one large one.

The three sites where an extent genuinely is flattened, and the three failure
modes:

| Site | Source, read | Fails how |
|---|---|---|
| `taichi/transforms/demote_dense_struct_fors.cpp:18,26,29,35` | `int64 total_n = 1;` (`:18`), `total_n *= snode->num_cells_per_container;` (`:26`), `TI_ASSERT(total_n <= std::numeric_limits<int>::max());` (`:29`), `offloaded->end_value = total_n;` (`:35`) into an `int32` field (`taichi/ir/statements.h:1416`) | **Abort.** Confirmed |
| `taichi/transforms/lower_ast.cpp:270-274` | `ConstStmt(TypedConstant(1))` at `:271` — i32 by the `TypedConstant(int32)` ctor at `taichi/ir/type.h:582` — then `BinaryOpStmt(mul, end, shape[i])` at `:273` per axis, each `shape[i]` an `ExternalTensorShapeAlongAxisStmt` pinned i32 at `taichi/transforms/type_check.cpp:119`. No assert, no warning anywhere in the block | **Silent overflow.** Confirmed |
| `taichi/ir/snode.cpp:89-101` | `int64 acc_shape = 1;` (`:89`), `extractors[i].acc_shape = static_cast<int>(acc_shape);` (`:92`), `if (acc_shape > std::numeric_limits<int>::max())` → `TaichiIndexWarning` (`:95-100`), then `num_cells_per_container = acc_shape;` untruncated (`:101`) | **Warn, then truncate.** The truncation and the field disagreement above 2^31 are confirmed. **But this row is withdrawn as a whole-structure flattening site — see section 7.2.** The accumulation is over one node's own axis shapes, so the quantity is per-container, not per-structure |

All three are right, and the contrast between them is the most useful single
paragraph in either report. Two qualifications, neither fatal:

1. `SNode::get_total_num_elements_towards_root` (`taichi/ir/snode.h:310-315`)
   also flattens a whole-structure extent, and does it with an `(int)`
   narrowing per factor at `:313` inside an `int64` accumulator. It has no
   callers tree-wide — both reports verified that independently, and I confirm
   it — so it fails no way today. A's flat "Three sites flatten a
   whole-structure or whole-array extent" would be exactly right with "reached"
   or "live" in it. B handles this better by escalating the uncalled narrowing
   separately (escalation 12).
2. A files `lower_ast.cpp:270-274` in §3.6, under a heading that reads "32-bit
   arithmetic that overflows **before any IR type is involved** … These are
   host-side compile-time `int` computations. They cannot be fixed by changing
   an IR type." That is false for this row: `:271` and `:273` construct IR
   statements, and the site *can* be fixed by changing an IR type. It is the
   only row in that table that is not host-side — I checked
   `check_out_of_bound.cpp:54-57` and `:169-173`,
   `handle_external_ptr_boundary.cpp:62-67`, `frontend_ir.cpp:742-747`,
   `simplify.cpp:176,187` and `scratch_pad.h`, and every one of those is a plain
   host `int`. A's own §4 treats the same site as an IR-level formation site, so
   the report contradicts itself across two sections.

---

## 3. Newly wrong, or still missing

### 3.1 Pass B: `constant_fold.cpp:115` and the two counts derived from it

Stated in 2.2. Concretely, B §3.6 "**Thirteen call sites** therefore fail
loudly" should read twelve, with `constant_fold.cpp:115` moved out and
described as guarded at `:113` with an i64 branch at `:116-118`; and headline 2's
"sixteen hard aborts" should read fifteen. This is the only place the two
revised reports contradict each other on a matter of fact, and A is right.

### 3.2 Both: the in-pass `type_check` re-run surface is badly understated

Both reports make "the coercion is not a one-shot" load-bearing — A gives it a
whole section, B puts it in headline 2. Both give the seven
`compile_to_offloads.cpp` calls correctly (`:57,66,193,200,208,310,391`, which I
reproduced). Both then name three or so further sites. A names
`simplify.cpp:219`, `handle_external_ptr_boundary.cpp:100`, and
"inside `check_out_of_bound.cpp`'s `run()`". B names `lower_access.cpp:294`,
`handle_external_ptr_boundary.cpp:100`, `simplify.cpp:219`.

The reason both are short is that most re-runs call `type_check(root, config)`
unqualified from inside `namespace irpass`, which a grep for `irpass::type_check`
misses. B found one of these (`lower_access.cpp:294` — verified, and A does not
have it). The full in-territory set:

```
taichi/transforms/auto_diff.cpp:2407,2410,2417,2420,2430
taichi/transforms/demote_atomics.cpp:241
taichi/transforms/demote_mesh_statements.cpp:160
taichi/transforms/demote_operations.cpp:300,303
taichi/transforms/handle_external_ptr_boundary.cpp:100
taichi/transforms/check_out_of_bound.cpp:248
taichi/transforms/lower_access.cpp:294
taichi/transforms/make_block_local.cpp:391
taichi/transforms/make_mesh_block_local.cpp:681
taichi/transforms/make_mesh_thread_local.cpp:165
taichi/transforms/make_thread_local.cpp:229
taichi/transforms/offload.cpp:766,773
taichi/ir/ir.cpp:566
taichi/transforms/simplify.cpp:219,344,349   (via DelayedIRModifier::type_check)
```

Every one of those files is inside territory 01. The point both reports are
making is strengthened, not weakened — but a planner sizing item 6.2 from
"seven, plus three" will undercount the re-application surface by more than
half. This is the one shared incompleteness that matters.

### 3.3 Pass A: `frontend_ir.cpp:731-740` is still absent

Adversary 1 raised this in round one (its §5.7). Pass B added it — §3.4 row
`taichi/ir/frontend_ir.cpp:732,735`. Pass A did not; A's §3.6 still has only the
constant-folded branch at `:742-747`. Verified:

```
731:  if (needs_dynamic_index) {
732:    offset_stmt = ctx->push_back<ConstStmt>(TypedConstant(0));
735:      Stmt *shape_stmt = ctx->push_back<ConstStmt>(TypedConstant(shape[i]));
736-739:  mul then add, per axis
```

Both `TypedConstant(0)` and `TypedConstant(shape[i])` are i32 by
`taichi/ir/type.h:582`. The whole flattened tensor offset is built in i32 on the
dynamic branch as well as the folded one, and the dynamic branch is the one that
produces IR rather than a compile-time constant.

### 3.4 Pass A: §3.3's stated count contradicts its own table

The heading is "Hardcoded `ret_type = PrimitiveType::i32` — **eighteen sites**".
The table has eighteen rows but twenty sites, because one row is
`frontend_ir.cpp:1358`, `:1362`, `:1392`. A counts by site elsewhere — its §3.1
lists each of the ten cast sites on its own row — so eighteen is wrong under A's
own convention. Note N024 lists exactly eighteen; the report then added two rows
and did not renumber.

Two of those rows are also not `ret_type` assignments and do not belong under
that heading:

- `taichi/transforms/lower_ast.cpp:330` is
  `fctx.push_back<AllocaStmt>(PrimitiveType::i32);` — an alloca element type.
- `taichi/ir/type_factory.cpp:204` is a struct *member* type in
  `get_ndarray_struct_type`, and A already lists it correctly in §3.7.

Both belong in the inventory. Neither is a pinned `ret_type`. This is the same
category of error — a headline number its own enumeration contradicts — that A
correctly charged adversary 1 with in §7.2, so it should be fixed rather than
left.

A separate omission from the same table: `taichi/transforms/type_check.cpp:170-171`
does set a range-for bound `ConstStmt`'s `ret_type` to i32, via `mark_as_if_const`
(`:40-44`). B lists it (§3.1); A discusses it only in §7.1. If A's table is a
count, that is a twenty-first site.

### 3.5 Pass A: the CPU pass gate is stated without its flag

A §3.4 says the pass is "run at `taichi/transforms/compile_to_offloads.cpp:199`
whenever the arch is CPU". The gate at `:198` is
`if (config.make_cpu_multithreading_loop && arch_is_cpu(config.arch))`.
`make_cpu_multithreading_loop` is a `CompileConfig` field
(`taichi/program/compile_config.h:44`) defaulting to true at
`taichi/program/compile_config.cpp:48` and settable from Python at
`taichi/python/export_lang.cpp:224-225`. B states the gate in full (§5.1). A's
escalation 4 asks the planner whether the CPU path is in scope; the planner
should know the pass is switchable. One clause.

### 3.6 Both: two off-by-a-little visitor spans in `simplify.cpp`

`visit(GetChStmt *)` spans `taichi/transforms/simplify.cpp:251-273` — `:270`
closes the `if`, `:272` is `set_done(stmt);`, `:273` closes the visitor. A says
`:251-270` (§7.3 item 5), B says `:251-274`. Both cite the right code and reach
the right conclusion; neither span is the visitor. `visit(SNodeLookupStmt *)` at
`:222-249` is right in both. Trivial, listed only because both reports assert
line-level verification.

### 3.7 Pass B: the per-axis qualification is applied to two rows and not to two others

B's §3.4 correctly marks `scalar_pointer_lowerer.cpp:37` and
`demote_dense_struct_fors.cpp:24` "**per axis**". The same table lists
`taichi/ir/snode.cpp:22-23` and `:83` unmarked, and those are per-axis too —
`new_ch->extractors[i].num_elements_from_root *= extractors[i].num_elements_from_root`
at `:22-23` and `new_node.extractors[ind].num_elements_from_root *= sizes[i]` at
`:83` are both indexed into `extractors[]`. Not an error, since the heading only
claims unguarded `int * int`. But a reader who takes the bolded qualification as
exhaustive will misread two rows. One word each.

---

## 4. Independent citation verification

Given that this project has produced systematic citation faults, I sampled hard
rather than lightly. Every line below was opened in this round.

**Verified exact, pass A:** `type_check.cpp:114`, `:119`, `:122-125`, `:147-156`,
`:154`, `:159-163`, `:169-173`, `:234-244`, `:457-463`, `:461`, `:466-476`,
`:520-522`, `:524-526`, `:565-567`; `statements.h:376`, `:379`, `:456`,
`:512-520`, `:521-529`, `:988`, `:993`, `:1260`, `:1280`, `:1370`, `:1411-1412`,
`:1415-1416`, `:1439`, `:1597`, `:1618`, `:1684`, `:1770`, `:1787`, `:2064`,
`:2083`; `type.h:151-155`, `:177-209`, `:206-208`, `:222-227`, `:243`, `:257`,
`:305`, `:307-321`, `:336`, `:530-532`, `:545`, `:551`, `:556-569`, `:582`,
`:661-677`; `type.cpp:59-61`, `:462-465`; `type_utils.cpp:30`, `:31-34`, `:35`,
`:44-48`, `:59`, `:64`, `:113-137`; `type_utils.h:107`, `:112`, `:123`;
`type_factory.cpp:86-95`, `:89`, `:168-169`, `:204`, `:234-237`, `:248-262`,
`:249-251`; `snode.h:28-30`, `:41`, `:45`, `:49`, `:78`, `:79`, `:81`, `:88-89`,
`:98`, `:310-315`, `:313`, `:317`; `snode.cpp:21`, `:89-101`, `:90`, `:92`,
`:95-100`, `:101`, `:105`, `:220`; `frontend_ir.cpp:151-152`, `:936-947`,
`:1114`, `:1306`, `:1358`, `:1362`, `:1392`; `frontend_ir.h:529`, `:615`, `:617`,
`:676`, `:712`; `statements.cpp:146`; `lower_ast.cpp:270-274`, `:330`;
`make_cpu_multithreaded_range_for.cpp:68`, `:70`, `:72`, `:81`, `:84`, `:90`,
`:93`; `demote_dense_struct_fors.cpp:18-19`, `:23-24`, `:26`, `:29`, `:35`, `:74`;
`scalar_pointer_lowerer.cpp:33`, `:36-37`, `:40`, `:56`, `:63-65`, `:78`, `:81-82`;
`simplify.cpp:176`, `:187`, `:189-205`, `:222-249`, `:233-235`, `:238-239`,
`:260-261`; `check_out_of_bound.cpp:54-57`, `:91`, `:123-126`, `:153`, `:169-173`,
`:201`; `handle_external_ptr_boundary.cpp:62-67`; `make_block_local.cpp:157`,
`:292-296`, `:334`; `make_mesh_block_local.cpp:80`, `:118`, `:200`, `:212`,
`:265`, `:301`, `:376`, `:399`, `:575`; `lower_matrix_ptr.cpp:536-540`;
`arithmetic_interpretor.cpp:98-109`; `value_diff.cpp:71-76`, `:81-87`, `:142-146`;
`bit_loop_vectorize.cpp:180`, `:202`, `:311-325`, `:321`;
`alias_analysis.cpp:59`, `:142`, `:178`; `lower_access.cpp:237`;
`bls_analyzer.cpp:50`; `ir_builder.h:134`, `:136`; `scratch_pad.h:33-42`,
`:47-55`, `:96-100`, `:131-138`, `:140-147`, `:149-157`;
`offline_cache_util.cpp:110`; `rhi_constants.inc.h:14`;
`device_capability.h:11-15`; `spirv_ir_builder.cpp:64-66`, `:166-169`, `:311-314`,
`:325-328`; `vulkan_device_creator.cpp:632`; `opengl_device.cpp:511`;
`metal_device.mm:132`, `:1052`; `codegen_llvm.cpp:1052-1060`, `:1736-1744`;
`spirv_codegen.cpp:532-541`; `runtime.cpp:288-290`; `constants.h:5`, `:12`, `:13`;
`ndarray.cpp:57`, `:100` (both are the `TaichiIndexWarning()` line of the
emitter, the same convention A uses for `snode.cpp:97` — consistent, not an
error).

**Verified exact, pass B, beyond the shared set:** `type_utils.h:13`;
`type.h:229-235`, `:250`, `:557`, `:560`, `:568`, `:588`, `:612`;
`snode.h:80`, `:97`, `:99-100`, `:306-308`, `:329-334`; `type_factory.h:43`;
`lower_access.cpp:162-169`, `:294`; `alg_simp.cpp:118`, `:132`;
`binary_op_simplify.cpp:66`; `bit_loop_vectorize.cpp:74`;
`bls_analyzer.cpp:27`; `make_mesh_block_local.cpp:434`, `:440`, `:444`, `:526`;
`offload.cpp:236-237`, `:244`, `:358`; `frontend_ir.cpp:132`, `:732`, `:735`,
`:1109-1110`; `frontend_type_check.cpp:71-78`; `scratch_pad.h:75-76`, `:106`,
`:107-108`, `:182`, `:214`, `:229-234`; `struct_llvm.cpp:171`;
`launch_context_builder.cpp:230`, `:247`, `:269`, `:302`, `:322`;
`export_lang.cpp:559`, `:1222`; `runtime.cpp:1012`;
`codegen_llvm.cpp:2363` (B's `:2356-2376` and `:2365-2372` both bracket the
`PhysicalCoordinates` layout assertion correctly).

**Independently re-derived, not taken from either report:** the twelve
in-territory `taichi_max_num_indices` use lines plus the declaration; the tree-wide
`taichi_max_num_snodes` sites (`constants.h:12`, `struct_llvm.cpp:266`,
`runtime.cpp:567,568,569`); the `kMaxNumSnodeTreesLlvm` sites
(`constants.h:13`, `runtime.cpp:562,563`); the fourteen `val_int32()` call sites;
the ten `cast_type = PrimitiveType::i32` and `insert_type_cast_before` sites; the
seven `irpass::type_check` calls; the complete unqualified `type_check(` set in
section 3.2.

**Citation faults found in this round: none in pass B, and in pass A only the
two visitor-span approximations in section 3.6 above.** The systematic
attribution problem of round one is gone. A's §7.1 item 6 explains why it
happened — the first pass checked that a line said what was quoted without
checking it was the intended line — and the fix held.

---

## 5. Escalations

Unresolved. Recorded, not decided. All of these are already in one or both
reports and I am adding nothing to the list; I record them because both reports
should carry the same set forward.

1. **Mesh scope.** Eight of the ten cast sites and four pinned `ret_type`s are
   mesh. Both reports escalate it (A item 1, B item 4). Unresolved.
2. **The CPU multithreaded range-for.** Both escalate (A item 4, B item 5). The
   planner should be told it is gated on `config.make_cpu_multithreading_loop`
   (`taichi/transforms/compile_to_offloads.cpp:198`), default true.
3. **What "the address" means for item 6.2** — a wider per-level index, a wider
   flattened index, or a wider byte address. Both escalate (A item 6, B item 7).
   This is the question that separates the map plan 8.1 item 1 asks for from the
   list both reports deliver, and it is not answerable inside territory 01.
4. **`PhysicalCoordinates`** (`taichi/runtime/llvm/runtime_module/runtime.cpp:288-290`)
   fixing the frontend's `LoopIndexStmt` width. Spans 01, 02 and 03. Both
   escalate.
5. **The link from the type layer to `spirv_has_int64`.** Declared at
   `taichi/inc/rhi_constants.inc.h:14`, enforced at
   `taichi/codegen/spirv/spirv_ir_builder.cpp:312`, `:326`, consulted nowhere in
   `taichi/ir/`, `taichi/analysis/` or `taichi/transforms/`. Building the link is
   added structure, which standing instruction 4 forbids all of us from
   proposing. Both reports say so and stop, correctly.
6. **`MatrixPtrStmt::offset`'s two semantics**, and the i32 byte offset at
   `taichi/transforms/lower_matrix_ptr.cpp:536-540`. Both escalate.
7. **Whether the C++ tests are a constraint or a casualty.** Both escalate.
8. **`simplify.cpp:261`.** Neither agent, neither adversary, and not I, can
   settle whether the `SNodeLookupStmt` assertion transitively covers the
   `GetChStmt` fold. Both reports now mark it inferred and escalate it. That is
   the right disposition and it should not be pushed further by an agent.

---

## 6. What remains, scoped for one pass

Three items. Each is a specific edit against a line I have given.

**Pass B, one correctness fix.**
Section 3.6: move `taichi/transforms/constant_fold.cpp:115` out of the
fail-loudly list and describe it as guarded by
`if (dt->is_primitive(PrimitiveTypeID::i32))` at `:113` with an i64 branch at
`:116-118`, where `dt` is `lhs->val.dt` (`:65`) — the same field `val_int32()`
asserts against. Change "Thirteen call sites" to twelve, and headline 2's
"sixteen hard aborts" to fifteen. Nothing else in B needs to move.

**Both reports, two completeness fixes.**
First, the flattening-site correction and `num_elements_from_root`, set out in section 7.4.
Second, extend the in-pass `type_check` re-run list with the unqualified
`type_check(root, config)` calls given in section 3.2 above, or state the count
as "at least fifteen further in-pass re-runs across fourteen files in this
territory" and give the grep. Both reports rest a structural conclusion on this
number.

**Pass A, three small fixes.**
1. Add `taichi/ir/frontend_ir.cpp:731-740` — the dynamic tensor-offset branch,
   i32 at `:732` and `:735` — alongside the folded branch at `:742-747`.
2. Section 3.3: reconcile the "eighteen sites" heading with the twenty sites the
   table lists, and move `lower_ast.cpp:330` and `type_factory.cpp:204` out of a
   heading that says `ret_type` (they are an alloca element type and a struct
   member type respectively). Optionally add `type_check.cpp:170-171`.
3. Section 3.6: either move `lower_ast.cpp:270-274` out of a table headed
   "before any IR type is involved … cannot be fixed by changing an IR type", or
   qualify the heading. Section 4 of the same report already treats it as an
   IR-level site.

Optional, not blocking: the `visit(GetChStmt *)` span is `simplify.cpp:251-273`
in both reports; A's §3.4 should carry the `config.make_cpu_multithreading_loop`
half of the gate; B's §3.4 should mark `snode.cpp:22-23,83` per-axis as it marks
the other two; A's §4 "three sites" would be exact with "live" or "reached" in
it, given `snode.h:310-315`.

**On the central structural finding I again could not break it.** The type
system can represent i64 and models no index type and no address type;
`PointerType` (`taichi/ir/type.h:177-209`) carries a pointee, an `addr_space_`
int tagged TODO at `:207`, and a bit-pointer flag, and no width; pointer types
are interned on `(element, is_bit_pointer)` at `taichi/ir/type_factory.cpp:89`;
and promotion is unavailable to anything carrying a pointer
(`taichi/ir/type_factory.cpp:249-251`). Both reports state this correctly and
both now state the `spirv_has_int64` capability correctly against it. That is
the answer plan 8.1 item 1 was asking for, and it is sound.

---

## 7. Divergence from adversary2-01-1

`adversary2-01-1.md` landed after I finished section 6. I have read it and
re-checked every point on which we differ, against source, before conceding or
holding.

### 7.1 Where we agree, independently

We reached the same conclusions by separate routes on every one of the five
adjudications the lead named, and on the two residual defects that matter:

- `taichi/transforms/constant_fold.cpp:115` is guarded and is not an abort site.
  Pass A is right, both of us were wrong in round one, and **pass B still has it
  wrong** in section 3.6 and headline 2. Same chain, same numbers: twelve
  `val_int32()` aborts, fifteen hard aborts.
- Ten cast-insertion sites, eight of them mesh. Adversary2 01-1 concedes that
  both slips in round one were in its own file.
- Pass B's headline 3 rewrite is coherent, and rewriting rather than withdrawing
  was the right call.
- `data_type_size` strips the pointer flag and returns the pointee's size.
- Pass A section 3.3 counts eighteen over a table of twenty, and two of its rows
  are not `ret_type` assignments.
- Pass A section 3.6 misfiles `taichi/transforms/lower_ast.cpp:270-274` under a
  heading that says host-side compile-time `int`.
- Pass A still omits `taichi/ir/frontend_ir.cpp:731-740`.
- `visit(GetChStmt *)` spans `taichi/transforms/simplify.cpp:251-273`; A says
  `:251-270`, B says `:251-274`, both wrong in opposite directions.

Two independent adversaries converging on the same six-item defect list, having
worked blind, is a stronger signal than either list alone.

### 7.2 Where adversary2 01-1 is right and I was wrong

**My section 2.5 site 3 is wrong, and I am withdrawing it.** I wrote
"Confirmed" against `taichi/ir/snode.cpp:89-101` as a whole-structure flattening
site. I verified the mechanics of the truncation correctly and did not test
whether the accumulation crosses SNode levels. It does not. Verified now:

- `SNode::create_node` obtains `new_node` from `insert_children(type)`
  (`taichi/ir/snode.cpp:58`).
- `insert_children` (`:14-35`) propagates **only** `num_elements_from_root`, at
  `:21-23`. It does not touch `shape` or `acc_shape`.
- `AxisExtractor::shape` and `::acc_shape` therefore start at their declared
  defaults of 1 (`taichi/ir/snode.h:45`, `:49`), and `:84` assigns
  `new_node.extractors[ind].shape = sizes[i]` — this node's own axis size, a
  plain assignment, not an accumulation.
- So `acc_shape` at `:89-93` is the product of one node's own axis shapes, and
  `:101` stores it into `num_cells_per_container`, which `taichi/ir/snode.h:93-97`
  documents as "Product of the |shape| of all the activated axes identified by
  |extractors|" with an explicit pointer to the cell/container terminology.

That is a per-container extent. It is exactly the quantity both reports'
hierarchical-addressing finding says is bounded by the level rather than by the
structure, so calling it a whole-structure flattening contradicts their own
section 1. Adversary2 01-1 caught this and I did not.

**The site that belongs in that slot is `AxisExtractor::num_elements_from_root`,
and it is in neither report and was not in my analysis.** Verified:

- `taichi/ir/snode.h:41` — `int num_elements_from_root{1};`
- `taichi/ir/snode.cpp:21-23` — each child's per-axis value is multiplied by its
  parent's, so it accumulates root-to-leaf.
- `taichi/ir/snode.cpp:83` — `new_node.extractors[ind].num_elements_from_root *= sizes[i];`
- No assert, no warning, no `int64` shadow on that path.
- `SNode::shape_along_axis` (`taichi/ir/snode.cpp:174-177`) **returns this
  field**, at `:176`.
- `taichi/transforms/check_out_of_bound.cpp:123-126` materialises that return
  value as the i32 `ConstStmt` compared against every SNode field index.

Both reports cite `snode.h:317` and both cite `check_out_of_bound.cpp:123-126`.
Neither joins them, and neither says the bound is an unguarded root-to-leaf `int`
product. My round-one section 5.2 raised the bound and stopped at
`shape_along_axis` being declared `int`; I did not open its definition. This is
the better version of my own finding and it is adversary2 01-1's.

The seam fact it adds is also verified:
`taichi/codegen/spirv/snode_struct_compiler.cpp:115-121` builds
`sn_desc.total_num_cells_from_root *= e.num_elements_from_root;` — the genuine
whole-structure cell count, assembled from the territory-01 `int` field.

**Three smaller items it has and I do not**, each verified:

- The `auto_diff.cpp` i32 index constructions at `:587`, `:639`, `:646`, `:787`,
  `:839`, `:846`, `:1828`, `:1857`. Bounded by matrix element count, so low
  consequence, but both reports inventory that file's aborts and not its index
  constructions.
- `taichi/transforms/make_mesh_block_local.cpp:526`
  (`mapping_data_type_ = PrimitiveType::i32;`) and
  `taichi/transforms/lower_ast.cpp:168`, `:348`, `:361`, all present in pass B
  and absent from pass A.
- The nuance at `constant_fold.cpp:115` that the guard tests only `lhs->val.dt`
  while the line also calls `rhs->val.val_int32()`, safe only because operand
  types are unified upstream, which the in-tree comment asserts rather than
  proves. I saw this while checking and did not write it down.

### 7.3 Where I am right and adversary2 01-1 is silent

**The in-pass `type_check` re-run surface** (my section 3.2). Adversary2 01-1's
section 5 confirms the seven `irpass::type_check` calls in
`compile_to_offloads.cpp` and goes no further. Both reports rest a load-bearing
structural claim on that number, and the true in-territory set is at least
fifteen further re-runs across fourteen files, most of them invisible to a grep
for `irpass::type_check` because they call `type_check(root, config)` unqualified
from inside `namespace irpass`. Pass B found exactly one of them
(`taichi/transforms/lower_access.cpp:294`); pass A found none. The list is in my
section 3.2. This remains a shared incompleteness that neither adversary but me
has recorded, and it strengthens rather than weakens both reports' argument.

**The `make_cpu_multithreading_loop` gate** (my section 3.5). Adversary2 01-1
gives the gate line correctly in passing but does not flag that pass A's section
3.4 says the pass runs "whenever the arch is CPU", dropping the config flag. The
flag is settable from Python (`taichi/python/export_lang.cpp:224-225`) and pass
A's escalation 4 asks the planner to rule on the CPU path; the planner should
know it is switchable.

**Pass B's unmarked per-axis rows** (my section 3.7). B marks
`scalar_pointer_lowerer.cpp:37` and `demote_dense_struct_fors.cpp:24` "per axis"
and leaves `taichi/ir/snode.cpp:22-23` and `:83` unmarked in the same table.
Adversary2 01-1's finding makes this sharper than I put it: those two rows are
not merely per-axis, they are per-axis **and root-to-leaf**, which is what makes
them the load-bearing ones in that table.

### 7.4 Net effect on my verdicts

Direction unchanged, and the completeness verdict is worse than I stated. My
sections 0 and 6 said the shared completeness gap was one item. It is two: the
`type_check` re-run surface, and `num_elements_from_root` as the unguarded
root-to-leaf extent feeding the bound both reports already cite.

**Amendments to my section 6, incorporating adversary2 01-1:**

- **Both reports:** strike `taichi/ir/snode.cpp:89-101` as a *whole-structure*
  flattening site — it is `num_cells_per_container`, per container, per
  `taichi/ir/snode.h:93-97` — while keeping the `:92` truncation and its warning
  in the inventory. Put `AxisExtractor::num_elements_from_root` in its place:
  `taichi/ir/snode.h:41`, formed at `taichi/ir/snode.cpp:21-23` and `:83`,
  returned by `shape_along_axis` at `taichi/ir/snode.cpp:174-177`, consumed at
  `taichi/transforms/check_out_of_bound.cpp:123-126`, and multiplied into a
  whole-structure count at the seam
  (`taichi/codegen/spirv/snode_struct_compiler.cpp:115-121`).
- **Pass A:** additionally add `taichi/transforms/make_mesh_block_local.cpp:526`
  and `taichi/transforms/lower_ast.cpp:168`, `:348`, `:361`.

Everything else in my section 6 stands as written.

Neither of us contradicts the other on any verdict. Both of us find pass A
correct with presentational defects, pass B incorrect on one count, both
incomplete, and consensus one tightly scoped pass away. The union of my section
3 and adversary2 01-1's sections 2 and 3, not either alone, is what should go
back with the reports.
