# Notes 02B — codegen, both paths

Contemporaneous working notes. Appended as I go. Agent 02B.

## 1. Setup

Read `modernization/PROJECT-PLAN.md` in full first. Governing items for me:
- 6.1 SNode ceiling `taichi_max_num_snodes = 1024`, assertion at
  `taichi/codegen/llvm/struct_llvm.cpp:266`.
- 6.2 64 bit addressing, end to end, no silent 32 bit residue.
  `taichi_max_num_indices = 12`. SPIR-V Int64 is a declared capability.
- 6.3 adaptive module loading (backends) — touches codegen only where the
  codegen is parameterised by device caps.
- 6.4 separation not abstraction; no additions unless something is removed.

Territory: `taichi/codegen/` in full. 51 files, 15351 lines of .cpp/.h.

Layout of the territory:
- LLVM path: `llvm/` (codegen_llvm.cpp/.h, codegen_llvm_quant.cpp,
  struct_llvm.cpp/.h, llvm_codegen_utils.*, kernel_compiler.*,
  compiled_kernel_data.*, llvm_compiled_data.h)
- SPIR-V path: `spirv/` (spirv_codegen.cpp, spirv_ir_builder.cpp/.h,
  spirv_types.cpp/.h, snode_struct_compiler.cpp/.h, kernel_utils.cpp/.h,
  lib_tiny_ir.h, kernel_compiler.*, compiled_kernel_data.*)
- Device flavours of the LLVM path: `cpu/`, `cuda/`, `amdgpu/`, `dx12/`
- Shared: `codegen.cpp/.h`, `codegen_utils.h`, `compiled_kernel_data.*`,
  `kernel_compiler.h`

Plan of attack: (a) constants first, (b) LLVM index/address arithmetic,
(c) SPIR-V index/address arithmetic, (d) Int64 capability, (e) SNode-count
scaling, (f) diff the two paths.

## 2. Constants, and the LLVM struct compiler

`taichi/inc/constants.h`: `taichi_max_num_indices = 12` (line 5),
`taichi_max_num_snodes = 1024` (line 12), `kMaxNumSnodeTreesLlvm = 512`
(line 13). Everything in `constants.h` that concerns me is `int`, not a
width-parameterised type.

Grep for the three constants across `taichi/codegen/` gives only three hits:
- `llvm/struct_llvm.cpp:171` — `for (int i = 0; i < taichi_max_num_indices; i++)`
- `llvm/struct_llvm.cpp:266` — `TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);`
- `llvm/codegen_llvm.cpp:2363` — a comment only, `//   i32 val[taichi_max_num_indices];`

Surprise #1: `kMaxNumSnodeTreesLlvm` and `taichi_max_num_snodes` appear
NOWHERE else in codegen. The SPIR-V path never mentions either. So the SNode
ceiling is not a codegen-wide constraint; it is one assertion plus the runtime
struct arrays (which are territory 03).

### 2.1 The assertion at struct_llvm.cpp:266

```
TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);
```
`snodes` is `StructCompiler::snodes` (`taichi/struct/struct.h:11`), filled by
`collect_snodes(root)` at `struct_llvm.cpp:249`. `run()` is called with a
single tree root. So the check is PER SNODE TREE, not global — need to verify
whether the runtime arrays it is proxying for are also per-tree or global.
Flagging for cross-check with territory 03.

Also note the assertion fires AFTER all types and accessors are generated
(line 266 is after `generate_child_accessors(root)` at line 253 and after the
optional IR dump at 255-259). It is a post-hoc bound check, not a guard.

### 2.2 `generate_refine_coordinates` — the LLVM index arithmetic

`struct_llvm.cpp:145-188`. This is the core per-SNode index refinement, and it
is entirely 32 bit:

- line 152: the third parameter `l` is `llvm::Type::getInt32Ty` — the linear
  cell index within the parent container is an **i32**.
- line 171: loops `i` over `taichi_max_num_indices` (12), emitting up to 12
  div/rem/mul/add chains per SNode. Emitted code size scales linearly with
  `taichi_max_num_indices`.
- lines 172-176: `tlctx_->get_constant(0)`,
  `get_constant(extractors[i].acc_shape * extractors[i].shape)`,
  `get_constant(extractors[i].acc_shape)`. `AxisExtractor::shape`,
  `acc_shape`, `num_elements_from_root` are all `int`
  (`taichi/ir/snode.h:41,45,49`), and `TaichiLLVMContext::get_constant<T>` for
  int32 emits `llvm::APInt(32, ...)`
  (`taichi/runtime/llvm/llvm_context.cpp:738-740`). So these are **i32
  constants**.
- line 177: `CreateUDiv(CreateURem(l, prev), next)` — i32 unsigned div/rem.
- lines 178-185: `PhysicalCoordinates_get_val` / `_set_val`, then
  `CreateMul(in, get_constant(extractors[i].shape))` and `CreateAdd`. The
  multiply is an **i32 multiply with no overflow protection** — this is where
  a coordinate silently truncates once `shape * acc_shape` exceeds 2^31.

The `acc_shape * shape` product at line 174 is computed in **host C++ `int`
arithmetic before it ever reaches LLVM**, so it can overflow in the compiler
itself, not just in emitted code.

`PhysicalCoordinates` is a runtime struct — need to check its element type.

## 3. LLVM path — index and address arithmetic, file `codegen/llvm/codegen_llvm.cpp`

Verified by reading. All line numbers are in
`taichi/codegen/llvm/codegen_llvm.cpp` unless stated.

### 3.1 Linearisation — the single most important site
`visit(LinearizeStmt *)`, lines 1736-1744:
```
llvm::Value *val = tlctx->get_constant(0);
for (int i = 0; i < (int)stmt->inputs.size(); i++) {
  val = builder->CreateAdd(
      builder->CreateMul(val, tlctx->get_constant(stmt->strides[i])),
      llvm_val[stmt->inputs[i]]);
}
```
`get_constant(0)` with an `int` literal produces `APInt(32, ...)`
(`taichi/runtime/llvm/llvm_context.cpp:738-740`). `LinearizeStmt::strides` is
`std::vector<int>` (checked `taichi/ir/statements.h`). So the accumulator, the
strides and therefore the whole Horner chain are **i32**. LLVM would reject a
mismatched Add, so `llvm_val[stmt->inputs[i]]` must be i32 too. This is the
Taichi-index-to-linear-cell-index conversion, and it wraps silently at 2^31.

### 3.2 Range-for loop variable
- `create_naive_range_for`, lines 1169-1171:
  `loop_var_ty = tlctx->get_data_type(PrimitiveType::i32)` and
  `create_entry_block_alloca(PrimitiveType::i32)`. Hardcoded i32, not derived
  from the statement.
- Comparisons at 1188 and 1192 are `ICMP_SLT` / `ICMP_SGE`, i.e. **signed**.
  Range-for is therefore capped at 2^31-1 iterations, and by signed
  comparison, not merely by width.
- `get_range_for_bounds`, lines 2063-2087: both bounds go through
  `PrimitiveType::i32` explicitly (lines 2071-2073, 2080-2084), and the
  constant form uses `stmt->begin_value` / `end_value` which are `int32` in
  `taichi/ir/statements.h:1415-1416`.

### 3.3 Struct-for loop index
- line 2157: `auto loop_index_ty = llvm::Type::getInt32Ty(*llvm_context);`
  hardcoded i32 for the struct-for element index.
- line 2175: `call(refine, parent_coordinates, block_corner_coordinates,
  tlctx->get_constant(0))` — the refine function's third arg is i32 by
  `struct_llvm.cpp:152`.

### 3.4 Reading a loop index back
`visit(LoopIndexStmt *)`, lines 2319-2342. **Both** branches hardcode the load
type:
- line 2336 `builder->CreateLoad(llvm::Type::getInt32Ty(*llvm_context), GEP)`
  for the struct-for case, reading out of `PhysicalCoordinates`.
- lines 2339-2340, same i32 for the range-for / mesh-for case.

`visit(BlockCornerIndexStmt *)`, lines 2352-2379, is the one place that
**derives** the element type from the LLVM struct rather than hardcoding
(line 2374-2375 `val_ty_as_array->getElementType()`). It carries an explicit
comment at 2362-2364 restating `i32 val[taichi_max_num_indices]`. So if
`PhysicalCoordinates::val` were widened, this one site follows automatically
and `LoopIndexStmt` does not.

### 3.5 Pointer / address arithmetic
- `visit(MatrixPtrStmt *)`, lines 1853-1873. The non-index branch (1864-1872)
  does `CreatePtrToInt(origin, i64)`, `CreateSExt(offset, i64)`, `CreateAdd`,
  `CreateIntToPtr`. Address arithmetic itself is **64-bit**, but the offset
  arrives as i32 and is sign-extended, so any wrap already happened upstream.
- `visit(ExternalPtrStmt *)`, lines 1875-1990:
  - lines 1920-1929: ndarray shapes are loaded as
    `tlctx->get_data_type(PrimitiveType::i32)`. Hardcoded i32.
  - line 1931 `auto linear_index = tlctx->get_constant(0);` — i32 accumulator.
  - lines 1932-1945: Mul/Add chain, all i32.
  - line 1960-1961: TensorType branch `CreateSExt(linear_index, i64)`, then
    multiplies by an explicitly i64 constant (line 1969-1972,
    `get_constant(get_data_type<int64>(), ...)`). So this branch is
    64-bit-clean **after** a 32-bit linearisation.
  - line 1988: the non-TensorType branch does
    `builder->CreateGEP(base_ty, base, linear_index)` with the raw **i32**
    index. LLVM sign-extends a narrow GEP index to pointer width, so the
    address is fine but the index arithmetic already wrapped.
- `visit(SNodeLookupStmt *)`, lines 1792-1829:
  - root case line 1805-1806: `CreateGEP(parent_ty, parent,
    llvm_val[stmt->input_index])`, index width inherited from the IR value.
  - dense/pointer/dynamic/bitmasked, lines 1812-1820: calls the generated
    `activate` and `lookup_element` struct functions with `input_index`.
    Those runtime signatures take `int i`
    (`taichi/runtime/llvm/runtime_module/runtime.cpp:314,318`), so the ABI
    itself is 32-bit.
  - quant_array case, lines 1821-1826: `get_constant(element_num_bits)` i32,
    multiplied by `input_index`, fed to `create_bit_ptr`.
- `create_bit_ptr`, lines 1748-1770: line 1755
  `TI_ASSERT(bit_offset->getType()->isIntegerTy(32));` — a **hard assertion
  that the bit offset is exactly i32**. Widening the bit offset trips this
  assertion. The comment at 1750-1753 documents the struct as
  `{ iX* byte_ptr; i32 bit_offset; }`.
- `visit(GlobalTemporaryStmt *)`, lines 2383-2391: already i64,
  `get_constant((int64)stmt->offset)`. This one is correct today.
- `visit(ThreadLocalPtrStmt *)`, lines 2393-2400: `get_constant(stmt->offset)`
  — `offset` is `std::size_t`, and `get_constant` for `std::size_t` emits
  `APInt(64, ...)` (`llvm_context.cpp:741-745`). i64. Correct today.
- `visit(BlockLocalPtrStmt *)`, lines 2402-2410: GEP with
  `{get_constant(0), llvm_val[stmt->offset]}` — mixed i32 zero and IR-supplied
  offset.

## 4. LLVM path — the runtime ABI that codegen must match

Codegen emits calls into the LLVM runtime module, so the runtime signatures
pin the widths. Verified in `taichi/runtime/llvm/runtime_module/runtime.cpp`:

| Site | Signature | Width |
|---|---|---|
| `runtime.cpp:288-290` | `struct PhysicalCoordinates { i32 val[taichi_max_num_indices]; }` | i32 |
| `runtime.cpp:314` | `Ptr (*lookup_element)(Ptr, Ptr, int i)` | int |
| `runtime.cpp:318` | `u1 (*is_active)(Ptr, Ptr, int i)` | int |
| `runtime.cpp:320` | `i32 (*get_num_elements)(Ptr, Ptr)` | i32 |
| `runtime.cpp:322-324` | `refine_coordinates(..., int index)` | int |
| `runtime.cpp:306` | `StructMeta::element_size` is `std::size_t` | 64 |
| `runtime.cpp:307` | `StructMeta::max_num_elements` is `i64` | **64 already** |
| `runtime.cpp:305` | `StructMeta::snode_id` is `i32` | i32 |
| `runtime.cpp:520` | `Element::loop_bounds[2]` is `int` | int |
| `runtime.cpp:43` | `using RangeForTaskFunc = void(RuntimeContext*, const char *tls, int i)` | int |
| `runtime.cpp:1385` | `using BlockTask = void(RuntimeContext*, char*, Element*, int, int)` | int |
| `runtime.cpp:1422-1428` | `parallel_struct_for(ctx, int snode_id, int element_size, int element_split, ...)` | int |
| `runtime.cpp:1511-1520` | `cpu_parallel_range_for(ctx, int num_threads, int begin, int end, int step, int block_dim, ...)` | int |
| `runtime.cpp:1541-1547` | `gpu_parallel_range_for(ctx, int begin, int end, ...)` | int |
| `runtime.cpp:1548` | `int idx = thread_idx() + block_dim()*block_idx() + begin;` | int, and the multiply can overflow |

Corresponding codegen sites that match these signatures:
- `codegen_llvm.cpp:278` `common.set("snode_id", tlctx->get_constant(snode->id))`
  — `SNode::id` is `int` (`taichi/ir/snode.h:88`). i32.
- `codegen_llvm.cpp:279` `element_size` as `get_constant((uint64)element_size)`
  — i64, already correct.
- `codegen_llvm.cpp:280-281` `max_num_elements` via
  `snode->max_num_elements()` which returns `int64` — i64, already correct.
  Surprise #2: `max_num_elements` is the only piece of SNode geometry that is
  already 64-bit end to end, on both the codegen and runtime sides.
- `codegen_llvm.cpp:2290-2291`
  `int list_element_size = std::min(leaf_block->max_num_elements(), (int64)taichi_listgen_max_element_size);`
  — an `int64` narrowed into an `int`. Safe only because
  `taichi_listgen_max_element_size` is 1024 (`taichi/inc/constants.h:27`).
- `codegen_llvm.cpp:2308-2311` passes `leaf_block->id`, `list_element_size`,
  `num_splits` to `parallel_struct_for` as i32.

### 4.1 Device flavours of the LLVM path
All four repeat the same i32 shape:
- `cpu/codegen_cpu.cpp:57` body signature third arg is
  `tlctx->get_data_type<int>()`; `:59` `create_entry_block_alloca(PrimitiveType::i32)`;
  `:103` mesh loop index `getInt32Ty`; `:110,:126` loads via `getInt32Ty`;
  `:98` `ICMP_SLT` signed.
- `cuda/codegen_cuda.cpp:480` body third arg `get_data_type<int>()`;
  `:482` loop var `PrimitiveType::i32`; `:503` `gpu_parallel_range_for` call;
  `:522-523` mesh loop index i32; `:533-535` `ICMP_SLT`; `:549` i32 add.
- `amdgpu/codegen_amdgpu.cpp:258` loop var `PrimitiveType::i32`;
  `:464,:466` i32 workgroup dims.
- `dx12/codegen_dx12.cpp:42` loop var `PrimitiveType::i32`; `:82-83` i32 mesh
  loop index; `:86,:89` i32 thread idx / block dim constants;
  `:98` `ICMP_SLT`; `:112` i32 add.
- `dx12/dx12_lower_intrinsic.cpp:54,83-84` i32 zero / i32 block dim constants
  for the DXIL thread-id intrinsics. DXIL thread/group ids are i32 by the
  DirectX intrinsic definition, so this one is externally fixed, not a Taichi
  choice.

None of the four device files contain any 64-bit index path at all.

## 5. SPIR-V path — the Int64 capability. Section 6.2's specific question.

### 5.1 It is declared, and it is checked. Both.
- Declared: `taichi/codegen/spirv/spirv_ir_builder.cpp:64-66`
  ```
  if (caps_->get(cap::spirv_has_int64)) {
    ib_.begin(spv::OpCapability).add(spv::CapabilityInt64).commit(&header_);
  }
  ```
- The i64/u64 SPIR-V types are only *declared* under the same guard:
  `spirv_ir_builder.cpp:166-169` in `init_pre_defs()`. `t_int64_` and
  `t_uint64_` stay default-constructed `SType` if the capability is absent.
- Checked at use: `IRBuilder::get_primitive_type`,
  `spirv_ir_builder.cpp:310-313` (i64) and `:325-328` (u64), both
  `TI_ERROR("Type {} not supported.", ...)` if the cap is off.
- By contrast `t_int32_` / `t_uint32_` are declared unconditionally
  (`spirv_ir_builder.cpp:164-165`) and returned with no guard
  (`:308-309`, `:323-324`). i32 is the only integer width the SPIR-V path
  treats as always present.

So the mechanism section 6.2 asks for — express a target that lacks Int64 —
**already exists in the SPIR-V path**, and it fails loudly rather than
silently. That is the good news. The bad news is in 5.2.

### 5.2 Where the capability is set, and the surprise
- Vulkan: `taichi/rhi/vulkan/vulkan_device_creator.cpp:630-633`, gated on
  `VkPhysicalDeviceFeatures::shaderInt64`. Correct.
- OpenGL: `taichi/rhi/opengl/opengl_device.cpp:509-512`, set unconditionally
  for the non-GLES profile, with the comment "64bit isn't supported in ES
  profile". So on desktop GL it is *assumed*, not queried.

Surprise #3, and it is a big one. `spirv_has_physical_storage_buffer` is the
capability that selects 64-bit pointers. It is set in exactly one place,
`vulkan_device_creator.cpp:820-828`, and that place is inside:
```
#if !defined(__APPLE__) && false
          caps.set(DeviceCapability::spirv_has_physical_storage_buffer, true);
#endif
```
`&& false`. It is **permanently disabled** upstream (comment cites
taichi-dev/taichi issue 6295 and "until device capability is ready"). Nothing
else in the tree sets it. Therefore `has_buffer_ptr` is false everywhere
today, and every consumer of it takes the 32-bit branch.

### 5.3 The consequence: SPIR-V pointers are u32
`IRBuilder::from_taichi_type`, `spirv_ir_builder.cpp:334-341`:
```
} else if (dt->is<PointerType>()) {
    if (has_buffer_ptr) { return t_uint64_; } else { return t_uint32_; }
```
and the identical shape in `spirv_types.cpp:486-494`
(`translate_ti_type`). With `has_buffer_ptr` dead-false, **a Taichi pointer is
a 32-bit unsigned integer in every SPIR-V module Taichi currently emits.**

Consumers of `has_buffer_ptr` in codegen: `spirv_codegen.cpp:783`, `:2339-2340`,
`:2415-2416`, `:2490-2491`.

Note also a latent hazard: if `spirv_has_physical_storage_buffer` were ever
enabled without `spirv_has_int64`, `from_taichi_type` returns `t_uint64_`
which was never declared. The Vulkan setter does guard on
`device_supported_features.shaderInt64` (`vulkan_device_creator.cpp:823`), so
the guard exists, but it lives in the RHI, not in the builder. `from_taichi_type`
itself does not check.

## 6. SPIR-V path — index and address arithmetic

All line numbers `taichi/codegen/spirv/spirv_codegen.cpp` unless stated.

### 6.1 `use_64bit_pointers`
Line 82: `const bool use_64bit_pointers = false;` — a hardcoded const member of
`TaskCodegen`. It is read in exactly one place, `make_pointer`, lines 2317-2324:
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
Note the explicit `uint32_t(offset)` narrowing cast on a `size_t` argument in
the taken branch. Grep confirms `use_64bit_pointers` exists nowhere else in
the tree. Surprise #4: the 64-bit-pointer switch already exists in the SPIR-V
path, is hardcoded false, and its own comment says the 64-bit branch was never
finished ("should check out how to encode uint64 values in spirv").

Callers of `make_pointer`, i.e. every SNode address constant in SPIR-V:
- line 355 `visit(GetRootStmt)` — `make_pointer(0)`, the root base.
- line 371 `visit(GetChStmt)` — `make_pointer(desc.mem_offset_in_parent_cell)`.
- line 408 `bitmasked_activation` —
  `make_pointer(desc.cell_stride * desc.snode->num_cells_per_container)`.
- line 506 `visit(SNodeLookupStmt)` — `make_pointer(desc.cell_stride)`.

### 6.2 SNode address computation
`visit(SNodeLookupStmt)`, lines 467-511:
```
spirv::Value input_index_val = ir_->cast(parent_val.stype, query_value(...));
spirv::Value stride = make_pointer(desc.cell_stride);
spirv::Value offset = ir_->mul(input_index_val, stride);
val = ir_->add(parent_val, offset);
```
Divergence from LLVM worth noting: SPIR-V **casts the index up to the pointer
type before multiplying** (line 504-505), so if `make_pointer` returned u64
the multiply would happen at 64 bits. The LLVM path has no equivalent widen
step; it multiplies at i32 and only widens at the GEP. So structurally the
SPIR-V path is closer to being width-parameterised than the LLVM path is —
which I did not expect.

`visit(GetChStmt)`, lines 358-378: `add(input_ptr_val, make_pointer(offset))`.
Pointer arithmetic at whatever `make_pointer` returns.

### 6.3 The byte-pointer to SSBO-index conversion
`at_buffer`, lines 2194-2220. This is the address-to-storage-index bottleneck:
```
size_t width = ir_->get_primitive_type_size(dt);
spirv::Value idx_val = ir_->make_value(
    spv::OpShiftRightLogical, ptr_val.stype, ptr_val,
    ir_->uint_immediate_number(ptr_val.stype, size_t(std::log2(width))));
spirv::Value ret = ir_->struct_array_access(get_primitive_type(dt), buffer, idx_val);
```
`ptr_val.stype` is u32 today, so the byte pointer, the shift and the resulting
SSBO element index are all u32. **Root buffers are capped at 4 GiB by the
SPIR-V pointer representation, independently of any SNode count.**

Lines 2197-2205 are the alternative: when `ptr_val.stype.dt == u64` it emits
`OpConvertUToPtr` into `StorageClassPhysicalStorageBuffer`. This is the real
64-bit address path. It is currently unreachable because nothing sets
`spirv_has_physical_storage_buffer` (see note 5.2) and `use_64bit_pointers` is
false.

`IRBuilder::struct_array_access`, `spirv_ir_builder.cpp:770-790`, emits
`OpAccessChain` with `const_i32_zero_` and the index. The index operand width
follows whatever is passed in.

### 6.4 Linearisation, SPIR-V
`visit(LinearizeStmt)`, lines 531-541:
```
spirv::Value val = ir_->const_i32_zero_;
... strides_val = ir_->int_immediate_number(ir_->i32_type(), stmt->strides[i]);
val = ir_->add(ir_->mul(val, strides_val), input_val);
```
Identical i32 shape to the LLVM path (`codegen_llvm.cpp:1736-1744`). **Both
paths linearise at 32 bits.** This is the single point where the two agree and
both are wrong for a widened index.

### 6.5 Other 32-bit sites in SPIR-V
- `visit(GlobalTemporaryStmt)`, lines 707-712:
  `ir_->int_immediate_number(ir_->i32_type(), stmt->offset, false)`.
  `stmt->offset` is `std::size_t` (`taichi/ir/statements.h`), narrowed to i32.
  **Divergence:** the LLVM path uses `get_constant((int64)stmt->offset)`
  (`codegen_llvm.cpp:2385`). Same statement, different widths in the two paths.
- `visit(ExternalTensorShapeAlongAxisStmt)`, lines 714-732: ndarray shape read
  as i32 (`ir_->i32_type()` at 727, 729).
- `visit(ExternalPtrStmt)`, lines 734-800:
  - line 737 `linear_offset = int_immediate_number(i32_type(), 0)`.
  - lines 751-755 shape loads as i32.
  - lines 764-765 element-shape immediates as i32.
  - lines 770-771 `mul` / `add` chain at i32.
  - lines 772-776 `OpShiftLeftLogical` by `log2(element size)` **at i32** —
    the byte offset into an external array is an i32.
  - lines 777-781: if `spirv_has_no_integer_wrap_decoration`, the offset is
    decorated `DecorationNoSignedWrap`. Taichi is asserting to the driver that
    this i32 byte offset does not overflow. At scale that assertion becomes
    false and the behaviour is undefined, not merely wrapped.
  - lines 783-792: the physical-storage-buffer branch, `OpSConvert` to u64.
    Unreachable today.
- `bitmasked_activation`, lines 380-434: the bitmask word index/offset chain.
  Lines 405, 411-412, 414 hardcode `ir_->u32_type()` for the word pointer, the
  access and the atomics. Lines 394 and 397 use `ptr_dt` (the parent pointer
  type) for the >>5 and &31, so those follow the pointer width, but the
  subsequent `OpShiftLeftLogical` at 405 and the
  `OpShiftRightLogical`/`struct_array_access` at 411-414 force u32 regardless.
  So bitmasked SNodes are hard-wired to a u32 bitmask address space even if
  `make_pointer` were widened.
- `visit(LoopIndexStmt)`, lines 543-563: only `range_for` and `RangeForStmt`
  are handled; anything else is `TI_NOT_IMPLEMENTED` (lines 550, 562).
  The SPIR-V path has **no struct-for**, so `generate_refine_coordinates` and
  `PhysicalCoordinates` have no SPIR-V counterpart at all. Confirms the two
  paths are not symmetric in what a width change must touch.

## 7. Correction to note 6.5, and SPIR-V loop / dispatch widths

Correction: I wrote in 6.5 that the SPIR-V path has no struct-for. That is
wrong and I am striking it. `generate_struct_for_kernel` exists at
`spirv_codegen.cpp:2123-2189`. What is true is narrower:
`visit(LoopIndexStmt)` at `:543-563` handles only `range_for` for an
`OffloadedStmt` and `TI_NOT_IMPLEMENTED`s everything else (line 550), while
`generate_struct_for_kernel` registers its index under the name `"ii"`
(line 2172). How a struct-for body reads its index in the SPIR-V path without
going through `LoopIndexStmt` is not something I can settle from within my
territory. **Escalating** rather than resolving.

What is verifiable about `generate_struct_for_kernel`:
- lines 2137-2141: the ListGen buffer is `PrimitiveType::u32` and the element
  count is loaded as `ir_->u32_type()`.
- line 2146: `loop_index_var = ir_->alloca_variable(ir_->u32_type())`.
- lines 2164-2167: each listgen entry is a **u32** SNode cell index.
- lines 2178-2185: grid-stride increment computed at u32.
- line 2126: `advisory_total_num_threads = 65536` hardcoded; line 2127
  `advisory_num_threads_per_group = 128` hardcoded.

So the SPIR-V struct-for element list is a u32 index list end to end. There is
no 64-bit variant.

### 7.1 SPIR-V range-for
`generate_range_for_kernel`, `spirv_codegen.cpp:1985-2121`:
- lines 1989-1992 copy `stmt->begin_value`/`begin_offset` into
  `TaskAttributes::RangeForAttributes::begin`/`end`, which are `size_t`
  (`taichi/codegen/spirv/kernel_utils.h:110-111`). Widening on the host side.
- line 1999 `const int num_elems = range_for_attribs.end - range_for_attribs.begin;`
  — a `size_t` difference narrowed back into an `int`.
- lines 2000-2004: `begin_expr_value` and `total_elems` emitted as
  `ir_->i32_type()`.
- line 2004 `task_attribs_.advisory_total_num_threads = num_elems;` —
  `advisory_total_num_threads` is `int` (`kernel_utils.h:99`).
- lines 2017-2044: the non-const-range path reads bounds out of the global
  temporaries buffer, entirely at `ir_->i32_type()`, including the
  `OpShiftRightArithmetic` by 2 that converts a byte offset to a u32-word
  index (lines 2018-2021, 2031-2034).
- line 2054 `begin_ = add(cast(i32_type(), get_global_invocation_id(0)), begin_expr_value)`
  — the invocation id is cast down to i32.
- lines 2064-2070 `total_invocs` computed at u32 then cast to i32.
- line 2095 `loop_var = ir_->make_phi(begin_.stype, 2)` — i32 induction var.
- line 2097 `loop_cond = ir_->lt(loop_var, end_)` — signed less-than on i32.

`visit(RangeForStmt)` (the non-offloaded, serial form),
`spirv_codegen.cpp:1773-1830`: induction variable width follows
`init_value.stype` (line 1797), so it is whatever the frontend gave the
begin/end statements. Lines 1791, 1821, 1823 use `ir_->const_i32_one_` for the
step, which forces i32.

### 7.2 The SPIR-V struct compiler is 64-bit clean on the host
`taichi/codegen/spirv/snode_struct_compiler.h:13-36`: `SNodeDescriptor` uses
`size_t` for `cell_stride`, `container_stride`, `total_num_cells_from_root`
and `mem_offset_in_parent_cell`. `CompiledSNodeStructs::root_size` is `size_t`
(`:42`). `compute_snode_size` in `snode_struct_compiler.cpp` accumulates in
`std::size_t` throughout.

So the SPIR-V layout computation never loses width. The only narrowing is at
emission time, in `make_pointer`'s `uint32_t(offset)` cast
(`spirv_codegen.cpp:2322`).

Also: `snode_struct_compiler.cpp` contains **no** `taichi_max_num_snodes`
check of any kind. `snode_descriptors_` is an `unordered_map<int, SNodeDescriptor>`
(`snode_struct_compiler.h:38`), unbounded. The SPIR-V path has no SNode-count
ceiling at all. The 1024 limit is an LLVM-path-only construct.

## 8. The two paths' entry points, and section 6.3

- LLVM path entry: `KernelCodeGen::create`, `taichi/codegen/codegen.cpp:36-72`.
  Dispatch is on `compile_config.arch` alone, wrapped in `#if defined(TI_WITH_CUDA)`
  / `TI_WITH_DX12` / `TI_WITH_AMDGPU` blocks, each falling through to
  `TI_NOT_IMPLEMENTED` when the backend was not compiled in.
- `LLVM::KernelCompiler::compile`,
  `taichi/codegen/llvm/kernel_compiler.cpp:29-48`: the signature takes
  `const DeviceCapabilityConfig &device_caps` (line 31) and **never uses it**.
  It is not forwarded to `KernelCodeGen::create` at line 35-36. Verified by
  reading the whole function.
- SPIR-V path entry: `spirv::KernelCompiler::compile`,
  `taichi/codegen/spirv/kernel_compiler.cpp:25-45`, line 37
  `params.caps = device_caps;`, which reaches `TaskCodegen::caps_` and every
  `caps_->get(...)` site listed in note 5.

Surprise #5: the two paths are asymmetric in exactly the dimension section 6.2
cares about. The SPIR-V path is capability-parameterised from the compiler
entry point down. The LLVM path has the capability parameter in its signature
and drops it on the floor. Any "express a target that lacks feature X"
architecture has a plumbing point on one side and nothing on the other.

Recording, not resolving: whether that asymmetry is intended (LLVM targets are
selected at build time by `TI_WITH_*` and CPU/CUDA/AMDGPU do not lack integer
widths) is a judgement call above my assignment. **Escalating.**

## 9. Offline cache / serialisation

`taichi/codegen/compiled_kernel_data.h:30` `kHashSize = 64`;
`compiled_kernel_data.cpp:82-93` `update_hash()` hashes metadata plus source.
There is no explicit ABI or width version field in `CompiledKernelDataFile`.
If index or address width changed, cached artefacts would differ only through
the hash of the emitted source, not through an explicit version.
`spirv/kernel_compiler.cpp:42`
`internal_data.metadata.num_snode_trees = config_.compiled_struct_data->size();`
is the only SNode-tree count recorded in a compiled artefact I found in my
territory.

## 10. Files in my territory with nothing to report

Read and found no index/address width or SNode-count content:
- `taichi/codegen/codegen_utils.h` — printf specifier parsing and a vector
  codegen predicate only.
- `taichi/codegen/codegen.h` — declarations and the codegen design comment.
- `taichi/codegen/compiled_kernel_data.{h,cpp}` — beyond note 9.
- `taichi/codegen/llvm/llvm_compiled_data.h`, `llvm/compiled_kernel_data.*`,
  `llvm/kernel_compiler.h`, `spirv/compiled_kernel_data.*`,
  `spirv/kernel_compiler.h` — plumbing.
- `taichi/codegen/llvm/codegen_llvm_quant.cpp` — the i32 uses at lines 193,
  222, 313, 333, 369, 390, 398, 422, 514, 535-537 are f32 bit manipulation for
  quantised float encode/decode, not indices or addresses. Its only index-side
  contact is via `load_bit_ptr` (lines 23, 34, 74), which inherits the i32
  bit-offset assertion from `codegen_llvm.cpp:1755`.
- All `CMakeLists.txt` under `taichi/codegen/`.

## 11. What in codegen scales with SNode count (item 6.1)

### 11.1 The assertion is checking the wrong quantity
This is the most consequential thing I found on 6.1, and it is verified, not
inferred.

- `struct_llvm.cpp:266` `TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);`
- `StructCompiler::snodes` (`taichi/struct/struct.h:11`) is filled by
  `collect_snodes` (`taichi/struct/struct.cpp:7-13`), which walks **one root**.
- `LlvmProgramImpl::compile_snode_tree_types_impl`
  (`taichi/runtime/program_impls/llvm/llvm_program.cpp:45-56`) constructs a
  **fresh** `StructCompilerLLVM` per SNode tree (line 51-52) and calls
  `run(*root)` (line 53). So `snodes.size()` is the count for that one tree.
- The three runtime arrays the constant sizes
  (`taichi/runtime/llvm/runtime_module/runtime.cpp:567-569`) are indexed by
  `snode_id`, which is `SNode::id` — see `runtime.cpp:1005, 1016, 1029, 1038,
  1271, 1287-1288, 1334-1336, 1429, 1692, 1723, 1740, 1784`.
- `SNode::id` comes from a **process-global** `static std::atomic<int> counter`
  (`taichi/ir/snode.cpp:12`, assigned at `snode.cpp:220` `id = counter++`).
  It is reset in exactly two places: `SNode::reset_counter()`
  (`taichi/ir/snode.h:348-350`) and `Program`'s constructor
  (`taichi/program/program.cpp:144` `SNode::counter = 0;`). Not per tree, not
  on tree destruction.
- `kMaxNumSnodeTreesLlvm = 512` (`taichi/inc/constants.h:13`) bounds the number
  of trees (`runtime.cpp:562-563`).

So: the assertion bounds **per-tree SNode count** while the arrays it stands in
for are indexed by a **program-global, monotonically increasing** id. Several
trees each well under 1024 can drive the global id past 1024 with the assertion
never firing. Whatever replaces 1024 has to decide which of the two quantities
it is bounding. **Flagging for the planner and for territory 03**, not
resolving.

### 11.2 Emitted code volume per SNode, LLVM path
`StructCompilerLLVM::run` (`struct_llvm.cpp:245-274`) emits, per SNode in a
tree:
1. one dummy function `<name>_type_stubs_func` — `struct_llvm.cpp:142-143`,
   for every non-bit-level SNode.
2. one `refine_coordinates` function for every non-leaf SNode —
   `struct_llvm.cpp:199-200` calling `:145-188`. Its body is a **fully
   unrolled loop of `taichi_max_num_indices` = 12 iterations**
   (`struct_llvm.cpp:171`), each up to a UDiv, a URem, a Mul, an Add and two
   calls (`:177-185`).
3. one `get_ch_from_parent` function for every non-root SNode —
   `struct_llvm.cpp:204-232`.

So struct-module size is roughly linear in SNode count, with the per-SNode
constant set by `taichi_max_num_indices`. Raising the SNode ceiling raises
LLVM struct-module compile time and module size proportionally. That is a real
cost on item 6.1, distinct from the runtime array footprint.

Per kernel task, `emit_struct_meta_object` (`codegen_llvm.cpp:228-257`) plus
`emit_struct_meta_base` (`:259-311`) build a `StructMeta` on the stack for each
SNode the task touches, including two `get_struct_function` lookups
(`:301-310`). That scales with SNodes touched per kernel, not the ceiling.

### 11.3 SPIR-V path
No scaling with the SNode ceiling and no bound of any kind — see note 7.2.
`snode_descriptors` is an unbounded `unordered_map`. The SPIR-V struct
compiler emits no per-SNode functions; addresses are folded into immediates at
the use site via `make_pointer`.

## 12. A safety property of the LLVM path worth recording

`check_func_call_signature` (`taichi/codegen/llvm/llvm_codegen_utils.cpp:103-144`)
compares every emitted call's argument types against the callee's declared
parameter types and `TI_ERROR`s on any mismatch that is not a same-type pointer
cast (lines 127-142). It is on every path into a runtime call:
`llvm_codegen_utils.h:110` (`LLVMModuleBuilder::call`),
`llvm_codegen_utils.h:212` (`RuntimeObject::call`),
`codegen_llvm.cpp:1715` (`call_struct_func`).

Consequence for item 6.2: in the LLVM path, if index or address types in
codegen were widened without simultaneously widening the corresponding
signatures in the runtime bitcode module, the failure would be a **loud compile
error naming the parameter**, not a silent truncation. Codegen and the runtime
module have to be widened together, and the compiler enforces that.

The SPIR-V path has no equivalent. It has no external ABI to check against; the
narrowing happens at `uint32_t(offset)` in `make_pointer`
(`spirv_codegen.cpp:2322`) and at `int_immediate_number(i32_type(), <size_t>)`
in `visit(GlobalTemporaryStmt)` (`spirv_codegen.cpp:708`), silently.

Additional SPIR-V-format constraint, verified: `InstrBuilder` has exactly one
scalar `add` overload, `add(const uint32_t &)`
(`spirv_ir_builder.h:149-159`). Every literal operand is a 32-bit SPIR-V word.
So `size_t` values passed to `decorate(...)` narrow implicitly. This is how
`DecorationOffset` gets `nth_element_offset` (a `size_t`) at
`spirv_types.cpp:451-452` and how `DecorationArrayStride` gets `nbytes` at
`spirv_types.cpp:471` and `spirv_ir_builder.cpp:605`. SPIR-V struct member
offsets and array strides are **32-bit literals by the format**, not by
Taichi's choice.

## 13. Remaining LLVM sites checked, nothing new

- `visit(SNodeOpStmt)`, `codegen_llvm.cpp:1367-1396`: `allocate`, `length`,
  `is_active`, `activate`, `deactivate` all go through
  `call(snode, ptr, method, {val})` (`:1674-1693`), which prefixes the runtime
  SNode-type name and dispatches to a runtime function whose index parameter is
  `int` (see note 4).
- `get_root(int snode_tree_id)`, `:2689-2692`: `get_constant(snode_tree_id)`,
  i32 tree id into `LLVMRuntime_get_roots`, backing `roots[kMaxNumSnodeTreesLlvm]`
  (`runtime.cpp:562`).
- `visit(MeshPatchIndexStmt)`, `:2565-2567`: `get_arg(2)`, the i32 body arg.
- `visit(ExternalTensorShapeAlongAxisStmt)`, `:1992-1999` and
  `visit(ExternalTensorBasePtrStmt)`, `:2001-2007`: read out of the ndarray
  struct type; width follows `TypeFactory`'s ndarray struct, which is territory
  01. The shape field is read as i32 in `visit(ExternalPtrStmt)` at `:1926-1927`.
- `visit(IntegerOffsetStmt)`, `:1746`: `TI_NOT_IMPLEMENTED`. Dead in the LLVM
  path.

## 14. Resolving part of the escalation in note 7

`CompileConfig::fit()`, `taichi/program/compile_config.cpp:72-74`:
```
if (arch_uses_spirv(arch)) {
  demote_dense_struct_fors = true;
}
```
and `demote_dense_struct_fors` defaults true anyway (`compile_config.cpp:18`).
The pass runs at `taichi/transforms/compile_to_offloads.cpp:191-196`. So dense
struct-fors are rewritten to range-fors before the SPIR-V codegen sees them,
which is why `visit(LoopIndexStmt)` at `spirv_codegen.cpp:543-563` only handles
`range_for`.

What that does **not** explain is `generate_struct_for_kernel`
(`spirv_codegen.cpp:2123-2189`) still existing and registering `"ii"`
(line 2171), with no visitor that reads `"ii"` for a `struct_for` task type —
`query_value("ii")` appears at line 549 only, inside the `range_for` branch.
Whether a non-dense (bitmasked / pointer) struct-for can reach the SPIR-V
codegen is outside my territory. **Still escalating that narrower question.**

## 15. LLVM struct-for body, remaining detail

`create_offload_struct_for`, `codegen_llvm.cpp:2088-2317`, verified:
- line 2213-2215 loop test `ICMP_SLT` on the i32 `loop_index` against
  `upper_bound` (which is `get_arg(4)`, the `BlockTask` `int` parameter,
  `runtime.cpp:1385`).
- line 2224 `new_coordinates = create_entry_block_alloca(physical_coordinate_ty)`
  — `PhysicalCoordinates`, i.e. `i32 val[12]`.
- lines 2226-2227 `call(refine, parent_coordinates, new_coordinates,
  CreateLoad(loop_index_ty, loop_index))` — the i32 cell index into the
  generated refine function.
- lines 2231-2235 the bit-vectorised second refine, `get_constant(0)` i32.
- lines 2250-2252 `is_active` called with the i32 loop index.
- line 2272 `create_increment(loop_index, block_dim)` — i32 stride.

So the whole struct-for driver is i32: the loop index, the coordinate array,
the refine argument, the activity query and the increment.

## 16. Dead ends and negative results

Recording these so the adversaries know they were checked, not skipped.

1. **Expected more `taichi_max_num_snodes` hits in codegen.** Grep across the
   whole tree returns 4: `constants.h:12` (definition),
   `struct_llvm.cpp:266` (the assertion), and `runtime.cpp:567-569` (the three
   arrays). Nothing in the SPIR-V path, nothing in cpu/cuda/amdgpu/dx12,
   nothing in `llvm_codegen_utils`.
2. **Looked for a size cap from the SPIR-V root buffer's array type.** There
   isn't one. `buffer_argument` calls `get_struct_array_type(value_type, 0)`
   (`spirv_ir_builder.cpp:747`), and `get_array_type` with `num_elems == 0`
   emits `OpTypeRuntimeArray` (`spirv_ir_builder.cpp:583-589`). The cap is the
   u32 element index in `at_buffer`, not the type.
3. **Looked for a 64-bit index path anywhere in cpu/cuda/amdgpu/dx12 codegen.**
   None exists. Every loop variable in all four is
   `create_entry_block_alloca(PrimitiveType::i32)` or `getInt32Ty`, and every
   comparison is signed.
4. **Checked `llvm_codegen_utils.{h,cpp}` for index or address width
   assumptions.** None. Grepping for `32`, `i32`, `int32` in both files returns
   nothing but `check_func_call_signature` machinery.
5. **Checked `spirv/kernel_utils.{h,cpp}` and `spirv/lib_tiny_ir.h`.** All host-
   side sizes/offsets/strides are `size_t` (`kernel_utils.h:110-111, 165, 167,
   171, 278, 285, 339-340`; `lib_tiny_ir.h:127-209`). No 32-bit narrowing on
   the host side. The one `i32` in `kernel_utils.cpp:89-93` is a documented
   placeholder for a return-value dtype, unrelated to indices.
6. **Checked whether `taichi_max_num_indices` appears anywhere in codegen other
   than `struct_llvm.cpp:171`.** It does not; the only other mention is the
   explanatory comment at `codegen_llvm.cpp:2363`. But
   `physical_coordinate_ty` (the `PhysicalCoordinates` LLVM type) is sized by
   it indirectly through the runtime module, and `BlockCornerIndexStmt`
   (`codegen_llvm.cpp:2356-2379`) asserts on its exact shape.
7. **`visit(IntegerOffsetStmt)` in both paths.** LLVM: `TI_NOT_IMPLEMENTED`
   (`codegen_llvm.cpp:1746`). SPIR-V: no visitor at all. Dead statement in my
   territory; noting only so nobody assumes it is an offset-widening hook.

## 17. Line-number corrections to note 4.1

Checked the device-flavour line numbers again before finalising the report.

- Note 4.1 gives `cpu/codegen_cpu.cpp:98` for the mesh-for `ICMP_SLT`. **Wrong.**
  The correct location is `cpu/codegen_cpu.cpp:111-112`
  (`CreateICmp(ICMP_SLT, loop_index_load, ...)`). Corrected in the report; the
  original error left standing here because these are contemporaneous notes.
- `dx12/codegen_dx12.cpp:98` is inside the `CreateICmp` call but is the operand
  line; the call spans `:96-98`. Report uses `:96-98`.
- Everything else in note 4.1 re-checked against the source and is correct.

---

# Revision pass, 2026-09-09

Notes 18 onwards are the correction pass. Round one's report went to two
adversaries; the planner arbitrated and sent it back. Same conventions as
above: written as I go, nothing reconstructed, errors left standing where they
were made and corrected in place with a note.

## 18. What I was told, and what I checked first

Directed to correct four things: (1) the section 4 Int64 claim is false,
(2) the defect is wider than Int64, (3) section 5 stopped one call site short
of an out-of-bounds store, (4) OpenGL ES is not the only in-tree target without
Int64 — Metal gates on `family_apple3`. Also to keep and re-verify what
survived, and to adjudicate the two adversaries against each other where they
differ.

Read `PROJECT-PLAN.md` again (sections 6, 9, 10), then both adversary files in
full, then went to the source. Deliberately did not take either adversary's
line numbers on trust — the two of them correct each other in places, so
neither is a safe base.

## 19. The Int64 claim. I was wrong.

Opened `taichi/codegen/spirv/spirv_types.cpp:393-419`.
`Translate2Spirv::visit_int_type` calls `spir_builder_->i64_type()` at `:403`
and `u64_type()` at `:415`. Neither goes through `get_primitive_type`.

Opened `spirv_ir_builder.h:529-534`. Both accessors are one-line
`return t_int64_;` / `return t_uint64_;`. No `caps_` in either.

Opened `spirv_ir_builder.cpp:166-169`. `t_int64_` and `t_uint64_` are assigned
only inside `if (caps_->get(cap::spirv_has_int64))`. Absent the capability they
stay default-constructed.

Opened `spirv_ir_builder.h:51`. `uint32_t id{0}`. Zero is not a valid SPIR-V
result id.

So the chain both adversaries described is exactly what the source says. My
round-one "**Checked at every use**" was false. I checked
`IRBuilder::get_primitive_type`, found it guarded, and generalised. Searched my
own round-one notes for `visit_int_type` and `Translate2Spirv`: no hits.
`spirv_types.cpp` appears at note 1 line 23 in the file-list sweep, at note
`:486-494` for `translate_ti_type`, and at `:451-452`/`:471` for the
decorations. I never opened the visitor. The adversary who pointed that out was
right about my notes as well as about the code.

Section 4 of the report is rewritten from scratch rather than patched.

## 20. The width of the defect. Five capabilities, eight call sites.

Read `spirv_types.cpp:393-431` line by line and the whole of
`get_primitive_type` (`spirv_ir_builder.cpp:288-332`).

Bare accessor calls in the two visitors: `:397` i8, `:399` i16, `:403` i64,
`:409` u8, `:411` u16, `:415` u64, `:424` f16, `:428` f64. Eight.

Capabilities gating those types in `init_pre_defs`: `spirv_has_int8`
(`:156-159`), `spirv_has_int16` (`:160-163`), `spirv_has_int64` (`:166-169`),
`spirv_has_float16` (`:171-173`), `spirv_has_float64` (`:174-176`). Five.

The arbitration note said "eight accessors across six optional widths". Eight
call sites is exactly right. On the count of widths I could not make six come
out of the source however I grouped it: eight types, five capability flags. I
have put the enumeration in the report as a table so nobody has to recount, and
said plainly that the flag count is five. Reporting what the source says rather
than echoing a number I cannot reproduce.

`bool_type()` at `:407` is also a bare accessor but `t_bool_` is unconditional
(`:155`), so it is not part of this.

## 21. Exact guard lines in `get_primitive_type`, since three documents disagree

Printed `:288-332` numbered. For each optional type the shape is: `else if`
opens, capability test, `TI_ERROR`, return. i64 is `:311`/`:312`/`:313`/`:314`.
u64 is `:325`/`:326`/`:327`/`:328`.

My round-one report said i64 at `:310-313`. `:310` is `return t_int32_;`, so
that was off by one at the start. My u64 `:325-328` was right. My f16
`:296-299` and f64 `:300-303` were both wrong — actual `:291-294` and
`:297-300`. My i32 `:308-309` was wrong; the branch is `:309-310`. Adversary
02-1 gave i64 as `:311-313`, adversary 02-2 as `:311-313` plus return `:314`.
Full table now in report section 4.2.

## 22. That the path is live. Traced it myself.

`ir_translate_to_spirv` is `spirv_types.cpp:476-483`. Grepped for callers:
`spirv_codegen.cpp:2386`, `:2464`, `:2511` — args struct, rets struct, argpack
struct. All three inside `TaskCodegen`.

Opened `compile_args_struct` (`spirv_codegen.cpp:2326-2400`). It translates
every element of `ctx_attribs_->args_type()->elements()` at `:2356-2367`, with
the recursive lambda at `:2343-2355`, and both call `translate_ti_type`
(`:2344`, `:2358`).

`translate_ti_type` is `:484-514`, not `:486-495` as my round-one report said.
It hands primitives to `translate_ti_primitive` at `:488`.

`translate_ti_primitive` is `:167-214`. i64 → `IntType(64, signed)` at
`:179-181`, u64 → `IntType(64, unsigned)` at `:199-201`. Signature at
`:167-168`: `(tinyir::Block &, const DataType)`. No capability parameter.

Then checked whether the guarded call earlier in the chain saves it.
`get_buffer_value` (`:2266-2315`) does call `ir_->get_primitive_type(dt)` at
`:2267`, but every `BufferType::Args` caller passes `PrimitiveType::i32`
hardcoded: `:618`, `:728`, `:753`, `:788`, `:2293`. Adversary 02-2 gave `:620`
for the `ArgLoadStmt` one; it is `:618`. So the guard runs on i32, passes, and
the unguarded translation then runs over every argument type.

## 23. Whether anything outside codegen rejects a 64-bit argument

Adversary 02-2 says nothing does, and rests it on `is_extension_supported`
being called once. Checked both halves separately.

`grep -rn data64` over `.cpp/.h/.mm/.inc.h`: `extensions.inc.h:6` (declaration),
`extension.cpp:12,16,20` (grants to x64, arm64, cuda), `extension.cpp:29-30`
(commented-out OpenGL grant). Nothing reads it. **Their conclusion holds.**

`grep -rn is_extension_supported`: `extension.cpp:8` (definition),
`extension.h:24`, `program.cpp:149` (assertion), `codegen_llvm.cpp:2726` (bls),
`compile_to_offloads.cpp:92,205,218,236,245,288` (mesh, quant),
`export_lang.cpp:1225` (Python binding). **Their supporting claim is wrong** —
it is called many times, just never for `data64`. Recorded the correction in
report section 7.5 and stated the fact the accurate way in section 4.4.

This is the kind of thing that would have propagated if I had transcribed an
adversary instead of checking.

## 24. The RHI capability landscape

Grepped `spirv_has_int64` across `.cpp/.h/.mm/.inc.h`, excluding `python/`.
Five setters, one declaration, four codegen readers.

- Vulkan `vulkan_device_creator.cpp:632`, gated `:630` on
  `device_supported_features.shaderInt64`. My round-one `:630-633` was right.
- OpenGL `opengl_device.cpp:511`, gated `:509` on `!is_gles()`, comment at
  `:510`. Right in round one.
- Metal `metal_device.mm:1052`, gated `:1051` on `feature_64_bit_integer_math`,
  which is `family_apple3` at `:1038`, from `supportsFamily:kMTLGPUFamilyApple3`
  at `:1035-1036`. **Missing from my round-one report entirely.** Pre-Apple3
  Metal is a second in-tree SPIR-V target without Int64.
- `c_api/src/taichi_opengl_impl.cpp:9`, no guard at all, then
  `get_gl().set_caps(std::move(caps))` at `:12` — whole-object replacement, so
  it discards the `is_gles()` guard `GLDevice::GLDevice` applied.
  `ti_import_opengl_runtime` takes `bool use_gles` at `:21-22` and forwards it
  to `set_gles_override` at `:26`. Confirmed by reading `:1-30`. Outside
  `taichi/` — escalated, not resolved.
- Direct3D 11: `dx_device.cpp:557-568` builds `caps{}` at `:563`, sets only
  `spirv_version` at `:564`, `set_caps` at `:565`. Never sets any integer
  capability. Third target without Int64.

Also found something neither adversary mentions: `metal_device.mm:131-135`
reads `caps.contains(spirv_has_int64)` to raise the MSL version to 2.3. A
consumer of the capability outside codegen. Not a guard, but worth having in
the report since section 6.2 asks where the capability is used at all.

## 25. The SNode assertion. Opened the write site.

`runtime.cpp:986-1017` is `runtime_initialize_snodes`. The loop is
`:1003-1006`, writing `element_lists[i]` for `i` in `[root_id, root_id +
num_snodes)`. Parameters `root_id` and `num_snodes` at `:988-989`.

Traced the arguments: `llvm_runtime_executor.cpp:442-444` calls it with
`root_id` (read at `:400`) and `(int)snode_metas.size()` (at `:444`).
`root_id` originates at `llvm_program.cpp:61`, `int root_id = tree->root()->id;`
— the global id.

So it is a range store keyed on a global id, bounded by an assertion on a
per-tree count. Members after the three arrays: `temporaries` `:570`,
`rand_states` `:571`. Out of range writes land in them.

Two filters neither of my round-one paragraphs had:
- `all_dense` early return at `:1000-1002`. Computed at
  `llvm_runtime_executor.cpp:402-410`. Adversary 02-2 said `:400-409`; it is
  `:402-410`. Also — neither adversary noticed — it is *seeded* from
  `config_.demote_dense_struct_fors` at `:402` and only then narrowed by the
  loop. Recorded.
- `is_gc_able` filter at `:447` for the other two arrays, which is
  `pointer || dynamic` (`snode_types.cpp:21-23`).

Both surviving routes need sparse SNodes. Brief section 4.1 says sparsity is
required. So the configuration that trips it is the one the project wants.

Other two write sites: `runtime.cpp:1029` (`runtime_NodeAllocator_initialize`,
`:1026-1031`) and `:1038` (`runtime_allocate_ambient`, `:1033-1040`), called
from `llvm_runtime_executor.cpp:460-466`. Adversary 02-2 gave `:1291-1296` and
`:1298-1304` for these — those lines are `element_listgen_root`, a different
function. Adversary 02-1's `:1029`/`:1038` are right. Adjudicated in favour of
02-1, recorded in report section 7.3.

Tree-id recycling: `program.cpp:235` pushes onto `free_snode_tree_ids_`,
`:559-567` pops. `SNode::counter` only increments (`snode.cpp:220`), reset only
at `program.cpp:144`. So the 512-tree ceiling does not bound this.

`TI_ASSERT` at `common/logging.h:100-107` expands to `TI_ERROR` with no
`NDEBUG` guard. The per-tree check is in release builds. It is always on and
measures the wrong thing.

## 26. `bitmasked_activation`. My original citation was right.

The two adversaries flatly contradict each other here, so I printed
`spirv_codegen.cpp:380-436` numbered.

`ir_->u32_type()` appears at `:405`, `:411`, `:412`, `:414`, then `:417`,
`:421`, `:422`, `:427`, `:428`, `:430`, `:431`, `:433` in the op branches.
`:404` is `ir_->make_value(spv::OpShiftLeftLogical, ptr_dt, bitmask_word_index,`
— the statement start, no `u32_type()` on it. `:410` and `:413` likewise.

So adversary 02-1's correction of my `:405, 411-412, 414` to `:404, 410, 411,
413` is wrong, and adversary 02-2 is right to say so. No change made to that
citation. Recorded in report section 7.1.

While there: `:409` is `bitmask_word_ptr = ir_->add(parent_ptr,
bitmask_word_ptr);`, so the Base of the `:410-412` `OpShiftRightLogical`
carries `parent_ptr`'s width while the result type is forced `u32`. Under a
widened pointer that is a type mismatch on the instruction, not just a
narrowing. Adversary 02-2's point, and the code bears it out.

## 27. `use_64bit_pointers`. Followed it, which I did not do in round one.

`make_pointer` is `:2317-2324`. The 64-bit branch at `:2320` is
`ir_->uint_immediate_number(ir_->u64_type(), offset)` — the same bare accessor
as note 19. So on a device without Int64 it yields id 0.

`at_buffer` is `:2194-2220`. The branch at `:2197` is
`if (ptr_val.stype.dt == PrimitiveType::u64)` — on the SType's `dt`, **not** on
`has_buffer_ptr`. `declare_primitive_type` sets `t.dt = dt` at
`spirv_ir_builder.cpp:1536`.

That defeats a claim in my own round-one report: I listed `:2197-2205` among
the branches made unreachable by the dead physical-storage-buffer switch. It is
not gated by that switch. Grepped the capability: real gates are
`spirv_codegen.cpp:783`, `:2340`, `:2416`, `:2491`, and
`spirv_ir_builder.cpp:73` (the `OpCapability`) and `:113` (the extension and
the `AddressingModelPhysicalStorageBuffer64` memory model at `:120`). Corrected
in report section 4.6.

Consequence, which is adversary 02-2's and which I now agree with: flipping
`use_64bit_pointers` alone routes every SNode access into the
`OpConvertUToPtr` branch while the header declares no
`PhysicalStorageBufferAddresses` capability. Invalid module, not a wider one.

## 28. Re-verification sweep of what survived

Everything in report sections 2, 3, 5 and 6 re-opened at the named line.
Findings, all corrections applied to the report:

Right in round one, confirmed: `spirv_codegen.cpp:82`; `make_pointer`
`:2317-2324` and its comment at `:2319`; `make_pointer` callers `:355`, `:371`,
`:408`, `:506`; `LinearizeStmt` LLVM `:1736-1744` with accumulator `:1737` and
strides `:1740`; `create_bit_ptr` assertion `:1755` and struct comment
`:1750-1753`; `loop_index_ty` `:2157`; `LoopIndexStmt` loads `:2336` and
`:2339-2340`; range-for loop var `:1169`, `:1171`, comparisons `:1188`, `:1192`;
`get_constant` dispatch `llvm_context.cpp:738-740` i32, `:741-745` i64;
`AxisExtractor` fields `snode.h:41,45,49`; `list_element_size`
`codegen_llvm.cpp:2290-2291`; `codegen_llvm.cpp:278`, `:279`, `:280-281`,
`:2308-2311`, `:2689-2692`; `struct_llvm.cpp:142-143`; `PhysicalCoordinates`
`runtime.cpp:288-290`; `SNodeDescriptorsMap` `snode_struct_compiler.h:38` and
the `size_t` fields at `:16,19,28,31,42`; `InstrBuilder` single scalar overload
`spirv_ir_builder.h:158`; `buffer_argument` `spirv_ir_builder.cpp:747`;
`check_func_call_signature` `llvm_codegen_utils.cpp:103`, `.h:110`, `:212`,
`codegen_llvm.cpp:1715`; SPIR-V `kernel_compiler.cpp:37`; `"ii"` registration
`spirv_codegen.cpp:2094`, `:2171` and `LoopIndexStmt` `:543-563` with
`TI_NOT_IMPLEMENTED` at `:553`; `demote_dense_struct_fors` forced at
`compile_config.cpp:72-74`, pass at `compile_to_offloads.cpp:191-196`;
`spirv_codegen.cpp:1999` and `kernel_utils.h:99,110-111`;
`DecorationNoSignedWrap` block; `SNodeLookupStmt` upcast `:504-508`.

Wrong in round one, corrected:
- `struct_llvm.cpp:152` refine third param → `:153`.
- `struct_llvm.cpp:177` UDiv/URem → `:179`.
- `struct_llvm.cpp:145-188, 199-200` → `:146-190`, `:199-201`.
- `struct_llvm.cpp:204-232` get_ch → `:203-233`.
- `struct_llvm.cpp:253`, `:255-259` (assertion predecessors) → `:258`,
  `:260-264`.
- `codegen_llvm.cpp:2385` GlobalTemporary i64 → `:2386`.
- `codegen_llvm.cpp:2063-2087` get_range_for_bounds → `:2065-2087`.
- `codegen_llvm.cpp:2404-2405` BlockLocalPtr GEP → `:2405-2407`.
- `codegen_llvm.cpp:1864-1872` → `:1864-1871`; `:1960-1972` → `:1960-1971`;
  `:2394-2395` → `:2393-2396`.
- `runtime.cpp:307` StructMeta::max_num_elements → `:310`.
- `constants.h:27` listgen constant → `:28`.
- `runtime.cpp:562-563` as the definition of `kMaxNumSnodeTreesLlvm` → defined
  at `constants.h:13`; those runtime lines are the arrays sized by it.
- `spirv_codegen.cpp:772-776` shift → `:773-777`; `:777-781` decoration →
  `:778-781`.
- `spirv_codegen.cpp:531-541` LinearizeStmt → function opens `:532`.
- `spirv_types.cpp:486-495` translate_ti_type → `:484-497` for the pointer
  branch, function `:484-514`.
- `spirv_ir_builder.cpp:583-589` array-stride narrowing → the narrowing is
  `:591`, length emission `:576`, decoration `:605`.
- `spirv_ir_builder.cpp:308-309` unguarded i32 → `:309-310`.
- `spirv_ir_builder.cpp:955-996` builtins → `get_local_invocation_id`
  `:963-979`, `get_global_invocation_id` `:981-997`, num-work-groups load
  `:960`.
- `vulkan_device_creator.cpp:823` shaderInt64 for PSB → `:821`; `:820-828` →
  `#if` `:825`, setter `:826`, `#endif` `:827`.
- `llvm/kernel_compiler.cpp:29-48`, caps at `:31` → `:30-48`, `:32`.
- `codegen.cpp:36-72` → `:37-74`.
- `compiled_kernel_data.h:30` → `:26-30`; `.cpp:82-93` → `:82-94`.

## 29. A claim of mine that the source defeats

Round one section 3.1 called `max_num_elements` "the only piece of SNode
geometry already 64-bit on both sides". Opened `runtime.cpp:307-325`: the field
is `i64` at `:310`, but the function-pointer field is
`i32 (*get_num_elements)(Ptr, Ptr)` at `:318`, and
`node_dense.h:10-12` is `i32 Dense_get_num_elements(...) { return
((StructMeta *)meta)->max_num_elements; }`. The accessor narrows it back.
Adversary 02-2 found this; adversary 02-1 credited pass A with it. Both right.
Claim withdrawn in the report, and the sentence rewritten to say what the
LLVM-side truncations actually are.

## 30. The stride producers, outside my territory

Adversary 02-2's point, checked: `transforms/scalar_pointer_lowerer.cpp:33` is
`std::array<int, taichi_max_num_indices> total_shape;` and
`transforms/demote_dense_struct_fors.cpp:19` is the same declaration.
`LinearizeStmt::strides` is `std::vector<int>` at `ir/statements.h:1280`. So
the site I called "the one that must change first" has two `int` producers
upstream in territory 01. Recorded in report section 6.2 as a seam, not
claimed as mine to change.

## 31. What I did not do

- Touched no source file. Territory 02 is an investigation, not an edit.
- Touched no other agent's file. Report 02A, the adversary files and the
  project plan are unchanged.
- Did not decide anything is unnecessary, did not propose removing
  `use_64bit_pointers`, the `&& false` block, `Extension::data64`, or the C API
  override. All four are in escalations, unresolved.
- Did not resolve which quantity the SNode ceiling should bound, what closes
  the tinyir capability hole, or whether DX11 is in scope. Escalations 1, 10
  and 13.
- Did not attempt to build a failing case for the SNode overflow. The chain is
  read, each link, and the reachability argument is stated as what it is.

---

# Revision 3 — amendment pass, 2026-09-09

Round two adversary analyses `adversary2-02-1.md` and `adversary2-02-2.md` read
in full before any source was opened. Eight claims were put to me. Each was
treated as a claim, not a finding, and opened at the line. Numbered entries
below record the adjudication of each.

## 32. How the false method claim at report `:16-17` came to be made

This is the entry the planner asked for specifically, and it is the one I would
rather not have to write.

Revision 2's report said: "Every **[V]** in this revision was re-opened at the
named line during this pass, not carried forward on trust." Adversary2 02-2 §4.2
says that is false and names four rows; adversary2 02-1 §4.2 names six. I
checked and **both are right, and the true number is larger than either gave.**

The mechanism, traced through my own notes:

- Note §28 is the record of what revision 2 actually re-opened. It is written as
  two lists — "right in round one, confirmed" and "wrong in round one,
  corrected" — and it is a list of the citations **an adversary had disputed or
  that I had reason to doubt**. It was never an enumeration of the section 3
  tables.
- The rows the round-one adversaries did not challenge were not in either list
  and were not opened. `codegen_llvm.cpp:2272`, `:2213-2216`, `:1841-1843`,
  `:1823-1825`, `spirv_codegen.cpp:2146`, `:2095`, `:2097`, `:1821`,
  `:2064-2070` and `runtime.cpp:1335` are all in that category.
- The report sentence was then written from note §28 as though §28 covered the
  tables. It generalised a targeted sweep into a universal claim. Nothing in
  §28 supports the word "every".

**The failure is standing instruction §10.6 applied to my own method statement.**
I had verified a set of citations. I wrote a sentence quantifying over all of
them. The verification certified the citations I opened and could not certify a
statement about the ones I did not. It is the same shape as the two substantive
faults this pass corrected: the §5.3 sparsity conclusion (note §34) and the
`:2464`/`:2511` attribution (note §33). Three instances in one document.

**What revision 3 did instead of repeating the claim.** I extracted every
`file:line` citation from the report with a script, resolved each to a real path,
printed the named lines from source, and read each extract against the sentence
it supports. That is 278 distinct file-qualified citations. The bare `:NNNN`
references, which name a line without repeating a filename, are not machine
resolvable from the text alone, so I resolved them to their governing file from
the surrounding prose and opened them by hand — `spirv_codegen.cpp:2267`,
`:2276`, `:2293`, `:728`, `:753`, `:788`, `:2340`, `:2343-2367`, `:2416`,
`:2491`, `:543-563`; `spirv_ir_builder.cpp:113-122`, `:576`, `:581-583`,
`:596-602`, `:960`; `spirv_types.cpp:17`, `:47`, `:71`, `:336`, `:346`, `:356`,
`:365`, `:421-431`, `:430`, `:440-452`, `:167-214`, `:199-201`;
`struct_llvm.cpp:247`, `:258`, `:260-264`; `metal_device.mm:1025`, `:1035-1038`,
`:1051`; `vulkan_device_creator.cpp:630`; `opengl_device.cpp:510`;
`taichi_opengl_impl.cpp:26`, `:28`; `program.cpp:559-567`;
`llvm_runtime_executor.cpp:442-444`; `llvm_codegen_utils.h:110`, `:212`. The
report now states what was done rather than a universal, and the twelve faults
found are in report section 10 with attribution for each.

The claim was struck rather than made true by assertion. It is made true by the
sweep, and the sweep is described so it can be repeated.

## 33. Claim 1 — the `:2464` / `:2511` swap. Upheld against me.

Opened `spirv_codegen.cpp:2320-2530` and extracted every function opening,
closing brace and `ir_translate_to_spirv` call:

```
2326:   void compile_args_struct() {
2386:         ir_translate_to_spirv(...)
2387:     args_struct_type_.id = ir2spirv_map[struct_type];
2400:   }
2402:   spirv::Value compile_argpack_struct(const std::vector<int> &arg_id,
2464:         ir_translate_to_spirv(...)
2465:     argpack_struct_type.id = ir2spirv_map[struct_type];
2481:   }
2483:   void compile_ret_struct() {
2511:         ir_translate_to_spirv(...)
2512:     ret_struct_type_.id = ir2spirv_map[struct_type];
2527:   }
```

`:2464` is inside `compile_argpack_struct`; `:2511` is inside
`compile_ret_struct`. My revision 2 had them swapped **while printing the spans
`:2483-...` and `:2402-...` in the same table cells as the labels**. Adversary2
02-1 §3.1 is right that the report is self-refuting on its face, and that is the
part that stings: no new source reading was required to catch it, only reading
two of my own verified citations against each other. Standing instruction §10.7
asks for numbers to be reconcilable against the list they summarise; the same
discipline applied to spans and labels would have caught this in revision 2.

Also checked the consequence they draw. The five `translate_ti_type` call sites
are `:2344`, `:2358`, `:2420`, `:2436`, `:2494` — confirmed by grep — and they
partition as args `:2344, :2358`, argpack `:2420, :2436`, ret `:2494`, which is
the reverse of what escalation 10 implied. Report section 4.4 corrected.

## 34. Claim 3 — the `all_dense` seeding. Upheld against me.

Opened `llvm_runtime_executor.cpp:395-415`. Line `:402` is
`bool all_dense = config_.demote_dense_struct_fors;`. The loop `:403-410`
contains one assignment, `all_dense = false;` at `:407`, inside an `if` that
tests for a non-dense/place/root node, followed by `break`. It can only clear.

`runtime.cpp:1000-1002` is the early return; `:1003-1007` is the range store.
`demote_dense_struct_fors` is `bool` at `compile_config.h:28`, set true at
`compile_config.cpp:18`, forced true for SPIR-V archs at `:72-74` — which does
not reach this LLVM-only runtime — and writable from Python at
`export_lang.cpp:201-202`.

So with the field false, `all_dense` is false regardless of tree shape and the
store fires for every tree. **My revision 2 conclusion, "both surviving routes
therefore require sparse SNodes", is false and is withdrawn.**

How it happened is worth recording alongside note §32 because it is the same
mechanism. I added the seeding to §5.3 as a *citation correction* — my §7.6
shows me correcting adversary 02-2's `:400-409` to `:402-410` and noting the
seed as a thing neither adversary had mentioned. Having verified that `:402` says
what I quoted, I left the conclusion two sentences below untouched. The citation
certified the citation. The sentence next to it was a claim about which
configurations reach the store, and no citation could test that.

Adversary2 02-2 §5.3 additionally holds that `report-02-codegen.md` states the
seeding correctly and then draws the same overstated conclusion, so adversary2
02-1 §6 item 6 is wrong to ask the correction of this report alone. I opened
the lines it cites in that report and the reading is right. That report is not
mine to amend; recorded as an adjudication only.

## 35. Claim 4 — the second widening-invalidity site. Upheld, and it is mine to have missed.

`spirv_codegen.cpp:398-399`:

```
398:      auto bitmask_mask = ir_->make_value(spv::OpShiftLeftLogical, ptr_dt,
399:                                          ir_->const_i32_one_, bitmask_bit_index);
```

`make_value(spv::Op op, const SType &out_type, Args &&...args)` is
`spirv_ir_builder.h:290-291`, so argument two is Result Type and argument three
is the first operand. Result Type `ptr_dt`, Base `const_i32_one_`.
`const_i32_one_ = int_immediate_number(t_int32_, 1)` at
`spirv_ir_builder.cpp:224`. Permanently 32-bit. Today `ptr_dt` is u32 so the
widths agree; widen `make_pointer` and Result Type is 64-bit over a 32-bit Base.

This is inside `bitmasked_activation`, a function I analysed in revision 1,
defended a citation of in §7.1, and re-opened in revision 2. I had the right
line numbers for the `u32_type()` calls three times running and never asked what
the `ptr_dt`-typed instructions above them do under widening. Adversary2 02-1
§3.2 found it; adversary2 02-2 §10.2 confirms it and records that it missed it
too. Report section 6 now carries both directions.

I accepted one further refinement from adversary2 02-2 §3.1 after checking
`:404-414`: `:405` is the Shift operand of a `ptr_dt`-typed
`OpShiftLeftLogical`, and SPIR-V does not constrain the Shift operand's width to
Base's, so `:405` is benign where `:411`, `:412` and `:414` are not. Report
sections 3.2 and 6 now say which is which instead of listing four
undifferentiated.

## 36. Claim 5 — the sweep list. Upheld. Report section 9 is new.

`find taichi/codegen -type f` gives 51; restricted to `.h`/`.cpp` it gives 45;
the difference is six `CMakeLists.txt`. I then checked each of the 45 by name
against the report body and found 21 cited with line numbers and 24 not
mentioned at all. Adversary2 02-2 §6.3 item 3 gave 24 named; my count of the
body is 21, and the discrepancy is that basename matching credits the report
with `llvm/compiled_kernel_data.*` and `spirv/compiled_kernel_data.*` when the
only citation is the top-level `taichi/codegen/compiled_kernel_data.{h,cpp}`.
Either way their point stands: the report claimed 51 files and evidenced fewer
than half.

I opened all 24 rather than transcribing note §10, which covered fewer and
covered them loosely. Three of the 24 produced content worth carrying:

- `spirv/spirv_types.h:71-90` — `PhysicalPointerType` derives from
  `IntType(64, unsigned)` at `:75` with **no capability gate**. An ungated
  64-bit type in the SPIR-V type layer, directly on 6.2.
- `spirv/snode_struct_compiler.cpp:34-...` — `construct` builds it at `:53`, and
  the only call to `construct` is at `:18`, inside a `/* */` opened `:16` closed
  `:19`. Dead in the current tree.
- `spirv/lib_tiny_ir.h` — the whole layout interface is `size_t` (`:127-130`,
  `:134`, `:139`, `:151`, `:157`, `:165`, `:173`, `:190`, `:191`, `:196`,
  `:209`). A positive result: the tinyir layout computation is 64-bit clean and
  the narrowing is downstream, at the 32-bit SPIR-V literal operands.

Also carried, because adversary2 02-2 §6.3 item 5 asked for it: 55 lines of
`llvm/codegen_llvm_quant.cpp` carry i32 constants and every one is bit
manipulation inside quantised encode/decode, with the only index-side contact
through `load_bit_ptr` at `:23`, `:34`, `:74`. Not a scaling limit — but that is
now in the report where a reader can check it, not only in notes §10.

One thing I found while sweeping that neither adversary raised:
`spirv/kernel_utils.cpp:87-95` pushes a single `RetAttributes` with
`ra.dtype = PrimitiveTypeID::i32` at `:93`, its own comment describing it as a
placeholder retained only so `GfxRuntime::device_to_host::require_sync` works,
and marked redundant pending removal. Recorded in section 9.2. Not mine to
change and I am not proposing removal.

## 37. Claim 6 — the surviving citation faults. Upheld, and the count is twelve, not six.

Adversary2 02-1 §4.2 listed six in its table plus two further span
inconsistencies; adversary2 02-2 §4.2 listed four in its table plus two spans.
I opened all of theirs, found every one real, and found two more in the full
sweep. Counted from the ledger in report section 10 row by row: **twelve** —
rows 1-4 in §3.1, rows 5-10 in §3.2 with row 10 also in §4.6, row 11 in §5.1,
row 12 in §6. By finder: adversary2 02-1 eight, adversary2 02-2 six, overlap
four, this pass alone two.

The two neither adversary had:

- `codegen_llvm.cpp:1823-1825` for the quant_array offset. The
  `get_constant(element_num_bits)` is at `:1822`, the multiply at `:1823`,
  `create_bit_ptr` at `:1824`; `:1825` is `} else {`.
- `spirv_codegen.cpp:2064-2070` for `total_invocs`. `:2064` is blank and the
  statement runs `:2066-2071`.

Adversary2 02-1's characterisation of row 11 is worth repeating against myself:
`runtime.cpp:1335` is `int num_parent_elements = parent_list->size();` and not a
subscript, and this is the exact fault `report-02-codegen.md` found and
corrected in itself. It was flagged in a document I read and I did not take it.

The three non-citation defects — the `:2464`/`:2511` swap, the `data64`
quantifier, and `SNode::reset_counter()` presented as live — are recorded
separately in report section 10 because none of them is a wrong line number and
lumping them in would misstate what a citation audit found.

## 38. Claim 7 — the severity chain. Adversary2 02-2 is right; adversary2 02-1 is not.

This is the divergence the planner said had been reported twice and been wrong
twice, so I traced every link rather than choosing between two accounts.

1. `spirv_codegen.cpp:2710` — `set_run_validator(false)`, unconditional, outside
   the `if (params.enable_spv_opt)` block that opens `:2683` and closes `:2709`.
2. `:2746-2747` — `spirv_opt_->Run(...)` on every kernel; only the pass
   registrations `:2685-2708` are gated.
3. `external/SPIRV-Tools/source/opt/optimizer.cpp:584-598` — the `tools.Validate`
   call is skipped when `run_validator_` is false at `:590-594`; `BuildModule` is
   then called **unconditionally** at `:596-597` and `Run` returns false if it
   yields null at `:598`.
4. `external/SPIRV-Tools/source/opt/build_module.cpp:56-75` — `spvBinaryParse` at
   `:68-69`, null unless `SPV_SUCCESS` at `:74`.
5. `external/SPIRV-Tools/source/binary.cpp:450` "Error: Type Id is 0", `:456`
   "Error: Result Id is 0", `:473` "Id is 0" for `SPV_OPERAND_TYPE_ID` — which is
   what an `OpTypeStruct` member operand is.
6. `spirv_codegen.cpp:2682` installs `spriv_message_consumer` (`:2641-2659`).
   `:2646` tests `level <= SPV_MSG_FATAL`. In the enum at
   `external/SPIRV-Tools/include/spirv-tools/libspirv.h:83-95` the order is
   FATAL 0, INTERNAL_ERROR 1, ERROR 2, WARNING 3, INFO 4, DEBUG 5. So a parse
   error at level 2 fails the `<= 0` test and lands in the `TI_WARN` branch at
   `:2649`. The **branch order**, not the severity, is what downgrades it.
7. `:2745-2748` `TI_WARN_IF(..., "SPIRV optimization failed")`, `:2750`
   `success = false`, `:2760` the only read, inside `if constexpr (false)` at
   `:2758`, `:2775` pushes the module regardless. On a failed `Run` the vector
   still holds the copy made at `:2740`.

**Adjudication.** Adversary2 02-2 §2.1 and §10.3 describe exactly this and the
source supports them at every link. Adversary2 02-1 §2.1 point 3 ("no
Taichi-level diagnostic") is false — there are two `TI_WARN`s — and point 4
("deferred to the driver at shader-module creation") is false as a statement
about where the failure is first detected. Its §6 item 3 asks both reports to
add that sentence; adding it would put a false statement in both.

**What adversary2 02-1 is right about, and I record it because it is load
bearing.** A zero `<id>` operand *is* a hard parse failure for any conformant
consumer — the SPIRV-Tools parser is one, which is the only reason the chain
fires at all — and nothing in this tree says what a driver does with the module
that ships anyway at `:2775`. Its §2.1 finding that a tree-wide grep shows no
SPIR-V validation anywhere, not merely none in codegen, I re-ran and confirm.

**What it decides for item 6.2, since the planner asked.** Two work items, not
one, and neither is mine to choose. A diagnostic at the type boundary
(`spirv_types.cpp:397-428`), because there is none at the point where the type
and the capability are both still known. And separately the dead `success` flag
at `:2742-2775`, which is a defect whether or not the type boundary is closed.
Escalations 15 and 16.

The word "silently" is out of the report.

## 39. Claim 8 — Metal and Float64, and the other four capabilities. Upheld.

Grepped `taichi/` and `c_api/` for `caps.set(...)` of each of the five optional
scalar-type capabilities enumerated at `taichi/inc/rhi_constants.inc.h:12-16`.
Seventeen setters. The table is in report section 4.5.1 and the count is derived
from its rows: int8 2, int16 4, int64 4, float16 4, float64 3.

`taichi/rhi/metal/metal_device.mm` contains **no occurrence of the string
`float64`**. It sets int8, int16, float16 unconditionally at `:1046-1048` and
int64 under `feature_64_bit_integer_math` at `:1051-1052`. `t_fp64_` is declared
only inside `if (caps_->get(cap::spirv_has_float64))` at
`spirv_ir_builder.cpp:174-176`, so it keeps `SType::id{0}`
(`spirv_ir_builder.h:51`). With the bare `f64_type()` at `spirv_types.cpp:428`,
an `f64` kernel argument on any Metal device emits an `OpTypeStruct` member
operand of 0. Unconditional, on current hardware. Adversary2 02-2 §6.1 is right
and this is a stronger instance than the Int64 one both reports lead with.

Three further consequences of the same table, all opened:

- DX11 (`dx_device.cpp:563-565`) and imported Vulkan
  (`taichi_vulkan_impl.cpp:19-56`, `caps{}` `:35`, `spirv_version` only
  `:37-43`, `set_caps` `:53`, PSB set commented out `:46-51`) lack **all five**.
  My revision 2 said DX11 "sets no integer capability at all", which is narrower
  than the truth, and had no imported-Vulkan row at all.
- The C API OpenGL override at `taichi_opengl_impl.cpp:8-12` *removes* as well
  as adds, because `set_caps` (`public_device.h:855-857`) is a whole-object
  move-assign, so it discards the int16/float16 the probes at
  `opengl_device.cpp:515-526` may have granted. I had the addition and not the
  removal.
- `ti_set_runtime_capabilities_ext` (`taichi_core_impl.cpp:317-334`) lets any
  embedder install any capability set with no device query and no enum
  validation. Absent from my report entirely; both adversaries flagged it
  independently. It is the general case of which my escalation 12 was one
  instance.

Whether Metal genuinely cannot do f64 is not determinable here, and I am not
deciding it. Escalation 17.

While checking this I also confirmed adversary2 02-1 §2.4 and adversary2 02-2
§2.4 against me: `SNode::reset_counter()` (`snode.h:348-350`) has **no caller**.
Grep over `taichi/`, `c_api/`, `tests/`, `python/` returns the definition, the
unrelated `Stmt::reset_counter` at `ir.h:509`, and the single call
`Stmt::reset_counter();` at `program.cpp:347`. My §5.1 presented two live reset
avenues where there is one, which matters for escalation 1. Corrected.

## 40. What I upheld against the adversaries

Two things, and nothing changed in the report for either.

- **Section 7.1, the `bitmasked_activation` line numbers.** Both round-two
  adversaries confirm the round-one adjudication: `:405`, `:411`, `:412`, `:414`
  are the `u32_type()` calls and adversary 02-1's round-one `:404`, `:410`,
  `:413` were statement starts. Adversary2 02-2 §3.1 re-derived it by extracting
  every `u32_type()` line in `:383-435`. Stands.
- **The count of five, refused against the planner's figure of six.** Both
  adversaries now confirm six has no reading in the source. Adversary2 02-1 §2.2
  calls it its own arithmetic slip; adversary2 02-2 §2.2 identifies six as the
  count of the *integer* accessors alone. Adversary2 02-1 §2.2 further holds
  that my wording — "eight optional types, five capability flags" — is the
  precise one and the paired report's "five distinct optional bit-widths" is
  loose, because the distinct widths are three. Unchanged, per the planner's
  instruction and because the source says so.

## 41. What I did not do in revision 3

- Touched no source file. No file belonging to any other agent. Not the project
  plan, not `report-02-codegen.md`, not either adversary file.
- Did not decide anything is unnecessary. Did not propose removing
  `use_64bit_pointers`, the `&& false` block, `Extension::data64`, the two C API
  capability overrides, the disabled validator, the dead `success` flag, the
  dead `SNode::reset_counter()`, the dead `PhysicalPointerType` construction, or
  the `kernel_utils.cpp` i32 ret placeholder. **Ten items.** Seven of them carry
  an escalation of their own — 3, 2, 11, 12, 18, 15, 16 in that order — and the
  remaining three do not: `SNode::reset_counter()` is a fact inside escalation
  1, and `PhysicalPointerType` and the `kernel_utils.cpp` placeholder are
  recorded in report section 9.2 as sweep findings with no decision attached.
- Did not resolve which quantity the SNode ceiling should bound, whether
  `demote_dense_struct_fors` will ever be false in this project, whether Metal's
  missing Float64 is permanent, or whether DX11 and imported Vulkan are in
  scope. Escalations 1, 19, 17, 13 and 20.
- Did not build or run anything. The SPIRV-Tools chain in note §38 is read, link
  by link, from a checked-out submodule. I have not observed a driver reject an
  id-0 module and I do not claim to have.

---

# Revision 4, 2026-09-09. Round three close-out.

Plan re-read in full at its **2026-09-09** stamp before starting and again
before finalising, per standing instruction §10.6. Section 10's priority
statement is what §42 below turns on.

## 42. Claim 1 — the widening enumeration. Upheld against me, and it is the worst fault this report has carried.

The claim: of the four `u32_type()` hardcodes in `bitmasked_activation`, only
`:411` breaks under a widened pointer, not `:411`, `:412` and `:414` as the
report said at `:385`, `:1099` and `:1330`.

I did not take `adversary3-02-2.md` §7 on its word and I deliberately did not
reason from the SPIR-V specification, because reasoning from the specification
is what produced the error. The brief said to check the validator in this tree.
I printed it.

`external/SPIRV-Tools/source/val/validate_bitwise.cpp`, shift case `:65-104`:

```
 74:      const uint32_t base_type = _.GetOperandTypeId(inst, 2);
 75:      const uint32_t shift_type = _.GetOperandTypeId(inst, 3);
 88:      if (_.GetBitWidth(base_type) != _.GetBitWidth(result_type))
 90:               << "Expected Base to have the same bit width "
 93:      if (!shift_type ||
 94:          (!_.IsIntScalarType(shift_type) && !_.IsIntVectorType(shift_type)))
 99:      if (_.GetDimension(shift_type) != result_dimension)
101:               << "Expected Shift to have the same dimension "
```

Base: type, dimension, **width**. Shift: type, dimension, **and nothing else**.
The file contains no width test on the Shift operand. So `:412` — which is
operand index 3 of the instruction whose Result Type is at `:411` — is exempt
by exactly the rule the report already applied to `:405`. The report granted the
exemption to one Shift operand and withheld it from another **in the same
sentence**, having verified both citations.

`:414` is worse, because acting on it would have been a regression rather than a
no-op. `struct_array_access(res_type, buffer, index)` at
`taichi/codegen/spirv/spirv_ir_builder.cpp:770-790`:

```
774:  TI_ASSERT(res_type.flag == TypeKind::kPrimitive);
783:  SType ptr_type = this->get_pointer_type(res_type, storage_class);
785:  ib_.begin(spv::OpAccessChain)
786:      .add_seq(ptr_type, ret, buffer, const_i32_zero_, index)
```

`res_type` is the **pointee** type, not an index type. The buffer it indexes is
requested at `spirv_codegen.cpp:401-402` as
`get_buffer_value(BufferInfo(BufferType::Root, root_id), PrimitiveType::u32)` —
32-bit bitmask words, which is what the whole function manipulates. The index is
`bitmask_word_ptr`, and the access-chain index rule is
`external/SPIRV-Tools/source/val/validate_memory.cpp:1342-1348`:

```
1342:    // The index must be a scalar integer type (See OpAccessChain in the Spec.)
1344:    if (!index_type || spv::Op::OpTypeInt != index_type->opcode()) {
1346:             << "Indexes passed to " << instr_name
1347:             << " must be of type integer.";
```

Integer type only. No width. So `:414` is correct under any pointer width and
widening it would retype the buffer's elements.

**How this survived three adversarial rounds, which is the part worth
recording.** Every round asked the same question — *which lines does this
citation name?* — and every round answered it correctly. §7.1 in revision 2,
§7.12 in revision 3, both round-one and both round-two adversaries. Nobody asked
*what rule decides whether a named line breaks*, and the file that answers it,
`validate_bitwise.cpp`, had not been opened by anyone. That is the plan's
section 10 priority statement almost verbatim: one file nobody has opened
against a second pass over one everybody has. It is also §10.7 for the third
time in this report, after §7.11 and §7.12.

## 43. Claim 2 — the second mechanism. Upheld, and I verified the mechanism rather than the two lines.

The two sites are `spirv_codegen.cpp:392-394` (`OpShiftRightLogical`, Base
`input_index`) and `:395-397` (`OpBitwiseAnd`, operand `input_index`). Both
Result Types are `ptr_dt` = `parent_ptr.stype` (`:388`); both immediates are
built at `ptr_dt`. The one operand that does not follow the pointer is
`input_index`, and `input_index` is a **parameter** at `:387`. So this is not a
hardcode at all — whether it breaks is decided outside the function.

`OpBitwiseAnd`'s rule is `validate_bitwise.cpp:106-141`, and unlike the shift
case it tests **every** operand:

```
118:      for (size_t operand_index = 2; operand_index < inst->operands().size();
134:        if (_.GetBitWidth(type_id) != result_bit_width)
136:                 << "Expected operands to have the same bit width "
```

I then opened both callers in full rather than the two lines cited, because a
claim about a difference between call paths is not testable from a citation:

- `visit(SNodeOpStmt*)`, `:437-466`. Casts at `:443-444`:
  `ir_->cast(parent_val.stype, ir_->query_value(stmt->val->raw_name()))`, then
  calls at `:448-449`, `:455-456`, `:458-459`. Three calls, all with a
  pointer-typed index.
- `visit(SNodeLookupStmt*)`, `:468-511`. Reads at `:488-489`:
  `ir_->query_value(stmt->input_index->raw_name())` — no cast — and calls at
  `:490-491`. Then at `:504-505`, for the dense-offset branch, it casts **the
  same expression**: `ir_->cast(parent_val.stype, ir_->query_value(stmt->input_index->raw_name()))`.

Sixteen lines apart, same value, one cast and one not.
`adversary3-02-2.md` §8.2 writes "eighteen lines below"; it is sixteen, 504 − 488.
The finding is unaffected and I record the arithmetic only because §10.9 asks
for counts to be derivable.

So: four broken instructions on the `SNodeLookupStmt` path (`:392-394`,
`:395-397`, `:398-399`, `:410-412`), two on the `SNodeOpStmt` path (`:398-399`,
`:410-412`). Graded ARCHITECTURAL under §10.8 — the mixing of a pointer-width
Result Type with 32-bit operands is a representation decision inside this
codebase and no external change retires it — with a blast radius of five places
in one file, which is small. Kind and radius stated separately, as §10.8
requires.

**One consequence I checked because it changes the severity and I already had
both halves in the report.** These are all **validator** faults, and section 4.4
of the report establishes that Taichi disables the validator unconditionally at
`spirv_codegen.cpp:2710` and that the only thing reading a produced module
in-process is the SPIRV-Tools binary **parser**, which catches structural faults
such as a zero `<id>`. So unlike the id-0 case, which yields two `TI_WARN`s,
these would ship with no Taichi-level diagnostic at all.
`adversary3-02-1.md` §2 reaches the same point from report A's §4.2.10 and
prints the same validator lines. I did not treat this as a new line of
investigation: both facts were already in the report and this is the join.

## 44. Claim 3 — the two surviving withdrawn spans. Upheld.

`report-02b-codegen.md:121` and `:244-245` carried `spirv_ir_builder.cpp:334-341`
and `spirv_types.cpp:484-497`, both of which section 10's ledger rows 9 and 10
had already withdrawn in revision 3 in favour of `:334-353` and `:484-514`. The
second sits inside a bullet list closed at `:248` with "All **[V]**", so a
blanket verification mark was standing over two citations the same document
declares wrong. `adversary3-02-1.md` §8.3 is right. Both corrected.

The ledger was honest about *where* it applied each fix — row 10 says "§3.2 and
§4.6" — which is precisely the failure: the correction landed where the
adversary raised it and nowhere else.

## 45. The check the brief asked for, and what it turned up

The brief's general fault: a correction that lands where it was raised and not
where it was repeated. I grepped this report for every fact I corrected this
round.

- `411` / `412` / `414` / `405`. Every occurrence in the report was opened and
  sorted into three sets, and the sets are the enumeration the count comes
  from. **Three** carried the faulty invalidity claim — §3.2's table row, §6's
  bullet, §7.12's closing paragraph — and all three are corrected. **Three** are
  inventories of where `u32_type()` is hardcoded — §2's bullet, §7.8's list,
  §7.12's opening — which is true, is upheld by all four adversaries, and is
  left standing; §7.8 and §2 gain pointers to the narrower reading, §7.12's
  opening needs none because its own closing paragraph now carries the
  correction. The remainder are other files with coincidental digits:
  `spirv_types.cpp:411` in §4.3's accessor table, `codegen_llvm.cpp:2405-2407`
  in §3.1, and `:319-322`-style rows in §4.2.
- "two directions" / "two sites". Four places: §0 item 11, §1's closing
  paragraph, §6's bullet, §7.12. All four updated.
- `334-341` / `484-497`. **Eight occurrences** in revision 3, five of the first
  span and three of the second, counted by grep and each opened. **Three of the
  eight were live faults**: `:121` in §1's executive summary point 2, and
  `:244` and `:245` in §2's touchpoint list, which carried one of each span.
  Two places, three citations, all corrected. **The other five quote the
  superseded form deliberately** as the "Was" half of a correction — §3.2's two
  rows, §4.6's opening parenthesis, and §10's ledger rows 9 and 10 — and must
  keep it to stay legible. 3 + 5 = 8.
- **And one the grep would not have found, which is the reason to do this by
  hand as well.** §3.2's "Not 32-bit-limited in the SPIR-V path" list says
  `SNodeLookupStmt` casts the index up to the pointer type and that a widened
  `make_pointer` would need "no further change". That sentence names `:504-508`
  and contains none of the corrected tokens, but the finding in §43 falsifies
  it as a statement about that visitor: the visitor's other branch is the
  uncast one. Qualified in place rather than deleted, per §10.9 — a scope
  ruling removes work, not evidence.

## 46. Two places where a round-three adversary is wrong, and I changed nothing

- `adversary3-02-2.md` §7 gives the blast radius as "report B's escalation 10
  and its section 6". Escalation 10 of this report is the tinyir capability
  hole at `spirv_types.cpp:397-428` and has nothing to do with
  `bitmasked_activation`. The escalation this material feeds is **3**,
  `use_64bit_pointers`. Recorded at report §7.14; no change.
- `adversary3-02-2.md` §12 item 3 calls the fault "one sentence repeated at four
  places" and then names five, two of which (`:237`, `:1315`) carry no
  invalidity claim. It stood at three. Recorded at report §7.14; nothing
  withdrawn at either of those two.

I also record that the two round-three adversaries **split**:
`adversary3-02-1.md` §10 states nothing substantive is wrong in either report.
On the item that matters it did not test the widening enumeration against the
validator, and it is wrong. Its four residuals are all real, and the one that
lands on this report is §44 above. Its verification of the ledger, the counts,
the census, the sweep partition and the severity chain is independent of 3-2's
and agrees with it, which is what makes the upheld set safe to leave alone.

## 47. The citation figure, and what I did not do

Revision 3's "278 distinct file-qualified citations" is withdrawn. It is not
reproducible. Both round-three adversaries extracted the same three numbers from
revision 3 independently — 311 triples, 298 pairs, 87 files — and I reproduce
all three exactly with my own extraction. Neither is 278, and the report stated
no counting rule, so the figure could not be tested by anyone including me.
Revision 4 states the rule in the header and gives the figure it yields for this
revision: 404 raw occurrences, **322** distinct `(path-as-written, start, end)`
triples, 309 distinct `(path, start)` pairs, 91 distinct paths. I re-ran the
resolution test on this revision: 322 of 322 resolve to a real file at a real
line, zero failures. The sweep itself is not withdrawn — only the number that
described it.

**What I did not do in revision 4.**

- Touched no source file, no other agent's file, and not the plan. Only
  `report-02b-codegen.md` and this notes file.
- Opened no new line of investigation beyond the brief's item 2. The validator
  files were opened to test a claim I was given, not to start a survey of
  SPIRV-Tools.
- Did not re-verify the upheld set — the bitmasked line numbers as originally
  given, the width and type counts, the severity chain, the sparsity withdrawal
  and the config-flag route, the seventeen-setter census, and the 21 + 24
  partition. The brief instructed me not to disturb them and two independent
  adversaries verified each at source this round. Re-deriving them would have
  been a second pass over ground everybody has covered, which is the move
  section 10 of the plan tells agents not to make.
- Proposed no fix. Escalation 21 states the two options for the uncast index —
  typing the parameter, or casting at the call site — says why they are not
  equivalent, and chooses neither.
- Built and ran nothing. The validator behaviour in §42 and §43 is read from the
  checked-out submodule at `external/SPIRV-Tools`. I have not compiled a widened
  pointer path and I do not claim to have observed any of these instructions
  rejected.
