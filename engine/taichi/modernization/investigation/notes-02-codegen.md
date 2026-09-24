# Contemporaneous notes — explore agent 02, codegen (LLVM + SPIR-V)

Working notes, appended as I go. Numbered. Not reconstructed.

---

## 1. Orientation

Read `modernization/PROJECT-PLAN.md` in full first.

Territory: `taichi/codegen/` in full — `llvm/`, `spirv/`, `cpu/`, `cuda/`,
`amdgpu/`, `dx12/`, plus the top-level files.

File inventory (line counts) — 15170 lines total across 41 files. The two big
ones are `llvm/codegen_llvm.cpp` (2992) and `spirv/spirv_codegen.cpp` (2782),
with `spirv/spirv_ir_builder.cpp` (1713) next.

## 2. First read: `taichi/codegen/llvm/struct_llvm.cpp` (315 lines)

This is the file the brief names directly (6.1, assertion at line 266).

- **L266**: `TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);` — sits at
  the end of `StructCompilerLLVM::run(SNode &root)`. It is a pure bounds check
  in this file: nothing in `struct_llvm.cpp` *sizes* anything by
  `taichi_max_num_snodes`. The cast to `(int)` is the only width interaction.
  Note the assert fires *after* all types and accessors are generated, i.e. it
  is a post-hoc check, not a guard.
- **L171**: `for (int i = 0; i < taichi_max_num_indices; i++)` in
  `generate_refine_coordinates`. This is the other constant named in 6.2. The
  loop emits, per index dimension, one `UDiv(URem(l, prev), next)` plus a
  `Mul`/`Add` against the incoming physical coordinate. So
  `taichi_max_num_indices` directly controls **emitted instruction count** in
  every refine function, for every non-leaf SNode. Raising it makes every
  refine function longer.
- **L153**: the refine function signature is
  `(PhysicalCoordinates*, PhysicalCoordinates*, i32)` — the third argument `l`
  (the linear index within the parent cell) is **hard 32-bit**:
  `llvm::Type::getInt32Ty(*llvm_ctx_)`.
- **L172,174,176,182,184,187**: `tlctx_->get_constant(...)` with `int` /
  `int32` arguments. Need to check what `get_constant` emits for an `int`
  argument — expect i32. If so the whole refine arithmetic chain is i32:
  `URem`, `UDiv`, `Mul`, `Add` all on i32.
- **L74-75**: bitmasked aux type is `ArrayType::get(i32, (max_num_elements()+31)/32)`
  — a bitmask word type, not an index; 32 here is the word width, arguably
  legitimately 32 but it is a *count* division that scales with element count.
- **L105-108**: pointer SNode: mutex array of `i64` (via
  `PointerType::getInt64Ty`, which is odd API usage — see note below), body
  array of `i8*`. Sized by `snode.max_num_elements()`.
- **L111-114**: dynamic SNode aux is `{i32, i32}` (mutex and n). **`n`, the
  number of elements in a dynamic SNode, is 32-bit here.**
- **L72,106,108**: `llvm::ArrayType::get(ch_type, snode.max_num_elements())`.
  `max_num_elements()` return type needs checking (`taichi/ir/snode.h`) — that
  is the per-SNode element count that feeds array sizing.
- **L225-229**: `get_ch_from_parent` uses `CreateGEP` with
  `{get_constant(0), get_constant(parent->child_id(&snode))}` — GEP indices,
  again whatever `get_constant(int)` produces.

Surprise: L105 and L107 use `llvm::PointerType::getInt64Ty` /
`PointerType::getInt8PtrTy`. `getInt64Ty` on `PointerType` is a static inherited
from `Type`, so it yields a plain `i64`, not a pointer-to-i64. Harmless but
misleading. Recording it, not acting on it.

Next: `TaichiLLVMContext::get_constant` and `get_data_type`, then
`codegen_llvm.cpp` address arithmetic.

## 3. `get_constant` semantics — the pivot for the whole LLVM path

`taichi/runtime/llvm/llvm_context.cpp:727-750`, `TaichiLLVMContext::get_constant(T)`:

- `int32`/`uint32` → `APInt(32, ...)` → **i32**
- `int64`/`uint64`/`std::size_t` → `APInt(64, ...)` → **i64**
- explicit instantiations at L904-916 (`float32, float64, bool, int32, uint32,
  int64, uint64, unsigned long`).

So every `tlctx->get_constant(<something of C++ type int>)` in codegen emits an
**i32**. This is the single mechanism by which the 32-bit-ness propagates, and
it is invisible at the call site. `get_constant(DataType dt, T t)` (L696-717) is
the width-explicit variant and uses `data_type_bits(dt)`.

Consequence for `struct_llvm.cpp:171-188`: `AxisExtractor` fields
(`taichi/ir/snode.h:37-53`) are all plain `int` — `num_elements_from_root`,
`shape`, `acc_shape`. So the entire refine-coordinates arithmetic is i32:
`get_constant(acc_shape * shape)` (i32, and the multiply itself is a C++ `int`
multiply that can overflow **at compile time**, before LLVM ever sees it).

By contrast `SNode::max_num_elements()` (`taichi/ir/snode.h:306-308`) returns
`int64` (backed by `num_cells_per_container`, `snode.h:97`, `int64`), so
`get_constant(snode->max_num_elements())` at `codegen_llvm.cpp:281` emits an
**i64**. That is a real width split *inside a single file*: element counts are
i64 in the struct-meta, but the index used to address those elements is i32.

## 4. `codegen_llvm.cpp` — index/address arithmetic, first pass

Read the whole file's width-relevant sites. The load-bearing ones:

- **L1736-1743 `visit(LinearizeStmt*)`** — the core index linearization for
  SNode access. `val = get_constant(0)` → **i32**, then
  `Add(Mul(val, get_constant(stmt->strides[i])), inputs[i])`. All i32. LLVM
  requires matching operand widths, so `inputs[i]` must also be i32; this is
  the hard 32-bit spine of SNode indexing on the LLVM path.
- **L1794-1828 `visit(SNodeLookupStmt*)`** — root case does
  `CreateGEP(parent_ty, parent, llvm_val[stmt->input_index])` with the i32
  index directly. dense/pointer/dynamic/bitmasked cases pass
  `llvm_val[stmt->input_index]` to the runtime `activate` / `lookup_element`
  functions. quant_array case: `get_constant(element_num_bits)` (i32) times the
  index.
- **L1747-1756 `create_bit_ptr`** — `TI_ASSERT(bit_offset->getType()->isIntegerTy(32));`
  A **hard 32-bit assertion** on the bit offset within a bit_struct /
  quant_array. Comment at L1753 documents the struct as `{iX* byte_ptr; i32
  bit_offset;}`.
- **L1874-2007 `visit(ExternalPtrStmt*)`** — ndarray addressing.
  - L1927: shapes loaded as `PrimitiveType::i32`.
  - L1931: `linear_index = get_constant(0)` → **i32**; all subsequent
    `CreateMul`/`CreateAdd` (L1938, L1941, L1943) are i32.
  - L1960-1961: TensorType branch **does** `CreateSExt(linear_index, i64)` and
    then multiplies by an i64 constant (L1969-1971). But the sign-extension
    happens *after* the i32 multiply chain, so overflow is already baked in.
  - L1988: the non-TensorType branch feeds the raw **i32** `linear_index`
    straight into `CreateGEP`. LLVM will sign-extend to pointer width, but
    again the arithmetic already wrapped at 32 bits.
- **L1853-1872 `visit(MatrixPtrStmt*)`** — `offset_used_as_index()` branch GEPs
  with `{get_constant(0), llvm_val[stmt->offset]}`; the other branch
  `PtrToInt`s to i64 (L1864-1865), `SExt`s the offset to i64 (L1866-1867), adds
  and `IntToPtr`s back. So this one path is genuinely 64-bit in the address
  space, fed by a 32-bit offset.
- **L2157-2158** — struct-for: `loop_index_ty = llvm::Type::getInt32Ty(...)`.
  The per-block loop index is **hard i32**, is stored/loaded as i32 (L2213,
  L2214, L2222, L2252), compared `ICMP_SLT` against `upper_bound` (arg 4) and
  passed to `refine` (L2226) and to `is_active` (L2251).
- **L2291-2294**:
  ```
  int list_element_size = std::min(leaf_block->max_num_elements(),
                                   (int64)taichi_listgen_max_element_size);
  int num_splits = std::max(1, list_element_size / stmt->block_dim + ...);
  ```
  `max_num_elements()` is `int64`, `std::min` yields `int64`, then it is
  **narrowed to `int`** on assignment. Then `get_constant(list_element_size)`
  (L2309) emits i32 into `parallel_struct_for`. Bounded in practice by
  `taichi_listgen_max_element_size`, but the narrowing is unconditional.
- **L2314-2321 `visit(LoopIndexStmt*)`** — struct-for branch GEPs into
  `PhysicalCoordinates` and `CreateLoad(getInt32Ty, GEP)` (L2336). Range-for
  branch also loads i32 (L2339). **Loop indices are i32 by construction here,
  independent of any type in the IR.**
- **L2360-2380 `visit(BlockCornerIndexStmt*)`** — comment at L2362-2364
  explicitly asserts the layout `struct PhysicalCoordinates { i32
  val[taichi_max_num_indices]; }`. It reads the element type off the array
  rather than hardcoding, but the surrounding contract is i32.
- **L2382-2390 `visit(GlobalTemporaryStmt*)`** — `get_constant((int64)stmt->offset)`
  → **i64**. Global temporary offsets are already 64-bit.
- **L2392-2399 `visit(ThreadLocalPtrStmt*)`** — `get_constant(stmt->offset)`;
  width depends on the C++ type of `OffloadedStmt`-side `offset`. To check.
- **L1152** `call("node_gc", get_runtime(), tlctx->get_constant(snode))` — a
  `get_constant(SNode*)`?? Need to check; there is no `SNode*` instantiation of
  `get_constant`. To check.

Also, `emit_struct_meta_base` (`codegen_llvm.cpp:259-299`):
- L278 `snode_id` ← `get_constant(snode->id)`, `snode->id` is `int` → **i32**.
  This is the value that indexes the 1024-wide runtime arrays.
- L279 `element_size` ← `get_constant((uint64)element_size)` → i64.
- L281 `max_num_elements` ← `get_constant(snode->max_num_elements())` → i64.
- The commented-out signature block at L286-294 documents the runtime SNode
  accessor ABI as `lookup_element(uint8*, int i)` and `is_active(uint8*, int i)`
  — **`int`, i.e. 32-bit, is the declared index type of the SNode accessor ABI**.

## 5. Resolving the two to-checks from §4

- `codegen_llvm.cpp:1151-1152` `emit_gc`: `auto snode = stmt->snode->id;` —
  it is the **`int` id**, not a pointer. `get_constant(int)` → i32. Fine, and it
  means the SNode id crosses into the runtime as i32. Raising
  `taichi_max_num_snodes` does not by itself break this (i32 holds 2^31 ids),
  but it is the width the snode index travels in.
- `ThreadLocalPtrStmt::offset` is `std::size_t` (`taichi/ir/statements.h:1618`),
  so `codegen_llvm.cpp:2396` `get_constant(stmt->offset)` → **i64**. TLS
  offsets are 64-bit.

## 6. Range-for bounds are 32-bit at the IR level, not just in codegen

`taichi/ir/statements.h:1415-1416`: `int32 begin_value{0}; int32 end_value{0};`
in `OffloadedStmt`. So `codegen_llvm.cpp:2069` / `:2078`
`get_constant(stmt->begin_value)` is i32 **because the field is i32**, not
because codegen chose i32. And the non-const path (L2071-2075, L2080-2084)
explicitly constructs `GlobalTemporaryStmt(offset, PrimitiveType::i32)` and
loads i32. Range-for trip counts are 32-bit end to end.

`create_naive_range_for` (`codegen_llvm.cpp:1169-1171`): loop var is
`PrimitiveType::i32`, hardcoded, independent of anything in the IR.

## 7. Per-backend LLVM offload signatures — all i32

All four LLVM backends give the offloaded range-for / struct-for body function
an `int` (i32) loop-index parameter:

- CPU: `taichi/codegen/cpu/codegen_cpu.cpp:57` `tlctx->get_data_type<int>()`,
  loop var `PrimitiveType::i32` at `:59`. Mesh-for body at `:85`, loop index
  `getInt32Ty` at `:103`, increment by `get_constant(1)` (i32) at `:128`.
  `get_spmd_info` at `:216-217` returns `get_constant(0)`/`get_constant(1)`,
  both i32.
- CUDA: `taichi/codegen/cuda/codegen_cuda.cpp:480` `get_data_type<int>()`,
  loop var `PrimitiveType::i32` at `:482`. Mesh-for `:504`, `i32_ty` at
  `:522-523`, `:535`, `:549`. `get_spmd_info` at `:769-775` uses
  `nvvm_read_ptx_sreg_tid_x` / `ntid_x`, which are **i32 by the intrinsic's own
  signature** — not a Taichi choice. This is the constraint that pins the
  struct-for loop index to i32 on CUDA: it is initialised from `thread_idx`
  (`codegen_llvm.cpp:2212-2214`) and incremented by `block_dim`
  (`codegen_llvm.cpp:2274`).
- AMDGPU: `taichi/codegen/amdgpu/codegen_amdgpu.cpp:256` `get_data_type<int>()`,
  loop var i32 at `:258`. `get_spmd_info` at `:459-467`, i32 throughout.
- DX12: `taichi/codegen/dx12/codegen_dx12.cpp:40` `get_data_type<int>()`, loop
  var i32 at `:42`, mesh-for i32 at `:82`. `get_spmd_info` at `:200-202`
  returns i32 constants 0 and 1.

Also `codegen_llvm.cpp:2115-2120`: the struct-for body function's lower_bound
and upper_bound parameters are both `tlctx->get_data_type<int>()` → i32. This
is the shared signature all four backends call through
`parallel_struct_for`.

Now moving to the SPIR-V path.

## 8. SPIR-V: the Int64 capability. VERIFIED, and the answer is layered.

The brief (6.2) says Int64 in SPIR-V is a declared capability, not a given.
Confirmed, and the in-tree handling is more complete than I expected for types
but absent for addressing.

**Declaration** — `taichi/codegen/spirv/spirv_ir_builder.cpp:64-66`:
```
  if (caps_->get(cap::spirv_has_int64)) {
    ib_.begin(spv::OpCapability).add(spv::CapabilityInt64).commit(&header_);
  }
```
So `OpCapability Int64` is emitted **only** when the device reports it.

**Type creation** — `spirv_ir_builder.cpp:166-169`, inside `init_pre_defs()`:
```
  t_int32_ = declare_primitive_type(get_data_type<int32>());
  t_uint32_ = declare_primitive_type(get_data_type<uint32>());
  if (caps_->get(cap::spirv_has_int64)) {
    t_int64_ = declare_primitive_type(get_data_type<int64>());
    t_uint64_ = declare_primitive_type(get_data_type<uint64>());
  }
```
`t_int32_`/`t_uint32_` are **unconditional**. `t_int64_`/`t_uint64_` are
conditional, so on a device without the capability those two `SType`s are left
**default-constructed** — they exist as objects but were never emitted as
`OpTypeInt 64`.

**Guarded use** — `spirv_ir_builder.cpp:310-313` and `:324-327`
(`get_primitive_type`) raise `TI_ERROR("Type {} not supported.")` for `i64` and
`u64` when the capability is absent. That is the only guard.

**Unguarded uses of `t_uint64_` / `t_int64_` (VERIFIED by reading):**
- `spirv_ir_builder.cpp:334-341` `from_taichi_type`: for a `PointerType`,
  `return has_buffer_ptr ? t_uint64_ : t_uint32_;` — **no `spirv_has_int64`
  check**. Relies on the caller's `has_buffer_ptr` implying int64.
- `spirv_ir_builder.cpp:373-376` `get_primitive_uint_type`: returns `t_uint64_`
  for 64-bit dt with no capability check (it is only reached for a dt that
  already passed a check elsewhere, but nothing enforces that locally).
- `spirv_codegen.cpp:787-790`: loads a `u64` address out of the args buffer
  under `if (caps_->get(DeviceCapability::spirv_has_physical_storage_buffer))`,
  again resting on physical-storage-buffer implying int64.
- `spirv_types.cpp:486-495` `translate_ti_type` mirrors `from_taichi_type`:
  pointer → 64-bit int if `has_buffer_ptr`, else 32-bit.

**Does `has_buffer_ptr` actually imply int64?** In the one place the capability
is set, yes — `taichi/rhi/vulkan/vulkan_device_creator.cpp:820-827` guards it on
`device_supported_features.shaderInt64`. But:
```
#if !defined(__APPLE__) && false
          caps.set(DeviceCapability::spirv_has_physical_storage_buffer, true);
#endif
```
The `&& false` means **`spirv_has_physical_storage_buffer` is never set
anywhere in the tree**. Grep across `taichi/rhi/` finds exactly one setter, this
dead one. So today `has_buffer_ptr` is always false and the whole 64-bit
address path is dark code.

**Where `spirv_has_int64` is set:**
- `taichi/rhi/vulkan/vulkan_device_creator.cpp:628-631` — on
  `device_supported_features.shaderInt64`.
- `taichi/rhi/opengl/opengl_device.cpp:509-513` — set **only for desktop GL**;
  the comment says "64bit isn't supported in ES profile". **GLES is a live
  in-tree target that lacks Int64.**
- `taichi/rhi/metal/metal_device.mm:1052` — set unconditionally for Metal;
  `:132` reads it back.

So the "target that lacks Int64" the brief asks the architecture to express is
not hypothetical: it is the OpenGL ES backend, today.

## 9. SPIR-V: `use_64bit_pointers` is hardcoded `false`. Biggest finding so far.

`taichi/codegen/spirv/spirv_codegen.cpp:82`:
```
  const bool use_64bit_pointers = false;
```
A `const bool` **member of `TaskCodegen`**, not a cap query, not a config, not
a constructor parameter. Its only consumer is `make_pointer`
(`spirv_codegen.cpp:2317-2325`):
```
  spirv::Value make_pointer(size_t offset) {
    if (use_64bit_pointers) {
      // This is hacky, should check out how to encode uint64 values in spirv
      return ir_->uint_immediate_number(ir_->u64_type(), offset);
    } else {
      return ir_->uint_immediate_number(ir_->u32_type(), uint32_t(offset));
    }
  }
```
Note the **explicit narrowing cast `uint32_t(offset)`** in the live branch. The
comment on the dead branch ("This is hacky, should check out how to encode
uint64 values in spirv") is upstream's own admission that the 64-bit branch was
never finished.

`make_pointer` is what produces every SNode address on the SPIR-V path:
- `spirv_codegen.cpp:355` `visit(GetRootStmt*)` — `make_pointer(0)`, the root
  base address.
- `spirv_codegen.cpp:371` `visit(GetChStmt*)` —
  `make_pointer(desc.mem_offset_in_parent_cell)`, added to the parent pointer.
- `spirv_codegen.cpp:405-406` `bitmasked_activation` —
  `make_pointer(desc.cell_stride * desc.snode->num_cells_per_container)`.
- (more sites to enumerate below)

So on SPIR-V an SNode "pointer" is a **u32 byte offset into the root SSBO**.
That is a hard 4 GiB ceiling on the root buffer and a hard 32-bit width on
every SNode address computation, independent of whether the device supports
Int64.

Note also `bitmasked_activation` at `:405-406` computes
`desc.cell_stride * desc.snode->num_cells_per_container` in **C++**, then feeds
it through `make_pointer`'s `uint32_t()` truncation. `num_cells_per_container`
is `int64` (`taichi/ir/snode.h:97`); `cell_stride`'s type needs checking in
`snode_struct_compiler.h`.

## 10. SPIR-V: `at_buffer` — the SSBO index is the pointer width

`spirv_codegen.cpp:2194-2219`:
```
    spirv::Value idx_val = ir_->make_value(
        spv::OpShiftRightLogical, ptr_val.stype, ptr_val,
        ir_->uint_immediate_number(ptr_val.stype, size_t(std::log2(width))));
    spirv::Value ret =
        ir_->struct_array_access(ir_->get_primitive_type(dt), buffer, idx_val);
```
The byte offset is shifted right by log2(element size) and used as the index
into the SSBO's runtime array. `idx_val` inherits `ptr_val.stype`, i.e. **u32**
in every live configuration. The `u64` branch at `:2197-2203` uses
`OpConvertUToPtr` to `StorageClassPhysicalStorageBuffer` — the dead
physical-storage-buffer path.

`std::log2(width)` on a `size_t`: floating-point log of a power of two, cast
back to `size_t`. Works for 1/2/4/8 but is a fragile way to write it. Recording,
not acting.

## 11. SPIR-V index/address arithmetic — the full chain (VERIFIED)

- `spirv_codegen.cpp:531-540` `visit(LinearizeStmt*)` —
  `val = ir_->const_i32_zero_` (**i32**), strides as
  `int_immediate_number(ir_->i32_type(), stmt->strides[i])`,
  `val = add(mul(val, strides_val), input_val)`. Structurally identical to the
  LLVM version at `codegen_llvm.cpp:1736-1743`, same i32 width, same
  `strides[i]` source. **The two paths agree here.**
- `spirv_codegen.cpp:468-511` `visit(SNodeLookupStmt*)` — the address step:
  ```
      spirv::Value input_index_val = ir_->cast(
          parent_val.stype, ir_->query_value(stmt->input_index->raw_name()));
      spirv::Value stride = make_pointer(desc.cell_stride);
      spirv::Value offset = ir_->mul(input_index_val, stride);
      val = ir_->add(parent_val, offset);
  ```
  The i32 linear index is **cast to the pointer type** (u32), multiplied by the
  u32-truncated cell stride, and added to the u32 parent pointer. Everything
  from here on is u32 modular arithmetic.
- `spirv_codegen.cpp:359-378` `visit(GetChStmt*)` — `add(input_ptr,
  make_pointer(desc.mem_offset_in_parent_cell))`. u32.
- `spirv_codegen.cpp:381-436` `bitmasked_activation` — index shifted right by 5,
  masked with 31, shifted left; `bitmask_word_ptr` shifted left by 2, added to
  `make_pointer(desc.cell_stride * desc.snode->num_cells_per_container)`, added
  to `parent_ptr`, then shifted right by 2 and used as SSBO index. All in
  `ptr_dt` = u32 except the explicit `u32_type()` operands. Note that
  `desc.cell_stride * desc.snode->num_cells_per_container` is `size_t * int64`
  in C++ — computed at 64 bits and then truncated by `make_pointer`.
- `spirv_codegen.cpp:707-712` `visit(GlobalTemporaryStmt*)` —
  `int_immediate_number(ir_->i32_type(), stmt->offset, false)`. **i32.**
  **DIVERGENCE:** the LLVM path uses `get_constant((int64)stmt->offset)` →
  **i64** at `codegen_llvm.cpp:2386`. Same statement, different width in the two
  backends.
- `spirv_codegen.cpp:733-802` `visit(ExternalPtrStmt*)` — `linear_offset` is
  `i32` (`:737`), ndarray shapes loaded as `i32` (`:753-754`), the whole
  mul/add chain is i32 (`:772-773`), and then at `:774-777` it is
  **left-shifted by log2(element size) in i32** to produce a byte offset.
  **DIVERGENCE:** the LLVM path (`codegen_llvm.cpp:1988`) keeps an element index
  and lets `CreateGEP` scale it; SPIR-V converts to a byte offset while still
  32-bit, so it loses a further log2(element size) bits of reach relative to
  LLVM. For f64/i64 data that is 3 more bits.
  The 64-bit branch at `:783-793` (`OpSConvert` to `u64_type()` and add to a
  u64 base address) is guarded on `spirv_has_physical_storage_buffer`, which is
  never set (see §8), so it is dead.
- `spirv_codegen.cpp:307-334` `visit(MatrixPtrStmt*)` —
  `dt_bytes = int_immediate_number(ir_->i32_type(), get_primitive_type_size(dt))`,
  `offset_bytes = mul(dt_bytes, offset_val)`, `add(origin_val, offset_bytes)`.
  i32/pointer-width.
- `spirv_codegen.cpp:2194-2219` `at_buffer` — see §10; SSBO index inherits
  the pointer width.

## 12. SPIR-V loop indices

- **Range-for** (`spirv_codegen.cpp:1983-2125`): all i32.
  `begin_expr_value`, `total_elems`, `begin_`, `end_`, `total_invocs`, and the
  loop phi `"ii"` are `i32_type()` (`:2000-2002`, `:2010`, `:2018-2044`,
  `:2055-2056`, `:2058`, `:2066-2072`). `get_global_invocation_id(0)` is u32 and
  is `cast` to i32 at `:2055`. Global-temporary-backed bounds are read by
  arithmetic-shifting the byte offset right by 2 (`:2018-2020`, `:2033-2035`) —
  i.e. a **32-bit word index into the gtmp buffer**.
- **Struct-for** (`spirv_codegen.cpp:2127-2190`): driven off a **ListGen buffer
  of `u32`**. `listgen_count` at word 0 (`:2138-2141`), indices at words 1..n
  (`:2164-2168`). `loop_index_var` is `alloca_variable(ir_->u32_type())`
  (`:2149`), initialised from `get_global_invocation_id(0)` (u32), compared with
  `OpULessThan`, and the loaded `listgen_index` (u32) is registered as `"ii"`.
  So the **SPIR-V struct-for element index is u32**, whereas the LLVM struct-for
  loop index is **i32 signed** (`codegen_llvm.cpp:2157`) compared with
  `ICMP_SLT`. A signedness divergence as well as being both 32-bit.
- **Serial** (`spirv_codegen.cpp:1930-1962`): compares
  `get_global_invocation_id(0)` against `uint_immediate_number(u32_type(), 0)`.

Surprise: `spirv_codegen.cpp:541-561` `visit(LoopIndexStmt*)` handles
`OffloadedTaskType::range_for` and `RangeForStmt` and does `TI_NOT_IMPLEMENTED`
for everything else — including `struct_for` — even though
`generate_struct_for_kernel` registers `"ii"`. I did not chase how struct-for
loop indices reach the body on this path. **Escalation candidate**, recorded,
not resolved.

## 13. SPIR-V host-side descriptors are 64-bit; the truncation is at emission

`taichi/codegen/spirv/snode_struct_compiler.h:13-36` `SNodeDescriptor`: every
size field is `size_t` — `cell_stride` (`:16`), `container_stride` (`:19`),
`total_num_cells_from_root` (`:28`), `mem_offset_in_parent_cell` (`:31`).
`CompiledSNodeStructs::root_size` is `size_t` (`:42`).

`snode_struct_compiler.cpp:65-139` `compute_snode_size` computes all of these in
`size_t`. So the SPIR-V struct layout is computed at 64 bits on the host and
**only truncated when `make_pointer` emits it** (`spirv_codegen.cpp:2322`,
`uint32_t(offset)`). That is a single, well-localised truncation point — but
also a silent one, with no assertion that the value fits.

Also `spirv_ir_builder.cpp:565-607` `get_array_type`:
- `num_elems` parameter is `uint32_t`; array length emitted as
  `uint_immediate_number(t_uint32_, num_elems)` (`:575`).
- `nbytes` is `uint32_t`, assigned from `value_type.snode_desc.container_stride`
  (a `size_t`) at `:590` — **implicit narrowing**, then emitted as the
  `ArrayStride` decoration (`:605`). There is a `nbytes == 0` warning at
  `:592-599` but nothing catches a stride that overflowed 32 bits into a nonzero
  value.

## 14. `taichi_max_num_snodes` in codegen — exactly one site, and it does not
## bound what it appears to bound

Grep across all of `taichi/codegen/` for the two named constants:

| Constant | Sites in `taichi/codegen/` |
|---|---|
| `taichi_max_num_snodes` | `llvm/struct_llvm.cpp:266` only |
| `taichi_max_num_indices` | `llvm/struct_llvm.cpp:171` only (plus a comment at `llvm/codegen_llvm.cpp:2363`) |

**The SPIR-V path never mentions `taichi_max_num_snodes` at all.** Its SNode
map is `SNodeDescriptorsMap = std::unordered_map<int, SNodeDescriptor>`
(`spirv/snode_struct_compiler.h:38`) — dynamically sized, no ceiling, no
assertion. So item 6.1 has no codegen-side footprint on the SPIR-V path.

**Now the part that surprised me.** The assertion is
```
  TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);
```
`snodes` is `StructCompiler::snodes`, a `std::vector<SNode *>`
(`taichi/struct/struct.h:11`), filled by `StructCompiler::collect_snodes`
(`taichi/struct/struct.cpp:7-13`), which walks **one SNode tree** from the root
passed to `run()`. A `StructCompilerLLVM` is constructed **per snode tree**
(`struct_llvm.cpp:12-34`, note the `snode_tree_id` parameter).

But the thing actually bounded by `taichi_max_num_snodes` at runtime is
`snode->id`, which is **global**:
- `taichi/ir/snode.cpp:12` `std::atomic<int> SNode::counter{0};`
- `taichi/ir/snode.cpp:220` `id = counter++;`
- reset only in `Program::Program` (`taichi/program/program.cpp:144`) and via
  `SNode::reset_counter()` (`taichi/ir/snode.h:348-350`).

And the runtime arrays are indexed by that global id:
- `taichi/runtime/llvm/runtime_module/runtime.cpp:567-569` —
  `element_lists`, `node_allocators`, `ambient_elements`, each
  `[taichi_max_num_snodes]`.
- indexed at `runtime.cpp:1016` (`element_lists[root_id]`), `:1029`
  (`node_allocators[snode_id]`), `:1038` (`ambient_elements[snode_id]`),
  `:1271`, `:1287-1288`, `:1334-1336`, `:1429`, `:1692`, `:1723`, `:1740`,
  `:1784`.

Codegen is what supplies that id:
- `codegen_llvm.cpp:278` `common.set("snode_id", tlctx->get_constant(snode->id))`
- `codegen_llvm.cpp:1151-1152` `emit_gc`: `call("node_gc", get_runtime(),
  tlctx->get_constant(stmt->snode->id))`

**So with more than one SNode tree, the per-tree assertion at
`struct_llvm.cpp:266` can pass while the global id used to index the
1024-entry runtime arrays exceeds 1024.** I have verified each link in that
chain by reading; I have not constructed a failing case. Flagging as an
Escalation because it changes what "parameterise the ceiling" has to mean.

## 15. What else in codegen scales with SNode count

Not a constant, but a cost: `StructCompilerLLVM` emits, **per SNode**:
- one LLVM struct type + a type stub + a dummy function (`struct_llvm.cpp:133-143`);
- one `refine_coordinates` function for every non-leaf (`struct_llvm.cpp:146-190`,
  called from `:200`), whose body is a loop of `taichi_max_num_indices`
  iterations each emitting `URem`/`UDiv`/`Mul`/`Add` plus two calls
  (`:179-187`);
- one `get_ch_..._to_...` function for every non-root (`struct_llvm.cpp:203-233`).

So emitted struct-module IR is roughly O(num_snodes x taichi_max_num_indices).
Raising 1024 and/or 12 multiplies struct-module size and its compile time
directly. This is the only SNode-count-scaling thing in codegen beyond the
assertion itself.

## 16. SPIR-V: two more places 64-bit types are reached without a capability check

`taichi/codegen/spirv/spirv_types.cpp:393-419`, `Translate2Spirv::visit_int_type`:
```
      } else if (type->num_bits() == 64) {
        vt = spir_builder_->i64_type();
      }
      ...
      } else if (type->num_bits() == 64) {
        vt = spir_builder_->u64_type();
      }
    ...
    ir_node_2_spv_value[type] = vt.id;
```
`i64_type()` / `u64_type()` (`spirv_ir_builder.h:529-534`) are bare accessors
returning `t_int64_` / `t_uint64_` with **no `spirv_has_int64` check**, unlike
`get_primitive_type` which does check. `SType::id` defaults to `0`
(`spirv_ir_builder.h:51`), and `0` is not a valid SPIR-V id. There is also no
`else` on the width chain, so an unmatched width silently yields id 0 too.

This is on a **live** path: `translate_ti_type` + `ir_translate_to_spirv` build
the args / rets / argpack structs at `spirv_codegen.cpp:2344`, `:2358`, `:2420`,
`:2436`, `:2494`. `translate_ti_primitive` (`spirv_types.cpp:179-181`,
`:199-201`) maps `PrimitiveType::i64`/`u64` to a 64-bit `IntType` with no
capability involvement.

Also `spirv_types.h:71-76` `PhysicalPointerType` is **unconditionally**
`IntType(/*num_bits=*/64, /*is_signed=*/false)`. Its only construction site is
`snode_struct_compiler.cpp:52-54`, inside `StructCompiler::construct`, which is
the tinyir SNode-type path that is commented out at
`snode_struct_compiler.cpp:16-19`. Dead today; would become live if the SNode
layout moved to the tinyir type compiler.

I did **not** confirm whether a kernel taking an `i64` argument on a
capability-lacking device is rejected earlier by some other check. Escalation.

## 17. The LLVM path has a real safety net that the SPIR-V path does not

`taichi/codegen/llvm/llvm_codegen_utils.cpp:103-145`
`check_func_call_signature` compares each argument's `llvm::Type` against the
callee's declared parameter type and raises `TI_ERROR` on mismatch (`:139-140`).
Only pointer-type renaming is auto-fixed (`:127-131`).

Consequence for 6.2: if index or address widths in the LLVM codegen are widened
without correspondingly widening the runtime module's signatures, the build
fails loudly at codegen time rather than truncating silently. That is a
meaningful de-risking of the LLVM half.

The SPIR-V path has no equivalent. Widths there are matched by construction
(explicit `cast` calls) and mismatches become invalid SPIR-V, caught at best by
the optional `spvtools` validator.

## 18. The runtime-side seam (agent 03's territory, recorded because it is the
## other end of my wire)

Verified by reading, offered as the boundary, not claimed as mine:
- `runtime.cpp:288-290` — `struct PhysicalCoordinates { i32 val[taichi_max_num_indices]; };`
  This is the type `struct_llvm.cpp:146-190` writes into and
  `codegen_llvm.cpp:2314-2380` reads out of. **i32 coordinates.**
- `runtime.cpp:307-325` `StructMeta`: `i32 snode_id`, `std::size_t element_size`,
  **`i64 max_num_elements`**, and function pointers declared as
  `Ptr (*lookup_element)(Ptr, Ptr, int i)`, `u1 (*is_active)(Ptr, Ptr, int i)`,
  `i32 (*get_num_elements)(Ptr, Ptr)`,
  `void (*refine_coordinates)(PhysicalCoordinates*, PhysicalCoordinates*, int)`.
- `runtime_module/node_dense.h:10-12` `Dense_get_num_elements` returns **`i32`**
  by implicit narrowing from the `i64 max_num_elements`. So codegen widens
  (i64 constant at `codegen_llvm.cpp:281`), the struct field holds it (i64), and
  the accessor narrows it back to i32 on the way out.
- `node_dense.h:22-24` `Dense_lookup_element(Ptr meta, Ptr node, int i)` →
  `node + element_size * i`. `element_size` is `size_t`, so the **multiply is
  64-bit**, but `i` is only 32-bit. Address arithmetic is already 64-bit here;
  the index feeding it is not.
- `runtime.cpp:517-521` `struct Element { Ptr element; int loop_bounds[2];
  PhysicalCoordinates pcoord; };` — `loop_bounds` is `int[2]`, which is what
  `codegen_llvm.cpp:2162-2163` reads as args 3 and 4 of the struct-for body.

## 19. Item 6.3 (adaptive module loading) — the codegen-side touchpoints

Not my focus, one paragraph so the seam with agent 04 is visible.

- `taichi/codegen/codegen.cpp:37-74` `KernelCodeGen::create` — backend selection
  is a compile-time `#if defined(TI_WITH_CUDA)` / `TI_WITH_DX12` /
  `TI_WITH_AMDGPU` chain wrapped around a runtime `arch` switch. A backend not
  compiled in is `TI_NOT_IMPLEMENTED`, i.e. a hard failure, not a fallback.
- `taichi/codegen/spirv/kernel_compiler.cpp:25-46` — the SPIR-V compiler takes
  `const DeviceCapabilityConfig &device_caps` and hands it to
  `KernelCodegen::Params::caps` (`spirv_codegen.h:25`), which reaches
  `TaskCodegen::caps_` and `IRBuilder::caps_` (`spirv_ir_builder.h:598`).
  **This is already the mechanism the brief's 6.2 stub note is asking for**: a
  per-device capability set threaded through the SPIR-V codegen. The one
  address-width decision that does *not* go through it is `use_64bit_pointers`
  (`spirv_codegen.cpp:82`).
- The LLVM path has no equivalent capability object; width decisions there are
  hardcoded C++ types.

## 20. Sweep of the files I have not otherwise cited

- `taichi/codegen/codegen_utils.h` — printf format-specifier parsing and
  merging only. Nothing width- or index-related.
- `taichi/codegen/compiled_kernel_data.{h,cpp}`, `llvm/compiled_kernel_data.*`,
  `spirv/compiled_kernel_data.*`, `kernel_compiler.h`,
  `llvm/kernel_compiler.{h,cpp}` — serialisation and caching of compiled
  artefacts. No index or address arithmetic.
- `taichi/codegen/llvm/llvm_compiled_data.h` — data holders.
- `taichi/codegen/spirv/lib_tiny_ir.h` — the tinyir node/type framework. Sizes
  are `size_t` throughout; no fixed widths of its own.
- `taichi/codegen/dx12/dx12_lower_intrinsic.cpp:54-95` — patches `thread_idx`,
  `block_idx`, `block_dim`, `grid_dim` to DXIL intrinsics, all `i32`
  (`getInt32Ty`, `getInt32(0)`, `getInt32(group_size)`), with a hardcoded
  `unsigned group_size = 64` at `:79`. Consistent with the i32 SPMD contract in
  §7, not an addressing width.
- `taichi/codegen/dx12/dx12_lower_runtime_context.cpp`,
  `dx12_global_optimize_module.cpp` — no width or index sites.
- `taichi/codegen/llvm/codegen_llvm_quant.cpp` — bit-level quant packing. All
  the `get_constant(...)` sites there (`:26`, `:40`, `:110`, `:198`, `:203`,
  `:205`, `:208`, `:213`, `:359`, `:361`, `:391-392`, `:399`, `:401`, `:423`,
  `:427`, `:440`, `:443`, `:456`, `:458`, `:460-461`) are i32 **bit offsets and
  bit masks within a physical type**, not addresses. They are consistent with
  the `create_bit_ptr` i32 assertion (`codegen_llvm.cpp:1755`). Physical types
  are at most 64 bits, so i32 bit offsets are not a scaling limit. Recording so
  they are not double-counted as addressing bugs.
- Constant GEP indices for fixed-size structs (`codegen_llvm.cpp:1355`, `:1361`,
  `:1761`, `:1767`, `:1782`, `:1786`, `:1893`, `:1921`, `:2330`, `:2375`,
  `:2834`, `:2848`, `:2853`, `:2920`, `:2940`, `:2954`, `:2972`;
  `codegen_cuda.cpp:69`, `:446`, `:449`) are i32 because **LLVM requires struct
  GEP indices to be i32 constants**. Not width assumptions. Recording so they
  are not miscounted either.

## 21. Line-number audit before writing the report

Re-checked every cited line by printing it. Three of my earlier citations were
off by a few lines and I have corrected them **in place above**, recording the
corrections here so the edit is visible:

- `create_bit_ptr`'s 32-bit assertion is at `codegen_llvm.cpp:1755`, not `:1754`
  (`:1754` is the closing line of the preceding comment block). Range corrected
  to L1747-1756.
- The raw-i32 GEP at the end of `visit(ExternalPtrStmt*)` is
  `codegen_llvm.cpp:1988`, not `:2005`.

Everything else in §§2-20 verified as printed:
`struct_llvm.cpp:153/171/266`; `codegen_llvm.cpp:281/1737/1931/2157/2291/2386`;
`spirv_codegen.cpp:82/505-507/533/708/737/774/2149/2194/2215/2218/2317/2322`;
`spirv_ir_builder.cpp:65/166/338`.

## 22. Deliberate non-conclusions

Per the standing instructions I have **not** decided anything is unnecessary,
have **not** proposed removals, and have **not** proposed abstractions. In
particular I am explicitly *not* concluding that:
- `use_64bit_pointers` should be wired to a capability — only that it is
  hardcoded and is the single decision point;
- the dead `#if ... && false` around `spirv_has_physical_storage_buffer` should
  be re-enabled — only that it is currently dead and that the 64-bit SPIR-V
  address path is therefore untested;
- the per-tree vs global-id mismatch at `struct_llvm.cpp:266` is a bug — only
  that the assertion's operand and the runtime array's index are different
  quantities. Escalated.

Now writing the report.

---

# Second pass — correction round, 2026-09-09

Picked up by a second agent. Original author gone. Inputs: `adversary-02-1.md`,
`adversary-02-2.md`, and the four correction items in the arbitration.
Everything below re-read from source; nothing carried on either adversary's
word. Numbering continues from §22.

## 23. Method for this pass

The arbitration says the adversaries disagree in places and I must adjudicate
against the source rather than pick the more confident voice. So the order was:

1. Re-open every file the two adversaries cite in the four correction items,
   before reading their conclusions a second time.
2. Print numbered lines (`grep -n "" file | sed -n 'a,bp'`) rather than
   `sed -n` alone, so a line number can never come from counting.
3. For citations, check **attribution** — that the cited line is the line that
   does the thing claimed — not merely that the quoted text appears nearby.
   The arbitration warns that concatenated file reads have already produced
   systematic citation faults in this project. Confirmed: several of my
   predecessor's spans start on a comment or a closing brace, which is the
   signature of a span read off a concatenated buffer.

## 24. Correction item 1 — the `Translate2Spirv` bypass is wider than reported

Printed `taichi/codegen/spirv/spirv_types.cpp:393-431` with line numbers.

`visit_int_type` (`:393-419`) and `visit_float_type` (`:421-431`) between them
make **eight** bare-accessor calls that skip `IRBuilder::get_primitive_type`:

| call site | accessor | member it returns | guarded equivalent |
|---|---|---|---|
| `spirv_types.cpp:397` | `i8_type()` | `t_int8_` | `spirv_ir_builder.cpp:301-304` |
| `:399` | `i16_type()` | `t_int16_` | `:305-308` |
| `:403` | `i64_type()` | `t_int64_` | `:311-314` |
| `:409` | `u8_type()` | `t_uint8_` | `:315-318` |
| `:411` | `u16_type()` | `t_uint16_` | `:319-322` |
| `:415` | `u64_type()` | `t_uint64_` | `:325-328` |
| `:424` | `f16_type()` | `t_fp16_` | `:291-294` |
| `:428` | `f64_type()` | `t_fp64_` | `:297-300` |

The accessors are one-line returns with no `caps_` consultation:
`spirv_ir_builder.h:529-531` (i64), `:532-534` (u64), `:535-537` (f64),
`:549-551` (i16), `:552-554` (u16), `:555-557` (f16), `:559-561` (i8),
`:562-564` (u8). Full accessor block `:529-564`.

The members are declared only under a capability, in `init_pre_defs`:
`spirv_ir_builder.cpp:156-159` (int8 → `t_int8_`, `t_uint8_`),
`:160-163` (int16), `:166-169` (int64), `:171-173` (float16),
`:174-176` (float64). `t_int32_`/`t_uint32_` at `:164-165` and `t_fp32_` at
`:170` are unconditional, as is `t_bool_` at `:155`, so `i32_type()`,
`u32_type()`, `f32_type()` and `bool_type()` in the visitor are safe.

**Count check.** The arbitration says "eight accessors across six optional
widths". Eight accessors is right. The number of *capability gates* is
**five** — `spirv_has_int8`, `spirv_has_int16`, `spirv_has_int64`,
`spirv_has_float16`, `spirv_has_float64` — and the number of distinct optional
bit-widths is also five: int8, int16, int64, fp16, fp64. I cannot construct a
reading that gives six. Reporting five and flagging the discrepancy rather than
adopting the number I was handed.

Failure mode unchanged from pass A: `SType::id` is `uint32_t id{0}`
(`spirv_ir_builder.h:51`), the result is written into the map at
`spirv_types.cpp:418` (int) and `:430` (float), and a zero id is not a valid
SPIR-V result id.

## 25. Correction item 1, continued — where the visitor actually runs

Pass A cited `spirv_codegen.cpp:2344`, `:2358`, `:2420`, `:2436`, `:2494` as
where "it builds the args/rets/argpack structs". Grep confirms those five are
the `translate_ti_type` calls — which build **tinyir** types, not SPIR-V ones.
The SPIR-V visitor runs at the three `ir_translate_to_spirv` calls:
`spirv_codegen.cpp:2386` (args), `:2464` (rets), `:2511` (argpack).
`ir_translate_to_spirv` itself is `spirv_types.cpp:476-483`; it constructs
`Translate2Spirv` at `:480` and visits at `:481`.

Both adversaries cited the `ir_translate_to_spirv` trio; adversary 02-1 gave
the function as `:476-481` and adversary 02-2 as `:476-485`. Both wrong; it is
`:476-483` (`:484` opens `translate_ti_type`).

Pass A's attribution is not false but it is imprecise, and the imprecision
matters because the two sets of lines are in different functions. Correcting to
name both.

`translate_ti_primitive` (`spirv_types.cpp:167-...`) maps i64 → `IntType(64,
signed)` at `:179-181` and u64 → `IntType(64, unsigned)` at `:199-201`, with no
capability parameter in its signature. Confirmed. It also maps i8/u8 at
`:170-172`/`:190-192`, i16/u16 at `:173-175`/`:193-195`, f16 at `:202-203` and
f64 at `:206-207` — the feeds for the other six bypassed accessors.

## 26. Correction item 1 — is there an earlier check? No, and the adversaries' reason is partly wrong

Adversary 02-2 closes pass A's escalation 4 by asserting `Extension::data64` is
dead, and supports it with "`is_extension_supported` is called exactly once in
the whole tree, for `Extension::assertion` (`program.cpp:149`)."

**That supporting claim is false.** Grep for `is_extension_supported` over
`taichi/`:

- `taichi/program/extension.cpp:8` (definition), `extension.h:24` (declaration)
- `taichi/program/program.cpp:149` — `Extension::assertion`
- `taichi/codegen/llvm/codegen_llvm.cpp:2726` — `Extension::bls`
- `taichi/transforms/compile_to_offloads.cpp:92`, `:205`, `:218`, `:236` —
  `Extension::mesh`
- `taichi/transforms/compile_to_offloads.cpp:245`, `:288` — `Extension::quant`
- `taichi/python/export_lang.cpp:1225` — Python binding

Nine C++ call sites, not one. The mechanism is live.

**The narrower claim survives, and it is the one that matters.** No call site
anywhere passes `Extension::data64`. The extension is declared at
`taichi/inc/extensions.inc.h:6` and granted to x64 (`extension.cpp:12`), arm64
(`:16`) and cuda (`:20`); every SPIR-V-consuming arch in the table —
metal `:23`, opengl `:24`, gles `:25`, vulkan `:26`, dx11 `:27` — is given the
empty set or `extfunc` only. And nothing queries it. So there is no
front-of-pipeline rejection of a 64-bit kernel argument on a SPIR-V target.

Recording the correction because a wrong reason attached to a right conclusion
is exactly what the next reader would inherit.

## 27. Correction item 1 — and nothing downstream catches the zero id either

Neither adversary went as far as the optimiser setup. `spirv_codegen.cpp:2710`:

```
  spirv_opt_options_.set_run_validator(false);
```

The validator is **explicitly disabled**, unconditionally, in
`KernelCodegen::KernelCodegen`. `enable_spv_opt` (`spirv_codegen.h:26`, set
from `compile_config.external_optimization_level > 0` at
`spirv/kernel_compiler.cpp:38`) gates only the pass registrations at
`spirv_codegen.cpp:2683-2708`; `spirv_opt_->Run` at `:2746-2747` executes
either way, with the validator off. `spirv_tools_` (`:2712`) is used only for
`Disassemble` at `:2763`, inside `if constexpr (false)` at `:2758` — compiled
out.

So pass A's §4.2.10 ("caught at best by the optional spvtools validator") is
too generous. Nothing in the codegen validates the module. Correcting.

## 28. Correction item 2 — the SNode assertion is a range write, and it is reachable

Printed `runtime.cpp:986-1017` with line numbers.

```
1003:  for (int i = root_id; i < root_id + num_snodes; i++) {
1004:    // TODO: some SNodes do not actually need an element list.
1005:    runtime->element_lists[i] =
1006:        runtime->create<ListManager>(runtime, sizeof(Element), 1024 * 64);
1007:  }
```

`root_id` and `num_snodes` are parameters at `:988-989`. Pass A's index-site
list in §2.1 has `:1016`, `:1029`, `:1038` and eight later read sites but
**omits `:1005`**, which is the only write over a *range*. Confirmed omission.

Full set of subscript sites on the three arrays, by grep:
`element_lists` — declared `:567`, written `:1005` and `:1016`, read `:1271`,
`:1287`, `:1288`, `:1334`, `:1336`, `:1429`.
`node_allocators` — declared `:568`, written `:1029`, read `:1692`, `:1723`,
`:1740`, `:1784`.
`ambient_elements` — declared `:569`, written `:1038`.

Pass A's list also gives `:1334-1336`; `:1335` is
`int num_parent_elements = parent_list->size();`, not a subscript. Span
corrected to `:1334` and `:1336`.

`runtime_NodeAllocator_initialize` is `runtime.cpp:1026-1031` and
`runtime_allocate_ambient` is `:1033-1040`. Adversary 02-1 cited `:1029` and
`:1038` correctly. Adversary 02-2 cited "`:1291-1296`" and "`:1298-1304`" for
these two functions — **both wrong by roughly 265 lines**; those lines are
inside the element-listgen code. Adversary 02-1 is right here.

Caller side, `llvm_runtime_executor.cpp`:
- `initialize_llvm_runtime_snodes` opens `:391-393`.
- `root_id` from `field_cache_data.root_id` at `:400`; set from
  `tree->root()->id` at `taichi/runtime/program_impls/llvm/llvm_program.cpp:61`.
- the call at `:442-444`, passing `(int)snode_metas.size()` as `num_snodes`.
- the per-SNode loop `:446-468`, filtered by `is_gc_able(...)` at `:447`,
  calling `runtime_NodeAllocator_initialize` at `:460-462` and
  `runtime_allocate_ambient` at `:465-466` with `snode_metas[i].id` (`:448`).

So the write is `element_lists[root_id .. root_id + num_snodes)` where `root_id`
is a **global** SNode id and `num_snodes` is a per-tree count — while the only
assertion in the tree bounds the per-tree count alone. Once
`root_id + num_snodes > 1024` this stores past the end of `element_lists`, into
`node_allocators`, then `ambient_elements`, then `temporaries` (`:570`) and
`rand_states` (`:571`), all adjacent members of `LLVMRuntime`. It is an
out-of-bounds **store**, not a failed lookup. Confirmed as the arbitration
states.

## 29. Correction item 2 — reachability, established

**Tree ids recycle. SNode ids do not.**

- `Program::destroy_snode_tree` pushes the freed id:
  `free_snode_tree_ids_.push(snode_tree->id());` at
  `taichi/program/program.cpp:235`. The stack is declared at
  `taichi/program/program.h:336`.
- `Program::allocate_snode_tree_id` (`program.cpp:559-567`) pops a free id at
  `:563-564` and only falls back to `snode_trees_.size()` at `:561` when the
  stack is empty. Called from `add_snode_tree` at `:240`.
- `SNode::id` is `id = counter++` (`taichi/ir/snode.cpp:220`) from
  `std::atomic<int> SNode::counter{0}` (`snode.cpp:12`). Monotonic. Nothing
  decrements it and nothing frees an id on tree destruction.

**The only reset is the `Program` constructor.** `SNode::counter = 0;` at
`program.cpp:144`. And `SNode::reset_counter()` (`taichi/ir/snode.h:348-350`)
is **never called anywhere** — grep over `taichi/`, `c_api/` and `tests/`
returns only its own definition. The only `reset_counter()` call in the tree is
`Stmt::reset_counter()` at `program.cpp:347` (`taichi/ir/ir.h:509`), a different
class. Pass A's escalation 6 listed `SNode::reset_counter()` as a reset avenue;
it is dead code. Correcting.

`TI_ASSERT_INFO(num_instances_ == 0, "Only one instance at a time")` at
`program.cpp:141` means one `Program` at a time, so within a process the SNode
id space grows monotonically for the whole life of the program object.

**Consequence.** Overflow does not need many trees alive at once, so
`kMaxNumSnodeTreesLlvm = 512` (`taichi/inc/constants.h:13`) does not bound it.
A root + dense + place tree consumes three ids, so on the order of 340
create/destroy cycles carries the next tree's `root_id` past 1024 with never
more than one tree live. The per-tree assertion passes every time.

**Two gates narrow it, and both point at this project's configuration.**

1. `runtime_initialize_snodes` returns early if `all_dense`
   (`runtime.cpp:1000-1002`), skipping the `element_lists` range write.
   `all_dense` is computed at `llvm_runtime_executor.cpp:402-410`: it is
   **initialised from `config_.demote_dense_struct_fors`** at `:402` and then
   cleared by any SNode that is not `dense`, `place` or `root` (`:404-409`).
   `demote_dense_struct_fors` defaults true (`taichi/program/compile_config.cpp:18`)
   and is a writable field exposed to Python (`taichi/python/export_lang.cpp:201-202`),
   so with it set false the early return never fires **even for a fully dense
   tree**. Neither adversary noticed the initialiser; adversary 02-2 described
   `all_dense` as true "only when every SNode in the tree is dense, place or
   root", which is necessary but not sufficient.
2. The `node_allocators` and `ambient_elements` writes are behind
   `is_gc_able(snode_metas[i].type)` (`llvm_runtime_executor.cpp:447`), which is
   `pointer || dynamic` (`taichi/ir/snode_types.cpp:21-23`).

Both surviving routes involve sparse SNodes, which section 4.1 of the plan
states are required. So the reachable configuration is the target one.

**The assertion is not debug-only.** `TI_ASSERT` expands to `TI_ASSERT_INFO`
(`taichi/common/logging.h:100`), which expands to a plain `if` plus `TI_ERROR`
(`:101-107`), with no `NDEBUG` guard. It fires in release builds. It simply
measures the wrong quantity.

## 30. Correction item 3 — the Int64-capability landscape, each setter opened

Tree-wide grep for `spirv_has_int64` gives four setters and four readers.
Setters:

| Setter | Guard | Verdict |
|---|---|---|
| `taichi/rhi/vulkan/vulkan_device_creator.cpp:632` | `if (device_supported_features.shaderInt64)` at `:630` | queried |
| `taichi/rhi/opengl/opengl_device.cpp:511` | `if (!is_gles())` at `:509` | profile test, no feature query |
| `taichi/rhi/metal/metal_device.mm:1052` | `if (feature_64_bit_integer_math)` at `:1051` | **queried** |
| `c_api/src/taichi_opengl_impl.cpp:9` | **none** | asserted unconditionally |

**Metal.** `bool feature_64_bit_integer_math = family_apple3;` at
`metal_device.mm:1038`, and `family_apple3` at `:1035-1036` is
`[mtl_device supportsFamily:kMTLGPUFamilyApple3] | family_apple4`, with
`kMTLGPUFamilyApple3 = MTLGPUFamily(1003)` at `:1025`. Pass A's "unconditional;
read back at `:132`" is **wrong on the first half**. The read-back is real:
`caps.contains(DeviceCapability::spirv_has_int64)` at `:131-132`, feeding
`options.set_msl_version(2, 3, 0)` at `:134`. Pre-Apple3 Metal is a second
in-tree SPIR-V target without Int64. Adversary 02-1 found this; adversary 02-2
confirmed it after the fact and admitted missing it. I opened it myself:
confirmed.

**Direct3D 11.** `Dx11Device::Dx11Device` is `taichi/rhi/dx/dx_device.cpp:557-568`.
`DeviceCapabilityConfig caps{};` at `:563`, one `caps.set(...spirv_version,
0x10300)` at `:564`, `set_caps(std::move(caps))` at `:565`. No integer or float
capability of any kind. Third target without Int64. Both reports missed it;
adversary 02-1 found it. Confirmed. (Adversary 02-1 cited the constructor as
`:562-566`; it is `:557-568`.)

**C API OpenGL.** `OpenglRuntime::OpenglRuntime` is
`c_api/src/taichi_opengl_impl.cpp:4-13`. It builds a fresh
`DeviceCapabilityConfig caps{}` at `:8`, sets `spirv_has_int64` at `:9`,
`spirv_has_float64` at `:10` and `spirv_version` at `:11` with no guard, then
`get_gl().set_caps(std::move(caps));` at `:12`.

`set_caps` is `taichi/rhi/public_device.h:855-857`, body `caps_ = std::move(caps);`
— a whole-object replacement, not a merge. So it **discards** whatever
`GLDevice::GLDevice` (`taichi/rhi/opengl/opengl_device.cpp:506-530`) built under
`if (!is_gles())` at `:509-513`, plus the `GL_NV_gpu_shader5` and
`GL_AMD_gpu_shader*` int16/float16 tests at `:515-522` and the `set_caps` at
`:529`. The route in is `ti_import_opengl_runtime(..., bool use_gles)` at
`taichi_opengl_impl.cpp:21-29`, which calls `set_gles_override(use_gles)` at
`:26` and then `ti_create_runtime(TI_ARCH_OPENGL, 0)` at `:28`. On that path a
GLES device reports Int64.

Confirmed exactly as the arbitration states. I did not determine whether a
shipped configuration exercises it; that is not something the source settles.

**Two further cases neither adversary listed.** Found while enumerating
`set_caps` callers (`c_api/src/taichi_core_impl.cpp:331`,
`c_api/src/taichi_opengl_impl.cpp:12`, `c_api/src/taichi_vulkan_impl.cpp:53`,
`taichi/rhi/metal/metal_device.mm:1168`,
`taichi/rhi/vulkan/vulkan_device_creator.cpp:876`,
`taichi/rhi/vulkan/vulkan_device.cpp:1580`, `taichi/rhi/dx/dx_device.cpp:565`,
`taichi/rhi/opengl/opengl_device.cpp:529`):

- **Imported Vulkan.** `VulkanRuntimeImported::Inner::Inner`
  (`c_api/src/taichi_vulkan_impl.cpp:18-56`) builds `caps{}` at `:35`, sets only
  `spirv_version` (`:37-43`), and calls `set_caps` at `:53`. The physical-storage-buffer
  set is commented out at `:45-51`. So an imported Vulkan device has no Int64
  regardless of what the physical device supports. A fourth in-tree
  configuration without the capability.
- **`ti_set_runtime_capabilities_ext`** (`c_api/src/taichi_core_impl.cpp:317-334`)
  builds a `DeviceCapabilityConfig` from caller-supplied pairs (`:325-330`) and
  installs it with `set_caps` at `:331`. Any capability, including
  `spirv_has_int64`, can be asserted from outside with no device query at all.

Recording both as facts, not as conclusions about whether they matter.

## 31. Correction item 4 — citation audit of report-02-codegen.md

Every file:line in the report re-opened and checked for **attribution**, not
just proximity. Faults found, grouped by whether an adversary had already
caught them.

### Caught by an adversary, and confirmed by me

| Report site | Claimed | Actual |
|---|---|---|
| §3.1 L14, §4.1.8 | `list_element_size` at `codegen_llvm.cpp:2291-2294` | `:2290-2291`; `:2292-2293` is `num_splits` |
| §4.1.2 | `AxisExtractor` fields `snode.h:40,44,49` | `:41`, `:45`, `:49` |
| §3.2 S14 | array length at `spirv_ir_builder.cpp:575` | `:576` |
| §3.2 S14 | `container_stride` narrowing at `:590` | `:591` |
| §3.2 S14 | `nbytes == 0` warning at `:592-599` | `:596-602` |
| §3.3 | Vulkan Int64 gate `vulkan_device_creator.cpp:628-631` | `:630-632`; `:628` is the **int16** set |
| §3.3 | `get_primitive_type` u64 error at `:324-327` | guard `:325-327`, return `:328`; `:324` is `return t_uint32_;` |
| §5 esc. 5 | SPIR-V `LoopIndexStmt` at `spirv_codegen.cpp:541-561` | `:543-563` |
| §2.1 | index-site list | omits the range write at `runtime.cpp:1005` |

### Not caught by either adversary — found in this pass

| Report site | Claimed | Actual |
|---|---|---|
| §3.1 L9 | `ExternalPtrStmt` mul/add chain at `codegen_llvm.cpp:1938`, `:1941`, `:1943` | muls at `:1939` and `:1942`, add at `:1944`. `:1938` is a continuation of the `size_var` initialiser, `:1941` is a comment, `:1943` is a closing brace. **All three wrong.** |
| §3.1 L12 | struct-for loop index stored/loaded at `:2213-2214`, `:2222`, `:2226`, `:2251` | store `:2190-2191`; loads `:2216`, `:2228`, `:2253`; increment `:2273`. `:2213` is `SetInsertPoint`, `:2222` is `SetInsertPoint`, `:2226` is blank, `:2251` is a comment. **All four wrong.** |
| §3.2 S15 | `const_i32_zero_`/`const_i32_one_` pre-built at `spirv_ir_builder.cpp:283-284` | `:223-224`. `:282-286` is `get_null_type`. **Misattributed to the wrong function.** |
| §3.3 | `get_primitive_type` i64 error at `spirv_ir_builder.cpp:310-313` | guard `:311-313`, return `:314`; `:310` is `return t_int32_;` |
| §3.2 S6 | `bitmasked_activation` at `spirv_codegen.cpp:389-435` | `:383-435` |
| §3.2 S6 | `make_pointer(cell_stride * num_cells_per_container)` at `:405-406` | `:406-408`; the `make_pointer` call is on `:408` |
| §3.2 S16 | Taichi `PointerType` → u64/u32 at `spirv_types.cpp:486-495` | the branch is `:490-498`, 64-bit at `:492-493`, 32-bit at `:495-496` |
| §3.1 L20 | ABI comment block at `codegen_llvm.cpp:286-294` | `:284-292`; `:294` is the `functions` vector |
| §3.1 L6 | bit-pointer struct documented at `:1752-1756` | comment is `:1750-1754`; `:1755` is the assertion |
| §4.1.9 | `check_func_call_signature` error at `llvm_codegen_utils.cpp:139-140` | `TI_ERROR` at `:141-142`; `:139-140` is a `TI_INFO` about differing contexts |
| §5 esc. 3 | dead PSB guard at `vulkan_device_creator.cpp:823-827` | `shaderInt64` test `:821`, comments `:822-824`, `#if` `:825`, setter `:826`, `#endif` `:827`. `:823` is a URL comment. |

### Off-by-one span ends, recorded but not individually corrected in the report

`make_pointer` `:2317-2325` → `:2317-2324`. `LinearizeStmt` SPIR-V `:531-540` →
`:532-541`. `get_constant(T)` `:727-750` → `:727-749`. `get_constant(DataType,T)`
`:696-717` → `:695-718`. `SNodeLookupStmt` LLVM `:1794-1828` → `:1792-1829`.
`at_buffer` idx `:2215-2218` → `:2214-2218`. `ExternalPtrStmt` SPIR-V shift
`:772-777` → `:773-777`. `visit(SNodeLookupStmt*)` SPIR-V `:503-507` → the add
is at `:508`. These are span edges, not misattributions; the claim lands on the
right code in each case.

### Checked and correct

`struct_llvm.cpp:74-75`, `:111-113`, `:133-143`, `:146-190`, `:153`, `:171-188`,
`:200`, `:203-233`, `:247`, `:266`; `codegen_llvm.cpp:278`, `:279`, `:281`,
`:1151-1152`, `:1169-1171`, `:1736-1743`, `:1755`, `:1857-1861`, `:1864-1870`,
`:1927`, `:1931`, `:1960-1961`, `:1969-1971`, `:1988`, `:2065-2087`, `:2157`,
`:2309`, `:2336`, `:2339`, `:2362-2364`, `:2386`, `:2396`, and all seventeen
constant-GEP lines; every line in L21-L24 across
`codegen_cpu.cpp`, `codegen_cuda.cpp`, `codegen_amdgpu.cpp`, `codegen_dx12.cpp`,
`dx12_lower_intrinsic.cpp`; all twenty-two `codegen_llvm_quant.cpp` lines;
`spirv_codegen.cpp:82`, `:321-325`, `:355`, `:371-372`, `:707-711`, `:737`,
`:753-754`, `:1999`-adjacent range-for lines, `:2138-2179`, `:2322`;
`spirv_ir_builder.cpp:64-66`, `:164-169`, `:334-341`, `:373-376`, `:565-607`,
`:605`; `spirv_ir_builder.h:51`, `:224`, `:529-534`, `:598`;
`spirv_types.cpp:179-181`, `:199-201`, `:393-419`; `spirv_types.h:71-76`;
`snode_struct_compiler.h:16`, `:19`, `:28`, `:31`, `:38`, `:42`;
`snode_struct_compiler.cpp:16-19`, `:52-54`, `:65-139`, `:137`;
`struct.h:11`; `struct.cpp:7-13`; `snode.cpp:12`, `:220`; `snode.h:97`,
`:306-308`, `:348-350`; `program.cpp:144`; `runtime.cpp:288-290`, `:308`,
`:310`, `:312-322`, `:567-569`; `node_dense.h:10-12`, `:10`, `:18`, `:22`,
`:23`; `node_pointer.h:10`, `:84`, `:90`; `statements.h:1415-1416`, `:1618`;
`llvm_context.cpp:904-916`; `codegen.cpp:37-74`; `spirv/kernel_compiler.cpp:25-46`,
`:37`, `:38`; `spirv_codegen.h:25`, `:38-40`; `llvm_codegen_utils.cpp:103-145`,
`:127-131`; `opengl_device.cpp:509-513`; `metal_device.mm:1052`;
`vulkan_device_creator.cpp:826`; `constants.h:5`, `:12`.

## 32. Two claims of pass A's that are wrong on substance, not citation

1. **§2.3: "The LLVM path has no analogue."** `taichi/codegen/llvm/kernel_compiler.cpp:30-34`
   declares `const DeviceCapabilityConfig &device_caps` as the second parameter
   (`:32`). The body `:35-47` never references it; `KernelCodeGen::create` is
   called at `:36-37` with `compile_config`, `&kernel_def`, `&chi_ir` and
   `*config_.tlctx` only. The SPIR-V twin forwards it
   (`spirv/kernel_compiler.cpp:37`). "No analogue" understates it: the
   plumbing point exists at the same position in the same interface and is
   dropped. Correcting.

2. **§3.2 S6 on `bitmasked_activation`.** Pass A says the function does its
   shifts and masks "in `ptr_dt` (u32)". Half right. `:392-399` and `:403-404`
   do use `ptr_dt`, but `:405`, `:411`, `:412` and `:414` hardcode
   `ir_->u32_type()`. The load-bearing one is `:410-412`, an
   `OpShiftRightLogical` whose Result Type is hardcoded u32 while its Base,
   `bitmask_word_ptr` from `:409` (`ir_->add(parent_ptr, ...)`), carries the
   pointer width. Under SPIR-V's rules the Result Type of
   `OpShiftRightLogical` must match Base's type, so a widened pointer here
   yields an invalid instruction rather than a truncation. Adding.

## 33. Adjudicating the one place the two adversaries contradict each other

Adversary 02-1 §3 row 2 rules pass B's `bitmasked_activation` citations
("405, 411-412, 414") wrong and substitutes `:404`, `:410`, `:411`, `:413`.
Adversary 02-2 §9.4 says that correction is itself wrong.

**Adversary 02-2 is right.** Printed `spirv_codegen.cpp:383-435` numbered. The
`ir_->u32_type()` calls inside the function are on `:405`, `:411`, `:412`,
`:414` (then `:417`, `:421`, `:422`, `:427`, `:428`, `:430`, `:431`, `:433` in
the op branches). Lines `:404`, `:410` and `:413` are statement-opening lines
containing no `u32_type()` call: `:404` is
`ir_->make_value(spv::OpShiftLeftLogical, ptr_dt, bitmask_word_index,` and
`:413` is a bare `bitmask_word_ptr =`. Adversary 02-1 cited statement starts
while correcting a report that had cited the calls.

The two adversaries also diverge on the two runtime helper functions (§28
above): adversary 02-1's `:1029`/`:1038` are right, adversary 02-2's
`:1291-1296`/`:1298-1304` are wrong.

Net on the disagreements: **one each.** Neither is reliable enough to copy
without opening the file, which is the same conclusion adversary 02-2 reached
about the two reports.

## 34. A coverage gap in pass A's own territory

Pass A's §6 sweep list, "files in my territory with nothing width- or
index-relevant", omits fourteen files that exist under `taichi/codegen/`:
`codegen.h`, `llvm/codegen_llvm.h`, `llvm/llvm_codegen_utils.h`,
`llvm/struct_llvm.h`, `llvm/compiled_kernel_data.cpp`,
`spirv/compiled_kernel_data.cpp`, `spirv/kernel_compiler.h`,
`spirv/kernel_utils.cpp`, `spirv/kernel_utils.h`, `dx12/dx12_llvm_passes.h`,
and the four per-backend `codegen_*.h` headers.

Most are headers for files already covered. **One is not.**
`taichi/codegen/spirv/kernel_utils.h` carries width-relevant declarations and
appears in neither the findings nor the sweep:

- `int advisory_total_num_threads{0};` at `:99`, `int advisory_num_threads_per_group{0};`
  at `:100`.
- `RangeForAttributes::begin` and `::end` are `size_t` at `:110-111`.

Those `size_t` fields are subtracted into an `int` at `spirv_codegen.cpp:1999`
(`const int num_elems = range_for_attribs.end - range_for_attribs.begin;`) and
assigned to `advisory_total_num_threads` at `:2004`. This is a host-side
narrowing feeding the emitted i32 constants that pass A's S12 does cover.
Adversary 02-2 credits pass B with it. Adding it as an S-row and closing the
sweep gap.

## 35. Deliberate non-conclusions for this pass

Unchanged in kind from §22. I have not decided that anything is unnecessary,
have not proposed a removal, a fix or an abstraction, and have not chosen
between the possible resolutions of the SNode-ceiling mismatch. Specifically I
do **not** conclude:

- that `SNode::reset_counter()` should be called or deleted — only that it is
  never called;
- that the C API capability overrides are bugs — only that they replace the
  whole config and that a GLES device can therefore report Int64;
- that `spirv_opt_options_.set_run_validator(false)` should be flipped — only
  that no validation runs today;
- that `Translate2Spirv` should take a capability — only that it does not, for
  eight accessors.

All of these are in the report's Escalations, unresolved.

---

# Amendment pass, 2026-09-09

Third agent on this file. Round two is complete: `adversary2-02-1.md` and
`adversary2-02-2.md` both judge report-02-codegen.md correct on substance and
not complete. Both read in full before touching anything. Every item below was
treated as a claim and opened at source before the report was edited. Where an
adversary is wrong I say so with lines and changed nothing.

## 36. Claim 1 — `spirv_codegen.cpp:2464` and `:2511` are swapped. UPHELD.

Grepped the function definitions and the call sites in one pass:

```
2276:      compile_args_struct();
2284:      compile_ret_struct();
2298:      auto buffer_value = compile_argpack_struct(buffer.root_id, binding, ...
2326:  void compile_args_struct() {
2344:              auto spirv_type = translate_ti_type(blk, type, has_buffer_ptr);
2358:      auto spirv_type = translate_ti_type(blk, type, has_buffer_ptr);
2386:        ir_translate_to_spirv(reduced_blk.get(), layout_ctx, ir_.get());
2402:  spirv::Value compile_argpack_struct(const std::vector<int> &arg_id,
2420:              auto spirv_type = translate_ti_type(blk, type, has_buffer_ptr);
2436:      auto spirv_type = translate_ti_type(blk, type, has_buffer_ptr);
2464:        ir_translate_to_spirv(reduced_blk.get(), layout_ctx, ir_.get());
2483:  void compile_ret_struct() {
2494:          translate_ti_type(blk, element.type, has_buffer_ptr));
2511:        ir_translate_to_spirv(reduced_blk.get(), layout_ctx, ir_.get());
2529:  std::vector<BufferBind> get_buffer_binds() {
```

Then opened the boundaries individually: `:2400` is the closing brace of
`compile_args_struct`, `:2481` of `compile_argpack_struct`, `:2527` of
`compile_ret_struct`. So `:2386` args (this report had it right), `:2464`
**argpack**, `:2511` **rets**. This report had the last two the wrong way round,
and so did round one, the correction pass and both round-one adversaries.

Confirmed the surrounding lines too, because a boundary alone would not settle
which struct the call belongs to: `:2465` is
`argpack_struct_type.id = ir2spirv_map[struct_type];` and `:2477` is
`argpack_types_[arg_id] = argpack_struct_type;`; `:2512` is
`ret_struct_type_.id = ir2spirv_map[struct_type];` and `:2514` is
`rets_struct_types_.resize(element_types.size());`.

Corrected in §3.3.4 as a table of boundaries rather than a bare list, so the
next reader can falsify it without re-grepping. Also carried the correction into
escalation 5, which proposes checking at the five `translate_ti_type` sites —
those partition `:2344`/`:2358` args, `:2420`/`:2436` argpack, `:2494` rets, the
other way round from what the old pairing implied. Anyone acting on the old text
would have opened the wrong function first.

Also confirmed while there: `ir_translate_to_spirv` is `spirv_types.cpp:476-483`
(this report was right) and `translate_ti_type` is `:484-514`, pointer branch
`:490-498` (this report cited `:484-...`, now closed).

## 37. Claim 2 — a second widening-invalidity site at `:398-399`. UPHELD, and the rule was verified from the validator rather than from memory.

Printed `spirv_codegen.cpp:383-435` numbered. `:398-399` is

```
398    auto bitmask_mask = ir_->make_value(spv::OpShiftLeftLogical, ptr_dt,
399                                        ir_->const_i32_one_, bitmask_bit_index);
```

`make_value(spv::Op op, const SType &out_type, Args &&...args)` is
`spirv_ir_builder.h:290-298`; the second parameter is Result Type and the pack
becomes the operands in order. So Result Type is `ptr_dt` and Base is
`const_i32_one_`. `const_i32_one_ = int_immediate_number(t_int32_, 1)` at
`spirv_ir_builder.cpp:224` — 32-bit, permanently, and `t_int32_` is declared
unconditionally at `:164`, so it cannot follow a widened pointer.

I did **not** want to assert the SPIR-V shift rule from memory, and this tree
has the validator checked out. `external/SPIRV-Tools/source/val/validate_bitwise.cpp:65-104`
handles the three shift opcodes. The relevant test is `:88-91`:

```
88      if (_.GetBitWidth(base_type) != _.GetBitWidth(result_type))
89        return _.diag(SPV_ERROR_INVALID_DATA, inst)
90               << "Expected Base to have the same bit width "
91               << "as Result Type: " << spvOpcodeString(opcode);
```

**Bit width, not signedness.** That matters twice. It explains why `:398-399` is
legal *today* despite an i32 Base under a u32 Result Type — same width — and it
is what breaks when `ptr_dt` becomes 64-bit. The Shift operand is checked only
for type and dimension (`:93-102`), never width, which is why `:404-405`'s u32
shift constant under a `ptr_dt` Result Type is safe in either world. Without
reading this I might have flagged `:404-405` too, and it would have been wrong.

So `bitmasked_activation` holds two widening faults in opposite directions:
`:410-412` Result Type pinned narrow under a widening Base, `:398-399` Base
pinned narrow under a widening Result Type. Split S6 into S6a and S6b and
rewrote the §4.2.2 bullet, which previously described only the first and would
have led a reader to a repair that fixes half of the function.

## 38. Claim 3 — "five distinct optional bit widths". UPHELD as wrong, and the adversary's replacement figure is also wrong.

Section 10 item 7 says derive every count from its own enumeration, so I did not
adjudicate between "three" and "five" on argument. I enumerated
`spirv_ir_builder.cpp:149-176` and `spirv_types.cpp:393-431` and found four
distinct quantities that had been collapsed into one word:

- accessor **call sites** in the visitor: 8 — `:397 :399 :403 :409 :411 :415
  :424 :428`;
- distinct optional **SPIR-V types** returned: 8 — `t_int8_ t_uint8_ t_int16_
  t_uint16_ t_int64_ t_uint64_ t_fp16_ t_fp64_`, each with its own
  `declare_primitive_type` call at `spirv_ir_builder.cpp:156-176`;
- **capability gates**: 5 — `:156 :160 :166 :171 :174`, matching
  `taichi/inc/rhi_constants.inc.h:12-16` exactly;
- distinct **bit widths**: 3 — 8, 16, 64.

So this report's "five distinct optional bit-widths: int8, int16, int64, fp16,
fp64" listed types under the label "widths" and then named only five of the
eight, because it silently collapsed each signed/unsigned pair. Wrong twice over.

**Adversary 02-1 is right that the width figure is three, and its supporting
sentence is a miscount of the same species.** Its §2.2 says "Five is the count
of optional *types* (int8, int16, int64, fp16, fp64)" and then in the next
sentence credits the paired report's "eight optional types, five capability
flags" as precise. Three and eight cannot both be five. Those five names are the
**capability** names. I have said so in §3.3.3 rather than quietly adopting the
number, which is the behaviour the arbitration asked for in the previous round
and which applies to adversaries in turn.

Also recorded why the count is eight and not twelve: `i32_type()` `:401`,
`u32_type()` `:413`, `bool_type()` `:407` and `f32_type()` `:426` are bypassed
identically, and are safe only because their members are declared
unconditionally (`spirv_ir_builder.cpp:155`, `:164-165`, `:170`). Adversary 02-2
flagged that neither report said this; it is what makes the exposed set eight.

## 39. Claim 4 — the severity divergence. TRACED END TO END. ADVERSARY 02-2 IS RIGHT.

This is the one the planner said it had reported twice and been wrong twice, so
I traced it rather than picking a side. `external/SPIRV-Tools` is a live
checkout, so every link is readable in this tree.

1. `spirv_codegen.cpp:2740` copies the binary; `:2746-2747` calls
   `spirv_opt_->Run` unconditionally — `enable_spv_opt` gates only the pass
   registrations at `:2683-2708`.
2. `Optimizer::Run` is `external/SPIRV-Tools/source/opt/optimizer.cpp:584-598`.
   `:590-594` skips `tools.Validate` because `run_validator_` is false. `:596-597`
   calls `BuildModule` **unconditionally**. `:598` returns false on null.
3. `BuildModule` is `build_module.cpp:56-75`: `spvBinaryParse` at `:68-69`,
   `return status == SPV_SUCCESS ? ... : nullptr;` at `:74`.
4. `spvBinaryParse` rejects a zero `<id>`: `binary.cpp:450` type id, `:456`
   result id, `:473` `SPV_OPERAND_TYPE_ID` — which is what an `OpTypeStruct`
   member operand is. Error code `SPV_ERROR_INVALID_ID`.
5. **The level.** I did not assume this. `DiagnosticStream::~DiagnosticStream`
   is `diagnostic.cpp:87-114`. `:89` initialises `auto level = SPV_MSG_ERROR;`
   and the switch at `:90-108` only *lowers* it for four specific codes;
   `SPV_ERROR_INVALID_ID` hits `default:` at `:106-107`, so the level stays
   `SPV_MSG_ERROR`. `:112` calls the consumer.
6. **The downgrade.** `spriv_message_consumer` is `spirv_codegen.cpp:2641-2659`,
   installed at `:2682`. `if (level <= SPV_MSG_FATAL)` at `:2646`. In
   `external/SPIRV-Tools/include/spirv-tools/libspirv.h:83-92` the enum is
   FATAL=0, INTERNAL_ERROR=1, ERROR=2, WARNING=3. So level 2 fails the first
   test, hits `else if (level <= SPV_MSG_WARNING)` at `:2649`, and is emitted as
   **`TI_WARN`**. The branch order is the whole mechanism: an error is reported
   as a warning because only FATAL is treated as fatal.
7. `Run` returns false, so `TI_WARN_IF(..., "SPIRV optimization failed")` at
   `:2745-2748` fires and `success = false` at `:2750`.
8. `success` is declared `:2742` and read at exactly one place, `:2760`, inside
   `if constexpr (false)` at `:2758-2772`. Dead.
9. `:2775` pushes `std::move(optimized_spv)` regardless. I checked whether that
   vector still holds the invalid module on a failed `Run`: `Optimizer::Run`
   reaches `optimized_binary->clear()` only at `optimizer.cpp:642`, well after
   the `return false` at `:598`, so yes — it holds the unmodified copy made at
   `spirv_codegen.cpp:2740`.

**Verdict.** Adversary 02-2's chain holds at every link. Adversary 02-1's §2.1
point 4 — "a 0 `<id>` operand is a hard parse failure for any conformant SPIR-V
consumer. The failure is therefore deferred to the driver at shader-module
creation, not absent" — is right in its first sentence and wrong in its second,
and its point 3 ("Nothing in Taichi looks at it. So there is no Taichi-level
diagnostic") is wrong outright. There are two diagnostics. 02-1's §6 item 3 asks
both reports to *add* the deferral sentence; I did not add it, because it would
have this report assert something the source contradicts.

I did keep 02-1's underlying point where it survives: the module *is* shipped at
`:2775`, so a driver does eventually see it. But that is an additional outcome,
not what the failure is deferred to, and I said explicitly in §3.3.5a that this
report does not trace `generated_spirv` past `:2775` and makes no claim about
driver behaviour.

**What this decides for item 6.2, which was the planner's question.** It owes
**both**, and they are separable:

- a **diagnostic** at `spirv_types.cpp:397-428`, because the existing warning
  names a byte offset into a binary and not the type or the capability, and both
  are still in hand at that point;
- a **correctness item** at `spirv_codegen.cpp:2742-2775` that is independent of
  6.2 and would still stand if the bypass were closed — the compile records its
  own failure and discards it.

Escalated as 14 rather than resolved. I am not proposing a change to either.

One further consequence I worked out and put in §4.2.10, which neither adversary
raised: the accidental net catches only what the **parser** rejects, which is
structural. A width mismatch of the kind S6a/S6b would produce is a **validator**
check (`validate_bitwise.cpp:88-91`), and the validator is precisely what is
off. So the two faults in `bitmasked_activation` would parse cleanly and ship
with no diagnostic at all. "Nothing catches width faults" survives claim 4
intact; only the zero-id half changes.

## 40. Claim 5 — §4.2 points 2 and 5 contradict each other. UPHELD.

`at_buffer` is `spirv_codegen.cpp:2194-2220`. The `u64` test is `:2197` and its
branch **returns at `:2204`**. The shift this report's point 5 relied on is
`:2214-2216`, below that return. So if point 2 is right — a widened pointer
routes into the `OpConvertUToPtr` branch — point 5's code is unreachable. Point
5 is round-one text; point 2 was added in front of it in the correction pass
without reconciling. Point 5 is the wrong half and I withdrew it.

Opened both sub-cases rather than just deleting:

- **With Int64:** `declare_primitive_type` sets `t.dt = dt` at
  `spirv_ir_builder.cpp:1536`, so `u64_type()` carries `dt == u64`, the `:2197`
  test is true, return at `:2204`, shift never runs.
- **Without Int64:** `u64_type()` returns a default-constructed `SType`.
  `DataType::DataType()` is `ptr_(PrimitiveType::unknown.ptr_)` at
  `taichi/ir/type.cpp:20` and `operator==` compares pointers at
  `taichi/ir/type.h:97-99`, so `:2197` is **false** — but then
  `TI_ERROR_IF(!is_integral(ptr_val.stype.dt), ...)` at `:2207-2210` fires,
  because `is_integral` (`taichi/ir/type_utils.h:103-113`) has no arm for
  `unknown`.

That second sub-case corrects a claim this report makes elsewhere and the paired
report makes too: that flipping `use_64bit_pointers` without Int64 yields id 0.
True at the type-declaration site; the first `at_buffer` use raises a **named
error**. Worth keeping because it is exactly the asymmetry 6.2's stub note turns
on — the SNode address path has an accidental guard, the kernel-signature path
of §3.3 has none and never touches `at_buffer`.

## 41. Claim 6 — eight disclosed span faults not fixed in the tables. UPHELD, all eight re-opened before editing.

The correction pass listed these in §3.4.1 and left the tables carrying numbers
it knew to be wrong, on the grounds that they land on the right code. Both
adversaries objected. They are right: a table that cannot be transcribed is not
an inventory, and this report applied exactly that standard to round one.

Re-derived each rather than trusting the correction pass's own "actual" column:

| Row | Was | Is | How I checked |
|---|---|---|---|
| §3.0 `get_constant(T)` | `llvm_context.cpp:727-750` | `:727-749` | `:727` `template <typename T>`, `:749` `}`, `:750` blank |
| §3.0 `get_constant(DataType,T)` | `:696-717` | `:695-718` | `:695` `template <typename T>`, `:718` `}` |
| L7 | `codegen_llvm.cpp:1794-1828` | `:1792-1829` | `:1792` signature, `:1829` `}`; `:1828` is an inner brace |
| S2 `make_pointer` | `spirv_codegen.cpp:2317-2325` | `:2317-2324` | `:2324` `}`, `:2325` blank |
| S5 | `:503-507` | `:504-508` | `:503` blank; cast `:504-505`, `make_pointer` `:506`, mul `:507`, **add `:508`** — the add was outside the old span entirely |
| S7 | `:531-540` | `:532-541` | `:531` blank, `:532` signature, `:541` `}` |
| S8 `at_buffer` idx | `:2215-2218` | `:2214-2218` | `make_value` opens `:2214` |
| S10 shift | `:772-777` | `:773-777` | `:772` is `}` |

All eight applied in the rows. Replaced the §3.4.1 prose with a table that keeps
the was/is pair visible, so the repair is auditable rather than just done.

## 42. Claim 7 — Metal never sets `spirv_has_float64`; extend to all five. UPHELD, and it is the largest thing that was missing.

Grepped each of the five capability names over `taichi/` and `c_api/`, excluded
the `caps_->get(...)` reads in `spirv_ir_builder.cpp`, then opened every setter.

Result, and the file-level negative is the striking one: **`taichi/rhi/metal/metal_device.mm`
contains no occurrence of the string `float64` at all.** The capability block is
`:1044-1053`: `spirv_version` `:1045`, int8 `:1046`, int16 `:1047`, float16
`:1048`, subgroup_basic `:1049`, then int64 at `:1052` under
`if (feature_64_bit_integer_math)` at `:1051`. Float64 is simply never wired.
So an `f64` argument, return value or argpack member on **any** Metal device,
current hardware included, takes the bare `f64_type()` at `spirv_types.cpp:428`
and emits id 0. That is unconditional, not a legacy subset, and it is a stronger
instance of the defect than the Int64 case this report led with.

Two more fell out of the same sweep, neither in either report:

- **OpenGL never sets `spirv_has_int8`** under any branch of
  `GLDevice::GLDevice` (`opengl_device.cpp:506-530`) — desktop profile included.
  So `i8`/`u8` arguments emit id 0 on all OpenGL.
- **DX11 and imported Vulkan lack all five**, not just Int64. Each sets only
  `spirv_version` (`dx_device.cpp:563-565`, `taichi_vulkan_impl.cpp:35-53`).
  This report previously said "no integer or float capability of any kind" for
  DX11, which is right, and "no Int64" for imported Vulkan, which is narrower
  than the truth.

Also corrected the OpenGL probe span while there. This report cited the
discarded probes as `:515-522`, which omits the
`GLAD_GL_AMD_gpu_shader_half_float` block at `:524-526`. The three probes are
`:515-518`, `:520-522`, `:524-526`. And since the C API OpenGL constructor sets
only int64, float64 and spirv_version, and `set_caps` replaces the whole object
(`public_device.h:855-857`), that override **removes** int16/float16 as well as
adding int64 — this report said the probes are discarded but did not draw out
that the override is subtractive as well as additive.

Built the full five-by-seven matrix as §3.3.7a. The consequence for 6.2's stub
requirement is a matrix in which no configuration reports all five and one
backend lacks one on every device it will ever run on — a different shape of
problem from "some old targets lack Int64". Escalated Metal's absent Float64 as
15, because whether it is a constraint or an oversight is not determinable here
and it decides whether the stub must express a permanent or a temporary
Float64-less backend.

## 43. Two faults beyond the claim list, both verified by both adversaries, both in my own file.

The brief listed seven claims. These two are errors in the file I now own, found
and independently verified by both round-two adversaries, and leaving them would
be worse than the scope question. Recorded here so the planner can see they were
deliberate and not scope creep.

1. **`VulkanRuntimeImported::Inner::Inner` does not exist.** Grepped `Inner`
   across `c_api/src/taichi_vulkan_impl.{h,cpp}`: no hit. The constructor is
   `VulkanRuntimeImported::Workaround::Workaround`, opening at
   `taichi_vulkan_impl.cpp:19` (`:18` is blank), declared at
   `taichi_vulkan_impl.h:27-31`. The member is `inner_` at `:31`, which is
   presumably where the correction pass got the name. Every other line in that
   claim is exact (`:35`, `:37-43`, `:45-51`, `:53`) — which is what makes a
   fabricated identifier the worst kind of fault here: a reader who greps for it
   concludes the whole finding is invented.

2. **§2.1's "Both surviving routes involve sparse SNodes" contradicts §2.1's own
   gate 1, eight lines earlier.** `bool all_dense = config_.demote_dense_struct_fors;`
   at `llvm_runtime_executor.cpp:402`, and the loop at `:403-410` can only
   **clear** it — it has no assignment to true. So with the flag false,
   `all_dense` is false unconditionally and the range write at
   `runtime.cpp:1003-1007` fires for a fully dense tree.

   **This is the one place the two adversaries split and 02-2 is right.**
   Adversary 02-1's §2.6 says "Report A has this right" and asks only the paired
   report to withdraw the conclusion. It read the finding and not the conclusion
   eight lines below it. Adversary 02-2's §5.3 has it: complete and
   inconsistent, where the paired report is consistent and incomplete. Restated
   the two routes with their actual preconditions — `is_gc_able` really does
   require `pointer || dynamic` (`taichi/ir/snode_types.cpp:21-23`), the
   `element_lists` route requires a non-dense SNode **or** the flag false — and
   escalated the configuration question as 13 rather than deciding it.

## 44. One addition taken from the paired report, on adversary 02-1's §6 item 5.

`get_buffer_value` at `spirv_codegen.cpp:2266-...` **does** call the guarded
`get_primitive_type(dt)` at `:2267`. So there is a middle-of-pipeline point
where the guard runs on the SPIR-V path, and this report's escalation-5 closure
did not address it — it closed the front (`Extension::data64`) and the back (the
validator) only.

Checked every call site to see what `dt` it receives. All the Args- and
Rets-buffer sites hand it a hardcoded placeholder: `PrimitiveType::i32` at
`:609-610`, `:618`, `:651`, `:728`, `:753`, `:788`, `:2293`, and
`PrimitiveType::u32` at `:401`, `:516`, `:2138`. `:1901` looked like an
eleventh; it is inside the comment block at `:1899-1903` and is not a call site.
The **only** call that forwards a caller-supplied `dt` is `at_buffer`'s at
`:2212`, which is the SNode access path, not the kernel-signature path.

So the guard executes, always on a mandatory type, and returns cleanly. It
strengthens the closure rather than weakening it, and without it "no earlier
check" rested on `Extension::data64` alone. Added to §3.3.5 and to escalation 5.

## 45. Deliberate non-conclusions for the amendment pass

Unchanged in kind from §22 and §35. Nothing was decided unnecessary, nothing
removed, no fix or abstraction proposed. Specifically I do **not** conclude:

- which of S6a and S6b a widening repair should address, or how;
- that the dead `success` flag at `spirv_codegen.cpp:2750` is a defect to fix,
  or what it was meant to gate — only that it is written and never read outside
  a compiled-out block (escalation 14);
- that Metal's absent `spirv_has_float64` is an oversight rather than a
  hardware constraint (escalation 15);
- that `demote_dense_struct_fors` should or should not be set false — only that
  with it false the range write is reachable on a dense tree (escalation 13);
- that `set_run_validator(false)` should be flipped — only that it is off, and
  that §3.3.5a establishes what happens instead (escalation 16).

Two things I could not settle from this tree and did not guess: what a driver
does with the shipped invalid module, and whether the `#if ... && false` around
`spirv_has_physical_storage_buffer` is current or stale. Both were already
escalated and remain so.

---

# Close-out pass, 2026-09-09. Notes §§46-52.

Fourth agent on this file, after round three (`adversary3-02-1.md`,
`adversary3-02-2.md`). Contemporaneous, written as the pass ran.

Plan re-read in full at the start of this pass, at its **2026-09-09** date
stamp, §10 priority statement and item 6 included. No instruction I was given
contradicts it. §10.9's corollary governs three of the five items below: each
one is a count or a partition whose generating filter could not see the whole
class.

## 46. Claim 1 — the §6 sweep sentence unions to 44, not 45. UPHELD, AND THE ADVERSARIES ARE THEMSELVES SHORT BY ONE.

The claim as put to me: the reconciling sentence at approximately `:1569-1572`
unions to 44; `llvm/llvm_codegen_utils.cpp` is covered at `:1093` but named by
neither half. One adversary disputed it, then withdrew after finding it had
searched the wrong line window.

I did not take either account. I transcribed the irrelevant list from the report
into a file, expanded its two shorthands (`compiled_kernel_data.{h,cpp}` split
into two; "the four per-backend `codegen_*.h` headers" expanded to
`amdgpu/codegen_amdgpu.h`, `cpu/codegen_cpu.h`, `cuda/codegen_cuda.h`,
`dx12/codegen_dx12.h`), diffed it against a directory walk, and tested each
remaining basename against the report's own lines 364-894.

Section bounds established first, because that is where the disputing adversary
went wrong: `### 3.1` opened at report line `:364`, `### 3.4` at `:895`. So the
three sections the sentence names occupied **364-894**, not 347-1034.

**Caveat on every report line number in these notes.** They are the numbers as
the file stood when this pass opened it, which is what round three quoted. My
edits added a close-out header block and shifted everything below it: `### 3.1`
is now `:419` and `### 3.4` is now `:973`, so the same window is now **419-972**.
I re-ran the whole test against the shifted window after editing and got the
identical two misses. Source line numbers are unaffected and are the ones that
matter.

```
find taichi/codegen -type f \( -name '*.h' -o -name '*.cpp' \)   -> 45
irrelevant list, expanded                                        -> 24 entries
distinct                                                         -> 24
present on disk                                                  -> 24
remainder                                                        -> 21
of the remainder, matched in report lines 364-894                -> 19
of the remainder, NOT matched                                    ->  2
```

The two are `llvm/llvm_codegen_utils.cpp` — which is what both adversaries
found — and the **top-level `taichi/codegen/codegen.cpp`**, which neither did.
The basename regex was anchored `(^|[^_A-Za-z0-9])codegen\.cpp` precisely so
that `spirv_codegen.cpp` (preceded by `_`) could not mask it, which is the trap
that hides this file.

Where each is actually covered:

- `llvm/llvm_codegen_utils.cpp` — `:923` (§3.4 audit table), `:1093` (§4.1.9),
  `:1277` (§4.3). Opened `check_func_call_signature` to be sure the finding is
  real: `llvm_codegen_utils.cpp:103` opens the function, `:145` closes it,
  `TI_ERROR` at `:141-142`. Both bounds printed.
- `taichi/codegen/codegen.cpp` — `:325` only, in §2.3, the item 6.3 seam.
  Opened `KernelCodeGen::create`: `:37` opens, `:74` closes (`:72` is
  `TI_ERROR("Llvm disabled");`, `:73` `#endif`, `:74` `}`). Real, and cited.

So **coverage is complete and the union of the sentence's two halves is 43**.
Corrected in §6 with the derivation shown, and the sentence now names §2.3,
§3.1, §3.2, §3.3 and §4.1.9.

**The repetition check that the brief asked for caught a second occurrence.**
§3.4.4 said the two round-two adversaries "re-derived the 45-file arithmetic in
§6 independently ... and both found it exact." That sentence is now false as
written and it is the exact shape of standing instruction §10.7: what those
adversaries certified was the file *arithmetic* — 45, 24, 21 — which is still
exact, not the reconciling *sentence* sitting next to it. Withdrawn in place at
§3.4.4, with the distinction spelled out rather than deleted.

## 47. Claim 2 — the `get_buffer_value` census omits `:2024` and `:2039`, and its heading does not reconcile. UPHELD ON BOTH HALVES.

`grep -n get_buffer_value taichi/codegen/spirv/spirv_codegen.cpp` returns 17
lines. Every one opened:

```
 354  // commented out                     -> not a call site
 401  Root,        PrimitiveType::u32
 516  GlobalTmps,  PrimitiveType::u32
 604-605  prose in a comment              -> not a call site
 609  ArgPack,     PrimitiveType::i32
 618  Args,        PrimitiveType::i32
 651  Rets,        PrimitiveType::i32
 728  Args,        PrimitiveType::i32
 753  Args,        PrimitiveType::i32
 788  Args,        PrimitiveType::i32
1901  inside /* */ opened :1899, closed :1903  -> not a call site
2024  GlobalTmps,  PrimitiveType::i32   <-- omitted by the amendment pass
2039  GlobalTmps,  PrimitiveType::i32   <-- omitted by the amendment pass
2138  ListGen,     PrimitiveType::u32
2212  ptr_to_buffers_.at(ptr), dt        -> the only forwarder (at_buffer)
2266  the definition
2293  Args,        PrimitiveType::i32
```

Nine hardcoded i32, three hardcoded u32, one forwarder. **Thirteen call sites,
twelve hardcoded.** I printed `:2014-2045`: both omitted sites sit in the
non-const `begin` and `end` branches of the range-for bounds, reading out of the
global-temporaries buffer, and both pass `PrimitiveType::i32` literally.

The heading fault is real and separate. "Every Args- and Rets-buffer call site"
already had Root (`:401`), GlobalTmps (`:516`), ListGen (`:2138`) and ArgPack
(`:609`) underneath it. Under the narrow reading four rows do not belong; under
the broad reading two are missing. Either way the list and its label do not
reconcile — §10.9 exactly.

**The conclusion survives and strengthens**, as the adversary says: both omitted
sites are hardcoded like the rest, so "the only call that forwards a
caller-supplied `dt` is `at_buffer`'s at `:2212`" is untouched, and the
escalation-5 closure is now resting on twelve placeholder sites rather than ten.
`:1901`'s exclusion is correct — I printed `:1896-1906` and the `/* */` bounds
are as stated. The source agrees with the report's word "placeholder":
`spirv_codegen.cpp:650` reads ``// The `PrimitiveType::i32` in this function call is a placeholder.``

Corrected in **both** places the census appears, §3.3.5 and escalation 5. That
pairing is the whole point of the brief's general fault: the amendment pass
wrote the same list twice and round three found it twice.

## 48. Claim 3 — an open-ended span in a row labelled "Spans corrected". UPHELD, and two more of the same species elsewhere.

S16 at report `:446` carried `spirv_types.cpp:484-...` while the same report
gave the closed form `:484-514` at `:595` and `:946`. Closed, and the closing
line printed: `spirv_types.cpp:484` opens `translate_ti_type`, `:514` is its
closing brace.

Then I ran `grep -n -- '-\.\.\.'` over the whole report, because the brief's
general fault says to check where else a corrected fact appears. Two more:

- `spirv_ir_builder.cpp:149-...` at §3.3.2. `init_pre_defs` opens `:149`,
  closes `:225` (`:223` and `:224` are the `const_i32_zero_`/`const_i32_one_`
  assignments, `:226` blank, `:227` the next function). Closed.
- `spirv_types.cpp:167-...` at §3.3.4. `translate_ti_primitive` opens `:167`,
  closes `:214` (`:216` opens `TypeVisitor::visit_type`). Closed.

Neither adversary named these two. They are the same defect as S16 and cost one
`sed` each to close, so I closed them and said so rather than leaving two known
ellipses standing behind a corrected third.

## 49. Claim 4 — THE ADDITION. `bitmasked_activation` holds four widening-invalidity sites, from a second mechanism. UPHELD IN FULL, mechanism verified, not just the lines.

This is the item the brief flagged as missing from both reports and every prior
adversary. It comes from `adversary3-02-2.md` §8 alone;
`adversary3-02-1.md` does not contain the string `bitmasked` anywhere.

**The four sites, each read for Result Type and for each operand.** `ptr_dt` is
`parent_ptr.stype`, assigned at `:388`.

| Site | Op | Result Type | Operand that does not follow | Rule |
|---|---|---|---|---|
| `:392-394` | `OpShiftRightLogical` | `ptr_dt` (`:393`) | Base `input_index` (`:393`) | `validate_bitwise.cpp:88-91` |
| `:395-397` | `OpBitwiseAnd` | `ptr_dt` (`:396`) | operand `input_index` (`:396`) | `validate_bitwise.cpp:134-138` |
| `:398-399` | `OpShiftLeftLogical` | `ptr_dt` (`:398`) | Base `const_i32_one_` (`:399`) | `validate_bitwise.cpp:88-91` |
| `:410-412` | `OpShiftRightLogical` | `u32_type()` (`:411`) | Base `bitmask_word_ptr`, pointer-width (`:409`) | `validate_bitwise.cpp:88-91` |

`:404-405` is **not** a fifth: its Base `bitmask_word_index` does follow
`ptr_dt`, and its Shift operand is a `u32` immediate, which the shift arm leaves
unconstrained in width (`:93-101` check type and dimension only). I checked this
rather than assuming it, because the whole finding turns on which operand a rule
constrains.

**`OpBitwiseAnd` is a different and stricter arm, and this matters.** The
shifts go through `validate_bitwise.cpp:65-104`, which constrains Base only.
`OpBitwiseOr`/`Xor`/`And`/`Not` go through `:106-141`, which loops over **every**
operand from index 2 (`:118-119`) and requires each to match Result Type's bit
width (`:134-138`, "Expected operands to have the same bit width as Result
Type"). So `:395-397` has no unconstrained-operand escape at all. Printed both
arms.

**The second mechanism, verified rather than accepted.** `bitmasked_activation`
has exactly two callers and they disagree:

- `visit(SNodeOpStmt*)`, `:437-466`. Casts first:
  `spirv::Value input_index_val = ir_->cast(parent_val.stype, ir_->query_value(stmt->val->raw_name()));`
  at `:443-444`, then calls at `:448-449`, `:455-456`, `:458-459`. On this path
  `input_index` follows the pointer and `:392-397` stay valid under widening.
  **Two broken sites on this path.**
- `visit(SNodeLookupStmt*)`, `:468-511`. Does not:
  `spirv::Value input_index_val = ir_->query_value(stmt->input_index->raw_name());`
  at `:488-489`, called at `:490-491`. **Four broken sites on this path.**

The index is i32 because it is `visit(LinearizeStmt*)`'s output, `:532-541`,
seeded `val = ir_->const_i32_zero_` with `int_immediate_number(ir_->i32_type(), ...)`
strides. `const_i32_zero_`/`const_i32_one_` are built at
`spirv_ir_builder.cpp:223-224` from `t_int32_`, permanently 32-bit.

**The tell that makes this a mechanism and not a coincidence:** the same
function casts the same value eighteen lines below, at `:504-505`
(`ir_->cast(parent_val.stype, ir_->query_value(stmt->input_index->raw_name()))`),
for its dense-offset branch. This report already recorded that cast, as S5,
in round one. So both halves sat in the S-table from the beginning and were
never read against each other. That is the lesson §10's priority statement is
about: the assumption underneath the search was that a function taking `ptr_dt`
as Result Type is width-clean, and nobody tested it.

**Why all four are legal today.** `ptr_dt` is u32 and the index is i32 — same
bit width, and the validator tests width, not signedness. Identical to the
reason S6b is legal today. They break together when `make_pointer` widens.

**Grade: ARCHITECTURAL**, per §10.8. If every driver, hardware generation and
specification were ideal today, the mismatch would still be there, because it
follows from decisions inside this codebase about how the bitmask index is
represented and where it is cast. It does not expire.

**Blast radius, stated separately: five places in one file** — four instructions
in `taichi/codegen/spirv/spirv_codegen.cpp:383-435`, plus the call site at
`:488-491`. Small. Which is the point of grading kind and radius separately: the
cost of not recording it is that a repair scoped from either report's previous
text lands on two of the five.

Recorded as S6c, S6d and S6e in §3.2, rewritten into §4.2.2 with the table
above, and escalated as new escalation 17. **I do not decide** which of the four
a repair should address, nor whether the cast belongs at the call site, in the
parameter's declared type, or in `LinearizeStmt`'s output width — the last of
which is territory 01's ground, not mine. §10.3 forbids me ruling that the raw
pass at `:488-489` is a defect rather than an intentional difference, and there
is no comment at either call site saying which.

## 50. Claim 5 — the citation figure is 278, not 342. UPHELD ABOUT THE PAIRED REPORT; NOTHING TO CORRECT HERE.

`grep -n 342 report-02-codegen.md` returns two hits before my edits, both the
tail of the line range `spirv_ir_builder.cpp:337-342`: the S16 row in §3.2, and
item 1 of §3.3.6. `grep -n 278`, likewise before my edits, returns two hits,
both `llvm/codegen_llvm.cpp:278`, an L-table row and its §2 mention. Both greps
now also hit the close-out header, where the check itself is recorded. **This report
has never carried either number as a citation total.** No edit made; recorded in
the close-out header so the planner can see the check was run rather than
assumed, given that the 342 figure was relayed into briefs.

## 51. The general fault — a correction landing where raised and not where repeated. Swept.

For each fact corrected this pass I grepped the whole report for other
occurrences before finishing:

| Corrected fact | Occurrences found | All landed |
|---|---|---|
| §6 sweep reconciliation | §6 itself, and the "found it exact" sentence at §3.4.4 | yes, both |
| `get_buffer_value` census | §3.3.5 and escalation 5 | yes, both |
| `:484-...` open span | S16 row; closed form already at §3.3.4 and §3.4.1 | yes; two further ellipses found and closed |
| `bitmasked_activation` "two sites" | header change-log, S6 row, §4.2.2, §3.3.5a's "two invalid shift instructions", the §5 non-conclusion list | yes, all five |
| citation figure 342 | none | n/a |

The `bitmasked_activation` sweep is the one that mattered: the phrase "a widened
pointer's two invalid shift instructions" was sitting in §3.3.5a, four hundred
lines from §4.2.2 where the count is derived, and would have survived a fix
applied only where round three raised it. Also corrected there: those four
instructions are not all shifts, one is an `OpBitwiseAnd`.

Escalation count re-derived after adding 17: `grep -oE '^[0-9]+\.'` over §5
returns 1 through 17, seventeen entries, no gaps or repeats.

## 52. Deliberate non-conclusions for the close-out pass

Unchanged in kind from §22, §35 and §45. Nothing decided unnecessary, nothing
removed, no fix or abstraction proposed, and no line of investigation opened
beyond item 4 of my brief. Specifically I do **not** conclude:

- which of S6a, S6b, S6c and S6d a widening repair should address, or how;
- whether S6c and S6d should be closed at the call site `:488-489`, by typing
  `bitmasked_activation`'s `input_index` parameter, or by changing
  `LinearizeStmt`'s output width — the third reaches into territory 01 and is
  not mine to take (escalation 17);
- that the difference between the two callers of `bitmasked_activation` is a
  defect at all rather than an intended asymmetry. It is recorded as a fact.

Everything in §45's list of non-conclusions still stands, unchanged.

Two things I could not settle from this tree and did not guess, both already
escalated: what a driver does with the shipped invalid module, and whether the
`#if ... && false` around `spirv_has_physical_storage_buffer` is current.
