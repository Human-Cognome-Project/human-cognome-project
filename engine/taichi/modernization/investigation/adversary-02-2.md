# Adversary 02-2 — Territory 02, code generation

Judging `report-02-codegen.md` (pass A) and `report-02b-codegen.md` (pass B),
with `notes-02-codegen.md` and `notes-02b-codegen.md` consulted where the
reports left me uncertain.

Every claim below was checked by opening the named file. Where I correct a
report I give the line I read.

---

## 0. Verdicts, stated first

| Question | Verdict |
|---|---|
| Is the work CORRECT? | **Pass A: yes, with citation slippage.** **Pass B: no on one load-bearing claim** (SPIR-V Int64), correct elsewhere. |
| Is the work COMPLETE? | **Neither is complete.** Both stopped one call-site short of the facts that decide 6.1 and 6.2. |
| Is major revision required? | **No for pass A. Yes, targeted, for pass B** — section 4 of B must be rewritten, and its conclusion "the mechanism the brief asks for exists" withdrawn. Both reports need the additions in my §5. |

The single most consequential correction in this analysis: **the SPIR-V type
system has no guard on Int64 at all on the live kernel-argument path, and
nothing anywhere in the C++ core rejects a 64-bit kernel argument on a device
that lacks the capability.** Pass B's contrary claim is the one place where
accepting a report at face value would lead the project to build 6.2's stub
architecture on a guarantee that does not exist.

---

## 1. The known contradiction, resolved

**Pass A is right. Pass B is wrong.** They are not describing different call
paths in a way that makes both partially right; B checked one function,
generalised from it, and never opened the file A named.

### 1.1 What each claimed

- A (`report-02-codegen.md` §3.3, "Assumed"): four unguarded reads of the
  64-bit STypes, one of them live, naming
  `Translate2Spirv::visit_int_type` at `taichi/codegen/spirv/spirv_types.cpp:393-419`
  as calling bare `i64_type()`/`u64_type()` accessors, and asserting that an
  unset `SType` has `id == 0`, which is not a valid SPIR-V id.
- B (`report-02b-codegen.md` §4): "**Checked at every use.**
  `IRBuilder::get_primitive_type`, `spirv_ir_builder.cpp:310-313` (i64) and
  `:325-328` (u64), each `TI_ERROR`." And in the assessment: "The SPIR-V codegen
  already expresses 'a target that lacks Int64' correctly ... The mechanism the
  brief asks for exists."

### 1.2 What the source says

**B's cited guard exists and is real, but it guards one function only.**
`IRBuilder::get_primitive_type` spans `spirv_ir_builder.cpp:288-332`. The i64
guard is at `:311-313` with the return at `:314`; the u64 guard is at `:325-327`
with the return at `:328`. Both are `TI_ERROR("Type {} not supported.", ...)`.
That much of B is verified.

**A's cited hole also exists, and it bypasses that function entirely.**
`Translate2Spirv::visit_int_type` at `spirv_types.cpp:393-419`:

```
} else if (type->num_bits() == 64) {
  vt = spir_builder_->i64_type();          // spirv_types.cpp:403
...
} else if (type->num_bits() == 64) {
  vt = spir_builder_->u64_type();          // spirv_types.cpp:415
}
ir_node_2_spv_value[type] = vt.id;         // spirv_types.cpp:418
```

`i64_type()` and `u64_type()` are bare accessors returning `t_int64_` /
`t_uint64_` with no capability test, at `spirv_ir_builder.h:529-534`. A's
citation of that range is exact.

`t_int64_` and `t_uint64_` are only ever assigned inside
`if (caps_->get(cap::spirv_has_int64))` at `spirv_ir_builder.cpp:166-169`.
Absent the capability they stay default-constructed. `SType::id` has the
default member initialiser `uint32_t id{0}` at `spirv_ir_builder.h:51`. A's
"id 0 is not a valid SPIR-V id" is correct: SPIR-V ids are strictly positive.

**The path is live, and reachable, which A asserted and I confirmed
independently.** `visit_int_type` is reached through `ir_translate_to_spirv`
(`spirv_types.cpp:476-485`), called from three sites in the codegen —
`spirv_codegen.cpp:2386`, `:2464`, `:2511` — for the args struct, the rets
struct and argpack structs respectively. The types fed into it come from
`translate_ti_type` (`spirv_types.cpp:484-505`) via `translate_ti_primitive`
(`:172-215`), which maps `PrimitiveType::i64` to `IntType(64, signed)` at
`:184-186` and `u64` to `IntType(64, unsigned)` at `:203-205`, with **no
capability parameter in scope at all**.

### 1.3 Why no earlier check saves it — the part neither report established

A escalated this (its escalation 4) as "I did not confirm whether a kernel with
an i64 parameter on a capability-lacking device is rejected by an earlier check
somewhere outside my territory." I resolved it. It is not.

1. `compile_args_struct` (`spirv_codegen.cpp:2326-2400`) is entered from
   `get_buffer_value` at `spirv_codegen.cpp:2276`. `get_buffer_value` does call
   `ir_->get_primitive_type(dt)` first, at `:2267` — but the `dt` it is given
   from `visit(ArgLoadStmt*)` is a **hardcoded placeholder**:
   `get_buffer_value(BufferType::Args, PrimitiveType::i32)` at
   `spirv_codegen.cpp:620`. So the guarded function is called with i32 and
   returns cleanly, and then `compile_args_struct` translates **every** argument
   type, including the i64 one, through the unguarded tinyir path.
2. `compile_args_struct` builds the struct from all of
   `ctx_attribs_->args_type()->elements()` (`spirv_codegen.cpp:2355-2367`),
   not just the argument being loaded. One i32 argument is enough to trigger it.
3. The zero id then propagates. `visit_struct_type`
   (`spirv_types.cpp:440-452`) pushes each member's id into an `OpTypeStruct`
   operand list at `:442-443`. `visit(ArgLoadStmt*)` then takes
   `val_type = args_struct_types_[arg_id]` at `spirv_codegen.cpp:621` and builds
   `get_pointer_type(val_type, StorageClassUniform)` at `:622-623`. Nothing on
   that route consults `caps_`.
4. There is no compensating check outside codegen. `Extension::data64` exists
   (`taichi/inc/extensions.inc.h:6`) and is granted to x64, arm64 and cuda only
   (`taichi/program/extension.cpp:12,16,20`) — and it is **never read anywhere
   in the C++ core**. The only reference outside `extension.cpp` is a name in a
   Python list, `python/taichi/lang/misc.py:186`. `is_extension_supported` is
   called exactly once in the whole tree, for `Extension::assertion`
   (`taichi/program/program.cpp:149`). Since the Python front end is explicitly
   out of scope (brief §1.2), for this project the guard does not exist at all.

**Answer, with line numbers.** SPIR-V Int64 is guarded on the
`IRBuilder::get_primitive_type` path (`spirv_ir_builder.cpp:311-314`, `:325-328`)
and unguarded on the tinyir type-translation path
(`spirv_types.cpp:393-419`, accessors at `spirv_ir_builder.h:529-534`), which is
the path the args, rets and argpack structs actually take
(`spirv_codegen.cpp:2386`, `:2464`, `:2511`). The failure mode on a device
without the capability is a SPIR-V module containing type id 0, not a diagnostic.

### 1.4 What this does to section 6.2

The brief's note under 6.2 says the architecture "must be able to express a
target that lacks" Int64. B's report tells the planner that mechanism already
exists and fails loudly. That is the wrong input to the decision. The correct
input is: **one of the two type-lowering paths has the mechanism, the other has
none, and the one without it is the one on the live argument path.** Anything
6.2 builds on top of the SPIR-V type layer has to close that hole first, or it
inherits a silent-invalid-module failure at exactly the point the brief
identifies as the risk.

### 1.5 Where A overstated, slightly

A listed four "assumed" sites. Three of the four are weaker than A implies:

- `from_taichi_type`'s `t_uint64_` return (`spirv_ir_builder.cpp:337-339`) and
  the ndarray base load at `spirv_codegen.cpp:783-792` are both reachable only
  under `spirv_has_physical_storage_buffer`, which on Vulkan is set at
  `vulkan_device_creator.cpp:826` inside `if
  (device_supported_features.shaderInt64)` at `:821`, and `spirv_has_int64` is
  set under the identical condition at `:630-632`. So the invariant holds in
  practice on the only backend that could set it. B's characterisation here —
  "enforced in the device layer, not in the builder that depends on it"
  (`report-02b` §4) — is the more precise one, and I credit B for it. Both are
  moot today because the setter is compiled out (§3.3 below).
- `get_primitive_uint_type` (`spirv_ir_builder.cpp:373-376`) is only reached
  with a 64-bit `dt`, which would have had to survive `get_primitive_type`
  elsewhere. Weakest of the four.

Only site 4, `visit_int_type`, is live and unguarded. A's own text says as much
("This is a **live** path"), so this is a matter of emphasis, not error. A's
central claim stands.

---

## 2. The `struct_llvm.cpp:266` assertion, verified independently

Both passes claim the assertion bounds a per-tree count while the runtime arrays
are indexed by a process-global SNode id. **Both are right. It is a live defect,
and it is reachable — more concretely than either report established.**

### 2.1 The chain, each link opened

| Link | Source |
|---|---|
| The assertion | `taichi/codegen/llvm/struct_llvm.cpp:266`, `TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);` |
| `snodes` | `StructCompiler::snodes`, `std::vector<SNode *>`, `taichi/struct/struct.h:11` |
| Filled by | `StructCompiler::collect_snodes`, `taichi/struct/struct.cpp:7-13`, recursing from one root |
| Called from | `StructCompilerLLVM::run(SNode &root)`, `struct_llvm.cpp:247-271`; `collect_snodes(root)` at `:250` |
| One compiler per tree | `LlvmProgramImpl::compile_snode_tree_types_impl`, `taichi/runtime/program_impls/llvm/llvm_program.cpp:45-57`; fresh `StructCompilerLLVM` at `:51-53`, `run(*root)` at `:54` |
| The arrays | `taichi/runtime/llvm/runtime_module/runtime.cpp:567-569` — `element_lists`, `node_allocators`, `ambient_elements`, each `[taichi_max_num_snodes]` |
| The index | `SNode::id`, from `std::atomic<int> SNode::counter` at `taichi/ir/snode.cpp:12`, `id = counter++` at `:220` |
| Reset scope | `Program::Program` sets `SNode::counter = 0` at `taichi/program/program.cpp:144`; also `SNode::reset_counter()` at `taichi/ir/snode.h:348-350`. Not per tree, not on tree destruction. |

Confirmed. Neither report is wrong on any link.

### 2.2 Reachability — the part both reports left as "no failing case built"

Pass A escalated this as inferred ("I read every link; I did not build a failing
case"). Pass B stated it as **[V]** but likewise gave no mechanism. Neither
opened the code that actually writes the arrays. I did.

**The write site is a range, not a single element.**
`runtime_initialize_snodes` at `runtime.cpp:986-1017`:

```
for (int i = root_id; i < root_id + num_snodes; i++) {   // runtime.cpp:1003
  runtime->element_lists[i] =                            // runtime.cpp:1005
      runtime->create<ListManager>(runtime, sizeof(Element), 1024 * 64);
}
```

`root_id` is the **global** id of the tree's root: `int root_id = tree->root()->id;`
at `taichi/runtime/program_impls/llvm/llvm_program.cpp:61`, carried into
`FieldCacheData::root_id` (`taichi/runtime/llvm/llvm_offline_cache.h:77`) and
read back at `taichi/runtime/llvm/llvm_runtime_executor.cpp:399`. The call is
at `llvm_runtime_executor.cpp:442-444`, passing `(int)snode_metas.size()` as
`num_snodes`. So tree *k* writes `element_lists[root_id .. root_id+n)`, and
`root_id` grows monotonically across trees while the assertion only ever sees
each tree's own `n`.

The other two arrays are written the same way, per global id:
`runtime_NodeAllocator_initialize` writes `node_allocators[snode_id]`
(`runtime.cpp:1291-1296`) and `runtime_allocate_ambient` writes
`ambient_elements[snode_id]` (`runtime.cpp:1298-1304`), both called with
`snode_metas[i].id` at `llvm_runtime_executor.cpp:460-466`.

**The assertion is not compiled out.** `TI_ASSERT` expands unconditionally —
`taichi/common/logging.h:100-101`, no `NDEBUG` guard. So the per-tree bound
genuinely fires; it just bounds the wrong quantity.

**Two conditions narrow, but do not remove, reachability.** Neither report
noticed either:

1. `runtime_initialize_snodes` returns early when `all_dense`
   (`runtime.cpp:1000-1002`), so the `element_lists` range write is skipped for
   an entirely dense tree. `all_dense` is computed at
   `llvm_runtime_executor.cpp:400-409` and is true only when every SNode in the
   tree is `dense`, `place` or `root`.
2. The `node_allocators` / `ambient_elements` writes are filtered by
   `is_gc_able(snode_metas[i].type)` (`llvm_runtime_executor.cpp:446`), which is
   `pointer || dynamic` (`taichi/ir/snode_types.cpp:21-23`).

**This makes the defect *more* relevant to this project, not less.** Both
surviving routes require sparse SNodes, and the brief states at §4.1 that
"**Sparsity is required.** Do not treat sparse machinery as surplus." The
configuration that trips this is precisely the configuration the project needs.

**Failure mode.** An out-of-range store into `element_lists[i]` lands in
`node_allocators`, then `ambient_elements`, then `temporaries` and
`rand_states` (`runtime.cpp:567-572`) — adjacent members of the same
`LLVMRuntime` struct. Silent memory corruption of the device runtime, not a
crash at the point of error.

**Verdict on item 5 of my brief:** confirmed live, confirmed reachable, and the
reachable configuration is the project's target configuration. Both reports were
right to raise it and both under-supported it. It is not merely theoretical.

---

## 3. Every other place the two reports disagree, resolved

### 3.1 `bitmasked_activation` — B right, A wrong

A's S3.2 entry S6 describes `bitmasked_activation` as doing its "shifts/masks in
`ptr_dt` (u32)". B (`report-02b` §3.2 and §6) says it **hardcodes** `u32_type()`
at `:405, 411-412, 414` "independently of `make_pointer`", so bitmasked SNodes
stay u32 even if the pointer width were raised.

Source, `spirv_codegen.cpp:383-435`. Both descriptions are partly true and B's
is the load-bearing one. Lines `:392-399` do use `ptr_dt`. But `:410-412` build
`OpShiftRightLogical` with an explicitly hardcoded `ir_->u32_type()` **result
type** applied to `bitmask_word_ptr`, which at `:409` is
`ir_->add(parent_ptr, ...)` and therefore carries the pointer width. `:414`
then does `struct_array_access(ir_->u32_type(), ...)`.

This is worse than B stated. In SPIR-V, `OpShiftRightLogical`'s Result Type must
match the type of Base. A u64 `Base` with a u32 `Result Type` is not merely a
narrowing — it is an **invalid instruction**. B identified the right lines and
the right conclusion; A missed the hardcodes entirely. **B right.**

### 3.2 `DecorationNoSignedWrap` on the external-pointer offset — B only

`spirv_codegen.cpp:773-781`: the byte offset is computed by
`OpShiftLeftLogical` at i32 (`:773-777`) and then, when
`spirv_has_no_integer_wrap_decoration` is present, decorated
`DecorationNoSignedWrap` (`:778-781`). B flags it; A does not mention it.
B's characterisation — that this converts overflow from wrapping to undefined —
is correct SPIR-V semantics. **B's finding, and it belongs in any 6.2 scope.**

### 3.3 The `spirv_has_physical_storage_buffer` guard — both right, both imprecise

`vulkan_device_creator.cpp`: the enclosing test is
`if (device_supported_features.shaderInt64) {` at `:821`; the comments citing
upstream issue 6295 are at `:822-824`; the guard is
`#if !defined(__APPLE__) && false` at `:825`; the single tree-wide setter is
`caps.set(DeviceCapability::spirv_has_physical_storage_buffer, true);` at
`:826`; `#endif` at `:827`. A cited "823-827", B cited "820-828", and B
additionally attributed the `shaderInt64` test to `:823` (that line is a URL
comment). Both reached the right conclusion — the whole 64-bit SPIR-V address
path is dead. **Substantively both right; B has the extra citation error.**

### 3.4 `use_64bit_pointers` — A's framing right, B's framing misleading

`spirv_codegen.cpp:82`, `const bool use_64bit_pointers = false;`, read only at
`:2318` inside `make_pointer` (`:2317-2324`). A calls it "the one decision point"
that does not consult `caps_`. B says "**both** switches must be turned on".

B's framing implies flipping one alone is merely insufficient. It is worse than
that, and neither report says so — see §5.2. **A's framing is the safer input to
a 6.2 decision.**

### 3.5 `SNodeLookupStmt` index handling — B's reading is the more useful one

`spirv_codegen.cpp:504-508`. A (S5) says "the i32 linear index is `cast` to
`parent_val.stype` (u32), multiplied ... All SNode address arithmetic is u32
modular." B says it "casts the index **up** to the pointer type before
multiplying by the stride. If `make_pointer` returned u64, that multiply would
happen at 64 bits with no further change."

Both describe `ir_->cast(parent_val.stype, ...)` at `:504-505` correctly. B's
observation that this site follows the pointer width automatically is a
scoping fact 6.2 needs; A's is a statement of today's behaviour. **Not a
conflict. B adds information A does not.**

### 3.6 `Dense_get_num_elements` narrows i64 back to i32 — A only

A's escalation 7. Verified: `StructMeta::max_num_elements` is `i64` at
`runtime.cpp:310`, the function pointer field `get_num_elements` is declared
returning `i32` at `runtime.cpp:318`, and `Dense_get_num_elements` at
`taichi/runtime/llvm/runtime_module/node_dense.h:10-12` returns
`((StructMeta *)meta)->max_num_elements` through an `i32` return type. A
truncation on the one piece of SNode geometry that is already 64-bit end to
end. **A right; B calls `max_num_elements` "the only piece of SNode geometry
already 64-bit on both sides" (§3.1) without noticing that the accessor
un-does it.** B's statement as written is wrong.

### 3.7 The LLVM `KernelCompiler` drops `device_caps` — B only

`taichi/codegen/llvm/kernel_compiler.cpp:30-48`: `const DeviceCapabilityConfig
&device_caps` is the parameter at `:32` and is never referenced in the body; the
`KernelCodeGen::create` call at `:36-37` does not receive it. The SPIR-V
equivalent does forward it, `taichi/codegen/spirv/kernel_compiler.cpp:37`,
`params.caps = device_caps;`. **B right, and this is a stronger statement of the
6.3 seam than A's "the LLVM path has no analogue."**

### 3.8 `PhysicalPointerType` is unconditionally 64-bit and its only construction
site is dead — A only

`spirv_types.h:71-77`: `PhysicalPointerType` derives from
`IntType(/*num_bits=*/64, /*is_signed=*/false)`. Its only construction is
`snode_struct_compiler.cpp:53`, inside `StructCompiler::construct`
(`:34-63`), whose only call was in the commented-out block at
`snode_struct_compiler.cpp:16-19`. **A right; B does not mention it.**

### 3.9 Host-side `size_t` narrowed into the range-for kernel — B only

`spirv_codegen.cpp:1999`, `const int num_elems = range_for_attribs.end -
range_for_attribs.begin;`, where both fields are `size_t`
(`taichi/codegen/spirv/kernel_utils.h:110-111`), and
`advisory_total_num_threads` is `int` (`kernel_utils.h:99`), assigned at
`spirv_codegen.cpp:2004`. **B right; A's S12 covers the emitted i32 constants
but not the host-side narrowing that feeds them.**

### 3.10 SPIR-V struct-for `LoopIndexStmt` — same escalation, B did more work

Both raise it. `visit(LoopIndexStmt*)` is at `spirv_codegen.cpp:543-563`, with
`TI_NOT_IMPLEMENTED` at `:553` for any offloaded task type other than
`range_for`. B additionally established that `demote_dense_struct_fors` is forced
true for every SPIR-V arch at `taichi/program/compile_config.cpp:72-74`, which
explains why dense struct-fors never reach it. **B's escalation is better
supported.** A's line range (`:541-561`) is two off.

### 3.11 `list_element_size` narrowing — B's assessment is correct, A's is not wrong but is vaguer

`codegen_llvm.cpp:2290-2291`. B notes it is safe today because
`taichi_listgen_max_element_size` bounds it. That constant is `1024` at
`taichi/inc/constants.h:28` — B cited `:27`, which is blank. A flags it as
something "that must be revisited" without noting the bound makes it currently
safe. **B substantively right, with a citation slip.**

---

## 4. Citation corrections

Every one of these I opened. Ordering: report, claimed, actual.

| Report | Claimed | Actual | Severity |
|---|---|---|---|
| A §3.3 | `vulkan_device_creator.cpp:628-631` gates on `shaderInt64` | `:630-632` (`:628` is the int16 `caps.set`) | wrong lines |
| A §3.3 | `get_primitive_type` u64 error at `:324-327` | `:325-328` | off by one |
| A §4.1.2, §4.1 | `AxisExtractor` fields at `snode.h:40,44,49` | `:41,45,49` | off by one; **B right** |
| A §3.1 L14, §4.1.8 | `list_element_size` at `codegen_llvm.cpp:2291-2294` | `:2290-2291` | wrong span |
| A §3.2 S14 | `container_stride` narrowing at `spirv_ir_builder.cpp:590` | `:591` | off by one |
| A §3.2 S14 | array length emitted at `:575` | `:576` | off by one |
| A escalation 5 | SPIR-V `LoopIndexStmt` at `spirv_codegen.cpp:541-561` | `:543-563` | off by two |
| A §2.1 | runtime index sites list omits the range write | the loop is `runtime.cpp:1003-1006`; **B lists `:1005`** | material omission |
| B §4 | i64 guard at `spirv_ir_builder.cpp:310-313` | `:311-314` (`:310` is `return t_int32_;`) | off by one |
| B §4 | `shaderInt64` test for PSB at `vulkan_device_creator.cpp:823` | `:821` (`:823` is a comment) | wrong line |
| B §3.1 | `StructMeta::max_num_elements` is i64, `runtime.cpp:307` | `:310` (`:307` is `struct StructMeta {`); **A right** | wrong line |
| B §3.1, §6 | `GlobalTemporaryStmt` i64 at `codegen_llvm.cpp:2385` | the `get_constant((int64)...)` is `:2386`; **A right** | off by one |
| B §3.1 | `taichi_listgen_max_element_size` at `constants.h:27` | `:28` | off by one |
| B §5 | `kMaxNumSnodeTreesLlvm = 512` at `runtime.cpp:562-563` | defined at `taichi/inc/constants.h:13`; `:562-563` are the array declarations that use it | misattributed |
| B §6.2 list | `generate_refine_coordinates` third param at `struct_llvm.cpp:152` | `:153`; **A right** | off by one |
| B §6.3 | `llvm/kernel_compiler.cpp:29-48`, `device_caps` at `:31` | `:30-48`, parameter at `:32` | off by one |
| B §3.2 | `get_array_type` narrowing at `spirv_ir_builder.cpp:583-589` | the narrowing is `:591`; `:583-589` is the type emission; **A closer** | wrong region |

None of these individually changes a conclusion. Collectively they mean that
neither report can be transcribed into the plan without re-opening the file.
Pass B has more of them and two of them (`runtime.cpp:307`, `codegen_llvm.cpp:2385`)
sit on claims B uses to argue a conclusion.

I checked and could **not** break the following, which I list because I tried:
`spirv_codegen.cpp:82`; `make_pointer` at `:2317-2324`; `at_buffer` at
`:2193-2220`; `LinearizeStmt` at `codegen_llvm.cpp:1736-1744` and
`spirv_codegen.cpp:531-541`; `create_bit_ptr`'s assertion at
`codegen_llvm.cpp:1755` with the documented struct at `:1750-1753`;
`loop_index_ty` at `:2157`; the two `LoopIndexStmt` i32 loads at `:2336` and
`:2339-2340`; `get_constant(T)` width dispatch at
`taichi/runtime/llvm/llvm_context.cpp:727-750` with instantiations at
`:904-916`; `PhysicalCoordinates` at `runtime.cpp:288-290`; the SNode accessor
ABI at `runtime.cpp:312-322` and `node_dense.h:10-24`; `taichi_max_num_snodes`
appearing exactly four times tree-wide (`constants.h:12`, `struct_llvm.cpp:266`,
`runtime.cpp:567-569`); `taichi_max_num_indices` appearing in codegen exactly
twice, live at `struct_llvm.cpp:171` and in a comment at `codegen_llvm.cpp:2363`;
`SNodeDescriptorsMap` as an unbounded `unordered_map` at
`snode_struct_compiler.h:38`; all `SNodeDescriptor` fields `size_t` at
`snode_struct_compiler.h:16,19,28,31` with `root_size` at `:42`.

---

## 5. What both reports missed

Shared blindness, in descending order of consequence for section 6.

### 5.1 Nothing in the C++ core rejects a 64-bit kernel argument on any device

Established in §1.3. `Extension::data64` is declared at
`taichi/inc/extensions.inc.h:6` and granted at `taichi/program/extension.cpp:12,16,20`,
and **no code anywhere reads it**. `is_extension_supported`
(`extension.cpp:8-33`) is called once in the tree, for `Extension::assertion`
(`taichi/program/program.cpp:149`). A left this as an open escalation; B
asserted the opposite of the truth. Neither ran the grep.

This is the fact that decides how much work 6.2's stub-architecture note is.

### 5.2 Flipping `use_64bit_pointers` alone does not merely fail to help — it emits a module that uses a capability the header never declares

Neither report traced what `at_buffer` does with a widened pointer.
`at_buffer` (`spirv_codegen.cpp:2193-2220`) branches on
`ptr_val.stype.dt == PrimitiveType::u64` at `:2197` — **on the SType's data
type, not on `has_buffer_ptr`**. `declare_primitive_type` sets `t.dt = dt`
(`spirv_ir_builder.cpp:1536`), so `u64_type()` carries `dt == u64` whenever the
Int64 capability was present.

Therefore, on a device with Int64 but without physical storage buffers — which
is every Vulkan device today, since the PSB setter is compiled out — setting
`use_64bit_pointers = true` sends every SNode access through the
`OpConvertUToPtr` into `spv::StorageClassPhysicalStorageBuffer` branch at
`:2198-2204`, while `OpCapability PhysicalStorageBufferAddresses` is emitted
only under `caps_->get(cap::spirv_has_physical_storage_buffer)` at
`spirv_ir_builder.cpp:73-77`. The result is an invalid module, not a wider one.

And on a device *without* Int64, `make_pointer`'s 64-bit branch calls
`ir_->u64_type()` (`spirv_codegen.cpp:2320`) — the same bare accessor as §1.2 —
producing type id 0.

So `use_64bit_pointers` is coupled to **two** capabilities, not one, and to
neither by any code. B's "both switches must flip" understates the coupling;
A's "it does not consult `caps_`" is correct but stops before the consequence.

### 5.3 The refine-coordinates ABI truncates a value the code itself computes at 64 bits

Both reports note that `generate_refine_coordinates`'s third parameter `l` is
i32 (`struct_llvm.cpp:153`) and that `PhysicalCoordinates::val` is `i32`
(`runtime.cpp:289`). Neither connects it to the quantity `l` actually carries.
`l` is the within-container linear index, bounded by
`SNode::max_num_elements()`, which returns `int64`
(`taichi/ir/snode.h:306-308`, backed by `int64 num_cells_per_container` at
`:97`), is stored as `i64` in `StructMeta` (`runtime.cpp:310`) and is used to
size the LLVM body array at `struct_llvm.cpp:72`. So the *only* place the
container size is narrowed on the LLVM side is this ABI and
`Dense_get_num_elements` (§3.6). That is the precise shape of the LLVM half of
6.2, and neither report states it that way.

### 5.4 `LinearizeStmt`'s strides are produced outside codegen, at `int`

Both reports correctly report the consumer. Neither names the producer:
`taichi/transforms/scalar_pointer_lowerer.cpp:33`,
`std::array<int, taichi_max_num_indices> total_shape;`, and the same shape at
`taichi/transforms/demote_dense_struct_fors.cpp:19`. Widening
`LinearizeStmt::strides` without those is a half-change. It is territory 01's
file, but it is the direct upstream of the one site both reports name as "the
site that must change first". The seam was left unmarked by both.

### 5.5 The struct-for `element_lists` overflow is gated by `all_dense`

Covered in §2.2. Neither report opened
`llvm_runtime_executor.cpp:400-409` or `runtime.cpp:1000-1003`, so neither could
say which SNode configurations actually reach the overflow. The answer matters,
because it is the sparse configurations, which the brief requires.

### 5.6 `TI_ASSERT` is unconditional

`taichi/common/logging.h:100-101`. Neither report says whether the per-tree
bound survives a release build. It does. This matters for reasoning about the
defect in §2: the assertion is a real, always-on check that simply measures the
wrong thing, rather than a debug-only check that vanishes.

---

## 6. Verdicts

### 6.1 Correct?

**Pass A: correct.** Every substantive claim I tested held. Its inventory tables
are accurate on content and slightly loose on line numbers (seven slips, all
minor, listed in §4). Its central factual claims — the `use_64bit_pointers`
constant, the u32 SPIR-V address representation, the dead physical-storage-buffer
path, the `get_constant` C++-type dispatch as the mechanism that hides i32 on
the LLVM path, the per-tree-versus-global-id mismatch, the unguarded tinyir
Int64 path — are all verified. Its escalations are correctly identified as
escalations rather than smuggled in as conclusions.

**Pass B: not correct on one load-bearing claim, correct elsewhere.** Section 4
of B is wrong: "Checked at every use" is false, and the assessment built on it —
"The SPIR-V codegen already expresses 'a target that lacks Int64' correctly ...
The mechanism the brief asks for exists" — is the opposite of what the source
shows on the live argument path. B marked that assessment **[V]**, and it is not
verified; B's own notes (`notes-02b-codegen.md:23`) list `spirv_types.cpp` in the
file sweep but the notes contain no trace of `visit_int_type` being opened. B
also states as fact that `max_num_elements` is "the only piece of SNode geometry
already 64-bit on both sides" (§3.1), which `node_dense.h:10-12` contradicts.
Everything else in B that I tested held, and B has four findings A lacks (§3.1,
§3.2, §3.7, §3.9).

### 6.2 Complete?

**Neither.** They cover the territory's files thoroughly — B swept 51 files, A
lists its empty-sweep files explicitly, and I found no file in
`taichi/codegen/` that neither opened. The incompleteness is not coverage, it is
depth: both stopped at the boundary of `taichi/codegen/` on the three questions
where one more call site would have settled the answer.

- The Int64 question needed one grep for `Extension::data64` (§5.1). A escalated
  it; B answered it wrongly.
- The `struct_llvm.cpp:266` question needed
  `llvm_runtime_executor.cpp:390-467` (§2.2). Both stopped at "indexed by
  `snode->id`" and neither could say whether it is reachable.
- The `use_64bit_pointers` question needed `at_buffer`'s branch condition at
  `spirv_codegen.cpp:2197` (§5.2). Both described the switch; neither followed it.

Territorial boundaries explain the stop but do not excuse it, because the brief
§9.3 says the territories carry "deliberate overlap at the seams so the
adversaries have something to test", and §10.5 asks for findings, not
jurisdiction.

### 6.3 Is major revision required?

**Pass A: no.** Correct the seven citations in §4 and fold in §5.1, §5.2, §5.3
and §5.5. Its escalation 4 can be closed with the answer in §1.3.

**Pass B: yes, targeted.** Section 4 must be rewritten and its concluding
assessment withdrawn — as written it would tell the planner that a guarantee
central to 6.2 already exists when it does not. The `max_num_elements` claim in
§3.1 must be corrected against `node_dense.h:10-12`. The five citation errors in
§4 above must be fixed. B's sections 3, 5, 6 and 7 are sound and should be kept;
B's four unique findings should survive into whatever is merged.

**On the pair as a whole:** the divergence worked. The one place the two passes
flatly contradicted each other is the one place a single report would have
misled the project, and it took two passes to expose it. Where they agreed —
the per-tree assertion — they agreed on an under-supported conclusion that
turned out to be right but for reasons neither had established. That is the
shape of the shared blindness in §5: agreement between the passes was a weaker
signal than disagreement.

---

## 7. Escalations

Unresolved. These require a decision the project owner has not made, and I am
not making it.

1. **Which quantity the SNode ceiling should bound.** Both reports raise it and
   I have now shown the mismatch is reachable in sparse configurations (§2.2).
   The choice between bounding per-tree SNode count and bounding the global id
   space is a design decision interacting with `kMaxNumSnodeTreesLlvm = 512`
   (`taichi/inc/constants.h:13`) and with territory 03's arrays. Item 8.1.2 of
   the brief reserves the value; the *quantity* is equally unreserved.

2. **Whether the global `SNode::counter` should be reset per tree, or the arrays
   re-indexed.** These are different fixes with different blast radii and I am
   not choosing. `taichi/ir/snode.cpp:12,220`;
   `taichi/program/program.cpp:144`; `taichi/ir/snode.h:348-350`.

3. **What closes the tinyir Int64 hole.** `spirv_types.cpp:393-419` needs either
   a capability parameter threaded into `Translate2Spirv`, or a check at the
   `translate_ti_type` call sites (`spirv_codegen.cpp:2344`, `:2358`, `:2420`,
   `:2436`, `:2494`), or a front-of-pipeline rejection. Which of the three is
   right depends on where 6.2 decides the capability boundary sits. Not mine.

4. **Whether `Extension::data64` is meant to be live.** It is declared, populated
   per-arch, and read by nothing (§5.1). Whether it is a vestigial mechanism to
   revive or a dead one to route around is a decision, and the standing
   instructions forbid me proposing removal.

5. **Whether `spirv_has_physical_storage_buffer`'s `&& false`
   (`vulkan_device_creator.cpp:825`) is current or stale.** Both reports
   escalated this and I concur it is not determinable from the source. It
   decides whether the 64-bit SPIR-V branches are a starting point or a
   liability.

6. **Signedness of a widened loop counter.** B raises it; I confirm every loop
   comparison in both paths is signed (`codegen_llvm.cpp:1188`, `:1192`,
   `:2213-2215`; `cpu/codegen_cpu.cpp:111-112`) except the SPIR-V struct-for,
   which is unsigned (`spirv_codegen.cpp:2156-2158`, `OpULessThan`). Width and
   signedness are separate decisions and neither is settled.

7. **`create_bit_ptr`'s `isIntegerTy(32)` assertion** (`codegen_llvm.cpp:1755`).
   Both reports raise it; both decline to decide whether a bit offset within a
   physical type needs widening. I also decline. It is a real collision point
   with any generic widening.

---

## 8. Divergence with adversary 02-1

At the time of writing,
`/opt/project/taichi/modernization/investigation/adversary-02-1.md` **does not
exist**. I checked the directory after completing my own analysis; it contains
only the eight notes files and eight report files. I did not wait for it, per
my instructions. No comparison section can be recorded.

---

## 9. Divergence with adversary 02-1 — appended after writing

`adversary-02-1.md` did not exist when I finished §1-§8; it appeared before I
closed out. §8 above records the state at the time of writing and is left
standing. This section is the comparison. I verified every claim of theirs that
I had not already checked myself before crediting it.

### 9.1 Where we agree

We reached the same verdict on the contradiction, independently and by the same
route: **pass A right, pass B wrong**, `Translate2Spirv::visit_int_type` at
`spirv_types.cpp:393-419` calling the bare accessors at
`spirv_ir_builder.h:529-534`, `SType::id{0}` at `spirv_ir_builder.h:51`, and the
three live call sites at `spirv_codegen.cpp:2386`, `:2464`, `:2511`. We also
agree that pass B's "the mechanism the brief asks for exists" must not be
carried into the plan as written. Two adversaries arriving at that separately
should settle it.

We also agree that the `struct_llvm.cpp:266` defect is live rather than
theoretical, and both of us independently found the write site neither report
opened — the range loop at `runtime.cpp:1003-1006`. We agree on the resolution
of most report-versus-report disagreements: B on `bitmasked_activation`,
`AxisExtractor` line numbers, `list_element_size`, the Vulkan Int64 setter, the
discarded `device_caps`, `spirv_codegen.cpp:1999`, and the struct-for
`LoopIndexStmt`; A on `codegen_llvm.cpp:2386`, `runtime.cpp:310`,
`struct_llvm.cpp:153`, `Dense_get_num_elements`, and `PhysicalPointerType`.

### 9.2 Where they found what I did not — checked, and they are right

Three findings of theirs I had not made. I opened each before accepting it.

1. **Pass A's "Metal sets `spirv_has_int64` unconditionally" is factually
   wrong.** `taichi/rhi/metal/metal_device.mm:1052` sits inside
   `if (feature_64_bit_integer_math) {` at `:1051`, and
   `bool feature_64_bit_integer_math = family_apple3;` at `:1038`, derived from
   `[mtl_device supportsFamily:kMTLGPUFamilyApple3]` at `:1035-1036`. Confirmed.
   Pre-Apple3 Metal is a second in-tree SPIR-V target without Int64. **They are
   right and I missed it.** My §1 accepted A's enumeration of the three setters
   without opening the Metal one.

2. **Direct3D 11 sets no integer capabilities at all.** `Dx11Device::Dx11Device`
   at `taichi/rhi/dx/dx_device.cpp:557-568` builds `DeviceCapabilityConfig caps{}`
   at `:563`, sets only `spirv_version` at `:564`, and calls `set_caps` at `:565`.
   Confirmed. (Their span `:562-566` is two lines wide; the substance is right.)
   A third target without Int64, absent from both reports and from my analysis.

3. **The C API OpenGL runtime overrides the GLES guard.**
   `OpenglRuntime::OpenglRuntime` at `c_api/src/taichi_opengl_impl.cpp:4-13`
   sets `spirv_has_int64` at `:9` with no guard and then calls
   `get_gl().set_caps(std::move(caps))` at `:12`, a whole-object replacement of
   the config that `GLDevice::GLDevice` had built under `if (!is_gles())`
   at `taichi/rhi/opengl/opengl_device.cpp:509-513`. Confirmed, including that
   `ti_import_opengl_runtime` takes `bool use_gles` at
   `taichi_opengl_impl.cpp:21-22` and forwards it to `set_gles_override` at `:26`.
   My grep was scoped to `taichi/` and missed `c_api/`. **They are right.**

   This compounds my §5.1 rather than duplicating it: I established that nothing
   rejects a 64-bit kernel argument; they established that a target which cannot
   execute one can affirmatively claim it can.

Two smaller ones I also confirmed and had not made:

4. **`taichi_global_tmp_buffer_size` is what makes the SPIR-V i32
   `GlobalTemporaryStmt` offset safe today** (their §4.2). Stronger than they
   put it: the bound is not implicit, it is asserted —
   `TI_ASSERT(global_offset_ < taichi_global_tmp_buffer_size);` at
   `taichi/transforms/offload.cpp:358`, inside `allocate_global` at `:344`, with
   the constant `1024 * 1024` at `taichi/inc/constants.h:15`. Both reports list
   the LLVM-versus-SPIR-V width divergence as a headline; it has no live
   consequence while that assertion holds.

5. **Tree ids recycle, SNode ids do not** (their §2.2).
   `Program::destroy_snode_tree` pushes onto `free_snode_tree_ids_` at
   `taichi/program/program.cpp:235`, popped in `allocate_snode_tree_id` at
   `:559-567`, while `SNode::counter` only increments
   (`taichi/ir/snode.cpp:220`). Confirmed. This is a **better reachability
   argument than mine.** I showed the overflow requires sparse SNodes (§2.2);
   they showed it does not require many concurrent trees, only many over the
   process lifetime. The two combine: a process that creates and destroys a few
   hundred small sparse trees reaches it, with `kMaxNumSnodeTreesLlvm = 512`
   never approached. Taken together the defect is more reachable than either of
   us argued alone.

### 9.3 Where I found what they did not

1. **Nothing in the C++ core rejects a 64-bit kernel argument on any device**
   (my §5.1). `Extension::data64` is declared at
   `taichi/inc/extensions.inc.h:6`, populated per-arch at
   `taichi/program/extension.cpp:12,16,20`, and read by nothing;
   `is_extension_supported` is called once in the tree, for
   `Extension::assertion`, at `taichi/program/program.cpp:149`. Adversary 1
   leaves pass A's escalation 4 open ("remains open"). It does not need to
   remain open — the answer is that no earlier check exists. This matters
   because their §4.1 and my §5.1 are the two halves of the same problem, and
   without mine theirs reads as a capability-reporting bug rather than an
   absent-guard one.

2. **Flipping `use_64bit_pointers` alone emits a module using a capability the
   header never declares** (my §5.2). `at_buffer` branches on
   `ptr_val.stype.dt == PrimitiveType::u64` at `spirv_codegen.cpp:2197`, and
   `declare_primitive_type` sets `t.dt = dt` at `spirv_ir_builder.cpp:1536`, so a
   widened `make_pointer` routes every SNode access into the
   `OpConvertUToPtr` / `StorageClassPhysicalStorageBuffer` branch at
   `:2198-2204` while `OpCapability PhysicalStorageBufferAddresses` is emitted
   only under its own capability at `spirv_ir_builder.cpp:73-77`. Their
   escalation 5 says flipping `make_pointer` alone is "insufficient" because of
   `bitmasked_activation`. It is worse than insufficient, and for a second
   reason they do not give.

3. **The refine-coordinates ABI is the only LLVM-side narrowing of a quantity
   the code otherwise carries at 64 bits** (my §5.3): `l` is i32 at
   `struct_llvm.cpp:153` and `PhysicalCoordinates::val` is i32 at
   `runtime.cpp:289`, while `SNode::max_num_elements()` is `int64`
   (`taichi/ir/snode.h:306-308`) and sizes the LLVM body array at
   `struct_llvm.cpp:72`. Neither they nor either report states the LLVM half of
   6.2 in those terms.

4. **`LinearizeStmt`'s strides are produced at `int` outside codegen** (my §5.4),
   `taichi/transforms/scalar_pointer_lowerer.cpp:33` and
   `taichi/transforms/demote_dense_struct_fors.cpp:19`. Both reports and
   adversary 1 name the consumer and none names the producer.

5. **The `all_dense` and `is_gc_able` filters on the overflow** (my §2.2):
   `runtime.cpp:1000-1002` and `llvm_runtime_executor.cpp:446` with
   `taichi/ir/snode_types.cpp:21-23`. Adversary 1 describes the range write but
   not what gates it. The gate is what ties the defect to the brief's §4.1
   sparsity requirement.

6. **`TI_ASSERT` is unconditional** (`taichi/common/logging.h:100-101`, my §5.6).
   Neither they nor either report establishes that the per-tree check survives a
   release build. It does.

### 9.4 Where we disagree, and what the source says

**One substantive disagreement.**

Adversary 1's §3 row 2 rules "**B on substance, A on line numbers**" for
`bitmasked_activation`, and gives the hardcoded sites as `:404`, `:410`, `:411`,
`:413`, correcting pass B's "405, 411-412, 414".

**Their correction is wrong and pass B's original citation is right.**
`grep -n "u32_type()" taichi/codegen/spirv/spirv_codegen.cpp` returns, within
this function, exactly `:405`, `:411`, `:412`, `:414` (then `:417` onward in the
activate/deactivate/query branches). Lines `:404`, `:410` and `:413` are
statement-opening lines that contain no `u32_type()` call — `:404` is
`ir_->make_value(spv::OpShiftLeftLogical, ptr_dt, bitmask_word_index,` and `:413`
is a bare `bitmask_word_ptr =`. They appear to have cited statement starts while
correcting a report that cited the actual call sites. On this row pass B needs no
correction, and adversary 1's amendment should not be applied.

We agree on the substance: A's description of the function as operating in
`ptr_dt` is incomplete, and B's is the one that matters for 6.2.

**Two differences of emphasis, not fact.**

- **On whether major revision is required for pass B, we split.** They say no
  for both reports; I say yes, targeted, for B. This is a smaller gap than it
  reads: they also write "pass B's 'the mechanism already exists' conclusion
  should not be carried into the plan as written", which is the same
  instruction. The disagreement is over whether rewriting a report's section 4
  and withdrawing its concluding assessment counts as revision or amendment.
  I hold that a claim the report marked **[V]**, which answers the brief's own
  6.2 question in the wrong direction, and which the report's notes show was
  never checked, is more than a typo. The planner holds the arbiter seat on
  this and the practical consequence is identical either way.

- **On the severity of `struct_llvm.cpp:174`.** They argue (§4.3) that an `int`
  product wrapping to exactly zero makes `CreateURem(l, prev)` at
  `struct_llvm.cpp:179` undefined behaviour in LLVM IR rather than a wrapped
  index. The code is as they describe and the reasoning is sound. I note only
  that wrapping to exactly zero mod 2^32 is a narrower case than wrapping in
  general; the more common outcome is a wrong-but-defined divisor. Their point
  stands as an additional failure mode, not the principal one.

### 9.5 Net

On the contradiction the brief asked us to resolve first, we agree completely
and independently: **pass A is right, pass B is wrong, and B's section 4 must
not reach the plan as written.**

Where we differ, the source supports them on the RHI capability landscape
(Metal, DX11, the C API OpenGL override) and on tree-id recycling, all of which
I had not checked; and it supports me on the absent `Extension::data64` guard,
on what flipping `use_64bit_pointers` actually emits, and on the one place their
correction of pass B is itself incorrect (`bitmasked_activation` line numbers).
Neither analysis is a superset of the other. Read together they close pass A's
escalation 4 and establish the SNode-ceiling defect as reachable from two
independent directions.
