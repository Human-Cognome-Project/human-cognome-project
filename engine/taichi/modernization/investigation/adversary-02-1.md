# Adversary 02-1 — judgement on report-02-codegen.md and report-02b-codegen.md

Adversary 1 of 2, territory 02 (codegen). Every claim below was checked by
opening the named file at the named line. Where I correct a report I give the
line I read.

---

## 0. Verdicts, stated first

| Question | Verdict |
|---|---|
| Is the work CORRECT? | **Substantially yes, with one material error in pass B, one material error and several citation errors in pass A.** Neither error invalidates the body of either report. |
| Is the work COMPLETE? | **No.** Both passes share three blind spots, one of which changes the answer to the brief's 6.2 question. |
| Is major revision required? | **No for pass A. No for pass B.** Both need targeted correction, not rewriting. The gaps in §4 below are additions, not repairs. Detail in §7. |

The single most consequential finding in this analysis is not in either report:
**there are at least three in-tree SPIR-V targets that lack Int64, not one**,
and the SPIR-V type translator bypasses the capability guard for **six**
optional widths, not two.

---

## 1. The known contradiction, resolved

**Pass A is right. Pass B is wrong. They are not describing different call
paths in any way that rescues B's claim — B never looked at the path A found.**

### 1.1 What pass B got right

`IRBuilder::get_primitive_type` (`taichi/codegen/spirv/spirv_ir_builder.cpp:288-332`)
does guard every optional width:

- i64 at `:311-313`, `TI_ERROR("Type {} not supported.", ...)` at `:312-313`.
- u64 at `:325-328`, `TI_ERROR` at `:326-327`.

Pass B's line numbers here are the more accurate of the two. Pass A cited the
u64 guard as `:324-327`; the actual `else if` opens at `:325` and the error is
at `:326-327`. Trivial, but B is exact and A is not.

The capability declaration (`spirv_ir_builder.cpp:64-66`) and the conditional
type declaration (`:166-169`) are exactly as both reports describe. `t_int64_`
and `t_uint64_` are left default-constructed when the capability is absent.

### 1.2 What pass B got wrong

B's report says, verbatim, "**Checked at every use.**" That is false.

`Translate2Spirv::visit_int_type`
(`taichi/codegen/spirv/spirv_types.cpp:393-419`) does not call
`get_primitive_type`. It calls the bare accessors directly:

- `spir_builder_->i64_type()` at `spirv_types.cpp:403`
- `spir_builder_->u64_type()` at `spirv_types.cpp:415`

Those accessors are `spirv_ir_builder.h:529-531` and `:532-534`. Each is a
one-line `return t_int64_;` / `return t_uint64_;` with no `caps_` consultation
of any kind. The result is written out at `spirv_types.cpp:418`
(`ir_node_2_spv_value[type] = vt.id;`), and `SType::id` is declared
`uint32_t id{0}` at `spirv_ir_builder.h:51`. Id 0 is not a valid SPIR-V result
id. There is no `TI_ASSERT` or `TI_ERROR` anywhere in `Translate2Spirv`
guarding the id (checked: the only assertions in `spirv_types.cpp` are at
`:17`, `:47`, `:71`, `:336`, `:346`, `:356`, `:365`, none of them on `vt.id`).

Pass A's characterisation of this site is accurate in every particular I
checked, including the `id{0}` claim and the `:393-419` span.

### 1.3 The path is live. Pass A is right about that too.

`Translate2Spirv` is reached through `ir_translate_to_spirv`
(`spirv_types.cpp:476-481`), called at three sites, all in live kernel
compilation:

- `spirv_codegen.cpp:2386` — `compile_args_struct`, whose element types are
  built by `translate_ti_type` at `:2344` and `:2358`.
- `spirv_codegen.cpp:2464` — the rets struct.
- `spirv_codegen.cpp:2511` — the argpack struct.

The type feed is `translate_ti_primitive` (`spirv_types.cpp:167-214`), which
maps `PrimitiveType::i64` to `IntType(64, signed)` at `:179-181` and
`PrimitiveType::u64` to `IntType(64, unsigned)` at `:199-201`, with no
capability parameter in its signature at all.

So: a kernel taking an `i64` argument, compiled for a device that does not
report `spirv_has_int64`, emits an `OpTypeStruct` referencing result id 0
rather than raising the named error. Pass A's escalation 4 is correct and
remains open.

`ir_reduce_types` at `snode_struct_compiler.cpp:23` is inside the commented-out
block at `:16-19`/`:22-28`, so the *SNode* tinyir path is indeed dead — pass A
said this and it is right — but that is a different call site from the three
above, which are live.

### 1.4 The defect is wider than pass A said

Pass A framed this as an Int64 problem. It is not. The same function bypasses
the same guards for every optional width:

| `spirv_types.cpp` line | Bare accessor | Guarded equivalent in `get_primitive_type` |
|---|---|---|
| `:397` | `i8_type()` | `spirv_ir_builder.cpp:301-304` |
| `:399` | `i16_type()` | `:305-308` |
| `:403` | `i64_type()` | `:311-314` |
| `:409` | `u8_type()` | `:315-318` |
| `:411` | `u16_type()` | `:319-322` |
| `:415` | `u64_type()` | `:325-328` |
| `:424` (`visit_float_type`) | `f16_type()` | `:291-294` |
| `:428` (`visit_float_type`) | `f64_type()` | `:297-300` |

The corresponding `t_*` members are declared only under their capabilities at
`spirv_ir_builder.cpp:156-158` (int8), `:160-163` (int16), `:166-169` (int64),
`:171-173` (fp16), `:174-176` (fp64). Both reports missed the float half and
the 8/16-bit half entirely.

### 1.5 Bearing on section 6.2

The brief says the architecture must be able to express a target lacking Int64.
Pass B concluded "the mechanism the brief asks for exists". That conclusion is
**half true and stated too strongly**. The mechanism exists on the
`get_primitive_type` path and is absent on the tinyir type-translation path
that builds the kernel argument, return and argpack structs. Any 6.2 work that
relies on "it already fails loudly" will be relying on a guarantee that does
not hold for kernel signatures.

---

## 2. The `struct_llvm.cpp:266` assertion — verified independently, and it is
   reachable

Both passes claim the assertion bounds a per-tree count while the runtime
arrays are indexed by a process-global SNode id. **Both are right, on every
link.** I re-read the chain rather than taking either report's word:

1. `TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);` at
   `taichi/codegen/llvm/struct_llvm.cpp:266`, inside
   `StructCompilerLLVM::run(SNode &root)` which opens at `:247`.
2. `snodes` is `StructCompiler::snodes`, `std::vector<SNode *>`
   (`taichi/struct/struct.h:11`), filled only by `collect_snodes`
   (`taichi/struct/struct.cpp:7-12`), which recurses from the single root
   passed in.
3. A fresh `StructCompilerLLVM` is constructed per tree:
   `taichi/runtime/program_impls/llvm/llvm_program.cpp:51-52`, then
   `struct_compiler->run(*root)` at `:53`.
4. `taichi_max_num_snodes = 1024` at `taichi/inc/constants.h:12`, sizing
   `element_lists`, `node_allocators`, `ambient_elements` at
   `taichi/runtime/llvm/runtime_module/runtime.cpp:567-569`.
5. `SNode::id` comes from `std::atomic<int> SNode::counter{0}`
   (`taichi/ir/snode.cpp:12`), assigned `id = counter++` at
   `taichi/ir/snode.cpp:220`, reset only at `taichi/program/program.cpp:144`.

Grepped the whole tree: `taichi_max_num_snodes` occurs at exactly four sites —
`constants.h:12`, `struct_llvm.cpp:266`, `runtime.cpp:567`, `:568`, `:569`.
There is no compensating check anywhere. Pass A's escalation 1 said it had not
searched outside `taichi/codegen/`; I have, and there is nothing.

### 2.1 What both passes missed — this is a write, not a lookup

Neither report opened `runtime_initialize_snodes`. It contains

```
  for (int i = root_id; i < root_id + num_snodes; i++) {
    // TODO: some SNodes do not actually need an element list.
    runtime->element_lists[i] =
        runtime->create<ListManager>(runtime, sizeof(Element), 1024 * 64);
  }
```

at `taichi/runtime/llvm/runtime_module/runtime.cpp:1003-1006`. `root_id` and
`num_snodes` are parameters (`:988-989`), supplied from
`field_cache_data.root_id` (`llvm_runtime_executor.cpp:399`) and
`(int)snode_metas.size()` (`:443`). So the array is written over a **contiguous
range starting at the tree root's global id**. Once `root_id + num_snodes`
exceeds 1024 this is an out-of-bounds *store* into the `LLVMRuntime` struct,
corrupting `temporaries`, `rand_states` and whatever follows
(`runtime.cpp:570-571`). The same applies to
`runtime->node_allocators[snode_id]` at `:1029` and
`runtime->ambient_elements[snode_id]` at `:1038`, both called per SNode from
`llvm_runtime_executor.cpp:460-466`.

Both reports treated this as a bound that might be exceeded. It is a bound
whose violation writes past the end of a struct on the device.

### 2.2 Reachability — established, and easier than either report implied

Pass A said "a program with several SNode trees can pass the per-tree assertion
while the global id exceeds the array bound" and marked it **[I]**, no failing
case built. Pass B said the same and stopped. Both framed it as needing many
simultaneous trees, bounded by `kMaxNumSnodeTreesLlvm = 512`
(`constants.h:13`).

That framing understates it. SNode tree **ids are recycled** —
`Program::destroy_snode_tree` pushes the id onto `free_snode_tree_ids_`
(`taichi/program/program.cpp:235`, popped at `:563-564`) — but **SNode ids are
not**. `SNode::counter` only ever increments (`taichi/ir/snode.cpp:220`) and is
reset only in the `Program` constructor (`taichi/program/program.cpp:144`).

So reaching the overflow does not require 512 live trees. It requires a process
that creates and destroys a few hundred small SNode trees over its lifetime
while never holding more than a handful at once. A tree of root + dense + place
consumes three ids, so roughly 342 create/destroy cycles put the next tree's
`root_id` past 1024. Nothing asserts, nothing warns, and the first symptom is
memory corruption on the device.

**This is a live defect, not a theoretical one.** Both reports were right to
flag it and both stopped one step short of showing why it matters.

I do not propose what should bound what. Per the standing instructions and
brief item 8.1.2 that is the planner's call; it goes to §8 below.

---

## 3. Every other place the two reports disagree, resolved

| # | Subject | Pass A | Pass B | Source | Who was right |
|---|---|---|---|---|---|
| 1 | LLVM `KernelCompiler` and `DeviceCapabilityConfig` | "The LLVM path has no analogue" (§2.3) | Takes `device_caps` at `llvm/kernel_compiler.cpp:31` and never uses it | Parameter declared at `taichi/codegen/llvm/kernel_compiler.cpp:32`; `KernelCodeGen::create` is called at `:36-37` with `compile_config, &kernel_def, &chi_ir, *config_.tlctx` and no caps | **B.** The plumbing point exists and is discarded, which is materially different from "no analogue" and matters for 6.3. B's line is off by one (32, not 31). |
| 2 | `bitmasked_activation` and pointer width | S6: "shifts/masks in `ptr_dt` (u32)" | Hardcodes `u32_type()` independently of `make_pointer`, so bitmasked SNodes stay u32 even if the pointer widened | `spirv_codegen.cpp:410` forces result type `u32_type()`, `:411` the immediate, `:413` `struct_array_access(u32_type(), ...)`, all regardless of `ptr_dt` | **B on substance, A on line numbers.** B cited "405, 411-412, 414"; the actual hardcoded sites are `:404`, `:410`, `:411`, `:413`. A's description is incomplete: it would leave a reader believing widening `make_pointer` propagates here, which B correctly says it does not. |
| 3 | `AxisExtractor` field lines | `taichi/ir/snode.h:40,44,49` | `taichi/ir/snode.h:41,45,49` | `num_elements_from_root` at `:41`, `shape` at `:45`, `acc_shape` at `:49` | **B.** A is off by one on two of three. |
| 4 | Refine-coordinates third parameter | `struct_llvm.cpp:153` | `struct_llvm.cpp:152` | `llvm::Type::getInt32Ty(*llvm_ctx_)` is at `:153` | **A.** |
| 5 | `list_element_size` narrowing | `codegen_llvm.cpp:2291-2294` | `codegen_llvm.cpp:2290-2291` | statement spans `:2290-2291`; `:2292-2293` is `num_splits` | **B.** A's span silently absorbs an unrelated statement. A's follow-on claim that it is emitted as i32 at `:2309` is correct. |
| 6 | `GlobalTemporaryStmt` on LLVM | `:2386` | `:2385` | `tlctx->get_constant((int64)stmt->offset)` at `:2386` | **A.** |
| 7 | `StructMeta::max_num_elements` | `runtime.cpp:310` | `runtime.cpp:307` | field at `:310`; `:307` is `struct StructMeta {` | **A.** |
| 8 | Vulkan Int64 setter | `vulkan_device_creator.cpp:628-631` | `:630-633` | `if (device_supported_features.shaderInt64) {` at `:630`, `caps.set` at `:632` | **B.** |
| 9 | Metal Int64 setter | `metal_device.mm:1052`, **"unconditional"** | not mentioned | `caps.set(...spirv_has_int64, 1)` at `:1052` is inside `if (feature_64_bit_integer_math)` at `:1051`, and `feature_64_bit_integer_math = family_apple3` at `:1038` | **Neither.** A's "unconditional" is **factually wrong**. See §4.1 — the correction strengthens the brief's 6.2 case rather than weakening it. |
| 10 | Where `spirv_has_physical_storage_buffer` is set | "one setter, `:826`, inside `#if` at `:823-827`" | "one setter `:826`, inside `#if !defined(__APPLE__) && false`", `shaderInt64` check at `:823` | Setter at `:826`; the `#if` is at `:825` and `#endif` at `:827`; the `shaderInt64` check is at `:821`, `:823` is a comment | **Both wrong on framing lines, both right on substance.** The single-setter claim is correct; I re-grepped. |
| 11 | SPIR-V `ExternalPtrStmt` byte-offset shift | `:774-777` | `:772-776`, decoration at `:777-781` | `OpShiftLeftLogical` spans `:772-776`; the `NoSignedWrap` block is `:777-780` | **B.** |
| 12 | Range-for `num_elems` narrowing | not mentioned | `spirv_codegen.cpp:1999`, a `size_t` difference narrowed to `int` | `const int num_elems = range_for_attribs.end - range_for_attribs.begin;` at `:1999`; both fields are `size_t` at `taichi/codegen/spirv/kernel_utils.h:110-111` | **B.** A missed it. B's file path is written as bare `kernel_utils.h`; the file is `taichi/codegen/spirv/kernel_utils.h`, and there is no `taichi/program/kernel_utils.h`. |
| 13 | `Dense_get_num_elements` narrowing | escalation 7: returns i32, narrowing an i64 | not mentioned | `i32 Dense_get_num_elements(...)` at `taichi/runtime/llvm/runtime_module/node_dense.h:10-12`, returning `StructMeta::max_num_elements` which is `i64` at `runtime.cpp:310`; the function-pointer field is `i32 (*get_num_elements)(Ptr, Ptr)` at `runtime.cpp:318` | **A.** B missed a genuine truncation on the far end of the same wire. |
| 14 | SPIR-V struct-for `LoopIndexStmt` | escalation 5: appears unhandled, did not trace | escalation 4: same, plus `demote_dense_struct_fors` forced true for SPIR-V | `visit(LoopIndexStmt*)` `TI_NOT_IMPLEMENTED`s for non-`range_for` offloads at `spirv_codegen.cpp:552-554`; `CompileConfig::fit` sets `demote_dense_struct_fors = true` when `arch_uses_spirv(arch)` at `taichi/program/compile_config.cpp:72-74`; pass applied at `taichi/transforms/compile_to_offloads.cpp:191-192` | **B.** Same observation, B carried it one step further and partially explains the apparent gap. |
| 15 | `BlockCornerIndexStmt` under widening | quotes the comment only | derives its element type from the struct and follows automatically, unlike `LoopIndexStmt` | `val_ty_as_array->getElementType()` at `codegen_llvm.cpp:2374`; `LoopIndexStmt` hardcodes `getInt32Ty` at `:2336` and `:2339` | **B.** A useful distinction A did not make. |

Neither report contains a disagreement I could not resolve against the source.

---

## 4. What both reports missed

### 4.1 There are at least three in-tree SPIR-V targets without Int64, not one

This is the most important omission, because the brief's 6.2 note turns on
exactly this question.

Both reports treat OpenGL ES as *the* in-tree target lacking Int64. Pass A
states `spirv_has_int64` "is set in three places" and names Vulkan, OpenGL and
Metal. That enumeration is wrong twice over.

A tree-wide grep for `spirv_has_int64` setters gives:

| Setter | Guard | Verdict |
|---|---|---|
| `taichi/rhi/vulkan/vulkan_device_creator.cpp:632` | `device_supported_features.shaderInt64` (`:630`) | queried |
| `taichi/rhi/opengl/opengl_device.cpp:511` | `if (!is_gles())` (`:509`) | profile-based, no feature query |
| `taichi/rhi/metal/metal_device.mm:1052` | `if (feature_64_bit_integer_math)` (`:1051`), `= family_apple3` (`:1038`) | **queried — A's "unconditional" is wrong** |
| `c_api/src/taichi_opengl_impl.cpp:9` | **none** | **missed by both** |
| `taichi/rhi/dx/dx_device.cpp:562-566` | never set — only `spirv_version` | **missed by both** |

Consequences:

- **Metal on pre-Apple3 GPUs** is a second in-tree SPIR-V target with no Int64.
  A said the opposite.
- **Direct3D 11** (`Dx11Device::Dx11Device`, `taichi/rhi/dx/dx_device.cpp:557-568`)
  sets only `spirv_version` at `:564`. It is a third target with no Int64, and
  neither report noticed it exists.
- **The C API OpenGL runtime sets Int64 unconditionally.**
  `OpenglRuntime::OpenglRuntime` (`c_api/src/taichi_opengl_impl.cpp:4-13`)
  builds a fresh `DeviceCapabilityConfig`, sets `spirv_has_int64` at `:9` and
  `spirv_has_float64` at `:10`, then calls `get_gl().set_caps(std::move(caps))`
  at `:12`. `set_caps` is a whole-object replacement —
  `caps_ = std::move(caps);` at `taichi/rhi/public_device.h:855-857` — so it
  **discards** the `is_gles()` guard that `GLDevice::GLDevice` applied at
  `taichi/rhi/opengl/opengl_device.cpp:509-513`. The entry point that leads
  here, `ti_import_opengl_runtime`, takes a `bool use_gles` parameter
  (`c_api/src/taichi_opengl_impl.cpp:20-28`) and forwards it to
  `set_gles_override` at `:25` before constructing the runtime at `:27`. So on
  the C API path, GLES claims Int64.

  I have not traced whether any shipped configuration exercises that path, and
  I am not asserting a live failure. What I am asserting is that the "one
  target lacks it, guarded by profile" picture both reports painted is not what
  the source says.

For 6.2's stub-architecture requirement this changes the shape of the problem.
The architecture is not being asked to accommodate one awkward legacy profile.
Three of the four SPIR-V-consuming device backends can present without Int64,
and one of them affirms it without checking.

### 4.2 The SPIR-V `GlobalTemporaryStmt` divergence has no current consequence

Both reports flag as a headline divergence that `GlobalTemporaryStmt` is i64 on
LLVM (`codegen_llvm.cpp:2386`) and i32 on SPIR-V (`spirv_codegen.cpp:708`).
Both are correct that the widths differ. Neither noticed that
`taichi_global_tmp_buffer_size = 1024 * 1024` (`taichi/inc/constants.h:15`),
so every value that offset can hold today is under 2^20. The i32 is not a live
truncation and will not become one until that constant moves.

This matters for prioritisation. Both reports place it in a table of things
that "disagree today", implying comparable weight to the `ExternalPtrStmt` and
`make_pointer` truncations, which are bounded by user data rather than by a
1 MiB constant.

### 4.3 The overflow at `struct_llvm.cpp:174` has a sharper failure mode than
     "wraps"

Both reports correctly identify that
`snode->extractors[i].acc_shape * snode->extractors[i].shape`
(`taichi/codegen/llvm/struct_llvm.cpp:174-175`) is an `int` multiply performed
in the host compiler. Neither says what the emitted code then does with it:
the product becomes `prev`, the divisor of a `CreateURem` at
`struct_llvm.cpp:179`. An `int` product that wraps to exactly zero yields
`urem x, 0`, which is immediate undefined behaviour in LLVM IR, not a wrapped
index. The distinction is between a wrong answer and a poison value that
propagates.

### 4.4 Emission-time id validation does not exist

Following on from §1: nothing between `Translate2Spirv` writing a zero id and
the module leaving `TaskCodegen` checks that ids are non-zero. The only
backstop is the optional `spvtools` validator, and it is optional twice over —
`enable_spv_opt` defaults true at `spirv_codegen.h:26` but is overwritten by
`compile_config.external_optimization_level > 0` at
`taichi/codegen/spirv/kernel_compiler.cpp:38`, and the registered passes
(`spirv_codegen.cpp:2683-2707`) are optimisation, not validation. Pass A said
"caught at best by the optional spvtools validator" and that is right, but
neither report established that the default configuration runs nothing.

---

## 5. Claims I checked and confirmed as stated

Recorded so the planner knows what has been independently verified rather than
merely repeated. All read at the cited line.

- `const bool use_64bit_pointers = false;` — `spirv_codegen.cpp:82`. A member
  of `TaskCodegen`, not a parameter, not a capability query. Both reports right.
- `make_pointer` — `spirv_codegen.cpp:2317-2324`; live branch at `:2322` is
  `uint_immediate_number(ir_->u32_type(), uint32_t(offset))`, an explicit
  narrowing; the dead branch's own comment at `:2319` is
  "This is hacky, should check out how to encode uint64 values in spirv".
- `LinearizeStmt` is i32 on both paths and structurally identical —
  `codegen_llvm.cpp:1736-1744` (accumulator `:1737`, strides `:1740`) and
  `spirv_codegen.cpp:532-541` (accumulator `:533`, strides `:535-536`).
- `TaichiLLVMContext::get_constant(T)` dispatches on the C++ type —
  `llvm_context.cpp:727-749`, i32 at `:738-740`, i64 at `:741-745`. This is the
  mechanism both reports name and it is exactly as described.
- `create_bit_ptr`'s hard assertion —
  `TI_ASSERT(bit_offset->getType()->isIntegerTy(32));` at `codegen_llvm.cpp:1755`.
- `taichi_max_num_indices` appears in `taichi/codegen/` exactly twice: live at
  `struct_llvm.cpp:171`, and in a comment at `codegen_llvm.cpp:2363`. Pass A's
  claim confirmed by grep.
- `PhysicalCoordinates { i32 val[taichi_max_num_indices]; }` —
  `runtime.cpp:288-290`. `StructMeta` ABI: `snode_id` i32 at `:308`,
  `element_size` `size_t` at `:309`, `max_num_elements` i64 at `:310`,
  `lookup_element(Ptr, Ptr, int i)` at `:312`, `is_active` at `:316`,
  `get_num_elements` returning i32 at `:318`, `refine_coordinates(..., int index)`
  at `:320-322`.
- SPIR-V host-side layout is `size_t` throughout —
  `snode_struct_compiler.h:16`, `:19`, `:28`, `:31`, `:42`.
- `SNodeDescriptorsMap = std::unordered_map<int, SNodeDescriptor>` at
  `snode_struct_compiler.h:38`. No SNode ceiling on the SPIR-V path. Both right.
- `PhysicalPointerType`'s only construction is `snode_struct_compiler.cpp:53`,
  inside `StructCompiler::construct`, which is unreachable because its call is
  commented out at `:16-19`. Pass A right.
- `get_array_type` — `spirv_ir_builder.cpp:565`; `num_elems` is `uint32_t`;
  `nbytes` is `uint32_t` assigned from `size_t container_stride` at `:591`;
  the only diagnostic is a `nbytes == 0` warning at `:596-602`; the
  `DecorationArrayStride` is emitted at `:605`. Pass A's line numbers here are
  each one low; the substance is right.
- SPIR-V literal operands are single 32-bit words — `InstrBuilder`'s only
  scalar overload is `ADD(uint32_t, v)` at `spirv_ir_builder.h:158`. Pass B's
  claim confirmed.
- `at_buffer` — `spirv_codegen.cpp:2194-2220`; the u64 branch keys on
  `ptr_val.stype.dt == PrimitiveType::u64` at `:2197`; the u32 branch shifts by
  `log2(width)` at `:2214-2216` and indexes the SSBO at `:2217-2218`.
- `SNodeLookupStmt` casts the index up to the parent pointer's type before
  multiplying — `spirv_codegen.cpp:504-508`. Pass B's observation that this one
  site would follow a widened `make_pointer` automatically is correct.
- SPIR-V `kernel_compiler.cpp:37` does forward `params.caps = device_caps;`.

---

## 6. Where each report is stronger

Not a courtesy. The two are unequal in different places and the planner should
know which to trust on which question.

**Pass A is stronger on:** the tinyir type-translation path and the Int64
guard gap (§1); the `Dense_get_num_elements` narrowing on the far end of the
`StructMeta` wire; the completeness sweep of files with nothing relevant in
them, which is the only explicit coverage claim either report makes; the
explicit verified-versus-inferred index in its §6.

**Pass B is stronger on:** line-number accuracy, materially so — of the twelve
citations where the two differ, B is right on eight; the discarded
`device_caps` on the LLVM `KernelCompiler`, which is the single most useful
6.3 fact in either report; `bitmasked_activation` not following a widened
pointer; `demote_dense_struct_fors` being forced for SPIR-V; the `size_t`
narrowings at `spirv_codegen.cpp:1999` and the `DecorationNoSignedWrap`
promise at `:777-780`; the signed-versus-unsigned distinction being a separate
decision from width.

Neither is a superset of the other. Both should be kept.

---

## 7. Verdicts, argued

### 7.1 Correct?

**Yes, substantially, with the corrections in §1, §3 and §4.1 applied.**

I attacked the two highest-value claims in each report and both survived: the
`use_64bit_pointers` finding is exactly as described, and the per-tree-versus-
global-id chain holds link by link. Of roughly sixty file:line citations I
opened across the two reports, the errors were confined to off-by-one and
off-by-few line numbers plus two errors of substance:

- Pass B's "checked at every use" (§1.2) — **material**, because it answers the
  brief's own 6.2 question in the wrong direction.
- Pass A's Metal setter being "unconditional" (§3 row 9, §4.1) — **material**,
  because it makes the set of Int64-lacking targets look smaller than it is.

Everything else I could break, I could not break.

### 7.2 Complete?

**No.** Three gaps, in descending order of consequence:

1. The Int64-lacking target set is understated by both (§4.1). Direct3D 11 is
   absent from both reports entirely. The C API OpenGL override is absent from
   both. Metal is mischaracterised by one and omitted by the other.
2. The capability-bypass in `Translate2Spirv` is presented by pass A as an
   Int64 issue when it covers eight accessors across six optional widths
   (§1.4), and is absent from pass B.
3. Neither established the reachability or the write-not-read character of the
   SNode ceiling defect (§2.1, §2.2), which is what determines whether 6.1 is
   a sizing exercise or a correctness fix.

Coverage of `taichi/codegen/` itself is good. Pass A's file sweep at the end of
its §6 is the right instinct and I found nothing width-relevant it omitted from
that directory. The incompleteness is at the seams, in `taichi/rhi/` and
`c_api/`, where both reports went far enough to make claims but not far enough
to make them exhaustive.

### 7.3 Major revision?

**No, for either report.**

Neither report's structure, method or conclusions need rebuilding. Pass B needs
one paragraph corrected (§1.2) and pass A needs one sentence corrected (§4.1),
and both would benefit from the additions in §4. That is amendment, not
revision. Sending them back with the combined notes attached, per section 9.1
step 6 of the brief, is not warranted on what I found.

The one thing I would say plainly: **pass B's "the mechanism already exists"
conclusion should not be carried into the plan as written.** If it is, 6.2 work
will be built on a guarantee that holds for one code path and not for the one
that compiles kernel signatures.

---

## 8. Escalations

Unresolved. Each requires a decision the project owner has not made.

1. **Which quantity should replace 1024, and what should it bound.** The
   assertion at `struct_llvm.cpp:266` bounds a per-tree count; the arrays at
   `runtime.cpp:567-569` are indexed by a monotonic global id that is never
   reclaimed (§2). These are different numbers and the gap between them is
   currently a memory-corruption path. Whether 6.1 means "raise the array size",
   "make the assertion bound the right thing", or both, is not mine to decide.
   Interacts with `kMaxNumSnodeTreesLlvm = 512` (`constants.h:13`) and with
   territory 03.

2. **Whether the `Translate2Spirv` capability bypass is in scope for 6.2.**
   The eight bare accessor calls at `spirv_types.cpp:397-428` sit in the type
   translator, not the codegen proper. Closing them is a change to the SPIR-V
   type layer. Whether that is 6.2 work, territory 01 work, or deferred, needs
   the planner.

3. **Whether the C API OpenGL capability override is intended.**
   `c_api/src/taichi_opengl_impl.cpp:9-12` replaces the whole capability config,
   discarding the `is_gles()` guard at `opengl_device.cpp:509`. I did not
   determine whether any supported configuration reaches it. It is outside
   territory 02 and outside `taichi/`.

4. **Whether Direct3D 11 is a target this project cares about.**
   `taichi/rhi/dx/dx_device.cpp:562-566` sets only `spirv_version`. If DX11 is
   out of scope the Int64-lacking target set shrinks; if it is in scope it is a
   third case the stub architecture must express. The brief's section 5.2 tiers
   are stated in NVIDIA generations and do not settle it.

5. **`use_64bit_pointers` and `spirv_has_physical_storage_buffer`.** Both
   reports escalated these and I concur they should stay escalated. I add only
   that flipping `make_pointer` alone is insufficient: `bitmasked_activation`
   hardcodes `u32_type()` at `spirv_codegen.cpp:410`, `:411` and `:413`
   independently of it (§3 row 2).

6. **`taichi_global_tmp_buffer_size = 1024 * 1024`** (`constants.h:15`) is what
   currently makes the SPIR-V i32 `GlobalTemporaryStmt` offset safe (§4.2). If
   that constant is also an installation factor under 6.1, the divergence at
   `spirv_codegen.cpp:708` becomes live. Nobody has said whether it is.

---

## 9. Divergence with adversary-02-2

At the time of writing,
`/opt/project/taichi/modernization/investigation/adversary-02-2.md` did not
exist. I did not wait for it.
