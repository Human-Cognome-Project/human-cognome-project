# Notes 07B — the `at_buffer` discriminator question

Agent: explore-07b-discriminator. Pass B of a blind pair.
Working against PROJECT-PLAN.md dated **2026-09-09** (read at start; will re-read before finalising per s.10.6).
Contemporaneous. Written as I go, appended, not reconstructed.

---

## 1. The predicate under dispute

`taichi/codegen/spirv/spirv_codegen.cpp:2204-2219`:

```cpp
spirv::Value at_buffer(const Stmt *ptr, DataType dt) {
    spirv::Value ptr_val = ir_->query_value(ptr->raw_name());

    if (ptr_val.stype.dt == PrimitiveType::u64) {        // :2206  <-- THE PREDICATE
      spirv::Value paddr_ptr = ir_->make_value(
          spv::OpConvertUToPtr, ...);
      paddr_ptr.flag = ValueKind::kPhysicalPtr;
      return paddr_ptr;
    }

    TI_ERROR_IF(!is_integral(ptr_val.stype.dt), ...);     // :2214

    spirv::Value buffer = get_buffer_value(ptr_to_buffers_.at(ptr), dt);  // :2212 (the `at`)
    ...
}
```

**FIRST STRUCTURAL FACT, and it changes the shape of the question.**
`ptr_to_buffers_` is declared at `:2635` as
`std::unordered_map<const Stmt *, BufferInfo> ptr_to_buffers_;`

It is keyed on the **`Stmt*`**, not on the SPIR-V `Value`, not on `raw_name()`.
`at_buffer` receives that same `const Stmt *ptr` as its first parameter. So the
map lookup at `:2212` is on exactly the same key the predicate at `:2206` is
deciding about. The two are not looking at different things: `:2206` looks at
the *SPIR-V type of the value registered under `ptr->raw_name()`*, and `:2212`
looks at the *statement identity*.

Consequence for the "keys collide" line of attack: `Stmt*` identity does not
collide the way a hash of a name or a value id could. Two distinct live
statements are distinct pointers. The collision risk is **stale reuse of a
freed `Stmt*` address**, which is a different and much narrower question. I
must check the map's lifetime against IR lifetime before I can dismiss it.

*(Line numbers in section 1 above were written before I ran a numbered listing.
Corrected here from `grep -n`: the predicate is at **`:2197`**, the
`TI_ERROR_IF` at **`:2207-2210`**, the map read at **`:2212`**, the function
opens at **`:2194`**, declaration of the map at **`:2635`**. Section 1's
":2204-2219 / :2206 / :2214" were wrong and are superseded.)*

---

## 2. Enumeration 1 — every writer of `ptr_to_buffers_`

Generated mechanically:
`grep -rn "ptr_to_buffers_" --include=*.cpp --include=*.h .`
Whole-tree, not scoped to the SPIR-V directory. Ten hits total, in ONE file,
`taichi/codegen/spirv/spirv_codegen.cpp`. Of those ten: 7 writes, 1 read
(`:2212`), 1 read-only count in an assert (`:376`), 1 declaration (`:2635`).

What this search CAN see: every textual occurrence of the identifier in any
`.cpp`/`.h` in the tree, including `external/`. What it CANNOT see: an alias
bound to the map (`auto &m = ptr_to_buffers_;`) used under another name. I
checked for that separately — there is no `= ptr_to_buffers_` binding among the
ten hits, and the only non-`ptr_to_buffers_`-spelled access would have to be
through a reference established on one of those ten lines. None of them do.
So the enumeration is closed.

| # | Line | Enclosing visitor | Guard | Value written |
|---|---|---|---|---|
| W1 | `:319` | `MatrixPtrStmt` | `offset_used_as_index()` && origin `is<AllocaStmt>()` && `origin->as<AllocaStmt>()->is_shared` | `ptr_to_buffers_[stmt->origin]` (copy) |
| W2 | `:326` | `MatrixPtrStmt` | `offset_used_as_index()` && origin `is<GlobalTemporaryStmt>()` | `ptr_to_buffers_[stmt->origin]` (copy) |
| W3 | `:332` | `MatrixPtrStmt` | `!offset_used_as_index()` (offset used as bytes) | `ptr_to_buffers_[stmt->origin]` (copy) |
| W4 | `:377` | `GetChStmt` | `out_snode->is_place()` | `BufferInfo(BufferType::Root, root)` |
| W5 | `:711` | `GlobalTemporaryStmt` | none | `BufferType::GlobalTmps` (implicit ctor) |
| W6 | `:798` | `ExternalPtrStmt` | `ctx_attribs_->arg_at(arg_id).is_array` | `{BufferType::ExtArr, arg_id}` |
| W7 | `:800` | `ExternalPtrStmt` | else of W6 | `BufferType::Args` (implicit ctor) |

Seven rows. Counted off the table, not asserted: W1..W7 = 7.

**Which `BufferType`s ever appear as a written value.** Directly: `Root` (W4),
`GlobalTmps` (W5), `ExtArr` (W6), `Args` (W7) — four. W1/W2/W3 write no new
value, they propagate. `BufferType` has seven enumerators
(`kernel_utils.h:23-31`: Root, GlobalTmps, Args, Rets, ListGen, ExtArr,
ArgPack). So `Rets`, `ListGen` and `ArgPack` are never written into this map —
they reach `get_buffer_value` only through direct calls, e.g. `:609`
(`{BufferType::ArgPack, indices_l}`), `:618` and `:753`
(`BufferType::Args`). That is 4 written + 3 never-written = 7. Consistent.

## 3. The single-argument `BufferInfo` constructor is implicit

`kernel_utils.h:39-41` carries a `// NOLINTNEXTLINE(google-explicit-constructor)`
and `BufferInfo(BufferType buffer_type) : type(buffer_type) {}`. So W5 and W7,
written as `= BufferType::GlobalTmps` and `= BufferType::Args`, are converting
constructions. They leave `root_id` at its NSDMI value `{-1}`
(`kernel_utils.h:35`), which is correct because `operator==` at `:51-59` only
compares `root_id` when `type` is `Root` or `ExtArr`.

This is the same *shape* of hazard section 9's corollary in the plan warns
about — a real site written with no width named on the line. Here it is a real
map write with no `BufferInfo` named on the line. My grep found them anyway
because I keyed on the map identifier, not on the type name. Stated so the
next reader knows the filter's reach.

---

## 4. VERIFIED BY COMPILATION — W1/W2/W3 fabricate map entries

W1, W2 and W3 are all written `ptr_to_buffers_[stmt] = ptr_to_buffers_[stmt->origin];`.
The **right-hand side uses `operator[]`, not `.at()`**. If the origin has no
entry, `operator[]` value-initialises one and inserts it, then the copy is made.

For W1 the origin is an `AllocaStmt`. **No writer in the enumeration of section
2 ever writes an `AllocaStmt` key.** The `AllocaStmt` visitor at `:284-303`
writes no map entry at all. So W1's right-hand side default-inserts **every
single time it executes**. It is not an edge case; it is the only case.

I did not want to argue this from the standard, so I compiled a verbatim
replica of `kernel_utils.h:23-49` and ran it. Source at
`/tmp/claude-1000/-opt-project-taichi/0b12bd28-d413-4ea3-b486-d56840a983ae/scratchpad/vi.cpp`.

```
missing-key operator[] -> type=0 root_id.size=1 root_id[0]=-1
BufferType::Root == 0
value-init BufferInfo{} -> type=0
default-init BufferInfo e -> type=6           (g++)
default-init BufferInfo e -> type=-1019198016 (clang++)
```

Identical under `g++ -O0`, `g++ -O2` and `clang++ -O2`. The arena was churned
with 4096 inserts of a non-zero `BufferInfo` and cleared first, so the zero is
not leftover fresh memory.

**Result.** A missing key read through `operator[]` yields
`{type = BufferType::Root, root_id = {-1}}`, deterministically. It is
well-defined, not UB: `BufferInfo() = default;` at `kernel_utils.h:37` is
user-*declared* but not user-*provided*, so value-initialisation
zero-initialises the object before running the (non-trivial, because of the
`root_id{-1}` NSDMI at `:35`) default constructor. `type` keeps the zero.
The contrast row — plain `BufferInfo e;` giving 6 and -1019198016 — is the
control showing the zero really comes from the zero-init step.

`BufferType::Root` is enumerator **0** (`kernel_utils.h:24`).

### 4.1 Three consequences, and they are the centre of this investigation

**C1. Entry existence is worthless as a signal.** `ptr_to_buffers_.count(p)`
returning 1 does not mean any writer wrote `p`. W1/W2/W3 manufacture entries
for keys nobody wrote, for BOTH `stmt` and `stmt->origin`. This is the precise
defect the brief says the inherited sentence was built on: four documents
checked *whether there is an entry*. There is always an entry once W1 has run,
including for the `AllocaStmt` that nothing ever described.

**C2. The fabricated value is `Root`, which is a real, meaningful enumerator.**
The fabrication is not distinguishable from a genuine W4 write by `type` alone.
It is distinguishable by `root_id`: W4 writes `BufferInfo(BufferType::Root, root)`
where `root` comes from `snode_to_root_.at(...)`, an index into
`compiled_structs_` filled at `:100-106` over `[0, compiled_structs_.size())`,
so a genuine root id is `>= 0`, while the fabricated one is `-1`. That
distinction is an accident of the NSDMI, not a design, and nothing in the file
relies on it or documents it.

**C3. The `-1` would be catastrophic if consumed.** `get_buffer_value` is
reached at `:2212` with whatever the map holds. I have not yet traced what
`get_buffer_value({Root, {-1}}, dt)` does; doing that next.

### 4.2 Not a bug — checked and cleared

`ptr_to_buffers_[stmt] = ptr_to_buffers_[stmt->origin];` also raises an
iterator-invalidation question: the RHS `operator[]` may insert, then the LHS
`operator[]` may insert again and rehash, while a reference to the RHS element
is live. This is **safe**. `unordered_map` is node-based; rehashing invalidates
iterators but not references or pointers to elements. C++17 sequences E2 before
E1 for `E1 = E2` including for an overloaded `operator=` with the built-in
sequencing, and `BufferInfo`'s copy-assignment is implicit. No defect here.
Recorded because I looked and it is the sort of thing a reader will ask.

---

## 5. `get_buffer_value({Root, {-1}})` — what the fabricated entry would do

`:2266-2315`. `Root` is not `Args`, `Rets` or `ArgPack`, so it falls to the
default tail at `:2304-2314`: it takes a fresh binding from `binding_head_++`
and calls `ir_->buffer_argument(type, 0, binding, buffer_instance_name(buffer))`.

`buffer_instance_name` for `Root` is
`kRootBufferName + "_" + fmt::join(root_id, "_")`, so the fabricated entry
produces the SSBO name **`root_-1`** and a `BufferBind{{Root,{-1}}, binding}`
entry that `get_buffer_binds()` (`:2183` region) hands to
`task_attribs_.buffer_binds`. It does not crash; it silently declares and binds
a root buffer numbered -1.

I am not chasing that further — it is downstream of my question. It is recorded
in Escalations because it is a live defect the discriminator question uncovered.

## 6. Enumeration 2 — every path a pointer value reaches `at_buffer` by

### 6.1 The call sites

`grep -n "at_buffer\|load_buffer(\|store_buffer("` over the file. Nine hits;
one is the definition (`:2194`), one is the error string (`:2209`).

Callers of `at_buffer`, seven:

| Call | Line | Argument | Guard |
|---|---|---|---|
| A1 | `:1617` | `stmt->dest` (`AtomicOpStmt`) | dt==f64 && `spirv_has_atomic_float64_add` && op==add |
| A2 | `:1621` | `stmt->dest` | dt==f64, else-branch, **and only when `!dest_is_ptr`** |
| A3 | `:1626` | `stmt->dest` | dt==f32 && `spirv_has_atomic_float_add` && op==add |
| A4 | `:1630` | `stmt->dest` | dt==f32, else-branch, **only when `!dest_is_ptr`** |
| A5 | `:1633` | `stmt->dest` | all other dt, **only when `!dest_is_ptr`** |
| A6 | `:1681` | `stmt->dest` | integral && op==mul |
| A7 | `:2233` / `:2257` | `ptr` forwarded from `load_buffer` / `store_buffer` | — |

`load_buffer` has one caller, `GlobalLoadStmt` at `:574`, argument `stmt->src`.
`store_buffer` has one caller, `GlobalStoreStmt` at `:568`, argument `stmt->dest`.

**So `at_buffer`'s `ptr` is one of exactly three IR operand slots:**
`GlobalLoadStmt::src`, `GlobalStoreStmt::dest`, `AtomicOpStmt::dest`. Three,
counted off the list above: A7 supplies two of them, A1-A6 the third.

### 6.2 A fourth discriminator already exists in the file, at `:1610-1612`

```cpp
spirv::Value dest_val = ir_->query_value(stmt->dest->raw_name());
// Shared arrays have already created an accesschain, use it directly.
const bool dest_is_ptr = dest_val.stype.flag == TypeKind::kPtr;
```

`SType::flag` (`spirv_ir_builder.h:59`, `TypeKind` at `:36-46`) is `kPtr` for
anything built by `get_pointer_type` (`spirv_ir_builder.cpp:407-424`). This is
a real, surviving property of the *SPIR-V type*, not of the value, so unlike
`ValueKind` it is not reset by arithmetic — arithmetic on a pointer type is not
expressible. **It separates "already an SPIR-V pointer" from "an integer to be
interpreted".** It does NOT separate physical-u64 from relative-u64: both of
those are `kPrimitive` integers with `dt == u64`. So it is not the discriminator
being sought, but it is a fourth thing available at the decision point and it
must be stated, because a reader will otherwise ask.

**A1, A3 and A6 have no `dest_is_ptr` guard.** A2, A4 and A5 do. So an
`AtomicOpStmt` whose dest is a shared-array access chain reaches `at_buffer`
directly if it is an f64 add on a device with `spirv_has_atomic_float64_add`
(A1), an f32 add with `spirv_has_atomic_float_add` (A3), or an integral mul
(A6). In `at_buffer` that value has `stype.flag == kPtr` and
`stype.dt == PrimitiveType::unknown` (a pointer `SType` never sets `dt`;
`DataType()` is `PrimitiveType::unknown`, `taichi/ir/type.cpp:20`). So it fails
`ptr_val.stype.dt == PrimitiveType::u64` at `:2197` and then fails
`is_integral` at `:2207`, producing the `TI_ERROR` "at_buffer failed,
`ptr_val.stype.dt` is not integeral". Not a crash — `is_integral` dereferences
a valid `Type*` — a clean hard error. Reachability of A1/A3/A6 with a shared
dest depends on the front end, which section 1.2 of the plan puts out of scope,
so I mark it **inferred**, not verified. Escalated.

### 6.3 Which statement classes can occupy those three slots

Constraint available from the code itself, without any reachability argument:
on the non-u64 path `at_buffer` calls `ptr_to_buffers_.at(ptr)` at `:2212`,
which throws `std::out_of_range` on a missing key. So **every statement that
actually reaches the relative path must be a key in the map.** The map's key
set is exactly the seven writers of section 2. That gives a closed upper bound
for the relative side:

- `GetChStmt` with `out_snode->is_place()` (W4)
- `GlobalTemporaryStmt` (W5)
- `ExternalPtrStmt` (W6/W7)
- `MatrixPtrStmt` (W1/W2/W3) — including, thanks to section 4, keys whose entry
  was fabricated rather than written

The u64 path has **no such constraint**, because `:2197` returns at `:2204`
before the map is ever touched. Whatever reaches it is unconstrained by the map.
That asymmetry is exactly why "does the map have an entry on both sides" was
never a test of anything: on the physical side, nothing has ever forced an
entry to exist or be correct. It happens to exist. That is luck, not structure.

### 6.4 Enumeration of u64 producers — the physical side

Every `register_value` in the file: `grep -n "register_value"` gives 32 hits,
one of which (`:600`) is commented out. Of the remaining 31, the ones whose
value can carry `stype.dt == PrimitiveType::u64` **and** whose statement can be
a pointer operand:

| # | Line | Statement | u64? |
|---|---|---|---|
| P1 | `:792` | `ExternalPtrStmt`, `caps_->get(spirv_has_physical_storage_buffer)` true | **yes, always** — `ir_->add` of a `u64_type()` load and an `OpSConvert` to u64 (`:786-791`) |
| P2 | `:794` | `ExternalPtrStmt`, caps false | no — `linear_offset` is i32 (`:737`, `:775-779`) |
| P3 | `:356` | `GetRootStmt` — `make_pointer(0)` | u32 today; u64 **iff `use_64bit_pointers`** |
| P4 | `:373` | `GetChStmt` — `add(input_ptr, make_pointer(offset))` | u32 today; u64 iff `use_64bit_pointers` |
| P5 | `:510` | `SNodeLookupStmt` — `add(parent, mul(idx, make_pointer(stride)))` | u32 today; u64 iff `use_64bit_pointers` |
| P6 | `:710` | `GlobalTemporaryStmt` | no — `int_immediate_number(i32_type(), ...)` |
| P7 | `:334` | `MatrixPtrStmt` | inherits origin's width |
| P8 | `:626` | `ArgLoadStmt` with `!create_load` | no — `kPtr` type, `dt` unknown |

`use_64bit_pointers` is `const bool use_64bit_pointers = false;` at **`:82`**,
a non-static member with no other assignment anywhere in the tree
(`grep -rn "use_64bit_pointers"` returns exactly two hits, `:82` and the read
at `:2318`). `make_pointer` at `:2317-2324` is the only consumer.

**So today set P — values that are physical device addresses — is exactly
{`ExternalPtrStmt` when the capability is on}.** One statement class. P3/P4/P5
are the ones that item 6.2 would widen to u64 and thereby collide.

### 6.5 The derived-physical-pointer hole, closed

The obvious attack is P7: a `MatrixPtrStmt` over an `ExternalPtrStmt`, which
would be a physical address derived by arithmetic. **It cannot reach codegen.**

`MatrixPtrStmt::offset_used_as_index()` (`taichi/ir/statements.h:521-529`)
returns true when the origin is `AllocaStmt`, `GlobalTemporaryStmt`,
**`ExternalPtrStmt`** or `MatrixPtrStmt`. In the SPIR-V visitor
(`:312-333`) the `offset_used_as_index()` branch handles only `AllocaStmt`
(`:313`, W1) and `GlobalTemporaryStmt` (`:321`, W2) and hits
**`TI_NOT_IMPLEMENTED` at `:328`** for everything else — which is precisely
`ExternalPtrStmt` and `MatrixPtrStmt`.

That would be a live hole if such statements survived to codegen. They do not:
`FuseMatrixPtr` at `taichi/transforms/scalarize.cpp:1260-1300` rewrites
`MatrixPtrStmt(ExternalPtrStmt, offset)` into a single fused `ExternalPtrStmt`
with the offset folded into `indices` (`:1271-1283`), erasing the
`MatrixPtrStmt`. So a physical address is **always produced directly by an
`ExternalPtrStmt`**, never derived.

**This is the load-bearing fact of the whole investigation.** Grade it
carefully: the `TI_NOT_IMPLEMENTED` is verified from the source; the claim that
`FuseMatrixPtr` always runs before SPIR-V codegen is a pass-ordering claim I
have not executed, so it is **inferred**. But the two together are
belt-and-braces: either the fusion happens, or codegen aborts loudly at `:328`.
There is no silent path to a derived physical pointer.

### 6.6 The contrapositive, and it is airtight from the source alone

W3 at `:332` is the only propagation site that could carry an `ExtArr` or `Args`
entry onto a non-`ExternalPtrStmt` key. It sits in the `else` of
`offset_used_as_index()`, so **its origin is by construction NOT an
`ExternalPtrStmt`** (an `ExternalPtrStmt` origin makes the predicate true).
W1's origin is an `AllocaStmt`; W2's is a `GlobalTemporaryStmt`. Neither can
hold `ExtArr` or `Args`.

Therefore, with no reachability argument at all:

> **`ptr_to_buffers_[k].type ∈ {ExtArr, Args}` ⟺ `k` is an `ExternalPtrStmt`.**

Left to right: only W6 and W7 write those two values, and both key on an
`ExternalPtrStmt`; W1/W2/W3 cannot propagate them. Right to left: W6/W7 are at
`:797-801`, **outside** the `caps_` branch at `:783-795`, so every
`ExternalPtrStmt` writes one of the two unconditionally.

### 6.7 What reaches W3 in practice

`lower_access.cpp:134-143` rewrites a `MatrixPtrStmt`'s origin from
`GlobalPtrStmt` to `lowered.back()`, and `lower_ptr` returns a `GetChStmt`
(`:273`/`:276`, `return last` at `:280`) whose `out_snode` is the place at the
end of the SNode path — so W4 has written a genuine `Root` for it. That is the
ordinary W3 case and it is sound.

The unsound W3 case is an origin that is none of the map's key classes —
`SNodeLookupStmt` (`:510`) or a non-place `GetChStmt` — which fabricates
`{Root, {-1}}` per section 4. I could not construct a front-end program that
produces one, and section 1.2 puts the front end out of scope, so I record it
as **inferred, unproven**, not as a demonstrated defect.

---

## 7. Enumeration 3 — collisions, aliasing, lifetime

The brief asks specifically about "keys that collide". Four distinct things
could mean that. Taking them one at a time.

**7.1 `Stmt *` key collision.** Impossible among live statements: distinct
objects, distinct addresses. The only route is a freed `Stmt` whose address is
reused while the map still holds it. `TaskCodegen` does not mutate the IR — a
grep for `insert_before`, `->erase(`, `replace_usages` and `modifier` over
`spirv_codegen.cpp` returns only two hits, both inside PrintStmt diagnostic
strings at `:204` and `:213`. `task_ir_` is `OffloadedStmt *const` and marked
"not owned" (`:2622`). So no statement is destroyed during codegen. **Closed.**

**7.2 `raw_name()` collision.** `Stmt::raw_name()` is `fmt::format("tmp{}", id)`
(`taichi/ir/ir.h:434-436`), and `id` is renumbered from 0 by
`taichi/transforms/re_id.cpp:20-22`. Two statements sharing an id would share a
name, and `at_buffer` looks the value up by name (`:2195`) while looking the
buffer up by pointer (`:2212`) — a genuine divergence risk. It is closed by the
code itself: `IRBuilder::register_value` (`spirv_ir_builder.cpp:1300-1309`)
raises `TI_ERROR("{} already exists.", name)` on any duplicate non-constant
name. A raw_name collision inside one task would abort codegen loudly before
reaching `at_buffer`. Each `TaskCodegen` also holds its own `IRBuilder`
(`:97`) and its own map (`:2635`), so ids need only be unique per task, which
`re_id` over the kernel root gives. **Closed.**

**7.3 SPIR-V `Value` aliasing.** Two statements can absolutely register the same
underlying SPIR-V id — the whole `ValueKind` problem is of this kind. It is
**irrelevant here**, and this is the crux of why the map is a different animal
from the tag. `ptr_to_buffers_` is keyed on the *statement*, not on the value.
Arithmetic that produces a fresh SPIR-V id and resets `ValueKind` does not touch
the map, because the map's key is the new `MatrixPtrStmt`, and W1/W2/W3 copy the
origin's entry onto it explicitly. **The map tracks provenance across arithmetic;
the tag does not.** That is the whole difference.

**7.4 Out-of-order visitation.** `gen_array_range` (`:1964-1979`) visits
statements ahead of block order and records them in `offload_loop_motion_`
(`:1977`) so `visit(Block)` at `:164` skips them later. This does not break
def-before-use for the map: a hoisted statement's visitor still runs its own map
write before any consumer runs. `check_value_existence` at `:1970` prevents a
second visit, which matters because a second visit of a `GetChStmt` would trip
`TI_ASSERT(ptr_to_buffers_.count(stmt) == 0)` at `:376`. **Closed.**

## 8. The predicate is at three sites, not one

`ptr_val.stype.dt == PrimitiveType::u64` appears at `:2197` (`at_buffer`),
`:2227` (`load_buffer`) and `:2249` (`store_buffer`). The latter two do not
choose the addressing mode; they choose `ti_buffer_type`, suppressing the
uint-reinterpret-and-bitcast that the buffer-relative path needs and the
physical path does not. They ask the same question and would need the same
answer. Three sites, one file.

## 9. Where the map+capability rule holds and where it stops

Candidate rule, using only things in scope at `:2197`:

    physical  ⟺  entry.type ∈ {ExtArr, Args}  ∧  caps_->get(spirv_has_physical_storage_buffer)

`caps_` is a member at `:2565`, already read at `:783` — inside the very
visitor that creates the ambiguity — and at `:2340`, `:2416`, `:2491`. It is a
`DeviceCapabilityConfig *` supplied once through `Params` (`:75`, `:86`) and
never written in this file (`grep -n "caps_->set"` returns nothing). So it is
constant for the life of the `TaskCodegen`.

Four obligations, all discharged in section 6:
1. `ExtArr`/`Args` ⟺ key is `ExternalPtrStmt` (§6.6, from source structure alone).
2. `ExternalPtrStmt` is physical iff the capability (`:783-795`, no other guard).
3. No physical address is ever derived from another (§6.5).
4. Every `ExternalPtrStmt` writes an entry (`:797-801`, outside the caps branch).

**Where it stops.** The rule reads *which buffer* and *what the device can do*,
and infers *how the pointer is addressed*. That inference is sound today only
because exactly one buffer class is ever physically addressed. `BufferType`
(`kernel_utils.h:23-31`) has no addressing-mode axis and cannot grow one without
becoming a different thing. The instant a second buffer class is physically
addressed — the root buffer by device address, which is the obvious shape for
64-bit SNode addressing — `Root` appears on both sides and the rule dies. So the
honest grading is: the map can carry it **for the shelved work as shelved**, and
cannot carry it **for an arbitrary extension of physical addressing**.

**What the rule does survive.** Flipping `use_64bit_pointers` at `:82` to true,
which is the change item 6.2's SPIR-V half actually needs. That widens
`make_pointer` to u64 and therefore makes `GetRootStmt` (`:356`),
`GetChStmt` (`:373`) and `SNodeLookupStmt` (`:510`) produce u64 values. Under
today's predicate at `:2197` every place-`GetChStmt` would then be
misclassified as a physical address — that is the collision, stated concretely.
Under the map+capability rule those keys carry `Root` (W4) and are correctly
classified relative regardless of width. **The rule is sufficient for the
switch-on that item 6.2 requires.**

---

# AMENDMENT PASS — after reading `adversary-07-1.md` and `adversary-07-2.md`

Appended, not rewritten. Everything above stands as it was written except where
this section says otherwise. Plan re-read at this point: **60989 bytes,
md5 `d82f5dafad045ccbe48652d6072a3c80`, "Last updated: 2026-09-09"**. It has
grown from 45208 bytes since I finalised. Section 10's numbering is fixed and
grading is item 8. Section 8.2 item 0 now records this territory as RESOLVED and
already carries most of what follows.

## A1. Two of my counts were wrong. Both fixed, with the rule stated first

Section 10's new preamble says to declare the granularity rule before counting.
I did not, and both counts were wrong in consequence.

**`at_buffer` call sites.** Rule: one site per source line containing the text
`at_buffer(`, excluding the definition line.

```
$ grep -c "at_buffer(" taichi/codegen/spirv/spirv_codegen.cpp
9
```
Nine lines: `:1617`, `:1621`, `:1626`, `:1630`, `:1633`, `:1681`, `:2194`
(definition), `:2233`, `:2257`. Nine minus the definition = **8 call sites**.
My report said seven over a seven-row table whose last row held two sites
(`:2233` **/** `:2257`). The rows were right; the total was not. **8, not 7.**

**`register_value` sites.** Rule: one site per occurrence of the text
`ir_->register_value(`, minus those on a commented-out line.

```
$ grep -o "ir_->register_value(" taichi/codegen/spirv/spirv_codegen.cpp | wc -l
33
$ grep -n "// *ir_->register_value(" taichi/codegen/spirv/spirv_codegen.cpp
600:      // ir_->register_value(stmt->raw_name(), val);
```
33 − 1 = **32 live**. I wrote 32 hits and 31 live; the 33-line listing was in
front of me and I miscounted it. **32, not 31.** No conclusion changes: the
enumeration's purpose was to find u64-capable producers and it found the same
one either way.

## A2. My `make_pointer` set was incomplete — `:408` was missing

Rule: one site per line containing `make_pointer(`, excluding the definition.

```
355:    spirv::Value root_val = make_pointer(0);
371:    spirv::Value offset = make_pointer(desc.mem_offset_in_parent_cell);
408:        make_pointer(desc.cell_stride * desc.snode->num_cells_per_container));
506:      spirv::Value stride = make_pointer(desc.cell_stride);
2317:  spirv::Value make_pointer(size_t offset) {   <- definition
```

**Four call sites**, not three. My report named the three *visitors* by their
`register_value` lines (`:356`, `:373`, `:510`) rather than their `make_pointer`
lines, which is why both adversaries read my numbers as one off; the citations
were of a different thing, but the SET was genuinely short by one.

`:408` is inside `bitmasked_activation` (`:384`). That function sets
`ptr_dt = parent_ptr.stype` at `:388` and then hardcodes `ir_->u32_type()` at
`:405`, `:411` and `:412`. Under `use_64bit_pointers = true`, `make_pointer` at
`:408` returns u64 into that u32 arithmetic. On the **sparse activation path**,
which plan section 4.1 makes non-negotiable. Outside the discrimination
question; recorded because it falsifies a bound my report stated.

## A3. The fabricated buffer name — I got the string wrong

I wrote `root_-1`. `kRootBufferName` is `"root_buffer"` at
`spirv_codegen.cpp:26`, and `buffer_instance_name` at `:47` concatenates
`kRootBufferName + "_" + fmt::join(root_id, "_")`. The name is
**`root_buffer_-1`**. Both adversaries caught it; both passes had reported a
string neither had looked up. I have now looked it up.

## A4. My stated limit was a SUFFICIENT condition dressed as a NECESSARY one

This is the substantive correction and I accept it.

I wrote that if item 6.2 addresses the root buffer physically, `Root` lands on
both sides and the map cannot carry it. **`Root` on both sides is not by itself
fatal.** Under a *uniform* policy — every root physical whenever the capability
is on — the rule simply widens to

    physical ⟺ type ∈ {ExtArr, Args, Root} ∧ cap

and discriminates exactly as well as it does today, because the addressing mode
is still a function of `(type, cap)`. What actually collapses the map is a
**non-uniform** policy, one where the mode varies *within* the `Root` class at a
fixed capability — "roots above some size threshold are physical, the rest stay
descriptor-bound". Only then do two `Root` entries need to mean different
things, and `BufferInfo` (`kernel_utils.h:33-62`) carries `type` and `root_id`
and nothing else with which to say so.

**And there is a sharper failure I missed entirely.** Verified by re-reading the
function layout: the predicate is at `:2197`, the `!is_integral` backstop at
`:2207-2210`, the map read at `:2212`. A map-based predicate replacing `:2197`
sits **above** the backstop. So under any root-physical variant, the fabricated
`{Root, {-1}}` from `:319` — which I established is produced on every execution
for a shared workgroup array — would be classified physical and fed to
`OpConvertUToPtr` at `:2198-2202`. That is a raw device pointer forged from a
workgroup access-chain result, and the backstop that catches it today never
runs.

**Consequence for my E1.** I filed the fabrication as an independent
pre-existing defect. It is not independent of my own conditional: it is a
**precondition** of the root-physical variant. Fix the fabrication first, or
that variant is unimplementable safely.

## A5. The conditional is settled empirically, and it does NOT close the way one adversary said

My report called this "the single largest determinant of this item's cost" and
said I could not settle it. It is now settled, and I re-measured rather than
inherit either adversary's numbers.

```
$ grep -n "maxStorageBufferRange\|maxMemoryAllocationSize" \
    external/Vulkan-Headers/include/vulkan/vulkan_core.h
3324:    uint32_t              maxStorageBufferRange;
5812:    VkDeviceSize       maxMemoryAllocationSize;
```

```
$ vulkaninfo | grep -E "deviceName|maxStorageBufferRange|maxMemoryAllocationSize"
	deviceName        = NVIDIA GeForce GTX 1070
	maxStorageBufferRange = 4294967295
	maxMemoryAllocationSize = 0xffe00000
	deviceName        = NVIDIA GeForce GTX 750 Ti
	maxStorageBufferRange = 4294967295
	maxMemoryAllocationSize = 0xffe00000
	deviceName        = llvmpipe (LLVM 20.1.2, 256 bits)
	maxStorageBufferRange = 134217728
	maxMemoryAllocationSize = 0x80000000
```

`0xffe00000` = 4292870144. `0x80000000` = 2147483648. And:

```
$ grep -rn "maxStorageBufferRange\|maxMemoryAllocationSize\|maxUniformBufferRange" \
    taichi/ tests/ cpp_examples/ | wc -l
0
```

The tree queries none of it.

**The two caps are different caps.** `maxStorageBufferRange` bounds the `range`
of a storage-buffer *descriptor*. A `PhysicalStorageBuffer` access uses a raw
u64 and no descriptor, so that limit does not reach it; what bounds it is
`maxMemoryAllocationSize`. Separating them:

| Device | descriptor-bound cap | physically-addressed cap | gain |
|---|---|---|---|
| GTX 1070 | 4292870144 | 4292870144 | none |
| GTX 750 Ti | 2147483648 (whole card) | 2147483648 | none |
| llvmpipe | **134217728** | **2147483648** | **16x** |

Three rows, one per device the loader enumerates. 2147483648 / 134217728 = 16
exactly.

**So the antecedent I could not settle DOES fire — on exactly one device, and it
is the CPU one.** Plan section 2 puts CPU-only operation first and section 5.2's
2026-09-09 correction says a gap on the less-served spine is a gap to record,
never evidence that it matters less. One adversary concluded physical addressing
is never needed here; that holds on the two cards and fails on the software
renderer.

**The 4 GiB wall is permanent by specification.** `maxStorageBufferRange` is a
`uint32_t` at `vulkan_core.h:3324`. No Vulkan device can advertise more, because
the field cannot express more, and both cards report exactly the field's
maximum — they are reporting the structure's ceiling, not their own. Grading per
section 10 item 8: this half does **not** expire on new silicon or a new driver,
only if Khronos widens the field. The `maxMemoryAllocationSize` half is a
`VkDeviceSize`, 64-bit, and genuinely can rise; the llvmpipe 128 MiB figure is a
Mesa choice and is retestable. **The bound is split, not uniform.**

## A6. `:1681` discards a guard rather than lacking one

I filed `:1617`, `:1626` and `:1681` as three calls missing the `dest_is_ptr`
guard. For `:1681` that understates it. Re-reading `:1632-1683`: for an integral
dtype, `:1633` assigns `addr_ptr = dest_is_ptr ? dest_val : at_buffer(...)`,
which is **correct** for a shared array. Control then reaches
`else if (is_integral(dt))` at `:1671`, and for `AtomicOpType::mul` `:1681`
**overwrites** `addr_ptr` unconditionally. The correct value computed four
statements earlier is discarded. So an atomic multiply on a shared array fails
on every device by throwing away a guard's result, not by lacking a guard.

## A7. The capability split is measured, not inferred

```
$ vulkaninfo | grep -E "deviceName|shaderBufferFloat32AtomicAdd|shaderBufferFloat64AtomicAdd"
	NVIDIA GeForce GTX 1070   : Float32AtomicAdd = true,  Float64AtomicAdd = true
	NVIDIA GeForce GTX 750 Ti : Float32AtomicAdd = true,  Float64AtomicAdd = false
	llvmpipe                  : Float32AtomicAdd = true,  Float64AtomicAdd = false
```

Mapped to the two capabilities at
`taichi/rhi/vulkan/vulkan_device_creator.cpp:740-746`. So `:1626` (f32) selects
the unguarded call on **all three** devices, and `:1617` (f64) selects it on the
**GTX 1070 alone**. The same f64 program takes the guarded path at `:1621` on
the baseline card and the unguarded path at `:1617` on the mid card. **The newer
card is the one that fails.**

What this settles and what it does not, per section 10 item 7: it settles which
branch the capability selects on each device, which was previously conditional.
It does **not** settle whether a front-end program producing an atomic float add
on a shared array exists. That remains my I3, and the plan puts the front end
out of scope.

## A8. W7 at `:800` is dead — accepted, and kept on the record

Both adversaries enumerated `taichi/program/callable.cpp` and found `is_array`
hardcoded true in five of the six `insert_*_param` routes, two carrying `FIXME`
comments that the flag has been overloaded to mean "pointer-like argument". So
`ctx_attribs_->arg_at(arg_id).is_array` is true at every `ExternalPtrStmt` in
this tree and the `else` at `:800` is unreachable. I accept the enumeration.

The `Args` disjunct in my rule is therefore over-broad but harmless; the
operative rule today is `type == ExtArr ∧ cap`. I am **not** narrowing the rule
to drop `Args`: nothing enforces the invariant, `:783-791` reads args-struct
member 1 unguarded on `is_array`, and section 10 item 3 forbids deciding a
branch is unnecessary. My E3 stays, restated as "dead today, unenforced".

## A9. My blast radius omitted the guard-order coupling — corrected below

Both adversaries flagged it. My six counted decision *lines* and missed that
three sites change failure mode without being edited. Re-derived under a stated
rule in the revised report.
