# Report 07 (pass A) — Can `ptr_to_buffers_` carry the `at_buffer` discrimination?

**Plan version worked against:** `modernization/PROJECT-PLAN.md`, "Last updated: 2026-09-09".
Re-read at the end of the pass per standing instruction 6. It had changed under me during
the pass: it grew from 41232 to 45208 bytes and gained a new section **2.2b, "How to cut a
backend gap"**. I re-read 2.2a, 2.2b, 6.2, 8.2 and 10 against the version I started from.
Nothing in the new material touches this assignment. Section 2.2a still puts item 6.2 in
scope as BASE work, and section 8.2 item 0 still carries the converged finding about the
value tag that my brief told me not to re-litigate. My contemporaneous notes are at
`modernization/investigation/notes-07-discriminator.md`.

**All line numbers are `taichi/codegen/spirv/spirv_codegen.cpp` unless another file is named.**

---

## 1. The answer, up front

**Yes, a discriminator exists at that point, and the inherited sentence is false.**

At line 2197 — the disputed predicate `if (ptr_val.stype.dt == PrimitiveType::u64)` — three
independent things are in scope that distinguish a physical device address from a
buffer-relative offset, and none of them is a function of the pointer's scalar width:

1. `ptr`, the `const Stmt *` argument itself. `ptr->is<ExternalPtrStmt>()` is directly
   testable. Upstream already discriminated at this exact site on statement identity, with
   `stmt->dest->is<MatrixPtrStmt>()`, until commit `715a04c98` replaced it in August 2023.
2. `ptr_to_buffers_`, the map, read one statement below at :2212.
3. `caps_->get(DeviceCapability::spirv_has_physical_storage_buffer)`, which is constant for
   the whole compile and is the same flag that decided, at :783, whether the value being
   discriminated is an address at all.

**But the map alone does not carry it, and the naive rule is wrong.** "Map value is
`BufferType::ExtArr` means physical" fails on a case I can exhibit in the source and could
not exhibit at runtime, and the map holds a wrong value today for a second case that is
currently masked by an error check standing between the predicate and the map read.

The honest formulation of what is true:

> `ptr_to_buffers_` carries the discrimination correctly for every path I could establish
> reaches `at_buffer`, EXCEPT the non-array `ExternalPtrStmt` branch at :800, whose
> reachability I could not settle either way. `caps_` is required alongside it, because
> `BufferType::ExtArr` means physical on one device and offset on another. And the map is
> not a sound *existence* oracle, because three of its seven writers insert entries as a
> side effect of reading.

That is a qualified yes, not a clean one. A wrong yes being worse than an honest limit, the
qualifications are set out in full in sections 4 and 5 and I have not smoothed them.

---

## 2. VERIFIED — every writer of `ptr_to_buffers_`, and what it writes

`grep -rn "ptr_to_buffers_"` over `taichi/`, `tests/` and `cpp_examples/` returns hits in
exactly one file. The member is private to the anonymous-namespace class `TaskCodegen`,
declared at :2635 as `std::unordered_map<const Stmt *, BufferInfo> ptr_to_buffers_;`.
Ten hits: seven writes, one `count`, one read, one declaration.

| id | line | statement visited | value written |
|---|---|---|---|
| W1 | 319 | `MatrixPtrStmt`, origin is a **shared** `AllocaStmt` | `ptr_to_buffers_[origin]` — see §3, this is always the value-initialised `{Root, {-1}}` |
| W2 | 326 | `MatrixPtrStmt`, origin is `GlobalTemporaryStmt` | propagated `GlobalTmps` |
| W3 | 332 | `MatrixPtrStmt`, offset-as-bytes arm | propagated from origin |
| W4 | 377 | `GetChStmt`, guarded by `out_snode->is_place()` at :375 | `BufferInfo(BufferType::Root, root)`, `root` from `snode_to_root_.at(stmt->input_snode->id)` at :361 |
| W5 | 711 | `GlobalTemporaryStmt`, unconditional | `BufferType::GlobalTmps` |
| W6 | 798 | `ExternalPtrStmt`, arg `is_array` | `{BufferType::ExtArr, arg_id}`, `arg_id` a `std::vector<int>` from :738 |
| W7 | 800 | `ExternalPtrStmt`, arg not `is_array` | `BufferType::Args` |

Seven rows, counted from the seven grep hits that are assignments. The single `count` is the
`TI_ASSERT(ptr_to_buffers_.count(stmt) == 0)` at :376. The single read is :2212.

**The brief's premise holds and is understated.** `GetChStmt` does write `Root` at :377 and
`ExternalPtrStmt` does write `ExtArr` at :798. But five distinct values are reachable, not
two: `Root`, `GlobalTmps`, `ExtArr`, `Args`, and the value-initialised `{Root, {-1}}` that
no writer intends.

---

## 3. VERIFIED — the map holds a value nobody wrote

`AllocaStmt` is not among the seven writers. So at :319 the right-hand side
`ptr_to_buffers_[stmt->origin]` is `operator[]` on a key that is never present. That
default-inserts and returns a **value-initialised** `BufferInfo`.

`BufferInfo` is at `taichi/codegen/spirv/kernel_utils.h:35-59`. It has `BufferType type;`
with no default member initialiser, `std::vector<int> root_id{-1}`, `BufferInfo() = default;`
declared on its first declaration, and three further user-provided constructors. So its
default constructor is not user-provided and is non-trivial, and the class is not an
aggregate. Value-initialisation zero-initialises the object and then runs the default
constructor.

I compiled and ran a faithful replica rather than reasoning about it
(`g++ -std=c++17 -O2`, replica of `kernel_utils.h:27-61`):

```
missing-origin read: type=0 root_id.size=1 root_id[0]=-1
map now has 1 entries (operator[] inserted)
is_aggregate=0 default_ctor_trivial=0
  trial 0..4: type=0
```

`BufferType::Root` is the first enumerator at `kernel_utils.h:22`, so `type=0` **is** `Root`.
W1 therefore stores `BufferInfo{Root, {-1}}`: a root buffer whose root id is −1. This is
deterministic, not undefined behaviour.

**Why it is not observed today.** The value that W1's `MatrixPtrStmt` registers is an
`OpAccessChain` typed by `ir_->get_pointer_type(...)` at :320-322.
`IRBuilder::get_pointer_type` (`spirv_ir_builder.cpp:407-424`) never assigns `SType::dt`, and
`DataType`'s default constructor is `DataType::DataType() : ptr_(PrimitiveType::unknown.ptr_)`
(`taichi/ir/type.cpp:20`). So inside `at_buffer` that value's `stype.dt` is `unknown`:
the u64 test at :2197 is false, and then
`TI_ERROR_IF(!is_integral(ptr_val.stype.dt), ...)` at :2207-2210 fires, because
`is_integral` (`taichi/ir/type_utils.h:103-113`) does not admit `unknown`.

**The bad value is unreachable only because that error check sits BETWEEN the predicate at
:2197 and the map read at :2212.** This is not my reconstruction. Commit `715a04c98`,
"[vulkan] Fix shared memory atomic float operations (#8315)", added that exact check in the
same change that introduced the `dest_is_ptr` escapes, and its message says `at_buffer` "will
be called with wrong pointer dtype. Causing the codegen attempting to generate offset
arithmetic with actual pointer type, which is invalid."

**Direct consequence for item 6.2, and the single most important line in this report:** any
change that consults `ptr_to_buffers_` at :2197, i.e. ABOVE that guard, moves the shared-array
`MatrixPtrStmt` from a loud compile error onto the `{Root, {-1}}` path. `buffer_instance_name`
at :43-67 would name it `Root_-1` and `get_buffer_value` at :2266-2316 would mint a fresh
descriptor binding for a buffer that no runtime ever binds. Silent wrong memory, not a crash.

---

## 4. VERIFIED — every path a pointer takes into `at_buffer`

`at_buffer` is reached from six call sites, all in `visit(AtomicOpStmt)` (:1617, :1621,
:1626, :1630, :1633, :1681), plus `load_buffer` (:2233) and `store_buffer` (:2257), which are
called only from `visit(GlobalLoadStmt)` at :574 and `visit(GlobalStoreStmt)` at :568. So the
pointer is always `GlobalLoadStmt::src`, `GlobalStoreStmt::dest` or `AtomicOpStmt::dest`.

The SPIR-V pipeline always lowers global access: `taichi/codegen/spirv/kernel_compiler.cpp:20`
passes `/*lower_global_access=*/true` as a literal, and that gates
`irpass::lower_access` at `taichi/transforms/compile_to_offloads.cpp:257-262`. It is not read
from `CompileConfig`. `lower_access` ends every lowered chain in a `GetChStmt`
(`taichi/transforms/lower_access.cpp:268-278`) and rewrites `src`/`dest`/`origin` to it
(`:123-142`, `:144-152`, `:178-186`). Consistently, this backend has no `visit(GlobalPtrStmt)`.

| # | statement reaching `at_buffer` | map entry at that moment | value type today |
|---|---|---|---|
| P1 | `GetChStmt` with `out_snode->is_place()` | `{Root, root}` | u32 |
| P2 | `GlobalTemporaryStmt` | `GlobalTmps` | i32 |
| P3 | `ExternalPtrStmt`, arg `is_array` | `{ExtArr, arg_id}` | **u64** with the physical-storage cap, else i32 |
| P4 | `ExternalPtrStmt`, arg not `is_array` | `Args` | **u64** on the same condition |
| P5 | `MatrixPtrStmt` over `GlobalTemporaryStmt` | `GlobalTmps` | i32 |
| P6 | `MatrixPtrStmt` over a lowered `GetChStmt` | `{Root, root}` | u32 |
| P7 | `MatrixPtrStmt` over a shared `AllocaStmt` | `{Root, {-1}}` (§3) | SPIR-V pointer, `dt` unknown |

Seven rows, derived by walking each `visit` in turn, not from a summary. Types are as of
`use_64bit_pointers = false` at :82.

**Why the list closes.** `MatrixPtrStmt::offset_used_as_index()`
(`taichi/ir/statements.h:521-529`) is true exactly for origins `AllocaStmt`,
`GlobalTemporaryStmt`, `ExternalPtrStmt`, `MatrixPtrStmt`. The SPIR-V visitor handles the
first two inside that branch and hits `TI_NOT_IMPLEMENTED` at :328-329 for the other two, so
`MatrixPtrStmt` over `ExternalPtrStmt` and nested `MatrixPtrStmt` are **not supported in this
backend at all**. The else arm at :330-333 therefore sees only origins outside that four,
which by `MatrixPtrStmt::MatrixPtrStmt` (`taichi/ir/statements.cpp:104-131`) leaves
`GlobalPtrStmt` (already lowered), `GetChStmt`, `MatrixOfGlobalPtrStmt`,
`MatrixOfMatrixPtrStmt`, `ThreadLocalPtrStmt`, `AdStackLoadTopStmt`. The two `MatrixOf*`
forms are removed before offloading (`compile_to_offloads.cpp:71`, `:362`;
`taichi/transforms/lower_matrix_ptr.cpp`). `ThreadLocalPtrStmt` and `AdStackLoadTopStmt` have
no `visit` here, so `ir_->query_value(stmt->origin->raw_name())` at :309 raises first —
`IRBuilder::query_value` on a missing name is a hard `TI_ERROR`
(`spirv_ir_builder.cpp:1311-1317`).

**Key collision is impossible.** `TaskCodegen` is constructed fresh per offloaded task
(`KernelCodegen::run`, :2720-2730) so the map starts empty each time, and `TaskCodegen` does
not mutate the IR — grepping the file for `modifier`, `erase(`, `insert_before`,
`replace_with` and `DelayedIRModifier` returns only the words "width modifier" and "length
modifier" in a printf diagnostic at :204 and :213. No `Stmt` is destroyed during codegen, so
no address is recycled. Separately, the value lookup at :2195 is by
`raw_name()` = `fmt::format("tmp{}", id)` (`taichi/ir/ir.h:434-436`), and `Stmt::id` is unique
within a tree, so the value table and the map cannot disagree about which statement is meant.

---

## 5. Where it does NOT work

### 5.1 VERIFIED — `BufferType` alone is not the discriminator; `caps_` is required

`BufferType::ExtArr` means **physical on one device and buffer-relative on another**. Read
:783-802 as one block: whether a u64 device address or an i32 linear offset is registered is
decided at :783 by `caps_->get(DeviceCapability::spirv_has_physical_storage_buffer)`; the
`ExtArr` vs `Args` choice is a separate, later test at :797. Both `ExtArr` paths exist.

This is not a defect in the map, and it is benign, because the same `caps_` object answers
the same question at both points. `caps_` is a `DeviceCapabilityConfig *` member declared at
:2565 and initialised at :86 from `params.caps`; `KernelCodegen::run` passes `&params_.caps`
unchanged for every task (:2727); and there is no `caps_->set` anywhere under
`taichi/codegen/spirv/`. It is constant for the compile.

But it means the discriminator is **`BufferInfo` plus `caps_`, never `BufferInfo` alone.**
A report that says "the map carries it" without that conjunct is wrong.

### 5.2 VERIFIED IN SOURCE, REACHABILITY UNRESOLVED — P4 breaks the obvious rule

The rule "map value `ExtArr` means physical, everything else means offset" is **wrong for
P4**. Because the caps test at :783 is outside the `is_array` test at :797, a non-array
`ExternalPtrStmt` on a physical-storage device is given a u64 device address at :792 and a
`BufferType::Args` map entry at :800. Today's width predicate sends it down the physical path,
which is at least self-consistent. A rule keyed on `type == ExtArr` would send it down the
offset path and emit `struct_array_access` on the args buffer with a device address as the
index.

That downstream step is itself already broken for `Args`: `get_buffer_value(Args, ...)`
returns `args_buffer_value_` (:2278-2280), built by `uniform_struct_argument` or
`buffer_struct_argument` (`spirv_ir_builder.cpp:664-732`), both of which tag it
`ValueKind::kStructArrayPtr` (:692, :721). So the assertion at `spirv_ir_builder.cpp:773`
passes and the backend emits an `OpAccessChain` into the args **struct** with a runtime index,
which is not valid SPIR-V for a struct member.

**I could not establish whether P4 is reachable.** `ExternalPtrStmt` has two construction
sites. `taichi/ir/frontend_ir.cpp:702-706` always builds from an `ExternalTensorExpression`,
so `ka.is_array` is true at `taichi/codegen/spirv/kernel_utils.cpp:66`. `IRBuilder::create_external_ptr`
(`taichi/ir/ir_builder.cpp:446-451`) accepts any `ArgLoadStmt`, and in-tree callers split:
`tests/cpp/ir/ndarray_kernel.cpp:8-9` uses `create_ndarray_arg_load`, while
`cpp_examples/autograd.cpp:160-161` uses plain `create_arg_load`. Also, the
`indices.push_back(1)` at :785 is the ndarray data-pointer slot
(cf. `kernel_utils.cpp:69-73`), which is evidence the branch is dead, not proof.

Per standing instruction 9, a scope ruling would remove work but not this evidence. I am
recording P4 as an unresolved counterexample rather than dismissing it.

### 5.3 VERIFIED — the map is not a sound existence oracle

Lines 319, 326 and 332 read the origin with `operator[]`, so each **inserts** an entry for the
origin statement as a side effect. After a task is compiled, `ptr_to_buffers_.count(origin)`
is 1 for statements no writer ever assigned. Any test of the form "does this statement have an
entry" is therefore unsound on this map. The existing `TI_ASSERT(ptr_to_buffers_.count(stmt) == 0)`
at :376 is exactly such a test; it survives only because no `GetChStmt` is looked up as a
`MatrixPtrStmt` origin before its own visit runs.

**This is the specific failure my brief predicted.** The inherited sentence was reached by
checking whether the map "has an ENTRY on both sides". On this map, having an entry is not
evidence that anyone wrote one.

### 5.4 VERIFIED — non-place `GetChStmt` has no entry, and fails loudly

:375-378 writes only when `out_snode->is_place()`. The bit-vectorised arm of `lower_access`
(`lower_access.cpp:271-274`) can make a chain's last `GetChStmt` target a `quant_array`. If
such a statement reached `at_buffer`, `.at()` would throw `std::out_of_range`. That is loud,
not silent, so it is a lesser hazard. Whether quantised or bit-vectorised code reaches this
backend at all I did not establish; there is no `BitStructStoreStmt` visit in the file, which
suggests it does not.

### 5.5 VERIFIED — three unguarded `at_buffer` calls on shared arrays

`visit(AtomicOpStmt)` computes `dest_is_ptr` at :1612 with the comment at :1611 "Shared arrays have
already created an accesschain, use it directly." It guards three of the six `at_buffer` calls
and not the other three:

| line | context | guarded |
|---|---|---|
| 1617 | f64, native `spirv_has_atomic_float64_add`, `op == add` | no |
| 1621 | f64 fallback | yes |
| 1626 | f32, native `spirv_has_atomic_float_add`, `op == add` | no |
| 1630 | f32 fallback | yes |
| 1633 | all other dtypes | yes |
| 1681 | integral `AtomicOpType::mul` | no |

Six rows read one at a time; three unguarded. `load_buffer` (:2222) and `store_buffer` (:2244)
have no such escape at all. `git show 715a04c98` confirms the three were left unguarded
deliberately, in the same commit that added the `!is_integral` backstop. So on a device
advertising native float atomic add, an atomic add on a shared array reaches `at_buffer` and
errors out at :2207. That is today's behaviour, and it is the exact call chain that §3 says a
map consultation at :2197 would silently redirect.

---

## 6. INFERRED — consequences I did not test

Marked separately, per standing instruction 7.

1. **Replacing the predicate is necessary but not sufficient for item 6.2.** With
   `use_64bit_pointers = true` at :82, `make_pointer` (:2317-2324) returns u64 immediates, so
   `GetRootStmt` (:355), `SNodeLookupStmt` (:508) and `GetChStmt` (:372) all yield u64 Root
   offsets. A map-based predicate would correctly route them to the offset path — and that
   path then does `OpShiftRightLogical` on a u64 at :2214-2216 and feeds the result to
   `struct_array_access` at :2217-2218, which emits `OpAccessChain` with a 64-bit index
   operand. Whether that is accepted, and what it costs, I did not test. I am recording the
   dependency, not designing anything.
2. **P4 is probably dead**, on the `indices.push_back(1)` evidence in §5.2. Probably is not
   established.
3. **Quantised and bit-vectorised code probably does not reach this backend**, on the absence
   of any bit-struct visit. Same caveat.

---

## 7. Grading the obstacle, per standing instruction 8

**Test: if every external thing were ideal today, would the obstacle still be there? Yes.**

**ARCHITECTURAL.** The collision exists because `at_buffer` encodes "which address space a
pointer belongs to" in the pointer's scalar width, a representation decision inside this
codebase. No driver, hardware generation or specification version makes it go away.

**Blast radius, stated concretely and separately from the grade, in files and call sites.**
For the discrimination question alone, and NOT for the whole of item 6.2:

- One predicate: :2197.
- One map read already present one line below: :2212.
- Three side-effecting propagation writes that would have to be made honest before the map
  can be consulted above the `!is_integral` guard: :319, :326, :332.
- One default that silently produces `{Root, {-1}}`: `kernel_utils.h:36-37`.
- Three unguarded `at_buffer` calls whose behaviour changes if the guard order changes:
  :1617, :1626, :1681.

Eight sites, in two files, counted from the list above. This is smaller than "a change to
that backend's value model", which is what section 8.2 of the plan currently records as the
converged adversarial finding. **My result narrows that finding rather than contradicting
it**, and only for the discrimination question: the `ValueKind` route genuinely is dead, for
the reason my brief stated and which I did not re-litigate, but `ValueKind` is not the only
route, and the plan's sentence generalises from the tag to "any route" without warrant.

---

## 8. Escalations

Per standing instructions 2 and 3, raised rather than resolved.

**E1. The inherited sentence in four documents is false and should be corrected.** "No
discriminator exists at that decision point by any route" is contradicted by §1. The
defensible replacement is narrower and I am not authorised to write it into the plan: the
*value tag* cannot carry it, and the *map alone* cannot carry it, but the map together with
`caps_`, or the statement type together with `caps_`, can — with the P4 exception unresolved.

**E2. `BufferInfo` has no default for `type`, and `ptr_to_buffers_` reads origins with
`operator[]`.** §3 and §5.3. This is a live latent defect at `kernel_utils.h:36` and :319,
:326, :332, independent of item 6.2. I have modified no source file. Whether it is fixed, and
whether that fix precedes item 6.2, is the planner's call. It matters to item 6.2 because it
determines whether a map consultation can be placed above the :2207 guard.

**E3. Three `at_buffer` calls in `visit(AtomicOpStmt)` are unguarded against shared-array
destinations** (:1617, :1626, :1681), so `x[i] += v` on a shared array is a hard compile
error on any device advertising native float atomic add, and `x[i] *= v` on any device at
all. §5.5. Upstream left these unguarded knowingly in `715a04c98`. This is a pre-existing
defect I found while enumerating, not a consequence of item 6.2. I am not deciding it is
unnecessary and I am not fixing it.

**E4. P4's reachability needs someone who can run the compiler.** §5.2. It is the difference
between a clean answer and a qualified one, and it cannot be settled by reading. It needs a
SPIR-V compile of a kernel whose `ExternalPtrStmt` is built over a non-ndarray `ArgLoadStmt`
on a device with `spirv_has_physical_storage_buffer`. I did not build the tree; my brief did
not ask me to and building it would have consumed the pass.

**E5. Scope boundary I did not cross.** My brief said to establish what is true and not to
design a remedy. I have not proposed one, have not chosen between the map route and the
statement-type route, and have not written the sufficiency question in §6 item 1 as a plan.
That question sizes the rest of item 6.2 and is not answered here.

---

## 9. What my searches could and could not see

Per standing instruction 9's corollary.

- **Could see, mechanically:** every occurrence of the identifier `ptr_to_buffers_` in
  `taichi/`, `tests/` and `cpp_examples/`. The identifier is unique and unabbreviated, so
  this enumeration is complete. Same for `at_buffer`, `load_buffer`, `store_buffer`,
  `use_64bit_pointers`, `kPhysicalPtr` and `offset_used_as_index`.
- **Could see, by reading:** every `visit(` override in `spirv_codegen.cpp`, listed with a
  single anchored grep, so the set of statement kinds this backend handles is complete.
- **Could NOT see:** which of the enumerated paths any real kernel actually takes. Every
  reachability claim in this report is derived from the source, not from a run. I did not
  build the tree and did not execute a kernel. Where that mattered I said so, at P4 (§5.2),
  at the quantised case (§5.4), and in §6.
- **Could NOT see:** the other agent's files, per my brief, or any file in
  `modernization/investigation/` other than the two I was told to write.
