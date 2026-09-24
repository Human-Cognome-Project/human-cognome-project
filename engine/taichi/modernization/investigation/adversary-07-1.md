# Adversary 07-1 — the discriminator question

**Plan version worked against:** `modernization/PROJECT-PLAN.md`, "Last updated: 2026-09-09",
45208 bytes at the time I read it. I read it in full. Section 10's priority statement
(assumptions tested, and files nobody has opened) and item 8 (grade the KIND, state the
BLAST RADIUS separately) govern what follows. Section 2.2a puts item 6.2 in scope as BASE
work in full, including the `at_buffer` pointer-width collision by name. Section 8.2 item 0
is marked UNDER REVISION pending this territory and currently quotes "eight sites in two
files" from pass A. **That quoted figure is wrong and section 9 of this file shows why.**

Judged blind of adversary 07-2, which did not exist when I finished
(`adversary-07-2.md`: no such file).

**All bare line numbers are `taichi/codegen/spirv/spirv_codegen.cpp`.**

---

## 1. The prior question, settled first and independently

Both passes assert a discriminator exists and that the inherited "none exists" sentence is
false. Four documents and two adversaries had asserted the opposite. I checked it myself
before reading either report's argument, and I confirm it.

At `:2194`, `at_buffer` has this signature:

```cpp
spirv::Value at_buffer(const Stmt *ptr, DataType dt) {
```

The disputed predicate is at `:2197`, `if (ptr_val.stype.dt == PrimitiveType::u64)`. Two
things are in scope there that are not functions of the pointer's scalar width:

- `ptr`, a `const Stmt *`. `ptr->is<ExternalPtrStmt>()` is directly testable.
- `caps_`, a `DeviceCapabilityConfig *` member declared at `:2565`, initialised at `:86`
  from `params.caps`, never reassigned anywhere under `taichi/codegen/spirv/`.

I verify that `ptr->is<ExternalPtrStmt>() && caps_->get(DeviceCapability::spirv_has_physical_storage_buffer)`
is exactly co-extensive with "the value registered for `ptr` is a physical device address":

1. **Only one site mints a physical address.** `:792`, `ir_->register_value(stmt->raw_name(), addr)`,
   inside `if (caps_->get(DeviceCapability::spirv_has_physical_storage_buffer))` at `:783`
   in `visit(ExternalPtrStmt)`. The value is a u64 loaded from args-struct member 1 plus an
   `OpSConvert`-widened linear offset (`:786-791`).
2. **The `ptr` argument can only be one of four statement classes.** `:2212` reads
   `ptr_to_buffers_.at(ptr)`, which throws `std::out_of_range` for any key no writer wrote.
   The seven writers key on `MatrixPtrStmt`, `GetChStmt`, `GlobalTemporaryStmt` and
   `ExternalPtrStmt` only.
3. **None of the other three can carry a physical address.** `GetChStmt` registers
   `ir_->add(input_ptr_val, make_pointer(...))` at `:372`; `GlobalTemporaryStmt` registers an
   i32 immediate at `:709`; `MatrixPtrStmt` registers an `OpAccessChain`, or an `add` over a
   `GlobalTemporaryStmt`, or an `add` over an origin that is provably not an
   `ExternalPtrStmt` (section 3 below).
4. **The capability is constant for the compile.** `KernelCodegen::run` passes the same
   `&params_.caps` to every task; there is no `caps_->set` in the SPIR-V codegen directory.

So the predicate at `:2197` can be replaced by a test that does not read the pointer's
width, using only values already in scope in the same function. **The inherited sentence is
false. Both passes are right, and I reached it without either of them.**

What is *not* false, and neither pass disturbs it: the `ValueKind` tag route is dead, for
the reason section 8.2 already records. `ValueKind` is not the only route. The plan's
sentence generalises from the tag to "any route" without warrant.

---

## 2. THE PRINCIPAL TASK, part one — pass B's rule

Pass B states:

```
physical  ⟺  ptr_to_buffers_.at(ptr).type ∈ {ExtArr, Args}
             ∧  caps_->get(DeviceCapability::spirv_has_physical_storage_buffer)
```

**The rule is correct on today's tree.** I checked every link.

### 2.1 The seven writers — confirmed, my own enumeration

`grep -rn "ptr_to_buffers_" --include=*.cpp --include=*.h --include=*.cc --include=*.hpp .`
over the whole repository returns **ten** hits, all in one file:

| # | Line | Kind | Value written |
|---|---|---|---|
| W1 | `:319` | write | `ptr_to_buffers_[stmt->origin]`, origin is a shared `AllocaStmt` |
| W2 | `:326` | write | `ptr_to_buffers_[stmt->origin]`, origin is `GlobalTemporaryStmt` |
| W3 | `:332` | write | `ptr_to_buffers_[stmt->origin]`, byte-offset arm |
| W4 | `:377` | write | `BufferInfo(BufferType::Root, root)` |
| W5 | `:711` | write | `BufferType::GlobalTmps` |
| W6 | `:798` | write | `{BufferType::ExtArr, arg_id}` |
| W7 | `:800` | write | `BufferType::Args` |
| — | `:376` | count | `TI_ASSERT(ptr_to_buffers_.count(stmt) == 0)` |
| — | `:2212` | read | `get_buffer_value(ptr_to_buffers_.at(ptr), dt)` |
| — | `:2635` | decl | `std::unordered_map<const Stmt *, BufferInfo> ptr_to_buffers_;` |

Seven writes, one count, one read, one declaration; 7 + 1 + 1 + 1 = 10, which is the hit
count. Both passes give the same seven lines and I confirm each by reading the visitor.

**What this enumeration cannot see:** an access through a reference or pointer bound to the
map under another name. I read all ten hits; none binds one. The enumeration is closed.

### 2.2 The map alone cannot work — confirmed

Pass B's argument is that `ExtArr` and `Args` sit on both sides, because `:783` (the
capability test) is a separate and *earlier* test than `:797` (the `is_array` test), so an
`ExternalPtrStmt` writes the same map value whether it registered a u64 device address or an
i32 linear offset. I read `:733-803` as one block and confirm the nesting: `:783` opens a
branch that closes at `:795`; `:797-801` is outside it and unconditional. Correct.

### 2.3 The biconditional at pass B's §3.4 — every link checked

> `ptr_to_buffers_[k].type ∈ {ExtArr, Args}` ⟺ `k` is an `ExternalPtrStmt`

**Right to left.** W6/W7 at `:797-801` are outside the capability branch, so every
`ExternalPtrStmt` visit writes one of the two. Confirmed by reading.

**Left to right.** Only W6 and W7 write those values and both key on the `ExternalPtrStmt`
being visited. The three propagation sites are the only route by which a value could move to
another key:

- **W1 (`:319`)** — guarded by `stmt->origin->is<AllocaStmt>()` at `:314` and
  `stmt->origin->as<AllocaStmt>()->is_shared` at `:318`. No writer writes an `AllocaStmt`
  key; `visit(AllocaStmt)` at `:284-303` calls only `register_value`. So the origin's entry
  is always absent, and section 4 shows what `operator[]` then produces.
- **W2 (`:326`)** — origin is a `GlobalTemporaryStmt`, whose only writer is W5, which writes
  `GlobalTmps`.
- **W3 (`:332`)** — the `else` of `offset_used_as_index()`. I read
  `MatrixPtrStmt::offset_used_as_index()` at `taichi/ir/statements.h:521-529`: it returns
  true when the origin is `AllocaStmt`, `GlobalTemporaryStmt`, `ExternalPtrStmt` or
  `MatrixPtrStmt`. So in the `else` arm the origin is **none of those four by construction**,
  which excludes `ExternalPtrStmt` directly. Pass B's argument is sound.

  The residual origin set follows from `MatrixPtrStmt::MatrixPtrStmt` at
  `taichi/ir/statements.cpp:104-131`: `GlobalPtrStmt`, `GetChStmt`, `MatrixOfGlobalPtrStmt`,
  `MatrixOfMatrixPtrStmt`, `ThreadLocalPtrStmt`, `AdStackLoadTopStmt`. None of these is ever
  a key holding `ExtArr` or `Args`. Pass A gives this same residual set and is right about
  it; pass B does not enumerate it but does not need to.

**No physical address is ever derived from another.** The one route that would derive one is
`MatrixPtrStmt` over an `ExternalPtrStmt`. It is closed twice:

- `FuseMatrixPtr::visit(MatrixPtrStmt *)` at `taichi/transforms/scalarize.cpp:1266-1300`
  folds `MatrixPtrStmt(ExternalPtrStmt, offset)` into a single fused `ExternalPtrStmt`
  (`:1281-1283`), calls `replace_usages_with` and erases the `MatrixPtrStmt` (`:1298-1299`).
  I read the pass. Pass B's citation is right.
- Independently, if it survived, the index-offset branch hits `TI_NOT_IMPLEMENTED` at `:328`
  because `:314` and `:322` handle only `AllocaStmt` and `GlobalTemporaryStmt`.

**Verdict on the principal rule: pass B's rule holds, and its claim that no reachability
argument is needed holds.** The proof is structural. I found no broken link.

---

## 3. THE PRINCIPAL TASK, part two — pass B's stated limit, which is WRONG AS WRITTEN

Pass B's limit:

> the rule infers addressing mode from buffer identity crossed with capability, and is sound
> only because exactly one buffer class is physically addressed today. `BufferType` has no
> addressing-mode axis. If item 6.2 addresses the ROOT buffer physically, `Root` lands on
> both sides and the map genuinely cannot carry it.

The first two sentences are right. `BufferType` at `taichi/codegen/spirv/kernel_utils.h:23-31`
has seven enumerators and none of them names an addressing mode. **The third sentence does
not follow, and I can show it does not.**

`Root` lands on both sides only if the addressing mode varies *within* the `Root` class at a
fixed capability. Under a uniform policy — every root buffer physical whenever the
capability is on — the rule becomes `type ∈ {ExtArr, Args, Root} ∧ cap`, which discriminates
exactly as well as today's. The map carries it fine. What breaks the map is a **per-buffer
or per-size policy**, for instance "roots above some threshold are physical, the rest stay
descriptor-bound". Pass B has stated a sufficient condition for collapse as if it were a
necessary one, and the difference is the whole cost of the item: a uniform policy needs no
new machinery at all.

**There is a second, sharper failure that neither pass states, and it is worse.** The moment
`Root` means physical, the fabricated entry from W1 becomes an *active* misclassification.
Section 4 shows `:319` produces `{Root, {-1}}` for a shared workgroup array on every
execution. Today that value is harmless because `Root` means relative. Under
`Root ∧ cap ⇒ physical`, a pointer into workgroup-shared memory would be classified as a
device address and fed to `OpConvertUToPtr` at `:2198-2202`. That is not a wrong descriptor
binding; it is a raw pointer forged from an access-chain result. **So the fabricating write
at `:319` and the missing default initialiser are not merely adjacent defects — they are
preconditions for the root-physical variant of item 6.2.** Pass B files them as E1, an
independent defect. They are not independent of its own conditional.

---

## 4. Verify item 1 — the value-inserting read, and whether it is well defined

Both passes report that the map is read with an operator that value-inserts, that both
compiled replicas got `{Root, root_id = {-1}}` deterministically, and that it is well
defined rather than undefined behaviour.

**Confirmed on all three points, by my own compilation, not by reading theirs.**

`BufferInfo` is at `taichi/codegen/spirv/kernel_utils.h:33-62`:

```
34:    BufferType type;
35:    std::vector<int> root_id{-1};  // only used if type==Root or type==ExtArr
37:    BufferInfo() = default;
```

`type` at `:34` has **no default member initialiser**. `root_id` at `:35` has one.
`BufferInfo() = default;` at `:37` is user-declared but not user-provided.

I compiled a replica of `:23-49` and pre-poisoned the heap with `0xAB` before the test, so a
zero could not come from a fresh page. Three configurations:

| Configuration | `m[missing_key]` | `BufferInfo{}` | `BufferInfo d;` (control) |
|---|---|---|---|
| `g++ -std=c++17 -O0` | `type=0`, `root_id={-1}` | `type=0` | `type=1866598768` |
| `g++ -std=c++17 -O2` | `type=0`, `root_id={-1}` | `type=0` | `type=0` |
| `clang++ -std=c++17 -O2` | `type=0`, `root_id={-1}` | `type=0` | `type=905987520` |

`map_size` went from 0 to 1 in every run, so `operator[]` did insert.
`is_aggregate=0`, `is_trivially_default_constructible=0` in all three.

`BufferType::Root` is the first enumerator, at `kernel_utils.h:24`, so `type=0` **is**
`Root`. The control row is the proof of well-definedness: default-initialisation leaves
`type` indeterminate and shows garbage under two of the three configurations, while
value-initialisation gives zero under all three. That is the zero-initialise-then-run-the-
non-trivial-default-constructor path. **Well defined, not undefined behaviour.**

**Both passes are correct. One detail in each is not.** Pass A cites the enum as
`kernel_utils.h:22` and the default as `:36-37`; the enum is at `:23-31` with `Root` at
`:24`, and the missing initialiser is at `:34`. Pass A's `kernel_utils.h` citations run two
lines low throughout, including in its escalation E2 and its blast radius. Pass B's line
numbers for this file are exact.

**Both are wrong about the fabricated buffer's name.** Pass A says `Root_-1`; pass B says
`root_-1`. `buffer_instance_name` at `:43-68` returns
`std::string(kRootBufferName) + "_" + fmt::join(b.root_id, "_")`, and
`kRootBufferName` is `"root_buffer"` at `:26`. The actual name is **`root_buffer_-1`**.
Trivial in itself, and worth stating because both passes reported a string neither had
looked up.

---

## 5. Verify item 2 — pass A's history claim. PARTLY FALSE

Pass A: "Upstream already discriminated at this exact site on statement identity, with
`stmt->dest->is<MatrixPtrStmt>()`, until commit `715a04c98` replaced it in August 2023."

`git log -1 715a04c98` gives
`715a04c989e1dfbd56a357b1b55a97ca71514c12`, Bob Cao, **Tue Aug 15 14:28:26 2023 -0700**,
"[vulkan] Fix shared memory atomic float operations (#8315)". The date and the replacement
are real. `git show 715a04c98 -- taichi/codegen/spirv/spirv_codegen.cpp` shows the removed
hunk verbatim:

```
-      if (stmt->dest->is<MatrixPtrStmt>()) {
-        // Shared arrays have already created an accesschain, use it directly.
-        addr_ptr = ir_->query_value(stmt->dest->raw_name());
-      } else {
-        addr_ptr = at_buffer(stmt->dest, dt);
-      }
+      addr_ptr = dest_is_ptr ? dest_val : at_buffer(stmt->dest, dt);
```

**"At this exact site" is false.** That test lived in `visit(AtomicOpStmt)` — today's
`:1633` — not in `at_buffer`, and not at `:2197`. It is at the *caller*, and it decided
something different: whether to call `at_buffer` at all, separating "already a SPIR-V
access chain" from "an integer to be interpreted". It never separated a physical device
address from a buffer-relative offset. Pass A's §1 presents it under the heading of the
`:2197` predicate, which makes a caller-side guard read as a precedent at the decision point.
It is a precedent for discriminating on something other than width; it is not a precedent at
that site or for that question.

The same diff also confirms two other pass A claims exactly: `715a04c98` added the
`TI_ERROR_IF(!is_integral(...))` backstop now at `:2207-2210` in the same change, and it
left the three `at_buffer` calls now at `:1617`, `:1626` and `:1681` untouched. Pass A's
gloss that they were left unguarded "deliberately" is an inference about intent that the
diff does not carry; the mechanical fact does.

**Pass B handles the same material better.** Its §3.5 identifies today's replacement,
`dest_is_ptr = dest_val.stype.flag == TypeKind::kPtr` at `:1612`, states correctly that
`SType::flag` is set by `get_pointer_type` (`spirv_ir_builder.cpp:407-424`, which I read and
which indeed never assigns `SType::dt`), and states correctly that it **does not** separate
physical-u64 from relative-u64. That is the honest reading of the same history. Pass B does
not connect it to the 2023 commit; pass A does but mislocates it.

---

## 6. Verify item 4 — pass A's unsettled counterexample. I CAN SETTLE IT

Pass A's P4: because `:783` sits outside `:797`, a non-array `ExternalPtrStmt` on a
physical-storage device gets a u64 device address at `:792` and a `BufferType::Args` entry
at `:800`, breaking any rule keyed on `type == ExtArr` alone. Pass A verified the source
shape and could not establish reachability, marking it E4 as needing a compiler run. Pass B
raises the mirror image as E3 and also declines it.

**The source shape is real — I read `:783-801` and confirm the nesting.** Reachability I can
settle by enumeration, and neither pass did it.

`grep -rn "ExternalPtrStmt" --include=*.cpp --include=*.h taichi/`, filtered to constructions,
gives **six** construction sites inside `taichi/`:

| Site | base pointer used |
|---|---|
| `taichi/ir/frontend_ir.cpp:705` | `flatten_lvalue` of an `ExternalTensorExpression` |
| `taichi/ir/ir_builder.cpp:450` | the `ArgLoadStmt *` its caller passes |
| `taichi/transforms/scalarize.cpp:1281` | `origin->base_ptr` of an existing `ExternalPtrStmt` |
| `taichi/transforms/auto_diff.cpp:1542` | `src->base_ptr` of an existing `ExternalPtrStmt` |
| `taichi/transforms/auto_diff.cpp:1616` | `dest->base_ptr` of an existing `ExternalPtrStmt` |
| `taichi/transforms/auto_diff.cpp:1688` | `dest->base_ptr` of an existing `ExternalPtrStmt` |

The last four reuse an existing `ExternalPtrStmt`'s `base_ptr`, so they cannot introduce a
new argument identity. `frontend_ir.cpp:705` is inside `make_ndarray_access`, which does
`var.cast<ExternalTensorExpression>()` at `:702`, so its argument is always an external
tensor.

`IRBuilder::create_external_ptr` (`taichi/ir/ir_builder.cpp:446-451`) is the only route that
accepts an arbitrary `ArgLoadStmt`, and **it has no caller inside `taichi/` at all.** Every
caller is in `tests/cpp/` or `cpp_examples/`. I checked each of those kernels' parameter
declarations:

- `cpp_examples/autograd.cpp:179-181` — three `insert_arr_param`.
- `cpp_examples/run_snode.cpp:126` — one `insert_arr_param`.
- `tests/cpp/ir/ndarray_kernel.cpp:24`, `:44` — `insert_ndarray_param`. The scalar parameter
  at `:45` exists but is not the base of any `ExternalPtrStmt`; `:39` uses `arg0`.

`Callable::insert_arr_param` (`taichi/program/callable.cpp:24-28`) and
`insert_ndarray_param` (`:34-50`) both construct `Parameter(..., /*is_array=*/true, ...)`.
`insert_scalar_param` (`:11-13`) passes `false` and is never an `ExternalPtrStmt` base.
`KernelContextAttributes` copies it straight through at
`taichi/codegen/spirv/kernel_utils.cpp:66`, `aa.is_array = ka.is_array;`.

**Therefore `ctx_attribs_->arg_at(arg_id).is_array` is true at every `ExternalPtrStmt` in
this tree, W7 at `:800` is dead, and pass A's P4 is unreachable.** The `Args` disjunct in
pass B's rule is over-broad but harmless, and the operative rule today is simply
`type == ExtArr ∧ cap`.

Two honest qualifications, per standing instruction 7. First, this is a static enumeration
over construction sites plus the clone route (which preserves `base_ptr`); it is not a run.
Second, it says nothing about whether a *future* caller of `create_external_ptr` would be
rejected — nothing enforces the invariant, which is exactly why pass B's E3 is worth keeping
on the record even though the branch is dead. The `indices.push_back(1)` at `:785` reads
args-struct member 1, `DATA_PTR_POS_IN_NDARRAY` (`taichi/ir/type_factory.h`), a field that
exists because the argument is an ndarray, and nothing guards that read on `is_array`.

Pass A was right to refuse to dismiss P4 and right that it could not settle it by the route
it took. It could have been settled by enumerating construction sites rather than by
building the tree.

---

## 7. Verify item 5 — the two pre-existing defects

### 7.1 The missing default initialiser — CONFIRMED

`kernel_utils.h:34`, `BufferType type;`, no initialiser, while `root_id` at `:35` has one.
Section 4 above is the evidence. Both passes report it; pass A's line citation is two low.

### 7.2 Three unguarded `at_buffer` calls — CONFIRMED, and worse than either pass says

`grep -n "at_buffer(" spirv_codegen.cpp` returns nine hits: six inside `visit(AtomicOpStmt)`,
the definition at `:2194`, and the two forwards at `:2233` and `:2257`.

| Line | Context | `dest_is_ptr` guard |
|---|---|---|
| `:1617` | f64, `spirv_has_atomic_float64_add`, op == add | **no** |
| `:1621` | f64 fallback | yes |
| `:1626` | f32, `spirv_has_atomic_float_add`, op == add | **no** |
| `:1630` | f32 fallback | yes |
| `:1633` | all other dtypes | yes |
| `:1681` | integral `AtomicOpType::mul` | **no** |

Six rows read one at a time; three unguarded. `dest_is_ptr` is computed once at `:1612`.
Both passes give this table correctly.

**Neither pass noticed that `:1681` is not merely unguarded — it discards a guard that
already ran.** For an integral dtype, `:1633` assigns
`addr_ptr = dest_is_ptr ? dest_val : at_buffer(stmt->dest, dt)`, which is *correct* for a
shared array. Control then reaches the `is_integral(dt)` branch, and for
`AtomicOpType::mul`, `:1681` **overwrites** `addr_ptr` unconditionally with
`at_buffer(stmt->dest, ir_->get_taichi_uint_type(dt))`. The correct value computed at
`:1633` is thrown away. So an atomic multiply on a shared array is a hard compile error on
**every** device, not because a guard is missing but because a guard's result is overwritten.
Pass A states the every-device outcome correctly (`x[i] *= v` on any device at all) without
identifying the mechanism; pass B files it as I3/E2 with reachability unresolved.

The failure mode both passes describe is right: `dest_val.stype.flag == TypeKind::kPtr` and
`dest_val.stype.dt` is `unknown`, because `IRBuilder::get_pointer_type`
(`spirv_ir_builder.cpp:407-424`) sets `id`, `flag`, `element_type_id` and `storage_class`
and never `dt`, and `DataType::DataType()` is `: ptr_(PrimitiveType::unknown.ptr_)` at
`taichi/ir/type.cpp:20`. So `:2197` is false, `is_integral` at `:2207` rejects `unknown`, and
the `TI_ERROR` fires. Loud, not silent. Confirmed by reading all three files.

Pass B adds that the float cases make this a **capability-dependent** failure — the same
program compiles on a device without native atomic float add and fails on one with it — and
grades that against section 2.2. That framing is correct and pass A does not make it. It is
also the sharper of the two escalations, because section 2.2 is a stated invariant of the
plan.

---

## 8. **THE EMPIRICAL SETTLEMENT** — the conditional, measured

The brief asked whether the root buffer must be physically addressed, and stated that it
turns on whether a bound storage buffer can be large enough, capped by `maxStorageBufferRange`,
which an earlier territory established this tree never queries.

**First, that premise, verified independently.**
`grep -rn "maxStorageBufferRange\|maxUniformBufferRange\|maxPerStageDescriptorStorageBuffers\|maxDescriptorSetStorageBuffers\|maxBoundDescriptorSets\|maxComputeSharedMemorySize\|maxMemoryAllocationCount"`
over `taichi/`, `tests/` and `cpp_examples/` returns **zero hits**. The only limits this tree
reads at all are `limits.maxComputeWorkGroupCount[0..2]` at
`taichi/rhi/vulkan/vulkan_device.cpp:1126-1128` and `limits.timestampPeriod` at `:1945`.
Confirmed: nothing anywhere consults the storage-buffer range.

**Second, the measurement.** `vulkaninfo`, read-only, three physical devices enumerated on
this machine.

| Device | `maxStorageBufferRange` (bytes) | Device-local heap | `maxDescriptorSetStorageBuffers` | `bufferDeviceAddress` | `shaderInt64` |
|---|---|---|---|---|---|
| GTX 1070 (`0x1b81`, driver 535.309.01) | 4294967295 | 8.00 GiB | 1048576 | true | true |
| GTX 750 Ti | 4294967295 | 2.00 GiB | 1048576 | true | true |
| llvmpipe (LLVM 20.1.2, 256 bits) | 134217728 | 15.50 GiB (host) | 1000000 | true | true |

Three rows, one per device reported by the loader. Instance version 1.3.275.

**Third, and this is the part that settles it.** `maxStorageBufferRange` is declared

```
3324:    uint32_t              maxStorageBufferRange;
```

in `external/Vulkan-Headers/include/vulkan/vulkan_core.h:3324`, inside
`VkPhysicalDeviceLimits`. It is a `uint32_t`. **No Vulkan device can ever advertise a
descriptor-bound storage buffer range above 4294967295 bytes, because the field cannot
express one.** Both NVIDIA cards report exactly that maximum. So they are not reporting a
hardware ceiling; they are reporting "as much as the structure can say".

### 8.1 What the numbers decide

`GfxRuntime::add_root_buffer` (`taichi/runtime/gfx/runtime.cpp:730-750`) allocates **one
storage buffer per SNode tree**, sized by `compiled_structs.root_size`
(`taichi/runtime/gfx/snode_tree_manager.cpp:14`), with `AllocUsage::Storage`, and checks
nothing against any device limit. `get_buffer_value` (`:2266-2315`) takes a fresh descriptor
binding per `(BufferInfo, primitive type)` pair, so each root gets its own binding and its
own offset space.

- A u32 byte offset spans 4294967296 bytes — exactly one byte more than the largest range
  any Vulkan device may advertise. **For a descriptor-bound root buffer, 32-bit root-relative
  offsets are sufficient by construction, on every Vulkan device that exists or can exist.**
  Widening them buys nothing.
- On the **GTX 750 Ti** the point is moot twice over: total VRAM (2 GiB) is half the
  expressible cap.
- On the **GTX 1070** the cap (4 GiB − 1) is half the card's 8 GiB, so a single
  descriptor-bound root buffer can use at most half the card.
- On **llvmpipe** the cap is **128 MiB against a 15.50 GiB host heap**, a ratio of about
  124 to 1. This is by far the tightest of the three, and it is the Vulkan software path.

### 8.2 The answer to the conditional

**Physical addressing of the root buffer is neither required below 4 GiB − 1 nor avoidable
above it.** The fork is exactly there, and it is now a measured line rather than an argued
one:

- To hold a single SNode tree whose root storage stays under 4 GiB − 1 (under 128 MiB on
  llvmpipe), nothing about the root's addressing has to change. `use_64bit_pointers` at
  `:82` can stay false, `:2197` never sees a u64 from the `Root` side, and the collision
  pass B's rule is aimed at does not arise for root pointers at all.
- To exceed it in one buffer, the descriptor route is closed by the Vulkan structure itself,
  not by this codebase. The only remaining routes are splitting across more root buffers —
  which the tree already supports, at roughly a million descriptors of headroom on all three
  devices — or `bufferDeviceAddress`, which all three devices support.

So pass B's conditional is real but **not forced by the hardware**, and the cheap route
exists. Its cost estimate stands for the shelved work as shelved. What its E4 should say,
and does not, is that the root-physical variant is the *only* route past 4 GiB − 1 per tree,
and that on the Vulkan software path the wall is 128 MiB.

Grading, per standing instruction 8: the 4 GiB − 1 ceiling is **not** environmental in the
sense of expiring on a driver upgrade. It is a fixed-width field in a published
specification, so it expires only if Khronos widens the field. Practically it should be
treated as permanent. The 128 MiB llvmpipe figure **is** environmental — it is a Mesa
software-renderer choice and could change with a Mesa version.

### 8.3 One thing to weigh that this measurement raises

Section 4.1 of the plan says sparsity is required. `bitmasked_activation` at
`:384-...` computes bitmask word pointers with `ptr_dt = parent_ptr.stype` at `:388`, but
hardcodes `ir_->u32_type()` at `:404` and again at `:411-412`, while calling `make_pointer`
at `:408` — which returns u64 the moment `use_64bit_pointers` is true. That is a width
mismatch on the sparse activation path. Neither pass found it, and neither pass's
`make_pointer` enumeration is complete: `grep -n "make_pointer("` gives **four** call sites,
`:355`, `:371`, `:408` and `:506`, plus the definition at `:2317`. Pass A names three
(`:355`, `:372`, `:508`, each one line off) and pass B names three (`:356`, `:373`, `:510`,
each one line off the other way). Both missed `:408`. It is outside the discrimination
question and I am not extending scope; I am recording it because it falsifies a bound both
reports stated.

---

## 9. Item 3 — the blast radius, derived from my own enumeration

Pass A gives "eight sites in two files". Pass B gives "three predicate sites plus three
fabricating writes", six lines in one file.

### 9.1 Pass A's total contradicts its own list

Pass A's §7 list, counted as written:

| Pass A's bullet | Sites |
|---|---|
| One predicate: `:2197` | 1 |
| One map read: `:2212` | 1 |
| Three propagation writes: `:319`, `:326`, `:332` | 3 |
| One default: `kernel_utils.h:36-37` | 1 |
| Three unguarded calls: `:1617`, `:1626`, `:1681` | 3 |

1 + 1 + 3 + 1 + 3 = **9**. Pass A writes "Eight sites, in two files, counted from the list
above." The list has nine entries. This is precisely the failure standing instruction 9
names, and it is the fifth instance the project has produced. **Section 8.2 of the plan
currently quotes pass A's figure of eight; it should not.**

Pass A's list is also incomplete: it names one predicate site when there are three. Pass B's
own arithmetic is correct — three plus three is six — and its three predicate sites are
right.

### 9.2 My own enumeration

`grep -n "PrimitiveType::u64" spirv_codegen.cpp` returns exactly three hits: `:2197`
(`at_buffer`), `:2227` (`load_buffer`), `:2249` (`store_buffer`). The latter two select
`ti_buffer_type`, suppressing the uint-reinterpret-and-bitcast that the relative path needs.
Same question, same answer required.

For the discrimination question alone, and **not** for the whole of item 6.2:

**Unconditional — must change under any route (3 sites, 1 file):**

| Site | What it is |
|---|---|
| `:2197` | `at_buffer` width predicate |
| `:2227` | `load_buffer` width predicate |
| `:2249` | `store_buffer` width predicate |

**Conditional on the map route rather than the statement-identity route (4 sites, 2 files):**

| Site | Why |
|---|---|
| `:319` | fabricating `operator[]` read, shared `AllocaStmt` origin |
| `:326` | fabricating `operator[]` read, `GlobalTemporaryStmt` origin |
| `:332` | fabricating `operator[]` read, byte-offset arm |
| `kernel_utils.h:34` | `BufferType type;` with no default initialiser |

These four are needed only if the map is consulted where a fabricated `Root` could be read,
which means anywhere above the `!is_integral` backstop at `:2207-2210`. Under the
`ptr->is<ExternalPtrStmt>() ∧ caps_` route, none of them has to change. **Under the
root-physical variant of section 3, all four become unconditional**, because a fabricated
`{Root, {-1}}` would then classify as a physical address.

**Behaviourally coupled — change if the guard order changes (3 sites, 1 file):**

`:1617`, `:1626`, `:1681`. `:1681` additionally overwrites the guarded assignment made at
`:1633`, per section 7.2.

**Not a change site:** `:2212`, the existing `ptr_to_buffers_.at(ptr)` read. It is already
there and stays as it is. Pass A counts it; I do not.

**Total: 10 sites in 2 files** — 3 unconditional, 4 conditional on route, 3 behaviourally
coupled. 3 + 4 + 3 = 10, which is the number of rows in the three tables above. Files:
`taichi/codegen/spirv/spirv_codegen.cpp` (9 rows) and
`taichi/codegen/spirv/kernel_utils.h` (1 row); 9 + 1 = 10.

**Grade: ARCHITECTURAL**, and both passes grade it the same way for the same reason. Applying
section 10 item 8's test: if every driver, hardware generation and specification version were
ideal today, the obstacle would still be there, because it exists from a decision inside this
codebase to encode addressing mode in a pointer's scalar width and nowhere else. Both passes
are right that this is materially smaller than "a change to that backend's value model",
which is what section 8.2 records.

**What my enumeration can and cannot see.** It keys on three unique identifiers —
`ptr_to_buffers_`, `at_buffer`, `make_pointer` — and on the literal `PrimitiveType::u64`. The
first three are unabbreviated and unique to this file, so those sets are complete. The u64
literal search would **not** see a width test written another way, for example via
`is_integral`, a `get_primitive_type_size` comparison, or a `DataType` equality helper. I did
not construct a search that could see those, and I say so rather than implying a closed set.

---

## 10. Where the two passes stand against each other

**On the principal rule, pass B is the stronger document.** Its biconditional is structural
and needs no reachability claim, its line numbers are accurate, its arithmetic reconciles,
its three predicate sites are complete where pass A's one is not, and its section 3.5 gets
the `SType::flag` precedent right where pass A mislocates the same history.

**Pass A contributes three things pass B does not.** The `ptr->is<ExternalPtrStmt>()` route,
which is simpler than the map route and, per section 9, needs none of the four conditional
sites. The counterexample P4, which pass B raises only as its own E3 and also declines —
raising it was right even though pass A could have settled it by enumeration rather than by
building the tree. And the observation that the `!is_integral` backstop sits *between* the
predicate and the map read, which is what makes the four conditional sites conditional.

**Both are wrong in the same place once:** the fabricated buffer's name is `root_buffer_-1`,
not `Root_-1` and not `root_-1`.

**Both state a `make_pointer` set that is missing `:408`**, and that omission sits on the
sparse activation path, which section 4.1 makes non-negotiable.

**Neither settles the empirical question**, and pass B says so plainly in its section 5,
calling it the single largest determinant of the item's cost. It was right about that. The
answer is in section 8: the Vulkan limit structure caps a descriptor-bound storage buffer at
4 GiB − 1 by the width of a `uint32_t` field, both cards report exactly that, the software
renderer reports 128 MiB, and this tree queries none of it.

---

## 11. Escalations

Raised, not resolved, per standing instructions 2 and 3.

**A1. Section 8.2's quoted figure of "eight sites in two files" is pass A's, and it
contradicts pass A's own list, which has nine entries and is missing two predicate sites.**
My own derivation in section 9 is 10 sites in 2 files, split 3 unconditional / 4 conditional
on route / 3 behaviourally coupled. The plan should not carry the eight.

**A2. Pass B's stated limit is a sufficient condition presented as a necessary one.** A
uniform "all roots physical when the capability is on" policy keeps the map rule sound. Only
a per-buffer policy collapses it. But the fabricated `{Root, {-1}}` from `:319` collapses it
under *any* root-physical policy, which makes `kernel_utils.h:34` and `:319`/`:326`/`:332`
preconditions of that variant rather than independent defects. Section 3.

**A3. The measured ceiling, and the thing to decide.** `maxStorageBufferRange` is a
`uint32_t` in `external/Vulkan-Headers/include/vulkan/vulkan_core.h:3324`, so 4 GiB − 1 is a
specification-level cap on any descriptor-bound storage buffer, not a property of these
cards. Measured: 4294967295 on the GTX 1070, 4294967295 on the GTX 750 Ti, 134217728 on
llvmpipe. This tree queries the limit nowhere. The decision this forces is whether one SNode
tree must ever exceed 4 GiB − 1 of root storage — under 128 MiB on the Vulkan software path.
If not, item 6.2 does not need root-physical addressing and pass B's cost estimate stands.
If so, `bufferDeviceAddress` is the only route, and section 9's four conditional sites become
unconditional. I am not deciding it.

**A4. `:1681` overwrites the correctly guarded `addr_ptr` computed at `:1633`.** So an
atomic multiply on a shared array is a hard compile error on every device by discarding a
guard, not by lacking one. Neither pass identified the mechanism. Pre-existing, not a
consequence of item 6.2. I have fixed nothing.

**A5. `bitmasked_activation` mixes widths under `use_64bit_pointers = true`.** `make_pointer`
at `:408` against hardcoded `ir_->u32_type()` at `:404` and `:411-412`. On the sparse path.
Outside the discrimination question; recorded because it falsifies a bound both reports
stated, and per standing instruction 9 a scope ruling removes work, not evidence.

**A6. Nothing enforces that an `ExternalPtrStmt`'s argument is an array.** W7 at `:800` is
dead today by my enumeration in section 6, and `:783-791` reads args-struct member 1
unguarded on `is_array`. A future caller of `IRBuilder::create_external_ptr` would break it
silently. Pass B's E3 should stay on the record even though the branch is dead.

---

## 12. Constraints observed

I modified exactly one file, this one. No source file was touched. All `git` use was
`log`, `show` and `status`. All system queries were read-only: `vulkaninfo`, `nvidia-smi`
with `--query-gpu`, and header reads. The only thing I compiled was a standalone replica of
`BufferInfo` in the session scratchpad, which links nothing from this tree. I decided nothing
unnecessary, added no abstraction, proposed no remedy, and derived every count in this file
from the enumeration printed beside it.

`adversary-07-2.md` did not exist when I finished, so there is no divergence section.

---

# 13. Appended after reading `adversary-07-2.md`

`adversary-07-2.md` did not exist when I wrote sections 1 to 12. It landed at
37473 bytes and I have now read it in full. Nothing above has been edited; this
section is added.

It is a strong report and it worked against a **later plan than I did**: it
records 48101 bytes where I read 45208. I re-read the plan; the growth is in
sections 6.1 and 8.2, and section 8.2 item 0's "UNDER REVISION, 2026-09-09"
paragraph is the material one. It names territory 07 and instructs that neither
pass's blast-radius figure be acted on until this territory closes. That does not
change anything I concluded, and it reinforces section 11 item A1.

## 13.1 Where we independently converge

Reached separately, blind, and agreeing row for row. Convergence on these is
worth as much as the divergences below.

- A discriminator exists at `:2197` and the inherited sentence is false.
- The ten `ptr_to_buffers_` hits, the seven writers, and pass B's biconditional,
  which we both re-derived from the writers rather than checking pass B's
  derivation, and neither of us could break.
- The value-inserting read: `{Root, {-1}}` under three compiler configurations,
  with a dirtied-storage control proving the zero comes from zero-initialisation.
  Well-defined, not undefined behaviour. Three independent replicas now agree.
- Pass A's history claim is **partly false**: `715a04c98` is real and dated
  15 Aug 2023, but the removed `stmt->dest->is<MatrixPtrStmt>()` was in
  `visit(AtomicOpStmt)` at today's `:1633`, not at `:2197`, and it discriminated
  a different question. "Deliberately" is not carried by the diff.
- Pass A's blast radius omits `:2227` and `:2249`, and its stated total of eight
  contradicts its own nine-row list.
- Both passes name the fabricated SSBO wrongly; it is `root_buffer_-1`.
- Pass A's `kernel_utils.h` citations are two lines low; the missing initialiser
  is at `:34`.
- The three unguarded `at_buffer` calls, and that `:1681` **overwrites** the
  correctly guarded assignment made at `:1633`. We found the overwrite
  separately. Adversary 2 credits both passes with it; on my reading neither pass
  states the overwrite, only the missing guard.
- P4 / W7 is **dead**, and pass A could have settled it by enumeration.
- `maxStorageBufferRange` measures 4294967295 on both NVIDIA cards and 134217728
  on llvmpipe. Two independent measurements, identical.
- The tree queries no storage-buffer limit anywhere; only
  `maxComputeWorkGroupCount` (`taichi/rhi/vulkan/vulkan_device.cpp:1126-1128`)
  and `timestampPeriod` (`:1945`).

## 13.2 Where adversary 2 is right and I was not — conceded

**1. `maxMemoryAllocationSize`. I did not query it; I should have.** Verified
myself before conceding: `0xffe00000` = 4292870144 on both NVIDIA cards,
`0x80000000` = 2147483648 on llvmpipe. So the cap on one *descriptor-bound*
storage buffer is `min(maxStorageBufferRange, maxMemoryAllocationSize)`, which is
4292870144 on NVIDIA, not the 4294967295 I gave in section 8. Adversary 2's
"2 MiB below the u32 reach" is exactly right and tightens my figure. **My section
8.1 should read 4292870144 for the NVIDIA cards.** On llvmpipe the minimum is
still 134217728, so my figure there stands.

**2. Six parameter routes, not three.** My section 6 checked
`insert_scalar_param`, `insert_arr_param` and `insert_ndarray_param` because
those are the ones the in-tree `ExternalPtrStmt` sites actually use. Adversary 2
read all six in `taichi/program/callable.cpp` and found `insert_texture_param`
(`:57`), `insert_pointer_param` (`:70`) and `insert_rw_texture_param` (`:77`)
also hardcode `is_array = true`, two with `FIXME`s admitting the flag is abused.
I confirm all six from my own grep of that file (`:13` false, `:28`, `:50`,
`:63`, `:72`, `:82` true). **Its enumeration closes the question for the class,
where mine closed it only for the sites in hand.** Its formulation is also
better: `:800` is unreachable because `is_array` has been overloaded to mean "is
a pointer-like argument", not because non-array arguments are rare.

**3. `:2227` and `:2249` are new failure surfaces, not edits.** Neither consults
`ptr_to_buffers_` today. A map-based rule introduces a `.at()` lookup at each,
which throws `std::out_of_range` on an absent key. I read both and confirm.
Neither pass says it and neither did I.

**4. `:2212` belongs in the count.** I excluded it in section 9 as "not a change
site", because its text does not change. Adversary 2 puts it in a **tier 2,
"behaviour changes without being edited"**, which is the correct treatment: under
a map consultation placed above the `:2207` guard, that line's meaning changes
even though its text does not. Adopting that framing, my total in section 9
becomes **11 sites in 2 files**, matching adversary 2's, and the two derivations
then agree row for row.

**5. Pass B's countable errors that I did not check.** `at_buffer` has **eight**
call sites where pass B says seven — `grep -n "at_buffer("` gives `:1617`,
`:1621`, `:1626`, `:1630`, `:1633`, `:1681`, `:2233`, `:2257`, plus the
definition at `:2194`. And `grep -c "ir_->register_value("` gives 33, with one
commented out at `:600-601`, so **32 live**, not pass B's 31. Both verified by me
after reading adversary 2, both correct, neither affects pass B's conclusions.

**6. The atomic-float capability measurement.** Adversary 2 measured what both
passes left as a conditional, and I did not think to. Verified independently:
`shaderBufferFloat32AtomicAdd` is true on all three devices;
`shaderBufferFloat64AtomicAdd` is true on the GTX 1070 and **false** on both the
GTX 750 Ti and llvmpipe. The mapping is at
`taichi/rhi/vulkan/vulkan_device_creator.cpp:741-746`. So the f32 unguarded path
at `:1626` is live on every device on this box, and the f64 path at `:1617` is
live on the 1070 alone — the same program compiles on the older card and fails on
the newer one. That is a measured section 2.2 violation, and it is the single
best move in either adversary file.

## 13.3 Where I diverge from adversary 2 — and the source supports me

**D1. Its central empirical conclusion is stated too broadly, and the machine
falsifies the general form.** Adversary 2 writes:

> `maxMemoryAllocationSize` caps the underlying allocation at 4292870144 bytes on
> NVIDIA and 2147483648 on llvmpipe, so device addresses do not escape the
> ceiling either.

and concludes that the root buffer "cannot be made large enough to need physical
addressing on this hardware". The premise about a single allocation is true. The
conclusion does not follow, because **the ceiling that binds is not the same
ceiling in the two addressing modes.**

`maxStorageBufferRange` limits the `range` of a storage-buffer *descriptor*. A
`PhysicalStorageBuffer` access uses a raw u64 and no descriptor at all, so that
limit does not apply to it. The two are separate caps and the measurement
separates them:

| Device | descriptor-bound cap | physically-addressed cap | ratio |
|---|---|---|---|
| GTX 1070 | 4292870144 | 4292870144 | 1.0 |
| GTX 750 Ti | 2147483648 (whole card) | 2147483648 | 1.0 |
| llvmpipe | **134217728** | **2147483648** | **16** |

Three rows, one per device. The NVIDIA rows carry adversary 2's conclusion
exactly: descriptor range and allocation size coincide there, so physical
addressing buys nothing. **The llvmpipe row does not.** On the Vulkan software
renderer the descriptor range is 128 MiB while a single allocation may be 2 GiB,
so physically addressing the root buffer is precisely the thing that lifts one
SNode tree from 128 MiB to 2 GiB — a factor of sixteen, on the path that serves
the no-GPU case section 2 puts first.

So pass B's antecedent **does fire on one of the three devices**, and adversary
2's E1 — "the conditional resolves in favour of the cheaper answer" — holds on
the two NVIDIA cards and fails on the software renderer.

**D2. A second, narrower falsifier on the GTX 1070, which I mark as a route and
not a fact about this code.** `maxBufferSize` measures `0xffffff0000` on both
NVIDIA cards, about 1024 GiB, against `maxMemoryAllocationSize` of 4292870144.
`sparseBinding` is true on all three devices and `sparseResidencyBuffer` is true
on the GTX 1070 and llvmpipe (false on the 750 Ti). A sparsely bound `VkBuffer`
may therefore exceed one allocation, and a device address over it is one
contiguous range while a descriptor over it is still capped at 4294967295. So
"device addresses do not escape the ceiling" is falsified in principle on the
1070 as well.

I will not overstate this. `GfxRuntime::add_root_buffer`
(`taichi/runtime/gfx/runtime.cpp:730-750`) makes one plain
`allocate_memory_unique` call and this tree has no sparse-binding machinery. D2
is a measured possibility, not a present capability, and it would need
machinery the fork does not have. **D1 needs no new machinery at all**, which is
why I weight it far higher.

**D3. We grade the environmental bound differently, and the header settles it.**
Adversary 2 grades the scope bound ENVIRONMENTAL and retestable, saying what
would lift it is "a device reporting `maxStorageBufferRange > 4294967295`
together with a matching `maxMemoryAllocationSize`", and calls it "a number to
re-query on new hardware".

**The first half of that can never be re-queried higher.** In
`external/Vulkan-Headers/include/vulkan/vulkan_core.h`:

```
3324:    uint32_t              maxStorageBufferRange;
5812:    VkDeviceSize          maxMemoryAllocationSize;
7575:    VkDeviceSize          maxBufferSize;
```

`maxStorageBufferRange` is a `uint32_t` in `VkPhysicalDeviceLimits`. No Vulkan
device can advertise a value above 4294967295 because the field cannot express
one. Both NVIDIA cards report exactly that maximum, which means they are not
reporting a hardware ceiling at all — they are reporting the largest number the
structure can hold. The other two limits are `VkDeviceSize`, 64-bit, and can
genuinely rise.

So the bound is **split, not uniform**: the `maxMemoryAllocationSize` half is
environmental and retestable, exactly as adversary 2 says; the
`maxStorageBufferRange` half is fixed by a published specification and should be
treated as permanent unless Khronos widens the field. That distinction is what
makes D1 durable rather than incidental: descriptor-bound root access can never
exceed 4 GiB − 1 on any Vulkan device, so a u32 root offset is sufficient for
that mode **by construction**, and physical addressing is the only route past it
anywhere, not merely here.

**D4. `make_pointer` has four call sites, and adversary 2 also gives three.**
`grep -n "make_pointer("` returns `:355`, `:371`, `:408`, `:506`, plus the
definition at `:2317`. Both passes name three and miss `:408`; adversary 2 does
not raise the site at all. `:408` is inside `bitmasked_activation` (`:384`),
which sets `ptr_dt = parent_ptr.stype` at `:388` but hardcodes `ir_->u32_type()`
at `:404` and again at `:411-412`. Under `use_64bit_pointers = true`,
`make_pointer` at `:408` returns u64 into that u32 arithmetic. It is on the
**sparse activation path**, which section 4.1 of the plan makes non-negotiable.
Outside the discrimination question; recorded because it falsifies a bound three
documents have now stated, and per standing instruction 9 a scope ruling removes
work, not evidence. This is section 11 item A5 and it stands unchallenged.

**D5. A framing difference that is not a disagreement.** Adversary 2's tier 1
makes `:319`, `:326`, `:332` and `kernel_utils.h:34` unconditional. Mine makes
them conditional. We are scoping differently, not contradicting: adversary 2
states its antecedent explicitly as "replacing width-based discrimination with
the map-plus-capability rule", and under that route they are unconditional and it
is right. Under the `ptr->is<ExternalPtrStmt>() ∧ caps_` route, which pass A
raises and adversary 2 does not cost, none of the four has to change. **The
choice of route is worth four sites and one file, and that is a number the
planner should have.** Neither of us is authorised to choose the route.

I also keep section 3's point, which adversary 2 does not make: under any
root-physical variant the fabricated `{Root, {-1}}` from `:319` stops being a
wrong descriptor binding and becomes a raw pointer forged by `OpConvertUToPtr` at
`:2198-2202` from a workgroup access chain. Given D1, that variant is live on
llvmpipe, so those four sites are not hypothetical there.

## 13.4 Net

On the source, adversary 2 is right and I was incomplete on five things, all
conceded in 13.2, and my section 8.1 figure for the NVIDIA cards should be
4292870144. On the empirical conclusion that decides item 6.2's cost, the
machine supports me: adversary 2's E1 is correct for the two NVIDIA cards and
wrong for the software renderer, where the descriptor range is 128 MiB against a
2 GiB allocation cap and physical addressing is worth a factor of sixteen on the
first-class no-GPU path. Pass B's conditional therefore does not close. It
narrows to one device, and that device is the one section 2 cares about most.

Both of us reached, separately, the same verdict on the two reports: pass B is
the better document, pass A contributes the guard-order coupling and the
statement-identity route that pass B lacks, and neither report contains an error
that reverses its main conclusion.

I modified exactly one file, this one. No source file was touched. All git use
was read-only and all system queries were read-only.
