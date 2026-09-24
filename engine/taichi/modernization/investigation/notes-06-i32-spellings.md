# Notes — agent 06A — spellings by which a 32-bit width enters the IR

Territory: `taichi/ir/`, `taichi/analysis/`, `taichi/transforms/`, `taichi/inc/`.

Contemporaneous. Numbered. Written as I go, not reconstructed.

Shorthand used throughout for the territory, so every command in these notes is
reproducible verbatim:

```
T="taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/"
```

All commands are run from `/opt/project/taichi`.

---

## N01 — Starting point, and the one correction I make to it before starting

Read `notes-01-ir-types.md` N055 to N062 and `report-01-ir-types.md` section
3.13. The mechanism as those state it: `TypedConstant` has an "implicit `int32`
constructor" at `taichi/ir/type.h:582`, so an i32 constant can be built with no
`i32` token on the line; report 01 closed three spellings and left a fourth at
78 candidate lines in 16 files, unclassified.

**Correction to the mechanism, verified at source before I build anything on
it.** `taichi/ir/type.h:582` reads:

```
582:  explicit TypedConstant(int32 x) : dt(PrimitiveType::i32), val_i32(x) {
```

It is `explicit`. So it is *not* an implicit conversion and never fires on a
plain assignment or a function argument. What it does do — and this is the
property that matters and that report 01 is right about — is make
`TypedConstant(i)` for a host `int i` select the i32 overload by ordinary
overload resolution, pinning `dt = PrimitiveType::i32` with no `i32` token
anywhere on the line. Every other arithmetic overload at `:585-611` is likewise
`explicit`. The two-argument form at `:614` (`TypedConstant(DataType, const T&)`)
is the explicitly-typed one and is NOT explicit-marked.

The distinction changes nothing about the size of the class. I record it because
"implicit constructor" is load-bearing wording in report 01 and it is wrong as
C++, and because it tells me the class is exactly *direct construction with one
integer argument*, which is a filter I can define precisely rather than
approximate.

## N02 — First pass: what tokens exist at all in the territory

```
T="taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/"
for p in "PrimitiveType::i32" "PrimitiveTypeID::i32" "get_data_type<int32>" \
         "get_data_type<int>" "TypedConstant(" "int32" "i32" \
         "get_primitive_int_type" "get_primitive_type"; do
  printf "%-28s %s\n" "$p" "$(grep -rn -- "$p" $T | wc -l)"
done
```

| Token | Lines |
|---|---|
| `PrimitiveType::i32` | 64 |
| `PrimitiveTypeID::i32` | 27 |
| `get_data_type<int32>` | 4 |
| `get_data_type<int>` | 0 |
| `TypedConstant(` | 139 |
| `int32` | 87 |
| `i32` | 321 |
| `get_primitive_int_type` | 5 |
| `get_primitive_type` | 20 |

Two things fall out immediately.

1. `PrimitiveType::i32` returns 64 over the four directories, the same figure
   report 01 got over three. So `taichi/inc/` contributes zero to that spelling.
   Report 01's territory excluded `taichi/inc/`; mine includes it.
2. `get_data_type<int32>` returns 4 over my territory against report 01's 3.
   There is a fourth line report 01's sweep could not see. Chase it in N03.
3. `PrimitiveTypeID::i32` (27) is a spelling report 01 never ran at all. It is
   not the same set as `PrimitiveType::i32`.

## N03 — Spelling B: `PrimitiveTypeID::i32`. Report 01 never ran this one.

```
grep -rn "PrimitiveTypeID::i32" $T
```

27 lines. This is a distinct spelling from `PrimitiveType::i32`: the former is
the enumerator, the latter the `DataType` singleton. `grep "PrimitiveType::i32"`
does **not** match `PrimitiveTypeID::i32` (the `ID` sits between), which I
confirmed by checking that the two sets are disjoint:

```
comm -12 <(grep -rn "PrimitiveType::i32" $T | sort) \
         <(grep -rn "PrimitiveTypeID::i32" $T | sort) | wc -l   -> 0
```

Most of the 27 are `dt->is_primitive(PrimitiveTypeID::i32)` predicate tests, not
construction. Three are construction or default-selection and are the interesting
ones:

- `taichi/ir/type_factory.cpp:167` — the `bits == 32` arm of
  `TypeFactory::get_primitive_int_type(int bits, bool is_signed)`. A numeric
  width selector.
- `taichi/ir/type.cpp:238` — `QuantIntType` ctor: when `compute_type == nullptr`
  the compute type **defaults to i32/u32**. A typed helper defaulting to 32 bits,
  which is exactly one of the forms the brief asks for.
- `taichi/ir/ir_builder.cpp:143` — the body of `IRBuilder::get_int32`.

Two are predicate tests that gate behaviour on i32-ness and are already carried
by report 01 through their sibling lines (`type_check.cpp:148` is the guard for
the cast at `:154`; `type_check.cpp:161` is the `MatrixPtrStmt` offset assert
report 01 cites as `:160-161`).

One is a predicate test in a file report 01 never lists:
`taichi/transforms/frontend_type_check.cpp:72`.

## N04 — Spelling C: `get_data_type<int32>()`. Report 01's count is one short.

```
grep -rn "get_data_type<int32>()" $T
```

Four lines, not three:

| Line | What |
|---|---|
| `taichi/ir/type.cpp:463` | `TI_ASSERT(get_data_type<int32>() == dt);` inside `TypedConstant::val_int32()` |
| `taichi/transforms/scalarize.cpp:63` | `TypedConstant(get_data_type<int32>(), i)` |
| `taichi/transforms/scalarize.cpp:114` | same |
| `taichi/transforms/scalarize.cpp:530` | same |

Report 01 section 3.13's third sweep row states "3" and then says the type.cpp
hits are "the only other hits **tree-wide**". `taichi/ir/type.cpp` is inside
`taichi/ir/`, which is inside that report's own stated territory, so the line is
in the set the command returns, and the figure under the command should be 4.
`type.cpp:478` is `get_data_type<int64>()` and never matched at all.

This is a counting error, not a missed site: `type.cpp:463` is the assert that
report 01's section 3.2 already builds its twelve-abort-site finding on, so the
site is inventoried under a different heading. I record it because standing
instruction 7 requires a stated figure to reconcile against its own command, and
this one does not.

## N05 — Spelling E, and it is not in report 01 at all: `taichi/ir/type_system.cpp`

`grep -rc "i32" $T` puts `taichi/ir/type_system.cpp` third by volume, at 37
lines. Neither `report-01-ir-types.md` nor `notes-01-ir-types.md` mentions the
file: `grep -n "type_system" <both files>` returns nothing.

The mechanism is a file-local macro at `taichi/ir/type_system.cpp:161-163`:

```
161:#define PRIM(dt) \
162:  DataType dt =  \
163:      TypeFactory::get_instance().get_primitive_type(PrimitiveTypeID::dt);
165:PRIM(i32)
171:DataType i32_void = i32;
```

So inside that anonymous namespace, the bare token `i32` **is a `DataType`
object**, and every use of it in the internal-operation signature table is a
32-bit type declaration. `i32_void` at `:171` is an alias for the same object,
used as the return type of context-taking void internals.

This is a fifth spelling, and it is invisible to all four of report 01's sweeps:
it contains no `PrimitiveType::i32`, no `TypedConstant`, no `(int32)` and no
`get_data_type<int32>`.

Enumeration of the uses is in N09.

## N06 — Spelling D, the open one. First: report 01's filter is not reproducible, so I replaced it.

Report 01 defines the class as `TypedConstant(` in territory "minus every call
whose first argument is a type", listing the type spellings it removed (`dt`,
`dst_type`, `data_type`, `load_data_type`, `PrimitiveType::…`, a `ret_type`,
`get_data_type<int32>()`, `…get_element_type()`). That is a blacklist of
identifier names. It cannot be re-derived by anyone who does not have the same
list, and it silently misses any type-valued first argument spelled some other
way.

I replaced it with a **structural** filter that needs no name list: parse from
the token `TypedConstant` forward, balance brackets, and count top-level commas.
Zero commas selects a one-argument construction; one comma selects the
two-argument `TypedConstant(DataType, const T&)` form, which is explicitly typed
by construction. This is exactly the class the brief asks about, and it is
reproducible.

The script is at
`/tmp/claude-1000/-opt-project-taichi/0b12bd28-d413-4ea3-b486-d56840a983ae/scratchpad/s06`
and is reproduced in the report.

## N07 — Spelling D is bigger than `TypedConstant(`. There is a brace-init form.

While reading `make_mesh_block_local.cpp` I hit this at `:169`:

```
169:        block_->push_back<ConstStmt>(TypedConstant{offload_->block_dim});
```

Brace-initialised. `grep "TypedConstant("` cannot see it. I then enumerated the
follow-character of every occurrence of the identifier in the territory:

```
grep -rno "TypedConstant." $T | sed 's/.*TypedConstant//' | sort | uniq -c | sort -rn
```

| Follows | Count | What it is |
|---|---|---|
| `(` | 139 | paren construction, the set report 01 measured |
| ` ` (space) | 22 | declarations, parameter types, `class TypedConstant`, a string literal |
| `:` | 19 | `TypedConstant::` qualified names |
| `>` | 12 | template arguments |
| `{` | **7** | **brace construction — invisible to every sweep in report 01** |

So the population is **146 constructions, not 139**. The seven brace-init lines:

```
grep -rn "TypedConstant{" $T
taichi/transforms/demote_mesh_statements.cpp:86
taichi/transforms/demote_mesh_statements.cpp:94
taichi/transforms/demote_mesh_statements.cpp:123
taichi/transforms/make_mesh_block_local.cpp:80
taichi/transforms/make_mesh_block_local.cpp:169
taichi/transforms/make_mesh_block_local.cpp:200
taichi/transforms/make_mesh_block_local.cpp:575
```

Report 01's second sweep pattern was `TypedConstant((int32)|TypedConstant{(int32)|int32(`,
so it caught the three `(int32)` casts at `make_mesh_block_local.cpp:80`, `:200`,
`:575` through the middle alternative. It could not catch `:169` or the three
`demote_mesh_statements.cpp` lines, because those carry no cast.

The 22 space-following lines were checked individually and none of them is a
construction with an untyped integer argument: they are the class definition
(`type.h:549`), field and parameter declarations (`statements.h:988`, `:990`,
`snode.h:103`, `frontend_ir.h:594`, `:830`, `arithmetic_interpretor.h:29`,
`gen_offline_cache_key.cpp:530`, `type_utils.h:145`, `:173`,
`type.h:655`, `:657`, `type.cpp:429`), a format string (`type.h:577`), and local
variable declarations in `constant_fold.cpp:53`, `:159`, `:178`, `:189`, `:283`,
`:297`, `:321` and `statements.h:2117`, all of which take either a `dst_type`
argument or a copy.

## N08 — The 146 split by arity

```
one-argument constructions : 86  (of which 13 are the ctor declarations in type.h)
two-argument constructions : 53  (of which  1 is  the ctor declaration at type.h:616)
brace constructions        :  7  (all one-argument)
```

86 + 53 + 7 = 146. Declarations 13 + 1 = 14. **Call sites: 132.**

Of the 132, **93** are one-argument (73 paren + 7 brace + ... no: 86 - 13 = 73
paren one-arg, plus 7 brace one-arg = 80) and **52** are two-argument.
80 + 52 = 132. Checked: 132 = 146 - 14.

The 52 two-argument calls are explicitly typed by construction. Thirteen of them
name a 32-bit type and therefore belong to the OTHER spellings, not to this one:

| Line | Type spelling used |
|---|---|
| `taichi/ir/ir_builder.cpp:142` | `get_primitive_type(PrimitiveTypeID::i32)` — spelling B |
| `taichi/transforms/utils.cpp:17` | `PrimitiveType::i32` — spelling A |
| `taichi/transforms/make_mesh_thread_local.cpp:42` | `PrimitiveType::i32` — spelling A |
| `taichi/transforms/make_cpu_multithreaded_range_for.cpp:68` | `PrimitiveType::i32` — spelling A |
| `taichi/transforms/make_cpu_multithreaded_range_for.cpp:70` | `PrimitiveType::i32` — spelling A |
| `taichi/transforms/make_cpu_multithreaded_range_for.cpp:72` | `PrimitiveType::i32` — spelling A |
| `taichi/transforms/make_cpu_multithreaded_range_for.cpp:81` | `PrimitiveType::i32` — spelling A |
| `taichi/transforms/make_cpu_multithreaded_range_for.cpp:90` | `PrimitiveType::i32` — spelling A |
| `taichi/transforms/scalarize.cpp:471` | `PrimitiveType::i32` — spelling A |
| `taichi/transforms/scalarize.cpp:473` | `PrimitiveType::i32` — spelling A |
| `taichi/transforms/scalarize.cpp:63` | `get_data_type<int32>()` — spelling C |
| `taichi/transforms/scalarize.cpp:114` | `get_data_type<int32>()` — spelling C |
| `taichi/transforms/scalarize.cpp:530` | `get_data_type<int32>()` — spelling C |

Five more name a non-i32 fixed type (`ir_builder.cpp:148` i64, `:154` u32,
`:160` u64, `:166` f32, `:172` f64). The remaining 34 take a type derived from
an operand (`dt`, `dst_type`, `ret_type`, `data_type`, `load_data_type`,
`get_element_type()`, `ptr_removed()`), so they carry no width of their own and
are excluded with that reason. 13 + 5 + 34 = 52.

**So the open class is exactly the 80 one-argument calls.** Every one of them is
opened and ruled on in N09 to N11. Report 01's figure was 78 candidates over a
blacklist filter; mine is 80 over a structural one, and the two disagree in both
directions.

## N09 — Every one of the 80 opened. Ruling on each.

I opened each of the 80 and read the declaration of the argument, because the
overload chosen depends on the argument's C++ type and nothing else. Full table
in the report. Outcome:

| Ruling | Count |
|---|---|
| Real 32-bit width site (selects `TypedConstant(int32)`) | 73 |
| Excluded — selects `TypedConstant(uint1)`, argument is `bool` | 3 |
| Excluded — selects `TypedConstant(float32)`, argument is `float32` | 2 |
| Excluded — selects `TypedConstant(DataType)`, argument is a type | 2 |
| **Total** | **80** |

73 + 3 + 2 + 2 = 80, and the table the report carries has 80 rows with no
duplicate `file:line`, checked with `cut -f1 … | sort | uniq -d` returning empty.

The seven exclusions, each with the reason on the row rather than dropped:

- `check_out_of_bound.cpp:48`, `:107`, `:180` — `TypedConstant(true)`.
  `uint1` is `bool` (`taichi/common/core.h:136`), so `true` is an exact match for
  the `uint1` overload and no i32 is created. These are the boolean accumulator
  that all the per-index checks are `bit_and`-ed into.
- `auto_diff.cpp:957`, `:963` — `TypedConstant(x)` where `x` is the `float32`
  parameter of `ADTransform::constant` at `:953`. Selects the `float32` overload.
- `alg_simp.cpp:71`, `:92` — `TypedConstant(rhs->ret_type)` and
  `TypedConstant(scalar_stmt->ret_type)`. A `DataType` argument selects the
  one-argument `explicit TypedConstant(DataType)` at `type.h:575`, which
  zero-initialises a constant of the operand's own type. No width of its own.

## N10 — The "macro continuation" category is empty under a structural filter.

Report 01 says of its 78 candidates that "`constant_fold.cpp` contributes five
macro continuation lines whose type argument is on the previous line, and
`ir_builder.cpp` six of the same shape". Under my filter those eleven lines never
enter the class at all: they are **two-argument** calls whose `dst_type` /
`get_primitive_type(...)` first argument is present, merely wrapped onto the
preceding physical line by clang-format. My extractor balances brackets across
lines, so it reads the whole call rather than one line of it.

Checked: `constant_fold.cpp` contributes 17 `TypedConstant(` lines, and my
arity split puts **all 17** in the two-argument bucket, first argument
`dst_type` in every case. `ir_builder.cpp` contributes 6, all two-argument.
17 + 6 = 23, none of them candidates.

This is a straight divergence from report 01. Its residue contained eleven lines
that were never members; mine contains none of them, and contains seven brace
constructions its filter could not see. Net 80 against 78, arrived at from
different populations.

## N11 — Which of the 73 report 01 already has, mechanically determined.

I did not eyeball this. Three tiers, computed by script against the report text
and then spot-checked by hand for false matches:

| Tier | Meaning | Count |
|---|---|---|
| A | named in report 01 as `file:line`, or as a `:NNN` shorthand on a line that also names the file | 22 |
| B | falls inside a line RANGE report 01 cites, but is not named as a site | 5 |
| C | absent from report 01 entirely | **46** |

22 + 5 + 46 = 73.

Two false matches were caught by hand and moved from A to C, which is why the
script alone cannot be trusted:

- `make_block_local.cpp:165` matched report 01's `make_mesh_thread_local.cpp:165`
  — a different file with the same line number.
- `make_mesh_block_local.cpp:204` matched report 01's `type_factory.cpp:204` —
  likewise.

The five tier-B sites: `frontend_ir.cpp:747` (inside `:742-747`, cited in
section 3.6 for the host-`int` fold, not for the constant),
`check_out_of_bound.cpp:126` (inside `:123-126`, and section 5 does quote the
construction), `check_out_of_bound.cpp:217` (inside `:205-230`),
`lower_matrix_ptr.cpp:537` (inside `:536-540`, and section 3.8 does quote it),
`simplify.cpp:193` (inside `:189-205`, cited for the assert, not the constant).

One tier-A site is cited by report 01 **and explicitly excluded from its
inventory**: `demote_operations.cpp:99`, which report 01 section 3.13 calls "the
one line of the residue I opened" and states is "**not** inventoried".

## N12 — The 46 that are new. Grouped by what the width feeds.

Full table with file, line, argument type and consumer is in the report. The
grouping:

- **Bounds and clamp constants on ndarray and field indices — 12 sites.**
  `check_out_of_bound.cpp:47`, `:74`, `:106`, `:142`, `:179`, `:186`;
  `handle_external_ptr_boundary.cpp:30`, `:39`, `:71`, `:78`;
  `bit_loop_vectorize.cpp:76`; `demote_operations.cpp:134`.
  `handle_external_ptr_boundary.cpp` is in report 01 only for its `:62-67`
  host-int product and its `:100` return; its four constants are new.
- **Block-local storage geometry — 15 sites.** `make_block_local.cpp:145`,
  `:152`, `:165`, `:183`, `:194`, `:202`, `:277`, `:285`, `:298`, `:303`,
  `:315`, `:328`; `make_mesh_block_local.cpp:116`, `:204`, `:268`, `:304`,
  `:83`, `:578` — recount below, this bullet is wrong as first written and I am
  fixing it rather than leaving it.
- **Index arithmetic helpers — 3 sites.** `utils.cpp:7`, `:10`, `:20`, the
  divisor and mask constants inside `generate_mod` and `generate_div`.
- **Struct-for reconstruction — 3 sites.** `demote_dense_struct_fors.cpp:49`,
  `:79`; `lower_ast.cpp:241`.
- **Address linearisation — 2 sites.** `simplify.cpp:175`, `:178`.
- **Mesh relation sizes — 3 sites.** `demote_mesh_statements.cpp:86`, `:94`,
  `:123`.
- **Frontend field-index offset — 1 site.** `frontend_ir.cpp:663`.
- **Not an index — 1 site.** `bit_loop_vectorize.cpp:188`.

The bullet counts above are hand-written and I do not trust them. Recomputed
mechanically from the tier file in N13.

## N13 — N12's grouping recomputed by file, from the tier file, not by hand.

```
awk -F'\t' '$1=="C"{print $2}' tier.tsv | sed 's/:.*//' | sort | uniq -c | sort -rn
```

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

12+9+6+4+3+3+2+2+2+1+1+1 = 46. Twelve files.

**N12's second bullet was wrong** — it listed eighteen sites under a heading of
fifteen and mixed two files. The by-file table above supersedes it. I am leaving
N12 in place with the error visible rather than editing it out, because these
notes are meant to be contemporaneous.

The two that matter most for item 6.2, on my reading:

- `simplify.cpp:175` and `:178` are inside `visit(LinearizeStmt *)`, which is
  where a multi-axis SNode coordinate becomes one linear address. The
  accumulator seed and every per-level stride constant are i32. Report 01
  section 3.6 has the host-`int` half of this (`:176` `auto stride_product = 1;`,
  `:187` `stride_product *= stmt->strides[i];`) and does not have the IR half:
  the `ConstStmt`s those host values are poured into. Both halves have to move
  together or the pass emits an i32 constant into a wider computation.
- `utils.cpp:7`, `:10`, `:20`. `generate_mod` and `generate_div` are, per report
  01's own section 3.7, "the most-used index-arithmetic helper in the territory".
  Report 01 records that the helpers take `int y` and that `:17` emits an
  explicit i32 constant. It does not record that the other three constants in
  the same two functions are i32 by the implicit overload. So the two functions
  contain four i32 constants and report 01 carries one.

## N14 — Spelling A re-verified rather than adopted.

```
grep -rn "PrimitiveType::i32" $T                       -> 64
grep -rn "PrimitiveType::i32" taichi/inc/              ->  0
```

`taichi/inc/` contributes nothing, which is why my four-directory territory and
report 01's three-directory one give the same 64.

I then checked report 01's section 3.13 table against the command rather than
trusting it:

```
awk 'NR>=650 && NR<=765 && /^\| `taichi\//' report-01-ir-types.md \
  | grep -oE 'taichi/[a-z_/]+\.(cpp|h):[0-9]+' | sort > r13.txt
grep -rn "PrimitiveType::i32" taichi/ir/ taichi/analysis/ taichi/transforms/ \
  | cut -d: -f1,2 | sort > actual.txt
diff r13.txt actual.txt
```

64 rows, 64 grep hits, `diff` empty. **Report 01's spelling-A enumeration is
exactly right and I adopt its dispositions.** Recorded as a positive finding,
since most of what I have to say about that report is corrective.

## N15 — Spelling B classified. 27 lines, nothing dropped.

| Disposition | Count | Lines |
|---|---|---|
| Construction or default that fixes a 32-bit width | 3 | `type_factory.cpp:167`, `type.cpp:238`, `ir_builder.cpp:143` |
| Predicate test that gates behaviour on i32-ness, already carried by report 01 | 5 | `type_check.cpp:148`, `:161`, `bit_loop_vectorize.cpp:321`, `value_diff.cpp:82`, `:143` |
| Predicate test, NOT in report 01 | 2 | `frontend_type_check.cpp:72`, `simplify.cpp:234` |
| Type-set membership test listing every integer type — unrelated | 3 | `type_utils.h:106`, `:122`, `constant_fold.cpp:37` |
| `if / else if` dispatch arm over the whole type enumeration — unrelated | 13 | `type_utils.h:137`, `:150`, `:178`, `type_utils.cpp:126`, `type.cpp:400`, `:434`, `:524`, `type.h:621`, `constant_fold.cpp:77`, `:113`, `:204`, `:233`, `:253` |
| C++-type-to-Taichi-type template mapping — excluded, same reason report 01 excluded `type_utils.h:40` | 1 | `type_utils.h:71` |

3 + 5 + 2 + 3 + 13 + 1 = 27.

The two that are not in report 01:

- `taichi/transforms/frontend_type_check.cpp:72` — the frontend's own
  index-type check. Report 01 cites `frontend_type_check.cpp:71-77` as a
  warning site in its pass-ordering section, so the file is known to it, but
  `:72` is not in any inventory row and it is a second, earlier i32 gate on
  field indices, before `type_check.cpp:148`.
- `taichi/transforms/simplify.cpp:234` — `TI_ASSERT` that every child of the
  SNode being offset-pushed is i32 or f32. This is the **guard that makes the
  `sizeof(int32)` at `:239` and `:261` correct**. Report 01 carries both
  `sizeof(int32)` sites (section 3.7) and does not carry the assert that pins
  the child type to four bytes in the first place. A width change that allows
  an i64 place under such an SNode aborts here before it can reach the
  arithmetic.

## N16 — Spelling F: a helper or member that defaults to 32 bits, and a literal bit count.

Found by asking a different question: which lines in the territory contain the
bare integer `32` as a WIDTH, rather than as part of a type name?

```
grep -rn "\b32\b" $T | grep -v "int32\|uint32\|i32\|u32\|f32\|float32\|//"
```

12 lines. Classified:

| Line | Disposition |
|---|---|
| `taichi/ir/frontend_ir.cpp:266` | **real 32-bit width site.** `get_primitive_int_type(32, /*is_signed=*/true)` builds the exponent member of the struct returned by `frexp`. A literal bit count. Invisible to all four of report 01's sweeps, and absent from report 01 |
| `taichi/ir/type.h:372` | **real 32-bit default.** `int num_bits_{32};` on `QuantIntType`. Absent from report 01 |
| `taichi/ir/type_factory.cpp:166` | the `bits == 32` arm of `get_primitive_int_type` — the selector, not an assumption |
| `taichi/ir/type_factory.cpp:183` | the `bits == 32` arm of `get_primitive_real_type` — float, not integer |
| `taichi/ir/frontend_ir.h:1174` | `TI_ASSERT((v % 32 == 0) …)` on `block_dim` for cuda/vulkan/amdgpu. A launch-granularity rule, not a data width |
| `taichi/inc/cuda_kernel_utils.inc.h:8` | `int warp_size() { return 32; }`, a host-side stub. Hardware warp width, not a data width |
| `taichi/analysis/build_cfg.cpp:25` | `((std::size_t)key.in_parallel_for << 32)` — a hash-key packing shift. Unrelated |
| `taichi/transforms/alg_simp.cpp:13` | `max_weaken_exponent = 32`, a strength-reduction budget. Unrelated |
| `taichi/inc/constants.h:11` | `taichi_max_num_args_extra = 32`. Argument count, unrelated to width |
| `taichi/inc/constants.h:19` | `taichi_error_message_max_num_arguments = 32`. Unrelated |
| `taichi/inc/constants.h:20` | `taichi_result_buffer_entries = 32`. Unrelated |
| `taichi/ir/type.h:372` counted once above | — |

Eleven distinct lines listed, `type.h:372` appearing once; the command returns
twelve rows because `taichi/inc/constants.h` contributes three and I have listed
all three. Recount: `frontend_ir.cpp:266`, `type.h:372`, `type_factory.cpp:166`,
`type_factory.cpp:183`, `frontend_ir.h:1174`, `cuda_kernel_utils.inc.h:8`,
`build_cfg.cpp:25`, `alg_simp.cpp:13`, `constants.h:11`, `constants.h:19`,
`constants.h:20` — **eleven**. The twelfth line the command returns is
`taichi/ir/type_factory.cpp:166` and `:183` counted separately, which I have
done. Twelve rows, twelve lines; the table above has twelve rows including the
struck duplicate line. I am flagging my own arithmetic rather than leaving it:
the correct statement is **twelve lines, of which two are real 32-bit width
sites** (`frontend_ir.cpp:266`, `type.h:372`), and neither is in report 01.

The `QuantIntType` default carries its own comment at `type.h:369-370`:

```
369:  // TODO(type): for now we can uniformly use i32 as the "compute_type". It may
370:  // be a good idea to make "compute_type" also customizable.
```

paired with the runtime half at `type.cpp:236-240`, where a null `compute_type`
resolves to i32 or u32. So the quantised type system has a stated, acknowledged
32-bit compute assumption that no revision of report 01 records.

## N17 — Spelling E enumerated: `taichi/ir/type_system.cpp`

```
grep -n "\bi32\b\|i32_void" taichi/ir/type_system.cpp     -> 37 lines
```

Two are the definitions (`:165` `PRIM(i32)`, `:171` `DataType i32_void = i32;`).
The other 35 are uses in `Operations::init_internals()`. Classified:

| Class | Count | Lines |
|---|---|---|
| Internal op whose RETURN type is i32 and which produces a thread or lane index | 4 | `:307` `linear_thread_idx`, `:369` `localInvocationId`, `:370` `vkGlobalThreadIdx`, `:376` `subgroupInvocationId` |
| Internal op with an i32 ARGUMENT that is a sparse-matrix row/column index | 1 | `:302` `insert_triplet_##dt(u64, i32, i32, dt)` |
| CUDA warp intrinsic, i32 as the hardware lane-mask or shuffle width | 12 | `:325`, `:327`, `:332`, `:333`, `:334`, `:339`, `:340`, `:341`, `:342`, `:343`, `:347`, `:348` |
| SPIR-V subgroup query returning i32 | 2 | `:373` `subgroupElect`, `:375` `subgroupSize` |
| `i32_void` as the return type of a context-taking void internal — a placeholder, not a width | 15 | `:302`, `:308`-`:315`, `:331`, `:335`, `:350`, `:367`, `:368`, `:371`, `:372` |
| Test-only internal | 1 | `:316` `test_internal_func_args` |

`:302` appears twice above because the same line carries both an `i32_void`
return and two i32 arguments. So the line counts sum to 4+1+12+2+15+1 = 35 with
`:302` double-counted, i.e. **34 distinct use lines plus `:302` counted in two
classes**, over 35 use lines total. Checked: `35 = 37 - 2` definitions.

The material ones for item 6.2 are the four index-producing returns. A global
thread index declared i32 caps the launch index space at 2^31 in the type system
itself, on both spines: `linear_thread_idx` is the LLVM-spine internal and
`vkGlobalThreadIdx` the SPIR-V one.

**Whether this file is in scope is not mine to rule.** It is inside
`taichi/ir/`, which is inside my territory, so I enumerate it. The consumers are
codegen, which is territory 02. Escalated.

## N18 — Spelling G: plain C++ `int` reaching an IR type or an index.

This is the spelling with no natural boundary, so I state the boundary I chose
and what it leaves out, rather than implying a sweep I did not run.

**Membership I closed:** every `int`, `std::vector<int>` or `std::array<int,N>`
declaration on its own line in a HEADER of the territory. Command:

```
grep -rnE '^[[:space:]]*(mutable[[:space:]]+)?(const[[:space:]]+)?(int|std::vector<int>|std::array<int,[^>]*>)[[:space:]]+[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*(\{[^}]*\})?[[:space:]]*(=[^;]*)?;' \
  taichi/ir/ taichi/analysis/ taichi/transforms/ taichi/inc/ --include=*.h
```

**102 lines, 16 files.** Classified, every one:

| Ruling | Count |
|---|---|
| ADDR — the value participates in computing an address, index, extent, stride or bit offset | 47 |
| NONADDR — an IR quantity that is not addressing (ids, launch config, argument paths, texture) | 46 |
| LOCAL — the regex caught a local variable inside an inline method, not a member | 7 |
| PARAM — the regex caught a function parameter with a default, not a member | 2 |
| **Total** | **102** |

47 + 46 + 7 + 2 = 102, and the 102 `file:line` keys diff clean against the
command's own output.

**What this does NOT cover, stated plainly:** `int` locals and `int` parameters
in `.cpp` files. Those are where `check_out_of_bound.cpp:170`'s
`int max_valid_index`, `simplify.cpp:176`'s `stride_product` and
`demote_dense_struct_fors.cpp:19`'s `total_shape` live, and report 01 sections
3.5 and 3.6 already inventory a set of them. I did not attempt to close that,
because "an `int` local that reaches an index" has no textual signature and
closing it means reading every function in the territory. It is the single
largest open surface I am leaving, and it is in Escalations.

## N19 — N16's arithmetic was wrong. Corrected against the command.

I wrote "twelve lines" in N16 and then tied myself in knots reconciling a table
that has eleven real rows. Ran the count instead of arguing about it:

```
grep -rn "\b32\b" $T | grep -v "int32\|uint32\|i32\|u32\|f32\|float32\|//" | wc -l
-> 11
```

**Eleven lines, not twelve.** The last three sentences of N16 are wrong and the
"twelfth line" I tried to account for does not exist. The eleven-row table above
those sentences is correct as it stands; the row labelled
"`taichi/ir/type.h:372` counted once above" is not a row and should be struck.

Corrected statement, which is what goes in the report: **eleven lines, of which
two are real 32-bit width sites** — `taichi/ir/frontend_ir.cpp:266` and
`taichi/ir/type.h:372` — **and neither is in report 01.** The other nine are
dispositioned in N16's table with a reason each.

I am leaving N16 intact with the error visible. This is the fifth or sixth
instance in this project of a stated total contradicting its own list, and the
useful thing is the record of how it happened: I wrote the total from the shape
of the table rather than from the command, which is exactly what standing
instruction 7 forbids.

## N20 — What I did not sweep, decided deliberately.

1. **`int` locals and parameters in `.cpp` files.** N18 states the reason. Report
   01 sections 3.5 and 3.6 hold a partial set; neither of us has closed it.
2. **`std::size_t` and `int64` quantities that are narrowed on assignment**
   without a cast token. `TypedConstant((int32)bls_offset_in_bytes)` is visible;
   `int x = some_size_t;` is not, and I have no textual filter for it.
3. **Anything outside the four directories.** Codegen, runtime and program are
   other territories. Where a spelling in my territory is consumed outside it
   (`type_system.cpp`'s internal-op table, `PhysicalCoordinates`) I say so and
   escalate rather than following it.
4. **`u32`.** The brief says 32-bit width, and unsigned 32-bit types are a real
   part of that, but every spelling I enumerated is the signed one. I did not
   run the `u32` family. `grep -rn "PrimitiveType::u32" $T` returns 3 lines
   (`type_utils.h:139`, `make_mesh_thread_local.cpp:26`, and `type.cpp:238`'s
   unsigned arm), and `make_mesh_thread_local.cpp:26` is a mesh address type
   declared `u32` with the comment "unt32_t type address". That is a 32-bit
   address in my territory. Report 01 does not carry it. I am recording it as a
   finding rather than opening a fifth enumeration on a pass that is meant to
   close a fourth. Escalated.

## N21 — N20 item 4 corrected the moment I ran it.

I wrote "returns 3 lines" from memory and then ran it:

```
grep -rn "PrimitiveType::u32" $T
taichi/ir/type_utils.h:50                     return PrimitiveType::u32;
taichi/ir/type_utils.h:138                    return PrimitiveType::u32;
taichi/ir/type.h:609                          explicit TypedConstant(uint32 x) : dt(PrimitiveType::u32), …
taichi/transforms/make_mesh_thread_local.cpp:26   auto data_type = PrimitiveType::u32;  // unt32_t type address
```

**Four lines, and not the ones I named.** `type.cpp:238` spells its unsigned arm
`PrimitiveTypeID::u32`, not `PrimitiveType::u32`, so it is in spelling B and not
here — the same disjointness I established in N03, working against me this time.

Dispositioned:

- `type_utils.h:50` — the `uint32` arm of `get_data_type<T>()`. Excluded, same
  reason as `type_utils.h:40` and `:71`: a correct C++-to-Taichi mapping.
- `type_utils.h:138` — the i32 arm of `to_unsigned()`. A dispatch arm.
- `type.h:609` — the `TypedConstant(uint32)` constructor. The mechanism, not a
  site, same disposition report 01 gives `type.h:582`.
- `taichi/transforms/make_mesh_thread_local.cpp:26` — **a real 32-bit address
  width.** `auto data_type = PrimitiveType::u32;  // unt32_t type address`, used
  for the mesh offset table. Absent from report 01, whose only
  `make_mesh_thread_local.cpp` rows are `:42`, `:106`, `:114`.

So the unsigned family contributes exactly one real site in this territory, and
that lets me close it rather than leave it open. Amending N20 item 4 from "not
swept" to "swept, one site". The remaining unswept items in N20 are 1, 2 and 3.

## N22 — Report written. Final mechanical checks on it.

```
D table rows      -> 80    (matches the 80 one-argument calls)
G table rows      -> 102   (matches the header-declaration command)
NEW table rows    -> 58    (matches 46 + 7 + 2 + 2 + 1)
rows marked NEW inside the D table -> 46
```

One headline figure was wrong on first write and is fixed: spelling C's real-site
column said 3 while section 3.3 dispositions all four lines as real
(`type.cpp:463` is the assert report 01's section 3.2 builds on). Corrected to 4.
The "new versus report 01" column for C stays at 0, because all four are already
inventoried somewhere in report 01.

No source file was touched. No other agent's file was read or touched beyond the
two named in my brief.

---

## N23 — Handover received. Two warnings from the discovering agent, plus N063-N066.

Read `notes-01-ir-types.md` N063 to N066. N065 is the mechanism statement and
N064 is the boundary ruling. Two warnings, taken in turn.

**Warning 1, over-collection.** N065 item 3: the filter over-collects because
macro continuation lines carry the type argument on the previous line, and no
line-spanning parse was written. **Already resolved before the handover arrived,
at N06 and N10.** My extractor balances brackets across lines and counts
top-level commas, so those lines are read as the two-argument calls they are and
never enter the class. Measured: `constant_fold.cpp` contributes 17
`TypedConstant(` lines and all 17 land in the two-argument bucket with
`dst_type` as the first argument; `ir_builder.cpp` contributes 6, all
two-argument. The eleven lines N065 warns about are eleven of those 23.

**Warning 2, under-collection: do other constructors in this territory have
narrowing 32-bit overloads of the same kind?** Not asked by anyone before. This
is new work and it is N24 to N26.

**The boundary.** N064 says five of the 78 are accounted for and the other 73 are
unopened, unsampled, and carry no formed view. My pass opened all 80 of my
one-argument calls from source, including all four `lower_matrix_ptr.cpp` sites
and `demote_operations.cpp:99`, and did not take any prior classification on
trust, so the boundary does not change anything I concluded. It does change how I
should present `demote_operations.cpp:99`: report 01 cites it as evidence and
places it in no inventory table, which is what my tier-A row already says.

## N24 — Warning 2, asked as a closed question rather than by guessing.

The trap in "are there other constructors like `TypedConstant`" is that you can
only find what you think to grep for. So I inverted it.

**Closure argument.** A construct that pins a Taichi 32-bit type must *name* that
type somewhere inside its own body. There are only so many ways to name it, and
I have already enumerated every one of them in this territory:

| Spelling | Lines |
|---|---|
| A `PrimitiveType::i32` | 64 |
| B `PrimitiveTypeID::i32` | 27 |
| C `get_data_type<int32>()` | 4 |
| E bare `DataType i32` (confined to `type_system.cpp`) | 37 |
| F literal bit count 32 | 11 |

So the question becomes finite: **which of those 143 lines sit inside a construct
that is keyed on a C++ argument type or template parameter, rather than on a type
the caller wrote?** Those and only those can produce an i32 from a line that
carries no type. Going through them:

| Line | Construct | Keyed on |
|---|---|---|
| `taichi/ir/type_utils.h:40` | `get_data_type<T>()`, the `int32` arm | template parameter `T` |
| `taichi/ir/type_utils.h:71` | `get_primitive_data_type<T>()`, the `int32` arm | template parameter `T` |
| `taichi/ir/type.h:582` | `explicit TypedConstant(int32 x)` | argument type — **spelling D, closed** |
| `taichi/ir/expr.cpp:93` | `Expr::Expr(int32 x)` body, `make_shared<ConstExpression>(PrimitiveType::i32, x)` | argument type — **NEW, spelling I** |
| `taichi/ir/ir_builder.cpp:143` | `IRBuilder::get_int32` body | nothing; the function name states the width |
| `taichi/ir/type_factory.cpp:167` | `get_primitive_int_type` | the runtime value of `bits` |
| `taichi/ir/type.cpp:238` | `QuantIntType` ctor | `compute_type == nullptr` |
| `taichi/ir/type.h:372` | `int num_bits_{32}` | nothing; a member default |
| everything else in A, B, C, E, F | dispatch arms, predicate tests, written-out types | the caller's written type |

**One further construct forwards into `TypedConstant` without naming a type at
all**, and it is not in the 143 because it names no type:

```
taichi/ir/frontend_ir.h:832-835
  template <typename T>
  explicit ConstExpression(const T &x) : val(x) {
    ret_type = val.dt;
  }
```

`val` is a `TypedConstant`, so `ConstExpression(i)` for a host `int i` is an
i32 constant expression spelled with neither `TypedConstant` nor `i32` on the
line. My spelling-D sweep cannot see it. Enumerated in N26.

**So the answer to warning 2 is yes, and the complete list of narrowing entry
points is three, not one:** `TypedConstant(int32)`, `Expr(int32)`, and the
`ConstExpression(const T&)` forwarder. All three are `explicit`, so all three
require direct construction and none fires on a conversion.

## N25 — Spelling I: `Expr(<host int>)`. Three constructions, both lines new.

`Expr::Expr(int32 x)` is declared at `taichi/ir/expr.h:30` and defined at
`taichi/ir/expr.cpp:92-94`:

```
92:Expr::Expr(int32 x) : Expr() {
93:  expr = std::make_shared<ConstExpression>(PrimitiveType::i32, x);
94:}
```

Report 01 saw `expr.cpp:93` in its spelling-A sweep and **excluded it, correctly,
as the mechanism rather than a site** — its section 3.13 row says so, and section
7.1 item 5 struck it in revision 1. What nobody then did was ask what calls it.

```
grep -rnE '(^|[^A-Za-z0-9_:>])Expr\(' $T   -> 39 lines
```

Dispositioned all 39: 7 are the constructor definitions in `expr.cpp:84-108`,
10 are the declarations in `expr.h:21-53`, 2 are `expr.h:90` and `:131`
(`Expr::make` and a helper returning `Expr(val)`), 1 is a defaulted parameter at
`frontend_ir.h:115`, and 17 are constructions from a `shared_ptr<Expression>`,
an `Identifier` or a `SNode` lookup. That leaves **2 lines**:

```
1835:          for (int i = 0; i < shape[0]; i++) {
1836:            auto ind = Expr(std::make_shared<IndexExpression>(
1837:                id_expr, ExprGroup(Expr(i)), expr->dbg_info));

1843:          for (int i = 0; i < shape[0]; i++) {
1844:            for (int j = 0; j < shape[1]; j++) {
1845:              auto ind = Expr(std::make_shared<IndexExpression>(
1846:                  id_expr, ExprGroup(Expr(i), Expr(j)), expr->dbg_info));
```

7 + 10 + 2 + 1 + 17 + 2 = 39.

`taichi/ir/frontend_ir.cpp:1837` carries one construction and `:1846` carries
two, so **three constructions on two lines**. Both are in
`ASTBuilder::expand_exprs`, which begins at `:1795`. `i` and `j` are host `int`
loop variables at `:1835`, `:1843` and `:1844`, bounded by
`tensor_type->get_shape()`, itself `std::vector<int>` at `taichi/ir/type.h:250`.

What the width feeds: each `Expr(i)` becomes a `ConstExpression` of
`PrimitiveType::i32`, is wrapped in an `ExprGroup`, and becomes the index of an
`IndexExpression` on a tensor-typed variable. This is **frontend tensor element
addressing**, the same class as the `MatrixPtrStmt` offsets in spelling D, one
layer earlier. It is the shape of site item 6.2 turns on.

Neither line is anywhere in report 01: `grep -n "frontend_ir.cpp:18" report-01`
returns nothing, and so do `1837` and `1846`.

## N26 — The other two forwarders: closed, and both empty in this territory.

**`ConstExpression(const T &x)`, the one-argument template at
`taichi/ir/frontend_ir.h:833`.**

```
grep -rn "ConstExpression" $T   -> 18 lines
```

All 18 opened: 6 are `make_shared<ConstExpression>(PrimitiveType::…, x)` inside
`Expr`'s own constructors at `expr.cpp:85`, `:89`, `:93`, `:97`, `:101`, `:105`
— every one of them the **two**-argument form, which is explicitly typed; 3 are
the class and its two constructor declarations at `frontend_ir.h:828`, `:833`,
`:837`; 2 are `is<>` / `cast<>` tests at `frontend_ir.cpp:726`, `:745`; 4 are the
class's own `type_check` and `flatten` at `frontend_ir.cpp:1284`, `:1287`,
`:1293` and the printer at `expression_printer.h:200`; 2 are the offline-cache
key emitter at `gen_offline_cache_key.cpp:218`, `:219`; 1 is the registration
macro at `inc/expressions.inc.h:17`. 6+3+2+4+2+1 = 18.

**Zero call sites of the one-argument form in this territory.** The forwarder is
real and would be a site if used; it is not used here. Any user is outside my
four directories, and the Python front end is out of scope by plan section 1.2.

**`value<T>`, `cast<T>`, `bit_cast<T>`, `expr_rand<T>` at `taichi/ir/expr.h:118`,
`:125`, `:131`, `:138`** — four more templates that route a C++ type into a
Taichi type, `value<T>` by calling `Expr(val)` directly. Counted:

```
grep -rn "value<"     $T | grep -v expr.h  -> 0
grep -rn "bit_cast<"  $T | grep -v expr.h  -> 0
grep -rn "expr_rand<" $T | grep -v expr.h  -> 0
grep -rn "cast<int"   $T | grep -v expr.h  -> 1
```

The single `cast<int` hit is `taichi/ir/snode.cpp:92`,
`static_cast<int>(acc_shape)`, which is a plain C++ narrowing and not this
template at all. Report 01 already carries it, at its sections 3.9 and 7, with
the `int64` accumulator and the `TaichiIndexWarning` at `:95-100`. Nothing new.

So of the three narrowing entry points in N24, one is spelling D with 73 sites,
one is spelling I with 3 constructions, and one has no callers here.

## N27 — Counts after folding the handover in.

Spellings: **nine**, not eight and not four.

New real sites, recomputed:

| Spelling | New |
|---|---|
| D — one-argument `TypedConstant` | 46 |
| E — bare `DataType i32` in `type_system.cpp` | 7 |
| I — `Expr(<host int>)` | 2 lines, 3 constructions |
| B — `PrimitiveTypeID::i32` | 2 |
| F — literal bit count or 32-bit default | 2 |
| H — `PrimitiveType::u32` | 1 |

46 + 7 + 2 + 2 + 2 + 1 = **60 new site lines**, of which one line carries two
constructions, so 61 constructions. I count lines, not constructions, in the
report's table, and say so there. Previous total was 58; the handover added 2.

Report amended: headline table gains a row I, a new section 3.9, section 4's
table and count updated, and escalations 10 and 11 added. Notes N23 to N27 are
the record of the amendment.
