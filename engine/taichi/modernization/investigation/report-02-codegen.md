# Report 02 — Code generation, both paths

Explore agent 02. Territory: `taichi/codegen/` in full.

Working notes: `modernization/investigation/notes-02-codegen.md`.

Everything below is marked **[V]** verified by reading the named code, or
**[I]** inferred. Every claim carries a file:line.

**Revised 2026-09-09, correction pass.** Picked up by a second agent after
adversarial review (`adversary-02-1.md`, `adversary-02-2.md`). Territory 02 was
the only pair not sent back after round one, so this is its first correction.
Working notes for the pass are `notes-02-codegen.md` §§23-35.

What changed:

- §3.3 rewritten. The capability bypass in `Translate2Spirv` covers **eight**
  accessors, not two, and the set of in-tree targets lacking Int64 is larger
  than round one reported. The claim that Metal sets Int64 unconditionally was
  **wrong** and is corrected.
- §2.1 and Escalation 1 extended. The SNode bound is violated by an
  out-of-bounds **store** over a contiguous range, and reachability is now
  established rather than inferred.
- §2.3, §3.1 L6/L9/L12/L14/L20, §3.2 S6/S14/S15/S16, §4.1.9, §4.2.10 and
  several escalations corrected on substance or on citation.
- New §3.4 records the citation audit. New §3.2 row S17 closes a coverage gap
  in this report's own sweep.

Where the two adversaries contradict each other, the source was opened and the
adjudication is shown in §3.4.3. Neither was adopted on confidence.

**Amended 2026-09-09, amendment pass.** Picked up by a third agent after round
two (`adversary2-02-1.md`, `adversary2-02-2.md`). Both round-two adversaries
judge this report correct on substance and not complete; the residue was
mechanical and is discharged here. Working notes for the pass are
`notes-02-codegen.md` §§36-45. Every item below was re-verified against source
before editing; where an adversary was wrong, this report says so and changed
nothing.

What changed in the amendment pass:

- **§3.3.4 attribution corrected.** `spirv_codegen.cpp:2464` and `:2511` were
  swapped. `:2464` is the **argpack** struct, `:2511` is the **return** struct.
  Round one, the correction pass, both reports and both round-one adversaries
  all carried the swap. The five `translate_ti_type` call sites are now
  partitioned across the three functions.
- **§3.2 S6 split into S6a and S6b, and §4.2.2 rewritten.**
  `bitmasked_activation` holds a **second** widening-invalidity site at
  `spirv_codegen.cpp:398-399`, breaking in the opposite direction from
  `:410-412`. The governing rule is read from the validator in this tree
  (`external/SPIRV-Tools/source/val/validate_bitwise.cpp:88-91`), not quoted
  from the specification.
- **§3.3.3 count re-derived by enumeration.** "Five distinct optional bit
  widths" was wrong. Eight accessors, eight optional types, five capability
  gates, **three** distinct bit widths. Adversary 02-1's supporting sentence is
  also a miscount and is corrected in place.
- **New §3.3.5a.** The severity of the id-0 module is traced end to end through
  the checked-out SPIRV-Tools. It is caught **in process**, downgraded to a
  warning, and shipped anyway. Adversary 02-2's chain is confirmed; adversary
  02-1's "deferred to the driver, no Taichi-level diagnostic" is **wrong on its
  second half** and was not adopted.
- **§4.2 point 5 withdrawn and replaced.** It contradicted §4.2 point 2. The
  older text was the wrong half.
- **§2.1 sparsity conclusion corrected.** "Both surviving routes involve sparse
  SNodes" contradicted this report's own gate 1 eight lines earlier.
- **All eight span faults §3.4.1 disclosed and declined to fix are now applied
  in the tables** (§3.0, L7, S2, S5, S7, S8, S10).
- **New §3.3.7a.** The capability landscape is enumerated for **all five**
  bypassed capabilities, not just Int64. Metal reports no Float64 on any device.
  OpenGL reports no Int8 on any profile.
- **§3.3.5 extended** with the `get_buffer_value` i32 placeholder, the middle
  link in this report's own escalation-5 closure, taken from the paired report.
- `VulkanRuntimeImported::Inner::Inner` corrected to
  `VulkanRuntimeImported::Workaround::Workaround`. There is no `Inner`; the
  correction pass introduced that name.
- The word **"silently"** removed from escalation 5. Escalations 13-16 added.

**Closed out 2026-09-09, close-out pass.** Picked up by a fourth agent after
round three (`adversary3-02-1.md`, `adversary3-02-2.md`). The two round-three
adversaries split on the verdict; both judge this report correct on substance,
and the residue put to this pass is four line-level defects and one addition.
Working notes are `notes-02-codegen.md` §§46-51. **Worked against
`PROJECT-PLAN.md` at its 2026-09-09 date stamp**, re-read in full at the start
of the pass, including the priority statement at the head of §10 and item 6 on
the plan changing under a dispatched agent. Every item below was opened at
source before editing; where an adversary is wrong or short, this report says so
with lines.

What changed in the close-out pass:

- **§6 sweep reconciliation corrected, and by more than round three found.**
  The sentence claiming the irrelevant list "plus every file cited in §3.1, §3.2
  and §3.3" accounted for all 45 files was wrong. **Both adversaries put the
  union at 44 and named one uncovered file; the union is 43 and there are two.**
  `llvm/llvm_codegen_utils.cpp` is the one they found; the top-level
  `taichi/codegen/codegen.cpp` is the one neither did, covered once in §2.3.
  Re-derived mechanically, with the check shown. §3.4.4's claim that the
  round-two adversaries "found it exact" is withdrawn in place: what they
  certified was the file arithmetic, not the sentence next to it.
- **§3.3.5 and escalation 5 `get_buffer_value` census corrected.** The ten-site
  list omitted `spirv_codegen.cpp:2024` and `:2039`, and its heading, "Every
  Args- and Rets-buffer call site", did not reconcile with a list that already
  carried Root, GlobalTmps, ListGen and ArgPack sites. Replaced with a full
  enumeration: **thirteen call sites, twelve hardcoded, one forwarding.** Both
  omitted sites pass a hardcoded i32, so the conclusion is strengthened, not
  damaged. Round three is right on both halves of this.
- **§3.2 S6 extended: S6c, S6d and S6e added, and §4.2.2 rewritten a second
  time. This is the addition, and it was in neither report and no adversary
  before round three.** `bitmasked_activation` holds **four**
  widening-invalidity sites on its `SNodeLookupStmt` path, not two:
  `spirv_codegen.cpp:392-394` and `:395-397` join `:398-399` and `:410-412`.
  They arise from a **second mechanism**: the caller at `:488-489` passes an
  uncast i32 index, while the other caller casts at `:443-444`, and the same
  function casts the same value eighteen lines below at `:504-505`.
  `OpBitwiseAnd` at `:395-397` is governed by a stricter validator arm than the
  shifts, `validate_bitwise.cpp:106-141`, which admits no unconstrained operand
  at all. Graded **architectural**; blast radius **five places in one file**.
  New escalation 17 records the decision this opens and does not take it.
- **Three open-ended spans closed.** S16's `spirv_types.cpp:484-...` stood in
  the very row labelled "Spans corrected" — round three is right. Checking for
  the same species elsewhere found two more, which round three did not name:
  `spirv_ir_builder.cpp:149-...` at §3.3.2 and `spirv_types.cpp:167-...` at
  §3.3.4. All three closed with the closing line printed: `:484-514`,
  `:149-225`, `:167-214`.
- **The citation figure. Nothing to correct here.** Round three reports that the
  paired report's figure is **278**, not 342, and that 342 reached it through a
  brief. Confirmed here: before this pass edited anything,
  `grep -n 342 report-02-codegen.md` returned two hits, both the tail of the
  line range `spirv_ir_builder.cpp:337-342` — in the S16 row of §3.2, and in
  §3.3.6 item 1. `grep -n 278` returned two, both `llvm/codegen_llvm.cpp:278`.
  **This report has never carried 342, or 278, as a citation total.** Both greps
  now also hit this paragraph.
  No edit was made, and none was warranted.

---

## 1. Summary of the answer

The two codegen paths are 32-bit for indices in the **same** places and for the
**same** reason, but they are 32-bit for *addresses* in completely different
ways, and the SPIR-V side is the harder half.

- **Index arithmetic** is i32 in both paths, and the two paths agree
  structurally. `LinearizeStmt` is the spine and it is i32 on both.
- **Address arithmetic** diverges. On LLVM an address is a real machine pointer;
  LLVM's `GEP` and `PtrToInt`/`IntToPtr` already work at 64 bits, and several
  sites already sign-extend into i64. On SPIR-V an address is a **`u32` byte
  offset into a storage buffer**, produced by a helper whose 64-bit branch is
  gated on a `const bool` hardcoded to `false`.
- **SPIR-V Int64** is a properly declared, properly gated capability on **one**
  of the two type-lowering paths. The other path, the tinyir translator that
  builds every kernel's argument, return and argpack structs, consults no
  capability at all — for Int64 and for four other optional capabilities,
  covering eight types across three distinct bit widths (§3.3.3). It is also
  **not** gated for the pointer/address representation, which never consults it.
- **The SNode bound at `struct_llvm.cpp:266` does not guard the write it appears
  to guard, and violating it is an out-of-bounds store, not a failed lookup.**
  Reachability is established in §2.1: SNode tree ids are recycled, SNode ids
  are not.
- `taichi_max_num_snodes` appears in codegen exactly **once**, at
  `taichi/codegen/llvm/struct_llvm.cpp:266`. The SPIR-V struct compiler does not
  reference it at all.

The three highest-leverage facts in this territory, after the correction pass:

> `taichi/codegen/spirv/spirv_codegen.cpp:82`
> `const bool use_64bit_pointers = false;`
>
> The single SPIR-V address-width decision. It does not consult `caps_`, and it
> is coupled to two capabilities rather than one (§4.2.2).

> `taichi/codegen/spirv/spirv_types.cpp:393-431`
>
> `Translate2Spirv::visit_int_type` and `visit_float_type` call **eight** bare
> type accessors that skip the capability guards entirely, on the path that
> builds every kernel's argument, return and argpack structs. Nothing upstream
> rejects the input and nothing in Taichi validates the output (§3.3). The
> invalid module is caught by the optimiser's own parser, downgraded to a
> warning by the message consumer's branch order, and shipped unchanged
> (§3.3.5a). On Metal an `f64` argument triggers it on **every** device
> (§3.3.7a).

> `taichi/runtime/llvm/runtime_module/runtime.cpp:1003-1007`
>
> The `element_lists` write is a contiguous range starting at a **global** SNode
> id, while the only assertion in the tree bounds a **per-tree** count. Violating
> it is an out-of-bounds store into the device runtime struct, and it is
> reachable without many concurrent trees (§2.1).

---

## 2. Section 6 work items, mapped to this territory

### 2.1 Item 6.1 — parameterise the SNode ceiling

**[V] The assertion named in the brief is the only reference to
`taichi_max_num_snodes` anywhere under `taichi/codegen/`.**

| Site | Content |
|---|---|
| `taichi/codegen/llvm/struct_llvm.cpp:266` | `TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);` |

Nothing in `taichi/codegen/` is *sized* by the constant. It is a pure bounds
check here. The footprint the brief refers to is in the runtime
(`taichi/runtime/llvm/runtime_module/runtime.cpp:567-569`), agent 03's ground.

**[V] The SPIR-V path has no SNode ceiling at all.** Its per-SNode data lives in
`SNodeDescriptorsMap = std::unordered_map<int, SNodeDescriptor>`
(`taichi/codegen/spirv/snode_struct_compiler.h:38`), populated at
`snode_struct_compiler.cpp:137`. No fixed array, no assertion, no constant.
Item 6.1 has **zero SPIR-V codegen footprint**.

**[V] The assertion does not bound the quantity the runtime arrays are indexed
by.** Chain, each link read:

1. `snodes` is `StructCompiler::snodes`, a `std::vector<SNode *>`
   (`taichi/struct/struct.h:11`), filled by `StructCompiler::collect_snodes`
   (`taichi/struct/struct.cpp:7-13`), which recurses from **one** root.
2. `StructCompilerLLVM` is constructed **per SNode tree** — note the
   `snode_tree_id` constructor parameter at `struct_llvm.cpp:12-34`, and
   `run(SNode &root)` at `:247`.
3. The runtime arrays are indexed by `snode->id`. Corrected and completed
   subscript list, by grep over
   `taichi/runtime/llvm/runtime_module/runtime.cpp`:
   `element_lists` declared `:567`, **written `:1005` and `:1016`**, read
   `:1271`, `:1287`, `:1288`, `:1334`, `:1336`, `:1429`;
   `node_allocators` declared `:568`, written `:1029`, read `:1692`, `:1723`,
   `:1740`, `:1784`; `ambient_elements` declared `:569`, written `:1038`.
   Round one omitted `:1005` and gave `:1334-1336` as a span, but `:1335` is
   `int num_parent_elements = parent_list->size();` and not a subscript.
4. `snode->id` comes from a **global** counter:
   `std::atomic<int> SNode::counter{0}` (`taichi/ir/snode.cpp:12`),
   `id = counter++` (`taichi/ir/snode.cpp:220`), reset only per `Program`
   (`taichi/program/program.cpp:144`) or via `SNode::reset_counter()`
   (`taichi/ir/snode.h:348-350`).
5. Codegen is what carries that id into the runtime:
   `codegen_llvm.cpp:278` (`common.set("snode_id", tlctx->get_constant(snode->id))`)
   and `codegen_llvm.cpp:1151-1152` (`node_gc`).

**[V] Therefore a program can pass the per-tree assertion while the global id
exceeds the array bound.** Round one marked this **[I]** with no failing case
built. The correction pass established the mechanism; it is now **[V]** except
where noted. Escalated as a decision, §5.

**[V] The violation is an out-of-bounds STORE over a contiguous range, not a
failed lookup.** `runtime_initialize_snodes`
(`taichi/runtime/llvm/runtime_module/runtime.cpp:986-1017`) writes

```
1003:  for (int i = root_id; i < root_id + num_snodes; i++) {
1004:    // TODO: some SNodes do not actually need an element list.
1005:    runtime->element_lists[i] =
1006:        runtime->create<ListManager>(runtime, sizeof(Element), 1024 * 64);
1007:  }
```

`root_id` and `num_snodes` are parameters at `:988-989`. On the caller side,
`LlvmRuntimeExecutor::initialize_llvm_runtime_snodes`
(`taichi/runtime/llvm/llvm_runtime_executor.cpp:391-469`) reads `root_id` from
`field_cache_data.root_id` at `:400` and calls at `:442-444` passing
`(int)snode_metas.size()` as `num_snodes`. `root_id` is set from
`tree->root()->id` at
`taichi/runtime/program_impls/llvm/llvm_program.cpp:61` — a **global** SNode id.

So the write covers `element_lists[root_id .. root_id + num_snodes)`, a range
starting at a global id, while the only assertion in the tree bounds a per-tree
count. Once `root_id + num_snodes` exceeds 1024 the store runs off the end of
`element_lists` into `node_allocators`, then `ambient_elements`, then
`temporaries` (`runtime.cpp:570`) and `rand_states` (`:571`) — adjacent members
of the same `LLVMRuntime` struct. The other two arrays are written per SNode:
`runtime_NodeAllocator_initialize` (`runtime.cpp:1026-1031`, write at `:1029`)
and `runtime_allocate_ambient` (`:1033-1040`, write at `:1038`), both called
from `llvm_runtime_executor.cpp:460-462` and `:465-466` with
`snode_metas[i].id`.

**[V] Reachability: tree ids are recycled, SNode ids are not.**

- `Program::destroy_snode_tree` pushes the freed tree id onto a free stack:
  `free_snode_tree_ids_.push(snode_tree->id());`
  (`taichi/program/program.cpp:235`), the stack declared at
  `taichi/program/program.h:336`. `Program::allocate_snode_tree_id`
  (`program.cpp:559-567`) pops it at `:563-564` and only extends the range at
  `:561` when the stack is empty.
- `SNode::id` is `id = counter++` (`taichi/ir/snode.cpp:220`) from
  `std::atomic<int> SNode::counter{0}` (`snode.cpp:12`). Nothing decrements it
  and nothing frees an id on tree destruction.
- **The only reset is the `Program` constructor**, `SNode::counter = 0;` at
  `program.cpp:144`. `SNode::reset_counter()` (`taichi/ir/snode.h:348-350`) is
  **never called anywhere** — grep over `taichi/`, `c_api/` and `tests/`
  returns only its own definition. The one `reset_counter()` call in the tree
  is `Stmt::reset_counter()` at `program.cpp:347`, a different class. This
  corrects round one's escalation 6, which listed it as a live reset avenue.
- `TI_ASSERT_INFO(num_instances_ == 0, "Only one instance at a time")` at
  `program.cpp:141` means one `Program` at a time, so the id space grows
  monotonically for the life of that object.

Consequently reaching the overflow does **not** require many trees alive at
once, and `kMaxNumSnodeTreesLlvm = 512` (`taichi/inc/constants.h:13`) does not
bound it. A root + dense + place tree consumes three ids, so on the order of
340 create/destroy cycles carries the next tree's `root_id` past 1024 with
never more than one tree live. The per-tree assertion passes every time.

**[V] Two gates narrow it, and both point at this project's configuration.**

1. `runtime_initialize_snodes` returns early when `all_dense`
   (`runtime.cpp:1000-1002`), skipping the range write. `all_dense` is computed
   at `llvm_runtime_executor.cpp:402-410`: it is **initialised from
   `config_.demote_dense_struct_fors`** at `:402`, then cleared by any SNode
   that is not `dense`, `place` or `root` (`:404-409`).
   `demote_dense_struct_fors` defaults true
   (`taichi/program/compile_config.cpp:18`) and is a writable field
   (`taichi/python/export_lang.cpp:201-202`), so with it false the early return
   never fires **even for a fully dense tree**.
2. The `node_allocators` and `ambient_elements` writes are behind
   `is_gc_able(snode_metas[i].type)` (`llvm_runtime_executor.cpp:447`), which is
   `pointer || dynamic` (`taichi/ir/snode_types.cpp:21-23`).

**[V] CORRECTED in the amendment pass. This report previously concluded "both
surviving routes involve sparse SNodes". That conclusion contradicts gate 1
above, eight lines earlier, and gate 1 is the half that is right.** Adversary
02-2 found it; adversary 02-1 credited this report with getting it right,
having read the finding and not the conclusion. The source settles it:
`bool all_dense = config_.demote_dense_struct_fors;`
(`llvm_runtime_executor.cpp:402`) and the loop at `:403-410` can only ever
**clear** the flag, never set it. So with `demote_dense_struct_fors` false,
`all_dense` is false unconditionally.

Stated correctly, the two routes have different preconditions:

- The `node_allocators` / `ambient_elements` route **does** require sparsity:
  `is_gc_able` is `pointer || dynamic` (`taichi/ir/snode_types.cpp:21-23`).
- The `element_lists` range write at `runtime.cpp:1003-1007` requires **either**
  a non-`dense`/`place`/`root` SNode **or** `demote_dense_struct_fors == false`.
  The second disjunct needs **no sparsity at all**. It is a one-flag change on a
  public field (`taichi/program/compile_config.h:28`) that defaults true
  (`compile_config.cpp:18`) and is forced true only for SPIR-V archs
  (`compile_config.cpp:72-74`), which do not use this runtime.

Section 4.1 of the plan states sparsity is required, so the sparse route is on
the target configuration. But the stronger and simpler statement is that **one
of the two routes to the out-of-bounds store is reachable on an entirely dense
tree**, and this report previously overstated the precondition. Whether this
project will ever set `demote_dense_struct_fors` false is a configuration
decision nobody has made; it is escalated at §5.13.

**[V] The assertion is not debug-only.** `TI_ASSERT` expands to
`TI_ASSERT_INFO` (`taichi/common/logging.h:100`), which expands to a plain `if`
plus `TI_ERROR` (`:101-107`), with no `NDEBUG` guard. It fires in release
builds. It simply measures the wrong quantity.

**[I]** What remains inferred is only the arithmetic of the cycle count above.
No failing case was built.

**[V] Emitted code volume scales with SNode count.** `StructCompilerLLVM` emits
per SNode: a type stub plus a dummy function (`struct_llvm.cpp:133-143`), a
`refine_coordinates` function for each non-leaf (`:146-190`, invoked at `:200`),
and a `get_ch_..._to_...` accessor for each non-root (`:203-233`). The refine
body is a `taichi_max_num_indices`-iteration loop each emitting
`URem`/`UDiv`/`Mul`/`Add` plus two calls (`:171-188`). So struct-module IR is
roughly O(num_snodes x taichi_max_num_indices). Raising 1024 raises struct
module size and its compile time proportionally.

### 2.2 Item 6.2 — 64-bit addressing

Covered in full by §3 (inventory) and §4 (what would change). The single
codegen-side decision point on SPIR-V is `spirv_codegen.cpp:82`.

**[V] `taichi_max_num_indices` appears in codegen exactly once in live code**,
`struct_llvm.cpp:171`, plus a documentary comment at `codegen_llvm.cpp:2363`.
Confirmed by grep over `taichi/codegen/` in the correction pass: two hits, no
others. It controls the emitted instruction count of every refine function, and
it sizes `PhysicalCoordinates::val` in the runtime (`runtime.cpp:288-290`).
It also sizes the `total_shape` arrays that produce `LinearizeStmt::strides`
outside this territory (`taichi/transforms/scalar_pointer_lowerer.cpp:33`,
`taichi/transforms/demote_dense_struct_fors.cpp:19`), all of them `int`.

### 2.3 Item 6.3 — adaptive module loading

Two codegen-side touchpoints, noted for the seam with agent 04. I did not
investigate further; 6.3 is not my assignment.

- **[V]** `taichi/codegen/codegen.cpp:37-74` `KernelCodeGen::create` — backend
  selection is a compile-time `#if defined(TI_WITH_CUDA)` / `TI_WITH_DX12` /
  `TI_WITH_AMDGPU` chain around a runtime `arch` switch. A backend not compiled
  in reaches `TI_NOT_IMPLEMENTED`.
- **[V]** `taichi/codegen/spirv/kernel_compiler.cpp:25-46` threads a
  `const DeviceCapabilityConfig &device_caps` into
  `KernelCodegen::Params::caps` (assignment at `:37`, field at
  `spirv_codegen.h:25`), which reaches `TaskCodegen::caps_` and
  `IRBuilder::caps_` (`spirv_ir_builder.h:598`, constructor at `:224-226`).
  **A per-device capability set already flows through the whole SPIR-V
  codegen.**
- **[V] CORRECTED.** Round one said "the LLVM path has no analogue". That
  understates it. `taichi/codegen/llvm/kernel_compiler.cpp:30-34` declares
  `const DeviceCapabilityConfig &device_caps` as its second parameter, at
  `:32` — the same position in the same `KernelCompiler::compile` interface as
  the SPIR-V twin. The body `:35-47` never references it, and
  `KernelCodeGen::create` is called at `:36-37` with `compile_config`,
  `&kernel_def`, `&chi_ir` and `*config_.tlctx` only. **The plumbing point
  exists and is discarded.** That is a materially different starting position
  for 6.3 than an absent interface.

---

## 3. Inventory of 32-bit width assumptions in emitted code

All **[V]** unless marked.

### 3.0 The mechanism that makes it invisible — LLVM path

`TaichiLLVMContext::get_constant(T)`
(`taichi/runtime/llvm/llvm_context.cpp:727-749`) dispatches on the **C++ type**
of its argument: `int32`/`uint32` -> `APInt(32,...)`; `int64`/`uint64`/`size_t`
-> `APInt(64,...)`. Explicit instantiations at `:904-916`.

Every `tlctx->get_constant(x)` where `x` is a C++ `int` therefore emits an
**i32**, with nothing at the call site saying so. This is the single mechanism
by which the LLVM path's 32-bit-ness propagates. The width-explicit variant is
`get_constant(DataType dt, T t)` (`:695-718`), which uses `data_type_bits(dt)`.

### 3.1 LLVM path

| # | Site | What is 32-bit |
|---|---|---|
| L1 | `llvm/struct_llvm.cpp:153` | Refine-coordinates function signature: third parameter `l` is `llvm::Type::getInt32Ty`. The within-cell linear index. |
| L2 | `llvm/struct_llvm.cpp:171-188` | Refine body: `get_constant(0)`, `get_constant(acc_shape * shape)`, `get_constant(acc_shape)`, `get_constant(i)`, `get_constant(shape)` — all i32. `AxisExtractor::{num_elements_from_root,shape,acc_shape}` are plain `int` (`taichi/ir/snode.h:41`, `:45`, `:49` — **corrected** from `:40,44,49`), so `acc_shape * shape` at `:174-175` is an **`int` multiply that can overflow in C++ before LLVM sees it**, and its product is the divisor of the `CreateURem` at `:179`. See §4.1.2. |
| L3 | `llvm/struct_llvm.cpp:111-113` | `dynamic` SNode aux type is `{i32, i32}` — mutex and `n`, the element count. |
| L4 | `llvm/struct_llvm.cpp:74-75` | `bitmasked` aux array is `i32[(max_num_elements()+31)/32]`. The 32 is a word width, but the count divides an `int64` element count. |
| L5 | `llvm/codegen_llvm.cpp:1736-1743` | **`visit(LinearizeStmt*)`.** `val = get_constant(0)` -> i32; `Add(Mul(val, get_constant(strides[i])), inputs[i])`, all i32. LLVM requires matched operand widths, so `inputs[i]` is i32 too. **The 32-bit spine of SNode indexing.** |
| L6 | `llvm/codegen_llvm.cpp:1755` | `create_bit_ptr`: `TI_ASSERT(bit_offset->getType()->isIntegerTy(32));` Hard 32-bit assertion. Struct documented in the comment at `:1750-1754` as `{iX* byte_ptr; i32 bit_offset;}`. |
| L7 | `llvm/codegen_llvm.cpp:1792-1829` | `visit(SNodeLookupStmt*)`. Root: `CreateGEP(parent_ty, parent, llvm_val[input_index])` with the i32 index. dense/pointer/dynamic/bitmasked: index passed to runtime `activate` / `lookup_element`. quant_array: `get_constant(element_num_bits)` (i32) times the index. |
| L8 | `llvm/codegen_llvm.cpp:1927` | `ExternalPtrStmt`: ndarray shapes loaded as `PrimitiveType::i32`. |
| L9 | `llvm/codegen_llvm.cpp:1931-1946` | `ExternalPtrStmt`: `linear_index = get_constant(0)` (i32) at `:1931`, and the whole chain is i32 — `CreateMul` at `:1939` and `:1942`, `CreateAdd` at `:1944`. **Line numbers corrected**; round one gave `:1938`, `:1941`, `:1943`, which are a continuation line, a comment and a closing brace. |
| L10 | `llvm/codegen_llvm.cpp:1988` | `ExternalPtrStmt` non-TensorType branch: the raw **i32** `linear_index` goes straight into `CreateGEP`. LLVM sign-extends to pointer width, but the i32 arithmetic has already wrapped. |
| L11 | `llvm/codegen_llvm.cpp:1857-1861` | `MatrixPtrStmt` index branch: `CreateGEP(type, casted_ptr, {get_constant(0), llvm_val[offset]})`. |
| L12 | `llvm/codegen_llvm.cpp:2157-2158` | Struct-for per-block loop index: `loop_index_ty = getInt32Ty` at `:2157`, alloca at `:2158`. **Line numbers corrected.** Stored at `:2190-2191`; loaded i32 at `:2216`, `:2228`, `:2253`; incremented at `:2273`; compared `ICMP_SLT` at `:2214-2216`. Round one gave `:2213-2214`, `:2222`, `:2226`, `:2251`, which are two `SetInsertPoint` calls, a blank line and a comment. |
| L13 | `llvm/codegen_llvm.cpp:2113-2119` | Struct-for body function signature: `lower_bound` and `upper_bound` are both `tlctx->get_data_type<int>()` -> i32, at `:2117-2118`. Read back as `get_arg(3)`/`get_arg(4)` at `:2163-2164`. Shared by all four LLVM backends. |
| L14 | `llvm/codegen_llvm.cpp:2290-2291` | `int list_element_size = std::min(leaf_block->max_num_elements(), (int64)taichi_listgen_max_element_size);` — an `int64` min result **narrowed to `int`** on assignment, then emitted as i32 at `:2309`. **Span corrected**; `:2292-2293` is the unrelated `num_splits`. Bounded today by `taichi_listgen_max_element_size = 1024` (`taichi/inc/constants.h:28`), so it is not a live truncation. |
| L15 | `llvm/codegen_llvm.cpp:2336`, `:2339` | `visit(LoopIndexStmt*)` loads loop indices as `getInt32Ty` in **both** branches. Loop indices are i32 by construction, independent of IR type. |
| L16 | `llvm/codegen_llvm.cpp:2362-2364` | `visit(BlockCornerIndexStmt*)` comment states the contract: `struct PhysicalCoordinates { i32 val[taichi_max_num_indices]; }`. Confirmed at `runtime.cpp:288-290`. |
| L17 | `llvm/codegen_llvm.cpp:1169-1171` | `create_naive_range_for`: loop var is `PrimitiveType::i32`, hardcoded. |
| L18 | `llvm/codegen_llvm.cpp:2065-2087` | `get_range_for_bounds`: both bounds i32. The const path reads `stmt->begin_value`/`end_value`, which are `int32` in the IR (`taichi/ir/statements.h:1415-1416`); the non-const path builds `GlobalTemporaryStmt(offset, PrimitiveType::i32)` and loads i32. |
| L19 | `llvm/codegen_llvm.cpp:278` | `common.set("snode_id", get_constant(snode->id))` — i32, matching `StructMeta::snode_id` (`runtime.cpp:308`). |
| L20 | `llvm/codegen_llvm.cpp:284-292` | The commented ABI block declares the SNode accessor contract as `lookup_element(uint8*, int i)` (`:285`) / `is_active(uint8*, int i)` (`:287`) / `int get_num_elements(uint8*)` (`:288`). **Span corrected**; `:294` is the `functions` vector. Confirmed live at `runtime.cpp:312-322` and `runtime_module/node_dense.h:10,18,22`, and again in `node_pointer.h:10,84,90`. **`int` is the declared index type of the SNode accessor ABI.** |
| L21 | `cpu/codegen_cpu.cpp:57`, `:59`, `:103`, `:110`, `:126`, `:128`, `:216-217` | CPU offload body arg `get_data_type<int>()`; range-for and mesh-for loop vars i32; `get_spmd_info` returns i32 constants 0 and 1. |
| L22 | `cuda/codegen_cuda.cpp:480`, `:482`, `:522-523`, `:535`, `:549`, `:769-775` | CUDA offload body arg i32; loop vars i32; `get_spmd_info` uses `nvvm_read_ptx_sreg_tid_x` / `ntid_x`, which are **i32 by the intrinsic's own signature**. |
| L23 | `amdgpu/codegen_amdgpu.cpp:256`, `:258`, `:459-467` | Same shape as CUDA; `get_spmd_info` builds i32. |
| L24 | `dx12/codegen_dx12.cpp:40`, `:42`, `:82`, `:200-202` | Same shape; `get_spmd_info` returns i32 constants. `dx12/dx12_lower_intrinsic.cpp:54`, `:83-95` patch `thread_idx`/`block_idx`/`block_dim`/`grid_dim` to i32 DXIL intrinsics, with `unsigned group_size = 64` at `:79`. |

**Already 64-bit on the LLVM path** (so the migration does not have to touch
them):

- `codegen_llvm.cpp:281` — `StructMeta::max_num_elements` is set from
  `get_constant(snode->max_num_elements())`, and `SNode::max_num_elements()`
  returns `int64` (`taichi/ir/snode.h:306-308`, backed by
  `int64 num_cells_per_container` at `:97`) -> **i64**.
- `codegen_llvm.cpp:279` — `element_size` <- `get_constant((uint64)...)` -> i64.
- `codegen_llvm.cpp:2386` — `GlobalTemporaryStmt` offset,
  `get_constant((int64)stmt->offset)` -> **i64**.
- `codegen_llvm.cpp:2396` — `ThreadLocalPtrStmt` offset; the field is
  `std::size_t` (`taichi/ir/statements.h:1618`) -> **i64**.
- `codegen_llvm.cpp:1864-1870` — `MatrixPtrStmt` byte-offset branch:
  `PtrToInt`->i64, `SExt` offset->i64, `Add`, `IntToPtr`. Genuinely 64-bit
  address space, fed by a 32-bit offset.
- `codegen_llvm.cpp:1960-1961`, `:1969-1971` — `ExternalPtrStmt` TensorType
  branch `SExt`s to i64 and multiplies by an i64 constant. But the extension
  happens **after** the i32 chain in L9, so overflow is already baked in.

**Not width assumptions, listed so they are not double-counted:**

- Constant GEP indices into fixed-layout structs: `codegen_llvm.cpp:1355`,
  `:1361`, `:1761`, `:1767`, `:1782`, `:1786`, `:1893`, `:1921`, `:2330`,
  `:2375`, `:2834`, `:2848`, `:2853`, `:2920`, `:2940`, `:2954`, `:2972`;
  `codegen_cuda.cpp:69`, `:446`, `:449`. **[V]** LLVM requires struct GEP
  indices to be i32 constants.
- All i32 constants in `llvm/codegen_llvm_quant.cpp` (`:26`, `:40`, `:110`,
  `:198`, `:203`, `:205`, `:208`, `:213`, `:359`, `:361`, `:391-392`, `:399`,
  `:401`, `:423`, `:427`, `:440`, `:443`, `:456`, `:458`, `:460-461`) are **bit
  offsets and masks inside a physical type**, consistent with L6. Physical types
  are at most 64 bits, so these are not a scaling limit.

### 3.2 SPIR-V path

| # | Site | What is 32-bit |
|---|---|---|
| S1 | `spirv/spirv_codegen.cpp:82` | `const bool use_64bit_pointers = false;` A `const bool` member of `TaskCodegen`. Not a capability query, not a config field, not a constructor parameter. |
| S2 | `spirv/spirv_codegen.cpp:2317-2324` | `make_pointer(size_t offset)`: the live branch is `uint_immediate_number(ir_->u32_type(), uint32_t(offset))` — an **explicit narrowing cast**. The dead branch carries upstream's own comment, "This is hacky, should check out how to encode uint64 values in spirv". |
| S3 | `spirv/spirv_codegen.cpp:355` | `visit(GetRootStmt*)`: root base address is `make_pointer(0)` -> u32. |
| S4 | `spirv/spirv_codegen.cpp:371-372` | `visit(GetChStmt*)`: `add(input_ptr, make_pointer(desc.mem_offset_in_parent_cell))` -> u32. |
| S5 | `spirv/spirv_codegen.cpp:504-508` | `visit(SNodeLookupStmt*)`: the i32 linear index is `cast` to `parent_val.stype` (u32) at `:504-505`, multiplied by `make_pointer(desc.cell_stride)` (u32) at `:506-507` and added to the u32 parent pointer at `:508`. **All SNode address arithmetic is u32 modular.** **Span corrected in the amendment pass**; `:503` is blank and the `add` at `:508` was outside the old span. |
| S6 | `spirv/spirv_codegen.cpp:383-435` | `bitmasked_activation`. **Corrected and expanded twice, then expanded again in the close-out pass.** The early shifts and masks take `ptr_dt` as their **Result Type** (`:392-399`, `:403-404`), and `make_pointer(desc.cell_stride * desc.snode->num_cells_per_container)` at `:406-408` computes `size_t * int64` in C++ and truncates through `make_pointer` — but the function also **hardcodes** `ir_->u32_type()` at `:405`, `:411`, `:412` and `:414`, independently of `make_pointer`. **Taking `ptr_dt` as Result Type is not the same as being width-clean.** Three of these instructions carry an **operand** that does not follow `ptr_dt`, and the phrase "do use `ptr_dt`" concealed that for three rounds. **This function holds four widening-invalidity sites on its `SNodeLookupStmt` path and two on its `SNodeOpStmt` path, and they break in both directions.** See S6a-S6e below. Round one described only the `ptr_dt` half; the amendment pass found the second site; the close-out pass found the remaining two and the caller-side mechanism behind them. |
| S6a | `spirv/spirv_codegen.cpp:410-412` | `bitmasked_activation`, **Result Type hardcoded narrow.** An `OpShiftRightLogical` whose Result Type is hardcoded `ir_->u32_type()` at `:411` while its Base, `bitmask_word_ptr` from `:409` (`ir_->add(parent_ptr, ...)`), carries the pointer width. A widened pointer gives a 64-bit Base under a 32-bit Result Type. |
| S6b | `spirv/spirv_codegen.cpp:398-399` | **New in the amendment pass. `bitmasked_activation`, Result Type follows the pointer while the Base is pinned at i32.** `ir_->make_value(spv::OpShiftLeftLogical, ptr_dt, ir_->const_i32_one_, bitmask_bit_index)`. `make_value(op, out_type, args...)` (`spirv_ir_builder.h:290-298`) takes its second argument as Result Type and its third as the instruction's first operand, so Result Type is `ptr_dt` and Base is `const_i32_one_` — `int_immediate_number(t_int32_, 1)` at `spirv_ir_builder.cpp:224`, permanently 32-bit. A widened pointer gives a 32-bit Base under a 64-bit Result Type: **invalid by the same rule as S6a and in the same function, in the opposite direction.** |
| S6c | `spirv/spirv_codegen.cpp:392-394` | **New in the close-out pass. `bitmasked_activation`, Result Type follows the pointer while the Base is the function's own uncast parameter.** `ir_->make_value(spv::OpShiftRightLogical, ptr_dt, input_index, ir_->uint_immediate_number(ptr_dt, 5))`. Result Type is `ptr_dt` (`:393`); Base is `input_index`, the parameter declared at `:387`. Same governing rule as S6a and S6b — `external/SPIRV-Tools/source/val/validate_bitwise.cpp:88-91`, `_.GetBitWidth(base_type) != _.GetBitWidth(result_type)`. The Shift operand at `:394` does follow `ptr_dt` and is unconstrained in width in any case (`:93-101`). **Whether this instruction is valid under a widened pointer depends entirely on which caller reached it: see S6e.** |
| S6d | `spirv/spirv_codegen.cpp:395-397` | **New in the close-out pass. `bitmasked_activation`, same shape as S6c under a stricter rule.** `ir_->make_value(spv::OpBitwiseAnd, ptr_dt, input_index, ir_->uint_immediate_number(ptr_dt, 31))`. Result Type `ptr_dt` at `:396`, operands `input_index` at `:396` and a `ptr_dt` immediate at `:397`. `OpBitwiseAnd`'s validation arm is `validate_bitwise.cpp:106-141`, not the shift arm: it loops over **every** operand from index 2 (`:118-119`) and requires each to match Result Type's bit width (`:134-138`, "Expected operands to have the same bit width as Result Type"). There is no unconstrained-shift escape here, so this is the least forgiving of the four. Same caller dependence as S6c. |
| S6e | `spirv/spirv_codegen.cpp:488-491`, read against `:443-444` and `:504-505` | **The second mechanism, new in the close-out pass, and it does not live in `bitmasked_activation` at all.** The function has two callers and they disagree about casting. `visit(SNodeOpStmt*)` (`:437-466`) casts first — `spirv::Value input_index_val = ir_->cast(parent_val.stype, ir_->query_value(stmt->val->raw_name()));` at `:443-444` — then calls at `:448-449`, `:455-456` and `:458-459`, so on that path `input_index` follows the pointer and S6c/S6d stay valid. `visit(SNodeLookupStmt*)` (`:468-511`) does not: `:488-489` is a bare `ir_->query_value(stmt->input_index->raw_name())`, handed straight to `bitmasked_activation` at `:490-491`. That value is the i32 built by `visit(LinearizeStmt*)` (S7, `:532-541`, seeded from `const_i32_zero_` with i32 strides). **The same function casts the same value eighteen lines below**, at `:504-505`, for the dense-offset branch — which this report already records as S5. Both halves of the mechanism sat in this table from round one and were never read against each other. |
| S7 | `spirv/spirv_codegen.cpp:532-541` | `visit(LinearizeStmt*)`: `val = ir_->const_i32_zero_`, strides as `int_immediate_number(ir_->i32_type(), ...)`. **i32.** Structurally identical to L5. |
| S8 | `spirv/spirv_codegen.cpp:2214-2218` | `at_buffer`: `idx_val = OpShiftRightLogical(ptr_val, log2(width))` inherits `ptr_val.stype` (u32) and is used as the SSBO runtime-array index via `struct_array_access`. **The storage-buffer index is the pointer width.** |
| S9 | `spirv/spirv_codegen.cpp:707-711` | `visit(GlobalTemporaryStmt*)`: `int_immediate_number(ir_->i32_type(), stmt->offset, false)` — **i32**. |
| S10 | `spirv/spirv_codegen.cpp:737`, `:753-754`, `:773-777` | `visit(ExternalPtrStmt*)`: `linear_offset` starts i32, shapes loaded i32, the whole mul/add chain i32, then **left-shifted by `log2(element size)` in i32** to make a byte offset. |
| S11 | `spirv/spirv_codegen.cpp:321-325` | `visit(MatrixPtrStmt*)` GlobalTemporary branch: `dt_bytes = int_immediate_number(ir_->i32_type(), get_primitive_type_size(dt))`, multiplied by the offset. |
| S12 | `spirv/spirv_codegen.cpp:2000-2002`, `:2010`, `:2018-2044`, `:2055-2058`, `:2066-2072` | Range-for kernel: `begin_expr_value`, `total_elems`, `begin_`, `end_`, `total_invocs` and the loop phi `"ii"` are all `i32_type()`. `get_global_invocation_id(0)` (u32) is `cast` to i32 at `:2055`. Global-tmp-backed bounds are read by arithmetic-shifting a byte offset right by 2 (`:2018-2020`, `:2033-2035`) — a 32-bit word index. |
| S13 | `spirv/spirv_codegen.cpp:2138-2179` | Struct-for kernel: driven off a **ListGen buffer of `u32`** — count at word 0 (`:2139-2141`), indices at words 1..n (`:2164-2168`). `loop_index_var` is `alloca_variable(ir_->u32_type())` (`:2149`), compared `OpULessThan`, and the loaded `listgen_index` (u32) is registered as `"ii"`. |
| S14 | `spirv/spirv_ir_builder.cpp:565-608` | `get_array_type`: `num_elems` parameter is `uint32_t` (`:565`), length emitted as `uint_immediate_number(t_uint32_, num_elems)` at `:576`; `nbytes` is `uint32_t` (`:586`) assigned from `snode_desc.container_stride`, a `size_t`, at `:591` — **implicit narrowing** — and emitted as the `ArrayStride` decoration at `:605`. Only a `nbytes == 0` warning exists (`:596-602`); a stride that overflows to a nonzero value is not caught. **Line numbers corrected**; round one gave `:575`, `:590`, `:592-599`. |
| S15 | `spirv/spirv_ir_builder.cpp:164-165`, `:223-224` | `t_int32_`/`t_uint32_` are declared unconditionally at `:164-165`, and `const_i32_zero_`/`const_i32_one_` are pre-built i32 constants at `:223-224`, used pervasively (scope and semantics operands, comparisons, selects). **Corrected**; round one cited `:283-284`, which is inside `get_null_type` (`:282-286`) and has nothing to do with these constants. |
| S16 | `spirv/spirv_ir_builder.cpp:337-342`, `spirv/spirv_types.cpp:490-498` | A Taichi `PointerType` translates to `t_uint64_` if `has_buffer_ptr` else **`t_uint32_`** — the 32-bit case being the only reachable one today (see §3.3). Two independent implementations: `IRBuilder::from_taichi_type` (function `:334-353`, pointer branch `:337-342`) and `translate_ti_type` (function `spirv_types.cpp:484-514`, pointer branch `:490-498`, 64-bit at `:492-493`, 32-bit at `:495-496`). **Spans corrected. Closing the function span in this row was itself missed by the amendment pass: the closed form `:484-514` was written at `:595` and `:946` and the ellipsis was left standing in the row that announces the correction. Closed in the close-out pass; `:514` printed.** |
| S17 | `spirv/kernel_utils.h:99-100`, `:110-111`; `spirv/spirv_codegen.cpp:1999`, `:2004` | **New.** `RangeForAttributes::begin` and `::end` are `size_t` (`kernel_utils.h:110-111`) but their difference is taken into an `int` at `spirv_codegen.cpp:1999` (`const int num_elems = range_for_attribs.end - range_for_attribs.begin;`) and stored into `advisory_total_num_threads`, itself `int` (`kernel_utils.h:99`), at `:2004`. A host-side narrowing feeding the emitted i32 constants in S12. `kernel_utils.h` appeared in neither round one's findings nor its sweep list; see §3.4.4. |

**Host-side sizes are already 64-bit; the truncation is at emission.** **[V]**
`SNodeDescriptor` fields are all `size_t` —
`cell_stride` (`snode_struct_compiler.h:16`), `container_stride` (`:19`),
`total_num_cells_from_root` (`:28`), `mem_offset_in_parent_cell` (`:31`) — and
`CompiledSNodeStructs::root_size` is `size_t` (`:42`).
`compute_snode_size` (`snode_struct_compiler.cpp:65-139`) computes them all in
`size_t`. So the SPIR-V struct layout is a 64-bit computation truncated at one
place, `make_pointer` (S2), with no assertion that the value fits.

### 3.3 SPIR-V optional-width capabilities — declared, checked on one path, bypassed on the other, and absent on more targets than round one reported

The brief asks specifically about Int64. **Rewritten in the correction pass.**
Round one framed this as an Int64 problem with one unguarded live site and one
in-tree target lacking the capability. Both halves were understated, and one
statement in it was factually wrong. All **[V]**.

#### 3.3.1 Declared

`spirv_ir_builder.cpp:64-66` emits `OpCapability Int64` only when
`caps_->get(cap::spirv_has_int64)`. The neighbouring optional capabilities are
emitted the same way: Int8 at `:58-60`, Int16 at `:61-63`, Float16 at `:67-69`,
Float64 at `:70-72`, PhysicalStorageBufferAddresses at `:73-77`.

#### 3.3.2 Checked, on the `get_primitive_type` path

`IRBuilder::init_pre_defs` (`spirv_ir_builder.cpp:149-225`) declares the
mandatory types unconditionally — `t_bool_` at `:155`, `t_int32_`/`t_uint32_` at
`:164-165`, `t_fp32_` at `:170` — and every optional type only under its
capability: `t_int8_`/`t_uint8_` at `:156-159`, `t_int16_`/`t_uint16_` at
`:160-163`, `t_int64_`/`t_uint64_` at `:166-169`, `t_fp16_` at `:171-173`,
`t_fp64_` at `:174-176`. Absent the capability the member stays
default-constructed.

`IRBuilder::get_primitive_type` (`spirv_ir_builder.cpp:288-332`) guards every
one of them with `TI_ERROR("Type {} not supported.", ...)`:

| type | guard | return |
|---|---|---|
| f16 | `:292-293` | `:294` |
| f64 | `:298-299` | `:300` |
| i8 | `:302-303` | `:304` |
| i16 | `:306-307` | `:308` |
| i64 | `:312-313` | `:314` |
| u8 | `:316-317` | `:318` |
| u16 | `:320-321` | `:322` |
| u64 | `:326-327` | `:328` |

Round one cited the i64 error as `:310-313` and the u64 error as `:324-327`.
Both are off by one at the start: `:310` is `return t_int32_;` and `:324` is
`return t_uint32_;`. Corrected above.

#### 3.3.3 Bypassed, on the tinyir type-translation path — EIGHT accessors, not two

`Translate2Spirv` (`spirv_types.cpp`) does not call `get_primitive_type` at all.
`visit_int_type` (`:393-419`) and `visit_float_type` (`:421-431`) call the bare
accessors directly:

| call site | accessor | member returned | accessor decl | guarded equivalent |
|---|---|---|---|---|
| `spirv_types.cpp:397` | `i8_type()` | `t_int8_` | `spirv_ir_builder.h:559-561` | `spirv_ir_builder.cpp:302-304` |
| `:399` | `i16_type()` | `t_int16_` | `:549-551` | `:306-308` |
| `:403` | `i64_type()` | `t_int64_` | `:529-531` | `:312-314` |
| `:409` | `u8_type()` | `t_uint8_` | `:562-564` | `:316-318` |
| `:411` | `u16_type()` | `t_uint16_` | `:552-554` | `:320-322` |
| `:415` | `u64_type()` | `t_uint64_` | `:532-534` | `:326-328` |
| `:424` | `f16_type()` | `t_fp16_` | `:555-557` | `:292-294` |
| `:428` | `f64_type()` | `t_fp64_` | `:535-537` | `:298-300` |

Every accessor in `spirv_ir_builder.h:529-564` is a one-line `return t_...;`
with no `caps_` consultation of any kind. The mandatory widths reached by the
same visitor — `i32_type()` at `:401`, `u32_type()` at `:413`, `bool_type()` at
`:407`, `f32_type()` at `:426` — are safe, because their members are declared
unconditionally.

**On the count. Re-derived by enumeration in the amendment pass, and the figure
this report gave in the correction pass was wrong.** Round one's arbitration
handed the correction pass "eight accessors across six optional widths"; the
correction pass returned "eight accessors, five capability gates, five distinct
optional bit-widths". Six had no reading in the source and was correctly
rejected. **Five was also wrong, for the widths.** Four separate quantities are
in play here and they take four different values. Each is enumerated below
rather than asserted, per standing instruction 7.

| Quantity | Value | Enumeration |
|---|---|---|
| Bare accessor **call sites** in `Translate2Spirv` | **8** | `spirv_types.cpp:397`, `:399`, `:403`, `:409`, `:411`, `:415`, `:424`, `:428` |
| Distinct optional **SPIR-V types** they return | **8** | `t_int8_`, `t_uint8_`, `t_int16_`, `t_uint16_`, `t_int64_`, `t_uint64_`, `t_fp16_`, `t_fp64_` — each a separate `SType` member, each with its own `declare_primitive_type` call in `spirv_ir_builder.cpp:156-176` |
| **Capability gates** guarding them | **5** | `spirv_has_int8` (`spirv_ir_builder.cpp:156`), `spirv_has_int16` (`:160`), `spirv_has_int64` (`:166`), `spirv_has_float16` (`:171`), `spirv_has_float64` (`:174`); the same five, and only those five, are the optional scalar-type capabilities in `taichi/inc/rhi_constants.inc.h:12-16` |
| Distinct optional **bit widths** | **3** | 8 (i8, u8), 16 (i16, u16, f16), 64 (i64, u64, f64) |

So: **eight accessors returning eight distinct optional types, under five
capability gates, spanning three distinct bit widths.** The correction pass's
"five distinct optional bit-widths" listed types, not widths, and named only
five of the eight because it silently collapsed each signed/unsigned pair. The
paired report's "eight optional types, five capability flags" is the correct
wording for those two quantities and this report now agrees with it.

Adversary 02-1 settled the width figure at three and this report confirms it,
but its supporting sentence — "Five is the count of optional *types* (int8,
int16, int64, fp16, fp64)" — is itself a miscount of the same kind: those five
names are the **capability** names, not type names, and the same paragraph then
credits the paired report's "eight optional types" as precise. Three and eight
cannot both describe the same quantity as five. The enumeration above is what
settles it.

**Why the mandatory widths do not enter the count.** The mandatory widths
reached by the same two visitors — `i32_type()` at `spirv_types.cpp:401`,
`u32_type()` at `:413`, `bool_type()` at `:407`, `f32_type()` at `:426` — are
bare accessors too, and are bypassed in exactly the same way. They are safe only
because their members are declared unconditionally at `spirv_ir_builder.cpp:155`
(`t_bool_`), `:164-165` (`t_int32_`, `t_uint32_`) and `:170` (`t_fp32_`), so
there is no gate to bypass. That is what makes the exposed count eight rather
than twelve.

**Failure mode.** `SType::id` is `uint32_t id{0}` (`spirv_ir_builder.h:51`), and
the result is written into the map at `spirv_types.cpp:418` (int) and `:430`
(float). Id 0 is not a valid SPIR-V result id. There is no `TI_ASSERT` or
`TI_ERROR` on `vt.id` anywhere in `Translate2Spirv`. **What then happens to the
module is traced end to end in §3.3.5a**, added in the amendment pass: it is
caught in-process by the optimiser's parser, downgraded to a warning, and
shipped unchanged.

#### 3.3.4 The bypassed path is live, and it is the kernel-signature path

The visitor runs at the three `ir_translate_to_spirv` calls in
`spirv_codegen.cpp`. **Attribution corrected in the amendment pass — round one,
both round-one adversaries, both reports and the first correction pass all had
the last two the wrong way round.** The enclosing function boundaries, read
directly:

| Function | Opens | Closes | `ir_translate_to_spirv` call |
|---|---|---|---|
| `compile_args_struct` | `spirv_codegen.cpp:2326` | `:2400` | `:2386` — **args struct** |
| `compile_argpack_struct` | `:2402` | `:2481` | `:2464` — **argpack struct** |
| `compile_ret_struct` | `:2483` | `:2527` | `:2511` — **return struct** |

`:2464` is followed by `argpack_struct_type.id = ir2spirv_map[struct_type];` at
`:2465` and `argpack_types_[arg_id] = argpack_struct_type;` at `:2477`; `:2511`
is followed by `ret_struct_type_.id = ir2spirv_map[struct_type];` at `:2512` and
`rets_struct_types_.resize(...)` at `:2514`. A line at `:2464` cannot sit inside
a function that opens at `:2483`. The set of three affected structs is unchanged;
only the pairing was wrong. `ir_translate_to_spirv` itself is
`spirv_types.cpp:476-483`, constructing `Translate2Spirv` at `:480` and visiting
at `:481`.

The tinyir types it consumes are built by `translate_ti_type`
(`spirv_types.cpp:484-514`), called at five sites, which partition across the
same three functions: `spirv_codegen.cpp:2344` and `:2358` in
`compile_args_struct`; `:2420` and `:2436` in `compile_argpack_struct`; `:2494`
in `compile_ret_struct`. Round one cited only that second set. Both are given
here because they are in different functions and the distinction matters: the
first set is where the SPIR-V ids are produced, and escalation 5's second option
— checking at the `translate_ti_type` call sites — has to be applied in the
function the partition above names, not the one the old pairing implied.

The type feed is `translate_ti_primitive` (`spirv_types.cpp:167-214`), which has
**no capability parameter in its signature** and maps i8/u8 at
`:170-172`/`:190-192`, i16/u16 at `:173-175`/`:193-195`, i64 at `:179-181`,
u64 at `:199-201`, f16 at `:202-203` and f64 at `:206-207`.

So a kernel taking, say, an `i64` or an `f64` argument, compiled for a device
that does not report the matching capability, emits an `OpTypeStruct`
referencing result id 0 rather than raising the guarded error.

#### 3.3.5 Nothing upstream rejects it, and nothing downstream catches it

**No front-of-pipeline guard.** `Extension::data64` is declared at
`taichi/inc/extensions.inc.h:6` and granted to x64 (`taichi/program/extension.cpp:12`),
arm64 (`:16`) and cuda (`:20`). Every SPIR-V-consuming arch in that table is
given the empty set or `extfunc` only: metal `:23`, opengl `:24`, gles `:25`,
vulkan `:26`, dx11 `:27`. And **no call site anywhere passes
`Extension::data64`**. `is_extension_supported` itself is live — it is called at
`taichi/program/program.cpp:149` (`assertion`),
`taichi/codegen/llvm/codegen_llvm.cpp:2726` (`bls`), and
`taichi/transforms/compile_to_offloads.cpp:92`, `:205`, `:218`, `:236` (`mesh`)
and `:245`, `:288` (`quant`), plus the Python binding at
`taichi/python/export_lang.cpp:1225`. It is simply never asked about `data64`.
This closes round one's escalation 4: there is no earlier check.

**No middle-of-pipeline check either. [V] Added in the amendment pass, taken
from the paired report.** The guarded `get_primitive_type` **does** run on the
SPIR-V path, at `spirv_codegen.cpp:2267` inside `get_buffer_value`. It never
sees a kernel argument's real type, because every call site but one hands it a
hardcoded placeholder. **Census corrected in the close-out pass.** The amendment
pass wrote ten sites under the label "Every Args- and Rets-buffer call site",
which was wrong twice: it omitted `:2024` and `:2039`, and the label did not
reconcile with its own list, which already carried Root, GlobalTmps, ListGen and
ArgPack sites. Full enumeration, every hit of
`grep -n get_buffer_value taichi/codegen/spirv/spirv_codegen.cpp` opened:

| Kind | Sites | Count |
|---|---|---|
| Hardcoded `PrimitiveType::i32` | `:609-610` (ArgPack), `:618` (Args), `:651` (Rets), `:728`, `:753`, `:788` (Args), `:2024` and `:2039` (GlobalTmps), `:2293` (Args) | 9 |
| Hardcoded `PrimitiveType::u32` | `:401` (Root), `:516` (GlobalTmps), `:2138` (ListGen) | 3 |
| Forwards a caller-supplied `dt` | `:2212` (`at_buffer`) | 1 |
| Not call sites — comments | `:354` (`//`), `:1901` (inside the `/* */` block opened `:1899`, closed `:1903`) | 2 |
| Not a call site — the definition | `:2266` | 1 |

**Thirteen call sites: twelve hardcoded, one forwarding.** `:2024` and `:2039`
are the two omitted ones, both
`get_buffer_value(BufferType::GlobalTmps, PrimitiveType::i32)`, reading the
non-const range-for bounds out of the global-temporaries buffer. Both are
hardcoded like the rest, so the finding is strengthened rather than damaged: the
omission was in the census, not in the conclusion. The source itself calls these
placeholders — `spirv_codegen.cpp:650` reads
``// The `PrimitiveType::i32` in this function call is a placeholder.``

So the guard executes, on a type that is always mandatory, and returns cleanly. The
only call that forwards a caller-supplied `dt` is `at_buffer`'s at `:2212`,
which is the SNode access path, not the kernel-signature path. Without this,
"no earlier check" would rest on `Extension::data64` alone.

**No back-of-pipeline validation.** `spirv_codegen.cpp:2710` is
`spirv_opt_options_.set_run_validator(false);` — unconditional, in
`KernelCodegen::KernelCodegen`. `enable_spv_opt` (`spirv_codegen.h:26`, set from
`compile_config.external_optimization_level > 0` at
`spirv/kernel_compiler.cpp:38`) gates only the pass registrations at
`spirv_codegen.cpp:2683-2708`; `spirv_opt_->Run` at `:2746-2747` executes either
way, with the validator off. `spirv_tools_` (`:2712`) is used only for
`Disassemble` at `:2763`, inside `if constexpr (false)` at `:2758` — compiled
out. **Nothing in the codegen validates the emitted module.**

#### 3.3.5a What actually happens to a module carrying id 0 — traced end to end

**New in the amendment pass, and it corrects the word "silently" that this
report used in escalation 5.** Round one and the correction pass both stopped at
"nothing validates it" and left the severity unstated. The two round-two
adversaries then divided on it: 02-1 held that the failure is *deferred to the
driver at shader-module creation*, with no Taichi-level diagnostic; 02-2 held
that the SPIRV-Tools source checked out in this tree contradicts that. **The
chain was traced link by link in this pass. Adversary 02-2 is right, and
adversary 02-1's proposed sentence is false on its second half.** All **[V]**;
`external/SPIRV-Tools` is a live checkout, so every link below was read.

| # | Link | Site |
|---|---|---|
| 1 | The bare accessor returns a default-constructed `SType`; `uint32_t id{0}` | `spirv_ir_builder.h:51` |
| 2 | That zero is written into the translator's map | `spirv_types.cpp:418` (int), `:430` (float) |
| 3 | It is read back into the `OpTypeStruct` member operand list, and again by `OpMemberDecorate` | `spirv_types.cpp:442-444`, declared `:447`, decorated `:449-453` |
| 4 | `KernelCodegen::run` copies the binary, then calls the optimiser on **every** kernel | copy `spirv_codegen.cpp:2740`, call `:2746-2747` |
| 5 | `Optimizer::Run` skips `tools.Validate` because `run_validator_` is false, then calls `BuildModule` **unconditionally** and returns false on null | skip `external/SPIRV-Tools/source/opt/optimizer.cpp:590-594`, build `:596-597`, return `:598` |
| 6 | `BuildModule` runs `spvBinaryParse` and returns null unless it succeeds | `external/SPIRV-Tools/source/opt/build_module.cpp:68-69`, `:74` |
| 7 | The parser rejects a zero `<id>`. An `OpTypeStruct` member operand is `SPV_OPERAND_TYPE_ID` | `external/SPIRV-Tools/source/binary.cpp:473`; also `:450` (type id), `:456` (result id). Error code `SPV_ERROR_INVALID_ID` |
| 8 | `DiagnosticStream`'s destructor maps that code to a **level**. `SPV_ERROR_INVALID_ID` hits the `default:` arm, so the level stays `SPV_MSG_ERROR` | `external/SPIRV-Tools/source/diagnostic.cpp:87-114`, initialiser `:89`, `default:` `:107-108`, dispatch `:112` |
| 9 | The consumer is Taichi's own, installed at codegen construction | `spirv_codegen.cpp:2682`, function `spriv_message_consumer` at `:2641-2659` |
| 10 | **The branch order downgrades it.** `if (level <= SPV_MSG_FATAL)` at `:2646` catches only level 0; `SPV_MSG_FATAL` is 0 and `SPV_MSG_ERROR` is 2 in the enum at `external/SPIRV-Tools/include/spirv-tools/libspirv.h:83-92`. A parse **error** therefore falls to `else if (level <= SPV_MSG_WARNING)` at `:2649` and is emitted as **`TI_WARN`** | `spirv_codegen.cpp:2646-2652` |
| 11 | `Run` returning false fires a second warning and clears a flag | `TI_WARN_IF(..., "SPIRV optimization failed")` at `:2745-2748`; `success = false` at `:2750` |
| 12 | **The flag is dead.** `success` is declared at `:2742` and read at exactly one place, `:2760`, which is inside `if constexpr (false)` at `:2758-2772` | `spirv_codegen.cpp:2742`, `:2750`, `:2758-2772` |
| 13 | The module is pushed to the caller's output vector regardless. On a failed `Run` that vector still holds the **unmodified** copy made at `:2740`, because `Optimizer::Run` bailed at `optimizer.cpp:598` before reaching `optimized_binary->clear()` at `:642` | `spirv_codegen.cpp:2775` |

**The answer.** The invalid module is **detected inside Taichi's own process**,
by the optimiser's parser rather than by any check Taichi wrote; the diagnostic
is **downgraded from error to warning** by `spriv_message_consumer`'s branch
order at `spirv_codegen.cpp:2646-2651`; the flag that records the failure is
**never read outside a compiled-out block**; and the unmodified invalid module
is **shipped to the caller** at `:2775`. It is not silent — two `TI_WARN`s are
emitted — and it is not fatal.

**Which adversary was right.** 02-2's chain is confirmed at every link. 02-1's
claim that "a 0 `<id>` operand is a hard parse failure for any conformant
SPIR-V consumer" is true and is exactly why link 7 fires; its inference that
"there is no Taichi-level diagnostic" and that "the failure is deferred to the
driver" is **wrong**, because the parse happens in-process before any driver
sees the module. Had 02-1's sentence been adopted, this report would have
asserted a deferral that does not occur and would have left the dead-flag defect
at `spirv_codegen.cpp:2750` unrecorded.

**What this report does not claim.** That the module also reaches a driver, and
what a driver does with it, is outside this tree. `generated_spirv` is an output
parameter of `KernelCodegen::run`; this report does not trace it past
`spirv_codegen.cpp:2775`. So the deferred-to-driver outcome may well hold **in
addition**, but it is not what the failure is deferred to — it is already caught
and discarded before then.

**Consequence for item 6.2, which is a decision and is escalated at §5.5.** This
splits into two work items, not one:

1. A **diagnostic** at `spirv_types.cpp:397-428`, where the type and the missing
   capability are both still in hand and a `TI_ERROR` matching
   `get_primitive_type`'s (`spirv_ir_builder.cpp:288-332`) could name both. The
   existing warning names neither — it reports a byte offset into a binary.
2. A **correctness question** at `spirv_codegen.cpp:2742-2775`, independent of
   6.2: the compile records its own failure in `success` and then discards it.
   Whether the intent was to fall back to `task_res.spirv_code`, to fail the
   compile, or neither, is not determinable from the source, and the standing
   instructions forbid this report proposing a change.

#### 3.3.6 Three further unguarded reads of the 64-bit STypes

Weaker than §3.3.3 but recorded:

1. `spirv_ir_builder.cpp:337-342` `from_taichi_type` — pointer -> `t_uint64_`
   when `has_buffer_ptr`, no `spirv_has_int64` check. Reachable only under
   `spirv_has_physical_storage_buffer`, whose only setter is guarded by the same
   `shaderInt64` test that sets `spirv_has_int64` on Vulkan
   (`vulkan_device_creator.cpp:821` and `:630`), so the invariant holds in
   practice on the one backend that could set it — and is moot today because
   that setter is compiled out (§3.3.8).
2. `spirv_ir_builder.cpp:373-376` `get_primitive_uint_type` — returns
   `t_uint64_` for 64-bit dt with no check. Only reached with a 64-bit `dt`
   that would have had to survive `get_primitive_type` elsewhere.
3. `spirv_codegen.cpp:783-792` — loads a `u64` ndarray base address under the
   `spirv_has_physical_storage_buffer` guard only. Same standing as (1).

Also `spirv_types.h:71-77` `PhysicalPointerType` derives from
`IntType(/*num_bits=*/64, /*is_signed=*/false)` at `:75`, **unconditionally**.
Its only construction site is `snode_struct_compiler.cpp:53`, inside
`StructCompiler::construct` (`:34-63`), which is dead — the tinyir SNode-type
path is commented out at `snode_struct_compiler.cpp:16-19` and `:22-28`.

#### 3.3.7 Absent — the in-tree target set, corrected

Round one said `spirv_has_int64` "is set in three places" and named Vulkan,
OpenGL and Metal, calling the Metal setter unconditional. **That enumeration was
wrong twice: the Metal setter is gated, and a fourth setter exists outside
`taichi/`.** Tree-wide grep for `spirv_has_int64` gives four setters:

| Setter | Guard | Verdict |
|---|---|---|
| `taichi/rhi/vulkan/vulkan_device_creator.cpp:632` | `if (device_supported_features.shaderInt64)` at `:630` | queried |
| `taichi/rhi/opengl/opengl_device.cpp:511` | `if (!is_gles())` at `:509`, comment "64bit isn't supported in ES profile" at `:510` | profile test, no feature query |
| `taichi/rhi/metal/metal_device.mm:1052` | `if (feature_64_bit_integer_math)` at `:1051` | **queried — round one's "unconditional" was wrong** |
| `c_api/src/taichi_opengl_impl.cpp:9` | **none** | asserted unconditionally |

Round one also cited the Vulkan gate as `:628-631`; `:628` is the **int16** set.
Corrected to `:630-632`.

**Metal is a second in-tree target lacking Int64.**
`bool feature_64_bit_integer_math = family_apple3;` at `metal_device.mm:1038`,
where `family_apple3` (`:1035-1036`) is
`[mtl_device supportsFamily:kMTLGPUFamilyApple3] | family_apple4` and
`kMTLGPUFamilyApple3 = MTLGPUFamily(1003)` at `:1025`. The read-back round one
noted is real — `caps.contains(DeviceCapability::spirv_has_int64)` at
`:131-132`, feeding `options.set_msl_version(2, 3, 0)` at `:134` — but the set
is gated. Pre-Apple3 Metal has no Int64.

**Direct3D 11 is a third.** `Dx11Device::Dx11Device`
(`taichi/rhi/dx/dx_device.cpp:557-568`) builds `DeviceCapabilityConfig caps{}`
at `:563`, sets **only** `spirv_version` at `:564`, and calls `set_caps` at
`:565`. No integer or float capability of any kind. Round one did not mention
DX11 at all.

**Imported Vulkan is a fourth.** `VulkanRuntimeImported::Workaround::Workaround`
(`c_api/src/taichi_vulkan_impl.cpp:19-56`, declared at
`c_api/src/taichi_vulkan_impl.h:27-31`) builds `caps{}` at `:35`, sets only
`spirv_version` (`:37-43`), and calls `set_caps` at `:53`; the
physical-storage-buffer set is commented out at `:45-51`. An imported Vulkan
device reports no Int64 regardless of what the physical device supports.

**The C API OpenGL runtime sets Int64 unconditionally and discards the GLES
guard.** `OpenglRuntime::OpenglRuntime` (`c_api/src/taichi_opengl_impl.cpp:4-13`)
builds a fresh `DeviceCapabilityConfig caps{}` at `:8`, sets `spirv_has_int64`
at `:9`, `spirv_has_float64` at `:10` and `spirv_version` at `:11` with no
guard, then calls `get_gl().set_caps(std::move(caps));` at `:12`.

`Device::set_caps` is `taichi/rhi/public_device.h:855-857`, body
`caps_ = std::move(caps);` — a **whole-object replacement, not a merge**. It
therefore discards everything `GLDevice::GLDevice`
(`taichi/rhi/opengl/opengl_device.cpp:506-530`) had built: the `is_gles()` guard
at `:509-513`, the `GLAD_GL_NV_gpu_shader5` int16/float16 tests at `:515-518`,
the `GLAD_GL_AMD_gpu_shader_int16` int16 test at `:520-522`, the
`GLAD_GL_AMD_gpu_shader_half_float` float16 test at `:524-526` — **span
corrected in the amendment pass** from `:515-522`, which omitted the half-float
block — and its own `set_caps` at `:529`. **So the override does not only add
capabilities, it removes them:** any `spirv_has_int16` or `spirv_has_float16`
those three probes had granted is discarded, because the C API constructor sets
neither. The entry point is
`ti_import_opengl_runtime(..., bool use_gles)` at `taichi_opengl_impl.cpp:21-29`,
which forwards to `set_gles_override(use_gles)` at `:26` before constructing the
runtime at `:28`. **On that path a GLES device reports Int64.**

Whether any shipped configuration exercises that path is not determinable from
the source, and no live failure is asserted here. What is asserted is that the
round-one picture — one awkward legacy profile, guarded by a profile test — is
not what the source says.

**Related, recorded as fact.** `ti_set_runtime_capabilities_ext`
(`c_api/src/taichi_core_impl.cpp:317-334`) builds a `DeviceCapabilityConfig`
from caller-supplied pairs at `:325-330` and installs it with `set_caps` at
`:331`. Any capability, `spirv_has_int64` included, can be asserted from outside
the library with no device query at all.

#### 3.3.7a The same question asked of all five bypassed capabilities

**New in the amendment pass, and it is the largest thing both reports were
missing.** §3.3.3 establishes that the bypass covers eight accessors under
**five** capability gates. §3.3.7 above, round one, the correction pass and both
round-two adversaries' round-one passes then enumerated the in-tree target set
for `spirv_has_int64` **only**. The other four were never asked about. Adversary
02-2 ran the enumeration and this pass re-derived it independently by grep over
`taichi/` and `c_api/`, then opened each site.

**Every setter of every one of the five, tree-wide.** Sites in
`taichi/codegen/spirv/spirv_ir_builder.cpp` are `get`s, not sets, and are
excluded; `taichi/inc/rhi_constants.inc.h:12-16` is the declaration of the five.

| | Vulkan `vulkan_device_creator.cpp` | OpenGL desktop `opengl_device.cpp` | OpenGL **ES** | Metal `metal_device.mm` | DX11 `dx_device.cpp` | Imported Vulkan `taichi_vulkan_impl.cpp` | C API OpenGL `taichi_opengl_impl.cpp` |
|---|---|---|---|---|---|---|---|
| `spirv_has_int8` | `:790`, queried (`shaderInt8`, `:789`) | **never set** | **never set** | `:1046`, **unconditional** | **never set** | **never set** | **never set** |
| `spirv_has_int16` | `:628`, queried (`shaderInt16`, `:626`) | `:516` / `:521`, extension probes | same probes | `:1047`, **unconditional** | **never set** | **never set** | **never set** |
| `spirv_has_int64` | `:632`, queried (`shaderInt64`, `:630`) | `:511`, profile test `:509` | **never set** | `:1052`, gated on `family_apple3` `:1051` | **never set** | **never set** | `:9`, **unconditional** |
| `spirv_has_float16` | `:787`, queried (`shaderFloat16`, `:786`) | `:517` / `:525`, extension probes | same probes | `:1048`, **unconditional** | **never set** | **never set** | **never set** |
| `spirv_has_float64` | `:636`, queried (`shaderFloat64`, `:634`) | `:512`, profile test `:509` | **never set** | **never set — see below** | **never set** | **never set** | `:10`, **unconditional** |

Three results follow, and none of them is in either report:

**1. [V] Metal reports no Float64 on any device, ever.**
`taichi/rhi/metal/metal_device.mm` contains **no occurrence of the string
`float64`** anywhere in the file. The capability block at `:1044-1053` sets
`spirv_version`, `spirv_has_int8`, `spirv_has_int16`, `spirv_has_float16` and
`spirv_has_subgroup_basic`, then `spirv_has_int64` under the `family_apple3`
test. Float64 is simply absent. Combined with the bare `f64_type()` at
`spirv_types.cpp:428`, **an `f64` kernel argument, return value or argpack
member compiled for Metal emits an `OpTypeStruct` referencing id 0 on every
Metal device, current hardware included.** This is stronger than the Int64 case
both reports led with: Int64 is missing on a legacy subset, Float64 is missing
on the whole backend unconditionally.

**2. [V] OpenGL reports no Int8 on any profile.** `GLDevice::GLDevice`
(`opengl_device.cpp:506-530`) never sets `spirv_has_int8` under any branch. So
an `i8` or `u8` kernel argument on OpenGL takes `i8_type()` / `u8_type()`
(`spirv_types.cpp:397`, `:409`) and emits id 0, desktop profile included.

**3. [V] DX11 and imported Vulkan lack all five, not just Int64.** Each sets
`spirv_version` and nothing else — `dx_device.cpp:563-565`,
`taichi_vulkan_impl.cpp:35-53`. §3.3.7 above already said "no integer or float
capability of any kind" for DX11 and "no Int64" for imported Vulkan; the second
was narrower than the truth, and neither was connected to the other four
accessors.

**What this does to 6.2's stub-architecture requirement, stated as fact rather
than as a proposal.** The architecture is not being asked to express "some old
targets lack Int64". It is being asked to express a **five-by-seven matrix in
which no configuration reports all five, one backend is missing a capability on
every device it will ever run on, and two paths assert capabilities with no
device query**. Under plan §5.2's closing consequence, a portable path that
emits an invalid module for an `f64` argument across an entire backend is a gap
in the SPIR-V spine to record, not evidence that the spine matters less.

For 6.2's stub-architecture requirement this changes the shape of the problem
twice over. The architecture is not being asked to accommodate one legacy
profile. Four in-tree device configurations can present without Int64, two paths
can affirm it without checking, and the same question asked of the other four
capabilities yields a backend with no Float64 at all.

#### 3.3.8 The 64-bit SPIR-V address path is entirely dead today

`spirv_has_physical_storage_buffer` has exactly one setter in the whole tree,
`vulkan_device_creator.cpp:826`. Precise framing, corrected: the enclosing test
is `if (device_supported_features.shaderInt64)` at `:821`; `:822-824` are
comments citing upstream issue 6295; the guard is
`#if !defined(__APPLE__) && false` at `:825`; the setter is `:826`; `#endif` at
`:827`. Round one cited the guard as `:823-827`, and `:823` is a URL comment.

The `&& false` means it is never set. So `has_buffer_ptr` is always false, and
every 64-bit branch that depends on it — `from_taichi_type`'s pointer case,
`at_buffer`'s `OpConvertUToPtr` path (`spirv_codegen.cpp:2197-2204`),
`ExternalPtrStmt`'s `OpSConvert`-to-u64 path (`:783-792`) — is unreachable and
untested.

**[I]** Independent of that: `use_64bit_pointers` (S1) is separate from both
capabilities. Even a device reporting Int64 *and* physical storage buffers gets
u32 SNode pointers, because `make_pointer` never consults `caps_`.

---

### 3.4 Citation audit, correction pass

Round one's citations were re-opened one at a time and checked for
**attribution** — that the cited line is the line doing the thing claimed — not
merely that the quoted text appears nearby. Full working in
`notes-02-codegen.md` §31.

#### 3.4.1 Faults corrected in place above

Nine were already flagged by one or both adversaries: `codegen_llvm.cpp:2291-2294`
(L14), `snode.h:40,44,49` (§4.1.2), `spirv_ir_builder.cpp:575`, `:590` and
`:592-599` (S14), `vulkan_device_creator.cpp:628-631` (§3.3.7),
`get_primitive_type`'s u64 error span (§3.3.2), `spirv_codegen.cpp:541-561`
(escalation 5), and the omission of `runtime.cpp:1005` (§2.1).

Eleven more were found in this pass and had been missed by both adversaries:

| Round-one site | Claimed | Actual |
|---|---|---|
| §3.1 L9 | mul/add chain at `codegen_llvm.cpp:1938`, `:1941`, `:1943` | muls `:1939`, `:1942`; add `:1944`. All three cited lines are a continuation line, a comment and a closing brace. |
| §3.1 L12 | loop index stored/loaded at `:2213-2214`, `:2222`, `:2226`, `:2251` | store `:2190-2191`; loads `:2216`, `:2228`, `:2253`; increment `:2273`. All four cited lines are two `SetInsertPoint` calls, a blank line and a comment. |
| §3.2 S15 | `const_i32_zero_`/`const_i32_one_` at `spirv_ir_builder.cpp:283-284` | `:223-224`. `:282-286` is `get_null_type` — wrong function. |
| §3.3 | i64 error at `spirv_ir_builder.cpp:310-313` | guard `:311-313`, return `:314`; `:310` is `return t_int32_;` |
| §3.2 S6 | `bitmasked_activation` at `spirv_codegen.cpp:389-435` | `:383-435` |
| §3.2 S6 | `make_pointer(...)` at `:405-406` | `:406-408`; the call is on `:408` |
| §3.2 S16 | `PointerType` translation at `spirv_types.cpp:486-495` | branch `:490-498` |
| §3.1 L20 | ABI comment at `codegen_llvm.cpp:286-294` | `:284-292` |
| §3.1 L6 | bit-pointer struct comment at `:1752-1756` | `:1750-1754`; `:1755` is the assertion |
| §4.1.9 | `check_func_call_signature` error at `llvm_codegen_utils.cpp:139-140` | `TI_ERROR` at `:141-142`; `:139-140` is a `TI_INFO` about differing contexts |
| §5 esc. 3 | dead PSB guard at `vulkan_device_creator.cpp:823-827` | `:821` test, `:825` `#if`, `:826` setter, `:827` `#endif` |

Eight further citations had span edges off by one. The correction pass
disclosed them here and left them standing in the tables above, on the grounds
that they land on the right code. **Both round-two adversaries objected to that
disposition, and they are right: a table a reader cannot transcribe is not an
inventory. All eight are now applied in the rows and paragraphs where they are
read.** Each was re-opened in the amendment pass before editing.

| Where | Was | Now | Verified |
|---|---|---|---|
| §3.0 `get_constant(T)` | `llvm_context.cpp:727-750` | `:727-749` | `:727` `template <typename T>`, `:749` closing brace, `:750` blank |
| §3.0 `get_constant(DataType,T)` | `:696-717` | `:695-718` | `:695` `template <typename T>`, `:696` signature, `:718` closing brace |
| §3.1 L7 LLVM `SNodeLookupStmt` | `codegen_llvm.cpp:1794-1828` | `:1792-1829` | `:1792` signature, `:1829` closing brace |
| §3.2 S2 `make_pointer` | `spirv_codegen.cpp:2317-2325` | `:2317-2324` | `:2317` signature, `:2324` closing brace, `:2325` blank |
| §3.2 S5 SPIR-V `SNodeLookupStmt` | `:503-507` | `:504-508` | `:503` blank; `add` at `:508` was outside the old span |
| §3.2 S7 SPIR-V `LinearizeStmt` | `:531-540` | `:532-541` | `:531` blank, `:532` signature, `:541` closing brace |
| §3.2 S8 `at_buffer` idx | `:2215-2218` | `:2214-2218` | `make_value` opens at `:2214` |
| §3.2 S10 SPIR-V `ExternalPtrStmt` shift | `:772-777` | `:773-777` | `:772` is a closing brace |

Two further spans, cited elsewhere in this report and confirmed exact in the
amendment pass rather than corrected: `ir_translate_to_spirv` is
`spirv_types.cpp:476-483` and `translate_ti_type` is `:484-514`.

#### 3.4.2 Checked and correct

Everything not listed above was re-opened and holds, including all of L1-L5,
L7-L8, L10-L11, L15-L19 and L21-L24; the seventeen constant-GEP lines and all
twenty-two `codegen_llvm_quant.cpp` lines; S1-S5, S7-S13; the `get_constant`
width dispatch; the `taichi_max_num_snodes` and `taichi_max_num_indices`
single-site claims (grep confirms four and two sites respectively); the whole
per-tree-versus-global-id chain; and the runtime ABI at `runtime.cpp:288-290`,
`:308`, `:310`, `:312-322` with `node_dense.h:10-24` and `node_pointer.h:10,84,90`.

#### 3.4.3 Adjudicating where the two adversaries contradict each other

Two places. The source was opened in both.

1. **`bitmasked_activation` line numbers.** `adversary-02-1.md` §3 row 2
   substitutes `:404`, `:410`, `:411`, `:413` for pass B's `:405`, `:411-412`,
   `:414`. `adversary-02-2.md` §9.4 says that correction is itself wrong.
   **Adversary 02-2 is right.** The `ir_->u32_type()` calls inside
   `spirv_codegen.cpp:383-435` are on `:405`, `:411`, `:412`, `:414` (then
   `:417`, `:421`, `:422`, `:427`, `:428`, `:430`, `:431`, `:433` in the op
   branches). `:404`, `:410` and `:413` are statement-opening lines with no
   `u32_type()` call — `:404` is
   `ir_->make_value(spv::OpShiftLeftLogical, ptr_dt, bitmask_word_index,` and
   `:413` is a bare `bitmask_word_ptr =`. Adversary 02-1 cited statement starts
   while correcting a report that had cited the calls.
2. **The two runtime helper functions.** Adversary 02-1 places
   `node_allocators[snode_id]` at `runtime.cpp:1029` and
   `ambient_elements[snode_id]` at `:1038`. Adversary 02-2 places the enclosing
   functions at `:1291-1296` and `:1298-1304`. **Adversary 02-1 is right.**
   `runtime_NodeAllocator_initialize` is `:1026-1031` and
   `runtime_allocate_ambient` is `:1033-1040`; the lines adversary 02-2 cites
   are roughly 265 lines away, inside the element-listgen code.

One each. Neither analysis is reliable enough to transcribe without opening the
file, which is the conclusion adversary 02-2 reached about the two reports and
which applies to the adversaries in turn.

One further correction to adversary 02-2, on a supporting claim rather than a
conclusion: it states `is_extension_supported` is "called exactly once in the
whole tree". It is called at nine C++ sites (§3.3.5). The conclusion it supports
— that `Extension::data64` is never queried — is nonetheless correct.

#### 3.4.4 A coverage gap in this report's own sweep

Round one's §6 sweep of "files in my territory with nothing width- or
index-relevant" omits fourteen files that exist under `taichi/codegen/`:
`codegen.h`, `llvm/codegen_llvm.h`, `llvm/llvm_codegen_utils.h`,
`llvm/struct_llvm.h`, `llvm/compiled_kernel_data.cpp`,
`spirv/compiled_kernel_data.cpp`, `spirv/kernel_compiler.h`,
`spirv/kernel_utils.cpp`, `spirv/kernel_utils.h`, `dx12/dx12_llvm_passes.h`, and
the four per-backend `codegen_*.h` headers.

Thirteen are headers or serialisation for files already covered. **One is not.**
`taichi/codegen/spirv/kernel_utils.h` carries the `size_t`-to-`int` narrowing
now recorded as S17. The sweep is closed by that row and by this list.

Both round-two adversaries re-derived the 45-file arithmetic in §6
independently — one by `find` and one by checking every basename against the
report — and both found it exact.

**Withdrawn in the close-out pass, and this is where the repetition fault
bites.** What those two adversaries checked, and what round three re-checked, is
the *file count*: 45 `.h`/`.cpp` files, 24 in the irrelevant list, 21 remaining.
That part is exact and survives. What none of the four checked until round three
is the *reconciling sentence*, which claimed the 21 were covered by §3.1, §3.2
and §3.3. They are not: two of them are covered elsewhere in this report.
§6 now carries the corrected statement and the mechanical derivation. The
sentence above overstated what was verified, in exactly the way standing
instruction §10.7 warns about — the adversaries certified an arithmetic, and it
was read here as certifying the claim the arithmetic sat next to.

#### 3.4.5 Adjudicating the round-two adversaries

Same method as §3.4.3: where they contradict each other, or contradict this
report, the source was opened. Nothing was adopted on confidence.

| # | Question | Ruling | Where |
|---|---|---|---|
| 1 | Are `spirv_codegen.cpp:2464` and `:2511` swapped in this report? | **Adversary 02-1 is right, and 02-2 concurs after seeing it.** Function boundaries read directly. Corrected. | §3.3.4 |
| 2 | Is there a second widening-invalidity site at `:398-399`? | **Adversary 02-1 is right**, and the rule is confirmed from the validator in this tree rather than from the specification. Added. | S6b, §4.2.2 |
| 3 | Is "five distinct optional bit widths" wrong? | **Yes, and so is adversary 02-1's replacement figure.** Enumerated: 8 accessors, 8 types, 5 gates, **3** widths. | §3.3.3 |
| 4 | What happens to the id-0 module? | **Adversary 02-2 is right; adversary 02-1 is wrong on the half that matters.** The failure is caught in-process by the optimiser's parser, not deferred to the driver, and there **are** two Taichi-level diagnostics. Traced link by link. | §3.3.5a |
| 5 | Does §4.2 point 2 contradict §4.2 point 5? | **Adversary 02-2 is right.** Point 5 was round-one text left standing behind contradicting new text. Withdrawn, with both sub-cases opened. | §4.2.5 |
| 6 | Should the eight disclosed span faults be fixed in the tables? | **Both adversaries are right.** Applied. | §3.4.1 |
| 7 | Does Metal ever set `spirv_has_float64`? | **Adversary 02-2 is right. It does not, anywhere, on any device.** The enumeration is extended to all five capabilities. | §3.3.7a |
| 8 | Does this report's §2.1 overstate the sparsity precondition? | **Adversary 02-2 is right and adversary 02-1's credit to this report was misplaced.** The finding was right; the conclusion eight lines later contradicted it. Corrected. | §2.1 |
| 9 | Is `VulkanRuntimeImported::Inner` a real class? | **Both adversaries are right. It is not.** The correction pass introduced the name; the member `inner_` at `taichi_vulkan_impl.h:31` is the likely source. Corrected to `Workaround::Workaround`. | §3.3.7 |
| 10 | Should this report take the paired report's `get_buffer_value` finding? | **Adversary 02-1 is right.** It is the middle link in this report's own escalation-5 closure. Added, with the placeholder call sites enumerated. | §3.3.5, §5.5 |

**Two round-two rulings that went in this report's favour and are recorded so
the planner does not carry an adversary's amendment forward.** Adversary 02-1
withdraws its round-one correction of `bitmasked_activation`'s line numbers and
confirms this report adjudicated correctly against it (its §1.3); adversary 02-2
confirms the same, and separately confirms that this report was right and 02-2
itself wrong on the two runtime helper functions at `runtime.cpp:1026-1031` and
`:1033-1040`. Both §3.4.3 rulings stand unchanged.

---

## 4. What would change in each path if index and address width became 64-bit

Descriptive, not a proposal. **[I]** unless a specific line is cited as **[V]**.

### 4.1 LLVM path

1. **`get_constant` call sites must become width-explicit.** Because the width
   comes from the C++ argument type (§3.0), every index-carrying site in the
   L-table has to change its argument type or move to
   `get_constant(DataType, T)`. There is no central switch.
2. **`AxisExtractor` widths.** `num_elements_from_root`, `shape` and
   `acc_shape` are `int` (`taichi/ir/snode.h:41`, `:45`, `:49` — **corrected**
   from `:40,44,49`). The `acc_shape * shape` product at
   `struct_llvm.cpp:174-175` overflows in C++ before reaching LLVM, so the field
   types must widen, not just the emitted constants. **Agent 01's territory —
   the codegen consumes it.**
   The product then becomes the divisor of a `CreateURem` at
   `struct_llvm.cpp:179`, so an `int` product that wraps to exactly zero yields
   `urem x, 0`, immediate undefined behaviour in LLVM IR rather than a wrapped
   index. Wrapping to any other value gives a wrong-but-defined divisor. Both
   are failure modes; the second is the commoner one.
   **[V] The strides that feed `LinearizeStmt` are produced outside codegen at
   `int` too**: `LinearizeStmt::strides` is `std::vector<int>`
   (`taichi/ir/statements.h:1280`), built at
   `taichi/transforms/scalar_pointer_lowerer.cpp:82` from
   `std::array<int, taichi_max_num_indices> total_shape` declared at `:33` and
   accumulated at `:37`. The same array shape appears at
   `taichi/transforms/demote_dense_struct_fors.cpp:19-24`. Widening
   `LinearizeStmt` without those is a half-change. Territory 01's files, named
   here because they are the direct upstream of L5 and S7.
3. **The refine-coordinates ABI changes shape.** `struct_llvm.cpp:153`'s third
   parameter, `PhysicalCoordinates::val`'s element type
   (`runtime.cpp:289`), and `StructMeta::refine_coordinates`'s signature
   (`runtime.cpp:320-322`) are one contract and must move together.
4. **The SNode accessor ABI changes.** `lookup_element(Ptr, Ptr, int i)`,
   `is_active(Ptr, Ptr, int i)`, `get_num_elements` (`runtime.cpp:312-318`,
   `node_dense.h:10-24`, `node_pointer.h:10,84,90`) take `int`. Codegen calls
   them at `codegen_llvm.cpp:1811-1815`.
5. **The offload body signatures change.** `codegen_llvm.cpp:2113-2119`, with
   the two `get_data_type<int>()` parameters at `:2117-2118`, and the four
   per-backend `get_data_type<int>()` sites (L21-L24).
6. **The SPMD builtins constrain the loop index.** **[V]**
   `nvvm_read_ptx_sreg_tid_x` and `ntid_x` (`codegen_cuda.cpp:770-774`) are i32
   by LLVM's own intrinsic signature; AMDGPU's equivalents at
   `codegen_amdgpu.cpp:459-467` likewise. A 64-bit `loop_index`
   (`codegen_llvm.cpp:2157`) would need explicit `ZExt` at the initialisation
   (`:2212-2214`) and the increment (`:2274`). The intrinsics themselves cannot
   widen.
7. **`create_bit_ptr`'s assertion** (`codegen_llvm.cpp:1755`) is a deliberate
   32-bit contract on bit offsets. Since physical types are at most 64 bits it
   need not widen, but any blanket widening of the constant-emission path will
   trip it. Worth knowing it will fail loudly rather than silently.
8. **`list_element_size`'s narrowing** (`codegen_llvm.cpp:2290-2291`) is an
   `int64` -> `int` assignment that must be revisited. It is safe today because
   `taichi_listgen_max_element_size = 1024` (`taichi/inc/constants.h:28`) bounds
   the `std::min`.
9. **[V] The build will tell you where you missed.**
   `check_func_call_signature` (`llvm/llvm_codegen_utils.cpp:103-145`,
   `TI_ERROR` at `:141-142` — **corrected** from `:139-140`, which is a
   `TI_INFO` about differing LLVM contexts) compares every argument's
   `llvm::Type` against the callee's
   declared parameter type and errors on mismatch; only pointer-type renaming is
   auto-fixed (`:127-131`). Any codegen-side widening not matched in the runtime
   module fails at compile time rather than truncating. **This substantially
   de-risks the LLVM half.**
10. **Machine addresses are already 64-bit.** `GEP`, `PtrToInt`/`IntToPtr`
    (`codegen_llvm.cpp:1864-1870`), `StructMeta::element_size` (`size_t`), and
    `Dense_lookup_element`'s `element_size * i` (`node_dense.h:23`, a `size_t`
    multiply) all already work at 64 bits. On the LLVM path the change is an
    **index** width change, not an address-space change.
11. **[V] The container size is carried at 64 bits everywhere on the LLVM side
    except two places, and both are ABI.** `SNode::max_num_elements()` returns
    `int64` (`taichi/ir/snode.h:306-308`, backed by
    `int64 num_cells_per_container` at `:97`), it sizes the LLVM body array at
    `struct_llvm.cpp:72`, and it is stored as `i64` in `StructMeta`
    (`runtime.cpp:310`) from an i64 constant (`codegen_llvm.cpp:281`). The two
    narrowings are the refine-coordinates ABI in point 3 above, whose `l`
    parameter carries the within-container linear index at i32
    (`struct_llvm.cpp:153`), and `Dense_get_num_elements`, which returns that
    same `i64` field through an `i32` return type (`node_dense.h:10-12`, field
    declared `i32 (*get_num_elements)(Ptr, Ptr)` at `runtime.cpp:318`). That
    pair is the precise shape of the LLVM half of 6.2.

### 4.2 SPIR-V path

1. **`use_64bit_pointers` (`spirv_codegen.cpp:82`) is the one decision point**
   for SNode addressing. Flipping it changes S3-S6 and S8 together, because they
   all flow through `make_pointer`.
2. **[V] It cannot be flipped unconditionally, and flipping it alone emits an
   invalid module rather than a wider one.** A u64 pointer requires
   `OpCapability Int64` (`spirv_ir_builder.cpp:64-66`) and therefore
   `spirv_has_int64`, which four in-tree device configurations do not report
   (§3.3.7). Both widths must remain expressible — exactly the
   stub-architecture constraint the brief's 6.2 note describes. Three further
   couplings, all **[V]**:
   - `make_pointer`'s 64-bit branch calls `ir_->u64_type()`
     (`spirv_codegen.cpp:2320`), the same unguarded accessor as §3.3.3. On a
     device without Int64 that yields type id 0.
   - `at_buffer` branches on `ptr_val.stype.dt == PrimitiveType::u64` at
     `spirv_codegen.cpp:2197`, i.e. on the SType's data type, **not** on
     `has_buffer_ptr`. `declare_primitive_type` sets `t.dt = dt`
     (`spirv_ir_builder.cpp:1536`), so `u64_type()` carries `dt == u64` whenever
     Int64 was present. A widened `make_pointer` therefore routes every SNode
     access into the `OpConvertUToPtr` /
     `spv::StorageClassPhysicalStorageBuffer` branch at `:2198-2204`, while
     `OpCapability PhysicalStorageBufferAddresses` is emitted only under
     `spirv_has_physical_storage_buffer` (`spirv_ir_builder.cpp:73-77`) — which
     is compiled out (§3.3.8). The module would use a capability its own header
     never declares.
   - `bitmasked_activation` breaks in **four** places on its `SNodeLookupStmt`
     path and **two** on its `SNodeOpStmt` path, in both directions (S6a-S6d),
     and the difference between the two paths is made by a caller, not by the
     function (S6e). **Corrected in the close-out pass; the text below said
     "two" unconditionally, and a repair scoped from it would have landed on two
     of five places.** **[V] The governing rule, read from the validator in this
     tree rather than quoted from the specification:**
     `external/SPIRV-Tools/source/val/validate_bitwise.cpp:65-104` handles
     `OpShiftLeftLogical` / `OpShiftRightLogical` / `OpShiftRightArithmetic` and
     requires Base to have the same **bit width** as Result Type — the test is
     `_.GetBitWidth(base_type) != _.GetBitWidth(result_type)` at `:88-91`. It
     checks bit width and dimension, **not signedness**, which is why S6b's i32
     Base under a u32 Result Type is legal today and stops being legal the
     moment `ptr_dt` becomes 64-bit. The Shift operand is unconstrained in
     width (`:93-101` check only its type and dimension), which is why
     `:404-405`'s u32 shift under a `ptr_dt` Result Type is safe either way.
     **`OpBitwiseAnd` is governed by a different and stricter arm**,
     `validate_bitwise.cpp:106-141`, which loops over every operand from index 2
     (`:118-119`) and requires each to match Result Type's bit width
     (`:134-138`). That arm has no unconstrained-operand escape at all.

     So a widened `make_pointer` yields **four invalid instructions on the
     `SNodeLookupStmt` path**, not a truncation:

     | Site | Result Type | The operand that does not match | Direction |
     |---|---|---|---|
     | `:410-412` (S6a) | hardcoded `u32_type()` | Base `bitmask_word_ptr`, pointer-width | Base wider than Result |
     | `:398-399` (S6b) | `ptr_dt` | Base `const_i32_one_`, permanently i32 | Base narrower than Result |
     | `:392-394` (S6c) | `ptr_dt` | Base `input_index`, i32 on this path | Base narrower than Result |
     | `:395-397` (S6d) | `ptr_dt` | operand `input_index`, i32 on this path | operand narrower than Result |

     **Two of the four are caller-dependent, and that is the second mechanism.**
     S6c and S6d are invalid only when `input_index` arrives uncast, which is
     what `visit(SNodeLookupStmt*)` does at `:488-489`; `visit(SNodeOpStmt*)`
     casts at `:443-444` and both stay valid on its path. **Today all four are
     legal, for the same reason S6b is: `ptr_dt` is u32 and i32 is the same
     width, and the validator tests bit width, not signedness.** They become
     invalid together the moment `ptr_dt` becomes 64-bit.

     Two consequences for any repair, neither of them a proposal:

     1. A repair that only replaces `u32_type()` with the pointer type fixes S6a
        and leaves S6b, S6c and S6d broken.
     2. **S6c and S6d cannot be fixed inside `bitmasked_activation` at all.**
        The function is already consistent with itself; what is inconsistent is
        that one of its two callers casts the index and the other does not. The
        fix is at the call site, or in the parameter's declared type, and this
        report does not decide which.

     **Grade, per standing instruction §10.8: ARCHITECTURAL.** If every driver,
     hardware generation and specification were ideal today, the mismatch would
     still be there, because it follows from decisions inside this codebase
     about how the bitmask index is represented and where it is cast. It does
     not expire. **Blast radius, stated separately: five places in one file** —
     four instructions in `taichi/codegen/spirv/spirv_codegen.cpp:383-435`, plus
     the call site at `:488-491` where the uncast index enters. Small, which is
     the point of grading kind and radius separately.
3. **The plumbing for that decision already exists.** **[V]**
   `DeviceCapabilityConfig` reaches `TaskCodegen::caps_` and `IRBuilder::caps_`
   (`kernel_compiler.cpp:37`, `spirv_codegen.h:25`, `spirv_ir_builder.h:224`,
   `:598`). `use_64bit_pointers` is the only address-width decision that does
   not consult it.
4. **The 64-bit branch is unfinished and untested.** **[V]** Its own comment
   says so (`spirv_codegen.cpp:2319`), and every 64-bit address path is gated on
   `spirv_has_physical_storage_buffer`, whose single setter at
   `vulkan_device_creator.cpp:826` sits inside `#if !defined(__APPLE__) && false`
   at `:825-827` (**framing corrected** from `:823-827`). Nothing in CI can be
   exercising it.
5. **[V] WITHDRAWN AND REPLACED in the amendment pass. The SSBO index does
   *not* widen with the pointer, because a widened pointer never reaches the
   shift.** Round one wrote "a u64 pointer yields a u64 `OpAccessChain` index",
   citing `at_buffer`'s shift. The correction pass added point 2's third bullet
   in front of it without reconciling the two, leaving this report claiming both
   that a widened pointer takes the `OpConvertUToPtr` branch and that it reaches
   the shift below that branch. Adversary 02-2 found the contradiction; it is
   real and point 5 was the wrong half.

   `at_buffer` is `spirv_codegen.cpp:2194-2220`. The `u64` test is at `:2197` and
   its branch **returns** at `:2204`. The shift at `:2214-2216` and the
   `struct_array_access` at `:2217-2218` are reached only when that test is
   false. Both sub-cases were then opened:

   - **On a device with Int64**, `u64_type()` carries `dt == u64`, because
     `declare_primitive_type` sets `t.dt = dt` (`spirv_ir_builder.cpp:1536`). The
     `:2197` test is true, the function returns at `:2204`, and the shift never
     runs. Point 2's third bullet is the correct account.
   - **On a device without Int64**, `u64_type()` returns a default-constructed
     `SType` whose `dt` is a default-constructed `DataType`, which is
     `PrimitiveType::unknown` (`taichi/ir/type.cpp:20`) and is compared by
     pointer (`taichi/ir/type.h:97-99`). So the `:2197` test is **false** —
     and execution then reaches
     `TI_ERROR_IF(!is_integral(ptr_val.stype.dt), ...)` at `:2207-2210`, with
     `is_integral` (`taichi/ir/type_utils.h:103-112`) returning false for
     `unknown`. **The error fires.**

   This is a correction to a claim made twice elsewhere in this report and once
   in the paired report: that flipping `use_64bit_pointers` on a device without
   Int64 yields type id 0. That holds at the type-declaration site, but the first
   `at_buffer` use raises a **named `TI_ERROR`** and fails loudly. The
   kernel-signature path of §3.3 does not go through `at_buffer` at all, and so
   has no such guard. **The distinction is exactly what 6.2's stub note turns
   on: one 64-bit path carries an accidental guard, the other carries none.**
6. **`ArrayStride` cannot widen.** **[V]** `spirv_ir_builder.cpp:605` emits it
   as a decoration literal, which is a 32-bit word by the SPIR-V encoding. The
   `uint32_t nbytes` narrowing at `:591` (**corrected** from `:590`) is
   therefore constrained by the format, not just by the code.
7. **Struct-for is bounded by the ListGen buffer's element type**, u32
   (`spirv_codegen.cpp:2138-2168`), and by `get_global_invocation_id`, which is
   u32 by the SPIR-V builtin's own type. Widening the struct-for index means
   changing the ListGen buffer format — which is produced by the runtime, not by
   codegen. **Agent 03's seam.**
8. **`GlobalTemporaryStmt` must be reconciled, but it has no live consequence
   today.** **[V]** It is i32 on SPIR-V (`spirv_codegen.cpp:708-709`) and i64 on
   LLVM (`codegen_llvm.cpp:2386`). The two paths disagree for the same IR
   statement. Round one listed this alongside the `ExternalPtrStmt` and
   `make_pointer` truncations, implying comparable weight. It is not comparable:
   the offset is bounded by `taichi_global_tmp_buffer_size = 1024 * 1024`
   (`taichi/inc/constants.h:15`), and the bound is **asserted**, not implicit —
   `TI_ASSERT(global_offset_ < taichi_global_tmp_buffer_size);` at
   `taichi/transforms/offload.cpp:358`, inside `allocate_global` at `:344`.
   Every value the offset can hold is under 2^20. It becomes live only if that
   constant moves.
9. **`ExternalPtrStmt` loses more range on SPIR-V than on LLVM.** **[V]** SPIR-V
   converts the index to a **byte** offset while still 32-bit — the
   `OpShiftLeftLogical` at `spirv_codegen.cpp:773-777` — whereas LLVM keeps an
   element index and lets `GEP` scale it (`codegen_llvm.cpp:1988`). For 8-byte
   elements SPIR-V reaches three bits less far than LLVM for the same kernel.
   **[V]** When `spirv_has_no_integer_wrap_decoration` is present the offset is
   additionally decorated `DecorationNoSignedWrap` (`:778-781`), which makes
   overflow undefined rather than wrapping.
10. **No safety net at all.** **[V] CORRECTED.** There is no SPIR-V analogue of
    `check_func_call_signature`, and round one's "caught at best by the optional
    `spvtools` validator" is too generous: the validator is **explicitly
    disabled**. `spirv_opt_options_.set_run_validator(false);` at
    `spirv_codegen.cpp:2710`, unconditional. `enable_spv_opt`
    (`spirv_codegen.h:26`, set from
    `compile_config.external_optimization_level > 0` at
    `spirv/kernel_compiler.cpp:38`) gates only the pass registrations at
    `spirv_codegen.cpp:2683-2708`, which are optimisation rather than
    validation; `spirv_opt_->Run` at `:2746-2747` executes either way.
    `spirv_tools_` (`:2712`) is used only for `Disassemble` at `:2763`, inside
    `if constexpr (false)` at `:2758`. Width mismatches and the zero ids of
    §3.3.3 leave the codegen unexamined.
    **[V] Extended in the amendment pass.** "No safety net" is right for
    anything Taichi wrote, and it is not the whole outcome. An **accidental**
    net exists downstream of it: `spirv_opt_->Run` calls `BuildModule`
    regardless of the validator flag, so the optimiser's own parser rejects a
    zero `<id>` and the diagnostic reaches Taichi's message consumer — where the
    branch order at `:2646-2651` downgrades it to a warning and the flag
    recording it is never read outside a compiled-out block. §3.3.5a traces it.
    Two consequences for 6.2. First, the accidental net catches only what the
    **parser** rejects, which is structural: a zero id, a malformed operand. A
    width mismatch of the kind S6a-S6d would produce is a **validator**
    check (`external/SPIRV-Tools/source/val/validate_bitwise.cpp:88-91` for the
    three shifts, `:106-141` for the `OpBitwiseAnd`), and
    the validator is exactly what is switched off — so a widened pointer's
    **four** invalid instructions on the `SNodeLookupStmt` path would parse
    cleanly and pass through with no
    diagnostic at all. **"Two" corrected to "four" in the close-out pass; see
    §4.2.2.** Second, being caught is not the same as being stopped:
    the module ships either way at `:2775`.

### 4.3 Where the two paths diverge — consolidated

| Concern | LLVM | SPIR-V |
|---|---|---|
| Address representation | machine pointer, already 64-bit (`codegen_llvm.cpp:1864-1870`) | **u32 byte offset** into an SSBO (`spirv_codegen.cpp:82`, `:2322`) |
| SNode address arithmetic | via `GEP` / runtime `lookup_element` | explicit u32 `mul` at `spirv_codegen.cpp:507`, `add` at `:508`, index cast up to the pointer type at `:504-505` |
| `LinearizeStmt` | i32 (`codegen_llvm.cpp:1737`) | i32 (`spirv_codegen.cpp:533`) — **agree** |
| `GlobalTemporaryStmt` offset | **i64** (`codegen_llvm.cpp:2386`) | **i32** (`spirv_codegen.cpp:708`) — **disagree** |
| `ExternalPtrStmt` | element index, `GEP` scales (`:1988`) | byte offset computed in i32 (`:773-777`) — **disagree** |
| Struct-for index | i32 **signed**, `ICMP_SLT` (`codegen_llvm.cpp:2157`, `:2214-2216`) | u32 **unsigned**, `OpULessThan` (`spirv_codegen.cpp:2149`, `:2157-2158`) — **disagree on signedness too**. Width and signedness are separate decisions. |
| SNode ceiling | asserted (`struct_llvm.cpp:266`) | none (`snode_struct_compiler.h:38`) |
| Capability model | interface parameter present and discarded (`llvm/kernel_compiler.cpp:32`) | `DeviceCapabilityConfig`, threaded through (`spirv/kernel_compiler.cpp:37`, `spirv_ir_builder.h:598`) |
| Type lowering | one path, `get_data_type` | **two** paths — `get_primitive_type`, guarded (`spirv_ir_builder.cpp:288-332`); `Translate2Spirv`, unguarded (`spirv_types.cpp:393-431`) — **disagree with each other** |
| Mismatch detection | hard `TI_ERROR` at codegen (`llvm_codegen_utils.cpp:141-142`) | none written by Taichi; validator explicitly off (`spirv_codegen.cpp:2710`). Structural faults are caught accidentally by the optimiser's parser and downgraded to a `TI_WARN` (`:2646-2651`); **width** faults are a validator check and are caught by nothing (§4.2.10) |

---

## 5. Escalations

Items stopped on rather than resolved. Revised in the correction pass:
escalation 4 is now closed on the factual question and re-opened as a decision;
escalations 1, 3 and 6 are restated on firmer ground; three are new.

Revised again in the amendment pass: escalation 5 is closed at a third point
and gains the diagnostic-versus-correctness split that §3.3.5a settles;
escalation 7 is extended from Int64 to all five optional capabilities; **four
are new**, 13 to 16, and they are listed under their own heading below.
Escalations 1 to 12 keep their numbers.

Revised once more in the close-out pass: **one is new, 17**, under its own
heading below. Escalations 1 to 16 keep their numbers and their text.
**Seventeen in total, counted from the numbered list itself.**

Nothing here is a proposal. Each needs a decision the project owner has not made.

1. **What the SNode ceiling should bound.** §2.1. The assertion at
   `struct_llvm.cpp:266` bounds a per-tree count; the arrays at
   `runtime.cpp:567-569` are indexed by a monotonic global id that is never
   reclaimed, and the `element_lists` write at `:1003-1007` covers a contiguous
   range starting at that global id. The correction pass established that this
   is an out-of-bounds **store**, that it is reachable without many concurrent
   trees, and that the reachable configurations are the sparse ones this project
   requires. What it did **not** do is decide whether 6.1 means raising the
   array size, making the assertion bound the right quantity, or both. That is
   the planner's call, and it interacts with `kMaxNumSnodeTreesLlvm = 512`
   (`taichi/inc/constants.h:13`) and with territory 03's arrays.

2. **Whether `SNode::counter` should be reset per tree, or the arrays
   re-indexed.** Different fixes with different blast radii, and not this
   report's choice. `taichi/ir/snode.cpp:12`, `:220`;
   `taichi/program/program.cpp:144`; `taichi/ir/snode.h:348-350`. Note that
   `SNode::reset_counter()` is currently never called (§2.1), so one of the two
   mechanisms already in the source is dead. Whether it is a vestige to revive
   or to route around is a decision, and the standing instructions forbid
   proposing removal.

3. **`use_64bit_pointers` is a hardcoded `const bool`, and this report does not
   propose what it should become.** `spirv_codegen.cpp:82`. It is the single
   SPIR-V address-width decision, it does not consult `caps_`, and the branch it
   gates is admitted-unfinished (`:2319`). §4.2.2 now records that it is coupled
   to **two** capabilities rather than one, and that flipping it alone emits a
   module using a capability its own header never declares. What it should be
   driven by is a design decision under 6.2 and 5.1's stub-architecture
   requirement.

4. **Whether the `#if !defined(__APPLE__) && false` around
   `spirv_has_physical_storage_buffer` is current or stale.**
   `vulkan_device_creator.cpp:825-827`, setter at `:826`, upstream issue 6295
   cited in the comments at `:822-824`. Not determinable from the source. It
   decides whether the 64-bit SPIR-V branches are a starting point or a
   liability.

5. **What closes the `Translate2Spirv` capability bypass.** §3.3.3. The factual
   half of round one's escalation 4 is now **closed at all three points**:
   there is no earlier check anywhere. At the **front**, `Extension::data64` is
   declared, granted per-arch, and never queried (§3.3.5); no SPIR-V arch is
   granted it in any case. In the **middle**, the guarded `get_primitive_type`
   does run, at `spirv_codegen.cpp:2267` inside `get_buffer_value`, but twelve
   of its thirteen call sites hand it a hardcoded `PrimitiveType::i32` or
   `u32` placeholder (i32 at `:609-610`, `:618`, `:651`, `:728`, `:753`, `:788`,
   `:2024`, `:2039`, `:2293`; u32 at `:401`, `:516`, `:2138` — nine and three,
   enumerated in full in §3.3.5, **`:2024` and `:2039` added and the
   "Args- and Rets-buffer" label dropped in the close-out pass**), so it never
   sees the real argument type —
   **added in the amendment pass from the paired report; without it the closure
   rested on `Extension::data64` alone.** At the **back**, nothing validates the
   module, because the validator is explicitly disabled at
   `spirv_codegen.cpp:2710`. So a kernel with an `i64`, `u64`, `f64`, `f16`,
   `i8`, `u8`, `i16` or `u16` parameter on a device lacking the matching
   capability produces a SPIR-V module containing type id 0. **Not silently —
   the word this report used before the amendment pass was wrong.** §3.3.5a
   traces what happens instead: the optimiser's own parser rejects the id
   (`external/SPIRV-Tools/source/binary.cpp:473`), Taichi's message consumer
   downgrades the error to a `TI_WARN` by its branch order
   (`spirv_codegen.cpp:2646-2651`), a second `TI_WARN` fires at `:2745-2748`,
   and the unmodified invalid module is shipped at `:2775` because the `success`
   flag set at `:2750` is read only inside the compiled-out block at
   `:2758-2772`. Two warnings and no effect on the output.
   The **decision** is what to do about it, and there are at least three shapes:
   thread a capability into `Translate2Spirv`; check at the `translate_ti_type`
   call sites — `spirv_codegen.cpp:2344` and `:2358` in `compile_args_struct`,
   `:2420` and `:2436` in `compile_argpack_struct`, `:2494` in
   `compile_ret_struct`, the partition corrected in §3.3.4; or reject at the
   front of the pipeline. Which is right depends on where 6.2 places the
   capability boundary. Also unresolved: whether this is 6.2 work, territory 01
   work, or deferred — the eight calls sit in the SPIR-V type layer, not in the
   codegen proper. **What §3.3.5a adds to this decision** is that the failure is
   already observable in the log, so one shape of the answer is a diagnostic
   that names the type and the capability at `spirv_types.cpp:397-428`, where
   both are still in hand, rather than a correctness fix for a silent
   miscompile. The dead `success` flag is a separate item, escalation 14.

6. **Whether the C API capability overrides are intended.**
   `c_api/src/taichi_opengl_impl.cpp:9-12` sets `spirv_has_int64` with no guard
   and replaces the whole capability config via
   `set_caps` (`taichi/rhi/public_device.h:855-857`), discarding the `is_gles()`
   guard at `opengl_device.cpp:509-513`. `ti_set_runtime_capabilities_ext`
   (`c_api/src/taichi_core_impl.cpp:317-334`) allows any capability to be
   asserted from outside with no device query. Whether any supported
   configuration reaches either is not settled by the source, and both are
   outside `taichi/`.

7. **Whether Direct3D 11 and imported Vulkan are targets this project cares
   about.** `taichi/rhi/dx/dx_device.cpp:557-568` and
   `c_api/src/taichi_vulkan_impl.cpp:19-56` each set only `spirv_version`, and
   §3.3.7a establishes they lack **all five** optional capabilities, not just
   Int64. If they are out of scope the Int64-lacking target set shrinks to Metal
   pre-Apple3 and OpenGL ES; if in scope, the stub architecture must express
   four cases for Int64 and the full matrix in §3.3.7a for the rest. Section 5.2
   of the plan states its tiers in NVIDIA generations and, by its own text, that
   is incidental — so the tier table does not settle which SPIR-V consumers the
   portable path must express.

8. **SPIR-V struct-for `LoopIndexStmt` appears unhandled.**
   `spirv_codegen.cpp:543-563` (**span corrected** from `:541-561`) handles
   `OffloadedTaskType::range_for` and `RangeForStmt` and falls to
   `TI_NOT_IMPLEMENTED` at `:553` otherwise, including `struct_for` — yet
   `generate_struct_for_kernel` registers `"ii"` (`:2170-2171`). Part of the
   explanation is that `demote_dense_struct_fors` is forced true for every
   SPIR-V arch (`taichi/program/compile_config.cpp:72-74`, pass applied at
   `taichi/transforms/compile_to_offloads.cpp:191-192`), so dense struct-fors
   never reach it. Whether that fully accounts for it needs the lowering passes,
   outside this territory.

9. **The runtime seam is agent 03's, and it contradicts itself.** Recorded
   because it is the far end of this wire: `StructMeta::max_num_elements` is
   `i64` (`runtime.cpp:310`) and codegen sets it from an i64 constant
   (`codegen_llvm.cpp:281`), but the function-pointer field is declared
   `i32 (*get_num_elements)(Ptr, Ptr)` at `runtime.cpp:318` and
   `Dense_get_num_elements` returns the `i64` field through that `i32`
   (`node_dense.h:10-12`), narrowing it right back. Consequences not chased.

10. **`create_bit_ptr`'s `isIntegerTy(32)` assertion**
    (`codegen_llvm.cpp:1755`). A deliberate 32-bit contract on bit offsets
    within a physical type. Whether it needs to widen is not decided here; it is
    a real collision point with any blanket widening of the constant-emission
    path, and it fails loudly rather than silently.

11. **`taichi_max_num_indices = 12` is instruction count, not just a bound.**
    `struct_llvm.cpp:171-188` emits four arithmetic instructions and two calls
    per index per non-leaf SNode. Against 6.4's packing-density criterion this
    is a throughput question, not a correctness one, and it belongs with whoever
    decides the replacement values in 8.1.

12. **Whether `taichi_global_tmp_buffer_size = 1024 * 1024`
    (`taichi/inc/constants.h:15`) is also an installation factor under 6.1.**
    It is what currently makes the SPIR-V i32 `GlobalTemporaryStmt` offset safe,
    via the assertion at `taichi/transforms/offload.cpp:358` (§4.2.8). If it
    moves, the LLVM-versus-SPIR-V divergence at `spirv_codegen.cpp:708` becomes
    live. Nobody has said whether it moves.

**Four added in the amendment pass.**

13. **Whether this project will ever set `demote_dense_struct_fors` false.**
    §2.1, corrected. It is a public field
    (`taichi/program/compile_config.h:28`) defaulting true
    (`taichi/program/compile_config.cpp:18`). With it false, `all_dense` at
    `taichi/runtime/llvm/llvm_runtime_executor.cpp:402` is false unconditionally
    and the `element_lists` range write at
    `taichi/runtime/llvm/runtime_module/runtime.cpp:1003-1007` fires for a fully
    dense tree. It changes how urgent the correctness half of 6.1 is, and it is
    a configuration decision nobody has made. Not this report's to make.

14. **Whether the `success` flag at `spirv_codegen.cpp:2742-2775` is meant to
    gate anything.** §3.3.5a. It is declared at `:2742`, set false on optimiser
    failure at `:2750`, and read at exactly one place, `:2760`, inside
    `if constexpr (false)` at `:2758-2772`. The module is pushed at `:2775`
    either way. Whether the intent was to fall back to `task_res.spirv_code`, to
    fail the compile, or neither, is not determinable from the source. This is a
    correctness question **independent of 6.2** — it would still stand if the
    capability bypass were closed tomorrow — and the standing instructions
    forbid this report proposing a change.

15. **Whether Metal's absent `spirv_has_float64` is a target constraint or an
    oversight.** §3.3.7a. `taichi/rhi/metal/metal_device.mm` sets int8, int16
    and float16 unconditionally at `:1046-1048` and int64 under a family test at
    `:1051-1052`, and never mentions float64 anywhere in the file. Whether Metal
    genuinely cannot carry it, or whether the capability was simply never wired,
    decides whether 6.2's stub architecture must express a **permanent**
    Float64-less backend or a temporary one. Not determinable from this tree,
    and the answer changes the stub's shape rather than its size.

16. **Whether `spirv_opt_options_.set_run_validator(false)`
    (`spirv_codegen.cpp:2710`) is deliberate.** Recorded, not proposed. It is an
    unconditional, uncommented, unconfigurable choice. Turning it on would catch
    the id-0 modules of §3.3.3 at the point of production rather than as a
    downgraded warning from the optimiser's parser (§3.3.5a). Turning it on also
    costs validation time on every kernel compile, which bears directly on
    6.4's dispatch-throughput criterion. That trade is the planner's; this
    report records only that nothing validates today, and that §3.3.5a
    establishes what happens instead.

#### Escalation new in the close-out pass

17. **Where the `bitmasked_activation` index should be cast, and whether its
    two callers should be made to agree.** S6c, S6d, S6e, §4.2.2. The two
    callers of `bitmasked_activation` differ: `visit(SNodeOpStmt*)` casts the
    index to the pointer type at `spirv_codegen.cpp:443-444`,
    `visit(SNodeLookupStmt*)` passes it raw at `:488-489`, and that second
    function casts the same value at `:504-505` for its other branch. Under a
    widened `make_pointer` the uncast path makes `:392-394` and `:395-397`
    invalid on top of `:398-399` and `:410-412`.

    Three routes exist and this report picks none of them: cast at the call
    site; give the `input_index` parameter a declared type rather than taking
    whatever the caller registered; or change `LinearizeStmt`'s output width
    (S7), which is territory 01's ground and not this report's to touch. The
    third would reach well beyond this function. **Standing instruction §10.3
    forbids me deciding that the raw pass at `:488-489` is a defect to remove
    rather than an intentional difference between the two paths**, and nothing
    in the source says which it is — there is no comment at either call site.
    The asymmetry is recorded as a fact, graded architectural, with a blast
    radius of five places in one file. What to do about it is a decision.
---

## 6. Verified versus inferred — explicit index

Rewritten in the correction pass, extended in the amendment pass. Everything
cited in this report has now been opened at least twice: once in round one, once
line by line and checked for attribution in the correction pass, and every item
touched by the amendment pass a third time. §3.4 lists what moved in the
correction pass; the header lists what moved in the amendment pass.

**Verified in the amendment pass specifically, each by opening the named file:**
the three `ir_translate_to_spirv` call sites and the enclosing function
boundaries that fix their attribution (`spirv_codegen.cpp:2326`, `:2400`,
`:2402`, `:2481`, `:2483`, `:2527`, calls at `:2386`, `:2464`, `:2511`); the
second widening-invalidity site at `:398-399` and `make_value`'s Result-Type
position (`spirv_ir_builder.h:290-298`) and `const_i32_one_`'s permanent i32
(`spirv_ir_builder.cpp:224`); the validator's shift rule as **bit width, not
signedness** (`external/SPIRV-Tools/source/val/validate_bitwise.cpp:65-104`,
width test `:88-91`, shift test `:93-102`); the four-quantity enumeration behind
the count 8 / 8 / 5 / 3 (§3.3.3); every link of the id-0 chain in §3.3.5a,
including `Optimizer::Run` (`optimizer.cpp:584-598`, `clear()` at `:642`),
`BuildModule` (`build_module.cpp:68-69`, `:74`), `spvBinaryParse`
(`binary.cpp:450`, `:456`, `:473`), `DiagnosticStream::~DiagnosticStream`
(`diagnostic.cpp:87-114`, level initialiser `:89`, `default:` arm `:106-107`,
dispatch `:112`), the message-level enum (`libspirv.h:83-92`) and the consumer's
branch order (`spirv_codegen.cpp:2646-2651`); `at_buffer`'s early return at
`:2204` and its `TI_ERROR_IF` at `:2207-2210`, with `DataType()`'s default
(`taichi/ir/type.cpp:20`), pointer comparison (`taichi/ir/type.h:97-99`) and
`is_integral` (`taichi/ir/type_utils.h:103-113`); all eight span corrections in
the §3.4.1 table; the complete five-capability setter matrix in §3.3.7a,
re-derived by tree-wide grep and then opened site by site, including that
`metal_device.mm` contains no occurrence of `float64` and that
`opengl_device.cpp` never sets `spirv_has_int8`; the `all_dense` seeding at
`llvm_runtime_executor.cpp:402` and the clear-only loop at `:403-410`, with
`is_gc_able` (`taichi/ir/snode_types.cpp:21-23`) and
`demote_dense_struct_fors`'s declaration (`compile_config.h:28`) and default
(`compile_config.cpp:18`); the `get_buffer_value` placeholder call sites; and
`VulkanRuntimeImported::Workaround::Workaround`
(`c_api/src/taichi_vulkan_impl.cpp:19-56`, declared
`c_api/src/taichi_vulkan_impl.h:27-31`).

**Verified in the close-out pass specifically, each by opening the named file:**
`bitmasked_activation`'s parameter list (`spirv_codegen.cpp:383-387`) and all
four widening-invalidity sites (`:392-394`, `:395-397`, `:398-399`, `:410-412`)
with the Result Type and the non-conforming operand identified in each; the
`OpBitwiseAnd` validation arm as **distinct from and stricter than** the shift
arm (`validate_bitwise.cpp:106-141`, operand loop `:118-119`, width test
`:134-138`); both callers of `bitmasked_activation` and their disagreement over
casting (`visit(SNodeOpStmt*)` `:437-466` with its cast at `:443-444`;
`visit(SNodeLookupStmt*)` `:468-511` with its uncast pass at `:488-491` and its
cast of the same value at `:504-505`); `visit(LinearizeStmt*)` at `:532-541` as
the i32 source of that index; every hit of
`grep -n get_buffer_value spirv_codegen.cpp` opened individually, including the
two previously omitted at `:2024` and `:2039` and the placeholder comment at
`:650`; the three function spans closed here — `translate_ti_type`
`spirv_types.cpp:484-514`, `IRBuilder::init_pre_defs`
`spirv_ir_builder.cpp:149-225`, `translate_ti_primitive`
`spirv_types.cpp:167-214` — each by printing its closing line;
`check_func_call_signature` `llvm/llvm_codegen_utils.cpp:103-145` and
`KernelCodeGen::create` `codegen.cpp:37-74`, the two files the §6 reconciliation
sentence failed to account for; and the 45-file partition itself, re-derived by
diffing the transcribed list against `find` output rather than by eye.

**Where an adversary was wrong and this report changed nothing.** Adversary
02-1's §2.1 point 4, that the id-0 failure is "deferred to the driver at shader
module creation" with "no Taichi-level diagnostic", and its §6 item 3 asking for
a sentence saying so. §3.3.5a traces the chain and the failure is caught
in-process. The sentence was not added. Adversary 02-1's §2.2 supporting
sentence, "Five is the count of optional *types* (int8, int16, int64, fp16,
fp64)", is a miscount of the same species it was correcting; §3.3.3 enumerates
eight optional types under those five **capability** names. Adversary 02-1's
§2.6 credit to this report on the sparsity conclusion was also misplaced — this
report had the finding right and the conclusion wrong, and §2.1 now corrects it.

**Where a round-three adversary was short, and this report goes further.** Both
`adversary3-02-1.md` §7.3 and `adversary3-02-2.md` §10 put the §6 sweep union at
**44** and name `llvm/llvm_codegen_utils.cpp` as the single uncovered file. Both
are right that the sentence does not close, and both are wrong about by how
much: the union is **43**, and the second uncovered file is the top-level
`taichi/codegen/codegen.cpp`, covered once in this report in §2.3 and
nowhere in §3.1-§3.3. Each appears to have stopped at the first miss rather than
testing all twenty-one remainder files. The finding is theirs; the arithmetic is
corrected in §6, mechanically. Separately, on the S6c/S6d addition the two
adversaries do **not** agree: `adversary3-02-2.md` §8 has it in full, including
the caller mechanism, while `adversary3-02-1.md` does not raise it at all — the
string `bitmasked` does not occur in that file. This report adopts 3-2's
finding, having opened every line of it at source, and records that 3-1 missed
it.

**Verified by reading the named code:** every entry in the L-table (§3.1) and
the S-table (§3.2), including the new S17; the `get_constant` width dispatch
(§3.0); every part of §3.3 — the capability declarations, the guarded
`get_primitive_type` path, all eight bypassed accessors and their five
capability gates, the three live `ir_translate_to_spirv` call sites and their
enclosing functions (attribution corrected in the amendment pass), the absent
`Extension::data64` guard, the disabled validator, the four `spirv_has_int64`
setters with their guards, the three further in-tree configurations that set
none, and that `spirv_has_physical_storage_buffer` has exactly one setter which
is compiled out; the single-site status of both named constants (§2.1, §2.2 —
four sites for `taichi_max_num_snodes`, two in codegen for
`taichi_max_num_indices`, both by grep); every link in the
per-tree-vs-global-id chain, the range write, and the tree-id-recycling and
`all_dense`/`is_gc_able` facts that establish reachability (§2.1); that
`TI_ASSERT` is unconditional; that `SNode::reset_counter()` is never called;
the `check_func_call_signature` safety net and its true error line (§4.1.9);
the divergence table (§4.3); the SPMD intrinsic widths (§4.1.6); the capability
plumbing into the SPIR-V codegen and its discarded LLVM counterpart (§2.3,
§4.2.3); the `at_buffer` branch condition and `declare_primitive_type`'s
`t.dt = dt` (§4.2.2); the `taichi_global_tmp_buffer_size` assertion (§4.2.8);
the `LinearizeStmt` stride producers in `taichi/transforms/` (§4.1.2).

**Inferred, and marked as such in the text:**

- the cycle-count arithmetic in §2.1 — roughly 340 create/destroy cycles of a
  three-SNode tree to carry `root_id` past 1024. Every input to it is verified;
  no failing case was built;
- that `use_64bit_pointers` would not take effect even on a fully capable device
  (§3.3.8 — follows from `make_pointer` never reading `caps_`);
- the change-impact lists in §4.1 and §4.2, except the sub-points individually
  marked **[V]**.

**No longer inferred.** Round one marked as **[I]** that a program could exceed
the runtime array bound while passing the assertion. The mechanism is now
verified (§2.1) and only the arithmetic above remains an inference.

**Files in this territory with nothing width- or index-relevant**, swept and
recorded so coverage is not in doubt. Corrected in the correction pass: round
one's list omitted fourteen files, one of which turned out to carry a real
narrowing (§3.4.4, S17). Full list of files opened and found irrelevant:
`codegen/codegen_utils.h` (printf specifier parsing only);
`codegen/codegen.h`; `codegen/compiled_kernel_data.{h,cpp}`;
`codegen/kernel_compiler.h`; `llvm/codegen_llvm.h`;
`llvm/llvm_codegen_utils.h`; `llvm/struct_llvm.h`;
`llvm/compiled_kernel_data.{h,cpp}`; `llvm/kernel_compiler.h`;
`llvm/llvm_compiled_data.h`; `spirv/compiled_kernel_data.{h,cpp}`;
`spirv/kernel_compiler.h`; `spirv/kernel_utils.cpp`;
`spirv/lib_tiny_ir.h` (sizes are `size_t` throughout);
`dx12/dx12_llvm_passes.h`; `dx12/dx12_lower_runtime_context.cpp`;
`dx12/dx12_global_optimize_module.cpp`; and the four per-backend
`codegen_*.h` headers (`amdgpu/codegen_amdgpu.h`, `cpu/codegen_cpu.h`,
`cuda/codegen_cuda.h`, `dx12/codegen_dx12.h`).
`spirv/kernel_utils.h` is **not** in this list — it is covered by S17.

**The reconciliation, re-derived mechanically in the close-out pass. The
sentence that stood here was wrong, and by more than round three said.** It
read: *"That, plus every file cited in §3.1, §3.2 and §3.3, accounts for all 45
`.h`/`.cpp` files under `taichi/codegen/`."* Both round-three adversaries tested
it and both found the union to be 44, naming `llvm/llvm_codegen_utils.cpp` as
the one file covered outside the three sections. **The union is 43. There are
two such files, not one.** I transcribed the list above into a file, diffed it
against `find taichi/codegen -type f \( -name '*.h' -o -name '*.cpp' \)`, and
tested each remaining basename against the span the three named sections
actually occupy. **Report line numbers below are as of this pass**, which added
a header block and so shifted every one of them: `### 3.1` now opens at report
`:419` and `### 3.4` at `:973`, so the window is **419-972**. Round three
quoted the pre-shift figures, 364 and 895, and tested the same text.

| Check | Result |
|---|---|
| `.h`/`.cpp` files on disk under `taichi/codegen/` | 45 |
| Entries in the irrelevant list above, once `compiled_kernel_data.{h,cpp}` is split and the four `codegen_*.h` headers are expanded | 24 |
| Distinct, and all present on disk | 24, yes |
| Remainder | 21 |
| Of the remainder, cited somewhere in report lines 419-972 | 19 |
| Of the remainder, **not** cited there | **2** |

The two are `llvm/llvm_codegen_utils.cpp` and the top-level
`taichi/codegen/codegen.cpp`. Both are opened and both carry a stated finding,
so the **coverage** is complete; what was defective was the statement of how it
closes.

- `llvm/llvm_codegen_utils.cpp` — `check_func_call_signature`, source
  `:103-145`, `TI_ERROR` at `:141-142`. Covered in §3.4's citation-audit table,
  in §4.1.9 and in §4.3's divergence table (report `:1001`, `:1182`, `:1409`).
  Both function bounds printed.
- `taichi/codegen/codegen.cpp` — `KernelCodeGen::create`, source `:37-74`, the
  compile-time `#if defined(TI_WITH_CUDA)` / `TI_WITH_DX12` / `TI_WITH_AMDGPU`
  chain around a runtime `arch` switch. Covered once, in §2.3, the item 6.3 seam
  (report `:380`). **Neither adversary found this one; both stopped at the first
  miss.** Function bounds printed: `:37` opens, `:74` closes.

**Corrected statement.** The 24 files above, plus every file cited in §2.3,
§3.1, §3.2, §3.3 and §4.1.9, account for all 45 `.h`/`.cpp` files under
`taichi/codegen/`: 24 + 19 + 2 = 45. The remaining six files in that tree are
the per-directory `CMakeLists.txt`, one each for `amdgpu`, `cpu`, `cuda`,
`dx12`, `llvm` and `spirv`, giving 51 files in total.

**Deliberately not concluded**, per the standing instructions. Nothing has been
decided unnecessary; no removal, cleanup, fix or abstraction is proposed. In
particular this report does **not** conclude:

- that `use_64bit_pointers` should be wired to a capability;
- that the dead physical-storage-buffer guard should be re-enabled;
- that the per-tree assertion is a bug, or which quantity should replace 1024;
- that `Translate2Spirv` should take a capability, or which of the three shapes
  in escalation 5 is right;
- that `SNode::reset_counter()` should be called or deleted;
- that `spirv_opt_options_.set_run_validator(false)` should be flipped;
- that the C API capability overrides are defects;
- that the dead `success` flag at `spirv_codegen.cpp:2750` is a defect to fix,
  or what it was meant to gate (escalation 14);
- that Metal's absent `spirv_has_float64` is an oversight rather than a
  constraint (escalation 15);
- that `demote_dense_struct_fors` should or should not be set false
  (escalation 13);
- which of S6a, S6b, S6c and S6d a widening repair should address, or how;
- whether S6c and S6d should be closed at the call site (`:488-489`) or by
  typing `bitmasked_activation`'s `input_index` parameter, or whether the two
  callers should be made to agree some third way (S6e).

All of those are §5, unresolved.
