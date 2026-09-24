# Report 07B — Can anything at the `at_buffer` decision point carry the physical/relative discrimination?

Agent: explore-07b-discriminator, pass B of a blind pair.
Contemporaneous notes: `modernization/investigation/notes-07b-discriminator.md`.

**Plan version worked against: `modernization/PROJECT-PLAN.md` dated
2026-09-09, 60989 bytes, md5 `d82f5dafad045ccbe48652d6072a3c80`.**

This is a **revision** after adversarial review. The first version of this
report worked against the same date stamp at 45208 bytes; the two adversaries
worked at 45208 and 48101. Per section 10 item 6 I re-read the plan before this
revision. It has grown by roughly 16 KB. What matters to this territory:

- Section 10's numbering, which was broken, is fixed. Grading is item 8.
- Section 10 gained a preamble: **"when two careful enumerations disagree on a
  count, suspect the rule before the count... declare the granularity rule
  before you count."** Two of my counts were wrong for exactly that reason and
  are re-derived below under a stated rule.
- Section 8.2 item 0 now records this territory as **RESOLVED** and withdraws
  the sentence recording the remedy as a change to the backend's value model. It
  already carries most of what follows.
- A new item 0a asks whether section 2.2 judges inherited code or only proposed
  changes. That question governs how my E2 should be weighted and I do not
  answer it.

Section 2.2a still puts item 6.2 in scope as BASE work in full, including the
`at_buffer` pointer-width collision by name. Nothing in the plan contradicts my
instructions.

**What changed in this revision.** My biconditional held under both adversaries;
neither could break a link, and both re-derived it rather than checking mine.
Four things did change: my stated limit was a sufficient condition presented as
a necessary one; the conditional I could not settle is now settled by
measurement and fires on one device; two of my counts were wrong; and my blast
radius omitted a coupling. Section 8 lists every correction and who found it.

---

## 1. The answer

**Qualified yes, and the qualification is the finding.**

`ptr_to_buffers_` **alone cannot** discriminate. Its values do not partition the
two sides. `BufferType::ExtArr` and `BufferType::Args` sit on **both** sides:
an `ExternalPtrStmt` is a physical device address when
`spirv_has_physical_storage_buffer` is on and a buffer-relative i32 offset when
it is off, and it writes the same map value either way. The map records **which
buffer**, never **how it is addressed**.

**The pair does discriminate, correctly, in every case reachable on today's
tree**, and both halves are already in scope at the decision point:

```
physical  ⟺  ptr_to_buffers_.at(ptr).type ∈ {ExtArr, Args}
             ∧  caps_->get(DeviceCapability::spirv_has_physical_storage_buffer)
```

`caps_` is a member at `taichi/codegen/spirv/spirv_codegen.cpp:2565`, read four
lines' worth of places already, including at `:783` inside the very visitor that
creates the ambiguity. It is constant for the life of a `TaskCodegen`.

**Critically, this rule survives the change item 6.2 actually needs.** Setting
`use_64bit_pointers` at `:82` to true widens every root-relative offset to u64
and breaks today's width predicate at `:2197` outright. It does not break the
map rule, because those pointers carry `BufferType::Root` and stay correctly
classified at any width.

**Where it stops, stated plainly.** The rule infers addressing mode from buffer
identity crossed with device capability. That inference is sound only because
exactly one buffer class is ever physically addressed today. `BufferType`
(`taichi/codegen/spirv/kernel_utils.h:23-31`) has no addressing-mode axis. If
item 6.2's design physically addresses a second buffer class — the root buffer
by device address, which is the obvious shape for 64-bit SNode addressing —
`Root` lands on both sides and the map genuinely cannot carry it. So: sufficient
for the shelved work as shelved; not sufficient for an arbitrary extension of
physical addressing.

## 2. The specific thing the four inherited documents got wrong

They tested whether `ptr_to_buffers_` has an **entry** on both sides. The brief
already knew that was the wrong test and that the **values** differ. Both of
those are right, and there is a third layer underneath which I do not think has
been stated.

**Entry existence is not merely the wrong test. It is a test of nothing at all,
because entries are fabricated.**

W1, W2 and W3 (`:319`, `:326`, `:332`) are each written
`ptr_to_buffers_[stmt] = ptr_to_buffers_[stmt->origin];`. **The right-hand side
uses `operator[]`, not `.at()`.** When the origin has no entry, `operator[]`
value-initialises one and inserts it.

For W1 the origin is an `AllocaStmt`, and **no writer anywhere writes an
`AllocaStmt` key** — the `AllocaStmt` visitor at `:284-303` writes none. So W1's
right-hand side fabricates on **every** execution. It is not an edge case; it is
the only case.

I compiled a verbatim replica of `kernel_utils.h:23-49` rather than argue it
from the standard. Under `g++ -O0`, `g++ -O2` and `clang++ -O2`, identically:

| Expression | `type` | `root_id` |
|---|---|---|
| `m[missing_key]` | `0` = `BufferType::Root` | `{-1}` |
| `BufferInfo{}` | `0` = `BufferType::Root` | `{-1}` |
| `BufferInfo e;` (control) | `6` (g++), `-1019198016` (clang) | — |

It is well-defined, not undefined behaviour: `BufferInfo() = default;` at
`kernel_utils.h:37` is user-declared but not user-provided, so
value-initialisation zero-initialises before running the constructor that the
`root_id{-1}` NSDMI at `:35` makes non-trivial. `type` keeps the zero. The
control row is what proves the zero comes from the zero-init step.

So `ptr_to_buffers_.count(p) == 1` proves nothing was written. Any future
implementation that reads presence, or that trusts a `Root` it finds, is reading
a value nobody chose. This does not by itself break the rule in section 1 —
fabrication only ever produces `Root`, never `ExtArr` or `Args`, so a fabricated
entry is never misread as physical — but it invalidates the reasoning every
prior document rested on.

## 3. Verified

Every item here is read directly off the source or produced by a program I ran.
File paths are relative to `/opt/project/taichi`.

### 3.1 The seven writers of `ptr_to_buffers_`

From `grep -rn "ptr_to_buffers_" --include=*.cpp --include=*.h .` over the whole
tree. Ten hits, all in `taichi/codegen/spirv/spirv_codegen.cpp`: 7 writes, 1
read (`:2212`), 1 count-assert (`:376`), 1 declaration (`:2635`).

| # | Line | Visitor | Guard | Value |
|---|---|---|---|---|
| W1 | `:319` | `MatrixPtrStmt` | index-offset, origin `AllocaStmt`, `is_shared` | copy of origin's entry |
| W2 | `:326` | `MatrixPtrStmt` | index-offset, origin `GlobalTemporaryStmt` | copy of origin's entry |
| W3 | `:332` | `MatrixPtrStmt` | byte-offset | copy of origin's entry |
| W4 | `:377` | `GetChStmt` | `out_snode->is_place()` | `BufferInfo(BufferType::Root, root)` |
| W5 | `:711` | `GlobalTemporaryStmt` | none | `BufferType::GlobalTmps` |
| W6 | `:798` | `ExternalPtrStmt` | `arg_at(arg_id).is_array` | `{BufferType::ExtArr, arg_id}` |
| W7 | `:800` | `ExternalPtrStmt` | else of W6 | `BufferType::Args` |

Seven rows; the count is the table. Four distinct `BufferType` values are ever
written — `Root`, `GlobalTmps`, `ExtArr`, `Args`. `BufferType` has seven
enumerators, so `Rets`, `ListGen` and `ArgPack` never enter this map; they reach
`get_buffer_value` only by direct call (`:609`, `:618`, `:753`). 4 + 3 = 7,
consistent with the enum.

**What my search can and cannot see.** It keys on the map identifier, so it
catches W5 and W7 even though neither names `BufferInfo` on the line — they use
the implicit single-argument constructor at `kernel_utils.h:39-41`, which
carries a `NOLINTNEXTLINE(google-explicit-constructor)`. That is the same shape
of hazard section 9's corollary describes. It would NOT see an access through a
reference bound to the map under another name; I checked the ten hits and no
such binding exists, so the enumeration is closed.

### 3.2 The three operand slots that reach `at_buffer`

`at_buffer` has seven callers: `:1617`, `:1621`, `:1626`, `:1630`, `:1633`,
`:1681` (all `AtomicOpStmt::dest`) and the two forwards from `load_buffer`
(`:2233`) and `store_buffer` (`:2257`). `load_buffer` has one caller,
`GlobalLoadStmt` at `:574` (`stmt->src`); `store_buffer` has one,
`GlobalStoreStmt` at `:568` (`stmt->dest`). So the `ptr` argument is one of
exactly three IR operand slots: `GlobalLoadStmt::src`, `GlobalStoreStmt::dest`,
`AtomicOpStmt::dest`.

### 3.3 The physical side is one statement class, with no derivations

Enumerating all 31 live `register_value` sites, exactly one produces a value
that is a physical device address: `:792`, the `ExternalPtrStmt` visitor inside
`if (caps_->get(DeviceCapability::spirv_has_physical_storage_buffer))` at
`:783`. It loads a u64 from args-struct field 1 (`DATA_PTR_POS_IN_NDARRAY`,
`taichi/ir/type_factory.h:69`) and adds the `OpSConvert`-widened linear offset
(`:786-791`).

`use_64bit_pointers` is `const bool use_64bit_pointers = false;` at `:82`.
`grep -rn "use_64bit_pointers"` over the tree returns exactly two hits: that
declaration and the read at `:2318` inside `make_pointer`. So `make_pointer`
never produces u64 today, and `GetRootStmt` (`:356`), `GetChStmt` (`:373`) and
`SNodeLookupStmt` (`:510`) are all u32.

**The derived-pointer hole is closed by the source, twice over.**
`MatrixPtrStmt::offset_used_as_index()` (`taichi/ir/statements.h:521-529`)
returns true when the origin is `AllocaStmt`, `GlobalTemporaryStmt`,
`ExternalPtrStmt` or `MatrixPtrStmt`. The SPIR-V visitor's index-offset branch
(`:312-329`) handles only the first two and hits **`TI_NOT_IMPLEMENTED` at
`:328`** for the other two. So a `MatrixPtrStmt` over an `ExternalPtrStmt`
cannot be compiled silently. And it does not arise: `FuseMatrixPtr` at
`taichi/transforms/scalarize.cpp:1260-1300` folds
`MatrixPtrStmt(ExternalPtrStmt, offset)` into a single fused `ExternalPtrStmt`
(`:1271-1283`) and erases the `MatrixPtrStmt`. **A physical address is always
produced directly, never derived.**

### 3.4 The biconditional, provable without any reachability argument

> `ptr_to_buffers_[k].type ∈ {ExtArr, Args}` **⟺** `k` is an `ExternalPtrStmt`.

Left to right: only W6 and W7 write those two values, and both key on an
`ExternalPtrStmt`. The only propagation sites are W1, W2 and W3. W1's origin is
an `AllocaStmt` and W2's is a `GlobalTemporaryStmt`, neither of which can hold
`ExtArr` or `Args`. W3 sits in the `else` of `offset_used_as_index()`, so **its
origin is by construction not an `ExternalPtrStmt`** — an `ExternalPtrStmt`
origin makes that predicate true. So no propagation can move `ExtArr` or `Args`
onto another key.

Right to left: W6/W7 are at `:797-801`, **outside** the capability branch at
`:783-795`, so every `ExternalPtrStmt` writes one of the two unconditionally.

This is structural. It needs no claim about which programs reach which line.

### 3.5 A fourth discriminator already in the file

`:1610-1612` computes `dest_is_ptr = dest_val.stype.flag == TypeKind::kPtr`,
commented "Shared arrays have already created an accesschain, use it directly."
`SType::flag` (`spirv_ir_builder.h:59`) is a property of the SPIR-V **type**,
set by `get_pointer_type` (`spirv_ir_builder.cpp:407-424`). Unlike `ValueKind`
it is not reset by arithmetic, because arithmetic on a pointer type is not
expressible. It separates "already a SPIR-V pointer" from "an integer to be
interpreted". **It does not separate physical-u64 from relative-u64** — both are
`kPrimitive` integers with `dt == u64` — so it is not the discriminator sought.
Stated because a reader will otherwise ask, and because it is a live precedent
for discriminating on something other than width.

### 3.6 The predicate is at three sites

`ptr_val.stype.dt == PrimitiveType::u64` appears at `:2197` (`at_buffer`),
`:2227` (`load_buffer`) and `:2249` (`store_buffer`). The latter two choose
`ti_buffer_type`, suppressing the uint-reinterpret-and-bitcast that the
buffer-relative path needs and the physical path does not. Same question, same
answer required. Three sites, one file.

### 3.7 Collisions and aliasing — all four routes closed

- **`Stmt *` key reuse.** `TaskCodegen` does not mutate the IR; a grep for
  `insert_before`, `->erase(`, `replace_usages` and `modifier` over
  `spirv_codegen.cpp` hits only two PrintStmt diagnostic strings (`:204`,
  `:213`). `task_ir_` is `OffloadedStmt *const`, "not owned" (`:2622`).
- **`raw_name()` collision.** `at_buffer` looks the value up by name (`:2195`)
  and the buffer up by pointer (`:2212`), so a shared name would diverge the
  two. `IRBuilder::register_value` (`spirv_ir_builder.cpp:1300-1309`) raises
  `TI_ERROR("{} already exists.")` on any duplicate non-constant name, so a
  collision aborts codegen before `at_buffer` is reached. Each `TaskCodegen`
  holds its own `IRBuilder` (`:97`) and its own map (`:2635`).
- **SPIR-V `Value` aliasing.** Real, and irrelevant. **This is why the map is a
  different animal from the `ValueKind` tag.** The map is keyed on the
  *statement*, so arithmetic that mints a fresh SPIR-V id and resets the tag
  does not touch it — W1/W2/W3 copy the origin's entry onto the new statement
  explicitly. The map tracks provenance across arithmetic; the tag cannot.
- **Out-of-order visitation.** `gen_array_range` (`:1964-1979`) visits ahead of
  block order and records statements in `offload_loop_motion_` so `visit(Block)`
  skips them (`:164`). Def-before-use for the map still holds, and
  `check_value_existence` (`:1970`) prevents the double visit that would trip
  `TI_ASSERT(ptr_to_buffers_.count(stmt) == 0)` at `:376`.

### 3.8 Grading

**ARCHITECTURAL.** Applying section 10 item 8's test: if every driver, hardware
generation and specification were ideal today, the obstacle would still be
there. It exists because of a decision inside this codebase — the SPIR-V value
model encodes addressing mode in scalar width and nowhere else.

**Blast radius, concretely.** Three predicate sites (`:2197`, `:2227`, `:2249`)
plus three fabricating writes (`:319`, `:326`, `:332`), all in
`taichi/codegen/spirv/spirv_codegen.cpp`. One file, six lines of decision. Both
inputs the rule needs are already members of the same class. No new plumbing,
no new type, no change to `BufferInfo` for the shelved case. That is materially
smaller than "a change to that backend's value model" as section 8.2 of the plan
currently records the converged adversarial finding.

## 4. Inferred, not verified

Marked separately per section 10 item 7. Each of these is a reachability or
pass-ordering claim that a citation cannot certify.

- **I1.** That `FuseMatrixPtr` always runs before SPIR-V codegen. I read the
  pass, not an execution. The consequence is soft either way: if it did not run,
  codegen aborts at `TI_NOT_IMPLEMENTED` (`:328`) rather than producing a wrong
  answer.
- **I2.** That `GetRootStmt` and `SNodeLookupStmt` never occupy one of the three
  operand slots. The evidence is that `.at()` at `:2212` would throw
  `std::out_of_range` for them today, and no such crash is reported. That is an
  argument from absence over exercised paths, not a proof.
- **I3.** That an `AtomicOpStmt` with a shared-array dest can reach `:1617`,
  `:1626` or `:1681`. Those three lack the `dest_is_ptr` guard that `:1621`,
  `:1630` and `:1633` have. Whether the front end emits such a program is a
  front-end question, and section 1.2 puts the front end out of scope. See E2.
- **I4.** That W3 never receives an origin outside the map's key classes. In the
  ordinary case `lower_access.cpp:134-143` sets the origin to `lowered.back()`,
  which `lower_ptr` returns as a place-`GetChStmt` (`:273`/`:276`, `return last`
  at `:280`), so W4 has written a genuine `Root`. I could not construct a
  program giving W3 an `SNodeLookupStmt` or non-place `GetChStmt` origin, and I
  did not prove none exists.

## 5. Cannot establish

- Whether item 6.2's intended design physically addresses the root buffer. That
  decides whether the map+capability rule is sufficient or collapses, and it is
  a design question the brief forbids me to answer. It is the single largest
  determinant of this item's cost and it is **not** settled by anything I read.
- Whether W7 is reachable at all. See E3.

## 6. Escalations

**E1 — `operator[]` on the right-hand side fabricates map entries.**
`:319`, `:326`, `:332`. Verified by compilation: a missing key yields
`{BufferType::Root, root_id = {-1}}`. W1's right-hand side fabricates every
time, because nothing ever writes an `AllocaStmt` key. If such an entry were
consumed, `get_buffer_value` (`:2266-2315`) falls to its default tail at
`:2304-2314`, takes a fresh binding and declares an SSBO named **`root_-1`**
(`buffer_instance_name`, `Root` case), which `get_buffer_binds()` then hands to
`task_attribs_.buffer_binds`. Silent wrong binding, not a crash. This is a live
defect independent of item 6.2, and it is the reason every prior "does the map
have an entry" argument is void. **I have changed no source file.**

**E2 — three `at_buffer` calls lack the `dest_is_ptr` guard.**
`:1617` (f64 add with `spirv_has_atomic_float64_add`), `:1626` (f32 add with
`spirv_has_atomic_float_add`), `:1681` (integral mul). Their siblings at
`:1621`, `:1630` and `:1633` have it. A shared-array dest reaching one of them
carries `stype.flag == kPtr` and `stype.dt == PrimitiveType::unknown`
(`taichi/ir/type.cpp:20`), so it fails `:2197`, then fails `is_integral` at
`:2207` and raises the "not integeral" `TI_ERROR`. A clean hard error, not a
crash, but it is a capability-dependent failure: the same program compiles on a
device without the atomic-float capability and fails on one with it. That is
hardware changing what the system can do, which is the section 2.2 test.
Reachability is I3.

**E3 — W7 may be dead, or may be a latent bug.** `:800` writes
`BufferType::Args` for an `ExternalPtrStmt` whose arg is not an array. But the
capability branch at `:783-791` is **not** guarded on `is_array` and reads
args-struct field 1, which is `DATA_PTR_POS_IN_NDARRAY`
(`taichi/ir/type_factory.h:69`) — a field that exists because the arg is an
ndarray. If a non-array `ExternalPtrStmt` is reachable with the capability on,
that access chain reads a field that need not be there. Either W7 is
unreachable, in which case a future reader should be told, or it is a defect.
I could not settle it and did not try to, being outside my question.

**E4 — the rule in section 1 is a proxy, and the plan should record it as one.**
It reads buffer identity and device capability and infers addressing mode. That
holds today only because exactly one buffer class is physically addressed. It is
sufficient to switch on `use_64bit_pointers` and close the width collision. It
is **not** a general answer, and if item 6.2 later addresses the root buffer
physically, this investigation must be redone. I am flagging this rather than
designing around it, per the brief and section 10 item 2.

## 7. What I did not do

I designed no remedy, per the brief. I modified no file except this report and
`notes-07b-discriminator.md`. I ran no git command that writes. I opened no file
in `modernization/investigation/` other than my own two and the project plan.
I did not re-litigate the `ValueKind` finding; section 3.5 records `SType::flag`
as a distinct thing from `Value::flag` and does not disturb the settled result.
