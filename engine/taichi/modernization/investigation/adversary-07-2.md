# Adversary 07-2 — the discriminator question

Adversary 2 of 2 on territory 07. Judging `report-07-discriminator.md` (pass A)
and `report-07b-discriminator.md` (pass B), written blind to each other.

**Plan version worked against:** `modernization/PROJECT-PLAN.md`, "Last updated:
2026-09-09", 48101 bytes at the time of this pass. This is a LATER version than
either pass worked against: both record re-reading it at 45208 bytes. The
material that grew since is in sections 6.1 and 8.2; section 8.2 item 0 now
carries an **"UNDER REVISION, 2026-09-09"** paragraph that names territory 07 by
name and instructs that neither pass's blast-radius figure be acted on until
this territory closes. I read section 10's priority preamble and item 8 on
grading with blast radius stated separately, and section 2.2a, which puts item
6.2 in scope as BASE work in full, including the `at_buffer` pointer-width
collision by name.

**Unqualified line numbers are `taichi/codegen/spirv/spirv_codegen.cpp`.**

`adversary-07-1.md` did not exist when I finished. Nothing is appended for it.

---

## 0. Verdict in one paragraph

Both passes are substantially right and pass B is the better report. A
discriminator does exist; the inherited "none exists" sentence is false; I
confirmed that from source before reading anything else. Pass B's rule survives
every attack I could mount on it, including its own proof of the biconditional,
which I re-derived independently and which holds. **Pass B's stated LIMIT,
however, is a conditional whose antecedent I can now show does not fire on any
device on this machine, and that is the result that decides the cost of item
6.2.** A 32-bit byte offset reaches 4 GiB. Every one of the three devices here
caps a single bound storage buffer at or below that: exactly 2 MiB below it on
both NVIDIA cards, and a factor of 32 below it on the software renderer. The
root buffer therefore cannot be made large enough to need physical addressing on
this hardware, so `Root` does not land on both sides, and pass B's rule is not
merely sufficient for the shelved case — it is sufficient for every case
reachable on the target set. I also settled pass A's unresolved counterexample
against pass A: it is dead, for a stronger reason than pass A guessed. Both
passes carry countable errors, listed in section 8.

---

## 1. The prior question, verified independently first

The brief required me to verify, before anything else, that a discriminator
exists. Four documents and two adversaries had asserted the opposite.

`grep -rn "ptr_to_buffers_" --include=*.cpp --include=*.h .` over the whole tree
returns **ten hits, all in one file**: seven writes (`:319`, `:326`, `:332`,
`:377`, `:711`, `:798`, `:800`), one `count` assert (`:376`), one read (`:2212`),
one declaration (`:2635`). I ran this myself. Both passes' tables are correct
row for row; I checked every one of the seven against the source.

At the disputed predicate `:2197`, three things are in scope and none is a
function of the pointer's scalar width:

- `ptr`, the `const Stmt *` parameter of `at_buffer` (`:2194`).
- `ptr_to_buffers_`, read one statement below at `:2212`.
- `caps_`, a `DeviceCapabilityConfig *` member at `:2565`.

**The sentence is false.** Confirmed. Whether any of the three is a *sound*
discriminator is section 2.

---

## 2. The principal task — testing pass B's rule

Pass B states the rule as:

```
physical  ⟺  ptr_to_buffers_.at(ptr).type ∈ {ExtArr, Args}
             ∧  caps_->get(DeviceCapability::spirv_has_physical_storage_buffer)
```

### 2.1 The map alone genuinely cannot work — confirmed

Pass B's ground for rejecting the map alone is that both `ExtArr` and `Args` sit
on both sides. Read `:783-801` as one block. The capability test at `:783` is
what decides whether a u64 device address (`:792`) or an i32 linear offset
(`:794`) is registered. The `ExtArr` versus `Args` choice is a **separate,
later** test at `:797`, outside the capability branch entirely. So the same
`ExternalPtrStmt` writes the same map value whether or not the capability is on.

Confirmed at source. Pass A reaches the same conclusion in its section 5.1.
Both are right, and any formulation without the `caps_` conjunct is wrong.

### 2.2 Pass B's proof needs no reachability argument — I re-derived it, it holds

Pass B claims the biconditional `type ∈ {ExtArr, Args} ⟺ k is an
ExternalPtrStmt` is structural. I derived it again from the seven writers rather
than checking pass B's derivation.

**Only `:798` and `:800` write those two values**, and both key on the
`ExternalPtrStmt` currently being visited. The only other writes that can move a
value onto a new key are the three propagation sites, each written
`ptr_to_buffers_[stmt] = ptr_to_buffers_[stmt->origin];`. Take them in turn:

- **`:319`** sits inside `if (stmt->origin->is<AllocaStmt>())` (`:313`). Its
  origin is an `AllocaStmt`. No writer anywhere writes an `AllocaStmt` key —
  `visit(AllocaStmt)` at `:284-304` writes none. So the origin's entry is always
  fabricated, and fabrication yields `Root` (section 4.1).
- **`:326`** sits inside `else if (stmt->origin->is<GlobalTemporaryStmt>())`
  (`:323`). Only `:711` writes a `GlobalTemporaryStmt` key, and it writes
  `GlobalTmps`.
- **`:332`** sits in the `else` of `offset_used_as_index()`. That predicate
  (`taichi/ir/statements.h:521-529`) returns true when the origin is
  `AllocaStmt`, `GlobalTemporaryStmt`, `ExternalPtrStmt` or `MatrixPtrStmt`, so
  in the `else` the origin is **none of those four**. It cannot be an
  `ExternalPtrStmt`, so it cannot hold `ExtArr` or `Args`. Of the remaining
  writers only `:377` can have written its key, and `:377` writes `Root`.

So no propagation can move `ExtArr` or `Args` onto another key. **Pass B's
left-to-right direction holds, and it holds structurally.** Right to left holds
because `:797-801` is outside the capability branch, so every visited
`ExternalPtrStmt` writes one of the two unconditionally. Verified.

I also checked the one hole this style of argument usually leaves: whether a
physical address can be **derived** from another and escape the map. It cannot.

- `grep -n "u64" spirv_codegen.cpp` returns **nine hits and no more**: `:270`,
  `:272` (a u64 `ConstStmt` value), `:787`, `:789`, `:790` (the physical address
  at `:792`), `:2197`, `:2227`, `:2249` (the three predicates), and `:2320`
  (`make_pointer` under `use_64bit_pointers`, which is `false` at `:82`).
  Exactly one of these produces a device address.
- `MatrixPtrStmt` over an `ExternalPtrStmt` is unreachable twice over. The
  index-offset branch hits `TI_NOT_IMPLEMENTED` at `:328`, and `FuseMatrixPtr`
  at `taichi/transforms/scalarize.cpp:1261-1300` folds the pair into a single
  fused `ExternalPtrStmt` and erases the `MatrixPtrStmt` (`:1281-1300`). I read
  the pass; pass B marks this as inference I1 and is right to.

**A physical address is always produced directly at `:792`, never derived.**
Pass B's claim stands.

### 2.3 Testing the LIMIT harder

Pass B's limit: the rule infers addressing mode from buffer identity crossed
with capability, and is sound only because exactly one buffer class is
physically addressed today. `BufferType`
(`taichi/codegen/spirv/kernel_utils.h:23-31`) has no addressing-mode axis. If
item 6.2 addresses the ROOT buffer physically, `Root` lands on both sides and
the map cannot carry it.

**As a statement of logic this is correct and I could not break it.** The enum
at `kernel_utils.h:23-31` has seven enumerators and none of them names an
addressing mode; I read it. `BufferInfo` at `:33-59` carries `type` and
`root_id` and nothing else. If two different `Root` entries had to mean
different addressing modes, the map could not distinguish them, and pass B's
rule would collapse to a rule about `ExtArr` only.

So the whole cost of item 6.2's discrimination half hangs on one conditional:
**must the root buffer be physically addressed?** Neither pass could answer it.
Pass B says so explicitly in its section 5 and calls it "the single largest
determinant of this item's cost". Section 3 answers it.

---

## 3. Settling the conditional empirically

This is the part of my brief that outranks the code reading, per section 10's
preamble, and it is the most important section of this report.

### 3.1 The measurement

Read-only query of the Vulkan loader on this machine, `vulkaninfo` from the
installed Vulkan SDK, instance version 1.3.275. Three physical devices are
enumerated. No device was created, no memory allocated, nothing written.

| Device | `maxStorageBufferRange` | `maxMemoryAllocationSize` | device-local heap |
|---|---|---|---|
| NVIDIA GeForce GTX 1070 | 4294967295 | 4292870144 | 8589934592 |
| NVIDIA GeForce GTX 750 Ti | 4294967295 | 4292870144 | 2147483648 |
| llvmpipe (LLVM 20.1.2, 256 bits) | 134217728 | 2147483648 | 16643198976 |

All figures in bytes. Driver 535.309.01 on both NVIDIA cards, `apiVersion`
1.3.242. The software renderer's heap is system RAM.

**Confirmed: this tree never queries the limit.** `grep -rn
"maxStorageBufferRange\|maxMemoryAllocationSize\|maxUniformBufferRange" taichi/`
returns nothing. The only two `VkPhysicalDeviceLimits` members read anywhere are
`maxComputeWorkGroupCount` at `taichi/rhi/vulkan/vulkan_device.cpp:1126-1128`
and `timestampPeriod` at `:1945`. The earlier territory's finding holds and I
re-derived it rather than inheriting it.

### 3.2 What the numbers mean

A u32 byte offset addresses 4294967296 bytes. `make_pointer` at `:2317-2322`
returns `ir_->uint_immediate_number(ir_->u32_type(), uint32_t(offset))` today,
so root offsets are u32 byte offsets, and that is their reach.

The largest single bound storage buffer is `min(maxStorageBufferRange,
maxMemoryAllocationSize)`:

| Device | cap on one bound storage buffer | versus the u32 reach |
|---|---|---|
| GTX 1070 | 4292870144 | 2097152 bytes BELOW it |
| GTX 750 Ti | 4292870144 | 2097152 bytes BELOW it |
| llvmpipe | 134217728 | a factor of 32 below it |

**On every device on this machine, the device limit binds before the 32-bit
offset does.** The u32 offset has 2 MiB of headroom on the NVIDIA cards that it
can never spend, and 32 times more reach than the software renderer will accept.

### 3.3 Why that settles the conditional

A root buffer is **one buffer**. `GfxRuntime::add_root_buffer`
(`taichi/runtime/gfx/runtime.cpp:730-750`) makes a single
`allocate_memory_unique` call with `AllocUsage::Storage` per root, and each root
gets its own binding. So the u32 offset caps one root buffer, not the total: N
roots give N separate buffers, each with its own base and its own u32 offset.

Physically addressing the root buffer would therefore be motivated only if a
**single** SNode tree's root buffer had to exceed roughly 4 GiB. It cannot:

- On the **GTX 750 Ti**, the whole card is 2147483648 bytes. Half the u32 reach.
  The question can never arise there.
- On the **software renderer**, a bound storage buffer is capped at 134217728
  bytes, a thirty-second of the u32 reach, despite a 15.5 GiB heap. Widening the
  offset there buys nothing at all; the limit is the renderer's.
- On the **GTX 1070**, the only card where 4 GiB is even physically present, the
  driver still refuses to allocate or bind more than 4292870144 bytes in one
  buffer, which is *less* than the u32 offset already reaches. One SNode tree
  would have to consume over half an 8 GiB card before the question became
  interesting, and at that point the device stops it, not the offset width.

**Conclusion. Pass B's limit is a true statement about a case that does not
arise on the target set.** Its antecedent — item 6.2 addressing the root buffer
physically — is not motivated by any measured device here, because a physical
address cannot make a single allocation bigger than a buffer-relative offset
already reaches. `maxMemoryAllocationSize` caps the underlying allocation at
4292870144 bytes on NVIDIA and 2147483648 on llvmpipe, so device addresses do
not escape the ceiling either. Exceeding 4 GiB of root data requires **multiple
allocations regardless of addressing mode**, which is what the per-root binding
scheme already does.

So the honest formulation, which neither pass could reach:

> The map-plus-capability rule is sufficient for item 6.2 as scoped, and the
> case that would break it is unreachable on all three devices on this machine.
> It would become reachable only on hardware whose `maxStorageBufferRange` and
> `maxMemoryAllocationSize` both exceed 4 GiB **and** a design that puts more
> than 4 GiB in one SNode tree. Neither condition holds here.

Per section 10 item 8 this is an **ENVIRONMENTAL** boundary on an
**ARCHITECTURAL** obstacle, and the two must be graded separately. The
pointer-width collision itself is architectural, as both passes say and as I
agree: it exists because the SPIR-V value model encodes addressing mode in
scalar width, a decision inside this codebase. But the *scope* of the remedy is
bounded environmentally, by a device limit, and that bound is retestable. What
would have to change for it to lift: a device reporting
`maxStorageBufferRange > 4294967295` together with a matching
`maxMemoryAllocationSize`. **This is a number to re-query on new hardware, not
an argument to re-run.** It is also, on the evidence above, the cheapest single
fact in this territory: it converts pass B's open conditional into a closed one.

**Caveat I will not smooth.** I measured three devices on one machine. Section
5.2's upper tier names an RTX 3060 that is not installed and that I could not
query. The conclusion is about the target set in hand, which is what section
8.2's narrowing precedent did for the capability question, and it carries the
same limit: it does not bind a future device.

---

## 4. The five also-verify items

### 4.1 Value-inserting read and the −1 root buffer — CONFIRMED, and well-defined

Both passes report this and both compiled a replica. I compiled a third,
independently, rather than trust either. Verbatim replica of
`kernel_utils.h:23-49`, under `g++ -O0`, `g++ -O2` and `clang++ -O2`,
identically on all three:

| Property | Result |
|---|---|
| `is_aggregate<BufferInfo>` | 0 |
| `is_trivially_default_constructible<BufferInfo>` | 0 |
| map `count` before the read | 0 |
| map `count` after the read | 1 |
| `type` from `m[missing]` | 0, and `BufferType::Root` is 0 |
| `root_id.size()` / `root_id[0]` | 1 / −1 |
| control: placement-new default-init over a 0x5A-filled buffer | 1515870810 |

The control row is mine and is the part that matters: default-**initialisation**
over dirtied storage leaves `type` at 0x5A5A5A5A, which proves the zero in the
rows above comes from the zero-initialisation step and not from luck. Five
repeated trials on dirtied stack gave `type=0, root_id[0]=-1` every time.

**It is well-defined, not undefined behaviour.** `BufferInfo() = default;` at
`kernel_utils.h:37` is user-declared but **not user-provided**, because it is
defaulted on its first declaration. The `root_id{-1}` initialiser at `:35` makes
the constructor non-trivial. Value-initialisation of such a class
zero-initialises the object and then runs the default constructor, which sets
`root_id` and leaves the zeroed `type` alone. `BufferType type;` at `:34` has no
default member initialiser, which is why the zero survives.

So `ptr_to_buffers_.count(k) == 1` proves nothing was written, and every prior
argument that tested for an entry tested nothing. Both passes are right. Pass B
states the additional point that fabrication only ever produces `Root` and never
`ExtArr` or `Args`, so a fabricated entry is never misread as physical; that is
correct and it is why the defect does not break the rule in section 2.

**Both passes name the fabricated buffer wrongly.** Pass A says the SSBO would
be named `Root_-1`; pass B says `root_-1`. `buffer_instance_name` at `:43-67`
builds the `Root` case at `:46-48` from `kRootBufferName`, which is
`constexpr char kRootBufferName[] = "root_buffer";` at `:26`. The name is
**`root_buffer_-1`**. A checkable literal, and both got it wrong, differently.

### 4.2 Pass A's history claim — PARTLY FALSE

Pass A writes, in its list of three things in scope at `:2197`: "Upstream already
discriminated at this exact site on statement identity, with
`stmt->dest->is<MatrixPtrStmt>()`, until commit `715a04c98` replaced it in
August 2023."

I read the diff. `git show 715a04c98 -- taichi/codegen/spirv/spirv_codegen.cpp`,
commit dated Tue Aug 15 2023, "[vulkan] Fix shared memory atomic float
operations (#8315)". The removed hunk is:

```
-      if (stmt->dest->is<MatrixPtrStmt>()) {
-        // Shared arrays have already created an accesschain, use it directly.
-        addr_ptr = ir_->query_value(stmt->dest->raw_name());
-      } else {
-        addr_ptr = at_buffer(stmt->dest, dt);
-      }
+      addr_ptr = dest_is_ptr ? dest_val : at_buffer(stmt->dest, dt);
```

Two things are wrong with pass A's sentence.

1. **It is not "this exact site".** The removed code was in
   `visit(AtomicOpStmt)`, at what is now `:1633` — a *caller* of `at_buffer`,
   not the predicate inside it. The variable is `stmt->dest`, not `ptr`, which
   is itself the tell that it is a different function.
2. **It discriminated a different question.** It separated "already a SPIR-V
   access chain, from a shared array" from "an integer needing offset
   arithmetic". That is not physical-address versus buffer-relative-offset.

The commit, the date, the identifier and the replacement are all real, so the
citation verifies. The **claim attached to it does not**, which is exactly the
failure mode section 10 item 7 describes. Pass A's weaker and unstated point —
that statement identity is testable here and upstream was willing to test it —
survives and is fair.

**Pass B handles the same ground correctly** in its section 3.5, identifying
`dest_is_ptr` at `:1612` as a fourth discriminator already in the file and
saying plainly that it does not separate physical-u64 from relative-u64, because
both are `kPrimitive` integers with `dt == u64`. I verified `SType::flag` at
`taichi/codegen/spirv/spirv_ir_builder.h:59` and `get_pointer_type` at
`spirv_ir_builder.cpp:407-424`, which sets `t.flag = TypeKind::kPtr` at `:416`
and **never assigns `t.dt`**, so the pointer's `dt` stays default-constructed —
`DataType::DataType() : ptr_(PrimitiveType::unknown.ptr_)` at
`taichi/ir/type.cpp:20`. Pass B is right on all of it.

Pass A also writes that `git show 715a04c98` "confirms the three were left
unguarded deliberately". The diff shows only that they were **not changed**. It
does not show intent. That word is not supported.

### 4.3 Blast radius — my own, in section 7

### 4.4 Pass A's unsettled counterexample P4 — SETTLED, AGAINST PASS A

Pass A's section 5.2 exhibits a case it could not resolve: the capability test at
`:783` sits outside the `is_array` test at `:797`, so a non-array
`ExternalPtrStmt` on a physical-storage device gets a u64 device address at
`:792` and a `BufferType::Args` entry at `:800`. Pass B raises the same case as
its E3 and also declines to settle it. Both were right not to dismiss it.

**I can settle it. It is unreachable, and the reason is stronger than the
`indices.push_back(1)` evidence pass A offered.**

`:797` reads `ctx_attribs_->arg_at(arg_id).is_array`, which comes from
`aa.is_array = ka.is_array` at `taichi/codegen/spirv/kernel_utils.cpp:66`, which
comes from the `Callable::Parameter` flag. `taichi/program/callable.cpp` has
exactly six parameter-insertion routes plus argpack, and I read all of them:

| route | line | `is_array` |
|---|---|---|
| `insert_scalar_param` | `:11` | default, false |
| `insert_arr_param` | `:24` | **true**, hardcoded |
| `insert_ndarray_param` | `:34` | **true**, hardcoded |
| `insert_texture_param` | `:57` | **true**, hardcoded |
| `insert_pointer_param` | `:70` | **true**, hardcoded |
| `insert_rw_texture_param` | `:77` | **true**, hardcoded |

Six rows, from the six `Callable::insert` definitions in that file. **Every
route that produces a pointer-like argument hardcodes `is_array = true`**, two
of them with a `FIXME` in the source admitting the flag is being abused
(`:58-60` and `:83`). The only route leaving it false yields a scalar, which
cannot be an `ExternalPtrStmt` base. So on any kernel that reaches SPIR-V
codegen, `:797` is true and **`:800` is dead**.

The `else` at `:800` is therefore not a "non-array" branch at all. It is
unreachable because `is_array` has been overloaded to mean "is a pointer-like
argument". That is a more useful statement than either pass reached and it
disposes of both P4 and E3.

I closed the two loose ends each pass left dangling:

- **Pass A cites `cpp_examples/autograd.cpp:160-161` as a caller using plain
  `create_arg_load`.** True, and irrelevant: `cpp_examples/autograd.cpp:44` is
  `auto program = Program(host_arch());`, and `cpp_examples/run_snode.cpp:42` is
  the same. **Both run on the LLVM spine and never reach `spirv_codegen.cpp`.**
  Pass A's counterexample rests on a file that cannot exercise the code it is a
  counterexample to.
- **`IRBuilder::create_external_ptr` (`taichi/ir/ir_builder.cpp:446-451`) has no
  production caller.** Every caller is under `tests/cpp/` or `cpp_examples/`.
  The one test that does target Vulkan, `tests/cpp/ir/ir_builder_test.cpp`
  (arch chosen at `:131-134`), uses `create_ndarray_arg_load` at `:99` and
  `insert_ndarray_param` at `:114`, so `is_array` is true there too.

I also checked `arg_at` itself, since its tail at
`taichi/codegen/spirv/kernel_utils.h:254` reads
`return arg_attribs_vec_[0].second;` and looks like a silent wrong-argument
fallback. It is not: `TI_ERROR` at `:251-253` throws first, and the return is
unreachable. A non-finding, recorded so nobody re-raises it.

### 4.5 The two pre-existing defects — BOTH CONFIRMED, and one is worse than reported

**The missing default initialiser.** `BufferType type;` at
`kernel_utils.h:34` has no default member initialiser while `root_id` at `:35`
has one. Confirmed by reading, and its consequence confirmed by compilation in
section 4.1. Pass A cites this as "`kernel_utils.h:36-37`"; line 36 is blank and
line 37 is the defaulted constructor. **The defect is at `:34`.** Pass B cites
`:35` and `:37` correctly for what it says about each.

**The three unguarded calls.** Confirmed by reading `visit(AtomicOpStmt)` at
`:1608-1690`. Six `at_buffer` calls; `dest_is_ptr` is computed at `:1612`:

| line | context | guarded by `dest_is_ptr` |
|---|---|---|
| 1617 | f64, `spirv_has_atomic_float64_add`, `op == add` | no |
| 1621 | f64 fallback | yes |
| 1626 | f32, `spirv_has_atomic_float_add`, `op == add` | no |
| 1630 | f32 fallback | yes |
| 1633 | all other dtypes | yes |
| 1681 | integral `AtomicOpType::mul` | no |

Six rows read one at a time; three unguarded. Both passes' tables are correct.

The mul case is the sharpest and both passes are right about it. At `:1633` a
shared-array integral dest is correctly given `dest_val`, and then `:1681`
**overwrites `addr_ptr`** with `at_buffer(stmt->dest, ...)` unconditionally. In
`at_buffer` the value's `stype.dt` is `unknown`, so `:2197` is false, and
`is_integral` at `taichi/ir/type_utils.h:103-113` does not admit `unknown`, so
`TI_ERROR_IF` at `:2207-2210` fires. **No capability gates `:1681`, so an atomic
multiply on a shared array is a hard compile error on every device.** Confirmed.

**What neither pass did, and what changes the grade: I measured the capability.**
`spirv_has_atomic_float_add` is set from `shaderBufferFloat32AtomicAdd` at
`taichi/rhi/vulkan/vulkan_device_creator.cpp:741-743`, and
`spirv_has_atomic_float64_add` from `shaderBufferFloat64AtomicAdd` at `:744-746`.
On this machine:

| Device | `shaderBufferFloat32AtomicAdd` | `shaderBufferFloat64AtomicAdd` |
|---|---|---|
| GTX 1070 | true | true |
| GTX 750 Ti | true | false |
| llvmpipe | true | false |

Consequences, which both passes left as conditionals:

- The **f32** unguarded path at `:1626` is live on **all three devices**. A f32
  atomic add on a shared array is a hard compile error everywhere on this box,
  not merely on hypothetical hardware.
- The **f64** unguarded path at `:1617` is live on the **GTX 1070 only**. The
  same program compiles on the GTX 750 Ti, where the capability is absent and
  the guarded fallback at `:1621` is taken, and **fails on the GTX 1070**.

That second row is a measured instance of the section 2.2 test failing on the
hardware in section 3.1: the better card can do strictly less. Pass B's E2 makes
exactly this argument as a possibility; I am recording it as a measurement. Pass
A's E3 records the defect without the 2.2 framing. Both are pre-existing and
neither is a consequence of item 6.2. I have changed no source file.

Reachability of a shared-array atomic remains a front-end question that neither
pass settled and I did not either; pass B marks it as inference I3, correctly.

---

## 5. Where I diverge from pass A

1. **The history claim is not "this exact site" and not the same question.**
   Section 4.2. This is pass A's weakest link and it is load-bearing: it is
   offered as the first of three discriminators in scope at `:2197`.
2. **"Deliberately" is not supported by the diff.** Section 4.2.
3. **P4 is dead, and pass A's evidence for it being live is inapplicable.**
   Section 4.4. Pass A was right to refuse to dismiss it and right that it could
   not be settled by the reasoning it used. It is settled by a different route.
4. **Pass A's blast radius omits `:2227` and `:2249`.** Its own section 4
   identifies `load_buffer` and `store_buffer` as the two forwards into
   `at_buffer`, but its section 7 list names only `:2197` as a predicate. Both
   forwards carry the identical `ptr_val.stype.dt == PrimitiveType::u64` test at
   `:2227` and `:2249`, and each uses it to choose `ti_buffer_type`, suppressing
   the uint reinterpret-and-bitcast that the relative path needs. I read all
   three. Same question, same answer required, and pass A misses two of the
   three. Pass B finds all three in its section 3.6.
5. **Pass A's total contradicts its own list.** Its section 7 enumerates
   `:2197`, `:2212`, `:319`, `:326`, `:332`, `kernel_utils.h:36-37`, `:1617`,
   `:1626`, `:1681` and calls it "Eight sites". That is **nine** rows. Section
   10 item 9 exists for precisely this, and names it as a repeated failure in
   this project.
6. **`BufferInfo` is at `kernel_utils.h:33`, not `:35`**, and the missing
   initialiser is at `:34`, not `:36-37`.

**Where pass A is right and pass B is not:** pass A's section 3 identifies the
one coupling pass B misses entirely — that the `!is_integral` guard at `:2207`
sits **between** the predicate at `:2197` and the map read at `:2212`, so moving
a map consultation above that guard converts a loud error into a silent wrong
binding. Pass A calls this "the single most important line in this report" and I
agree it is the most important thing in either report about the *remedy*. Pass B
argues the fabrication is harmless because it only produces `Root`, which is
true for classification but does not address what happens when the fabricated
`{Root, {-1}}` is then consumed by `get_buffer_value`. It reaches the default
tail at `:2304-2314`, takes a fresh binding from `binding_head_`, and declares
an SSBO named `root_buffer_-1`. Pass B does state this consequence in its E1 but
does not connect it to the guard order, and so leaves it out of its blast
radius. Pass A connects it and includes it.

## 6. Where I diverge from pass B

1. **The stated limit is a conditional that does not fire on this hardware.**
   Section 3. Pass B is right that it cannot settle the design question and
   right to escalate it as E4. It is wrong only in leaving it open, and it was
   not equipped to close it: closing it took a device query, not a read.
2. **"`at_buffer` has seven callers", then eight are listed.** Section 3.2 of
   pass B enumerates `:1617`, `:1621`, `:1626`, `:1630`, `:1633`, `:1681`, plus
   the two forwards at `:2233` and `:2257`. Six plus two is eight. The same
   section 10 item 9 failure as pass A's, in the opposite direction. Pass A
   phrases the identical fact correctly, as "six call sites, all in
   `visit(AtomicOpStmt)` ... plus `load_buffer` ... and `store_buffer`", and
   never states a wrong total.
3. **"31 live `register_value` sites" is 32.** `grep -o "ir_->register_value("`
   returns 33 occurrences; one is commented out at `:600`. So 32 live. The
   conclusion drawn from the enumeration — that exactly one produces a physical
   address — is unaffected and I verified it independently by the u64 route in
   section 2.2.
4. **The blast radius omits the guard-order coupling.** Section 5 above.
5. **`root_-1` should be `root_buffer_-1`.** Section 4.1.

**Where pass B is right and pass A is not:** the biconditional proof in its
section 3.4, which is genuinely structural and which I re-derived and could not
break; the three predicate sites rather than one; `SType::flag` correctly
distinguished from `Value::flag`, so that the settled `ValueKind` finding in
section 8.2 is left undisturbed while a real precedent is still recorded; and
the observation that the map rule **survives** turning on `use_64bit_pointers`,
because root offsets carry `BufferType::Root` and stay correctly classified at
any width. That last point is the one that makes the rule useful rather than
merely correct, and pass A does not make it.

---

## 7. Blast radius, derived from my own enumeration

Per section 10 item 8, stated separately from the grade. Grade: **ARCHITECTURAL**
for the collision, with the **ENVIRONMENTAL** bound on its scope established in
section 3.3. For the discrimination question only, not for the whole of item 6.2.

**Tier 1 — must be edited.** Replacing width-based discrimination with the
map-plus-capability rule:

| # | site | why |
|---|---|---|
| 1 | `:2197` | the predicate in `at_buffer` |
| 2 | `:2227` | the same predicate in `load_buffer`, choosing `ti_buffer_type` |
| 3 | `:2249` | the same predicate in `store_buffer` |
| 4 | `:319` | fabricating write; must stop fabricating before a predicate may read the map |
| 5 | `:326` | same |
| 6 | `:332` | same |
| 7 | `kernel_utils.h:34` | `BufferType type;` has no default member initialiser |

Seven rows. Six in `spirv_codegen.cpp`, one in `kernel_utils.h`. Two files.

Note that `:2227` and `:2249` do not consult the map at all today. The rule
introduces a `ptr_to_buffers_` lookup at each, and `.at()` on an absent key
throws `std::out_of_range`, so those two are new failure surfaces, not edits to
existing ones. Neither pass says this.

**Tier 2 — behaviour changes without being edited.**

| # | site | why |
|---|---|---|
| 8 | `:2212` | the existing map read; unchanged in content, but the new predicate reads the map ABOVE the `:2207` guard |
| 9 | `:1617` | unguarded call; today a loud error, becomes a silent `root_buffer_-1` binding |
| 10 | `:1626` | same |
| 11 | `:1681` | same |

Four rows. All in `spirv_codegen.cpp`.

**Total: 11 sites in 2 files**, seven of them requiring an edit. Counted from the
two tables above, row by row.

Against the two reports: pass B's six is the correct count of *decision lines*
and it is internally consistent, but it excludes the fabrication root and the
three calls whose failure mode inverts. Pass A's list is closer in kind, since it
includes the guard-order consequence, but it misses two of the three predicates
and its stated total of eight does not match its own nine rows.

**All three figures are materially smaller than "a change to that backend's
value model", which is what section 8.2 of the plan currently records.** Both
passes say so and both are right. Section 8.2's "UNDER REVISION" paragraph
already instructs that no figure be acted on until this territory closes; my
figure is offered to the planner as this adversary's derivation, not as a
replacement sentence for the plan.

---

## 8. Errors found, consolidated

| # | report | error | severity |
|---|---|---|---|
| 1 | A | "at this exact site" — the 2023 code was in `visit(AtomicOpStmt)`, now `:1633`, not in `at_buffer` | material; it is one of three claimed discriminators |
| 2 | A | that code discriminated shared-accesschain vs integer, not physical vs relative | material |
| 3 | A | "left unguarded deliberately" — the diff shows unchanged, not intended | minor |
| 4 | A | blast radius omits `:2227` and `:2249` | material |
| 5 | A | "Eight sites" over a nine-row list | countable, section 10 item 9 |
| 6 | A | `BufferInfo` at `:35`; it is at `:33`, missing initialiser at `:34` | minor |
| 7 | A | fabricated SSBO named `Root_-1`; it is `root_buffer_-1` | minor |
| 8 | A | P4 left open on evidence that cannot bear on it (a `host_arch()` example) | resolved here |
| 9 | B | "seven callers" over an eight-row list | countable, section 10 item 9 |
| 10 | B | "31 live `register_value` sites"; 33 occurrences, 1 commented at `:600`, so 32 | countable, conclusion unaffected |
| 11 | B | fabricated SSBO named `root_-1`; it is `root_buffer_-1` | minor |
| 12 | B | blast radius omits the `:2207` guard-order coupling | material |
| 13 | B | the limit left as an open conditional | closed here, by measurement |

Thirteen rows, counted from the table.

Neither report contains an error that reverses its main conclusion. The
discriminator exists, the inherited sentence is false, and the map plus `caps_`
carries it.

---

## 9. Escalations

**E1. The conditional that decided this item's cost is now measured, and it
resolves in favour of the cheaper answer.** Section 3. The root buffer cannot be
made large enough on any device on this machine to require physical addressing,
because `maxStorageBufferRange` and `maxMemoryAllocationSize` both cap a single
bound storage buffer at or below what a 32-bit byte offset already reaches. Pass
B's limit therefore does not fire on the target set. This is the planner's to
record; I am not writing it into the plan.

**E2. The tree queries two device limits and ignores the rest.** Only
`maxComputeWorkGroupCount` (`taichi/rhi/vulkan/vulkan_device.cpp:1126-1128`) and
`timestampPeriod` (`:1945`) are read from `VkPhysicalDeviceLimits`. Since the
sizing rule in section 8.1 item 2 is "optimise for what is there", and the
install-time agent of section 5.1a is the thing that measures the environment,
the absence of a `maxStorageBufferRange` query is a gap between the sizing rule
and the code that would have to implement it. I am flagging it, not scoping it.

**E3. The f64 unguarded atomic add splits the two cards.** Section 4.5. Measured,
not inferred: the capability is present on the GTX 1070 and absent on the GTX
750 Ti, so a f64 atomic add on a shared array compiles on the older card and is a
hard compile error on the newer one. That is hardware changing what the system
can do, which section 2.2 forbids. Pre-existing, unrelated to item 6.2.

**E4. `:800` is dead code guarded by an overloaded flag.** Section 4.4. Five of
six parameter routes hardcode `is_array = true`, two with `FIXME`s saying the
flag is abused. A future reader should be told the `else` is unreachable rather
than left to rediscover it. I did not remove it and am not proposing removal;
section 10 item 3 forbids deciding it is unnecessary.

**E5. Scope I did not cross.** I designed no remedy and chose between no routes.
Whether the fabrication defect must be fixed before item 6.2, and in what order,
is the planner's call. I did not build the tree.

---

## 10. What my searches could and could not see

- **Mechanically complete:** every occurrence of `ptr_to_buffers_`, `at_buffer`,
  `load_buffer`, `store_buffer`, `use_64bit_pointers`, `u64`,
  `ir_->register_value(`, `create_external_ptr` and `buffer_instance_name` in
  the tree. Each identifier is unique and unabbreviated. The `u64` search is the
  one that closes the derived-pointer question, and it is a substring search, so
  it also catches `u64_type()` and `val_u64` — it over-collects rather than
  under-collects, which is the safe direction here.
- **Read, not searched:** every `Callable::insert*_param` in
  `taichi/program/callable.cpp`, the whole of `visit(AtomicOpStmt)`, and all
  three predicate sites. The `is_array` enumeration in section 4.4 is complete
  for that file; it would **not** see a `Parameter` constructed directly
  somewhere else with `is_array` passed positionally. I grepped for
  `add_parameter(` and every caller is one of the six routes plus argpack, so
  the enumeration is closed for this tree.
- **Measured, not read:** the three device limit tables, the atomic-float
  capability table, and the value-initialisation behaviour. Read-only queries
  and a standalone replica compiled in a scratch directory. No device was
  created and no file in the project was modified.
- **Could NOT see:** which paths a real kernel takes. I did not build Taichi and
  ran no kernel. Every reachability statement here is derived from source or
  from a device query, never from an execution of this codebase.
- **Could NOT see:** hardware not installed. Section 5.2's RTX 3060 upper tier
  is not in this machine and my measurement does not speak for it.
- **Could NOT see:** `adversary-07-1.md`, which did not exist when I finished.

I modified exactly one file, this one.
