# Adversary 06-1 — the enumeration of 32-bit spellings in the frontend IR

Adversary 1 of 2, territory 06. Judging
`report-06-i32-spellings.md` (pass A) and `report-06b-i32-spellings.md` (pass B),
with `notes-06-i32-spellings.md` and `notes-06b-i32-spellings.md` consulted where
the reports left me uncertain.

Worked against `PROJECT-PLAN.md` dated **2026-09-09**, re-read at the point of
writing this sentence. Section 10 item 9 and its corollary govern everything
below; section 2.2a decides the scope rulings in section 7.

I modified no file except this one. Every command below was run from
`/opt/project/taichi` at commit `ba0e81dce`. Throughout,

```
T="taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/"
```

My own enumeration script is at
`/tmp/claude-1000/-opt-project-taichi/0b12bd28-d413-4ea3-b486-d56840a983ae/scratchpad/tc.py`.
It is a bracket-balancing parser over the raw text of every `.cpp` and `.h` in
`$T`; it finds every occurrence of the token `TypedConstant`, classifies the
syntactic form that follows it, and splits the argument list on top-level commas
across line breaks. I wrote it before reading either report's arity table, and I
did not tune it to reproduce either.

---

## 1. Verdict on the principal task, in one page

**Pass A is right that the brace form is real. Pass B's bound is not a bound.**

```
grep -rn "TypedConstant{" $T   -> 7
```

```
taichi/transforms/demote_mesh_statements.cpp:86
taichi/transforms/demote_mesh_statements.cpp:94
taichi/transforms/demote_mesh_statements.cpp:123
taichi/transforms/make_mesh_block_local.cpp:80
taichi/transforms/make_mesh_block_local.cpp:169
taichi/transforms/make_mesh_block_local.cpp:200
taichi/transforms/make_mesh_block_local.cpp:575
```

Pass A named `make_mesh_block_local.cpp:169` in its brief. It is
`TypedConstant{offload_->block_dim}` with `int block_dim{1}` at
`taichi/ir/statements.h:1418`. Direct-list-initialisation considers explicit
constructors, and `int` to `int32` is an identity conversion with no narrowing,
so the brace form selects `explicit TypedConstant(int32)` exactly as the paren
form does. It is a construction, not a decoration.

**But pass B's bound fails in a second way neither report saw, and pass A's
follow-character census cannot see it either.** There is a third syntactic form:

```
taichi/ir/statements.h:2117      TypedConstant constant(dt, value);
taichi/transforms/constant_fold.cpp:53   TypedConstant new_constant(dst_type);
taichi/transforms/constant_fold.cpp:189  TypedConstant new_constant(dst_type);
```

These are named-variable constructions. In pass A's census they fall in the
"22 space-following lines" bucket, which pass A characterised as "the class
definition, field and parameter declarations, one format string, and local
variable declarations that take a `dst_type` argument or a copy." That
description is accurate as prose but pass A counted them out of the class rather
than into it. Three real constructions sit outside both greps.

**And a fourth form exists with no `TypedConstant` token at all.**
`taichi/ir/type_utils.h:147-198` constructs 24 `TypedConstant`s as
`return {dt, std::numeric_limits<T>::max()};` — copy-list-initialisation of the
return value, reaching the two-argument template at `type.h:616`, which is the
one constructor in the set that is **not** `explicit`. This is precisely the
failure the corollary to standing instruction 9 was written about: a real
construction with no width and no type name anywhere on the line.

Its consequence is bounded, and I state the bound rather than the alarm: because
every single-argument constructor at `type.h:575-612` **is** `explicit`,
`return {5};` does not compile, so the tokenless form can never carry a
one-argument i32 site. It can only carry a two-argument DataType-derived one, and
all 24 do. **No site is lost. The enumeration mechanism both passes claimed to be
exact is not exact, and it happens not to matter here.** That is a different
sentence from the one either report wrote.

Ruled out by direct search, so that the closure is a result and not an
assumption:

```
grep -rn "using .*= *TypedConstant\|typedef .*TypedConstant" taichi/   -> 0
grep -rn "##TypedConstant\|TypedConstant##" taichi/                    -> 0
```

No alias and no token-pasting macro can produce the identifier.

### 1.1 The true membership of the constructor class

From my parser, over `$T`:

| Form | Arity | Count |
|---|---|---|
| `TypedConstant(` direct, paren | 1 | 85 |
| `TypedConstant(` direct, paren | 2 | 53 |
| `TypedConstant(` direct, paren | 0 | 1 |
| `TypedConstant{` direct, brace | 1 | 7 |
| `TypedConstant name(` named variable | 1 | 2 |
| `TypedConstant name(` named variable | 2 | 1 |
| identifier not followed by a construction | — | 47 |
| parser artefact: `class TypedConstant {` at `type.h:549` | — | 1 |

85 + 53 + 1 + 7 + 2 + 1 + 47 + 1 = **197 occurrences of the identifier**, over
199 rows of raw parser output before the two `type_utils.h` function definitions
(`:145`, `:173`) are struck as false named-variable matches.

Subtract the declarations in `taichi/ir/type.h` — the default at `:572` (arity 0),
twelve single-argument constructors at `:575`, `:582`, `:585`, `:588`, `:591`,
`:594`, `:597`, `:600`, `:603`, `:606`, `:609`, `:612`, and the two-argument
template at `:616`:

| | Call sites |
|---|---|
| one-argument, paren | 85 − 12 = 73 |
| one-argument, brace | 7 |
| one-argument, named variable | 2 |
| **one-argument total** | **82** |
| two-argument, paren | 53 − 1 = 52 |
| two-argument, named variable | 1 |
| **two-argument total** | **53** |
| **class total** | **135** |

Pass A says 132. Pass B implies 125 (139 lines minus 14 declarations). **Both are
low, and neither is 135.** Pass A's 132 is 80 + 52; it is short by the three
named-variable constructions. Pass B's is short by those three and by the seven
brace lines.

Pass A's internal arity split is also miscast in a way that cancels: it reports
"86 one-argument paren, of which 13 are the constructor declarations." There are
85 one-argument paren occurrences and 12 one-argument declarations; pass A folded
the zero-argument default at `type.h:572` into both halves. 86 − 13 and 85 − 12
are both 73, so its published figure survives its own arithmetic.

### 1.2 The i32 subset, and why the two reports differ

Of the 82 one-argument constructions:

| Overload selected | Count | Sites |
|---|---|---|
| `int32` — host `int` or `int32`, **no width token on the line** | 60 | 56 paren + 4 brace |
| `int32` — argument carries an explicit `(int32)` / `int32(` cast | 13 | 10 paren + 3 brace |
| `uint1` — argument declared `bool` | 3 | `check_out_of_bound.cpp:48`, `:107`, `:180` |
| `float32` — argument declared `float32` | 2 | `auto_diff.cpp:957`, `:963` |
| `DataType` — selects `type.h:575`, width derived from the operand | 4 | `alg_simp.cpp:71`, `:92`, `constant_fold.cpp:53`, `:189` |

60 + 13 + 3 + 2 + 4 = **82**.

**Real i32 sites in the one-argument class: 73.** That is pass A's number, and
it is correct. Pass B's 56 is correct for a narrower question — one-argument
constructions that fix i32 *with no width token on the line* and *in paren form*
— and pass B is explicit that it assigns the 13 cast sites to its S4 instead.
The two figures are not in conflict on the 69 sites they share.

**Where pass B is genuinely short is four sites, not seventeen.**
`demote_mesh_statements.cpp:86`, `:94`, `:123` and
`make_mesh_block_local.cpp:169` are uncast brace-form i32 constructions. Pass B's
S4 command is
`grep -rn "TypedConstant((int32)\|TypedConstant{(int32)\|int32(" $T`, whose
second alternative catches `make_mesh_block_local.cpp:80`, `:200`, `:575`. So
three of the seven brace lines reached pass B through a different door, and four
did not reach it at all.

**The four are not a rounding error.** `demote_mesh_statements.cpp:86` and `:94`
fix the size of a high-to-low mesh relation, and `:94` multiplies it into a
relation-table index at `:105`. `make_mesh_block_local.cpp:169` is the block
dimension of the block-local prologue loop on every GPU target.

### 1.3 The spelling count: neither eight nor six

The brief tells me not to average and not to pick the larger. I am not doing
either. **The disagreement is not empirical. It is a granularity rule that
neither report stated.** Pass A splits `PrimitiveType::u32` from
`PrimitiveType::i32` into separate spellings but merges the cast and uncast
`TypedConstant` forms into one. Pass B merges the unsigned into the signed but
splits the cast form out as its own S4. Applied to the same tree, those two rules
must give different totals, and eight versus six is exactly the difference they
produce. Neither report can be faulted for its arithmetic; both can be faulted
for publishing a headline count over an undeclared rule, which is standing
instruction 9's corollary read the other way — a count is only as complete as the
thing that generated the list, and it is only as *meaningful* as the membership
rule behind it.

Under one rule stated in advance — **a distinct textual form by which a 32-bit
width is fixed on an IR object in this territory** — I count **thirteen**, each
with its own command:

| # | Spelling | Command over `$T` | Lines |
|---|---|---|---|
| 1 | `PrimitiveType::i32` | `grep -rn "PrimitiveType::i32"` | 64 |
| 2 | `PrimitiveType::u32` | `grep -rn "PrimitiveType::u32"` | 4 |
| 3 | `PrimitiveTypeID::i32` | `grep -rn "PrimitiveTypeID::i32"` | 27 |
| 4 | `PrimitiveTypeID::u32` | `grep -rn "PrimitiveTypeID::u32"` | **17 — neither pass ran this** |
| 5 | `get_data_type<int32>()` / `<uint32>()` | `grep -rnF "get_data_type<int32>()"` / `<uint32>()` | 4 / 1 |
| 6 | explicit `(int32)` or `int32(` narrowing | report 01's own command | 37 |
| 7 | one-argument `TypedConstant`, host `int`, no width token | section 1.1, three syntactic forms | 60 sites |
| 8 | one-argument `Expr`, host `int` | `grep -rnE '(^\|[^A-Za-z0-9_])Expr\('` | 3 sites on 2 lines |
| 9 | bare `DataType i32` from the file-local `PRIM` macro | `grep -n "\bi32\b\|i32_void" taichi/ir/type_system.cpp` | 37, of which 35 uses |
| 10 | literal bit count into `get_primitive_int_type` | `grep -rn "get_primitive_int_type"` | 1 site, `frontend_ir.cpp:266` |
| 11 | a member defaulting to 32 | `taichi/ir/type.h:372` `int num_bits_{32}` | 1 |
| 12 | `sizeof(int32)` | `grep -rn "sizeof(int32)"` | 3 |
| 13 | plain `int` / `int32` / container-of-`int` declared on an IR class | section 5 | 112 + 4 |

Thirteen rows. **Spelling 4 is in neither report.** Spelling 8 is in pass B only.
Spelling 9 is in pass A only. Spelling 12's `statements.h:1787` is in pass B
only. So the union of the two reports is still short by one spelling, and neither
report alone is within two of the true figure under any consistent rule.

**On spelling 4.** Seventeen lines. Most are dispatch arms and mapping arms with
the same dispositions pass A gave the signed set, but two fix a 32-bit width:
`taichi/ir/type.cpp:238`, which pass B found through the signed token on the same
line, and **`taichi/ir/ir_builder.cpp:155`**, the body of `IRBuilder::get_uint32`,
which is the unsigned twin of `ir_builder.cpp:143`. Pass A inventoried `:143` and
not `:155`. Pass B named `get_uint32` under its S6 and concluded the spelling
"adds no site", which is true of its *calls* and not of its *body*. A third,
`taichi/ir/type.cpp:513`, is `TI_ASSERT(get_data_type<uint32>() == dt)` inside
`val_uint32()`, the unsigned twin of the `type.cpp:463` abort site both reports
carry.

---

## 2. Attack 1 — `taichi/ir/type_system.cpp`

**Pass A's claim is correct, is correct against both halves of report 01, and is
understated in its consequence. It is also mis-escalated.**

Verified at source, `taichi/ir/type_system.cpp:157-174`:

```
157: namespace {
161: #define PRIM(dt) \
162:   DataType dt =  \
163:       TypeFactory::get_instance().get_primitive_type(PrimitiveTypeID::dt);
165: PRIM(i32)
171: DataType i32_void = i32;
```

Inside that anonymous namespace the bare token `i32` is a `DataType` object. The
file contains no `PrimitiveType::i32`, no `TypedConstant`, no `(int32)` and no
`get_data_type<int32>`, so all four of report 01's sweeps pass over it.

```
grep -n "\bi32\b\|i32_void" taichi/ir/type_system.cpp   -> 37
```

Two definitions and 35 use lines. Pass A's six-class split of the 35 reproduces
row for row; I checked each class against the grep output and the arithmetic
(4 + 1 + 2 + 12 + 16 + 1 = 36 entries over 35 distinct lines, `:302` counted
twice) is right.

**Report 01's coverage, checked both ways:**

```
grep -c "type_system" report-01-ir-types.md notes-01-ir-types.md      -> 0, 0
grep -n  "type_system" report-01b-ir-types.md notes-01b-ir-types.md
```

Pass A wrote "neither report 01 nor its notes has this file", and for the A half
that is exactly right. The **B** half does cite the file, six times, but only at
`type_system.h:246`, `:280` and `type_system.cpp:140` — `Signature::type_check`
and `Operation::type_check`, the machinery. **No half of report 01 touches the
signature table at `:296-380`, and none names the `PRIM` macro.** Pass A's
finding survives the check it did not run.

**Why it is more consequential than pass A said.** The return type is not
decorative. Traced end to end, all inside territory 06:

- `type_system.cpp:286-288`, `PLAIN_OP(name, ret, ctx, ...)` stores `ret` as the
  operation's signature return.
- `type_system.cpp:252-258`, `InternalCallOperation` passes it to
  `Signature(type_exprs_from_dts(params), !result)`.
- `frontend_ir.cpp:635`, `InternalFuncCallExpression::type_check` assigns
  `ret_type = op->type_check(arg_types)`.
- `type_system.cpp:268-276`, `flatten` casts that `ret_type` onto the
  `InternalFuncStmt` it pushes.

So `PLAIN_OP(linear_thread_idx, i32, true)` at `:307` pins the `ret_type` of
every thread-index statement to i32, and `PLAIN_OP(vkGlobalThreadIdx, i32, false)`
at `:370` does the same on the other spine.

**Pass A's escalation is half wrong.** It escalated the file on the ground that
"the declarations are in my territory, the consumers are in territory 02." One
consumer is not:

```
taichi/ir/frontend_ir.cpp:1492-1493:
  return Expr::make<InternalFuncCallExpression>(
      Operations::get(InternalOp::linear_thread_idx), std::vector<Expr>{});
```

`ASTBuilder::insert_thread_idx_expr` is in `taichi/ir/`. The declaration, the
type-check that consumes it, and one call site are all inside territory 06. Only
the SPIR-V-side consumers are elsewhere. The escalation should be narrowed, not
withdrawn.

**One imprecision in pass A.** It files the sixteen `i32_void` rows as "a
placeholder, not a width." `type_system.cpp:171` is `DataType i32_void = i32;`.
It **is** i32; the name records only that the caller discards the value. Pass A's
practical ruling — that these do not bear on item 6.2 — is sound. Its stated
reason is not the reason.

**Scope, which pass A left open and which section 2.2a decides mechanically.**
Of the seven index-bearing rows:

| Line | Op | Spine | Under 2.2a |
|---|---|---|---|
| `:307` | `linear_thread_idx` | LLVM, reaches `x64`/`arm64` CPU-only | **BASE** |
| `:369` | `localInvocationId` | SPIR-V | **BASE** |
| `:370` | `vkGlobalThreadIdx` | SPIR-V | **BASE** |
| `:373` | `subgroupElect` | SPIR-V | **BASE** |
| `:375` | `subgroupSize` | SPIR-V | **BASE** |
| `:376` | `subgroupInvocationId` | SPIR-V | **BASE** |
| `:302` | `insert_triplet_f32/f64` | sparse matrix | ruling needed |

The twelve CUDA warp-intrinsic rows (`:325`, `:327`, `:332-334`, `:339-343`,
`:347`, `:348`) are vendor by 2.2a and park. **Six of the seven are base work on
both spines under the plan as written, and no ruling is needed for them.**

---

## 3. Attack 2 — `taichi/ir/type.h:582` is `explicit`

**Both passes are right. Report 01's mechanism sentence is wrong as C++.**

```
taichi/ir/type.h:582:  explicit TypedConstant(int32 x) : dt(PrimitiveType::i32), val_i32(x) {
```

Every single-argument constructor in the set carries `explicit`: `:575`, `:582`,
`:585`, `:588`, `:591`, `:594`, `:597`, `:600`, `:603`, `:606`, `:609`, `:612`.
Only the default at `:572` and the two-argument template at `:616` do not. Report
01 at line 735 reads "`TypedConstant` has an implicit `int32` constructor at
`taichi/ir/type.h:582`."

Pass B's gloss is the accurate one: *implicit* describes the width, never the
conversion. The width is fixed by the argument's declared C++ type at a place
that may be many lines away, and no width token need appear on the line.

**But pass B draws a false conclusion from a true premise**, and this is the
single largest error in either report:

> "The token `TypedConstant(` appears at every construction, so
> `grep -rn "TypedConstant("` bounds the class exactly."

Section 1 above falsifies it three times over. `explicit` blocks silent
*conversions*; it says nothing about *spelling*. Direct-list-initialisation with
braces, named-variable direct-initialisation, and copy-list-initialisation of a
non-explicit multi-argument constructor are all constructions that the token
`TypedConstant(` does not mark. Pass A found one of the three. Nobody found the
other two.

Pass B applied the same reasoning correctly to `Expr`, where I confirmed it holds
— see section 4 — which shows the error is in the inference, not in the method.

---

## 4. Attack 3 — the two disputes pass A raised against pass B

### 4.1 `get_data_type<int32>()`: 4, not 3. Both passes right.

```
grep -rn "get_data_type<int32>()" $T   -> 4
taichi/ir/type.cpp:463
taichi/transforms/scalarize.cpp:63
taichi/transforms/scalarize.cpp:114
taichi/transforms/scalarize.cpp:530
```

Report 01 line 741 states 3 for a command whose printed territory
(`taichi/ir/ taichi/analysis/ taichi/transforms/`) contains `taichi/ir/type.cpp`.
Its prose calls `type.cpp:463` and `:478` "the only other hits tree-wide"; `:478`
is `get_data_type<int64>()` and never matched the command at all. Both passes are
correct and neither loses a site, since `:463` is inventoried in report 01's
section 3.2.

Neither pass ran the unsigned twin. `grep -rn "get_data_type<uint32>()" $T`
returns one line, `taichi/ir/type.cpp:513`, in neither report.

### 4.2 The eleven macro continuations: **pass B is right, pass A is wrong.**

Pass A wrote that they are "two-argument calls whose type argument sits on the
**previous** physical line." Pass B tested all 52 two-argument rows and found the
type argument on the **following** line in eleven cases and on the previous line
in zero. Read at source:

```
taichi/transforms/constant_fold.cpp:79:      res = TypedConstant(                        \
taichi/transforms/constant_fold.cpp:80:          dst_type, PREFIX(lhs->val.val_int() OP_CPP rhs->val.val_int()));  \
```

```
taichi/ir/ir_builder.cpp:142:  return insert(Stmt::make_typed<ConstStmt>(TypedConstant(
taichi/ir/ir_builder.cpp:143:      TypeFactory::get_instance().get_primitive_type(PrimitiveTypeID::i32),
taichi/ir/ir_builder.cpp:144:      value)));
```

The type argument is on the line **after** in both files, in all eleven cases.
Pass B's eleven — `ir_builder.cpp:142`, `:148`, `:154`, `:160`, `:166`, `:172`
and `constant_fold.cpp:79`, `:83`, `:114`, `:117`, `:121` — are the correct set,
six and five, matching the files and counts report 01 gave.

**Pass A reproduced report 01's own error while criticising it.** Report 01 line
751 says "macro continuations whose type argument sits on the previous line";
pass A restated the direction unchanged and presented it as its own finding. Both
reach the right disposition — the eleven are two-argument, explicitly typed, and
not members of the one-argument class — so no site moves. The reasoning that
supports the disposition is wrong in pass A and right in pass B.

### 4.3 A dispute pass A did not raise: `Expr(int)`

Pass B found a spelling pass A missed entirely, and it is a real one.

```
taichi/ir/expr.h:30:   explicit Expr(int32 x);
taichi/ir/expr.cpp:92: Expr::Expr(int32 x) : Expr() {
taichi/ir/expr.cpp:93:   expr = std::make_shared<ConstExpression>(PrimitiveType::i32, x);
```

Report 01 swept `expr.cpp:93` under `PrimitiveType::i32` and excluded it as "one
overload of a pair" — the right call for the constructor, and the same call that
left `type.h:582`'s call sites unswept. Pass B is the only agent to have noticed
that the identical omission had happened twice.

Three sites on two lines, both in `expand_tensor_or_scalar`:

```
taichi/ir/frontend_ir.cpp:1837:  id_expr, ExprGroup(Expr(i)), expr->dbg_info));      // int i at :1835
taichi/ir/frontend_ir.cpp:1846:  id_expr, ExprGroup(Expr(i), Expr(j)), ...));       // int i at :1843, int j at :1844
```

**I applied pass A's own lesson to pass B's new class, and pass B survives it.**

```
grep -rnE '(^|[^A-Za-z0-9_])Expr\{' $T --include=*.h --include=*.cpp   -> 0
grep -rnE '(^|[^A-Za-z0-9_])Expr [a-zA-Z_][a-zA-Z0-9_]*[({]' $T        -> 40, all function
                                                                          return types
```

No brace form and no named-variable construction of `Expr` from an `int` exists
in the territory. Pass B's three sweeps for other width-selecting constructor
sets reproduce exactly at 4, 4 and 3 lines, and its reading of the third — that
`BitStructTypeBuilder(int)`, `Axis(int)` and `ReplaceLocalVarWithStacks(int)`
have no 64-bit sibling and so are not width-selecting overload sets — is correct
at source.

**So on this class, pass B's bound is a bound.** The same claim fails for
`TypedConstant` and holds for `Expr`, and the difference is a fact about those
two headers, not about the method.

---

## 5. Attack 4 — pass A's verification of report 01's section 3.13 table

**It reproduces exactly. It proves less than pass A claimed.**

```
awk 'NR>=650 && NR<=765 && /^\| `taichi\//' report-01-ir-types.md \
  | grep -oE 'taichi/[a-z_/]+\.(cpp|h):[0-9]+' | sort > r13.txt      -> 64 keys
grep -rn "PrimitiveType::i32" taichi/ir/ taichi/analysis/ taichi/transforms/ \
  | cut -d: -f1,2 | sort > actual.txt                                -> 64 keys
diff r13.txt actual.txt                                              -> empty
```

Both files hold 64 keys, `uniq -d` is empty on each, and the diff is empty.
`grep -rn "PrimitiveType::i32" $T` with `taichi/inc/` added also returns 64, so
the two territories agree, as pass A said.

**What that certifies, and what it does not.** It certifies a key-set identity:
the table's rows and the grep's lines are the same 64 `file:line` pairs.

Pass A then wrote: "Report 01's spelling-A enumeration is exactly right, **its
four exclusions are sound**, and I adopt its dispositions unchanged." The diff
cannot test the second and third clauses. This is standing instruction 7 exactly
— a verified citation does not verify the claim attached to it — and pass A wrote
the paragraph one section after quoting that instruction's spirit at report 01.

I spot-checked the four exclusions independently rather than adopting them:
`type_utils.h:40` is the `int32` arm of the `get_data_type<T>()` template
mapping; `expr.cpp:93` is `Expr::Expr(int32)`; `type.h:582` is the
`TypedConstant(int32)` constructor; `type_check.cpp:459` is the `!=` guard for
the cast at `:461`. All four exclusions are sound on my reading.

**But two of them are the very mechanisms whose call sites this territory
exists to enumerate**, and report 01 states no such consequence beside either.
Excluding `type.h:582` as "the mechanism, not a site" is correct and is what
left 73 sites unswept; excluding `expr.cpp:93` on the same ground is correct and
is what left pass B's three unswept. A sound exclusion that silently opens a
class of 76 is worth a sentence, and neither pass A's verification nor report 01
supplies one. Pass A's "I add nothing and I remove nothing" is true of the rows
and misses the thing the rows imply.

---

## 6. Attack 5 — the open `int` surface in `.cpp` files

**Judgement: the crossing surface is already closed. The arithmetic surface is
not closeable by enumeration, and no amount of grep will close it. Both reports
describe the open set correctly and both mis-size what closing it would take.**

Scale of the territory:

| Quantity | Value |
|---|---|
| `.cpp` files in `$T` | 99 |
| lines of `.cpp` in `$T` | 27663 |
| `int`-declaration-shaped lines in those `.cpp` files | 448 |
| `for (int` lines | 182 |
| `auto` declarations in those `.cpp` files | 1853 |

The last row is the whole answer. `auto` is the reason no textual sweep can
close this class, and the proof is a line report 01 already found by hand:

```
taichi/transforms/simplify.cpp:175:  auto sum = Stmt::make<ConstStmt>(TypedConstant(0));
taichi/transforms/simplify.cpp:176:  auto stride_product = 1;
taichi/transforms/simplify.cpp:178:    auto stride_stmt = Stmt::make<ConstStmt>(TypedConstant(stride_product));
taichi/transforms/simplify.cpp:187:    stride_product *= stmt->strides[i];
```

`stride_product` is an `int` that accumulates a product of SNode strides and is
poured into an i32 `ConstStmt` on the next line. **No search for the token `int`
can see it.** There are 1853 such declarations in territory `.cpp` files and no
filter distinguishes the ones that deduce to `int` from the ones that do not.

**What is already closed, which neither report states this way.** A host `int`
can only become an IR 32-bit quantity through a bounded set of crossings, and
every one of them is now enumerated:

| Crossing | Where it is closed |
|---|---|
| one-argument `TypedConstant` | section 1.1, 82 constructions, three syntactic forms |
| one-argument `Expr` | section 4.3, 3 sites, form-checked |
| `TypedConstant` / `Expr` two-argument with a 32-bit type argument | spellings 1, 3, 5 |
| `get_primitive_int_type(<literal>)` | spelling 10, one site |
| `int`-typed members of IR classes | section 7 below |

The single-argument constructors of both types are `explicit`, so no `int` can
cross into either by passing through a parameter. **The crossing set is
therefore complete**, and "the largest open surface" is not the crossings.

**What is open is the host-side arithmetic that happens before the crossing** —
whether `stride_product` overflows before it reaches `TypedConstant`, whether
`flattened_element` at `check_out_of_bound.cpp:54` overflows before it reaches
`:74`. That is a dataflow property of a value, not a property of any text, and
it has no textual signature at all.

**What it would take, concretely.** Three routes, in ascending cost:

1. **Compiler-assisted enumeration, and it is not currently possible in this
   tree.** `compile_commands.json` does not exist at the repo root or under
   `build/`, and only `clang++` is installed — no `clang-query`, no `clang-tidy`.
   A libTooling or clang-query pass over `VarDecl` and `ParmVarDecl` of type
   `int` in the 99 files, with `auto` resolved by the compiler rather than by a
   regex, needs a compilation database first. That is a build step, not an
   investigation step, and nobody has scoped it.
2. **Change the type and let the compiler enumerate.** Widen the fields in
   section 7 to `int64` and rebuild with `-Wconversion -Werror`. Every host-side
   narrowing becomes a diagnostic with a file and a line. This is exact,
   complete, needs no tooling that is not already required to build the project,
   and produces the list as a by-product of the work item rather than ahead of
   it. It is also destructive of the tree, so it belongs behind a branch and
   behind a planner ruling, not in an investigation pass.
3. **Read all 99 files.** 27663 lines. Complete, unfalsifiable in the bad sense,
   and it produces a list nobody can check.

**My judgement: route 2, and the list should be treated as an output of item 6.2
rather than an input to it.** Route 1 is the same work with a build dependency in
front of it. Neither report should be asked to close this class, and pass B's
refusal to estimate its size — "report 01's 78 is what an estimate over an
unclassified set looks like" — is the right instinct, stated well.

One qualification I owe. Route 2 enumerates *narrowings*, which is a superset of
what matters and includes every `int` loop counter that never touches an address.
It closes the class in the sense of producing a finite checkable list. It does
not rank it.

---

## 7. Attack 6 — the 24 sites and the mesh ruling

**Pass A's escalation 5 is wrong on its own terms, and the correct partition is
sharper and more consequential than either report's.**

Pass A wrote: "Of my 46 new spelling-D sites, 12 are in `make_block_local.cpp`,
9 in `make_mesh_block_local.cpp` and 3 in `demote_mesh_statements.cpp`. …
Twenty-four of my 46 turn on the same ruling, and `make_block_local.cpp` is
block-local storage rather than mesh, so the boundary matters."

The sentence contradicts itself: it names `make_block_local.cpp` as not-mesh in
the same breath as counting its twelve into the mesh-dependent 24. And it is
checkable:

```
grep -n "mesh\|Mesh" taichi/transforms/make_block_local.cpp   -> 0
```

Zero mesh references. `make_block_local_offload` at
`taichi/transforms/make_block_local.cpp:12-17` returns immediately unless
`offload->task_type == OffloadedStmt::TaskType::struct_for`. **The mesh ruling
does not reach a single one of the twelve.**

**But those twelve are not therefore base work, and this is the finding neither
report has.** Traced through the pass pipeline:

```
taichi/codegen/llvm/codegen_llvm.cpp:2725-2727:
    /*make_block_local=*/
    is_extension_supported(config.arch, Extension::bls) &&
        config.make_block_local);
```

```
taichi/program/extension.cpp:18-21:
      {Arch::cuda,
       {Extension::sparse, Extension::quant, Extension::quant_basic,
        Extension::data64, Extension::adstack, Extension::bls,
        Extension::assertion, Extension::mesh}},
```

`Extension::bls` is granted to **CUDA and to nothing else**. The only other
grep hit for it in the whole tree is `codegen_llvm.cpp:2726` itself. And on the
SPIR-V spine the flag is never passed: `taichi/codegen/spirv/kernel_compiler.cpp:17-21`
calls `irpass::compile_to_executable` without it, and the default at
`taichi/ir/transforms.h:196` is `bool make_block_local = false`.

**So `make_block_local.cpp` runs on CUDA alone.** Under section 2.2a CUDA is a
vendor backend, and vendor-specific work is BRANCH, parked. The largest single
group of new sites in both reports — 12 of pass A's 46, 12 of pass B's 40, the
group pass B called "the single largest group in section 4" — is out of base
scope for a reason that is arch gating, not mesh gating, and that neither report
looked for.

### 7.1 The partition, stated so the ruling can be applied mechanically

Every gate below is a source line. Nothing here assumes a ruling; it states which
gate each site sits behind so that whatever the planner rules can be applied
without re-reading the files.

**Group 1 — no arch gate. 22 of pass A's 46.**

| File | Sites | Gate |
|---|---|---|
| `taichi/ir/frontend_ir.cpp` | `:663` | frontend, unconditional |
| `taichi/transforms/utils.cpp` | `:7`, `:10`, `:20` | helper; callers in `scalar_pointer_lowerer.cpp` and `demote_dense_struct_fors.cpp` |
| `taichi/transforms/check_out_of_bound.cpp` | `:47`, `:74`, `:106`, `:142`, `:179`, `:186` | `config.check_out_of_bound`, `compile_to_offloads.cpp:123`; default false at `compile_config.cpp:24`, forced true with debug at `:70` |
| `taichi/transforms/handle_external_ptr_boundary.cpp` | `:30`, `:39`, `:71`, `:78` | unconditional, `compile_to_offloads.cpp:89` |
| `taichi/transforms/simplify.cpp` | `:175`, `:178` | unconditional |
| `taichi/transforms/demote_dense_struct_fors.cpp` | `:49`, `:79` | `config.demote_dense_struct_fors`, `compile_to_offloads.cpp:191`; default true at `compile_config.cpp:18` |
| `taichi/transforms/demote_operations.cpp` | `:134` | unconditional, `compile_to_offloads.cpp:275`, `:394` |
| `taichi/transforms/lower_ast.cpp` | `:241` | unconditional |

1 + 3 + 6 + 4 + 2 + 2 + 1 + 1 = 20. Plus:

**Group 1a — LLVM spine only, never SPIR-V. 2 sites.**

`taichi/transforms/bit_loop_vectorize.cpp:76`, `:188`. Gate at
`taichi/transforms/compile_to_offloads.cpp:63-64`:
`if (arch_is_cpu(config.arch) || config.arch == Arch::cuda || config.arch == Arch::amdgpu)`.
Reaches CPU-only operation, so **BASE under 2.2a**, but a width change here is
invisible on the portable GPU path and the report should say so.

20 + 2 = **22.**

**Group 2 — `Extension::mesh`. 3 sites. Reaches CPU-only, so BASE under 2.2a.**

`taichi/transforms/demote_mesh_statements.cpp:86`, `:94`, `:123`. Gate at
`compile_to_offloads.cpp:236`,
`if (is_extension_supported(config.arch, Extension::mesh))`. Per
`taichi/program/extension.cpp:9-27`, `Extension::mesh` is held by `x64`, `arm64`
and `cuda` — **including CPU-only operation, which section 2 puts first**, and by
no SPIR-V target at all.

All three are brace-form sites. Pass B has none of them.

**Group 3 — `Extension::mesh` AND `config.arch == Arch::cuda`. 9 sites. Vendor.**

`taichi/transforms/make_mesh_block_local.cpp:83`, `:116`, `:166`, `:169`, `:204`,
`:268`, `:304`, `:327`, `:578`. Gate at `compile_to_offloads.cpp:221`,
`if (config.make_mesh_block_local && config.arch == Arch::cuda)`. Pass B has
eight of the nine; it lacks `:169`, the brace form.

**Group 4 — `Extension::bls`, i.e. CUDA alone. 12 sites. Vendor.**

`taichi/transforms/make_block_local.cpp:145`, `:152`, `:165`, `:183`, `:194`,
`:202`, `:277`, `:285`, `:298`, `:303`, `:315`, `:328`. Gate at
`codegen_llvm.cpp:2726` and `extension.cpp:20`. Not mesh.

22 + 3 + 9 + 12 = **46**, derived from the four lists, not asserted over them.

**Also governed by the mesh gate, outside spelling D:**
`taichi/transforms/make_mesh_thread_local.cpp:26`, pass A's one new
`PrimitiveType::u32` site. Gate at `compile_to_offloads.cpp:218-219`, the same
`Extension::mesh` test as group 2, so **BASE**.

### 7.2 What this does to the two headline numbers

Under section 2.2a as written today, and without the planner making any new
ruling:

| Class | Pass A's 46 | Pass B's 40 |
|---|---|---|
| base work, no arch gate | 22 | 22 |
| base work, mesh gate, reaches CPU | 3 | 0 |
| vendor, CUDA only | 21 | 18 |

22 + 3 + 21 = 46. 22 + 0 + 18 = 40.

**Roughly 45 per cent of pass A's new sites and 45 per cent of pass B's are CUDA
only and park under 2.2a as branch work.** Pass B's inferred reading in its
section 8 item 3 — that the twenty block-local sites are bounded by
`default_shared_mem_size = 65536` and are cheap to leave alone — reaches the same
practical answer by a route it correctly labelled unverified. The arch gate gets
there without needing that bound to hold.

Neither report should be asked to re-rank on this. It is a scope fact and it is
the planner's to apply.

---

## 8. Two things both passes missed

### 8.1 Header declarations behind a `static` keyword

Pass A's spelling-G regex anchors on `^[[:space:]]*` followed immediately by the
type, and covers only `int`, `std::vector<int>` and `std::array<int,N>`. It
returns 102, which reproduces. Pass B's three sweeps return 109 + 2 + 1 = 112,
which also reproduces exactly. **Pass B's is the better enumeration**: it catches
`int32`-spelled declarations, arbitrary containers on `int`, and the one
declaration in the territory whose type and name are on different lines
(`taichi/ir/mesh.h:79-80`). Pass B also found its own leak by opening a line
rather than trusting the pattern, and recorded the leak instead of the corrected
regex, which is the behaviour standing instruction 9 asks for.

Concretely, pass A's 102 misses at least six that pass B's 112 holds:
`taichi/ir/mesh.h:78`, `taichi/ir/mesh.h:79-80`, `taichi/ir/statements.h:757`,
`taichi/ir/type.h:558`, and the two anchoring cases below.

**Both regexes require the type token to be first after the leading whitespace,
so both miss every `static` declaration:**

```
grep -rnE '^[[:space:]]+static[[:space:]]+[^;]*\bint\b[^;]*;' \
  taichi/ir/*.h taichi/analysis/*.h taichi/transforms/*.h taichi/inc/*.h

taichi/ir/ir.h:340:            static int get_snode_id(SNode *snode);
taichi/ir/ir.h:400:            static std::atomic<int> instance_id_counter;
taichi/ir/snode.h:88:          static std::atomic<int> counter;
taichi/ir/type_factory.h:66:   static DataType create_tensor_type(std::vector<int> shape, DataType element);
```

Two of the four are load-bearing:

- **`taichi/ir/snode.h:88`, `static std::atomic<int> counter`.** This is the
  process-global SNode id counter. Pass A rules `snode.h:89` `int id{0}` ADDR and
  calls it "the quantity `taichi_max_num_snodes` bounds"; pass B rules the same
  field not width-bearing because it is an id. **Neither has the counter that
  generates it.** The plan's own section 6.1 correction turns on this exact
  quantity: "those arrays are indexed by `SNode::id`, a process-global counter
  that is never recycled." The declaration of that counter is `int`, it is in
  territory 06, and it is in no report.
- **`taichi/ir/type_factory.h:66`, `std::vector<int> shape`.** The tensor-shape
  parameter of `TypeFactory::create_tensor_type`. Both passes carry
  `taichi/ir/type.h:250` `std::vector<int> shape_`, the field; neither carries the
  factory entry point that fills it.

I am not proposing a corrected count for spelling 13. Pass B's 112 plus these
four is 116, and I have no confidence that a fifth anchoring case does not exist.
**The right statement is the one neither report made: this class is bounded by
the shape of the regex used to find it, and every published figure for it is a
count of a regex.** Pass B came closest to saying so and then published 112 as a
count of a class.

### 8.2 The ADDR / NONADDR split is judgement and the two passes differ on it

Pass A: 47 ADDR, 46 NONADDR, 7 LOCAL, 2 PARAM over 102.
Pass B: 42 width-bearing, 70 not, over 112.

These are not reconcilable by arithmetic and should not be. The two disagree in
kind, not only in count: pass A files the five `scratch_pad.h` inline-method
locals (`:97`, `:133`, `:142`, `:150`, `:232`) as LOCAL and outside the class;
pass B files four of them inside its 42 as already carried by report 01 and the
fifth, `:232`, as new-and-qualified. Both dispositions are defensible. Pass A
flagged the split as "the softest thing in this report" and it was right to.

Pass B's treatment of `scratch_pad.h:232` deserves a note in its favour. It
recorded a declaration inside a function with no callers
(`grep -rn "generate_address_code" taichi/` returns only the definition at
`scratch_pad.h:229`), labelled it dead on two counts, and kept it anyway under
standing instruction 3 rather than tidying it away. That is the instruction
applied correctly against the agent's own interest.

---

## 9. Where the two disagree on report 01's coverage, settled

Both passes ran a script over report 01's text. They disagree on six sites.

| Site | Pass A | Pass B | Settled |
|---|---|---|---|
| `simplify.cpp:175` | absent from report 01 | inside a cited range | **Pass B.** Report 01 line 910 cites `simplify.cpp:160-220` |
| `simplify.cpp:178` | absent | inside a cited range | **Pass B**, same range |
| `demote_mesh_statements.cpp:86` | new | not found | **Pass A.** Brace form, invisible to B |
| `demote_mesh_statements.cpp:94` | new | not found | **Pass A** |
| `demote_mesh_statements.cpp:123` | new | not found | **Pass A** |
| `make_mesh_block_local.cpp:169` | new | not found | **Pass A** |

Report 01 has no inventory row for `simplify.cpp:175` or `:178` either way, so
the substantive answer — a reader working from report 01 will not find these
sites — is the same under both classifications. Pass A's tier rule and pass A's
placement of these two disagree with each other, since it filed
`simplify.cpp:193` in tier B through the adjacent `:189-205` citation at report
01 line 101 while filing `:175` and `:178` in tier C.

**The correct figure for one-argument constructor-class sites bearing i32 and
carrying no inventory row in report 01 is 44**, being pass B's 40 plus the four
brace-form sites. Pass A's 46 counts `simplify.cpp:175` and `:178` as absent when
they fall inside a cited range; pass B's 40 is short by the four. Neither
published figure is right and the difference between them is entirely accounted
for.

**A third site both passes handled inconsistently.** On spelling 3,
`PrimitiveTypeID::i32`, pass A names two new sites and pass B names two, and they
are not the same two:

| Site | Pass A | Pass B | Report 01 |
|---|---|---|---|
| `taichi/ir/type.cpp:238` | real, not flagged new | **new** | `grep -n "type.cpp:238\|QuantIntType"` returns nothing in either half |
| `taichi/transforms/frontend_type_check.cpp:72` | **new** | filed as already cited | cited as a range at report 01 line 1026, `:71-77`, no inventory row |
| `taichi/transforms/simplify.cpp:234` | **new** | **new** | absent |

**All three are new under a "no inventory row" rule. Neither pass has all three.**
Add `taichi/ir/ir_builder.cpp:155` from spelling 4 in section 1.3 and the figure
is four, in no report.

---

## 10. Scorecard

Neither pass is a better report than the other. They fail in different
directions and the union is closer to complete than either.

**Pass A is right and pass B is wrong on:** the brace form and its four uncast
sites; the 73-site figure for the one-argument class; `type_system.cpp` in its
entirety, which is the most consequential single finding in this territory and
which pass B does not contain.

**Pass B is right and pass A is wrong on:** the direction of the eleven macro
continuations; the `Expr(int)` spelling, which pass A does not contain; the
header-declaration enumeration, where 112 beats 102 and the leak was found by
opening a line rather than trusting a pattern; `simplify.cpp:175` and `:178`
falling inside a cited range; `type.cpp:238` as new.

**Both are wrong on:** `PrimitiveTypeID::u32`, 17 lines, never swept, carrying
`ir_builder.cpp:155`; the three named-variable constructions and the 24 tokenless
ones; the four `static`-prefixed header declarations including the SNode id
counter at `snode.h:88` and the tensor-shape factory parameter at
`type_factory.h:66`; and the arch gating in section 7, which parks about 45 per
cent of both reports' new sites as vendor work.

**Both published a headline count over an undeclared membership rule**, which is
why eight and six are both defensible and neither is the answer.

---

## 11. Escalations

1. **The constructor class has four syntactic forms, not one.** Any future sweep
   of it must state which forms it can see. Paren, brace, named-variable, and
   the tokenless braced return at `type_utils.h:147-198`. The last cannot carry a
   one-argument width because the single-argument constructors are `explicit`,
   and that is a result, not an assumption.
2. **`Extension::bls` is CUDA-only and gates `make_block_local.cpp` entirely.**
   Twelve of pass A's 46 and twelve of pass B's 40 park as branch work under
   2.2a. Together with `make_mesh_block_local.cpp`'s nine, that is 21 of 46 and
   18 of 40. This is a scope fact with source lines
   (`codegen_llvm.cpp:2726`, `extension.cpp:20`, `transforms.h:196`,
   `spirv/kernel_compiler.cpp:17-21`) and it is the planner's to apply.
3. **`Extension::mesh` reaches `x64` and `arm64`, so mesh is not a GPU question.**
   `extension.cpp:9-27`. CPU-only operation is a first-class target under section
   2, and the mesh gate does not exclude it. The mesh ruling that report 01
   escalated therefore bears on base work, and `demote_mesh_statements.cpp`'s
   three sites plus `make_mesh_thread_local.cpp:26` sit behind that gate alone.
   Mesh reaches no SPIR-V target at all, which is a gap in the less-served spine
   to record under section 5.2, not evidence that it matters less.
4. **`taichi/ir/snode.h:88` `static std::atomic<int> counter` is in no report.**
   It is the process-global, never-recycled SNode id counter the plan's own
   section 6.1 correction names, its type is `int`, and it is in territory 06.
   Both header sweeps miss it because both anchor on the type token.
5. **`taichi/ir/type_factory.h:66` is in no report.** `std::vector<int> shape` on
   `TypeFactory::create_tensor_type`, the entry point that fills
   `type.h:250 shape_`, which both reports carry.
6. **`PrimitiveTypeID::u32` was never swept.** Seventeen lines. Two fix a 32-bit
   width, `type.cpp:238` and `ir_builder.cpp:155`, and `type.cpp:513` is an abort
   site symmetric to `type.cpp:463`.
7. **Report 01 needs three corrections, none of which moves a site.** "Implicit"
   at line 735 is wrong as C++; the `get_data_type<int32>()` row at line 741
   states 3 for a command returning 4; the continuation direction at line 751 is
   backwards. The third was repeated by pass A while criticising the first.
8. **The `.cpp` `int` surface should be treated as an output of item 6.2, not an
   input.** Section 6 gives the reasoning and the measurements. It cannot be
   closed textually because 1853 `auto` declarations in the territory's `.cpp`
   files can deduce to `int` and no filter distinguishes them; the proof is
   `simplify.cpp:176`, which report 01 found by hand and no sweep can find. No
   compilation database exists in this tree, so the clang-tooling route needs a
   build step first. Neither report should be asked to close it.
9. **My own count moved once during this pass.** I first read pass A's arity
   split as wrong by one and pass A's 80 as consequently wrong. It is not: 86−13
   and 85−12 are both 73, so pass A's miscast of the zero-argument default at
   `type.h:572` cancels in its published figure. Recorded rather than tidied
   away, per section 10 item 9.
10. **Two judgement calls I did not make.** Whether inline-method locals in
    headers belong to the declaration class — pass A says no, pass B says yes,
    and they differ on five `scratch_pad.h` rows because of it. And the ADDR /
    NONADDR split itself, where 47/46 against 42/70 is a difference in kind. Pass
    A named the second as the softest thing in its report and I agree; a
    defensible count needs a stated rule for what "address-bearing" means, and
    no such rule exists in the plan.

---

## 12. Divergence from adversary 06-2

`modernization/investigation/adversary-06-2.md` did not exist when I finished
sections 1 to 11; it landed while I was writing and I read it in full before
writing this section. Nothing above was altered. Every correction to my own work
is recorded here rather than edited into the text, so that what I got wrong stays
visible.

### 12.1 Where we converge, independently

Both files were written without sight of the other and reach the same answer on
the four questions the brief called principal.

| Question | Adversary 06-2 | This file |
|---|---|---|
| Is the brace form real? | yes, 7 lines | yes, 7 lines |
| Does pass B's bound hold? | no | no |
| One-argument constructions | 80 | 80 |
| Of those, fixing i32 | 73 | 73 |
| Absent from report 01 entirely | **44** | **44** |
| Pass A right on the four uncast brace sites | yes | yes |
| Pass B right on `simplify.cpp:175`, `:178` | yes | yes |
| Pass B right on the continuation direction | yes | yes |
| Pass A's 3.13 diff reproduces but overreaches | yes | yes |
| `type.h:582` is `explicit` | yes | yes |
| `get_data_type<int32>()` is 4 | yes | yes |

The 44 is worth weighing as a convergence rather than as an agreement. We
reached it by opposite subtractions — adversary 06-2 took pass A's 46 and removed
the two `simplify.cpp` rows; I took pass B's 40 and added the four brace rows —
and the sets are identical because `comm` shows pass B's 40 is a strict subset of
pass A's 46 whose complement is exactly those six lines. Two arithmetics, one
set.

**One convergence the planner should not read as confirmation.** We both put the
route count at thirteen, and the two lists are not the same thirteen. Adversary
06-2 counts `TypedConstant(` and `TypedConstant{` as two routes and
`IRBuilder::get_int32` as a thirteenth; I fold the `TypedConstant` forms into one
spelling with three syntactic forms and count `PrimitiveTypeID::u32` and the
`(int32)` cast as separate routes. Eleven routes are common to both lists. That
two independent enumerations under different rules both landed on thirteen is a
coincidence, and treating it as agreement would be exactly the error section
1.3 warns about — a count is only as meaningful as the membership rule behind it,
and neither of us can borrow the other's total.

### 12.2 Where adversary 06-2 corrects me, and it is right

**`vkGlobalThreadIdx` is a dead declaration. My section 2 table is wrong on that
row.** I wrote that `type_system.cpp:370` is a base-work 32-bit cap on the SPIR-V
spine. Adversary 06-2 checked the consumer and I did not. Re-run here:

```
grep -rn "vkGlobalThreadIdx" taichi/
  taichi/ir/type_system.cpp:358      (a comment)
  taichi/ir/type_system.cpp:370      PLAIN_OP(vkGlobalThreadIdx, i32, false);
  taichi/inc/internal_ops.inc.h:25   PER_INTERNAL_OP(vkGlobalThreadIdx)

grep -rn "globalInvocationId" taichi/
  taichi/codegen/spirv/spirv_codegen.cpp:1415
```

There is no SPIR-V codegen arm for `vkGlobalThreadIdx`. Adversary 06-2 is right,
and my row for `:370` should read *declared, unreachable* rather than *BASE*.

**The finding is stronger than adversary 06-2 states, and the extra half is
mine.** It reports the mismatch as one-sided — a declaration with no arm, and an
arm dispatching on a name absent from the op table. The arm is dead too.
`InternalFuncStmt` has exactly one producer in the tree:

```
grep -rn "InternalFuncStmt>(\|InternalFuncStmt(" taichi/
  taichi/ir/statements.h:1676   (the constructor)
  taichi/ir/type_system.cpp:273 (the only construction)
```

`type_system.cpp:273` passes `internal_call_name_`, which `PLAIN_OP` fixes to the
stringised op name from `taichi/inc/internal_ops.inc.h`. `globalInvocationId` is
not an entry there, so no `InternalFuncStmt` can ever carry that name and
`spirv_codegen.cpp:1415-1416` cannot execute. **Both halves are dead, and the
32-bit global thread index on the SPIR-V spine is not capped anywhere, because it
is not reachable anywhere.**

What survives on that spine is `localInvocationId`, live at all three points —
`type_system.cpp:369`, `internal_ops.inc.h:24`, `spirv_codegen.cpp:1413` — and
capped i32 twice over, once in the signature table and once at
`spirv_codegen.cpp:1414`. Adversary 06-2 says this and I did not.

So of pass A's four thread-and-lane index rows, the correct disposition is: `:307`
`linear_thread_idx` live on the LLVM spine, `:369` `localInvocationId` live on the
SPIR-V spine, `:370` `vkGlobalThreadIdx` dead, `:376` `subgroupInvocationId` live
at `spirv_codegen.cpp:1447`. **Pass A's "on both spines" claim holds, but through
`:369`, not through the `:370` it named.** The claim is right and its citation is
wrong, which is standing instruction 7 running the other way.

### 12.3 Where I correct adversary 06-2, and the source supports me

**`make_block_local.cpp` runs on CUDA alone. Adversary 06-2's section 7 group 1
and its escalation 7 are wrong.**

Adversary 06-2's table row reads: `make_block_local` gated at
`compile_to_offloads.cpp:231` by "the `make_block_local` bool. **No mesh gate, no
arch gate**", and its escalation 7 concludes the twelve sites "are base work under
plan section 2.2a and section 4.1's sparsity requirement" and warns the planner
against parking them.

It stopped at the bool and did not trace it to its caller. The bool is a
parameter, and `irpass::make_block_local` has exactly one call site in the tree:

```
grep -rn "make_block_local" taichi/ --include=*.cpp --include=*.h
```

`compile_to_offloads.cpp:232` is the only call, guarded at `:231` by the
parameter declared at `:161`. That parameter is supplied from exactly two places:

```
taichi/codegen/llvm/codegen_llvm.cpp:2725-2727:
    /*make_block_local=*/
    is_extension_supported(config.arch, Extension::bls) &&
        config.make_block_local);

taichi/ir/transforms.h:196:  bool make_block_local = false,
```

The second is the default taken by the SPIR-V spine, which calls
`irpass::compile_to_executable` at `taichi/codegen/spirv/kernel_compiler.cpp:17-21`
without passing it. The first is the only place it can be true, and

```
taichi/program/extension.cpp:18-21:
      {Arch::cuda, {…, Extension::bls, …}},
```

grants `Extension::bls` to CUDA and to no other architecture. The only other
mention of the token in the tree is `codegen_llvm.cpp:2726` itself. The DirectX 12
path at `taichi/codegen/dx12/codegen_dx12.cpp:240` calls `compile_to_offloads`,
which does not reach `offload_to_executable` at all.

**So the twelve `make_block_local.cpp` sites are CUDA-only, which section 2.2a
puts in the BRANCH column.** Adversary 06-2's warning is inverted: it tells the
planner not to park twelve in-scope sites on a ruling that does not govern them,
when the correct statement is that a different gate does govern them and parks
them. Its sparsity argument from plan section 4.1 is sound about struct-for in
general and does not reach this pass, because this pass never runs off CUDA.

The consequence for its headline: adversary 06-2's group 1, 2, 3 split of 12 / 3 /
9 is right as a description of the mesh gate and wrong about what group 1 is. The
partition that survives is the one in my section 7.1 — 22 base, 3 mesh-gated but
reaching CPU-only, 21 CUDA-only.

**Adversary 06-2's "76 crossings" is short, and the shortfall is a silent
narrowing.** Its section 6.2 argues that the class of a host `int` reaching an IR
32-bit width is already closed at 76 crossings — 73 one-argument `TypedConstant`
plus 3 `Expr` — because both width-selecting constructors are `explicit`. I made
the same argument in my section 6 and we were both incomplete. The two-argument
template is not `explicit` and it narrows without a cast:

```
taichi/ir/type.h:616:  template <typename T>
             :617:  TypedConstant(DataType dt, const T &value) : dt(dt) {
             :622:    } else if (dt->is_primitive(PrimitiveTypeID::i32)) {
             :623:      val_i32 = value;
```

`val_i32` is `int32`. `value` is whatever the call site passes. A census of the
type argument across all 52 two-argument calls in territory:

| Type argument | Calls |
|---|---|
| names a 32-bit type outright | 14 |
| names a fixed non-32-bit type | 4 |
| **a runtime `DataType` from an operand** | **34** |

14 + 4 + 34 = 52, plus the named-variable construction at
`taichi/ir/statements.h:2117`, giving **35** calls where a host value of unstated
width is poured into whatever the runtime type says. When that type is i32,
`type.h:623` narrows with no cast and no diagnostic.

This is not hypothetical. The `constant_fold.cpp` rows pass
`lhs->val.val_int() OP rhs->val.val_int()`, and `taichi/ir/type.h:673` declares
`int64 val_int() const;`. **An `int64` is assigned to an `int32` at
`type.h:623` whenever the folded type is i32**, on a path both reports classify as
"width inherited, the line decides nothing." The line decides nothing about the
*type*. It narrows the *value*.

So the crossing surface is 76 sites where a width is *fixed* plus 35 where a
value is *narrowed*, and neither report nor either adversary file separates the
two. My section 6's claim that "the crossing set is complete" is true of the first
and not of the second, and I withdraw the unqualified form of it.

### 12.4 In this file and not in adversary 06-2

Checked by search against its text; each returns zero.

1. **`PrimitiveTypeID::u32`, 17 lines, never swept by anybody.** Section 1.3.
   `grep -c "PrimitiveTypeID::u32" adversary-06-2.md` → 0. It carries
   `taichi/ir/ir_builder.cpp:154-155`, the body of `IRBuilder::get_uint32`, whose
   type argument is `get_primitive_type(PrimitiveTypeID::u32)`. Pass A inventoried
   the signed twin at `:142-143` and filed `:154` among five calls "naming a
   non-i32 fixed type", which is wrong — u32 is 32 bits. My census of the 52
   two-argument type arguments gives 14 naming a 32-bit type where pass A gives 13,
   and this is the missing one. It also carries `taichi/ir/type.cpp:513`, the
   unsigned twin of the `type.cpp:463` abort site both reports hold.
2. **Two further syntactic forms of construction.** Section 1.
   `TypedConstant name(args)` at `statements.h:2117`, `constant_fold.cpp:53` and
   `:189`; and the tokenless `return {dt, v};` at `type_utils.h:147-198`, 24
   occurrences reaching the one non-`explicit` constructor. Adversary 06-2
   reproduces pass A's follow-character census and stops there, so it holds the
   brace form and neither of these. Neither carries a lost i32 site, and the
   reason the tokenless form cannot — every single-argument constructor is
   `explicit`, so `return {5};` does not compile — is a result rather than an
   assumption, and it is the thing that makes the class closable.
3. **Four header declarations behind a `static` keyword, invisible to both
   passes' regexes and to adversary 06-2.** Section 8.1. Two matter:
   `taichi/ir/snode.h:88` `static std::atomic<int> counter`, the process-global
   never-recycled SNode id counter that the plan's own section 6.1 correction
   turns on; and `taichi/ir/type_factory.h:66`
   `static DataType create_tensor_type(std::vector<int> shape, DataType element)`,
   the entry point that fills the `type.h:250` tensor shape both reports carry.
4. **The `Extension::bls` gate**, section 12.3 above.
   `grep -c "Extension::bls" adversary-06-2.md` → 0.
5. **`bit_loop_vectorize.cpp` is LLVM-spine only.** Section 7.1 group 1a.
   `compile_to_offloads.cpp:63-64` gates it on
   `arch_is_cpu(config.arch) || cuda || amdgpu`. Two of the 46 new sites are
   invisible on the portable GPU path. Base work under 2.2a because it reaches
   CPU-only operation, but the asymmetry belongs in the record under section 5.2.
6. **`Extension::mesh` reaches `x64` and `arm64`.** Section 7.1 group 2 and
   escalation 3. Adversary 06-2 gets the mesh gates right file by file but does not
   state that the mesh extension covers CPU-only operation and no SPIR-V target,
   which is what makes its group 2 base work and makes mesh a gap in the
   less-served spine rather than a GPU question.

### 12.5 In adversary 06-2 and not in this file

Recorded because the planner needs both, and because two of these change my text.

1. **The `vkGlobalThreadIdx` dead declaration**, section 12.2. It corrects my
   section 2 and I accept it.
2. **`make_mesh_block_local.cpp:166` and `:327` are unreachable.** Both sit in
   `if (config_.arch == Arch::x64 || config_.arch == Arch::arm64)` arms at `:165`
   and `:326`, inside a pass whose only caller requires `arch == Arch::cuda` at
   `compile_to_offloads.cpp:221`. I verified both branches and the gate and did not
   notice the contradiction. Two of my section 7.1 group 3 rows are dead code
   today. Recorded, not removed, and adversary 06-2 is right that the rule for
   unreachable code should be given once rather than invented three times — it
   would also govern `scratch_pad.h:229-232` and the uncalled
   `taichi::lang::value<T>` template at `expr.h:129-132`.
3. **Report 01 asserts "implicit `int32` constructor" at four places, not one.**
   I cited only `report-01-ir-types.md:735`; the others are `:76-77`, `:1592` and
   `:1618`. I checked all four and adversary 06-2's list is exact. Its restraint is
   also right: report 01's other uses — "the implicit form", "the implicitly-typed
   class" — describe the width and are correct English about a real thing. Only the
   four that call the *constructor* implicit are wrong as C++.
4. **`scalarize.cpp:1113` and `:1140` are inside a block comment.** Adversary 06-2
   read `:1104-1116` and confirms pass B's exclusion. I took pass B's word for it.
5. **The 94 cited line ranges in report 01, extracted and tested against all 46
   new sites.** A stronger version of the check I ran by hand for `simplify.cpp`
   alone, and it reaches the same answer: two of the 46 fall inside a range and
   none is exactly named.

### 12.6 Numeric differences that are method, not fact

Stated so nobody reconciles them as contradictions.

| Quantity | Adversary 06-2 | This file | Why |
|---|---|---|---|
| `.cpp` `int` declaration candidates | 447 | 448 | its sweep omits `taichi/inc/`; different regexes |
| `auto` declarations in `.cpp` | 1488 | 1853 | its pattern is `auto <name> =`; mine also counts `auto &`, `auto *` and range-for heads |
| header `int` declarations | 102, pass A's regex reproduced | 112, pass B's three sweeps reproduced, plus 4 `static` misses | it reproduced only pass A's; both regexes anchor on the type token |

Neither `auto` figure changes the argument, and both of us rest it on the same
line: `taichi/transforms/simplify.cpp:176`, `auto stride_product = 1;`, an `int`
local that no `int` sweep can see and that report 01 found by hand.

### 12.7 Net position

Where we disagree, the source supports me on the `make_block_local` arch gate and
supports adversary 06-2 on `vkGlobalThreadIdx`. Both are corrections to a headline
rather than to a count: the first moves twelve sites out of base scope, the second
moves pass A's most-cited citation off the line it names without disturbing the
finding it supports. Neither moves the figure of 44, which is now derived four
times by three routes and is the one number in this territory I would hand the
planner without qualification.

Both adversary files should be read together, and neither should be read alone.
Between them they hold five things absent from both reports: the
`PrimitiveTypeID::u32` sweep, the two extra construction forms, the four `static`
header declarations, the `Extension::bls` gate, and the two-argument narrowing at
`type.h:623`. The last of those is in neither report and in neither adversary
file until this section, and of the five it is the one I would look at first,
because it is a silent `int64`-to-`int32` assignment on the constant-folding path.
