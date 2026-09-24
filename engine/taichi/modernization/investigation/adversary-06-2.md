# Adversary 06-2 — territory 06, the enumeration of 32-bit spellings in the frontend IR

Adversary 2 of 2. Inputs: `report-06-i32-spellings.md` (pass A),
`report-06b-i32-spellings.md` (pass B), their notes, `report-01-ir-types.md`,
`report-01b-ir-types.md` and the source tree at
`ba0e81dce559fb63a5958bf82feb1d00c55c02fe`.

Worked against `PROJECT-PLAN.md` as of **2026-09-09**, re-read at the end of this
pass. No source file was modified. No file except this one was written.

Throughout:

```
cd /opt/project/taichi
T="taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/"
```

Report B changed on disk during this pass; it gained section 2's closing
paragraph on `Expr` and the continuation-line table now at lines 150-170. Every
judgement below is against the version on disk at the end, and I say where the
amendment altered a verdict.

---

## 0. Verdict in one page

**Neither report's spelling count is right, and the two failures are
symmetrical.** Pass A names eight and pass B names nine (S1-S9). The union of
routes that a text search must be separately keyed on is **thirteen**. Each pass
holds ten of the thirteen. Each misses three the other has. Neither is a
superset of the other, so the planner cannot take either report as the class.

**The brace form is real and pass B's bound is not a bound.** `TypedConstant{…}`
exists at exactly seven lines, listed in section 1. Pass B's sentence at report
06b line 91 — "the token `TypedConstant(` appears at every construction, so
`grep -rn "TypedConstant("` bounds the class exactly" — is false as written.

**But the damage is smaller than pass A's numbers imply, and pass A's numbers
are two too high for a different reason.** Both reports converge on **44** once
the brace form is admitted and report 01's cited line ranges are applied
consistently. That is a real convergence, derived below, not an average.

| Quantity | Pass A | Pass B | Established here |
|---|---|---|---|
| Textual routes (spellings) | 8 | 9 | **13**, union; 10 each |
| One-argument `TypedConstant` constructions | 80 | 73 | **80** (73 paren + 7 brace) |
| Of those, fixing i32 | 73 | 56 | **73** |
| Of those 73, absent from report 01 entirely | 46 | 40 | **44** |

Pass B's 56 is not a rival figure to pass A's 73. It is 73 minus the 13 sites
carrying an explicit `(int32)` cast, which pass B files under its S4 because
report 01's cast sweep already sees them. 56 + 10 paren-cast + 7 brace = 73.
The two passes never disagreed about a line; they disagreed about which bucket
a line belongs in, and about whether the brace form exists.

---

## 1. The principal task — the brace form, and the true membership

### 1.1 `TypedConstant{…}` is real

```
grep -rn "TypedConstant{" $T
```
→ **7 lines**, reproduced in full:

| Line | Text as it stands in the tree |
|---|---|
| `taichi/transforms/demote_mesh_statements.cpp:86` | `TypedConstant{from_type == …Cell && …Edge ? 6 : (from_order + 1)}` |
| `taichi/transforms/demote_mesh_statements.cpp:94` | same expression, `to_size` branch |
| `taichi/transforms/demote_mesh_statements.cpp:123` | `push_back<ConstStmt>(TypedConstant{1})` |
| `taichi/transforms/make_mesh_block_local.cpp:80` | `TypedConstant{(int32)mapping_bls_offset_in_bytes_}` |
| `taichi/transforms/make_mesh_block_local.cpp:169` | `push_back<ConstStmt>(TypedConstant{offload_->block_dim})` |
| `taichi/transforms/make_mesh_block_local.cpp:200` | `TypedConstant{(int32)mapping_bls_offset_in_bytes_}` |
| `taichi/transforms/make_mesh_block_local.cpp:575` | `TypedConstant{(int32)mapping_bls_offset_in_bytes_}` |

Pass A found these by a follow-character census of the identifier, which I
reproduced exactly:

```
grep -rno "TypedConstant." $T | sed 's/.*TypedConstant//' | sort | uniq -c | sort -rn
    139 (
     22 (space)
     19 :
     12 >
      7 {
```

The census is the right instrument and it is pass A's single best methodological
contribution in this territory. It is the only step in either report that could
have found the brace form, and pass B did not run it.

**The widths, checked at the declarations, not inferred from the names.**
`taichi/ir/mesh.h:48` declares `int element_order(MeshElementType type);`, so
`from_order` at `demote_mesh_statements.cpp:78` is `int` and the conditional
`… ? 6 : (from_order + 1)` at `:86` and `:94` has type `int`. `:123` is the
literal `1`. `taichi/ir/statements.h:1418` declares `int block_dim{1}`, so
`offload_->block_dim` at `make_mesh_block_local.cpp:169` is `int`. All four
select `explicit TypedConstant(int32 x)` at `taichi/ir/type.h:582`. Pass A's
rulings on all four are correct.

### 1.2 What pass B's bound actually cost it

Pass B was not blind to all seven. Its S4 command at report 06b line 33 includes
the alternative `TypedConstant{(int32)`, which matches
`make_mesh_block_local.cpp:80`, `:200` and `:575`. Those three are held.

The loss is the other four: `demote_mesh_statements.cpp:86`, `:94`, `:123` and
`make_mesh_block_local.cpp:169`. Pass B's forty-row status-C table at report 06b
lines 210-250 contains no `demote_mesh_statements.cpp` row at all, and its
`make_mesh_block_local.cpp` rows are `:83`, `:116`, `:166`, `:204`, `:268`,
`:304`, `:327`, `:578` — eight, without `:169`.

Confirmed mechanically. Extracting the spelling-D rows of pass A's section 4
table and pass B's status-C table and diffing the key columns:

```
comm -23 A_D.txt B_C.txt
  taichi/transforms/demote_mesh_statements.cpp:86
  taichi/transforms/demote_mesh_statements.cpp:94
  taichi/transforms/demote_mesh_statements.cpp:123
  taichi/transforms/make_mesh_block_local.cpp:169
  taichi/transforms/simplify.cpp:175
  taichi/transforms/simplify.cpp:178
comm -13 A_D.txt B_C.txt
  (empty)
```

Pass B's forty is a strict subset of pass A's forty-six. Four of the six extra
are the brace loss. The other two are a different dispute, settled in 1.4.

**None of the four is covered by report 01 under any citation form.** I tested
every `file:NNN-MMM` range in report 01 — 94 of them, extracted by

```
grep -oE 'taichi/[A-Za-z0-9_/]+\.(cpp|h):[0-9]+-[0-9]+' report-01-ir-types.md | sort -u
```

— against all 46 of pass A's new sites. Two fall inside a range. Neither of the
two is one of the four. `report-01-ir-types.md` cites
`demote_mesh_statements.cpp` only at `:16`, `:54`, `:131`, `:160` and
`make_mesh_block_local.cpp` only at `:80`, `:118`, `:200`, `:212`, `:265`,
`:376`, `:399`, `:526`, `:681`. So the four are genuinely new work, and they
were reachable only through the form pass B ruled out by construction.

### 1.3 The true membership of the constructor class

Both passes independently produce the same arity split, and it holds:

| | Lines | Source |
|---|---|---|
| `TypedConstant(` in territory | 139 | both, reproduced here |
| `TypedConstant{` in territory | 7 | pass A, reproduced here |
| Declarations in `taichi/ir/type.h` | 14 | `:572`, `:575`, `:582`, `:585`, `:588`, `:591`, `:594`, `:597`, `:600`, `:603`, `:606`, `:609`, `:612`, `:616` |
| Two-argument paren call sites | 52 | pass A's 53 less the `:616` declaration; pass B's `TYPED2` |
| **One-argument call sites** | **80** | 139 − 14 − 52 = 73 paren, plus 7 brace |

Of the 80:

| Ruling | Count | Which |
|---|---|---|
| `bool` argument → `PrimitiveType::u1` | 3 | `check_out_of_bound.cpp:48`, `:107`, `:180` |
| `float32` argument → `PrimitiveType::f32` | 2 | `auto_diff.cpp:957`, `:963` |
| `DataType` argument → `type.h:575` overload | 2 | `alg_simp.cpp:71`, `:92` |
| **Fixes `PrimitiveType::i32`** | **73** | the remainder |

3 + 2 + 2 + 73 = 80. Both passes agree row for row on the seven exclusions.
Pass B's reasoning on the `bool` three is the better of the two and is correct:
`taichi/common/core.h:136` makes `uint1` an alias for `bool`, so
`TypedConstant(true)` is an **exact** match on the `uint1` overload at
`taichi/ir/type.h:597` and beats the `bool`→`int` integral promotion. Pass A
reaches the same disposition and cites the same alias.

Pass A's table of the 80 is sound as a table: 80 rows, no duplicate `file:line`,
every key inside the 146-line union of the two commands, and the ruling column
sums 7 excluded + 5 + 22 + 46 = 80. Standing instruction 9 satisfied.

**One bookkeeping slip in pass A, no effect.** Its arity split at report 06 line
218 calls `type.h:572-612` "13 … constructor declarations" inside a bucket it
labels one-argument. `taichi/ir/type.h:572` is `TypedConstant()`, zero
arguments. It is a declaration either way, so no call site moves.

### 1.4 The two `simplify.cpp` rows — pass B is right

Pass A files `taichi/transforms/simplify.cpp:175` and `:178` as tier C, absent
from report 01. Pass B files both as status B, inside a cited range.

`report-01-ir-types.md:910` reads:

```
in this territory. `taichi/transforms/simplify.cpp:160-220` expands it into
adds and muls with an `int` stride accumulator;
```

`:175` and `:178` lie inside `:160-220`. Pass A's own tier definition at report
06 line 340 is "falls inside a line range report 01 cites, but is not named as a
site", and pass A applied that rule to `simplify.cpp:193` in the same function
while missing it for `:175` and `:178`. That is inconsistent application of its
own rule, not a difference of opinion.

Mechanically: of pass A's 46, exactly two fall inside any of report 01's 94
cited ranges, and they are these two. None of the 46 is exactly named.

**So the corrected tiering of the 73 is 22 named, 7 in range, 44 absent.**
22 + 7 + 44 = 73. Pass B's 40 plus the four brace sites is 44. The two passes
converge once both errors are corrected.

### 1.5 The true spelling count

The count is definition-dependent and both reports leave the definition
implicit, which is why they disagree about the size of the class. The operative
definition — the one that decides whether a sweep can see a site — is: **a
spelling is a distinct token sequence a text search must be separately keyed on
to return the line.** Under that definition, `TypedConstant(` and
`TypedConstant{` are two, and that is precisely the fact pass B's bound denies.

Thirteen routes exist in this territory. Coverage:

| # | Route | Site-bearing? | Pass A | Pass B |
|---|---|---|---|---|
| 1 | `PrimitiveType::i32` — 64 lines | yes, 60 | A | S1 |
| 2 | `PrimitiveTypeID::i32` — 27 lines | yes, 5 | B | S2 |
| 3 | `get_data_type<int32>()` — 4 lines | yes, 4 | C | S3 |
| 4 | `TypedConstant(<host int>)` — 73 one-arg calls | yes | D | S5 + `CASTED` |
| 5 | `TypedConstant{<host int>}` — 7 lines | yes, 7 | D | **missed 4 of 7** |
| 6 | `Expr(<host int>)` — 3 sites on 2 lines | yes, 3 | **missed** | S9 |
| 7 | bare `DataType i32` from the `PRIM` macro, `taichi/ir/type_system.cpp:161-165` — 35 use lines | yes | E | **missed** |
| 8 | literal bit count into `get_primitive_int_type(32, …)` | yes, 1 | F | S7 |
| 9 | `sizeof(int32)` as a hardcoded byte width — 3 code lines | yes, 1 new | **missed** | S7 |
| 10 | `int num_bits_{32}` default plus `type.cpp:238` | yes | F | S2 + S8 |
| 11 | `PrimitiveType::u32` — 4 lines | yes, 1 | H | **missed** |
| 12 | plain C++ `int` / `auto` host carrier on an IR class | yes | G | S8 |
| 13 | `IRBuilder::get_int32` | no new sites | **missed** | S6 |

Pass A holds 1-5, 7, 8, 10-12 = ten. Pass B holds 1-4, 6, 8-10, 12, 13 = ten.
Union thirteen; intersection seven.

Route 13 is site-empty and pass B says so, which is the right way to record a
sweep that came back nothing. Its omission from pass A costs no site: the body
of `IRBuilder::get_int32` at `taichi/ir/ir_builder.cpp:143` is already pass A's
spelling-B row, and its one call at `ir_builder.cpp:125` is in report 01.

---

## 2. Attack item 1 — `taichi/ir/type_system.cpp`

**The mechanism is exactly as pass A describes it.** Read at source:

```
taichi/ir/type_system.cpp:157: namespace {
:161: #define PRIM(dt) \
:162:   DataType dt =  \
:163:       TypeFactory::get_instance().get_primitive_type(PrimitiveTypeID::dt);
:165: PRIM(i32)
:171: DataType i32_void = i32;
:174: #undef PRIM
```

Inside that anonymous namespace the bare token `i32` is a `DataType` object
holding `PrimitiveType::i32`. The file contains no `PrimitiveType::i32`, no
`TypedConstant`, no `(int32)` and no `get_data_type<int32>`, so all four of
report 01's sweeps pass over it. `grep -n "\bi32\b\|i32_void"
taichi/ir/type_system.cpp` returns **37** lines, two of them the definitions at
`:165` and `:171`, leaving 35 uses. Pass A's counts reproduce exactly.

The table is a signature table. `PLAIN_OP` is defined at
`taichi/ir/type_system.cpp:286-288` as
`PLAIN_OP(name, ret, ctx, ...)` building an `InternalCallOperation` whose second
constructor argument is the return `DataType`. So `PLAIN_OP(linear_thread_idx,
i32, true)` at `:307` does declare an i32 return.

**This is a genuine finding and it is absent from report 01 and its notes.**
`grep -c "type_system" report-01-ir-types.md notes-01-ir-types.md` returns 0 and
0, as pass A states.

**But pass A's sentence in escalation 7 and section 4.1 item 3 — "no revision of
report 01 has it" — is too wide, and the SPIR-V half of the claim does not
survive.**

**Correction (a), a precision matter.** Report 01**b**, the other half of
territory 01's pair, cites the file three times:
`report-01b-ir-types.md:702` and `:1053-1054` name
`taichi/ir/type_system.h:246`, `:280` and `taichi/ir/type_system.cpp:140`. None
of the three is the `PRIM` macro or the signature table, so the substance of
pass A's finding stands against 01b too. The claim should read "absent from
report 01 and its notes; report 01b cites the file only at three type-check
entry points, not the table."

**Correction (b), and this one is load-bearing.** Pass A's section 4.1 item 3
says the two thread indices are declared i32 "on both spines, `linear_thread_idx`
on the LLVM side and `vkGlobalThreadIdx` on the SPIR-V side", and calls it "a
launch-index-space cap sitting in the type system, on both spines." The LLVM
half holds. The SPIR-V half does not.

- **LLVM half, live.** `taichi/runtime/llvm/runtime_module/runtime.cpp:1650`
  defines `i32 linear_thread_idx(RuntimeContext *context)`, called at `:1702`
  and `:1792`. The frontend builds it at `taichi/ir/frontend_ir.cpp:1493`. The
  declared i32 and the implemented i32 agree, and the route is reachable.

- **SPIR-V half, a dead declaration.** `vkGlobalThreadIdx` appears at exactly
  three places tree-wide: the comment at `taichi/ir/type_system.cpp:358`, the
  declaration at `:370`, and the op-name list at
  `taichi/inc/internal_ops.inc.h:25`. **`taichi/codegen/spirv/spirv_codegen.cpp`
  has no arm for it.** Its `visit(InternalFuncStmt *)` chain at `:1375-1450`
  dispatches on `stmt->func_name` and handles `localInvocationId` at `:1413` and
  `globalInvocationId` at `:1415`. `globalInvocationId` occurs exactly once in
  the whole tree, at that line, and is not an entry in
  `taichi/inc/internal_ops.inc.h` at all.

  So the name the SPIR-V codegen dispatches on is not in the signature table,
  and the name in the signature table has no codegen arm.

The phenomenon pass A points at is real — the SPIR-V global thread index **is**
capped at 32 bits — but the cap is at
`taichi/codegen/spirv/spirv_codegen.cpp:1416`,
`val = ir_->cast(ir_->i32_type(), ir_->get_global_invocation_id(0));`, which is
territory 02, not at `type_system.cpp:370`. Pass A's own section 5.2 item 3
marks the reach-to-codegen claim as inferred, which is honest, but the report's
prose in 4.1 and escalation 1 states it as established on both spines. It is
established on one.

**A further classification worth a second opinion.** Pass A files the 16
`i32_void` lines as "a placeholder, not a width". `taichi/ir/type_system.cpp:171`
is `DataType i32_void = i32;` — literally the same object. Every internal
declared with an `i32_void` return declares an i32 return. The disposition is
defensible for item 6.2, because none of the 16 carries an index, but it is a
judgement about intent and the report presents it as a class boundary. It should
say the width is real and the quantity is not an index, rather than that the
width is a placeholder.

**Verdict on item 1.** The finding is real, is the largest single discovery in
territory 06, and pass B missed the file entirely
(`grep -c "type_system" report-06b-i32-spellings.md notes-06b-i32-spellings.md`
returns 0 and 0). Two of pass A's seven index-bearing rows need requalifying:
`:370` `vkGlobalThreadIdx` is unreachable from the SPIR-V codegen as the tree
stands, and `:369` `localInvocationId` is the reachable Vulkan one.

---

## 3. Attack item 2 — is `taichi/ir/type.h:582` `explicit`?

**Both reports are right and report 01 is wrong as C++.** Read at source:

```
taichi/ir/type.h:582:  explicit TypedConstant(int32 x) : dt(PrimitiveType::i32), val_i32(x) {
```

Every single-argument constructor in the set carries `explicit`: `:575`, `:582`,
`:585`, `:588`, `:591`, `:594`, `:597`, `:600`, `:603`, `:606`, `:609`, `:612`.
`:572` is the zero-argument default and `:616` is the two-argument template;
neither is marked. Pass B's enumeration of the twelve is exact.

Report 01 says "implicit" at four places: `report-01-ir-types.md:77`, `:735`,
`:1592` and `:1618`, the last two inside escalation 16 which is the escalation
this whole territory answers.

**The class of miss report 01 describes is real; its stated mechanism is not.**
Nothing converts to a `TypedConstant` silently. What happens is overload
resolution: `TypedConstant(i)` for a host `int i` is an exact match on
`int32` — `taichi/common/core.h:146` makes `int32` an alias for `int32_t` — and
every other integer overload needs a conversion. So the line names no width and
still pins one. Both passes state this correctly, pass B at report 06b lines
89-97, pass A at report 06 lines 42-48.

The correction matters beyond pedantry in one direction only, and pass B names
it: because construction cannot be silent, the type's own name must appear at
every site, which is what makes the class enumerable at all. Report 01's wording
would send a reader hunting for conversions through function parameters, which
cannot occur.

**Neither pass drew the second consequence, and it is the one that generalises.**
`explicit` is what makes an enumeration possible, so the right question is which
other types in this territory have a width-selecting `explicit` overload set.
Pass B asked it. Pass A did not. See section 5.

---

## 4. Attack item 3 — the two disputes between the passes

### 4.1 `get_data_type<int32>()` — pass A is right, and so is pass B

```
grep -rn "get_data_type<int32>()" $T
  taichi/ir/type.cpp:463:  TI_ASSERT(get_data_type<int32>() == dt);
  taichi/transforms/scalarize.cpp:63
  taichi/transforms/scalarize.cpp:114
  taichi/transforms/scalarize.cpp:530
```
→ **4**.

There is no dispute here. Pass A's section 3.3 and escalation 2, and pass B's
S3 row and escalation 8, both state 4 against report 01's 3, and both identify
the fourth as `taichi/ir/type.cpp:463`. Pass A's framing at report 06 line 32
("report 01's count is 3 for a command that returns 4") and its section 7 item 2
implies it is correcting pass B. It is not; the passes agree.

Report 01's error is at `report-01-ir-types.md:741`, which prints the command
over `taichi/ir/ taichi/analysis/ taichi/transforms/`, states 3, and calls
`type.cpp:463` and `:478` "the only other hits tree-wide". `taichi/ir/type.cpp`
is inside `taichi/ir/`. `:478` is `get_data_type<int64>()` and never matched.
So the figure is one short against its own command and the prose misplaces the
missing line outside a territory that contains it. No site is lost — report 01
inventories `:463` under its section 3.2 — but standing instruction 9 requires
the figure to reconcile against the list, and it does not.

### 4.2 The eleven macro-continuation lines — pass B is right on direction, both are right on disposition

Report 01 at `:751` says `constant_fold.cpp` contributes five and
`ir_builder.cpp` six "macro continuations whose type argument sits on the
**previous** line". Pass A repeats "previous" at report 06 lines 55-57. Pass B's
amended section 3 tests all 52 two-argument rows and reports 41 same-line, 11
following-line, **0 previous-line**.

Read at source:

```
taichi/transforms/constant_fold.cpp:79:      res = TypedConstant(                       \
taichi/transforms/constant_fold.cpp:80:          dst_type, PREFIX(lhs->val.val_int() OP_CPP rhs->val.val_int()));

taichi/ir/ir_builder.cpp:141:  return insert(Stmt::make_typed<ConstStmt>(TypedConstant(
taichi/ir/ir_builder.cpp:142:      TypeFactory::get_instance().get_primitive_type(PrimitiveTypeID::i32),
taichi/ir/ir_builder.cpp:143:      value)));
```

The type argument is on the **following** line in every case. The eleven are
`constant_fold.cpp:79`, `:83`, `:114`, `:117`, `:121` and `ir_builder.cpp:142`,
`:148`, `:154`, `:160`, `:166`, `:172` — five and six, the split report 01
gives.

**So pass B is right that the direction is wrong, and pass A is right that the
eleven are not members of the one-argument class.** These are not competing
claims. Pass A's own section 3.4 lists `ir_builder.cpp:142` among the thirteen
two-argument calls that name a 32-bit type, so pass A classified them correctly
and described their geometry backwards.

Pass A's escalation 4 says the eleven "do not exist as members of the class",
which is true but is not a correction to any published count: report 01 named
them precisely because it was excluding them, and pass A's own residue argument
already concedes that report 01 published 78 candidates rather than 78 sites.
The claim is worth one sentence, not an escalation.

**Both passes miss the practical consequence.** Report 01's escalation 16 says
closing the class needs a line-spanning parse. Anyone writing that parse from
report 01's description will scan **backwards** and find nothing. Pass B states
this at report 06b lines 168-170; pass A does not.

---

## 5. Attack item 4 — pass A's verification of report 01 section 3.13

**The diff is exactly as pass A reports it, and I re-ran it independently.**

```
awk 'NR>=650 && NR<=765 && /^\| `taichi\//' report-01-ir-types.md \
  | grep -oE 'taichi/[a-z_/]+\.(cpp|h):[0-9]+' | sort > r13.txt      -> 64
grep -rn "PrimitiveType::i32" taichi/ir/ taichi/analysis/ taichi/transforms/ \
  | cut -d: -f1,2 | sort > actual.txt                                -> 64
diff r13.txt actual.txt                                              -> empty
uniq -d on each                                                      -> empty
awk 'NR>=650 && NR<=765 && /^\| `taichi\//' … | wc -l                -> 64 rows
grep -rn "PrimitiveType::i32" taichi/inc/ | wc -l                    -> 0
```

64 table rows, 64 grep hits, 64 distinct keys on each side, no duplicates, empty
diff, and pass A's point about the fourth directory holds.

**But the headline pass A draws from it overreaches, in exactly the way standing
instruction 7 names.** Report 06 line 113 reads: "Report 01's spelling-A
enumeration is exactly right, its four exclusions are sound, and I adopt its
dispositions unchanged." The diff certifies the first clause and nothing else.
It shows that the table enumerates the grep output and no more. It says nothing
about whether the four exclusions are sound or whether the 60 remaining
dispositions are right, and pass A performed no check on either — its own
section 5.3 admits "I did not re-derive report 01's sections 3.1 through 3.12."

The instruction's wording is "a verified citation does not verify the claim
attached to it … If the sentence it supports is a quantifier over the tree, or
an inference about reachability, the citation cannot test it and must not be
marked verified." "Its four exclusions are sound" is a claim about adjudication
sitting on evidence about enumeration. Pass A's section 5.1 then lists "the
identity of report 01's section 3.13 table with the spelling-A command output"
under VERIFIED, which is correct, and the report's prose converts that into a
warrant for the dispositions, which it is not.

Pass B handles the same problem better. Its escalation 1 says plainly: "I
reproduced report 01's 64 and did not re-adjudicate its dispositions of them. If
the planner wants S1 independently re-closed, that is a separate pass."

**Verdict.** The enumeration check is right, well constructed, and worth
keeping. The sentence it supports should be cut back to what it proves: report
01's section 3.13 table is a faithful transcription of the `PrimitiveType::i32`
sweep. **Sixty sites in this territory have now been enumerated three times and
adjudicated once.**

---

## 6. Attack item 5 — the `int` locals and parameters in `.cpp` files

Both passes leave this open and both call it the largest remaining surface. Pass
A, section 6 item 1: "'An `int` local that reaches an index' has no textual
signature, so closing it means reading every function in the territory." Pass B,
escalation 7: "Closing it means the same argument-declaration work as section 3,
over a much larger set, and nobody has scoped it."

**Judgement: as both passes frame it, the class is not closeable at any effort,
and both understate why. But the framing is wrong, and under the right framing
most of it is already closed.**

### 6.1 Why the textual route cannot work, measured

The territory's `.cpp` surface:

| Quantity | Value |
|---|---|
| `.cpp` files in `taichi/ir/`, `taichi/analysis/`, `taichi/transforms/` | 99 |
| Lines in them | 27663 |
| Textual `int` declaration candidates | 447 |
| `auto <name> = …` declarations | 1488 |

```
find taichi/ir/ taichi/analysis/ taichi/transforms/ -name '*.cpp' | wc -l
find … -name '*.cpp' | xargs wc -l | tail -1
grep -rnE '(^|[^A-Za-z0-9_>])(int|std::vector<int>|std::array<int)[ &]+[a-zA-Z_][a-zA-Z0-9_]*' … --include=*.cpp | wc -l
grep -rnE '\bauto +[a-zA-Z_][a-zA-Z0-9_]* *=' … --include=*.cpp | wc -l
```

The 1488 is the number that settles it. **The exemplar both reports use is
`auto`-declared.** `taichi/transforms/simplify.cpp:176` is

```
    auto stride_product = 1;
```

That is an `int` local. It is carried by report 01 section 3.6 at
`report-01-ir-types.md:487`, it is the argument at `simplify.cpp:178` which is
one of the two rows the passes dispute in section 1.4, and **no sweep keyed on
the token `int` can see it.** Pass A's own section 6 item 1 names
`simplify.cpp:176`'s `stride_product` as an example of the open class without
noticing that it falsifies the sweep it proposes for closing that class. So does
`demote_dense_struct_fors.cpp:19`'s `total_shape`, also named there.

A sweep over `int` in `.cpp` files would return 447 lines and miss a population
of 1488 in which an unknown fraction are `int`. That is a filter that
under-collects and over-collects at once — the corollary to standing instruction
9, in the same territory it was written for.

### 6.2 The framing is wrong, and the correction shrinks the problem

"An `int` local that reaches an index" conflates two different classes, and only
one of them is open.

**Class one — an `int` local that becomes an IR 32-bit width. Closed.** A host
integer becomes an IR width only by passing through a width-selecting
constructor, and there are exactly two in this territory, both `explicit`, both
enumerated: `TypedConstant` at `taichi/ir/type.h:582` and `Expr` at
`taichi/ir/expr.h:30` with its definition at `taichi/ir/expr.cpp:92`. Because
both are `explicit`, no local can cross silently, and the type's name appears at
every crossing. The crossings are **76**: 73 one-argument `TypedConstant`
constructions from section 1.3, plus 3 `Expr` sites from section 7. It does not
matter how the local was spelled, `int` or `auto` or a function return; it is
caught at the crossing. **Both passes had the material to say this and neither
said it.**

**Class two — an `int` local that overflows in host arithmetic before it ever
reaches the IR. Open, and it is not a spelling question.**
`simplify.cpp:176-187` is the example: `stride_product` accumulates
`*= stmt->strides[i]` across axes in a host `int`, and the product can exceed
2^31 with no IR object involved at any point. No grep over any token answers
this, because the defect is arithmetic, not vocabulary. Neither report separates
this class out, and both file it under the same heading as class one.

### 6.3 What closing class two would take

Stated as what it would require, not as a recommendation, and not as work I did.

1. **A compilation database.** There is no `compile_commands.json` in the tree
   and no build directory; `build.py` is the entry point. Everything below needs
   one first.
2. **`-Wconversion -Wshorten-64-to-32 -Wsign-conversion` over the 99 files.**
   This closes the narrowing-on-assignment class that pass A's section 6 item 2
   declares it has no filter for — `int x = some_size_t;` — completely and
   mechanically, because the compiler sees the declared type of both sides
   whether or not either was spelled `auto`.
3. **A `clang-query` or AST-matcher pass** for the overflow class: every
   `VarDecl` or `BinaryOperator` of integer type whose value reaches an index
   expression or one of the 76 crossings above. This is the only instrument that
   sees through `auto`.
4. **A ruling on where the boundary sits.** Class two is a host-arithmetic
   overflow question. It belongs to whoever owns item 6.2's arithmetic, not to a
   spelling enumeration, and calling it "the largest surface left open by this
   pass" attributes to territory 06 a problem territory 06's method cannot
   address in principle.

**Verdict.** Not closeable as posed. Class one is already closed at 76 crossings
and neither report says so. Class two is closeable by compiler diagnostics and
an AST query, needs a compilation database first, and is a different kind of
defect than the one this territory enumerates.

---

## 7. Attack item 6 — the 24 sites turning on the mesh ruling

Pass A's escalation 5: "Of my 46 new spelling-D sites, 12 are in
`make_block_local.cpp`, 9 in `make_mesh_block_local.cpp` and 3 in
`demote_mesh_statements.cpp` … Twenty-four of my 46 turn on the same ruling."

The 24 is arithmetically right — I counted it from pass A's own list rather than
from its prose:

```
grep -cE 'make_mesh_block_local|demote_mesh_statements|make_block_local' A_D.txt   -> 24
```

**But "the same ruling" is wrong, and pass A's own next clause contradicts it:**
"`make_block_local.cpp` is block-local storage rather than mesh, so the boundary
matters." It does, and the source settles it. The three files have three
different gates.

| Pass | Gate on the pass itself | Gate at its only call site |
|---|---|---|
| `make_block_local` | `make_block_local.cpp:16`: returns unless `task_type == struct_for` | `compile_to_offloads.cpp:231`: the `make_block_local` bool. **No mesh gate, no arch gate** |
| `make_mesh_block_local` | `make_mesh_block_local.cpp:647`: returns unless `task_type == mesh_for` | `compile_to_offloads.cpp:221`: `is_extension_supported(arch, Extension::mesh)` **and** `config.make_mesh_block_local` **and** `config.arch == Arch::cuda` |
| `demote_mesh_statements` | mesh statements only | `compile_to_offloads.cpp:237`: `is_extension_supported(arch, Extension::mesh)` |

So the ruling applies mechanically in three groups, not one.

**Group 1 — NOT mesh. Twelve sites. The mesh ruling does not reach them.**
`taichi/transforms/make_block_local.cpp` `:145`, `:152`, `:165`, `:183`, `:194`,
`:202`, `:277`, `:285`, `:298`, `:303`, `:315`, `:328`.

This pass runs on `struct_for` offloads and returns immediately on anything
else. Struct-for is the sparse path, and plan section 4.1 states "Sparsity is
required. Do not treat sparse machinery as surplus." These twelve are base work
under section 2.2a whatever the mesh ruling says. Filing them with the mesh
group, as pass A's escalation 5 and pass B's escalation 2 both do, invites the
planner to park twelve in-scope sites on a ruling that does not govern them.

**Group 2 — mesh-gated, arch-agnostic. Three sites.**
`taichi/transforms/demote_mesh_statements.cpp` `:86`, `:94`, `:123`.

These turn on the bare mesh ruling and nothing else. All three are visible only
through the brace form, so **pass B holds none of them.**

**Group 3 — mesh-gated AND CUDA-only. Nine sites.**
`taichi/transforms/make_mesh_block_local.cpp` `:83`, `:116`, `:166`, `:169`,
`:204`, `:268`, `:304`, `:327`, `:578`.

Doubly gated. Even if mesh is ruled in scope, `compile_to_offloads.cpp:221`
requires `config.arch == Arch::cuda`, and plan section 2.2a puts CUDA in the
BRANCH column: "A vendor backend is CUDA, AMDGPU, DirectX, or anything else tied
to one supplier or one operating system." So these nine need mesh ruled in
scope **and** the CUDA gate ruled irrelevant before any of them is base work.
Neither report records the arch gate.

12 + 3 + 9 = 24.

**Two of group 3 are unreachable as the tree stands, and neither report says
so.** `make_mesh_block_local.cpp:166` and `:327` sit in
`if (config_.arch == Arch::x64 || config_.arch == Arch::arm64)` branches:

```
make_mesh_block_local.cpp:165:  if (config_.arch == Arch::x64 || config_.arch == Arch::arm64) {
                        :166:    block_dim_val = block_->push_back<ConstStmt>(TypedConstant(1));
                        :167:  } else {
                        :169:    block_dim_val = …<ConstStmt>(TypedConstant{offload_->block_dim});

make_mesh_block_local.cpp:326:  if (config_.arch == Arch::x64 || config_.arch == Arch::arm64) {
                        :327:    thread_idx_stmt = block_->push_back<ConstStmt>(TypedConstant(0));
                        :328:  } else {
                        :329:    thread_idx_stmt = …<LoopLinearIndexStmt>(offload_);
```

The pass's only caller requires `arch == Arch::cuda`, so the `x64`/`arm64` arms
cannot execute. Pass A's rows describe them as "block dimension on x64/arm64"
and "thread index on x64/arm64" with no reachability note; pass B's rows say the
same. I record the qualification rather than removing them, per standing
instruction 3, and note that "dead today" is falsified by a future caller —
which is pass B's own stated principle at its escalation 6 for
`scratch_pad.h:232`, applied here to its own rows.

**I have not made the mesh ruling and this section does not assume it.** The
groups are stated so that whichever way it lands, the sites move mechanically.

---

## 8. What each pass found that the other did not

Recorded because the planner cannot use either report alone.

### 8.1 Pass A alone, all three verified here

1. **`taichi/ir/type_system.cpp`, the `PRIM` macro and the internal-op signature
   table.** Section 2 above. Real; the largest single finding in the territory;
   requalified on the SPIR-V half.
2. **`TypedConstant{…}`, seven lines.** Section 1.1. Four sites pass B cannot
   see.
3. **`PrimitiveType::u32` as an address element type.**
   `grep -rn "PrimitiveType::u32" $T` returns 4. Three are the mechanism or a
   dispatch arm. The fourth is
   `taichi/transforms/make_mesh_thread_local.cpp:26`:

   ```
   auto data_type = PrimitiveType::u32;  // unt32_t type address
   ```

   with `data_type_size(data_type)` taken from it at `:27`. The in-tree comment
   says "address". Report 01's rows for this file are `:42`, `:106`, `:114`.
   Pass B has no unsigned spelling at all and misses it.

### 8.2 Pass B alone, all three verified here

1. **`Expr(<host int>)` — the same mechanism on a second type.** Verified at
   source: `taichi/ir/expr.h:30` `explicit Expr(int32 x);`, defined at
   `taichi/ir/expr.cpp:92-94` as
   `expr = std::make_shared<ConstExpression>(PrimitiveType::i32, x);`.
   `grep -rnE '(^|[^A-Za-z0-9_])Expr\(' $T --include=*.h --include=*.cpp`
   returns 35 lines. The two live ones are
   `taichi/ir/frontend_ir.cpp:1837` `ExprGroup(Expr(i))` with `int i` at `:1835`,
   and `:1846` `ExprGroup(Expr(i), Expr(j))` with `int i` at `:1843` and
   `int j` at `:1844` — three sites on two lines, both inside
   `expand_tensor_or_scalar`, both becoming frontend `IndexExpression` indices.
   Neither line is in report 01. **Pass A misses the spelling entirely.**

   I checked pass B's own bound the way it failed to check its `TypedConstant`
   one. The follow-character census of `Expr` returns no `{`, so
   `grep -rnE '(^|[^A-Za-z0-9_])Expr\{' $T` is empty and pass B's `Expr(` bound
   does hold.

   I also reproduced all three of pass B's constructor sweeps at report 06b
   lines 300-310 and got its exact hits: four 32-bit constructor declarations
   across two types (`Expr`, `TypedConstant`), four 64-bit siblings across the
   same two, and three `int`-spelled constructors
   (`type_utils.h:203`, `snode.h:27`, `auto_diff.cpp:705`) with no 64-bit
   sibling. A fourth sweep of my own for unnamed-parameter declarations,
   `grep -rnE 'explicit +[A-Z][A-Za-z0-9_]*\((const )?(int|int32|int64|uint32|uint64|size_t|std::size_t)\)' $T`,
   returns nothing. **Pass B's "exactly two types" holds for this territory**,
   with one boundary it does not state: the sweeps see only declarations written
   on one line inside the four directories, so a type declared in
   `taichi/common/` or `taichi/program/` and constructed here would be invisible
   to them.

2. **`sizeof(int32)` as a hardcoded byte width.**
   `grep -rn "sizeof(int32)\|sizeof(int)\|sizeof(i32)" $T` returns 5.
   `simplify.cpp:239`, `:261` are report 01 section 3.7.
   `scalarize.cpp:1113`, `:1140` are inside a `/* … */` block opened at
   `scalarize.cpp:1107` — I read `:1104-1116` and confirm they are IR
   pseudo-code in a comment, so pass B's exclusion is right. The fifth is new:

   ```
   taichi/ir/statements.h:1786:  std::size_t size_in_bytes() const {
   taichi/ir/statements.h:1787:    return sizeof(int32) + entry_size_in_bytes() * max_size;
   ```

   `AdStackAllocaStmt`. Every neighbouring quantity is `std::size_t` —
   `max_size` at `:1770`, `element_size_in_bytes` at `:1779`,
   `entry_size_in_bytes` at `:1783` — and the four-byte header is the one
   hardcoded term. `grep -c "1787"` over pass A's report and notes returns 0 and
   0. **Pass A misses it**, because its spelling-F sweep is keyed on `\b32\b`,
   which cannot match inside the token `int32`.

3. **`IRBuilder::get_int32` swept and reported empty.** No site, but the sweep
   was run and the negative result recorded, which is what standing instruction
   9's corollary asks for.

### 8.3 A note on pass A's spelling-F filter

Its command is

```
grep -rn "\b32\b" $T | grep -v "int32\|uint32\|i32\|u32\|f32\|float32\|//"
```

The trailing `//` discards any line carrying a comment anywhere, including a
code line with a trailing comment. I tested the cost: `\b32\b` returns 12 lines
in the whole territory, the type-name alternatives remove none of them because
`\b32\b` cannot match inside `int32`, and the single line the `//` term discards
is `taichi/inc/constants.h:8`, a genuine comment. **The filter has a real blind
spot and it costs nothing here.** Recorded so nobody reuses the pattern on a
larger tree assuming it is safe.

---

## 9. Every count in this file, with its command

| Claim | Command | Result |
|---|---|---|
| brace form exists | `grep -rn "TypedConstant{" $T` | 7 |
| paren form | `grep -rn "TypedConstant(" $T` | 139 |
| follow-character census | `grep -rno "TypedConstant." $T \| sed 's/.*TypedConstant//' \| sort \| uniq -c` | 139 `(`, 22 space, 19 `:`, 12 `>`, 7 `{` |
| one-argument call sites | 139 − 14 decl − 52 two-arg + 7 brace | 80 |
| of those, fixing i32 | 80 − 3 bool − 2 float32 − 2 DataType | 73 |
| pass A's new-site table integrity | 58 rows = 46 D + 7 E + 2 B + 2 F + 1 H | consistent |
| pass A's 80-row table integrity | 80 rows, 0 duplicate keys, all inside the 146-line union; 7 + 5 + 22 + 46 = 80 | consistent |
| pass B's 40 ⊂ pass A's 46 | `comm -13 A_D.txt B_C.txt` | empty |
| the six-site gap | `comm -23 A_D.txt B_C.txt` | 6, listed in 1.2 |
| pass A's 46 against report 01's ranges | 94 ranges extracted, tested against all 46 | 2 inside a range, 0 exactly named |
| corrected tiering | 22 named + 7 in range + 44 absent | 73 |
| `type_system.cpp` i32 tokens | `grep -c "\bi32\b\|i32_void" taichi/ir/type_system.cpp` | 37 = 2 definitions + 35 uses |
| `type_system` in report 01 / notes 01 | `grep -c "type_system" …` | 0, 0 |
| `type_system` in report 01b / notes 01b | `grep -c "type_system" …` | 3, 3, none the table |
| `type_system` in report 06b / notes 06b | `grep -c "type_system" …` | 0, 0 |
| `vkGlobalThreadIdx` tree-wide | `grep -rn "vkGlobalThreadIdx" taichi/` | 3, none in codegen |
| `globalInvocationId` tree-wide | `grep -rn "globalInvocationId" taichi/` | 1, `spirv_codegen.cpp:1415` |
| `get_data_type<int32>()` | `grep -rn "get_data_type<int32>()" $T` | 4 |
| report 01 section 3.13 diff | `diff r13.txt actual.txt` | empty, 64 vs 64 |
| `PrimitiveType::i32` in `taichi/inc/` | `grep -rn "PrimitiveType::i32" taichi/inc/ \| wc -l` | 0 |
| `PrimitiveType::u32` | `grep -rn "PrimitiveType::u32" $T` | 4, 1 real |
| `sizeof(int32)` family | `grep -rn "sizeof(int32)\|sizeof(int)\|sizeof(i32)" $T` | 5 |
| `\b32\b` in territory | `grep -rn "\b32\b" $T \| wc -l` | 12 |
| `Expr(` lines | `grep -rnE '(^\|[^A-Za-z0-9_])Expr\(' $T --include=*.h --include=*.cpp` | 35 |
| `Expr{` lines | `grep -rnE '(^\|[^A-Za-z0-9_])Expr\{' $T` | 0 |
| 32-bit constructor-shaped declarations | pass B's sweep 1, rerun | 4, two types |
| 64-bit siblings | pass B's sweep 2, rerun | 4, same two types |
| `int`-spelled constructors | pass B's sweep 3, rerun | 3, no 64-bit sibling |
| unnamed-parameter constructors | `grep -rnE 'explicit +[A-Z][A-Za-z0-9_]*\((const )?(int\|int32\|int64\|uint32\|uint64\|size_t\|std::size_t)\)' $T` | 0 |
| pass A's header regex | reproduced verbatim | 102 lines, 16 files |
| territory `.cpp` files / lines | `find … -name '*.cpp'` | 99 / 27663 |
| textual `int` declaration candidates in `.cpp` | section 6.1 | 447 |
| `auto` declarations in `.cpp` | section 6.1 | 1488 |
| the 24 mesh-adjacent sites | `grep -cE 'make_mesh_block_local\|demote_mesh_statements\|make_block_local' A_D.txt` | 24 = 12 + 3 + 9 |

`A_D.txt` and `B_C.txt` are the spelling-D rows of pass A's section 4 table and
the status-C rows of pass B's section 4 table, extracted by `awk` on the table
delimiters. Both extractions are reproduced in the commands above their first
use.

---

## 10. What I did not do

1. **I did not re-adjudicate the 60 `PrimitiveType::i32` sites.** Section 5
   establishes that report 01's table transcribes the sweep faithfully, and that
   nobody in three passes has checked the dispositions. I did not check them
   either, and I say so rather than inheriting pass A's "exactly right".
2. **I did not open all 80 one-argument constructions.** I opened the seven
   brace lines, the seven exclusions' argument declarations, the two disputed
   `simplify.cpp` rows, the two dead-branch rows and the `demote_mesh_statements`
   conditional. For the remainder I verified the arithmetic of both partitions
   against each other and against the raw greps, which is a weaker check and is
   labelled as one. The two passes were blind to each other and agree row for
   row on the 73 paren lines, which is the strongest evidence available short of
   a third full pass.
3. **I did not follow `type_system.cpp`'s consumers past the two spines' entry
   points.** I established that `linear_thread_idx` reaches
   `runtime.cpp:1650` and that `vkGlobalThreadIdx` reaches no SPIR-V codegen
   arm. What the LLVM codegen does with the declared i32 is territory 02.
4. **I did not compile anything.** Section 6.3 describes what a compiler-based
   close would require; it is not a report of one. Both passes' premise that a
   host `int` argument selects `explicit TypedConstant(int32)` remains inferred
   from overload resolution, as both say.
5. **I made no scope ruling.** Mesh, autodiff, texture and quantised types are
   grouped so a ruling applies mechanically, and not pre-empted.
6. **I proposed no change to any source file** and did not decide that anything
   is unnecessary, including the two unreachable branches in section 7 and the
   uncalled `scratch_pad.h:229-232` that pass B flagged.

---

## 11. Escalations to the planner

1. **Neither report is usable alone.** Section 1.5. The planner needs the union
   of both, or a merged inventory. Pass A holds ten of thirteen routes, pass B
   holds ten, and the intersection is seven.
2. **The corrected figure is 44, and it is a convergence, not a compromise.**
   Section 1.4. Pass A's 46 is two too high by its own tiering rule; pass B's 40
   is four too low because of the bound. Both corrections are mechanical and
   both were verified against the source and against report 01's cited ranges.
3. **The SPIR-V half of pass A's headline finding does not hold as written.**
   Section 2. `vkGlobalThreadIdx` is declared i32 at
   `taichi/ir/type_system.cpp:370` and has no SPIR-V codegen arm; the name the
   codegen dispatches on, `globalInvocationId`, is not in the internal-op table.
   The 32-bit cap on the SPIR-V global thread index is real and lives at
   `taichi/codegen/spirv/spirv_codegen.cpp:1416`, which is territory 02. This
   should be settled before the finding is carried anywhere, because as written
   it claims a single type-system line caps both spines and it caps one.
4. **Report 01's mechanism sentence needs one word changed, in four places.**
   `report-01-ir-types.md:77`, `:735`, `:1592`, `:1618` say "implicit". The
   constructor is `explicit`. The class is real and the size is unaffected. Both
   passes raise this; I confirm it and add that the correction is what licenses
   the enumeration in the first place, so it is not cosmetic.
5. **Report 01's `get_data_type<int32>()` figure is 3 for a command returning
   4**, at `report-01-ir-types.md:741`, and the prose there places
   `taichi/ir/type.cpp:463` outside a territory containing it. No site lost.
   Both passes raise it independently; pass A's report frames it as a dispute
   with pass B and it is not one.
6. **Report 01's continuation-line description points the wrong way**, at
   `report-01-ir-types.md:751`. Following line, not previous, in all eleven.
   This only bites the line-spanning parse report 01's escalation 16 asks for.
7. **The mesh ruling governs 12 sites, not 24.** Section 7. Twelve more are
   struct-for block-local storage with no mesh gate and no arch gate, and are
   base work under plan section 2.2a and section 4.1's sparsity requirement.
   Nine more are additionally CUDA-gated at `compile_to_offloads.cpp:221` and so
   sit behind a second ruling that plan section 2.2a already answers for vendor
   backends. Both reports file all 24 under one heading.
8. **Two sites in group 3 are unreachable under the current call graph.**
   `make_mesh_block_local.cpp:166` and `:327`. Recorded, not removed. If the
   planner wants unreachable code excluded as a class, that rule governs these,
   pass B's `scratch_pad.h:232`, and pass B's dead `taichi::lang::value<T>`
   template at `expr.h:129-132` together, and it should be given rather than
   invented three times.
9. **"The largest open surface" is misdescribed by both reports.** Section 6.
   The class of an `int` local reaching an IR 32-bit width is already closed at
   76 crossings, because both width-selecting constructors are `explicit` and
   both are enumerated. What is open is host-arithmetic overflow before the IR,
   which no spelling sweep can address and which needs a compilation database
   and compiler diagnostics. That is a different territory's problem and should
   be assigned as one rather than left as this territory's residue.
10. **Sixty sites have been enumerated three times and adjudicated once.**
    Section 5. If the planner wants report 01's section 3.13 dispositions
    independently tested, no pass has done it and pass A's report reads as
    though one has.
11. **Pass B's constructor sweeps have a stated boundary worth recording.**
    Section 8.2 item 1. They see only single-line, named-parameter declarations
    inside the four directories. A width-selecting type declared in
    `taichi/common/` or `taichi/program/` and constructed in this territory
    would not appear. I found none, but I did not sweep those directories, and
    neither did pass B.

---

## 12. Divergence from adversary 06-1

`modernization/investigation/adversary-06-1.md` did not exist when this file was
written:

```
ls modernization/investigation/adversary-06-*.md
ls: cannot access 'modernization/investigation/adversary-06-*.md': No such file or directory
```

Nothing appended. If the planner wants the comparison, this file is ready to
take it.
