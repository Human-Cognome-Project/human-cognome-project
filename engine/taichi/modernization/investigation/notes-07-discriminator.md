# Notes 07 (pass A) — `at_buffer` discriminator

Contemporaneous. Written as I go. Plan version worked against: `modernization/PROJECT-PLAN.md`
"Last updated: 2026-09-09" (line 5).

---

## N1. The question restated in mechanical terms

`at_buffer` at `taichi/codegen/spirv/spirv_codegen.cpp:2194-2219`:

```cpp
spirv::Value at_buffer(const Stmt *ptr, DataType dt) {
  spirv::Value ptr_val = ir_->query_value(ptr->raw_name());

  if (ptr_val.stype.dt == PrimitiveType::u64) {        // <-- THE DISPUTED PREDICATE (2197)
    spirv::Value paddr_ptr = ir_->make_value(
        spv::OpConvertUToPtr, ...);
    paddr_ptr.flag = ValueKind::kPhysicalPtr;
    return paddr_ptr;
  }
  TI_ERROR_IF(!is_integral(ptr_val.stype.dt), ...);
  spirv::Value buffer = get_buffer_value(ptr_to_buffers_.at(ptr), dt);  // <-- 2212
  ... shift-right-by-log2(width), struct_array_access ...
}
```

Line 2197 is the predicate. Line 2212 is `ptr_to_buffers_.at(ptr)` — one statement below,
as the assignment says.

**First structural fact, and it changes the shape of the whole question.**
`ptr_to_buffers_` is declared at `spirv_codegen.cpp:2635`:

```cpp
std::unordered_map<const Stmt *, BufferInfo> ptr_to_buffers_;
```

The key is `const Stmt *` — a **Taichi IR statement pointer**, not a `spirv::Value` and not a
SPIR-V result id. `at_buffer`'s parameter is also `const Stmt *ptr`. So the lookup at 2212 is
keyed on exactly the same object the predicate at 2197 is asked about (2197 asks about the
spirv value *registered under that statement's name*, but the statement is the same one).

Consequence: the "values derived by arithmetic from a mapped value" hazard in my brief is
**not** the hazard it is for `ValueKind`. `ValueKind` lives on a `spirv::Value`, which every
`make_value` resets. `ptr_to_buffers_` lives on a `Stmt*`, and a `Stmt*` is not produced by
arithmetic — it is produced by the IR. Arithmetic in the SPIR-V builder cannot manufacture a
new `Stmt*`. So the failure mode has to be different: it must be a Stmt that reaches
`at_buffer` with NO entry, or with the WRONG entry propagated from an origin.

I have to test exactly that, and not assume it.

## N2. Writers of `ptr_to_buffers_` — raw grep, before analysis

`grep -n "ptr_to_buffers_" taichi/codegen/spirv/spirv_codegen.cpp`:

```
319:          ptr_to_buffers_[stmt] = ptr_to_buffers_[stmt->origin];
326:        ptr_to_buffers_[stmt] = ptr_to_buffers_[stmt->origin];
332:      ptr_to_buffers_[stmt] = ptr_to_buffers_[stmt->origin];
376:      TI_ASSERT(ptr_to_buffers_.count(stmt) == 0);
377:      ptr_to_buffers_[stmt] = BufferInfo(BufferType::Root, root);
711:    ptr_to_buffers_[stmt] = BufferType::GlobalTmps;
798:      ptr_to_buffers_[stmt] = {BufferType::ExtArr, arg_id};
800:      ptr_to_buffers_[stmt] = BufferType::Args;
2212:    spirv::Value buffer = get_buffer_value(ptr_to_buffers_.at(ptr), dt);
2635:  std::unordered_map<const Stmt *, BufferInfo> ptr_to_buffers_;
```

That is the WHOLE tree, not just this file — `grep -rn ptr_to_buffers_ taichi/` returns only
`spirv_codegen.cpp`. The member is private to the anonymous-namespace `TaskCodegen` class.
So: 7 write sites, 1 count site, 1 read site, 1 declaration. To be checked one at a time.

---

## N3. The seven writers, read one at a time

Text quoted from `taichi/codegen/spirv/spirv_codegen.cpp`.

**W1 — `MatrixPtrStmt`, shared-alloca branch, line 319** (visit at :307):
```cpp
if (stmt->offset_used_as_index()) {
  if (stmt->origin->is<AllocaStmt>()) {
    ... ptr_val = OpAccessChain(ptr_type, origin_val, offset_val);   // :320-322
    if (stmt->origin->as<AllocaStmt>()->is_shared) {
      ptr_to_buffers_[stmt] = ptr_to_buffers_[stmt->origin];         // :319 (318-319)
    }
```
Writes: *whatever `ptr_to_buffers_[origin]` holds*, where origin is an `AllocaStmt`.

**W2 — `MatrixPtrStmt`, global-temporary branch, line 326**: origin is a `GlobalTemporaryStmt`.

**W3 — `MatrixPtrStmt`, offset-as-bytes branch, line 332**: origin is unconstrained by the code.

**W4 — `GetChStmt`, line 377** (visit at :359):
```cpp
if (out_snode->is_place()) {
  TI_ASSERT(ptr_to_buffers_.count(stmt) == 0);      // :376
  ptr_to_buffers_[stmt] = BufferInfo(BufferType::Root, root);   // :377
}
```
Writes `BufferType::Root` with `root = snode_to_root_.at(stmt->input_snode->id)` (:361).
**Only when the output SNode is a place.** Non-place `GetChStmt` gets NO entry.

**W5 — `GlobalTemporaryStmt`, line 711** (visit at :707): writes `BufferType::GlobalTmps`,
unconditionally, via the implicit one-arg `BufferInfo(BufferType)` constructor
(`kernel_utils.h:39-41`, marked `NOLINTNEXTLINE(google-explicit-constructor)`).

**W6/W7 — `ExternalPtrStmt`, lines 798 / 800** (visit at :734):
```cpp
if (ctx_attribs_->arg_at(arg_id).is_array) {
  ptr_to_buffers_[stmt] = {BufferType::ExtArr, arg_id};   // :798
} else {
  ptr_to_buffers_[stmt] = BufferType::Args;               // :800
}
```
`arg_id` is `argload->arg_id`, a `std::vector<int>` (:738), so the vector-taking constructor
at `kernel_utils.h:46-48` is selected.

**This settles the brief's premise as stated, and adds a third value.** The two sides do
hold different values, `Root` at :377 and `ExtArr` at :798. But there are FIVE distinct
values reachable, not two: `Root`, `GlobalTmps`, `ExtArr`, `Args`, and a fifth that nobody
has named yet — see N4.

## N4. **W1 always inserts a value-initialised `BufferInfo`. Verified.**

`AllocaStmt` is not a writer of `ptr_to_buffers_`. It appears in no line of the grep in N2.
So at :319 the right-hand side `ptr_to_buffers_[stmt->origin]` is an `operator[]` on a key
that is **never present**, which default-inserts and returns a value-initialised `BufferInfo`.

`BufferInfo` (`kernel_utils.h:35-59`) has `BufferType type;` with **no default member
initialiser**, `std::vector<int> root_id{-1}`, `BufferInfo() = default;` declared on its
first declaration (so not user-provided), and three further user-provided constructors (so
the class is not an aggregate). Value-initialisation of such a class zero-initialises the
object and then runs the non-trivial default constructor.

Compiled and run rather than reasoned about
(`scratchpad/bi.cpp`, faithful replica of `kernel_utils.h:27-61`, `g++ -std=c++17 -O2`):

```
missing-origin read: type=0 root_id.size=1 root_id[0]=-1
map now has 1 entries (operator[] inserted)
is_aggregate=0 default_ctor_trivial=0
  trial 0..4: type=0
```

`BufferType::Root` is the first enumerator (`kernel_utils.h:22`), so `type=0` **is**
`BufferType::Root`. So W1 stores `BufferInfo{Root, {-1}}` — a Root buffer with root id −1,
which is not a root that exists.

This is deterministic, not undefined: [dcl.init]/8 guarantees the zero-init. It is the
fifth value, and it is a **wrong** value that the map holds today.

## N5. Why the wrong value in N4 is not currently observed

`at_buffer` reads the map at :2212, but the shared-array pointer never gets that far.

`MatrixPtrStmt`'s value at :320-322 is `OpAccessChain` with type
`ir_->get_pointer_type(ir_->get_primitive_type(dt), origin_val.stype.storage_class)`.
`IRBuilder::get_pointer_type` (`spirv_ir_builder.cpp:407-424`) sets `id`, `flag=kPtr`,
`element_type_id`, `storage_class` — and **never sets `dt`**. `SType::dt` is a `DataType`
member (`spirv_ir_builder.h:53`) whose default constructor is
`DataType::DataType() : ptr_(PrimitiveType::unknown.ptr_)` (`taichi/ir/type.cpp:20`).

So for that value `ptr_val.stype.dt == PrimitiveType::unknown`. In `at_buffer`:
- `:2197` `== PrimitiveType::u64` → false.
- `:2207-2210` `TI_ERROR_IF(!is_integral(...))` → `is_integral(unknown)` is false
  (`taichi/ir/type_utils.h:103-113` lists no `unknown`), so **`at_buffer` throws** before
  ever reaching :2212.

The bad map value is therefore unreachable **only because the error check at :2207 stands
between the predicate at :2197 and the map read at :2212.** Any change that moves a map
consultation ABOVE that error check exposes it. That is a direct constraint on item 6.2 and
I am recording it as such.

## N6. Where a shared `MatrixPtrStmt` does reach `at_buffer`

`AtomicOpStmt` (visit at :1540) computes at :1611:
```cpp
const bool dest_is_ptr = dest_val.stype.flag == TypeKind::kPtr;
```
with the comment "Shared arrays have already created an accesschain, use it directly."
It guards three call sites and **not** three others:

| line | expression | guarded by `dest_is_ptr`? |
|---|---|---|
| 1617 | `at_buffer(stmt->dest, dt)` — f64, native `atomic_float64_add` | NO |
| 1621 | `dest_is_ptr ? dest_val : at_buffer(...)` — f64 fallback | yes |
| 1626 | `at_buffer(stmt->dest, dt)` — f32, native `atomic_float_add` | NO |
| 1630 | `dest_is_ptr ? dest_val : at_buffer(...)` — f32 fallback | yes |
| 1633 | `dest_is_ptr ? dest_val : at_buffer(...)` — all other dt | yes |
| 1681 | `at_buffer(stmt->dest, ...)` — integral `AtomicOpType::mul` | NO |

Three unguarded. Derived by reading the six lines, not by counting a summary.
`load_buffer` (:2222) and `store_buffer` (:2244) have no `dest_is_ptr` escape at all.

---

## N7. Upstream already used a Stmt-type test at this decision point

`git log -L 2194,2220:taichi/codegen/spirv/spirv_codegen.cpp` (read-only) gives two commits.

`715a04c98` "[vulkan] Fix shared memory atomic float operations (#8315)", Bob Cao,
2023-08-15, is the one that created today's shape. It **removed** this:

```cpp
if (stmt->dest->is<MatrixPtrStmt>()) {
  // Shared arrays have already created an accesschain, use it directly.
  addr_ptr = ir_->query_value(stmt->dest->raw_name());
} else {
  addr_ptr = at_buffer(stmt->dest, dt);
}
```
and replaced it with the `dest_is_ptr` value-type test, and **added** the
`TI_ERROR_IF(!is_integral(...))` at :2207 in the same commit.

Two things follow that bear directly on my question:
1. The codebase has already discriminated at this exact point on `Stmt` identity
   (`stmt->dest->is<MatrixPtrStmt>()`). It is not a novel route.
2. The `!is_integral` error was added deliberately as the backstop for the shared-array
   case. My N5 conclusion is upstream's own stated intent, not my reconstruction:
   the PR body says "`at_buffer` will be called with wrong pointer dtype. Causing the
   codegen attempting to generate offset arithmetic with actual pointer type, which is
   invalid."

The same commit also changed `make_pointer`'s 64-bit arm from a `cast` of a u32 immediate to
a genuine `uint_immediate_number(u64_type(), offset)` (:2317-2324) — i.e. somebody was
maintaining the `use_64bit_pointers` arm in 2023 without switching it on.

The other commit, `06cb1c50d` "[vulkan] Disable buffer device address if int64 is not
supported (#4244)", predates it.

## N8. Key identity: can two statements collide in the map?

- Key type is `const Stmt *` (:2635). `TaskCodegen` is constructed fresh per offloaded task
  (`KernelCodegen::run`, :2720-2730: `TaskCodegen cgen(tp);` inside the task loop), so
  `ptr_to_buffers_` starts empty for each task.
- `TaskCodegen` does not mutate the IR. `grep -n "modifier\|erase(\|insert_before\|replace_with\|DelayedIRModifier" spirv_codegen.cpp` returns only two hits, both the words "width modifier"/"length modifier" in a printf diagnostic at :204 and :213. No statement is destroyed during codegen, so no address is recycled.
- **Conclusion: no key collision is possible.** Marked verified.

Separately, `at_buffer` looks the VALUE up by name, `ir_->query_value(ptr->raw_name())`
(:2195), where `raw_name()` is `fmt::format("tmp{}", id)` (`taichi/ir/ir.h:434-436`).
`Stmt::id` is unique within an IR tree, so the name table and the map cannot disagree about
which statement is meant. `IRBuilder::query_value` on a missing name is
`TI_ERROR("Value \"{}\" does not yet exist.")` (`spirv_ir_builder.cpp:1311-1317`) — a hard
error, not a silent default. That is the opposite of `ptr_to_buffers_`'s `operator[]`.

## N9. What actually reaches `at_buffer`

`at_buffer` is called from six places (`grep -n "at_buffer(\|load_buffer(\|store_buffer("`):
`:1617`, `:1621`, `:1626`, `:1630`, `:1633`, `:1681`, all inside `visit(AtomicOpStmt)`; plus
`load_buffer` at `:2233` and `store_buffer` at `:2257`, which are themselves called from
`visit(GlobalLoadStmt)` `:574` and `visit(GlobalStoreStmt)` `:568`. So the pointer argument
is always `GlobalLoadStmt::src`, `GlobalStoreStmt::dest`, or `AtomicOpStmt::dest`.

The SPIR-V pipeline always runs `lower_access`: `taichi/codegen/spirv/kernel_compiler.cpp:20`
passes `/*lower_global_access=*/true` to `irpass::offload_to_executable`, which gates the
call at `taichi/transforms/compile_to_offloads.cpp:257-262`. It is hard-coded there, not
read from `CompileConfig`. So no `GlobalPtrStmt` survives to this backend, which matches the
absence of any `visit(GlobalPtrStmt)` in the file (see the `visit(` listing).

`lower_access` replaces a `GlobalPtrStmt` with a chain whose last statement is always a
`GetChStmt` (`taichi/transforms/lower_access.cpp:268-278`, `handle_snode_at_level`), and
assigns it to `stmt->src` / `stmt->dest` / `stmt->origin` (`:123-142`, `:144-152`, `:178-186`).

So the reachable pointer statements are:

| # | Stmt reaching `at_buffer` | map entry | SPIR-V value type today |
|---|---|---|---|
| P1 | `GetChStmt`, `out_snode->is_place()` | `{Root, root}` (:377) | u32 (`make_pointer` :2317-2324 with `use_64bit_pointers=false`, added at :372) |
| P2 | `GlobalTemporaryStmt` | `GlobalTmps` (:711) | i32 immediate (:708-709) |
| P3 | `ExternalPtrStmt`, arg `is_array` | `{ExtArr, arg_id}` (:798) | **u64** if `caps_` has `spirv_has_physical_storage_buffer` (:783-792), else i32 (:794) |
| P4 | `ExternalPtrStmt`, arg **not** `is_array` | `Args` (:800) | **u64** on the same condition — the caps test at :783 is OUTSIDE the `is_array` test at :797 |
| P5 | `MatrixPtrStmt` over `GlobalTemporaryStmt` | propagated `GlobalTmps` (:326) | i32 (:323-325) |
| P6 | `MatrixPtrStmt` over a lowered `GetChStmt` | propagated `{Root, root}` (:332) | u32 (:331) |
| P7 | `MatrixPtrStmt` over a **shared** `AllocaStmt` | **`{Root, {-1}}`, never written by anyone** (:319, N4) | SPIR-V pointer, `stype.dt == unknown` |

Six rows, seven with P7. Derived by walking `visit(` one at a time, not from a summary.

Why nothing else: `offset_used_as_index()` (`taichi/ir/statements.h:521-529`) is true exactly
for origins `AllocaStmt`, `GlobalTemporaryStmt`, `ExternalPtrStmt`, `MatrixPtrStmt`. The
SPIR-V `visit(MatrixPtrStmt)` handles only the first two inside that branch and hits
`TI_NOT_IMPLEMENTED` at :328-329 for the other two. So `MatrixPtrStmt` over `ExternalPtrStmt`
and nested `MatrixPtrStmt` are **not supported at all** in this backend. The `else` arm at
:330-333 is therefore reached only for origins outside that four, which by
`MatrixPtrStmt::MatrixPtrStmt` (`taichi/ir/statements.cpp:104-131`) leaves `GlobalPtrStmt`
(lowered to `GetChStmt` before codegen), `GetChStmt`, `MatrixOfGlobalPtrStmt`,
`MatrixOfMatrixPtrStmt`, `ThreadLocalPtrStmt`, `AdStackLoadTopStmt`. The two `MatrixOf*`
forms are removed before offloading (`compile_to_offloads.cpp:71`, `:362`,
`transforms/lower_matrix_ptr.cpp`). `ThreadLocalPtrStmt` and `AdStackLoadTopStmt` have no
`visit` in this backend, so `ir_->query_value(stmt->origin->raw_name())` at :309 raises
before the map is touched. That leaves `GetChStmt`, which is P6.

## N10. P4 is the counterexample to the obvious rule

The obvious rule — "map value `ExtArr` means physical address, everything else means offset"
— is **wrong at P4**. Read :783-802 as one block: the `if (caps_->get(spirv_has_physical_storage_buffer))`
that decides whether a u64 device address or an i32 linear offset is registered is a
SEPARATE, EARLIER test from the `if (ctx_attribs_->arg_at(arg_id).is_array)` that decides
whether the map gets `ExtArr` or `Args`. A non-array arg on a physical-storage device gets
a u64 value and a `BufferType::Args` map entry.

Reachability of P4, honestly: I could not construct it. `ExternalPtrStmt` has two
construction sites, `taichi/ir/frontend_ir.cpp:702-706` (`make_ndarray_access`, always from
an `ExternalTensorExpression`, so the parameter is an ndarray and `ka.is_array` is true at
`taichi/codegen/spirv/kernel_utils.cpp:66`) and `taichi/ir/ir_builder.cpp:446-451`
(`IRBuilder::create_external_ptr`, used by `cpp_examples/` and `tests/cpp/`). The C++ builder
path takes any `ArgLoadStmt`, and `cpp_examples/autograd.cpp:160-161` builds one from
`create_arg_load({0}, PrimitiveType::f32, true, 0)` rather than
`create_ndarray_arg_load`, which is what `tests/cpp/ir/ndarray_kernel.cpp:8-9` uses. So the
shape exists in-tree; whether the resulting kernel parameter is registered as an array, and
whether such a kernel is ever compiled for SPIR-V, I have not established. **P4 is a
counterexample I can exhibit in the code and cannot exhibit at runtime.** Recorded as
unresolved rather than dismissed — a scope ruling would remove work, not this evidence
(standing instruction 9).

Note also that the `indices.push_back(1)` at :784-785 is the ndarray data-pointer slot
(`TypeFactory::DATA_PTR_POS_IN_NDARRAY`, cf. `kernel_utils.cpp:69-73`), so the physical
branch is written for array args only. That is evidence P4 is dead, not proof.

## N11. What else is available at `:2197`

Enumerated by reading the enclosing scope, not from memory:

1. `ptr`, the `const Stmt *` itself. `ptr->is<ExternalPtrStmt>()`, `ptr->is<GetChStmt>()`,
   `ptr->ret_type` are all directly available. Precedent at N7.
2. `ptr_to_buffers_` (:2635), with the hygiene caveats of N4 and N10.
3. `caps_` (declared :2565, initialised at :86 from `params.caps`). No
   `caps_->set` anywhere under `taichi/codegen/spirv/`, and `KernelCodegen::run` passes
   `&params_.caps` unchanged per task (:2727), so it is constant for the whole compile.
4. `ptr_val.stype.flag == TypeKind::kPtr` — already used as a discriminator at :1611.
   It separates "already a SPIR-V pointer" from "an integer", not physical from offset.
5. `use_64bit_pointers`, the `const bool` member at :82.
6. `compiled_structs_`, `ctx_attribs_`, `arch_`, `ir_`.

So the inherited sentence "no discriminator exists at that decision point by any route" is
false on its face: at least items 1, 2 and 3 are in scope at that line.

## N12. Where the map does NOT work — the honest list

1. **P7 / N4.** The map holds `{Root, {-1}}` for shared-array `MatrixPtrStmt`, written by
   nobody, produced by `operator[]` on an absent `AllocaStmt` key. It is currently
   unreachable ONLY because `TI_ERROR_IF(!is_integral(...))` at :2207 sits between the
   predicate at :2197 and the map read at :2212. A map consultation placed at :2197 is
   ABOVE that guard and would read the garbage.
2. **P4 / N10.** `BufferType` alone mis-describes a non-array `ExternalPtrStmt`, which today
   is given a physical u64 value and an `Args` entry. Reachability unresolved.
3. **`operator[]` pollution.** Lines 319, 326 and 332 read the origin with `operator[]`, so
   each of them INSERTS an entry for the origin statement as a side effect. After codegen,
   `ptr_to_buffers_.count(origin)` is 1 for statements that were never assigned a buffer.
   Any future "does this statement have an entry" test is therefore unsound on this map.
   Note the existing `TI_ASSERT(ptr_to_buffers_.count(stmt) == 0)` at :376 relies on exactly
   that kind of test; it survives only because no `GetChStmt` is ever a `MatrixPtrStmt`
   origin's lookup key before its own visit.
4. **Non-place `GetChStmt`.** :375-378 writes only when `out_snode->is_place()`. The
   bit-vectorised arm of `lower_access` (`lower_access.cpp:271-274`) can make the chain's
   last `GetChStmt` target a `quant_array`. If that reached `at_buffer`, `.at()` would
   throw `std::out_of_range` — loud, not silent. Whether quant/bit-vectorised code ever
   reaches this backend I did not establish; there is no `BitStructStoreStmt` visit in the
   file, which suggests not.
5. **`BufferType::Args` at :800 taking the offset path.** `at_buffer`'s offset path ends in
   `ir_->struct_array_access(..., buffer, idx_val)`, and `get_buffer_value(Args, ...)`
   returns `args_buffer_value_` (:2278-2280), built by `uniform_struct_argument` /
   `buffer_struct_argument` (`spirv_ir_builder.cpp:664-732`), both of which tag it
   `ValueKind::kStructArrayPtr` (:692, :721). So the `TI_ASSERT` at
   `spirv_ir_builder.cpp:773` passes, and the codegen emits an `OpAccessChain` into the args
   STRUCT with a runtime index. That is not valid SPIR-V for a struct member. Same P4
   reachability caveat.

## N13. Aliasing, in the sense the brief meant it

The brief warned about "values derived by arithmetic from a mapped value". For `ValueKind`
that is fatal, because the tag rides on a `spirv::Value` that every `make_value` recreates.
For `ptr_to_buffers_` it is **not the same hazard**, because the key is a `Stmt *` and
arithmetic in the SPIR-V builder never manufactures a `Stmt *`. Every pointer arithmetic
in the backend that changes buffer meaning is itself an IR statement with its own `Stmt *`,
and each of the three such statements (`GetChStmt`, `SNodeLookupStmt`, `MatrixPtrStmt`)
is visited by name.

The residual hazard is different and is item 3 of N12: propagation is done by
`ptr_to_buffers_[stmt] = ptr_to_buffers_[stmt->origin]`, which cannot fail loudly. Where a
`ValueKind` would be silently reset to `kNormal`, a `BufferInfo` is silently set to
`{Root, {-1}}`. Both are silent; they are silent about different things.

`SNodeLookupStmt` (:468-511) registers a value (:510) and writes NO map entry, and its value
is `ir_->add(parent_val, offset)` — pure arithmetic on a Root pointer. It never reaches
`at_buffer` because a `GetChStmt` is always interposed (N9), but it is the clearest case of a
derived pointer with no entry, and it is one IR-shape change away from mattering.
